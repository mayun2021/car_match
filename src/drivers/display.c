/**
 * @file display.c
 * @brief 调试显示/OLED 输出实现。
 *
 * 每 250 ms 把遥测数据发一行到调试串口，同时刷新到 0.96 寸 SSD1306
 * OLED（128x64，8 行 x 8 像素点阵字符，见 oled.h）。
 */

#include "display.h"
#include "hal.h"
#include "oled.h"
#include "robot_config.h"

#include <stdio.h>

/**
 * @brief 初始化调试显示模块和 OLED。
 */
void display_init(void)
{
    hal_uart_debug_write("display: debug output ready\r\n");
    oled_init();
}

/**
 * @brief 把运行状态枚举转换成定长的 4 字符缩写，方便对齐显示。
 *
 * @param state 当前任务状态。
 * @return 4 个字符（不含结尾符）的状态缩写。
 */
static const char *state_text(robot_state_t state)
{
    switch (state)
    {
    case ROBOT_STATE_IDLE:
        return "IDLE";
    case ROBOT_STATE_RUNNING:
        return "RUN ";
    case ROBOT_STATE_FINISHED:
        return "DONE";
    default:
        return "ERR ";
    }
}

/**
 * @brief 按固定排版把遥测数据画到 OLED 帧缓冲并刷新。
 *
 * 屏幕共 8 行（每行 8 像素）：模式/状态、运行用时、巡线误差、
 * 红外掩码、钢球位置、电机速度百分比、偏航角、按键功能提示。
 *
 * @param telemetry 当前机器人遥测数据。
 */
static void oled_render(const robot_telemetry_t *telemetry)
{
    char line[17];
    int32_t avg_pwm;
    int speed_pct;

    oled_clear();

    (void)snprintf(line, sizeof(line), "M%d %s", (int)telemetry->mode, state_text(telemetry->state));
    oled_draw_string(0, 0, line);

    (void)snprintf(line, sizeof(line), "TIME:%5.1fS", (double)telemetry->run_elapsed_ms / 1000.0);
    oled_draw_string(0, 8, line);

    (void)snprintf(line, sizeof(line), "LINE:%5.0f", (double)telemetry->line_error);
    oled_draw_string(0, 16, line);

    (void)snprintf(line, sizeof(line), "MASK:%c%c%c%c",
                    (telemetry->line_mask & 0x01u) ? '1' : '0',
                    (telemetry->line_mask & 0x02u) ? '1' : '0',
                    (telemetry->line_mask & 0x04u) ? '1' : '0',
                    (telemetry->line_mask & 0x08u) ? '1' : '0');
    oled_draw_string(0, 24, line);

    (void)snprintf(line, sizeof(line), "BALL:%6.1f", (double)telemetry->ball_x_mm);
    oled_draw_string(0, 32, line);

    avg_pwm = ((int32_t)telemetry->left_pwm + (int32_t)telemetry->right_pwm) / 2;
    speed_pct = (int)((avg_pwm * 100) / ROBOT_MOTOR_PWM_MAX);
    (void)snprintf(line, sizeof(line), "SPD:%4d%%", speed_pct);
    oled_draw_string(0, 40, line);

    (void)snprintf(line, sizeof(line), "YAW:%6.1f", (double)telemetry->yaw_deg);
    oled_draw_string(0, 48, line);

    oled_draw_string(0, 56, "MODE/STRT/CALIB");

    oled_flush();
}

/**
 * @brief 周期性输出机器人运行状态到调试串口和 OLED。
 *
 * @param telemetry 当前机器人遥测数据。
 */
void display_update(const robot_telemetry_t *telemetry)
{
    static uint32_t s_last_print_ms;
    char buf[192];

    if (telemetry->now_ms - s_last_print_ms < 250u)
    {
        return;
    }

    s_last_print_ms = telemetry->now_ms;
    (void)snprintf(buf,
                   sizeof(buf),
                   "mode=%d state=%d line=%.0f ball=%.1f target=%.1f yaw=%.1f L=%d R=%d servo=%u time=%.1f\r\n",
                   (int)telemetry->mode,
                   (int)telemetry->state,
                   (double)telemetry->line_error,
                   (double)telemetry->ball_x_mm,
                   (double)telemetry->ball_target_mm,
                   (double)telemetry->yaw_deg,
                   telemetry->left_pwm,
                   telemetry->right_pwm,
                   telemetry->servo_us,
                   (double)telemetry->run_elapsed_ms / 1000.0);
    hal_uart_debug_write(buf);

    oled_render(telemetry);
}
