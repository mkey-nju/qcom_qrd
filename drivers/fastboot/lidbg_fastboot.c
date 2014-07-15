/* Copyright (c) 2012, swlee
 *
 */
#include "lidbg.h"

LIDBG_DEFINE;

#if (defined(BOARD_V1) || defined(BOARD_V2))
#include <proc_comm.h>
#else
#include <mach/proc_comm.h>
#endif
#include <mach/clk.h>
#include <mach/socinfo.h>
#include <clock.h>
#include <clock-pcom.h>

#define FASTBOOT_LOG_PATH LIDBG_LOG_DIR"log_fb.txt"

//#define EXPORT_ACTIVE_WAKE_LOCKS
#define RUN_FASTBOOT


static LIST_HEAD(fastboot_kill_list);
static DECLARE_COMPLETION(suspend_start);
static DECLARE_COMPLETION(early_suspend_start);

static DECLARE_COMPLETION(resume_ok);
static DECLARE_COMPLETION(pwroff_start);
static DECLARE_COMPLETION(late_suspend_start);

void fastboot_set_status(LIDBG_FAST_PWROFF_STATUS status);
void fastboot_pwroff(void);
int lidbg_readwrite_file(const char *filename, char *rbuf, const char *wbuf, size_t length);



struct fastboot_data
{
    int suspend_pending;
    u32 resume_count;
    struct mutex lock;
    int kill_task_en;
    int kill_all_task;
    int haslock_resume_times;
    int max_wait_unlock_time;
    int clk_block_suspend;
    int has_wakelock_can_not_ignore;
    int is_quick_resume;
#if defined(CONFIG_HAS_EARLYSUSPEND)
    struct wake_lock flywakelock;
    struct early_suspend early_suspend;
#endif

};

struct fastboot_data *fb_data;

static spinlock_t kill_lock;
bool is_fake_suspend = 0;

int wakelock_occur_count = 0;
#define MAX_WAIT_UNLOCK_TIME  (5)

#if (defined(BOARD_V1) || defined(BOARD_V2))
#define WAIT_LOCK_RESUME_TIMES  (0)
#else
#define WAIT_LOCK_RESUME_TIMES  (0)
#endif

#define HAS_LOCK_RESUME

static struct task_struct *pwroff_task;
static struct task_struct *suspend_task;
static struct task_struct *resume_task;
static struct task_struct *pwroff_task;
static struct task_struct *late_suspend_task;

bool ignore_wakelock = 0;

#ifdef EXPORT_ACTIVE_WAKE_LOCKS
extern struct list_head active_wake_locks[];
void fastboot_get_wake_locks(struct list_head *p) {}
#else
struct list_head *active_wake_locks = NULL;
void fastboot_get_wake_locks(struct list_head *p)
{
    active_wake_locks = p;
}
#endif

static void log_active_locks(void)
{
    struct wake_lock *lock;
    int type = 0;
    if(active_wake_locks == NULL) return;
    list_for_each_entry(lock, &active_wake_locks[type], link)
    {
        lidbg_fs_log(FASTBOOT_LOG_PATH, "active wake lock %s\n", lock->name);
    }
}


static void list_active_locks(void)
{
#if 1//def EXPORT_ACTIVE_WAKE_LOCKS 
    struct wake_lock *lock;
    unsigned long flags_kill;

    int type = 0;
    if(active_wake_locks == NULL) return;
    spin_lock_irqsave(&kill_lock, flags_kill);
    list_for_each_entry(lock, &active_wake_locks[type], link)
    {
        //lidbg("active wake lock %s\n", lock->name);

        if(!strcmp(lock->name, "adsp"))
        {
            lidbg("wake_lock:adsp\n");
            //lidbg_fs_log(FASTBOOT_LOG_PATH,"wake_lock:adsp\n");
            fb_data->has_wakelock_can_not_ignore = 1;
        }
    }
    spin_unlock_irqrestore(&kill_lock, flags_kill);
#else
    //this can only see user_wakelock
    //cat /proc/wakelocks can see all wakelocks
#define READ_BUF (256)
    static char wakelock[READ_BUF];
    char *p;
    char *token;
    p = wakelock;

    memset(wakelock, '\0' , READ_BUF);
    lidbg_readwrite_file("/sys/power/wake_lock", p, NULL, READ_BUF);
    lidbg("%s\n", p);

    while((token = strsep(&p, " ")) != NULL )
    {
        printk("[%s]\n", token);

        if(!strcmp(token, "adsp"))
        {
            lidbg("wake_lock:%s\n", token);
            fs_mem_log("wake_lock:%s\n", token);
            fb_data->has_wakelock_can_not_ignore = 1;
        }
    }
#endif
}


