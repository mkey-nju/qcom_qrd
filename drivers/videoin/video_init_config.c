#include "lidbg.h"
#include "video_init_config.h"
#include "video_init_config_tab.h"
static int flag_io_config = 0;
vedio_channel_t info_vedio_channel_t = NOTONE;//用于记录现在状态下处理的对应通道
vedio_channel_t info_com_top_Channel = YIN2;//用于记录当前最新的上层切换到的视频通道状态 YIN3(倒车、AUX、TV等) 或 SEPARATION（DVD）
extern tw9912_signal_t signal_is_how[5];
extern last_config_t the_last_config;//上一次的通道状态
extern tw9912info_t global_tw9912_info_for_NTSC_I;
extern tw9912info_t global_tw9912_info_for_PAL_I;
vedio_channel_t_2 last_vide_format = OTHER_CHANNEL_4KO;
static u8 flag_now_config_channal_AUX_or_Astren = 0; //0 is Sstren 1 is AUX 2 is DVD
u8 global_debug_thread = 0;
static TW9912_Image_Parameter TW9912_Image_Parameter_fly[6] =
{//用于记录 HAL层配置的 视频页面下“设置”标签 的参数 0～10 待到真正配置到色彩设置代码时将使用该函数的值
    {BRIGHTNESS, 5},
    {CONTRAST, 5},
    {SHARPNESS, 5},
    {CHROMA_U, 5},
    {CHROMA_V, 5},
    {HUE, 5},
};

