/*---------------------------------------------------------------------------------------------------------
 * driver/input/touchscreen/goodix_touch.c
 *
 * Copyright(c) 2010 Goodix Technology Corp. All rights reserved.
 * Author: Eltonny
 * Date: 2010.11.11
 *
 *---------------------------------------------------------------------------------------------------------*/

#include <linux/input/mt.h>

#include "goodix_touch1.h"
#include "goodix_queue1.h"

/****************************modify by wangyihong**************************
************************************************************************/
#include "lidbg.h"

LIDBG_DEFINE;


#include "touch.h"
touch_t touch = {0, 0, 0};



#define 	RESOLUTION_X	(1024)  //this
#define 	RESOLUTION_Y	(600)
static bool xy_revert_en = 0;


unsigned int  touch_cnt = 0;
extern  bool is_ts_load;
extern int ts_should_revert;
extern unsigned int shutdown_flag_ts;
extern unsigned int irq_signal;

//extern void SOC_Log_Dump(int cmd);
/******************************************************
			added by wangyihong
******************************************************/

#ifndef GUITAR_GT80X
#error The code does not match the hardware version.
#endif

static struct workqueue_struct *goodix_wq;
struct goodix_ts_data *ts;


/********************************************
*	????????????,?????????????
*	???Guitar??		*/
static struct point_queue  finger_list;	//record the fingers list
/*************************************************/

const char *s3c_ts_name = "Goodix TouchScreen of GT80X";
/*used by guitar_update module */
struct i2c_client *i2c_connect_client = NULL;
EXPORT_SYMBOL(i2c_connect_client);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif


/*funciton: add node to open or close IRQ
    date: 2013.7.3
*/
bool debug_on = 0;
int open_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    printk("[wang]:=====GT801 enable_irq.\n");
    enable_irq(ts->client->irq);

    return 1;
}

int close_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    printk("[wang]:=====GT801 disable_irq_nosync. \n");
    disable_irq_nosync(ts->client->irq);

    return 1;
}

int print_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
    printk("[wang]:=====GT801 print on debug_log.\n");
    debug_on = 1;

    return 1;
}


void create_new_proc_entry()
{
    create_proc_read_entry("en_irq", 0, NULL, open_proc, NULL);
    // /cat proc/en_irq
    create_proc_read_entry("close_irq", 0, NULL, close_proc, NULL);
    // cat proc/close_irq
    create_proc_read_entry("print_on", 0, NULL, print_proc, NULL);
    // cat proc/print_on

}



/********************************************************
	add the debug printk_level function
********************************************************/
static int printk_level(int level, const char *format, ...)
{
    if(level < WANG_DEBUG_LEVEL)
    {
        va_list args;
        int r;
        va_start(args, format);
        r = vprintk(format, args);
        va_end(args);

        return r;
    }
    return 0;
}

/********************************************************
	create tsnod to check TS wether is  xy_revert or not
********************************************************/
#define feature_ts_nod

#ifdef feature_ts_nod
#define TS_DEVICE_NAME "tsnod"
static int major_number_ts = 0;
static struct class *class_install_ts;

struct ts_device
{
    unsigned int counter;
    struct cdev cdev;
};
struct ts_device *tsdev;

int ts_nod_open (struct inode *inode, struct file *filp)
{
    //do nothing
    filp->private_data = tsdev;
    printk("==[WANG] in ts_nod_open");

    return 0;          /* success */
}

ssize_t ts_nod_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char data_rec[20];
    struct ts_device *tsdev = filp->private_data;

    if (copy_from_user( data_rec, buf, count))
    {
        printk("copy_from_user ERR\n");
    }
    data_rec[count] =  '\0';
    printk("[futengfei]ts_nod_write:==%d====[%s]\n", count, data_rec);
    // processing data
    if(!(strnicmp(data_rec, "TSMODE_XYREVERT", count - 1)))
    {
        xy_revert_en = 1;
        printk("[futengfei]ts_nod_write:==========TSMODE_XYREVERT\n");
    }
    else if(!(strnicmp(data_rec, "TSMODE_NORMAL", count - 1)))
    {
        xy_revert_en = 0;
        printk("[futengfei]ts_nod_write:==========TSMODE_NORMAL\n");
    }

    return count;
}
static  struct file_operations ts_nod_fops =
{
    .owner = THIS_MODULE,
    .write = ts_nod_write,
    .open = ts_nod_open,
};

