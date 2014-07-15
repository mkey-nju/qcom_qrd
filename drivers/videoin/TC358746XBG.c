#include "lidbg.h"
#include "i2c_io.h"
#include "tw9912.h"
#include "tc358746xbg_config.h"
#include "TC358746XBG.h"
#define I2C_US_IO
#define APAT_BUS_ID 1

struct tc358746xbg_register_t tc358746xbg_show_colorbar_config_user_tab[] =
{
    //80 pixel of hui
    {0x00e8, 0x307f, register_value_width_16},
    {0x00e8, 0x307f, register_value_width_16},
};

void tc358746xbg_hardware_reset(void)
{
    tc358_RESX_DOWN;
    msleep(20);
    tc358_RESX_UP;
    msleep(20);
}

static void tc358746xbg_power_contorl(void)
{
    RTC358_CS_DOWN;
    tc358_MSEL_UP;//NULL now  1: Par_in -> CSI-2 TX
    tc358746xbg_hardware_reset();
}

i2c_ack tc358746xbg_read(u16 add, char *buf, u8 flag)
{
    i2c_ack ret = NACK;
    if(flag == register_value_width_16)
    {
        ret =  i2c_read_2byte(APAT_BUS_ID, TC358746_I2C_ChipAdd, add, buf, 2);
    }
    else if (flag == register_value_width_32)
    {
        ret =  i2c_read_2byte(APAT_BUS_ID, TC358746_I2C_ChipAdd, add, buf, 4);
    }
    else
        tc358746_dbg("you set regitset value width have error\n");
    return ret;
}

i2c_ack tc358746xbg_write(u16 *add, u32 *valu, u8 flag)
{
    i2c_ack ret;
    u8 BUF_tc358[6]; // address 2 valu=4;
    if(flag == register_value_width_16)
    {
        BUF_tc358[0] = ((*add) >> 8) & 0xff; //add MSB first
        BUF_tc358[1] = (*add) & 0xff;

        BUF_tc358[2] = (((u16) * valu) >> 8) & 0xff;
        BUF_tc358[3] = ((u16) * valu) & 0xff;
        ret = i2c_write_2byte(APAT_BUS_ID, TC358746_I2C_ChipAdd, BUF_tc358, 4);
    }
    else if (flag == register_value_width_32)
    {
        BUF_tc358[0] = (*add >> 8) & 0xff; //add MSB first
        BUF_tc358[1] = (*add) & 0xff;

        BUF_tc358[4] = (*valu >> 24) & 0xff; //32bit data valu MSB
        BUF_tc358[5] = (*valu >> 16) & 0xff;
        BUF_tc358[2] = (*valu >> 8) & 0xff;
        BUF_tc358[3] = (*valu) & 0xff;
        ret = i2c_write_2byte(APAT_BUS_ID, TC358746_I2C_ChipAdd, BUF_tc358, 6);
    }
    else
    {
        tc358746_dbg("you set regitset value width have error\n");
        return NACK;
    }
    return ret;
}

static void tc358746xbg_software_reset(void)
{
    u16 add = 0x0002;
    u32 valu = 1;
    tc358746xbg_write(&add, &valu, register_value_width_16	); //reset
    msleep(1);
    valu = 0;
    tc358746xbg_write(&add, &valu, register_value_width_16	); //normal
    msleep(1);
}

static int tc358746xbg_config_array(const struct tc358746xbg_register_t *TC358746_init_tab)
{
    i2c_ack back_ret;
    register int i = 0;
    while(TC358746_init_tab[i].add_reg != 0xffff)//write
    {

        back_ret =  tc358746xbg_write((u16 *) & (TC358746_init_tab[i].add_reg), \
                                      (u32 *) & (TC358746_init_tab[i].add_val), TC358746_init_tab[i].registet_width);
        if(back_ret  == NACK) goto NACK_BREAK;

        i++;
    }
    return 0;
NACK_BREAK:
    lidbg("interuppt config because TC358746 NACK\n");
    return -1;
}

static int tc358746xbg_get_id(void)
{
    u8 valu[4];
    int ret;

    int  retry;
    for(retry = 0; retry < 5; retry++)
    {
        tc358746xbg_read((tc358746_id[0].add_reg), valu, tc358746_id[0].registet_width);
        if(valu[0] == tc358746_id[0].add_val >> 8)
        {
            lidbg("TC358746xbg ID=%02x%02x\n", valu[0], valu[1]);
            ret = 1;
        }
        else
        {
            lidbg("TC358746xbg Read Back ID=0x%02x%02x\n", valu[0], valu[1]);
            lidbg("****************error TC358746xbg devieces is not fond********************\n");
            ret = -1;
        }
        if (ret < 0)
        {
            msleep(50);
            continue;
        }
        else
            return ret;
    }
    return ret;
}

void tc358746xbg_data_out_enable(u8 flag)
{
    u16 sub_addr = 0x4;
    u32 valu;
    if(flag == 1)
    {
        lidbg("TC358 output enable\n");
        valu = 0x1044; //enable
        tc358746xbg_write(&sub_addr, &valu, register_value_width_16);
    }
    else
    {
        lidbg("TC358 output disable\n");
        valu = 0x1014; //disable
        tc358746xbg_write(&sub_addr, &valu, register_value_width_16);
    }
}

void tc358746xbg_show_colorbar_config(void)
{
    u16 add_reg_1;
    u32 add_val_1;
    u8 i;
    register u8 j;
    i2c_ack ret;
    ret = tc358746xbg_config_array(toshiba_initall_list);
    if(ret == NACK) goto NACK_BREAK;
    lidbg("\n\nTC358746:parameter is toshiba_initall_list!\n");
    for(i = 4; i < 7; i += 2)
    {
        for(j = 180; j > 0; j--)
        {
            ret = tc358746xbg_write((u16 *) & (tc358746xbg_show_colorbar_config_tab[2*i].add_reg), (u32 *) & (tc358746xbg_show_colorbar_config_tab[2*i].add_val), (u8)tc358746xbg_show_colorbar_config_tab[2*i].registet_width);
            if(ret == NACK) goto NACK_BREAK;
            ret = tc358746xbg_write((u16 *) & (tc358746xbg_show_colorbar_config_tab[2*i+1].add_reg), (u32 *) & (tc358746xbg_show_colorbar_config_tab[2*i+1].add_val), (u8)tc358746xbg_show_colorbar_config_tab[2*i+1].registet_width);
            if(ret == NACK) goto NACK_BREAK;
        }
    }
    add_reg_1 = 0x00e0; //使能colobar
    add_val_1 = 0xc1df;
    tc358746xbg_write((u16 *)&add_reg_1, (u32 *)&add_val_1, (u8) register_value_width_16);
NACK_BREAK:
    ;
}

void tc358746xbg_show_colorbar_config_blue(u8 color_flag)
{
    u16 add_reg_1;
    u32 add_val_1;
    u16 i;
    register u16 j;
    int ret_back;
    i2c_ack ret;
    lidbg("TC358746:parameter is toshiba_initall_list!\n");
    ret_back = tc358746xbg_config_array(toshiba_initall_list);
    if(ret_back == -1) goto NACK_BREAK;

    i = color_flag - 1;
    for(j = 360; j > 0; j--)
    {
        ret = tc358746xbg_write((u16 *) & (tc358746xbg_show_colorbar_config_tab[2*i].add_reg), (u32 *) & (tc358746xbg_show_colorbar_config_tab[2*i].add_val), (u8)tc358746xbg_show_colorbar_config_tab[2*i].registet_width);
        if(ret == NACK) goto NACK_BREAK;
        ret = tc358746xbg_write((u16 *) & (tc358746xbg_show_colorbar_config_tab[2*i+1].add_reg), (u32 *) & (tc358746xbg_show_colorbar_config_tab[2*i+1].add_val), (u8)tc358746xbg_show_colorbar_config_tab[2*i+1].registet_width);
        if(ret == NACK) goto NACK_BREAK;
    }
    add_reg_1 = 0x00e0; //使能colobar
    add_val_1 = 0xc1df;
    tc358746xbg_write(&add_reg_1, &add_val_1, register_value_width_16);
    return ;
NACK_BREAK:
    lidbg("ERROR TC358 NACK :%s\n", __func__);
}

static void tc358746xbg_show_colorbar_config_grey(void)
{
    u16 add_reg_1;
    u32 add_val_1;
    int ret_back;
    lidbg("TC358746:parameter is toshiba_initall_list!\n");
    ret_back = tc358746xbg_config_array(toshiba_initall_list);
    if(ret_back == -1) goto NACK_BREAK;

    add_reg_1 = 0x00e0; //使能colobar
    add_val_1 = 0xc1df;
    tc358746xbg_write(&add_reg_1, &add_val_1, register_value_width_16);
    return ;
NACK_BREAK:
    lidbg("ERROR TC358 NACK :%s\n", __func__);
}

