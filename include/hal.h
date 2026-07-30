/**
 * @file hal.h
 * @brief 硬件抽象层接口。
 *
 * 应用层、算法层和驱动层只依赖这些通用接口，不直接调用 TI DriverLib。
 * 移植到真实 MSPM0 板卡时，只需要实现本文件声明的函数即可。
 */

#ifndef HAL_H
#define HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * 板级抽象层。
 *
 * 应用和驱动只调用这里的函数，不直接依赖 TI DriverLib。
 * 移植到真实 MSPM0 工程时，只需要把 src/platform/hal_ti_mspm0.c
 * 里的函数对接到 SysConfig 生成的 GPIO/PWM/I2C/UART 名称。
 */

typedef enum
{
    HAL_PIN_LINE_L2 = 0,      /* PB2，最左循迹 */
    HAL_PIN_LINE_L1,          /* PB3，左中循迹 */
    HAL_PIN_LINE_R1,          /* PB4，右中循迹 */
    HAL_PIN_LINE_R2,          /* PB5，最右循迹 */

    HAL_PIN_MOTOR_AIN1,       /* PA24 */
    HAL_PIN_MOTOR_AIN2,       /* PA28 */
    HAL_PIN_MOTOR_BIN1,       /* PA22 */
    HAL_PIN_MOTOR_BIN2,       /* PA14 */
    HAL_PIN_MOTOR_STBY,       /* PA25 */

    HAL_PIN_KEY_MODE,         /* PA26 */
    HAL_PIN_KEY_START,        /* PA27 */
    HAL_PIN_KEY_CALIB,        /* PA1，避开 PA25 冲突 */

    HAL_PIN_COUNT
} hal_pin_t;

typedef enum
{
    HAL_PWM_MOTOR_LEFT = 0,   /* PA8 / PWMA */
    HAL_PWM_MOTOR_RIGHT,      /* PA9 / PWMB */
    HAL_PWM_SERVO,            /* PA7 / MG996R */
    HAL_PWM_COUNT
} hal_pwm_t;

/**
 * @brief 初始化主控板所有底层外设。
 *
 * 包括 GPIO、PWM、I2C、UART、系统滴答定时器等。
 */
void hal_init(void);

/**
 * @brief 获取系统上电后的毫秒计数。
 *
 * @return 当前毫秒计数，允许自然溢出，应用层使用无符号差值计算时间间隔。
 */
uint32_t hal_millis(void);

/**
 * @brief 阻塞延时指定毫秒数。
 *
 * @param ms 延时时间，单位 ms。
 */
void hal_delay_ms(uint32_t ms);

/**
 * @brief 设置一个 GPIO 输出电平。
 *
 * @param pin 抽象引脚编号。
 * @param high true 输出高电平，false 输出低电平。
 */
void hal_gpio_write(hal_pin_t pin, bool high);

/**
 * @brief 读取一个 GPIO 输入电平。
 *
 * @param pin 抽象引脚编号。
 * @return true 表示高电平，false 表示低电平。
 */
bool hal_gpio_read(hal_pin_t pin);

/**
 * @brief 设置电机 PWM 占空比。
 *
 * @param pwm PWM 通道。
 * @param permille 占空比千分比，0 表示 0%，1000 表示 100%。
 */
void hal_pwm_set_duty_permille(hal_pwm_t pwm, uint16_t permille);

/**
 * @brief 设置舵机 PWM 脉宽。
 *
 * @param pwm PWM 通道，通常为 HAL_PWM_SERVO。
 * @param pulse_us 高电平脉宽，单位 us。
 */
void hal_pwm_set_servo_us(hal_pwm_t pwm, uint16_t pulse_us);

/**
 * @brief I2C 写数据。
 *
 * @param addr 7 位 I2C 从机地址。
 * @param data 待写入数据缓冲区。
 * @param len 待写入字节数。
 * @return true 表示写入成功，false 表示通信失败。
 */
bool hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len);

/**
 * @brief I2C 先写后读，常用于读取寄存器。
 *
 * @param addr 7 位 I2C 从机地址。
 * @param tx 写入缓冲区，通常是寄存器地址。
 * @param tx_len 写入字节数。
 * @param rx 读取缓冲区。
 * @param rx_len 读取字节数。
 * @return true 表示通信成功，false 表示通信失败。
 */
bool hal_i2c_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len);

/**
 * @brief 非阻塞读取 K230 串口的一个字节。
 *
 * @return 0..255 表示读到的字节；-1 表示当前没有新数据。
 */
int hal_uart_k230_read_byte(void);

/**
 * @brief 输出调试字符串。
 *
 * @param text 以 '\0' 结尾的字符串。
 */
void hal_uart_debug_write(const char *text);

#endif
