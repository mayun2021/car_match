/**
 * @file mpu6050.c
 * @brief MPU6050 姿态传感器驱动实现。
 *
 * 本实现完成基本寄存器初始化、Z 轴陀螺零偏校准和偏航角积分。
 * 对 H 题而言，这足够用于记录小车启动初始角度和运动过程的相对角度。
 */

#include "mpu6050.h"
#include "hal.h"
#include "robot_config.h"

#include <string.h>

#define MPU6050_REG_PWR_MGMT_1  0x6Bu
#define MPU6050_REG_SMPLRT_DIV  0x19u
#define MPU6050_REG_CONFIG      0x1Au
#define MPU6050_REG_GYRO_CONFIG 0x1Bu
#define MPU6050_REG_ACCEL_XOUT  0x3Bu
#define MPU6050_GYRO_LSB_250DPS 131.0f

static mpu6050_state_t s_mpu;

/**
 * @brief 将两个大端字节转换为有符号 16 位整数。
 *
 * @param data 指向高字节在前的两个字节。
 * @return 转换后的 int16_t 数据。
 */
static int16_t be16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

/**
 * @brief 写 MPU6050 单个寄存器。
 *
 * @param reg 寄存器地址。
 * @param value 待写入的值。
 * @return true 表示 I2C 写入成功。
 */
static bool write_reg(uint8_t reg, uint8_t value)
{
    const uint8_t data[2] = {reg, value};
    return hal_i2c_write(ROBOT_MPU6050_ADDR, data, sizeof(data));
}

/**
 * @brief 从 MPU6050 连续读取多个寄存器。
 *
 * @param reg 起始寄存器地址。
 * @param data 读取数据缓冲区。
 * @param len 读取字节数。
 * @return true 表示 I2C 读写成功。
 */
static bool read_regs(uint8_t reg, uint8_t *data, uint8_t len)
{
    return hal_i2c_write_read(ROBOT_MPU6050_ADDR, &reg, 1u, data, len);
}

/**
 * @brief 初始化 MPU6050。
 *
 * @return true 表示初始化成功，false 表示 I2C 通信失败或设备不在线。
 */
bool mpu6050_init(void)
{
    memset(&s_mpu, 0, sizeof(s_mpu));

    /*
     * 唤醒 MPU6050，并使用较温和的低通滤波。
     * 陀螺仪量程设为 +/-250 dps，便于获得较高角速度分辨率。
     */
    if (!write_reg(MPU6050_REG_PWR_MGMT_1, 0x00u))
    {
        s_mpu.online = false;
        return false;
    }
    hal_delay_ms(50u);

    s_mpu.online =
        write_reg(MPU6050_REG_SMPLRT_DIV, 0x07u) &&
        write_reg(MPU6050_REG_CONFIG, 0x03u) &&
        write_reg(MPU6050_REG_GYRO_CONFIG, 0x00u);

    if (s_mpu.online)
    {
        (void)mpu6050_calibrate_gyro();
        mpu6050_zero_yaw();
    }

    return s_mpu.online;
}

/**
 * @brief 静止采样陀螺仪 Z 轴并计算零偏。
 *
 * @return true 表示校准成功。
 */
bool mpu6050_calibrate_gyro(void)
{
    int32_t sum_gz = 0;
    uint8_t data[14];

    if (!s_mpu.online)
    {
        return false;
    }

    for (uint16_t i = 0; i < ROBOT_MPU_CALIB_SAMPLES; ++i)
    {
        if (!read_regs(MPU6050_REG_ACCEL_XOUT, data, sizeof(data)))
        {
            s_mpu.online = false;
            return false;
        }
        sum_gz += be16(&data[12]);
        hal_delay_ms(2u);
    }

    s_mpu.gyro_z_bias_dps = ((float)sum_gz / (float)ROBOT_MPU_CALIB_SAMPLES) / MPU6050_GYRO_LSB_250DPS;
    return true;
}

/**
 * @brief 更新 MPU6050 原始数据和偏航角积分。
 *
 * @param dt_ms 本次积分周期，单位 ms。
 * @return true 表示读取成功。
 */
bool mpu6050_update(uint32_t dt_ms)
{
    uint8_t data[14];
    float gz_dps;

    if (!s_mpu.online)
    {
        return false;
    }

    if (!read_regs(MPU6050_REG_ACCEL_XOUT, data, sizeof(data)))
    {
        s_mpu.online = false;
        return false;
    }

    s_mpu.ax = be16(&data[0]);
    s_mpu.ay = be16(&data[2]);
    s_mpu.az = be16(&data[4]);
    s_mpu.gx = be16(&data[8]);
    s_mpu.gy = be16(&data[10]);
    s_mpu.gz = be16(&data[12]);

    gz_dps = ((float)s_mpu.gz / MPU6050_GYRO_LSB_250DPS) - s_mpu.gyro_z_bias_dps;
    s_mpu.yaw_deg += gz_dps * ((float)dt_ms / 1000.0f);

    return true;
}

/**
 * @brief 获取 MPU6050 当前状态。
 *
 * @return 传感器在线状态、原始数据、零偏和 yaw 角。
 */
mpu6050_state_t mpu6050_get_state(void)
{
    return s_mpu;
}

/**
 * @brief 清零当前相对偏航角。
 */
void mpu6050_zero_yaw(void)
{
    s_mpu.yaw_deg = 0.0f;
}
