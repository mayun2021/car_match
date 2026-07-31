# 第1至3问：烧录、接线与首次运行

## 1. TI 天猛星烧录哪个文件

推荐打开：

```text
keil\CarMatch_MSPM0G3507.uvprojx
```

按 F7 编译，再按 F8 下载。Keil 实际下载的是：

```text
keil\Objects\CarMatch_MSPM0G3507.axf
```

不重新编译时，可直接烧录：

```text
firmware\CarMatch_MSPM0G3507_Q1_Q3_20260731.axf
```

通用烧录器可使用同目录 `.hex`。AXF 和 HEX 是同一次编译的两个格式，只选一个。

## 2. XDS110 与 Keil

断电接线：

| XDS110 | 天猛星 |
|---|---|
| SWDIO | SWDIO / DIO |
| SWCLK | SWCLK / CLK |
| GND | GND |
| RST | NRST / RST |

在 Keil 的 `Options for Target → Debug` 选择 `CMSIS-DAP Debugger`，Port 选
`SW`。`Flash Download` 中应有
`MSPM0G1X0X_G3X0X_MAIN_128KB` 算法。F8 显示 `Flash Load finished`
后按 RST 或重新上电。

## 3. 上电前接线

### TI 与 K230

```text
TI PA10 / TX  -> K230 IO45 / UART2_RX
TI PA11 / RX  <- K230 IO44 / UART2_TX
TI GND        -- K230 GND
```

### MG996R

```text
K230 IO42 / PWM0 -> 舵机信号线
独立 5–6 V / >=3 A -> 舵机红线
独立电源 GND -> 舵机棕/黑线
舵机电源 GND、K230 GND、TI GND 三者共地
```

TI PA7 必须悬空，不能和 IO42 并接。K230 若由天猛星 5 V 供电，必须确认该
5 V 稳压能持续带动 K230；一旦相机启动重启或花屏，应改为独立稳定 5 V
供电并共地。舵机不能和 K230 共用板载小电流 5 V。

## 4. 部署 K230

1. 用 CanMV IDE 同时复制 `companion_k230/config.py` 和 `main.py`。
2. 手动运行，确认 640×480 画面、球框和 `V:1` 正常。
3. 把球依次放在 -5 cm、O、+5 cm，填写三点 `CAL_PX_*`。
4. ROI、灰度阈值和三点值确认后，将
   `CALIBRATION_CONFIRMED = True`。
5. 先架空/解除齿条负载验证舵机方向；越控越远时只反转
   `SERVO_DIRECTION`。
6. 最后把 `main.py` 设为开机脚本。

第1问画面上电常开。赛题所需的环外实时显示、完整录像和回放，需要把你们的
实际无线图传接收/存储设备放在赛道外；车上的 K230 屏不能代替环外接收端。

## 5. 第2问首次验证

1. 先架空驱动轮，接好电机 VM 动力电源。
2. OLED 应显示 `Q2 READY`。
3. 正常中心黑线约为 `MASK=6`；白色 A 断线为 `MASK=0`；若 A 是宽黑横线，
   则为 `MASK=F`。
4. 按题图把小车朝顺时针方向放在 A。程序不决定顺/逆时针，车头朝向决定。
5. 稳定放置至少 0.1 s 后短按 K3。应先显示 `LEAVE A`，驶离后显示 `FOLLOW`。
6. 返回 A 后显示 `BRAKE`，再显示 `LAP DONE`，时间保持不变。
7. 运行中按 K3 是急停；K2 切 Q3；K1 清结果并重新校准 MPU。

若显示 `ALIGN A MARK`，说明最近 16 帧没有至少 12 帧一致。把探头重新对准
A 标志，静止 0.1 s 再按 K3。若显示 `MPU NEEDED`，先修复 PA15/PA16
I2C、MPU 地址或供电，程序不会冒险起跑。

第一次落地务必系安全绳。新高车架下先保留：

```text
BASE=220  STRAIGHT=245  KP=0.24  MAX_CORRECTION=140
```

不要同时改速度、KP、探头高度和电机方向。

## 6. 第3问首次验证

1. 小车保持静止，K2 切到 `Q3 READY`。
2. K230 画面必须持续有球框并显示 `V:1`。
3. 把钢球放到 O 点。TI 只在 K230 在线、视觉有效且球位于 O±10 mm 时启动。
4. 按 K1 可让舵机回中，OLED 脉宽应接近 1500 us。
5. 按 K3 后依次显示 `TO +5CM`、`TO -5CM`、`HOLD -5`、`BALL DONE`。
6. 完成后 K230 继续保持 -5 cm；OLED 锁存从 K3 按下到完成的 TI 本地总时间。

动作必须同时满足 K230 时间和从 K3 按下起的 TI 时间均不超过 5.000 s。
DONE 后若球偏差超过 10 mm 持续 500 ms，或同序号状态超过 500 ms 未更新，
OLED 保留完成时间但显示故障，不能把失稳当作持续成功。

## 7. OLED 故障对照

| OLED | 先检查 |
|---|---|
| `ALIGN A MARK` | A 标志、探头高度、等待 0.1 s 后再按 |
| `MPU NEEDED` | MPU6050 供电、0x68 地址、PA15/PA16 |
| `NO K230 ACK` | PA10→IO45、PA11←IO44、共地、115200 |
| `K230/VIS LOST` | K230 脚本、球框、O 点 ±10 mm、相机遮挡 |
| `K230 FAULT` | 三点确认锁、舵机方向、远端故障码 |
| `BALL TIMEOUT` | 机械摩擦、供电、PD 参数、动作是否超过 5 s |
| `LINE LOST` | 四路高度、电位器、线束、环境光 |
| `SPIN/WIRING` | 左右轮正方向、X1/X4 是否镜像 |

## 8. 上场前只按这个顺序调

1. 固紧高立柱、顶梁和长线束，电池尽量低且左右对称。
2. 探头横梁与车轴平行，高度固定；中心线稳定显示 `MASK=6`。
3. 用默认 Q2 参数跑一圈并录像，记录圈时和停止偏差。
4. 停过 A 点：主动刹车每次增加 10 ms；停在 A 前：每次减少 10 ms。
5. 弯道稳定但圈时 >18 s 时，直线速度每次只增加 5–10。
6. K230、相机、灯光、摆杆固定后，最后再做三点标定和 Q3 调参。