static int init_cdev_ts(void)
{
    int ret, err, result;

    //11creat cdev
    tsdev = (struct ts_device *)kmalloc( sizeof(struct ts_device), GFP_KERNEL );
    if (tsdev == NULL)
    {
        ret = -ENOMEM;
        printk("[futengfei]===========init_cdev_ts:kmalloc err \n");
        return ret;
    }

    dev_t dev_number = MKDEV(major_number_ts, 0);
    if(major_number_ts)
    {
        result = register_chrdev_region(dev_number, 1, TS_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev_number, 0, 1, TS_DEVICE_NAME);
        major_number_ts = MAJOR(dev_number);
    }
    printk("[futengfei]===========alloc_chrdev_region result:%d \n", result);

    cdev_init(&tsdev->cdev, &ts_nod_fops);
    tsdev->cdev.owner = THIS_MODULE;
    tsdev->cdev.ops = &ts_nod_fops;
    err = cdev_add(&tsdev->cdev, dev_number, 1);
    if (err)
        printk( "[futengfei]===========Error cdev_add\n");

    //cread cdev node in /dev
    class_install_ts = class_create(THIS_MODULE, "tsnodclass");
    if(IS_ERR(class_install_ts))
    {
        printk( "[futengfei]=======class_create err\n");
        return -1;
    }
    device_create(class_install_ts, NULL, dev_number, NULL, "%s%d", TS_DEVICE_NAME, 0);
}
#endif




/*******************************************************
??:
	??????
	????????i2c_msg??,?1???????????,
	?2??????????????;???????????
??:
	client:	i2c??,??????
	buf[0]:	????????
	buf[1]~buf[len]:?????
	len:	??????
return:
	?????
*********************************************************/
/*Function as i2c_master_send */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
    //add logic for update gt801
    if(shutdown_flag_ts != 0)
    {
        shutdown_flag_ts = 2;
        return -1;
    }

    struct i2c_msg msgs[2];
    int ret = -1;
    //?????
    msgs[0].flags = !I2C_M_RD; //???
    msgs[0].addr = client->addr;
    msgs[0].len = 1;
    msgs[0].buf = &buf[0];
    //????
    msgs[1].flags = I2C_M_RD; //???
    msgs[1].addr = client->addr;
    msgs[1].len = len - 1;
    msgs[1].buf = &buf[1];

    ret = i2c_transfer(client->adapter, msgs, 2);
    return ret;
}


/*******************************************************
??:
	??????
??:
	client:	i2c??,??????
	buf[0]:	???????
	buf[1]~buf[len]:?????
	len:	????
return:
	?????
*******************************************************/
/*Function as i2c_master_send */
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, int len)
{
    //add logic for update gt801
    if(shutdown_flag_ts != 0)
    {
        shutdown_flag_ts = 2;
        return -1;
    }

    struct i2c_msg msg;
    int ret = -1;
    //??????
    msg.flags = !I2C_M_RD; //???
    msg.addr = client->addr;  //client->addr;
    msg.len = len;  //len
    msg.buf = data;

    //printk("======before i2c_transfer, I2C address is %x====\n", client->addr);
    ret = i2c_transfer(client->adapter, &msg, 1);
    return ret;
}

/*******************************************************
??:
	Guitar?????,????????,??????
??:
	ts:	client???????
return:
	?????,0??????
*******************************************************/
static int goodix_init_panel(struct goodix_ts_data *ts)
{
    int ret = -1;

#ifdef BOARD_V1
    uint8_t config_info[] = {0x30,
                             0x0F, 0x05, 0x06, 0x28, 0x02, 0x14, 0x14, 0x10, 0x37, 0xB2,
                             0x02, 0x58, 0x04, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                             0xCD, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x4D, 0xC1, 0x20, 0x01,
                             0x01, 0x83, 0x64, 0x3C, 0x1E, 0x28, 0x0E, 0x00, 0x00, 0x00,
                             0x00, 0x50, 0x3C, 0x32, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x01
                            };
    //#endif


#elif defined(BOARD_V2)
    uint8_t config_info[] = {0x30,
                             0x13, 0x03, 0x07, 0x28, 0x02, 0x14, 0x14, 0x10, 0x3C, 0xB2,
                             0x02, 0x4e, 0x04, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                             0xCD, 0xE1, 0x00, 0x00, 0x35, 0x30, 0x4D, 0xC2, 0x20, 0x00,
                             0xE3, 0x80, 0x50, 0x3C, 0x1E, 0xB4, 0x00, 0x38, 0x33, 0x02,
                             0x30, 0x00, 0x5A, 0x32, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x01
                            };
    //#endif

#else
    uint8_t config_info[] = {0x30,
                             0x13, 0x03, 0x07, 0x28, 0x02, 0x14, 0x14, 0x10, 0x3C, 0xB2,
                             0x02, 0x4e, 0x04, 0x00, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB,
                             0xCD, 0xE1, 0x00, 0x00, 0x35, 0x30, 0x4D, 0xC2, 0x20, 0x00,
                             0xE3, 0x80, 0x50, 0x3C, 0x1E, 0xB4, 0x00, 0x38, 0x33, 0x02,
                             0x30, 0x00, 0x5A, 0x32, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x01
                            };
#endif


    ret = i2c_write_bytes(ts->client, config_info, 54);
    if (ret < 0)
        goto error_i2c_transfer;
    msleep(1);
    return 0;

error_i2c_transfer:

    return ret;
}
/*??GT80X???????*/
static int  goodix_read_version(struct goodix_ts_data *ts)
{
    int ret;
    uint8_t version[2] = {0x69, 0xff};	//command of reading Guitar's version
    uint8_t version_data[41];		//store touchscreen version infomation
    memset(version_data, 0 , sizeof(version_data));
    version_data[0] = 0x6A;
    ret = i2c_write_bytes(ts->client, version, 2);
    if (ret < 0)
    {
        printk("[wang]:===goodix_read_version.i2c_write_bytes error.\n");
        goto error_i2c_version;
    }
    msleep(16);
    ret = i2c_read_bytes(ts->client, version_data, 40);
    if (ret < 0)
    {
        printk("[wang]:===goodix_read_version.i2c_read_bytes error.\n");
        goto error_i2c_version;
    }
    dev_info(&ts->client->dev, " Guitar Version: %s\n", &version_data[1]);
    version[1] = 0x00;				//cancel the command
    i2c_write_bytes(ts->client, version, 2);
    return 0;

error_i2c_version:
    return ret;
}

