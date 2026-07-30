/**
 * @file servo.h
 * @brief MG996R 舵机控制接口。
 *
 * 舵机使用 50 Hz PWM，通过改变高电平脉宽控制摆杆/水管倾角。
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

/**
 * @brief 初始化舵机并回到中位。
 */
void servo_init(void);

/**
 * @brief 直接设置舵机 PWM 脉宽。
 *
 * @param pulse_us 舵机脉宽，单位 us，会自动限制在安全范围内。
 */
void servo_set_pulse_us(uint16_t pulse_us);

/**
 * @brief 以中位为基准设置舵机偏移。
 *
 * @param delta_us 相对 ROBOT_SERVO_CENTER_US 的脉宽偏移，单位 us。
 */
void servo_set_delta_us(int16_t delta_us);

/**
 * @brief 舵机回到机械中位。
 */
void servo_center(void);

/**
 * @brief 获取当前舵机脉宽。
 *
 * @return 当前输出脉宽，单位 us。
 */
uint16_t servo_get_pulse_us(void);

#endif
