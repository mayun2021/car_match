/**
 * @file hal_ti_mspm0.c
 * @brief MSPM0G3507/MSPM03507 真实硬件适配模板。
 *
 * 使用方式：
 * 1. 在 TI SysConfig 中建立 GPIO/PWM/I2C/UART 外设。
 * 2. 建议给信号使用下面注释中的名字，例如 GPIO_LINE_L2、PWM_MOTOR_LEFT。
 * 3. 根据 SysConfig 实际生成的宏，把每个 switch 分支里的 TODO 对接到 DriverLib。
 *
 * 注意：本文件是硬件工程模板，桌面仿真不会编译它。
 */

#ifdef TARGET_MSPM0

#include "hal.h"
#include "robot_config.h"
#include "ti_msp_dl_config.h"

/**
 * @brief 初始化 MSPM0 外设。
 *
 * 真实工程中由 SysConfig 生成的 SYSCFG_DL_init() 完成 GPIO、PWM、I2C、UART
 * 和时钟初始化。
 */
void hal_init(void)
{
    SYSCFG_DL_init();

    /*
     * SysConfig 建议：
     * - 电机 PWM：20 kHz，初始占空比 0。
     * - 舵机 PWM：50 Hz，初始脉宽 1500 us。
     * - K230 UART：115200 8N1。
     * - MPU6050 I2C：100 kHz 或 400 kHz。
     */
}

/**
 * @brief 获取 MSPM0 系统毫秒计数。
 *
 * @return 由 SysTick 或 Timer 中断维护的毫秒计数。
 */
uint32_t hal_millis(void)
{
    /*
     * 推荐使用 1 ms SysTick 或 Timer 中断维护毫秒计数。
     * 例如：
     * extern volatile uint32_t g_systick_ms;
     * return g_systick_ms;
     */
    extern volatile uint32_t g_systick_ms;
    return g_systick_ms;
}

/**
 * @brief 阻塞延时指定毫秒数。
 *
 * @param ms 延时时间，单位 ms。
 */
void hal_delay_ms(uint32_t ms)
{
    const uint32_t start = hal_millis();
    while (hal_millis() - start < ms)
    {
    }
}

/**
 * @brief 设置 MSPM0 GPIO 输出电平。
 *
 * @param pin 抽象引脚编号。
 * @param high true 输出高电平，false 输出低电平。
 */
void hal_gpio_write(hal_pin_t pin, bool high)
{
    /*
     * 下面以 TI DriverLib 常用写法为例：
     * high:  DL_GPIO_setPins(PORT, PIN);
     * low:   DL_GPIO_clearPins(PORT, PIN);
     */
    switch (pin)
    {
    case HAL_PIN_MOTOR_AIN1:
        /* TODO: PA24 */
        break;
    case HAL_PIN_MOTOR_AIN2:
        /* TODO: PA28 */
        break;
    case HAL_PIN_MOTOR_BIN1:
        /* TODO: PA22 */
        break;
    case HAL_PIN_MOTOR_BIN2:
        /* TODO: PA14 */
        break;
    case HAL_PIN_MOTOR_STBY:
        /* TODO: PA25 */
        break;
    default:
        (void)high;
        break;
    }
}

/**
 * @brief 读取 MSPM0 GPIO 输入电平。
 *
 * @param pin 抽象引脚编号。
 * @return true 表示高电平，false 表示低电平。
 */
bool hal_gpio_read(hal_pin_t pin)
{
    switch (pin)
    {
    case HAL_PIN_LINE_L2:
        /* TODO: return DL_GPIO_readPins(GPIOB, DL_GPIO_PIN_2) != 0; */
        break;
    case HAL_PIN_LINE_L1:
        /* TODO: PB3 */
        break;
    case HAL_PIN_LINE_R1:
        /* TODO: PB4 */
        break;
    case HAL_PIN_LINE_R2:
        /* TODO: PB5 */
        break;
    case HAL_PIN_KEY_MODE:
        /* TODO: PA26 */
        break;
    case HAL_PIN_KEY_START:
        /* TODO: PA27 */
        break;
    case HAL_PIN_KEY_CALIB:
        /* TODO: PA1 */
        break;
    default:
        break;
    }

    return false;
}

/**
 * @brief 设置电机 PWM 占空比。
 *
 * @param pwm PWM 通道。
 * @param permille 占空比千分比，0 到 1000。
 */
void hal_pwm_set_duty_permille(hal_pwm_t pwm, uint16_t permille)
{
    if (permille > 1000u)
    {
        permille = 1000u;
    }

    switch (pwm)
    {
    case HAL_PWM_MOTOR_LEFT:
        /* TODO: PA8/PWMA，占空比 permille/1000 */
        break;
    case HAL_PWM_MOTOR_RIGHT:
        /* TODO: PA9/PWMB，占空比 permille/1000 */
        break;
    default:
        break;
    }
}

/**
 * @brief 设置舵机 PWM 脉宽。
 *
 * @param pwm PWM 通道。
 * @param pulse_us 舵机高电平脉宽，单位 us。
 */
void hal_pwm_set_servo_us(hal_pwm_t pwm, uint16_t pulse_us)
{
    if (pwm == HAL_PWM_SERVO)
    {
        /*
         * TODO: PA7/MG996R。
         * 50 Hz 周期为 20000 us，把 pulse_us 转成定时器比较值。
         */
        (void)pulse_us;
    }
}

/**
 * @brief 通过 MSPM0 I2C 写入数据。
 *
 * @param addr 7 位 I2C 从机地址。
 * @param data 待写数据。
 * @param len 待写长度。
 * @return true 表示写入成功。
 */
bool hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    /*
     * TODO: 使用 SysConfig 生成的 I2C 控制器向 addr 写 len 字节。
     * MSPM0 SDK 示例中通常使用 DL_I2C_fillControllerTXFIFO 等函数。
     */
    (void)addr;
    (void)data;
    (void)len;
    return false;
}

/**
 * @brief 通过 MSPM0 I2C 先写寄存器地址再读取数据。
 *
 * @param addr 7 位 I2C 从机地址。
 * @param tx 写入缓冲区。
 * @param tx_len 写入长度。
 * @param rx 读取缓冲区。
 * @param rx_len 读取长度。
 * @return true 表示通信成功。
 */
bool hal_i2c_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    /*
     * TODO: 典型寄存器读取流程：
     * 1. 写寄存器地址 tx[0]，不释放总线或随后重新起始。
     * 2. 从同一从机读取 rx_len 字节到 rx。
     */
    (void)addr;
    (void)tx;
    (void)tx_len;
    (void)rx;
    (void)rx_len;
    return false;
}

/**
 * @brief 非阻塞读取 K230 串口字节。
 *
 * @return 0..255 表示读到的字节，-1 表示当前没有数据。
 */
int hal_uart_k230_read_byte(void)
{
    /*
     * TODO: 非阻塞读取 K230 UART。
     * 没有数据返回 -1，有数据返回 0..255。
     */
    return -1;
}

/**
 * @brief 输出调试字符串。
 *
 * @param text 待输出字符串。
 */
void hal_uart_debug_write(const char *text)
{
    /*
     * TODO: 可输出到调试串口/OLED，也可以留空。
     */
    (void)text;
}

#endif
