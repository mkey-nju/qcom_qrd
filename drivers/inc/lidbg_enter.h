
#ifndef _FLY_HAL__
#define _FLY_HAL__

#ifdef BUILD_DRIVERS
#include <devices.h>
#include <lidbg_bpmsg.h>
#include <lidbg_fastboot.h>
#include <lidbg_ts_probe.h>
#include <lidbg_monkey.h>
#include <lidbg_acc.h>
#include <lidbg_lpc.h>
#include <tw9912.h>
#else
#include <linux/miscdevice.h>
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
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/syscalls.h>
#include <asm/system.h>
#include <linux/fb.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/semaphore.h>
#include <linux/kfifo.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/vmalloc.h>
#endif

#ifndef _MDP_STRUCT__
#define _MDP_STRUCT__
typedef enum
{
    MDP_BLOCK_POWER_OFF,
    MDP_BLOCK_POWER_ON
} MDP_BLOCK_POWER_STATE;

typedef enum
{
    MDP_CMD_BLOCK,
    MDP_OVERLAY0_BLOCK,
    MDP_MASTER_BLOCK,
    MDP_PPP_BLOCK,
    MDP_DMA2_BLOCK,
    MDP_DMA3_BLOCK,
    MDP_DMA_S_BLOCK,
    MDP_DMA_E_BLOCK,
    MDP_OVERLAY1_BLOCK,
    MDP_OVERLAY2_BLOCK,
    MDP_MAX_BLOCK
} MDP_BLOCK_TYPE;

#endif


#define LOG_ALL (3)
#define LOG_CONT    (4)
#define WAKEUP_KERNEL (10)
#define SUSPEND_KERNEL (11)

#define USB_RST_ACK (88)

#define LOG_DVD_RESET (64)
#define LOG_CAP_TS_GT811 (65)
#define LOG_CAP_TS_FT5X06 (66)
#define LOG_CAP_TS_FT5X06_SKU7 (67)
#define LOG_CAP_TS_RMI (68)
#define LOG_CAP_TS_GT801 (69)
#define CMD_FAST_POWER_OFF (70)
#define LOG_CAP_TS_GT911 (71)
#define LOG_CAP_TS_FTX06 (73)
#define LOG_CAP_TS_GT910 (72)
#define UMOUNT_USB (80)
#define VIDEO_SET_PAL (81)
#define VIDEO_SET_NTSC (82)

#define CMD_FLY_POWER_OFF (101)


#define VIDEO_SHOW_BLACK (85)
#define VIDEO_NORMAL_SHOW (86)

#define UBLOX_EXIST   (88)
#define CMD_ACC_OFF_PROPERTY_SET (89)

#define VIDEO_PASSAGE_AUX (90)
#define VIDEO_PASSAGE_ASTERN (91)
#define VIDEO_PASSAGE_DVD (92)

#define CMD_ACC_OFF (93)
#define CMD_ACC_ON (94)
#define CMD_IGNORE_WAKELOCK (95)

#define NOTIFIER_MAJOR_SYSTEM_STATE (5)
#define NOTIFIER_MINOR_SYSTEM_OFF (0)
#define NOTIFIER_MINOR_SYSTEM_ON (1)



#if (defined(BUILD_SOC) || defined(BUILD_CORE) || defined(BUILD_DRIVERS))
#define NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE (110)
#define NOTIFIER_MINOR_XXX (0)

#define NOTIFIER_MAJOR_ACC_EVENT (111)
#define NOTIFIER_MINOR_ACC_ON (0)
#define NOTIFIER_MINOR_ACC_OFF (1)
#define NOTIFIER_MINOR_SUSPEND_PREPARE  (2)
#define NOTIFIER_MINOR_SUSPEND_UNPREPARE (3)
#define NOTIFIER_MINOR_POWER_OFF (4)
#define NOTIFIER_MINOR_BOOT_COMPLETE  (5)

