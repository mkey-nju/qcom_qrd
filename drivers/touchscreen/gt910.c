/* drivers/input/touchscreen/gt9xx.c
 *
 * 2010 - 2013 Goodix Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the GOODiX's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Version: 1.0
 * Authors: andrew@goodix.com, meta@goodix.com
 * Release Date: 2013/04/25
 * Revision record:
 *      V1.0:
 *          first Release. By Meta & Andrew, 2012/08/31
 */
#define dug_enable 0
#include <linux/irq.h>
#include <linux/kthread.h>
#include "gt910.h"
#if GTP_ICS_SLOT_REPORT
#include <linux/input/mt.h>
#endif

#include "lidbg.h"
LIDBG_DEFINE;

#include "touch.h"
touch_t touch = {0, 0, 0};

#define CFG_GROUP_LEN(p_cfg_grp)  (sizeof(p_cfg_grp) / sizeof(p_cfg_grp[0]))

static const char *goodix_ts_name = "Goodix Flashless TS";
static struct workqueue_struct *goodix_wq;
struct i2c_client *i2c_connect_client = NULL;
u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
    = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

#if GTP_HAVE_TOUCH_KEY
static const u16 touch_key_array[] = GTP_KEY_TAB;
#define GTP_MAX_KEY_NUM  (sizeof(touch_key_array)/sizeof(touch_key_array[0]))
#endif

static s32 gtp_init_panel(struct goodix_ts_data *ts);
static s8 gtp_i2c_test(struct i2c_client *client);
void gtp_int_sync(s32 ms);
void gtp_reset_guitar(struct i2c_client *client, s32 ms);
static void gtp_recovery_reset(struct i2c_client *client);
static void gtp_mask_reset(struct i2c_client *client);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void goodix_ts_early_suspend(struct early_suspend *h);
static void goodix_ts_late_resume(struct early_suspend *h);
#endif

#if GTP_CREATE_WR_NODE
//extern s32 init_wr_node(struct i2c_client*);
//extern void uninit_wr_node(void);
#endif
unsigned int  touch_cnt = 0;

#if GTP_ESD_PROTECT
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
static s32 gtp_init_ext_watchdog(struct i2c_client *client);
void gtp_esd_switch(struct i2c_client *client, s32 on);
static void gtp_esd_check_func(struct work_struct *);
#endif

// flashless
extern s32 gup_update_proc(void *dir);
extern s32 i2c_read_bytes(struct i2c_client *client, u16 addr, u8 *buf, s32 len);
extern s32 i2c_write_bytes(struct i2c_client *client, u16 addr, u8 *buf, s32 len);
extern s32 gup_fw_download_proc(void *dir, u8 dwn_mode);
extern s32 gup_check_fs_mounted(char *path);
extern s32 gup_clk_calibration(void);

s32 gtp_fw_startup(struct i2c_client *client);
static s32 gtp_bak_ref_proc(struct goodix_ts_data *ts, u8 mode);
static s32 gtp_esd_recovery(struct i2c_client *client);

#if GTP_SLIDE_WAKEUP
typedef enum
{
    DOZE_DISABLED = 0,
    DOZE_ENABLED = 1,
    DOZE_WAKEUP = 2,
} DOZE_T;
static DOZE_T doze_status = DOZE_DISABLED;
#endif
#define SCREEN_X (1024)
#define SCREEN_Y (600)
static int screen_x = 0;
static int screen_y = 0;
static bool xy_revert_en = 0;
static u8 chip_gt9xxs = 0;  // true if ic is gt9xxs, like gt915s
extern  bool is_ts_load;
/*******************************************************
Function:
    Read data from the i2c slave device.
Input:
    client:     i2c device.
    buf[0~1]:   read start address.
    buf[2~len-1]:   read data buffer.
    len:    GTP_ADDR_LENGTH + read bytes count
Output:
    numbers of i2c_msgs to transfer:
      2: succeed, otherwise: failed
*********************************************************/
s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret = -1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msgs[0].flags = !I2C_M_RD;
    msgs[0].addr  = client->addr;
    msgs[0].len   = GTP_ADDR_LENGTH;
    msgs[0].buf   = &buf[0];

    msgs[1].flags = I2C_M_RD;
    msgs[1].addr  = client->addr;
    msgs[1].len   = len - GTP_ADDR_LENGTH;
    msgs[1].buf   = &buf[GTP_ADDR_LENGTH];

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, msgs, 2);
        if(ret == 2)break;
        retries++;
    }
    if((retries >= 5))
    {
#if GTP_SLIDE_WAKEUP
        // reset chip would quit doze mode
        if (DOZE_ENABLED == doze_status)
        {
            return ret;
        }
#endif
        GTP_ERROR("I2C Read: failed to read 0x%02X%02X, %d byte(s), err-code: %d", buf[0], buf[1], len - 2, ret);
        gtp_recovery_reset(client);
    }
    return ret;
}

/*******************************************************
Function:
    Write data to the i2c slave device.
Input:
    client:     i2c device.
    buf[0~1]:   write start address.
    buf[2~len-1]:   data buffer
    len:    GTP_ADDR_LENGTH + write bytes count
Output:
    numbers of i2c_msgs to transfer:
        1: succeed, otherwise: failed
*********************************************************/
s32 gtp_i2c_write(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msg;
    s32 ret = -1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = len;
    msg.buf   = buf;

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }
    if((retries >= 5))
    {
#if GTP_SLIDE_WAKEUP
        if (DOZE_ENABLED == doze_status)
        {
            return ret;
        }
#endif
        GTP_ERROR("I2C Write: failed to write 0x%02X%02X, %d byte(s),err-code: %d", buf[0], buf[1], len - 2, ret);
        gtp_recovery_reset(client);
    }
    return ret;
}

static s32 gtp_bak_ref_proc(struct goodix_ts_data *ts, u8 mode)
{
    s32 ret = 0;
    s32 i = 0;
    u16 ref_sum = 0;
    u16 learn_cnt = 0;
    u16 chksum = 0;
    struct file *ref_filp = NULL;
    u8 *p_bak_ref;

    ret = gup_check_fs_mounted("/data");
    if (FAIL == ret)
    {
        ts->ref_chk_fs_times++;
        GTP_DEBUG("ref check /data times/MAX_TIMES: %d / %d", ts->ref_chk_fs_times, GTP_CHK_FS_MNT_MAX);
        if (ts->ref_chk_fs_times < GTP_CHK_FS_MNT_MAX)
        {
            msleep(50);
            GTP_INFO("/data not mounted.");
            return FAIL;
        }
        GTP_INFO("check /data mount timeout...");
    }
    else
    {
        GTP_INFO("/data/ mounted!!!(%d/%d)", ts->ref_chk_fs_times, GTP_CHK_FS_MNT_MAX);
    }

    p_bak_ref = (u8 *)kzalloc(ts->bak_ref_len, GFP_KERNEL);
    if (NULL == p_bak_ref)
    {
        GTP_ERROR("allocate memory failed for p_bak_ref");
        return FAIL;
    }
    ref_filp = filp_open(GTP_BAK_REF_PATH, O_RDWR | O_CREAT, 0666);
    if (IS_ERR(ref_filp))
    {
        GTP_INFO("%s is unavailable, default backup-reference used", GTP_BAK_REF_PATH);
        goto bak_ref_default;
    }

    switch (mode)
    {
    case GTP_BAK_REF_SEND:
        GTP_INFO("Send backup-reference");
        ref_filp->f_op->llseek(ref_filp, 0, SEEK_SET);
        ret = ref_filp->f_op->read(ref_filp, (char *)p_bak_ref, ts->bak_ref_len, &ref_filp->f_pos);
        if (ret < 0)
        {
            GTP_ERROR("failed to read bak_ref info from file, sending defualt bak_ref");
            goto bak_ref_default;
        }
        ref_sum = 0;
        for (i = 0; i < (ts->bak_ref_len); i += 2)
        {
            ref_sum += (p_bak_ref[i] << 8) + p_bak_ref[i + 1];
        }
        learn_cnt = (p_bak_ref[ts->bak_ref_len - 4] << 8) + (p_bak_ref[ts->bak_ref_len - 3]);
        chksum = (p_bak_ref[ts->bak_ref_len - 2] << 8) + (p_bak_ref[ts->bak_ref_len - 1]);
        GTP_DEBUG("learn count = %d", learn_cnt);
        GTP_DEBUG("chksum = %d", chksum);
        GTP_DEBUG("ref_sum = 0x%04X", ref_sum & 0xFFFF);
        // Sum(1~ts->bak_ref_len) == 1
        if (1 != ref_sum)
        {
            GTP_ERROR("wrong chksum for bak_ref, sending default bak_ref instead");
            goto bak_ref_default;
        }
        GTP_INFO("backup-reference data in %s used", GTP_BAK_REF_PATH);
        ret = i2c_write_bytes(ts->client, GTP_REG_BAK_REF, p_bak_ref, ts->bak_ref_len);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to send bak_ref because of iic comm error");
            filp_close(ref_filp, NULL);
            return FAIL;
        }
        break;

    case GTP_BAK_REF_STORE:
        GTP_INFO("Store backup-reference");
        ret = i2c_read_bytes(ts->client, GTP_REG_BAK_REF, p_bak_ref, ts->bak_ref_len);
        if (ret < 0)
        {
            GTP_ERROR("failed to read bak_ref info, sending default back-reference");
            goto bak_ref_default;
        }
        ref_filp->f_op->llseek(ref_filp, 0, SEEK_SET);
        ref_filp->f_op->write(ref_filp, (char *)p_bak_ref, ts->bak_ref_len, &ref_filp->f_pos);
        break;

    default:
        GTP_ERROR("invalid backup-reference request");
        break;
    }
    filp_close(ref_filp, NULL);
    return SUCCESS;

