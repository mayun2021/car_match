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

/* 控制主循环周期。5 ms 对四路数字循迹足够，并给停车线消抖留出余量。 */
#define ROBOT_CONTROL_PERIOD_MS        5u

/* 四路红外模块检测到黑线时的电平。多数循迹模块黑线输出低电平。 */
#define ROBOT_LINE_BLACK_LEVEL         0u

/* 按键按下电平。图纸为外接下拉/建议内部上拉，因此默认低电平按下。 */
#define ROBOT_KEY_PRESSED_LEVEL        0u
#define ROBOT_KEY_DEBOUNCE_MS          30u

/* 电机输出范围为 -1000 到 +1000。 */
#define ROBOT_MOTOR_PWM_MAX            1000
#define ROBOT_MOTOR_RAMP_STEP          10

/* 如果左右轮方向与预期相反，分别切换下面两个宏。 */
#define ROBOT_LEFT_MOTOR_REVERSE       0
#define ROBOT_RIGHT_MOTOR_REVERSE      0

/*
 * 第 2 问循线参数。
 *
 * 保留已经在实车上跑通的 220 / KP=0.24 / 最大修正 140。新高框架
 * 只在 MPU6050 判断为直线且探头居中时升到 245，弯道自动回到原速度。
 * 这样可缩短 1.5 m 直线段时间，同时不提高弯道侧倾风险。
 */
#define ROBOT_LINE_BASE_SPEED          220
#define ROBOT_LINE_STRAIGHT_SPEED      245
#define ROBOT_LINE_SEARCH_FAST_SPEED   180
#define ROBOT_LINE_SEARCH_SLOW_SPEED    80
#define ROBOT_LINE_KP                  0.24f
#define ROBOT_LINE_KI                  0.00f
#define ROBOT_LINE_KD                  0.00f
#define ROBOT_LINE_CORRECTION_LIMIT    140.0f

/* 运行中丢线只允许短时缓弯，随后安全停车。 */
#define ROBOT_LINE_LOST_STOP_MS        350u

/* 只有同时满足居中和低偏航角速度时，才使用直线加速值。 */
#define ROBOT_LINE_STRAIGHT_MAX_ERROR   80.0f
#define ROBOT_LINE_STRAIGHT_MAX_DPS     12.0f

/* 启动后若短时间内偏航过大，判定方向/接线异常并立即停车。 */
#define ROBOT_START_SPIN_GUARD_MS      700u
#define ROBOT_START_SPIN_MAX_DEG       35.0f

/*
 * 第 2 问：A 点起步/终点状态机。
 *
 * 题图标注 A 点为白色线，因此默认把全白 MASK=0 当作 A；若实物起点为宽黑线，
 * 程序会在 K3 启动时自动记住 MASK=15，并以同样的宽黑线作为终点。
 * 黑色横线默认要求四个探头全黑，避免弯道的三探头图样误触发。
 */
#define ROBOT_Q2_MARKER_MIN_BLACK       4u
#define ROBOT_Q2_DEFAULT_WHITE_MARKER   1u
#define ROBOT_Q2_START_VOTE_SAMPLES     16u
#define ROBOT_Q2_START_VOTE_MIN         12u
#define ROBOT_Q2_LEAVE_SPEED            210
#define ROBOT_Q2_LEAVE_CLEAR_MS          90u
#define ROBOT_Q2_LEAVE_TIMEOUT_MS      1200u
#define ROBOT_Q2_FINISH_MIN_MS         8000u
/* 300°起进入弯道降速段，350°视为跑完一圈，纯靠陀螺积分角度判定，不再依赖视觉终点标记。 */
#define ROBOT_Q2_MIN_LAP_YAW_DEG        300.0f
#define ROBOT_Q2_FINISH_YAW_DEG         350.0f
#define ROBOT_Q2_FINISH_APPROACH_SPEED   205
#define ROBOT_Q2_ACTIVE_BRAKE_MS         120u
#define ROBOT_Q2_TASK_TIMEOUT_MS       19800u

/*
 * 第 3 问由 K230 本地完成视觉 + 舵机闭环；MSPM0 只负责按键、OLED、
 * 总计时和急停。舵机已经接在 K230 IO42，禁止同时从 PA7 再跑一套 PID。
 */
#define ROBOT_Q3_TARGET_POS_MM          50
#define ROBOT_Q3_TARGET_NEG_MM         -50
#define ROBOT_Q3_ACK_RETRY_MS           200u
#define ROBOT_Q3_ACK_MAX_ATTEMPTS         3u
#define ROBOT_Q3_KEEPALIVE_MS            100u
#define ROBOT_Q3_START_MAX_ABS_MM        10.0f
#define ROBOT_Q3_LINK_TIMEOUT_MS         250u
#define ROBOT_Q3_TASK_TIMEOUT_MS        5000u
#define ROBOT_Q3_LOCAL_WATCHDOG_MS       5200u
#define ROBOT_Q3_HOLD_LINK_TIMEOUT_MS     500u
#define ROBOT_Q3_HOLD_ERROR_LIMIT_MM       10.0f
#define ROBOT_Q3_HOLD_ERROR_TIMEOUT_MS    500u

/*
 * 保留 PA7 舵机驱动的编译兼容值，但赛题状态机不再调用它。现物舵机信号
 * 只接 K230 IO42；PA7 必须悬空，不能与 IO42 并接。
 */
#define ROBOT_SERVO_CENTER_US           1500
#define ROBOT_SERVO_MIN_US              1300
#define ROBOT_SERVO_MAX_US              1700
#define ROBOT_SERVO_MAX_DELTA_US         200

/* K230 串口默认波特率。 */
#define ROBOT_K230_BAUDRATE            115200u

/* MPU6050 地址。AD0 接地时为 0x68。 */
#define ROBOT_MPU6050_ADDR             0x68u
#define ROBOT_MPU_CALIB_SAMPLES        300u

/*
 * OLED 显示屏 I2C 地址。0.96 寸 SSD1306 模块多数为 0x3C，
 * 少数为 0x3D，屏幕不亮时优先检查这个宏。与 MPU6050 共用
 * PA15/PA16 这一条 I2C 总线，靠地址区分设备。
 */
#define ROBOT_OLED_I2C_ADDR            0x3Cu

#endif
