/* drivers/input/touchscreen/gt9xx_update.c
 *
 * 2010 - 2012 Goodix Technology.
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
 * Version:1.0
 * Author: andrew@goodix.com
 * Revision Record:
 *      V1.0:
 *          first release. By Meta & Andrew, 2013/05/15
 */
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/namei.h>
#include "gt910.h"
#include "gt910fm_firmware.h"
#include <linux/irq.h>
#include <linux/delay.h>


#define GUP_REG_HW_INFO             0x4220
#define GUP_REG_FW_MSG              0x41E4
#define GUP_REG_PID_VID             0x8140
#define FL_UPDATE_PATH              "/flysystem/lib/out/_fm_update_.bin"
#define FL_UPDATE_PATH_SD           "/sdcard/_fm_update_.bin"

#define FW_HEAD_LENGTH               14
#define FW_DOWNLOAD_LENGTH           0x4000
#define FW_SS51_SECTION_LEN          0x2000     // 4 section, each 8k
#define FW_SECTION_LENGTH            0x2000
#define FW_DSP_LENGTH                0x1000

#define PACK_SIZE                    1024       // 1k
#define GUP_FW_CHK_SIZE              PACK_SIZE

#define MAX_CHECK_TIMES              1000

//for clk cal
#define PULSE_LENGTH      (200)
#define INIT_CLK_DAC      (50)
#define MAX_CLK_DAC       (120)
#define CLK_AVG_TIME      (1)
#define MILLION           1000000

#define _bRW_MISCTL__SRAM_BANK                    0x4048
#define _bRW_MISCTL__MEM_CD_EN                    0x4049
#define _bRW_MISCTL__CACHE_EN                     0x404B
#define _bRW_MISCTL__TMR0_EN                      0x40B0
#define _rRW_MISCTL__SWRST_B0_                    0x4180
#define _bWO_MISCTL__CPU_SWRST_PULSE              0x4184
#define _rRW_MISCTL__BOOTCTL_B0_                  0x4190
#define _rRW_MISCTL__BOOT_OPT_B0_                 0x4218
#define _rRW_MISCTL__BOOT_CTL_                    0x5094

#define _wRW_MISCTL__RG_DMY                       0x4282
#define _bRW_MISCTL__RG_OSC_CALIB                 0x4268
#define _fRW_MISCTL__GIO0                         0x41e9
#define _fRW_MISCTL__GIO1                         0x41ed
#define _fRW_MISCTL__GIO2                         0x41f1
#define _fRW_MISCTL__GIO3                         0x41f5
#define _fRW_MISCTL__GIO4                         0x41f9
#define _fRW_MISCTL__GIO5                         0x41fd
#define _fRW_MISCTL__GIO6                         0x4201
#define _fRW_MISCTL__GIO7                         0x4205
#define _fRW_MISCTL__GIO8                         0x4209
#define _fRW_MISCTL__GIO9                         0x420d
#define _fRW_MISCTL__MEA                          0x41a0
#define _bRW_MISCTL__MEA_MODE                     0x41a1
#define _wRW_MISCTL__MEA_MAX_NUM                  0x41a4
#define _dRO_MISCTL__MEA_VAL                      0x41b0
#define _bRW_MISCTL__MEA_SRCSEL                   0x41a3
#define _bRO_MISCTL__MEA_RDY                      0x41a8
#define _rRW_MISCTL__ANA_RXADC_B0_                0x4250
#define _bRW_MISCTL__RG_LDO_A18_PWD               0x426f
#define _bRW_MISCTL__RG_BG_PWD                    0x426a
#define _bRW_MISCTL__RG_CLKGEN_PWD                0x4269
#define _fRW_MISCTL__RG_RXADC_PWD                 0x426a
#define _bRW_MISCTL__OSC_CK_SEL                   0x4030
#define _rRW_MISCTL_RG_DMY83                      0x4283
#define _rRW_MISCTL__GIO1CTL_B2_                  0x41ee
#define _rRW_MISCTL__GIO1CTL_B1_                  0x41ed


#pragma pack(1)
typedef struct
{
    u8  hw_info[4];          //hardware info//
    u8  pid[8];              //product id   //
    u16 vid;                 //version id   //
} st_fw_head;
#pragma pack()

typedef struct
{
    u8 force_update;
    u8 fw_flag;
    struct file *file;
    struct file *cfg_file;
    st_fw_head  ic_fw_msg;
    mm_segment_t old_fs;
} st_update_msg;

st_update_msg update_msg;
u16 show_len;
u16 total_len;

u8 i2c_opr_buf[GTP_ADDR_LENGTH + GUP_FW_CHK_SIZE] = {0};
u8 chk_cmp_buf[GUP_FW_CHK_SIZE] = {0};

extern void gtp_reset_guitar(struct i2c_client *client, s32 ms);
extern s32  gtp_send_cfg(struct i2c_client *client);
extern struct i2c_client *i2c_connect_client;
extern void gtp_irq_enable(struct goodix_ts_data *ts);
extern void gtp_irq_disable(struct goodix_ts_data *ts);
extern s32 gtp_fw_startup(struct i2c_client *client);
static s32 gup_burn_fw_proc(struct i2c_client *client, u16 start_addr, s32 start_index, s32 burn_len);
static s32 gup_check_fw_proc(struct i2c_client *client, u16 start_addr, s32 start_index, s32 burn_len);
extern void gtp_int_sync(s32 ms);


