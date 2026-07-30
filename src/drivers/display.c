/**
 * @file display.c
 * @brief 调试显示输出实现。
 *
 * 当前版本通过调试串口周期性输出关键遥测数据。上板后可把本文件替换为
 * OLED 或串口屏显示实现。
 */

#include "display.h"
#include "hal.h"

#include <stdio.h>

/**
 * @brief 初始化调试显示模块。
 */
void display_init(void)
{
    hal_uart_debug_write("display: debug output ready\r\n");
}

/**
 * @brief 周期性输出机器人运行状态。
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
                   "mode=%d state=%d line=%.0f ball=%.1f target=%.1f yaw=%.1f L=%d R=%d servo=%u\r\n",
                   (int)telemetry->mode,
                   (int)telemetry->state,
                   (double)telemetry->line_error,
                   (double)telemetry->ball_x_mm,
                   (double)telemetry->ball_target_mm,
                   (double)telemetry->yaw_deg,
                   telemetry->left_pwm,
                   telemetry->right_pwm,
                   telemetry->servo_us);
    hal_uart_debug_write(buf);
}
