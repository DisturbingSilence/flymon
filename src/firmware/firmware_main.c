#include <drivers/devices/ssd1306.h>
#include <drivers/devices/w25qxx.h>
#include <drivers/devices/mpu60x0.h>
#include <drivers/devices/ble.h>

#include <drivers/peripherals/usart.h>
#include <drivers/peripherals/i2c.h>

#include <drivers/err.h>

#include <services/scheduler.h>
#include <services/buttons.h>
#include <services/font.h>
#include <services/ui.h>

#include <bsp/STM32F411CEU6.h>

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <mavlink/common/mavlink.h>

#include "stm32f4xx_ll_gpio.h"


#define USART1_RX_BUFFER_SIZE 1024
#define USART1_TX_BUFFER_SIZE 1024

#define DEG_TO_RAD             0.01745329251994329577f
#define RAD_TO_DEG             57.29577951308232f

#define ACCEL_LSB_PER_G        16384.0f
#define GYRO_LSB_PER_DPS       131.0f

#define ATTITUDE_DT            0.01f
#define COMPLEMENTARY_ALPHA    0.98f

#define CALIB_MAGIC             0xDEADBEEFUL
#define CALIB_FLASH_SECTOR      0
#define CALIB_FLASH_PAGE        0
#define CALIB_FLASH_OFFSET      0

#define CALIB_WARMUP_SAMPLES    100
#define CALIB_SAMPLES           2000

typedef struct
{
    uint32_t magic;

    int32_t gyro_x;
    int32_t gyro_y;
    int32_t gyro_z;

    int32_t accel_x;
    int32_t accel_y;
    int32_t accel_z;

    uint32_t reserved;
} imu_calibration_t;

typedef enum
{
    CALIB_STATE_IDLE = 0,
    CALIB_STATE_WARMUP,
    CALIB_STATE_SAMPLING,
    CALIB_STATE_SAVING
} calibration_state_t;
typedef struct
{
    i2c_bus_t i2c1;
    spi_bus_t spi2;
    usart_bus_t usart1;

    dma_channel_t dma1stream6;
    dma_channel_t dma2stream2;
    dma_channel_t dma2stream7;

    w25qxx_flash_t flash;
    ssd1306_device_t oled;
    ble_device_t ble;
    mpu60x0_t mpu;

    ui_context_t ui_ctx;

    imu_calibration_t calibration;

    uint8_t usart1_rxrb[USART1_RX_BUFFER_SIZE];
    uint8_t usart1_txrb[USART1_TX_BUFFER_SIZE];
    struct
    {
        float roll;
        float pitch;
        float yaw;

        float roll_rate;
        float pitch_rate;
        float yaw_rate;

        uint32_t last_update_ms;
    } attitude;
    struct
    {
        calibration_state_t state;

        uint32_t sample;

        int64_t gx_sum;
        int64_t gy_sum;
        int64_t gz_sum;

        int64_t ax_sum;
        int64_t ay_sum;
        int64_t az_sum;
    } calib;

} global_context_t;
static global_context_t g_ctx = {};

