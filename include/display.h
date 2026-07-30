/**
 * @file display.h
 * @brief 调试显示/串口输出接口。
 *
 * 本模块把当前模式、巡线误差、钢球位置、电机 PWM、舵机脉宽等运行状态
 * 输出到调试通道。真实比赛时可以对接 OLED、串口屏或普通调试串口。
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "robot_types.h"

/**
 * @brief 初始化显示/调试输出模块。
 *
 * 当前实现会输出一行启动提示；如果换成 OLED，可在这里完成屏幕初始化。
 */
void display_init(void);

/**
 * @brief 周期性刷新显示内容。
 *
 * @param telemetry 机器人当前遥测数据，包含模式、状态、传感器和执行器输出。
 */
void display_update(const robot_telemetry_t *telemetry);

#endif
