
#include "lidbg.h"
#define DEV_NAME "lidbg_uevent"

struct uevent_dev
{
    struct list_head tmp_list;
    char *focus;
    void (*callback)(char *focus, char *uevent);
};

LIST_HEAD(uevent_list);
struct miscdevice lidbg_uevent_device;
int uevent_dbg = 0;


bool uevent_focus(char *focus, void(*callback)(char *focus, char *uevent))
{
    struct uevent_dev *add_new_dev = NULL;
    add_new_dev = kzalloc(sizeof(struct uevent_dev), GFP_KERNEL);
    if (add_new_dev != NULL && focus && callback)
    {
        add_new_dev->focus = focus;
        add_new_dev->callback = callback;
        list_add(&(add_new_dev->tmp_list), &uevent_list);
        LIDBG_SUC("%s\n", focus);
        return true;
    }
    LIDBG_ERR("add_new_dev != NULL && focus && callback?\n");
    return false;
}

void uevent_send(enum kobject_action action, char *envp_ext[])
{
    LIDBG_WARN("%s,%s\n", (envp_ext[0] == NULL ? "null" : envp_ext[0]), (envp_ext[1] == NULL ? "null" : envp_ext[1]));
    if(kobject_uevent_env(&lidbg_uevent_device.this_device->kobj, action, envp_ext) < 0)
        LIDBG_ERR("uevent_send\n");
}

void uevent_shell(char *shell_cmd)
{
    char shellstring[256];
    char *envp[] = { "LIDBG_ACTION=shell", shellstring, NULL };
    snprintf(shellstring, 256, "LIDBG_PARAMETER=%s", shell_cmd );
    lidbg_uevent_send(KOBJ_CHANGE, envp);
}

int lidbg_uevent_open(struct inode *inode, struct file *filp)
{
    return 0;
}

ssize_t  lidbg_uevent_write(struct file *filp, const char __user *buf, size_t count, loff_t *offset)
{
    char *tmp;
    struct uevent_dev *pos;
    struct list_head *client_list = &uevent_list;
    tmp = memdup_user(buf, count + 1);
    if (IS_ERR(tmp))
    {
        LIDBG_ERR("<memdup_user>\n");
        return PTR_ERR(tmp);
    }
    tmp[count] = '\0';

    if(uevent_dbg)
        LIDBG_WARN("%s\n", tmp);

    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->focus && pos->callback && strstr(tmp, pos->focus))
        {
            LIDBG_SUC("called: %s  %ps\n", pos->focus, pos->callback);
            pos->callback(pos->focus, tmp);
        }
    }
    kfree(tmp);
    return count;
}

static const struct file_operations lidbg_uevent_fops =
{
    .owner = THIS_MODULE,
    .open = lidbg_uevent_open,
    .write = lidbg_uevent_write,
};

struct miscdevice lidbg_uevent_device =
{
    .minor = 255,
    .name = DEV_NAME,
    .fops = &lidbg_uevent_fops,
};

static int __init lidbg_uevent_init(void)
{
    //DUMP_BUILD_TIME;
    if (misc_register(&lidbg_uevent_device))
        LIDBG_ERR("misc_register\n");
    return 0;
}

static void __exit lidbg_uevent_exit(void)
{
    misc_deregister(&lidbg_uevent_device);
}


//zone below [interface]
bool lidbg_uevent_focus(char *focus, void(*callback)(char *focus, char *uevent))
{
    return uevent_focus(focus, callback);
}
void lidbg_uevent_send(enum kobject_action action, char *envp_ext[])
{
    uevent_send(action, envp_ext);
}
void lidbg_uevent_shell(char *shell_cmd)
{
    uevent_shell(shell_cmd);
}

void lidbg_uevent_main(int argc, char **argv)
{
    if(!strcmp(argv[0], "dbg"))
    {
        uevent_dbg = !uevent_dbg;
    }
    else if(!strcmp(argv[0], "uevent"))
    {
        lidbg_uevent_shell(argv[1]);
    }
}

EXPORT_SYMBOL(lidbg_uevent_focus);
EXPORT_SYMBOL(lidbg_uevent_send);
EXPORT_SYMBOL(lidbg_uevent_shell);
EXPORT_SYMBOL(lidbg_uevent_main);

module_init(lidbg_uevent_init);
module_exit(lidbg_uevent_exit);

MODULE_DESCRIPTION("futengfei 2014.3.8");
MODULE_LICENSE("GPL");

