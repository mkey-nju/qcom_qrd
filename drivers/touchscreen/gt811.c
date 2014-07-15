/* drivers/input/touchscreen/gt811.c
 *
 * Copyright (C) 2010 - 2011 Goodix, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 *Any problem,please contact andrew@goodix.com,+86 755-33338828
 *
 */
#include "lidbg.h"
LIDBG_DEFINE;

#include <linux/input/mt.h>

#include "gt811.h"
#include "gt811_firmware.h"

#include "touch.h"
touch_t touch = {0, 0, 0};



#define SCREEN_X (1024)
#define SCREEN_Y (600)
static bool xy_revert_en = 0;

extern  bool is_ts_load;
extern int ts_should_revert;
//SOC_Log_Dumpextern void SOC_Log_Dump(int cmd);

static int have_load = 0;    // 1: have load ,do't load again
static int sensor_id = 2;	    //0:  8cunTS;      2: 7cunTS ;   7cun for default


#define LOG_DMESG (1)

//static struct work_struct goodix_work;
static struct workqueue_struct *goodix_wq;
static const char *s3c_ts_name = "Goodix Capacitive TouchScreen";
//static struct point_queue finger_list;
struct i2c_client *i2c_connect_client = NULL;
//EXPORT_SYMBOL(i2c_connect_client);
static struct proc_dir_entry *goodix_proc_entry;
static short  goodix_read_version(struct goodix_ts_data *ts);
//static int tpd_button(struct goodix_ts_data *ts, unsigned int x, unsigned int y, unsigned int down);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
int  gt811_downloader( struct goodix_ts_data *ts, unsigned char *data);
#endif
//used by firmware update CRC
unsigned int oldcrc32 = 0xFFFFFFFF;
unsigned int crc32_table[256];
unsigned int ulPolynomial = 0x04c11db7;
unsigned int key_back_press = 0;
unsigned int  touch_cnt = 0;

unsigned int raw_data_ready = RAW_DATA_NON_ACTIVE;

#undef DEBUG

#ifdef DEBUG
int sum = 0;
int access_count = 0;
int int_count = 0;
#endif

/*******************************************************
Function:
	Read data from the slave
	Each read operation with two i2c_msg composition, for the first message sent from the machine address,
	Article 2 reads the address used to send and retrieve data; each message sent before the start signal
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte to read Address
	buf [1] ~ buf [len]: data buffer
	len: the length of read data
return:
	Execution messages
*********************************************************/
/*Function as i2c_master_send */
static int i2c_read_bytes(struct i2c_client *client, uint8_t *buf, int len)
{
    struct i2c_msg msgs[2];
    int ret = -1;

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr = client->addr;
    msgs[0].len = 2;
    msgs[0].buf = &buf[0];
    //msgs[0].scl_rate=200000;

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr = client->addr;
    msgs[1].len = len - 2;
    msgs[1].buf = &buf[2];
    //msgs[1].scl_rate=200000;

    ret = i2c_transfer(client->adapter, msgs, 2);
    return ret;
}

/*******************************************************
Function:
	Write data to a slave
Parameters:
	client: i2c devices, including device address
	buf [0]: The first byte of the write address
	buf [1] ~ buf [len]: data buffer
	len: data length
return:
	Execution messages
*******************************************************/
/*Function as i2c_master_send */
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, int len)
{
    struct i2c_msg msg;
    int ret = -1;
    //??????
    msg.flags = !I2C_M_RD; //???
    msg.addr = client->addr;
    msg.len = len;
    msg.buf = data;
    //msg.scl_rate=200000;

    ret = i2c_transfer(client->adapter, &msg, 1);
    return ret;
}

/*******************************************************
Function:
	Send a prefix command

Parameters:
	ts: client private data structure

return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int i2c_pre_cmd(struct goodix_ts_data *ts)
{
    int ret;
    uint8_t pre_cmd_data[2] = {0};

    lidbg("come into [%s]", __func__);
    pre_cmd_data[0] = 0x0f;
    pre_cmd_data[1] = 0xff;
    ret = i2c_write_bytes(ts->client, pre_cmd_data, 2);
    //msleep(2);
    return ret;
}

/*******************************************************
Function:
	Send a suffix command

Parameters:
	ts: client private data structure

return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int i2c_end_cmd(struct goodix_ts_data *ts)
{
    int ret;
    uint8_t end_cmd_data[2] = {0};

    lidbg("come into [%s]", __func__);
    end_cmd_data[0] = 0x80;
    end_cmd_data[1] = 0x00;
    ret = i2c_write_bytes(ts->client, end_cmd_data, 2);
    //msleep(2);
    return ret;
}

/********************************************************************

*********************************************************************/
#ifdef COOR_TO_KEY
static int list_key(s32 x_value, s32 y_value, u8 *key)
{
    s32 i;

#ifdef AREA_Y
    if (y_value <= AREA_Y)
#else
    if (x_value <= AREA_X)
#endif
    {
        return 0;
    }

    for (i = 0; i < MAX_KEY_NUM; i++)
    {
        if (abs(key_center[i][x] - x_value) < KEY_X
                && abs(key_center[i][y] - y_value) < KEY_Y)
        {
            (*key) |= (0x01 << i);
        }
    }

    return 1;
}
#endif

/*******************************************************
Function:
	Guitar initialization function, used to send configuration information, access to version information
Parameters:
	ts: client private data structure
return:
	Results of the implementation code, 0 for normal execution
*******************************************************/
static int goodix_init_panel(struct goodix_ts_data *ts)
{
    short ret = -1;
    lidbg("come to goodix_init_panel=======7.8heti===========futengfei=\n");
#ifdef BOARD_V2
    uint8_t config_info7[] =
    {
        0x06, 0xA2,
        0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00, 0x01, 0x11, 0x11, 0x11, 0x21, 0x11,
        0x31, 0x11, 0x41, 0x11, 0x51, 0x11, 0x61, 0x11, 0x71, 0x11, 0x81, 0x11, 0x91, 0x11, 0xA1, 0x11,
        0xB1, 0x11, 0xC1, 0x11, 0xD1, 0x11, 0xE1, 0x11, 0xF1, 0x11, 0x07, 0x03, 0x10, 0x10, 0x10, 0x20,
        0x20, 0x20, 0x10, 0x10, 0x0A, 0x48, 0x30, 0x07, 0x03, 0x00, 0x05, 0x44, 0x02, 0x00, 0x04, 0x00,
        0x00, 0x3C, 0x35, 0x38, 0x32, 0x00, 0x00, 0x23, 0x14, 0x05, 0x0A, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x14, 0x10, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01

    };
    //#endif
#else
    uint8_t config_info7[] =
    {
        0x06, 0xA2,
        0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00, 0x01, 0x11, 0x11, 0x11, 0x21, 0x11,
        0x31, 0x11, 0x41, 0x11, 0x51, 0x11, 0x61, 0x11, 0x71, 0x11, 0x81, 0x11, 0x91, 0x11, 0xA1, 0x11,
        0xB1, 0x11, 0xC1, 0x11, 0xD1, 0x11, 0xE1, 0x11, 0xF1, 0x11, 0x07, 0x03, 0x10, 0x10, 0x10, 0x20,
        0x20, 0x20, 0x10, 0x10, 0x0A, 0x48, 0x30, 0x07, 0x03, 0x00, 0x05, 0x58, 0x02, 0x00, 0x04, 0x00,
        0x00, 0x3C, 0x35, 0x38, 0x32, 0x00, 0x00, 0x23, 0x14, 0x05, 0x0A, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x14, 0x10, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01

    };
#endif
    uint8_t config_info8[] =
    {
        0x06, 0xA2,
        0x12, 0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06, 0x04, 0x02, 0x00, 0x01, 0x11, 0x11, 0x11, 0x21, 0x11,
        0x31, 0x11, 0x41, 0x11, 0x51, 0x11, 0x61, 0x11, 0x71, 0x11, 0x81, 0x11, 0x91, 0x11, 0xA1, 0x11,
        0xB1, 0x11, 0xC1, 0x11, 0xD1, 0x11, 0xE1, 0x11, 0xF1, 0x11, 0x07, 0x03, 0x10, 0x10, 0x10, 0x20,
        0x20, 0x20, 0x10, 0x10, 0x0A, 0x48, 0x30, 0x07, 0x03, 0x00, 0x05, 0x58, 0x02, 0x00, 0x04, 0x00,
        0x00, 0x3C, 0x35, 0x38, 0x32, 0x00, 0x00, 0x23, 0x14, 0x05, 0x0A, 0x80, 0x00, 0x00, 0x00, 0x00,
        0x14, 0x10, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01

    };
    //config_info[62] = TOUCH_MAX_WIDTH >> 8;
    //config_info[61] = TOUCH_MAX_WIDTH & 0xff;
    // config_info[64] = TOUCH_MAX_HEIGHT >> 8;
    // config_info[63] = TOUCH_MAX_HEIGHT & 0xff;


    //sensor_id [0:  8cunTS;      2: 7cunTS ;   7cun for default]
    if(sensor_id == 0) //0:  8cunTS;
    {
        ret = i2c_write_bytes(ts->client, config_info8, sizeof(config_info8) / sizeof(config_info8[0]));
        ts->abs_x_max = (config_info8[62] << 8) + config_info8[61];
        ts->abs_y_max = (config_info8[64] << 8) + config_info8[63];
        ts->max_touch_num = config_info8[60];
        ts->int_trigger_type = ((config_info8[57] >> 3) & 0x01);
        lidbg("[futengfei] goodix_init_panel=================config_info8\n");

    }

    else //7cun for default
    {
        ret = i2c_write_bytes(ts->client, config_info7, sizeof(config_info7) / sizeof(config_info7[0]));
        ts->abs_x_max = (config_info7[62] << 8) + config_info7[61];
        ts->abs_y_max = (config_info7[64] << 8) + config_info7[63];
        ts->max_touch_num = config_info7[60];
        ts->int_trigger_type = ((config_info7[57] >> 3) & 0x01);
        lidbg("[futengfei] goodix_init_panel=================config_info7\n");

    }

    if(ret < 0)
    {
        lidbg( "GT811 Send config failed!\n");
        return ret;
    }

    lidbg( "GT811 init info:X_MAX=%d,Y_MAX=%d,TRIG_MODE=%s\n",	ts->abs_x_max, ts->abs_y_max, ts->int_trigger_type ? "RISING EDGE" : "FALLING EDGE");
    lidbg("leave from goodix_init_panel==================futengfei=\n");
    return 0;
}

