# 天猛星 MSPM0G3507：Keil5 烧录与首次运行

## 1. 需要烧录哪个文件

最稳妥的方法是直接打开：

`keil/CarMatch_MSPM0G3507.uvprojx`

在 Keil 中按 `F7` 后再按 `F8`。Keil 实际下载的是：

`keil/Objects/CarMatch_MSPM0G3507.axf`

同时提供两个独立成品：

- `firmware/CarMatch_MSPM0G3507.axf`：Keil 调试/下载文件。
- `firmware/CarMatch_MSPM0G3507.hex`：通用 Intel HEX 烧录文件。
- `firmware/CarMatch_MSPM0G3507_STABLE_V2.axf`：同一固件的醒目 V2 文件名副本。
- `firmware/CarMatch_MSPM0G3507_STABLE_V2.hex`：同一固件的醒目 V2 文件名副本。

不要烧录 `tools` 或 `simulate_control`，它们只用于电脑仿真。

## 2. XDS110 接线

断电接线后再上电：

| XDS110 | 天猛星 |
|---|---|
| SWDIO | DIO / SWDIO |
| SWCLK | CLK / SWCLK |
| GND | GND |
| RST | RST / NRST |

目标板还要有正常的 3.3V 逻辑电源。若小车电池给天猛星供电，不要再把两个不同电源的 5V 输出硬并联，但所有模块必须共地。

## 3. Keil 设置与烧录

1. 双击 `keil/CarMatch_MSPM0G3507.uvprojx`。
2. `Project -> Options for Target -> Debug`，选择 `CMSIS-DAP Debugger`。
3. 点 `Settings`，Port 选 `SW`，时钟先用 1 MHz。
4. `Flash Download` 中确认存在 `MSPM0G1X0X_G3X0X_MAIN_128KB` 算法。
5. 按 `F7`。本包验证结果应为 `0 Error(s), 0 Warning(s)`。
6. 按 `F8`。看到 `Flash Load finished` 后，按一次板上 RST 或重新上电。

## 4. 首次运行

1. 先把车轮架空，防止接反后突然冲出桌面。
2. 四路模块从车头看由左至右接 PB2、PB3、PB4、PB5；供电接 3.3V 和 GND。
3. 探头置于黑线中央时，正常中心状态是 `MASK=6`。你目前提高安装高度后能稳定到 6，说明传感器没有坏，应保持该高度并逐路调电位器。
4. OLED 第一行应显示 `V2`。显示 `ALIGN V2` 时说明探头尚未对准，程序会拒绝启动。
5. 上电后程序处于停止状态，TB6612 的 STBY 被拉低。
6. 调整到 OLED 显示 `MASK:0110`，再短按底板 `A30`（你一直使用的 K3）启动；再次短按停止。
7. `A27` 切换循迹/滚球/组合模式；只测试循迹时保持模式 1。
8. `A17` 用于静止校准 MPU6050。

V2 不再使用“一只轮正转、一只轮反转”的原地找线。运行中短暂丢线只会低速
缓弯，持续丢线 350 ms 会自动回到停止状态。

## 5. 本版固定引脚

| 功能 | MSPM0G3507 |
|---|---|
| 循迹 X1/X2/X3/X4 | PB2/PB3/PB4/PB5 |
| PWMA/PWMB | PA8/PA9，2.5 kHz |
| AIN1/AIN2 | PA24/PA28 |
| BIN1/BIN2 | PA22/PA14 |
| TB6612 STBY | PA25 |
| OLED/MPU6050 SCL/SDA | PA15/PA16 |
| 模式/启动/校准 | PA27/PA30/PA17 |

`PB2` 与 TB6612 的 `BO2` 不是同一种名称：PB2 是 MSPM0 的 GPIO，BO2 是电机驱动输出端，绝不能互接。TB6612 的 AO1/AO2 和 BO1/BO2 只接两只电机。

## 6. 上电仍不转时只查这四项

1. OLED 按 A30 后运行状态是否变化；若不变，先检查 A30/PA30。
2. TB6612 `VM` 对 GND 是否有电机电源，`VCC` 是否为 3.3V。
3. 启动后 TB6612 `STBY` 对 GND 是否约为 3.3V。
4. AO1/AO2、BO1/BO2 是否只接电机，且 MSPM0、TB6612、电池共地。

如果以上均正确，再用万用表测 PA8/PA9 的平均电压是否随速度命令变化。
