/**
 * @file robot_types.h
 * @brief 全工程共享的数据类型定义。
 *
 * 这里放模式枚举、状态枚举和遥测结构体，避免各模块重复定义。
 */

#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ROBOT_MODE_LINE = 1,      /* 只巡线 */
    ROBOT_MODE_BALL = 2,      /* 只控球 */
    ROBOT_MODE_COMBINED = 3   /* 巡线 + 控球 */
} robot_mode_t;

typedef enum
{
    ROBOT_STATE_IDLE = 0,
    ROBOT_STATE_RUNNING,
    ROBOT_STATE_FINISHED,
    ROBOT_STATE_ERROR
} robot_state_t;

typedef struct
{
    uint32_t now_ms;
    robot_mode_t mode;
    robot_state_t state;
    float line_error;
    float ball_x_mm;
    float ball_target_mm;
    float yaw_deg;
    int16_t left_pwm;
    int16_t right_pwm;
    uint16_t servo_us;
    bool line_valid;
    bool vision_valid;
} robot_telemetry_t;

#endif
