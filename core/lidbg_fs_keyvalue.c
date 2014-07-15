
#include "lidbg.h"

//zone below [fs.keyvalue.tools]
int g_kvbug_on = 0;
//zone end

//zone below [fs.keyvalue.driver]
int get_int_value(struct list_head *client_list, char *key, int *int_value, void (*callback)(char *key, char *value))
{
    struct string_dev *pos;
    if (list_empty(client_list))
    {
        FS_ERR("<list_is_empty>%pf\n", client_list);
        return -2;
    }
    list_for_each_entry(pos, client_list, tmp_list)
    {
        if (!strcmp(pos->yourkey, key) )
        {
            if(pos->int_value)
            {
                *int_value = *(pos->int_value);
                FS_SUC("find_key:<%s=%d>\n", key, *(pos->int_value) );
                return 1;
            }
            if(int_value && pos->yourvalue)
            {
                pos->int_value = int_value;
                *(pos->int_value) = simple_strtoul(pos->yourvalue, 0, 0);
                pos->yourvalue = NULL;
            }
            if(callback)
                pos->callback = callback;
            return 1;
        }
    }
    FS_ERR("fail_find:<%s>\n", key);
    return -1;

}

int fileserver_deal_cmd(struct list_head *client_list, enum string_dev_cmd cmd, char *lookfor, char *key, char **string, int *int_value, void (*callback)(char *key, char *value))
{
    //note:you can add more func here,just copy one of the case as belows;
    struct string_dev *pos;
    char *p;
    if(g_kvbug_on)
        FS_WARN("cmd:[%d]\n", cmd);
    if (list_empty(client_list))
    {
        FS_ERR("<list_is_empty>%pf\n", client_list);
        return -2;
    }
    switch (cmd)
    {
    case FS_CMD_LIST_SPLITKV:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            p = memchr(pos->yourkey, '=', strlen(pos->yourkey));
            if(p != NULL)
            {
                pos->yourvalue = p + 1;
                *p = '\0';
            }
        }
        break;

    case FS_CMD_LIST_SHOW:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if(pos->yourvalue)
                FS_WARN("[%s]=[%s]\n", pos->yourkey, pos->yourvalue);
            else
                FS_WARN("[%s]\n", pos->yourkey);
        }
        break;

    case FS_CMD_LIST_SAVEINFO:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (!strcmp(pos->yourkey, key))
            {
                if(int_value)
                    pos->int_value = int_value;
                if(callback)
                    pos->callback = callback;
            }
        }
        return 1;

    case FS_CMD_LIST_GETVALUE:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if (!strcmp(pos->yourkey, key))
            {
                *string = pos->yourvalue;
                FS_SUC("find_key:<%s=%s>\n", key, *string);
                return 1;
            }
        }
        FS_ERR("fail_find:<%s>\n", key);
        return -1;

    case FS_CMD_LIST_SETVALUE:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            if ( (!strcmp(pos->yourkey, key)) && pos->yourvalue && (strcmp(pos->yourvalue, *string)))
            {

                pos->yourvalue = *string;
                FS_SUC("set_key:<%s=%s>\n", key, pos->yourvalue);
                return 1;
            }
        }
        if(g_kvbug_on)
            FS_ERR("set_key:<%s>\n", key);
        return -1;

    case FS_CMD_LIST_IS_STRINFILE:
        list_for_each_entry(pos, client_list, tmp_list)
        {
            int len = strlen(lookfor);
            if((p = memchr(pos->yourkey, lookfor[0], strlen(pos->yourkey))))
            {
                //note:you can use [strncmp()] to replace the code below;
                //I want to study the workflow;
                while(len)
                {
                    if( p[len - 1] == lookfor[len - 1])
                    {
                        if(g_kvbug_on)
                            FS_WARN("[futengfei]======%d[%c][%c]\n", len, p[len - 1], lookfor[len - 1]);
                        len--;
                        if(len == 0)
                        {
                            FS_SUC("have_find:<%s>\n", lookfor);
                            return 1;
                        }
                    }
                    else
                        break;
                }
            }
        }
        FS_ERR("[fail_find:<%s>\n", lookfor);
        return -1;
    default:
        FS_ERR("<unknown.cmd:%d\n", cmd);
        return -1;
    }
    return 1;
}