/*******************************************************
??:
	???????
	?????,??1?????,????????
??:
	ts:	client???????
return:
	?????,0??????
********************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    static uint8_t finger_bit = 0;	//last time fingers' state
    uint8_t read_position = 0;
    uint8_t point_data[35] = { 0 };
    uint8_t finger = 0;				//record which finger is changed
    int ret = -1;
    int count = 0;
    int check_sum = 0;

    if(debug_on)
        printk( "[wang]:====come into  goodix_ts_work_func!\n");

    struct goodix_ts_data *ts = container_of(work, struct goodix_ts_data, work);
    //printk( "come into goodix_ts_work_func!\n");

#ifdef SHUTDOWN_PORT
    if (gpio_get_value(SHUTDOWN_PORT))
    {
        //printk(KERN_ALERT  "Guitar stop working.The data is invalid. \n");
        goto NO_ACTION;
    }
#endif

    //if i2c transfer is failed, let it restart less than 10 times
    if( ts->retry > 20)
    {
        if(!ts->use_irq && (ts->timer.state != HRTIMER_STATE_INACTIVE))
            hrtimer_cancel(&ts->timer);
        // dev_info(&(ts->client->dev), "Because of transfer error, %s stop working.\n", s3c_ts_name);
        printk("[wang]:=======Because of transfer error, stop working.\n");
        ts->retry = 0;
        goto NO_ACTION;//return ;
    }
    if(ts->bad_data)
        msleep(16);
    ret = i2c_read_bytes(ts->client, point_data, 35);
    if(ret <= 0)
    {
        //dev_err(&(ts->client->dev),"I2C transfer error. Number:%d\n ", ret);
        ts->bad_data = 1;
        ts->retry++;
        if(ts->power)
        {
            ts->power(ts, 0);
            ts->power(ts, 1);
        }
        else
        {
            ret = goodix_init_panel(ts);
            if(ret < 0)
            {
                printk("[wang]:=====goodix_init_panel error in work_func. \n", ret);
            }
            msleep(500);
        }
        printk("[wang]:=====I2C read point data error. Number:%d\n", ret);
        goto XFER_ERROR;
    }

    if(debug_on)
    {
        printk( "[wang]:====i2c_read point_data success!\n");
        printk("[wang]:====point_data[1] is %d\n", point_data[1]);
    }
    ts->bad_data = 0;


    //printk("=========point_data[1] is %d\n==========",point_data[1]);
    //The bit indicate which fingers pressed down
    switch(point_data[1] & 0x1f)
    {
    case 0:
    case 1:
        for(count = 1; count < 8; count++)
            check_sum += (int)point_data[count];
        if((check_sum % 256) != point_data[8])
        {
            printk("[wang]:=======check_sum 1 is failed\n");
            goto XFER_ERROR;
        }
        break;
    case 2:
    case 3:
        for(count = 1; count < 13; count++)
            check_sum += (int)point_data[count];
        if((check_sum % 256) != point_data[13])
        {
            printk("[wang]:=======check_sum 2 or 3 is failed\n");
            goto XFER_ERROR;
        }
        break;
    default:		//(point_data[1]& 0x1f) > 3
        for(count = 1; count < 34; count++)
            check_sum += (int)point_data[count];
        if((check_sum % 256) != point_data[34])
        {
            printk("[wang]:=======check_sum >3 is failed\n");
            goto XFER_ERROR;
        }
    }

    point_data[1] &= 0x1f;
    finger = finger_bit ^ point_data[1];
    if(finger == 0 && point_data[1] == 0)
        goto NO_ACTION;						//no fingers and no action
    else if(finger == 0)							//the same as last time
        goto BIT_NO_CHANGE;
    //check which point(s) DOWN or UP
    for(count = 0; (finger != 0) && (count < MAX_FINGER_NUM);  count++)
    {
        if((finger & 0x01) == 1)		//current bit is 1, so NO.postion finger is change
        {
            if(((finger_bit >> count) & 0x01) == 1)	//NO.postion finger is UP
            {
                //printk("========UP finger_bit is %d===========\n",finger_bit);
                set_up_point(&finger_list, count);
            }
            else
            {
                //printk("========DOWN finger_bit is %d===========\n",finger_bit);
                add_point(&finger_list, count);
            }
        }
        finger >>= 1;
    }


    if(!g_var.is_fly)
    {
        if(finger_list.length == 3)
        {
            SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
        }
        if (finger_list.length == 4)
        {
            //printk("SOC_Log_Dump\n");
            //SOC_Log_Dump(LOG_DMESG);
        }
        if(finger_list.length == 5)
        {
            SOC_Key_Report(KEY_MENU, KEY_PRESSED_RELEASED);
        }
    }


BIT_NO_CHANGE:
    for(count = 0; count < finger_list.length; count++)
    {
        if(finger_list.pointer[count].state == FLAG_UP)
        {
            finger_list.pointer[count].x = finger_list.pointer[count].y = 0;
            finger_list.pointer[count].pressure = 0;
            continue;
        }

        if(finger_list.pointer[count].num < 3)
            read_position = finger_list.pointer[count].num * 5 + 3;
        else if (finger_list.pointer[count].num == 4)
            read_position = 29;

        if(finger_list.pointer[count].num != 3)
        {
            finger_list.pointer[count].x = (unsigned int) (point_data[read_position] << 8) + (unsigned int)( point_data[read_position + 1]);
            finger_list.pointer[count].y = (unsigned int)(point_data[read_position + 2] << 8) + (unsigned int) (point_data[read_position + 3]);
            finger_list.pointer[count].pressure = (unsigned int) (point_data[read_position + 4]);
        }
        else
        {
            finger_list.pointer[count].x = (unsigned int) (point_data[18] << 8) + (unsigned int)( point_data[25]);
            finger_list.pointer[count].y = (unsigned int)(point_data[26] << 8) + (unsigned int) (point_data[27]);
            finger_list.pointer[count].pressure = (unsigned int) (point_data[28]);
        }

        //finger_list.pointer[count].x = finger_list.pointer[count].y;
        //finger_list.pointer[count].y = finger_list.pointer[count].x;
        swap(finger_list.pointer[count].x, finger_list.pointer[count].y);
        if((1 == xy_revert_en) || (1 == ts_should_revert)) //if x and y coordinate are revert
        {
            finger_list.pointer[count].x = RESOLUTION_X - finger_list.pointer[count].x;
            finger_list.pointer[count].y = RESOLUTION_Y - finger_list.pointer[count].y;
        }
    }

#ifndef GOODIX_MULTI_TOUCH
    if(finger_list.pointer[0].state == FLAG_DOWN)
    {
        input_report_abs(ts->input_dev, ABS_X, finger_list.pointer[0].x);
        input_report_abs(ts->input_dev, ABS_Y, finger_list.pointer[0].y);
    }
    input_report_abs(ts->input_dev, ABS_PRESSURE, finger_list.pointer[0].pressure);
    input_report_key(ts->input_dev, BTN_TOUCH, finger_list.pointer[0].state);
#else

    /* ABS_MT_TOUCH_MAJOR is used as ABS_MT_PRESSURE in android. */
    for(count = 0; count < (finger_list.length); count++)
    {
#ifndef BOARD_V2
        input_mt_slot(ts->input_dev, finger_list.pointer[count].num);
        if(finger_list.pointer[count].state == FLAG_DOWN)
        {
            input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_X, finger_list.pointer[count].x);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, finger_list.pointer[count].y);
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
        }
        else
            input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