static void imu_calibration_reset_accumulators(global_context_t* ctx)
{
    ctx->calib.sample = 0;
    ctx->calib.gx_sum = 0;
    ctx->calib.gy_sum = 0;
    ctx->calib.gz_sum = 0;
    ctx->calib.ax_sum = 0;
    ctx->calib.ay_sum = 0;
    ctx->calib.az_sum = 0;
}
static void imu_calibration_start(global_context_t* ctx)
{
    if (ctx->calib.state != CALIB_STATE_IDLE) return;

    ctx->attitude.roll = 0.0f;
    ctx->attitude.pitch = 0.0f;
    ctx->attitude.yaw = 0.0f;
    ctx->attitude.roll_rate = 0.0f;
    ctx->attitude.pitch_rate = 0.0f;
    ctx->attitude.yaw_rate = 0.0f;
    imu_calibration_reset_accumulators(ctx);
    ctx->calib.state = CALIB_STATE_WARMUP;
}
static int imu_calibration_save(global_context_t* ctx)
{
    int ret;
    ret = w25qxx_erase_sector(&ctx->flash,CALIB_FLASH_SECTOR);
    if (ret != ERR_OK) return ret;
    return w25qxx_write( &ctx->flash,CALIB_FLASH_PAGE,CALIB_FLASH_OFFSET,(const uint8_t*)&ctx->calibration,sizeof(ctx->calibration));
}
static void imu_calibration_task(void* app_context)
{
    global_context_t* ctx = app_context;
    int16_t gx;
    int16_t gy;
    int16_t gz;

    int16_t ax;
    int16_t ay;
    int16_t az;

    switch (ctx->calib.state)
    {
        case CALIB_STATE_IDLE: return;
        case CALIB_STATE_WARMUP:
        {
            mpu60x0_read_gyroscope(&ctx->mpu,&gx,&gy,&gz);
            mpu60x0_read_accelerometer(&ctx->mpu,&ax,&ay,&az);
            ctx->calib.sample++;
            if (ctx->calib.sample >= CALIB_WARMUP_SAMPLES)
            {
                imu_calibration_reset_accumulators(ctx);
                ctx->calib.state = CALIB_STATE_SAMPLING;
            }
            break;
        }
        case CALIB_STATE_SAMPLING:
        {
            mpu60x0_read_gyroscope(&ctx->mpu,&gx,&gy,&gz);
            mpu60x0_read_accelerometer(&ctx->mpu,&ax,&ay,&az);
            ctx->calib.gx_sum += gx;
            ctx->calib.gy_sum += gy;
            ctx->calib.gz_sum += gz;

            ctx->calib.ax_sum += ax;
            ctx->calib.ay_sum += ay;
            ctx->calib.az_sum += az;

            ctx->calib.sample++;
            if (ctx->calib.sample >= CALIB_SAMPLES)
            {
                ctx->calibration.magic = CALIB_MAGIC;
                ctx->calibration.gyro_x = (int32_t)(ctx->calib.gx_sum / CALIB_SAMPLES);
                ctx->calibration.gyro_y = (int32_t)(ctx->calib.gy_sum / CALIB_SAMPLES);
                ctx->calibration.gyro_z = (int32_t)(ctx->calib.gz_sum / CALIB_SAMPLES);
                ctx->calibration.accel_x = (int32_t)(ctx->calib.ax_sum / CALIB_SAMPLES);
                ctx->calibration.accel_y = (int32_t)(ctx->calib.ay_sum / CALIB_SAMPLES);
                ctx->calibration.accel_z = (int32_t)(ctx->calib.az_sum / CALIB_SAMPLES) - 16384;
                ctx->calib.state = CALIB_STATE_SAVING;

            }
            break;
        }
        case CALIB_STATE_SAVING:
        {
            [[maybe_unused]]int ret = imu_calibration_save(ctx);
            ctx->calib.state = CALIB_STATE_IDLE;
            break;
        }
        default:
            ctx->calib.state = CALIB_STATE_IDLE;
            break;
    }
}
static int imu_calibration_load(global_context_t* ctx)
{
    imu_calibration_t cal;
    memset(&cal, 0, sizeof(cal));
    int ret = w25qxx_read(&ctx->flash,CALIB_FLASH_PAGE,CALIB_FLASH_OFFSET,(uint8_t*)&cal,sizeof(cal));
    if (ret != ERR_OK) return ret;
    if (cal.magic != CALIB_MAGIC) return ERR_INV_MAGIC;
    ctx->calibration = cal;
    return ERR_OK;
}
void oled_task(void* app_context)
{
    global_context_t* ctx = app_context;
    ssd1306_clear(&ctx->oled, 1);
    ui_render(&ctx->ui_ctx);
}
static void buttons_callback(void* app_context, button_t btn)
{
    global_context_t* ctx = app_context;
    ui_handle_button(&ctx->ui_ctx, btn);
}
static float wrap_angle(float angle)
{
    while (angle > (float)M_PI) angle -= 2.0f * (float)M_PI;
    while (angle < -(float)M_PI) angle += 2.0f * (float)M_PI;
    return angle;
}
void attitude_task(void* app_context)
{
    global_context_t* ctx = app_context;
    if (ctx->calib.state != CALIB_STATE_IDLE) return;
    int16_t gx_raw,gy_raw,gz_raw;
    int16_t ax_raw,ay_raw,az_raw;
    mpu60x0_read_gyroscope(&ctx->mpu,&gx_raw,&gy_raw,&gz_raw);
    mpu60x0_read_accelerometer(&ctx->mpu,&ax_raw,&ay_raw,&az_raw);
    float gx = ((float)(gx_raw - ctx->calibration.gyro_x) / GYRO_LSB_PER_DPS) * DEG_TO_RAD;
    float gy = ((float)(gy_raw - ctx->calibration.gyro_y) / GYRO_LSB_PER_DPS) * DEG_TO_RAD;
    float gz = ((float)(gz_raw - ctx->calibration.gyro_z) / GYRO_LSB_PER_DPS) * DEG_TO_RAD;

    float ax = (float)(ax_raw - ctx->calibration.accel_x) / ACCEL_LSB_PER_G;
    float ay = (float)(ay_raw - ctx->calibration.accel_y) / ACCEL_LSB_PER_G;
    float az = (float)(az_raw - ctx->calibration.accel_z) / ACCEL_LSB_PER_G;
    float accel_roll = atan2f(ay, az);
    float accel_pitch = atan2f(-ax,sqrtf(ay * ay + az * az));
    float gyro_roll = ctx->attitude.roll + gx * ATTITUDE_DT;
    float gyro_pitch = ctx->attitude.pitch + gy * ATTITUDE_DT;
    float gyro_yaw = ctx->attitude.yaw + gz * ATTITUDE_DT;
    ctx->attitude.roll = COMPLEMENTARY_ALPHA * gyro_roll + (1.0f - COMPLEMENTARY_ALPHA) * accel_roll;
    ctx->attitude.pitch = COMPLEMENTARY_ALPHA * gyro_pitch + (1.0f - COMPLEMENTARY_ALPHA) * accel_pitch;
    ctx->attitude.yaw = wrap_angle(gyro_yaw);
    ctx->attitude.roll_rate = gx;
    ctx->attitude.pitch_rate = gy;
    ctx->attitude.yaw_rate = gz;
}

