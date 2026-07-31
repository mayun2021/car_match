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
    /*
     * 第 1 问为 K230 图像发送/环外录像，上电即运行，不占用车端按键模式。
     * 当前比赛固件只开放第 2、3 问，避免现场误选尚未验收的组合模式。
     */
    ROBOT_MODE_Q2_LAP = 2,
    ROBOT_MODE_Q3_BALL = 3
} robot_mode_t;

typedef enum
{
    ROBOT_STATE_IDLE = 0,
    ROBOT_STATE_RUNNING,
    ROBOT_STATE_FINISHED,
    ROBOT_STATE_ERROR
} robot_state_t;

typedef enum
{
    ROBOT_PHASE_READY = 0,
    ROBOT_PHASE_Q2_LEAVE_A,
    ROBOT_PHASE_Q2_FOLLOW,
    ROBOT_PHASE_Q2_BRAKE,
    ROBOT_PHASE_Q2_DONE,
    ROBOT_PHASE_Q3_WAIT_ACK,
    ROBOT_PHASE_Q3_TO_POS,
    ROBOT_PHASE_Q3_TO_NEG,
    ROBOT_PHASE_Q3_HOLD_NEG,
    ROBOT_PHASE_Q3_DONE,
    ROBOT_PHASE_FAULT
} robot_phase_t;

typedef enum
{
    ROBOT_FAULT_NONE = 0,
    ROBOT_FAULT_Q2_LEAVE_A_TIMEOUT,
    ROBOT_FAULT_Q2_LINE_LOST,
    ROBOT_FAULT_Q2_TIMEOUT,
    ROBOT_FAULT_Q2_SPIN,
    ROBOT_FAULT_Q2_IMU_REQUIRED,
    ROBOT_FAULT_Q2_START_UNSTABLE,
    ROBOT_FAULT_Q3_NO_ACK,
    ROBOT_FAULT_Q3_VISION,
    ROBOT_FAULT_Q3_TIMEOUT,
    ROBOT_FAULT_Q3_REMOTE
} robot_fault_t;

typedef struct
{
    uint32_t now_ms;
    robot_mode_t mode;
    robot_state_t state;
    robot_phase_t phase;
    robot_fault_t fault;
    float line_error;
    float ball_x_mm;
    float ball_target_mm;
    float ball_error_mm;
    float yaw_deg;
    float yaw_rate_dps;
    int16_t left_pwm;
    int16_t right_pwm;
    uint16_t servo_us;
    bool line_valid;
    bool vision_valid;
    bool k230_link;
    bool q2_marker_locked;
    bool q2_marker_black;
    uint8_t line_mask;        /* 四路红外掩码，bit0=最左，bit3=最右 */
    uint32_t run_elapsed_ms;  /* 本次任务已运行时间；完成/停止后定格 */
    uint32_t result_time_ms;  /* 完成时锁存，直到清除结果或切换模式 */
    bool result_valid;
} robot_telemetry_t;

#endif
