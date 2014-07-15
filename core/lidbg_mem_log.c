#include "lidbg.h"

#define BUFF_SIZE (4 * 1024 )
#define DEVICE_NAME "lidbg_mem_log"
#define LIDBG_FIFO_SIZE (4 * 1024 * 1024)
#define MEM_LOG_EN


struct lidbg_fifo_device *glidbg_msg_fifo;

static int fifo_to_file(char *file2amend, char *str_append)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    int  flags = 0;
    unsigned int file_len;

    if(str_append == NULL)
    {
        printk(KERN_CRIT"[futengfei]err.fileappend_mode:<str_append=null>\n");
        return -1;
    }
    flags = O_CREAT | O_RDWR | O_APPEND;

    filep = filp_open(file2amend, flags , 0777);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"[futengfei]err.open:<%s>\n", file2amend);
        return -1;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    file_len = file_len + 1;

    filep->f_op->llseek(filep, 0, SEEK_END);//note:to the end to append;

    filep->f_op->write(filep, str_append, strlen(str_append), &filep->f_pos);
    set_fs(old_fs);
    filp_close(filep, 0);
    return 1;
}

static int lidbg_get_curr_time(char *time_string, struct rtc_time *ptm)
{
    int  tlen = -1;
    struct timespec ts;
    struct rtc_time tm;
    getnstimeofday(&ts);
    rtc_time_to_tm(ts.tv_sec, &tm);
    if(time_string)
        tlen = sprintf(time_string, "%d-%02d-%02d_%02d:%02d:%02d ",
                       tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour , tm.tm_min, tm.tm_sec);
    if(ptm)
        *ptm = tm;
    return tlen;
}

static void fifo_is_enough(struct lidbg_fifo_device *dev, int len)
{
    if(kfifo_is_full(&dev->fifo) || (kfifo_avail(&dev->fifo) < len))
    {
        int ret = 0;
        char msg_clean_buff[len];
        ret = kfifo_out(&dev->fifo, msg_clean_buff, len);
    }
}

int lidbg_fifo_put( struct lidbg_fifo_device *dev, const char *fmt, ... )
{
    int ret = 0;
    if(dev && dev->is_inited)
    {
        int len;
        va_list args;

        down(&dev->sem);

        lidbg_get_curr_time(dev->msg_in_buff, NULL);
        len = strlen(dev->msg_in_buff);
        fifo_is_enough(dev, len);

        ret = kfifo_in(&dev->fifo, dev->msg_in_buff, len);

        va_start ( args, fmt );
        ret = vsprintf (dev->msg_in_buff, (const char *)fmt, args );
        va_end ( args );

        dev->msg_in_buff[BUFF_SIZE - 1] = '\0';
        len = strlen(dev->msg_in_buff);
        fifo_is_enough(dev, len);
        ret = kfifo_in(&dev->fifo, dev->msg_in_buff, len);

        up(&dev->sem);

        ret = 1;
    }
    else
        LIDBG_ERR("dev->is_inited(%s)\n", dev ? dev->owner : "null");
    return ret;
}
EXPORT_SYMBOL(lidbg_fifo_put);

int lidbg_fifo_get(struct lidbg_fifo_device *dev, char *to_file, int out_mode)
{
    int len = 0;
    unsigned int ret = 1;
    char *msg_out_buff = NULL;

    if(dev && dev->is_inited)
    {
        down(&dev->sem);
        len = kfifo_len(&dev->fifo);
        up(&dev->sem);

        LIDBG_WARN("%s:kfifo_len=%d\n", dev->owner, len);

        msg_out_buff = kmalloc(len, GFP_KERNEL);
        if (msg_out_buff == NULL)
        {
            ret = -1;
            LIDBG_ERR("kmalloc\n");
            return ret;
        }

        memset(msg_out_buff, '\0', sizeof(msg_out_buff));
        down(&dev->sem);
        ret = kfifo_out(&dev->fifo, msg_out_buff, len);
        up(&dev->sem);

        ret = fifo_to_file(to_file, msg_out_buff);
        if(ret != 1)
        {
            LIDBG_ERR("out msg to file.\n");
            return -1;
        }

        kfree(msg_out_buff);
    }
    else
        LIDBG_ERR("dev->is_inited(%s)\n", dev ? dev->owner : "null");
    return ret;
}
EXPORT_SYMBOL(lidbg_fifo_get);

struct lidbg_fifo_device *lidbg_fifo_alloc(char *owner, int fifo_size, int buff_size)
{
    struct lidbg_fifo_device *dev;
    if(!owner)
    {
        LIDBG_ERR("ower\n");
        return NULL;
    }

    dev = (struct lidbg_fifo_device *)kmalloc( sizeof(struct lidbg_fifo_device), GFP_KERNEL);
    if (dev == NULL)
    {
        LIDBG_ERR("kmalloc.lidbg_fifo_device(%s)\n", owner);
        return NULL;
    }
    dev->is_inited = false;
    dev->owner = NULL;
    dev->msg_in_buff = NULL;

    if(kfifo_alloc(&dev->fifo, fifo_size, GFP_KERNEL))
    {
        LIDBG_ERR("kfifo_alloc(%s)\n", owner);
        kfree(dev);
        return NULL;
    }
    sema_init(&dev->sem, 1);

