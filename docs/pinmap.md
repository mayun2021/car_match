# 最终接线表

## YB-MVX01 V1.2

| 模块 | 车体位置 | MSPM0 |
|---|---|---|
| X1 | 最左 | PB2 |
| X2 | 左内 | PB3 |
| X3 | 右内 | PB4 |
| X4 | 最右 | PB5 |
| VCC | — | 3.3 V |
| GND | — | GND |

黑线时低电平。不要把 `PB2` 和 TB6612 的 `BO2` 混淆：PB2 是主控输入，
BO2 是 H 桥功率输出，只能接电机。

## 板载 TB6612 与 MG310

| TB6612 | MSPM0/连接 |
|---|---|
| PWMA | PA8 |
| AIN1 | PA24 |
| AIN2 | PA28 |
| AO1/AO2 | 左轮电机 M+/M- |
| PWMB | PA9 |
| BIN1 | PA22 |
| BIN2 | PA14 |
| BO1/BO2 | 右轮电机 M+/M- |
| STBY | PA25 |
| VCC | 3.3 V |
| VM | 电机电源 |
| GND/PGND | 共地 |

MG310 尾部 A/B/VCC/GND 是编码器信号与电源，M+/M- 才是电机功率线。

## OLED 与 MPU6050

| 信号 | MSPM0 |
|---|---|
| SCL | PA15 |
| SDA | PA16 |
| 逻辑电源 | 3.3 V |
| GND | GND |

两者共用 I2C。OLED 地址默认 0x3C，MPU6050 为 0x68。

## 按键

| 功能 | 板上标记 | MSPM0 |
|---|---|---|
| 模式 | K2/A27 | PA27 |
| 开始/急停 | K3/A30 | PA30 |
| 清除/校准/舵机回中 | K1/A17 | PA17 |

## K230 UART

| 方向 | TI | K230 |
|---|---|---|
| TI 发命令 | PA10/TX | IO45/UART2_RX |
| K230 回传 | PA11/RX | IO44/UART2_TX |
| 参考地 | GND | GND |

UART 115200、8N1。TX/RX 必须交叉。

## MG996R

| 舵机线 | 接法 |
|---|---|
| 橙/黄信号 | K230 IO42 / PWM0 |
| 红线 | 独立 5–6 V、≥3 A |
| 棕/黑线 | 独立电源 GND，并与 K230/TI 共地 |

TI PA7 不接舵机。禁止 K230 IO42 与 PA7 并接。
