
#include "lidbg.h"


//zone below [fs.log.tools]
int max_file_len = 1;
int g_iskmsg_ready = 1;
int g_pollkmsg_en = 0;
int g_pollkmsg_en_fclear = 0;
static struct task_struct *fs_kmsgtask;
static struct completion kmsg_wait;
//zone end

//zone below [fs.log.driver]
int delay_disable_tracemsg(void *data);
bool upload_machine_log(void)
{
    if(lidbg_exe("/flysystem/lib/out/client_mobile", NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        lidbg_exe("/system/lib/modules/out/client_mobile", NULL, NULL, NULL, NULL, NULL, NULL);
    return true;
}
void remount_system(void)
{
    static int g_is_remountd_system = 0;
    if(!g_is_remountd_system)
    {
        g_is_remountd_system = 1;
        lidbg_chmod("/system/bin/mount");
        lidbg_mount("/system");
        lidbg_mount("/flysystem");
        msleep(300);
        lidbg_chmod("/system");
        lidbg_chmod("/flysystem");
        lidbg_chmod("/system/bin");
        lidbg_chmod("/flysystem/bin");
        lidbg_chmod("/system/lib/hw");
        lidbg_chmod("/flysystem/lib/hw");
        lidbg_chmod("/flysystem/lib/out");
        msleep(200);
    }
}
void chmod_for_apk(void)
{
    remount_system();
    lidbg_chmod("/system/bin/mount");
    lidbg_mount("/system");
    lidbg_chmod("/data");
    lidbg_chmod("/system/app");
}
void call_apk(void)
{
    chmod_for_apk();
    analysis_copylist("/flysystem/lib/out/copylist_app.conf");
}
int bfs_file_amend(char *file2amend, char *str_append, int file_limit_M)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    int  flags, is_file_cleard = 0, file_limit = max_file_len;
    unsigned int file_len;

    if(str_append == NULL)
    {
        lidbg("[futengfei]err.fileappend_mode:<str_append=null>\n");
        return -1;
    }
    flags = O_CREAT | O_RDWR | O_APPEND;

    if(file_limit_M > max_file_len)
        file_limit = file_limit_M;

again:
    filep = filp_open(file2amend, flags , 0777);
    if(IS_ERR(filep))
    {
        lidbg("[futengfei]err.open:<%s>\n", file2amend);
        return -1;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    file_len = file_len + 1;


    if(file_len > file_limit * MEM_SIZE_1_MB)
    {
        lidbg("[futengfei]warn.fileappend_mode:< file>8M.goto.again >\n");
        is_file_cleard = 1;
        flags = O_CREAT | O_RDWR | O_APPEND | O_TRUNC;
        set_fs(old_fs);
        filp_close(filep, 0);
        goto again;
    }
    filep->f_op->llseek(filep, 0, SEEK_END);//note:to the end to append;
    if(1 == is_file_cleard)
    {
        char *str_warn = "============have_cleard=============\n\n";
        is_file_cleard = 0;
        filep->f_op->write(filep, str_warn, strlen(str_warn), &filep->f_pos);
    }
    filep->f_op->write(filep, str_append, strlen(str_append), &filep->f_pos);
    set_fs(old_fs);
    filp_close(filep, 0);
    return 1;
}


void file_separator(char *file2separator)
{
    char buf[32];
    lidbg_get_current_time(buf, NULL);
    fs_string2file(0, file2separator, "------%s------\n", buf);
}
int dump_kmsg(char *node, char *save_msg_file, int size, int *always)
{
    struct file *filep;
    mm_segment_t old_fs;

    int  ret = -1;
    int  kmsg_file_limit = 3;
    CREATE_KTHREAD(delay_disable_tracemsg, NULL);
    if(!size && !always)
    {
        FS_ERR("<size_k=null&&always=null>\n");
        return -1;
    }
    filep = filp_open(node,  O_RDONLY , 0);
    if(!IS_ERR(filep))
    {
        old_fs = get_fs();
        set_fs(get_ds());

        if(size)
        {
            char *psize = NULL;
            psize = (unsigned char *)kmalloc(size, GFP_KERNEL);
            if(psize == NULL)
            {
                FS_ERR("<cannot kmalloc memory!>\n");
                return ret;
            }

            ret = filep->f_op->read(filep, psize, size - 1, &filep->f_pos);
            if(ret > 0)
            {
                psize[ret] = '\0';
                bfs_file_amend(save_msg_file, psize, kmsg_file_limit);
            }
            kfree(psize);
        }
        else
        {
            char buff[512];
            memset(buff, 0, sizeof(buff));
            while( (!always ? 0 : *always) )
            {
                ret = filep->f_op->read(filep, buff, 512 - 1, &filep->f_pos);
                if(ret > 0)
                {
                    buff[ret] = '\0';
                    bfs_file_amend(save_msg_file, buff, kmsg_file_limit);
                }
            }
        }

        set_fs(old_fs);
        filp_close(filep, 0);
    }
    return ret;
}
static int thread_pollkmsg_func(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    while(!kthread_should_stop())
    {
        if( !wait_for_completion_interruptible(&kmsg_wait))
            dump_kmsg(KMSG_NODE, LIDBG_KMSG_FILE_PATH, 0, &g_pollkmsg_en);
    }
    return 1;
}
void cb_kv_pollkmsg(char *key, char *value)
{
    FS_WARN("<%s=%s>\n", key, value);
    if (strcmp(value, "0" ))
        complete(&kmsg_wait);
}

static int fs_kmsg_reboot_notifier_func(struct notifier_block *nb, unsigned long event, void *unused)
{
    switch (event)
    {
    case SYS_RESTART:
        if(g_pollkmsg_en_fclear)
        {
            lidbg_rm(LIDBG_KMSG_FILE_PATH);
            FS_ALWAYS("<rm %s>\n", LIDBG_KMSG_FILE_PATH);
            ssleep(1);
        }
        break;
    case SYS_HALT:
        break;
    case SYS_POWER_OFF:
        break;
    default:
        break;
    }
    return NOTIFY_DONE;
}
static struct notifier_block fs_kmsg_reboot_notifier =
{
    .notifier_call  = fs_kmsg_reboot_notifier_func,
};
//zone end


//zone below [fs.log.interface]
void fs_file_separator(char *file2separator)
{
    file_separator(file2separator);
}
int fs_dump_kmsg(char *tag, int size )
{
    file_separator(LIDBG_KMSG_FILE_PATH);
    if(tag != NULL)
        fs_string2file(3, LIDBG_KMSG_FILE_PATH, "fs_dump_kmsg: %s\n", tag);
    return dump_kmsg(KMSG_NODE, LIDBG_KMSG_FILE_PATH, size, NULL);
}
void fs_enable_kmsg( bool enable )
{
    if(enable)
    {
        g_pollkmsg_en = 1;
        if(g_iskmsg_ready)
            complete(&kmsg_wait);
    }
    else
        g_pollkmsg_en = 0;
}
int fs_string2file(int file_limit_M, char *filename, const char *fmt, ... )
{
    va_list args;
    int n;
    char str_append[768];
    va_start ( args, fmt );
    n = vsprintf ( str_append, (const char *)fmt, args );
    va_end ( args );

    return bfs_file_amend(filename, str_append, file_limit_M);
}
int fs_mem_log( const char *fmt, ... )
{
    int len;
    va_list args;
    int n;
    char str_append[256];
    va_start ( args, fmt );
    n = vsprintf ( str_append, (const char *)fmt, args );
    va_end ( args );

    len = strlen(str_append);

    bfs_file_amend(LIDBG_MEM_LOG_FILE, str_append, 0);

    return 1;
}
bool fs_upload_machine_log(void)
{
    lidbg_rm(LIDBG_MEM_DIR"mobile.txt");
    remount_system();
    return upload_machine_log();
}
int thread_call_apk_init(void *data)
{
    call_apk();
    return 0;
}
void fs_call_apk(void)
{
    CREATE_KTHREAD(thread_call_apk_init, NULL);
}
void fs_remove_apk(void)
{
    lidbg_rm("/system/app/fileserver.apk");
    lidbg_rm("/system/app/aw_dbg.apk");
}
void fs_remount_system(void)
{
    remount_system();
}
//zone end


int delay_disable_tracemsg(void *data)
{
    {
        lidbg("delay 3 s to disable  lidbg_trace_msg\n");
        msleep(3000);
        lidbg_readwrite_file("/dev/mlidbg0", NULL, "c lidbg_trace_msg disable", sizeof("c lidbg_trace_msg disable") - 1);
    }  //disable lidbg_trace_msg
    return 1;
}
void lidbg_fs_log_init(void)
{

    init_completion(&kmsg_wait);

    FS_REGISTER_INT(g_pollkmsg_en, "fs_kmsg_en", 0, cb_kv_pollkmsg);
    FS_REGISTER_INT(g_pollkmsg_en_fclear, "fs_kmsg_en_fclear", 0, cb_kv_pollkmsg);

    if(g_pollkmsg_en_fclear)
    {
        g_pollkmsg_en = 1;
        register_reboot_notifier(&fs_kmsg_reboot_notifier);
    }

    if(g_pollkmsg_en == 1)
        complete(&kmsg_wait);


    FS_REGISTER_INT(max_file_len, "fs_max_file_len", 1, NULL);

    fs_kmsgtask = kthread_run(thread_pollkmsg_func, NULL, "ftf_kmsgtask");
}

EXPORT_SYMBOL(fs_file_separator);
EXPORT_SYMBOL(fs_dump_kmsg);
EXPORT_SYMBOL(fs_enable_kmsg);
EXPORT_SYMBOL(fs_string2file);
EXPORT_SYMBOL(fs_mem_log);
EXPORT_SYMBOL(fs_upload_machine_log);
EXPORT_SYMBOL(fs_call_apk);
EXPORT_SYMBOL(fs_remove_apk);
EXPORT_SYMBOL(fs_remount_system);
EXPORT_SYMBOL(bfs_file_amend);


