/* Copyright (c) 2012, swlee */

#include "lidbg.h"
LIDBG_DEFINE;

int led_en ;
int temp_log_freq;

#define  USB_1_1 "/flysystem/flyconfig/default/usb11"
#define LIDBG_GPIO_PULLUP  GPIO_CFG_PULL_UP

static struct task_struct *resume_task;

#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#else
static struct task_struct *Thermal_task = NULL;
static int fan_onoff_temp;
static bool flag_fan_run_statu = false;
static bool hal_fan_on = true;
#endif

static DECLARE_COMPLETION(usb_resume);

int thread_dev_init(void *data);
int thread_led(void *data);
int thread_key(void *data);
int thread_pwr(void *data);
int thread_usb(void *data);
static int thread_resume(void *data);
int thread_usb_11(void *data);

void fly_devices_init(void);
static int lidbg_event(struct notifier_block *this, unsigned long event, void *ptr);
static struct file_operations dev_fops;

int platform_id;
bool i2c_c_ctrl = 0;
int i2c_ctrl = 0;

struct platform_devices_resource devices_resource;

static struct task_struct *udisk_event_task;
static struct completion udisk_event_wait;


static struct notifier_block lidbg_notifier =
{
    .notifier_call = lidbg_event,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void devices_early_suspend(struct early_suspend *handler);
static void devices_late_resume(struct early_suspend *handler);
struct early_suspend early_suspend;

#endif

bool suspend_test = 0;
bool suspend_flag = 0;

wait_queue_head_t read_wait;
u8 audio_data_for_hal[2];

void cb_password_add_udisk(char *password )
{
    lidbgerr("\n--------------usb add event-----\n\n\n");
    lidbg_readwrite_file("/sys/block/sda/uevent", NULL, "add", sizeof("add") - 1);
}

void cb_password_remove_udisk(char *password )
{
    lidbgerr("\n--------------usb remove event-----\n\n\n");
    lidbg_readwrite_file("/sys/block/sda/uevent", NULL, "remove", sizeof("remove") - 1);
}
static int usb_event(struct notifier_block *nb, unsigned long action, void *data)
{
    switch (action)
    {
    case USB_DEVICE_ADD:
        //complete(&udisk_event_wait);
        break;
    case USB_DEVICE_REMOVE:
        break;
    }
    return NOTIFY_OK;
}

static struct notifier_block usb_nb_event =
{
    .notifier_call = usb_event,
};

static int thread_udisk_uevent(void *data)
{
    while(!kthread_should_stop())
    {
        if(!wait_for_completion_interruptible(&udisk_event_wait))
        {
            ssleep(5);
            if(fs_get_file_size("/mnt/usbdisk") == 0)
            {
                lidbgerr("--------------usb add event-----\n\n\n");
                lidbg_readwrite_file("/sys/block/sda/uevent", NULL, "add", sizeof("add") - 1);
            }
        }
    }
    return 1;
}



int thread_usb_delay_enable(void *data)
{
    if(!recovery_mode)
        ssleep(20);
    else
        lidbg("diable thread_usb_delay_enable:%d\n", recovery_mode);
    USB_WORK_ENABLE;
    return 0;
}
int thread_usb_11(void *data)
{
    USB_WORK_DISENABLE;
    g_var.is_usb11 = 0;
    ssleep(15);
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            if( !g_var.is_usb11)
            {
                lidbg("\n\nTranslate to usb1.1 modes\n\n");
                USB_ID_HIGH_DEV;
                msleep(1000);
                //USB_WORK_ENABLE;
                USB_ID_LOW_HOST;
                msleep(1000);
            }
            else
            {
                //USB_WORK_DISENABLE;
                //msleep(1000);
                ssleep(10);
                USB_WORK_ENABLE;
                break;
            }
        }
        else
            schedule_timeout(HZ);
    }

    return 0;
}

int thread_usb(void *data)
{
    while(1)
    {
        wait_for_completion(&usb_resume);
        msleep(5000);
        USB_WORK_ENABLE;
    }
    return 0;
}

void cb_password_enable_usb(char *password )
{
    USB_WORK_ENABLE;
}
void cb_password_disable_usb(char *password )
{
    USB_WORK_DISENABLE;
}

