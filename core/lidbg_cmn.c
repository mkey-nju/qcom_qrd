/* Copyright (c) 2012, swlee
 *
 */
#include "lidbg.h"

#define GET_INODE_FROM_FILEP(filp) ((filp)->f_path.dentry->d_inode)
char g_binpath[50];

bool is_file_exist(char *file)
{
    struct file *filep;
    filep = filp_open(file, O_RDONLY , 0);
    if(IS_ERR(filep))
        return false;
    else
    {
        filp_close(filep, 0);
        return true;
    }
}

char *get_bin_path( char *buf)
{
#ifdef USE_CALL_USERHELPER
    if(!strchr(buf, '/'))
    {
        char *path;
        path = (is_file_exist(RECOVERY_MODE_DIR)) ? "/sbin/" : "/system/bin/";
        sprintf(g_binpath, "%s%s", path, buf);
        return g_binpath;
    }
    else
        return buf;
#else
    return buf;
#endif
}
int lidbg_readwrite_file(const char *filename, char *rbuf, const char *wbuf, size_t length)
{
    int ret = 0;
    struct file *filp = (struct file *) - ENOENT;
    mm_segment_t oldfs;
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    do
    {
        int mode = (wbuf) ? O_RDWR : O_RDONLY;
        filp = filp_open(filename, mode, S_IRUSR);
        if (IS_ERR(filp) || !filp->f_op)
        {
            ret = -ENOENT;
            break;
        }
        if (!filp->f_op->write || !filp->f_op->read)
        {
            filp_close(filp, NULL);
            ret = -ENOENT;
            break;
        }
        if (length == 0)
        {
            struct inode    *inode;
            inode = GET_INODE_FROM_FILEP(filp);
            if (!inode)
            {
                lidbg("kernel_readwrite_file: Error 2\n");
                ret = -ENOENT;
                break;
            }
            ret = i_size_read(inode->i_mapping->host);
            break;
        }
        if (wbuf)
        {
            ret = filp->f_op->write(filp, wbuf, length, &filp->f_pos);
            if (ret < 0)
            {
                lidbg("kernel_readwrite_file: Error 3\n");
                break;
            }
        }
        else
        {
            ret = filp->f_op->read(filp, rbuf, length, &filp->f_pos);
            if (ret < 0)
            {
                lidbg("kernel_readwrite_file: Error 4\n");
                break;
            }
        }
    }
    while (0);

    if (!IS_ERR(filp))
        filp_close(filp, NULL);
    set_fs(oldfs);
    return ret;
}

void set_power_state(int state)
{
    lidbg("set_power_state:%d\n", state);

    if(state == 0)
        lidbg_readwrite_file("/sys/power/state", NULL, "mem", sizeof("mem") - 1);
    else
        lidbg_readwrite_file("/sys/power/state", NULL, "on", sizeof("on") - 1);
}

int read_proc(char *buf, char **start, off_t offset, int count, int *eof, void *data )
{
    int len = 0;
    struct task_struct *task_list;

    for_each_process(task_list)
    {
        len  += sprintf(buf + len, "%s %d \n", task_list->comm, task_list->pid);
    }
    return len;
}

void create_new_proc_entry(void)
{
    create_proc_read_entry("ps_list", 0, NULL, read_proc, NULL);
}

int lidbg_task_kill_select(char *task_name)
{
    struct task_struct *p;
    struct task_struct *selected = NULL;
    DUMP_FUN_ENTER;

    //read_lock(&tasklist_lock);
    for_each_process(p)
    {
        struct mm_struct *mm;
        struct signal_struct *sig;

        task_lock(p);
        mm = p->mm;
        sig = p->signal;
        task_unlock(p);

        selected = p;
        //lidbg( "process %d (%s)\n",p->pid, p->comm);

        if(!strcmp(p->comm, task_name))
        {
            lidbg("find %s to kill\n", task_name);

            if (selected)
            {
                force_sig(SIGKILL, selected);
                return 1;
            }
        }
        //read_unlock(&tasklist_lock);
    }
    DUMP_FUN_LEAVE;
    return 0;
}