#if GTP_ESD_PROTECT
void gtp_esd_switch(struct i2c_client *, s32);
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
#endif

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
s32 gup_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msgs[2];
    s32 ret = -1;
    s32 retries = 0;

    GTP_DEBUG_FUNC();
    //GTP_DEBUG("i2c read 0x%04X, %d bytes", ((buf[0] << 8) | buf[1]), len - 2);

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
s32 gup_i2c_write(struct i2c_client *client, u8 *buf, s32 len)
{
    struct i2c_msg msg;
    s32 ret = -1;
    s32 retries = 0;
    //lidbg( "====updata ==gup_i2c_write:client===.addr:0x%02X\n",   client->addr );
    GTP_DEBUG_FUNC();
    //GTP_DEBUG("i2c write 0x%04X, %d bytes(0x%02X, 0x%02X)", ((buf[0] << 8) | buf[1]), len - 2, buf[2], ((len > 3) ? buf[3] : 0xFF));

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

    return ret;
}

s32 i2c_write_bytes(struct i2c_client *client, u16 addr, u8 *buf, s32 len)
{
    s32 ret = 0;
    s32 write_bytes = 0;
    s32 retry = 0;
    u8 *tx_buf = buf;

    while (len > 0)
    {
        i2c_opr_buf[0] = (u8)(addr >> 8);
        i2c_opr_buf[1] = (u8)(addr & 0xFF);
        if (len > PACK_SIZE)
        {
            write_bytes = PACK_SIZE;
        }
        else
        {
            write_bytes = len;
        }
        memcpy(i2c_opr_buf + 2, tx_buf, write_bytes);
        for (retry = 0; retry < 5; ++retry)
        {
            ret = gup_i2c_write(client, i2c_opr_buf, write_bytes + GTP_ADDR_LENGTH);
            if (ret == 1)
            {
                break;
            }
        }
        if (retry >= 5)
        {
            GTP_ERROR("retry timeout, I2C write 0x%04X %d bytes failed!", addr, write_bytes);
            return -1;
        }
        addr += write_bytes;
        len -= write_bytes;
        tx_buf += write_bytes;
    }

    return 1;
}

s32 i2c_read_bytes(struct i2c_client *client, u16 addr, u8 *buf, s32 len)
{
    s32 ret = 0;
    s32 read_bytes = 0;
    s32 retry = 0;
    u8 *tx_buf = buf;

    while (len > 0)
    {
        i2c_opr_buf[0] = (u8)(addr >> 8);
        i2c_opr_buf[1] = (u8)(addr & 0xFF);
        if (len > PACK_SIZE)
        {
            read_bytes = PACK_SIZE;
        }
        else
        {
            read_bytes = len;
        }
        for (retry = 0; retry < 5; ++retry)
        {
            ret = gup_i2c_read(client, i2c_opr_buf, read_bytes + GTP_ADDR_LENGTH);
            if (ret == 2)
            {
                break;
            }
        }
        if (retry >= 5)
        {
            GTP_ERROR("retry timeout, I2C read 0x%04X %d bytes failed!", addr, read_bytes);
            return -1;
        }
        memcpy(tx_buf, i2c_opr_buf + 2, read_bytes);
        addr += read_bytes;
        len -= read_bytes;
        tx_buf += read_bytes;
    }
    return 2;
}

static u8 gup_get_ic_msg(struct i2c_client *client, u16 addr, u8 *msg, s32 len)
{
    s32 i = 0;

    msg[0] = (addr >> 8) & 0xff;
    msg[1] = addr & 0xff;

    for (i = 0; i < 5; i++)
    {
        if (gup_i2c_read(client, msg, GTP_ADDR_LENGTH + len) > 0)
        {
            break;
        }
    }

    if (i >= 5)
    {
        GTP_ERROR("Read data from 0x%02x%02x failed!", msg[0], msg[1]);
        return FAIL;
    }

    return SUCCESS;
}

static u8 gup_set_ic_msg(struct i2c_client *client, u16 addr, u8 val)
{
    s32 i = 0;
    u8 msg[3];

    msg[0] = (addr >> 8) & 0xff;
    msg[1] = addr & 0xff;
    msg[2] = val;
    lidbg( "==== gup_set_ic_msg:client===.addr:0x%02X\n",   client->addr );
    for (i = 0; i < 5; i++)
    {
        if (gup_i2c_write(client, msg, GTP_ADDR_LENGTH + 1) > 0)
        {
            break;
        }
    }

    if (i >= 5)
    {
        GTP_ERROR("Set data to 0x%02x%02x failed!", msg[0], msg[1]);
        return FAIL;
    }

    return SUCCESS;
}

