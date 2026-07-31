"""2026 H题第1/3问 K230 CanMV MicroPython 程序。

功能：
1. 上电立即显示凹槽与钢球实时画面（第1问图传源）。
2. UART2 接收 TI MSPM0G3507 的第3问命令。
3. K230 本机识别钢球并用 IO42/PWM0 闭环控制 MG996R：
   O -> +50 mm -> -50 mm，完成后继续闭环保持 -50 mm。

程序面向 CanMV-K230 v1.2+ API。现场参数集中在 config.py。
"""

import gc
import os
import time

from machine import FPIOA, PWM, UART
from media.display import Display
from media.media import MediaManager
from media.sensor import Sensor

import config as cfg


# 与 TI 工程约定的 H3 状态值。
H3_IDLE = 0
H3_POS = 1
H3_NEG = 2
H3_HOLD = 3
H3_DONE = 4
H3_FAULT = 5

# 故障码；TI 端按数值显示即可。
FAULT_BAD_COMMAND = 1
FAULT_BAD_TARGET = 2
FAULT_BAD_CALIBRATION = 3
FAULT_VISION_LOST = 4
FAULT_POS_TIMEOUT = 5
FAULT_TOTAL_TIMEOUT = 6
FAULT_RUNTIME = 7
FAULT_HOLD_LOST = 8
FAULT_HOST_TIMEOUT = 9


def clamp(value, low, high):
    if value < low:
        return low
    if value > high:
        return high
    return value


def ticks_since(now_ms, then_ms):
    return time.ticks_diff(now_ms, then_ms)


class ThreePointCalibration:
    """用 -50/0/+50 mm 三点做分段线性像素换算，兼容画面左右翻转。"""

    def __init__(self):
        self.p_neg = float(cfg.CAL_PX_NEG50)
        self.p_zero = float(cfg.CAL_PX_ZERO)
        self.p_pos = float(cfg.CAL_PX_POS50)

    def valid(self):
        d1 = abs(self.p_zero - self.p_neg)
        d2 = abs(self.p_pos - self.p_zero)
        same_direction = ((self.p_zero - self.p_neg) *
                          (self.p_pos - self.p_zero)) > 0.0
        return d1 >= 20.0 and d2 >= 20.0 and same_direction

    @staticmethod
    def _map(px, px0, mm0, px1, mm1):
        return mm0 + (px - px0) * (mm1 - mm0) / (px1 - px0)

    def px_to_mm(self, px):
        if not self.valid():
            return 0.0
        # 判断当前像素是否位于物理负方向一侧，不依赖相机是否镜像。
        on_negative_side = ((px - self.p_zero) *
                            (self.p_neg - self.p_zero)) >= 0.0
        if on_negative_side:
            mm = self._map(px, self.p_neg, -50.0,
                           self.p_zero, 0.0)
        else:
            mm = self._map(px, self.p_zero, 0.0,
                           self.p_pos, 50.0)
        return clamp(mm, -cfg.BALL_MM_LIMIT, cfg.BALL_MM_LIMIT)