#endif
        if(finger_list.pointer[count].state == FLAG_DOWN)
        {
#ifdef BOARD_V2
            input_report_abs(ts->input_dev, ABS_MT_POSITION_X, finger_list.pointer[count].x);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, finger_list.pointer[count].y);
            //input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, finger_list.pointer[count].pressure);
            //input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, finger_list.pointer[count].pressure);
            input_report_key(ts->input_dev, BTN_TOUCH, finger_list.pointer[count].state);
#endif
            g_var.flag_for_15s_off++;
            if(g_var.flag_for_15s_off >= 1000)
            {
                g_var.flag_for_15s_off = 1000;
            }
            if(g_var.flag_for_15s_off < 0)
            {
                printk("\n[wang]:====err:FLAG_FOR_15S_OFF===[%d]\n", g_var.flag_for_15s_off);
            }

            g_curr_tspara.x = finger_list.pointer[0].x;
            g_curr_tspara.y = finger_list.pointer[0].y;
            g_curr_tspara.press = true;

            if(1 == recovery_mode)
            {
                if( (finger_list.pointer[0].x >= 0) && (finger_list.pointer[0].y >= 0) )
                {
                    touch.x = finger_list.pointer[0].x;
                    touch.y = finger_list.pointer[0].y;
                    touch.pressed = 1;
                    set_touch_pos(&touch);
                }
            }
            touch_cnt++;
            if (touch_cnt > 100)
            {
                touch_cnt = 0;
                printk("QR%d[%d,%d]\n", xy_revert_en, finger_list.pointer[count].x, finger_list.pointer[count].y);
            }


        }
        else
        {
#ifdef BOARD_V2
            input_report_key(ts->input_dev, BTN_TOUCH, finger_list.pointer[count].state);
#endif

            g_curr_tspara.press = false;

            if(1 == recovery_mode)
            {
                {
                    touch.pressed = 0;
                    set_touch_pos(&touch);
                }
            }
            printk("%d[0,0]\n", xy_revert_en);

        }
        //input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, finger_list.pointer[count].pressure);
        //input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, finger_list.pointer[count].pressure);
