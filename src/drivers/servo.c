/**
 * @file servo.c
 * @brief MG996R 舵机控制实现。
 *
 * 该模块负责把滚球 PID 输出转换成舵机 PWM 脉宽，并限制最大脉宽范围，
 * 防止机械结构被过大的舵机角度顶住。
 */

#include "servo.h"
#include "hal.h"
#include "robot_config.h"

static uint16_t s_servo_us;

/**
 * @brief 限制舵机脉宽到安全范围。
 *
 * @param value 输入脉宽，单位 us。
 * @return 限幅后的脉宽，单位 us。
 */
static uint16_t clamp_servo_us(int32_t value)
{
    if (value < ROBOT_SERVO_MIN_US)
    {
        return ROBOT_SERVO_MIN_US;
    }
    if (value > ROBOT_SERVO_MAX_US)
    {
        return ROBOT_SERVO_MAX_US;
    }
    return (uint16_t)value;
}

/**
 * @brief 初始化舵机并回中。
 */
void servo_init(void)
{
    servo_center();
}

/**
 * @brief 设置舵机绝对脉宽。
 *
 * @param pulse_us 目标脉宽，单位 us。
 */
void servo_set_pulse_us(uint16_t pulse_us)
{
    s_servo_us = clamp_servo_us(pulse_us);
    hal_pwm_set_servo_us(HAL_PWM_SERVO, s_servo_us);
}

/**
 * @brief 设置相对中位的舵机偏移。
 *
 * @param delta_us 相对 ROBOT_SERVO_CENTER_US 的偏移，单位 us。
 */
void servo_set_delta_us(int16_t delta_us)
{
    if (delta_us > ROBOT_SERVO_MAX_DELTA_US)
    {
        delta_us = ROBOT_SERVO_MAX_DELTA_US;
    }
    else if (delta_us < -ROBOT_SERVO_MAX_DELTA_US)
    {
        delta_us = -ROBOT_SERVO_MAX_DELTA_US;
    }

    servo_set_pulse_us((uint16_t)(ROBOT_SERVO_CENTER_US + delta_us));
}

/**
 * @brief 舵机回到中位。
 */
void servo_center(void)
{
    servo_set_pulse_us(ROBOT_SERVO_CENTER_US);
}

/**
 * @brief 获取当前舵机脉宽。
 *
 * @return 当前舵机脉宽，单位 us。
 */
uint16_t servo_get_pulse_us(void)
{
    return s_servo_us;
}
