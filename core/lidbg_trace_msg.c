
#include "lidbg.h"

#define DEVICE_NAME "lidbg_trace_msg"
#define LIDBG_TRACE_MSG_FIFO_SIZE		(32  * 1024)
#define TRACE_MSG_FROM_KMSG 1

static LIST_HEAD(lidbg_trace_msg_string_list);
static LIST_HEAD(lidbg_trace_msg_cb_list_head);

struct lidbg_trace_message_device
{
    char *name;
    int disable_flag;
    struct kfifo fifo;
    struct semaphore sem;
};

struct lidbg_trace_msg_cb_list
{
    struct list_head tmp_list;
    char *key_word;
    void *data;
    void (*cb_func)(char *key_word, void *data);
};

static struct lidbg_trace_message_device *pdev;

void lidbg_trace_msg_disable(int flag)
{
    pdev->disable_flag = flag;
}
EXPORT_SYMBOL(lidbg_trace_msg_disable);

bool lidbg_trace_msg_cb_register(char *key_word, void *data, void (*cb_func)(char *key_word, void *data))
{
    struct lidbg_trace_msg_cb_list *cb_list;

    cb_list = kzalloc(sizeof(struct lidbg_trace_msg_cb_list), GFP_KERNEL);
    if(cb_list)
        lidbg("[%s]: Kzalloc mem for lidbg_trace_msg_cb_list faild !\n", __func__);

    if(key_word && cb_func)
    {
        cb_list->key_word = key_word;
        cb_list->data = data;
        cb_list->cb_func = cb_func;
        list_add(&(cb_list->tmp_list), &lidbg_trace_msg_cb_list_head);		//attention: some protection on lidbg_trace_msg_cb_list_head will be better
        return true;
    }
    else
        return false;
}
EXPORT_SYMBOL(lidbg_trace_msg_cb_register);

static void lidbg_trace_key_word(char *string)
{
    char *ret = NULL;
    struct string_dev *pmsg;
    struct lidbg_trace_msg_cb_list *cb_list;

    //down(&pdev->sem);
    list_for_each_entry(pmsg, &lidbg_trace_msg_string_list, tmp_list)
    {
        if(strlen(pmsg->yourkey))
            ret = strstr(string, pmsg->yourkey);
        if(ret)
        {
            bfs_file_amend(LIDBG_TRACE_MSG_PATH, string, 0);
            ret = NULL;
        }
    }

    list_for_each_entry(cb_list, &lidbg_trace_msg_cb_list_head, tmp_list)	//attention: some protection on lidbg_trace_msg_cb_list_head will be better
    {
        if(strlen(cb_list->key_word))
            ret = strstr(string, cb_list->key_word);
        if(ret)
        {
            cb_list->cb_func(cb_list->key_word, cb_list->data);
            ret = NULL;
        }

    }
    //up(&pdev->sem);
}

static void lidbg_trace_msg_is_enough(int len)
{
    if(kfifo_is_full(&pdev->fifo) || (kfifo_avail(&pdev->fifo) < len))
    {
        int ret;
        char msg_clean_buff[len];

        down(&pdev->sem);
        ret = kfifo_out(&pdev->fifo, msg_clean_buff, len);
        up(&pdev->sem);

    }

}

static int thread_trace_msg_in(void *data)
{
    int len;
    char buff[512];
    struct file *filep;
    mm_segment_t old_fs;

    filep = filp_open("/proc/kmsg", O_RDONLY, 0644);
    if(filep < 0)
        lidbg("Open /proc/kmsg failed !\n");

    memset(buff, '\0', sizeof(buff));

    while(1)
    {
        old_fs = get_fs();
        set_fs(get_ds());

        if(!pdev->disable_flag)
        {
            len = filep->f_op->read(filep, buff, 512, &filep->f_pos);

            lidbg_trace_msg_is_enough(len);

            down(&pdev->sem);
            kfifo_in(&pdev->fifo, buff, len);
            up(&pdev->sem);

            msleep(500);
        }
        else
            msleep(1000);

        set_fs(old_fs);
    }

    filp_close(filep, 0);
    return 0;
}

static int thread_trace_msg_out(void *data)
{
    int i = 0;
    int len;
    char buff[512];

    memset(buff, '\0', sizeof(buff));
    while(1)
    {

        if(!pdev->disable_flag)
        {
            if(kfifo_is_empty(&pdev->fifo))
            {
                msleep(100);
                continue;
            }

            down(&pdev->sem);
            len = kfifo_out(&pdev->fifo, &buff[i], 1);
            up(&pdev->sem);

            if(buff[i] == '\n' || (i >= 510))
            {
                buff[i + 1] = '\0';
                lidbg_trace_key_word(buff);
                i = 0;
                continue;
            }

            i++;
        }
        else
            msleep(1000);
    }

    return 0;

}

