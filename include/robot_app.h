/**
 * @file robot_app.h
 * @brief 机器人应用层总状态机接口。
 *
 * 应用层负责按键处理、模式切换、任务开始/结束、巡线闭环和滚球闭环的组合调度。
 */

#ifndef ROBOT_APP_H
#define ROBOT_APP_H

#include <stdint.h>
#include "robot_types.h"

/**
 * @brief 初始化机器人应用、驱动和 PID 控制器。
 */
void robot_app_init(void);

/**
 * @brief 机器人主循环周期任务。
 *
 * @param now_ms 当前系统毫秒时间。函数内部会按 ROBOT_CONTROL_PERIOD_MS 节流。
 */
void robot_app_tick(uint32_t now_ms);

/**
 * @brief 获取当前遥测数据。
 *
 * @return 当前模式、状态、传感器值和执行器输出。
 */
robot_telemetry_t robot_app_get_telemetry(void);

#endif
