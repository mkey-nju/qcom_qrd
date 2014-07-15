#include "i2c_io.h"
#include "tw9912.h"
#include "tw9912_config.h"
#include "lidbg.h"

struct mutex lock_com_chipe_config;
unsigned int car_backing_times_cnt = 0;
u8 tw9912_reset_flag_jam = 0;//验证是否可以去除
u8 tw9912_signal_unstabitily_for_tw9912_config_array_flag = 0;//改变量还未使用

tw9912_input_info_t tw9912_input_information;
tw9912_run_flag_t tw912_run_sotp_flag;
tw9912_signal_t signal_is_how[5] = //用于记录四个通道的信息
{
    {NOTONE, OTHER, source_other}, //YIN0
    {NOTONE, OTHER, source_other}, //YIN1
    {NOTONE, OTHER, source_other}, //YIN2
    {NOTONE, OTHER, source_other}, //YIN3
    {NOTONE, OTHER, source_other}, //SEPARATION
};
tw9912_initall_status_t tw9912_status = {TW9912_initall_not, NOTONE, OTHER};
last_config_t the_last_config = {NOTONE, OTHER};

void tw9912_hardware_reset(void)
{
    tw9912_dbg("%s:tw9912_RESX_DOWN\n", __func__);
    tw9912_RESX_DOWN;
    msleep(20);
    tw9912_RESX_UP;
}
i2c_ack tw9912_read(unsigned int sub_addr, char *buf )
{
    i2c_ack ret;
    ret = i2c_read_byte(1, TW9912_I2C_ChipAdd, sub_addr, buf, 1);
    //tw9912_dbg("tw9912:read addr=0x%.2x value=0x%.2x\n", sub_addr, buf[0]);
    return ret;
}
i2c_ack tw9912_write(char *buf )
{
    i2c_ack ret;
    tw9912_dbg("tw9912:write addr=0x%.2x value=0x%.2x\n", buf[0], buf[1]);
    ret = i2c_write_byte(1, TW9912_I2C_ChipAdd, buf, 2);
    return ret;
}
void tw9912_get_input_info(tw9912_input_info_t *input_information)
{
    i2c_ack ret;
    input_information->chip_status1.index = 0x01;	//register index
    input_information->component_video_format.index = 0x1e;
    input_information->macrovision_detection.index = 0x30;
    ret = tw9912_read(input_information->chip_status1.index, \
                      &input_information->chip_status1.valu);
    if (ret == NACK) goto NACK_BREAK;

    ret = tw9912_read(input_information->component_video_format.index, \
                      &input_information->component_video_format.valu);
    if (ret == NACK) goto NACK_BREAK;

    ret = tw9912_read(input_information->macrovision_detection.index, \
                      &input_information->macrovision_detection.valu);
NACK_BREAK:
    ;
}
void tw9912_analysis_input_signal(tw9912_input_info_t *input_information, vedio_channel_t channel)
{
    u8 channel_1;
    u8 format_1;

    if(channel > SEPARATION)
    {
        tw9912_dbg_err("tw9912:%s:input chanel paramal have error!\n", __func__);
        goto CHANNEL_FAILD;
    }
    signal_is_how[channel].Channel = channel;
    signal_is_how[channel].Format = OTHER;
    signal_is_how[channel].vedio_source = source_other;

    if(input_information->chip_status1.valu & 0x08 )//bit3=1 Vertical logi is locked to the incoming video soruce
    {
        lidbg("tw9912:Signal is lock<<<<<<<<<<<<<<\n");
        signal_is_how[channel].Channel = channel;
        tw9912_read(0x02, &channel_1); //register 0x02 channel selete
        if(channel != SEPARATION)
        {
            format_1 = input_information->component_video_format.valu & 0x70;
            if(format_1 == 0x00)
            {
                signal_is_how[channel].Format = NTSC_I;
            }
            else if(format_1 == 0x10)
            {
                signal_is_how[channel].Format = PAL_I;
            }
            else if(format_1 == 0x20)
            {
                signal_is_how[channel].Format = NTSC_P;
            }
            else if(format_1 == 0x30)
            {
                signal_is_how[channel].Format = PAL_P;
            }
            else
            {
                signal_is_how[channel].Format = OTHER;
            }
			
        }
        else
        {
            format_1 = input_information->input_detection.valu & 0x07;
            if(format_1 == 0x2)
            {
                signal_is_how[channel].Format = NTSC_P;
            }
            else if(format_1 == 0x3)
            {
                signal_is_how[channel].Format = PAL_P;
            }
            else
            {
                lidbg("tw9912:Signal is lock but testing >>>>>>>>>>>>failed\n");
            }
        }
    }
    else
    {
        lidbgerr("tw9912:testing NOT lock>>>>>>>>>>>\n");
        signal_is_how[channel].Format = OTHER;
        signal_is_how[channel].vedio_source = source_other;
    }
CHANNEL_FAILD:
    ;
}
void Disabel_video_data_out(void)
{
    u8 disabel[] = {0x03, 0x27,};
    tw9912_dbg("Disabel_video_data_out()\n");
    tw9912_read(0x03, &disabel[1]); //register 0x02 channel selete
    disabel[1] |= 0x07;
    tw9912_write(disabel);
}
void Enabel_video_data_out(void)
{
    u8 disabel[] = {0x03, 0x0,};
    tw9912_dbg("Enabel_video_data_out()\n");
    tw9912_read(0x03, &disabel[1]); //register 0x02 channel selete
    disabel[1] &= 0xf0;
    tw9912_write(disabel);
}
static int tw9912_channel_choeces_again(vedio_channel_t channel)
{
    u8 Tw9912_input_pin_selet[] = {0x02, 0x40,}; //default input pin selet YIN0
    tw9912_dbg("fly_video now channal choices %d", channel);
    the_last_config.Channel = channel;
    switch(channel)//Independent testing
    {
    case YIN0: 	//	 YIN0
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
        break;
    case YIN1: //	 YIN1
        Tw9912_input_pin_selet[1] = 0x44; //register valu selete YIN1
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
        break;
    case YIN2: //	 YIN2
        Tw9912_input_pin_selet[1] = 0x48;
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;

        break;
    case YIN3: //	 YIN3
        Tw9912_input_pin_selet[1] = 0x4c;
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;

        Tw9912_input_pin_selet[0] = 0xe8;
        Tw9912_input_pin_selet[1] = 0x3f; //disable YOUT buffer
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
        break;
    case SEPARATION: 	//	 YUV
        Tw9912_input_pin_selet[1] = 0x70;
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
	 Tw9912_input_pin_selet[0] = 0xe8;
        Tw9912_input_pin_selet[1] = 0x30; //disable YOUT buffer
        if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
        break;
    default :
        tw9912_dbg_err("%s:you input Channel = %d >>>>>>>>>>>>>>error!\n", __FUNCTION__, channel);
        break;
    }
    return 0;
CONFIG_not_ack_fail:
    tw9912_dbg_err("tw9912:tw9912_channel_choeces_again()--->NACK error\n");
    return -1;
}
vedio_format_t tw9912_testing_channal_signal(vedio_channel_t Channel)
{
    vedio_format_t ret = OTHER;
    u8 signal = 0, valu;
    register u8 i;
    u8 tw9912_register[] = {0x1c, 0x07,}; //default input pin selet YIN0
    lidbg("tw9912_testing_channal_signal(channel = %d)\n", Channel);
    if(Channel == YIN2 || Channel == SEPARATION)
        return NTSC_P;
    mutex_lock(&lock_com_chipe_config);

    if(the_last_config.Channel  != Channel)
    {
        tw9912_channel_choeces_again(Channel);
    }
    else
    {
        tw9912_dbg("the_last_config.Channel  == Channel so not have change channal\n");
    }

    for(i = 0; i < 20; i++)
    {
        tw9912_read(0x01, &signal); //register 0x02 channel selete
        if(signal & 0x80) lidbgerr("tw9912:fly_video at channal (%d) not find signal\n", Channel);
        else
        {
            lidbg("tw9912:fly_video at channal (%d) find signal\n", Channel);
            goto break_for;
        }
        mutex_unlock(&lock_com_chipe_config);
        msleep(50);
        mutex_lock(&lock_com_chipe_config);

    }
    if(i == 2)goto SIGNAL_NOT_LOCK;
break_for:
    tw9912_write(tw9912_register);// Auto detection
    mutex_unlock(&lock_com_chipe_config);
    msleep(40);//waiting......
    mutex_lock(&lock_com_chipe_config);
    tw9912_read(0x1c, &signal);
    signal = signal & 0x70;
    switch(signal)
    {
    case 0:
        ret = NTSC_I;
        lidbg("\nSTDNOW is NTSC(M)\n");
        break;
    case 0x10:
        ret = PAL_I;
        lidbg("\nSTDNOW is PAL (B,D,G,H,I)\n");
        break;
    case 0x20:
        ret = OTHER;
        lidbg("\nSTDNOW is SECAM\n");
        break;
    case 0x30:
        ret = NTSC_I;
        lidbg("\nSTDNOW is NTSC4.43\n");
        break;
    case 0x40:
        ret = PAL_I;
        lidbg("\nSTDNOW is PAL (M)\n");
        break;
    case 0x50:
        ret = PAL_I;
        lidbg("\nSTDNOW is PAL (CN)\n");
        break;
    case 0x60:
        ret = PAL_I;
        lidbg("\nSTDNOW is PAL 60\n");
        break;
    default :
        ret = OTHER;
        lidbg("\nSTDNOW is NA\n");
        break;
    }
    if(Channel == YIN2 || Channel == SEPARATION)
    {
        tw9912_read(0xc1, &valu);
        if(valu & 0x08) //bit3 -->Composite Sync detection status
        {
            switch(valu & 0x7) //bit[2:0]
            {
            case 0:
                ret = NTSC_I;
                break;
            case 1:
                ret = PAL_I;
                break;

            case 2:
                ret = NTSC_P;
                lidbg("\nDVD is NTSC_P\n");
                break;
            case 3:
                ret = PAL_P;
                break;

            default:
                ret = OTHER;
                break;
            }
            lidbg("tw9912:tw9912_yuv_signal_testing() singal lock back %d\n", ret);
        }
    }
    lidbg("tw9912:fly_video test signal is %d\n", ret);
    mutex_unlock(&lock_com_chipe_config);
    return ret;
SIGNAL_NOT_LOCK:
    mutex_unlock(&lock_com_chipe_config);
    return OTHER;
}

vedio_format_t camera_open_video_signal_test_in_2(void)
{
    u8 channel_1;
    u8 format_1;
    tw9912_signal_t signal_is_how_1 = {NOTONE, OTHER, source_other};
    tw9912_input_info_t tw9912_input_information_1;

    tw9912_dbg("camera_open_video_signal_test()!\n");

    tw9912_get_input_info(&tw9912_input_information_1);
    if(tw9912_input_information_1.chip_status1.valu & 0x08 )//bit3=1 Vertical logi is locked to the incoming video soruce
    {
        if(tw9912_input_information_1.chip_status1.valu & 0x01)  signal_is_how_1.vedio_source = source_50Hz;
        else signal_is_how_1.vedio_source = source_60Hz;

        tw9912_read(0x02, &channel_1); //register 0x02 channel selete
        channel_1 = (channel_1 & 0x0c) >> 2 ;
        if(channel_1 == tw9912_status.Channel)
        {
            format_1 = tw9912_input_information_1.component_video_format.valu & 0x70;
            if(format_1 == 0x00)
            {
                signal_is_how_1.Format = NTSC_I;
            }
            else if(format_1 == 0x10)
            {
                signal_is_how_1.Format = PAL_I;
            }
            else if(format_1 == 0x20)
            {
                signal_is_how_1.Format = NTSC_P;
            }
            else if(format_1 == 0x30)
            {
                signal_is_how_1.Format = PAL_P;
            }

        }
        else
        {
            signal_is_how_1.Format = OTHER;
        }
    }

    return signal_is_how_1.Format;
}
int read_tw9912_chips_status_fast(u8 *valu)
{
    int ret = 0;
    ret = tw9912_read(0x01, valu);
    return ret;//have change return 1 else retrun 0
}
int tw9912_read_chips_status(u8 cmd)
{
    static tw9912_input_info_t tw9912_input_information_status;
    static tw9912_input_info_t tw9912_input_information_status_next;
    if(cmd != 2 )
        tw9912_get_input_info(&tw9912_input_information_status_next);
    if(cmd == 1)
    {
        if(tw9912_input_information_status_next.macrovision_detection.valu != tw9912_input_information_status.macrovision_detection.valu)
        {

            lidbg("worning:macrovision_detection(0x30) have change --old = %d,new =%d\n",
                   tw9912_input_information_status.macrovision_detection.valu,
                   tw9912_input_information_status_next.macrovision_detection.valu);

            tw9912_input_information_status.macrovision_detection.valu =
                tw9912_input_information_status_next.macrovision_detection.valu;
            return 1;
        }
        return 0;
    }
    else if (cmd == 2)
    {
        u8 valu;
        tw9912_read(0x01, &valu);
        lidbg("0x01 = 0x%.2x\n", valu);
        if(valu & 0x80)
        {
            lidbg("signal bad \n");
        }
    }
    else
    {
        return tw9912_input_information_status_next.macrovision_detection.valu;
    }
}
vedio_format_t tw9912_yuv_signal_testing(void)
{
    vedio_format_t ret = OTHER;
    u8 valu;//default input pin selet YIN0
    tw9912_dbg("tw9912_yuv_signal_testing()\n");
    if(the_last_config.Channel != SEPARATION)
        Tw9912_YIN3ToYUV_init_agin();
    tw9912_read(0xc1, &valu);
    if(valu & 0x08) //bit3 -->Composite Sync detection status
    {
        switch(valu & 0x7) //bit[2:0]
        {
        case 0:
            ret = NTSC_I;
            break;
        case 1:
            ret = PAL_I;
            break;

        case 2:
            ret = NTSC_P;
            break;
        case 3:
            ret = PAL_P;
            break;

        default:
            ret = OTHER;
            break;
        }
        lidbg("tw9912_yuv_signal_testing() singal lock back %d\n", ret);
    }
    return ret;
}
vedio_format_t tw9912_cvbs_signal_tsting(vedio_channel_t Channel)
{
    vedio_format_t ret = OTHER;
    u8 format_1;
    tw9912_signal_t signal_is_how_1 = {NOTONE, OTHER, source_other};
    tw9912_input_info_t tw9912_input_information_1;
    mutex_lock(&lock_com_chipe_config);
    if((the_last_config.Channel != Channel))
    {
    	lidbg("the_last_config.Channel=%d\n",the_last_config.Channel);
        tw9912_config_array_agin();
    }

    if(Channel > SEPARATION) goto CHANNAL_ERROR;
    signal_is_how_1.Channel = Channel;

    signal_is_how_1.Format = OTHER;
    signal_is_how_1.vedio_source = source_other;

    tw9912_get_input_info(&tw9912_input_information_1);
    if( (tw9912_input_information_1.chip_status1.valu & 0x08) && !(tw9912_input_information_1.chip_status1.valu & 0x80) )//bit3=1 Vertical logi is locked to the incoming video soruce
    {
        // bit7 =0 video is detected
        if(tw9912_input_information_1.chip_status1.valu & 0x01)  signal_is_how_1.vedio_source = source_50Hz;
        else signal_is_how_1.vedio_source = source_60Hz;

        format_1 = tw9912_input_information_1.component_video_format.valu & 0x70;
        if(format_1 == 0x00)
        {
            signal_is_how_1.Format = NTSC_I;
        }
        else if(format_1 == 0x10)
        {
            signal_is_how_1.Format = PAL_I;
        }
        else if(format_1 == 0x20)
        {
            signal_is_how_1.Format = NTSC_P;
        }
        else if(format_1 == 0x30)
        {
            signal_is_how_1.Format = PAL_P;
        }
		else
			lidbgerr("tw9912_cvbs_signal_tsting err %d\n",format_1);
    }

    //tw9912_dbg("testing_signal(): back %d\n", signal_is_how_1.Format);
    mutex_unlock(&lock_com_chipe_config);
    return signal_is_how_1.Format;
CHANNAL_ERROR:
    mutex_unlock(&lock_com_chipe_config);
    tw9912_dbg("tw9912:tw9912_cvbs_signal_tsting()--->Channel input error\n");
    ret = OTHER;
    return ret;
}
vedio_format_t Tw9912_appoint_pin_tw9912_cvbs_signal_tsting(vedio_channel_t Channel)
{
    int ret = -1;
    u8 channel_1;
    u8 Tw9912_input_pin_selet[] = {0x02, 0x40,}; //default input pin selet YIN0
    u8 manually_initiate_auto_format_detection[] = {0x1d, 0xff,}; //default input pin selet YIN0
    mutex_lock(&lock_com_chipe_config);
    tw9912_dbg("@@@@@Tw9912_appoint_pin_tw9912_cvbs_signal_tsting!\n");

    if(Channel != SEPARATION)
    {
        tw9912_read(0x02, &channel_1); //register 0x02 channel selete
        channel_1 = (channel_1 & 0x0c) >> 2 ; //read back now config Channel
        if( ( (channel_1 != Channel ) && Channel != NOTONE) \
                || (tw9912_status.flag != TW9912_initall_yes) )//if now config Channel is not testing Channel
            // or tw9912 is not have initall
        {
            if(tw9912_status.flag == TW9912_initall_not )
            {//用来判断tw9912 是否有过初始化
                tw9912_status.flag = TW9912_initall_yes;
                tw9912_status.Channel = Channel;
                tw9912_status.format = PAL_I;
                tw9912_dbg("first initalll!\n");//应该取消这个逻辑，在数组const u8 TW9912_INIT_AGAIN[] = 添加必要的寄存器配置就可以
                tw9912_config_array_PALi();//initall all register
            }

            the_last_config.Channel = Channel;
            switch(Channel)//Independent testing
            {
            case YIN0: 	//	 YIN0
                if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
                break;
            case  YIN1: //	 YIN1
                Tw9912_input_pin_selet[1] = 0x44; //register valu selete YIN1
                if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
                break;
            case  YIN2: //	 YIN2
                Tw9912_input_pin_selet[1] = 0x48;
                if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;

                break;
            case  YIN3: //	 YIN3
                Tw9912_input_pin_selet[1] = 0x4c;
                if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;

                Tw9912_input_pin_selet[0] = 0xe8;
                Tw9912_input_pin_selet[1] = 0x3f; //disable YOUT buffer
                if(tw9912_write(Tw9912_input_pin_selet) == NACK) goto CONFIG_not_ack_fail;
                break;
            default :
                tw9912_dbg("%s:you input Channel = %d error!\n", __FUNCTION__, Channel);
                break;

            }
        }
    }
    else
    {
        lidbg("\r\r\n\n");
        lidbg("tw9912:testing NTSCp The default detection!\n");
        tw9912_status.flag = TW9912_initall_yes;
        tw9912_status.Channel = SEPARATION;
        tw9912_status.format = NTSC_P;
        signal_is_how[SEPARATION].Format = NTSC_P;
        ret = NTSC_P;
        goto TEST_NTSCp;
    }
    tw9912_write(manually_initiate_auto_format_detection);
    tw9912_get_input_info(&tw9912_input_information);
    tw9912_analysis_input_signal(&tw9912_input_information, Channel);
    switch(signal_is_how[Channel].Format)
    {
    case NTSC_I:
        ret = 1;
        break;
    case PAL_I:
        ret = 2;
        break;

    case NTSC_P:
        ret = 3;
        break;
    case PAL_P:
        ret = 4;
        break;

    default:
        ret = 5;
        break;
    }
TEST_NTSCp:
    mutex_unlock(&lock_com_chipe_config);
    return ret;
CONFIG_not_ack_fail:
    mutex_unlock(&lock_com_chipe_config);
    tw9912_dbg_err("Tw9912_appoint_pin_tw9912_cvbs_signal_tsting()--->NACK error\n");
    ret = -1;
    return ret;
}
static void tw9912_get_chip_id(void)
{
    u8 TW9912_ID[] = {0x00, 0x00};
    u8 valu;
    tw9912_read(TW9912_ID[0], &TW9912_ID[1]);
    valu = TW9912_ID[1] >> 4; //ID = 0x60
    if( valu == 0x6)
        lidbg("TW9912 ID=0x%.2x\n", TW9912_ID[1]);
    else
        tw9912_dbg_err("TW9912 Communication error!(0x%.2x)\n", TW9912_ID[1]);

}
int tw9912_config_array_NTSCp(void)
{
    u32 i = 0;
    const u8 *config_pramat_piont = NULL;
    if (tw9912_reset_flag_jam == 0)
    {
        tw9912_reset_flag_jam = 1;
        tw9912_dbg("%s:tw9912_RESX_DOWN\n", __func__);
        tw9912_RESX_DOWN;//\u8fd9\u91cc\u5bf9tw9912\u590d\u4f4d\u7684\u539f\u56e0\u662f\u89e3\u51b3\u5012\u8f66\u9000\u56deDVD\u65f6\u89c6\u9891\u5361\u6b7b\u3002
        tw9912_RESX_UP;
    }
    tw9912_dbg("tw9912_config_array_NTSCp initall tw9912+\n");
    tw9912_get_chip_id();
    the_last_config.Channel = SEPARATION;
    the_last_config.format = NTSC_P;
    config_pramat_piont = TW9912_INIT_NTSC_Progressive_input;
    while(config_pramat_piont[i*2] != 0xfe)
    {
        if(tw9912_write((char *)&config_pramat_piont[i*2]) == NACK) goto CONFIG_not_ack_fail;
        //		tw9912_dbg("w a=%x,v=%x\n",config_pramat_piont[i*2],config_pramat_piont[i*2+1]);
        i++;
    }
    tw9912_dbg("tw9912_config_array_NTSCp initall tw9912-\n");
    return 1;
CONFIG_not_ack_fail:
    tw9912_dbg_err("%s:have NACK error!\n", __FUNCTION__);
    return -1;
}
int tw9912_config_array_agin(void)
{
    u32 i = 0;
    const u8 *config_pramat_piont = NULL;
    tw9912_dbg("tw9912_config_array_agin +\n");
    the_last_config.Channel = YIN3;
    the_last_config.format = NTSC_I;
    config_pramat_piont = TW9912_INIT_AGAIN;
    while(config_pramat_piont[i*2] != 0xfe)
    {
        if(tw9912_write((char *)&config_pramat_piont[i*2]) == NACK) goto CONFIG_not_ack_fail;
        i++;
    }
    tw9912_dbg("tw9912_config_array_agin -\n");
    return 1;
CONFIG_not_ack_fail:
    tw9912_dbg_err("%s:have NACK error!\n", __FUNCTION__);
    return -1;
}
int Tw9912_YIN3ToYUV_init_agin(void)
{
    u32 i = 0;
    const u8 *config_pramat_piont = NULL;
    tw9912_dbg("Tw9912_YIN3ToYUV_init_agin +\n");
    the_last_config.Channel = SEPARATION;
    the_last_config.format = NTSC_P;
    config_pramat_piont = TW9912_YIN3ToYUV_INIT_AGAIN;
    while(config_pramat_piont[i*2] != 0xfe)
    {
        if(tw9912_write((char *)&config_pramat_piont[i*2]) == NACK) goto CONFIG_not_ack_fail;
        i++;
    }
    tw9912_dbg("Tw9912_YIN3ToYUV_init_agin -\n");
    return 1;
CONFIG_not_ack_fail:
    tw9912_dbg_err("%s:have NACK error!\n", __FUNCTION__);
    return -1;
}
int tw9912_config_array_PALi(void)
{
    u32 i = 0;
    const u8 *config_pramat_piont = NULL;
    tw9912_dbg("tw9912_config_array_PALi initall tw9912+\n");
    tw9912_get_chip_id();
    the_last_config.Channel = YIN3;
    the_last_config.format = PAL_I;
    config_pramat_piont = TW9912_INIT_PAL_Interlaced_input;
    while(config_pramat_piont[i*2] != 0xfe)
    {
        if(tw9912_write((char *)&config_pramat_piont[i*2]) == NACK) goto CONFIG_not_ack_fail;
        i++;
    }
    tw9912_dbg("Tw9912_appoint_pin_tw9912_cvbs_signal_tsting initall tw9912-\n");
    return 1;
CONFIG_not_ack_fail:
    tw9912_dbg_err("%s:have NACK error!\n", __FUNCTION__);
    return -1;
}

int tw9912_config_array( vedio_channel_t Channel)
{
    vedio_format_t ret_format;
    u32 i = 0;
    int ret = 0, delte_signal_count = 0;
    const u8 *config_pramat_piont = NULL;
    u8 Tw9912_input_pin_selet[] = {0x02, 0x40,}; //default input pin selet YIN0
    lidbg("tw9912: inital begin\n");
    mutex_lock(&lock_com_chipe_config);
    tw9912_get_chip_id();
    //if(config_pramat != STOP_VIDEO)
    {
        tw9912_channel_choeces_again(Channel);//切换通道
        tw9912_dbg("tw9912:tw9912_config_array()-->Tw9912_appoint_pin_tw9912_cvbs_signal_tsting(%d)\n", Channel);
        mutex_unlock(&lock_com_chipe_config);
SIGNAL_DELTE_AGAIN:
        tw9912_dbg("tw9912 inital befor test signal count:%d;", delte_signal_count);
        ret = Tw9912_appoint_pin_tw9912_cvbs_signal_tsting(Channel);//bad
        delte_signal_count++;
        //msleep(20);
        if(ret == STOP_VIDEO && delte_signal_count < 2) goto SIGNAL_DELTE_AGAIN;
        delte_signal_count = 0;
        mutex_lock(&lock_com_chipe_config);
        if(ret == STOP_VIDEO) //the channel is not signal input
        {
        	lidbgerr("Tw9912_appoint_pin_tw9912_cvbs_signal_tsting fail,try another\n");
            tw9912_signal_unstabitily_for_tw9912_config_array_flag = 0;//find colobar flag signal bad
            mutex_unlock(&lock_com_chipe_config);
            ret_format = tw9912_testing_channal_signal(Channel);
            mutex_lock(&lock_com_chipe_config);
            if(ret_format == OTHER )
                goto NOT_signal_input;
            else
            {
            	mutex_unlock(&lock_com_chipe_config);
				ret = tw9912_cvbs_signal_tsting(Channel);
				      lidbg("tw9912_cvbs_signal_tsting Video Forma Is %d\n",ret);
				mutex_lock(&lock_com_chipe_config);
                signal_is_how[Channel].Format = ret;
            }
        }
        if(ret == -1)
            goto CONFIG_not_ack_fail;

        switch(ret)
        {
        case NTSC_I:
            lidbg("Video Forma Is NTSC_I\n");
            break;
        case NTSC_P:
            lidbg("Video Format Is NTSC\n");
            break;
        case PAL_I:
            lidbg("Video Format Is PAL_I\n");
            break;
        case PAL_P:
            lidbg("Video Format Is PAL_P\n");
            break;
        default:
            ;
            break;

        }
        tw9912_status.flag = TW9912_initall_yes;
        tw9912_status.Channel = Channel;
        switch(signal_is_how[Channel].Format)
        {//根据检测到的视频制式 决定配置的数组
        case NTSC_I:
            tw9912_status.format = NTSC_I;
            config_pramat_piont = TW9912_INIT_NTSC_Interlaced_input;
	     car_backing_times_cnt++;
	     lidbg("tw9912_config_array: car_backing_times_cnt = %d\n", car_backing_times_cnt);
            break;

        case PAL_I:
            tw9912_status.format = PAL_I;
            config_pramat_piont = TW9912_INIT_PAL_Interlaced_input;
            break;

        case NTSC_P:
            tw9912_status.format = NTSC_P;
            config_pramat_piont = TW9912_INIT_NTSC_Progressive_input;
            break;

        case PAL_P:

            tw9912_status.format = PAL_P;
            config_pramat_piont = TW9912_INIT_PAL_Progressive_input;
            break;

        default:
            tw9912_status.flag = TW9912_initall_not;
            tw9912_status.Channel = NOTONE;
            lidbgerr("Format is Invalid ******\n");
            lidbgerr("tw9912:%s:signal_is_how[Channel].Format=%d\n", __func__, signal_is_how[Channel].Format);
            goto NOT_signal_input;
            break;
        }

        the_last_config.Channel = Channel;
        the_last_config.format = signal_is_how[Channel].Format;
        while(config_pramat_piont[i*2] != 0xfe)
        {
            if(tw9912_write((char *)&config_pramat_piont[i*2]) == NACK) goto CONFIG_not_ack_fail;
            if(signal_is_how[Channel].Format == NTSC_P \
                    && config_pramat_piont[i*2] > 0x24\
                    && config_pramat_piont[i*2] < 0x2d)
                usleep(100);
            i++;
        }
    }
    tw912_run_sotp_flag.format = signal_is_how[Channel].Format;
    lidbg("tw9912: inital done\n");
    mutex_unlock(&lock_com_chipe_config);
    return 1;
CONFIG_not_ack_fail:
    mutex_unlock(&lock_com_chipe_config);
    tw9912_dbg_err("%s:have NACK error!\n", __FUNCTION__);
    return -2;
NOT_signal_input:
    mutex_unlock(&lock_com_chipe_config);
    tw9912_dbg_err("%s:the channal=%d not have video signal!\n", __FUNCTION__, Channel);

    return -1;
}