void set_cpu_governor(int state)
{
    char buf[16];
    int len = -1;
    lidbg("set_cpu_governor:%d\n", state);

#if 0
    {
        lidbg("do nothing\n");
        return;
    }
#endif

    if(state == 0)
    {
        len = sprintf(buf, "%s", "ondemand");
    }
    else if(state == 1)
    {
        len = sprintf(buf, "%s", "performance");

    }
    else if(state == 2)
    {
        len = sprintf(buf, "%s", "powersave");

    }
    lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", NULL, buf, len);
}

void wakelock_stat(bool lock, const char *name)
{
#if 0
    char buf[64];
    int len = -1;
    if(lock)
        len = sprintf(buf, "c wakelock lock uncount %s", name);
    else
        len = sprintf(buf, "c wakelock unlock uncount %s", name);

    lidbg_readwrite_file("/dev/mlidbg0", NULL, buf, len);

#else
    lidbg_wakelock_register(lock, name);
#endif
}







int task_find_by_pid(int pid)
{
    struct task_struct *p;
    struct task_struct *selected = NULL;
    unsigned long flags_kill;
    char name[64];
    memset(name, '\0', sizeof(name));

    // DUMP_FUN_ENTER;
    if(ptasklist_lock != NULL)
    {
        lidbg("read_lock+\n");
        read_lock(ptasklist_lock);
    }
    else
        spin_lock_irqsave(&kill_lock, flags_kill);

    for_each_process(p)
    {
        struct mm_struct *mm;
        struct signal_struct *sig;

        task_lock(p);
        mm = p->mm;
        sig = p->signal;
        task_unlock(p);

        selected = p;

        if (p->pid == pid)
        {
            // lidbg("find %s by pid-%d\n", p->comm, pid);
            //lidbg_fs_log(FASTBOOT_LOG_PATH,"find %s by pid-%d\n", p->comm, pid);
            strcpy(name, p->comm);
            //if (selected)
            {
                //force_sig(SIGKILL, selected);
                break;
            }
        }
    }

    if(ptasklist_lock != NULL)
        read_unlock(ptasklist_lock);
    else
        spin_unlock_irqrestore(&kill_lock, flags_kill);

    lidbg_fs_log(FASTBOOT_LOG_PATH, "find %s by pid-%d\n", name, pid);

    // DUMP_FUN_LEAVE;
    return 0;
}




void show_wakelock(bool file_log)
{
    int index = 0;
    struct wakelock_item *pos;
    struct list_head *client_list = &lidbg_wakelock_list;

    if(list_empty(client_list))
        lidbg("<err.lidbg_show_wakelock:nobody_register>\n");
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->name && pos->cunt > 0)
        {
            if(!strcmp(pos->name, "mmc0_detect") || !strcmp(pos->name, "mmc1_detect") || !strcmp(pos->name, "mmc2_detect")
                    || !strcmp(pos->name, "msm_serial_hs_rx") || !strcmp(pos->name, "msm_serial_hs_dma")
              )
                continue;

            index++;
            lidbg("<THE%d:[%d,%d][%s][%s]>,%d,MAX:%d\n",  index, pos->pid, pos->uid, lock_type(pos->is_count_wakelock), pos->name, pos->cunt, pos->cunt_max );
            if(file_log)
            {
                lidbg_fs_log(FASTBOOT_LOG_PATH, "block wakelock %s\n", pos->name);

                if(pos->pid != 0)
                    task_find_by_pid(pos->pid);
            }
        }
    }
}


static int pc_clk_is_enabled(int id)
{
    if (msm_proc_comm(PCOM_CLKCTL_RPC_ENABLED, &id, NULL))
        return 0;
    else
        return id;
}

int check_all_clk_disable(void)
{
    int i = P_NR_CLKS - 1;
    int ret = 0;
    DUMP_FUN;
    while(i >= 0)
    {
        if (pc_clk_is_enabled(i))
        {
            lidbg("pc_clk_is_enabled:%3d\n", i);
            ret++;
        }
        i--;
    }
    return ret;
}
int safe_clk[] = {113, 105, 103, 102, 95, 51, 31, 20, 16, 15, 12, 10, 8, 4, 3, 1};

