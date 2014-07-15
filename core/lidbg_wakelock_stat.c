#include "lidbg.h"


#define WL_COUNT_TYPE "count"
#define WL_UNCOUNT_TYPE "uncount"

static LIST_HEAD(waklelock_detail_list);
LIST_HEAD(lidbg_wakelock_list);
static spinlock_t  new_item_lock;
static struct task_struct *wl_item_dbg_task;
static int g_wakelock_dbg = 0;
static int g_wakelock_dbg_item = 0;

struct wakelock_item *get_wakelock_item(struct list_head *client_list, const char *name)
{
    struct wakelock_item *pos;
    unsigned long flags;

    spin_lock_irqsave(&new_item_lock, flags);
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (!strcmp(pos->name, name))
        {
            spin_unlock_irqrestore(&new_item_lock, flags);
            return pos;
        }
    }
    spin_unlock_irqrestore(&new_item_lock, flags);
    return NULL;
}
bool new_wakelock_item(struct list_head *client_list, bool cnt_wakelock, const char *name, const char *package_name, unsigned int pid, unsigned int uid)
{
    struct wakelock_item *add_new_item;
    unsigned long flags;

    add_new_item = kmalloc(sizeof(struct wakelock_item), GFP_ATOMIC);
    if(add_new_item == NULL)
    {
        lidbg("<err.register_wakelock:kzalloc.wakelock_item>\n");
        return false;
    }

    add_new_item->name = kmalloc(strlen(name) + 1, GFP_ATOMIC);
    if(add_new_item->name == NULL)
    {
        lidbg("<err.register_wakelock:kzalloc.name,%s>\n", name);
        return false;
    }
    strcpy(add_new_item->name, name);


    if(package_name)
    {
        add_new_item->package_name = kmalloc(strlen(package_name) + 1, GFP_ATOMIC);
        if(add_new_item->package_name == NULL)
        {
            lidbg("<err.register_wakelock:kzalloc.package_name%s>\n", package_name);
            return false;
        }
        strcpy(add_new_item->package_name, package_name);
    }
    else
        add_new_item->package_name = "null";

    add_new_item->cunt = 1;
    add_new_item->cunt_max = 1;
    add_new_item->is_count_wakelock = cnt_wakelock;
    add_new_item->pid = pid;
    add_new_item->uid = uid;


    spin_lock_irqsave(&new_item_lock, flags);
    list_add(&(add_new_item->tmp_list), client_list);
    spin_unlock_irqrestore(&new_item_lock, flags);
    lidbg("<NEW:[%s][%s,%s]>\n", lock_type(cnt_wakelock) , add_new_item->name, add_new_item->package_name);
    return true;
}


bool register_wakelock(struct list_head *client_list, struct list_head *detail_list, bool cnt_wakelock, const char *name, const char *package_name, unsigned int pid, unsigned int uid)
{
    bool is_item_req_detail = false;
    struct wakelock_item *wakelock_pos = get_wakelock_item(client_list, name);
    if(detail_list && get_wakelock_item(detail_list, name))
        is_item_req_detail = true;

    if(wakelock_pos)
    {
        wakelock_pos->cunt++;
        wakelock_pos->cunt_max++;
        if(g_wakelock_dbg || is_item_req_detail)
            lidbg("<ADD:[%s][%d,%d][%s,%s]>\n", lock_type(wakelock_pos->is_count_wakelock), wakelock_pos->cunt, wakelock_pos->cunt_max, wakelock_pos->name , wakelock_pos->package_name);
        return true;
    }
    else
        return new_wakelock_item( client_list, cnt_wakelock, name, package_name, pid, uid);
}

bool unregister_wakelock(struct list_head *client_list, struct list_head *detail_list, const char *name)
{
    bool is_item_req_detail = false;
    struct wakelock_item *wakelock_pos = get_wakelock_item(client_list, name);
    if(detail_list && get_wakelock_item(detail_list, name))
        is_item_req_detail = true;

    if(wakelock_pos)
    {

        if(wakelock_pos->is_count_wakelock)
        {
            if(wakelock_pos->cunt > 0)
                wakelock_pos->cunt--;
        }
        else
            wakelock_pos->cunt = 0;

        if(g_wakelock_dbg || is_item_req_detail)
            lidbg("<DEL:[%s][%d,%d][%s]>\n", lock_type(wakelock_pos->is_count_wakelock), wakelock_pos->cunt, wakelock_pos->cunt_max, wakelock_pos->name );
        return true;
    }
    else
    {
        //can't find the wakelock? show the wakelock list.
        lidbg("<LOST:[%s]>\n", name );
        if(g_wakelock_dbg)
        {
            if(!list_empty(client_list))
            {
                lidbg_show_wakelock(0);
            }
        }
        return false;
    }
}