void unmute_ns(void)
{
    lidbg("[futengfei].unmute_ns");
    audio_data_for_hal[0] = 0x01;
    audio_data_for_hal[1] = 0x00;
    wake_up_interruptible(&read_wait);
}
void mute_s(void)
{
    lidbg("[futengfei].mute_s");
    audio_data_for_hal[0] = 0x01;
    audio_data_for_hal[1] = 0x01;
    wake_up_interruptible(&read_wait);
}

int read_proc_dev(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;

    lidbg("USB_ID_HIGH_DEV\n");
    USB_ID_HIGH_DEV;
    len  = sprintf(buf, "usb_set_dev\n");

    return len;
}

int read_proc_host(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;
    lidbg("USB_ID_LOW_HOST\n");
    USB_ID_LOW_HOST;
    len  = sprintf(buf, "usb_set_host\n");
    return len;
}

static void create_new_proc_entry_usb_dev(void)
{
    create_proc_read_entry("usb_dev", 0, NULL, read_proc_dev, NULL);
}

static void create_new_proc_entry_usb_host(void)
{
    create_proc_read_entry("usb_host", 0, NULL, read_proc_host, NULL);
}



int i2c_devices_probe(int i2c_bus, unsigned char *i2c_devices_list)
{
    int rc, i, j = 0;
    //  u8 *i2c_devices;
    u8 tmp[8];

#define I2C_PROBE_INTERVAL_TIME (10)

    for(i = 1; i < (0xff >> 1); i++)
    {
        rc = SOC_I2C_Rec(i2c_bus, (char)i, 0, (char *)tmp, 1);

        if (rc >= 0)
        {
            i2c_devices_list[j] = i;
            j++;
        }
        else
        {
            msleep(I2C_PROBE_INTERVAL_TIME);
        }
    }
    lidbg("\n");
    lidbg("find <%d> i2c_devices:", j);
    for(i = 0; i < j; i++)
    {
        lidbg("0x%x, ", i2c_devices_list[i]);

    }
    lidbg("\n");
    lidbg("\n");
    return j;
}


void get_platform(void)
{
#if 0
    u8 i2c_devices_found[32], i;
    memset(i2c_devices_found, 0, 32);
    i2c_devices_probe(1, i2c_devices_found);
    for(i = 0; i < 32; i++)
    {
        if(i2c_devices_found[i] == 0xe)
        {
            platform_id = PLATFORM_SKU7;
        }
        else
        {
            platform_id = PLATFORM_FLY;
        }
    }
#else
    PLATFORM_GET;
#endif

    lidbg("platform_id = %d\n", platform_id);

}

void pwr_key_scan(void)
{
#if 1
    bool val;
    if(suspend_test == 0)
        return;

    val = SOC_IO_Input(PWR_SLEEP_PORT, PWR_SLEEP_INDEX, LIDBG_GPIO_PULLUP);

    if(val == 0)
    {

        while(SOC_IO_Input(PWR_SLEEP_PORT, PWR_SLEEP_INDEX, LIDBG_GPIO_PULLUP) == 0)
        {
            WHILE_ENTER;
            msleep(100);

        }

        lidbg("pwr_key_scan goto sleep !\n");

#ifdef DEBUG_UMOUNT_USB
        SOC_Write_Servicer(UMOUNT_USB);
        msleep(3000);
#endif

        SOC_PWR_ShutDown();

    }
#endif
}



int thread_pwr(void *data)
{
    //msleep(PWR_SLEEP_TEST_TIME_DELAY);
    //LPCControlSupendTestStart();
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            pwr_key_scan();

            if(suspend_test == 1)
                msleep(PWR_SLEEP_PORT_POLLING_TIME);
            else
                msleep(1000);
        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}

struct ad_key_remap
{
    u32 ch;
    u32 ad_value;
    u32 key;
};


static struct ad_key_remap ad_key[] =
{
    //left
    {0, 530 , KEY_RESERVED/*KEY_MEDIA*/},
    {2, 1548, KEY_NEXT},
    {2, 2040, KEY_PREVIOUS},
    {2, 2517, KEY_HOME},


    //right
    {1, 1034, KEY_VOLUMEUP},
    {1, 1530, KEY_VOLUMEDOWN},
    {4, 1108, KEY_MENU},
    {4, 1372, KEY_BACK},

    //left button
    //{2, 3055, KEY_POWER},
    //right button
    // {1, 3055, KEY_MUTE},

};
#define AD_OFFSET  (100)
#define AD_VAL_MAX  (3300)