u32 lidbg_get_ns_count(void)
{
    struct timespec t_now;
    getnstimeofday(&t_now);
    return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;

}

//return how many ms since boot
u32 get_tick_count(void)
{
    struct timespec t_now;
    do_posix_clock_monotonic_gettime(&t_now);
    monotonic_to_bootbased(&t_now);
    return t_now.tv_sec * 1000 + t_now.tv_nsec / 1000000;
}

int lidbg_get_current_time(char *time_string, struct rtc_time *ptm)
{
    int  tlen = -1;
    struct timespec ts;
    struct rtc_time tm;
    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &tm);
    if(time_string)
        tlen = sprintf(time_string, "%d-%02d-%02d__%02d.%02d.%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour , tm.tm_min, tm.tm_sec);
    if(ptm)
        *ptm = tm;
    return tlen;
}
int  lidbg_launch_user( char bin_path[], char argv1[], char argv2[], char argv3[], char argv4[], char argv5[], char argv6[])
{
#ifdef USE_CALL_USERHELPER
    int ret;
    char *argv[] = { bin_path, argv1, argv2, argv3, argv4, argv5, argv6, NULL };
    static char *envp[] = { "HOME=/", "TERM=linux", "PATH=/system/bin:/sbin", NULL };
    lidbg("%s ,%s\n", bin_path, argv1);
    ret = call_usermodehelper(bin_path, argv, envp, UMH_WAIT_EXEC);
    //NOTE:  I test that:use UMH_NO_WAIT can't lunch the exe; UMH_WAIT_PROCwill block the ko,
    //UMH_WAIT_EXEC  is recommended.
    if (ret < 0)
        lidbg("lunch [%s %s] fail!\n", bin_path, argv1);
    return ret;
#else
    char shell_cmd[512] = {0};
    sprintf(shell_cmd, "%s %s %s %s %s %s %s ", bin_path, argv1 == NULL ? "" : argv1, argv2 == NULL ? "" : argv2, argv3 == NULL ? "" : argv3, argv4 == NULL ? "" : argv4, argv5 == NULL ? "" : argv5, argv6 == NULL ? "" : argv6);
    lidbg_uevent_shell(shell_cmd);
    return 1;
#endif
}

static struct class *lidbg_cdev_class = NULL;
bool new_cdev(struct file_operations *cdev_fops, char *nodename)
{
    struct cdev *new_cdev = NULL;
    struct device *new_device = NULL;
    dev_t dev_number = 0;
    int major_number_ts = 0;
    int err, result;

    if(!cdev_fops->owner || !nodename)
    {
        LIDBG_ERR("cdev_fops->owner||nodename \n");
        return false;
    }

    new_cdev = kzalloc(sizeof(struct cdev), GFP_KERNEL);
    if (!new_cdev)
    {
        LIDBG_ERR("kzalloc \n");
        return false;
    }

    dev_number = MKDEV(major_number_ts, 0);
    if(major_number_ts)
        result = register_chrdev_region(dev_number, 1, nodename);
    else
        result = alloc_chrdev_region(&dev_number, 0, 1, nodename);

    if (result)
    {
        LIDBG_ERR("alloc_chrdev_region result:%d \n", result);
        return false;
    }
    major_number_ts = MAJOR(dev_number);

    cdev_init(new_cdev, cdev_fops);
    new_cdev->owner = cdev_fops->owner;
    new_cdev->ops = cdev_fops;
    err = cdev_add(new_cdev, dev_number, 1);
    if (err)
    {
        LIDBG_ERR("cdev_add result:%d \n", err);
        return false;
    }

    if(!lidbg_cdev_class)
    {
        lidbg_cdev_class = class_create(cdev_fops->owner, "lidbg_cdev_class");
        if(IS_ERR(lidbg_cdev_class))
        {
            LIDBG_ERR("class_create\n");
            cdev_del(new_cdev);
            kfree(new_cdev);
            lidbg_cdev_class = NULL;
            return false;
        }
    }

    new_device = device_create(lidbg_cdev_class, NULL, dev_number, NULL, "%s%d", nodename, 0);
    if (!new_device)
    {
        LIDBG_ERR("device_create\n");
        cdev_del(new_cdev);
        kfree(new_cdev);
        return false;
    }

    return true;
}

u32 lidbg_get_random_number(u32 num_max)
{
    u32 ret;
    get_random_bytes(&ret, sizeof(ret));
    return ret % num_max;
}
void lidbg_domineering_ack(void)
{
    DUMP_FUN;
    lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK));
}
int  lidbg_exe(char path[], char argv1[], char argv2[], char argv3[], char argv4[], char argv5[], char argv6[])
{
    return lidbg_launch_user(get_bin_path(path), argv1, argv2, argv3, argv4, argv5, argv6);
}
int  lidbg_mount(char path[])
{
    return lidbg_launch_user(MOUNT_PATH, "-o", "remount", path, NULL, NULL, NULL);
}
int  lidbg_chmod(char path[])
{
    return lidbg_launch_user(CHMOD_PATH, "777", path, NULL, NULL, NULL, NULL);
}
int  lidbg_mv(char from[], char to[])
{
    return lidbg_launch_user(MV_PATH, from, to, NULL, NULL, NULL, NULL);
}
int  lidbg_rm(char path[])
{
    return lidbg_launch_user(RM_PATH, path, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_rmdir(char path[])
{
    return lidbg_launch_user(RMDIR_PATH, "-r", path, NULL, NULL, NULL, NULL);
}
int  lidbg_mkdir(char path[])
{
    return lidbg_launch_user(MKDIR_PATH, path, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_touch(char path[])
{
    return lidbg_launch_user(TOUCH_PATH, path, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_reboot(void)
{
    return lidbg_launch_user(REBOOT_PATH, NULL, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_setprop(char key[], char value[])
{
    return lidbg_launch_user(SETPROP_PATH, key, value, NULL, NULL, NULL, NULL);
}
int  lidbg_start(char server[])
{
    return lidbg_launch_user(get_bin_path("start"), server, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_stop(char server[])
{
    return lidbg_launch_user(get_bin_path("stop"), server, NULL, NULL, NULL, NULL, NULL);
}
int  lidbg_force_stop_apk(char packagename[])
{
    return lidbg_launch_user(get_bin_path("am"), "force-stop", packagename, "&", NULL, NULL, NULL);
}
int  lidbg_toast_show(char *string, int int_value)
{
    char para[128] = {0};
    sprintf(para, "--es action %s --ei int %d &", string ? string : "null", int_value);
    return lidbg_launch_user(get_bin_path("am"), "broadcast", "-a", "com.lidbg.broadcast", para, NULL, NULL);
}

void pm_install_apk(char apkpath[])
{
    lidbg_launch_user(get_bin_path("pm"), "install", "-r", apkpath, "&", NULL, NULL);
}
void callback_pm_install(char *dirname, char *filename)
{
    char apkpath[256];
    if(!filename || strstr(filename, "apk") == NULL)
    {
        LIDBG_ERR("failed:%s\n", filename);
        return;
    }
    memset(apkpath, '\0', sizeof(apkpath));
    sprintf(apkpath, "'%s/%s'", dirname, filename);
    pm_install_apk(apkpath);
}
int  lidbg_pm_install_dir(char apkpath_or_apkdirpath[])
{
    if(strstr(apkpath_or_apkdirpath, "apk") == NULL)
        lidbg_readdir_and_dealfile(apkpath_or_apkdirpath, callback_pm_install);
    else
        pm_install_apk(apkpath_or_apkdirpath);
    return 1;
}

struct name_list
{
    char name[33];
    struct list_head list;
};
static int readdir_build_namelist(void *arg, const char *name, int namlen,	loff_t offset, u64 ino, unsigned int d_type)
{
    struct list_head *names = arg;
    struct name_list *entry;
    entry = kzalloc(sizeof(struct name_list), GFP_KERNEL);
    if (entry == NULL)
        return -ENOMEM;
    memcpy(entry->name, name, namlen);
    entry->name[namlen] = '\0';
    list_add(&entry->list, names);
    return 0;
}
bool lidbg_readdir_and_dealfile(char *insure_is_dir, void (*callback)(char *dirname, char *filename))
{
    LIST_HEAD(names);
    struct file *dir_file;
    struct dentry *dir;
    int status;

    if(!insure_is_dir || !callback)
        return false;

    dir_file = filp_open(insure_is_dir, O_RDONLY | O_DIRECTORY, 0);
    if (IS_ERR(dir_file))
    {
        LIDBG_ERR("open%s,%ld\n", insure_is_dir, PTR_ERR(dir_file));
        return false;
    }
    else
    {
        struct name_list *entry;
        LIDBG_SUC("open:<%s,%s>\n", insure_is_dir, dir_file->f_path.dentry->d_name.name);
        dir = dir_file->f_path.dentry;
        status = vfs_readdir(dir_file, readdir_build_namelist, &names);
        if (dir_file)
            fput(dir_file);
        while (!list_empty(&names))
        {
            entry = list_entry(names.next, struct name_list, list);
            if (!status && entry)
            {
                if(callback && entry->name[0] != '.') // ignore "." and ".."
                    callback(insure_is_dir, entry->name);
                list_del(&entry->list);
                kfree(entry);
            }
            entry = NULL;
        }
        return true;
    }
}
bool lidbg_new_cdev(struct file_operations *cdev_fops, char *nodename)
{
    if(new_cdev(cdev_fops, nodename))
    {
        char path[32];
        sprintf(path, "/dev/%s0", nodename);
        ssleep(1);
        lidbg_chmod(path);
        LIDBG_SUC("[%s]\n", path);
        return true;
    }
    else
    {
        LIDBG_ERR("[/dev/%s0]\n", nodename);
        return false;
    }
}

/*
input:  c io w 27 1
output:  token[0]=c  token[4]=1  return:5
*/
int lidbg_token_string(char *buf, char *separator, char **token)
{
    char *token_tmp;
    int pos = 0;
    if(!buf || !separator)
    {
        LIDBG_ERR("buf||separator NULL?\n");
        return pos;
    }
    while((token_tmp = strsep(&buf, separator)) != NULL )
    {
        *token = token_tmp;
        token++;
        pos++;
    }
    return pos;
}

#define ITEM_MAX (35)
#define MOUNT_BUF_MAX (3 * 1024)
static int invaled_mount_point = 0;
static char buf_mount[MOUNT_BUF_MAX];
static struct mounted_volume g_mounts_state[ITEM_MAX];
static int read_file(const char *filename, char *rbuff, int readlen)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int read_len = 1;

    filep = filp_open(filename,  O_RDONLY, 0);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"err:filp_open\n\n\n\n");
        return -1;
    }
    old_fs = get_fs();
    set_fs(get_ds());

    filep->f_op->llseek(filep, 0, 0);
    read_len = filep->f_op->read(filep, rbuff, readlen, &filep->f_pos);

    set_fs(old_fs);
    filp_close(filep, 0);
    return read_len;
}
bool scan_mounted_volumes(char *info_path)
{
    char *mount_item[ITEM_MAX] = {NULL};
    char 	*item_token[12] = {NULL};
    char	 *p = buf_mount;
    int	 ret = false;
    int mount_item_max;
    int item_token_len, loop;

    memset(p, '\0', MOUNT_BUF_MAX);
    if(read_file(info_path, p, MOUNT_BUF_MAX - 1) >= 0)
    {
        mount_item_max = lidbg_token_string(buf_mount, "\n", mount_item);
        invaled_mount_point = mount_item_max;
        for(loop = 0; loop < mount_item_max; loop++)
        {
            item_token_len = lidbg_token_string(mount_item[loop], " ", item_token);
            if(item_token_len >= 4)
            {
                g_mounts_state[loop].device = item_token[0];
                g_mounts_state[loop].mount_point = item_token[1];
                g_mounts_state[loop].filesystem = item_token[2];
                g_mounts_state[loop].others = item_token[3];
                ret = true;
            }
        }
    }
    else
        lidbg("err.scan_mounted_volumes:%d\n", invaled_mount_point);
    return ret;
}
struct mounted_volume *find_mounted_volume_by_mount_point(char *mount_point)
{
    int i;
    scan_mounted_volumes("/proc/mounts");
    msleep(300);
    if (invaled_mount_point <= 0)
    {
        lidbg("err.invaled_mount_point <= 0,%d\n", invaled_mount_point);
        return NULL;
    }

    for (i = 0; i < invaled_mount_point; i++)
    {
        if (g_mounts_state[i].mount_point != NULL && !strcmp(g_mounts_state[i].mount_point, mount_point))
        {
            LIDBG_SUC("%s\n", mount_point);
            return &g_mounts_state[i];
        }
    }
    LIDBG_ERR("%s\n", mount_point);
    return NULL;
}

void mod_cmn_main(int argc, char **argv)
{

    if(!strcmp(argv[0], "user"))
    {

        if(argc < 2)
        {
            lidbg("Usage:\n");
            lidbg("bin_path\n");
            lidbg("bin_path argv1\n");
            return;

        }

        {
            char *argv2[] = { argv[1], argv[2], argv[3], argv[4], NULL };
            static char *envp[] = { "HOME=/", "TERM=linux", "PATH=/system/bin:/sbin", NULL };
            int ret;
            lidbg("%s ,%s ,%s ,%s\n", argv[1], argv[2], argv[3], argv[4]);

            ret = call_usermodehelper(argv[1], argv2, envp, UMH_WAIT_PROC);
            if (ret < 0)
                lidbg("lunch fail!\n");
            else
                lidbg("lunch  success!\n");
        }

    }

    return;
}

static int __init cmn_init(void)
{
    create_new_proc_entry();
    LIDBG_MODULE_LOG;
    return 0;
}

static void __exit cmn_exit(void)
{
    remove_proc_entry("proc_entry", NULL);
}

module_init(cmn_init);
module_exit(cmn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");

EXPORT_SYMBOL(find_mounted_volume_by_mount_point);
EXPORT_SYMBOL(lidbg_readdir_and_dealfile);
EXPORT_SYMBOL(lidbg_token_string);
EXPORT_SYMBOL(lidbg_get_random_number);
EXPORT_SYMBOL(lidbg_exe);
EXPORT_SYMBOL(lidbg_mount);
EXPORT_SYMBOL(lidbg_chmod);
EXPORT_SYMBOL(lidbg_new_cdev);
EXPORT_SYMBOL(lidbg_mv);
EXPORT_SYMBOL(lidbg_rm);
EXPORT_SYMBOL(lidbg_rmdir);
EXPORT_SYMBOL(lidbg_mkdir);
EXPORT_SYMBOL(lidbg_touch);
EXPORT_SYMBOL(lidbg_reboot);
EXPORT_SYMBOL(lidbg_setprop);
EXPORT_SYMBOL(lidbg_start);
EXPORT_SYMBOL(lidbg_stop);
EXPORT_SYMBOL(lidbg_pm_install_dir);
EXPORT_SYMBOL(lidbg_toast_show);
EXPORT_SYMBOL(lidbg_force_stop_apk);
EXPORT_SYMBOL(lidbg_domineering_ack);
EXPORT_SYMBOL(mod_cmn_main);
EXPORT_SYMBOL(lidbg_get_ns_count);
EXPORT_SYMBOL(get_tick_count);
EXPORT_SYMBOL(lidbg_launch_user);
EXPORT_SYMBOL(lidbg_readwrite_file);
EXPORT_SYMBOL(lidbg_task_kill_select);
EXPORT_SYMBOL(lidbg_get_current_time);
EXPORT_SYMBOL(set_power_state);
EXPORT_SYMBOL(get_bin_path);