s32 gup_hold_ss51_dsp(struct i2c_client *client)
{
    s32 ret = -1;
    s32 retry = 0;
    u8 rd_buf[3];
    lidbg( "==== gup_hold_ss51_dsp:client.addr:0x%02X\n",   client->addr );
    while(retry++ < 200)
    {
        // step4:Hold ss51 & dsp
        ret = gup_set_ic_msg(client, _rRW_MISCTL__SWRST_B0_, 0x0C);
        if(ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);
            continue;
        }

        // step5:Confirm hold
        ret = gup_get_ic_msg(client, _rRW_MISCTL__SWRST_B0_, rd_buf, 1);
        if (ret <= 0)
        {
            GTP_DEBUG("Hold ss51 & dsp I2C error,retry:%d", retry);
            continue;
        }
        if (0x0C == rd_buf[GTP_ADDR_LENGTH])
        {
            GTP_DEBUG("[enter_update_mode]Hold ss51 & dsp confirm SUCCESS");
            break;
        }
        GTP_DEBUG("Hold ss51 & dsp confirm 0x4180 failed,value:%d", rd_buf[GTP_ADDR_LENGTH]);
    }
    if(retry >= 200)
    {
        GTP_ERROR("Enter update Hold ss51 failed.");
        return FAIL;
    }
    //DSP_CK and DSP_ALU_CK PowerOn
    ret = gup_set_ic_msg(client, 0x4010, 0x00);
    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]DSP_CK and DSP_ALU_CK PowerOn fail.");
        return FAIL;
    }

    //disable wdt
    ret = gup_set_ic_msg(client, _bRW_MISCTL__TMR0_EN, 0x00);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]disable wdt fail.");
        return FAIL;
    }

    //clear cache enable
    ret = gup_set_ic_msg(client, _bRW_MISCTL__CACHE_EN, 0x00);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]clear cache enable fail.");
        return FAIL;
    }

    //set boot from sram
    ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOTCTL_B0_, 0x02);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]set boot from sram fail.");
        return FAIL;
    }

    //software reboot
    ret = gup_set_ic_msg(client, _bWO_MISCTL__CPU_SWRST_PULSE, 0x01);
    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]software reboot fail.");
        return FAIL;
    }

    return SUCCESS;
}


s32 gup_enter_update_mode(struct i2c_client *client)
{
    s32 ret = -1;
    //s32 retry = 0;
    //u8 rd_buf[3];
    gtp_reset_guitar(i2c_connect_client, 20);
    //lidbg("====gup_enter_update_mode======");
    //step1:RST output low last at least 2ms
    /* GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);

        msleep(2);

        //step2:select I2C slave addr,INT:0--0xBA;1--0x28.
        GTP_GPIO_OUTPUT(GTP_INT_PORT, (client->addr == 0x14));
        msleep(2);

        //step3:RST output high reset guitar
     GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);

        msleep(6);*/

    //select addr & hold ss51_dsp
    ret = gup_hold_ss51_dsp(client);
    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]hold ss51 & dsp failed.");
        return FAIL;
    }

    //clear control flag
    ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_CTL_, 0x00);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]clear control flag fail.");
        return FAIL;
    }

    //set scramble
    ret = gup_set_ic_msg(client, _rRW_MISCTL__BOOT_OPT_B0_, 0x00);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]set scramble fail.");
        return FAIL;
    }

    //enable accessing code
    ret = gup_set_ic_msg(client, _bRW_MISCTL__MEM_CD_EN, 0x01);

    if (ret <= 0)
    {
        GTP_ERROR("[enter_update_mode]enable accessing code fail.");
        return FAIL;
    }

    return SUCCESS;
    return ret;
}

