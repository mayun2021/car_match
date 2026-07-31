/**
 * @file simulate_control.c
 * @brief 第 2 问白色 A 线状态机桌面回归测试。
 */

#include "hal.h"
#include "robot_app.h"
#include "robot_types.h"

#include <stdio.h>

void hal_stub_set_line_mask(uint8_t mask);
void hal_stub_set_key(hal_pin_t pin, bool pressed);
void hal_stub_push_k230_text(const char *text);

static void tick_for(uint32_t duration_ms, uint8_t mask)
{
    uint32_t elapsed;

    hal_stub_set_line_mask(mask);
    for (elapsed = 0u; elapsed < duration_ms; elapsed += 5u)
    {
        hal_delay_ms(5u);
        robot_app_tick(hal_millis());
    }
}

static void press_key(hal_pin_t pin)
{
    hal_stub_set_key(pin, true);
    tick_for(40u, 0x00u);
    hal_stub_set_key(pin, false);
    tick_for(40u, 0x00u);
}

int main(void)
{
    robot_telemetry_t tel;

    hal_init();
    robot_app_init();

    /* 在题图的白色 A 线上启动。 */
    tick_for(30u, 0x00u);
    press_key(HAL_PIN_KEY_START);

    /* 停留在起点时绝不能误判完成。 */
    tick_for(180u, 0x00u);
    tel = robot_app_get_telemetry();
    if (tel.phase != ROBOT_PHASE_Q2_LEAVE_A)
    {
        puts("FAIL: did not remain in LEAVE_A");
        return 1;
    }

    /* 驶离白线并稳定看到正常黑线，随后跑过最短圈时。 */
    tick_for(150u, 0x06u);
    tick_for(4800u, 0x06u);

    /* 白线脉冲不足确认时间，不能停车。 */
    tick_for(15u, 0x00u);
    tick_for(50u, 0x06u);
    tel = robot_app_get_telemetry();
    if (tel.state != ROBOT_STATE_RUNNING ||
        tel.phase != ROBOT_PHASE_Q2_FOLLOW)
    {
        puts("FAIL: short marker pulse caused a stop");
        return 2;
    }

    /* 回到 A：白线稳定确认，主动刹车后锁存完成时间。 */
    tick_for(45u, 0x00u);
    tick_for(180u, 0x00u);
    tel = robot_app_get_telemetry();

    printf("final mode=%d state=%d phase=%d time=%lu valid=%d\n",
           (int)tel.mode,
           (int)tel.state,
           (int)tel.phase,
           (unsigned long)tel.result_time_ms,
           tel.result_valid ? 1 : 0);

    if (tel.state != ROBOT_STATE_FINISHED ||
        tel.phase != ROBOT_PHASE_Q2_DONE ||
        !tel.result_valid ||
        tel.left_pwm != 0 ||
        tel.right_pwm != 0)
    {
        puts("FAIL: lap did not finish safely");
        return 3;
    }

    return 0;
}