class BallTracker:
    """灰度 blob + 形状过滤 + 时序门控 + EMA 的轻量钢球跟踪器。"""

    def __init__(self, calibration):
        self.calibration = calibration
        self.last_px = None
        self.filtered_px = None
        self.lost_frames = 99
        self.valid_streak = 0
        self.locked = False
        self.last_valid_ms = 0
        self.detected_this_frame = False
        self.x_mm = 0.0
        self.y_px = 0
        self.rect = None

    @staticmethod
    def _density(blob):
        try:
            return float(blob.density())
        except Exception:
            area = max(1, blob.w() * blob.h())
            return float(blob.pixels()) / float(area)

    def _candidate_score(self, blob):
        w = blob.w()
        h = blob.h()
        area = w * h
        if h <= 0 or area < cfg.BALL_AREA_MIN or area > cfg.BALL_AREA_MAX:
            return None

        aspect = float(w) / float(h)
        density = self._density(blob)
        if (aspect < cfg.BALL_ASPECT_MIN or aspect > cfg.BALL_ASPECT_MAX or
                density < cfg.BALL_DENSITY_MIN or
                density > cfg.BALL_DENSITY_MAX):
            return None

        cx = blob.cx()
        if (self.last_px is not None and
                self.lost_frames < cfg.BALL_REACQUIRE_FRAMES):
            jump = abs(cx - self.last_px)
            if jump > cfg.BALL_MAX_JUMP_PX:
                return None
        else:
            jump = 0.0

        area_error = abs(area - cfg.BALL_EXPECTED_AREA)
        aspect_error = abs(aspect - 1.0) * cfg.BALL_EXPECTED_AREA
        return area_error + aspect_error + jump * 12.0

    def update(self, img, now_ms):
        self.detected_this_frame = False
        best = None
        best_score = None
        blobs = img.find_blobs(
            cfg.BALL_GRAY_THRESHOLDS,
            roi=cfg.BALL_ROI,
            x_stride=2,
            y_stride=2,
            pixels_threshold=cfg.BALL_PIXELS_MIN,
            area_threshold=cfg.BALL_AREA_MIN,
            merge=False,
        )

        for blob in blobs:
            score = self._candidate_score(blob)
            if score is not None and (best_score is None or score < best_score):
                best = blob
                best_score = score

        if best is None:
            self.lost_frames += 1
            if self.lost_frames >= cfg.BALL_REACQUIRE_FRAMES:
                self.valid_streak = 0
            if (self.last_valid_ms == 0 or
                    ticks_since(now_ms, self.last_valid_ms) >
                    cfg.VISION_TIMEOUT_MS):
                self.locked = False
            self.rect = None
            return False

        measured_px = float(best.cx())
        self.detected_this_frame = True
        if self.filtered_px is None or self.lost_frames >= cfg.BALL_REACQUIRE_FRAMES:
            self.filtered_px = measured_px
        else:
            alpha = cfg.BALL_EMA_ALPHA
            self.filtered_px += alpha * (measured_px - self.filtered_px)

        self.last_px = measured_px
        self.lost_frames = 0
        self.valid_streak += 1
        self.last_valid_ms = now_ms
        self.x_mm = self.calibration.px_to_mm(self.filtered_px)
        self.y_px = best.cy()
        self.rect = best.rect()
        if self.valid_streak >= cfg.VISION_VALID_MIN_FRAMES:
            self.locked = True
        return self.locked

    def is_fresh(self, now_ms):
        return (self.locked and
                ticks_since(now_ms, self.last_valid_ms) <= cfg.VISION_TIMEOUT_MS)


class SafeServo:
    """所有写入都经过脉宽限幅和斜率限制。"""

    def __init__(self, fpioa):
        pwm_function = getattr(FPIOA, cfg.SERVO_PWM_FUNCTION)
        fpioa.set_function(cfg.SERVO_PWM_IO, pwm_function)
        self.current_us = int(cfg.SERVO_NEUTRAL_US)
        duty = self._pulse_to_duty(self.current_us)
        self.pwm = PWM(cfg.SERVO_PWM_CHANNEL,
                       cfg.SERVO_PWM_HZ,
                       duty,
                       enable=True)

    @staticmethod
    def _pulse_to_duty(pulse_us):
        return float(pulse_us) * float(cfg.SERVO_PWM_HZ) / 10000.0

    def set_us(self, requested_us, immediate=False):
        target = int(clamp(requested_us,
                           cfg.SERVO_MIN_US,
                           cfg.SERVO_MAX_US))
        if not immediate:
            step = int(cfg.SERVO_SLEW_US_PER_FRAME)
            target = int(clamp(target,
                               self.current_us - step,
                               self.current_us + step))
        self.current_us = target
        self.pwm.duty(self._pulse_to_duty(target))

    def neutral(self, immediate=False):
        self.set_us(cfg.SERVO_NEUTRAL_US, immediate)

    def deinit(self):
        self.neutral(True)
        self.pwm.deinit()