void gup_leave_update_mode(void)
{
    // GTP_GPIO_AS_INT(GTP_INT_PORT);
    gpio_tlmm_config(GPIO_CFG(GTP_INT_PORT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    gpio_get_value(GTP_INT_PORT);

    GTP_DEBUG("[leave_update_mode]reset chip.");
    gtp_reset_guitar(i2c_connect_client, 20);
}

s32 gup_check_fs_mounted(char *path_name)
{
    struct path root_path;
    struct path path;
    int err;
    err = kern_path("/", LOOKUP_FOLLOW, &root_path);

    if (err)
    {
        GTP_DEBUG("\"/\" NOT Mounted: %d", err);
        return FAIL;
    }
    err = kern_path(path_name, LOOKUP_FOLLOW, &path);

    if (err)
    {
        return FAIL;
    }

    return SUCCESS;

}


static s32 gup_get_ic_fw_msg(struct i2c_client *client)
{
    memcpy(update_msg.ic_fw_msg.hw_info, &gtp_fm_fw[0], 4);

    memset(update_msg.ic_fw_msg.pid, 0, sizeof(update_msg.ic_fw_msg.pid));
    memcpy(update_msg.ic_fw_msg.pid, &gtp_fm_fw[4], 8);

    memcpy((char *)&update_msg.ic_fw_msg.vid, &gtp_fm_fw[12], 2);
    return SUCCESS;
}


/* Update Conditions:
    1. Same hardware info
    2. Same PID
    3. File PID > IC PID
   Force Update Conditions:
    1. INVALID IC PID or VID
    2. IC PID == 91XX || File PID == 91XX
*/
static u8 gup_enter_update_judge(st_fw_head *fw_head)
{
    s32 i = 0;

    GTP_INFO("FILE HARDWARE INFO:%02x%02x%02x%02x", fw_head->hw_info[0], fw_head->hw_info[1], fw_head->hw_info[2], fw_head->hw_info[3]);
    GTP_INFO("FILE PID:%s", fw_head->pid);
    fw_head->vid = ((fw_head->vid & 0xFF00) >> 8) + ((fw_head->vid & 0x00FF) << 8);
    GTP_INFO("FILE VID:%04x", fw_head->vid);

    GTP_INFO("IC HARDWARE INFO:%02x%02x%02x%02x", update_msg.ic_fw_msg.hw_info[0], update_msg.ic_fw_msg.hw_info[1],
             update_msg.ic_fw_msg.hw_info[2], update_msg.ic_fw_msg.hw_info[3]);
    GTP_INFO("IC PID:%s", update_msg.ic_fw_msg.pid);
    GTP_INFO("IC VID:%04x", update_msg.ic_fw_msg.vid);

    //First two conditions
    if (!memcmp(fw_head->hw_info, update_msg.ic_fw_msg.hw_info, sizeof(update_msg.ic_fw_msg.hw_info)))
    {
        GTP_INFO("Get the same hardware info.");
        //        if (update_msg.force_update != 0xBE)
        //        {
        //            GTP_INFO("FW chksum error,need enter update.");
        //            return SUCCESS;
        //        }
        // 20130523 start
        if (strlen(update_msg.ic_fw_msg.pid) < 3)
        {
            GTP_INFO("Illegal IC pid, need enter update");
            return SUCCESS;
        }
        else
        {
            for (i = 0; i < 3; i++)
            {
                if ((update_msg.ic_fw_msg.pid[i] < 0x30) || (update_msg.ic_fw_msg.pid[i] > 0x39))
                {
                    GTP_INFO("Illegal IC pid, out of bound, need enter update");
                    return SUCCESS;
                }
            }
        }
        // 20130523 end


        if ((!memcmp(fw_head->pid, update_msg.ic_fw_msg.pid, (strlen(fw_head->pid) < 3 ? 3 : strlen(fw_head->pid)))) ||
                (!memcmp(update_msg.ic_fw_msg.pid, "91XX", 4)) ||
                (!memcmp(fw_head->pid, "91XX", 4)))
        {
            if(!memcmp(fw_head->pid, "91XX", 4))
            {
                GTP_DEBUG("Force none same pid update mode.");
            }
            else
            {
                GTP_DEBUG("Get the same pid.");
            }

            //The third condition
            if (fw_head->vid > update_msg.ic_fw_msg.vid)
            {

                GTP_INFO("Need enter update.");
                return SUCCESS;
            }

            GTP_INFO("File VID <= IC VID, update aborted!");
        }
        else
        {
            GTP_INFO("Different PID, update aborted!");
        }
    }
    else
    {
        GTP_INFO("Different Hardware Info, update aborted!");
    }

    return FAIL;
}
static s32 gup_prepare_fl_fw(char *path, st_fw_head *fw_head)
{
    s32 ret = 0;
    s32 i = 0;
    struct goodix_ts_data *ts = i2c_get_clientdata(i2c_connect_client);

    if (!memcmp(path, "update", 6))
    {
        GTP_INFO("Search for Flashless firmware file to update");

        for (i = 0; i < 50; ++i)
        {
            GTP_DEBUG("Search for %s (%d/50)", FL_UPDATE_PATH, i + 1);
            update_msg.file = filp_open(FL_UPDATE_PATH, O_RDONLY, 0);
            if (IS_ERR(update_msg.file))
            {
                update_msg.file = filp_open(FL_UPDATE_PATH_SD, O_RDONLY, 0);
                GTP_DEBUG("Search for %s (%d/50)", FL_UPDATE_PATH_SD, i + 1);
                if (IS_ERR(update_msg.file))
                {
                    msleep(3000);
                    continue;
                }
                else
                {
                    path = FL_UPDATE_PATH_SD;
                    break;
                }
            }
            else
            {
                path = FL_UPDATE_PATH;
                break;
            }
        }
        if (i == 50)
        {
            GTP_INFO("Search timeout, update aborted");
            return FAIL;
        }
        else
        {
            GTP_INFO("Flashless firmware file %s found!", path);
            filp_close(update_msg.file, NULL);
        }
        while (ts->rqst_processing)
        {
            GTP_DEBUG("request processing, waiting for accomplishment");
            msleep(1000);
        }
    }
    GTP_INFO("Firmware update file path: %s", path);

    update_msg.file = filp_open(path, O_RDONLY, 0);

    if (IS_ERR(update_msg.file))
    {
        GTP_ERROR("Open update file(%s) error!", path);
        return FAIL;
    }
    ret = gup_get_ic_fw_msg(i2c_connect_client);
    if (FAIL == ret)
    {
        GTP_ERROR("failed to get ic firmware info");
        filp_close(update_msg.file, NULL);
        return FAIL;
    }

    update_msg.old_fs = get_fs();
    set_fs(KERNEL_DS);
    update_msg.file->f_op->llseek(update_msg.file, 0, SEEK_SET);
    update_msg.file->f_op->read(update_msg.file, (char *)fw_head, FW_HEAD_LENGTH, &update_msg.file->f_pos);

    ret = gup_enter_update_judge(fw_head);
    if (FAIL == ret)
    {
        set_fs(update_msg.old_fs);
        filp_close(update_msg.file, NULL);
        return FAIL;
    }

    update_msg.file->f_op->llseek(update_msg.file, 0, SEEK_SET);
    ret = update_msg.file->f_op->read(update_msg.file, (char *)gtp_fm_fw,
                                      FW_HEAD_LENGTH + 2 * FW_DOWNLOAD_LENGTH + FW_DSP_LENGTH,
                                      &update_msg.file->f_pos);
    set_fs(update_msg.old_fs);
    filp_close(update_msg.file, NULL);

    if (ret < 0)
    {
        GTP_ERROR("read %s failed, err-code: %d", path, ret);
        return FAIL;
    }
    return SUCCESS;
}
static u8 gup_check_update_file(struct i2c_client *client, st_fw_head *fw_head, char *path)
{
    s32 ret = 0;
    s32 i = 0;
    s32 fw_checksum = 0;

    if (NULL != path)
    {
        ret = gup_prepare_fl_fw(path, fw_head);
        if (FAIL == ret)
        {
            return FAIL;
        }
    }
    else
    {
        memcpy(fw_head, gtp_fm_fw, FW_HEAD_LENGTH);
        GTP_INFO("FILE HARDWARE INFO: %02x%02x%02x%02x", fw_head->hw_info[0], fw_head->hw_info[1], fw_head->hw_info[2], fw_head->hw_info[3]);
        GTP_INFO("FILE PID: %s", fw_head->pid);
        fw_head->vid = ((fw_head->vid & 0xFF00) >> 8) + ((fw_head->vid & 0x00FF) << 8);
        GTP_INFO("FILE VID: %04x", fw_head->vid);
    }

    //check firmware legality
    fw_checksum = 0;
    for(i = FW_HEAD_LENGTH; i < (FW_HEAD_LENGTH + FW_SECTION_LENGTH * 4 + FW_DSP_LENGTH); i += 2)
    {
        fw_checksum += (gtp_fm_fw[i] << 8) + gtp_fm_fw[i + 1];
    }
    ret = SUCCESS;

    GTP_DEBUG("firmware checksum: %x", fw_checksum & 0xFFFF);
    if (fw_checksum & 0xFFFF)
    {
        GTP_ERROR("Illegal firmware file.");
        ret = FAIL;
    }

    return ret;
}


static u8 gup_download_fw_dsp(struct i2c_client *client, u8 dwn_mode)
{
    s32 ret = 0;

    //step1:select bank2
    GTP_DEBUG("[download_fw_dsp]step1:select bank2");
    ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, 0x02);
    if (ret == FAIL)
    {
        GTP_ERROR("select bank 2 fail");
        return FAIL;
    }

    if (GTP_FL_FW_BURN == dwn_mode)
    {
        GTP_INFO("[download_fw_dsp]Begin download dsp fw---->>");

        if (ret <= 0)
        {
            GTP_ERROR("[download_fw_dsp]select bank2 fail.");
            return FAIL;
        }
        GTP_DEBUG("burn fw dsp");
        ret = gup_burn_fw_proc(client, 0xC000, 2 * FW_DOWNLOAD_LENGTH, FW_DSP_LENGTH); // write the second ban
        if (FAIL == ret)
        {
            GTP_ERROR("[download_fw_dsp]download FW dsp fail.");
            return FAIL;
        }
        GTP_INFO("check firmware dsp");
        ret = gup_check_fw_proc(client, 0xC000, 2 * FW_DOWNLOAD_LENGTH, FW_DSP_LENGTH);
        if (FAIL == ret)
        {
            GTP_ERROR("check fw dsp failed!");
            return FAIL;
        }
    }
    else
    {
        GTP_INFO("[download_fw_dsp]Begin esd check dsp fw---->>");
        //GTP_INFO("esd recovery: check fw dsp");
        //ret = gup_check_fw_proc(client, 0xC000, 2 * FW_DOWNLOAD_LENGTH, FW_DSP_LENGTH);

        //if(FAIL == ret)
        {
            //GTP_ERROR("[download_fw_dsp]Checked FW dsp fail, redownload fw dsp");
            GTP_INFO("esd recovery redownload firmware dsp code");
            ret = gup_burn_fw_proc(client, 0xC000, 2 * FW_DOWNLOAD_LENGTH, FW_DSP_LENGTH);
            if (FAIL == ret)
            {
                GTP_ERROR("redownload fw dsp failed!");
                return FAIL;
            }
        }
    }
    return SUCCESS;
}

static s32 gup_burn_fw_proc(struct i2c_client *client, u16 start_addr, s32 start_index, s32 burn_len)
{
    s32 ret = 0;

    GTP_DEBUG("burn firmware: 0x%04X, %d bytes, start_index: 0x%04X", start_addr, burn_len, start_index);

    ret = i2c_write_bytes(client, start_addr, (u8 *)&gtp_fm_fw[FW_HEAD_LENGTH + start_index], burn_len);
    if (ret < 0)
    {
        GTP_ERROR("burn 0x%04X, %d bytes failed!", start_addr, burn_len);
        return FAIL;
    }
    return SUCCESS;
}

static s32 gup_check_fw_proc(struct i2c_client *client, u16 start_addr, s32 start_index, s32 chk_len)
{
    s32 ret = 0;
    s32 cmp_len = 0;
    u16 cmp_addr = start_addr;
    s32 i = 0;
    u8 chk_fail = 0;

    GTP_DEBUG("check firmware: start 0x%04X, %d bytes", start_addr, chk_len);
    while (chk_len > 0)
    {
        if (chk_len >= GUP_FW_CHK_SIZE)
        {
            cmp_len = GUP_FW_CHK_SIZE;
        }
        else
        {
            cmp_len = chk_len;
        }
        ret = i2c_read_bytes(client, cmp_addr, chk_cmp_buf, cmp_len);
        if (ret < 0)
        {
            chk_fail = 1;
            break;
        }
        for (i = 0; i < cmp_len; ++i)
        {
            if (chk_cmp_buf[i] != gtp_fm_fw[FW_HEAD_LENGTH + start_index + i])
            {
                chk_fail = 1;
                GTP_ERROR("Check failed index: %d(%d != %d)", i, chk_cmp_buf[i],
                          gtp_fm_fw[FW_HEAD_LENGTH + start_index + i]);
                break;
            }
        }
        if (chk_fail == 1)
        {
            break;
        }
        cmp_addr += cmp_len;
        start_index += cmp_len;
        chk_len -= cmp_len;
    }

    if (1 == chk_fail)
    {
        GTP_ERROR("cmp_addr: 0x%04X, start_index: 0x%02X, chk_len: 0x%04X", cmp_addr,
                  start_index, chk_len);
        return FAIL;
    }
    return SUCCESS;
}

static u8 gup_download_fw_ss51(struct i2c_client *client, u8 dwn_mode)
{
    s32 section = 0;
    s32 ret = 0;
    s32 start_index = 0;
    u8  bank = 0;
    u16 burn_addr = 0xC000;

    GTP_INFO("download firmware ss51");
    for (section = 1; section <= 4; section += 2)
    {
        switch (section)
        {
        case 1:
        case 2:
            bank = 0x00;
            burn_addr = (section - 1) * FW_SS51_SECTION_LEN + 0xC000;
            break;
        case 3:
        case 4:
            bank = 0x01;
            burn_addr = (section - 3) * FW_SS51_SECTION_LEN + 0xC000;
            break;
        }
        start_index = (section - 1) * FW_SS51_SECTION_LEN;

        GTP_DEBUG("download firmware ss51: select bank%d", bank);
        ret = gup_set_ic_msg(client, _bRW_MISCTL__SRAM_BANK, bank);
        if (GTP_FL_FW_BURN == dwn_mode)
        {
            GTP_INFO("download firmware ss51 section%d & %d", section, section + 1);
            ret = gup_burn_fw_proc(client, burn_addr, start_index, 2 * FW_SS51_SECTION_LEN);
            if (ret == FAIL)
            {
                GTP_ERROR("download fw ss51 section%d & %d failed!", section, section + 1);
                return FAIL;
            }
            GTP_INFO("check firmware ss51 section%d & %d", section, section + 1);
            ret = gup_check_fw_proc(client, burn_addr, start_index, 2 * FW_SS51_SECTION_LEN);
            if (ret == FAIL)
            {
                GTP_ERROR("check ss51 section%d & %d failed!", section, section + 1);
                return FAIL;
            }
        }
        else // esd recovery mode
        {
            // GTP_INFO("esd recovery check ss51 section%d & %d", section, section+1);
            // ret = gup_check_fw_proc(client, burn_addr, start_index, FW_SS51_SECTION_LEN);
            // if (ret == FAIL)
            {
                // GTP_ERROR("check ss51 section%d failed, redownload section%d", section, section);
                GTP_INFO("esd recovery redownload ss51 section%d & %d", section, section + 1);
                ret = gup_burn_fw_proc(client, burn_addr, start_index, 2 * FW_SS51_SECTION_LEN);
                if (ret == FAIL)
                {
                    GTP_ERROR("download fw ss51 section%d failed!", section);
                    return FAIL;
                }
            }
        }
    }

    return SUCCESS;
}

s32 gup_fw_download_proc(void *dir, u8 dwn_mode)
{
    s32 ret = 0;
    u8  retry = 0;
    st_fw_head fw_head;
    struct goodix_ts_data *ts;

    ts = i2c_get_clientdata(i2c_connect_client);
    //lidbg( " client.addr:0x%02X\n",   client->addr );
    if (NULL == dir)
    {
        if(GTP_FL_FW_BURN == dwn_mode)       // flashless firmware burn mode
        {
            GTP_INFO("[fw_download_proc]Begin fw download ......");
        }
        else        // GTP_FL_ESD_RECOVERY: flashless esd recovery mode
        {
            GTP_INFO("[fw_download_proc]Begin fw esd recovery check ......");
        }
    }

    else
    {
        GTP_INFO("[fw_download_proc]Begin firmware update by bin file");
    }

    total_len = 100;
    show_len = 0;

    ret = gup_check_update_file(i2c_connect_client, &fw_head, (char *)dir);
    show_len = 10;

    if (FAIL == ret)
    {
        GTP_ERROR("[fw_download_proc]check update file fail.");
        goto file_fail;
    }

    gtp_irq_disable(ts);
    // ts->enter_update = 1;
#if GTP_ESD_PROTECT
    if (NULL != dir)
    {
        gtp_esd_switch(ts->client, SWITCH_OFF);
    }
#endif

    ret = gup_enter_update_mode(i2c_connect_client);
    show_len = 20;
    if (FAIL == ret)
    {
        GTP_ERROR("[fw_download_proc]enter update mode fail.");
        goto download_fail;
    }

    while (retry++ < 5)
    {
        ret = gup_download_fw_ss51(i2c_connect_client, dwn_mode);
        show_len = 60;
        if (FAIL == ret)
        {
            GTP_ERROR("[fw_download_proc]burn ss51 firmware fail.");
            continue;
        }

        ret = gup_download_fw_dsp(i2c_connect_client, dwn_mode);
        show_len = 80;
        if (FAIL == ret)
        {
            GTP_ERROR("[fw_download_proc]burn dsp firmware fail.");
            continue;
        }

        GTP_INFO("[fw_download_proc]UPDATE SUCCESS.");
        break;
    }

    if (retry >= 5)
    {
        GTP_ERROR("[fw_download_proc]retry timeout,UPDATE FAIL.");
        goto download_fail;
    }

    //ts->enter_udpate = 0;
    if (NULL != dir)
    {
        gtp_fw_startup(ts->client);
#if GTP_ESD_PROTECT
        gtp_esd_switch(ts->client, SWITCH_ON);
#endif
    }
    gtp_irq_enable(ts);
    show_len = 100;
    return SUCCESS;

download_fail:
    if (NULL != dir)
    {
        gtp_fw_startup(ts->client);
#if GTP_ESD_PROTECT
        gtp_esd_switch(ts->client, SWITCH_ON);
#endif
    }
file_fail:
    gtp_irq_enable(ts);
    show_len = 200;

    //ts->enter_udpate = 0;
    return FAIL;
}

s32 gup_update_proc(void *dir)
{
    return gup_fw_download_proc(dir, GTP_FL_FW_BURN);
}


// main clock calibration
// bit: 0~7, val: 0/1
static void gup_bit_write(s32 addr, s32 bit, s32 val)
{
    u8 buf;
    i2c_read_bytes(i2c_connect_client, addr, &buf, 1);

    buf = (buf & (~((u8)1 << bit))) | ((u8)val << bit);

    i2c_write_bytes(i2c_connect_client, addr, &buf, 1);
}

static void gup_clk_count_init(s32 bCh, s32 bCNT)
{
    u8 buf;

    //_fRW_MISCTL__MEA_EN = 0; //Frequency measure enable
    gup_bit_write(_fRW_MISCTL__MEA, 0, 0);
    //_fRW_MISCTL__MEA_CLR = 1; //Frequency measure clear
    gup_bit_write(_fRW_MISCTL__MEA, 1, 1);
    //_bRW_MISCTL__MEA_MODE = 0; //Pulse mode
    buf = 0;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__MEA_MODE, &buf, 1);
    //_bRW_MISCTL__MEA_SRCSEL = 8 + bCh; //From GIO1
    buf = 8 + bCh;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__MEA_SRCSEL, &buf, 1);
    //_wRW_MISCTL__MEA_MAX_NUM = bCNT; //Set the Measure Counts = 1
    buf = bCNT;
    i2c_write_bytes(i2c_connect_client, _wRW_MISCTL__MEA_MAX_NUM, &buf, 1);
    //_fRW_MISCTL__MEA_CLR = 0; //Frequency measure not clear
    gup_bit_write(_fRW_MISCTL__MEA, 1, 0);
    //_fRW_MISCTL__MEA_EN = 1;
    gup_bit_write(_fRW_MISCTL__MEA, 0, 1);
}