void lidbg_show_wakelock(int is_should_kill_apk)
{
    int index = 0;
    char *p1 = NULL, *p2 = NULL;
    struct wakelock_item *pos;
    struct list_head *client_list ;

    client_list = &lidbg_wakelock_list;
    if(!list_empty(client_list))
    {
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (pos->name && pos->cunt && pos->is_count_wakelock)
            {
                index++;
                printk(KERN_CRIT"[ftf_pm.wl]%d,MAX%d<THE%d:[%d,%d][%s][%s,%s]>\n", pos->cunt, pos->cunt_max, index, pos->pid, pos->uid, lock_type(pos->is_count_wakelock), pos->name, pos->package_name);
                if(is_should_kill_apk == 1)
                {
                    p1 = strchr(pos->package_name, ',');
                    p2 = strchr(pos->package_name, '.');

                    if(p1)
                        ;//lidbg_force_stop_apk(p1 + 1);
                    else if(p2)
                        ;//lidbg_force_stop_apk(pos->package_name);
                }
                if(is_should_kill_apk == 2)
                    fs_string2file(1, LIDBG_OSD_DIR"can_t_sleep.txt", "[J]%d,[%s,%s]>\n", index, pos->name, pos->package_name);
                }
        }
    }
    else
        lidbg("<err.lidbg_show_wakelock:nobody_register>\n");
}

static int thread_wl_item_dbg(void *data)
{
    char cmd_tmp[128];
    int count = 0;
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    while(!kthread_should_stop())
    {
        sprintf(cmd_tmp, "c wakelock lock count 0 0 f%d", ++count);
        fs_file_write("/dev/mlidbg0", cmd_tmp);
        if(!g_wakelock_dbg_item)
            ssleep(30);
        else
            msleep(g_wakelock_dbg_item);
    }
    return 1;
}
void start_wl_dbg_thread(char *dbg_ms)
{
    g_wakelock_dbg_item = simple_strtoul(dbg_ms, 0, 0);
    wl_item_dbg_task = kthread_run(thread_wl_item_dbg, NULL, "ftf_wl_dbg");
}
void lidbg_wakelock_stat(int argc, char **argv)
{

    char *wakelock_action = NULL;
    char *wakelock_type = NULL;
    char *wakelock_name = NULL;
    char *wakelock_package_name = NULL;
    bool is_count_wakelock = false;
    int pid, uid;
    if(argc < 5)
    {
        lidbg("%d[futengfei]err.lidbg_wakelock_stat:echo \"c wakelock lock count  pid uid name\" > /dev/mlidbg0\n", argc);
        return;
    }

    wakelock_action = argv[0];
    wakelock_type = argv[1];
    pid = simple_strtoul(argv[2], 0, 0);
    uid = simple_strtoul(argv[3], 0, 0);
    wakelock_name = argv[4];

    if(argc > 5)
        wakelock_package_name = argv[5];

    if(!strcmp(wakelock_type, WL_COUNT_TYPE))
        is_count_wakelock = true;

    if (!strcmp(wakelock_action, "lock"))
        register_wakelock(&lidbg_wakelock_list, &waklelock_detail_list, is_count_wakelock, wakelock_name, wakelock_package_name, pid, uid);
    else if (!strcmp(wakelock_action, "unlock"))
        unregister_wakelock(&lidbg_wakelock_list, &waklelock_detail_list, wakelock_name);
    else if (!strcmp(wakelock_action, "show"))
        lidbg_show_wakelock(0);
    else if (!strcmp(wakelock_action, "dbg"))
        start_wl_dbg_thread(wakelock_type);

}
void lidbg_wakelock_register(bool to_lock, const char *name)
{
    if(g_wakelock_dbg)
        lidbg("<lidbg_wakelock_register:[%d][%s]>\n", to_lock, name);

#ifndef SOC_msm8x25
    return ;
#endif

    if(to_lock)
    {
        register_wakelock(&lidbg_wakelock_list, &waklelock_detail_list, false, name, NULL, 0, 0);
    }
    else
    {
        unregister_wakelock(&lidbg_wakelock_list, &waklelock_detail_list, name);
    }
}

void cb_kv_show_list(char *key, char *value)
{
    lidbg_show_wakelock(0);
}

static int __init lidbg_wakelock_stat_init(void)
{
    DUMP_BUILD_TIME;

    if(fs_fill_list("/flysystem/lib/out/wakelock_detail_list.conf", FS_CMD_FILE_LISTMODE, &waklelock_detail_list) < 0)
        fs_fill_list("/system/lib/modules/out/wakelock_detail_list.conf", FS_CMD_FILE_LISTMODE, &waklelock_detail_list);

    spin_lock_init(&new_item_lock);
    FS_REGISTER_INT(g_wakelock_dbg, "wakelock_dbg", 0, cb_kv_show_list);
    FS_REGISTER_INT(g_wakelock_dbg_item, "wakelock_dbg_item", 0, NULL);

    LIDBG_MODULE_LOG;

    return 0;
}

static void __exit lidbg_wakelock_stat_exit(void)
{
}

module_init(lidbg_wakelock_stat_init);
module_exit(lidbg_wakelock_stat_exit);

MODULE_DESCRIPTION("wakelock.stat");
MODULE_LICENSE("GPL");


EXPORT_SYMBOL(lidbg_wakelock_list);
EXPORT_SYMBOL(lidbg_wakelock_stat);
EXPORT_SYMBOL(lidbg_show_wakelock);
EXPORT_SYMBOL(lidbg_wakelock_register);