extern  bool recovery_mode;
#else
#define NOTIFIER_VALUE(major,minor)  (((major)&0xffff)<<16 | ((minor)&0xffff))

#define lidbg_io(fmt,...) //do{SOC_IO_Uart_Send(IO_UART_DELAY_245_115200,fmt,##__VA_ARGS__);}while(0)

#define BEGIN_KMEM do{old_fs = get_fs();set_fs(get_ds());}while(0)
#define END_KMEM   do{set_fs(old_fs);}while(0)


#define KEY_RELEASED    (0)
#define KEY_PRESSED      (1)
#define KEY_PRESSED_RELEASED   ( 2)

typedef irqreturn_t (*pinterrupt_isr)(int irq, void *dev_id);

#if 1
#define ADC_MAX_CH (8)
struct fly_smem
{
    unsigned char reserved[4];
    unsigned int ch[ADC_MAX_CH];
    int reserved2;
    int bl_value;
};
#else

struct fly_smem
{
    unsigned char bp2ap[16];
    unsigned char ap2bp[8];
};
#endif

typedef enum
{
    YIN0 = 0, //now is use Progressive Yin
    YIN1,//not use
    YIN2,// now is use Y of SEPARATION / DVD_CVBS
    YIN3,//now is use AUX/BACK_CVBS
    SEPARATION,//Progressive
    NOTONE,
} vedio_channel_t;
typedef enum
{
    AUX_4KO = 0,
    TV_4KO,
    ASTREN_4KO,
    DVD_4KO,
    OTHER_CHANNEL_4KO,
} vedio_channel_t_2;
typedef enum
{
    NTSC_I = 1,
    PAL_I,
    NTSC_P,
    PAL_P,
    STOP_VIDEO,
    COLORBAR,
    OTHER,
} vedio_format_t;
typedef enum
{
    BRIGHTNESS = 1,//ok
    CONTRAST,//ok
    SHARPNESS,
    CHROMA_U,//ok
    CHROMA_V,
    HUE,//ok
    //Positive value results in red hue and negative value gives green hue.
    //These bits control the color hue. It is in 2\u201fs complement form with 0 being the center 00 value.
} Vedio_Effect;


typedef enum
{
    PM_STATUS_EARLY_SUSPEND_PENDING,
    PM_STATUS_SUSPEND_PENDING,
    PM_STATUS_RESUME_OK,
    PM_STATUS_LATE_RESUME_OK,
    PM_STATUS_READY_TO_PWROFF,
    PM_STATUS_READY_TO_FAKE_PWROFF,

} LIDBG_FAST_PWROFF_STATUS;

typedef enum
{
    FLY_ACC_ON,
    FLY_ACC_OFF,
    FLY_READY_TO_SUSPEND,
    FLY_SUSPEND,
} FLY_SYSTEM_STATUS;

#endif


struct lidbg_fn_t
{
    //io
    /*
     GPIO TLMM: Pullup/Pulldown
    enum {
    	GPIO_CFG_NO_PULL,
    	GPIO_CFG_PULL_DOWN,
    	GPIO_CFG_KEEPER,
    	GPIO_CFG_PULL_UP,
    };

    GPIO TLMM: Drive Strength
    enum {
    	GPIO_CFG_2MA,
    	GPIO_CFG_4MA,
    	GPIO_CFG_6MA,
    	GPIO_CFG_8MA,
    	GPIO_CFG_10MA,
    	GPIO_CFG_12MA,
    	GPIO_CFG_14MA,
    	GPIO_CFG_16MA,
    };

    */
    void (*pfnSOC_IO_Output) (unsigned int group, unsigned int index, bool status);
    bool (*pfnSOC_IO_Input) (unsigned int group, unsigned int index, unsigned int pull);
    void (*pfnSOC_IO_Output_Ext)(unsigned int group, unsigned int index, bool status, unsigned int pull, unsigned int drive_strength);
    bool (*pfnSOC_IO_Config)(unsigned int index, bool direction, unsigned int pull, unsigned int drive_strength);

    //i2c
    /*
    7bit i2c sub_addr
    bus_id : 0/1
    return how many bytes read/write
    when err , <0
    */
    int (*pfnSOC_I2C_Send) (int bus_id, char chip_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec_Simple)(int bus_id, char chip_addr, char *buf, unsigned int size);

    int (*pfnSOC_I2C_Rec_SAF7741)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Send_TEF7000)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    int (*pfnSOC_I2C_Rec_TEF7000)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);


    //io-irq
    //interrupt_type

    /*

    #define IRQF_TRIGGER_RISING	0x00000001
    #define IRQF_TRIGGER_FALLING	0x00000002
    #define IRQF_TRIGGER_HIGH	0x00000004
    #define IRQF_TRIGGER_LOW	0x00000008

    */
    bool (*pfnSOC_IO_ISR_Add)(unsigned int irq, unsigned int interrupt_type, pinterrupt_isr func, void *dev);
    bool (*pfnSOC_IO_ISR_Enable)(unsigned int irq);
    bool (*pfnSOC_IO_ISR_Disable)(unsigned int irq);
    bool (*pfnSOC_IO_ISR_Del )(unsigned int irq);

    //ad
    /*
     return 0 when err
    // 0-AIN2
    // 1-AIN3
    // 2-AIN4
    // 3-REM1
    // 4-REM2
    */
    bool (*pfnSOC_ADC_Get)(unsigned int channel , unsigned int *value);


    //key
    void (*pfnSOC_Key_Report)(unsigned int key_value, unsigned int type);
    //bl
    /*level : 0~255   0-dim, 255-bright*/
    int (*pfnSOC_BL_Set)( unsigned int level);

    //pwr
    void (*pfnSOC_PWR_ShutDown)(void);
    int (*pfnSOC_PWR_GetStatus)(void);
    void (*pfnSOC_PWR_SetStatus)(LIDBG_FAST_PWROFF_STATUS status);

    //
    void (*pfnSOC_Write_Servicer)(int cmd );

    //video
    void (*pfnlidbg_video_main)(int argc, char **argv);
    void (*pfnvideo_io_i2c_init)(void);
    int (*pfnflyVideoInitall)(unsigned char  Channel);
    vedio_format_t (*pfnflyVideoTestSignalPin)(unsigned char  Channel);
    int (*pfnflyVideoImageQualityConfig)(Vedio_Effect cmd , unsigned char  valu);
    void (*pfnvideo_init_config)(vedio_format_t config_pramat);

    //display/touch
    int (*pfnSOC_Display_Get_Res)(unsigned int *screen_x, unsigned int *screen_y);

    //lpc
    void (*pfnSOC_LPC_Send)(unsigned char *p, unsigned int len);

    //video
    vedio_format_t (*pfncamera_open_video_signal_test)(void);
    void (*pfncamera_open_video_color)(u8 color_flag);
    vedio_format_t pfnglobal_video_format_flag;
    vedio_channel_t_2 pfnglobal_video_channel_flag;


    //dev
    void (*pfnSOC_Dev_Suspend_Prepare)(void);

    //wakelock
    bool (*pfnSOC_PWR_Ignore_Wakelock)(void);

    //mic
    void (*pfnSOC_Mic_Enable)(bool enable);
    //video
    int (*pfnread_tw9912_chips_signal_status)(void);



    // this func must put last!!
    //void (*pfnlidbg_version_show)(void);
    //video
    int pfnglobal_video_camera_working_status;
    //LCD
    void (*pfnSOC_F_LCD_Light_Con)(unsigned int iOn);
    //fake suspend
    void (*pfnSOC_Fake_Register_Early_Suspend)(struct early_suspend *handler);
    //video
    int (*pfnVideoReset)(void);
    //add huang	SOC_I2C_Rec_2B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
    int (*pfnSOC_I2C_Rec_2B_SubAddr)(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
    //I2c_Rate	 i2c_api_set_rate(int  bus_id, int rate)
    int (*pfnSOC_I2C_Set_Rate)(int  bus_id, int rate);

    void (*pfnSOC_IO_Uart_Send)( u32 baud, const char *fmt, ... );

    void (*pfnSOC_Get_WakeLock)(struct list_head *p);


    struct fly_smem *(*pfnSOC_Get_Share_Mem)(void);
    void (*pfnSOC_System_Status)(FLY_SYSTEM_STATUS status);


    int (*pfnSOC_Get_CpuFreq)(void);

    void (*pfnSOC_LCD_Reset)(void);


    void (*pfnSOC_WakeLock_Stat)(bool lock, const char *name);

    //screan_off :0 screan_on :1 suspendon:2 suspendoff:3
    void (*pfnHal_Acc_Callback)(int para);

    int (*pfnNotifier_Call)(unsigned long val);

    bool (*pfnSOC_PWR_Status_Accon)(void);

    void(*pfn_Call_mdp_pipe_ctrl)(MDP_BLOCK_TYPE block, MDP_BLOCK_POWER_STATE state, unsigned int isr);

};