static u32 gup_clk_count_get(void)
{
    s32 ready = 0;
    s32 temp;
    s8  buf[4];

    while ((ready == 0)) //Wait for measurement complete
    {
        i2c_read_bytes(i2c_connect_client, _bRO_MISCTL__MEA_RDY, buf, 1);
        ready = buf[0];
    }

    msleep(50);

    //_fRW_MISCTL__MEA_EN = 0;
    gup_bit_write(_fRW_MISCTL__MEA, 0, 0);
    i2c_read_bytes(i2c_connect_client, _dRO_MISCTL__MEA_VAL, buf, 4);
    GTP_DEBUG("Clk_count 0: %2X", buf[0]);
    GTP_DEBUG("Clk_count 1: %2X", buf[1]);
    GTP_DEBUG("Clk_count 2: %2X", buf[2]);
    GTP_DEBUG("Clk_count 3: %2X", buf[3]);

    temp = (s32)buf[0] + ((s32)buf[1] << 8) + ((s32)buf[2] << 16) + ((s32)buf[3] << 24);
    GTP_INFO("Clk_count : %d", temp);
    return temp;
}
u8 gup_clk_dac_setting(int dac)
{
    s8 buf1, buf2;

    i2c_read_bytes(i2c_connect_client, _wRW_MISCTL__RG_DMY, &buf1, 1);
    i2c_read_bytes(i2c_connect_client, _bRW_MISCTL__RG_OSC_CALIB, &buf2, 1);

    buf1 = (buf1 & 0xFFCF) | ((dac & 0x03) << 4);
    buf2 = (dac >> 2) & 0x3f;

    i2c_write_bytes(i2c_connect_client, _wRW_MISCTL__RG_DMY, &buf1, 1);
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__RG_OSC_CALIB, &buf2, 1);

    return 0;
}

