/**
 * @file robot_app.c
 * @brief 2026 H 题第 2、3 问比赛状态机。
 *
 * 第 2 问沿用已经实车跑通的四路循迹和 TB6612 引脚，仅增加 A 点驶离/回线、
 * 主动制动和毫秒计时锁存。第 3 问由 K230 本地完成视觉与舵机闭环，
 * MSPM0 负责按键、总计时、OLED、通信监督和急停。
 */

#include "robot_app.h"

#include "display.h"
#include "hal.h"
#include "k230_protocol.h"
#include "line_sensor.h"
#include "motor.h"
#include "mpu6050.h"
#include "pid.h"
#include "robot_config.h"

#include <string.h>

typedef struct
{
    bool raw_pressed;
    bool pressed;
    bool pressed_event;
    uint32_t changed_ms;
} button_t;

typedef enum
{
    Q2_MARKER_WHITE = 0,
    Q2_MARKER_BLACK
} q2_marker_t;

typedef enum
{
    Q2_BRAKE_FINISH = 0,
    Q2_BRAKE_ABORT,
    Q2_BRAKE_FAULT
} q2_brake_reason_t;

static robot_telemetry_t s_tel;
static pid_t s_line_pid;
static button_t s_key_mode;
static button_t s_key_start;
static button_t s_key_calib;

static uint32_t s_last_tick_ms;
static uint32_t s_task_start_ms;
static uint32_t s_line_lost_since_ms;
static uint32_t s_last_line_seen_ms;
static uint32_t s_marker_since_ms;
static uint32_t s_marker_clear_since_ms;
static uint32_t s_brake_start_ms;
static uint32_t s_q3_last_command_ms;
static uint32_t s_q3_last_keepalive_ms;
static uint32_t s_q3_last_status_ms;
static uint32_t s_q3_hold_bad_since_ms;

static float s_previous_yaw_deg;
static float s_last_valid_line_error;
static bool s_have_previous_yaw;
static bool s_imu_online;
static bool s_q2_finish_armed;
static q2_marker_t s_q2_marker;
static q2_brake_reason_t s_q2_brake_reason;
static uint16_t s_q3_seq;
static uint8_t s_q3_attempts;
static uint8_t s_q2_start_masks[ROBOT_Q2_START_VOTE_SAMPLES];
static uint8_t s_q2_start_mask_count;
static uint8_t s_q2_start_mask_index;

static float absf_local(float value)
{
    return value < 0.0f ? -value : value;
}

static int16_t clamp_motor_command(float value)
{
    if (value > (float)ROBOT_MOTOR_PWM_MAX)
    {
        return ROBOT_MOTOR_PWM_MAX;
    }
    if (value < (float)-ROBOT_MOTOR_PWM_MAX)
    {
        return -ROBOT_MOTOR_PWM_MAX;
    }
    return (int16_t)value;
}

static void update_button(button_t *button, hal_pin_t pin, uint32_t now_ms)
{
    const bool level = hal_gpio_read(pin);
    const bool raw_pressed =
        (level ? 1u : 0u) == ROBOT_KEY_PRESSED_LEVEL;

    button->pressed_event = false;

    if (raw_pressed != button->raw_pressed)
    {
        button->raw_pressed = raw_pressed;
        button->changed_ms = now_ms;
    }

    if ((now_ms - button->changed_ms) >= ROBOT_KEY_DEBOUNCE_MS &&
        button->pressed != button->raw_pressed)
    {
        button->pressed = button->raw_pressed;
        if (button->pressed)
        {
            button->pressed_event = true;
        }
    }
}

static void poll_buttons(uint32_t now_ms)
{
    update_button(&s_key_mode, HAL_PIN_KEY_MODE, now_ms);
    update_button(&s_key_start, HAL_PIN_KEY_START, now_ms);
    update_button(&s_key_calib, HAL_PIN_KEY_CALIB, now_ms);
}

static void reset_task_runtime(void)
{
    pid_reset(&s_line_pid);
    s_line_lost_since_ms = 0u;
    s_marker_since_ms = 0u;
    s_marker_clear_since_ms = 0u;
    s_brake_start_ms = 0u;
    s_q3_last_status_ms = 0u;
    s_q3_last_keepalive_ms = 0u;
    s_q3_hold_bad_since_ms = 0u;
    s_q2_finish_armed = false;
    s_tel.run_elapsed_ms = 0u;
    s_tel.result_time_ms = 0u;
    s_tel.result_valid = false;
    s_tel.fault = ROBOT_FAULT_NONE;
}