struct mutex lock_chipe_config;
extern struct mutex lock_com_chipe_config;
struct semaphore sem;
void video_io_i2c_init_in(void)
{
    if (!flag_io_config)
    {
        mutex_init(&lock_chipe_config);
        mutex_init(&lock_com_chipe_config);
        sema_init(&sem, 0);
        i2c_io_config_init();
        flag_io_config = 1;
    }
}
void chips_hardware_reset(void)
{
    lidbg("reset tw9912 and tc358746\n");
    //tw9912_hardware_reset();
    tc358746xbg_hardware_reset();
}
/*
函数名：int static video_signal_channel_switching_occurs(void)
功能：  通道变换时，复位9912，（CVBS切到YUV时，YUV解码出问题，来一次复位问题解决）
*/
int static video_signal_channel_switching_occurs(void)
{
    lidbg("TC358:video_signal_channel_switching_occurs() \n");
    tc358746xbg_data_out_enable(DISABLE);//禁用输出  让用户看不到切换过程
    lidbg("%s:tw9912_RESX_DOWN\n", __func__);

    tw9912_RESX_DOWN;//\u8fd9\u91cc\u5bf9tw9912\u590d\u4f4d\u7684\u539f\u56e0\u662f\u89e3\u51b3\u5012\u8f66\u9000\u56deDVD\u65f6\u89c6\u9891\u5361\u6b7b\u3002
    msleep(20);
    tw9912_RESX_UP;//在YIN3 切换 到DVD通道，如果不复位一下 视频回定住
    msleep(20);
    //  mutex_unlock(&lock_chipe_config);
    return 0;
}
int static video_image_config_parameter_buffer(void)
{
    if (info_com_top_Channel == YIN3)
    {
/*
目前倒车的亮度等值，只有以下的一组。
*/
//TW9912_Image_Parameter_fly[1].valu 等于240是一般倒车，0xF1是天籁倒车，小于10为AUX输入
        /**************************************Astren************************************************/
        if(TW9912_Image_Parameter_fly[1].valu > 10)//240 是和蒋工商量好的值，用于区别目前是倒车的配置
        {
            lidbg("Astern 2 Normal \n");
            flag_now_config_channal_AUX_or_Astren = 0;
            if(signal_is_how[info_com_top_Channel].Format == NTSC_I)
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK[0][1] = \
                                                             Image_Config_CarBacking_Normal[0][5];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK[1][1] = \
                                                             Image_Config_CarBacking_Normal[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu - 240];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK[3][1] = \
                                                             Image_Config_CarBacking_Normal[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        Tw9912_image_global_AUX_BACK[4][1] = \
                                                             Image_Config_CarBacking_Normal[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK[2][1] = \
                                                             Image_Config_CarBacking_Normal[2][10-(TW9912_Image_Parameter_fly[HUE-1].valu - 240)];
                        break;
                    default :
                        break;
                    }
                }
                // lidbg("Tw9912_image_global_AUX_BACK reset valu from NTSC_I\n");
            }
            else//PALi
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK_PAL_I[0][1] = \
                                Image_Config_AUX_BACK_PAL_I[0][5];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK_PAL_I[1][1] = \
                                Image_Config_AUX_BACK_PAL_I[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu - 240];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK_PAL_I[3][1] = \
                                Image_Config_AUX_BACK_PAL_I[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        Tw9912_image_global_AUX_BACK_PAL_I[4][1] = \
                                Image_Config_AUX_BACK_PAL_I[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK_PAL_I[2][1] = \
                                Image_Config_AUX_BACK_PAL_I[2][10-(TW9912_Image_Parameter_fly[HUE-1].valu - 240)];
                        break;
                    default :
                        break;
                    }
                }
                // lidbg("Tw9912_image_global_AUX_BACK reset valu from PAL_I\n");
            }
            return 1;
        }
        else if(TW9912_Image_Parameter_fly[1].valu == 0xF1)
        {
            lidbg("Astern 2 New Teana\n");
            flag_now_config_channal_AUX_or_Astren = 0;
            if(signal_is_how[info_com_top_Channel].Format == NTSC_I)
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK[0][1] = \
                                                             Image_Config_CarBacking_NewTeana[0][5];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK[1][1] = \
                                                             Image_Config_CarBacking_NewTeana[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu - 240];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK[3][1] = \
                                                             Image_Config_CarBacking_NewTeana[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        Tw9912_image_global_AUX_BACK[4][1] = \
                                                             Image_Config_CarBacking_NewTeana[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK[2][1] = \
                                                             Image_Config_CarBacking_NewTeana[2][10-(TW9912_Image_Parameter_fly[HUE-1].valu - 240)];
                        break;
                    default :
                        break;
                    }
                }
                // lidbg("Tw9912_image_global_AUX_BACK reset valu from NTSC_I\n");
            }
            else//PALi
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK_PAL_I[0][1] = \
                                Image_Config_AUX_BACK_PAL_I[0][5];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK_PAL_I[1][1] = \
                                Image_Config_AUX_BACK_PAL_I[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu - 240];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK_PAL_I[3][1] = \
                                Image_Config_AUX_BACK_PAL_I[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        Tw9912_image_global_AUX_BACK_PAL_I[4][1] = \
                                Image_Config_AUX_BACK_PAL_I[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu - 240];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK_PAL_I[2][1] = \
                                Image_Config_AUX_BACK_PAL_I[2][10-(TW9912_Image_Parameter_fly[HUE-1].valu - 240)];
                        break;
                    default :
                        break;
                    }
                }
                // lidbg("Tw9912_image_global_AUX_BACK reset valu from PAL_I\n");
            }
            return 1;
        }
        else if(TW9912_Image_Parameter_fly[1].valu > 250)
        {
            lidbg("Error at %s you input valu = %d paramter have Problems", __func__, TW9912_Image_Parameter_fly[1].valu);
            return -1;
        }
        /**************************************AUX********************************************/
        else
        {
		//AUX和DVD参数需要根据上层（蒋工）传入的参数配置(范围0~10) 
            lidbg("AUX\n");
            flag_now_config_channal_AUX_or_Astren = 1;
            if(signal_is_how[info_com_top_Channel].Format == NTSC_I)
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK[0][1] = \
                                                             Image_Config_AUX_BACK[0][10-TW9912_Image_Parameter_fly[BRIGHTNESS-1].valu];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK[1][1] = \
                                                             Image_Config_AUX_BACK[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK[3][1] = \
                                                             Image_Config_AUX_BACK[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                        Tw9912_image_global_AUX_BACK[4][1] = \
                                                             Image_Config_AUX_BACK[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK[2][1] = \
                                                             Image_Config_AUX_BACK[2][10-TW9912_Image_Parameter_fly[HUE-1].valu];
                        break;
                    default :
                        break;
                    }
                }
            }
            else//PAL_i
            {
                u8 i = 0;
                for (i = BRIGHTNESS; i <= HUE; i++)
                {
                    switch (i)
                    {
                    case BRIGHTNESS:
                        Tw9912_image_global_AUX_BACK_PAL_I[0][1] = \
                                Image_Config_AUX_BACK_PAL_I[0][10-TW9912_Image_Parameter_fly[BRIGHTNESS-1].valu];
                        break;
                    case CONTRAST:
                        Tw9912_image_global_AUX_BACK_PAL_I[1][1] = \
                                Image_Config_AUX_BACK_PAL_I[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu];
                        break;
                    case CHROMA_U:
                        Tw9912_image_global_AUX_BACK_PAL_I[3][1] = \
                                Image_Config_AUX_BACK_PAL_I[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                        Tw9912_image_global_AUX_BACK_PAL_I[4][1] = \
                                Image_Config_AUX_BACK_PAL_I[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                        break;
                    case HUE:
                        Tw9912_image_global_AUX_BACK_PAL_I[2][1] = \
                                Image_Config_AUX_BACK_PAL_I[2][10-TW9912_Image_Parameter_fly[HUE-1].valu];
                        break;
                    default :
                        break;
                    }
                }
            }
            return 1;
        }
    }
    /**************************************DVDS********************************************/
    else
    {
        u8 i = 0;
        lidbg("DVD\n");
        flag_now_config_channal_AUX_or_Astren = 2;
        for (i = BRIGHTNESS; i <= HUE; i++)
        {
            switch (i)
            {
            case  CONTRAST:
                Tw9912_image_global_separation[0][1] = \
                                                       Image_Config_separation[0][TW9912_Image_Parameter_fly[BRIGHTNESS-1].valu];
                break;
            case BRIGHTNESS:
                Tw9912_image_global_separation[1][1] = \
                                                       Image_Config_separation[1][TW9912_Image_Parameter_fly[CONTRAST-1].valu];
                break;
            case HUE:
                Tw9912_image_global_separation[2][1] = \
                                                       Image_Config_separation[2][TW9912_Image_Parameter_fly[HUE-1].valu];
                break;
            case CHROMA_U:
                Tw9912_image_global_separation[3][1] = \
                                                       Image_Config_separation[3][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                Tw9912_image_global_separation[4][1] = \
                                                       Image_Config_separation[4][TW9912_Image_Parameter_fly[CHROMA_U-1].valu];
                break;
            default :
                break;
            }
        }

    }
    return 1;
}
int static video_image_config_begin(void)
{
    int ret;
    register int i = 0;
    u8 Tw9912_image[2] = {0x17, 0x87,}; //default input pin selet YIN0ss
    lidbg("video_image_config_begin()\n");
    video_image_config_parameter_buffer();
    for(i = 0; i < 5; i++)
    {
        if(info_com_top_Channel == YIN3)//back or AUX
        {
            if(signal_is_how[info_com_top_Channel].Format == NTSC_I)
                ret = tw9912_write((char *)&Tw9912_image_global_AUX_BACK[i]);
            else
                ret = tw9912_write((char *)&Tw9912_image_global_AUX_BACK_PAL_I[i]);
        }
        else //DVD SEPARATION
            ret = tw9912_write((char *)&Tw9912_image_global_separation[i]);
    }

    //图像偏移量
    if(flag_now_config_channal_AUX_or_Astren == 0)//Astren
    {
        if(signal_is_how[info_com_top_Channel].Format == NTSC_I)
        {
            Tw9912_image[0] = 0x12;
            Tw9912_image[1] = 0x1f;
            ret = tw9912_write((char *)&Tw9912_image);
            Tw9912_image[0] = 0x08;
            Tw9912_image[1] = global_tw9912_info_for_NTSC_I.reg_val;//form qcamerahwi_preview.cpp
            ret = tw9912_write((char *)&Tw9912_image);
        }
        else
        {
            Tw9912_image[0] = 0x12;
            Tw9912_image[1] = 0xff;
            ret = tw9912_write((char *)&Tw9912_image);
            Tw9912_image[0] = 0x08;
            Tw9912_image[1] = global_tw9912_info_for_PAL_I.reg_val;//form qcamerahwi_preview.cpp
            ret = tw9912_write((char *)&Tw9912_image);
        }
    }

#ifdef BOARD_V1
    if(info_com_top_Channel == YIN3)// back or AUX
    {
        ret = tw9912_write((char *)Tw9912_image);
        Tw9912_image[0] = 0x08;
        Tw9912_image[1] = 0x14; // image down 5 line
        ret = tw9912_write((char *)Tw9912_image);
        Tw9912_image[0] = 0x0a;
        Tw9912_image[1] = 0x22; // image down 5 line
        ret = tw9912_write((char *)Tw9912_image);
    }
    else if(info_com_top_Channel == YIN2)//DVD
    {
        u8 Tw9912_image[2] = {0x08, 0x1a,}; //image reft 5 line
        ret = tw9912_write((char *)Tw9912_image);
    }
    else
    {
        ;
    }
#endif
    return ret;
}

//蒋工直接调用传入的参数配置
int flyVideoImageQualityConfig_in(Vedio_Effect cmd , u8 valu)//valu的值的范围是0～10
{
    lidbg("flyvideo_image_config_begin(%d,%d)\n", cmd, valu);
    switch(cmd)
    {
    case CONTRAST:
        TW9912_Image_Parameter_fly[CONTRAST-1].valu = valu;
        break;
    case BRIGHTNESS:
        TW9912_Image_Parameter_fly[BRIGHTNESS-1].valu = valu;
        break;
    case SHARPNESS:
        TW9912_Image_Parameter_fly[SHARPNESS-1].valu = valu;
        break;
    case CHROMA_U:
        TW9912_Image_Parameter_fly[CHROMA_U-1].valu = valu;
        break;
    case CHROMA_V:
        TW9912_Image_Parameter_fly[CHROMA_V-1].valu = valu;
        break;
    case HUE:
        TW9912_Image_Parameter_fly[HUE-1].valu = valu;
        break;
    default :
        lidbg("Error at %s you input cmd = %d paramter have Problems", __func__, cmd);
        break;
    }
   //蒋工每配一次参数最后写BRIGHTNESS
    if(cmd == BRIGHTNESS)//wait all set doen befor application
        video_image_config_begin();
    return 0;
}
int tw9912_config_for_cvbs_signal(vedio_channel_t Channel);
int flyVideoInitall_in(u8 Channel)//给蒋工的视频通道切换接口
{//该函数实际上是改变通道标志变量的值，没有实际改变芯片的通道
    int ret = 1 ;
    mutex_lock(&lock_chipe_config);
    lidbg("flyVideoInitall_in(%d)\n", Channel);
    if (Channel >= YIN0 && Channel <= NOTONE)
    {
        info_com_top_Channel = Channel;
        if(Channel == YIN2)//DVD通道
            info_com_top_Channel = SEPARATION;//DVD 的 逐行视频
    }
    else
    {
        lidbg("%s: you input TW9912 Channel=%d error!\n", __FUNCTION__, Channel);
    }
    mutex_unlock(&lock_chipe_config);
    return ret;
    //success return 1 fail return -1
}
int tw9912_config_for_cvbs_signal(vedio_channel_t Channel)
{
    int ret = -1 ;
    video_config_debug("tw9912_config_for_cvbs_signal(%d)\n", Channel);
    switch (Channel)
    {
    case YIN0:
        info_vedio_channel_t = YIN0;
        ret = tw9912_config_array( YIN0);
        video_config_debug("TW9912:Channel selet YIN0\n");
        break;
    case YIN1:
        info_vedio_channel_t = YIN1;
        ret = tw9912_config_array( YIN1);
        video_config_debug("TW9912:Channel selet YIN1\n");
        break;
    case YIN2:
        info_vedio_channel_t = YIN2;
        ret = tw9912_config_array( YIN2);
        video_config_debug("TW9912:Channel selet YIN2\n");
        break;
    case YIN3:
        info_vedio_channel_t = YIN3;
        ret = tw9912_config_array( YIN3);
        video_config_debug("TW9912:Channel selet YIN3\n");
        break;
    default :
        video_config_debug("%s: you input TW9912 Channel=%d error!\n", __FUNCTION__, Channel);
        break;
    }
    return ret;
}
vedio_format_t camera_open_video_signal_test_in(void)
{
    down(&sem);
    return camera_open_video_signal_test_in_2();
}

vedio_format_t flyVideoTestSignalPin_in(u8 Channel)//给蒋工的视频检测函数接口
{
    static vedio_format_t lidbg_count_flag_next = 0;
    static u8 lidbg_format_count = 0;
    vedio_format_t ret = NOTONE;
    static u8 Format_count = 0;
    static u8 Format_count_flag = 0;
    mutex_lock(&lock_chipe_config);
    if(
        ( (the_last_config.Channel == YIN2 || the_last_config.Channel == SEPARATION) && Channel == YIN3) || \
        (the_last_config.Channel == YIN3  && (Channel == YIN2 || Channel == SEPARATION))
    )//这次和上次的通道不是一个通道
    {
        video_signal_channel_switching_occurs();
    }

    /*
     *ensure AGC configed right when  video format
     *changed in YIN3 channel
     */
    if((the_last_config.Channel == YIN3) && (Channel == YIN3) && (last_vide_format != global_video_channel_flag))
    {
    	int i = 0;
	const u8 video_format_changed_config[] = {0x02, 0x4C, 0x03, 0x20, 0x06, 0x03, 0xC0, 0x01, 0xE6, 0x00, 0xE8, 0x3F, 0xfe};

	tc358746xbg_data_out_enable(DISABLE);

	tw9912_RESX_DOWN;
	msleep(10);
	tw9912_RESX_UP;
	msleep(10);

	while(video_format_changed_config[i*2] != 0xfe)
      {
          if(tw9912_write((char *)&video_format_changed_config[i*2]) == NACK)
		  	lidbg("I2C write err when video format changed.\n");
          i++;
      }	
	
	lidbg("Video format changed : (last)-%d, (now)-%d\n", last_vide_format, global_video_channel_flag);
	last_vide_format = global_video_channel_flag;
    }
	
    //mutex_unlock(&lock_chipe_config);
    // return NTSC_I;
    if(Channel == SEPARATION || Channel == YIN2)
    {
        ret = tw9912_yuv_signal_testing();
    }
    else
    {
        static u8 goto_agian_test = 0;
AGAIN_TEST_FOR_BACK_NTSC_I:
        goto_agian_test ++;
        switch (Channel)
        {
        case 0:
            ret =  tw9912_cvbs_signal_tsting(YIN0);
            break;
        case 1:
            ret =  tw9912_cvbs_signal_tsting(YIN1);
            break;
        case 2:
            ret =  tw9912_cvbs_signal_tsting(YIN2);
            break;
        case 3:
            ret =  tw9912_cvbs_signal_tsting(YIN3);
            break;
        case 4:
            ret =  tw9912_cvbs_signal_tsting(SEPARATION);
            break;
        default :
            lidbg("%s:you input TW9912 Channel=%d error!\n", __FUNCTION__, Channel);
            break;
        }
        if(ret == 1 && goto_agian_test <= 2 )//当时发现 NTSC_I 的检测不准确，到这里 当检测的是NTSC_I 再来做一次检测
            goto AGAIN_TEST_FOR_BACK_NTSC_I;
        else goto_agian_test = 0;
    }
    if (lidbg_count_flag_next != ret || lidbg_format_count > 20)
    {//打印屏蔽，同样的信息每20次输出一个，不一样马上输出
        lidbg_format_count = 0;
        lidbg_count_flag_next = ret;
        lidbg("C=%d,F=%d\n", Channel, ret);
    }
    else lidbg_format_count++;
    mutex_unlock(&lock_chipe_config);

    if( ret > 4 )
    {//如果检测30次以上都没检测到信号 可能是tw9912工作不正常啦，做一次重新配置
        Format_count ++;
        if(Format_count > 30 && Format_count_flag == 0)
        {
            Format_count = 0 ;
            Format_count_flag = 1;
            lidbgerr("warning : Test Input Signal Fail Now Reconfig Tw9912 \n");
            // chips_config_begin( PAL_P);
            if(info_com_top_Channel == SEPARATION || info_com_top_Channel == YIN2)
                tw9912_config_array_NTSCp();
            else
                tw9912_config_array( YIN3);
        }
    }
    else
    {
        Format_count = 0;
        Format_count_flag = 0;//When the AUX does not enter ,you can not repeat the configuration
        signal_is_how[info_com_top_Channel].Format = ret;
    }
    return ret;
}
int read_chips_signal_status(u8 cmd)
{
    int ret = 0;
    mutex_lock(&lock_chipe_config);
    ret = tw9912_read_chips_status(cmd);//return 0 or 1  ,if back 1 signal have change
    mutex_unlock(&lock_chipe_config);
    return ret;//have change return 1 else retrun 0
}
int read_chips_signal_status_fast(u8 *valu)
{

    int ret = 0;
    mutex_lock(&lock_chipe_config);
    ret = read_tw9912_chips_status_fast(valu);//return 0 or 1  ,if back 1 signal have change
    mutex_unlock(&lock_chipe_config);
    return ret;//have change return 1 else retrun 0
}
int IfInputSignalNotStable(void)
{
    int i;
    vedio_format_t ret2;
    static u8 IfInputSignalNotStable_count = 1;
    static u8 SignalNotFind_count = 0;
    video_config_debug("IfInputSignalNotStable(info_com_top_Channel =%d)\n", info_com_top_Channel);
    if( (info_com_top_Channel != SEPARATION && info_com_top_Channel != YIN2))
    {
        //YIN3 CVBS
        if(read_chips_signal_status(1) == 0 )
        {
            video_config_debug("Signal is good\n");
            goto SIGNALISGOOD;
        }
        SignalNotFind_count = 0;
        if(IfInputSignalNotStable_count == 2)
        {
            IfInputSignalNotStable_count = 1;    //if IfInputSignalNotStable_count ==1 the is first find signal bad,
            goto SIGNALINPUT;
        }
        //IfInputSignalNotStable_count ==2 is signal recovery
        IfInputSignalNotStable_count ++;
        video_config_debug("signal astable\n");
        mutex_lock(&lock_chipe_config);
        //Disabel_video_data_out();
        //tc358746xbg_data_out_enable(0);
        mutex_unlock(&lock_chipe_config);
        while(global_camera_working_status)//camera not exit
        {
            SignalNotFind_count++;
            if(SignalNotFind_count > 50) goto BREAKTHEWHILE;
            ret2 = tw9912_testing_channal_signal(info_com_top_Channel) ;
            switch( ret2 )
            {
            case NTSC_I:
                video_config_debug("find signal NTSC_I\n");
                goto BREAKTHEWHILE;
                break;
            case PAL_I:
                video_config_debug("find signal PAL_I\n");
                goto BREAKTHEWHILE;
                break;
            case NTSC_P:
                video_config_debug("find signal NTSC_P\n");
                goto BREAKTHEWHILE;
                break;
            default :
                for(i = 0; i < 10; i++)
                {
                    if(!global_camera_working_status) goto BREAKTHEWHILE;
                    msleep(1);
                }
                video_config_debug("not signal\n");
                break;
            }
        }
BREAKTHEWHILE:
        ;
    }
    else//DVD YUV
    {
        ;
    }
SIGNALISGOOD:
SIGNALINPUT:
    return 0;
}
static void chips_config_cvbs_begin(void)
{
    tw9912_config_for_cvbs_signal(info_com_top_Channel);
    video_image_config_begin();

    lidbg("global_video_channel_flag = %x\n", global_video_channel_flag);
    if(global_video_channel_flag == TV_4KO)//只针对TV的特殊配置
    {
        u8 Tw9912_register_valu[] = {0x08, 0x17,};
        tw9912_write((char *)Tw9912_register_valu);
        Tw9912_register_valu[0] = 0xa;
        Tw9912_register_valu[1] = 0x1e;
        tw9912_write((char *)Tw9912_register_valu);
	 last_vide_format = TV_4KO;
    }
    else if(global_video_channel_flag == AUX_4KO)//只针对AUX的特殊配置
    {
        u8 Tw9912_register_valu[] = {0x08, 0x15,};
        tw9912_write((char *)Tw9912_register_valu);
	 last_vide_format = AUX_4KO;
    }

    if(info_vedio_channel_t <= SEPARATION)
    {
        switch (signal_is_how[info_vedio_channel_t].Format)
        {
        case NTSC_I:
            SOC_Write_Servicer(VIDEO_NORMAL_SHOW);
            tc358746xbg_config_begin(NTSC_I);
            //tc358746xbg_config_begin(PAL_Interlace);
            break;
        case PAL_I:
            SOC_Write_Servicer(VIDEO_NORMAL_SHOW);
            tc358746xbg_config_begin(PAL_I);
            break;
        case NTSC_P:
            SOC_Write_Servicer(VIDEO_NORMAL_SHOW);
            tc358746xbg_config_begin(NTSC_P);
            break;
        case PAL_P:
            SOC_Write_Servicer(VIDEO_NORMAL_SHOW);
            tc358746xbg_config_begin(PAL_P);
            break;
        default :
            lidbg("video not signal input..\n");
            SOC_Write_Servicer(VIDEO_SHOW_BLACK);//若设置啦该属性，在flycamera.cpp中会使用到该属性，这在延时如干时间后将重做 chips_config_begin(vedio_format_t config_pramat)
            tc358746xbg_config_begin(COLORBAR + TC358746XBG_BLACK); //blue
            break;
        }
    }//if(info_vedio_channel_t<=SEPARATION)
    else
    {
        lidbg("Video_init_config:TW9912 not config!\n");
        tc358746xbg_config_begin(COLORBAR);
    }
}
static void chips_config_yuv_begin(void)
{
    tw9912_config_array_NTSCp();//刷寄存器数组
    video_image_config_begin();//刷颜色寄存器的配置
    SOC_Write_Servicer(VIDEO_NORMAL_SHOW);//设置一个属性，该属性将在Flycamera.cpp中使用
    tc358746xbg_config_begin(NTSC_P);
    lidbg("Vedio Format Is NTSCp\n");
}
void chips_config_begin(vedio_format_t config_pramat)// 配置主入口在内核的ov5647_truly_cm6868_v4l2.c 的xxx_setting_xx中调用
{
    lidbg("\n\nVideo Module Build Time: %s %s  %s \n", __FUNCTION__, __DATE__, __TIME__);
    video_config_debug("tw9912:config channal is %d\n", info_com_top_Channel);
    mutex_lock(&lock_chipe_config);
    if(info_com_top_Channel == SEPARATION)//将工主动修改啦这个参数
        chips_config_yuv_begin();//DVD
    else	if(config_pramat != STOP_VIDEO)
        chips_config_cvbs_begin();//其他视频（倒车、TV、AUX。。。）
    else
    {
        lidbg("TW9912:warning -->config_pramat == STOP_VIDEO\n");
        tc358746xbg_config_begin(COLORBAR);//这个流程目前没有使用，当时给TC358调试用的
    } 
    up(&sem);//该信号量目前还没有使用
    mutex_unlock(&lock_chipe_config);
}
void tc358746xbg_show_color(u8 color_flag)
{
    mutex_lock(&lock_chipe_config);
    lidbg("flyvideo:error tc358746xbg_show_color()\n");
    tc358746xbg_config_begin(color_flag);
    mutex_unlock(&lock_chipe_config);
}