class H3Controller:
    """第3问的 +5 cm -> -5 cm -> 持续保持 状态机。"""

    def __init__(self, servo, uart):
        self.servo = servo
        self.uart = uart
        self.seq = 0
        self.state = H3_IDLE
        self.target_mm = 0.0
        self.target_pos_mm = 50.0
        self.target_neg_mm = -50.0
        self.error_mm = 0.0
        self.start_ms = 0
        self.stage_ms = 0
        self.stable_ms = 0
        self.last_ctrl_ms = 0
        self.last_x_mm = None
        self.speed_mm_s = 0.0
        self.integral = 0.0
        self.done_sent = False
        self.done_elapsed_ms = 0
        self.fault_code = 0
        self.calibration_ok = True
        self.vision_lost_since_ms = None
        self.done_error_since_ms = None
        self.last_host_ms = 0

    def elapsed_ms(self, now_ms):
        if self.state == H3_DONE:
            return self.done_elapsed_ms
        if self.start_ms == 0:
            return 0
        return max(0, ticks_since(now_ms, self.start_ms))

    def send(self, text):
        try:
            self.uart.write(text)
        except Exception as exc:
            print("UART write:", exc)

    def ack(self, action):
        self.send("$ACK,%d,%s\n" % (self.seq, action))

    def reset_control(self, now_ms):
        self.integral = 0.0
        self.speed_mm_s = 0.0
        self.last_x_mm = None
        self.last_ctrl_ms = now_ms
        self.stable_ms = 0

    def start(self, seq, pos_mm, neg_mm, now_ms):
        # TI 可能因 ACK 丢失而重发同一 seq；只重发 ACK，不把正在进行的动作归零。
        if (seq == self.seq and
                self.state in (H3_POS, H3_NEG, H3_HOLD, H3_DONE)):
            self.last_host_ms = now_ms
            self.ack("START")
            return
        if not self.calibration_ok:
            self.seq = seq
            self.set_fault(FAULT_BAD_CALIBRATION)
            return
        if not (10.0 <= pos_mm <= 70.0 and -70.0 <= neg_mm <= -10.0):
            self.seq = seq
            self.set_fault(FAULT_BAD_TARGET)
            return
        self.seq = seq
        self.target_pos_mm = pos_mm
        self.target_neg_mm = neg_mm
        self.target_mm = pos_mm
        self.state = H3_POS
        self.start_ms = now_ms
        self.stage_ms = now_ms
        self.done_sent = False
        self.done_elapsed_ms = 0
        self.fault_code = 0
        self.vision_lost_since_ms = None
        self.done_error_since_ms = None
        self.last_host_ms = now_ms
        self.reset_control(now_ms)
        self.servo.neutral(True)
        self.ack("START")

    def abort(self, seq):
        self.seq = seq
        self.state = H3_IDLE
        self.target_mm = 0.0
        self.start_ms = 0
        self.done_elapsed_ms = 0
        self.fault_code = 0
        self.vision_lost_since_ms = None
        self.done_error_since_ms = None
        self.last_host_ms = 0
        self.servo.neutral(True)
        self.ack("ABORT")

    def neutral(self, seq):
        self.seq = seq
        self.state = H3_IDLE
        self.target_mm = 0.0
        self.start_ms = 0
        self.done_elapsed_ms = 0
        self.fault_code = 0
        self.vision_lost_since_ms = None
        self.done_error_since_ms = None
        self.last_host_ms = 0
        self.servo.neutral(True)
        self.ack("NEUTRAL")

    def keepalive(self, seq, now_ms):
        if (seq == self.seq and
                self.state in (H3_POS, H3_NEG, H3_HOLD, H3_DONE)):
            self.last_host_ms = now_ms

    def set_fault(self, code):
        if self.state != H3_FAULT or self.fault_code != code:
            self.state = H3_FAULT
            self.fault_code = int(code)
            self.servo.neutral(False)
            self.send("$FAULT,%d,%d\n" % (self.seq, self.fault_code))

    def _within_target(self):
        return (abs(self.error_mm) <= cfg.TARGET_TOLERANCE_MM and
                abs(self.speed_mm_s) <= cfg.TARGET_MAX_SPEED_MM_S)

    def _update_stability(self, now_ms, required_ms):
        if self._within_target():
            if self.stable_ms == 0:
                self.stable_ms = now_ms
            return ticks_since(now_ms, self.stable_ms) >= required_ms
        self.stable_ms = 0
        return False

    def _pd_control(self, x_mm, now_ms):
        dt_ms = ticks_since(now_ms, self.last_ctrl_ms)
        if dt_ms <= 0:
            dt_ms = 1
        if dt_ms > 100:
            dt_ms = 20
        dt_s = float(dt_ms) / 1000.0

        raw_speed = 0.0
        if self.last_x_mm is not None:
            raw_speed = (x_mm - self.last_x_mm) / dt_s
            raw_speed = clamp(raw_speed,
                              -cfg.BALL_SPEED_LIMIT_MM_S,
                              cfg.BALL_SPEED_LIMIT_MM_S)
        a = cfg.BALL_SPEED_FILTER_ALPHA
        self.speed_mm_s += a * (raw_speed - self.speed_mm_s)

        self.error_mm = self.target_mm - x_mm
        self.integral += self.error_mm * dt_s
        self.integral = clamp(self.integral,
                              -cfg.BALL_INTEGRAL_LIMIT,
                              cfg.BALL_INTEGRAL_LIMIT)

        command = (cfg.BALL_KP_US_PER_MM * self.error_mm +
                   cfg.BALL_KI_US_PER_MM_S * self.integral -
                   cfg.BALL_KD_US_PER_MM_S * self.speed_mm_s)
        pulse = (cfg.SERVO_NEUTRAL_US +
                 cfg.SERVO_DIRECTION * command)
        self.servo.set_us(pulse)

        self.last_x_mm = x_mm
        self.last_ctrl_ms = now_ms

    def update(self, tracker, now_ms):
        if self.state in (H3_IDLE, H3_FAULT):
            self.servo.neutral(False)
            return

        if (self.last_host_ms == 0 or
                ticks_since(now_ms, self.last_host_ms) >
                cfg.HOST_KEEPALIVE_TIMEOUT_MS):
            self.set_fault(FAULT_HOST_TIMEOUT)
            return

        fresh = tracker.is_fresh(now_ms)
        detected = getattr(tracker, "detected_this_frame", fresh)
        if detected:
            self.vision_lost_since_ms = None
        else:
            self.done_error_since_ms = None
            if self.state in (H3_POS, H3_NEG, H3_HOLD, H3_DONE):
                if self.vision_lost_since_ms is None:
                    self.vision_lost_since_ms = now_ms
                elif (ticks_since(now_ms, self.vision_lost_since_ms) >
                      cfg.VISION_FAULT_MS):
                    self.set_fault(FAULT_VISION_LOST)
                    return

        if not detected:
            # 禁止用旧像素继续积分/微分；短时漏检立即缓回中，重捕首帧无D冲击。
            self.last_x_mm = None
            self.speed_mm_s = 0.0
            self.last_ctrl_ms = now_ms
            self.servo.neutral(False)
            return

        self._pd_control(tracker.x_mm, now_ms)
        elapsed = self.elapsed_ms(now_ms)

        if self.state in (H3_POS, H3_NEG, H3_HOLD) and \
                elapsed > cfg.TOTAL_DEADLINE_MS:
            self.set_fault(FAULT_TOTAL_TIMEOUT)
            return

        if self.state == H3_POS:
            if self._update_stability(now_ms, cfg.POS_STABLE_MS):
                self.state = H3_NEG
                self.target_mm = self.target_neg_mm
                self.stage_ms = now_ms
                self.reset_control(now_ms)
            elif ticks_since(now_ms, self.stage_ms) > cfg.POS_TIMEOUT_MS:
                self.set_fault(FAULT_POS_TIMEOUT)

        elif self.state == H3_NEG:
            if self._update_stability(now_ms, cfg.NEG_STABLE_MS):
                self.state = H3_HOLD
                self.stage_ms = now_ms
                self.stable_ms = now_ms

        elif self.state == H3_HOLD:
            if not self._within_target():
                self.stable_ms = now_ms
            elif ticks_since(now_ms, self.stable_ms) >= cfg.FINAL_HOLD_MS:
                self.done_elapsed_ms = max(
                    0, ticks_since(now_ms, self.start_ms))
                self.state = H3_DONE

        if self.state == H3_DONE and not self.done_sent:
            self.done_sent = True
            self.send("$DONE,%d,%d,%.2f\n" %
                      (self.seq, self.elapsed_ms(now_ms), self.error_mm))
            # 不回中：仍继续执行 _pd_control()，把钢球保持在 -5 cm 附近。

        if self.state == H3_DONE:
            if abs(self.error_mm) > cfg.DONE_ERROR_LIMIT_MM:
                if self.done_error_since_ms is None:
                    self.done_error_since_ms = now_ms
                elif (ticks_since(now_ms, self.done_error_since_ms) >=
                      cfg.DONE_ERROR_FAULT_MS):
                    self.set_fault(FAULT_HOLD_LOST)
            else:
                self.done_error_since_ms = None


