#include "lidbg.h"
LIDBG_DEFINE;

#define BUTTON_LED_NODE "/sys/class/leds/button-backlight/brightness"
#define GPIO_WP (35)
#define GPIO_APP_STATUS (36)
static atomic_t is_in_sleep = ATOMIC_INIT(-1);
static int list_count ;
static bool is_pm_toast_dbg_en = false;
static DECLARE_COMPLETION(sleep_observer_wait);

typedef enum
{
    PM_ACTION_PRINT,
    PM_ACTION_PRINT_FORCE_UNLOCK,
    PM_ACTION_PRINT_KILL_HASLOCK_APK,
    PM_ACTION_PRINT_KILL_ALL_APK,
    PM_ACTION_SAVE_LOCK_MSG,
    PM_ACTION_NULL
} ws_list_action_types;

void PM_action_entry(char *info, ws_list_action_types type);

bool set_factory_reset_mode(bool on)
{
    PM_WARN("%d\n", on);
    lidbg_uevent_shell("am broadcast -a android.intent.action.MASTER_CLEAR &");
    return true;
}

void observer_start(void)
{
    atomic_set(&is_in_sleep, 1);
    complete(&sleep_observer_wait);
}
void observer_stop(void)
{
    atomic_set(&is_in_sleep, 0);
}
void observer_prepare(void)
{
    char cmd[128] = {0};
    lidbg_chmod("/sys/module/msm_show_resume_irq/parameters/debug_mask");
    ssleep(1);
    fs_file_write("/sys/module/msm_show_resume_irq/parameters/debug_mask", "1");
    ssleep(1);
    lidbg_chmod("/dev/lidbg*");
    sprintf(cmd, "cat /proc/interrupts > %sinterrupts.txt &", LIDBG_LOG_DIR);
    lidbg_uevent_shell(cmd);
    fs_register_filename_list(LIDBG_LOG_DIR"interrupts.txt", true);
}
static int thread_observer(void *data)
{
    int have_triggerd_sleep_S = 0;
    char when[64] = {0};
    ssleep(10);
    observer_prepare();

    while(!kthread_should_stop())
    {
        have_triggerd_sleep_S = 0;
        if( !wait_for_completion_interruptible(&sleep_observer_wait))
        {
            ssleep(2);
            PM_action_entry("start:", PM_ACTION_PRINT);
            while((atomic_read(&is_in_sleep) == 1))
            {
                ssleep(1);
                have_triggerd_sleep_S++;
                switch (have_triggerd_sleep_S)
                {
                case 5:
                    //PM_action_entry("start5:", PM_ACTION_PRINT);
                    break;

                default:
                    if(have_triggerd_sleep_S > 5 && !(have_triggerd_sleep_S % 5))
                    {
                        sprintf(when, "start%d:", have_triggerd_sleep_S);
                        PM_action_entry(when,  PM_ACTION_PRINT);
                    }
                    break;
                }

            }
            PM_action_entry("stop:", PM_ACTION_PRINT);
        }
    }
    return 1;
}

static LIST_HEAD(pm_list_3);
void pm_list_kill_each(void)
{
    struct string_dev *pos;
    char *p = NULL;
    list_for_each_entry(pos, &pm_list_3, tmp_list)
    {
        if(pos->yourkey && (p = strchr(pos->yourkey, ':')))
        {
            ++p;
            lidbg_force_stop_apk(p);
            msleep(30);
        }
        p = NULL;
    }
}
static int thread_am_kill_apk(void *data)
{
    pm_list_kill_each();
    return 1;
}
static int thread_led_monitor(void *data)
{
    ssleep(8);
    while(!kthread_should_stop())
    {
        fs_file_write(BUTTON_LED_NODE, "0");
        ssleep(1);
        fs_file_write(BUTTON_LED_NODE, "255");
        ssleep(1);
    }
    PM_WARN("stop\n");
    return 1;
}
static int ws_func_print(char *info, struct wakeup_source *ws)
{
    spin_lock_irq(&ws->lock);
    if (ws->active)
    {
        PM_WARN("[%s]:%d:%s,ps.%lld,ac.%lu,rc%lu,wc.%lu\n", info, list_count, ws->name, ktime_to_ms(ws->prevent_sleep_time), ws->active_count, ws->relax_count, ws->wakeup_count);
        list_count++;
    }
    spin_unlock_irq(&ws->lock);
    return 1;
}
static int ws_func_print_unlock(char *info, struct wakeup_source *ws)
{
    if ( ws->active)
    {
        PM_WARN("[%s]:%d:%s,ps.%lld,ac.%lu,rc%lu,wc.%lu\n", info, list_count, ws->name, ktime_to_ms(ws->prevent_sleep_time), ws->active_count, ws->relax_count, ws->wakeup_count);
        __pm_relax(ws);
        list_count++;
    }
    return 1;
}
static int ws_func_save_log_msg(char *info, struct wakeup_source *ws)
{
    if (ws->active)
    {
        rcu_read_unlock();
        fs_string2file(1, LIDBG_OSD_DIR"can_t_sleep.txt", "[K]%d:%s\n", list_count, ws->name);
        list_count++;
        rcu_read_lock();
    }
    return 1;
}

static int (*wl_list_action_func[]) (char *info, struct wakeup_source *ws) =
{
    [PM_ACTION_PRINT] = ws_func_print,
    [PM_ACTION_PRINT_FORCE_UNLOCK] = ws_func_print_unlock,
    [PM_ACTION_SAVE_LOCK_MSG] = ws_func_save_log_msg,
};
void deal_kernel_wakelock(char *info, ws_list_action_types type)
{
    struct wakeup_source *ws;
    list_count = 0;
    if(g_var.ws_lh == NULL || type >= PM_ACTION_NULL)
    {
        PM_ERR("g_var.ws_lh==NULL||type.%d\n", type);
        return ;
    }
    rcu_read_lock();
    list_for_each_entry_rcu(ws, g_var.ws_lh, entry)
    {
        wl_list_action_func[type] (info, ws);
    }
    rcu_read_unlock();
}
void deal_userspace_wakelock(int is_should_kill_apk)
{
    lidbg_show_wakelock(is_should_kill_apk);
}
void PM_action_entry(char *info, ws_list_action_types type)
{
    PM_WARN("%s,%d\n", info, type);

    switch (type)
    {
    case PM_ACTION_PRINT:
        deal_userspace_wakelock(0);
        deal_kernel_wakelock(info, PM_ACTION_PRINT);
        break;
    case PM_ACTION_PRINT_FORCE_UNLOCK:
        deal_userspace_wakelock(1);
        deal_kernel_wakelock(info, PM_ACTION_PRINT_FORCE_UNLOCK);
        break;
    case PM_ACTION_PRINT_KILL_HASLOCK_APK:
        deal_userspace_wakelock(1);
        deal_kernel_wakelock(info, PM_ACTION_PRINT);
        break;
    case PM_ACTION_PRINT_KILL_ALL_APK:
        deal_userspace_wakelock(1);
        CREATE_KTHREAD(thread_am_kill_apk, NULL);
        deal_kernel_wakelock(info, PM_ACTION_PRINT);
        break;
    case PM_ACTION_SAVE_LOCK_MSG:
        fs_clear_file(LIDBG_OSD_DIR"can_t_sleep.txt");
        deal_userspace_wakelock(2);
        deal_kernel_wakelock(info, PM_ACTION_SAVE_LOCK_MSG);
        break;
    default:
        break;
    }

}

void lidbg_pm_step_call(fly_pm_stat_step step, void *data)
{
    switch (step)
    {
    case PM_AUTOSLEEP_STORE1:
    {
        char *buff = data;
        if(!strcmp(buff, "mem"))
            observer_start();
        else if(!strcmp(buff, "off"))
            observer_stop();
        PM_WARN("PM_AUTOSLEEP_STORE1:[%s,%d]\n", buff, atomic_read(&is_in_sleep));
    }
    break;
    case PM_AUTOSLEEP_SET_STATE2:
        break;
    case PM_QUEUE_UP_SUSPEND_WORK3:
        break;
    case PM_TRY_TO_SUSPEND4:
        // PM_action_entry("start1",PM_ACTION_PRINT);
        break;
    case PM_TRY_TO_SUSPEND4P1:
    case PM_TRY_TO_SUSPEND4P2:
    case PM_TRY_TO_SUSPEND4P3:
    case PM_TRY_TO_SUSPEND4P4:
    case PM_TRY_TO_SUSPEND4P5:
        //PM_action_entry("stop1",PM_ACTION_PRINT);
        break;
    case PM_SUSPEND5:
        break;
    case PM_ENTER_STATE6:
        break;
    case PM_ENTER_STATE6P1:
        break;
    case PM_ENTER_STATE6P2:
        break;
    case PM_SUSPEND_DEVICES_AND_ENTER7:
        //suspend_ops->enter.in
        break;
    case PM_SUSPEND_ENTER8:
        fs_file_write(BUTTON_LED_NODE, "0");
        break;
    case PM_SUSPEMD_OPS_ENTER9:
        break;
    case PM_SUSPEMD_OPS_ENTER9P1:
        //suspend_ops->enter.out
        fs_file_write(BUTTON_LED_NODE, "255");
        break;
    case PM_NULL:
        PM_action_entry("test", PM_ACTION_PRINT_FORCE_UNLOCK);
        break;
    default:
        break;
    }
}

