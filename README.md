# H题：车载平衡滚球运动控制系统软件工程

> 稳态循迹 V2：针对实车视频中启动后连续原地旋转的问题，增加对线启动互锁、
> 三采样数字滤波、无反转丢线缓弯、350 ms 丢线停车和启动偏航保护。OLED
> 第一行会显示 `V2`，用于确认已经烧录本修正版。

本工程面向立创天猛星 MSPM0G3507 主控板，完成 H 题的软件部分：

- 模式一：四路红外循迹 + TB6612FNG + MG310 减速电机，实现沿黑色环形线行驶。
- 模式二：K230 视觉识别钢球位置，主控驱动 MG996R 舵机改变摆杆/水管倾角，使钢球移动并稳定到指定位置。
- 模式三：巡线与滚球控制同时运行，MPU6050 记录启动初始角度，车辆沿线行驶时保持钢球在目标位置附近。

工程按“应用层、算法层、驱动层、板级适配层”组织，便于比赛现场快速调参和替换引脚。

## 目录结构

```text
.
├── include/                 # 公开头文件与统一配置
├── src/
│   ├── app/                 # 模式状态机、PID 算法
│   ├── drivers/             # 电机、巡线、舵机、MPU6050、K230 通信、OLED 显示
│   ├── platform/            # MSPM0 板级适配层与桌面仿真桩
│   └── main.c               # 程序入口
├── docs/                    # 接线表、调参流程、K230 协议
└── tools/                   # 简单桌面仿真/算法自测
```

## 重要引脚

按你给的图纸，默认引脚如下：

| 模块 | 信号 | 引脚 |
|---|---|---|
| 红外循迹 YB-MVX01 | OUT1/OUT2/OUT3/OUT4 | PB2/PB3/PB4/PB5 |
| TB6612 A路 | PWMA/AIN1/AIN2 | PA8/PA24/PA28 |
| TB6612 B路 | PWMB/BIN1/BIN2 | PA9/PA22/PA14 |
| TB6612 | STBY | PA25 |
| MPU6050 | SDA/SCL | PA16/PA15 |
| OLED (SSD1306 0.96寸) | SDA/SCL | PA16/PA15，与 MPU6050 共用 I2C 总线，地址 0x3C |
| MG996R | PWM | PA7，若硬件不同只改配置 |
| K230 | UART RX/TX | PA10/PA11，若硬件不同只改配置 |
| 按键 MODE/START/CALIB | A27/A30/A17 | PA27/PA30/PA17 |

注意：`PA25` 专用于 TB6612 的 `STBY`，不能再接循迹或按键。工程已经按天猛星底板丝印适配：A27 切换模式、A30（K3）启动/停止、A17 校准。

## 模式操作

默认按键逻辑：

- `A27 / KEY_MODE`：切换模式一/二/三。
- `A30 / K3 / KEY_START`：启动或停止当前模式。
- `A17 / KEY_CALIB`：静止时校准 MPU6050 零偏，并把舵机回中。

默认按键为低电平按下。如果你的按键接法是高电平按下，修改 `ROBOT_KEY_PRESSED_LEVEL`。

## K230 到 TI 的串口协议

K230 每 20 ms 到 50 ms 发送一帧 ASCII 文本，任选一种格式：

```text
B:<x_mm>\n
$BALL,<x_mm>,<y_px>,<valid>\n
```

示例：

```text
B:-12.5
$BALL,8.0,120,1
```

其中 `x_mm` 是钢球相对摆杆中心的横向位置，单位 mm，左负右正；`valid=1` 表示识别有效。

## Keil5 直接编译和烧录

工程已经包含 MSPM0G3507 启动文件、TI DriverLib、等效 SysConfig 生成文件和 CMSIS-DAP 下载配置，不需要再建立 CCS/Theia 工程：

1. 打开 `keil/CarMatch_MSPM0G3507.uvprojx`。
2. 在 Keil 中按 `F7` 编译，正常结果为 0 Error、0 Warning。
3. XDS110 按 SWDIO、SWCLK、GND、RST 与天猛星相连，并给目标板供电。
4. 按 `F8` 下载；下载目标是 `CarMatch_MSPM0G3507`，不要烧录桌面仿真程序。
5. 上电后先架空车轮，短按底板 A30（K3）启动/停止。

循迹模式下，启动前应把探头放到黑线中央，使 OLED 显示 `MASK:0110`
（十进制 MASK=6）。若显示 `ALIGN V2`，按 A30 不会驱动电机，这是本版的
防原地旋转保护，不是烧录失败。

完整步骤见 `Keil5_烧录与首次运行说明.md`。如果只想在电脑上看算法是否能跑，也可使用桌面桩：

```bash
make sim
./build/simulate_control
```

## 调参顺序

推荐顺序：

1. 只接电机，确认左右轮方向，必要时改 `ROBOT_LEFT_MOTOR_REVERSE` 或 `ROBOT_RIGHT_MOTOR_REVERSE`。
2. 只跑模式一，先低速调巡线 PID。
3. 只跑模式二，确认 K230 输出方向和舵机方向。
4. 最后跑模式三，同时降低小车速度和滚球 PID，避免两个闭环互相打架。

详细流程见 [docs/tuning.md](docs/tuning.md)。