class CommandReceiver:
    def __init__(self, uart, controller):
        self.uart = uart
        self.controller = controller
        self.buf = ""

    def _bad(self, seq=None):
        if seq is not None:
            self.controller.seq = seq
        self.controller.set_fault(FAULT_BAD_COMMAND)

    def parse_line(self, line, now_ms):
        fields = line.strip().split(",")
        if len(fields) < 4 or fields[0] != "$CMD" or fields[1] != "Q3":
            return
        action = fields[2]
        try:
            seq = int(fields[3])
        except Exception:
            self._bad()
            return

        if action == "START" and len(fields) == 6:
            try:
                pos_mm = float(fields[4])
                neg_mm = float(fields[5])
            except Exception:
                self._bad(seq)
                return
            self.controller.start(seq, pos_mm, neg_mm, now_ms)
        elif action == "ABORT" and len(fields) == 4:
            self.controller.abort(seq)
        elif action == "NEUTRAL" and len(fields) == 4:
            self.controller.neutral(seq)
        elif action == "KEEP" and len(fields) == 4:
            self.controller.keepalive(seq, now_ms)
        else:
            self._bad(seq)

    def poll(self, now_ms):
        data = self.uart.read()
        if not data:
            return
        try:
            self.buf += data.decode("ascii")
        except Exception:
            return
        if len(self.buf) > 256:
            self.buf = self.buf[-128:]
        while "\n" in self.buf:
            line, self.buf = self.buf.split("\n", 1)
            self.parse_line(line.rstrip("\r"), now_ms)


