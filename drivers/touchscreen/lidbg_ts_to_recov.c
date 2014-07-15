/*
 * 12linux/drivers/serial/tcc_dev_cypress.c
 anxiao_cap_dev.ko
 deyi----gooddix.ko
 zhongchu------gh.ko
 qimei ------qnovox.ko


 * Author: <linux@telechips.com>
 * Created: June 10, 2008
 * Description: Touchscreen driver for devC2003 on Telechips TCC Series
 *
 * Copyright (C) 2008-2009 Telechips
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/i2c.h>
//#include <linux/tcc_dev_cypress.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

//#include <asm/io.h>
//#include <asm/irq.h>
#include <asm/system.h>
#include <asm/uaccess.h>
//#include <mach/hardware.h>

//#include <mach/bsp.h>
//#include <mach/irqs.h>

#include <linux/cdev.h>
#include "touch.h"

#define DEVICE_NAME "fenzhi"
static touch_t touch = {0, 0, 0};

struct test_input_dev
{
    char *name;
    struct input_dev *input_dev;
    struct timer_list timer;
    unsigned int counter;
    struct miscdevice *miscdev;
};

void set_touch_pos(touch_t *t)
{
#if 0
    int i = 0;
    i = touch.x;
    touch.x = touch.y;
    touch.y = i;
#endif
    memcpy( &touch, t, sizeof(touch_t) );
}

EXPORT_SYMBOL_GPL(set_touch_pos);

//global variable
struct test_input_dev *dev;
int fenzhi_major;
int data = 8;
static void dev_timer_func(unsigned long arg)
{
    mod_timer(&dev->timer, jiffies + HZ);
    (dev->counter)++;
    if(dev->counter > 60)dev->counter = 0;
    printk("touch.x:%d touch.y:%d touch.press:[%d]\n", touch.x, touch.y, touch.pressed);
    //printk("second%d\n", dev->counter);
    //input_report_abs(dev->input_dev, ABS_X, dev->counter);
    //input_report_abs(dev->input_dev, ABS_Y, dev->counter);
    //input_sync(dev->input_dev);
}


int fenzhi_open (struct inode *inode, struct file *filp)
{
    //do nothing
    return 0;          /* success */
}



/*
 * Data management: read and write
 */
static status = 0;
ssize_t fenzhi_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct test_input_dev *dev = filp->private_data;

    if(sizeof(touch_t) > count)return 0;

    if( !(touch.x || touch.y) )return 0;

    copy_to_user(buf, &touch, sizeof(touch_t));

    return sizeof(touch_t);
}



ssize_t fenzhi_write (struct file *filp, const char __user *buf, size_t count,
                      loff_t *f_pos)
{
    struct test_input_dev *dev = filp->private_data;

    return 0;
}

struct file_operations fenzhi_fops =
{
    .owner =     THIS_MODULE,
    .read =	     fenzhi_read,
    .write =     fenzhi_write,
    .open =	     fenzhi_open,
};

static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &fenzhi_fops,
};

//static int __devinit
//static int initialized = 0;

static  int my_input_driver_init(void)
{
    int ret;

    //printk("my_input_driver_init,write by hujian 2012/2/8 16:45");
    printk(KERN_CRIT"lidbg_ts_to_recov.ko  my_input_driver_init. \n");
    dev = (struct test_input_dev *)kmalloc( sizeof(struct test_input_dev), GFP_KERNEL );
    if (dev == NULL)
    {
        ret = -ENOMEM;
        printk(KERN_CRIT "%s: Failed to allocate input device\n", __func__);
        return ret;
    }

    //miscdev
    dev->miscdev = &misc;

    ret = misc_register(&misc);
    if(ret < 0 )
    {
        kfree(dev);
        return ret;
    }

#if 0
    init_timer(&dev->timer);
    dev->timer.data = (unsigned long)dev;
    dev->timer.function = dev_timer_func;
    dev->timer.expires = jiffies + HZ;

    add_timer(&dev->timer);
    printk("\n\nTest input driver has launch successfully!\n\n");
#else
    printk(KERN_CRIT"\n\nTest input driver has launch successfully!_delete the time ==futengfei=1025\n\n");
#endif
    return 0;

}

//void __exit
static void my_input_driver_exit(void)
{
    misc_deregister(&misc);
    del_timer(&dev->timer);
    //input_unregister_device(dev->input_dev);
    kfree(dev);
}
//EXPORT_SYMBOL_GPL(my_input_driver_init);
//EXPORT_SYMBOL_GPL(my_input_driver_exit);

module_init(my_input_driver_init);
module_exit(my_input_driver_exit);

MODULE_AUTHOR("hujian");
MODULE_DESCRIPTION("input driver,just for test");
MODULE_LICENSE("GPL");