int linux_to_lidbg_receiver(linux_to_lidbg_transfer_t _enum, void *data)
{
    switch (_enum)
    {
    case LTL_TRANSFER_RTC:
        break;
    case LTL_TRANSFER_NULL:
        break;
    default:
        break;
    }
    return 1;
}


static DECLARE_COMPLETION(thread_kernel_msg_completion);
static int thread_kernel_msg_completion_func(void *data)
{
    while(1)
    {
        wait_for_completion(&thread_kernel_msg_completion);
        lidbg_uevent_shell("am broadcast -a com.flyaudio.ap.broadcast --es powerkey longpress &");
        observer_start();
    }
    return 1;
}
int pm_open (struct inode *inode, struct file *filp)
{
    return 0;
}
ssize_t pm_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char *cmd[8] = {NULL};
    int cmd_num  = 0;
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        PM_ERR("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    PM_WARN("-----FLYSTEP------------------[%s]---\n", cmd_buf);

    cmd_num = lidbg_token_string(cmd_buf, " ", cmd) ;

    //kernel logic
    if(!strcmp(cmd[0], "kernel"))
    {
        if(!strcmp(cmd[1], "short_press"))
            SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
        else  if(!strcmp(cmd[1], "long_press_c"))
            complete(&thread_kernel_msg_completion);
        else  if(!strcmp(cmd[1], "long_press"))
        {
            SOC_Key_Report(KEY_POWER, KEY_PRESSED);
            msleep(530);
            SOC_Key_Report(KEY_POWER, KEY_RELEASED);
        }
    }

    //flyaudio logic
    if(!strcmp(cmd[0], "flyaudio"))
    {
        if(is_pm_toast_dbg_en)
            lidbg_toast_show(cmd[1], -1);
        if(!strcmp(cmd[1], "android_up"))
            SOC_IO_Output(0, GPIO_APP_STATUS, 0);
        else  if(!strcmp(cmd[1], "android_down"))
            SOC_IO_Output(0, GPIO_APP_STATUS, 1);//complete(&thread_kernel_msg_completion);
        else if(!strcmp(cmd[1], "devices_down"))
            ;
    }

    //pm debug
    if(!strcmp(cmd[0], "ws"))
    {
        if(!strcmp(cmd[1], "chmod"))
            lidbg_chmod(cmd[2]);
        else if(!strcmp(cmd[1], "rm"))
            lidbg_rm(cmd[2]);
        else  if(!strcmp(cmd[1], "list"))
        {
            int  ws_action_type = simple_strtoul(cmd[2], 0, 0);
            PM_action_entry("app", (ws_list_action_types)ws_action_type);
        }
        else  if(!strcmp(cmd[1], "pm"))
        {
            int pm_observer_mode = simple_strtoul(cmd[2], 0, 0);
            if(pm_observer_mode)
                observer_start();
            else
                observer_stop();
        }
        else  if(!strcmp(cmd[1], "fac"))
        {
            set_factory_reset_mode(true);
        }
        else  if(!strcmp(cmd[1], "reb"))
        {
            msleep(100);
            lidbg_reboot();
        }
        else  if(!strcmp(cmd[1], "apk"))
        {
            lidbg_pm_install_dir(cmd[2]);
        }
        else  if(!strcmp(cmd[1], "kill3"))
        {
            CREATE_KTHREAD(thread_am_kill_apk, NULL);
        }
        else  if(!strcmp(cmd[1], "kill"))
        {
            lidbg_force_stop_apk(cmd[2]);
        }
        else  if(!strcmp(cmd[1], "toast"))
        {
            int  enum_value = simple_strtoul(cmd[3], 0, 0);
            lidbg_toast_show(cmd[2], enum_value);
        }
        else  if(!strcmp(cmd[1], "dbgt"))
        {
            is_pm_toast_dbg_en = !is_pm_toast_dbg_en;
        }

    }
    return size;
}
static  struct file_operations pm_nod_fops =
{
    .owner = THIS_MODULE,
    .write = pm_write,
    .open = pm_open,
};

