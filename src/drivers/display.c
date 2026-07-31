/**
 * @file display.c
 * @brief 第 2、3 问 OLED 比赛界面。
 */

#include "display.h"
#include "hal.h"
#include "oled.h"
#include "robot_config.h"

#include <stdio.h>

static const char *phase_text(robot_phase_t phase)
{
    switch (phase)
    {
    case ROBOT_PHASE_READY:
        return "READY";
    case ROBOT_PHASE_Q2_LEAVE_A:
        return "LEAVE A";
    case ROBOT_PHASE_Q2_FOLLOW:
        return "FOLLOW";
    case ROBOT_PHASE_Q2_BRAKE:
        return "BRAKE";
    case ROBOT_PHASE_Q2_DONE:
        return "LAP DONE";
    case ROBOT_PHASE_Q3_WAIT_ACK:
        return "WAIT K230";
    case ROBOT_PHASE_Q3_TO_POS:
        return "TO +5CM";
    case ROBOT_PHASE_Q3_TO_NEG:
        return "TO -5CM";
    case ROBOT_PHASE_Q3_HOLD_NEG:
        return "HOLD -5";
    case ROBOT_PHASE_Q3_DONE:
        return "BALL DONE";
    default:
        return "FAULT";
    }
}

static const char *fault_text(robot_fault_t fault)
{
    switch (fault)
    {
    case ROBOT_FAULT_Q2_LEAVE_A_TIMEOUT:
        return "LEAVE TIMEOUT";
    case ROBOT_FAULT_Q2_LINE_LOST:
        return "LINE LOST";
    case ROBOT_FAULT_Q2_TIMEOUT:
        return "LAP TIMEOUT";
    case ROBOT_FAULT_Q2_SPIN:
        return "SPIN/WIRING";
    case ROBOT_FAULT_Q2_IMU_REQUIRED:
        return "MPU NEEDED";
    case ROBOT_FAULT_Q2_START_UNSTABLE:
        return "ALIGN A MARK";
    case ROBOT_FAULT_Q3_NO_ACK:
        return "NO K230 ACK";
    case ROBOT_FAULT_Q3_VISION:
        return "K230/VIS LOST";
    case ROBOT_FAULT_Q3_TIMEOUT:
        return "BALL TIMEOUT";
    case ROBOT_FAULT_Q3_REMOTE:
        return "K230 FAULT";
    default:
        return "NONE";
    }
}

void display_init(void)
{
    oled_init();
    hal_uart_debug_write("display ready\r\n");
}

static void draw_time(uint8_t y, uint32_t time_ms)
{
    char line[17];

    (void)snprintf(line,
                   sizeof(line),
                   "TIME:%2lu.%03luS",
                   (unsigned long)(time_ms / 1000u),
                   (unsigned long)(time_ms % 1000u));
    oled_draw_string(0, y, line);
}

static void render_q2(const robot_telemetry_t *telemetry)
{
    char line[17];
    const uint32_t time_ms = telemetry->result_valid
        ? telemetry->result_time_ms
        : telemetry->run_elapsed_ms;

    (void)snprintf(
        line, sizeof(line), "Q2 %s", phase_text(telemetry->phase));
    oled_draw_string(0, 0, line);
    draw_time(8, time_ms);

    (void)snprintf(line,
                   sizeof(line),
                   "MASK:%X CNT:%u",
                   (unsigned int)telemetry->line_mask,
                   (unsigned int)(
                       ((telemetry->line_mask >> 0) & 1u) +
                       ((telemetry->line_mask >> 1) & 1u) +
                       ((telemetry->line_mask >> 2) & 1u) +
                       ((telemetry->line_mask >> 3) & 1u)));
    oled_draw_string(0, 16, line);

    (void)snprintf(line,
                   sizeof(line),
                   "ERR:%5d",
                   (int)telemetry->line_error);
    oled_draw_string(0, 24, line);

    (void)snprintf(line,
                   sizeof(line),
                   "PWM:%d/%d",
                   (int)telemetry->left_pwm,
                   (int)telemetry->right_pwm);
    oled_draw_string(0, 32, line);

    (void)snprintf(line,
                   sizeof(line),
                   "YAW:%d RATE:%d",
                   (int)telemetry->yaw_deg,
                   (int)telemetry->yaw_rate_dps);
    oled_draw_string(0, 40, line);

    if (telemetry->state == ROBOT_STATE_ERROR)
    {
        (void)snprintf(
            line, sizeof(line), "FAIL:%s", fault_text(telemetry->fault));
        oled_draw_string(0, 48, line);
    }
    else if (telemetry->result_valid)
    {
        oled_draw_string(0, 48, "RESULT LOCKED");
    }
    else
    {
        if (telemetry->q2_marker_locked)
        {
            oled_draw_string(
                0,
                48,
                telemetry->q2_marker_black ? "A:BLACK LOCKED" :
                                             "A:WHITE LOCKED");
        }
        else
        {
            oled_draw_string(0, 48, "A:AUTO DETECT");
        }
    }

    oled_draw_string(0, 56, "K2 MODE K3 GO");
}

