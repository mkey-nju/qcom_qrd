/*
 * include/linux/guitar_update.h
 *
 * Copyright (C) 2010 Goodix, Inc. All rights reserved.
 * Author: Eltonny
 * Date:   2010.9.26
 *
 */
//----------------------------------------------
#ifndef _LINUX_GUITAR_UPDATE_H
#define _LINUX_GUITAR_UPDATE_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>

#include <asm/uaccess.h>

#include <mach/gpio.h>
#define SHUTDOWN_PORT  27 //S5PV210_GPJ3(6)

struct goodix_ts_data
{
    spinlock_t irq_lock;
    struct i2c_client *client;
    struct input_dev  *input_dev;
    struct hrtimer timer;
    struct work_struct  work;
    struct early_suspend early_suspend;
    s32 irq_is_disable;
    s32 use_irq;
    u16 abs_x_max;
    u16 abs_y_max;
    u8  max_touch_num;
    u8  int_trigger_type;
    u8  green_wake_mode;
    u8  chip_type;
    u8  enter_update;
    u8  gtp_is_suspend;
    u8  gtp_rawdiff_mode;
    u8  gtp_cfg_len;
};

//7, MAX NUMBER of BYTES by MTK's i2c operation(not include data address).
#define MAX_I2C_BYTES_NUM	7

//?????
#define ERROR_NO_FILE				ENOENT	// 2
#define ERROR_FILE_READ				ENFILE	// 23
#define ERROR_FILE_TYPE				EISDIR	// 21
#define ERROR_GPIO_REQUEST			EINTR	//	4
#define ERROR_I2C_TRANSFER			EIO		// 5
#define ERROR_TIMEOUT				ETIMEDOUT //110			

#define GT80X_CHIP_TYPE_0 		"GT800"
#define GT80X_CHIP_TYPE_1 		"GT801"

#define GT80X_VERSION_LENGTH	40

#define MAX_IAP_RETRY			5 //10

enum gt80x_driver_state
{
    GOODIX_TS_STOP	=	0,	/*Stop driver(work) running.*/
    GOODIX_TS_CONTINUE	=	1,	/*Resume driver to read coordinates*/
};

enum gt80x_version_state
{
    VERSION_INVALID 	= 0,
    /**?????????,???????? */
    VERSION_CONFUSION 	= 1,
    /**?????IC??????? */
    VERSION_LOWER		= 2,
    /**????? */
    VERSION_HIGHER		= 3,
    /**?????,???? */
    VERSION_EQUAL		= 4,
};

enum print_info_level
{
    LEVEL_ERROR	= 3,
    LEVEL_WARN 	= 4,
    LEVEL_INFO	= 6,
    LEVEL_DEBUG	= 7,
};

enum gt80x_iap_state
{
    STATE_NULL	= 0,		/*No IAP Process*/
    STATE_CHECK	= 1,		/*On Check Process*/
    /*Others, On IAP Process*/
    STATE_START,
    STATE_WRITE_RAM,
    STATE_WRITE_ROM,
    STATE_CHECK_ROM,
    STATE_SUCCESS,
    STATE_FINASH,
};

/* 8 is debug level, and change it to set what can be printed*/
#define GT80X_DEBUG_LEVEL 8
/* kernel log tag*/
#define GT80X_LOG_TAG  "GT80X-IAP"

static int printk_level(int level, const char *fmt, ...)	\
__attribute__ ((format(printf, 2, 3)));
#define debug_printk(level, fmt, arg...) \
	printk_level(level, "%s: "fmt, GT80X_LOG_TAG, ##arg)

#if 0
#define assert(exptr) 	\
		do { if(!exptr)  { debug_printk(LEVEL_DEBUG, "Null Pointer in function:%s, lines:%d\n", __FUNCTION__,__LINE__); } }  while(0)
#define i2c_assert(exptr) \
		do { if(exptr <= 0)  { debug_printk(LEVEL_DEBUG, "I2C Error in function:%s, line:%d\n", __FUNCTION__,_LINE__); } }  while(0)
#else
#define assert(exptr)
#define i2c_assert(exptr)
#endif

#endif	/* _LINUX_GUITAR_UPDATE_H */
