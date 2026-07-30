/**
 * @file robot_app.c
 * @brief H 题小车应用层状态机实现。
 *
 * 本文件负责把红外巡线、电机差速、K230 视觉滚球、MG996R 舵机和 MPU6050
 * 角度记录组合成题目要求的三种模式。
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
#include "servo.h"

typedef struct
{
    bool raw_pressed;
    bool pressed;
    bool pressed_event;
    uint32_t changed_ms;
} button_t;

typedef enum
{
    BALL_STAGE_POSITIVE = 0,
    BALL_STAGE_NEGATIVE,
    BALL_STAGE_DONE
} ball_stage_t;

static robot_telemetry_t s_tel;
static pid_t s_line_pid;
static pid_t s_ball_pid;
static button_t s_key_mode;
static button_t s_key_start;
static button_t s_key_calib;
static uint32_t s_last_tick_ms;
static uint32_t s_task_start_ms;
static uint32_t s_line_all_black_since_ms;
static uint32_t s_ball_stage_start_ms;
static uint32_t s_ball_stable_since_ms;
static ball_stage_t s_ball_stage;

/**
 * @brief 计算浮点数绝对值。
 *
 * @param value 输入值。
 * @return 输入值的绝对值。
 */
static float absf_local(float value)
{
    return value < 0.0f ? -value : value;
}

/**
 * @brief 将电机控制量限制在可输出范围。
 *
 * @param value 浮点控制量。
 * @return 可直接发送给 motor_set_raw() 的整数速度命令。
 */
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

/**
 * @brief 更新一个按键的消抖状态。
 *
 * @param button 按键状态对象。
 * @param pin 按键对应的硬件抽象引脚。
 * @param now_ms 当前系统时间，单位 ms。
 */