static bool q2_black_marker(const line_sample_t *line)
{
    return line->count >= ROBOT_Q2_MARKER_MIN_BLACK;
}

static uint8_t mask_bit_count(uint8_t mask)
{
    return (uint8_t)(((mask >> 0) & 1u) +
                     ((mask >> 1) & 1u) +
                     ((mask >> 2) & 1u) +
                     ((mask >> 3) & 1u));
}

static void q2_record_start_sample(const line_sample_t *line)
{
    s_q2_start_masks[s_q2_start_mask_index] = line->mask;
    s_q2_start_mask_index =
        (uint8_t)((s_q2_start_mask_index + 1u) %
                  ROBOT_Q2_START_VOTE_SAMPLES);
    if (s_q2_start_mask_count < ROBOT_Q2_START_VOTE_SAMPLES)
    {
        ++s_q2_start_mask_count;
    }
}

static void q2_clear_start_samples(void)
{
    memset(s_q2_start_masks, 0, sizeof(s_q2_start_masks));
    s_q2_start_mask_count = 0u;
    s_q2_start_mask_index = 0u;
}

static q2_marker_t q2_detect_start_marker(bool *on_marker)
{
    uint8_t white_votes = 0u;
    uint8_t black_votes = 0u;

    for (uint8_t i = 0u; i < s_q2_start_mask_count; ++i)
    {
        const uint8_t mask = s_q2_start_masks[i];
        if (mask == 0u)
        {
            ++white_votes;
        }
        else if (mask_bit_count(mask) >= ROBOT_Q2_MARKER_MIN_BLACK)
        {
            ++black_votes;
        }
    }

    if (s_q2_start_mask_count == ROBOT_Q2_START_VOTE_SAMPLES &&
        white_votes >= ROBOT_Q2_START_VOTE_MIN &&
        white_votes > black_votes)
    {
        *on_marker = true;
        return Q2_MARKER_WHITE;
    }
    if (s_q2_start_mask_count == ROBOT_Q2_START_VOTE_SAMPLES &&
        black_votes >= ROBOT_Q2_START_VOTE_MIN &&
        black_votes > white_votes)
    {
        *on_marker = true;
        return Q2_MARKER_BLACK;
    }

    *on_marker = false;
#if ROBOT_Q2_DEFAULT_WHITE_MARKER
    return Q2_MARKER_WHITE;
#else
    return Q2_MARKER_BLACK;
#endif
}

static bool q2_raw_marker(const line_sample_t *line)
{
    if (s_q2_marker == Q2_MARKER_BLACK)
    {
        return q2_black_marker(line);
    }
    return line->mask == 0u;
}

static void enter_ready(void)
{
    motor_stop();
    s_tel.state = ROBOT_STATE_IDLE;
    s_tel.phase = ROBOT_PHASE_READY;
    s_tel.fault = ROBOT_FAULT_NONE;
    s_tel.run_elapsed_ms = 0u;
    s_tel.result_time_ms = 0u;
    s_tel.result_valid = false;
    s_marker_since_ms = 0u;
    s_marker_clear_since_ms = 0u;
    s_line_lost_since_ms = 0u;
    s_tel.q2_marker_locked = false;
    q2_clear_start_samples();
    pid_reset(&s_line_pid);
}

static void finish_q3(uint32_t result_time_ms)
{
    motor_stop();
    s_tel.state = ROBOT_STATE_FINISHED;
    s_tel.phase = ROBOT_PHASE_Q3_DONE;
    s_tel.fault = ROBOT_FAULT_NONE;
    s_tel.result_time_ms = result_time_ms;
    s_tel.run_elapsed_ms = s_tel.result_time_ms;
    s_tel.result_valid = true;
    s_q3_hold_bad_since_ms = 0u;
    /*
     * 不发送 NEUTRAL：第 3 问完成后 K230 必须继续闭环保持 -5 cm，
     * 直到 K1 清除或 K3 再次开始。
     */
}

static void fail_q3(robot_fault_t fault, uint32_t now_ms)
{
    k230_protocol_send_q3_abort(s_q3_seq);
    motor_stop();
    s_tel.state = ROBOT_STATE_ERROR;
    s_tel.phase = ROBOT_PHASE_FAULT;
    s_tel.fault = fault;
    s_tel.run_elapsed_ms = now_ms - s_task_start_ms;
    s_tel.result_valid = false;
}