bool find_unsafe_clk(void)
{
    int j, i = P_NR_CLKS - 1;
    int ret = 0;
    bool is_safe = 0;
    DUMP_FUN;
    while(i >= 0)
    {
        if (pc_clk_is_enabled(i))
        {
            is_safe = 0;
            for(j = 0; j < sizeof(safe_clk); j++)
            {
                if(i == safe_clk[j] )
                {
                    is_safe = 1;
                    break;
                }
            }
            if(is_safe == 0)
            {
                lidbg_fs_log(FASTBOOT_LOG_PATH, "block unsafe clk:%d\n", i);
                ret = 1;
                // return ret;
            }
        }
        i--;
    }
    return ret;

}



int check_clk_disable(void)
{
    int ret = 0;
    DUMP_FUN_ENTER;
    if(pc_clk_is_enabled(P_UART1DM_CLK))
    {
        lidbg("find clk:%d\n", P_UART1DM_CLK);
        lidbg_fs_log(FASTBOOT_LOG_PATH, "clk:%d\n", P_UART1DM_CLK);
        ret = 1;
    }

    if(pc_clk_is_enabled(P_MDP_CLK))
    {
        lidbg("find clk:%d\n", P_MDP_CLK);
        lidbg_fs_log(FASTBOOT_LOG_PATH, "clk:%d\n", P_MDP_CLK);
        ret = 1;
    }

    if(pc_clk_is_enabled(P_ADSP_CLK))
    {
        lidbg("find clk:%d\n", P_ADSP_CLK);
        lidbg_fs_log(FASTBOOT_LOG_PATH, "clk:%d\n", P_ADSP_CLK);
        ret = 1;
    }


    if(pc_clk_is_enabled(P_UART3_CLK))
    {
        lidbg("find clk:%d\n", P_UART3_CLK);
        lidbg_fs_log(FASTBOOT_LOG_PATH, "clk:%d\n", P_UART3_CLK);
        ret = 1;
    }

    DUMP_FUN_LEAVE;
    return ret;

}



int fastboot_task_kill_select(char *task_name)
{
    struct task_struct *p;
    struct task_struct *selected = NULL;
    unsigned long flags_kill;
    DUMP_FUN_ENTER;

    if(ptasklist_lock != NULL)
    {
        lidbg("read_lock+\n");
        read_lock(ptasklist_lock);
    }
    else
        spin_lock_irqsave(&kill_lock, flags_kill);

    for_each_process(p)
    {
        struct mm_struct *mm;
        struct signal_struct *sig;

        task_lock(p);
        mm = p->mm;
        sig = p->signal;
        task_unlock(p);

        selected = p;

        if(!strcmp(p->comm, task_name))
        {
            lidbg("find %s to kill\n", task_name);

            if (selected)
            {
                force_sig(SIGKILL, selected);
                break;
            }
        }
    }

    if(ptasklist_lock != NULL)
        read_unlock(ptasklist_lock);
    else
        spin_unlock_irqrestore(&kill_lock, flags_kill);

    DUMP_FUN_LEAVE;
    return 0;
}


static void fastboot_task_kill_exclude(char *exclude_process[])
{
    static char kill_process[32][25];
    unsigned long flags_kill;
    struct task_struct *p;
    struct mm_struct *mm;
    struct signal_struct *sig;
    u32 i, j = 0;
    bool safe_flag = 0;
    DUMP_FUN_ENTER;

    lidbg("-----------------------\n");
    if(ptasklist_lock != NULL)
    {
        lidbg("read_lock+\n");
        read_lock(ptasklist_lock);
    }
    else
        spin_lock_irqsave(&kill_lock, flags_kill);

    for_each_process(p)
    {
        task_lock(p);
        mm = p->mm;
        sig = p->signal;
        task_unlock(p);
        safe_flag = 0;
        i = 0;

        if(
            (strncmp(p->comm, "flush", sizeof("flush") - 1) == 0) ||
            (strncmp(p->comm, "mtdblock", sizeof("mtdblock") - 1) == 0) ||
            (strncmp(p->comm, "kworker", sizeof("kworker") - 1) == 0) ||
            (strncmp(p->comm, "yaffs", sizeof("yaffs") - 1) == 0) ||
            (strncmp(p->comm, "irq", sizeof("irq") - 1) == 0) ||
            (strncmp(p->comm, "migration", sizeof("migration") - 1) == 0) ||
            (strncmp(p->comm, "mmcqd", sizeof("mmcqd") - 1) == 0) ||
            (strncmp(p->comm, "Fly", sizeof("Fly") - 1) == 0) ||
            (strncmp(p->comm, "fly", sizeof("fly") - 1) == 0) ||
            (strncmp(p->comm, "flyaudio", sizeof("flyaudio") - 1) == 0) ||
            (strncmp(p->comm, "ksdioirqd", sizeof("ksdioirqd") - 1) == 0) ||
            (strncmp(p->comm, "jbd2", sizeof("jbd2") - 1) == 0) ||
            (strncmp(p->comm, "ext4", sizeof("ext4") - 1) == 0) ||
            (strncmp(p->comm, "scsi", sizeof("scsi") - 1) == 0) ||
            (strncmp(p->comm, "loop", sizeof("loop") - 1) == 0) ||
            (strncmp(p->comm, "ServiceHandler", sizeof("ServiceHandler") - 1) == 0) ||
            (strncmp(p->comm, "system", sizeof("system") - 1) == 0) ||
            (strncmp(p->comm, "ksoftirqd", sizeof("ksoftirqd") - 1) == 0) ||
            (strncmp(p->comm, "ftf", sizeof("ftf") - 1) == 0) ||
            (strncmp(p->comm, "lidbg_", sizeof("lidbg_") - 1) == 0)
        )
        {
            continue;
        }


        {
            struct string_dev *pos;
            list_for_each_entry(pos, &fastboot_kill_list, tmp_list)
            {
                if(strcmp(p->comm, pos->yourkey) == 0)
                {
                    safe_flag = 1;
                    //lidbg("nokill:%s\n", pos->yourkey);
                    break;
                }
                else
                {

                }
            }
        }

        if(safe_flag == 0)
        {
            if (p)
            {
                sprintf(kill_process[j++] , p->comm);
                force_sig(SIGKILL, p);
            }
        }
    }//for_each_process

    if(ptasklist_lock != NULL)
        read_unlock(ptasklist_lock);
    else
        spin_unlock_irqrestore(&kill_lock, flags_kill);

    lidbg("-----------------------\n\n");


    if(j == 0)
        lidbg("find nothing to kill\n");
    else
        for(i = 0; i < j; i++)
        {
            {
                lidbg("## find %s to kill ##\n", kill_process[i]);

            }
        }
    DUMP_FUN_LEAVE;

}


u32 GetTickCount(void)
{
    struct timespec t_now;

    do_posix_clock_monotonic_gettime(&t_now);

    monotonic_to_bootbased(&t_now);

    return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;
}


int kill_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{

    fastboot_task_kill_exclude(NULL);
    return 1;
}

void create_new_proc_entry(void)
{
    create_proc_read_entry("kill_task", 0, NULL, kill_proc, NULL);

}

int pwroff_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    DUMP_FUN_ENTER;
    if(PM_STATUS_LATE_RESUME_OK == fastboot_get_status())
        fastboot_pwroff();
    //list_active_locks();
    return 0;
}


void create_new_proc_entry2(void)
{
    create_proc_read_entry("fastboot_pwroff", 0, NULL, pwroff_proc, NULL);
}


#include "./fake_suspend.c"

static int thread_pwroff(void *data)
{
    int time_count;

    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            time_count = 0;
            wait_for_completion(&early_suspend_start);
            while(1)
            {
                msleep(1000);
                time_count++;
                if(fastboot_get_status() == PM_STATUS_READY_TO_PWROFF)
                {

                    if(time_count >= 5 * 2)
                    {
                        lidbgerr("thread_pwroff wait early suspend timeout!\n");
                        set_power_state(0);
                        break;
                    }
                }
                else
                {
                    lidbg("thread_pwroff wait time_count=%d\n", time_count);
                    break;

                }
            }
        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}


static int thread_fastboot_pwroff(void *data)
{
    while(1)
    {

        wait_for_completion(&pwroff_start);
        DUMP_FUN_ENTER;
        fastboot_pwroff();
    }
    return 0;
}


static int thread_fastboot_suspend(void *data)
{
    int time_count;
    static u32 lock_resume_last_time = 0;
    u32 lock_resume_interval;
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            time_count = 0;
            wait_for_completion(&suspend_start);
            while(1)
            {
                msleep(1000);
                time_count++;
                if(fastboot_get_status() == PM_STATUS_EARLY_SUSPEND_PENDING)
                {

                    if(fb_data->kill_task_en)
                        fastboot_task_kill_exclude(NULL);

                    if((fb_data->clk_block_suspend) && (wakelock_occur_count < 5 ))
                    {
                        lidbg("some clk block suspend!\n");
                        wakelock_occur_count++;
                        fb_data->is_quick_resume = 1;
                        set_power_state(1);
                        break;
                    }
#if (defined(BOARD_V1) || defined(BOARD_V2))
#else
                    list_active_locks();
                    if((fb_data->has_wakelock_can_not_ignore) && (wakelock_occur_count < 5 ))
                    {
#if 0
                        fast6boot_task_kill_select(".flyaudio.media");
                        //fastboot_task_kill_select("d.process.media");
#else
                        lidbg("some wakelock block suspend!\n");
                        wakelock_occur_count++;
                        fb_data->is_quick_resume = 1;
                        set_power_state(1);
                        break;

#endif
                    }
#endif


#ifdef HAS_LOCK_RESUME
                    if(time_count >= /*MAX_WAIT_UNLOCK_TIME*/fb_data->max_wait_unlock_time)
#else
                    if(time_count >= 10)
#endif
                    {
                        lidbgerr("thread_fastboot_suspend wait suspend timeout!\n");

#ifdef HAS_LOCK_RESUME
                        lock_resume_interval = GetTickCount() - lock_resume_last_time;
                        lock_resume_last_time = GetTickCount();
                        lidbg("wakelock_occur_interval=%d,GetTickCount=%d\n", lock_resume_interval, GetTickCount());
                        if(lock_resume_interval > 600 * 1000)//10min
                        {
                            lidbg("reset wakelock_occur_count!!\n");
                            wakelock_occur_count = 0;
                        }
                        wakelock_occur_count++;
                        lidbg("wakelock_occur_count=%d\n", wakelock_occur_count);

                        show_wakelock(1);

                        if(wakelock_occur_count <= /*WAIT_LOCK_RESUME_TIMES*/fb_data->haslock_resume_times)
                        {
                            fb_data->is_quick_resume = 1;
                            set_power_state(1);
                            lidbg_fs_log(FASTBOOT_LOG_PATH, "quick resume\n");
                        }
                        else
#endif
                        {
#if 0//def HAS_LOCK_RESUME
                            lidbg("$+\n");
                            msleep((10 - MAX_WAIT_UNLOCK_TIME) * 1000);
                            lidbg("$-\n");
#endif
                            //log_active_locks();
                            fs_dump_kmsg((char *)__FUNCTION__, __LOG_BUF_LEN);
                            lidbg_fs_log(FASTBOOT_LOG_PATH, "force suspend\n");
                            wakelock_occur_count = 0;
                            if(fastboot_get_status() == PM_STATUS_EARLY_SUSPEND_PENDING)
                            {
                                lidbg("start force suspend...\n");
                                ignore_wakelock = 1;
                                wake_lock(&(fb_data->flywakelock));
                                wake_unlock(&(fb_data->flywakelock));
                            }
                            else
                            {
                                lidbg("thread_fastboot_suspend wait time_count=%d\n", time_count);
                                complete(&late_suspend_start);
                            }
                        }
                        break;
                    }
                }
                else
                {
                    lidbg("thread_fastboot_suspend wait time_count=%d\n", time_count);
                    complete(&late_suspend_start);
#ifdef HAS_LOCK_RESUME
                    wakelock_occur_count = 0;
#endif
                    break;

                }
            }
        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}


