

#include "lidbg.h"

#define DEVICE_NAME "suspend_monitor"

#define LED_FLASH_FAST (500)
#define LED_FLASH_SLOW (2000)

char const *const BUTTON_FILE
    = "/sys/class/leds/button-backlight/brightness";
int suspend_times = 0;
int led_ctrl_en = 1;

void led_ctrl(bool on)
{
    int len;
    char buf[4];
    len = sprintf(buf, "%d", on);
    lidbg_readwrite_file(BUTTON_FILE, NULL, buf, len);
}

int sleep_time;
void led_tigger(void)
{
    static int status = 0;
    status = status % 2;
    if(led_ctrl_en)
        led_ctrl(status);
    if(status == 1)
        msleep(sleep_time / 10);
    else
        msleep(sleep_time);

    status++;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void suspend_monitor_early_suspend(struct early_suspend *h)
{
    DUMP_FUN;
    sleep_time = LED_FLASH_FAST;
    led_ctrl_en = 0;
    led_ctrl(1);
}

static void suspend_monitor_late_resume(struct early_suspend *h)
{
    DUMP_FUN;
    sleep_time = LED_FLASH_SLOW;
    led_ctrl(0);
    led_ctrl_en = 1;
}

#endif


int thread_led(void *data)
{
#if 1
    while(1)
        led_tigger();
#else
    while(1)
    {
        if(led_ctrl_en)
            led_ctrl(0);
        msleep(500);
    }
#endif
    return 0;
}


struct early_suspend early_suspend;
static int  suspend_monitor_probe(struct platform_device *pdev)
{
    DUMP_FUN_ENTER;
#ifdef CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend =  suspend_monitor_early_suspend;
    early_suspend.resume =  suspend_monitor_late_resume;
    register_early_suspend(&early_suspend);
#endif
    fs_regist_state("suspend_times", &suspend_times);

    sleep_time = LED_FLASH_SLOW;
    CREATE_KTHREAD(thread_led, NULL);


    return 0;
}


static int  suspend_monitor_remove(struct platform_device *pdev)
{
    return 0;
}


#ifdef CONFIG_PM
static int suspend_monitor_resume(struct device *dev)
{
    DUMP_FUN_ENTER;
    suspend_times ++;
    led_ctrl(1);
    return 0;

}

static int suspend_monitor_suspend(struct device *dev)
{
    DUMP_FUN_ENTER;
    led_ctrl(0);
    return 0;

}

static struct dev_pm_ops suspend_monitor_pm_ops =
{
    .suspend	= suspend_monitor_suspend,
    .resume		= suspend_monitor_resume,
};
#endif

static struct platform_driver suspend_monitor_driver =
{
    .probe		= suspend_monitor_probe,
    .remove     = suspend_monitor_remove,
    .driver         = {
        .name = "lidbg_suspend_monitor",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &suspend_monitor_pm_ops,
#endif
    },
};

static struct platform_device lidbg_suspend_monitor_device =
{
    .name               = "lidbg_suspend_monitor",
    .id                 = -1,
};


static  int suspend_monitor_init(void)
{
    platform_device_register(&lidbg_suspend_monitor_device);
    return platform_driver_register(&suspend_monitor_driver);

}

static void suspend_monitor_exit(void)
{

}

module_init(suspend_monitor_init);
module_exit(suspend_monitor_exit);

MODULE_LICENSE("GPL");


