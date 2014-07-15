/* Copyright (c) 2012, swlee
 *
 */
// http://blog.csdn.net/luoshengyang/article/details/6568411
#include "lidbg.h"

#define LIDBG_SIZE (MEM_SIZE_4_KB)
struct lidbg_dev_smem
{
    unsigned long smemaddr;
    unsigned long smemsize;
    unsigned long valid_offset;
};


/*lidbg设备结构体*/
struct lidbg_dev
{
    struct cdev cdev; /*cdev结构体*/
    unsigned char mem[LIDBG_SIZE]; /*全局内存*/
    union
    {
        unsigned char lidbg_smem[LIDBG_SIZE / 4]; // 1k
        struct lidbg_dev_smem s;

    } smem;
};

struct lidbg_dev *global_lidbg_devp = NULL;

#define MEM_CLEAR 0x1  /*清0全局内存*/
#define GET_GLOBAL 0x2

#define LIDBG_MAJOR 167    /*预设的lidbg的主设备号*/
#define LIDBG_MINOR 0    /*预设的lidbg的主设备号*/

struct cdev *my_cdev;
struct class *my_class;

static int lidbg_major = LIDBG_MAJOR;

static int  debug_mask = 0;
module_param_named(debug_mask, debug_mask, int, 0644 );

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36)
DECLARE_MUTEX(lidbg_lock);
#else
DEFINE_SEMAPHORE(lidbg_lock);
#endif


/*文件打开函数*/
int lidbg_open(struct inode *inode, struct file *filp)
{

    if(debug_mask) DUMP_FUN;
#if 0

    /*将设备结构体指针赋值给文件私有数据指针*/
    filp->private_data = (struct lidbg_dev *)global_lidbg_devp;
    if (filp->f_flags & O_NONBLOCK)
    {
        if (down_trylock(&lidbg_lock))

            return -EBUSY;
    }
    else
    {
        down(&lidbg_lock);
    }
#endif
    return 0;
}


/*文件释放函数*/
int lidbg_release(struct inode *inode, struct file *filp)
{
    if(debug_mask) DUMP_FUN;
#if 0
    up(&lidbg_lock);
#endif
    return 0;
}


#if 0
/* ioctl设备控制函数 */
static int lidbg_ioctl(struct inode *inodep, struct file *filp, unsigned
                       int cmd, unsigned long arg)
{
    struct lidbg_dev *dev = filp->private_data;/*获得设备结构体指针*/

    switch (cmd)
    {
    case MEM_CLEAR:
        memset(dev->mem, 0, LIDBG_SIZE);
        lidbg(KERN_INFO "lidbg is set to zero\n");
        break;
    case GET_GLOBAL:
        break;
    default:
        return  - EINVAL;
    }
    return 0;
}
#endif


/*读函数*/
static ssize_t lidbg_read(struct file *filp, char __user *buf, size_t size,
                          loff_t *ppos)
{
    unsigned int count = size;
    int ret = 0, read_value = 0;
    struct lidbg_dev *dev = filp->private_data; /*获得设备结构体指针*/

    /*内核空间->用户空间*/
    memcpy(&read_value, (void *)(dev->mem), 4);
    lidbg("lidbg_read:read_value=%x,read_count=%d\n", (u32)read_value, count);
    if (copy_to_user(buf, (void *)(dev->mem), count))
    {
        ret =  - EFAULT;
    }
    else
    {
        ret = count;
    }

    return count;
}

#include "lidbg_cmd.c"