bak_ref_default:
    memset(p_bak_ref, 0, ts->bak_ref_len);
    p_bak_ref[ts->bak_ref_len - 1] = 0x01;  // checksum = 1
    ret = i2c_write_bytes(ts->client, GTP_REG_BAK_REF, p_bak_ref, ts->bak_ref_len);
    if (!IS_ERR(ref_filp))
    {
        GTP_INFO("write backup-reference data into %s", GTP_BAK_REF_PATH);
        ref_filp->f_op->llseek(ref_filp, 0, SEEK_SET);
        ref_filp->f_op->write(ref_filp, (char *)p_bak_ref, ts->bak_ref_len, &ref_filp->f_pos);
        filp_close(ref_filp, NULL);
    }
    if (ret == FAIL)
    {
        GTP_ERROR("failed to load the default backup reference");
        return FAIL;
    }
    return SUCCESS;

}

static s32 gtp_verify_main_clk(u8 *p_main_clk)
{
    u8 chksum = 0;
    u8 main_clock = p_main_clk[0];
    s32 i = 0;

    if (main_clock < 50 || main_clock > 120)
    {
        return FAIL;
    }

    for (i = 0; i < 5; ++i)
    {
        if (main_clock != p_main_clk[i])
        {
            return FAIL;
        }
        chksum += p_main_clk[i];
    }
    chksum += p_main_clk[5];
    if ( (chksum) == 0)
    {
        return SUCCESS;
    }
    else
    {
        return FAIL;
    }
}


static s32 gtp_main_clk_proc(struct goodix_ts_data *ts)
{
    s32 ret = 0;
    s32 i = 0;
    s32 clk_chksum = 0;
    struct file *clk_filp = NULL;
    u8 p_main_clk[6] = {0};

    ret = gup_check_fs_mounted("/data");
    if (FAIL == ret)
    {
        ts->clk_chk_fs_times++;
        GTP_DEBUG("clock check /data times/MAX_TIMES: %d / %d", ts->clk_chk_fs_times, GTP_CHK_FS_MNT_MAX);
        if (ts->clk_chk_fs_times < GTP_CHK_FS_MNT_MAX)
        {
            msleep(50);
            GTP_INFO("/data not mounted.");
            return FAIL;
        }
        GTP_INFO("check /data mount timeout...");
    }
    else
    {
        GTP_INFO("/data/ mounted!!!(%d/%d)", ts->clk_chk_fs_times, GTP_CHK_FS_MNT_MAX);
    }

    clk_filp = filp_open(GTP_MAIN_CLK_PATH, O_RDWR | O_CREAT, 0666);
    if (IS_ERR(clk_filp))
    {
        GTP_ERROR("%s is unavailable, calculate main clock", GTP_MAIN_CLK_PATH);
    }
    else
    {
        clk_filp->f_op->llseek(clk_filp, 0, SEEK_SET);
        clk_filp->f_op->read(clk_filp, (char *)p_main_clk, 6, &clk_filp->f_pos);

        ret = gtp_verify_main_clk(p_main_clk);
        if (FAIL == ret)
        {
            // recalculate main clock & rewrite main clock data to file
            GTP_ERROR("main clock data in %s is wrong, recalculate main clock", GTP_MAIN_CLK_PATH);
        }
        else
        {
            GTP_INFO("main clock data in %s used, main clock freq: %d", GTP_MAIN_CLK_PATH, p_main_clk[0]);
            filp_close(clk_filp, NULL);
            goto update_main_clk;
        }
    }

#if GTP_ESD_PROTECT
    gtp_esd_switch(ts->client, SWITCH_OFF);
#endif
    ret = gup_clk_calibration();
    gtp_esd_recovery(ts->client);

#if GTP_ESD_PROTECT
    gtp_esd_switch(ts->client, SWITCH_ON);
#endif

    GTP_INFO("calibrate main clock: %d", ret);
    if (ret < 50 || ret > 120)
    {
        GTP_ERROR("wrong main clock: %d", ret);
        goto exit_main_clk;
    }

    // Sum{0x8020~0x8025} = 0
    for (i = 0; i < 5; ++i)
    {
        p_main_clk[i] = ret;
        clk_chksum += p_main_clk[i];
    }
    p_main_clk[5] = 0 - clk_chksum;

    if (!IS_ERR(clk_filp))
    {
        GTP_DEBUG("write main clock data into %s", GTP_MAIN_CLK_PATH);
        clk_filp->f_op->llseek(clk_filp, 0, SEEK_SET);
        clk_filp->f_op->write(clk_filp, (char *)p_main_clk, 6, &clk_filp->f_pos);
        filp_close(clk_filp, NULL);
    }

update_main_clk:
    ret = i2c_write_bytes(ts->client, GTP_REG_MAIN_CLK, p_main_clk, 6);
    if (FAIL == ret)
    {
        GTP_ERROR("update main clock failed!");
        return FAIL;
    }
    return SUCCESS;

exit_main_clk:
    if (!IS_ERR(clk_filp))
    {
        filp_close(clk_filp, NULL);
    }
    return FAIL;
}