#ifdef BOARD_V2
        input_mt_sync(ts->input_dev);
#endif
    }
#ifndef BOARD_V2
    input_report_key(ts->input_dev, BTN_TOUCH, !!finger_list.length);
    input_sync(ts->input_dev);
#endif




#if 0
    read_position = finger_list.length - 1;
    if(finger_list.length > 0 && finger_list.pointer[read_position].state == FLAG_DOWN)
    {
        input_report_abs(ts->input_dev, ABS_MT_POSITION_X, finger_list.pointer[read_position].x);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, finger_list.pointer[read_position].y);
    }
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, finger_list.pointer[read_position].pressure);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, finger_list.pointer[read_position].pressure);
    input_mt_sync(ts->input_dev);
#endif
#endif

#ifdef BOARD_V2
    input_sync(ts->input_dev);
#endif
    del_point(&finger_list);
    finger_bit = point_data[1];

XFER_ERROR:
NO_ACTION:
    if(ts->use_irq)
        enable_irq(ts->client->irq);
    if(debug_on)
    {
        printk( "[wang]:====end of goodix_ts_work_func!\n");
    }

}

/*******************************************************
??:
	???????
	??????,???????????;??????
??:
	timer:????????
return:
	???????,HRTIMER_NORESTART?????????
********************************************************/
static enum hrtimer_restart goodix_ts_timer_func(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);

    queue_work(goodix_wq, &ts->work);
    if(ts->timer.state != HRTIMER_STATE_INACTIVE)
        hrtimer_start(&ts->timer, ktime_set(0, 16000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
??:
	??????
	?????,???????????
??:
	timer:????????
return:
	???????,HRTIMER_NORESTART?????????
********************************************************/
#if defined(INT_PORT)
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;
    if(debug_on)
        printk( "[wang]:====come into  goodix_ts_irq_handler!\n");

    disable_irq_nosync(ts->client->irq);
    queue_work(goodix_wq, &ts->work);

    return IRQ_HANDLED;
}
#endif

/*******************************************************
??:
	GT80X?????
??:
	on:??GT80X????,0???Sleep??
return:
	??????,??0??????
********************************************************/
#if defined(SHUTDOWN_PORT)
static int goodix_ts_power(struct goodix_ts_data *ts, int on)
{
    int ret = -1;
    if(ts == NULL || (ts && !ts->use_shutdown))
        return -1;
    switch(on)
    {
    case 0:
        gpio_set_value(SHUTDOWN_PORT, 1);
        msleep(5);
        if(gpio_get_value(SHUTDOWN_PORT))	//has been suspend
            ret = 0;
        break;
    case 1:
        gpio_set_value(SHUTDOWN_PORT, 0);
        msleep(5);
        if(gpio_get_value(SHUTDOWN_PORT))	//has been suspend
            ret = -1;
        else
        {
            ret = goodix_init_panel(ts);
            if(!ret)
                msleep(500);
        }
        break;
        //default:printk("Command ERROR.\n");
    }
    //printk(on?"Set Guitar's Shutdown LOW\n":"Set Guitar's Shutdown HIGH\n");
    return 0;
}
#endif

/*******************************************************
??:
	???????
	????????(???????client);
	??IO,???????;????;?????????
??:
	client:?????????
	id:??ID
return:
	?????,0??????
********************************************************/

static int screen_x = 0;
static int screen_y = 0;
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    int retry = 0;
    int count = 0;
    struct goodix_i2c_rmi_platform_data *pdata;                               //what is the variable used to?
    i2c_connect_client = client;				                                 //using  for Guitar Updating.

    printk( "====wang======come into goodix_ts_probe============\n");

    /*veryfy the i2c adpater funtion*/
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        //dev_err(&client->dev, "System need I2C function.\n");
        printk("[wang]========i2c_check_functionality failed.\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }


    /*configure the SHUTDOWN_PORT, here not used*/
#ifdef SHUTDOWN_PORT
    ret = gpio_request(SHUTDOWN_PORT, "TS_SHUTDOWN");	//Request IO
    if (ret < 0)
    {
        printk(KERN_ALERT "Failed to request GPIO:%d, ERRNO:%d\n", (int)SHUTDOWN_PORT, ret);
        goto err_gpio_request;
    }
    gpio_direction_output(SHUTDOWN_PORT, 0);	//Touchscreen is waiting to wakeup
    ret = gpio_get_value(SHUTDOWN_PORT);
    if (ret)
    {
        printk(KERN_ALERT  "Cannot set touchscreen to work.\n");
        goto err_i2c_failed;
    }
#endif


    msleep(16);
    unsigned int buf = 0;
    client->addr = 0x55;

    /*allocate kernel space for ts, and initialize it*/
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        printk("[wang]========alloc ts_data failed.\n");
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }
    INIT_WORK(&ts->work, goodix_ts_work_func);                      //initialize work struct for TS.
    ts->client = client;
    i2c_set_clientdata(client, ts);
    pdata = client->dev.platform_data;                                        //pdata used for what?

    /*Test i2c communication whether is rihgt or not*/
    for(retry = 0; retry < 5; retry++)
    {
        ret = i2c_write_bytes(client, &buf, 1);
        if (ret > 0)
            break;
    }
    if(ret < 0)
    {
        //dev_err(&client->dev, "========Warnning: I2C connection might be something wrong!\n");
        printk("[wang]========I2C connection might be something wrong!\n");
        goto err_i2c_failed;
    }

#ifdef SHUTDOWN_PORT
    ts->use_shutdown = 1;
    gpio_set_value(SHUTDOWN_PORT, 1);		         //suspend
#endif



    /*allocate input_dev, and initialize it, then register to kernel input sub-system */
    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        ret = -ENOMEM;
        //dev_dbg(&client->dev, "Failed to allocate input device\n");
        printk("[wang]========allocate input device failed!\n");
        goto err_input_dev_alloc_failed;
    }