def make_uart(fpioa):
    fpioa.set_function(cfg.UART_TX_IO, FPIOA.UART2_TXD)
    fpioa.set_function(cfg.UART_RX_IO, FPIOA.UART2_RXD)
    return UART(UART.UART2,
                baudrate=cfg.UART_BAUD,
                bits=UART.EIGHTBITS,
                parity=UART.PARITY_NONE,
                stop=UART.STOPBITS_ONE)


def init_display():
    try:
        if cfg.DISPLAY_DRIVER == "VIRT":
            Display.init(Display.VIRT,
                         width=cfg.DISPLAY_WIDTH,
                         height=cfg.DISPLAY_HEIGHT,
                         fps=30)
        else:
            driver = getattr(Display, cfg.DISPLAY_DRIVER)
            Display.init(driver,
                         width=cfg.DISPLAY_WIDTH,
                         height=cfg.DISPLAY_HEIGHT,
                         to_ide=cfg.DISPLAY_TO_IDE)
    except Exception as exc:
        print("Physical display init failed, use VIRT:", exc)
        try:
            Display.deinit()
        except Exception:
            pass
        Display.init(Display.VIRT,
                     width=cfg.FRAME_WIDTH,
                     height=cfg.FRAME_HEIGHT,
                     fps=30)


def draw_overlay(img, tracker, controller, valid):
    roi = cfg.BALL_ROI
    img.draw_rectangle(roi, color=190, thickness=2)

    # 三点标定线同时也是现场摆球标尺。
    for px, label in ((int(cfg.CAL_PX_NEG50), "-50"),
                      (int(cfg.CAL_PX_ZERO), "O"),
                      (int(cfg.CAL_PX_POS50), "+50")):
        img.draw_line(px, roi[1], px, roi[1] + roi[3],
                      color=115 if label == "O" else 80,
                      thickness=1)
        img.draw_string(px - 12, roi[1] - 18, label, color=230, scale=1)

    if tracker.rect is not None:
        img.draw_rectangle(tracker.rect, color=255, thickness=2)
        img.draw_cross(int(tracker.filtered_px), tracker.y_px,
                       color=255, size=10, thickness=2)

    state_names = ("IDLE", "POS", "NEG", "HOLD", "DONE", "FAULT")
    state_name = state_names[controller.state]
    img.draw_string(8, 8, "Q1 LIVE / Q3 %s" % state_name,
                    color=255, scale=2)
    img.draw_string(8, 34,
                    "PX:%s X:%+.1f T:%+.1f E:%+.1f" %
                    ("%.1f" % tracker.filtered_px
                     if tracker.filtered_px is not None else "---",
                     tracker.x_mm,
                     controller.target_mm,
                     controller.error_mm),
                    color=255, scale=1)
    img.draw_string(8, 52,
                    "SERVO:%dus V:%d SEQ:%d" %
                    (controller.servo.current_us, 1 if valid else 0,
                     controller.seq),
                    color=255, scale=1)