static void fail_q3_hold(robot_fault_t fault)
{
    /*
     * 第3问动作已经完成后，保持异常必须报警和回中，但不能抹掉已经锁存的
     * 完成成绩。OLED 会同时显示原成绩与保持故障。
     */
    k230_protocol_send_q3_abort(s_q3_seq);
    motor_stop();
    s_tel.state = ROBOT_STATE_ERROR;
    s_tel.phase = ROBOT_PHASE_FAULT;
    s_tel.fault = fault;
    s_tel.run_elapsed_ms = s_tel.result_time_ms;
    s_tel.result_valid = true;
}

static void begin_q2_brake(q2_brake_reason_t reason,
                           robot_fault_t fault,
                           uint32_t now_ms)
{
    s_q2_brake_reason = reason;
    s_brake_start_ms = now_ms;
    s_marker_since_ms = 0u;
    s_tel.phase = ROBOT_PHASE_Q2_BRAKE;
    s_tel.fault = fault;
    s_tel.result_valid = false;
    motor_brake();
}

static void update_q2_brake(uint32_t now_ms)
{
    motor_brake();

    if ((now_ms - s_brake_start_ms) < ROBOT_Q2_ACTIVE_BRAKE_MS)
    {
        return;
    }

    motor_stop();
    s_tel.run_elapsed_ms = now_ms - s_task_start_ms;
    q2_clear_start_samples();

    if (s_q2_brake_reason == Q2_BRAKE_FINISH)
    {
        s_tel.state = ROBOT_STATE_FINISHED;
        s_tel.phase = ROBOT_PHASE_Q2_DONE;
        s_tel.fault = ROBOT_FAULT_NONE;
        s_tel.result_time_ms = s_tel.run_elapsed_ms;
        s_tel.result_valid = true;
    }
    else if (s_q2_brake_reason == Q2_BRAKE_ABORT)
    {
        enter_ready();
    }
    else
    {
        s_tel.state = ROBOT_STATE_ERROR;
        s_tel.phase = ROBOT_PHASE_FAULT;
        s_tel.result_valid = false;
    }
}

static void start_q2(const line_sample_t *line, uint32_t now_ms)
{
    bool starts_on_marker = false;

    (void)line;
    reset_task_runtime();
    s_q2_marker = q2_detect_start_marker(&starts_on_marker);
    s_tel.q2_marker_locked = starts_on_marker;
    s_tel.q2_marker_black = s_q2_marker == Q2_MARKER_BLACK;

    if (!starts_on_marker)
    {
        s_tel.state = ROBOT_STATE_ERROR;
        s_tel.phase = ROBOT_PHASE_FAULT;
        s_tel.fault = ROBOT_FAULT_Q2_START_UNSTABLE;
        motor_stop();
        q2_clear_start_samples();
        return;
    }

    /*
     * A 点白色断线与普通丢线都表现为 MASK=0。只有 MPU 累计偏航角
     * 能可靠证明已经绕场一圈，因此 MPU 离线时明确拒绝起跑。
     */
    if (!s_imu_online)
    {
        s_tel.state = ROBOT_STATE_ERROR;
        s_tel.phase = ROBOT_PHASE_FAULT;
        s_tel.fault = ROBOT_FAULT_Q2_IMU_REQUIRED;
        motor_stop();
        q2_clear_start_samples();
        return;
    }

    s_tel.state = ROBOT_STATE_RUNNING;
    s_task_start_ms = now_ms;
    mpu6050_zero_yaw();
    s_previous_yaw_deg = 0.0f;
    s_have_previous_yaw = false;
    s_tel.yaw_deg = 0.0f;
    s_tel.yaw_rate_dps = 0.0f;

    s_tel.phase = ROBOT_PHASE_Q2_LEAVE_A;
}

