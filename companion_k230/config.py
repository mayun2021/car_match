"""H题第1/3问 K230 现场参数表。

只改本文件即可完成 ROI、灰度阈值、三点像素标定和舵机方向调整。
所有长度单位为 mm，舵机脉宽单位为 us。
"""

# -------------------- 摄像头与显示 --------------------
FRAME_WIDTH = 640
FRAME_HEIGHT = 480

# 正点原子 K230 配套屏通常为 ST7701；若只在 CanMV IDE 看画面，改成 "VIRT"。
DISPLAY_DRIVER = "ST7701"
DISPLAY_WIDTH = 640
DISPLAY_HEIGHT = 480
DISPLAY_TO_IDE = True

# 只在该矩形内找球：(x, y, w, h)。ROI 应覆盖凹槽，但避开黑色边框、螺钉和舵机。
BALL_ROI = (55, 120, 530, 235)

# 灰度阈值。钢球在白色凹槽中通常是较暗目标；现场用 CanMV 阈值编辑器微调上限。
BALL_GRAY_THRESHOLDS = [(0, 92)]
BALL_PIXELS_MIN = 70
BALL_AREA_MIN = 110
BALL_AREA_MAX = 7200
BALL_ASPECT_MIN = 0.55
BALL_ASPECT_MAX = 1.80
BALL_DENSITY_MIN = 0.22
BALL_DENSITY_MAX = 0.96
BALL_EXPECTED_AREA = 1000

# 时序门控：锁定后拒绝一帧内跳得过远的假目标；连续丢失若干帧后允许全 ROI 重捕。
BALL_MAX_JUMP_PX = 65
BALL_REACQUIRE_FRAMES = 5
BALL_EMA_ALPHA = 0.38
VISION_VALID_MIN_FRAMES = 2
VISION_TIMEOUT_MS = 220
VISION_FAULT_MS = 700

# -------------------- 三点像素标定 --------------------
# 把钢球依次放到 -5 cm、O、+5 cm 刻线，读取画面左上角 PX 数值后填到这里。
# 支持画面左右颠倒，因此三个数既可递增也可递减，但必须互不相同。
# 填完实测值后把 CALIBRATION_CONFIRMED 改成 True；False 时 Q3 安全锁定，
# 但第1问实时画面仍正常工作。
CALIBRATION_CONFIRMED = False
CAL_PX_NEG50 = 176.0
CAL_PX_ZERO = 320.0
CAL_PX_POS50 = 464.0
BALL_MM_LIMIT = 80.0

# -------------------- UART2 与舵机 --------------------
# K230 IO44=UART2_TX，IO45=UART2_RX。
# 官方 FPIOA 表规定 IO42 对应 PWM0（PWM3 对应 IO47），不可写成 PWM3。
UART_BAUD = 115200
UART_TX_IO = 44
UART_RX_IO = 45
SERVO_PWM_IO = 42
SERVO_PWM_CHANNEL = 0
SERVO_PWM_FUNCTION = "PWM0"
SERVO_PWM_HZ = 50

# MG996R 安全范围：本程序任何路径都不会越过 1300~1700 us。
SERVO_NEUTRAL_US = 1500
SERVO_MIN_US = 1300
SERVO_MAX_US = 1700
SERVO_DIRECTION = 1          # 越控越远时只改成 -1
SERVO_SLEW_US_PER_FRAME = 18

# 球位置 PD + 小积分。先只调 KP/KD；有恒定偏差时再少量增加 KI。
BALL_KP_US_PER_MM = 2.65
BALL_KI_US_PER_MM_S = 0.08
BALL_KD_US_PER_MM_S = 0.42
BALL_INTEGRAL_LIMIT = 220.0
BALL_SPEED_FILTER_ALPHA = 0.28
BALL_SPEED_LIMIT_MM_S = 500.0

# 赛题第3问：+5 cm 到达后反向，再在 -5 cm 稳定，整个过程必须在 5 s 内完成。
TARGET_TOLERANCE_MM = 7.0
TARGET_MAX_SPEED_MM_S = 42.0
POS_STABLE_MS = 150
NEG_STABLE_MS = 180
FINAL_HOLD_MS = 220
POS_TIMEOUT_MS = 2350
TOTAL_DEADLINE_MS = 4800

# DONE 后仍持续闭环；若最终位置持续跑出题目允许的 +/-10 mm，
# 向 TI 上报同序号故障；TI 会保留已完成的动作时间，同时明确显示保持故障。
DONE_ERROR_LIMIT_MM = 10.0
DONE_ERROR_FAULT_MS = 500

# 25 Hz 上报 H3；旧版 BALL 调试帧已停用。
TELEMETRY_PERIOD_MS = 40

# TI 在 Q3 运行/完成保持期间每 100 ms 发送 KEEP。若 TI 复位、掉线或停止
# 喂狗，K230 最迟 450 ms 内回中，避免旧任务在无人监督时继续驱动舵机。
HOST_KEEPALIVE_TIMEOUT_MS = 450
