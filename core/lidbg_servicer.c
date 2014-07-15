
#include "lidbg.h"
#define DEVICE_NAME "lidbg_servicer"

#define FIFO_SIZE (64)
struct kfifo k2u_fifo;
struct kfifo u2k_fifo;

int k2u_fifo_buffer[FIFO_SIZE];
int u2k_fifo_buffer[FIFO_SIZE];

static spinlock_t fifo_k2u_lock;

unsigned long flags_k2u;
unsigned long flags_u2k;

//http://blog.csdn.net/yjzl1911/article/details/5654893
static struct fasync_struct *fasync_queue;

static DECLARE_WAIT_QUEUE_HEAD(k2u_wait);
static DECLARE_COMPLETION(u2k_com);

int thread_u2k(void *data)
{
    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop()) break;

        wait_for_completion(&u2k_com);
        lidbg ("wait_for_completion ok\n");
    }
    return 0;
}

static int servicer_fasync(int fd, struct file *filp, int on)
{
    int retval;
    retval = fasync_helper(fd, filp, on, &fasync_queue);
    if(retval < 0)
        return retval;
    return 0;
}


void k2u_write(int cmd)
{

    if(cmd != SERVICER_DONOTHING)
    {
        lidbg ("k2u_write=%d\n", cmd);

        spin_lock_irqsave(&fifo_k2u_lock, flags_k2u);
        kfifo_in(&k2u_fifo, &cmd, sizeof(int));
        spin_unlock_irqrestore(&fifo_k2u_lock, flags_k2u);

        wake_up(&k2u_wait);

        if (fasync_queue)
            kill_fasync(&fasync_queue, SIGIO, POLL_IN);
    }
}

// read from kernel to user
int k2u_read(void)
{
    int ret, count;
    spin_lock_irqsave(&fifo_k2u_lock, flags_k2u);
    if(kfifo_is_empty(&k2u_fifo))
    {
        ret = SERVICER_DONOTHING;
    }
    else
    {
        count = kfifo_out(&k2u_fifo, &ret, sizeof(int));
    }
    spin_unlock_irqrestore(&fifo_k2u_lock, flags_k2u);
    return ret;
}

// read from  user to  kernel

int u2k_read(void)
{
    int ret, count;
    spin_lock_irqsave(&fifo_k2u_lock, flags_k2u);
    count = kfifo_out(&u2k_fifo, &ret, sizeof(int));
    spin_unlock_irqrestore(&fifo_k2u_lock, flags_k2u);
    return ret;
}


ssize_t  servicer_read(struct file *filp, char __user *buffer, size_t size, loff_t *offset)
{
    int cmd ;
    cmd = k2u_read();
    lidbg("servicer_read:cmd = %d\n", cmd);
    if (copy_to_user(buffer, &cmd, 4))
    {
        lidbg("copy_to_user ERR\n");
    }
    return size;
}


//u2k_write
ssize_t  servicer_write(struct file *filp, const char __user *buffer, size_t size, loff_t *offset)
{
    int cmd ;

    if (copy_from_user( &cmd, buffer, 4))
    {
        lidbg("copy_from_user ERR\n");
    }

    spin_lock_irqsave(&fifo_k2u_lock, flags_k2u);
    kfifo_in(&u2k_fifo, &cmd, sizeof(int));
    spin_unlock_irqrestore(&fifo_k2u_lock, flags_k2u);

    complete(&u2k_com);

    return size;
}


int servicer_open(struct inode *inode, struct file *filp)
{
    lidbg ("servicer_open\n");
    return 0;
}


void lidbg_servicer_main(int argc, char **argv)
{
    u32 cmd = 0;
    if(!strcmp(argv[0], "dmesg"))
    {
        //cmd = LOG_DMESG ;
    }
    else if(!strcmp(argv[0], "logcat"))
    {
        //cmd = LOG_LOGCAT ;
    }

    k2u_write(cmd);

}

static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   = servicer_open,
    .read   =   servicer_read,
    .write  =  servicer_write,
    .fasync = servicer_fasync,
};

static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,

};


static int __init servicer_init(void)
{
    int ret;

    //下面的代码可以自动生成设备节点，但是该节点在/dev目录下，而不在/dev/misc目录下
    //其实misc_register就是用主设备号10调用register_chrdev()的 misc设备其实也就是特殊的字符设备。
    //注册驱动程序时采用misc_register函数注册，此函数中会自动创建设备节点，即设备文件。无需mknod指令创建设备文件。因为misc_register()会调用class_device_create()或者device_create()。


    ret = misc_register(&misc);
    lidbg (DEVICE_NAME"servicer_init\n");
    //DECLARE_KFIFO(cmd_fifo);
    //INIT_KFIFO(cmd_fifo);
    lidbg ("kfifo_init,FIFO_SIZE=%d\n", FIFO_SIZE);
    kfifo_init(&k2u_fifo, k2u_fifo_buffer, FIFO_SIZE);
    kfifo_init(&u2k_fifo, u2k_fifo_buffer, FIFO_SIZE);
    spin_lock_init(&fifo_k2u_lock);

    CREATE_KTHREAD(thread_u2k, NULL);

    lidbg_chmod("/dev/lidbg_servicer");
	
    LIDBG_MODULE_LOG;
    return ret;
}

static void __exit servicer_exit(void)
{
    misc_deregister(&misc);
    lidbg (DEVICE_NAME"servicer  dev_exit\n");
}

module_init(servicer_init);
module_exit(servicer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");


EXPORT_SYMBOL(lidbg_servicer_main);
EXPORT_SYMBOL(k2u_write);
EXPORT_SYMBOL(k2u_read);
EXPORT_SYMBOL(u2k_read);

