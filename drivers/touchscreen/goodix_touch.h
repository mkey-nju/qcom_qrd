/*
 * include/linux/goodix_touch.h
 *
 * Copyright (C) 2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef 	_LINUX_GOODIX_TOUCH_H
#define		_LINUX_GOODIX_TOUCH_H

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>

#define GOODIX_I2C_NAME "ft5x06_ts"
#define GUITAR_SMALL
//???????
//#define TOUCH_MAX_HEIGHT 	7680
//#define TOUCH_MAX_WIDTH	 	5120
#define TOUCH_MAX_HEIGHT 	5120
#define TOUCH_MAX_WIDTH	 	7680




//???????
#define SCREEN_MAX_HEIGHT	480//480				
#define SCREEN_MAX_WIDTH	800//272



//#define    GPIOEIT   (42)
//#define 	SHUTDOWN_PORT_GROUP  (4)
//#define 	SHUTDOWN_PORT_INDEX   (26)//TP12 E26

#define    GPIOEIT   (48)
#define 	SHUTDOWN_PORT_GROUP  (2)
#define 	SHUTDOWN_PORT_INDEX   (30)//TP12 E26



//#define  INT_PORT  	S3C64XX_GPL(10)			//Int IO port
//#define TS_INT 		gpio_to_irq(INT_PORT)		//Interrupt Number,EINT18 as 119

//#define 	SHUTDOWN_PORT S3C64XX_GPF(3)	//SHUTDOWN???
//#define  INT_CFG    	S3C_GPIO_SFN(3)		//IO configer,EINT type

#define MAX_FINGER_NUM	5
#define FLAG_UP 		0
#define FLAG_DOWN 	1

#define GOODIX_MULTI_TOUCH
//#define GOODIX_SINGL_TOUCH
#undef GOODIX_TS_DEBUG
#define GOODIX_TS_DEBUG  1 //lsw
//#undef CONFIG_HAS_EARLYSUSPEND
#define goodix_swap(x, y) do { typeof(x) z = x; x = y; y = z; } while (0)

struct goodix_ts_data
{
    uint16_t addr;
    struct i2c_client *client;
    struct input_dev *input_dev;
    int use_irq;
    struct hrtimer timer;
    struct work_struct  work;
    char phys[32];
    int bad_data;
    int retry;
    int (*power)(int on);
    struct early_suspend early_suspend;
};

struct goodix_i2c_rmi_platform_data
{
    uint32_t version;	/* Use this entry for panels with */
    //??????????????
    //??,?????????

};




#endif /* _LINUX_GOODIX_TOUCH_H */
