# CCS / SysConfig 移植说明

`src/platform/hal_ti_mspm0.c` 是唯一需要和 TI SDK/SysConfig 深度绑定的文件。其它应用代码不直接碰寄存器。

## 1. GPIO

建议在 SysConfig 中建立这些 GPIO 名称：

| 建议名称 | 方向 | 引脚 | 初始状态 |
|---|---|---|---|
| GPIO_LINE_L2 | 输入 | PB2 | 上拉或按模块要求 |
| GPIO_LINE_L1 | 输入 | PB3 | 上拉或按模块要求 |
| GPIO_LINE_R1 | 输入 | PB4 | 上拉或按模块要求 |
| GPIO_LINE_R2 | 输入 | PB5 | 上拉或按模块要求 |
| GPIO_MOTOR_AIN1 | 输出 | PA24 | 低 |
| GPIO_MOTOR_AIN2 | 输出 | PA28 | 低 |
| GPIO_MOTOR_BIN1 | 输出 | PA22 | 低 |
| GPIO_MOTOR_BIN2 | 输出 | PA14 | 低 |
| GPIO_MOTOR_STBY | 输出 | PA25 | 高 |
| GPIO_KEY_MODE | 输入 | PA27 | 内部上拉 |
| GPIO_KEY_START | 输入 | PA17 | 内部上拉 |
| GPIO_KEY_CALIB | 输入 | PA30 | 内部上拉 |

## 2. PWM

| 建议名称 | 引脚 | 频率 | 用途 |
|---|---|---|---|
| PWM_MOTOR_LEFT | PA8 | 20 kHz | TB6612 PWMA |
| PWM_MOTOR_RIGHT | PA9 | 20 kHz | TB6612 PWMB |
| PWM_SERVO | PA7 | 50 Hz | MG996R |

电机 PWM 用 0% 到 100% 占空比。舵机 PWM 用 20 ms 周期，比较值对应 700 us 到 2300 us。

## 3. I2C

| 建议名称 | 引脚 | 频率 |
|---|---|---|
| I2C_MPU | PA16 SDA / PA15 SCL | 100 kHz 或 400 kHz |

MPU6050 地址默认 `0x68`。

## 4. UART

| 建议名称 | 引脚 | 参数 |
|---|---|---|
| UART_K230 | PA10 RX / PA11 TX | 115200, 8N1 |
| UART_DEBUG | 可选 | 115200, 8N1 |

K230 接 TI 时要注意交叉连接：K230 TX 接 MSPM0 RX，K230 RX 接 MSPM0 TX。

## 5. 推荐中断

- 1 ms SysTick 或 Timer：维护 `g_systick_ms`。
- UART_K230 RX：把接收字节放入环形缓冲区，`hal_uart_k230_read_byte()` 非阻塞取出。
- I2C 可先用阻塞式，代码简单，后续再改 DMA/中断。

## 6. 上板最小验证

1. 只验证 GPIO：按键切换模式，调试串口能看到 mode 改变。
2. 只验证 PWM：舵机回中，电机空载转动方向正确。
3. 只验证 I2C：MPU6050 初始化成功，静止 yaw 变化很小。
4. 只验证 UART：K230 发送 `B:0.0`，调试输出中 `vision_valid=1` 或 ball 值更新。
5. 再合并运行模式一、二、三。