int bfs_fill_list(char *filename, enum string_dev_cmd cmd, struct list_head *client_list)
{
    struct file *filep;
    struct inode *inode = NULL;
    struct string_dev *add_new_dev;
    mm_segment_t old_fs;
    char *token, *file_ptr = NULL, *file_ptmp;
    int all_purpose;
    unsigned int file_len;

    filep = filp_open(filename,  O_RDONLY, 0);//O_RDWR
    if(IS_ERR(filep))
    {
        FS_ERR("open:<%s>\n", filename);
        return -1;
    }
    FS_SUC("open:<%s>\n", filename);

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    FS_WARN("File_length:<%d>\n", file_len);
    file_len = file_len + 1;

    file_ptr = (unsigned char *)kmalloc(file_len, GFP_KERNEL);
    if(file_ptr == NULL)
    {
        FS_ERR( "vmalloc:<cannot malloc memory!>\n");
        set_fs(old_fs);
        filp_close(filep, 0);
        return -1;
    }

    filep->f_op->llseek(filep, 0, 0);
    all_purpose = filep->f_op->read(filep, file_ptr, file_len, &filep->f_pos);
    if(all_purpose <= 0)
    {
        FS_ERR( "f_op->read:<read file data failed>\n");
        set_fs(old_fs);
        filp_close(filep, 0);
        return -1;
    }
    set_fs(old_fs);
    filp_close(filep, 0);

    file_ptr[file_len - 1] = '\0';
    if(g_kvbug_on)
        FS_WARN("%s\n", file_ptr);

    FS_WARN("toke:<%s>\n", filename);
    all_purpose = 0;
    file_ptmp = file_ptr;
    while((token = strsep(&file_ptmp, "\n")) != NULL )
    {
        if( token[0] != '#' && token[0] != '\n'  && token[0] != 18 && token[0] != 13  && token[0] != '\0' && token[0] != 10)//del char:er,dc2 look for ASCII
        {
            if(g_kvbug_on)
                FS_WARN("%d[%s]\n", all_purpose, token);
            add_new_dev = kzalloc(sizeof(struct string_dev), GFP_KERNEL);
            add_new_dev->yourkey = token;
            list_add(&(add_new_dev->tmp_list), client_list);
            all_purpose++;
        }
        else if(g_kvbug_on)
            FS_WARN("\ndroped[%s]\n", token);
    }
    if(cmd == FS_CMD_FILE_CONFIGMODE)
        fileserver_deal_cmd(client_list, FS_CMD_LIST_SPLITKV, NULL, NULL, NULL, NULL, NULL);
    return 1;
}

void save_list_to_file(struct list_head *client_list, char *filename)
{
    struct string_dev *pos;
    struct file *filep;
    mm_segment_t old_fs;
    char buff[100];
    filep = filp_open(filename, O_CREAT | O_RDWR | O_TRUNC , 0777);
    if(!IS_ERR(filep))
    {
        old_fs = get_fs();
        set_fs(get_ds());
        if(filep->f_op->write)
        {
            list_for_each_entry(pos, client_list, tmp_list)
            {
                memset(buff, '\0', sizeof(buff));
                if(pos->yourkey && pos->int_value)
                    sprintf(buff, "%s=%d\n", pos->yourkey, *(pos->int_value));
                else if(pos->yourkey && pos->yourvalue)
                    sprintf(buff, "%s=%s\n", pos->yourkey, pos->yourvalue);
                filep->f_op->write(filep, buff, strlen(buff), &filep->f_pos);
                filep->f_op->llseek(filep, 0, SEEK_END);
            }
        }
        set_fs(old_fs);
        filp_close(filep, 0);
    }
    else
        FS_ERR("<fail open:%s>\n", filename);
}