/*******************************************************
Function:
    Send config.
Input:
    client: i2c device.
Output:
    result of i2c write operation.
        1: succeed, otherwise: failed
*********************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = 2;

    s32 retry = 0;
    GTP_INFO("driver send config");
    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);
        if (ret > 0)
        {
            break;
        }
    }

    return ret;
}

/*******************************************************
Function:
    Disable irq function
Input:
    ts: goodix i2c_client private data
Output:
    None.
*********************************************************/
void gtp_irq_disable(struct goodix_ts_data *ts)
{
    unsigned long irqflags;

    GTP_DEBUG_FUNC();

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (!ts->irq_is_disable)
    {
        ts->irq_is_disable = 1;
        disable_irq_nosync(ts->client->irq);
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}

/*******************************************************
Function:
    Enable irq function
Input:
    ts: goodix i2c_client private data
Output:
    None.
*********************************************************/
void gtp_irq_enable(struct goodix_ts_data *ts)
{
    unsigned long irqflags = 0;

    GTP_DEBUG_FUNC();

    spin_lock_irqsave(&ts->irq_lock, irqflags);
    if (ts->irq_is_disable)
    {
        enable_irq(ts->client->irq);
        ts->irq_is_disable = 0;
    }
    spin_unlock_irqrestore(&ts->irq_lock, irqflags);
}


/*******************************************************
Function:
    Report touch point event
Input:
    ts: goodix i2c_client private data
    id: trackId
    x:  input x coordinate
    y:  input y coordinate
    w:  input pressure
Output:
    None.
*********************************************************/
static void gtp_touch_down(struct goodix_ts_data *ts, s32 id, s32 x, s32 y, s32 w)
{
    if(xy_revert_en == 1)
        GTP_SWAP(x, y);
#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, screen_y - y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
#else
    input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
    input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, screen_y - y);
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, id);
    input_mt_sync(ts->input_dev);
#endif
    touch_cnt++;
    if (touch_cnt > 100)
    {
        touch_cnt = 0;
        GTP_DEBUG("%d[%d,%d];", id, x, screen_y - y);
    }
}

/*******************************************************
Function:
    Report touch release event
Input:
    ts: goodix i2c_client private data
Output:
    None.
*********************************************************/
static void gtp_touch_up(struct goodix_ts_data *ts, s32 id)
{
#if GTP_ICS_SLOT_REPORT
    input_mt_slot(ts->input_dev, id);
    input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, -1);
    GTP_DEBUG("Touch id[%2d] release!", id);
#else
    input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
    input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
    input_mt_sync(ts->input_dev);
#endif
}


/*******************************************************
Function:
    Goodix touchscreen work function
Input:
    work: work struct of goodix_workqueue
Output:
    None.
*********************************************************/
static void goodix_ts_work_func(struct work_struct *work)
{
    u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  touch_num = 0;
    u8  finger = 0;
    static u16 pre_touch = 0;
    static u8 pre_key = 0;
    u8  key_value = 0;
    u8 *coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 id = 0;
    s32 i  = 0;
    s32 ret = -1;
    u8 rqst_buf[3] = {0x80, 0x43};  // for flashless
    struct goodix_ts_data *ts = NULL;

#if GTP_SLIDE_WAKEUP
    u8 doze_buf[3] = {0x81, 0x4B};
#endif

    GTP_DEBUG_FUNC();

    ts = container_of(work, struct goodix_ts_data, work);
    if (ts->enter_update)
    {
        return;
    }

#if GTP_SLIDE_WAKEUP
    if (DOZE_ENABLED == doze_status)
    {
        ret = gtp_i2c_read(i2c_connect_client, doze_buf, 3);
        GTP_DEBUG("0x814B = 0x%02X", doze_buf[2]);
        if (ret > 0)
        {
            if (doze_buf[2] == 0xAA)
            {
                GTP_INFO("Slide To Light up the screen!");
                doze_status = DOZE_WAKEUP;
                input_report_key(ts->input_dev, KEY_POWER, 1);
                input_sync(ts->input_dev);
                input_report_key(ts->input_dev, KEY_POWER, 0);
                input_sync(ts->input_dev);
                // clear 0x814B
                doze_buf[2] = 0x00;
                gtp_i2c_write(i2c_connect_client, doze_buf, 3);
            }
        }
        if (ts->use_irq)
        {
            gtp_irq_enable(ts);
        }
        return;
    }
#endif

    ret = gtp_i2c_read(ts->client, point_data, 12);
    if (ret < 0)
    {
        goto exit_work_func;
    }
    else
    {
        //lidbg("=======GT910=====gtp_i2c_read SUCCSEEFUL====\n");
    }
    finger = point_data[GTP_ADDR_LENGTH];

    // flashless
    if (finger == 0x00)     // request arrived
    {
        ret = gtp_i2c_read(ts->client, rqst_buf, 3);
        if (ret < 0)
        {
            GTP_ERROR("read request status error!");
            goto exit_work_func;
        }

        else //lidbg("=======GT910=====gtp_i2c  request status  right====\n");
        {
            //lidbg("=======rqst_buf[2]:%02x\n",rqst_buf[2]);
        }

        switch (rqst_buf[2] & 0x0F)
        {
        case GTP_RQST_CONFIG:
            GTP_INFO("Request for config...");
            ret = gtp_send_cfg(ts->client);
            if (ret < 0)
            {
                GTP_ERROR("Request for config Unresponded!");
            }
            else
            {
                rqst_buf[2] = GTP_RQST_RESPONDED;
                gtp_i2c_write(ts->client, rqst_buf, 3);
                GTP_INFO("Request for config Responded!");
            }
            break;

        case GTP_RQST_BAK_REF:
            GTP_INFO("Request for backup reference...");
            ret = gtp_bak_ref_proc(ts, GTP_BAK_REF_SEND);
            if (SUCCESS == ret)
            {
                rqst_buf[2] = GTP_RQST_RESPONDED;
                gtp_i2c_write(ts->client, rqst_buf, 3);
                GTP_INFO("Request for backup reference Responded!");
            }
            else
            {
                GTP_ERROR("Requeset for backup reference unresponed!");
            }
            break;

        case GTP_RQST_RESET:
            GTP_INFO("Request for reset");
            gtp_recovery_reset(ts->client);
            break;

        case GTP_RQST_CLK_RESENT:
            GTP_INFO("Request for main clock calibration...");
            ts->rqst_processing = 1;
            ret = gtp_main_clk_proc(ts);
            if (FAIL == ret)
            {
                GTP_ERROR("Request for main clock cali Unresponded!");
            }
            else
            {
                GTP_INFO("Request for main clock cali Responded!");
                rqst_buf[2] = GTP_RQST_RESPONDED;
                gtp_i2c_write(ts->client, rqst_buf, 3);
                ts->rqst_processing = 0;
            }
            break;

        case GTP_RQST_IDLE:
        default:
            break;
        }
    }

    if ((finger & 0x80) == 0)
    {
        goto exit_work_func;
    }

    touch_num = finger & 0x0f;
    if (touch_num > GTP_MAX_TOUCH)
    {
        goto exit_work_func;
    }

    if (touch_num > 1)
    {
        u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};

        ret = gtp_i2c_read(ts->client, buf, 2 + 8 * (touch_num - 1));
        memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
    }

#if GTP_HAVE_TOUCH_KEY
    key_value = point_data[3 + 8 * touch_num];

    if(key_value || pre_key)
    {
        for (i = 0; i < GTP_MAX_KEY_NUM; i++)
        {
            input_report_key(ts->input_dev, touch_key_array[i], key_value & (0x01 << i));
        }
        touch_num = 0;
        pre_touch = 0;
    }
#endif
    pre_key = key_value;

    //GTP_DEBUG("pre_touch:%02x, finger:%02x.", pre_touch, finger);

