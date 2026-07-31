/**
 * @file simulate_control.c
 * @brief 桌面仿真入口。
 *
 * 该程序模拟按键、巡线传感器和 K230 数据，用于在没有 MSPM0 硬件时先验证
 * 应用状态机、巡线 PID 调用路径和停车线识别逻辑。
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
 * @brief 仿真程序主入口。
 *
 * @return 0 表示模拟任务完成，1 表示状态机未按预期完成。
 */
int main(void)
{
    static const uint8_t line_masks[] = {
        0x06u, /* 中间两个探头压线 */
        0x02u, /* 略偏左 */
        0x04u, /* 略偏右 */
        0x08u, /* 黑线在最右 */
        0x01u, /* 黑线在最左 */
        0x0Fu  /* A 点停车线 */
    };

    hal_init();
    robot_app_init();

    puts("simulate: start line-following mode");
    press_key(HAL_PIN_KEY_START);

    for (unsigned step = 0; step < 1400u; ++step)
    {
        if (step < 1100u)
        {
            hal_stub_set_line_mask(line_masks[step % 5u]);
        }
        else
        {
            hal_stub_set_line_mask(line_masks[5]);
        }

        if ((step % 10u) == 0u)
        {
            hal_stub_push_k230_text("B:0.0\n");
        }

        hal_delay_ms(5u);
        robot_app_tick(hal_millis());
    }

    const robot_telemetry_t tel = robot_app_get_telemetry();
    printf("simulate: final mode=%d state=%d left=%d right=%d servo=%u\n",
           (int)tel.mode,
           (int)tel.state,
           tel.left_pwm,
           tel.right_pwm,
           tel.servo_us);
    return tel.state == ROBOT_STATE_FINISHED ? 0 : 1;
}