#define SUPPORT_MULTI_RESOLUTION
#ifdef SUPPORT_MULTI_RESOLUTION
    screen_x = RESOLUTION_X;
    screen_y = RESOLUTION_Y;
    SOC_Display_Get_Res(&screen_x, &screen_y);
    printk( "[wang]:===get touch screen resolution is [%d*%d].\n", screen_x, screen_y);
#endif

#ifdef BOARD_V2
    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);	//can remove
    ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);		//can remove

    /*	for android 1.x multi-touch, not realized.
    	BIT_MASK(ABS_MT_TOUCH_MAJOR)| BIT_MASK(ABS_MT_WIDTH_MAJOR)
    	BIT_MASK(ABS_MT_POSITION_X) |
    	BIT_MASK(ABS_MT_POSITION_Y);
    */

    input_set_abs_params(ts->input_dev, ABS_X, 0, SCREEN_MAX_HEIGHT, 0, 0);          //can remove
    input_set_abs_params(ts->input_dev, ABS_Y, 0, SCREEN_MAX_WIDTH, 0, 0);		  //can remove
    input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 38, 0, 0);	                          //can remove

    /*support auto adapt TS with various resolution*/
#ifdef GOODIX_MULTI_TOUCH
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, screen_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, screen_y, 0, 0);
#endif

#else
    __set_bit(EV_KEY, ts->input_dev->evbit);
    __set_bit(EV_ABS, ts->input_dev->evbit);
    __set_bit(BTN_TOUCH, ts->input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

    input_mt_init_slots(ts->input_dev, 5);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, screen_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, screen_y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
#endif

    sprintf(ts->phys, "input/ts)");
    ts->input_dev->name = s3c_ts_name;
    ts->input_dev->phys = ts->phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;

    /*register input device*/
    finger_list.length = 0;
    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        //dev_err(&client->dev, "Probe: Unable to register %s input device\n", ts->input_dev->name);
        printk( "[wang]:====input_register_device failed.\n");
        goto err_input_register_device_failed;
    }

    ts->use_irq = 0;
    ts->retry = 0;
    ts->bad_data = 0;

    /*configure INT_PORT, and request irq*/