#if GTP_ICS_SLOT_REPORT

    if (pre_touch || touch_num)
    {
        s32 pos = 0;
        u16 touch_index = 0;

        coor_data = &point_data[3];

        if(touch_num)
        {
            id = coor_data[pos];        //  & 0x0F;

            touch_index |= (0x01 << id);
        }

        //GTP_DEBUG("id = %d,touch_index = 0x%x, pre_touch = 0x%x\n",id, touch_index,pre_touch);
        for (i = 0; i < GTP_MAX_TOUCH; i++)
        {
            if (touch_index & (0x01 << i))
            {
                input_x  = coor_data[pos + 1] | (coor_data[pos + 2] << 8);
                input_y  = coor_data[pos + 3] | (coor_data[pos + 4] << 8);
                input_w  = coor_data[pos + 5] | (coor_data[pos + 6] << 8);

                gtp_touch_down(ts, id, input_x, input_y, input_w);
                pre_touch |= 0x01 << i;

                pos += 8;
                id = coor_data[pos] & 0x0F;
                touch_index |= (0x01 << id);
                if(1 == recovery_mode)
                {
                    if( (input_y >= 0) && (input_x >= 0) )
                    {
                        touch.x = point_data[4] | (point_data[5] << 8);
                        touch.y = screen_y - (point_data[6] | (point_data[7] << 8));
                        touch.pressed = 1;
                        set_touch_pos(&touch);
                    }
                }
                g_var.flag_for_15s_off++;
                if(g_var.flag_for_15s_off >= 1000)
                {
                    g_var.flag_for_15s_off = 1000;
                }
                if(g_var.flag_for_15s_off < 0)
                {
                    lidbg("\nerr:FLAG_FOR_15S_OFF===[%d]\n", g_var.flag_for_15s_off);
                }
            }
            else
            {
                gtp_touch_up(ts, i);
                pre_touch &= ~(0x01 << i);
                if(1 == recovery_mode)
                {
                    {
                        touch.pressed = 0;
                        set_touch_pos(&touch);
                    }
                }
            }
        }
    }
    else
        input_report_key(ts->input_dev, BTN_TOUCH, (touch_num || key_value));   // 20130502

    if (touch_num)
    {
        for (i = 0; i < touch_num; i++)
        {
            coor_data = &point_data[i * 8 + 3];

            id = coor_data[0] & 0x0F;
            input_x  = coor_data[1] | (coor_data[2] << 8);
            input_y  = coor_data[3] | (coor_data[4] << 8);
            input_w  = coor_data[5] | (coor_data[6] << 8);

            gtp_touch_down(ts, id, input_x, input_y, input_w);
            if(1 == recovery_mode)
            {
                if( (input_y >= 0) && (input_x >= 0) )
                {
                    touch.x = point_data[4] | (point_data[5] << 8);
                    touch.y = screen_y - (point_data[6] | (point_data[7] << 8));
                    touch.pressed = 1;
                    set_touch_pos(&touch);
                }
            }
        }
        g_var.flag_for_15s_off++;
        if(g_var.flag_for_15s_off >= 1000)
        {
            g_var.flag_for_15s_off = 1000;
        }
        if(g_var.flag_for_15s_off < 0)
        {
            lidbg("\nerr:FLAG_FOR_15S_OFF===[%d]\n", g_var.flag_for_15s_off);
        }
    }
    else if (pre_touch)
    {

        GTP_DEBUG("Touch Release!");
        gtp_touch_up(ts, 0);
        if(1 == recovery_mode)
        {
            touch.pressed = 0;
            set_touch_pos(&touch);
        }

    }

    pre_touch = touch_num;
#endif

    input_sync(ts->input_dev);

exit_work_func:
    if(!ts->gtp_rawdiff_mode)
    {
        ret = gtp_i2c_write(ts->client, end_cmd, 3);
        if (ret < 0)
        {
            GTP_INFO("I2C write end_cmd error!");
        }
    }
    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
}

/*******************************************************
Function:
    Timer interrupt service routine for polling mode.
Input:
    timer: timer struct pointer
Output:
    Timer work mode.
        HRTIMER_NORESTART: no restart mode
*********************************************************/
static enum hrtimer_restart goodix_ts_timer_handler(struct hrtimer *timer)
{
    struct goodix_ts_data *ts = container_of(timer, struct goodix_ts_data, timer);

    GTP_DEBUG_FUNC();

    queue_work(goodix_wq, &ts->work);
    hrtimer_start(&ts->timer, ktime_set(0, (GTP_POLL_TIME + 6) * 1000000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/*******************************************************
Function:
    External interrupt service routine for interrupt mode.
Input:
    irq:  interrupt number.
    dev_id: private data pointer
Output:
    Handle Result.
        IRQ_HANDLED: interrupt handled successfully
*********************************************************/
static irqreturn_t goodix_ts_irq_handler(int irq, void *dev_id)
{
    struct goodix_ts_data *ts = dev_id;

    GTP_DEBUG_FUNC();
    //lidbg("====gt910===enter goodix_ts_irq_handler=====\n");
    gtp_irq_disable(ts);

    queue_work(goodix_wq, &ts->work);

    return IRQ_HANDLED;
}
/*******************************************************
Function:
    Synchronization.
Input:
    ms: synchronization time in millisecond.
Output:
    None.
*******************************************************/
void gtp_int_sync(s32 ms)
{
    /* GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(ms);
    //GTP_GPIO_AS_INT(GTP_INT_PORT);*/
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(ms);
    gpio_tlmm_config(GPIO_CFG(GTP_INT_PORT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    gpio_get_value(GTP_INT_PORT);
    //GTP_GPIO_AS_INT(GTP_INT_PORT);
}

/*******************************************************
Function:
    Reset chip.
Input:
    ms: reset time in millisecond
Output:
    None.
*******************************************************/
void gtp_reset_guitar(struct i2c_client *client, s32 ms)
{
    GTP_DEBUG_FUNC();

    //GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);   // begin select I2C slave addr
    SOC_IO_Output(0, 27, 1);
    msleep(ms);                         // T2: > 10ms
    // HIGH: 0x28/0x29, LOW: 0xBA/0xBB
    GTP_GPIO_OUTPUT(GTP_INT_PORT, client->addr == 0x5d);

    msleep(2);                          // T3: > 100us
    // GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);
    SOC_IO_Output(0, 27, 0);
    msleep(6);                          // T4: > 5ms

    GTP_GPIO_AS_INPUT(GTP_RST_PORT);    // end select I2C slave addr
    // gtp_int_sync(50);
}

static s32 gtp_esd_recovery(struct i2c_client *client)
{
    s32 retry = 0;
    s32 ret = 0;
    struct goodix_ts_data *ts;

    ts = i2c_get_clientdata(client);

    gtp_irq_disable(ts);

    if (GTP_TYPE_FLASHLESS == ts->chip_type)
    {
        GTP_INFO("esd recovery mode: GT900_FLASHLESS");
        gtp_reset_guitar(ts->client, 20);       // reset & select I2C addr
        for (retry = 0; retry < 5; ++retry)
        {
            ret = gup_fw_download_proc(NULL, GTP_FL_ESD_RECOVERY);
            if (FAIL == ret)
            {
                GTP_ERROR("esd recovery failed %d", retry + 1);
                continue;
            }
            ret = gtp_fw_startup(ts->client);
            if (FAIL == ret)
            {
                GTP_ERROR("flashless start up failed %d", retry + 1);
                continue;
            }
            break;
        }
    }
    else    // GT900_MASK
    {
        GTP_INFO("esd recovery mode: GT900_MASK");
        gtp_mask_reset(ts->client);
        gtp_irq_enable(ts);
        return SUCCESS;
    }
    gtp_irq_enable(ts);

    if (retry >= 5)
    {
        GTP_ERROR("failed to esd recovery");
        return FAIL;
    }
    GTP_INFO("esd recovery successful");
    return SUCCESS;
}

void gtp_recovery_reset(struct i2c_client *client)
{
#if GTP_ESD_PROTECT
    gtp_esd_switch(client, SWITCH_OFF);
#endif

    gtp_esd_recovery(client);

#if GTP_ESD_PROTECT
    gtp_esd_switch(client, SWITCH_ON);
#endif
}

static void gtp_mask_reset(struct i2c_client *client)
{
    gtp_reset_guitar(client, 20);
    gtp_int_sync(25);
#if GTP_ESD_PROTECT
    gtp_init_ext_watchdog(client);
#endif
}


#if GTP_SLIDE_WAKEUP
/*******************************************************
Function:
    Enter doze mode for sliding wakeup.
Input:
    ts: goodix tp private data
Output:
    1: succeed, otherwise failed
*******************************************************/
static s8 gtp_enter_doze(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 8};

    GTP_DEBUG_FUNC();

    gtp_irq_disable(ts);

    GTP_DEBUG("entering doze mode...");
    while(retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            doze_status = DOZE_ENABLED;
            GTP_INFO("GTP has been working in doze mode!");
            gtp_irq_enable(ts);
            return ret;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send doze cmd failed.");
    gtp_irq_enable(ts);
    return ret;
}
#else
/*******************************************************
Function:
    Enter sleep mode.
Input:
    ts: private data.
Output:
    Executive outcomes.
       1: succeed, otherwise failed.
*******************************************************/
static s8 gtp_enter_sleep(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};

    u8 status_buf[4] = {0x80, 0x44};

    GTP_DEBUG_FUNC();

    // flashless: host interact with ic
    ret = gtp_i2c_read(ts->client, status_buf, 4);
    if (ret < 0)
    {
        GTP_ERROR("failed to get backup-reference status");
    }

    if (status_buf[2] & 0x80)
    {
        ret = gtp_bak_ref_proc(ts, GTP_BAK_REF_STORE);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to store bak_ref");
        }
    }

    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(5);

    while(retry++ < 5)
    {
        ret = gtp_i2c_write(ts->client, i2c_control_buf, 3);
        if (ret > 0)
        {
            GTP_INFO("GTP enter sleep!");

            return ret;
        }
        msleep(10);
    }
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}
#endif
/*******************************************************
Function:
    Wakeup from sleep.
Input:
    ts: private data.
Output:
    Executive outcomes.
        >0: succeed, otherwise: failed.
*******************************************************/
static s8 gtp_wakeup_sleep(struct goodix_ts_data *ts)
{
    s32 retry = 0;
    s32 ret = -1;
    u8 opr_buf[3] = {0x41, 0x80};

    GTP_DEBUG_FUNC();

#if GTP_SLIDE_WAKEUP
    gtp_reset_guitar(i2c_client_point, 10);
#else
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
    msleep(5);
#endif
    if (GTP_TYPE_MASK == ts->chip_type)
    {
        GTP_INFO("GT900_MASK wakeup");
        gtp_int_sync(25);
#if GTP_ESD_PROTECT
        gtp_init_ext_watchdog(ts->client);
#endif
        return 1;
    }
    for (retry = 0; retry < 10; ++retry)
    {
        // hold ss51 & dsp
        opr_buf[2] = 0x0C;
        ret = gtp_i2c_write(ts->client, opr_buf, 3);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to hold ss51 & dsp!");
            continue;
        }
        opr_buf[2] = 0x00;
        ret = gtp_i2c_read(ts->client, opr_buf, 3);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to get ss51 & dsp status!");
            continue;
        }
        if (0x0C != opr_buf[2])
        {
            GTP_ERROR("ss51 & dsp not been hold!");
            continue;
        }
        GTP_DEBUG("ss51 & dsp confirmed hold");

        ret = gtp_fw_startup(ts->client);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to startup flashless");
            continue;
        }
        break;
    }
    if (retry >= 10)
    {
        GTP_ERROR("failed to wakeup, processing esd recovery");
        gtp_esd_recovery(ts->client);
    }
    else
    {
        GTP_INFO("flashless gtp wakeup");
    }
    return ret;
}