static void init_sys(global_context_t* ctx)
{
    board_init();

    board_init_i2c1_pins();
    board_init_spi2_pins();
    board_init_button_input_pins();
    board_init_usart1_pins();
    LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_13);
    PANIC(systime_init(1000));
    dma_config_t i2c_tx_dma_cfg =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .channel = LL_DMA_CHANNEL_1,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_NORMAL,
        .irq = DMA1_Stream6_IRQn,
        .timeout = 3
    };
    dma_config_t usart1_rx_dma_cfg =
    {
        .dma = DMA2,
        .stream = LL_DMA_STREAM_2,
        .channel = LL_DMA_CHANNEL_4,
        .direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_CIRCULAR,
        .irq = DMA2_Stream2_IRQn,
        .timeout = 100
    };
    dma_config_t usart1_tx_dma_cfg =
    {
        .dma = DMA2,
        .stream = LL_DMA_STREAM_7,
        .channel = LL_DMA_CHANNEL_4,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_NORMAL,
        .irq = DMA2_Stream7_IRQn,
        .timeout = 100
    };
    PANIC(dma_init(&ctx->dma1stream6,&i2c_tx_dma_cfg));
    PANIC(dma_init(&ctx->dma2stream2,&usart1_rx_dma_cfg));
    PANIC(dma_init(&ctx->dma2stream7,&usart1_tx_dma_cfg));
    ctx->i2c1 = (i2c_bus_t)
    {
        .instance = I2C1,
        .timeout = 100,
        .tx_dma = &ctx->dma1stream6
    };
    ctx->spi2 = (spi_bus_t)
    {
        .bus = SPI2,
        .timeout = 50
    };
    PANIC(i2c_init(&ctx->i2c1,100000U));
    PANIC(spi_init(&ctx->spi2));
    usart_config_t usart1_cfg =
    {
        .instance = USART1,
        .baudrate = 9600,
        .transfer_direction = LL_USART_DIRECTION_TX_RX,
        .enable_clock = LL_USART_CLOCK_DISABLE,
        .rx_buffer = ctx->usart1_rxrb,
        .rx_capacity = USART1_RX_BUFFER_SIZE,
        .tx_buffer = ctx->usart1_txrb,
        .tx_capacity = USART1_TX_BUFFER_SIZE,
        .rx_dma = &ctx->dma2stream2,
        .tx_dma = &ctx->dma2stream7
    };
    PANIC(usart_init(&ctx->usart1,&usart1_cfg));
}
static void init_peripherals(global_context_t* ctx)
{
    ctx->oled.i2c_bus.bus = &ctx->i2c1;
    ctx->oled.i2c_bus.address = SSD1306_I2C_ADDR;
    ctx->mpu.i2c_bus.bus = &ctx->i2c1;
    ctx->mpu.i2c_bus.address = MPU60X0_I2C_ADDR;
    PANIC(mpu60x0_init(&ctx->mpu));
    PANIC(mpu60x0_set_sample_rate(&ctx->mpu,1000));
    PANIC(mpu60x0_configure_sensors(&ctx->mpu,FS_SEL_250,AFS_SEL_2));
    PANIC(w25qxx_init(GPIOB,LL_GPIO_PIN_12,&ctx->spi2,&ctx->flash));
    PANIC(ssd1306_init(&ctx->oled));
    memset(&ctx->calibration,0,sizeof(ctx->calibration));
    int calib_ret = imu_calibration_load(ctx);
    if (calib_ret != ERR_OK)
    {
        memset(&ctx->calibration,0,sizeof(ctx->calibration));
    }
    sleep(10);
    ble_config_t ble_cfg =
    {
        .usart_bus = &ctx->usart1,
        .pwrc_port = GPIOB,
        .pwrc_pinmask = LL_GPIO_PIN_5,
        .stat_port = GPIOB,
        .stat_pinmask = LL_GPIO_PIN_1
    };
    PANIC(ble_init(&ctx->ble,&ble_cfg));
    sleep(10);

}
void ui_on_render_callback(void* app_context)
{
    global_context_t* ctx = app_context;
    char buf[100];
    switch (ctx->ui_ctx.state)
    {
        case UI_STATE_MAIN:
        {
            sprintf(buf,"MAIN %c %i %c",
                ctx->ui_ctx.state > 0 ? '<' : ' ',
                ctx->ui_ctx.state,
                ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' '
            );
            ssd1306_draw_text(&ctx->oled,buf,0,0);
            sprintf(buf,"Device: %s\nFirmware: %s",PROJECT_NAME,PROJECT_VERSION);
            ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
            break;
        }
        case UI_STATE_CALIBRATE:
        {
            sprintf(buf,"CALIBRATION %c %i %c",
                ctx->ui_ctx.state > 0 ? '<' : ' ',
                ctx->ui_ctx.state,
                ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' '
            );
            ssd1306_draw_text(&ctx->oled,buf,0,0);
            if (ctx->calib.state == CALIB_STATE_IDLE)
            {
                sprintf(buf,"SELECT: calibrate");
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
            }
            else if (ctx->calib.state == CALIB_STATE_WARMUP)
            {
                sprintf(buf,"Keep still...");
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
                sprintf(buf,"Warmup %lu/%d",(unsigned long)ctx->calib.sample,CALIB_WARMUP_SAMPLES);
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT * 2);
            }
            else if (ctx->calib.state == CALIB_STATE_SAMPLING)
            {
                uint32_t percent =(ctx->calib.sample * 100U) / CALIB_SAMPLES;
                sprintf(buf,"Keep still...");
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
                sprintf(buf,"Cal %lu%%",(unsigned long)percent);
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT * 2);
                sprintf(buf,"%lu/%d",(unsigned long)ctx->calib.sample,CALIB_SAMPLES);
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT * 3);
            }
            else if (ctx->calib.state == CALIB_STATE_SAVING)
            {
                sprintf(buf,"Saving...");
                ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
            }
            break;
        }
        case UI_STATE_LIVE_VIEW:
        {
            sprintf(buf,"LIVE_STATS %c %i %c",
                ctx->ui_ctx.state > 0 ? '<' : ' ',
                ctx->ui_ctx.state,
                ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' ');
            ssd1306_draw_text(&ctx->oled,buf,0,0);
            char sign;
            int intpart;
            int fracpart;
            float tmpval;
            tmpval = ctx->attitude.roll;
            sign = tmpval < 0.0f ? '-' : '+';
            if (tmpval < 0.0f) tmpval = -tmpval;
            intpart = (int)tmpval;
            fracpart =(int)((tmpval - (float)intpart) * 1000.0f);
            sprintf(buf,"Roll:%c%d.%03d",sign,intpart,fracpart);
            ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
            tmpval = ctx->attitude.pitch;
            sign = tmpval < 0.0f ? '-' : '+';
            if (tmpval < 0.0f) tmpval = -tmpval;
            intpart = (int)tmpval;
            fracpart = (int)((tmpval - (float)intpart) * 1000.0f);
            sprintf(buf,"Pitch:%c%d.%03d",sign,intpart,fracpart);
            ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT * 2);
            tmpval = ctx->attitude.yaw;
            sign = tmpval < 0.0f ? '-' : '+';
            if (tmpval < 0.0f)tmpval = -tmpval;
            intpart = (int)tmpval;
            fracpart = (int)((tmpval - (float)intpart) * 1000.0f);
            sprintf(buf,"Yaw:%c%d.%03d",sign,intpart,fracpart);
            ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT * 3);
            break;
        }
        default: break;
    }
}
void ui_on_press_callback(void* app_context, button_t btn)
{
    global_context_t* ctx = (global_context_t*)app_context;

    ui_state_t state = ctx->ui_ctx.state;
    if (btn == BUTTON_SELECT)
    {
        if (state == UI_STATE_CALIBRATE && ctx->calib.state == CALIB_STATE_IDLE)
        {
            imu_calibration_start(ctx);
        }
        return;
    }
    if (btn == BUTTON_LEFT)
    {
        if (state > UI_STATE_MAIN) state--;
    }
    else if (btn == BUTTON_RIGHT)
    {
        if (state < UI_STATE_COUNT - 1) state++;
    }
    ui_set_state(&ctx->ui_ctx, state);
}