static u8 gup_clk_calibration_pin_select(s32 bCh)
{
    s32 i2c_addr;

    switch (bCh)
    {
    case 0:
        i2c_addr = _fRW_MISCTL__GIO0;
        break;

    case 1:
        i2c_addr = _fRW_MISCTL__GIO1;
        break;

    case 2:
        i2c_addr = _fRW_MISCTL__GIO2;
        break;

    case 3:
        i2c_addr = _fRW_MISCTL__GIO3;
        break;

    case 4:
        i2c_addr = _fRW_MISCTL__GIO4;
        break;

    case 5:
        i2c_addr = _fRW_MISCTL__GIO5;
        break;

    case 6:
        i2c_addr = _fRW_MISCTL__GIO6;
        break;

    case 7:
        i2c_addr = _fRW_MISCTL__GIO7;
        break;

    case 8:
        i2c_addr = _fRW_MISCTL__GIO8;
        break;

    case 9:
        i2c_addr = _fRW_MISCTL__GIO9;
        break;
    }

    gup_bit_write(i2c_addr, 1, 0);

    return 0;
}

void gup_output_pulse(int t)
{
    unsigned long flags;
    //s32 i;

    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(10);

    local_irq_save(flags);

    GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
    msleep(50);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(t - 50);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);

    local_irq_restore(flags);

    msleep(20);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
}

