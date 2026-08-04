#pragma once

#include <drivers/i2c.h>
#define MPU60X0_I2C_ADDR 0xD0

#define MPU60X0_REG_PWR_MGMGT_1 0x6B
#define MPU60X0_REG_CONFIG 0x1A
#define MPU60X0_REG_SMPLRT_DIV 0x19
#define MPU60X0_REG_WHO_AM_I 0x75
#define MPU60X0_REG_ACCEL_CONFIG 0x1C
#define MPU60X0_REG_GYRO_CONFIG 0x1B

#define MPU60X0_REG_ACCEL_XOUT_H 0x3B
#define MPU60X0_REG_ACCEL_XOUT_L 0x3C
#define MPU60X0_REG_ACCEL_YOUT_H 0x3D
#define MPU60X0_REG_ACCEL_YOUT_L 0x3E
#define MPU60X0_REG_ACCEL_ZOUT_H 0x3F
#define MPU60X0_REG_ACCEL_ZOUT_L 0x40

#define MPU60X0_REG_GYRO_XOUT_H 0x43
#define MPU60X0_REG_GYRO_XOUT_L 0x44
#define MPU60X0_REG_GYRO_YOUT_H 0x45
#define MPU60X0_REG_GYRO_YOUT_L 0x46
#define MPU60X0_REG_GYRO_ZOUT_H 0x47
#define MPU60X0_REG_GYRO_ZOUT_L 0x48

typedef enum
{
    FS_SEL_250 = 0,
    FS_SEL_500 = 1,
    FS_SEL_1000 = 2,
    FS_SEL_2000 = 3,
} gyroscope_range_t;
typedef enum
{
    AFS_SEL_2 = 0,
    AFS_SEL_4 = 1,
    AFS_SEL_8 = 2,
    AFS_SEL_16 = 3,
} accelerometer_range_t;
typedef struct
{
    i2c_device_t i2c_bus;
    gyroscope_range_t gyro_range;
    accelerometer_range_t accel_range;
    uint16_t sample_rate;
} mpu60x0_t;

int mpu60x0_init(mpu60x0_t* mpu);
int mpu60x0_set_sample_rate(mpu60x0_t* mpu,uint16_t sample_rate);
int mpu60x0_configure_sensors(mpu60x0_t* mpu,gyroscope_range_t gr,accelerometer_range_t ar);
int mpu60x0_read_gyroscope(mpu60x0_t* mpu,int16_t* x,int16_t* y,int16_t* z);
int mpu60x0_read_accelerometer(mpu60x0_t* mpu,int16_t* x,int16_t* y,int16_t* z);