/*******************************************************
Function:
    Initialize gtp.
Input:
    ts: goodix private data
Output:
    Executive outcomes.
        0: succeed, otherwise: failed
*******************************************************/
static s32 gtp_init_panel(struct goodix_ts_data *ts)
{
    u8 sensor_num, driver_num;
    u8 have_key;
    s32 ret = -1;
    s32 i;
    u8 check_sum = 0;
    u8 opr_buf[16];
    u8 sensor_id = 0;

    u8 cfg_info_group1[] = CTP_CFG_GROUP1;
    u8 cfg_info_group2[] = CTP_CFG_GROUP2;
    u8 cfg_info_group3[] = CTP_CFG_GROUP3;
    u8 cfg_info_group4[] = CTP_CFG_GROUP4;
    u8 cfg_info_group5[] = CTP_CFG_GROUP5;
    u8 cfg_info_group6[] = CTP_CFG_GROUP6;
    u8 *send_cfg_buf[] = {cfg_info_group1, cfg_info_group2, cfg_info_group3,
                          cfg_info_group4, cfg_info_group5, cfg_info_group6
                         };
    u8 cfg_info_len[] = { CFG_GROUP_LEN(cfg_info_group1),
                          CFG_GROUP_LEN(cfg_info_group2),
                          CFG_GROUP_LEN(cfg_info_group3),
                          CFG_GROUP_LEN(cfg_info_group4),
                          CFG_GROUP_LEN(cfg_info_group5),
                          CFG_GROUP_LEN(cfg_info_group6)
                        };

    GTP_DEBUG("Config Groups\' Lengths: %d, %d, %d, %d, %d, %d",
              cfg_info_len[0], cfg_info_len[1], cfg_info_len[2], cfg_info_len[3],
              cfg_info_len[4], cfg_info_len[5]);

    if ((!cfg_info_len[1]) && (!cfg_info_len[2]) &&
            (!cfg_info_len[3]) && (!cfg_info_len[4]) &&
            (!cfg_info_len[5]))
    {
        sensor_id = 0;
    }
    else
    {
        msleep(50);         // ensure Sensor_ID is ready

        opr_buf[0] = (u8)(GTP_REG_SENSOR_ID >> 8);
        opr_buf[1] = (u8)(GTP_REG_SENSOR_ID & 0xff);
        // 20130522 start
        i = 0;
        while(i++ < 3)
        {
            opr_buf[2] = 255;
            gtp_i2c_read(ts->client, opr_buf, 3);
            sensor_id = opr_buf[2];

            opr_buf[2] = 255;
            gtp_i2c_read(ts->client, opr_buf, 3);

            if((opr_buf[2] < 0x06) && (opr_buf[2] == sensor_id))
            {
                break;
            }
            msleep(5);
        }
        if (i >= 3)
        {
            GTP_ERROR("Failed to read sensor_id or invalid sensor_id(%d), No config Sent!", sensor_id);
            return -1;
        }
        // 20130522 end
    }
    GTP_DEBUG("Sensor_ID: %d", sensor_id);

    ts->gtp_cfg_len = cfg_info_len[sensor_id];

    if (ts->gtp_cfg_len < GTP_CONFIG_MIN_LENGTH)
    {
        GTP_ERROR("Sensor_ID(%d) matches with NULL or INVALID CONFIG GROUP! NO Config Sent! You need to check you header file CFG_GROUP section!", sensor_id);
        return -1;
    }

    memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
    memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[sensor_id], ts->gtp_cfg_len);


#if GTP_CUSTOM_CFG
    config[RESOLUTION_LOC]     = (u8)GTP_MAX_WIDTH;
    config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH >> 8);
    config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
    config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT >> 8);

    if (GTP_INT_TRIGGER == 0)  //RISING
    {
        config[TRIGGER_LOC] &= 0xfe;
    }
    else if (GTP_INT_TRIGGER == 1)  //FALLING
    {
        config[TRIGGER_LOC] |= 0x01;
    }