static int thread_remove_wakeup_page(void *data)
{
    int i = 23;
    SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    msleep(500);
    SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
    msleep(500);
    while(i > 1)
    {
        SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);
        msleep(200);
        i--;
    }
    SOC_IO_Output(0, GPIO_APP_STATUS, 1);
    return 1;
}

#ifdef CONFIG_PM
static int pm_suspend(struct device *dev)
{
    DUMP_FUN;
    return 0;
}
static int pm_resume(struct device *dev)
{
    DUMP_FUN;
    CREATE_KTHREAD(thread_remove_wakeup_page, NULL);
    return 0;
}
static struct dev_pm_ops lidbg_pm_ops =
{
    .suspend	= pm_suspend,
    .resume		= pm_resume,
};
#endif
static int thread_pm_late_probe(void *data)//because of blocking other driver to insmod.
{
    ssleep(10);
    lidbg_uevent_shell("pm list packages -3 > /data/lidbg/pm_3.txt");
    ssleep(5);
    fs_fill_list(LIDBG_LOG_DIR"pm_3.txt", FS_CMD_FILE_LISTMODE, &pm_list_3);
    fs_register_filename_list(LIDBG_LOG_DIR"pm_3.txt", true);

    lidbg_chmod("/dev/lidbg*");
    ssleep(15);
    lidbg_rm(LIDBG_LOG_DIR"lidbg_kmsg.txt");
    return 1;
}

static int  lidbg_pm_probe(struct platform_device *pdev)
{
    DUMP_FUN;
    PM_WARN("<==IN==>\n");

    CREATE_KTHREAD(thread_kernel_msg_completion_func, NULL);

    if(!g_var.is_fly)
    {
        CREATE_KTHREAD(thread_led_monitor, NULL);
        lidbg_chmod("/sys/power/*");
    }

    lidbg_new_cdev(&pm_nod_fops, "lidbg_pm");
    kthread_run(thread_observer, NULL, "ftf_pmtask");
    LIDBG_MODULE_LOG;

    PM_WARN("<==OUT==>\n\n");
    CREATE_KTHREAD(thread_pm_late_probe, NULL);

    return 0;
}
static struct platform_device lidbg_pm =
{
    .name               = "lidbg_pm",
    .id                 = -1,
};

static struct platform_driver lidbg_pm_driver =
{
    .probe		= lidbg_pm_probe,
    .driver         = {
        .name = "lidbg_pm",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &lidbg_pm_ops,
#endif
    },
};

static void set_func_tbl(void)
{
    plidbg_dev->soc_func_tbl.pfnLINUX_TO_LIDBG_TRANSFER = linux_to_lidbg_receiver;
    plidbg_dev->soc_func_tbl.pfnSOC_PM_STEP = lidbg_pm_step_call;
}

static int __init lidbg_pm_init(void)
{
    DUMP_FUN;
    LIDBG_GET;
    set_func_tbl();
    lidbg_uevent_shell("echo 7 7 7 7  > /proc/sys/kernel/printk");
    platform_device_register(&lidbg_pm);
    platform_driver_register(&lidbg_pm_driver);
    PM_WARN("<set GPIO_WP[%d] 1>\n\n", GPIO_WP);
    SOC_IO_Output(0, GPIO_WP, 0);
    return 0;
}

static void __exit lidbg_pm_exit(void)
{
}

module_init(lidbg_pm_init);
module_exit(lidbg_pm_exit);

MODULE_DESCRIPTION("lidbg.powermanager2014.04");
MODULE_LICENSE("GPL");