static void start_q3(uint32_t now_ms)
{
    const k230_status_t status = k230_protocol_get();

    reset_task_runtime();
    motor_stop();

    if (!status.link_alive)
    {
        s_tel.state = ROBOT_STATE_ERROR;
        s_tel.phase = ROBOT_PHASE_FAULT;
        s_tel.fault = ROBOT_FAULT_Q3_NO_ACK;
        return;
    }
    if (!status.vision_valid ||
        absf_local(status.x_mm) > ROBOT_Q3_START_MAX_ABS_MM)
    {
        s_tel.state = ROBOT_STATE_ERROR;
        s_tel.phase = ROBOT_PHASE_FAULT;
        s_tel.fault = ROBOT_FAULT_Q3_VISION;
        return;
    }

    s_tel.state = ROBOT_STATE_RUNNING;
    s_tel.phase = ROBOT_PHASE_Q3_WAIT_ACK;
    s_task_start_ms = now_ms;
    s_tel.ball_target_mm = (float)ROBOT_Q3_TARGET_POS_MM;

    ++s_q3_seq;
    if (s_q3_seq == 0u)
    {
        ++s_q3_seq;
    }
    s_q3_attempts = 1u;
    s_q3_last_command_ms = now_ms;
    s_q3_last_keepalive_ms = now_ms;
    /*
     * 每次 K3 启动先用同一新序号强制 K230 回中并清除旧任务，再开始本次
     * 动作。这样即使 TI 单独复位、K230 未断电，也不会接管上一次的计时。
     */
    k230_protocol_send_q3_neutral(s_q3_seq);
    s_q3_last_status_ms = now_ms;
    k230_protocol_send_q3_start(
        s_q3_seq, ROBOT_Q3_TARGET_POS_MM, ROBOT_Q3_TARGET_NEG_MM);
}

static void start_selected_task(const line_sample_t *line, uint32_t now_ms)
{
    if (s_tel.mode == ROBOT_MODE_Q2_LAP)
    {
        start_q2(line, now_ms);
    }
    else
    {
        start_q3(now_ms);
    }
}

static void stop_running_task(uint32_t now_ms)
{
    if (s_tel.mode == ROBOT_MODE_Q2_LAP)
    {
        if (s_tel.phase != ROBOT_PHASE_Q2_BRAKE)
        {
            begin_q2_brake(
                Q2_BRAKE_ABORT, ROBOT_FAULT_NONE, now_ms);
        }
    }
    else
    {
        k230_protocol_send_q3_abort(s_q3_seq);
        enter_ready();
    }
}

static void handle_button_events(const line_sample_t *line, uint32_t now_ms)
{
    if (s_key_mode.pressed_event &&
        s_tel.state != ROBOT_STATE_RUNNING)
    {
        if (s_tel.mode == ROBOT_MODE_Q3_BALL)
        {
            k230_protocol_send_q3_neutral(s_q3_seq);
            s_tel.mode = ROBOT_MODE_Q2_LAP;
        }
        else
        {
            s_tel.mode = ROBOT_MODE_Q3_BALL;
        }
        enter_ready();
    }

    if (s_key_start.pressed_event)
    {
        if (s_tel.state == ROBOT_STATE_RUNNING)
        {
            stop_running_task(now_ms);
        }
        else
        {
            start_selected_task(line, now_ms);
        }
    }

    if (s_key_calib.pressed_event &&
        s_tel.state != ROBOT_STATE_RUNNING)
    {
        motor_stop();
        if (s_tel.mode == ROBOT_MODE_Q3_BALL)
        {
            k230_protocol_send_q3_neutral(s_q3_seq);
        }
        (void)mpu6050_calibrate_gyro();
        mpu6050_zero_yaw();
        enter_ready();
    }
}

static void run_line_control(const line_sample_t *line,
                             float dt_s,
                             uint32_t now_ms)
{
    float correction;
    int16_t base_speed = ROBOT_LINE_BASE_SPEED;
    int16_t left;
    int16_t right;
    const bool finish_approach =
        s_imu_online &&
        (now_ms - s_task_start_ms) >= ROBOT_Q2_FINISH_MIN_MS &&
        absf_local(s_tel.yaw_deg) >= ROBOT_Q2_MIN_LAP_YAW_DEG;

    if (!line->valid)
    {
        if (s_line_lost_since_ms == 0u)
        {
            s_line_lost_since_ms = now_ms;
        }

        if ((now_ms - s_line_lost_since_ms) >
            ROBOT_LINE_LOST_STOP_MS)
        {
            begin_q2_brake(
                Q2_BRAKE_FAULT, ROBOT_FAULT_Q2_LINE_LOST, now_ms);
            return;
        }

        if (line->error >= 0.0f)
        {
            motor_set_raw(ROBOT_LINE_SEARCH_FAST_SPEED,
                          ROBOT_LINE_SEARCH_SLOW_SPEED);
        }
        else
        {
            motor_set_raw(ROBOT_LINE_SEARCH_SLOW_SPEED,
                          ROBOT_LINE_SEARCH_FAST_SPEED);
        }
        return;
    }

    s_line_lost_since_ms = 0u;

    if (finish_approach)
    {
        /*
         * 实跑一圈约 18.5~18.8 s；最后约 60° 弯段主动降速，可显著减小
         * 高框架惯性和 A 点制动距离，同时仍给 20 s 上限留出余量。
         */
        base_speed = ROBOT_Q2_FINISH_APPROACH_SPEED;
    }
    else if (s_imu_online &&
        absf_local(line->error) <= ROBOT_LINE_STRAIGHT_MAX_ERROR &&
        absf_local(s_tel.yaw_rate_dps) <=
            ROBOT_LINE_STRAIGHT_MAX_DPS)
    {
        base_speed = ROBOT_LINE_STRAIGHT_SPEED;
    }

    correction = pid_update(&s_line_pid, line->error, dt_s);
    left = clamp_motor_command((float)base_speed + correction);
    right = clamp_motor_command((float)base_speed - correction);
    motor_set_raw(left, right);
}