int find_ad_key(u32 ch)
{
    int val = 0;
    int i;

    SOC_ADC_Get(ch, &val);
    if(val > AD_VAL_MAX)
        return 0xff;

    for(i = 0; i < SIZE_OF_ARRAY(ad_key); i++)
    {
        if(ch == ad_key[i].ch)
            if((val > ad_key[i].ad_value - AD_OFFSET) && (val < ad_key[i].ad_value + AD_OFFSET))
            {
                //SOC_Key_Report(ad_key[i].key, KEY_PRESSED_RELEASED);
                lidbg("find_ad_key:ch%d=%d,key_id=%d,sendkey=%d\n", ch, val, i, ad_key[i].key);
                return i;
            }
    }

    return 0xff;

}

void key_scan(void)
{
#if 0
    {
        u32 i, val;
#define ADC_MAX_CH (8)
        for(i = 0; i < ADC_MAX_CH; i++)
        {
            SOC_ADC_Get(i, &val);
            lidbg("SOC_ADC_Get:ch%d=%d\n", i, val);
        }
        lidbg("\n");

    }
#else
    static int old_key = 0xff;
    int key = 0xff;
    //key = find_ad_key(0);
    //if(key != 0xff) goto find_key;
    key = find_ad_key(4);
    if(key != 0xff) goto find_key;
    key = find_ad_key(2);
    if(key != 0xff) goto find_key;

find_key:

    if((old_key != 0xff) && (key == 0xff))
    {
        SOC_Key_Report(ad_key[old_key].key, KEY_PRESSED_RELEASED);

        if(old_key == 0)
        {
            static bool usb_id_host = 1;
            if(usb_id_host)
            {
                USB_HUB_DISABLE;
                USB_ID_HIGH_DEV;
                usb_id_host = 0;
                lidbg("usb_id change to dev\n");
            }
            else
            {
                USB_HUB_ENABLE;
                USB_ID_LOW_HOST;
                usb_id_host = 1;
                lidbg("usb_id change to host\n");

            }
        }

#if 0

        if(old_key == 0)
        {
            SOC_Log_Dump(LOG_DMESG);

        }
        else if(old_key == 1)
        {
            SOC_Log_Dump(LOG_LOGCAT);

        }
        else if(old_key == 2)
        {
            SOC_Log_Dump(LOG_ALL);

        }
        else if(old_key == 4)
        {
            static bool pwr_en = 1;
            if(pwr_en)
            {
                lidbg("LPCControlPWRDisenable\n");
                LPCControlPWRDisenable();
                pwr_en = 0;

            }
            else
            {
                lidbg("LPCControlPWREnable\n");
                LPCControlPWREnable();
                pwr_en = 1;
            }

        }

#endif

    }

    old_key = key;


#endif

}

#define TEMP_LOG_PATH LIDBG_LOG_DIR"log_ct.txt"

void log_temp(void)
{
    static int old_temp, cur_temp;
    int tmp;
    g_var.temp = cur_temp = soc_temp_get();
    tmp = cur_temp - old_temp;
    if((temp_log_freq != 0) && (ABS(tmp) >= temp_log_freq))
    {
        lidbg_fs_log(TEMP_LOG_PATH, "%d\n", cur_temp);
        old_temp = cur_temp;
    }
}

#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#else
static int thread_thermal(void *data)
{
    int cur_temp;
    DUMP_FUN;
    while(!kthread_should_stop())
    {
        msleep(1000);

        log_temp();
        cur_temp = soc_temp_get();
        // lidbg("MSM_THERM: %d *C\n",cur_temp);
        if( (cur_temp > fan_onoff_temp) && hal_fan_on && (suspend_flag == 0))//on
        {
            if(!flag_fan_run_statu)
            {
                flag_fan_run_statu = true;
                AIRFAN_BACK_ON;
                lidbg_fs_log(TEMP_LOG_PATH, "AIR_ON:%d\n", cur_temp);
            }
        }
        else //off
        {
            if(flag_fan_run_statu)
            {
                flag_fan_run_statu = false;
                AIRFAN_BACK_OFF;
                lidbg_fs_log(TEMP_LOG_PATH, "AIR_OFF:%d\n", cur_temp);
            }
        }
    }
    return 0;
}
#endif

void led_on(void)
{

#if 1
    static int led_status = 0;


    if(suspend_flag == 1)
    {
        LED_ON;
        if(i2c_c_ctrl == 1)
            TELL_LPC_PWR_ON;
        return;
    }
    if(led_status == 0)
    {
        LED_OFF;
        led_status = 1;
    }
    else
    {
        LED_ON;
        led_status = 0;
    }
    TELL_LPC_PWR_ON;


#endif
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
    log_temp();
#endif


}


int thread_dev_init(void *data)
{

    fly_devices_init();
    return 0;
}


int thread_led(void *data)
{

    //linux驱动开发之内核线程
    //http://hi.baidu.com/zzcqh/blog/item/36b2b2eabebcf9ded539c9ec.html
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1) //条件为真
        {
            if(led_en)
                led_on();
            if(!g_var.is_fly)
                msleep(500);
            else
                msleep(2000);

        }
        else //条件为假
        {
            //让出CPU运行其他线程，并在指定的时间内重新被调度
            schedule_timeout(HZ);
        }
    }
    return 0;
}


int thread_key(void *data)
{

    //linux驱动开发之内核线程
    //http://hi.baidu.com/zzcqh/blog/item/36b2b2eabebcf9ded539c9ec.html
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1) //条件为真
        {
            key_scan();
            msleep(AD_KEY_READ_POLLING_TIME);
            // msleep(10);
        }
        else //条件为假
        {
            //让出CPU运行其他线程，并在指定的时间内重新被调度
            schedule_timeout(HZ);
        }
    }
    return 0;
}




struct platform_device soc_devices =
{
    .name			= "soc_devices",
    .id 			= 0,
};

int lcd_reset_en = 0;

void lcd_reset(char *key, char *value )
{
    SOC_LCD_Reset();
}

static int soc_dev_probe(struct platform_device *pdev)
{

    DUMP_FUN;

    init_waitqueue_head(&read_wait);
    fs_register_filename_list(TEMP_LOG_PATH, true);

    fs_regist_state("cpu_temp", &(g_var.temp));
    FS_REGISTER_INT(lcd_reset_en, "lcd_reset", 0, lcd_reset);
    create_new_proc_entry_usb_dev();
    create_new_proc_entry_usb_host();

    PWR_EN_ON;
    TELL_LPC_PWR_ON;

    get_platform();

    if(platform_id ==  PLATFORM_FLY)
    {
        devices_resource.led_gpio = GPIO_LED_FLY;

    }
    else if(platform_id ==  PLATFORM_SKU7)
    {
        devices_resource.led_gpio = GPIO_LED_SKU7;

    }

    FS_REGISTER_INT(i2c_ctrl, "i2c_ctrl", 0, NULL);
    FS_REGISTER_INT(temp_log_freq, "temp_log_freq", 5, NULL);
    fs_file_separator(TEMP_LOG_PATH);

    INIT_COMPLETION(usb_resume);
    CREATE_KTHREAD(thread_usb, NULL);


#ifdef DEBUG_LED
    {
        FS_REGISTER_INT(led_en, "led_en", 1, NULL);

        if(led_en)
        {
            CREATE_KTHREAD(thread_led, NULL);
        }
    }
#endif

    if(platform_id ==  PLATFORM_FLY)
    {
        CREATE_KTHREAD(thread_dev_init, NULL);


        if(!g_var.is_fly)
        {
            if(1) 	//LPCBackLightOn
            {
                u8 buff[] = {0x00, 0x94, 0x01, 0x99};
                SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));
            }

            {
                u8 buff[] = {0x00, 0x05, 0x01};//LPCControlPWREnable
                lidbg("LPCControlPWREnable\n");
                SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));
            }

#ifdef DEBUG_AD_KEY
            CREATE_KTHREAD(thread_key, NULL);

#endif

#ifdef DEBUG_POWER_KEY
            CREATE_KTHREAD(thread_pwr, NULL);
#endif

        }

    }


#ifdef CONFIG_HAS_EARLYSUSPEND
    {
        early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;//EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
        early_suspend.suspend = devices_early_suspend;
        early_suspend.resume = devices_late_resume;
        register_early_suspend(&early_suspend);

    }
#endif
    //fake suspend
    //SOC_Fake_Register_Early_Suspend(&early_suspend);
    te_regist_password("001210", cb_password_enable_usb);
    te_regist_password("001211", cb_password_disable_usb);
    te_regist_password("111200", cb_password_add_udisk);
    te_regist_password("112200", cb_password_remove_udisk);

    register_lidbg_notifier(&lidbg_notifier);

#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#else
    Thermal_task =  kthread_run(thread_thermal, NULL, "flythermalthread");
    FS_REGISTER_INT(fan_onoff_temp, "fan_onoff_temp", 65, NULL);
#endif

    lidbg_new_cdev(&dev_fops, "flydev");
    init_completion(&udisk_event_wait);
    usb_register_notify(&usb_nb_event);
    CREATE_KTHREAD(thread_udisk_uevent, NULL);
    return 0;

}
static int soc_dev_remove(struct platform_device *pdev)
{
    lidbg("soc_dev_remove\n");
#if 0
    if(led_task)
    {
        kthread_stop(led_task);
        led_task = NULL;
    }
#ifdef DEBUG_AD_KEY
    if(key_task)
    {
        kthread_stop(key_task);
        key_task = NULL;
    }
#endif
#ifdef DEBUG_POWER_KEY
    if(pwr_task)
    {
        kthread_stop(pwr_task);
        pwr_task = NULL;
    }
#endif
#endif
    return 0;

}

#ifdef CONFIG_PM


#ifdef CONFIG_HAS_EARLYSUSPEND
static void devices_early_suspend(struct early_suspend *handler)
{

    DUMP_FUN_ENTER;
    if(platform_id ==  PLATFORM_FLY)
    {
        if(!g_var.is_fly)  LCD_OFF;

        LED_ON;
        TELL_LPC_PWR_ON;
        i2c_c_ctrl = 1;
        suspend_flag = 1;
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#else
        flag_fan_run_statu = false;
        AIRFAN_BACK_OFF;
        if(g_var.fake_suspend == 0)
        {
            lidbg("disable usb after kill process\n");
            USB_WORK_DISENABLE;
        }
#endif

    }
    DUMP_FUN_LEAVE;
}

static void devices_late_resume(struct early_suspend *handler)
{

    DUMP_FUN_ENTER;

    suspend_flag = 0;
    if(platform_id ==  PLATFORM_FLY)
    {
        i2c_c_ctrl = 0;

        //SOC_IO_Config(MCU_IIC_REQ_I, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA);
        //TELL_LPC_PWR_ON;

        if(!g_var.is_fly)
        {
            BL_SET(BL_MAX / 2);
            LCD_ON;

            if(1) 	//LPCBackLightOn
            {
                u8 buff[] = {0x00, 0x94, 0x01, 0x99};
                SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));
            }

            if(0)
            {
                u8 buff[] = {0x00, 0x05, 0x01};//LPCControlPWREnable
                lidbg("LPCControlPWREnable\n");
                SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));
            }

            CREATE_KTHREAD(thread_resume, NULL);

        }

        //lidbg_fs_log(TEMP_LOG_PATH,"*\n");
    }

    DUMP_FUN_LEAVE;
}
#endif


static int  soc_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    lidbg("soc_dev_suspend\n");

    if(platform_id ==  PLATFORM_FLY)
    {
        lidbg("turn lcd off!\n");
        if(!g_var.is_fly)
        {
#ifdef DEBUG_BUTTON
            SOC_IO_ISR_Disable(BUTTON_LEFT_1);
            SOC_IO_ISR_Disable(BUTTON_LEFT_2);
            SOC_IO_ISR_Disable(BUTTON_RIGHT_1);
            SOC_IO_ISR_Disable(BUTTON_RIGHT_2);
#endif
        }
        i2c_c_ctrl = 0;

        PWR_EN_OFF;
        LED_ON;
        i2c_c_ctrl = 0;
        if(i2c_ctrl)
            TELL_LPC_PWR_OFF;

    }

    return 0;

}


static int thread_resume(void *data)
{
    DUMP_FUN_ENTER;

    msleep(3000 + 4000);

    if(SOC_PWR_GetStatus() == PM_STATUS_LATE_RESUME_OK)
    {
        USB_WORK_ENABLE;
    }
    DUMP_FUN_LEAVE;
    return 0;

}

static void soc_dev_suspend_prepare(void)
{

    DUMP_FUN;

    if(!g_var.is_fly)
    {
        //LPCBackLightOff
        u8 buff[] = {0x00, 0x94, 0x00, 0x98};
        SOC_LPC_Send(buff, SIZE_OF_ARRAY(buff));
    }


#ifdef DEBUG_UMOUNT_USB
    SOC_Write_Servicer(UMOUNT_USB);
    if(!g_var.is_fly)    msleep(1000);
#endif

    //disable usb first
    USB_WORK_DISENABLE;

    if(!g_var.is_fly)    msleep(3000);


}



static int soc_dev_resume(struct platform_device *pdev)
{
    lidbg("soc_dev_resume\n");

    if(platform_id ==  PLATFORM_FLY)
    {

        //SOC_IO_Config(MCU_IIC_REQ_I, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA);
        //TELL_LPC_PWR_ON;
        //PWR_EN_ON;

        //USB_SWITCH_DISCONNECT;
        if(!g_var.is_fly)
        {
#ifdef DEBUG_BUTTON
            SOC_IO_ISR_Enable(BUTTON_LEFT_1);
            SOC_IO_ISR_Enable(BUTTON_LEFT_2);
            SOC_IO_ISR_Enable(BUTTON_RIGHT_1);
            SOC_IO_ISR_Enable(BUTTON_RIGHT_2);
#endif
        }

        //lidbg("turn lcd on!\n");
        //LCD_ON;

        //reconfig led
        SOC_IO_Config(devices_resource.led_gpio, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA);

    }

    return 0;

}
#endif

static struct platform_driver soc_devices_driver =
{
    .probe = soc_dev_probe,
    .remove = soc_dev_remove,
    .suspend =  soc_dev_suspend,
    .resume =  soc_dev_resume,
    .driver = {
        .name = "soc_devices",
        .owner = THIS_MODULE,
    },
};


struct work_struct work_left_button1;
static void work_left_button1_fn(struct work_struct *work)
{
    SOC_Key_Report(KEY_HOME, KEY_PRESSED_RELEASED);
}

struct work_struct work_right_button1;
static void work_right_button1_fn(struct work_struct *work)
{
    SOC_Key_Report(KEY_MENU, KEY_PRESSED_RELEASED);

}


irqreturn_t irq_left_button1(int irq, void *dev_id)
{
#if 0
    static int bl = 0;
    bl = bl + 10;
    SOC_BL_Set(bl & BL_MAX);
    lidbg("irq_left_button1: %d, bl=%d\n", irq, bl & BL_MAX);
#else

    lidbg("irq_left_button1: %d\n", irq);
    schedule_work(&work_left_button1);

#endif
    return IRQ_HANDLED;

}


irqreturn_t irq_left_button2(int irq, void *dev_id)
{

    lidbg("irq_left_button2: %d\n", irq);

    return IRQ_HANDLED;

}


irqreturn_t irq_right_button1(int irq, void *dev_id)
{
    lidbg("irq_right_button1: %d\n", irq);
    //led_on();
    schedule_work(&work_right_button1);
    return IRQ_HANDLED;

}

irqreturn_t irq_right_button2(int irq, void *dev_id)
{

    lidbg("irq_right_button2: %d\n", irq);
    return IRQ_HANDLED;

}

void fly_devices_init(void)
{
    lidbg("fly_devices_init\n");

    if(platform_id ==  PLATFORM_FLY)
    {


        if(1 == recovery_mode)
        {
            DVD_RESET_HIGH;
            TELL_LPC_PWR_ON;
            PWR_EN_ON;
            LCD_ON;
            lidbg("turn lcd on!\n");
            BL_SET(BL_MAX / 2);
        }

#ifdef DEBUG_BUTTON
        if(!g_var.is_fly)
        {
            INIT_WORK(&work_left_button1, work_left_button1_fn);
            INIT_WORK(&work_right_button1, work_right_button1_fn);

            SOC_IO_Input(BUTTON_LEFT_1, BUTTON_LEFT_1, GPIO_CFG_PULL_UP);
            SOC_IO_Input(BUTTON_LEFT_2, BUTTON_LEFT_2, GPIO_CFG_PULL_UP);
            SOC_IO_Input(BUTTON_RIGHT_1, BUTTON_RIGHT_1, GPIO_CFG_PULL_UP);
            SOC_IO_Input(BUTTON_RIGHT_2, BUTTON_RIGHT_2, GPIO_CFG_PULL_UP);

            SOC_IO_ISR_Add(BUTTON_LEFT_1, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, irq_left_button1, NULL);
            SOC_IO_ISR_Add(BUTTON_LEFT_2, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, irq_left_button2, NULL);
            SOC_IO_ISR_Add(BUTTON_RIGHT_1, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, irq_right_button1, NULL);
            SOC_IO_ISR_Add(BUTTON_RIGHT_2, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT, irq_right_button2, NULL);
        }
#endif

        unmute_ns();

        if(!fs_is_file_exist(USB_1_1))
            CREATE_KTHREAD(thread_usb_delay_enable, NULL);
        else
        {
            if(1 == recovery_mode)
                USB_WORK_ENABLE;
            else
                CREATE_KTHREAD(thread_usb_11, NULL);
        }
        /*if( fs_is_file_exist(USB_1_1))
        {
        	lidbg("Translate to usb1.1 modes\n");
        	USB_ID_HIGH_DEV;
        	ssleep(30);
        }
        USB_WORK_ENABLE;*/
    }
}


static void set_func_tbl(void)
{
    //lpc
    ((struct lidbg_hal *)plidbg_dev)->soc_func_tbl.pfnSOC_Dev_Suspend_Prepare = soc_dev_suspend_prepare;
    if(!g_var.is_fly)
        ((struct lidbg_hal *)plidbg_dev)->soc_func_tbl.pfnHal_Acc_Callback = NULL;
}



int dev_open(struct inode *inode, struct file *filp)
{
    return 0;

}
static void parse_cmd(char *pt)
{
    int argc = 0;
    int i = 0;
    u8 lpc_buf[3];

    lidbg("%s\n", pt);
    char *argv[32] = {NULL};
    argc = lidbg_token_string(pt, " ", argv);
	
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#else
    if (!strcmp(pt, "fan_on"))
    {
        hal_fan_on = true;
    }
    else if (!strcmp(pt, "fan_off"))
    {
        hal_fan_on = false;
    }

    else if(!strcmp(pt, "screen_on"))
    {
        lidbg("******into screen_on********\n");
        // LCD_RESET;
        if(SOC_Hal_Acc_Callback)
        {
            lidbg("hal callback 1\n");
            SOC_Hal_Acc_Callback(1);
        }
    }
    else if(!strcmp(pt, "screen_off"))
    {
        lidbg("******into screen_off********\n");
        if(SOC_Hal_Acc_Callback)
        {
            lidbg("hal callback 0\n");
            SOC_Hal_Acc_Callback(0);
        }
    }
    else if(!strcmp(pt, "suspend_on"))
    {
        lidbg("******into suspend_on********\n");
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_SUSPEND_UNPREPARE));
        if(SOC_Hal_Acc_Callback)
        {
            lidbg("hal callback 2\n");
            SOC_Hal_Acc_Callback(2);
        }
    }
    else if(!strcmp(pt, "suspend_off"))
    {
        lidbg("******into suspend_off********\n");
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_SUSPEND_PREPARE));
        if(SOC_Hal_Acc_Callback)
        {
            lidbg("hal callback 3\n");
            SOC_Hal_Acc_Callback(3);
        }
    }

    else if(!strcmp(pt, "usb_reset"))
    {
        lidbg("usb_reset\n");
        USB_WORK_DISENABLE;
        msleep(200);
        USB_WORK_ENABLE;
    }
	else if (!strcmp(argv[0], "lpc"))
    {
		lidbg("%s:[%s] [%s] [%s]\n", argv[0], argv[1],argv[2],argv[3]);
		lpc_buf[0] = simple_strtoul(argv[1], 0, 0);
		lpc_buf[1] = simple_strtoul(argv[2], 0, 0);
		lpc_buf[2] = simple_strtoul(argv[3], 0, 0);
		lidbg("%s:[%d] [%d] [%d]\n", argv[0], lpc_buf[0],lpc_buf[1],lpc_buf[2]);
		SOC_LPC_Send(lpc_buf, SIZE_OF_ARRAY(lpc_buf));
    }


