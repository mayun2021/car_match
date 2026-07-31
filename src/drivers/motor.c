/**
 * @file motor.c
 * @brief TB6612FNG 双电机驱动实现。
 *
 * 该模块把左右轮速度命令转换成 TB6612 的方向脚和 PWM 输出，并加入斜率限制，
 * 减少起步、停车和急转弯时的机械冲击。
 */

#include "motor.h"
#include "hal.h"
#include "robot_config.h"

static int16_t s_left_pwm;
static int16_t s_right_pwm;

/**
 * @brief 限制电机 PWM 命令范围。
 *
 * @param value 输入速度命令。
 * @return 限制在 -ROBOT_MOTOR_PWM_MAX 到 +ROBOT_MOTOR_PWM_MAX 的速度命令。
 */
static int16_t clamp_pwm(int16_t value)
{
    if (value > ROBOT_MOTOR_PWM_MAX)
    {
        return ROBOT_MOTOR_PWM_MAX;
    }
    if (value < -ROBOT_MOTOR_PWM_MAX)
    {
        return -ROBOT_MOTOR_PWM_MAX;
    }
    return value;
}

/**
 * @brief 对电机输出做斜率限制。
 *
 * @param current 当前输出。
 * @param target 目标输出。
 * @return 本周期允许到达的新输出。
 */
static int16_t ramp_to(int16_t current, int16_t target)
{
    if (target > current + ROBOT_MOTOR_RAMP_STEP)
    {
        return current + ROBOT_MOTOR_RAMP_STEP;
    }
    if (target < current - ROBOT_MOTOR_RAMP_STEP)
    {
        return current - ROBOT_MOTOR_RAMP_STEP;
    }
    return target;
}

/**
 * @brief 把单个电机速度命令转换成方向脚和 PWM。
 *
 * @param pwm_channel PWM 通道。
 * @param in1 TB6612 方向输入 1。
 * @param in2 TB6612 方向输入 2。
 * @param speed 速度命令，正负表示方向。
 * @param reverse 是否反转该侧电机方向。
 */
static void apply_one_motor(hal_pwm_t pwm_channel,
                            hal_pin_t in1,
                            hal_pin_t in2,
                            int16_t speed,
                            int reverse)
{
    bool forward = speed >= 0;
    uint16_t duty;

    if (reverse)
    {
        forward = !forward;
    }

    duty = (uint16_t)(speed >= 0 ? speed : -speed);

    if (duty == 0)
    {
        /* 两个方向脚都拉低，TB6612 为短暂滑行停止，适合巡线小车。 */
        hal_gpio_write(in1, false);
        hal_gpio_write(in2, false);
    }
    else if (forward)
    {
        hal_gpio_write(in1, true);
        hal_gpio_write(in2, false);
    }
    else
    {
        hal_gpio_write(in1, false);
        hal_gpio_write(in2, true);
    }

    hal_pwm_set_duty_permille(pwm_channel, duty);
}

/**
 * @brief 初始化电机模块并停止输出。
 */
void motor_init(void)
{
    s_left_pwm = 0;
    s_right_pwm = 0;
    motor_stop();
}

/**
 * @brief 设置左右轮速度命令。
 *
 * @param left 左轮速度，范围 -1000 到 +1000。
 * @param right 右轮速度，范围 -1000 到 +1000。
 */
void motor_set_raw(int16_t left, int16_t right)
{
    left = clamp_pwm(left);
    right = clamp_pwm(right);

    s_left_pwm = ramp_to(s_left_pwm, left);
    s_right_pwm = ramp_to(s_right_pwm, right);

    hal_gpio_write(HAL_PIN_MOTOR_STBY, true);
    apply_one_motor(HAL_PWM_MOTOR_LEFT,
                    HAL_PIN_MOTOR_AIN1,
                    HAL_PIN_MOTOR_AIN2,
                    s_left_pwm,
                    ROBOT_LEFT_MOTOR_REVERSE);
    apply_one_motor(HAL_PWM_MOTOR_RIGHT,
                    HAL_PIN_MOTOR_BIN1,
                    HAL_PIN_MOTOR_BIN2,
                    s_right_pwm,
                    ROBOT_RIGHT_MOTOR_REVERSE);
}

/**
 * @brief 停止左右电机。
 */
void motor_stop(void)
{
    s_left_pwm = 0;
    s_right_pwm = 0;
    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_LEFT, 0);
    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_RIGHT, 0);
    hal_gpio_write(HAL_PIN_MOTOR_AIN1, false);
    hal_gpio_write(HAL_PIN_MOTOR_AIN2, false);
    hal_gpio_write(HAL_PIN_MOTOR_BIN1, false);
    hal_gpio_write(HAL_PIN_MOTOR_BIN2, false);
    /* Disable the H-bridge whenever the robot is stopped or waiting. */
    hal_gpio_write(HAL_PIN_MOTOR_STBY, false);
}

/**
 * @brief 对两只电机执行 TB6612 短刹车。
 *
 * 先撤掉 PWM，再建立 IN1=IN2 的短刹车组合，最后恢复 100% PWM，
 * 避免方向脚切换瞬间产生不确定的窄脉冲。
 */
void motor_brake(void)
{
    s_left_pwm = 0;
    s_right_pwm = 0;

    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_LEFT, 0u);
    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_RIGHT, 0u);

    hal_gpio_write(HAL_PIN_MOTOR_AIN1, true);
    hal_gpio_write(HAL_PIN_MOTOR_AIN2, true);
    hal_gpio_write(HAL_PIN_MOTOR_BIN1, true);
    hal_gpio_write(HAL_PIN_MOTOR_BIN2, true);
    hal_gpio_write(HAL_PIN_MOTOR_STBY, true);

    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_LEFT, 1000u);
    hal_pwm_set_duty_permille(HAL_PWM_MOTOR_RIGHT, 1000u);
}

/**
 * @brief 获取当前左轮速度命令。
 *
 * @return 左轮当前 PWM 命令。
 */
int16_t motor_get_left(void)
{
    return s_left_pwm;
}

/**
 * @brief 获取当前右轮速度命令。
 *
 * @return 右轮当前 PWM 命令。
 */
int16_t motor_get_right(void)
{
    return s_right_pwm;
}