#endif  // GTP_CUSTOM_CFG

    check_sum = 0;
    for (i = GTP_ADDR_LENGTH; i < ts->gtp_cfg_len; i++)
    {
        check_sum += config[i];
    }
    config[ts->gtp_cfg_len] = (~check_sum) + 1;

    have_key = (config[GTP_REG_HAVE_KEY - GTP_REG_CONFIG_DATA + 2] & 0x01);
    driver_num = (config[CFG_LOC_DRVA_NUM] & 0x1F) + (config[CFG_LOC_DRVB_NUM] & 0x1F);
    if (have_key)
    {
        driver_num--;
    }
    sensor_num = (config[CFG_LOC_SENS_NUM] & 0x0F) + ((config[CFG_LOC_SENS_NUM] >> 4) & 0x0F);
    ts->bak_ref_len = (driver_num * (sensor_num - 2) + 2) * 2;

    GTP_DEBUG_FUNC();
    if ((ts->abs_x_max == 0) && (ts->abs_y_max == 0))
    {
        ts->abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
        ts->abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
        ts->int_trigger_type = (config[TRIGGER_LOC]) & 0x03;
    }

    GTP_DEBUG("Drv * Sen: %d * %d(key: %d), X_MAX = %d, Y_MAX = %d, TRIGGER = 0x%02x",
              driver_num, sensor_num, have_key, ts->abs_x_max, ts->abs_y_max, ts->int_trigger_type);

    /* ret = gtp_send_cfg(ts->client);
     if (ret < 0)
     {
    GTP_ERROR("Send config error.");
        return -1;
    }
    else
    	{
    lidbg("======Send config succseeful.======GT910===");
    }
    msleep(10);*/
    return 0;
}

/*******************************************************
Function:
    Read chip version.
Input:
    client:  i2c device
    version: buffer to keep ic firmware version
Output:
    read operation return.
        2: succeed, otherwise: failed
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16 *version)
{
    s32 ret = -1;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, sizeof(buf));
    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed");
        return ret;
    }

    if (version)
    {
        *version = (buf[7] << 8) | buf[6];
    }

    if (buf[5] == 0x00)
    {
        GTP_INFO("IC Version: %c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[7], buf[6]);
    }
    else
    {
        if (buf[5] == 'S' || buf[5] == 's')
        {
            chip_gt9xxs = 1;
        }
        GTP_INFO("IC Version: %c%c%c%c_%02x%02x", buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);
    }
    return ret;
}

/*******************************************************
Function:
    I2c test Function.
Input:
    client:i2c client.
Output:
    Executive outcomes.
        2: succeed, otherwise failed.
*******************************************************/
static s8 gtp_i2c_test(struct i2c_client *client)
{
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
    u8 retry = 0;
    s8 ret = -1;

    GTP_DEBUG_FUNC();

    while(retry++ < 5)
    {
        ret = gtp_i2c_read(client, test, 3);
        if (ret > 0)
        {
            return ret;
        }
        GTP_ERROR("GTP i2c test failed time %d.", retry);
        msleep(10);
    }
    return ret;
}

/*******************************************************
Function:
    Request gpio(INT & RST) ports.
Input:
    ts: private data.
Output:
    Executive outcomes.
        >= 0: succeed, < 0: failed
*******************************************************/
static s8 gtp_request_io_port(struct goodix_ts_data *ts)
{
    s32 ret = 0;
    //lidbg("=======gtp_request_io_port====gt910=====\n");
    ret = GTP_GPIO_REQUEST(GTP_INT_PORT, "GTP_INT_IRQ");
    if (ret < 0)
    {
        GTP_ERROR("Failed to request GPIO:%d, ERRNO:%d", (s32)GTP_INT_PORT, ret);
        ret = -ENODEV;
    }
    else
    {
        gpio_tlmm_config(GPIO_CFG(GTP_INT_PORT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_8MA), GPIO_CFG_ENABLE);
        gpio_get_value(GTP_INT_PORT);
        //GTP_GPIO_AS_INT(GTP_INT_PORT);
        ts->client->irq = GTP_INT_IRQ;
        //lidbg("=====succeful to request== GT910INTGPIO:====\n");
    }

    ret = GTP_GPIO_REQUEST(GTP_RST_PORT, "GTP_RST_PORT");
    if (ret < 0)
    {
        GTP_ERROR("Failed to request GPIO:%d, ERRNO:%d", (s32)GTP_RST_PORT, ret);
        ret = -ENODEV;
    }

    GTP_GPIO_AS_INPUT(GTP_RST_PORT);
    gtp_reset_guitar(ts->client, 20);
    ret = 1;
    if(ret < 0)
    {
        GTP_GPIO_FREE(GTP_RST_PORT);
        GTP_GPIO_FREE(GTP_INT_PORT);
    }

    return ret;
}

/*******************************************************
Function:
    Request interrupt.
Input:
    ts: private data.
Output:
    Executive outcomes.
        0: succeed, -1: failed.
*******************************************************/
static s8 gtp_request_irq(struct goodix_ts_data *ts)
{
    s32 ret = -1;
    const u8 irq_table[] = GTP_IRQ_TAB;

    GTP_DEBUG("INT trigger type: %s", (ts->int_trigger_type == 1) ? "falling-edge" : "rising-edge");

    ret  = request_irq(ts->client->irq, goodix_ts_irq_handler, irq_table[ts->int_trigger_type], ts->client->name, ts);
    if (ret)
    {
        lidbg("======gtp_request_irq fail========GT910===\n");
    }

    /* if (ret)
     {
         GTP_ERROR("Request IRQ failed!ERRNO:%d.", ret);
         GTP_GPIO_AS_INPUT(GTP_INT_PORT);
         GTP_GPIO_FREE(GTP_INT_PORT);

         hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
         ts->timer.function = goodix_ts_timer_handler;
         hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
         return -1;
     }*/
    else
    {
        //lidbg("====GT910 Request IRQ succseeful======");
        gtp_irq_disable(ts);
        ts->use_irq = 1;
        return 0;
    }
}

/*******************************************************
Function:
    Request input device Function.
Input:
    ts:private data.
Output:
    Executive outcomes.
        0: succeed, otherwise: failed.
*******************************************************/

static s8 gtp_request_input_dev(struct goodix_ts_data *ts)
{
    s8 ret = -1;
    s8 phys[32];
#if GTP_HAVE_TOUCH_KEY
    u8 index = 0;
#endif

    GTP_DEBUG_FUNC();

    ts->input_dev = input_allocate_device();
    if (ts->input_dev == NULL)
    {
        GTP_ERROR("Failed to allocate input device.");
        return -ENOMEM;
    }
    screen_x = SCREEN_X;
    screen_y = SCREEN_Y;
    SOC_Display_Get_Res(&screen_x, &screen_y);
    ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
#if GTP_ICS_SLOT_REPORT
    __set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);
    input_mt_init_slots(ts->input_dev, 10);     // in case of "out of memory"
#else
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#endif

#if GTP_HAVE_TOUCH_KEY
    for (index = 0; index < GTP_MAX_KEY_NUM; index++)
    {
        input_set_capability(ts->input_dev, EV_KEY, touch_key_array[index]);
    }
#endif

#if GTP_SLIDE_WAKEUP
    input_set_capability(ts->input_dev, EV_KEY, KEY_POWER);
#endif

    if(xy_revert_en == 1)
        GTP_SWAP(ts->abs_x_max, ts->abs_y_max);

    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, screen_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, screen_y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);

    sprintf(phys, "input/ts");
    ts->input_dev->name = goodix_ts_name;
    ts->input_dev->phys = phys;
    ts->input_dev->id.bustype = BUS_I2C;
    ts->input_dev->id.vendor = 0xDEAD;
    ts->input_dev->id.product = 0xBEEF;
    ts->input_dev->id.version = 10427;

    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        GTP_ERROR("Register %s input device failed", ts->input_dev->name);
        return -ENODEV;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = goodix_ts_early_suspend;
    ts->early_suspend.resume = goodix_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    return 0;
}
/*******************************************************
Function:
    Competible with GT9M
Input:
    client: client
Output:
    Executive outcomes.
        0: succeed, otherwise: failed.
*******************************************************/
void gtp_get_chiptype(struct i2c_client *client)
{
    struct goodix_ts_data *ts = NULL;
    u8 type_buf[2 + 10]  = {0x80, 0x00};
    s32 ret = 0;

    ts = i2c_get_clientdata(client);
    ret = gtp_i2c_read(client, type_buf, 12);
    if (ret < 0)
    {
        GTP_ERROR("failed to get chiptype");
        return;
    }
    /*
    for (i = 0; i < 32; ++i)
    {
        lidbg("%c", type_buf[2 + i]);
    }
    lidbg("\n");
    */

    if (!memcmp(&type_buf[2], "GT900_MASK", 10))
    {
        GTP_INFO("chip_type: GT900_MASK");
        ts->chip_type = GTP_TYPE_MASK;
    }
    else
    {
        GTP_INFO("chip_type: GT900_FLASHLESS");
        ts->chip_type = GTP_TYPE_FLASHLESS;
    }
}

