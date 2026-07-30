/**
 * @file display.h
 * @brief 调试显示/OLED 输出接口。
 *
 * 本模块把当前模式、巡线误差、钢球位置、电机 PWM、舵机脉宽等运行状态
 * 同时输出到调试串口和 0.96 寸 SSD1306 OLED（128x64，I2C，见 oled.h）。
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "robot_types.h"

/**
 * @brief 初始化显示/调试输出模块。
 *
 * 输出一行调试串口启动提示，并完成 OLED 初始化。
 */
void display_init(void);

/**
 * @brief 周期性刷新显示内容。
 *
 * @param telemetry 机器人当前遥测数据，包含模式、状态、传感器和执行器输出。
 */
void display_update(const robot_telemetry_t *telemetry);

#endif
