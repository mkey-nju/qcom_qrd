#ifndef _LIGDBG_FILESERVER__
#define _LIGDBG_FILESERVER__

#define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)

//zone start
enum string_dev_cmd
{
    FS_CMD_FILE_CONFIGMODE,
    FS_CMD_FILE_LISTMODE,

    FS_CMD_LIST_SPLITKV,
    FS_CMD_LIST_SHOW,
    FS_CMD_LIST_IS_STRINFILE,
    FS_CMD_LIST_GETVALUE,
    FS_CMD_LIST_SETVALUE,
    FS_CMD_LIST_SETVALUE2,
    FS_CMD_LIST_SAVEINFO,
    FS_CMD_LIST_SAVE2FILE,
    FS_CMD_COUNT,
};
struct string_dev
{
    struct list_head tmp_list;
    char *yourkey;
    char *yourvalue;
    char *filedetec;
    int *int_value;
    bool is_warned;
    void (*callback)(char *key, char *value);
    void (*cb_filedetec)(char *filename );
};
struct fs_filename_item
{
    struct list_head tmp_list;
    char *filename;
    bool remove_after_copy;
};
#define LIDBG_LOG_DIR "/data/lidbg/"
#define LIDBG_MEM_DIR "/dev/log/"
#define LIDBG_OSD_DIR "/data/lidbg_osd/"

#define LIDBG_MEM_LOG_FILE LIDBG_MEM_DIR"lidbg_log.txt"
#define PRE_CONF_INFO_FILE LIDBG_LOG_DIR"conf_info.txt"
#define build_time_sd_path LIDBG_LOG_DIR"build_time.txt"
#define build_time_fly_path "/flysystem/lib/out/build_time.conf"
#define build_time_lidbg_path "/system/lib/modules/out/build_time.conf"
extern void lidbg_fileserver_main(int argc, char **argv);
extern void fs_cp_data_to_udisk(bool encode);
extern void fs_file_separator(char *file2separator);
extern void fs_regist_filedetec(char *filename, void (*cb_filedetec)(char *filename ));
extern void fs_enable_kmsg( bool enable );
extern void fs_save_state(void);
extern int fs_get_file_size(char *file);
extern int  lidbg_cp(char from[], char to[]);
extern int get_machine_id(void);
extern int fs_file_read(const char *filename, char *rbuff, int readlen);
extern int fs_file_write(char *filename, char *wbuff);
extern int fs_update(const char *ko_list, const char *fromdir, const char *todir);
extern int analysis_copylist(const char *copy_list);
extern int fs_dump_kmsg(char *tag, int size );
extern int fs_regist_state(char *key, int *value);
extern int fs_get_intvalue(struct list_head *client_list, char *key, int *int_value, void (*callback)(char *key, char *value));
extern int fs_get_value(struct list_head *client_list, char *key, char **string);
extern int fs_set_value(struct list_head *client_list, char *key, char *string);
extern int fs_find_string(struct list_head *client_list, char *string);
extern int fs_string2file(int file_limit_M, char *filename, const char *fmt, ... );
extern int fs_show_list(struct list_head *client_list);
extern int fs_mem_log( const char *fmt, ...);
extern int fs_fill_list(char *filename, enum string_dev_cmd cmd, struct list_head *client_list);
extern bool fs_clear_file(char *filename);
extern bool fs_is_file_exist(char *file);
extern bool fs_is_file_updated(char *filename, char *infofile);
extern bool fs_copy_file(char *from, char *to);
extern bool fs_copy_file_encode(char *from, char *to);
extern bool fs_upload_machine_log(void);
extern bool fs_register_filename_list(char *filename, bool remove_after_copy);
extern void fs_show_filename_list(void);
extern void fs_remount_system(void);
extern void fs_call_apk(void);
extern void fs_remove_apk(void);
extern void fs_clean_all(void);
extern void fs_save_list_to_file(void);

extern bool is_out_updated;
extern bool is_fs_work_enable;

extern int fs_slient_level;

extern struct list_head lidbg_drivers_list;
extern struct list_head lidbg_core_list;
extern struct list_head fs_filename_list;

#define printk_fs(msg...)  do { lidbg(  "lidbg_fs: " msg); }while(0)


#define lidbg_fs_log(path,fmt,...) do{	char buf[32];\
								printk_fs(fmt,##__VA_ARGS__);\
								lidbg_get_current_time(buf,NULL);\
								fs_string2file(0,path,"[%s] ",buf);\
								fs_string2file(0,path,fmt,##__VA_ARGS__);\
								}while(0)


#define lidbg_fs_mem(fmt,...) do{	char buf[32];\
								printk_fs(fmt,##__VA_ARGS__);\
								lidbg_get_current_time(buf,NULL);\
								fs_mem_log("[%s] ",buf);\
								fs_mem_log(fmt,##__VA_ARGS__);\
								}while(0)

#define FS_REGISTER_INT(intvalue,key,def_value,callback) intvalue=def_value; \
			if(fs_get_intvalue(&lidbg_drivers_list, key,&intvalue,callback)<0) \
				fs_get_intvalue(&lidbg_core_list, key,&intvalue,callback);\
			lidbg("config:%s=%d\n",key,intvalue);
#define FS_REGISTER_KEY(key,callback)\
			if(fs_get_intvalue(&lidbg_drivers_list, key,NULL,callback)<0) \
				fs_get_intvalue(&lidbg_core_list, key,NULL,callback);
//zone end


#endif

