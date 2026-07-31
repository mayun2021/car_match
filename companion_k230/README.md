# K230 第1问画面 + 第3问摆球闭环

`main.py` 面向正点原子 K230 的 CanMV MicroPython。上电后立即显示 640×480
灰度画面并叠加球框、像素/毫米坐标和任务状态；收到 TI 的 Q3 命令后，K230
直接控制接在 IO42/PWM0 的 MG996R。

## 接线

| 信号 | TI 天猛星 MSPM0G3507 | K230 |
|---|---|---|
| TI 发命令 | PA10 / TX | IO45 / UART2_RX |
| K230 回传 | PA11 / RX | IO44 / UART2_TX |
| 通信参考地 | GND | GND |
| 舵机 PWM | 不接 TI | IO42 / PWM0 |

MG996R 红线接独立 5–6 V、至少 3 A 稳压电源，棕/黑线接电源 GND，橙/黄
信号线接 IO42。舵机电源 GND、K230 GND、TI GND 必须共地。TI PA7 悬空。

CanMV-K230 的 IO42 对应 PWM0；PWM3 对应 IO47。不要把代码改成
`FPIOA.PWM3`。

## 放入 K230

1. 用 CanMV IDE 连接 K230。
2. 把本目录 `config.py` 和 `main.py` 一起复制到脚本根目录。
3. 先在 IDE 手动运行，确认画面、PX、MM、V 状态和 1500 us 中位。
4. 完成下面的标定并确认方向。
5. 最后把 `main.py` 设为开机脚本。

第1问画面不需要 TI 命令。赛题要求的环外显示、完整录像和回放必须由实际的
无线发送/接收或录像设备完成；车上 K230 屏只能作为本地预览。

## 必做三点标定

1. 固定相机、灯光、摆杆和球槽，暂不启动 Q3。
2. 把球放在 `-5 cm`，读取画面 `PX`，填入 `CAL_PX_NEG50`。
3. 放在中心 O，填入 `CAL_PX_ZERO`。
4. 放在 `+5 cm`，填入 `CAL_PX_POS50`。
5. 三个值可以递增或递减，但必须互不相同，相邻至少差 20 px。
6. 调好 ROI 和灰度阈值后，把：

```python
CALIBRATION_CONFIRMED = True
```

未确认时 Q1 画面仍工作，但 Q3 会报故障码 3 并拒绝驱动舵机。这是为了防止
示例像素值造成失控，不能跳过。

三点分段换算能补偿透视：负半轴用 `-50→0`，正半轴用 `0→+50`，比整槽
单比例更适合 1 cm 精度要求。

## 识别参数

先只调 `config.py`：

- `BALL_ROI`：只框住凹槽，避开黑边、螺钉、齿条和反光区。
- `BALL_GRAY_THRESHOLDS`：默认 `(0,92)` 找暗球；以球框和 `V:1` 为准。
- `BALL_AREA_MIN/MAX`：排除灰尘、螺钉和大块阴影。
- `BALL_MAX_JUMP_PX`：限制相邻帧假跳；丢失后会重新捕获。

连续丢球超过 700 ms 会回中并报错。任一帧没有可靠识别到球时，立即冻结
积分/微分并让舵机缓回中；重捕首帧重新建立速度，避免用旧像素产生 D 冲击。

## TI ↔ K230 协议（115200, 8N1）

TI 发：

```text
$CMD,Q3,START,<seq>,50,-50\n
$CMD,Q3,ABORT,<seq>\n
$CMD,Q3,NEUTRAL,<seq>\n
$CMD,Q3,KEEP,<seq>\n
```

K230 回：

```text
$ACK,<seq>,START|ABORT|NEUTRAL\n
$H3,<seq>,<state>,<x_mm>,<target_mm>,<error_mm>,<servo_us>,<valid>,<elapsed_ms>\n
$DONE,<seq>,<elapsed_ms>,<error_mm>\n
$FAULT,<seq>,<code>\n
```

旧 `$BALL` 调试帧已停止发送，防止无序号帧掩盖 Q3 链路中断。H3 每 40 ms
发送一次。TI 每 100 ms 发送 KEEP；K230 450 ms 收不到同序号 KEEP 就报错并
缓回中。每次 START 前 TI 先发同序号 NEUTRAL，清除 TI 复位前的旧会话。状态：

| state | 含义 |
|---:|---|
| 0 | IDLE |
| 1 | 去 +5 cm |
| 2 | 去 -5 cm |
| 3 | -5 cm 最终稳定确认 |
| 4 | DONE，仍闭环保持 -5 cm |
| 5 | FAULT，舵机安全回中 |

故障码：

| code | 含义 |
|---:|---|
| 1 | 命令格式错误 |
| 2 | 目标范围错误 |
| 3 | 三点标定未确认/无效 |
| 4 | 视觉持续丢失 |
| 5 | +5 cm 阶段超时 |
| 6 | 总动作超过 4.8 s |
| 7 | Python 运行异常 |
| 8 | DONE 后偏差 >10 mm 持续 500 ms |
| 9 | TI 主控 KEEP 超时 |

进入 DONE 时动作时间会锁存；后续 H3 不会继续增加该时间。DONE 后仍持续
PD 闭环，丢球或保持超差会用同一 `seq` 报错。

## Q3 调参顺序

1. 架空/解除齿条负载，先验证 1500 us 中位。
2. START 后若球越控越远，只把 `SERVO_DIRECTION` 从 `1` 改为 `-1`。
3. 太慢：小幅提高 `BALL_KP_US_PER_MM`。
4. 到点来回振荡：先提高 `BALL_KD_US_PER_MM_S`，再小幅降低 KP。
5. 长期有小偏差：最后才少量增加 KI。
6. 不要扩大 1300–1700 us 安全范围；先解决齿条摩擦、回差和电源压降。

默认到点容差 7 mm，严于题目 10 mm。最终必须固定全部机械结构后连续测试
至少 10 次，记录总时间、+5 cm 最大误差、-5 cm 保持误差和丢球次数。
