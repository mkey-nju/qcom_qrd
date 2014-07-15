
#include "lidbg.h"
//zone below [fs.cmn.tools]
LIST_HEAD(fs_filedetec_list);
LIST_HEAD(fs_filename_list);
static struct task_struct *fs_fdetectask;
static spinlock_t  new_item_lock;
int g_filedetec_dbg = 0;
int g_filedetect_ms = 10000;
//zone end

//zone below [fs.fs_filename_list]
struct fs_filename_item *get_filename_list_item(struct list_head *client_list, const char *filename)
{
    struct fs_filename_item *pos;
    unsigned long flags;

    spin_lock_irqsave(&new_item_lock, flags);
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (!strcmp(pos->filename, filename))
        {
            spin_unlock_irqrestore(&new_item_lock, flags);
            return pos;
        }
    }
    spin_unlock_irqrestore(&new_item_lock, flags);
    return NULL;
}
bool new_filename_list_item(struct list_head *client_list, char *filename, bool remove_after_copy)
{
    struct fs_filename_item *add_new_item;
    unsigned long flags;

    add_new_item = kmalloc(sizeof(struct fs_filename_item), GFP_ATOMIC);
    if(add_new_item == NULL)
    {
        FS_ERR("<kmalloc.fs_filename_item>\n");
        return false;
    }

    add_new_item->filename = kmalloc(strlen(filename) + 1, GFP_ATOMIC);
    if(add_new_item->filename == NULL)
    {
        FS_ERR("<kmalloc.strlen(name)>\n");
        return false;
    }

    strcpy(add_new_item->filename, filename);
    add_new_item->remove_after_copy = remove_after_copy;

    spin_lock_irqsave(&new_item_lock, flags);
    list_add(&(add_new_item->tmp_list), client_list);
    spin_unlock_irqrestore(&new_item_lock, flags);
    return true;
}

bool register_filename_list(struct list_head *client_list, char *filename, bool remove_after_copy)
{
    struct fs_filename_item *pos = get_filename_list_item(client_list, filename);
    if(pos)
    {
        FS_ERR("<EEXIST.%s>\n", filename);
        return false;
    }
    else
        return new_filename_list_item(client_list, filename, remove_after_copy);
}
void show_filename_list(struct list_head *client_list)
{
    int index = 0;
    struct fs_filename_item *pos;

    if(!list_empty(client_list))
    {
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (pos->filename)
            {
                index++;
                FS_WARN("<%d.%dINFO:[%s]>\n", pos->remove_after_copy, index, pos->filename);
            }
        }
    }
    else
        FS_ERR("<nobody_register>\n");
}
//zone end

