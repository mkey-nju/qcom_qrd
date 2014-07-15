#ifndef __LIDBG_VIDEOIN_MAIN_H__
#define __LIDBG_VIDEOIN_MAIN_H__


#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <asm/system.h>
#include <linux/time.h>
#include <linux/pwm.h>
#include <linux/hrtimer.h>

#include <linux/stat.h>


#include <linux/i2c.h>
#include <linux/spi/spi.h>

//#include <linux/smp_lock.h>


#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>

#include <linux/types.h>

#include <linux/sched.h>   //wake_up_process()
#include <linux/kthread.h> //kthread_create()¡¢kthread_run()
#include <linux/input.h>


#include <linux/proc_fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/kfifo.h>


#include "i2c_io.h"
#include "tw9912.h"
//#include "tc358746xbg_config.h"
#include "TC358746XBG.h"

#define TW9912_IOC_MAGIC  'T'
#define COPY_TW9912_STATUS_REGISTER_0X01_4USER   _IO(TW9912_IOC_MAGIC, 1)
#define AGAIN_RESET_VIDEO_CONFIG_BEGIN   _IO(TW9912_IOC_MAGIC, 2)
#define TW9912_IOC_MAXNR 2

#endif
