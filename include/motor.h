/**
 * @file motor.h
 * @brief TB6612FNG 双路直流电机驱动接口。
 *
 * 左右 MG310 减速电机使用 -1000 到 +1000 的速度命令。
 * 正负号表示方向，绝对值表示 PWM 占空比大小。
 */

#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

/**
 * @brief 初始化电机驱动状态并使能 TB6612。
 */
void motor_init(void);

/**
 * @brief 设置左右轮原始速度命令。
 *
 * @param left 左轮速度，范围 -1000 到 +1000。
 * @param right 右轮速度，范围 -1000 到 +1000。
 */
void motor_set_raw(int16_t left, int16_t right);

/**
 * @brief 停止左右电机输出。
 */
void motor_stop(void);

/**
 * @brief 获取当前左轮 PWM 命令。
 *
 * @return 左轮当前速度命令。
 */
int16_t motor_get_left(void);

/**
 * @brief 获取当前右轮 PWM 命令。
 *
 * @return 右轮当前速度命令。
 */
int16_t motor_get_right(void);

#endif