static void update_q2_leave(const line_sample_t *line, uint32_t now_ms)
{
    const bool clear_of_marker =
        !q2_raw_marker(line) && line->valid;

    motor_set_raw(ROBOT_Q2_LEAVE_SPEED, ROBOT_Q2_LEAVE_SPEED);

    if (clear_of_marker)
    {
        if (s_marker_clear_since_ms == 0u)
        {
            s_marker_clear_since_ms = now_ms;
        }
        else if ((now_ms - s_marker_clear_since_ms) >=
                 ROBOT_Q2_LEAVE_CLEAR_MS)
        {
            s_q2_finish_armed = true;
            s_marker_since_ms = 0u;
            s_line_lost_since_ms = 0u;
            pid_reset(&s_line_pid);
            s_tel.phase = ROBOT_PHASE_Q2_FOLLOW;
        }
    }
    else
    {
        s_marker_clear_since_ms = 0u;
    }

    if ((now_ms - s_task_start_ms) > ROBOT_Q2_LEAVE_TIMEOUT_MS)
    {
        begin_q2_brake(
            Q2_BRAKE_FAULT,
            ROBOT_FAULT_Q2_LEAVE_A_TIMEOUT,
            now_ms);
    }
}

static bool q2_finish_marker_candidate(const line_sample_t *line,
                                       uint32_t now_ms)
{
    if (!s_q2_finish_armed ||
        (now_ms - s_task_start_ms) < ROBOT_Q2_FINISH_MIN_MS ||
        !s_imu_online ||
        absf_local(s_tel.yaw_deg) < ROBOT_Q2_MIN_LAP_YAW_DEG)
    {
        return false;
    }

    if (s_q2_marker == Q2_MARKER_BLACK)
    {
        return q2_black_marker(line) &&
               absf_local(s_tel.yaw_rate_dps) <=
                   ROBOT_Q2_WHITE_MARKER_MAX_DPS;
    }

    /*
     * 题图的 A 点是白色线。仅在直线、刚刚还看见正常黑线时，把 MASK=0
     * 识别为白色停车标志；弯道长时间丢线不会被当成终点。
     */
    return line->mask == 0u &&
           absf_local(s_last_valid_line_error) <=
               ROBOT_LINE_START_MAX_ERROR &&
           (!s_imu_online ||
            absf_local(s_tel.yaw_rate_dps) <=
                ROBOT_Q2_WHITE_MARKER_MAX_DPS) &&
           (now_ms - s_last_line_seen_ms) <=
               ROBOT_Q2_WHITE_MARKER_ARM_MS;
}

static void update_q2_follow(const line_sample_t *line,
                             float dt_s,
                             uint32_t now_ms)
{
    const uint32_t elapsed_ms = now_ms - s_task_start_ms;

    if (elapsed_ms >= ROBOT_Q2_TASK_TIMEOUT_MS)
    {
        begin_q2_brake(
            Q2_BRAKE_FAULT, ROBOT_FAULT_Q2_TIMEOUT, now_ms);
        return;
    }

    if (s_imu_online &&
        elapsed_ms <= ROBOT_START_SPIN_GUARD_MS &&
        absf_local(s_tel.yaw_deg) > ROBOT_START_SPIN_MAX_DEG)
    {
        begin_q2_brake(
            Q2_BRAKE_FAULT, ROBOT_FAULT_Q2_SPIN, now_ms);
        return;
    }

    if (q2_finish_marker_candidate(line, now_ms))
    {
        if (s_marker_since_ms == 0u)
        {
            s_marker_since_ms = now_ms;
        }

        /* 穿过白线的确认窗口保持直行，不能进入丢线搜索。 */
        motor_set_raw(ROBOT_Q2_FINISH_APPROACH_SPEED,
                      ROBOT_Q2_FINISH_APPROACH_SPEED);

        if ((now_ms - s_marker_since_ms) >=
            ROBOT_Q2_MARKER_CONFIRM_MS)
        {
            begin_q2_brake(
                Q2_BRAKE_FINISH, ROBOT_FAULT_NONE, now_ms);
        }
        return;
    }

    s_marker_since_ms = 0u;
    run_line_control(line, dt_s, now_ms);
}