//zone below [fs.cmn.driver]
int fs_file_write(char *filename, char *wbuff)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int file_len = 1;

    filep = filp_open(filename,  O_WRONLY, 0);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"err:filp_open,%s\n\n\n\n",filename);
        return -1;
    }

    old_fs = get_fs();
    set_fs(get_ds());

    if(wbuff)
        filep->f_op->write(filep, wbuff, strlen(wbuff), &filep->f_pos);
    set_fs(old_fs);
    filp_close(filep, 0);
    return file_len;
}
int fs_file_read(const char *filename, char *rbuff, int readlen)
{
    struct file *filep;
    mm_segment_t old_fs;
    unsigned int read_len = 1;

    filep = filp_open(filename,  O_RDONLY, 0);
    if(IS_ERR(filep))
    {
        printk(KERN_CRIT"err:filp_open,%s\n\n\n\n",filename);
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

bool clear_file(char *filename)
{
    struct file *filep;
    filep = filp_open(filename, O_CREAT | O_RDWR | O_TRUNC , 0777);
    if(IS_ERR(filep))
        return false;
    else
        filp_close(filep, 0);
    return true;
}
bool get_file_mftime(const char *filename, struct rtc_time *ptm)
{
    struct file *filep;
    struct inode *inode = NULL;
    struct timespec mtime;
    filep = filp_open(filename, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        lidbg("[futengfei]err.open:<%s>\n", filename);
        return false;
    }
    inode = filep->f_dentry->d_inode;
    mtime = inode->i_mtime;
    rtc_time_to_tm(mtime.tv_sec, ptm);
    filp_close(filep, 0);
    return true;
}
bool is_tm_updated(struct rtc_time *pretm, struct rtc_time *newtm)
{
    if(pretm->tm_sec == newtm->tm_sec && pretm->tm_min == newtm->tm_min && pretm->tm_hour == newtm->tm_hour &&
            pretm->tm_mday == newtm->tm_mday && pretm->tm_mon == newtm->tm_mon && pretm->tm_year == newtm->tm_year )
        return false;
    else
        return true;
}
bool is_file_tm_updated(const char *filename, struct rtc_time *pretm)
{
    struct rtc_time tm;
    if(get_file_mftime(filename, &tm) && is_tm_updated(pretm, &tm))
    {
        *pretm = tm; //get_file_mftime(filename, pretm);//give the new tm to pretm;
        return true;
    }
    else
        return false;
}
void new_filedetec_dev(char *filename, void (*cb_filedetec)(char *filename))
{
    struct string_dev *add_new_dev;
    add_new_dev = kzalloc(sizeof(struct string_dev), GFP_KERNEL);
    add_new_dev->filedetec = filename;
    add_new_dev->cb_filedetec = cb_filedetec;
    list_add(&(add_new_dev->tmp_list), &fs_filedetec_list);
}
bool is_file_registerd(char *filename)
{
    struct string_dev *pos;
    struct list_head *client_list = &fs_filedetec_list;

    if(list_empty(client_list))
        return false;
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (!strcmp(pos->filedetec, filename))
            return true;
    }
    return false;
}
void show_filedetec_list(void)
{
    struct string_dev *pos;
    struct list_head *client_list = &fs_filedetec_list;

    if(list_empty(client_list))
        return ;
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->filedetec && pos->cb_filedetec)
            FS_WARN("<registerd_list:%s>\n", pos->filedetec);
    }
}
void call_filedetec_cb(void)
{
    struct string_dev *pos;
    struct list_head *client_list = &fs_filedetec_list;

    if(list_empty(client_list))
        return ;
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (pos->filedetec && pos->cb_filedetec && fs_is_file_exist(pos->filedetec) )
        {
            if(g_filedetec_dbg)
                FS_WARN("<should call :%s>\n", pos->filedetec);
            if (!pos->is_warned)
            {
                pos->cb_filedetec(pos->filedetec);
                if(g_filedetec_dbg)
                    FS_WARN("<have called :%s>\n", pos->filedetec);
            }
            pos->is_warned = true;
        }
        else
            pos->is_warned = false;
    }
}
void regist_filedetec(char *filename, void (*cb_filedetec)(char *filename))
{
    if(  !is_file_registerd(filename))
        new_filedetec_dev(filename, cb_filedetec);
    else
        FS_ERR("<existed :%s>\n", filename);
}
static int thread_filedetec_func(void *data)
{
    allow_signal(SIGKILL);
    allow_signal(SIGSTOP);
    FS_WARN("<thread start>\n");
    if(g_filedetec_dbg)
        show_filedetec_list();
    while(!kthread_should_stop())
    {
        if(g_filedetect_ms && is_fs_work_enable)
        {
            call_filedetec_cb();
            msleep(g_filedetect_ms);
        }
        else
        {
            ssleep(1);
        }
    }
    return 1;
}
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
int fs_get_file_size(char *file)
{
    struct file *filep;
    struct inode *inodefrom = NULL;
    unsigned int file_len;
    filep = filp_open(file, O_RDONLY , 0);
    if(IS_ERR(filep))
        return -1;
    else
    {
        inodefrom = filep->f_dentry->d_inode;
        file_len = inodefrom->i_size;
        filp_close(filep, 0);
        return file_len;
    }
}
void mem_encode(char *addr, int length, int step_size)
{
    int loop = 0;
    char temp;
    while(loop < length)
    {
        temp = *(addr + loop) + step_size;
        if(temp > 0 && temp < 127)
            *(addr + loop) = temp;
        loop++;
    }
}

