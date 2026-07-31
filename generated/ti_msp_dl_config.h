/*
 * MSPM0G3507 board configuration for the car_match Keil5 project.
 *
 * Target board/package: MSPM0G3507SPTR, LQFP-48 (PT)
 * System clock:         80 MHz
 */
#ifndef TI_MSP_DL_CONFIG_H
#define TI_MSP_DL_CONFIG_H

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__ARMCC_VERSION)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POWER_STARTUP_DELAY                                                (16)
#define CPUCLK_FREQ                                                     80000000

/*
 * TIMA0: TB6612 PWMA=PA8, PWMB=PA9, 2.5 kHz.
 *
 * The supplied MG310/TB6612 carrier has already been proven to start
 * reliably at 2.5 kHz.  A 20 kHz carrier was retained by an earlier
 * generic port and could leave both motors stalled on this particular
 * chassis.
 */
#define Motor_INST                                                         TIMA0
#define Motor_INST_CLK_FREQ                                             80000000
#define MOTOR_PWM_PERIOD_COUNTS                                          32000U
#define GPIO_Motor_C0_PORT                                                 GPIOA
#define GPIO_Motor_C0_PIN                                          DL_GPIO_PIN_8
#define GPIO_Motor_C0_IOMUX                                      (IOMUX_PINCM19)
#define GPIO_Motor_C0_IOMUX_FUNC                     IOMUX_PINCM19_PF_TIMA0_CCP0
#define GPIO_Motor_C0_IDX                                    DL_TIMER_CC_0_INDEX
#define GPIO_Motor_C1_PORT                                                 GPIOA
#define GPIO_Motor_C1_PIN                                          DL_GPIO_PIN_9
#define GPIO_Motor_C1_IOMUX                                      (IOMUX_PINCM20)
#define GPIO_Motor_C1_IOMUX_FUNC                     IOMUX_PINCM20_PF_TIMA0_CCP1
#define GPIO_Motor_C1_IDX                                    DL_TIMER_CC_1_INDEX

/*
 * 旧版兼容定义：当前 Q1-Q3 比赛版不会初始化/启动 TIMG8，也不会复用 PA7。
 * MG996R 只接 K230 IO42/PWM0，PA7 保持复位态并悬空。
 */
#define Servo_INST                                                         TIMG8
#define Servo_INST_CLK_FREQ                                              1000000
#define SERVO_PWM_PERIOD_COUNTS                                          20000U
#define GPIO_Servo_C0_PORT                                                 GPIOA
#define GPIO_Servo_C0_PIN                                          DL_GPIO_PIN_7
#define GPIO_Servo_C0_IOMUX                                      (IOMUX_PINCM14)
#define GPIO_Servo_C0_IOMUX_FUNC                     IOMUX_PINCM14_PF_TIMG8_CCP0
#define GPIO_Servo_C0_IDX                                    DL_TIMER_CC_0_INDEX

/* UART0: PA10=MCU TX, PA11=MCU RX, 115200 8N1. */
#define UART_K230_INST                                                     UART0
#define UART_K230_INST_FREQUENCY                                        40000000
#define UART_K230_INST_IRQHandler                               UART0_IRQHandler
#define UART_K230_INST_INT_IRQN                                   UART0_INT_IRQn
#define GPIO_UART_K230_TX_PORT                                             GPIOA
#define GPIO_UART_K230_RX_PORT                                             GPIOA
#define GPIO_UART_K230_TX_PIN                                     DL_GPIO_PIN_10
#define GPIO_UART_K230_RX_PIN                                     DL_GPIO_PIN_11
#define GPIO_UART_K230_IOMUX_TX                                  (IOMUX_PINCM21)
#define GPIO_UART_K230_IOMUX_RX                                  (IOMUX_PINCM22)
#define GPIO_UART_K230_IOMUX_TX_FUNC                   IOMUX_PINCM21_PF_UART0_TX
#define GPIO_UART_K230_IOMUX_RX_FUNC                   IOMUX_PINCM22_PF_UART0_RX
#define UART_K230_IBRD_40_MHZ_115200_BAUD                               (21U)
#define UART_K230_FBRD_40_MHZ_115200_BAUD                               (45U)

/* TB6612 direction and standby outputs. */
#define GPIO_MOTOR_PORT                                                    GPIOA
#define GPIO_MOTOR_AIN1_PIN                                        DL_GPIO_PIN_24
#define GPIO_MOTOR_AIN1_IOMUX                                     (IOMUX_PINCM54)
#define GPIO_MOTOR_AIN2_PIN                                        DL_GPIO_PIN_28
#define GPIO_MOTOR_AIN2_IOMUX                                      (IOMUX_PINCM3)
#define GPIO_MOTOR_BIN1_PIN                                        DL_GPIO_PIN_22
#define GPIO_MOTOR_BIN1_IOMUX                                     (IOMUX_PINCM47)
#define GPIO_MOTOR_BIN2_PIN                                        DL_GPIO_PIN_14
#define GPIO_MOTOR_BIN2_IOMUX                                     (IOMUX_PINCM36)
#define GPIO_MOTOR_STBY_PIN                                        DL_GPIO_PIN_25
#define GPIO_MOTOR_STBY_IOMUX                                     (IOMUX_PINCM55)

/* YB-MVX01 X1..X4, physical left-to-right, active-low. */
#define GPIO_LINE_PORT                                                     GPIOB
#define GPIO_LINE_L2_PIN                                            DL_GPIO_PIN_2
#define GPIO_LINE_L2_IOMUX                                       (IOMUX_PINCM15)
#define GPIO_LINE_L1_PIN                                            DL_GPIO_PIN_3
#define GPIO_LINE_L1_IOMUX                                       (IOMUX_PINCM16)
#define GPIO_LINE_R1_PIN                                            DL_GPIO_PIN_4
#define GPIO_LINE_R1_IOMUX                                       (IOMUX_PINCM17)
#define GPIO_LINE_R2_PIN                                            DL_GPIO_PIN_5
#define GPIO_LINE_R2_IOMUX                                       (IOMUX_PINCM18)

/*
 * Tianmengxing carrier keys, active-low with internal pull-ups:
 * A27 = mode, A30 = start/stop (the user's K3 key), A17 = calibration.
 */
#define GPIO_KEY_PORT                                                      GPIOA
#define GPIO_KEY_MODE_PIN                                          DL_GPIO_PIN_27
#define GPIO_KEY_MODE_IOMUX                                       (IOMUX_PINCM60)
#define GPIO_KEY_START_PIN                                         DL_GPIO_PIN_30
#define GPIO_KEY_START_IOMUX                                       (IOMUX_PINCM5)
#define GPIO_KEY_CALIB_PIN                                         DL_GPIO_PIN_17
#define GPIO_KEY_CALIB_IOMUX                                      (IOMUX_PINCM39)

/* MPU6050 software-I2C, open-drain behavior implemented in the HAL. */
#define GPIO_MPU_I2C_PORT                                                  GPIOA
#define GPIO_MPU_I2C_SCL_PIN                                      DL_GPIO_PIN_15
#define GPIO_MPU_I2C_SCL_IOMUX                                   (IOMUX_PINCM37)
#define GPIO_MPU_I2C_SDA_PIN                                      DL_GPIO_PIN_16
#define GPIO_MPU_I2C_SDA_IOMUX                                   (IOMUX_PINCM38)

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_Motor_init(void);
void SYSCFG_DL_Servo_init(void);
void SYSCFG_DL_UART_K230_init(void);

bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif
