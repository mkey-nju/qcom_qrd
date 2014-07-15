#include "lidbg.h"
#include "video_init_config.h"
#include "lidbg_videoin_main.h"

#define MAJOR_Tw9912 0
#define DEV_NAME "tw9912config"

extern u8 global_debug_thread;
extern tw9912_run_flag_t tw912_run_sotp_flag;
extern vedio_channel_t info_vedio_channel_t;
extern tw9912_signal_t signal_is_how[5];
extern vedio_channel_t info_com_top_Channel;
extern struct lidbg_hal *plidbg_dev; //add by huangzongqiang
extern struct tc358746xbg_register_t tc358746xbg_show_colorbar_config_user_tab[2];

static u8 tw9912_read_func_read_data = 0;
dev_t tw9912_dev;
struct class *tw9912_class;
struct cdev *tw9912_cdev;

LIDBG_DEFINE;

tw9912info_t global_tw9912_info_for_NTSC_I =
{
    0x08,
    0x17,
    false,//true is find black line;
    true,//true is neet again find the black line;
    true,//first open the camera
};
tw9912info_t global_tw9912_info_for_PAL_I =
{
    0x08,
    0x17,
    false,//true is find black line;
    true,//true is neet again find the black line;
    true,//first open the camera
};
#ifdef DEBUG_TC358
void terminal_tc358746xbg_config(int argc, char **argv)
{//使用方法请阅读该文件目录下的README
    lidbg("terminal_tc358746xbg_config()\n");
    if(!strcmp(argv[1], "w"))
    {
        u8 buf[4] = {0, 0, 0, 0}, flag;
        u16 sub_addr;
        u32 valu;

        sub_addr = simple_strtoul(argv[2], 0, 0);
        valu = simple_strtoul(argv[3], 0, 0);
        flag =  simple_strtoul(argv[4], 0, 0);
        if(flag == 16)
        {
            lidbg("TC358 write in adder=0x%02x, valu=0x%02x\n", sub_addr, valu);
            tc358746xbg_write(&sub_addr, &valu, register_value_width_16);
            tc358746xbg_read(sub_addr, buf, register_value_width_16);
            lidbg("TC358 read back adder=0x%02x, valu=0x%02x%02x%02x%02x\n", sub_addr, buf[2], buf[3], buf[0], buf[1]);
        }
        else
        {
            lidbg("TC358 write in adder=0x%02x, valu=0x%02x\n", sub_addr, valu);
            tc358746xbg_write(&sub_addr, &valu, register_value_width_32);
            tc358746xbg_read(sub_addr, buf, register_value_width_32);
            lidbg("TC358 read back adder=0x%02x, valu=0x%02x%02x%02x%02x\n", sub_addr, buf[2], buf[3], buf[0], buf[1]);
        }

    }
    else if(!strcmp(argv[1], "r"))
    {
        u8 flag, buf[4] = {0, 0, 0, 0};
        u16 sub_addr;
        sub_addr = simple_strtoul(argv[2], 0, 0);
        flag =  simple_strtoul(argv[3], 0, 0);
        if(flag == 16)
        {
            tc358746xbg_read(sub_addr, buf, register_value_width_16);
            lidbg("TC358 read back adder=0x%02x, valu=0x%02x%02x%02x%02x\n", sub_addr, buf[2], buf[3], buf[0], buf[1]);
        }
        else
        {
            tc358746xbg_read(sub_addr, buf, register_value_width_32);
            lidbg("TC358 read back adder=0x%02x, valu=0x%02x%02x%02x%02x\n", sub_addr, buf[2], buf[3], buf[0], buf[1]);
        }
    }
    else if(!strcmp(argv[1], "ShowColor"))
    {
        u16 cmd;
        lidbg("call tc358746xbg_config_begin();\n\n");
        cmd = simple_strtoul(argv[2], 0, 0);
        tc358746xbg_config_begin(cmd);
    }
    else if(!strcmp(argv[1], "ShowColordebug"))
    {
        u16 cmd;
        lidbg("call tc358746xbg_config_begin();\n\n");
        cmd = simple_strtoul(argv[2], 0, 0);
        tc358746xbg_show_colorbar_config_user_tab[0].add_val = cmd;
        cmd = simple_strtoul(argv[3], 0, 0);
        tc358746xbg_show_colorbar_config_user_tab[1].add_val = cmd;
        tc358746xbg_config_begin(7);
    }

}
#endif
#ifdef DEBUG_TW9912
void terminal_tw9912_config(int argc, char **argv)
{
    if(!strcmp(argv[1], "w"))
    {
        u8 buf[2];

        u16 sub_addr;
        u32 valu;
        sub_addr = simple_strtoul(argv[2], 0, 0);
        valu = simple_strtoul(argv[3], 0, 0);
        buf[0] = sub_addr;
        buf[1] = valu;
#ifdef FLY_VIDEO_BOARD_V3
        i2c_write_byte(3, 0x44, buf , 2);
        lidbg("FLY_VIDEO_BOARD_V3 write add=0x%02x, valu=0x%02x\n", sub_addr, valu);
#else
        i2c_write_byte(1, 0x44, buf , 2);
        i2c_read_byte(1, 0x44, sub_addr , (char *)&valu, 1);
        lidbg("write add=0x%02x, valu=0x%02x\n", sub_addr, valu);
#endif
    }
    else if(!strcmp(argv[1], "r"))
    {
        u16 sub_addr;
        u8 buf[2] = {0, 0};
	 int ret = 0;
	 
        sub_addr = simple_strtoul(argv[2], 0, 0);
#ifdef FLY_VIDEO_BOARD_V3
        i2c_read_byte(3, 0x44, sub_addr , buf, 1);
        lidbg(" FLY_VIDEO_BOARD_V3 read add=0x%02x,valu=0x%02x ret=%d \n", sub_addr, buf[0], ret);
#else
        i2c_read_byte(1, 0x44, sub_addr , buf, 1);
        lidbg("read add=0x%02x,valu=0x%02x\n", sub_addr, buf[0]);
#endif
    }
    else if(!strcmp(argv[1], "set"))
    {
    	Vedio_Effect cmd = 0;
	u8 val = 0;

	cmd = simple_strtoul(argv[2], 0, 0);
	val = (u8)simple_strtoul(argv[3], 0, 0);
	
	flyVideoImageQualityConfig_in(cmd , val);
    }
    else
    {
        ;
    }
}
#endif
void lidbg_video_main(int argc, char **argv)
{
    u8 ret = 0;
    lidbg("In lidbg_video_main()\n");
    if(!strcmp(argv[0], "agian_initall"))
    {
        chips_config_begin(NTSC_P);
    }
#ifdef DEBUG_TW9912
    else if(!strcmp(argv[0], "9912"))
    {
        terminal_tw9912_config(argc, argv);
    }
#endif
    else  if(!strcmp(argv[0], "debug_thread_run"))
        global_debug_thread = !global_debug_thread;
#ifdef DEBUG_TC358
    else if(!strcmp(argv[0], "358"))
    {
        terminal_tc358746xbg_config(argc, argv);
    }
#endif
    else if(!strcmp(argv[0], "TestingPresentChannalSignal"))
    {
        ret = (u8) camera_open_video_signal_test();
        switch(ret)
        {
        case 1:
            lidbg("TestingPresentChannalSignal format :NTSC_i\n");
            break;
        case 2:
            lidbg("TestingPresentChannalSignal format :PAL_i\n");
            break;
        case 3:
            lidbg("TestingPresentChannalSignal format :NTSC_p\n");
            break;
        }
    }
    else
    {
        ;
    }
}
EXPORT_SYMBOL(lidbg_video_main);

