/**
 * @file line_sensor.c
 * @brief 四路红外循迹采样与误差计算实现。
 *
 * 通过 PB2-PB5 读取 YB-MVX01 四路红外模块，按左负右正的权重计算巡线误差。
 */

#include "line_sensor.h"
#include "hal.h"
#include "robot_config.h"

static float s_last_error;
static uint8_t s_black_history[4];

/**
 * @brief 初始化循迹模块内部状态。
 */
void line_sensor_init(void)
{
    s_last_error = 0.0f;
    for (uint8_t i = 0u; i < 4u; ++i)
    {
        s_black_history[i] = 0u;
    }
}

/**
 * @brief 读取四路红外探头并计算黑线位置。
 *
 * @return 当前循迹采样结果，包含掩码、有效性、全黑判断和连续误差值。
 */
line_sample_t line_sensor_read(void)
{
    static const int16_t weights[4] = {-3, -1, 1, 3};
    static const hal_pin_t pins[4] = {
        HAL_PIN_LINE_L2,
        HAL_PIN_LINE_L1,
        HAL_PIN_LINE_R1,
        HAL_PIN_LINE_R2,
    };
    line_sample_t sample = {0};
    int16_t weighted_sum = 0;

    for (uint8_t i = 0; i < 4; ++i)
    {
        const bool level = hal_gpio_read(pins[i]);
        const bool raw_black = (level ? 1u : 0u) == ROBOT_LINE_BLACK_LEVEL;
        uint8_t history;
        uint8_t black_votes;

        /*
         * 三次采样多数表决，抑制模块临界高度、电位器阈值和电机干扰
         * 引起的单周期跳变。5 ms 控制周期下只增加约 10 ms 延迟。
         */
        s_black_history[i] =
            (uint8_t)(((s_black_history[i] << 1) | (raw_black ? 1u : 0u)) & 0x07u);
        history = s_black_history[i];
        black_votes = (uint8_t)((history & 0x01u) +
                                ((history >> 1) & 0x01u) +
                                ((history >> 2) & 0x01u));
        sample.black[i] = black_votes >= 2u;

        if (sample.black[i])
        {
            sample.mask |= (uint8_t)(1u << i);
            sample.count++;
            weighted_sum += weights[i];
        }
    }

    sample.valid = sample.count > 0u;
    sample.all_black = sample.count == 4u;

    if (sample.valid)
    {
        /*
         * 4 路传感器只能提供离散位置，这里用加权平均变成连续误差：
         * -1000 表示黑线在最左侧，+1000 表示黑线在最右侧。
         */
        sample.error = ((float)weighted_sum / (float)sample.count) * (1000.0f / 3.0f);
        s_last_error = sample.error;
    }
    else
    {
        /*
         * 丢线时保留上一次方向，应用层会按该方向低速找线。
         * 这比直接停车更适合圆角弯道和短暂抖动。
         */
        sample.error = s_last_error;
    }

    return sample;
}