static void update_button(button_t *button, hal_pin_t pin, uint32_t now_ms)
{
    const bool level = hal_gpio_read(pin);
    const bool raw_pressed = (level ? 1u : 0u) == ROBOT_KEY_PRESSED_LEVEL;

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

/**
 * @brief 读取并消抖所有用户按键。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
static void poll_buttons(uint32_t now_ms)
{
    update_button(&s_key_mode, HAL_PIN_KEY_MODE, now_ms);
    update_button(&s_key_start, HAL_PIN_KEY_START, now_ms);
    update_button(&s_key_calib, HAL_PIN_KEY_CALIB, now_ms);
}

/**
 * @brief 清空巡线和滚球 PID 的历史状态。
 */
static void reset_controllers(void)
{
    pid_reset(&s_line_pid);
    pid_reset(&s_ball_pid);
    s_line_all_black_since_ms = 0u;
    s_ball_stable_since_ms = 0u;
}

/**
 * @brief 将当前任务置为完成状态并停止执行器。
 */
static void set_finished(void)
{
    s_tel.state = ROBOT_STATE_FINISHED;
    motor_stop();
    servo_center();
}

/**
 * @brief 启动当前选择的比赛模式。
 *
 * @param now_ms 当前系统时间，单位 ms，用于记录任务开始时间。
 */
static void start_task(uint32_t now_ms)
{
    s_tel.state = ROBOT_STATE_RUNNING;
    s_task_start_ms = now_ms;
    s_ball_stage_start_ms = now_ms;
    s_ball_stage = BALL_STAGE_POSITIVE;
    s_tel.ball_target_mm = ROBOT_BALL_TARGET_POS_MM;
    s_tel.run_elapsed_ms = 0u;
    reset_controllers();
    mpu6050_zero_yaw();
}

/**
 * @brief 手动停止当前任务并回到空闲状态。
 */
static void stop_task(void)
{
    s_tel.state = ROBOT_STATE_IDLE;
    reset_controllers();
    motor_stop();
    servo_center();
}

/**
 * @brief 根据按键事件执行模式切换、启停和校准。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
static void handle_button_events(uint32_t now_ms)
{
    if (s_key_mode.pressed_event && s_tel.state != ROBOT_STATE_RUNNING)
    {
        if (s_tel.mode == ROBOT_MODE_COMBINED)
        {
            s_tel.mode = ROBOT_MODE_LINE;
        }
        else
        {
            s_tel.mode = (robot_mode_t)((int)s_tel.mode + 1);
        }
    }

    if (s_key_start.pressed_event)
    {
        if (s_tel.state == ROBOT_STATE_RUNNING)
        {
            stop_task();
        }
        else
        {
            start_task(now_ms);
        }
    }

    if (s_key_calib.pressed_event && s_tel.state != ROBOT_STATE_RUNNING)
    {
        motor_stop();
        servo_center();
        (void)mpu6050_calibrate_gyro();
        mpu6050_zero_yaw();
    }
}

/**
 * @brief 执行一次巡线闭环控制。
 *
 * @param line 当前红外循迹采样结果。
 * @param dt_s 控制周期，单位 s。
 * @param now_ms 当前系统时间，单位 ms。
 */
static void run_line_control(const line_sample_t *line, float dt_s, uint32_t now_ms)
{
    float correction;
    int16_t left;
    int16_t right;

    s_tel.line_error = line->error;
    s_tel.line_valid = line->valid;

    if (!line->valid)
    {
        /*
         * 丢线后按最近一次误差方向原地/小半径搜索。
         * error 为正说明黑线最后出现在右边，因此向右搜索。
         */
        if (line->error >= 0.0f)
        {
            motor_set_raw(ROBOT_LINE_SEARCH_SPEED, -ROBOT_LINE_SEARCH_SPEED);
        }
        else
        {
            motor_set_raw(-ROBOT_LINE_SEARCH_SPEED, ROBOT_LINE_SEARCH_SPEED);
        }
        return;
    }

    correction = pid_update(&s_line_pid, line->error, dt_s);

    /*
     * error > 0：黑线在右侧，小车需要右转。
     * 差速车右转 = 左轮更快、右轮更慢。
     */
    left = clamp_motor_command((float)ROBOT_LINE_BASE_SPEED + correction);
    right = clamp_motor_command((float)ROBOT_LINE_BASE_SPEED - correction);
    motor_set_raw(left, right);

    if (now_ms - s_task_start_ms > ROBOT_START_LINE_IGNORE_MS)
    {
        if (line->all_black)
        {
            if (s_line_all_black_since_ms == 0u)
            {
                s_line_all_black_since_ms = now_ms;
            }
            else if (now_ms - s_line_all_black_since_ms > 60u)
            {
                set_finished();
            }
        }
        else
        {
            s_line_all_black_since_ms = 0u;
        }
    }

    if (now_ms - s_task_start_ms > ROBOT_LINE_TASK_TIMEOUT_MS)
    {
        set_finished();
    }
}

/**
 * @brief 推进模式二的钢球目标位置阶段。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
static void advance_ball_stage(uint32_t now_ms)
{
    if (s_ball_stage == BALL_STAGE_POSITIVE)
    {
        s_ball_stage = BALL_STAGE_NEGATIVE;
        s_ball_stage_start_ms = now_ms;
        s_ball_stable_since_ms = 0u;
        s_tel.ball_target_mm = ROBOT_BALL_TARGET_NEG_MM;
        pid_reset(&s_ball_pid);
    }
    else if (s_ball_stage == BALL_STAGE_NEGATIVE)
    {
        s_ball_stage = BALL_STAGE_DONE;
        set_finished();
    }
}

/**
 * @brief 执行一次滚球闭环控制。
 *
 * @param dt_s 控制周期，单位 s。
 * @param now_ms 当前系统时间，单位 ms。
 * @param use_sequence true 表示模式二按 +5 cm、-5 cm 顺序演示；false 表示模式三稳定在中心。
 */
static void run_ball_control(float dt_s, uint32_t now_ms, bool use_sequence)
{
    const k230_vision_t vision = k230_protocol_get();
    float error;
    float servo_delta;

    s_tel.vision_valid = vision.valid;
    s_tel.ball_x_mm = vision.x_mm;

    if (!use_sequence)
    {
        s_tel.ball_target_mm = 0.0f;
    }

    if (!vision.valid)
    {
        /*
         * 视觉丢失时不要继续沿最后误差猛打舵，回中更稳。
         * 等视觉恢复后 PID 从零重新开始，避免积分残留。
         */
        pid_reset(&s_ball_pid);
        servo_center();
        return;
    }

    error = s_tel.ball_target_mm - vision.x_mm;
    servo_delta = pid_update(&s_ball_pid, error, dt_s);

#if ROBOT_BALL_SERVO_REVERSE
    servo_delta = -servo_delta;
#endif

    servo_set_delta_us((int16_t)servo_delta);

    if (use_sequence)
    {
        if (absf_local(error) <= ROBOT_BALL_TARGET_TOLERANCE_MM)
        {
            if (s_ball_stable_since_ms == 0u)
            {
                s_ball_stable_since_ms = now_ms;
            }
            else if (now_ms - s_ball_stable_since_ms >= ROBOT_BALL_STABLE_MS)
            {
                advance_ball_stage(now_ms);
            }
        }
        else
        {
            s_ball_stable_since_ms = 0u;
        }

        if (now_ms - s_ball_stage_start_ms > ROBOT_BALL_STAGE_TIMEOUT_MS)
        {
            advance_ball_stage(now_ms);
        }
    }
}

/**
 * @brief 初始化应用层、驱动层和控制器。
 */
void robot_app_init(void)
{
    s_tel.now_ms = 0u;
    s_tel.mode = ROBOT_MODE_LINE;
    s_tel.state = ROBOT_STATE_IDLE;
    s_tel.ball_target_mm = 0.0f;

    pid_init(&s_line_pid,
             ROBOT_LINE_KP,
             ROBOT_LINE_KI,
             ROBOT_LINE_KD,
             ROBOT_LINE_CORRECTION_LIMIT,
             3000.0f);
    pid_init(&s_ball_pid,
             ROBOT_BALL_KP,
             ROBOT_BALL_KI,
             ROBOT_BALL_KD,
             ROBOT_BALL_OUTPUT_LIMIT_US,
             2000.0f);

    motor_init();
    line_sensor_init();
    servo_init();
    k230_protocol_init();
    (void)mpu6050_init();
    display_init();
}

/**
 * @brief 周期执行机器人控制逻辑。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
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

    poll_buttons(now_ms);
    handle_button_events(now_ms);

    k230_protocol_poll(now_ms);
    (void)mpu6050_update(dt_ms);
    imu = mpu6050_get_state();
    s_tel.yaw_deg = imu.yaw_deg;

    line = line_sensor_read();
    s_tel.line_error = line.error;
    s_tel.line_valid = line.valid;
    s_tel.line_mask = line.mask;

    if (s_tel.state == ROBOT_STATE_RUNNING)
    {
        s_tel.run_elapsed_ms = now_ms - s_task_start_ms;

        if (s_tel.mode == ROBOT_MODE_LINE)
        {
            run_line_control(&line, dt_s, now_ms);
            servo_center();
        }
        else if (s_tel.mode == ROBOT_MODE_BALL)
        {
            motor_stop();
            run_ball_control(dt_s, now_ms, true);
        }
        else
        {
            run_line_control(&line, dt_s, now_ms);
            if (s_tel.state == ROBOT_STATE_RUNNING)
            {
                run_ball_control(dt_s, now_ms, false);
            }
        }
    }
    else
    {
        motor_stop();
        servo_center();
    }

    s_tel.left_pwm = motor_get_left();
    s_tel.right_pwm = motor_get_right();
    s_tel.servo_us = servo_get_pulse_us();
    display_update(&s_tel);
}

/**
 * @brief 获取当前机器人遥测数据。
 *
 * @return 当前模式、状态、传感器和执行器输出。
 */
robot_telemetry_t robot_app_get_telemetry(void)
{
    return s_tel;
}