#if defined(INT_PORT)
    client->irq = TS_INT;
    if (client->irq)
    {
        ret = gpio_request(INT_PORT, "TS_INT");	//Request IO
        if (ret < 0)
        {
            // dev_err(&client->dev, "Failed to request GPIO:%d, ERRNO:%d\n", (int)INT_PORT, ret);
            printk( "[wang]:====gpio_request INT failed.\n");
            goto err_int_request_failed;
        }
        //ret = s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port function
        ret = gpio_tlmm_config(GPIO_CFG(INT_PORT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_16MA), GPIO_CFG_ENABLE);
        ret = gpio_get_value(INT_PORT);

        ret  = request_irq(TS_INT, goodix_ts_irq_handler ,  IRQ_TYPE_EDGE_FALLING,
                           client->name, ts);
        if (ret != 0)
        {
            //dev_err(&client->dev, "Can't allocate touchscreen's interrupt!ERRNO:%d\n", ret);
            printk( "[wang]:========request_irq failed.\n");
            gpio_direction_input(INT_PORT);
            gpio_free( INT_PORT);
            goto err_int_request_failed;
        }
        else
        {
            disable_irq(TS_INT);
            ts->use_irq = 1;
            printk( "[wang]:======Reques EIRQ %d succesd on GPIO:%d\n", TS_INT, INT_PORT);
        }
    }
#endif


err_int_request_failed:
    if (!ts->use_irq)
    {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
        printk( "[wang]:===Reques EIRQ %d failed on GPIO:%d\n", TS_INT, INT_PORT);

    }

#ifdef SHUTDOWN_PORT
    gpio_set_value(SHUTDOWN_PORT, 0);
    msleep(10);
    ts->power = goodix_ts_power;
#endif

    /*initialize the IC and read version*/
    for(count = 0; count < 5; count++)
    {
        ret = goodix_init_panel(ts);
        if(ret != 0)		//Initiall failed
        {
            printk("[wang]:=====goodix_ts_probe.goodix_init_panel fail and again----------->GT801the %d times\n", count);
#ifdef BOARD_V2
            SOC_IO_Output(0, 27, 0);
            msleep(300);
            SOC_IO_Output(0, 27, 1);//NOTE:GT801 SHUTDOWN PIN ,set LOW  to work.
            msleep(700);
#else
            SOC_IO_Output(0, 27, 1);
            msleep(300);
            SOC_IO_Output(0, 27, 0);//NOTE:GT801 SHUTDOWN PIN ,set LOW  to work.
            msleep(700);
#endif
            continue;
        }
        else
        {
            printk( "[wang]:====goodix_init_panel success !\n");
            if(ts->use_irq)
                enable_irq(TS_INT);
            break;
        }
    }
    if(ret != 0)
    {
        printk( "[wang]:====goodix_init_panel failed !\n");
        ts->bad_data = 1;
        goto err_init_godix_ts;
    }
    goodix_read_version(ts);
    msleep(500);

    create_new_proc_entry();  // cat /proc/en_irq  to enable IRQ , /proc/close_irq to disable IRQ,  cat proc/print_on to print log.


    /*power management*/
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 20;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif
    // SOC_Fake_Register_Early_Suspend(&ts->early_suspend);  //fake suspend

    dev_info(&client->dev, "Start  %s in %s mode\n",
             ts->input_dev->name, ts->use_irq ? "Interrupt" : "Polling");
    printk("[wang]:====goodix_ts_probe is finished\n");
    return 0;

err_init_godix_ts:
    if(ts->use_irq)
    {
        free_irq(TS_INT, ts);
#if defined(INT_PORT)
        gpio_free(INT_PORT);
#endif
    }

err_input_register_device_failed:
    input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
    i2c_set_clientdata(client, NULL);
err_i2c_failed:
#ifdef SHUTDOWN_PORT
    gpio_direction_input(SHUTDOWN_PORT);
    gpio_free(SHUTDOWN_PORT);
#endif
err_gpio_request:
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
    return ret;
}


