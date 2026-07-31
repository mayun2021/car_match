/*
 * MSPM0G3507 board configuration for the car_match Keil5 project.
 * This generated-equivalent source is intentionally kept in the repository
 * so the project builds without a separate SysConfig installation.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gMotorBackup;

SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_Motor_init();
    SYSCFG_DL_UART_K230_init();

    gMotorBackup.backupRdy = false;
}

SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool ok = true;
    ok &= DL_TimerA_saveConfiguration(Motor_INST, &gMotorBackup);
    return ok;
}

SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool ok = true;
    ok &= DL_TimerA_restoreConfiguration(Motor_INST, &gMotorBackup, false);
    return ok;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(Motor_INST);
    DL_UART_Main_reset(UART_K230_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(Motor_INST);
    DL_UART_Main_enablePower(UART_K230_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{
    /* Motor PWM pin mux. PA7 stays in its reset/high-impedance state. */
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_Motor_C0_IOMUX, GPIO_Motor_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_Motor_C0_PORT, GPIO_Motor_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_Motor_C1_IOMUX, GPIO_Motor_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_Motor_C1_PORT, GPIO_Motor_C1_PIN);

    /* K230 UART: PA10 TX, PA11 RX. */
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_K230_IOMUX_TX, GPIO_UART_K230_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_K230_IOMUX_RX, GPIO_UART_K230_IOMUX_RX_FUNC);

    /* Keep TB6612 disabled until the application explicitly starts it. */
    DL_GPIO_initDigitalOutput(GPIO_MOTOR_AIN1_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_MOTOR_AIN2_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_MOTOR_BIN1_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_MOTOR_BIN2_IOMUX);
    DL_GPIO_initDigitalOutput(GPIO_MOTOR_STBY_IOMUX);
    DL_GPIO_clearPins(GPIO_MOTOR_PORT,
        GPIO_MOTOR_AIN1_PIN |
        GPIO_MOTOR_AIN2_PIN |
        GPIO_MOTOR_BIN1_PIN |
        GPIO_MOTOR_BIN2_PIN |
        GPIO_MOTOR_STBY_PIN);
    DL_GPIO_enableOutput(GPIO_MOTOR_PORT,
        GPIO_MOTOR_AIN1_PIN |
        GPIO_MOTOR_AIN2_PIN |
        GPIO_MOTOR_BIN1_PIN |
        GPIO_MOTOR_BIN2_PIN |
        GPIO_MOTOR_STBY_PIN);

    /* Active-low line sensors. An unplugged input reads high/no-line. */
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_L2_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_L1_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_R1_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_LINE_R2_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* Active-low keys. */
    DL_GPIO_initDigitalInputFeatures(GPIO_KEY_MODE_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_KEY_START_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_KEY_CALIB_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    /* Software-I2C starts released; external or internal pull-ups hold high. */
    DL_GPIO_initDigitalInputFeatures(GPIO_MPU_I2C_SCL_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initDigitalInputFeatures(GPIO_MPU_I2C_SDA_IOMUX,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq   = DL_SYSCTL_SYSPLL_INPUT_FREQ_16_32_MHZ,
    .rDivClk2x   = 3,
    .rDivClk1    = 0,
    .rDivClk0    = 0,
    .enableCLK2x = DL_SYSCTL_SYSPLL_CLK2X_ENABLE,
    .enableCLK1  = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
    .enableCLK0  = DL_SYSCTL_SYSPLL_CLK0_DISABLE,
    .sysPLLMCLK  = DL_SYSCTL_SYSPLL_MCLK_CLK2X,
    .sysPLLRef   = DL_SYSCTL_SYSPLL_REF_SYSOSC,
    .qDiv        = 9,
    .pDiv        = DL_SYSCTL_SYSPLL_PDIV_2
};

SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *)&gSYSPLLConfig);
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_enableMFCLK();
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);
}

static const DL_TimerA_ClockConfig gMotorClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_1,
    .prescale = 0U
};

static const DL_TimerA_PWMConfig gMotorConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = MOTOR_PWM_PERIOD_COUNTS,
    .isTimerWithFourCC = true,
    .startTimer = DL_TIMER_STOP
};