def main():
    fpioa = FPIOA()
    uart = None
    servo = None
    sensor = None
    controller = None
    display_ready = False
    media_ready = False

    try:
        uart = make_uart(fpioa)
        servo = SafeServo(fpioa)
        calibration = ThreePointCalibration()
        tracker = BallTracker(calibration)
        controller = H3Controller(servo, uart)
        commands = CommandReceiver(uart, controller)

        controller.calibration_ok = (
            bool(getattr(cfg, "CALIBRATION_CONFIRMED", False)) and
            calibration.valid())
        if not controller.calibration_ok:
            controller.set_fault(FAULT_BAD_CALIBRATION)

        sensor = Sensor()
        sensor.reset()
        sensor.set_framesize(width=cfg.FRAME_WIDTH,
                             height=cfg.FRAME_HEIGHT)
        sensor.set_pixformat(Sensor.GRAYSCALE)

        init_display()
        display_ready = True
        MediaManager.init()
        media_ready = True
        sensor.run()

        last_tx_ms = 0
        frame_count = 0
        print("H1/H3 ready: IO44 TX, IO45 RX, IO42 PWM0")

        while True:
            os.exitpoint()
            now_ms = time.ticks_ms()
            commands.poll(now_ms)

            img = sensor.snapshot()
            tracker.update(img, now_ms)
            valid = tracker.is_fresh(now_ms)
            controller.update(tracker, now_ms)

            if ticks_since(now_ms, last_tx_ms) >= cfg.TELEMETRY_PERIOD_MS:
                last_tx_ms = now_ms
                elapsed = controller.elapsed_ms(now_ms)
                controller.send(
                    "$H3,%d,%d,%.2f,%.2f,%.2f,%d,%d,%d\n" %
                    (controller.seq, controller.state, tracker.x_mm,
                     controller.target_mm, controller.error_mm,
                     servo.current_us, 1 if valid else 0, elapsed))

            draw_overlay(img, tracker, controller, valid)
            x_off = max(0, (cfg.DISPLAY_WIDTH - cfg.FRAME_WIDTH) // 2)
            y_off = max(0, (cfg.DISPLAY_HEIGHT - cfg.FRAME_HEIGHT) // 2)
            Display.show_image(img, x=x_off, y=y_off)

            frame_count += 1
            if frame_count % 120 == 0:
                gc.collect()

    except KeyboardInterrupt:
        print("User stopped")
    except BaseException as exc:
        print("Fatal:", exc)
        if servo is not None:
            servo.neutral(True)
        if uart is not None:
            try:
                fault_seq = controller.seq if controller is not None else 0
                uart.write("$FAULT,%d,%d\n" %
                           (fault_seq, FAULT_RUNTIME))
            except Exception:
                pass
        raise
    finally:
        if servo is not None:
            servo.deinit()
        if uart is not None:
            uart.deinit()
        if sensor is not None:
            sensor.stop()
        if display_ready:
            Display.deinit()
        os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
        time.sleep_ms(100)
        if media_ready:
            MediaManager.deinit()


main()