/*******************************************************
??:
	??????
??:
	client:?????
return:
	?????,0??????
********************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif
    if (ts->use_irq)
    {
        free_irq(client->irq, ts);
#if defined(INT_PORT)
        gpio_free(INT_PORT);
#endif
    }
    else
        hrtimer_cancel(&ts->timer);

#ifdef SHUTDOWN_PORT
    if(ts->use_shutdown)
    {
        gpio_direction_input(SHUTDOWN_PORT);
        gpio_free(SHUTDOWN_PORT);
    }
#endif
    dev_notice(&client->dev, "The driver is removing...\n");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    if(ts->input_dev)
        kfree(ts->input_dev);
    kfree(ts);
    return 0;
}

//????
static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    printk("[futengfei]come into [%s]=====remodify timing sequence====2013.07.12=\n", __func__);

    if (ts->use_irq)
    {
        printk( "disable_irq((INT_PORT));\n");
        disable_irq(MSM_GPIO_TO_INT(INT_PORT));
    }

    else if(ts->timer.state)
        hrtimer_cancel(&ts->timer);
    ret = cancel_work_sync(&ts->work);
    if(ret && ts->use_irq)
    {
        printk("[wang]:=====enable irq in suspend.\n");
        enable_irq(client->irq);
    }
    return 0;
}
//????
static int goodix_ts_resume(struct i2c_client *client)
{
    int ret = 0, retry = 0, init_err = 0;
    uint8_t GT811_check[6] = {0x55};
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    printk("come into [%s]====remodify the timing sequence====2013.07.12=== [futengfei]=\n", __func__);
    for(retry = 0; retry < 5; retry++)
    {

#ifdef BOARD_V2
        SOC_IO_Output(0, 27, 0);
        msleep(20);
        SOC_IO_Output(0, 27, 1);
        msleep(200);
#else
        SOC_IO_Output(0, 27, 1);
        msleep(20);
        SOC_IO_Output(0, 27, 0);//NOTE:GT801 SHUTDOWN PIN ,set LOW  to work.
        msleep(200);
#endif
        ret = goodix_init_panel(ts);
        if(ret < 0)
        {
            printk("[wang]:====goodix_init_panel failed in goodix_ts_resume\n");
        }
        init_err = SOC_I2C_Rec(1, 0x55, 0x68, GT811_check, 6 );
        ret = 0;
        if(init_err < 0)
        {
            printk("[futengfei]goodix_init_panel:goodix_init_panel failed====retry=[%d] init_err=%d\n", retry, init_err);
            ret = 1;
        }
        else
        {
            printk("[futengfei]goodix_init_panel:goodix_init_panel success====retry=[%d] init_err=%d\n\n\n", retry, init_err);
            ret = 0;
        }

        msleep(8);
        if(ret != 0)	//Initiall failed
        {
            printk("[futengfei]goodix_init_panel:goodix_init_panel failed=========retry=[%d]===ret[%d]\n", retry, ret);
            //SOC_IO_Output(0, 27, 0);
            //msleep(20);
            //SOC_IO_Output(0, 27, 1);
            //msleep(200);
            continue;
        }

        else
            break;

        printk("[futengfei] goodix_ts_resume:if this is appear ,that is say the continue no goto for directly!\n");

    }

    if(ret != 0)
    {
        printk("goodix_init_panel:Initiall failed============");
        ts->bad_data = 1;
    }


    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
    goodix_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    ts = container_of(h, struct goodix_ts_data, early_suspend);
    goodix_ts_resume(ts->client);

    if (ts->use_irq)
    {
        printk( "[wang]:========enable_irq((INT_PORT));\n");
        enable_irq(MSM_GPIO_TO_INT(INT_PORT));
    }

    else
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
}
#endif

//??????? ???—??ID ??
//only one client
static const struct i2c_device_id goodix_ts_id[] =
{
    { GOODIX_I2C_NAME, 0 },
    { }
};

//???????
static struct i2c_driver goodix_ts_driver =
{
    .probe		= goodix_ts_probe,
    .remove		= goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend	= goodix_ts_suspend,
    .resume		= goodix_ts_resume,
#endif
    .id_table	= goodix_ts_id,
    .driver = {
        .name	= GOODIX_I2C_NAME,
        .owner = THIS_MODULE,
    },
};

/*******************************************************
??:
	??????
return:
	?????,0??????
********************************************************/
static int __devinit goodix_ts_init(void)
{
    int ret;

    LIDBG_GET;

    is_ts_load = 1;
    printk("================into Gt801.ko=1024590==============2013.07.12==\n");

    /*configure shutdown pin,ensure this pin is low, make IC in working state*/
#ifdef BOARD_V2
    SOC_IO_Output(0, 27, 0);
    msleep(200);
    SOC_IO_Output(0, 27, 1);
    msleep(300);
#else
    SOC_IO_Output(0, 27, 1);
    msleep(200);
    SOC_IO_Output(0, 27, 0);
    msleep(300);
#endif

    init_cdev_ts();
    goodix_wq = create_workqueue("goodix_wq");
    if (!goodix_wq)
    {
        printk(KERN_ALERT "Creat workqueue faiked\n");
        return -ENOMEM;

    }


    ret = i2c_add_driver(&goodix_ts_driver);
    if(irq_signal == 1)
    {
        printk("[wang]:=======GT801 update is successful, enable the irq.\n");
        enable_irq(ts->client->irq);
    }

    printk("====================out Gt801.ko===============2013.07.12==\n");
    return ret;
}

/*******************************************************
??:
	??????
??:
	client:?????
********************************************************/
static void __exit goodix_ts_exit(void)
{
    printk(KERN_DEBUG "%s is exiting...\n", s3c_ts_name);
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
        destroy_workqueue(goodix_wq);
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL");
