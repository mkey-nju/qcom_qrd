
#include "lidbg.h"

//zone below [fs.update.tools]

//zone end


//zone below [fs.update.driver]
int analysis_copylist(const char *copy_list)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    char *token = NULL, *file_ptr = NULL, *file_ptmp = NULL, *pchar = NULL;
    int all_purpose;
    unsigned int file_len;

    filep = filp_open(copy_list, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        FS_ERR("<open:%s>\n", copy_list);
        return -1;
    }
    FS_SUC("<open:%s>\n", copy_list);

    fs_remount_system();

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    file_len = file_len + 1;

    file_ptr = (unsigned char *)kzalloc(file_len, GFP_KERNEL);
    if(file_ptr == NULL)
    {
        FS_ERR( "<kzalloc>\n");
        return -1;
    }

    filep->f_op->llseek(filep, 0, 0);
    all_purpose = filep->f_op->read(filep, file_ptr, file_len, &filep->f_pos);
    if(all_purpose <= 0)
    {
        FS_ERR( "<f_op->read>\n");
        return -1;
    }
    set_fs(old_fs);
    filp_close(filep, 0);

    file_ptr[file_len - 1] = '\0';
    file_ptmp = file_ptr;
    while((token = strsep(&file_ptmp, "\n")) != NULL )
    {
        if(token && token[0] != '#' && (pchar = strchr(token, '=')))
        {
            *pchar++ = '\0';
            FS_WARN("<cp:%s,%s>\n", token, pchar);
            fs_copy_file(token, pchar);
            pchar = NULL;
        }
    }
    kfree(file_ptr);
    return 1;
}
int update_ko(const char *ko_list, const char *fromdir, const char *todir)
{
    struct file *filep;
    struct inode *inode = NULL;
    mm_segment_t old_fs;
    char *token, *file_ptr = NULL, *file_ptmp;
    char file_from[256];
    char file_to[256];
    int all_purpose;
    unsigned int file_len;

    filep = filp_open(ko_list, O_RDONLY , 0);
    if(IS_ERR(filep))
    {
        FS_ERR("<open:%s>\n", ko_list);
        return -1;
    }
    FS_SUC("<open:%s>\n", ko_list);

    fs_remount_system();

    old_fs = get_fs();
    set_fs(get_ds());

    inode = filep->f_dentry->d_inode;
    file_len = inode->i_size;
    file_len = file_len + 1;

    file_ptr = (unsigned char *)kzalloc(file_len, GFP_KERNEL);
    if(file_ptr == NULL)
    {
        FS_ERR( "<kzalloc>\n");
        return -1;
    }

    filep->f_op->llseek(filep, 0, 0);
    all_purpose = filep->f_op->read(filep, file_ptr, file_len, &filep->f_pos);
    if(all_purpose <= 0)
    {
        FS_ERR( "<f_op->read>\n");
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
            memset(file_from, '\0', sizeof(file_from));
            memset(file_to, '\0', sizeof(file_to));
            sprintf(file_from, "%s/%s", fromdir, token);
            sprintf(file_to, "%s/%s", todir, token);
            FS_WARN("<cp:%s,%s>\n", file_from, file_to);
            fs_copy_file(file_from, file_to);
        }
    }
    kfree(file_ptr);
    return 1;
}
//zone end


//zone below [fs.update.interface]
int fs_update(const char *ko_list, const char *fromdir, const char *todir)
{
    return update_ko(ko_list, fromdir, todir);
}
//zone end

void lidbg_fs_update_init(void)
{

}

EXPORT_SYMBOL(fs_update);
EXPORT_SYMBOL(analysis_copylist);

