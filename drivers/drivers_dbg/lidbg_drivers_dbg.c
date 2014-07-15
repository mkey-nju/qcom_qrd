
#include "lidbg.h"

void lidbg_video_main(int argc, char **argv);

LIDBG_DEFINE;

int drivers_dbg_open (struct inode *inode, struct file *filp)
{
    return 0;
}

#include "lidbg_drivers_cmd.c"

ssize_t drivers_dbg_write (struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    if(cmd_buf[size - 1] == '\n')
        cmd_buf[size - 1] = '\0';
    parse_cmd(cmd_buf);
    return size;

}

static  struct file_operations drivers_dbg_nod_fops =
{
    .owner = THIS_MODULE,
    .write = drivers_dbg_write,
    .open = drivers_dbg_open,
};
int thread_drivers_dbg_init(void *data)
{
    lidbg_new_cdev(&drivers_dbg_nod_fops, "lidbg_drivers_dbg");
    return 0;
}

int lidbg_drivers_dbg_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;

    CREATE_KTHREAD(thread_drivers_dbg_init, NULL);

    return 0;

}
void lidbg_drivers_dbg_deinit(void)
{
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

module_init(lidbg_drivers_dbg_init);
module_exit(lidbg_drivers_dbg_deinit);