struct lidbg_pvar_t
{
    //all pointer
    rwlock_t *pvar_tasklist_lock;
    int temp;
    FLY_SYSTEM_STATUS system_status;
    int machine_id;
    int cpu_freq;
    bool is_fly;
    unsigned int flag_for_15s_off;
    bool is_usb11;
    bool fake_suspend;
    bool acc_flag;

};

struct lidbg_hal
{
    union
    {
        struct lidbg_fn_t soc_func_tbl;
        unsigned char reserve[256];
    };
    union
    {
        struct lidbg_pvar_t soc_pvar_tbl;
        unsigned char reserve1[128];
    };
};



#define LIDBG_DEV_CHECK_READY  (plidbg_dev != NULL)

#define LIDBG_DEFINE  struct lidbg_hal *plidbg_dev = NULL

#define g_var  plidbg_dev->soc_pvar_tbl

#define LIDBG_GET  \
 	do{\
	 mm_segment_t old_fs;\
	 struct file *fd = NULL;\
	 printk("lidbg:call LIDBG_GET by %s\n",__FUNCTION__);\
	 while(1){\
	 	printk("lidbg: %s:%s try open lidbg_hal!\n",__FILE__,__FUNCTION__);\
	 	fd = filp_open("/dev/lidbg_hal", O_RDWR, 0);\
	 	printk("lidbg:get fd=%x\n",(int)fd);\
	    if((fd == NULL)||((int)fd == 0xfffffffe)){printk("lidbg:get fd fail!\n");msleep(500);}\
	    else break;\
	 }\
	 BEGIN_KMEM;\
	 fd->f_op->read(fd, (void*)&plidbg_dev, 4 ,&fd->f_pos);\
	 END_KMEM;\
	filp_close(fd,0);\
	if(plidbg_dev == NULL)\
	{\
		printk(KERN_CRIT"LIDBG_GET fail!\n");\
	}\
}while(0)


#define LIDBG_THREAD_DEFINE   \
    struct lidbg_hal *plidbg_dev = NULL;\
	static struct task_struct *getlidbg_task;\
	static int thread_getlidbg(void *data);\
	int thread_getlidbg(void *data)\
	{\
		LIDBG_GET;\
		return 0;\
	}

#define LIDBG_GET_THREAD  do{\
	getlidbg_task = kthread_create(thread_getlidbg, NULL, "getlidbg_task");\
	if(IS_ERR(getlidbg_task))\
	{\
		printk("Unable to start kernel thread.\n");\
	}else wake_up_process(getlidbg_task);\
}while(0)

#ifndef LIDBG_FLY_HAL
#include "lidbg_func_def.h"
#endif

#endif