static void render_q3(const robot_telemetry_t *telemetry)
{
    char line[17];
    const uint32_t time_ms = telemetry->result_valid
        ? telemetry->result_time_ms
        : telemetry->run_elapsed_ms;

    (void)snprintf(
        line, sizeof(line), "Q3 %s", phase_text(telemetry->phase));
    oled_draw_string(0, 0, line);
    draw_time(8, time_ms);

    (void)snprintf(
        line, sizeof(line), "BALL:%+4dMM", (int)telemetry->ball_x_mm);
    oled_draw_string(0, 16, line);

    (void)snprintf(line,
                   sizeof(line),
                   "T:%+3d E:%+3d",
                   (int)telemetry->ball_target_mm,
                   (int)telemetry->ball_error_mm);
    oled_draw_string(0, 24, line);

    (void)snprintf(line,
                   sizeof(line),
                   "SERVO:%4uUS",
                   (unsigned int)telemetry->servo_us);
    oled_draw_string(0, 32, line);

    (void)snprintf(line,
                   sizeof(line),
                   "LINK:%s VIS:%s",
                   telemetry->k230_link ? "OK" : "--",
                   telemetry->vision_valid ? "OK" : "--");
    oled_draw_string(0, 40, line);

    if (telemetry->state == ROBOT_STATE_ERROR)
    {
        (void)snprintf(
            line, sizeof(line), "FAIL:%s", fault_text(telemetry->fault));
        oled_draw_string(0, 48, line);
    }
    else if (telemetry->result_valid)
    {
        oled_draw_string(0, 48, "HOLDING -5CM");
    }
    else
    {
        oled_draw_string(0, 48, "K1=NEUTRAL");
    }

    oled_draw_string(0, 56, "K2 MODE K3 GO");
}

void display_update(const robot_telemetry_t *telemetry)
{
    static uint32_t s_last_refresh_ms;
    static robot_mode_t s_last_mode;
    static robot_state_t s_last_state;
    static robot_phase_t s_last_phase;
    static bool s_have_last_frame;
    const bool state_changed =
        !s_have_last_frame ||
        telemetry->mode != s_last_mode ||
        telemetry->state != s_last_state ||
        telemetry->phase != s_last_phase;

    /*
     * SSD1306 全屏经软件 I2C 刷新约占用二十多毫秒，会打断 5 ms 循迹周期。
     * 第2问只要求停车后显示总时间，因此 LEAVE/FOLLOW/BRAKE 期间保留上一帧，
     * 等完成、故障或急停后立即刷新。第3问车体静止，仍可每 250 ms 实时刷新。
     */
    if (telemetry->mode == ROBOT_MODE_Q2_LAP &&
        telemetry->state == ROBOT_STATE_RUNNING)
    {
        return;
    }

    if (!state_changed &&
        (telemetry->now_ms - s_last_refresh_ms) < 250u)
    {
        return;
    }
    s_last_refresh_ms = telemetry->now_ms;

    oled_clear();
    if (telemetry->mode == ROBOT_MODE_Q2_LAP)
    {
        render_q2(telemetry);
    }
    else
    {
        render_q3(telemetry);
    }
    oled_flush();

    s_last_mode = telemetry->mode;
    s_last_state = telemetry->state;
    s_last_phase = telemetry->phase;
    s_have_last_frame = true;
}