/*******************************************************
FUNCTION:
	Read gt811 IC Version
Argument:
	ts:	client
return:
	0:success
       -1:error
*******************************************************/
static short  goodix_read_version(struct goodix_ts_data *ts)
{
    short ret;
    uint8_t version_data[5] = {0x07, 0x17, 0, 0};	//store touchscreen version infomation
    uint8_t version_data2[5] = {0x07, 0x17, 0, 0};	//store touchscreen version infomation

    char i = 0;
    char cpf = 0;
    memset(version_data, 0, 5);

    lidbg("come into [%s]", __func__);
    version_data[0] = 0x07;
    version_data[1] = 0x17;

    ret = i2c_read_bytes(ts->client, version_data, 4);
    if (ret < 0)
        return ret;

    for(i = 0; i < 10; i++)
    {
        i2c_read_bytes(ts->client, version_data2, 4);
        if((version_data[2] != version_data2[2]) || (version_data[3] != version_data2[3]))
        {
            version_data[2] = version_data2[2];
            version_data[3] = version_data2[3];
            msleep(5);
            break;
        }
        msleep(5);
        cpf++;
    }

    if(cpf == 10)
    {
        ts->version = (version_data[2] << 8) + version_data[3];
        lidbg( "GT811 Verion:0x%04x\n", ts->version);
        ret = 0;
    }
    else
    {
        lidbg(" Guitar Version Read Error: %d.%d\n", version_data[3], version_data[2]);
        ts->version = 0xffff;
        ret = -1;
    }

    return ret;

}
/******************start add by kuuga*******************/
static void gt811_irq_enable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;

    lidbg("come into [%s]", __func__);
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable)
    {
        enable_irq(ts->irq);
        ts->irq_is_disable = 0;
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

static void gt811_irq_disable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;

    lidbg("come into [%s]", __func__);
    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        disable_irq_nosync(ts->irq);
        ts->irq_is_disable = 1;
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*****************end add by kuuga****************/

/*******************************************************
Function:
	Touch-screen work function
	Triggered by the interruption, to accept a set of coordinate data,
	and then analyze the output parity
Parameters:
	ts: client private data structure
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    uint8_t  point_data[READ_BYTES_NUM] = {READ_TOUCH_ADDR_H, READ_TOUCH_ADDR_L, 0}; //point_data[8*MAX_FINGER_NUM+2]={ 0 };
    uint8_t  check_sum = 0;
    uint8_t  read_position = 0;
    uint8_t  track_id[MAX_FINGER_NUM];
    uint8_t  point_index = 0;
    uint8_t  point_tmp = 0;
    uint8_t  point_count = 0;
    uint8_t  input_w = 0;
    uint8_t  finger = 0;
    uint8_t  key = 0;
    uint16_t input_x = 0;
    uint16_t input_y = 0;
    static uint8_t  last_key = 0;
    unsigned int  count = 0;
    unsigned int position = 0;
    int finger_up_cunt = 0;
    int ret = -1;
    int tmp = 0;

    struct goodix_ts_data *ts = container_of(work, struct goodix_ts_data, work);
    SOC_IO_ISR_Disable(GPIOEIT);
    //lidbg("\n4:come into %s=========================futengfei====\n",__func__);
    //return 0;
#ifdef DEBUG
    lidbg("int count :%d\n", ++int_count);
    lidbg("ready?:%d\n", raw_data_ready);
#endif
    if (RAW_DATA_ACTIVE == raw_data_ready)
    {
        raw_data_ready = RAW_DATA_READY;
#ifdef DEBUG
        lidbg("ready!\n");
#endif
    }

#ifndef INT_PORT
#endif
COORDINATE_POLL:

    if( tmp > 9)
    {
        lidbg( "Because of transfer error,touchscreen stop working.\n");
        goto XFER_ERROR ;
    }

    ret = i2c_read_bytes(ts->client, point_data, sizeof(point_data) / sizeof(point_data[0]));
    if(ret <= 0)
    {
        lidbg("I2C transfer error. Number:%d\n ", ret);
        ts->bad_data = 1;
        tmp ++;
        ts->retry++;

#ifndef INT_PORT
        {
            lidbg("goto COORDINATE_POL[%d]times\n", tmp);
            goto COORDINATE_POLL;
        }
#else
        {
            lidbg("goto XFER_ERROR[%d]times\n", tmp);
            goto XFER_ERROR;
        }
#endif

    }
#if 0
    for(count = 0; count < (sizeof(point_data) / sizeof(point_data[0])); count++)
    {
        lidbg("[%2d]:0x%2x", count, point_data[count]);
        if((count + 1) % 10 == 0)lidbg("\n");
    }
    lidbg("\n");
#endif

    if(0)//if(have_load==0) //now disable
    {
        sensor_id = point_data[2] >> 6; //0:  8cunTS;      2: 7cunTS ;   7cun for default
        if (sensor_id == 0)
        {
            goodix_init_panel(ts);
            have_load = 1;
        }
    }

    if(point_data[2] & 0x20)
    {
        if(point_data[3] == 0xF0)
        {
            //gpio_direction_output(SHUTDOWN_PORT, 0);
            msleep(1);
            //gpio_direction_input(SHUTDOWN_PORT);
            lidbg("word:goodix_init_panel again=======futengfei===\n");
            goodix_init_panel(ts);
            goto WORK_FUNC_END;
        }
    }


    switch(point_data[2] & 0x1f)
    {
    case 0:
        read_position = 3;
        break;
    case 1:
        for(count = 2; count < 9; count++)
            check_sum += (int)point_data[count];
        read_position = 9;
        break;
    case 2:
    case 3:
        for(count = 2; count < 14; count++)
            check_sum += (int)point_data[count];
        read_position = 14;
        break;
    default:		//touch finger larger than 3
        for(count = 2; count < 35; count++)
            check_sum += (int)point_data[count];
        read_position = 35;
    }
    if(check_sum != point_data[read_position])
    {
        //lidbg( "coor chksum error!===goto end==========futengfei==\n");
        goto XFER_ERROR;
    }

    point_index = point_data[2] & 0x1f;
    point_tmp = point_index;
    for(position = 0; (position < MAX_FINGER_NUM) && point_tmp; position++)
    {
        if(point_tmp & 0x01)
        {
            track_id[point_count++] = position;
        }
        point_tmp >>= 1;
    }


    finger = point_count;
    if (finger == 4)
    {
        //lidbg("SOC_Log_Dump\n");
        //SOC_Log_Dump(LOG_DMESG);
    }
    if (finger > 5)
    {
        lidbg("finger>5\n");
    }
    //lidbg("finger=[%d]\n",finger);
    //return 0;
    finger_up_cunt = 5 - finger;
    if(finger)
    {
        for(count = 0; count < finger; count++)
        {
            if(track_id[count] != 3)
            {
                if(track_id[count] < 3)
                    position = 4 + track_id[count] * 5;
                else
                    position = 30;
                input_x = (uint16_t)(point_data[position] << 8) + (uint16_t)point_data[position + 1];
                input_y = (uint16_t)(point_data[position + 2] << 8) + (uint16_t)point_data[position + 3];
                //input_w = point_data[position+4];
            }
            else
            {
                input_x = (uint16_t)(point_data[19] << 8) + (uint16_t)point_data[26];
                input_y = (uint16_t)(point_data[27] << 8) + (uint16_t)point_data[28];
                //input_w = point_data[29];
            }

            if((input_x > ts->abs_x_max) || (input_y > ts->abs_y_max))continue;
            if((xy_revert_en == 1) || (1 == ts_should_revert))
            {
                input_y = SCREEN_X - input_y;
                input_x = SCREEN_Y - input_x;
            }
#ifdef BOARD_V2
            input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_y);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_x);
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
            input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, track_id[count]);
            input_mt_sync(ts->input_dev);

#else
            input_mt_slot(ts->input_dev, count);
            input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_y);
            input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_x);
            input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
#endif

        }

#ifndef BOARD_V2
        if(finger_up_cunt > 0)
        {
            int cunt;
            int rept = 4;
            for(cunt = 0; cunt < finger_up_cunt; cunt++, rept--)
            {
                input_mt_slot(ts->input_dev, rept);
                input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
            }
        }
        input_report_key(ts->input_dev, BTN_TOUCH, 1);
        input_sync(ts->input_dev);
#endif
        g_var.flag_for_15s_off++;
        if(g_var.flag_for_15s_off >= 1000)
        {
            g_var.flag_for_15s_off = 1000;
        }
        if(g_var.flag_for_15s_off < 0)
        {
            lidbg("\nerr:FLAG_FOR_15S_OFF===[%d]\n", g_var.flag_for_15s_off);
        }

        g_curr_tspara.x = input_y;
        g_curr_tspara.y = input_x;
        g_curr_tspara.press = true;

        if(1 == recovery_mode)
        {
            if( (input_y >= 0) && (input_x >= 0) )
            {
                touch.x = input_y;
                touch.y = input_x;
                touch.pressed = 1;
                set_touch_pos(&touch);
            }
        }

    }
    else
    {
#ifdef BOARD_V2
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
        input_mt_sync(ts->input_dev);

#else
        {
            int count = 0;
            for(count = 0; count < 5; count++)
            {
                input_mt_slot(ts->input_dev, count);
                input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
                input_report_key(ts->input_dev, BTN_TOUCH, 0);
                input_sync(ts->input_dev);
            }
        }

#endif

        if(1 == recovery_mode)
        {
            {
                touch.pressed = 0;
                set_touch_pos(&touch);
            }
        }
        g_curr_tspara.press = false;

    }
#ifdef BOARD_V2
    input_report_key(ts->input_dev, BTN_TOUCH, finger > 0);
    input_sync(ts->input_dev);
#endif
    touch_cnt++;
    if (touch_cnt == 90)
    {
        touch_cnt = 0;
        lidbg("QR%d,%d[%d,%d]\n", xy_revert_en, sensor_id, input_y, input_x);
    }

#ifdef HAVE_TOUCH_KEY_REPORT
    if(finger == 1 && key_back_press == 0 && input_y > 784 && input_x < 13)
    {
        input_report_key(ts->input_dev, KEY_BACK, KEY_PRESS);
        input_sync(ts->input_dev);
        key_back_press = 1;
    }
    if(finger == 0)
    {
        input_report_key(ts->input_dev, KEY_BACK, KEY_RELEASE);
        input_sync(ts->input_dev);
        key_back_press = 0;
    }
#endif


    if(!g_var.is_fly)
    {
        static int key = 0;
        if(finger == 3)
            key = KEY_BACK;
        else if(finger == 5)
            key = KEY_MENU;
        if(finger == 0)
        {
            if(key)
            {
                SOC_Key_Report(key, KEY_PRESSED_RELEASED);
                key = 0;
            }
        }
    }


    //#ifdef HAVE_TOUCH_KEY
#if 0
    key = point_data[3] & 0x0F;
    if((last_key != 0) || (key != 0))
    {
        for(count = 0; count < MAX_KEY_NUM; count++)
        {
            input_report_key(ts->input_dev, touch_key_array[count], !!(key & (0x01 << count)));
        }
    }
    last_key = key;
#endif

XFER_ERROR:
WORK_FUNC_END:
    SOC_IO_ISR_Enable(GPIOEIT);
#ifndef STOP_IRQ_TYPE
    //if(ts->use_irq)
    //	gt811_irq_enable(ts);     //KT ADD 1202
#endif
}

/*******************************************************
Function:
	Response function timer
	Triggered by a timer, scheduling the work function of the touch screen operation; after re-timing
Parameters:
	timer: the timer function is associated
return:
	Timer mode, HRTIMER_NORESTART that do not automatically restart
********************************************************/
static enum hrtimer_restart goodix_ts_timer_func(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);

    lidbg("come into [%s]", __func__);
    //queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (POLL_TIME + 6) * 1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
	Interrupt response function
	Triggered by an interrupt, the scheduler runs the touch screen handler
********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;
    // lidbg("\n3:come into %s============================futengfei====\n",__func__);
    //lidbg("-------------------ts_irq_handler------------------\n");
    //#ifndef STOP_IRQ_TYPE
    //lidbg("../n");
#if 0
    gt811_irq_disable(ts);     //KT ADD 1202
#endif
    //disable_irq_nosync(ts->client->irq);
    queue_work(goodix_wq, &ts->work);
    //	schedule_work(&goodix_work);
    return IRQ_HANDLED;
}

