/**
 * @file pid.h
 * @brief 通用 PID 控制器接口。
 *
 * 巡线闭环和滚球闭环都使用同一个 PID 实现。输出和积分都带限幅，
 * 便于在比赛现场避免舵机/电机突然打满。
 */

#ifndef PID_H
#define PID_H

#include <stdbool.h>

typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral;
    float prev_error;
    float output_limit;
    float integral_limit;
    bool has_prev;
} pid_t;

/**
 * @brief 初始化 PID 参数和内部状态。
 *
 * @param pid PID 控制器对象。
 * @param kp 比例系数。
 * @param ki 积分系数。
 * @param kd 微分系数。
 * @param output_limit 输出绝对值限幅。
 * @param integral_limit 积分项绝对值限幅。
 */
void pid_init(pid_t *pid, float kp, float ki, float kd, float output_limit, float integral_limit);

/**
 * @brief 清空 PID 内部积分和历史误差。
 *
 * @param pid PID 控制器对象。
 */
void pid_reset(pid_t *pid);

/**
 * @brief 根据误差和周期计算 PID 输出。
 *
 * @param pid PID 控制器对象。
 * @param error 当前误差。
 * @param dt_s 控制周期，单位 s。
 * @return 限幅后的控制输出。
 */
float pid_update(pid_t *pid, float error, float dt_s);

#endif
