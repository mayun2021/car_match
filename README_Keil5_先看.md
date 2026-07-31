# car_match MSPM0G3507 Keil5 可烧录工程

本目录是“稳态循迹 V2”。OLED 第一行必须能看到 `V2`；若看不到，说明仍在
运行旧固件。

本目录由用户提供的 `car_match-main (1).zip` 中的应用代码移植而来。原压缩包的控制层完整，
但 `src/platform/hal_ti_mspm0.c` 仍是 TODO 模板，也没有启动文件、链接脚本和
Keil 工程。本版本已经补齐真实 MSPM0G3507 外设实现，并通过 Keil Arm Compiler
6.7 完整编译、链接和 HEX 生成验证。

## 最快烧录步骤

1. 第一次使用时双击 `01_安装_MSPM0G3507_器件包.bat`，按 Keil Pack Installer
   提示完成安装；如果 Keil 已能识别 MSPM0G3507，可跳过。
2. 双击 `02_打开_Keil5_工程.bat`。
3. 用 XDS110/CMSIS-DAP 连接目标板，确认 Keil 工程目标为
   `CarMatch_MSPM0G3507`。
4. 点击 Build 或按 `F7`；点击 Download 或按 `F8` 烧录。

已经编译好的文件位于：

- `firmware/CarMatch_MSPM0G3507.hex`
- `firmware/CarMatch_MSPM0G3507.axf`

第一次上电请架空驱动轮，舵机使用独立 5 V/6 V 大电流电源，并与主控共地。

循迹启动前必须将探头对准黑线中央。屏幕显示 `ALIGN V2` 时程序会拒绝启动；
调整到 `MASK=6`（OLED 为 `MASK:0110`）后，再短按 A30/K3。

## 固定引脚

| 模块 | 信号 | MSPM0G3507 |
|---|---|---|
| YB-MVX01 | X1/X2/X3/X4（左到右） | PB2/PB3/PB4/PB5 |
| TB6612 | PWMA/PWMB | PA8/PA9，2.5 kHz |
| TB6612 | AIN1/AIN2 | PA24/PA28 |
| TB6612 | BIN1/BIN2 | PA22/PA14 |
| TB6612 | STBY | PA25 |
| MG996R | PWM | PA7，50 Hz |
| MPU6050 | SCL/SDA | PA15/PA16 |
| K230 | MCU TX / MCU RX | PA10 / PA11，115200 8N1 |
| 按键 | MODE/START/CALIB | PA27/PA30/PA17 |

K230 必须交叉连接：

- K230 RX 接 PA10（MSPM0 TX）
- K230 TX 接 PA11（MSPM0 RX）
- 两者 GND 共地

原压缩包文档把 K230 的 TX/RX 对应关系写反；本工程按 MSPM0G3507 的真实复用
功能修正为 PA10=UART0_TX、PA11=UART0_RX。

## 按键与模式

- A27 / MODE：空闲时依次切换循迹、滚球、组合三种模式。
- A30 / K3 / START：开始或停止当前模式。
- A17 / CALIB：空闲时校准 MPU6050 陀螺仪并让舵机回中。

按键默认低电平按下，YB-MVX01 默认低电平表示检测到黑线。

## K230 文本协议

K230 每 20~50 ms 发送一行，支持：

```text
B:-12.5
$BALL,8.0,120,1
```

`x_mm` 左负右正。超过 200 ms 没有有效视觉数据时，舵机自动回中。

## 重要参数

比赛现场主要修改 `include/robot_config.h`：

- 电机左右反向
- 黑线有效电平
- 循迹基础速度和 PID
- 舵机方向、脉宽范围和滚球 PID
- K230 超时时间

V2 实车保守参数为：基础 PWM 22%、转向修正不超过 14%、两轮始终同向；
丢线时只缓弯寻找，超过 350 ms 自动停车，不再无限原地转圈。

## 已完成验证

- Keil Arm Compiler 6.7
- MSPM0G3507SPTR LQFP-48
- 0 Error(s), 0 Warning(s)
- Code=14068，RO-data=1320，RW-data=4，ZI-data=5892
- 已生成 AXF/HEX

编译通过不等于实车参数已经标定。电机相序、传感器安装左右顺序、黑白电平、
舵机机械方向和 K230 坐标方向仍需按实车架空确认。

**注意**：以上是旧固件的编译记录，当时"跑完一圈"靠四探头全黑停车线判定。
源码已改成纯偏航角判定（从 0 转过 `ROBOT_LINE_FINISH_YAW_DEG`，默认 350°，
即视为跑完一圈直接停车），`firmware/` 里已编译好的文件是改动前的版本，
上板前请在 Keil5 里按 F7 重新编译。