static void tc358746xbg_show_colorbar_config_user(void)
{
    u16 add_reg_1;
    u32 add_val_1;
    int ret_back;
    register u16 j;
    i2c_ack ret;
    lidbg("TC358746:parameter is toshiba_initall_list!\n");
    ret_back = tc358746xbg_config_array(toshiba_initall_list);
    if(ret_back == -1) goto NACK_BREAK;
    lidbg("tc358746xbg_show_colorbar_config_user_tab[0] = 0x%.2x\n", tc358746xbg_show_colorbar_config_user_tab[0].add_val);
    lidbg("tc358746xbg_show_colorbar_config_user_tab[1] = 0x%.2x\n", tc358746xbg_show_colorbar_config_user_tab[1].add_val);

    for(j = 360; j > 0; j--)
    {
        ret = tc358746xbg_write(&(tc358746xbg_show_colorbar_config_user_tab[0].add_reg), &(tc358746xbg_show_colorbar_config_user_tab[0].add_val), tc358746xbg_show_colorbar_config_user_tab[0].registet_width);
        if(ret == NACK) goto NACK_BREAK;
        ret = tc358746xbg_write(&(tc358746xbg_show_colorbar_config_user_tab[1].add_reg), &(tc358746xbg_show_colorbar_config_user_tab[1].add_val), tc358746xbg_show_colorbar_config_user_tab[1].registet_width);
        if(ret == NACK) goto NACK_BREAK;
    }
    add_reg_1 = 0x00e0; //使能colobar
    add_val_1 = 0xc1df;
    tc358746xbg_write(&add_reg_1, &add_val_1, register_value_width_16);
    return ;
NACK_BREAK:
    lidbg("ERROR TC358 NACK :%s\n", __func__);
}

void tc358746xbg_config_begin(vedio_format_t flag)
{
    int ret;
    lidbg("TC358 inital begin\n");
    tc358746_dbg("flag= %d\n", flag);
    tc358746xbg_power_contorl();
    ret = tc358746xbg_get_id();
    if(ret < 0)
        return;
    if(flag <= COLORBAR + TC358746XBG_WHITE + 1)
    {
        switch (flag)
        {
        case NTSC_I:
        case NTSC_P:
            tc358746xbg_config_array(NTSCp_init_tab);
            lidbg("TC358746:parameter is NTSCp_init_tab!\n");
            break;

        case PAL_I:
        case PAL_P:
            tc358746xbg_config_array(PALp_init_tab);
            lidbg("TC358746:parameter is PALp_init_tab!\n");
            break;

        case STOP_VIDEO:
            tc358746xbg_config_array(Stop_tab);
            lidbg("TC358746:parameter is is Stop_tab!\n");
            break;
        case COLORBAR:
            tc358746xbg_show_colorbar_config();
            lidbg("TC358746:parameter is is COLORBAR!\n");
            break;
        case COLORBAR+TC358746XBG_BLUE:
            tc358746xbg_show_colorbar_config_blue(TC358746XBG_BLUE);
            lidbg("TC358746:parameter is is COLORBAR TC358746XBG_BLUE!\n");
            break;
        case COLORBAR+TC358746XBG_RED:
            tc358746xbg_show_colorbar_config_blue(TC358746XBG_RED);
            lidbg("TC358746:parameter is is COLORBAR TC358746XBG_RED!\n");
            break;
        case COLORBAR+TC358746XBG_GREEN:
            tc358746xbg_show_colorbar_config_blue(TC358746XBG_GREEN);
            lidbg("TC358746:parameter is is COLORBAR TC358746XBG_GREEN!\n");
            break;
        case COLORBAR+TC358746XBG_LIGHT_BLUE:
            tc358746xbg_show_colorbar_config_blue(TC358746XBG_LIGHT_BLUE);
            lidbg("TC358746:parameter is is COLORBAR TC358746XBG_LIGHT_BLUE!\n");
            break;
        case COLORBAR+TC358746XBG_BLACK:
            tc358746xbg_show_colorbar_config_user();
            lidbg("TC358746:parameter is tc358746xbg_show_colorbar_config_user!\n");
            break;
        case COLORBAR+TC358746XBG_YELLOW:
            tc358746xbg_show_colorbar_config_blue(TC358746XBG_YELLOW);
            lidbg("TC358746:parameter is is COLORBAR TC358746XBG_YELLOW!\n");
            break;
        case COLORBAR + TC358746XBG_WHITE+1://number 15
            tc358746xbg_show_colorbar_config_grey();
            lidbg("TC358746:parameter only toshiba_initall_list!\n");
            break;

        default :
            tc358746xbg_show_colorbar_config();
            lidbg("TC358746:parameter is default NTSCi_init_tab!\n");
            break;
        }
    }
    else
    {
        lidbg("TC358746:error:you input TC358746 parameter is not have !\n");
    }
    lidbg("TC358 inital done\n\n");
}