static int thread_late_suspend(void *data)
{
    int time_count;

    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;
        if(1)
        {
            time_count = 0;
            wait_for_completion(&late_suspend_start);
            while(1)
            {
                msleep(1000);
                time_count++;
                if(fastboot_get_status() == PM_STATUS_SUSPEND_PENDING)
                {

                    if(time_count >= 10)
                    {
                        lidbgerr("late suspend wait early suspend timeout!\n");
#if 0
                        lidbg("start force suspend...\n");
                        ignore_wakelock = 1;
                        wake_lock(&(fb_data->flywakelock));
                        wake_unlock(&(fb_data->flywakelock));
#else
                        fastboot_set_status(PM_STATUS_RESUME_OK);
                        set_power_state(1);
#endif
                        break;
                    }
                }
                else
                {
                    lidbg("late suspend wait time_count=%d\n", time_count);
                    break;

                }
            }
        }
        else
        {
            schedule_timeout(HZ);
        }
    }
    return 0;
}


void fastboot_set_status(LIDBG_FAST_PWROFF_STATUS status);

static int thread_fastboot_resume(void *data)
{
    while(1)
    {
        wait_for_completion(&resume_ok);
        DUMP_FUN_ENTER;
        msleep(3000);
        set_power_state(1);
        //SOC_Key_Report(KEY_HOME, KEY_PRESSED_RELEASED);
        //SOC_Key_Report(KEY_BACK, KEY_PRESSED_RELEASED);

        msleep(1000);
        fastboot_set_status(PM_STATUS_LATE_RESUME_OK);

        //log acc off times
        if(fb_data->resume_count  % 5 == 0)
        {
            lidbg_fs_log(FASTBOOT_LOG_PATH, "%d\n", fb_data->resume_count);
            fs_save_state();
        }



        DUMP_FUN_LEAVE;
    }
    return 0;
}


int fastboot_get_status(void)
{
    return fb_data->suspend_pending;
}

void fastboot_set_status(LIDBG_FAST_PWROFF_STATUS status)
{
    lidbg("fastboot_set_status:%d\n", status);
    fb_data->suspend_pending = status;
}

void fastboot_pwroff(void)
{
    u32 err_count = 0;
    DUMP_FUN_ENTER;

    while((PM_STATUS_LATE_RESUME_OK != fastboot_get_status()) && (PM_STATUS_READY_TO_FAKE_PWROFF != fastboot_get_status()))
    {
        lidbgerr("Call SOC_PWR_ShutDown when suspend_pending != PM_STATUS_LATE_RESUME_OK :%d\n", fastboot_get_status());
        err_count++;

        if(err_count > 50)//10s
        {
            lidbgerr("err_count > 50,force fastboot_pwroff!\n");
            break;
        }
        msleep(200);

    }
    fastboot_set_status(PM_STATUS_READY_TO_PWROFF);

    SOC_Dev_Suspend_Prepare();

    //set_cpu_governor(1);//performance

    //avoid mem leak
    //    fastboot_task_kill_select("mediaserver");
    //    fastboot_task_kill_select("void");

    //fs_save_state();

    if(!g_var.is_fly)
    {
        msleep(1000);

#ifdef RUN_FASTBOOT
#if (defined(BOARD_V1) || defined(BOARD_V2))
        //SOC_Write_Servicer(CMD_ACC_OFF_PROPERTY_SET);
        SOC_Write_Servicer(CMD_FAST_POWER_OFF);
#endif
#else
        SOC_Key_Report(KEY_POWER, KEY_PRESSED_RELEASED);
#endif

    }
    complete(&early_suspend_start);

}


bool fastboot_is_ignore_wakelock(void)
{
    return ignore_wakelock;
}

void fastboot_go_pwroff(void)
{
    DUMP_FUN_ENTER;
    complete(&pwroff_start);
}