/*******************************************************
Function:
	Power management gt811, gt811 allowed to sleep or to wake up
Parameters:
	on: 0 that enable sleep, wake up 1
return:
	Is set successfully, 0 for success
	Error code: -1 for the i2c error, -2 for the GPIO error;-EINVAL on error as a parameter
********************************************************/
static int goodix_ts_power(struct goodix_ts_data *ts, int on)
{
    int ret = -1;

    unsigned char i2c_control_buf[3] = {0x06, 0x92, 0x01};		//suspend cmd
    lidbg("come into [%s]", __func__);
#ifdef INT_PORT
    if(ts != NULL && !ts->use_irq)
        return -2;
#endif
    switch(on)
    {
    case 0:
        ret = i2c_write_bytes(ts->client, i2c_control_buf, 3);
        lidbg( "Send suspend cmd\n");
        if(ret > 0)						//failed
            ret = 0;
        return ret;

    case 1:
        //#ifdef INT_PORT 					 //suggest use INT PORT to wake up !!!
#if 0					 //suggest use INT PORT to wake up !!!
        gpio_direction_output(INT_PORT, 0);
        msleep(1);
        gpio_direction_output(INT_PORT, 1);
        msleep(1);
        gpio_direction_output(INT_PORT, 0);
        gpio_free(INT_PORT);
        s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE);
        if(ts->use_irq)
            s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port as interrupt port
        else
            gpio_direction_input(INT_PORT);

#else
        //gpio_direction_output(SHUTDOWN_PORT,0);
        msleep(1);
        //gpio_direction_input(SHUTDOWN_PORT);
#endif
        msleep(40);
        ret = 0;
        return ret;

    default:
        lidbg( "%s: Cant't support this command.", s3c_ts_name);
        return -EINVAL;
    }

}
/*******************************************************
Function:
	Touch-screen detection function
	Called when the registration drive (required for a corresponding client);
	For IO, interrupts and other resources to apply; equipment registration; touch screen initialization, etc.
Parameters:
	client: the device structure to be driven
	id: device ID
return:
	Results of the implementation code, 0 for normal execution
********************************************************/

static int screen_x = 0;
static int screen_y = 0;
static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    int retry = 0;
    char test_data = 1;
    const char irq_table[2] = {IRQ_TYPE_EDGE_FALLING, IRQ_TYPE_EDGE_RISING};
    struct goodix_ts_data *ts;
    struct goodix_i2c_rmi_platform_data *pdata;

    lidbg("2:come into %s====================futengfei=====\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        lidbg( "Must have I2C_FUNC_I2C.\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    i2c_connect_client = client;
    /*
    	ret = gpio_request(SHUTDOWN_PORT, "RESET_INT");
    	if (ret < 0)
            {
    		lidbg( "Failed to request RESET GPIO:%d, ERRNO:%d\n",(int)SHUTDOWN_PORT,ret);
    		goto err_gpio_request;
    	}
    	s3c_gpio_setpull(SHUTDOWN_PORT, S3C_GPIO_PULL_UP);		//set GPIO pull-up

    	for(retry=0;retry >= 0; retry++)
    	{
    		gpio_direction_output(SHUTDOWN_PORT,0);
    		msleep(1);
    		gpio_direction_input(SHUTDOWN_PORT);
    		msleep(100);

    		ret =i2c_write_bytes(client, &test_data, 1);	//Test I2C connection.
    		if (ret > 0)
    			break;
    		lidbg( "GT811 I2C TEST FAILED!Please check the HARDWARE connect\n");
    	}

    	if(ret <= 0)
    	{
    		lidbg( "Warnning: I2C communication might be ERROR!\n");
    		goto err_i2c_failed;
    	}
     */
    INIT_WORK(&ts->work, goodix_ts_work_func);		//init work_struct
    ts->client = client;
    ts->client->addr = 0x5d;
    i2c_set_clientdata(client, ts);
    pdata = client->dev.platform_data;

    /////////////////////////////// UPDATE STEP 1 START/////////////////////////////////////////////////////////////////
    //#ifdef AUTO_UPDATE_GT811		//modify by andrew
#if 0		//modify by andrew
    msleep(20);
    goodix_read_version(ts);

    ret = gt811_downloader( ts, goodix_gt811_firmware);
    if(ret < 0)
    {
        lidbg( "Warnning: gt811 update might be ERROR!\n");
        //goto err_input_dev_alloc_failed;
    }
#endif
    ///////////////////////////////UPDATE STEP 1 END////////////////////////////////////////////////////////////////
    //#ifdef INT_PORT
#if 0
    client->irq = TS_INT;		//If not defined in client
    if (client->irq)
    {
        ret = gpio_request(INT_PORT, "TS_INT");	//Request IO
        if (ret < 0)
        {
            lidbg( "Failed to request GPIO:%d, ERRNO:%d\n", (int)INT_PORT, ret);
            goto err_gpio_request_failed;
        }
        s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE);	//ret > 0 ?
        s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port function


#ifndef STOP_IRQ_TYPE
        ts->irq = TS_INT;     //KT ADD 1202
        ts->irq_is_disable = 0;           // enable irq
#endif
    }