static void set_func_tbl(void)
{
    //video
    plidbg_dev->soc_func_tbl.pfnlidbg_video_main = lidbg_video_main;
    plidbg_dev->soc_func_tbl.pfnvideo_io_i2c_init = video_io_i2c_init_in;
    plidbg_dev->soc_func_tbl.pfnflyVideoInitall = flyVideoInitall_in;
    plidbg_dev->soc_func_tbl.pfnflyVideoTestSignalPin = flyVideoTestSignalPin_in;
    plidbg_dev->soc_func_tbl.pfnflyVideoImageQualityConfig = flyVideoImageQualityConfig_in;
    plidbg_dev->soc_func_tbl.pfnvideo_init_config = chips_config_begin;
    plidbg_dev->soc_func_tbl.pfncamera_open_video_signal_test = camera_open_video_signal_test_in;
    plidbg_dev->soc_func_tbl.pfncamera_open_video_color = tc358746xbg_show_color;
    plidbg_dev->soc_func_tbl.pfnread_tw9912_chips_signal_status = read_chips_signal_status;
    plidbg_dev->soc_func_tbl.pfnVideoReset = chips_hardware_reset;
    global_video_format_flag = NTSC_I;//视频格式标志，在内核msm_sensor.c中使用
    global_video_channel_flag = TV_4KO;//视频当前通道标志，TV AUX 倒车 用的都是YIN3通道，该标志可以区分
    global_camera_working_status = 0;
}

struct early_suspend early_suspend;
static int  video_early_suspend(struct early_suspend *h)
{
    DUMP_BUILD_TIME;
    return 0;
}
static int video_late_resume(struct early_suspend *h)
{
    lidbg("video_late_resume tw9912 reset\n");
    tw9912_hardware_reset();
    flyVideoChannelInitall(YIN2); // DVD
    chips_config_begin(OTHER);//first initall tw9912 all register
    return 0;
}

static int video_dev_probe(struct platform_device *pdev)
{
    DUMP_BUILD_TIME;
#ifdef CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend = video_early_suspend;
    early_suspend.resume = video_late_resume;
    register_early_suspend(&early_suspend);
#endif
    return 0;
}
static int video_dev_remove(struct platform_device *pdev)
{
    DUMP_BUILD_TIME;
    return 0;
}
static int  video_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    DUMP_BUILD_TIME;
    return 0;
}
static int video_dev_resume(struct platform_device *pdev)
{
    lidbg("resume video chip environment variables reset\n");
    //tw9912_hardware_reset();
    global_tw9912_info_for_NTSC_I.flag = true; //true is neet again find the black line;
    global_tw9912_info_for_NTSC_I.reg_val = 0x17;
    global_tw9912_info_for_NTSC_I.this_is_first_open = true;
    global_tw9912_info_for_PAL_I.flag = true; //true is neet again find the black line;
    global_tw9912_info_for_PAL_I.reg_val = 0x12;
    global_tw9912_info_for_PAL_I.this_is_first_open = true;//first open the camera
//以上两个数组，是记录当前倒车状态下 图像向下移动寻找 黑线（用来判断分屏的参考）的9912寄存器的参素
//这些值的修改在HAL文件：FlyCamera.cpp (qcom\lidbg_qrd\android\msm8x25\8x25q_camera)	40966	2014-1-22的函数
//static int ToFindBlackLineAndSetTheTw9912VerticalDelayRegister(mm_camera_ch_data_buf_t *frame)中使用

    return 0;
}
static struct platform_driver video_driver =
{
    .probe = video_dev_probe,
    .remove = video_dev_remove,
    .suspend =  video_dev_suspend,
    .resume =  video_dev_resume,
    .driver = {
        .name = "video_devices",
        .owner = THIS_MODULE,
    },
};
struct platform_device video_devices =
{
    .name			= "video_devices",
    .id 			= 0,
};
int tw9912_node_open(struct inode *inode, struct file *filp)
{
    return 0;
}
static int tw9912_node_ioctl(struct file *filp, unsigned
                             int cmd, unsigned long arg)
{//这个函数的调用在HAL文件：FlyCamera.cpp (qcom\lidbg_qrd\android\msm8x25\8x25q_camera)	40966	2014-1-22文件中的函数
//static int FlyCameraReadTw9912StatusRegitsterValue()中使用

    /*  if(_IOC_TYPE(cmd) != TW9912_IOC_MAGIC)//_IOC_TYPE()-->//kernel\include\asm-generic
    	 return -ENOTTY;
       if (_IOC_NR(cmd) > TW9912_IOC_MAXNR)
       	 return -ENOTTY;*/
    switch (cmd)
    {
    case COPY_TW9912_STATUS_REGISTER_0X01_4USER:
        tw9912_read_func_read_data = 1;
        break;
    case AGAIN_RESET_VIDEO_CONFIG_BEGIN :
	chips_config_begin(OTHER);//FlyCamera.cpp 文件中控制视频芯片重新配置
	lidbg("at last open backcar not signal but app opened camera,so, now again config the chips\n");
	break;
    default:
        return  - EINVAL;
    }
    return 0;
}
static ssize_t tw9912_node_read(struct file *filp, char __user *buf, size_t size,
                                loff_t *ppos)
{
    unsigned int count = size;
    ssize_t ret;
    if(!tw9912_read_func_read_data)//该分支的执行流向受 tw9912_node_ioctl函数的控制
    {
        if(signal_is_how[info_vedio_channel_t].Format == NTSC_I)
        {
            if (copy_to_user(buf, (void *)(&global_tw9912_info_for_NTSC_I), count))
            {
                lidbg("TW9912config : copy_to_user ERR\n");
                ret =  - EFAULT;
            }
            else
            {
                lidbg("TW9912config : NTSC_I paramter copy to user : %.2x%.2x\n", \
                       global_tw9912_info_for_NTSC_I.reg, global_tw9912_info_for_NTSC_I.reg_val);
                ret = count;
            }
        }
        else if(signal_is_how[info_vedio_channel_t].Format == PAL_I)
        {
            if (copy_to_user(buf, (void *)(&global_tw9912_info_for_PAL_I), count))
            {
                lidbg("TW9912config : copy_to_user ERR\n");
                ret =  - EFAULT;
            }
            else
            {
                lidbg("TW9912config : PAL_I paramter copy to user : %.2x%.2x\n", \
                       global_tw9912_info_for_PAL_I.reg, global_tw9912_info_for_PAL_I.reg_val);
                ret = count;
            }
        }
        else
        {
            if (copy_to_user(buf, (void *)(&global_tw9912_info_for_PAL_I), count))
            {
                lidbg("TW9912config : copy_to_user ERR\n");
                ret =  - EFAULT;
            }
            else
            {
                lidbg("TW9912config : PAL_I paramter copy to user : 0x%.2x\n", \
                       global_tw9912_info_for_PAL_I.this_is_first_open);
                ret = count;
            }
        }
    }
    else if(tw9912_read_func_read_data == 1)
    {
        u8 valu;
        int ret = 0;
        tw9912_read_func_read_data = 0 ;// at every turn have ioct once
        ret = read_chips_signal_status_fast(&valu);//读取现在视频信号的状态，判断信号的质量
        if(ret == NACK)
        {
            lidbg("Worning read tw9912 NACK\n");
            valu = 0x68;//the value representative vedio signal is good
        }
        if (copy_to_user(buf, (void *)(&valu), sizeof(u8)))
        {
            lidbg("TW9912config : copy_to_user ERR\n");
            ret =  - EFAULT;
        }
    }
    return count;
}
static ssize_t tw9912_node_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    u8 para[] = {0x0, 0x0,};

    if(signal_is_how[info_vedio_channel_t].Format == NTSC_I)
    {
        if (copy_from_user(&global_tw9912_info_for_NTSC_I, buf, count))
        {
            lidbg("TW9912config : copy_from_user ERR\n");
        }
        if(global_tw9912_info_for_NTSC_I.reg_val  > 0x10 && global_tw9912_info_for_NTSC_I.reg == 0x8\
                && info_com_top_Channel != SEPARATION && info_com_top_Channel != YIN2)
        {
            lidbg("TW9912config : paramter is %.2x%.2x NOW write in the register\n", \
                   global_tw9912_info_for_NTSC_I.reg, global_tw9912_info_for_NTSC_I.reg_val);
            para[0] = global_tw9912_info_for_NTSC_I.reg;
            para[1] = global_tw9912_info_for_NTSC_I.reg_val;
            tw9912_write(para);
        }
        else
        {
            lidbg("TW9912config : paramter is %.2x%.2x NOT write in the regitster\n", \
                   global_tw9912_info_for_NTSC_I.reg, global_tw9912_info_for_NTSC_I.reg_val);
        }
    }
    else if(signal_is_how[info_vedio_channel_t].Format == PAL_I)
    {
        if (copy_from_user(&global_tw9912_info_for_PAL_I, buf, count))
        {
            lidbg("TW9912config : copy_from_user ERR\n");
        }

        if(global_tw9912_info_for_PAL_I.reg_val  > 0x10 && global_tw9912_info_for_PAL_I.reg == 0x8\
                && info_com_top_Channel != SEPARATION && info_com_top_Channel != YIN2)
        {
            lidbg("TW9912config : paramter is %.2x%.2x NOW write in the register\n", \
                   global_tw9912_info_for_PAL_I.reg, global_tw9912_info_for_PAL_I.reg_val);
            para[0] = global_tw9912_info_for_PAL_I.reg;
            para[1] = global_tw9912_info_for_PAL_I.reg_val;
            tw9912_write(para);
        }
        else
        {
            lidbg("TW9912config : paramter is %.2x%.2x NOT write in the regitster\n", \
                   global_tw9912_info_for_PAL_I.reg, global_tw9912_info_for_PAL_I.reg_val);
        }

    }
    else
    {
        tw9912info_t global_tw9912_info;
        if (copy_from_user(&global_tw9912_info, buf, count))
        {
            lidbg("TW9912config : copy_from_user ERR\n");
        }
        global_tw9912_info_for_PAL_I.this_is_first_open = global_tw9912_info.this_is_first_open;
        lidbg("TW9912config :Synchronous DVD open count\n");
    }
    return 0;
}
static const struct file_operations tw9912_node_fops =
{
    .owner = THIS_MODULE,
    .llseek = NULL,
    .read = tw9912_node_read,
    .write = tw9912_node_write,
    .open = tw9912_node_open,
    .unlocked_ioctl = tw9912_node_ioctl,
    .release = NULL,
};
int tw9912_index_init(void)
{
    int err;
    err = alloc_chrdev_region(&tw9912_dev, 0, 1, DEV_NAME); //\u5411\u5185\u6838\u6ce8\u518c\u4e00\u4e2a\u8bbe\u5907
    if(err < 0)
    {
        lidbg("register_chrdev tw9912config is error!\n ");
        return err;
    }
    tw9912_cdev = cdev_alloc();
    tw9912_cdev->owner = THIS_MODULE;
    tw9912_cdev->ops = &tw9912_node_fops;
    err = cdev_add(tw9912_cdev, tw9912_dev, 1);
    if(err)
        lidbg(KERN_NOTICE "Error for cdec_add() -->adding tw9912config %d \n", MINOR(tw9912_dev));
    tw9912_class = class_create(THIS_MODULE, "tw9912config_class");
    if(IS_ERR(tw9912_class))
    {
        lidbg("Err: failed in creating mlidbg class.\n");
        goto fail_class_create;
    }
    device_create(tw9912_class, NULL, tw9912_dev, NULL, DEV_NAME); //\u521b\u5efa\u4e00\u4e2a\u8bbe\u5907\u8282\u70b9\uff0c\u8282\u70b9\u540d\u4e3aDEV_NAME "%d"
    lidbg("tw9912config device_create ok\n");
    lidbg_chmod("/dev/tw9912config");
    return 0;
fail_class_create:
    unregister_chrdev_region(tw9912_dev, 1);
    return -1;
}
int lidbg_video_init(void)
{
    int err;
    lidbg("lidbg_video_init modules ismod\n");
    LIDBG_GET;
    err = tw9912_index_init();
    if(err < 0)
        return err;

    set_func_tbl();
    video_io_i2c_init();
    tw9912_hardware_reset();
    flyVideoChannelInitall(YIN2); // DVD
    chips_config_begin(OTHER);//first initall tw9912 all register
    platform_driver_register(&video_driver);
    platform_device_register(&video_devices);
    return 0;
}
void tw9912_node_exit( void )
{
    lidbg("tw9912config_exit_now.....\n");
    cdev_del(tw9912_cdev);
    device_destroy(tw9912_class, tw9912_dev);
    class_destroy(tw9912_class);
    unregister_chrdev_region(tw9912_dev, 1);
}

void lidbg_video_deinit(void)
{
    lidbg("lidbg_video_deinit module exit.....\n");
    tw9912_node_exit();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");
module_init(lidbg_video_init);
module_exit(lidbg_video_deinit);