static void set_func_tbl(void)
{
    if(!g_var.is_fly)
        plidbg_dev->soc_func_tbl.pfnSOC_PWR_ShutDown = fastboot_go_pwroff;
    else
        plidbg_dev->soc_func_tbl.pfnSOC_PWR_ShutDown = fastboot_pwroff;

    plidbg_dev->soc_func_tbl.pfnSOC_PWR_GetStatus = fastboot_get_status;
    plidbg_dev->soc_func_tbl.pfnSOC_PWR_SetStatus = fastboot_set_status;
    plidbg_dev->soc_func_tbl.pfnSOC_PWR_Ignore_Wakelock = fastboot_is_ignore_wakelock;
    plidbg_dev->soc_func_tbl.pfnSOC_Fake_Register_Early_Suspend = fake_register_early_suspend;

    plidbg_dev->soc_func_tbl.pfnSOC_Get_WakeLock = fastboot_get_wake_locks;

    plidbg_dev->soc_func_tbl.pfnSOC_WakeLock_Stat = wakelock_stat;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fastboot_early_suspend(struct early_suspend *h)
{

    lidbg("fastboot_early_suspend:%d\n", fb_data->resume_count);
    if(PM_STATUS_EARLY_SUSPEND_PENDING != fastboot_get_status())
    {
        lidbgerr("Call devices_early_suspend when suspend_pending != PM_STATUS_READY_TO_PWROFF\n");
    }

    set_cpu_governor(1);//performance
    if(fb_data->kill_task_en)
    {

        lidbg("kill_task_en+\n");
        msleep(1000); //for test
        fastboot_task_kill_exclude(NULL);
        msleep(1000); //for test
        lidbg("kill_task_en-\n");
    }
    check_all_clk_disable();

    set_cpu_governor(0);//ondemand

#if 0 //for test
    ignore_wakelock = 1;
#endif
#if 0//(defined(BOARD_V1) || defined(BOARD_V2))
    fb_data->clk_block_suspend = 0;
    wake_unlock(&(fb_data->flywakelock));
#else

    //if(check_clk_disable())
    if(find_unsafe_clk())
    {
        // fb_data->clk_block_suspend = 1;
    }
    //else
    {
        fb_data->clk_block_suspend = 0;
        wake_unlock(&(fb_data->flywakelock));
    }
#endif

    //wake_lock(&(fb_data->flywakelock));//to test force suspend
    complete(&suspend_start);
}

static void fastboot_late_resume(struct early_suspend *h)
{
    DUMP_FUN;

#ifdef HAS_LOCK_RESUME
    //if((wakelock_occur_count != 0)||(fb_data->clk_block_suspend == 1)||(fb_data->has_wakelock_can_not_ignore == 1))
    if(fb_data->is_quick_resume)
    {
        lidbg("quick late resume\n");
        if(fb_data->clk_block_suspend == 0)
            wake_lock(&(fb_data->flywakelock));
        //reset flag
        fb_data->has_wakelock_can_not_ignore = 0;
        fb_data->clk_block_suspend = 0;
        fb_data->is_quick_resume = 0;

    }
    fastboot_set_status(PM_STATUS_LATE_RESUME_OK);

#endif
    lidbg("machine_id=%d\n", g_var.machine_id);
    //set_cpu_governor(0);//ondemand
}
#endif


void kill_all_task(char *key, char *value)
{
    fastboot_task_kill_exclude(NULL);
}


void cb_password_fastboot_pwroff(char *password )
{
    fastboot_pwroff();
}

static int  fastboot_probe(struct platform_device *pdev)
{
    int ret;
    DUMP_FUN_ENTER;

    fb_data = kmalloc(sizeof(struct fastboot_data), GFP_KERNEL);
    if (!fb_data)
    {
        ret = -ENODEV;
        goto fail_mem;
    }
    mutex_init(&fb_data->lock);
    wake_lock_init(&(fb_data->flywakelock), WAKE_LOCK_SUSPEND, "lidbg_wake_lock");

    fastboot_set_status(PM_STATUS_LATE_RESUME_OK);
    fb_data->resume_count = 0;
    fb_data->clk_block_suspend = 0;
    fb_data->has_wakelock_can_not_ignore = 0;
    fb_data->is_quick_resume = 0;

#ifdef CONFIG_HAS_EARLYSUSPEND
    fb_data->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 5; //the later the better
    fb_data->early_suspend.suspend = fastboot_early_suspend;
    fb_data->early_suspend.resume = fastboot_late_resume;
    register_early_suspend(&fb_data->early_suspend);
    wake_lock(&(fb_data->flywakelock));
#endif


    fs_regist_state("acc_times", (int *)&fb_data->resume_count);
    FS_REGISTER_INT(fb_data->kill_task_en, "kill_task_en", 1, NULL);
    FS_REGISTER_INT(fb_data->kill_all_task, "kill_all_task", 0, kill_all_task);
    FS_REGISTER_INT(fb_data->haslock_resume_times, "haslock_resume_times", 0, NULL);
    FS_REGISTER_INT(fb_data->max_wait_unlock_time, "max_wait_unlock_time", 5, NULL);

    fs_file_separator(FASTBOOT_LOG_PATH);
    fs_register_filename_list(FASTBOOT_LOG_PATH, true);

    INIT_COMPLETION(early_suspend_start);
    pwroff_task = kthread_create(thread_pwroff, NULL, "pwroff_task");
    if(IS_ERR(pwroff_task))
    {
        lidbg("Unable to start kernel thread.\n");

    }
    else wake_up_process(pwroff_task);

    INIT_COMPLETION(pwroff_start);
    pwroff_task = kthread_create(thread_fastboot_pwroff, NULL, "pwroff_task");
    if(IS_ERR(pwroff_task))
    {
        lidbg("Unable to start kernel thread.\n");

    }
    else wake_up_process(pwroff_task);


    INIT_COMPLETION(suspend_start);
    suspend_task = kthread_create(thread_fastboot_suspend, NULL, "suspend_task");
    if(IS_ERR(suspend_task))
    {
        lidbg("Unable to start kernel thread.\n");

    }
    else wake_up_process(suspend_task);


    INIT_COMPLETION(resume_ok);
    resume_task = kthread_create(thread_fastboot_resume, NULL, "pwroff_task");
    if(IS_ERR(resume_task))
    {
        lidbg("Unable to start kernel thread.\n");

    }
    else wake_up_process(resume_task);


    INIT_COMPLETION(late_suspend_start);
    late_suspend_task = kthread_create(thread_late_suspend, NULL, "late_suspend_task");
    if(IS_ERR(late_suspend_task))
    {
        lidbg("Unable to start kernel thread.\n");

    }
    else wake_up_process(late_suspend_task);

    spin_lock_init(&kill_lock);
    create_new_proc_entry();
    create_new_proc_entry2();
    create_proc_entry_fake_suspend();
    create_proc_entry_fake_wakeup();

    if(fs_fill_list("/flysystem/lib/out/fastboot_not_kill_list.conf", FS_CMD_FILE_LISTMODE, &fastboot_kill_list) < 0)
        fs_fill_list("/system/lib/modules/out/fastboot_not_kill_list.conf", FS_CMD_FILE_LISTMODE, &fastboot_kill_list);

    lidbg_chmod("/sys/power/state");

    te_regist_password("001200", cb_password_fastboot_pwroff);
    DUMP_FUN_LEAVE;

    return 0;

fail_mem:
    return ret;
}

static int  fastboot_remove(struct platform_device *pdev)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&fb_data->early_suspend);
#endif

    kfree(fb_data);

    return 0;
}