#endif
#if 1
err_gpio_request_failed:
    //lidbg("goodix_init_panel:come to send init_panel==============futengfei=\n");
    for(retry = 0; retry < 5; retry++)
    {
        ret = goodix_init_panel(ts);
        msleep(2);
        if(ret != 0)	//Initiall failed
        {
            lidbg("[futengfei]goodix_ts_probe.goodix_init_panel fail and again----------->GT811the %d times\n", retry);
#ifdef BOARD_V2
            SOC_IO_Output(0, 27, 1);
            msleep(300);
            SOC_IO_Output(0, 27, 0);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
            msleep(700);
#else
            SOC_IO_Output(0, 27, 0);
            msleep(300);
            SOC_IO_Output(0, 27, 1);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
            msleep(700);
#endif

            continue;
        }

        else
            break;
    }
    if(ret != 0)
    {
        lidbg("goodix_init_panel:============Initiall failed");
        ts->bad_data = 1;
        //goto err_init_godix_ts;
    }
#endif

    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        ret = -ENOMEM;
        lidbg("goodix_ts_probe: Failed to allocate input device=======futengfei======\n");
        goto err_input_dev_alloc_failed;
    }
    screen_x = SCREEN_X;
    screen_y = SCREEN_Y;
    SOC_Display_Get_Res(&screen_x, &screen_y);
#ifdef BOARD_V2
    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
#ifdef HAVE_TOUCH_KEY
    //#if 1
    for(retry = 0; retry < MAX_KEY_NUM; retry++)
    {
        input_set_capability(ts->input_dev, EV_KEY, touch_key_array[retry]);
    }
#endif

    input_set_abs_params(ts->input_dev, ABS_X, 0,  ts->abs_x_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->abs_y_max, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 255, 0, 0);
#ifdef GOODIX_MULTI_TOUCH
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, screen_x  , 0, 0); //ts->abs_y_max
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, screen_y , 0, 0);	//ts->abs_x_max
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

    lidbg("check your screen [%d*%d]=================futengfei===\n", screen_x, screen_y);

    sprintf(ts->phys, "input/ts");
    ts->input_dev->name = s3c_ts_name;
    ts->input_dev->phys = ts->phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;	//screen firmware version

    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        lidbg("Probe: Unable to register %s input device=======futengfei====\n", ts->input_dev->name);
        goto err_input_register_device_failed;
    }
    ts->bad_data = 0;

    //#ifdef INT_PORT
#if 0
    ret  = request_irq(client->irq, goodix_ts_irq_handler ,  irq_table[ts->int_trigger_type],
                       client->name, ts);
    if (ret != 0)
    {
        lidbg("Cannot allocate ts INT!ERRNO:%d\n", ret);
        gpio_direction_input(INT_PORT);
        gpio_free(INT_PORT);
        goto err_init_godix_ts;
    }
    else
    {
#ifndef STOP_IRQ_TYPE
        gt811_irq_disable(ts);     //KT ADD 1202
#elif
        disable_irq(client->irq);
#endif
        ts->use_irq = 1;
        lidbg("Reques EIRQ %d succesd on GPIO:%d\n", TS_INT, INT_PORT);
    }
#endif

#if 0
    if (!ts->use_irq)
    {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = goodix_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

    if(ts->use_irq)
#ifndef STOP_IRQ_TYPE
        gt811_irq_enable(ts);     //KT ADD 1202
#elif
        enable_irq(client->irq);
#endif

    ts->power = goodix_ts_power;

    goodix_read_version(ts);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 20;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif
    //fake suspend
    //SOC_Fake_Register_Early_Suspend(&ts->early_suspend);

    /////////////////////////////// UPDATE STEP 2 START /////////////////////////////////////////////////////////////////
    //#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
#if 0
    goodix_proc_entry = create_proc_entry("goodix-update", 0666, NULL);
    if(goodix_proc_entry == NULL)
    {
        lidbg( "Couldn't create proc entry!\n");
        ret = -ENOMEM;
        goto err_create_proc_entry;
    }
    else
    {
        lidbg( "Create proc entry success!\n");
        goodix_proc_entry->write_proc = goodix_update_write;
        goodix_proc_entry->read_proc = goodix_update_read;
    }
#endif
    ///////////////////////////////UPDATE STEP 2 END /////////////////////////////////////////////////////////////////
    //	lidbg("Start %s in %s mode,Driver Modify Date:2012-01-05\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

    //SOC_IO_ISR_Disable(GPIOEIT);
    SOC_IO_Input(0, GPIOEIT, GPIO_CFG_PULL_UP);

    ret = SOC_IO_ISR_Add(GPIOEIT, IRQF_TRIGGER_FALLING, goodix_ts_irq_handler, ts);
    if(ret == 0)
    {
        lidbg("[futengfei]gt811:------->SOC_IO_ISR_Add err\n");
    }
    ts->use_irq = 1;
    ts->client->irq = GPIOEIT;
    //SOC_IO_ISR_Enable(GPIOEIT);
    lidbg("=OUT==============touch INFO==================%s\n\n", __func__);
    return 0;

err_init_godix_ts:
    i2c_end_cmd(ts);
    if(ts->use_irq)
    {
        ts->use_irq = 0;
        free_irq(client->irq, ts);
#ifdef INT_PORT
        //gpio_direction_input(INT_PORT);
        //gpio_free(INT_PORT);
#endif
    }
    else
        hrtimer_cancel(&ts->timer);

err_input_register_device_failed:
    input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
    i2c_set_clientdata(client, NULL);
err_gpio_request:
err_i2c_failed:
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
err_create_proc_entry:

    lidbg("\nerr_init_godix_ts==================futengfei=========\n");
    return ret;
}


/*******************************************************
Function:
	Drive the release of resources
Parameters:
	client: the device structure
return:
	Results of the implementation code, 0 for normal execution
********************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    lidbg("come into [%s]", __func__);


#ifdef CONFIG_HAS_EARLYSUSPEND
    //unregister_early_suspend(&ts->early_suspend);
#endif
    /////////////////////////////// UPDATE STEP 3 START/////////////////////////////////////////////////////////////////
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
    remove_proc_entry("goodix-update", NULL);
#endif
    /////////////////////////////////UPDATE STEP 3 END///////////////////////////////////////////////////////////////

    if (ts && ts->use_irq)
    {
#ifdef INT_PORT
        //gpio_direction_input(INT_PORT);
        //gpio_free(INT_PORT);
#endif
        free_irq(client->irq, ts);
    }
    else if(ts)
        hrtimer_cancel(&ts->timer);

    dev_notice(&client->dev, "The driver is removing...\n");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);
    return 0;
}

//????
static int goodix_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret;
    struct goodix_ts_data *ts = i2c_get_clientdata(client);

    lidbg(" [%s]========futengfei=======\n\n\n", __func__);
    /*
    	if (ts->use_irq)
    		//disable_irq(client->irq);
    #ifndef STOP_IRQ_TYPE
    		gt811_irq_disable(ts);     //KT ADD 1202
    #elif
    		disable_irq(client->irq);
    #endif

    	else
    		hrtimer_cancel(&ts->timer);
    #ifndef STOP_IRQ_TYPE
    		cancel_work_sync(&ts->work);
    #endif
    	//ret = cancel_work_sync(&ts->work);
    	//if(ret && ts->use_irq)
    		//enable_irq(client->irq);

    	if (ts->power) {
    		ret = ts->power(ts, 0);
    		if (ret < 0)
    			lidbg("goodix_ts_resume power off failed\n");
    	}
    */

    return 0;
}