static void update_q2(const line_sample_t *line,
                      float dt_s,
                      uint32_t now_ms)
{
    s_tel.run_elapsed_ms = now_ms - s_task_start_ms;

    /*
     * 接线或电机方向错误时，LEAVE_A 阶段也可能原地急转。
     * 保护覆盖整个起步窗口，不让高车架持续旋转到离线超时。
     */
    if (s_tel.phase != ROBOT_PHASE_Q2_BRAKE &&
        s_tel.run_elapsed_ms <= ROBOT_START_SPIN_GUARD_MS &&
        absf_local(s_tel.yaw_deg) > ROBOT_START_SPIN_MAX_DEG)
    {
        begin_q2_brake(
            Q2_BRAKE_FAULT, ROBOT_FAULT_Q2_SPIN, now_ms);
        return;
    }

    if (s_tel.phase == ROBOT_PHASE_Q2_LEAVE_A)
    {
        update_q2_leave(line, now_ms);
    }
    else if (s_tel.phase == ROBOT_PHASE_Q2_FOLLOW)
    {
        update_q2_follow(line, dt_s, now_ms);
    }
    else if (s_tel.phase == ROBOT_PHASE_Q2_BRAKE)
    {
        update_q2_brake(now_ms);
    }
}

static void map_q3_phase(uint8_t task_state)
{
    switch (task_state)
    {
    case K230_TASK_TO_POS:
        s_tel.phase = ROBOT_PHASE_Q3_TO_POS;
        break;
    case K230_TASK_TO_NEG:
        s_tel.phase = ROBOT_PHASE_Q3_TO_NEG;
        break;
    case K230_TASK_HOLD_NEG:
        s_tel.phase = ROBOT_PHASE_Q3_HOLD_NEG;
        break;
    case K230_TASK_DONE:
        s_tel.phase = ROBOT_PHASE_Q3_DONE;
        break;
    default:
        break;
    }
}

static void copy_k230_telemetry(const k230_status_t *status)
{
    s_tel.k230_link = status->link_alive;
    s_tel.vision_valid = status->vision_valid;
    s_tel.ball_x_mm = status->x_mm;
    s_tel.ball_target_mm = status->target_mm;
    s_tel.ball_error_mm = status->error_mm;
    s_tel.servo_us = status->servo_us;
}