    dev->msg_in_buff = kmalloc(buff_size, GFP_KERNEL);
    if (dev->msg_in_buff == NULL)
    {
        kfifo_free(&dev->fifo);
        kfree(dev);
        LIDBG_ERR("kmalloc.msg_in_buff(%s)\n", owner);
        return NULL;
    }

    down(&dev->sem);
    dev->is_inited = true;
    dev->owner = owner;
    memset(dev->msg_in_buff, '\0', sizeof(dev->msg_in_buff));
    up(&dev->sem);

    LIDBG_SUC("new:<%s,fifo.%d,buff.%d>\n", dev->owner, fifo_size, buff_size);

    return dev;
}
EXPORT_SYMBOL(lidbg_fifo_alloc);

static int lidbg_msg_open (struct inode *inode, struct file *filp)
{
    filp->private_data = glidbg_msg_fifo;
    DUMP_FUN;
    return 0;
}

static ssize_t lidbg_msg_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    DUMP_FUN;
    return 0;
}

static ssize_t lidbg_msg_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;

    if((count > 0) && glidbg_msg_fifo->is_inited)
    {
        int len;

        down(&glidbg_msg_fifo->sem);

        lidbg_get_curr_time(glidbg_msg_fifo->msg_in_buff, NULL);
        len = strlen(glidbg_msg_fifo->msg_in_buff);
        fifo_is_enough(glidbg_msg_fifo, len);
        ret = kfifo_in(&glidbg_msg_fifo->fifo, glidbg_msg_fifo->msg_in_buff, len);

        if (copy_from_user( glidbg_msg_fifo->msg_in_buff, buf, count))
            LIDBG_ERR("copy_from_user\n");
        glidbg_msg_fifo->msg_in_buff[BUFF_SIZE - 1] = '\0';

        len = strlen(glidbg_msg_fifo->msg_in_buff);
        fifo_is_enough(glidbg_msg_fifo, len);
        ret = kfifo_in(&glidbg_msg_fifo->fifo, glidbg_msg_fifo->msg_in_buff, len);

        up(&glidbg_msg_fifo->sem);

        ret = 1;
    }
    else
        ret = 0;

    return ret;
}

int lidbg_msg_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static  struct file_operations lidbg_msg_fops =
{
    .owner = THIS_MODULE,
    .read = lidbg_msg_read,
    .write = lidbg_msg_write,
    .open = lidbg_msg_open,
    .release = lidbg_msg_release,
};

static struct cdev cdev;
static int  lidbg_msg_probe(struct platform_device *pdev)
{
    int ret, err;
    int major_number = 0;
    dev_t dev_number;
    static struct class *class_install;

#ifndef MEM_LOG_EN
    return 0;
#endif

    glidbg_msg_fifo = lidbg_fifo_alloc("lidbg_commen_owner", LIDBG_FIFO_SIZE, BUFF_SIZE);

    dev_number = MKDEV(major_number, 0);
    if(major_number)
    {
        ret = register_chrdev_region(dev_number, 1, DEVICE_NAME);
    }
    else
    {
        ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
        major_number = MAJOR(dev_number);
    }

    cdev_init(&cdev, &lidbg_msg_fops);
    cdev.owner = THIS_MODULE;
    cdev.ops = &lidbg_msg_fops;

    err = cdev_add(&cdev, dev_number, 1);
    if (err)
        LIDBG_ERR( "cdev_add\n");

    class_install = class_create(THIS_MODULE, "trace_msg");
    if(IS_ERR(class_install))
    {
        LIDBG_ERR( "class_create\n");
        return -1;
    }
    device_create(class_install, NULL, dev_number, NULL, "%s%d", DEVICE_NAME, 0);

    return 0;
}


static int  lidbg_msg_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver lidbg_mem_log_driver =
{
    .probe		= lidbg_msg_probe,
    .remove     = lidbg_msg_remove,
    .driver         = {
        .name = "lidbg_mem_log",
        .owner = THIS_MODULE,
    },
};

static struct platform_device lidbg_mem_log_device =
{
    .name               = "lidbg_mem_log",
    .id                 = -1,
};


static  int lidbg_msg_init(void)
{
    
    LIDBG_MODULE_LOG;
    platform_device_register(&lidbg_mem_log_device);

    return platform_driver_register(&lidbg_mem_log_driver);

}

static void lidbg_msg_exit(void)
{
    DUMP_FUN;

    platform_device_unregister(&lidbg_mem_log_device);
    platform_driver_unregister(&lidbg_mem_log_driver);
}


void mem_log_main(int argc, char **argv)
{
    if(!strcmp(argv[0], "dump"))
    {
        lidbg("dump mem log\n");
        lidbg_fifo_get(glidbg_msg_fifo, LIDBG_LOG_DIR"lidbg_mem_log.txt", 0);
    }
}
EXPORT_SYMBOL(mem_log_main);
EXPORT_SYMBOL(glidbg_msg_fifo);

module_init(lidbg_msg_init);
module_exit(lidbg_msg_exit);

MODULE_LICENSE("GPL");