s32 gtp_fw_startup(struct i2c_client *client)
{
    u8 opr_buf[4];
    s32 ret = 0;

    //init sw WDT
    opr_buf[0] = 0xAA;
    opr_buf[1] = 0xAA;
    ret = i2c_write_bytes(client, 0x8040, opr_buf, 2);
    if (ret < 0)
    {
        return FAIL;
    }
    //release SS51 & DSP
    opr_buf[0] = 0x00;
    ret = i2c_write_bytes(client, 0x4180, opr_buf, 1);
    if (ret < 0)
    {
        return FAIL;
    }
    //int sync
    gtp_int_sync(25);

    //check fw run status
    ret = i2c_read_bytes(client, 0x8041, opr_buf, 1);
    if (ret < 0)
    {
        return FAIL;
    }
    if(0xAA == opr_buf[0])
    {
        GTP_ERROR("IC works abnormally,startup failed.");
        return FAIL;
    }
    else
    {
        GTP_INFO("IC works normally, Startup success.");
        opr_buf[0] = 0xAA;
        opr_buf[1] = 0xAA;
        i2c_write_bytes(client, 0x8040, opr_buf, 2);
        return SUCCESS;
    }
}

static s32 gtp_flashless_init(struct goodix_ts_data *ts)
{
    s32 ret = 0;

    GTP_DEBUG("flashless init");

    gtp_reset_guitar(ts->client, 20);   // to ensure reliability
    gtp_get_chiptype(ts->client);

    // GT900_MASK has no need to burn firmware
    if (GTP_TYPE_FLASHLESS == ts->chip_type)
    {
        ret = gup_update_proc(NULL);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to burn the firmware");
            return FAIL;
        }
        ret = gtp_fw_startup(ts->client);
        if (FAIL == ret)
        {
            GTP_ERROR("failed to startup flashless");
            return FAIL;
        }
    }
    else
    {
        gtp_int_sync(25);
#if GTP_ESD_PROTECT
        gtp_init_ext_watchdog(ts->client);
#endif
    }

    return SUCCESS;
}


/*******************************************************
Function:
    I2c probe.
Input:
    client: i2c device struct.
    id: device id.
Output:
    Executive outcomes.
        0: succeed.
*******************************************************/

static int goodix_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 ret = -1;
    struct goodix_ts_data *ts;
    u16 version_info;

    GTP_DEBUG_FUNC();
    lidbg("=======enter probe====gt910=====\n");
    //do NOT remove these logs
    GTP_INFO("GTP Driver Version: %s", GTP_DRIVER_VERSION);
    GTP_INFO("GTP Driver Built@%s, %s", __TIME__, __DATE__);
    GTP_INFO("GTP I2C Address: 0x%02x", client->addr);
    client->addr = 0x5d;
    i2c_connect_client = client;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        GTP_ERROR("I2C check functionality failed.");
        return -ENODEV;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        GTP_ERROR("Alloc GFP_KERNEL memory failed.");
        return -ENOMEM;
    }

    memset(ts, 0, sizeof(*ts));
    INIT_WORK(&ts->work, goodix_ts_work_func);
    ts->client = client;
    spin_lock_init(&ts->irq_lock);          // 2.6.39 later
    // ts->irq_lock = SPIN_LOCK_UNLOCKED;   // 2.6.39 & before
    i2c_set_clientdata(client, ts);

    ts->gtp_rawdiff_mode = 0;

    ret = gtp_request_io_port(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request IO port failed.");
        kfree(ts);
        return ret;
    }
    else
    {
        lidbg("===gtp_request_io_port====suc=====");
    }
    // flashless
    ret = gtp_flashless_init(ts);
    if (FAIL == ret)
    {
        GTP_ERROR("flashless init failed!");
    }

    ret = gtp_i2c_test(client);
    if (ret < 0)
    {
        GTP_ERROR("I2C communication ERROR!");
    }
    else
    {
        lidbg("======test succseful======GT910===\n");
    }
    ret = gtp_init_panel(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP init panel failed.");
    }
    else
    {
        lidbg("======GTP init panel succseful======GT910===\n");
    }
    ret = gtp_request_input_dev(ts);
    if (ret < 0)
    {
        GTP_ERROR("GTP request input dev failed");
    }
    else
    {
        lidbg("======GTP request input dev  succseful======GT910===\n");
    }
    ret = gtp_request_irq(ts);
    if (ret < 0)
    {
        GTP_INFO("GTP works in polling mode.");
    }
    else
    {
        GTP_INFO("GTP works in interrupt mode.");
    }

    ret = gtp_read_version(client, &version_info);
    if (ret < 0)
    {
        GTP_ERROR("Read version failed.");
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }

#if GTP_CREATE_WR_NODE
    //init_wr_node(client);
#endif

#if GTP_ESD_PROTECT
    gtp_esd_switch(ts->client, SWITCH_ON);
#endif

#if GTP_AUTO_UPDATE
    if (GTP_TYPE_FLASHLESS == ts->chip_type)
    {
        kthread_run(gup_update_proc, "update", "auto update");
    }
#endif
    lidbg("=======gt910=====goodix_ts_probe======\n");
    return 0;
}


/*******************************************************
Function:
    Goodix touchscreen driver release function.
Input:
    client: i2c device struct.
Output:
    Executive outcomes. 0---succeed.
*******************************************************/
static int goodix_ts_remove(struct i2c_client *client)
{
    struct goodix_ts_data *ts = i2c_get_clientdata(client);

    GTP_DEBUG_FUNC();

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&ts->early_suspend);
#endif

#if GTP_CREATE_WR_NODE
    //uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    if (ts)
    {
        if (ts->use_irq)
        {
            GTP_GPIO_AS_INPUT(GTP_INT_PORT);
            GTP_GPIO_FREE(GTP_INT_PORT);
            free_irq(client->irq, ts);
        }
        else
        {
            hrtimer_cancel(&ts->timer);
        }
    }

    GTP_INFO("GTP driver removing...");
    i2c_set_clientdata(client, NULL);
    input_unregister_device(ts->input_dev);
    kfree(ts);

    return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
/*******************************************************
Function:
    Early suspend function.
Input:
    h: early_suspend struct.
Output:
    None.
*******************************************************/
static void goodix_ts_early_suspend(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);

    GTP_DEBUG_FUNC();

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 1;
    gtp_esd_switch(ts->client, SWITCH_OFF);
#endif

#if GTP_SLIDE_WAKEUP
    ret = gtp_enter_doze(ts);
#else
    if (ts->use_irq)
    {
        gtp_irq_disable(ts);
    }
    else
    {
        hrtimer_cancel(&ts->timer);
    }
    ret = gtp_enter_sleep(ts);
#endif
    if (ret < 0)
    {
        GTP_ERROR("GTP early suspend failed.");
    }
    // to avoid waking up while not sleeping
    //  delay 48 + 10ms to ensure reliability
    msleep(58);
}