static void update_q3(uint32_t now_ms)
{
    const k230_status_t status = k230_protocol_get();
    const uint32_t elapsed_ms = now_ms - s_task_start_ms;
    const bool status_for_this_task =
        status.seq == s_q3_seq && status.frames > 0u;
    const bool ack_for_this_task =
        status.ack == K230_ACK_START &&
        status.ack_seq == s_q3_seq;

    motor_stop();
    s_tel.run_elapsed_ms = elapsed_ms;
    copy_k230_telemetry(&status);

    if ((now_ms - s_q3_last_keepalive_ms) >= ROBOT_Q3_KEEPALIVE_MS)
    {
        k230_protocol_send_q3_keepalive(s_q3_seq);
        s_q3_last_keepalive_ms = now_ms;
    }

    if (status_for_this_task && status.last_update_ms != 0u)
    {
        s_q3_last_status_ms = status.last_update_ms;
    }

    /*
     * 先处理同序号的 FAULT/DONE，再检查本地看门狗。这样 K230 已在
     * 5 秒内完成、但串口帧恰好在边界到达时，不会被 TI 误判超时。
     */
    if (status_for_this_task &&
        (status.fault || status.task_state == K230_TASK_FAULT))
    {
        fail_q3(ROBOT_FAULT_Q3_REMOTE, now_ms);
        return;
    }
    if (status_for_this_task &&
        (status.done || status.task_state == K230_TASK_DONE))
    {
        if (status.elapsed_ms > ROBOT_Q3_TASK_TIMEOUT_MS ||
            elapsed_ms > ROBOT_Q3_TASK_TIMEOUT_MS)
        {
            fail_q3(ROBOT_FAULT_Q3_TIMEOUT, now_ms);
        }
        else if (!status.vision_valid ||
                 absf_local(status.error_mm) >
                     ROBOT_Q3_HOLD_ERROR_LIMIT_MM)
        {
            fail_q3(ROBOT_FAULT_Q3_REMOTE, now_ms);
        }
        else
        {
            /* 题目计时从 K3 按下开始，OLED 锁存 TI 本地总时间。 */
            finish_q3(elapsed_ms);
        }
        return;
    }

    if (elapsed_ms >= ROBOT_Q3_LOCAL_WATCHDOG_MS)
    {
        fail_q3(ROBOT_FAULT_Q3_TIMEOUT, now_ms);
        return;
    }

    if (s_tel.phase == ROBOT_PHASE_Q3_WAIT_ACK)
    {
        /*
         * 若 ACK 丢了但已经收到同序号 H3，也直接进入运行态；
         * 这样不会因为单个串口字节受干扰而误判整项失败。
         */
        if (ack_for_this_task ||
            (status_for_this_task &&
             status.task_state >= K230_TASK_TO_POS &&
             status.task_state <= K230_TASK_DONE))
        {
            if (ack_for_this_task)
            {
                s_q3_last_status_ms = status.last_update_ms;
            }
            map_q3_phase(status.task_state);
            if (s_tel.phase == ROBOT_PHASE_Q3_WAIT_ACK)
            {
                s_tel.phase = ROBOT_PHASE_Q3_TO_POS;
            }
        }
        else if ((now_ms - s_q3_last_command_ms) >=
                 ROBOT_Q3_ACK_RETRY_MS)
        {
            if (s_q3_attempts >= ROBOT_Q3_ACK_MAX_ATTEMPTS)
            {
                fail_q3(ROBOT_FAULT_Q3_NO_ACK, now_ms);
                return;
            }

            ++s_q3_attempts;
            s_q3_last_command_ms = now_ms;
            k230_protocol_send_q3_start(
                s_q3_seq,
                ROBOT_Q3_TARGET_POS_MM,
                ROBOT_Q3_TARGET_NEG_MM);
        }
    }

    if (status_for_this_task)
    {
        map_q3_phase(status.task_state);
    }

    if (s_tel.phase != ROBOT_PHASE_Q3_WAIT_ACK &&
        (s_q3_last_status_ms == 0u ||
         (now_ms - s_q3_last_status_ms) >
             ROBOT_Q3_LINK_TIMEOUT_MS))
    {
        fail_q3(ROBOT_FAULT_Q3_VISION, now_ms);
    }
}

static void supervise_q3_hold(uint32_t now_ms)
{
    const k230_status_t status = k230_protocol_get();
    const bool status_for_this_task =
        status.seq == s_q3_seq && status.frames > 0u;
    const bool hold_bad =
        !status.vision_valid ||
        absf_local(status.error_mm) >
            ROBOT_Q3_HOLD_ERROR_LIMIT_MM;

    motor_stop();
    copy_k230_telemetry(&status);
    s_tel.run_elapsed_ms = s_tel.result_time_ms;

    if ((now_ms - s_q3_last_keepalive_ms) >= ROBOT_Q3_KEEPALIVE_MS)
    {
        k230_protocol_send_q3_keepalive(s_q3_seq);
        s_q3_last_keepalive_ms = now_ms;
    }

    if (status_for_this_task && status.last_update_ms != 0u)
    {
        s_q3_last_status_ms = status.last_update_ms;
    }

    if (status_for_this_task &&
        (status.fault || status.task_state == K230_TASK_FAULT))
    {
        fail_q3_hold(ROBOT_FAULT_Q3_REMOTE);
        return;
    }

    if (!status_for_this_task ||
        s_q3_last_status_ms == 0u ||
        (now_ms - s_q3_last_status_ms) >
            ROBOT_Q3_HOLD_LINK_TIMEOUT_MS)
    {
        fail_q3_hold(ROBOT_FAULT_Q3_VISION);
        return;
    }

    if (hold_bad)
    {
        if (s_q3_hold_bad_since_ms == 0u)
        {
            s_q3_hold_bad_since_ms = now_ms;
        }
        else if ((now_ms - s_q3_hold_bad_since_ms) >=
                 ROBOT_Q3_HOLD_ERROR_TIMEOUT_MS)
        {
            fail_q3_hold(ROBOT_FAULT_Q3_VISION);
        }
    }
    else
    {
        s_q3_hold_bad_since_ms = 0u;
    }
}