#ifdef CONFIG_PM
static int fastboot_suspend(struct device *dev)
{
    DUMP_FUN;
    if(PM_STATUS_SUSPEND_PENDING != fastboot_get_status())
    {
        lidbgerr("Call fastboot_suspend when suspend_pending != PM_STATUS_SUSPEND_PENDING\n");
    }
#if 0
    if(check_all_clk_disable())
    {
        // lidbg_io("wake up\n");
        // set_power_state(1);//fail
    }
#endif
    return 0;
}

static int fastboot_resume(struct device *dev)
{
    DUMP_FUN;
    lidbg("fastboot_resume:%d\n", fb_data->resume_count);
    ++fb_data->resume_count;
    fastboot_set_status(PM_STATUS_RESUME_OK);
    wake_lock(&(fb_data->flywakelock));
    complete(&resume_ok);

    ignore_wakelock = 0;
    wakelock_occur_count = 0;

    return 0;
}

static struct dev_pm_ops fastboot_pm_ops =
{
    .suspend	= fastboot_suspend,
    .resume		= fastboot_resume,
};
#endif

static struct platform_driver fastboot_driver =
{
    .probe		= fastboot_probe,
    .remove     = fastboot_remove,
    .driver         = {
        .name = "lidbg_fastboot",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &fastboot_pm_ops,
#endif
    },
};

static struct platform_device lidbg_fastboot_device =
{
    .name               = "lidbg_fastboot",
    .id                 = -1,
};

static int __init fastboot_init(void)
{
    DUMP_BUILD_TIME;

    LIDBG_GET;
    set_func_tbl();

    platform_device_register(&lidbg_fastboot_device);
    return platform_driver_register(&fastboot_driver);
}

static void __exit fastboot_exit(void)
{
    platform_driver_unregister(&fastboot_driver);
}


module_init(fastboot_init);
module_exit(fastboot_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("lidbg fastboot driver");

