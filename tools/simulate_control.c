/**
 * @file simulate_control.c
 * @brief 桌面仿真入口。
 *
 * 该程序模拟按键、巡线传感器和陀螺仪角速度，用于在没有 MSPM0 硬件时先
 * 验证应用状态机、巡线 PID 调用路径和"跑完一圈"角度判定逻辑。
 */

#include "hal.h"
#include "line_sensor.h"
#include "robot_app.h"
#include "robot_types.h"

#include <stdio.h>

/**
 * @brief 设置仿真巡线黑线掩码。
 *
 * @param mask bit0=最左探头，bit3=最右探头。
 */
void hal_stub_set_line_mask(uint8_t mask);

/**
 * @brief 设置仿真按键状态。
 *
 * @param pin 按键抽象引脚。
 * @param pressed true 表示按下。
 */
void hal_stub_set_key(hal_pin_t pin, bool pressed);

/**
 * @brief 向仿真 K230 串口注入文本。
 *
 * @param text 待注入的文本帧。
 */
void hal_stub_push_k230_text(const char *text);

/**
 * @brief 设置仿真 MPU6050 陀螺仪 Z 轴角速度，用于驱动偏航角积分。
 *
 * @param dps 角速度，单位 度/秒。
 */
void hal_stub_set_gyro_z_dps(float dps);

/**
 * @brief 模拟一次按键短按。
 *
 * @param pin 要模拟的按键引脚。
 */
static void press_key(hal_pin_t pin)
{
    hal_stub_set_key(pin, true);
    for (int i = 0; i < 8; ++i)
    {
        hal_delay_ms(5);
        robot_app_tick(hal_millis());
    }

    hal_stub_set_key(pin, false);
    for (int i = 0; i < 8; ++i)
    {
        hal_delay_ms(5);
        robot_app_tick(hal_millis());
    }
}

/**
 * @brief 按给定黑线掩码持续推进若干毫秒，并周期性模拟 OLED 刷新阻塞。
 *
 * 真实硬件上 OLED 每 250 ms 整屏刷新一次，走的是软件位带 I2C，单次刷新
 * 会阻塞主循环几十毫秒。这里每 250 ms 模拟时间就额外多阻塞 55 ms，
 * 用来回归验证陀螺仪积分不会因为控制环 dt 限幅而系统性地跑慢
 * （回归此前"转一圈要转一圈半才停"的 bug）。
 *
 * @param duration_ms 推进时长，单位 ms（不含额外插入的阻塞时间）。
 * @param mask 巡线黑线掩码。
 */
static void tick_for(uint32_t duration_ms, uint8_t mask)
{
    static uint32_t s_since_oled_flush_ms;
    uint32_t elapsed;

    hal_stub_set_line_mask(mask);
    for (elapsed = 0u; elapsed < duration_ms; elapsed += 5u)
    {
        hal_delay_ms(5u);
        s_since_oled_flush_ms += 5u;
        if (s_since_oled_flush_ms >= 250u)
        {
            s_since_oled_flush_ms = 0u;
            hal_delay_ms(55u); /* 模拟一次 OLED 整屏刷新阻塞 */
        }
        robot_app_tick(hal_millis());
    }
}

/**
 * @brief 仿真程序主入口。
 *
 * @return 0 表示状态机按预期完成一圈并停车，1 表示未按预期完成。
 */
int main(void)
{
    robot_telemetry_t tel;

    hal_init();
    robot_app_init();
    hal_stub_set_gyro_z_dps(0.0f);

    /* 在正常压线状态（中间两探头，MASK=0x06）下按 START 起步。 */
    hal_stub_set_line_mask(0x06u);
    puts("simulate: start line-following mode");
    press_key(HAL_PIN_KEY_START);

    tel = robot_app_get_telemetry();
    if (tel.state != ROBOT_STATE_RUNNING)
    {
        puts("FAIL: did not start");
        return 1;
    }

    /*
     * 用恒定 40 dps 角速度模拟绕场一圈：起步 700 ms 内偏航仍远低于防
     * 原地打转门槛（35°），约 8.75 s 后偏航角越过
     * ROBOT_LINE_FINISH_YAW_DEG（350°）。
     */
    hal_stub_set_gyro_z_dps(40.0f);

    /* 转到约一半（角度约 200°）时，纯角度判定绝不能提前停车。 */
    tick_for(5000u, 0x06u);
    tel = robot_app_get_telemetry();
    if (tel.state != ROBOT_STATE_RUNNING)
    {
        puts("FAIL: stopped before completing the lap");
        return 2;
    }

    /* 继续转过 350°（无需任何视觉停车线标记），应自动停车。 */
    tick_for(4500u, 0x06u);

    tel = robot_app_get_telemetry();
    printf("simulate: final mode=%d state=%d yaw=%.1f left=%d right=%d servo=%u\n",
           (int)tel.mode,
           (int)tel.state,
           (double)tel.yaw_deg,
           tel.left_pwm,
           tel.right_pwm,
           tel.servo_us);

    if (tel.state != ROBOT_STATE_FINISHED ||
        tel.left_pwm != 0 ||
        tel.right_pwm != 0)
    {
        puts("FAIL: lap did not finish safely");
        return 3;
    }

    return 0;
}
