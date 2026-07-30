/**
 * @file pid.c
 * @brief 通用 PID 控制器实现。
 *
 * 本文件提供带积分限幅和输出限幅的 PID 算法，用于巡线差速控制和滚球舵机控制。
 */

#include "pid.h"

/**
 * @brief 将浮点值限制到指定范围。
 *
 * @param value 待限制的输入值。
 * @param min_value 允许的最小值。
 * @param max_value 允许的最大值。
 * @return 限幅后的结果。
 */
static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

/**
 * @brief 初始化 PID 控制器参数并清空内部状态。
 *
 * @param pid PID 控制器对象。
 * @param kp 比例系数。
 * @param ki 积分系数。
 * @param kd 微分系数。
 * @param output_limit 输出绝对值限幅。
 * @param integral_limit 积分累计绝对值限幅。
 */
void pid_init(pid_t *pid, float kp, float ki, float kd, float output_limit, float integral_limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = output_limit;
    pid->integral_limit = integral_limit;
    pid_reset(pid);
}

/**
 * @brief 清空 PID 控制器的历史误差和积分。
 *
 * @param pid PID 控制器对象。
 */
void pid_reset(pid_t *pid)
{
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->has_prev = false;
}

/**
 * @brief 根据当前误差计算一次 PID 输出。
 *
 * @param pid PID 控制器对象。
 * @param error 当前控制误差。
 * @param dt_s 本次控制周期，单位 s。
 * @return 限幅后的控制量。
 */
float pid_update(pid_t *pid, float error, float dt_s)
{
    float derivative = 0.0f;

    if (dt_s <= 0.0001f)
    {
        dt_s = 0.0001f;
    }

    pid->integral += error * dt_s;
    pid->integral = clampf(pid->integral, -pid->integral_limit, pid->integral_limit);

    if (pid->has_prev)
    {
        derivative = (error - pid->prev_error) / dt_s;
    }
    else
    {
        pid->has_prev = true;
    }

    pid->prev_error = error;

    return clampf(pid->kp * error + pid->ki * pid->integral + pid->kd * derivative,
                  -pid->output_limit,
                  pid->output_limit);
}