static int goodix_ts_resume(struct i2c_client *client)
{
    int ret = 0, retry = 0, init_err = 0;
    uint8_t GT811_check[6] = {0x55};
    struct goodix_ts_data *ts = i2c_get_clientdata(client);
    lidbg("come into [%s]========futengfei===fukesi===0829forGT811 RESUME RESET [futengfei]=\n", __func__);
    lidbg(KERN_INFO "Build Time: %s %s  %s \n", __FUNCTION__, __DATE__, __TIME__);

    for(retry = 0; retry < 5; retry++)
    {
        goodix_init_panel(ts);
        init_err = SOC_I2C_Rec(1, 0x5d, 0x68, GT811_check, 6 );
        ret = 0;
        //if( GT811_check[0] == 0xff&&GT811_check[1] == 0xff&&GT811_check[2] == 0xff&&GT811_check[3] == 0xff&&GT811_check[4] == 0xff&&GT811_check[5] == 0xff)
        if(init_err < 0)
        {
            lidbg("[futengfei]goodix_init_panel:goodix_init_panel failed====retry=[%d]\n", retry);
            ret = 1;
        }
        else
        {
            lidbg("[futengfei]goodix_init_panel:goodix_init_panel success====retry=[%d]\n\n\n", retry);
            ret = 0;
        }

        msleep(8);
        if(ret != 0)	//Initiall failed
        {
            lidbg("[futengfei]goodix_init_panel:goodix_init_panel failed=========retry=[%d]===ret[%d]\n", retry, ret);
#ifdef BOARD_V2
            SOC_IO_Output(0, 27, 1);
            msleep(300);
            SOC_IO_Output(0, 27, 0);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
            msleep(700);
#else
            SOC_IO_Output(0, 27, 0);
            msleep(300);
            SOC_IO_Output(0, 27, 1);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
            msleep(700);
#endif
            continue;
        }

        else
            break;

        lidbg("[futengfei] goodix_ts_resume:if this is appear ,that is say the continue no goto for directly!\n");

    }

    if(ret != 0)
    {
        lidbg("goodix_init_panel:Initiall failed============");
        ts->bad_data = 1;
        //goto err_init_godix_ts;
    }

    /*
    	if (ts->power) {
    		ret = ts->power(ts, 1);
    		if (ret < 0)
    			lidbg("goodix_ts_resume power on failed\n");
    	}

    	if (ts->use_irq)
    		//enable_irq(client->irq);
    #ifndef STOP_IRQ_TYPE
    		gt811_irq_enable(ts);     //KT ADD 1202
    #elif
    		enable_irq(client->irq);
    #endif
    	else
    		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    */
    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    ts = container_of(h, struct goodix_ts_data, early_suspend);

    lidbg("\n\n\n[futengfei]come into===1024580=======disable_irq20=== [%s]\n", __func__);
    disable_irq(MSM_GPIO_TO_INT(GPIOEIT));

    goodix_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    lidbg("\n\n\n[futengfei]come into===1024580========enable_irq20== [%s]\n", __func__);

    ts = container_of(h, struct goodix_ts_data, early_suspend);
    goodix_ts_resume(ts->client);
    enable_irq(MSM_GPIO_TO_INT(GPIOEIT));

}
#endif
/////////////////////////////// UPDATE STEP 4 START/////////////////////////////////////////////////////////////////
//******************************Begin of firmware update surpport*******************************
#ifdef CONFIG_TOUCHSCREEN_GOODIX_IAP
static struct file *update_file_open(char *path, mm_segment_t *old_fs_p)
{
    struct file *filp = NULL;
    int errno = -1;

    filp = filp_open(path, O_RDONLY, 0644);

    if(!filp || IS_ERR(filp))
    {
        if(!filp)
            errno = -ENOENT;
        else
            errno = PTR_ERR(filp);
        lidbg("The update file for Guitar open error.\n");
        return NULL;
    }
    *old_fs_p = get_fs();
    set_fs(get_ds());

    filp->f_op->llseek(filp, 0, 0);
    return filp ;
}

static void update_file_close(struct file *filp, mm_segment_t old_fs)
{
    set_fs(old_fs);
    if(filp)
        filp_close(filp, NULL);
}
static int update_get_flen(char *path)
{
    struct file *file_ck = NULL;
    mm_segment_t old_fs;
    int length ;

    file_ck = update_file_open(path, &old_fs);
    if(file_ck == NULL)
        return 0;

    length = file_ck->f_op->llseek(file_ck, 0, SEEK_END);
    //lidbg("File length: %d\n", length);
    if(length < 0)
        length = 0;
    update_file_close(file_ck, old_fs);
    return length;
}

static int goodix_update_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    unsigned char cmd[120];
    int ret = -1;
    int retry = 0;
    static unsigned char update_path[60];
    struct goodix_ts_data *ts;
    struct file *file_data = NULL;
    mm_segment_t old_fs;
    unsigned char *file_ptr = NULL;
    unsigned int file_len;
    lidbg("come into [%s]", __func__);
    ts = i2c_get_clientdata(i2c_connect_client);
    if(ts == NULL)
    {
        lidbg("goodix write to kernel via proc file!@@@@@@\n");
        return 0;
    }

    //lidbg("goodix write to kernel via proc file!@@@@@@\n");
    if(copy_from_user(&cmd, buff, len))
    {
        lidbg("goodix write to kernel via proc file!@@@@@@\n");
        return -EFAULT;
    }
    //lidbg("Write cmd is:%d,write len is:%ld\n",cmd[0], len);
    switch(cmd[0])
    {
    case APK_UPDATE_TP:
        lidbg("Write cmd is:%d,cmd arg is:%s,write len is:%ld\n", cmd[0], &cmd[1], len);
        memset(update_path, 0, 60);
        strncpy(update_path, cmd + 1, 60);

        //#ifndef STOP_IRQ_TYPE
        //		gt811_irq_disable(ts);     //KT ADD 1202
        //#elif
        disable_irq(ts->client->irq);
        //#endif
        file_data = update_file_open(update_path, &old_fs);
        if(file_data == NULL)   //file_data has been opened at the last time
        {
            lidbg( "cannot open update file\n");
            return 0;
        }

        file_len = update_get_flen(update_path);
        lidbg( "Update file length:%d\n", file_len);
        file_ptr = (unsigned char *)vmalloc(file_len);
        if(file_ptr == NULL)
        {
            lidbg( "cannot malloc memory!\n");
            return 0;
        }

        ret = file_data->f_op->read(file_data, file_ptr, file_len, &file_data->f_pos);
        if(ret <= 0)
        {
            lidbg( "read file data failed\n");
            return 0;
        }
        update_file_close(file_data, old_fs);

        ret = gt811_downloader(ts, file_ptr);
        vfree(file_ptr);
        if(ret < 0)
        {
            lidbg("Warnning: GT811 update might be ERROR!\n");
            return 0;
        }

        //       i2c_pre_cmd(ts);

        //gpio_direction_output(SHUTDOWN_PORT, 0);
        msleep(5);
        //gpio_direction_input(SHUTDOWN_PORT);
        msleep(20);
        for(retry = 0; retry < 3; retry++)
        {
            ret = goodix_init_panel(ts);
            msleep(2);
            if(ret != 0)	//Initiall failed
            {
                lidbg( "Init panel failed!\n");
                continue;
            }
            else
                break;

        }
        //gpio_free(INT_PORT);
        //ret = gpio_request(INT_PORT, "TS_INT"); //Request IO
        //     if (ret < 0)
        //    {
        //          dev_err(&ts->client->dev, "Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
        //      return 0;
        //     }
        //s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE); //ret > 0 ?
        //s3c_gpio_cfgpin(INT_PORT, INT_CFG);     //Set IO port function
        //gpio_direction_input(INT_PORT);
        //	s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_UP);
        //        s3c_gpio_cfgpin(INT_PORT, INT_CFG);	//Set IO port as interrupt port
        //s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE);
        //	while(1);
        //#ifndef STOP_IRQ_TYPE
        //	gt811_irq_enable(ts);     //KT ADD 1202
        //#elif
        enable_irq(ts->client->irq);
        //#endif
        //        i2c_end_cmd(ts);
        return 1;

    case APK_READ_FUN:							//functional command
        if(cmd[1] == CMD_READ_VER)
        {
            lidbg("Read version!\n");
            ts->read_mode = MODE_RD_VER;
        }
        else if(cmd[1] == CMD_READ_CFG)
        {
            lidbg("Read config info!\n");

            ts->read_mode = MODE_RD_CFG;
        }
        else if (cmd[1] == CMD_READ_RAW)
        {
            lidbg("Read raw data!\n");

            ts->read_mode = MODE_RD_RAW;
        }
        else if (cmd[1] == CMD_READ_CHIP_TYPE)
        {
            lidbg("Read chip type!\n");

            ts->read_mode = MODE_RD_CHIP_TYPE;
        }
        return 1;

    case APK_WRITE_CFG:
        lidbg("Begin write config info!Config length:%d\n", cmd[1]);
        i2c_pre_cmd(ts);
        ret = i2c_write_bytes(ts->client, cmd + 2, cmd[1] + 2);
        i2c_end_cmd(ts);
        if(ret != 1)
        {
            lidbg("Write Config failed!return:%d\n", ret);
            return -1;
        }
        return 1;

    default:
        return 0;
    }
    return 0;
}

