# SysConfig 复建对照（当前比赛版）

本包已经包含可在 Keil5 直接编译的 `generated/ti_msp_dl_config.*`，普通烧录
不需要重新运行 SysConfig。本页仅用于以后在 CCS/Theia 复建外设。

## GPIO

| 名称 | 方向 | 引脚 | 说明 |
|---|---|---|---|
| LINE_X1…X4 | 输入 | PB2/PB3/PB4/PB5 | 黑线低电平 |
| KEY_MODE | 输入上拉 | PA27 | K2 |
| KEY_START | 输入上拉 | PA30 | K3 |
| KEY_CLEAR | 输入上拉 | PA17 | K1 |
| AIN1/AIN2 | 输出 | PA24/PA28 | TB6612 A路方向 |
| BIN1/BIN2 | 输出 | PA22/PA14 | TB6612 B路方向 |
| STBY | 输出 | PA25 | TB6612 使能 |

## PWM

| 名称 | 引脚 | 频率 | 用途 |
|---|---|---:|---|
| PWM_MOTOR_LEFT | PA8 | 2.5 kHz | TB6612 PWMA |
| PWM_MOTOR_RIGHT | PA9 | 2.5 kHz | TB6612 PWMB |

当前比赛版没有 TI 舵机 PWM。MG996R 信号只接 K230 IO42/PWM0；PA7 悬空。

## I2C

| 总线 | SCL/SDA | 设备 |
|---|---|---|
| I2C0 | PA15/PA16 | SSD1306 0x3C、MPU6050 0x68 |

## UART

| 外设 | TI 引脚 | 参数 |
|---|---|---|
| UART_K230 | PA10 TX、PA11 RX | 115200, 8N1 |

交叉连接：

```text
PA10/TX -> K230 IO45/UART2_RX
PA11/RX <- K230 IO44/UART2_TX
GND     -- GND
```

RX 中断把字节放入环形缓冲区，应用层非阻塞解析一行 ASCII 协议。不要使用旧文档
中的 `B:0.0` 或 `$BALL` 作为任务心跳；当前协议见 `k230_protocol.md`。