/*******************************************************
Function:
    Late resume function.
Input:
    h: early_suspend struct.
Output:
    None.
*******************************************************/
static void goodix_ts_late_resume(struct early_suspend *h)
{
    struct goodix_ts_data *ts;
    s8 ret = -1;
    ts = container_of(h, struct goodix_ts_data, early_suspend);

    GTP_DEBUG_FUNC();

    ret = gtp_wakeup_sleep(ts);

#if GTP_SLIDE_WAKEUP
    doze_status = DOZE_DISABLED;
#endif

    if (ret < 0)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    if (ts->use_irq)
    {
        gtp_irq_enable(ts);
    }
    else
    {
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }

#if GTP_ESD_PROTECT
    ts->gtp_is_suspend = 0;
    gtp_esd_switch(ts->client, SWITCH_ON);
#endif
}
#endif

#if GTP_ESD_PROTECT

static s32 gtp_init_ext_watchdog(struct i2c_client *client)
{
    u8 opr_buffer[4] = {0x80, 0x40, 0xAA, 0xAA};
    struct i2c_msg msg;
    s32 ret = -1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();
    GTP_DEBUG("init external watchdog");

    msg.flags = !I2C_M_RD;
    msg.addr  = client->addr;
    msg.len   = 4;
    msg.buf   = opr_buffer;

    while(retries < 5)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if (ret == 1)break;
        retries++;
    }
    return ret;
}

void gtp_esd_switch(struct i2c_client *client, s32 on)
{
    struct goodix_ts_data *ts;

    ts = i2c_get_clientdata(client);
    if (SWITCH_ON == on)     // switch on esd
    {
        if (!ts->esd_running)
        {
            ts->esd_running = 1;
            GTP_INFO("Esd started");
            queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
        }
    }
    else    // switch off esd
    {
        if (ts->esd_running)
        {
            ts->esd_running = 0;
            GTP_INFO("Esd cancelled");
            cancel_delayed_work_sync(&gtp_esd_check_work);
        }
    }
}

/*******************************************************
Function:
    Esd protect function.
    Added external watchdog by meta, 2013/03/07
Input:
    work: delayed work
Output:
    None.
*******************************************************/
static void gtp_esd_check_func(struct work_struct *work)
{
    s32 i;
    s32 ret = -1;
    struct goodix_ts_data *ts = NULL;
    u8 test[4] = {0x80, 0x40};

    GTP_DEBUG_FUNC();

    ts = i2c_get_clientdata(i2c_connect_client);

    if (ts->gtp_is_suspend || ts->enter_update)
    {
        ts->esd_running = 0;
        GTP_INFO("Esd terminated");
        return;
    }
    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_read(ts->client, test, 4);

        GTP_DEBUG("0x8040 = 0x%02X, 0x8041 = 0x%02X", test[2], test[3]);
        if ((ret < 0))
        {
            // IIC communication problem
            continue;
        }
        else
        {
            if ((test[2] == 0xAA) || (test[3] != 0xAA))
            {
                // IC works abnormally..
                GTP_ERROR("0x8040 = 0x%02X, 0x8041 = 0x%02X", test[2], test[3]);
                i = 3;
                break;
            }
            else
            {
                // IC works normally, Write 0x8040 0xAA, feed the dog
                test[2] = 0xAA;
                gtp_i2c_write(ts->client, test, 3);
                break;
            }
        }
    }
    if (i >= 3)
    {
        if (ts->rqst_processing)
        {
            GTP_INFO("request processing, no esd reset");
        }
        else
        {
            GTP_ERROR("IC Working ABNORMALLY, esd recovering...");
            gtp_esd_recovery(ts->client);
        }
    }

    if(!ts->gtp_is_suspend)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, GTP_ESD_CHECK_CIRCLE);
    }
    else
    {
        GTP_INFO("Esd terminated");
        ts->esd_running = 0;
    }
    return;
}
#endif

static const struct i2c_device_id goodix_ts_id[] =
{
    { GTP_I2C_NAME, 0 },
    { }
};

static struct i2c_driver goodix_ts_driver =
{
    .probe      = goodix_ts_probe,
    .remove     = goodix_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend    = goodix_ts_early_suspend,
    .resume     = goodix_ts_late_resume,
#endif
    .id_table   = goodix_ts_id,
    .driver = {
        .name     = GTP_I2C_NAME,
        .owner    = THIS_MODULE,
    },
};
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
    lidbg("gt910-ts_nod_write:==%d====[%s]\n", count, data_rec);
    // processing data
    if(!(strnicmp(data_rec, "TSMODE_XYREVERT", count - 1)))
    {
        xy_revert_en = 1;
        lidbg("[gt910]ts_nod_write:==========TSMODE_XYREVERT\n");
    }
    else if(!(strnicmp(data_rec, "TSMODE_NORMAL", count - 1)))
    {
        xy_revert_en = 0;
        lidbg("[gt910]ts_nod_write:==========TSMODE_NORMAL\n");
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
        lidbg("gt910===========init_cdev_ts:kmalloc err \n");
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
    lidbg("gt910===========alloc_chrdev_region result:%d \n", result);

    cdev_init(&tsdev->cdev, &ts_nod_fops);
    tsdev->cdev.owner = THIS_MODULE;
    tsdev->cdev.ops = &ts_nod_fops;
    err = cdev_add(&tsdev->cdev, dev_number, 1);
    if (err)
        lidbg( "gt910===========Error cdev_add\n");

    //cread cdev node in /dev
    class_install_ts = class_create(THIS_MODULE, "tsnodclass");
    if(IS_ERR(class_install_ts))
    {
        lidbg( "gt910=======class_create err\n");
        return -1;
    }
    device_create(class_install_ts, NULL, dev_number, NULL, "%s%d", TS_DEVICE_NAME, 0);
}
#endif
/*******************************************************
Function:
    Driver Install function.
Input:
    None.
Output:
    Executive Outcomes. 0---succeed.
********************************************************/
static int __devinit goodix_ts_init(void)
{

    LIDBG_GET;

    s32 ret;
    is_ts_load = 1;
#ifdef BOARD_V2
    SOC_IO_Output(0, 27, 1);
    msleep(200);
    SOC_IO_Output(0, 27, 0);//NOTE:GT910 SHUTDOWN PIN ,set hight to work.
    msleep(300);
#else
    SOC_IO_Output(0, 27, 0);
    msleep(200);
    SOC_IO_Output(0, 27, 1);//NOTE:GT910 SHUTDOWN PIN ,set hight to work.
    msleep(300);
#endif
    GTP_DEBUG_FUNC();
    GTP_INFO("=====GTP driver installing...=====gt910");
    //ui_print("===ui==GTP driver installing...=====gt910");
    goodix_wq = create_singlethread_workqueue("goodix_wq");
    if (!goodix_wq)
    {
        GTP_ERROR("Creat workqueue failed.");
        return -ENOMEM;
    }
    else
    {
        lidbg("Creat workqueue succseeful!\n");
    }
#if GTP_ESD_PROTECT
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
#endif

    ret = i2c_add_driver(&goodix_ts_driver);
    init_cdev_ts();
    return ret;
}

/*******************************************************
Function:
    Driver uninstall function.
Input:
    None.
Output:
    Executive Outcomes. 0---succeed.
********************************************************/
static void __exit goodix_ts_exit(void)
{
    GTP_DEBUG_FUNC();
    GTP_INFO("GTP driver exited.");
    i2c_del_driver(&goodix_ts_driver);
    if (goodix_wq)
    {
        destroy_workqueue(goodix_wq);
    }
}

late_initcall(goodix_ts_init);
module_exit(goodix_ts_exit);
//EXPORT_SYMBOL(i2c_connect_client);
MODULE_DESCRIPTION("GTP Series Driver");
MODULE_LICENSE("GPL");
