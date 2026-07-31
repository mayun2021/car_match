/**
 * @file hal_stub.c
 * @brief 桌面仿真用硬件抽象层实现。
 *
 * 这个文件不控制真实硬件，只在电脑上模拟 GPIO、PWM、UART 和时间，
 * 用于检查控制算法、状态机和协议解析是否能正常编译运行。
 */

#include "hal.h"
#include "robot_config.h"

#include <stdio.h>
#include <string.h>

static bool s_gpio[HAL_PIN_COUNT];
static uint16_t s_pwm[HAL_PWM_COUNT];
static uint32_t s_now_ms;
static char s_uart_rx[256];
static size_t s_uart_rx_read;
static size_t s_uart_rx_write;
/* MPU6050 仿真陀螺仪 Z 轴角速度，单位 dps，供测试驱动偏航角积分。 */
static int16_t s_mpu_gyro_z_raw;

/**
 * @brief 设置仿真 MPU6050 陀螺仪 Z 轴角速度，用于驱动偏航角积分测试。
 *
 * @param dps 角速度，单位 度/秒。
 */
void hal_stub_set_gyro_z_dps(float dps)
{
    s_mpu_gyro_z_raw = (int16_t)(dps * 131.0f);
}

/**
 * @brief 设置仿真的四路巡线传感器黑线掩码。
 *
 * @param mask bit0=最左探头，bit3=最右探头；对应位为 1 表示检测到黑线。
 */
void hal_stub_set_line_mask(uint8_t mask)
{
    /*
     * 桌面仿真默认沿用真实硬件逻辑：低电平代表检测到黑线。
     * mask bit0=最左，bit3=最右。
     */
    s_gpio[HAL_PIN_LINE_L2] = (mask & 0x01u) == 0u;
    s_gpio[HAL_PIN_LINE_L1] = (mask & 0x02u) == 0u;
    s_gpio[HAL_PIN_LINE_R1] = (mask & 0x04u) == 0u;
    s_gpio[HAL_PIN_LINE_R2] = (mask & 0x08u) == 0u;
}

/**
 * @brief 设置仿真按键状态。
 *
 * @param pin 按键对应的抽象引脚。
 * @param pressed true 表示按下，false 表示松开。
 */
void hal_stub_set_key(hal_pin_t pin, bool pressed)
{
    s_gpio[pin] = !pressed;
}

/**
 * @brief 向仿真 K230 串口接收缓冲区写入文本。
 *
 * @param text 待模拟接收的 ASCII 文本。
 */
void hal_stub_push_k230_text(const char *text)
{
    while (*text != '\0' && s_uart_rx_write < sizeof(s_uart_rx))
    {
        s_uart_rx[s_uart_rx_write++] = *text++;
    }
}

/**
 * @brief 获取某个仿真 PWM 通道当前值。
 *
 * @param pwm PWM 通道。
 * @return 电机通道返回千分比占空比，舵机通道返回 us 脉宽。
 */
uint16_t hal_stub_get_pwm(hal_pwm_t pwm)
{
    return s_pwm[pwm];
}

/**
 * @brief 初始化仿真硬件状态。
 */
void hal_init(void)
{
    memset(s_gpio, 1, sizeof(s_gpio));
    memset(s_pwm, 0, sizeof(s_pwm));
    s_now_ms = 0u;
    s_uart_rx_read = 0u;
    s_uart_rx_write = 0u;
    hal_stub_set_line_mask(0x06u);
}

/**
 * @brief 获取仿真毫秒计数。
 *
 * @return 当前仿真时间，单位 ms。
 */
uint32_t hal_millis(void)
{
    return s_now_ms;
}

/**
 * @brief 推进仿真时间。
 *
 * @param ms 时间增量，单位 ms。
 */
void hal_delay_ms(uint32_t ms)
{
    s_now_ms += ms;
}

/**
 * @brief 设置仿真 GPIO 输出电平。
 *
 * @param pin 抽象引脚。
 * @param high true 为高电平。
 */
void hal_gpio_write(hal_pin_t pin, bool high)
{
    if (pin < HAL_PIN_COUNT)
    {
        s_gpio[pin] = high;
    }
}

/**
 * @brief 读取仿真 GPIO 电平。
 *
 * @param pin 抽象引脚。
 * @return 当前仿真电平。
 */
bool hal_gpio_read(hal_pin_t pin)
{
    if (pin < HAL_PIN_COUNT)
    {
        return s_gpio[pin];
    }
    return false;
}

/**
 * @brief 设置仿真电机 PWM 占空比。
 *
 * @param pwm PWM 通道。
 * @param permille 占空比千分比。
 */
void hal_pwm_set_duty_permille(hal_pwm_t pwm, uint16_t permille)
{
    if (pwm < HAL_PWM_COUNT)
    {
        s_pwm[pwm] = permille;
    }
}

/**
 * @brief 设置仿真舵机 PWM 脉宽。
 *
 * @param pwm PWM 通道。
 * @param pulse_us 舵机脉宽，单位 us。
 */
void hal_pwm_set_servo_us(hal_pwm_t pwm, uint16_t pulse_us)
{
    if (pwm < HAL_PWM_COUNT)
    {
        s_pwm[pwm] = pulse_us;
    }
}

/**
 * @brief 仿真 I2C 写接口，恒定成功（OLED/MPU6050 寄存器写入不需要具体行为）。
 *
 * @param addr 从机地址。
 * @param data 写入数据。
 * @param len 写入长度。
 * @return 恒定为 true。
 */
bool hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    (void)addr;
    (void)data;
    (void)len;
    return true;
}

/**
 * @brief 仿真 I2C 先写后读接口。
 *
 * 只模拟 MPU6050 一次性读取 ACCEL_XOUT 起始的 14 字节：加速度和陀螺 X/Y
 * 固定为 0，陀螺 Z 使用 hal_stub_set_gyro_z_dps() 设置的仿真角速度，
 * 供桌面测试驱动偏航角积分（例如验证跑完一圈的角度判定）。
 *
 * @param addr 从机地址。
 * @param tx 写入数据。
 * @param tx_len 写入长度。
 * @param rx 读取缓冲区。
 * @param rx_len 读取长度。
 * @return 恒定为 true。
 */
bool hal_i2c_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    (void)tx;
    (void)tx_len;

    if (addr == ROBOT_MPU6050_ADDR && rx_len == 14u)
    {
        memset(rx, 0, rx_len);
        rx[12] = (uint8_t)((uint16_t)s_mpu_gyro_z_raw >> 8);
        rx[13] = (uint8_t)((uint16_t)s_mpu_gyro_z_raw & 0xFFu);
    }
    else if (rx_len > 0u)
    {
        memset(rx, 0, rx_len);
    }

    return true;
}

/**
 * @brief 从仿真 K230 串口读取一个字节。
 *
 * @return 0..255 表示一个字节，-1 表示缓冲区为空。
 */
int hal_uart_k230_read_byte(void)
{
    if (s_uart_rx_read >= s_uart_rx_write)
    {
        return -1;
    }
    return (unsigned char)s_uart_rx[s_uart_rx_read++];
}

/**
 * @brief 输出仿真调试字符串到标准输出。
 *
 * @param text 待输出字符串。
 */
void hal_uart_debug_write(const char *text)
{
    fputs(text, stdout);
}
