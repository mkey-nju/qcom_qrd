

extern struct lidbg_fifo_device *fs_msg_fifo;

#define FS_SUC(fmt, args...) do {if(fs_slient_level>=3)printk(KERN_CRIT"[futengfei.fs]suceed.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]suceed.%s: " fmt,__func__,##args);} while (0)
#define FS_WARN(fmt, args...) do {if(fs_slient_level>=2)printk(KERN_CRIT"[futengfei.fs]warn.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]warn.%s: " fmt,__func__,##args);} while (0)
#define FS_ERR(fmt, args...) do {if(fs_slient_level>=1)printk(KERN_CRIT"[futengfei.fs]err.%s: " fmt,__func__,##args);else lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]err.%s: " fmt,__func__,##args);} while (0)

#define FS_ALWAYS(fmt, args...) do {printk(KERN_CRIT"[futengfei.fs]warn.%s: " fmt,__func__,##args);lidbg_fifo_put(fs_msg_fifo,"[futengfei.fs]warn.%s: " fmt,__func__,##args);} while (0)


#define LIDBG_KMSG_FILE_PATH LIDBG_LOG_DIR"lidbg_kmsg.txt"
#define MACHINE_ID_FILE LIDBG_LOG_DIR"MIF.txt"
#define LIDBG_NODE "/dev/mlidbg0"
#define KMSG_NODE "/proc/kmsg"

#define FS_FIFO_FILE LIDBG_LOG_DIR"fs_msg_fifo.txt"

#define driver_sd_path LIDBG_LOG_DIR"drivers.txt"
#define driver_fly_path "/flysystem/lib/out/drivers.conf"
#define driver_lidbg_path "/system/lib/modules/out/drivers.conf"
#define core_sd_path LIDBG_LOG_DIR"core.txt"
#define core_fly_path "/flysystem/lib/out/core.conf"
#define core_lidbg_path "/system/lib/modules/out/core.conf"
#define cmd_sd_path LIDBG_LOG_DIR"cmd.txt"
#define cmd_fly_path "/flysystem/lib/out/cmd.conf"
#define cmd_lidbg_path "/system/lib/modules/out/cmd.conf"
#define state_sd_path LIDBG_LOG_DIR"state.txt"
#define state_mem_path LIDBG_MEM_DIR"state.txt"
#define state_fly_path "/flysystem/lib/out/state.conf"
#define state_lidbg_path "/system/lib/modules/out/state.conf"

extern struct list_head fs_state_list;
void fs_msg_fifo_to_file(char *key, char *value);
extern int analysis_copylist(const char *copy_list);
extern int update_list(const char *filename, struct list_head *client_list);
extern int bfs_file_amend(char *file2amend, char *str_append, int file_limit_M);
extern void save_list_to_file(struct list_head *client_list, char *filename);
extern void set_machine_id(void);
extern void copy_all_conf_file(void);
extern void lidbg_fs_cmn_init(void);
extern void lidbg_fs_conf_init(void);
extern void lidbg_fs_keyvalue_init(void);
extern void lidbg_fs_log_init(void);
extern void lidbg_fs_update_init(void);
extern void chmod_for_apk(void);
extern void update_file_tm(void);
extern bool get_file_mftime(const char *filename, struct rtc_time *ptm);
extern bool get_file_tmstring(char *filename, char *tmstring);
extern bool is_file_tm_updated(const char *filename, struct rtc_time *pretm);
extern bool is_fly_system(void);


