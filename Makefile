# H 题小车软件工程的桌面仿真构建脚本。
# 真实 MSPM0 上板时通常由 CCS/Theia 工程编译，本 Makefile 只用于在电脑上
# 快速检查算法层、驱动接口和状态机是否存在语法或链接错误。

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude -DROBOT_DESKTOP_SIM
BUILD_DIR := build

SIM_SRCS := \
	tools/simulate_control.c \
	src/app/robot_app.c \
	src/app/pid.c \
	src/drivers/display.c \
	src/drivers/k230_protocol.c \
	src/drivers/line_sensor.c \
	src/drivers/motor.c \
	src/drivers/mpu6050.c \
	src/drivers/oled.c \
	src/drivers/servo.c \
	src/platform/hal_stub.c

.PHONY: sim clean

sim: $(BUILD_DIR)/simulate_control

$(BUILD_DIR)/simulate_control: $(SIM_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SIM_SRCS) -o $@

clean:
	rm -rf $(BUILD_DIR)
