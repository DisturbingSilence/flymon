#include <drivers/mpu60x0.h>
#include <drivers/err.h>

int mpu60x0_init(mpu60x0_t* mpu)
{
    if(!mpu) return ERR_INV_ARG;

    uint8_t regs[2] = {MPU60X0_REG_WHO_AM_I};
    uint8_t whoami;

    RET_ERR(i2c_write_read(&mpu->i2c_bus,regs,1,&whoami,1));
    if(whoami != 0x68 && whoami != 0x69) return ERR_INV_DEVICE;
    regs[0] = MPU60X0_REG_PWR_MGMGT_1;
    regs[1] = 0x00;
    RET_ERR(i2c_write(&mpu->i2c_bus,regs,2));
    return ERR_OK;
}
int mpu60x0_set_sample_rate(mpu60x0_t* mpu,uint16_t sample_rate)
{
    if(!mpu) return ERR_INV_ARG;
    if (sample_rate == 0 || sample_rate > 8000) return ERR_INV_ARG;
    uint8_t dlpf;
    uint16_t base_freq;
    if(sample_rate > 1000)
    {
        dlpf = 0;
        base_freq = 8000;
    }
    else
    {
        dlpf = 3;
        base_freq = 1000;
    }
    uint16_t div = (base_freq + sample_rate / 2) / sample_rate;
    if (div == 0) div = 1;
    uint8_t smplrt_div_cmds[] = {MPU60X0_REG_SMPLRT_DIV,div - 1};
    uint8_t config_cmds[] = {MPU60X0_REG_CONFIG,dlpf & 0b111};
    RET_ERR(i2c_write(&mpu->i2c_bus,config_cmds,sizeof(config_cmds)));
    RET_ERR(i2c_write(&mpu->i2c_bus,smplrt_div_cmds,sizeof(smplrt_div_cmds)));
    mpu->sample_rate = sample_rate;
    return ERR_OK;
}
int mpu60x0_configure_sensors(mpu60x0_t* mpu,gyroscope_range_t gr,accelerometer_range_t ar)
{
    if(!mpu) return ERR_INV_ARG;
    uint8_t gyro_cmds[2] = {MPU60X0_REG_GYRO_CONFIG,(gr & 0b11) << 3};
    uint8_t accel_cmds[2] = {MPU60X0_REG_ACCEL_CONFIG,(ar & 0b11) << 3};
    RET_ERR(i2c_write(&mpu->i2c_bus,gyro_cmds,sizeof(gyro_cmds)));
    RET_ERR(i2c_write(&mpu->i2c_bus,accel_cmds,sizeof(accel_cmds)));
    mpu->gyro_range = gr;
    mpu->accel_range = ar;
    return ERR_OK;
}
int mpu60x0_read_accelerometer(mpu60x0_t* mpu,int16_t* x,int16_t* y,int16_t* z)
{
    if(!mpu) return ERR_INV_ARG;
    if(!(x || y || z)) return ERR_INV_ARG;

    uint8_t cmds[] ={MPU60X0_REG_ACCEL_XOUT_H};
    uint8_t regvalues[6] = {};
    RET_ERR(i2c_write_read(&mpu->i2c_bus,cmds,sizeof(cmds),regvalues,sizeof(regvalues)));
    if(x) *x = (regvalues[0] << 8) | regvalues[1];
    if(y) *y = (regvalues[2] << 8) | regvalues[3];
    if(z) *z = (regvalues[4] << 8) | regvalues[5];
    return ERR_OK;
}
int mpu60x0_read_gyroscope(mpu60x0_t* mpu,int16_t* x,int16_t* y,int16_t* z)
{
    if(!mpu) return ERR_INV_ARG;
    if(!(x || y || z)) return ERR_INV_ARG;

    uint8_t cmds[] ={MPU60X0_REG_GYRO_XOUT_H};
    uint8_t regvalues[6] = {};
    RET_ERR(i2c_write_read(&mpu->i2c_bus,cmds,sizeof(cmds),regvalues,sizeof(regvalues)));
    if(x) *x = (regvalues[0] << 8) | regvalues[1];
    if(y) *y = (regvalues[2] << 8) | regvalues[3];
    if(z) *z = (regvalues[4] << 8) | regvalues[5];
    return ERR_OK;
}