SYSCONFIG_WEAK void SYSCFG_DL_Motor_init(void)
{
    DL_TimerA_setClockConfig(
        Motor_INST, (DL_TimerA_ClockConfig *)&gMotorClockConfig);
    DL_TimerA_initPWMMode(
        Motor_INST, (DL_TimerA_PWMConfig *)&gMotorConfig);
    DL_TimerA_setCounterControl(Motor_INST,
        DL_TIMER_CZC_CCCTL0_ZCOND,
        DL_TIMER_CAC_CCCTL0_ACOND,
        DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(Motor_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptCompUpdateMethod(Motor_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
        DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(
        Motor_INST, MOTOR_PWM_PERIOD_COUNTS, DL_TIMER_CC_0_INDEX);

    DL_TimerA_setCaptureCompareOutCtl(Motor_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_DISABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptCompUpdateMethod(Motor_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
        DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptureCompareValue(
        Motor_INST, MOTOR_PWM_PERIOD_COUNTS, DL_TIMER_CC_1_INDEX);

    DL_TimerA_enableClock(Motor_INST);
    DL_TimerA_setCCPDirection(
        Motor_INST, DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT);
}

/* MFCLK / 4 = 1 MHz; 20000 counts = 20 ms. */
static const DL_TimerG_ClockConfig gServoClockConfig = {
    .clockSel = DL_TIMER_CLOCK_MFCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_4,
    .prescale = 0U
};

static const DL_TimerG_PWMConfig gServoConfig = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN,
    .period = SERVO_PWM_PERIOD_COUNTS,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_STOP
};

SYSCONFIG_WEAK void SYSCFG_DL_Servo_init(void)
{
    DL_TimerG_setClockConfig(
        Servo_INST, (DL_TimerG_ClockConfig *)&gServoClockConfig);
    DL_TimerG_initPWMMode(
        Servo_INST, (DL_TimerG_PWMConfig *)&gServoConfig);
    DL_TimerG_setCounterControl(Servo_INST,
        DL_TIMER_CZC_CCCTL0_ZCOND,
        DL_TIMER_CAC_CCCTL0_ACOND,
        DL_TIMER_CLC_CCCTL0_LCOND);
    DL_TimerG_setCaptureCompareOutCtl(Servo_INST,
        DL_TIMER_CC_OCTL_INIT_VAL_LOW,
        DL_TIMER_CC_OCTL_INV_OUT_ENABLED,
        DL_TIMER_CC_OCTL_SRC_FUNCVAL,
        DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptCompUpdateMethod(Servo_INST,
        DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE,
        DL_TIMERG_CAPTURE_COMPARE_0_INDEX);
    DL_TimerG_setCaptureCompareValue(
        Servo_INST, 1500U, DL_TIMER_CC_0_INDEX);
    DL_TimerG_enableClock(Servo_INST);
    DL_TimerG_setCCPDirection(Servo_INST, DL_TIMER_CC0_OUTPUT);
}

static const DL_UART_Main_ClockConfig gUARTK230ClockConfig = {
    .clockSel = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUARTK230Config = {
    .mode = DL_UART_MAIN_MODE_NORMAL,
    .direction = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity = DL_UART_MAIN_PARITY_NONE,
    .wordLength = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_K230_init(void)
{
    DL_UART_Main_setClockConfig(
        UART_K230_INST, (DL_UART_Main_ClockConfig *)&gUARTK230ClockConfig);
    DL_UART_Main_init(
        UART_K230_INST, (DL_UART_Main_Config *)&gUARTK230Config);
    DL_UART_Main_setOversampling(
        UART_K230_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(
        UART_K230_INST,
        UART_K230_IBRD_40_MHZ_115200_BAUD,
        UART_K230_FBRD_40_MHZ_115200_BAUD);
    DL_UART_Main_enableInterrupt(
        UART_K230_INST, DL_UART_MAIN_INTERRUPT_RX);
    DL_UART_Main_enable(UART_K230_INST);
}
