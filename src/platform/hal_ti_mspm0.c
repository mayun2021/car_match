/**
 * @file hal_ti_mspm0.c
 * @brief MSPM0G3507 hardware abstraction for the Keil5 target.
 */

#ifdef TARGET_MSPM0

#include "hal.h"
#include "ti_msp_dl_config.h"

#define UART_RX_BUFFER_SIZE              128U
#define UART_RX_BUFFER_MASK              (UART_RX_BUFFER_SIZE - 1U)
/* 400 kHz software-I2C keeps the full-frame OLED update from stalling control. */
#define I2C_HALF_PERIOD_CYCLES           (CPUCLK_FREQ / 800000U)
#define I2C_CLOCK_STRETCH_TIMEOUT        1000U

volatile uint32_t g_systick_ms;

static volatile uint8_t s_uart_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t s_uart_rx_head;
static volatile uint8_t s_uart_rx_tail;

static void i2c_delay(void)
{
    DL_Common_delayCycles(I2C_HALF_PERIOD_CYCLES);
}

static void i2c_release(uint32_t iomux, uint32_t pin)
{
    DL_GPIO_disableOutput(GPIO_MPU_I2C_PORT, pin);
    DL_GPIO_initDigitalInputFeatures(
        iomux,
        DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
}

static void i2c_drive_low(uint32_t iomux, uint32_t pin)
{
    DL_GPIO_clearPins(GPIO_MPU_I2C_PORT, pin);
    DL_GPIO_initDigitalOutput(iomux);
    DL_GPIO_enableOutput(GPIO_MPU_I2C_PORT, pin);
}

static void i2c_sda_release(void)
{
    i2c_release(GPIO_MPU_I2C_SDA_IOMUX, GPIO_MPU_I2C_SDA_PIN);
}

static void i2c_sda_low(void)
{
    i2c_drive_low(GPIO_MPU_I2C_SDA_IOMUX, GPIO_MPU_I2C_SDA_PIN);
}

static void i2c_scl_low(void)
{
    i2c_drive_low(GPIO_MPU_I2C_SCL_IOMUX, GPIO_MPU_I2C_SCL_PIN);
}

static bool i2c_scl_release_and_wait(void)
{
    uint32_t timeout = I2C_CLOCK_STRETCH_TIMEOUT;

    i2c_release(GPIO_MPU_I2C_SCL_IOMUX, GPIO_MPU_I2C_SCL_PIN);
    while ((DL_GPIO_readPins(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SCL_PIN) &
            GPIO_MPU_I2C_SCL_PIN) == 0U)
    {
        if (--timeout == 0U)
        {
            return false;
        }
    }
    return true;
}

static bool i2c_sda_is_high(void)
{
    return (DL_GPIO_readPins(GPIO_MPU_I2C_PORT, GPIO_MPU_I2C_SDA_PIN) &
            GPIO_MPU_I2C_SDA_PIN) != 0U;
}

static bool i2c_start(void)
{
    i2c_sda_release();
    if (!i2c_scl_release_and_wait())
    {
        return false;
    }
    i2c_delay();

    if (!i2c_sda_is_high())
    {
        return false;
    }

    i2c_sda_low();
    i2c_delay();
    i2c_scl_low();
    i2c_delay();
    return true;
}

static void i2c_stop(void)
{
    i2c_sda_low();
    i2c_delay();
    (void)i2c_scl_release_and_wait();
    i2c_delay();
    i2c_sda_release();
    i2c_delay();
}

static bool i2c_write_byte(uint8_t value)
{
    uint8_t bit;
    bool ack;

    for (bit = 0U; bit < 8U; ++bit)
    {
        if ((value & 0x80U) != 0U)
        {
            i2c_sda_release();
        }
        else
        {
            i2c_sda_low();
        }
        i2c_delay();
        if (!i2c_scl_release_and_wait())
        {
            return false;
        }
        i2c_delay();
        i2c_scl_low();
        value <<= 1;
    }

    i2c_sda_release();
    i2c_delay();
    if (!i2c_scl_release_and_wait())
    {
        return false;
    }
    i2c_delay();
    ack = !i2c_sda_is_high();
    i2c_scl_low();
    i2c_delay();
    return ack;
}

static bool i2c_read_byte(uint8_t *value, bool ack)
{
    uint8_t bit;
    uint8_t data = 0U;

    i2c_sda_release();
    for (bit = 0U; bit < 8U; ++bit)
    {
        data <<= 1;
        if (!i2c_scl_release_and_wait())
        {
            return false;
        }
        i2c_delay();
        if (i2c_sda_is_high())
        {
            data |= 1U;
        }
        i2c_scl_low();
        i2c_delay();
    }

    if (ack)
    {
        i2c_sda_low();
    }
    else
    {
        i2c_sda_release();
    }
    i2c_delay();
    if (!i2c_scl_release_and_wait())
    {
        return false;
    }
    i2c_delay();
    i2c_scl_low();
    i2c_sda_release();
    i2c_delay();

    *value = data;
    return true;
}

void SysTick_Handler(void)
{
    ++g_systick_ms;
}

void UART_K230_INST_IRQHandler(void)
{
    uint8_t next;
    uint8_t value;

    if (DL_UART_Main_getPendingInterrupt(UART_K230_INST) ==
        DL_UART_MAIN_IIDX_RX)
    {
        while (!DL_UART_Main_isRXFIFOEmpty(UART_K230_INST))
        {
            value = DL_UART_Main_receiveData(UART_K230_INST);
            next = (uint8_t)((s_uart_rx_head + 1U) & UART_RX_BUFFER_MASK);
            if (next != s_uart_rx_tail)
            {
                s_uart_rx_buffer[s_uart_rx_head] = value;
                s_uart_rx_head = next;
            }
        }
    }
}

void hal_init(void)
{
    s_uart_rx_head = 0U;
    s_uart_rx_tail = 0U;
    g_systick_ms = 0U;

    SYSCFG_DL_init();

    DL_TimerA_startCounter(Motor_INST);
    DL_TimerG_startCounter(Servo_INST);

    NVIC_ClearPendingIRQ(UART_K230_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_K230_INST_INT_IRQN);

    (void)SysTick_Config(CPUCLK_FREQ / 1000U);
    NVIC_SetPriority(SysTick_IRQn, 3U);
}

uint32_t hal_millis(void)
{
    return g_systick_ms;
}

void hal_delay_ms(uint32_t ms)
{
    const uint32_t start = hal_millis();
    while ((hal_millis() - start) < ms)
    {
    }
}

void hal_gpio_write(hal_pin_t pin, bool high)
{
    uint32_t mask = 0U;

    switch (pin)
    {
    case HAL_PIN_MOTOR_AIN1:
        mask = GPIO_MOTOR_AIN1_PIN;
        break;
    case HAL_PIN_MOTOR_AIN2:
        mask = GPIO_MOTOR_AIN2_PIN;
        break;
    case HAL_PIN_MOTOR_BIN1:
        mask = GPIO_MOTOR_BIN1_PIN;
        break;
    case HAL_PIN_MOTOR_BIN2:
        mask = GPIO_MOTOR_BIN2_PIN;
        break;
    case HAL_PIN_MOTOR_STBY:
        mask = GPIO_MOTOR_STBY_PIN;
        break;
    default:
        return;
    }

    if (high)
    {
        DL_GPIO_setPins(GPIO_MOTOR_PORT, mask);
    }
    else
    {
        DL_GPIO_clearPins(GPIO_MOTOR_PORT, mask);
    }
}

bool hal_gpio_read(hal_pin_t pin)
{
    GPIO_Regs *port;
    uint32_t mask;

    switch (pin)
    {
    case HAL_PIN_LINE_L2:
        port = GPIO_LINE_PORT;
        mask = GPIO_LINE_L2_PIN;
        break;
    case HAL_PIN_LINE_L1:
        port = GPIO_LINE_PORT;
        mask = GPIO_LINE_L1_PIN;
        break;
    case HAL_PIN_LINE_R1:
        port = GPIO_LINE_PORT;
        mask = GPIO_LINE_R1_PIN;
        break;
    case HAL_PIN_LINE_R2:
        port = GPIO_LINE_PORT;
        mask = GPIO_LINE_R2_PIN;
        break;
    case HAL_PIN_KEY_MODE:
        port = GPIO_KEY_PORT;
        mask = GPIO_KEY_MODE_PIN;
        break;
    case HAL_PIN_KEY_START:
        port = GPIO_KEY_PORT;
        mask = GPIO_KEY_START_PIN;
        break;
    case HAL_PIN_KEY_CALIB:
        port = GPIO_KEY_PORT;
        mask = GPIO_KEY_CALIB_PIN;
        break;
    default:
        return false;
    }

    return (DL_GPIO_readPins(port, mask) & mask) != 0U;
}

void hal_pwm_set_duty_permille(hal_pwm_t pwm, uint16_t permille)
{
    uint32_t compare;

    if (permille > 1000U)
    {
        permille = 1000U;
    }

    compare = MOTOR_PWM_PERIOD_COUNTS -
        (((uint32_t)MOTOR_PWM_PERIOD_COUNTS * permille + 500U) / 1000U);

    if (pwm == HAL_PWM_MOTOR_LEFT)
    {
        DL_TimerA_setCaptureCompareValue(
            Motor_INST, compare, GPIO_Motor_C0_IDX);
    }
    else if (pwm == HAL_PWM_MOTOR_RIGHT)
    {
        DL_TimerA_setCaptureCompareValue(
            Motor_INST, compare, GPIO_Motor_C1_IDX);
    }
}

void hal_pwm_set_servo_us(hal_pwm_t pwm, uint16_t pulse_us)
{
    if (pwm != HAL_PWM_SERVO)
    {
        return;
    }

    if (pulse_us < 500U)
    {
        pulse_us = 500U;
    }
    else if (pulse_us > 2500U)
    {
        pulse_us = 2500U;
    }

    DL_TimerG_setCaptureCompareValue(
        Servo_INST, pulse_us, GPIO_Servo_C0_IDX);
}

bool hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    size_t index;
    bool ok;

    if ((data == NULL && len != 0U) || addr > 0x7FU)
    {
        return false;
    }

    ok = i2c_start();
    if (ok)
    {
        ok = i2c_write_byte((uint8_t)(addr << 1));
    }
    for (index = 0U; ok && index < len; ++index)
    {
        ok = i2c_write_byte(data[index]);
    }
    i2c_stop();
    return ok;
}

