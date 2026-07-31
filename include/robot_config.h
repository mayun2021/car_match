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

/*
 * 固件版本号，显示在 OLED 第一行右侧，方便现场一眼确认板子上跑的是
 * 哪个版本。每次发布一个值得留痕的版本（调完一批参数、修完一个坑）
 * 就改这一个地方，不用去改 display.c 里的格式字符串。
 */
#define ROBOT_FIRMWARE_VERSION         "V3"

/* 控制主循环周期。5 ms 对巡线和舵机控制都比较够用。 */
#define ROBOT_CONTROL_PERIOD_MS        5u

/*
 * 陀螺仪积分允许使用的最大真实 dt。OLED 整屏刷新（软件位带 I2C）偶尔
 * 会阻塞主循环几十毫秒，这段时间对应的真实转角不能丢，所以陀螺仪积分
 * 用真实 dt 而不是控制环限幅后的 dt；这里只挡真正卡死（远超一次 OLED
 * 刷新耗时）的极端情况，避免单次异常大 dt 积出一个离谱的角度。
 */
#define ROBOT_GYRO_MAX_DT_MS           300u

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
 * 模式一/三巡线参数，按当前大尺寸、高重心车架采用低速稳态值。
 *
 * 四路数字传感器的误差会以约 333 为一级跳变。原来的 KD=0.18 在
 * 5 ms 周期下会产生上万的微分冲击并立即打满输出，因此本版先用
 * 稳妥的纯 P 控制。修正量严格小于基础速度，正常循迹绝不反转车轮。
 */
#define ROBOT_LINE_BASE_SPEED          220
#define ROBOT_LINE_SEARCH_FAST_SPEED   180
#define ROBOT_LINE_SEARCH_SLOW_SPEED    80
#define ROBOT_LINE_KP                  0.24f
#define ROBOT_LINE_KI                  0.00f
#define ROBOT_LINE_KD                  0.00f
#define ROBOT_LINE_CORRECTION_LIMIT    140.0f

/* 启动时必须已经压住黑线；运行中丢线只允许短时缓弯，随后安全停车。 */
#define ROBOT_LINE_START_MAX_ERROR     400.0f
#define ROBOT_LINE_LOST_STOP_MS        350u

/* 启动后若短时间内偏航过大，判定方向/接线异常并立即停车。 */
#define ROBOT_START_SPIN_GUARD_MS      700u
#define ROBOT_START_SPIN_MAX_DEG       35.0f

/* 车辆起步后忽略跑完一圈判定的时间，避免刚起步的姿态噪声误判完成。 */
#define ROBOT_START_LINE_IGNORE_MS     1500u

/*
 * 跑完一圈的判定：MPU6050 从起步时清零偏航角，纯靠角度积分判断，不再
 * 依赖停车线视觉标记（原来的四探头全黑判定）。起始角度为 0，转过本角度
 * 即视为跑完一圈，直接停车。
 */
#define ROBOT_LINE_FINISH_YAW_DEG      350.0f

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

/*
 * OLED 显示屏 I2C 地址。0.96 寸 SSD1306 模块多数为 0x3C，
 * 少数为 0x3D，屏幕不亮时优先检查这个宏。与 MPU6050 共用
 * PA15/PA16 这一条 I2C 总线，靠地址区分设备。
 */
#define ROBOT_OLED_I2C_ADDR            0x3Cu

#endif