void mavlink_send_msg(ble_device_t* dev,const mavlink_message_t* msg)
{
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len =mavlink_msg_to_send_buffer(buf,msg);
    ble_write(dev,buf,len);
}
void mavlink_send_attitude(void* app_context)
{
    global_context_t* ctx = app_context;
    if (!ble_is_connected(&ctx->ble)) return;
    mavlink_message_t msg;
    mavlink_msg_attitude_pack(
        MAVLINK_SYSTEM_ID,
        MAV_COMP_ID_ALL,
        &msg,
        systime_get(),
        ctx->attitude.roll,
        ctx->attitude.pitch,
        ctx->attitude.yaw,
        ctx->attitude.roll_rate,
        ctx->attitude.pitch_rate,
        ctx->attitude.yaw_rate);
    mavlink_send_msg(&ctx->ble,&msg);
}
int main()
{
    memset(&g_ctx,0,sizeof(g_ctx));
    init_sys(&g_ctx);
    init_peripherals(&g_ctx);
    ui_init(&g_ctx.ui_ctx,&g_ctx.oled,&g_ctx,ui_on_render_callback,ui_on_press_callback);
    sleep(20);
    PANIC(buttons_init(buttons_callback,&g_ctx));
    scheduler_add_task(oled_task,500,&g_ctx);
    scheduler_add_task(imu_calibration_task,1,&g_ctx);
    scheduler_add_task(mavlink_send_attitude,1000,&g_ctx);
    scheduler_add_task(attitude_task,10,&g_ctx);

    scheduler_run();
    return 0;
}