static int goodix_update_read( char *page, char **start, off_t off, int count, int *eof, void *data )
{
    int ret = -1, i = 0;
    int len = 0;
    int read_times = 0;
    struct goodix_ts_data *ts;
    lidbg("come into [%s]", __func__);
    unsigned char read_data[360] = {80, };

    ts = i2c_get_clientdata(i2c_connect_client);
    if(ts == NULL)
        return 0;

    lidbg("___READ__\n");
    if(ts->read_mode == MODE_RD_VER)		//read version data
    {
        i2c_pre_cmd(ts);
        ret = goodix_read_version(ts);
        i2c_end_cmd(ts);
        if(ret < 0)
        {
            lidbg("Read version data failed!\n");
            return 0;
        }

        read_data[1] = (char)(ts->version & 0xff);
        read_data[0] = (char)((ts->version >> 8) & 0xff);

        memcpy(page, read_data, 2);
        //*eof = 1;
        return 2;
    }
    else if (ts->read_mode == MODE_RD_CHIP_TYPE)
    {
        page[0] = GT811;
        return 1;
    }
    else if(ts->read_mode == MODE_RD_CFG)
    {

        read_data[0] = 0x06;
        read_data[1] = 0xa2;       // cfg start address
        lidbg("read config addr is:%x,%x\n", read_data[0], read_data[1]);

        len = 106;
        i2c_pre_cmd(ts);
        ret = i2c_read_bytes(ts->client, read_data, len + 2);
        i2c_end_cmd(ts);
        if(ret <= 0)
        {
            lidbg("Read config info failed!\n");
            return 0;
        }

        memcpy(page, read_data + 2, len);
        return len;
    }
    else if (ts->read_mode == MODE_RD_RAW)
    {
#define TIMEOUT (-100)
        int retry = 0;
        if (raw_data_ready != RAW_DATA_READY)
        {
            raw_data_ready = RAW_DATA_ACTIVE;
        }

RETRY:
        read_data[0] = 0x07;
        read_data[1] = 0x11;
        read_data[2] = 0x01;

        ret = i2c_write_bytes(ts->client, read_data, 3);

#ifdef DEBUG
        sum += read_times;
        lidbg("count :%d\n", ++access_count);
        lidbg("A total of try times:%d\n", sum);
#endif

        read_times = 0;
        while (RAW_DATA_READY != raw_data_ready)
        {
            msleep(4);

            if (read_times++ > 10)
            {
                if (retry++ > 5)
                {
                    return TIMEOUT;
                }
                goto RETRY;
            }
        }
#ifdef DEBUG
        lidbg("read times:%d\n", read_times);
#endif
        read_data[0] = 0x08;
        read_data[1] = 0x80;       // raw data address

        len = 160;

        // msleep(4);

        i2c_pre_cmd(ts);
        ret = i2c_read_bytes(ts->client, read_data, len + 2);
        //      i2c_end_cmd(ts);

        if(ret <= 0)
        {
            lidbg("Read raw data failed!\n");
            return 0;
        }
        memcpy(page, read_data + 2, len);

        read_data[0] = 0x09;
        read_data[1] = 0xC0;
        //	i2c_pre_cmd(ts);
        ret = i2c_read_bytes(ts->client, read_data, len + 2);
        i2c_end_cmd(ts);

        if(ret <= 0)
        {
            lidbg("Read raw data failed!\n");
            return 0;
        }
        memcpy(&page[160], read_data + 2, len);

#ifdef DEBUG
        //**************
        for (i = 0; i < 300; i++)
        {
            lidbg("%6x", page[i]);

            if ((i + 1) % 10 == 0)
            {
                lidbg("\n");
            }
        }
        //********************/
#endif
        raw_data_ready = RAW_DATA_NON_ACTIVE;

        return (2 * len);

    }
    return 0;
#endif
}
//********************************************************************************************
static u8  is_equal( u8 *src , u8 *dst , int len )
{
    int i;

#if 0
    for( i = 0 ; i < len ; i++ )
    {
        lidbg("[%02X:%02X]", src[i], dst[i]);
        if((i + 1) % 10 == 0)lidbg("\n");
    }
#endif

    for( i = 0 ; i < len ; i++ )
    {
        if ( src[i] != dst[i] )
        {
            return 0;
        }
    }

    return 1;
}

static  u8 gt811_nvram_store( struct goodix_ts_data *ts )
{
    int ret;
    int i;
    u8 inbuf[3] = {REG_NVRCS_H, REG_NVRCS_L, 0};
    //u8 outbuf[3] = {};

    lidbg("come into [%s]", __func__);
    ret = i2c_read_bytes( ts->client, inbuf, 3 );

    if ( ret < 0 )
    {
        return 0;
    }

    if ( ( inbuf[2] & BIT_NVRAM_LOCK ) == BIT_NVRAM_LOCK )
    {
        return 0;
    }

    inbuf[2] = (1 << BIT_NVRAM_STROE);		//store command

    for ( i = 0 ; i < 300 ; i++ )
    {
        ret = i2c_write_bytes( ts->client, inbuf, 3 );

        if ( ret < 0 )
            break;
    }

    return ret;
}

static u8  gt811_nvram_recall( struct goodix_ts_data *ts )
{
    int ret;
    u8 inbuf[3] = {REG_NVRCS_H, REG_NVRCS_L, 0};

    lidbg("come into [%s]", __func__);
    ret = i2c_read_bytes( ts->client, inbuf, 3 );

    if ( ret < 0 )
    {
        return 0;
    }

    if ( ( inbuf[2]&BIT_NVRAM_LOCK) == BIT_NVRAM_LOCK )
    {
        return 0;
    }

    inbuf[2] = ( 1 << BIT_NVRAM_RECALL );		//recall command
    ret = i2c_write_bytes( ts->client , inbuf, 3);
    return ret;
}

static  int gt811_reset( struct goodix_ts_data *ts )
{
    int ret = 1;
    u8 retry;

    unsigned char outbuf[3] = {0, 0xff, 0};
    unsigned char inbuf[3] = {0, 0xff, 0};
    //outbuf[1] = 1;
    lidbg("come into [%s]", __func__);

    //gpio_direction_output(SHUTDOWN_PORT,0);
    msleep(20);
    // gpio_direction_input(SHUTDOWN_PORT);
    msleep(100);
    for(retry = 0; retry < 80; retry++)
    {
        ret = i2c_write_bytes(ts->client, inbuf, 0);	//Test I2C connection.
        if (ret > 0)
        {
            msleep(10);
            ret = i2c_read_bytes(ts->client, inbuf, 3);	//Test I2C connection.
            if (ret > 0)
            {
                if(inbuf[2] == 0x55)
                {
                    ret = i2c_write_bytes(ts->client, outbuf, 3);
                    msleep(10);
                    break;
                }
            }
        }
        else
        {
            //gpio_direction_output(SHUTDOWN_PORT,0);
            msleep(20);
            //gpio_direction_input(SHUTDOWN_PORT);
            msleep(20);
            lidbg( "i2c address failed\n");
        }

    }
    lidbg( "Detect address %0X\n", ts->client->addr);
    //msleep(500);
    return ret;
}