#endif

}
static int lidbg_event(struct notifier_block *this,
                       unsigned long event, void *ptr)
{
    DUMP_FUN;

    switch (event)
    {

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_ACC_OFF):
        if(!g_var.is_fly)LCD_OFF;
        //mute_s();
        break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_SUSPEND_PREPARE):
        //USB_WORK_DISENABLE;
        mute_s();
        break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_POWER_OFF):
        mute_s();
        break;

    case NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_ACC_ON):
        if(!g_var.is_fly)
        {
            LCD_RESET;
            LCD_ON;
        }
        unmute_ns();
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_ACC_EVENT, NOTIFIER_MINOR_SUSPEND_UNPREPARE):
        complete(&usb_resume);
        break;
    case NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK):
        SOC_BL_Set(BL_MIN);
        msleep(100);
        SOC_BL_Set(BL_MAX);
        break;
    default:
        break;
    }

    return NOTIFY_DONE;
}


static ssize_t dev_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    char *p = NULL;
    int len = size;
#if 1
    char tmp[32];
    char *mem = tmp;
    bool is_alloc = 0;
    if(size >= 32)
    {
        mem = (char *)kmalloc(size + 1, GFP_KERNEL); //size+1 for '\0'
        if (mem == NULL)
        {
            lidbg("dev_write kmalloc err\n");
            return 0;
        }
        is_alloc = 1;
    }
#else
    char tmp[size + 1];//C99 variable length array
    char *mem = tmp;
    bool is_alloc = 0;
#endif

    memset(mem, '\0', size + 1);

    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    if((p = memchr(mem, '\n', size)))
    {
        len = p - mem;
        *p = '\0';
    }
    else
        mem[len] =  '\0';


    parse_cmd(mem);

    if(is_alloc)
        kfree(mem);

    return size;//warn:don't forget it;
}

int dev_close(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  dev_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    lidbg("dev_read.%d,%d,%d\n", audio_data_for_hal[0], audio_data_for_hal[1], size);
    if(audio_data_for_hal[0] != 0x01)
        return -1;
    if(copy_to_user(buffer, audio_data_for_hal, sizeof(audio_data_for_hal)))
    {
        return -1;
    }
    else
    {
        audio_data_for_hal[0] = 0x00;
        return size;
    }
}

static unsigned int dev_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    interruptible_sleep_on(&read_wait);
    mask |= POLLIN | POLLRDNORM;
    return mask;
}
static struct file_operations dev_fops =
{
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .poll = dev_poll,
    .release = dev_close,
};


int dev_init(void)
{
    DUMP_BUILD_TIME;

#ifdef BOARD_V1
    lidbg("FLY_V1 version\n");
#endif

#ifdef BOARD_V2
    lidbg("FLY_V2 version\n");
#endif

#ifdef BOARD_V3
    lidbg("FLY_V3 version\n");
#endif

#ifdef BOARD_V4
    lidbg("FLY_V4 version\n");
#endif

    LIDBG_GET;
    set_func_tbl();

    if(!g_var.is_fly)
        lidbg("debug version\n");
    else
        lidbg("release version\n");


#if 0
    PWR_EN_ON;

    get_platform();

    if(platform_id ==  PLATFORM_FLY)
    {
        devices_resource.led_gpio = GPIO_LED_FLY;

    }
    else if(platform_id ==  PLATFORM_SKU7)
    {
        devices_resource.led_gpio = GPIO_LED_SKU7;

    }
#endif

    platform_device_register(&soc_devices);
    platform_driver_register(&soc_devices_driver);

    return 0;
}

void dev_exit(void)
{
    lidbg("dev_exit\n");

}

void lidbg_device_main(int argc, char **argv)
{

    lidbg("lidbg_device_main\n");

    if(argc < 1)
    {
        lidbg("Usage:\n");
        lidbg("bl value\n");
        return;

    }

    if(!strcmp(argv[0], "bl"))
    {
        u32 bl;
        bl = simple_strtoul(argv[1], 0, 0);
        SOC_BL_Set(bl);
    }

    if(!strcmp(argv[0], "ad"))
    {
        u32 ch, val;
        ch = simple_strtoul(argv[1], 0, 0);
        SOC_ADC_Get(ch, &val);
        lidbg("ch%d = %x\n", ch, val);
    }
}


EXPORT_SYMBOL(lidbg_device_main);


MODULE_AUTHOR("lsw, <sw.lee.g2@gmail.com>");
MODULE_DESCRIPTION("Devices Driver");
MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);