static void gup_sys_clk_init(void)
{
    u8 buf;

    //_fRW_MISCTL__RG_RXADC_CKMUX = 0;
    gup_bit_write(_rRW_MISCTL__ANA_RXADC_B0_, 5, 0);
    //_bRW_MISCTL__RG_LDO_A18_PWD = 0; //DrvMISCTL_A18_PowerON
    buf = 0;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__RG_LDO_A18_PWD, &buf, 1);
    //_bRW_MISCTL__RG_BG_PWD = 0; //DrvMISCTL_BG_PowerON
    buf = 0;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__RG_BG_PWD, &buf, 1);
    //_bRW_MISCTL__RG_CLKGEN_PWD = 0; //DrvMISCTL_CLKGEN_PowerON
    buf = 0;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__RG_CLKGEN_PWD, &buf, 1);
    //_fRW_MISCTL__RG_RXADC_PWD = 0; //DrvMISCTL_RX_ADC_PowerON
    gup_bit_write(_rRW_MISCTL__ANA_RXADC_B0_, 0, 0);
    //_fRW_MISCTL__RG_RXADC_REF_PWD = 0; //DrvMISCTL_RX_ADCREF_PowerON
    gup_bit_write(_rRW_MISCTL__ANA_RXADC_B0_, 1, 0);
    //gup_clk_dac_setting(60);
    //_bRW_MISCTL__OSC_CK_SEL = 1;;
    buf = 1;
    i2c_write_bytes(i2c_connect_client, _bRW_MISCTL__OSC_CK_SEL, &buf, 1);
}