int update_list(const char *filename, struct list_head *client_list)
{
    struct file *filep;
    struct inode *inode = NULL;
    struct string_dev *pos;
    mm_segment_t old_fs;
    char *token, *file_ptr = NULL, *file_ptmp, *ptmp, *key, *value;
    int all_purpose, curren_intvalue;
    unsigned int file_len;

    filep = filp_open(filename, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        FS_ERR("open:<%s>\n", filename);
        return -1;
    }
    FS_SUC("open:<%s>\n", filename);

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    file_len = file_len + 1;

    file_ptr = (unsigned char *)kzalloc(file_len, GFP_KERNEL);
    if(file_ptr == NULL)
    {
        FS_ERR( "kmalloc:<cannot kzalloc memory!>\n");
        set_fs(old_fs);
        filp_close(filep, 0);
        return -1;
    }

    filep->f_op->llseek(filep, 0, 0);
    all_purpose = filep->f_op->read(filep, file_ptr, file_len, &filep->f_pos);
    if(all_purpose <= 0)
    {
        FS_ERR( "f_op->read:<read file data failed>\n");
        kfree(file_ptr);
        set_fs(old_fs);
        filp_close(filep, 0);
        return -1;
    }
    set_fs(old_fs);
    filp_close(filep, 0);

    file_ptr[file_len - 1] = '\0';
    file_ptmp = file_ptr;
    while((token = strsep(&file_ptmp, "\n")) != NULL )
    {
        if( token[0] != '#' && strlen(token) > 2)
        {
            key = token;
            ptmp = memchr(key, '=', strlen(key));
            if(ptmp != NULL)
            {
                value = ptmp + 1;
                *ptmp = '\0';
                FS_WARN("<%s,%s>\n", key, value);
                {
                    //start
                    list_for_each_entry(pos, client_list, tmp_list)
                    {
                        if ( !strcmp(pos->yourkey, key))
                        {
                            if (pos->int_value )
                            {
                                curren_intvalue = simple_strtoul(value, 0, 0);
                                if(*(pos->int_value) != curren_intvalue)
                                {
                                    *(pos->int_value) = curren_intvalue;
                                    if (pos->callback)
                                        pos->callback(key, value);
                                    FS_SUC("<%s=%d>\n", key, *(pos->int_value));
                                }
                            }
                            else if (pos->yourvalue && value && strcmp(pos->yourvalue, value))
                            {
                                if (pos->callback)
                                    pos->callback(key, value);
                                FS_SUC("<%s=%s>\n", key, value);
                            }
                        }
                    }
                }//end

            }
            else if(g_kvbug_on)
                FS_WARN("\ndroped[%s]\n", token);
        }
    }
    kfree(file_ptr);
    lidbg_domineering_ack();
    return 1;
}
//zone end

//zone below [fs.keyvalue.interface]
void fs_save_list_to_file(void)
{
    save_list_to_file(&lidbg_core_list, core_sd_path);
    save_list_to_file(&lidbg_drivers_list, driver_sd_path);
    fs_copy_file(state_mem_path, state_sd_path);
    update_file_tm();
}
int fs_regist_state(char *key, int *value)
{
    return fileserver_deal_cmd( &fs_state_list, FS_CMD_LIST_SAVEINFO, NULL, key, NULL, value, NULL);
}
int fs_get_intvalue(struct list_head *client_list, char *key, int *int_value, void (*callback)(char *key, char *value))
{
    return get_int_value(client_list, key, int_value, callback);
}
int fs_get_value(struct list_head *client_list, char *key, char **string)
{
    return fileserver_deal_cmd( client_list, FS_CMD_LIST_GETVALUE, NULL, key, string, NULL, NULL);
}
int fs_set_value(struct list_head *client_list, char *key, char *string)
{
    return fileserver_deal_cmd( client_list, FS_CMD_LIST_SETVALUE, NULL, key, &string, NULL, NULL);
}
int fs_find_string(struct list_head *client_list, char *string)
{
    return fileserver_deal_cmd(client_list, FS_CMD_LIST_IS_STRINFILE, string, NULL, NULL, NULL, NULL);
}
int fs_show_list(struct list_head *client_list)
{
    return fileserver_deal_cmd(client_list, FS_CMD_LIST_SHOW, NULL, NULL, NULL, NULL, NULL);
}
int fs_fill_list(char *filename, enum string_dev_cmd cmd, struct list_head *client_list)
{
    int ret = -1;
    if(list_empty(client_list))
        ret = bfs_fill_list(filename, cmd, client_list);
    else
        FS_ERR(":<your list is not empty>\n");
    return ret;
}
//zone end


void lidbg_fs_keyvalue_init(void)
{
    fs_get_intvalue(&lidbg_core_list, "fs_kv_dbg", &g_kvbug_on, NULL);
}


EXPORT_SYMBOL(fs_save_list_to_file);
EXPORT_SYMBOL(fs_regist_state);
EXPORT_SYMBOL(fs_get_intvalue);
EXPORT_SYMBOL(fs_get_value);
EXPORT_SYMBOL(fs_set_value);
EXPORT_SYMBOL(fs_find_string);
EXPORT_SYMBOL(fs_show_list);
EXPORT_SYMBOL(fs_fill_list);