static  int gt811_reset2( struct goodix_ts_data *ts )
{
    int ret = 1;
    u8 retry;

    //unsigned char outbuf[3] = {0,0xff,0};
    unsigned char inbuf[3] = {0, 0xff, 0};
    //outbuf[1] = 1;
    lidbg("come into [%s]", __func__);

    //gpio_direction_output(SHUTDOWN_PORT,0);
    msleep(20);
    //gpio_direction_input(SHUTDOWN_PORT);
    msleep(100);
    for(retry = 0; retry < 80; retry++)
    {
        ret = i2c_write_bytes(ts->client, inbuf, 0);	//Test I2C connection.
        if (ret > 0)
        {
            msleep(10);
            ret = i2c_read_bytes(ts->client, inbuf, 3);	//Test I2C connection.
            if (ret > 0)
            {
                //   if(inbuf[2] == 0x55)
                //       {
                //	    ret =i2c_write_bytes(ts->client, outbuf, 3);
                //	    msleep(10);
                break;
                //		}
            }
        }

    }
    lidbg( "Detect address %0X\n", ts->client->addr);
    //msleep(500);
    return ret;
}
static  int gt811_set_address_2( struct goodix_ts_data *ts )
{
    unsigned char inbuf[3] = {0, 0, 0};
    int i;
    lidbg("come into [%s]", __func__);

    for ( i = 0 ; i < 12 ; i++ )
    {
        if ( i2c_read_bytes( ts->client, inbuf, 3) )
        {
            lidbg( "Got response\n");
            return 1;
        }
        lidbg( "wait for retry\n");
        msleep(50);
    }
    return 0;
}
static u8  gt811_update_firmware( u8 *nvram, u16 start_addr, u16 length, struct goodix_ts_data *ts)
{
    u8 ret, err, retry_time, i;
    u16 cur_code_addr;
    u16 cur_frame_num, total_frame_num, cur_frame_len;
    u32 gt80x_update_rate;

    unsigned char i2c_data_buf[PACK_SIZE + 2] = {0,};
    unsigned char i2c_chk_data_buf[PACK_SIZE + 2] = {0,};

    lidbg("come into [%s]", __func__);
    if( length > NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN )
    {
        lidbg( "Fw length %d is bigger than limited length %d\n", length, NVRAM_LEN - NVRAM_BOOT_SECTOR_LEN );
        return 0;
    }

    total_frame_num = ( length + PACK_SIZE - 1) / PACK_SIZE;

    //gt80x_update_sta = _UPDATING;
    gt80x_update_rate = 0;

    for( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )
    {
        retry_time = 5;

        lidbg( "PACK[%d]\n", cur_frame_num);
        cur_code_addr = /*NVRAM_UPDATE_START_ADDR*/start_addr + cur_frame_num * PACK_SIZE;
        i2c_data_buf[0] = (cur_code_addr >> 8) & 0xff;
        i2c_data_buf[1] = cur_code_addr & 0xff;

        i2c_chk_data_buf[0] = i2c_data_buf[0];
        i2c_chk_data_buf[1] = i2c_data_buf[1];

        if( cur_frame_num == total_frame_num - 1 )
        {
            cur_frame_len = length - cur_frame_num * PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }

        //strncpy(&i2c_data_buf[2], &nvram[cur_frame_num*PACK_SIZE], cur_frame_len);
        for(i = 0; i < cur_frame_len; i++)
        {
            i2c_data_buf[2 + i] = nvram[cur_frame_num * PACK_SIZE + i];
        }
        do
        {
            err = 0;

            //ret = gt811_i2c_write( guitar_i2c_address, cur_code_addr, &nvram[cur_frame_num*I2C_FRAME_MAX_LENGTH], cur_frame_len );
            ret = i2c_write_bytes(ts->client, i2c_data_buf, (cur_frame_len + 2));
            if ( ret <= 0 )
            {
                lidbg( "write fail\n");
                err = 1;
            }

            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len + 2));
            // ret = gt811_i2c_read( guitar_i2c_address, cur_code_addr, inbuf, cur_frame_len);
            if ( ret <= 0 )
            {
                lidbg( "read fail\n");
                err = 1;
            }

            if( is_equal( &i2c_data_buf[2], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                lidbg( "not equal\n");
                err = 1;
            }

        }
        while ( err == 1 && (--retry_time) > 0 );

        if( err == 1 )
        {
            break;
        }

        gt80x_update_rate = ( cur_frame_num + 1 ) * 128 / total_frame_num;

    }

    if( err == 1 )
    {
        lidbg( "write nvram fail\n");
        return 0;
    }

    ret = gt811_nvram_store(ts);

    msleep( 20 );

    if( ret == 0 )
    {
        lidbg( "nvram store fail\n");
        return 0;
    }

    ret = gt811_nvram_recall(ts);

    msleep( 20 );

    if( ret == 0 )
    {
        lidbg( "nvram recall fail\n");
        return 0;
    }

    for ( cur_frame_num = 0 ; cur_frame_num < total_frame_num ; cur_frame_num++ )		 //	read out all the code
    {

        cur_code_addr = NVRAM_UPDATE_START_ADDR + cur_frame_num * PACK_SIZE;
        retry_time = 5;
        i2c_chk_data_buf[0] = (cur_code_addr >> 8) & 0xff;
        i2c_chk_data_buf[1] = cur_code_addr & 0xff;


        if ( cur_frame_num == total_frame_num - 1 )
        {
            cur_frame_len = length - cur_frame_num * PACK_SIZE;
        }
        else
        {
            cur_frame_len = PACK_SIZE;
        }

        do
        {
            err = 0;
            //ret = gt811_i2c_read( guitar_i2c_address, cur_code_addr, inbuf, cur_frame_len);
            ret = i2c_read_bytes(ts->client, i2c_chk_data_buf, (cur_frame_len + 2));

            if ( ret == 0 )
            {
                err = 1;
            }

            if( is_equal( &nvram[cur_frame_num * PACK_SIZE], &i2c_chk_data_buf[2], cur_frame_len ) == 0 )
            {
                err = 1;
            }
        }
        while ( err == 1 && (--retry_time) > 0 );

        if( err == 1 )
        {
            break;
        }

        gt80x_update_rate = 127 + ( cur_frame_num + 1 ) * 128 / total_frame_num;
    }

    gt80x_update_rate = 255;
    //gt80x_update_sta = _UPDATECHKCODE;

    if( err == 1 )
    {
        lidbg( "nvram validate fail\n");
        return 0;
    }

    return 1;
}

static u8  gt811_update_proc( u8 *nvram, u16 start_addr , u16 length, struct goodix_ts_data *ts )
{
    u8 ret;
    u8 error = 0;
    //struct tpd_info_t tpd_info;
    //GT811_SET_INT_PIN( 0 );
    msleep( 20 );

    lidbg("come into [%s]", __func__);
    ret = gt811_reset(ts);
    if ( ret < 0 )
    {
        error = 1;
        lidbg( "reset fail\n");
        goto end;
    }

    ret = gt811_set_address_2( ts );
    if ( ret == 0 )
    {
        error = 1;
        lidbg( "set address fail\n");
        goto end;
    }

    ret = gt811_update_firmware( nvram, start_addr, length, ts);
    if ( ret == 0 )
    {
        error = 1;
        lidbg( "firmware update fail\n");
        goto end;
    }

end:
    // GT811_SET_INT_PIN( 1 );
    //    gpio_free(INT_PORT);
    //   s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE);

    msleep( 500 );
    ret = gt811_reset2(ts);
    if ( ret < 0 )
    {
        error = 1;
        lidbg( "final reset fail\n");
        goto end;
    }
    if ( error == 1 )
    {
        return 0;
    }

    //    i2c_pre_cmd(ts);
    while(goodix_read_version(ts) < 0);

    //    i2c_end_cmd(ts);
    return 1;
}

u16 Little2BigEndian(u16 little_endian)
{
    u16 temp = 0;
    temp = little_endian & 0xff;
    return (temp << 8) + ((little_endian >> 8) & 0xff);
}

int  gt811_downloader( struct goodix_ts_data *ts,  unsigned char *data)
{
    struct tpd_firmware_info_t *fw_info = (struct tpd_firmware_info_t *)data;
    //int i;
    //unsigned short checksum = 0;
    //unsigned int  checksum = 0;

    unsigned int  fw_checksum = 0;
    //unsigned char fw_chip_type;
    unsigned short fw_version;
    unsigned short fw_start_addr;
    unsigned short fw_length;
    unsigned char *data_ptr;
    //unsigned char *file_ptr = &(fw_info->chip_type);
    int retry = 0, ret;
    int err = 0;
    unsigned char rd_buf[4] = {0};
    unsigned char *mandatory_base = "GOODIX";
    unsigned char rd_rom_version;
    unsigned char rd_chip_type;
    unsigned char rd_nvram_flag;
    lidbg("come into [%s]", __func__);

    //struct file * file_data = NULL;
    //mm_segment_t old_fs;
    //unsigned int rd_len;
    //unsigned int file_len = 0;
    //unsigned char i2c_data_buf[PACK_SIZE] = {0,};

    rd_buf[0] = 0x14;
    rd_buf[1] = 0x00;
    rd_buf[2] = 0x80;
    ret = i2c_write_bytes(ts->client, rd_buf, 3);
    if(ret < 0)
    {
        lidbg( "i2c write failed\n");
        goto exit_downloader;
    }
    rd_buf[0] = 0x40;
    rd_buf[1] = 0x11;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret <= 0)
    {
        lidbg( "i2c request failed!\n");
        goto exit_downloader;
    }
    rd_chip_type = rd_buf[2];
    rd_buf[0] = 0xFB;
    rd_buf[1] = 0xED;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret <= 0)
    {
        lidbg( "i2c read failed!\n");
        goto exit_downloader;
    }
    rd_rom_version = rd_buf[2];
    rd_buf[0] = 0x06;
    rd_buf[1] = 0x94;
    ret = i2c_read_bytes(ts->client, rd_buf, 3);
    if(ret <= 0)
    {
        lidbg( "i2c read failed!\n");
        goto exit_downloader;
    }
    rd_nvram_flag = rd_buf[2];

    fw_version = Little2BigEndian(fw_info->version);
    fw_start_addr = Little2BigEndian(fw_info->start_addr);
    fw_length = Little2BigEndian(fw_info->length);
    data_ptr = &(fw_info->data);

    lidbg("chip_type=0x%02x\n", fw_info->chip_type);
    lidbg("version=0x%04x\n", fw_version);
    lidbg("rom_version=0x%02x\n", fw_info->rom_version);
    lidbg("start_addr=0x%04x\n", fw_start_addr);
    lidbg("file_size=0x%04x\n", fw_length);
    fw_checksum = ((u32)fw_info->checksum[0] << 16) + ((u32)fw_info->checksum[1] << 8) + ((u32)fw_info->checksum[2]);
    lidbg("fw_checksum=0x%06x\n", fw_checksum);
    lidbg("%s\n", __func__ );
    lidbg("current version 0x%04X, target verion 0x%04X\n", ts->version, fw_version );

    //chk_chip_type:
    if(rd_chip_type != fw_info->chip_type)
    {
        lidbg( "Chip type not match,exit downloader\n");
        goto exit_downloader;
    }

    //chk_mask_version:
    if(!rd_rom_version)
    {
        if(fw_info->rom_version != 0x45)
        {
            lidbg( "Rom version not match,exit downloader\n");
            goto exit_downloader;
        }
        lidbg( "Rom version E.\n");
        goto chk_fw_version;
    }
    else if(rd_rom_version != fw_info->rom_version);
    {
        lidbg( "Rom version not match,exidownloader\n");
        goto exit_downloader;
    }
    lidbg( "Rom version %c\n", rd_rom_version);

    //chk_nvram:
    if(rd_nvram_flag == 0x55)
    {
        lidbg( "NVRAM correct!\n");
        goto chk_fw_version;
    }
    else if(rd_nvram_flag == 0xAA)
    {
        lidbg( "NVRAM incorrect!Need update.\n");
        goto begin_upgrade;
    }
    else
    {
        lidbg( "NVRAM other error![0x694]=0x%02x\n", rd_nvram_flag);
        goto begin_upgrade;
    }