/*写函数 主要操作在这里 入口*/
static ssize_t lidbg_write(struct file *filp, const char __user *buf,
                           size_t size, loff_t *ppos)
{
#if 0
    struct lidbg_dev *dev = filp->private_data; /*获得设备结构体指针*/
    memset(dev->mem, '\0', LIDBG_SIZE);

    /*用户空间->内核空间*/
    if(copy_from_user(dev->mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    parse_cmd(dev->mem);
#else
#if 1
    char tmp[64];
    char *mem = tmp;
    bool is_alloc = 0;
    if(size >= 64)
    {
        mem = (char *)kmalloc(size + 1, GFP_KERNEL); //size+1 for '\0'
        if (mem == NULL)
        {
            lidbg("lidbg_write kmalloc err\n");
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

    /*用户空间->内核空间*/
    if(copy_from_user(mem, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }

    //lidbg("size:%d,%s\n",size,mem);
    parse_cmd(mem);
    if(is_alloc)
        kfree(mem);
#endif

    return size;//若不为size则重复执行
}

/* seek文件定位函数 */
static loff_t lidbg_llseek(struct file *filp, loff_t offset, int orig)
{
    loff_t ret = 0;
    switch (orig)
    {
    case 0:   /*相对文件开始位置偏移*/
        if (offset < 0)
        {
            ret =  - EINVAL;
            break;
        }
        if ((unsigned int)offset > LIDBG_SIZE)
        {
            ret =  - EINVAL;
            break;
        }
        filp->f_pos = (unsigned int)offset;
        ret = filp->f_pos;
        break;
    case 1:   /*相对文件当前位置偏移*/
        if ((filp->f_pos + offset) > LIDBG_SIZE)
        {
            ret =  - EINVAL;
            break;
        }
        if ((filp->f_pos + offset) < 0)
        {
            ret =  - EINVAL;
            break;
        }
        filp->f_pos += offset;
        ret = filp->f_pos;
        break;
    default:
        ret =  - EINVAL;
        break;
    }
    return ret;
}

/*文件操作结构体*/
static const struct file_operations lidbg_fops =
{
    .owner = THIS_MODULE,
    .llseek = lidbg_llseek,
    .read = lidbg_read,
    .write = lidbg_write,
#if 0
    .ioctl = lidbg_ioctl,
#endif
    .open = lidbg_open,
    .release = lidbg_release,
};

/*初始化并注册cdev*/
static void lidbg_setup_cdev(struct lidbg_dev *dev, int index)
{
    int err, devno = MKDEV(lidbg_major, index);

    cdev_init(&dev->cdev, &lidbg_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &lidbg_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        lidbg(KERN_NOTICE "Error %d adding LED%d", err, index);
}


void lidbg_create_proc(void);
void lidbg_remove_proc(void);


/*设备驱动模块加载函数*/
int lidbg_init(void)
{
    int result;
    dev_t devno = MKDEV(lidbg_major, 0);

    /* 申请设备号*/
    if (lidbg_major)
        result = register_chrdev_region(devno, 1, "lidbg");
    else  /* 动态申请设备号 */
    {
        result = alloc_chrdev_region(&devno, 0, 1, "lidbg");
        lidbg_major = MAJOR(devno);
    }
    if (result < 0)
        return result;
    /* 动态申请设备结构体的内存*/
    global_lidbg_devp = kmalloc(sizeof(struct lidbg_dev), GFP_KERNEL);

    if (!global_lidbg_devp)    /*申请失败*/
    {
        result =  - ENOMEM;
        goto fail_malloc;
    }
    memset(global_lidbg_devp, 0, sizeof(struct lidbg_dev));

    lidbg_setup_cdev((struct lidbg_dev *)global_lidbg_devp, 0);


    lidbg("creating mlidbg class.\n");

    //自动在/dev下创建my_device设备文件
    //在驱动初始化的代码里调用class_create为该设备创建一个class，
    /* creating your own class */
    my_class = class_create(THIS_MODULE, "mlidbg");
    if(IS_ERR(my_class))
    {
        lidbg("Err: failed in creating mlidbg class.\n");
        return -1;
    }

    //再为每个设备调用 class_device_create创建对应的设备
    /* register your own device in sysfs, and this will cause udevd to create corresponding device node */
    // class_device_create(my_class, NULL, MKDEV(LIDBG_MAJOR, LIDBG_MINOR), NULL, "mlidbg" "%d", LIDBG_MINOR );
    device_create(my_class, NULL, MKDEV(LIDBG_MAJOR, LIDBG_MINOR), NULL, "mlidbg" "%d", LIDBG_MINOR );

    lidbg_create_proc();

    lidbg_chmod("/dev/mlidbg0");
    LIDBG_MODULE_LOG;

    return 0;

fail_malloc:
    unregister_chrdev_region(devno, 1);
    return result;
}

/*模块卸载函数*/
void lidbg_exit(void)
{
    lidbg("mlidbg_exit\n");

    cdev_del(&((struct lidbg_dev *)global_lidbg_devp)->cdev);   /*注销cdev*/
    kfree(((struct lidbg_dev *)global_lidbg_devp));     /*释放设备结构体内存*/

    // class_device_destroy(my_class, MKDEV(LIDBG_MAJOR, LIDBG_MINOR));
    device_destroy(my_class, MKDEV(LIDBG_MAJOR, LIDBG_MINOR));

    class_destroy(my_class);

    lidbg_remove_proc();

    unregister_chrdev_region(MKDEV(lidbg_major, 0), 1); /*释放设备号*/
}


#define LIDBG_DEVICE_PROC_NAME  "lidbg"
ssize_t lidbg_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len;

    char *hello_str = "Hello, world!\n";
    lidbg("lidbg_proc_read\n");
    len = strlen(hello_str);
    strcpy(page, hello_str);
    /*     * Signal EOF.     */
    *eof = 1;
    return len;

}

ssize_t lidbg_proc_write(struct file *filp, const char __user *buff, unsigned long len, void *data)
{
    lidbg("lidbg_proc_write\n");

    filp->private_data = (struct lidbg_dev *)global_lidbg_devp;
    lidbg_write(filp, buff, len, 0);
    return len;
}

void lidbg_create_proc(void)
{
    struct proc_dir_entry *entry;

    entry = create_proc_entry(LIDBG_DEVICE_PROC_NAME, 0, NULL);
    if(entry)
    {
        entry->read_proc = lidbg_proc_read;
        entry->write_proc = lidbg_proc_write;
    }
}

void lidbg_remove_proc(void)
{
    remove_proc_entry(LIDBG_DEVICE_PROC_NAME, NULL);
}

MODULE_AUTHOR("Lsw");
MODULE_LICENSE("GPL");


module_param(lidbg_major, int, S_IRUGO);

module_init(lidbg_init);
module_exit(lidbg_exit);
