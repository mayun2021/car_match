/**
 * @file mpu6050.h
 * @brief MPU6050 陀螺仪/加速度计驱动接口。
 *
 * 本赛题中 MPU6050 主要用于记录车辆启动时的初始角度，并用 Z 轴陀螺积分
 * 得到相对偏航角，便于调试和报告分析。
 */

#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool online;
    float gyro_z_bias_dps;
    float yaw_deg;
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu6050_state_t;

/**
 * @brief 初始化 MPU6050 并执行一次陀螺零偏校准。
 *
 * @return true 表示设备在线且初始化成功。
 */
bool mpu6050_init(void);

/**
 * @brief 静止状态下采样陀螺仪 Z 轴，计算零偏。
 *
 * @return true 表示校准成功。
 */
bool mpu6050_calibrate_gyro(void);

/**
 * @brief 读取 MPU6050 并更新相对偏航角。
 *
 * @param dt_ms 距离上一次更新的时间，单位 ms。
 * @return true 表示本次读取成功。
 */
bool mpu6050_update(uint32_t dt_ms);

/**
 * @brief 获取 MPU6050 当前状态快照。
 *
 * @return 包含在线标志、原始数据、零偏和 yaw 角的结构体。
 */
mpu6050_state_t mpu6050_get_state(void);

/**
 * @brief 将当前相对偏航角清零。
 */
void mpu6050_zero_yaw(void);

#endif