chk_fw_version:
    //	ts->version -= 1;               //test by andrew
    if( ts->version >= fw_version )   // current low byte higher than back-up low byte
    {
        lidbg( "Fw verison not match.\n");
        goto chk_mandatory_upgrade;
    }
    lidbg("Need to upgrade\n");
    goto begin_upgrade;
chk_mandatory_upgrade:
    //	lidbg( "%s\n", mandatory_base);
    //	lidbg( "%s\n", fw_info->mandatory_flag);
    ret = memcmp(mandatory_base, fw_info->mandatory_flag, 6);
    if(ret)
    {
        lidbg("Not meet mandatory upgrade,exit downloader!ret:%d\n", ret);
        goto exit_downloader;
    }
    lidbg( "Mandatory upgrade!\n");
begin_upgrade:
    lidbg( "Begin upgrade!\n");
    //   goto exit_downloader;
    lidbg("STEP_0:\n");
    //gpio_free(INT_PORT);
    // ret = gpio_request(INT_PORT, "TS_INT");	//Request IO
    // if (ret < 0)
    // {
    //     lidbg("Failed to request GPIO:%d, ERRNO:%d\n",(int)INT_PORT,ret);
    //     err = -1;
    //     goto exit_downloader;
    // }

    lidbg( "STEP_1:\n");
    err = -1;
    while( retry < 3 )
    {
        ret = gt811_update_proc( data_ptr, fw_start_addr, fw_length, ts);
        if(ret == 1)
        {
            err = 1;
            break;
        }
        retry++;
    }

exit_downloader:
    //mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    // mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
    // gpio_direction_output(INT_PORT,1);
    // msleep(1);
    //8gpio_free(INT_PORT); //futengfei
    //8s3c_gpio_setpull(INT_PORT, S3C_GPIO_PULL_NONE);//futengfei
    return err;

}
//******************************End of firmware update surpport*******************************
/////////////////////////////// UPDATE STEP 4 END /////////////////////////////////////////////////////////////////

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


static struct i2c_board_info i2c_gt811[]  =
{
    {
        I2C_BOARD_INFO(GOODIX_I2C_NAME, 0x5d),
        .irq = GPIOEIT,
        // .platform_data = &synaptics_platformdata,
    },

};

/*******************************************************
??:
	??????
return:
	?????,0??????
********************************************************/
#define MSM_GSBI1_QUP_I2C_BUS_ID	1

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
    lidbg("[futengfei]==================ts_nod_open\n");

    return 0;          /* success */
}

ssize_t ts_nod_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    char data_rec[20];
    struct ts_device *tsdev = filp->private_data;

    if (copy_from_user( data_rec, buf, count))
    {
        lidbg("copy_from_user ERR\n");
    }
    data_rec[count] =  '\0';
    lidbg("[futengfei]ts_nod_write:==%d====[%s]\n", count, data_rec);
    // processing data
    if(!(strnicmp(data_rec, "TSMODE_XYREVERT", count - 1)))
    {
        xy_revert_en = 1;
        lidbg("[futengfei]ts_nod_write:==========TSMODE_XYREVERT\n");
    }
    else if(!(strnicmp(data_rec, "TSMODE_NORMAL", count - 1)))
    {
        xy_revert_en = 0;
        lidbg("[futengfei]ts_nod_write:==========TSMODE_NORMAL\n");
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
    dev_t dev_number = MKDEV(major_number_ts, 0);

    //11creat cdev
    tsdev = (struct ts_device *)kmalloc( sizeof(struct ts_device), GFP_KERNEL );
    if (tsdev == NULL)
    {
        ret = -ENOMEM;
        lidbg("[futengfei]===========init_cdev_ts:kmalloc err \n");
        return ret;
    }

    if(major_number_ts)
    {
        result = register_chrdev_region(dev_number, 1, TS_DEVICE_NAME);
    }
    else
    {
        result = alloc_chrdev_region(&dev_number, 0, 1, TS_DEVICE_NAME);
        major_number_ts = MAJOR(dev_number);
    }
    lidbg("[futengfei]===========alloc_chrdev_region result:%d \n", result);

    cdev_init(&tsdev->cdev, &ts_nod_fops);
    tsdev->cdev.owner = THIS_MODULE;
    tsdev->cdev.ops = &ts_nod_fops;
    err = cdev_add(&tsdev->cdev, dev_number, 1);
    if (err)
        lidbg( "[futengfei]===========Error cdev_add\n");

    //cread cdev node in /dev
    class_install_ts = class_create(THIS_MODULE, "tsnodclass");
    if(IS_ERR(class_install_ts))
    {
        lidbg( "[futengfei]=======class_create err\n");
        return -1;
    }
    device_create(class_install_ts, NULL, dev_number, NULL, "%s%d", TS_DEVICE_NAME, 0);
}
#endif

static int __devinit goodix_ts_init(void)
{
    int ret = 0;
    is_ts_load = 1;
    LIDBG_GET;
    lidbg("\n\n==in=GT811.KO=====1024580==========touch INFO===========futengfei\n");

    //V2????,V3??
#ifdef BOARD_V2
    SOC_IO_Output(0, 27, 1);
    msleep(200);
    SOC_IO_Output(0, 27, 0);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
    msleep(300);
#else
    SOC_IO_Output(0, 27, 0);
    msleep(200);
    SOC_IO_Output(0, 27, 1);//NOTE:GT811 SHUTDOWN PIN ,set hight to work.
    msleep(300);
#endif

#if 0
    {
        struct i2c_adapter *i2c_adap;
        //struct i2c_board_info i2c_info;
        void *act_client = NULL;

        i2c_adap = i2c_get_adapter(MSM_GSBI1_QUP_I2C_BUS_ID);

        act_client = i2c_new_device(i2c_adap, i2c_gt811);
        if (!act_client)
            lidbg(KERN_INFO "i2c_new_device fail!");


        i2c_put_adapter(i2c_adap);


    }
#endif


#if 0
    uint8_t device_check[6] = {0x55};
    lidbg("tvp5150 init+\n");
    SOC_IO_Output(5, 11, 1);
    SOC_IO_Output(4, 8, 1);

again:
    i2c_api_do_recv(1, 0x5d, 0x68, device_check, 6 );
    lidbg("\ngoodix_ts_init:===device_check=[%x].[%x].[%x].[%x].[%x].[%x]\n", device_check[0], device_check[1], device_check[2], device_check[3], device_check[4], device_check[5]);
    if( device_check[0] == 0xff && device_check[1] == 0xff && device_check[2] == 0xff && device_check[3] == 0xff && device_check[4] == 0xff && device_check[5] == 0xff)
    {
        lidbg("\ngoodix_ts_init:not the 7cun gh_touch_GT811 devices or broken! skip this driver!_futengfei~\n\n\n");
        ret++;
        if (ret >= 3)return -1;
        msleep(200);
        goto again;
    }

    if(device_check[0] == 0x00 && device_check[1] == 0x00)
    {
        lidbg("IIC err: SDA is not hight!5150_  insmod err! futengfei~\n");
    }
#endif

    goodix_wq = create_workqueue("goodix_wq");		//create a work queue and worker thread
    if (!goodix_wq)
    {
        lidbg("creat workqueue faiked\n");
        return -ENOMEM;
    }

    ret = i2c_add_driver(&goodix_ts_driver);

    lidbg("[futengfei]  init_cdev_ts();\n");
    init_cdev_ts();
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
    lidbg("Touchscreen driver of guitar exited.\n");

    lidbg("come into [%s]", __func__);
    i2c_del_driver(&goodix_ts_driver);
    //if (goodix_wq)
    //destroy_workqueue(goodix_wq);		//release our work queue
}

late_initcall(goodix_ts_init);				//???????felix
module_exit(goodix_ts_exit);

MODULE_DESCRIPTION("Goodix Touchscreen Driver");
MODULE_LICENSE("GPL");