static int lidbg_trace_msg_open (struct inode *inode, struct file *filp)
{
    DUMP_FUN;
    filp->private_data = pdev;
    return 0;
}

static ssize_t lidbg_trace_msg_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    DUMP_FUN;
    return 0;
}

static ssize_t lidbg_trace_msg_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    char pbuff[256];
    memset(pbuff, '\0', sizeof(pbuff));

    if (copy_from_user( pbuff, buf, count))
        lidbg("Lidbg msg copy_from_user ERR\n");

    lidbg_trace_msg_is_enough(count);

    down(&pdev->sem);
    kfifo_in(&pdev->fifo, pbuff, count);
    up(&pdev->sem);

    return ret;
}

int lidbg_trace_msg_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static  struct file_operations lidbg_trace_msg_fops =
{
    .owner = THIS_MODULE,
    .read = lidbg_trace_msg_read,
    .write = lidbg_trace_msg_write,
    .open = lidbg_trace_msg_open,
    .release = lidbg_trace_msg_release,
};
void callback_disable_trace_msg(char * focus, char * uevent)
{
    lidbg_trace_msg_disable(1);
}
static int  lidbg_trace_msg_probe(struct platform_device *ppdev)
{
    int ret;

    DUMP_FUN;
    pdev = (struct lidbg_trace_message_device *)kmalloc( sizeof(struct lidbg_trace_message_device), GFP_KERNEL);
    if (pdev == NULL)
    {
        ret = -ENOMEM;
        lidbg("Kmalloc space for lidbg msg failed.\n");
        return ret;
    }
    ret = kfifo_alloc(&pdev->fifo, LIDBG_TRACE_MSG_FIFO_SIZE, GFP_KERNEL);
    if(ret)
    {
        lidbg("Alloc kfifo for lidbg dev failed.\n");
        return ret;
    }
    pdev->disable_flag = 1;

    sema_init(&pdev->sem, 1);

    if(fs_fill_list("/flysystem/lib/out/lidbg_trace_msg_string_list.conf", FS_CMD_FILE_LISTMODE, &lidbg_trace_msg_string_list) < 0)
        fs_fill_list("/system/lib/modules/out/lidbg_trace_msg_string_list.conf", FS_CMD_FILE_LISTMODE, &lidbg_trace_msg_string_list);

    fs_file_separator(LIDBG_TRACE_MSG_PATH);
    fs_register_filename_list(LIDBG_TRACE_MSG_PATH, true);
#ifndef TRACE_MSG_DISABLE
    FS_REGISTER_INT(pdev->disable_flag, "trace_msg_disable", 1, NULL);
#endif

#if  TRACE_MSG_FROM_KMSG
    CREATE_KTHREAD(thread_trace_msg_in, NULL);
#endif
    CREATE_KTHREAD(thread_trace_msg_out, NULL);

    lidbg_new_cdev(&lidbg_trace_msg_fops, DEVICE_NAME);
#ifndef USE_CALL_USERHELPER
    lidbg_uevent_focus("USB_STATE=CONFIGURED",callback_disable_trace_msg );//USB_STATE=DISCONNECTED
#endif

	LIDBG_MODULE_LOG;

    return 0;
}


static int  lidbg_trace_msg_remove(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver lidbg_trace_msg_driver =
{
    .probe		= lidbg_trace_msg_probe,
    .remove     = lidbg_trace_msg_remove,
    .driver         = {
        .name = "lidbg_trace_msg",
        .owner = THIS_MODULE,
    },
};

static struct platform_device lidbg_trace_msg_device =
{
    .name               = "lidbg_trace_msg",
    .id                 = -1,
};

static int __init lidbg_trace_msg_init(void)
{
    DUMP_BUILD_TIME;
    platform_device_register(&lidbg_trace_msg_device);

    return platform_driver_register(&lidbg_trace_msg_driver);
}

static void __exit lidbg_trace_msg_exit(void)
{
    DUMP_FUN;

    platform_device_unregister(&lidbg_trace_msg_device);
    platform_driver_unregister(&lidbg_trace_msg_driver);
}





void trace_msg_main(int argc, char **argv)
{

    if(!strcmp(argv[0], "disable"))
    {
        lidbg("\n============disable======\n\n\n");
        lidbg_trace_msg_disable(1);
    }

}

module_init(lidbg_trace_msg_init);
module_exit(lidbg_trace_msg_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");
EXPORT_SYMBOL(trace_msg_main);


