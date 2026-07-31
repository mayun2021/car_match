# 先看：Keil5 直接烧录

当前目录是 H 题第1至3问比赛版，不是旧的“稳态循迹 V2”固件。

## 烧录 TI 天猛星

1. 双击 `02_打开_Keil5_工程.bat`。
2. 工程应为 `keil/CarMatch_MSPM0G3507.uvprojx`。
3. 按 F7 编译，确认 `0 Error(s), 0 Warning(s)`。
4. 按 F8 下载。Keil 实际烧录：

```text
keil/Objects/CarMatch_MSPM0G3507.axf
```

若不重新编译，也可烧 `firmware/CarMatch_MSPM0G3507_Q1_Q3_20260731.axf`。
HEX 是同一程序的通用格式，不需要 AXF、HEX 两个都烧。

## 烧录后正常现象

- 上电默认显示 `Q2 READY`。
- K2 在 Q2/Q3 间切换；K3 开始/急停；K1 清结果与校准。
- Q2 必须把车稳定放在 A 白线（`MASK=0`）或宽黑横线（`MASK=F`）约
  0.1 s 后按 K3；`MASK=6` 是正常中心循迹线，不是 A 起点。
- `ALIGN A MARK` 表示 A 标志不稳定；`MPU NEEDED` 表示 MPU6050 未在线。
- Q2 返回 A 后显示 `BRAKE`，随后 `LAP DONE`，计时定格。

## 第3问还必须部署 K230 脚本

TI 固件只负责任务命令、计时与监督。请同时将：

```text
companion_k230/config.py
companion_k230/main.py
```

复制到 K230，并把 `main.py` 设为开机脚本。舵机信号只接 K230 IO42/PWM0，
TI PA7 悬空。完成三点实测后把 `CALIBRATION_CONFIRMED=True`，否则 Q3
按设计保持安全锁定。

完整接线和首次测试请看
[Keil5_烧录与首次运行说明.md](Keil5_烧录与首次运行说明.md)。
