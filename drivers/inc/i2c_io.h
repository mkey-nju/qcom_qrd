#ifndef __i2c_io_H__
#define __i2c_io_H__
#include <asm/atomic.h>
#include <linux/semaphore.h>
//#define GPIO_I2C_SCL   43 //tw9912 reset
//#define GPIO_I2C_SDA   15//tp0708
#if (defined(BOARD_V1) || defined(BOARD_V2))
#define GPIO_I2C_SCL   32
#define GPIO_I2C_SDA   107
#else
#define FLY_VIDEO_BOARD_V3 //add by huangzongqiang
#define GPIO_I2C_SCL   35
#define GPIO_I2C_SDA   109
#endif

typedef enum
{
    NACK = 0,
    ACK,
} i2c_ack;

typedef enum
{
    hkj_WRITE = 0,
    hkj_READ,
} i2c_WR_flag;
void i2c_io_config_init(void);
int i2c_io_config(unsigned int index, unsigned int direction, unsigned int pull, unsigned int drive_strength, unsigned int flag);
i2c_ack i2c_read_byte(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
i2c_ack i2c_write_byte(int bus_id, char chip_addr, char *buf, unsigned int size);
i2c_ack i2c_read_2byte(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
i2c_ack i2c_write_2byte(int bus_id, char chip_addr, char *buf, unsigned int size);

#endif  /* __mm_i2c_H__ */