s32 gup_clk_calibration(void)
{
    u8 buf;
    //u8 trigger;
    s32 i;
    struct timeval start, end;
    s32 count;
    s32 count_ref;
    s32 sec;
    s32 usec;
    //unsigned long flags;

    buf = 0x0C; // hold ss51 and dsp
    i2c_write_bytes(i2c_connect_client, _rRW_MISCTL__SWRST_B0_, &buf, 1);

    //_fRW_MISCTL__CLK_BIAS = 0; //disable clock bias
    gup_bit_write(_rRW_MISCTL_RG_DMY83, 7, 0);

    //_fRW_MISCTL__GIO1_PU = 0; //set TOUCH INT PIN MODE as input
    gup_bit_write(_rRW_MISCTL__GIO1CTL_B2_, 0, 0);

    //_fRW_MISCTL__GIO1_OE = 0; //set TOUCH INT PIN MODE as input
    gup_bit_write(_rRW_MISCTL__GIO1CTL_B1_, 1, 0);

    //buf = 0x00;
    //i2c_write_bytes(i2c_connect_client, _rRW_MISCTL__SWRST_B0_, &buf, 1);
    //msleep(1000);

    GTP_INFO("CLK calibration GO");
    gup_sys_clk_init();
    gup_clk_calibration_pin_select(1);//use GIO1 to do the calibration

    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);

    for (i = INIT_CLK_DAC; i < MAX_CLK_DAC; i++)
    {
        GTP_INFO("CLK calibration DAC %d", i);

        gup_clk_dac_setting(i);
        gup_clk_count_init(1, CLK_AVG_TIME);

#if 0
        gup_output_pulse(PULSE_LENGTH);
        count = gup_clk_count_get();

        if (count > PULSE_LENGTH * 60)//60= 60Mhz * 1us
        {
            break;
        }

#else
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);

        //local_irq_save(flags);
        do_gettimeofday(&start);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
        //local_irq_restore(flags);

        msleep(1);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
        msleep(1);

        //local_irq_save(flags);
        do_gettimeofday(&end);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
        //local_irq_restore(flags);

        count = gup_clk_count_get();
        msleep(20);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);

        usec = end.tv_usec - start.tv_usec;
        sec = end.tv_sec - start.tv_sec;
        count_ref = 60 * (usec + sec * MILLION); //60= 60Mhz * 1us

        GTP_DEBUG("== time %d, %d, %d", sec, usec, count_ref);

        if (count > count_ref)
        {
            GTP_DEBUG("== count_diff %d", count - count_ref);
            break;
        }

#endif
    }

    //clk_dac = i;

    gtp_reset_guitar(i2c_connect_client, 20);

#if 0//for debug
    //-- ouput clk to GPIO 4
    buf = 0x00;
    i2c_write_bytes(i2c_connect_client, 0x41FA, &buf, 1);
    buf = 0x00;
    i2c_write_bytes(i2c_connect_client, 0x4104, &buf, 1);
    buf = 0x00;
    i2c_write_bytes(i2c_connect_client, 0x4105, &buf, 1);
    buf = 0x00;
    i2c_write_bytes(i2c_connect_client, 0x4106, &buf, 1);
    buf = 0x01;
    i2c_write_bytes(i2c_connect_client, 0x4107, &buf, 1);
    buf = 0x06;
    i2c_write_bytes(i2c_connect_client, 0x41F8, &buf, 1);
    buf = 0x02;
    i2c_write_bytes(i2c_connect_client, 0x41F9, &buf, 1);
#endif

    //GTP_GPIO_AS_INT(GTP_INT_PORT);
    gpio_tlmm_config(GPIO_CFG(GTP_INT_PORT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,  GPIO_CFG_8MA), GPIO_CFG_ENABLE);
    gpio_get_value(GTP_INT_PORT);
    return i;
}