void robot_app_init(void)
{
    memset(&s_tel, 0, sizeof(s_tel));
    memset(&s_key_mode, 0, sizeof(s_key_mode));
    memset(&s_key_start, 0, sizeof(s_key_start));
    memset(&s_key_calib, 0, sizeof(s_key_calib));

    s_tel.mode = ROBOT_MODE_Q2_LAP;
    s_tel.state = ROBOT_STATE_IDLE;
    s_tel.phase = ROBOT_PHASE_READY;

    pid_init(&s_line_pid,
             ROBOT_LINE_KP,
             ROBOT_LINE_KI,
             ROBOT_LINE_KD,
             ROBOT_LINE_CORRECTION_LIMIT,
             3000.0f);

    motor_init();
    line_sensor_init();
    k230_protocol_init();
    /* TI 上电/复位时立即请求 K230 回中；K230 尚未启动时由自身看门狗兜底。 */
    k230_protocol_send_q3_neutral(0u);
    s_imu_online = mpu6050_init();

    /* 某些 0.96 寸 OLED 上电后需要约 100 ms 才能可靠响应。 */
    hal_delay_ms(110u);
    display_init();
    s_last_tick_ms = hal_millis();
}

void robot_app_tick(uint32_t now_ms)
{
    uint32_t dt_ms = now_ms - s_last_tick_ms;
    float dt_s;
    line_sample_t line;
    mpu6050_state_t imu;

    if (dt_ms < ROBOT_CONTROL_PERIOD_MS)
    {
        return;
    }
    if (dt_ms > 50u)
    {
        dt_ms = ROBOT_CONTROL_PERIOD_MS;
    }

    s_last_tick_ms = now_ms;
    dt_s = (float)dt_ms / 1000.0f;
    s_tel.now_ms = now_ms;

    k230_protocol_poll(now_ms);
    {
        const k230_status_t status = k230_protocol_get();
        copy_k230_telemetry(&status);
    }

    s_imu_online = mpu6050_update(dt_ms);
    imu = mpu6050_get_state();
    s_tel.yaw_deg = imu.yaw_deg;
    if (s_imu_online && s_have_previous_yaw)
    {
        s_tel.yaw_rate_dps =
            (imu.yaw_deg - s_previous_yaw_deg) / dt_s;
    }
    else
    {
        s_tel.yaw_rate_dps = 0.0f;
    }
    s_previous_yaw_deg = imu.yaw_deg;
    s_have_previous_yaw = s_imu_online;

    line = line_sensor_read();
    s_tel.line_error = line.error;
    s_tel.line_valid = line.valid;
    s_tel.line_mask = line.mask;
    if (line.valid)
    {
        s_last_line_seen_ms = now_ms;
        s_last_valid_line_error = line.error;
    }
    if (s_tel.mode == ROBOT_MODE_Q2_LAP &&
        s_tel.state != ROBOT_STATE_RUNNING)
    {
        q2_record_start_sample(&line);
    }

    /*
     * 先读取本周期传感器，再处理 K3；避免刚上电按键时用上一周期 MASK。
     */
    poll_buttons(now_ms);
    handle_button_events(&line, now_ms);

    if (s_tel.state == ROBOT_STATE_RUNNING)
    {
        if (s_tel.mode == ROBOT_MODE_Q2_LAP)
        {
            update_q2(&line, dt_s, now_ms);
        }
        else
        {
            update_q3(now_ms);
        }
    }
    else
    {
        motor_stop();

        if (s_tel.mode == ROBOT_MODE_Q3_BALL &&
            s_tel.state == ROBOT_STATE_FINISHED &&
            s_tel.result_valid)
        {
            supervise_q3_hold(now_ms);
        }
        else if (s_tel.state == ROBOT_STATE_FINISHED &&
            s_tel.result_valid)
        {
            s_tel.run_elapsed_ms = s_tel.result_time_ms;
        }
    }

    s_tel.left_pwm = motor_get_left();
    s_tel.right_pwm = motor_get_right();
    display_update(&s_tel);
}

robot_telemetry_t robot_app_get_telemetry(void)
{
    return s_tel;
}
