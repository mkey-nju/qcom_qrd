
#include "lidbg.h"

//zone below [tools]
#define FS_BUFF_SIZE (1 * 1024 )
#define FS_FIFO_SIZE (64 * 1024)

#define FS_VERSION "FS.VERSION:  [20131031]"
bool is_out_updated = false;
bool is_fs_work_enable = false;
int fs_slient_level = 0;//level:0->mem_fifo    (1 err 2 warn 3suc)->uart
struct lidbg_fifo_device *fs_msg_fifo;
//zone end

void lidbg_fileserver_main(int argc, char **argv)
{
    int cmd = 0;
    int cmd_para = 0;
    int thread_count = 0;
    if(argc < 3)
    {
        FS_ERR("echo \"c file 1 1 1\" > /dev/mlidbg0\n");
        return;
    }

    thread_count = simple_strtoul(argv[0], 0, 0);
    cmd = simple_strtoul(argv[1], 0, 0);
    cmd_para = simple_strtoul(argv[2], 0, 0);

    if(thread_count)
        FS_ERR("thead_test:remove\n");

    switch (cmd)
    {
    case 1:
        break;
    case 2:
        fs_enable_kmsg(cmd_para);
        break;
    case 3:
        fs_dump_kmsg((char *)__FUNCTION__, cmd_para * 1024);
        break;
    case 4:
        FS_WARN("machine_id:%d\n", get_machine_id());
        break;
    case 5:
        fs_save_list_to_file();
        break;
    case 6:
        fs_mem_log("chmod_for_apk\n");
        chmod_for_apk();
        break;
    case 7:
        fs_mem_log("fs_upload_machine_log\n");
        fs_upload_machine_log();
        break;
    case 8:
        fs_mem_log("fs_clean_all\n");
        fs_clean_all();
        break;
    case 9:
        FS_WARN("<lidbg_mount>\n");
        lidbg_mount("/system");
        break;
    case 10:
        FS_WARN("<fs_show_filename_list>\n");
        fs_show_filename_list();
        break;
    case 11:
        FS_ALWAYS("<fs_msg_fifo_to_file>\n");
        fs_msg_fifo_to_file(NULL, NULL);
        break;
    default:
        FS_ERR("<check you cmd:%d>\n", cmd);
        break;
    }
}


//zone below [fileserver]
bool is_fly_system(void)
{
    if(fs_is_file_exist(build_time_fly_path))
        return true;
    else
        return false;
}
void copy_all_conf_file(void)
{
    if(is_fly_system())
    {
        fs_copy_file(build_time_fly_path, build_time_sd_path);
        fs_copy_file(core_fly_path, core_sd_path);
        fs_copy_file(driver_fly_path, driver_sd_path) ;
        fs_copy_file(state_fly_path, state_sd_path);
        fs_copy_file(cmd_fly_path, cmd_sd_path);
    }
    else
    {
        fs_copy_file(build_time_lidbg_path, build_time_sd_path);
        fs_copy_file(core_lidbg_path, core_sd_path);
        fs_copy_file(driver_lidbg_path, driver_sd_path);
        fs_copy_file(state_lidbg_path, state_sd_path);
        fs_copy_file(cmd_lidbg_path, cmd_sd_path);
    }
}
void check_conf_file(void)
{
    int size[6];

    size[0] = fs_is_file_updated(build_time_fly_path, PRE_CONF_INFO_FILE);
    size[1] = fs_get_file_size(driver_sd_path);
    size[2] = fs_get_file_size(core_sd_path);
    size[3] = fs_get_file_size(state_sd_path);
    size[4] = fs_get_file_size(PRE_CONF_INFO_FILE);

    fs_mem_log("<check_conf_file:%d,%d,%d,%d,%d>\n", size[0], size[1], size[2], size[3], size[4]);

    if(size[0] || size[1] < 1 || size[2] < 1 || size[3] < 1 || size[4] < 1)
    {
        FS_ALWAYS( "<overwrite:push,update?>\n");
        fs_mem_log( "<overwrite:push,update?>\n");
        fs_remount_system();
        is_out_updated = true;
        analysis_copylist("/flysystem/lib/out/copylist.conf");
        copy_all_conf_file();
        lidbg_rm("/data/kmsg.txt");
        lidbg_rm(LIDBG_KMSG_FILE_PATH);
        lidbg_rm(LIDBG_LOG_DIR"lidbg_mem_log.txt");
        lidbg_rm(FS_FIFO_FILE);
        lidbg_rm(LIDBG_TRACE_MSG_PATH);
    }

}
void fs_msg_fifo_to_file(char *key, char *value)
{
    fs_mem_log("%s\n", __func__);
    lidbg_fifo_get(fs_msg_fifo, FS_FIFO_FILE, 0);
}
void lidbg_fileserver_main_prepare(void)
{

    fs_msg_fifo = lidbg_fifo_alloc("fileserver", FS_FIFO_SIZE, FS_BUFF_SIZE);

    FS_WARN("<%s>\n", FS_VERSION);

    lidbg_mkdir(LIDBG_LOG_DIR);
    lidbg_mkdir(LIDBG_OSD_DIR);

    check_conf_file();

    set_machine_id();
    FS_ALWAYS("machine_id:%d\n", get_machine_id());

    fs_fill_list(core_sd_path, FS_CMD_FILE_CONFIGMODE, &lidbg_core_list);
    fs_fill_list(driver_sd_path, FS_CMD_FILE_CONFIGMODE, &lidbg_drivers_list);
    fs_fill_list(state_sd_path, FS_CMD_FILE_CONFIGMODE, &fs_state_list);

    fs_copy_file(build_time_fly_path, LIDBG_MEM_DIR"build_time.txt");

    fs_get_intvalue(&lidbg_core_list, "fs_slient_level", &fs_slient_level, NULL);
    fs_get_intvalue(&lidbg_core_list, "fs_save_msg_fifo", NULL, fs_msg_fifo_to_file);

}
void lidbg_fileserver_main_init(void)
{
    fs_register_filename_list(LIDBG_MEM_LOG_FILE, false);
    fs_register_filename_list(build_time_sd_path, false);
    fs_register_filename_list(LIDBG_KMSG_FILE_PATH, true);
    fs_register_filename_list(MACHINE_ID_FILE, false);
    fs_register_filename_list(driver_sd_path, false);
    fs_register_filename_list(core_sd_path, false);
    fs_register_filename_list(FS_FIFO_FILE, true);
}
//zone end

static int __init lidbg_fileserver_init(void)
{
    FS_ALWAYS("<==IN==>\n\n");

    lidbg_fileserver_main_prepare();

    lidbg_fs_keyvalue_init();
    lidbg_fs_log_init();
    lidbg_fs_update_init();
    lidbg_fs_conf_init();
    lidbg_fs_cmn_init();

    lidbg_fileserver_main_init();//note,put it in the end.
    msleep(100);
	LIDBG_MODULE_LOG;

    FS_ALWAYS("<==OUT==>\n");
    return 0;
}

static void __exit lidbg_fileserver_exit(void)
{
}


EXPORT_SYMBOL(lidbg_fileserver_main);
EXPORT_SYMBOL(is_out_updated);
EXPORT_SYMBOL(is_fs_work_enable);
EXPORT_SYMBOL(fs_slient_level);

module_init(lidbg_fileserver_init);
module_exit(lidbg_fileserver_exit);

MODULE_AUTHOR("futengfei");
MODULE_DESCRIPTION("fileserver.entry");
MODULE_LICENSE("GPL");