bool hal_i2c_write_read(uint8_t addr,
                        const uint8_t *tx,
                        size_t tx_len,
                        uint8_t *rx,
                        size_t rx_len)
{
    size_t index;
    bool ok;

    if ((tx == NULL && tx_len != 0U) ||
        (rx == NULL && rx_len != 0U) ||
        addr > 0x7FU)
    {
        return false;
    }

    ok = i2c_start();
    if (ok && tx_len != 0U)
    {
        ok = i2c_write_byte((uint8_t)(addr << 1));
        for (index = 0U; ok && index < tx_len; ++index)
        {
            ok = i2c_write_byte(tx[index]);
        }
    }

    if (ok && rx_len != 0U)
    {
        ok = i2c_start();
        if (ok)
        {
            ok = i2c_write_byte((uint8_t)((addr << 1) | 1U));
        }
        for (index = 0U; ok && index < rx_len; ++index)
        {
            ok = i2c_read_byte(
                &rx[index], (index + 1U) < rx_len);
        }
    }

    i2c_stop();
    return ok;
}

int hal_uart_k230_read_byte(void)
{
    uint8_t value;

    if (s_uart_rx_tail == s_uart_rx_head)
    {
        return -1;
    }

    value = s_uart_rx_buffer[s_uart_rx_tail];
    s_uart_rx_tail =
        (uint8_t)((s_uart_rx_tail + 1U) & UART_RX_BUFFER_MASK);
    return (int)value;
}

void hal_uart_debug_write(const char *text)
{
    /*
     * PA10/PA11 are dedicated to K230. Keep diagnostics silent so telemetry
     * text cannot interfere with the vision link. Add a second UART here if
     * the board exposes spare debug pins.
     */
    (void)text;
}

#endif
