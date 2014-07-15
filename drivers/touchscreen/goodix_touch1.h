/*---------------------------------------------------------------------------------------------------------
 * kernel/include/linux/goodix_touch.h
 *
 * Copyright(c) 2010 Goodix Technology Corp. All rights reserved.
 * Author: Eltonny
 * Date: 2010.11.11
 *
 *---------------------------------------------------------------------------------------------------------*/

#ifndef 	_LINUX_GOODIX_TOUCH_H
#define		_LINUX_GOODIX_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#define GOODIX_I2C_NAME "ft5x06_ts"
#define GUITAR_GT80X
//???????
#define TOUCH_MAX_HEIGHT 	7680//7680	
#define TOUCH_MAX_WIDTH	 5120	//5120
//???????,????????,??????????
#define SCREEN_MAX_HEIGHT	1024
#define SCREEN_MAX_WIDTH	600

//#define SHUTDOWN_PORT 	S3C64XX_GPF(3)			//SHUTDOWN???
#define INT_PORT  		48			//Int IO port
#ifdef INT_PORT
#define TS_INT 		MSM_GPIO_TO_INT(INT_PORT)	//Interrupt Number,EINT18 as 119
#define  INT_CFG    	S3C_GPIO_SFN(3)			//IO configer,EINT type
#else
#define TS_INT 	0
#endif

#define FLAG_UP 		0
#define FLAG_DOWN 	1

#define GOODIX_MULTI_TOUCH
#ifndef GOODIX_MULTI_TOUCH
#define MAX_FINGER_NUM 1
#else
#define MAX_FINGER_NUM 5					//???????(<=5)
#endif
#undef GOODIX_TS_DEBUG

#define swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)

struct goodix_ts_data
{
    uint16_t addr;
    struct i2c_client *client;
    struct input_dev *input_dev;
    uint8_t use_irq;
    uint8_t use_shutdown;
    struct hrtimer timer;
    struct work_struct  work;
    char phys[32];
    int bad_data;
    int retry;
    struct early_suspend early_suspend;
    int (*power)(struct goodix_ts_data *ts, int on);
};

struct goodix_i2c_rmi_platform_data
{
    uint32_t version;	/* Use this entry for panels with */
    //??????????????
    //??,?????????

};

/******************************************************
		add the debug_printk switch to control the log
*******************************************************/
enum print_info_level
{
    LEVEL_ERROR	= 3,
    LEVEL_WARN 	= 4,
    LEVEL_INFO	= 6,
    LEVEL_DEBUG	= 7,
};
/* 8 is debug level, and change it to set what can be printed*/
#define WANG_DEBUG_LEVEL  8
/* kernel log tag*/
#define WANG_LOG_TAG  "WANG**YI**HONG"

static int printk_level(int level, const char *fmt, ...)	\
__attribute__ ((format(printf, 2, 3)));
#define debug_printk(level, fmt, arg...) \
	printkl("%s: "arg)


#endif /* _LINUX_GOODIX_TOUCH_H */
