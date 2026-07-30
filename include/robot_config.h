/**
 * @file robot_config.h
 * @brief H 题小车软件的集中配置文件。
 *
 * 比赛现场调参时优先修改本文件，例如巡线速度、PID 参数、舵机范围、
 * 黑线有效电平、K230 超时时间等。
 */

#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/*
 * 全局配置文件。
 *
 * 比赛现场最常改的参数都集中放在这里：
 * - 引脚逻辑电平
 * - 电机方向
 * - PID 参数
 * - 速度/舵机限幅
 * 这样调车时不用在多个源文件里来回找。
 */

#include <stdint.h>

/* 控制主循环周期。5 ms 对巡线和舵机控制都比较够用。 */
#define ROBOT_CONTROL_PERIOD_MS        5u

/* 四路红外模块检测到黑线时的电平。多数循迹模块黑线输出低电平。 */
#define ROBOT_LINE_BLACK_LEVEL         0u

/* 按键按下电平。图纸为外接下拉/建议内部上拉，因此默认低电平按下。 */
#define ROBOT_KEY_PRESSED_LEVEL        0u
#define ROBOT_KEY_DEBOUNCE_MS          30u

/* 电机输出范围为 -1000 到 +1000。 */
#define ROBOT_MOTOR_PWM_MAX            1000
#define ROBOT_MOTOR_RAMP_STEP          30

/* 如果左右轮方向与预期相反，分别切换下面两个宏。 */
#define ROBOT_LEFT_MOTOR_REVERSE       0
#define ROBOT_RIGHT_MOTOR_REVERSE      0

/* 模式一/三巡线参数。先保守，跑稳后再提速。 */
#define ROBOT_LINE_BASE_SPEED          320
#define ROBOT_LINE_SEARCH_SPEED        220
#define ROBOT_LINE_KP                  0.62f
#define ROBOT_LINE_KI                  0.00f
#define ROBOT_LINE_KD                  0.18f
#define ROBOT_LINE_CORRECTION_LIMIT    450.0f

/* 车辆起步后忽略停车线的时间，避免刚从 A 点启动就误判完成。 */
#define ROBOT_START_LINE_IGNORE_MS     1500u

/* 一圈任务最大运行时间保护。 */
#define ROBOT_LINE_TASK_TIMEOUT_MS     30000u

/* MG996R 舵机参数。50 Hz PWM，常用有效脉宽约 500-2500 us。 */
#define ROBOT_SERVO_CENTER_US          1500
#define ROBOT_SERVO_MIN_US             700
#define ROBOT_SERVO_MAX_US             2300
#define ROBOT_SERVO_MAX_DELTA_US       420

/*
 * 舵机方向。
 * 如果钢球偏右时控制后更偏右，说明方向反了，把该宏从 0 改为 1。
 */
#define ROBOT_BALL_SERVO_REVERSE       0

/* 模式二/三滚球位置闭环参数。单位输入为 mm。 */
#define ROBOT_BALL_KP                  8.0f
#define ROBOT_BALL_KI                  0.02f
#define ROBOT_BALL_KD                  2.4f
#define ROBOT_BALL_OUTPUT_LIMIT_US     380.0f

/* 视觉超过该时间没有有效数据，判定为丢球。 */
#define ROBOT_VISION_TIMEOUT_MS        200u

/* 模式二演示任务：先到 +5 cm，再到 -5 cm。 */
#define ROBOT_BALL_TARGET_POS_MM       50.0f
#define ROBOT_BALL_TARGET_NEG_MM       -50.0f
#define ROBOT_BALL_TARGET_TOLERANCE_MM 10.0f
#define ROBOT_BALL_STABLE_MS           500u
#define ROBOT_BALL_STAGE_TIMEOUT_MS    5000u

/* K230 串口默认波特率。 */
#define ROBOT_K230_BAUDRATE            115200u

/* MPU6050 地址。AD0 接地时为 0x68。 */
#define ROBOT_MPU6050_ADDR             0x68u
#define ROBOT_MPU_CALIB_SAMPLES        300u

#endif