bool copy_file(char *from, char *to, bool encode)
{
    char *string = NULL;
    unsigned int file_len;
    struct file *pfilefrom;
    struct file *pfileto;
    struct inode *inodefrom = NULL;
    mm_segment_t old_fs;

    if(!fs_is_file_exist(from))
    {
        FS_ALWAYS("<file_miss:%s>\n", from);
        return false;
    }

    pfilefrom = filp_open(from, O_RDONLY , 0);
    pfileto = filp_open(to, O_CREAT | O_RDWR | O_TRUNC, 0777);
    if(IS_ERR(pfileto))
    {
        lidbg_rm(to);
        msleep(500);
        pfileto = filp_open(to, O_CREAT | O_RDWR | O_TRUNC, 0777);
        if(IS_ERR(pfileto))
        {
            FS_ALWAYS("fail to open <%s>\n", to);
            filp_close(pfilefrom, 0);
            return false;
        }
    }
    old_fs = get_fs();
    set_fs(get_ds());
    inodefrom = pfilefrom->f_dentry->d_inode;
    file_len = inodefrom->i_size;

    string = (unsigned char *)kmalloc(file_len, GFP_KERNEL);
    if(string == NULL)
    {
        FS_ALWAYS(" <kmalloc>\n");
        return false;
    }

    pfilefrom->f_op->llseek(pfilefrom, 0, 0);
    pfilefrom->f_op->read(pfilefrom, string, file_len, &pfilefrom->f_pos);
    set_fs(old_fs);
    filp_close(pfilefrom, 0);

    if(encode)
        mem_encode(string, file_len, 1);

    old_fs = get_fs();
    set_fs(get_ds());
    pfileto->f_op->llseek(pfileto, 0, 0);
    pfileto->f_op->write(pfileto, string, file_len, &pfileto->f_pos);
    set_fs(old_fs);
    filp_close(pfileto, 0);

    kfree(string);
    return true;
}
bool get_file_tmstring(char *filename, char *tmstring)
{
    struct rtc_time tm;
    if(filename && tmstring && get_file_mftime(filename, &tm) )
    {
        sprintf(tmstring, "%d-%02d-%02d %02d:%02d:%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour + 8, tm.tm_min, tm.tm_sec);
        return true;
    }
    return false;
}
bool fs_is_file_updated(char *filename, char *infofile)
{
    char pres[64], news[64];
    memset(pres, '\0', sizeof(pres));
    memset(news, '\0', sizeof(news));

    get_file_tmstring(filename, news);

    if(fs_get_file_size(infofile) > 0)
    {
    
        if ( (fs_file_read(infofile, pres, sizeof(pres)) > 0) && (!strcmp(news, pres)))
            return false;
    }
    fs_clear_file(infofile);
    bfs_file_amend(infofile, news, 0);
    return true;
}
void cb_kv_filedetecen(char *key, char *value)
{
    FS_WARN("<%s=%s>\n", key, value);
    if ( (!strcmp(key, "fs_dbg_file_detect" ))  &&  (strcmp(value, "0" )) )
        show_filedetec_list();
}
void cp_data_to_udisk(bool encode)
{
    struct list_head *client_list = &fs_filename_list;
    FS_ALWAYS("encode:%d\n", encode);
    if(!list_empty(client_list))
    {
        int index = 0;
        char dir[128], tbuff[128];
        int copy_delay = 300;
        struct fs_filename_item *pos;

        memset(dir, '\0', sizeof(dir));
        memset(tbuff, '\0', sizeof(tbuff));

        lidbg_get_current_time(tbuff, NULL);
        sprintf(dir, "/mnt/usbdisk/ID%d-%s", get_machine_id(), tbuff);
        lidbg_mkdir(dir);
        msleep(1000);

        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (pos->filename)
            {
                char *file = strrchr(pos->filename, '/');
                if(file)
                {
                    index++;
                    memset(tbuff, '\0', sizeof(tbuff));
                    sprintf(tbuff, "%s/%s", dir, ++file);
                    if(encode)
                        fs_copy_file_encode(pos->filename, tbuff);
                    else
                        fs_copy_file(pos->filename, tbuff);
                }
                if(pos->remove_after_copy)
                    lidbg_rm(pos->filename);
                msleep(copy_delay);

            }
        }
        lidbg_domineering_ack();
    }
    else
        FS_ERR("<nobody_register>\n");
}

//zone end



//zone below [fs.cmn.interface]
void fs_regist_filedetec(char *filename, void (*cb_filedetec)(char *filename ))
{
    if(filename && cb_filedetec)
        regist_filedetec(filename, cb_filedetec);
    else
        FS_ERR("<filename||cb_filedetec:null?>\n");
}
bool fs_copy_file(char *from, char *to)
{
    return copy_file(from, to, false);
}
bool fs_copy_file_encode(char *from, char *to)
{
    return copy_file(from, to, true);
}
bool fs_is_file_exist(char *file)
{
    return is_file_exist(file);
}
bool fs_clear_file(char *filename)
{
    return clear_file(filename);
}
int  lidbg_cp(char from[], char to[])
{
    return fs_copy_file(from, to);
}
bool fs_register_filename_list(char *filename, bool remove_after_copy)
{
    if(filename)
        return register_filename_list(&fs_filename_list, filename, remove_after_copy);
    else
    {
        FS_ERR("<filename.null>\n");
        return false;
    }
}
void fs_show_filename_list(void)
{
    show_filename_list(&fs_filename_list);
}
static bool data_encode = false;
int thread_cp_data_to_udiskt(void *data)
{
    fs_msg_fifo_to_file(NULL, NULL);
    cp_data_to_udisk(data_encode);
    return 0;
}
void fs_cp_data_to_udisk(bool encode)
{
    data_encode = encode;
    CREATE_KTHREAD(thread_cp_data_to_udiskt, NULL);
}
//zone end



void lidbg_fs_cmn_init(void)
{
    spin_lock_init(&new_item_lock);
    fs_get_intvalue(&lidbg_core_list, "fs_filedetect_ms", &g_filedetect_ms, NULL);
    fs_get_intvalue(&lidbg_core_list, "fs_filedetect_dbg", &g_filedetec_dbg, cb_kv_filedetecen);
    fs_fdetectask = kthread_run(thread_filedetec_func, NULL, "ftf_fdetectask");
}


EXPORT_SYMBOL(fs_file_read);
EXPORT_SYMBOL(fs_file_write);
EXPORT_SYMBOL(fs_cp_data_to_udisk);
EXPORT_SYMBOL(fs_regist_filedetec);
EXPORT_SYMBOL(fs_copy_file);
EXPORT_SYMBOL(fs_copy_file_encode);
EXPORT_SYMBOL(fs_is_file_exist);
EXPORT_SYMBOL(fs_is_file_updated);
EXPORT_SYMBOL(fs_clear_file);
EXPORT_SYMBOL(fs_filename_list);
EXPORT_SYMBOL(fs_register_filename_list);
EXPORT_SYMBOL(fs_show_filename_list);
EXPORT_SYMBOL(fs_get_file_size);
EXPORT_SYMBOL(lidbg_cp);



