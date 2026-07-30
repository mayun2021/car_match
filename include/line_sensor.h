/**
 * @file line_sensor.h
 * @brief 四路红外循迹模块读取与误差计算接口。
 *
 * 传感器从左到右连接 PB2、PB3、PB4、PB5。模块会把 4 路离散信号转换成
 * 适合 PID 的连续巡线误差。
 */

#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool black[4];          /* 从左到右：PB2/PB3/PB4/PB5 */
    uint8_t mask;           /* bit0=最左，bit3=最右 */
    uint8_t count;          /* 检测到黑线的探头数量 */
    bool valid;             /* 是否看到了黑线 */
    bool all_black;         /* 常用于识别 A 点停车线 */
    float error;            /* 归一化位置误差，左负右正，范围约 -1000 到 1000 */
} line_sample_t;

/**
 * @brief 初始化循迹模块状态。
 *
 * 当前主要用于清除上一次误差，真实硬件 GPIO 初始化在 HAL 层完成。
 */
void line_sensor_init(void);

/**
 * @brief 读取四路红外传感器并计算巡线误差。
 *
 * @return line_sample_t，包含黑线掩码、是否有效、是否全黑和误差值。
 */
line_sample_t line_sensor_read(void);

#endif
