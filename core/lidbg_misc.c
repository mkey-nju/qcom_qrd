#include "lidbg.h"

static int logcat_en;
static int reboot_delay_s = 0;
static int cp_data_to_udisk_en = 0;
static int update_lidbg_out_dir_en = 0;
static int delete_out_dir_after_update = 1;
static int dump_mem_log = 0;
static int loop_warning_en = 0;

bool set_wifi_adb_mode(bool on)
{
    LIDBG_WARN("<%d>\n", on);
    if(on)
        lidbg_setprop("service.adb.tcp.port", "5555");
    else
        lidbg_setprop("service.adb.tcp.port", "-1");
    lidbg_stop("adbd");
    lidbg_start("adbd");
    return true;
}
void cb_password_chmod(char *password )
{
    fs_mem_log("<called:%s>\n", __func__ );

    lidbg_chmod("/system/bin/mount");
    lidbg_mount("/system");
    lidbg_chmod("/data");
}
void cb_password_upload(char *password )
{
    fs_mem_log("<called:%s>\n", __func__ );
    fs_upload_machine_log();
}
void cb_password_call_apk(char *password )
{
    fs_mem_log("<called:%s>\n", __func__ );
    fs_call_apk();
}
void cb_password_remove_apk(char *password )
{
    fs_mem_log("<called:%s>\n", __func__ );
    fs_remove_apk();
}
void cb_password_clean_all(char *password )
{
    fs_mem_log("<called:%s>\n", __func__ );
    fs_clean_all();
}
void callback_copy_file(char *dirname, char *filename)
{
    char from[64], to[64];
    memset(from, '\0', sizeof(from));
    memset(to, '\0', sizeof(to));
    sprintf(from, "%s/%s", dirname, filename);
    sprintf(to, "/flysystem/lib/out/%s", filename);
    if(fs_copy_file(from,  to))
        LIDBG_WARN("<cp:%s,%s>\n", from, to);
}
void cb_password_update(char *password )
{
    analysis_copylist("/mnt/usbdisk/conf/copylist.conf");

    if(fs_is_file_exist("/mnt/usbdisk/out/release"))
    {
        LIDBG_WARN("<===============UPDATE_INFO =================>\n" );
        if( fs_update("/mnt/usbdisk/out/release", "/mnt/usbdisk/out", "/flysystem/lib/out") >= 0)
        {
            if(delete_out_dir_after_update)
                lidbg_rmdir("/mnt/usbdisk/out");
            lidbg_launch_user(CHMOD_PATH, "777", "/flysystem/lib/out", "-R", NULL, NULL, NULL);
            lidbg_reboot();
        }
    }
    else if(lidbg_readdir_and_dealfile("/mnt/usbdisk/out", callback_copy_file))
    {
        lidbg_rmdir(LIDBG_LOG_DIR);
        if(delete_out_dir_after_update)
            lidbg_rmdir("/mnt/usbdisk/out");
        lidbg_launch_user(CHMOD_PATH, "777", "/flysystem/lib/out", "-R", NULL, NULL, NULL);
        lidbg_reboot();
    }
    else
        LIDBG_ERR("<up>\n" );
}
void update_lidbg_out_dir(char *key, char *value )
{
    cb_password_update(NULL);
}

void cb_password_gui_kmsg(char *password )
{
    if(lidbg_exe("/flysystem/lib/out/lidbg_gui", "/proc/kmsg", "1", NULL, NULL, NULL, NULL) < 0)
        LIDBG_ERR("Exe lidbg_kmsg failed !\n");
}

void cb_password_gui_state(char *password )
{
    if(lidbg_exe("/flysystem/lib/out/lidbg_gui", "/dev/log/state.txt", "1", NULL, NULL, NULL, NULL) < 0)
        LIDBG_ERR("Exe status failed !\n");
}

void cb_password_mem_log(char *password )
{
    lidbg_fifo_get(glidbg_msg_fifo, LIDBG_LOG_DIR"lidbg_mem_log.txt", 0);
}
void cb_int_mem_log(char *key, char *value )
{
    cb_password_mem_log(NULL);
}
int thread_reboot(void *data)
{
    bool volume_find;
    if(!reboot_delay_s)
    {
        lidbg("<reb.exit.%d>\n", reboot_delay_s);
        return 0;
    }
    ssleep(reboot_delay_s);
    volume_find = !!find_mounted_volume_by_mount_point("/mnt/usbdisk") ;
    if(volume_find && !te_is_ts_touched())
    {
        lidbg("<lidbg:thread_reboot,call reboot,%d>\n", te_is_ts_touched());
        msleep(100);
        kernel_restart(NULL);
    }
    return 0;
}

void logcat_lunch(char *key, char *value )
{
    k2u_write(LOG_LOGCAT);
}
void cb_cp_data_to_udisk(char *key, char *value )
{
    fs_cp_data_to_udisk(false);
}
int loop_warnning(void *data)
{
    while(1)
    {
        lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SIGNAL_EVENT, NOTIFIER_MINOR_SIGNAL_BAKLIGHT_ACK));
        msleep(1000);
    }
    return 0;
}

void lidbg_loop_warning(void)
{
    if(loop_warning_en)
    {
        DUMP_FUN;
        CREATE_KTHREAD(loop_warnning, NULL);
    }
}
void cb_kv_wifiadb(char *key, char *value)
{
    if(value && *value == '1')
        set_wifi_adb_mode(true);
    else
        set_wifi_adb_mode(false);
}
void cb_kv_app_install(char *key, char *value)
{
    if(value && *value == '1')
        lidbg_pm_install_dir("/mnt/usbdisk/apps");
    else
        fs_mem_log("cb_kv_app_install:fail,%s\n", value);
}

void cb_kv_cmd(char *key, char *value)
{
    if(value)
    {
        char *cmd[8] = {NULL};
        char *param[8] = {NULL};
        int cmd_num, num, loop = 0;
        cmd_num = lidbg_token_string(value, ";", cmd) ;

        for(loop = 0; loop < cmd_num; loop++)
        {
            num = lidbg_token_string(cmd[loop], ",", param);
            if(num > 1)
            {
                if(!strcmp(param[0], "echo"))
                    (param[3] && param[1]) ? fs_file_write(param[3], param[1]) : printk(KERN_CRIT"echo err\n");
                else
                    lidbg_exe(param[0], param[1], param[2] ? param[2] : NULL, param[3] ? param[3] : NULL, param[4] ? param[4] : NULL, param[5] ? param[5] : NULL, param[6] ? param[6] : NULL);

                fs_mem_log("cb_kv_cmd:%d,%s,%s\n", num, param[0], param[1]);
                msleep(20);
            }
        }

    }
}
int misc_init(void *data)
{
    LIDBG_WARN("<==IN==>\n");

    te_regist_password("001100", cb_password_remove_apk);
    te_regist_password("001101", cb_password_upload);
    te_regist_password("001102", cb_password_call_apk);
    te_regist_password("001110", cb_password_clean_all);
    te_regist_password("001111", cb_password_chmod);
    te_regist_password("001112", cb_password_update);
    te_regist_password("001120", cb_password_gui_kmsg);
    te_regist_password("001121", cb_password_gui_state);
    te_regist_password("011200", cb_password_mem_log);

    FS_REGISTER_INT(dump_mem_log, "dump_mem_log", 0, cb_int_mem_log);
    FS_REGISTER_INT(logcat_en, "logcat_en", 0, logcat_lunch);
    FS_REGISTER_INT(reboot_delay_s, "reboot_delay_s", 0, NULL);
    FS_REGISTER_INT(cp_data_to_udisk_en, "cp_data_to_udisk_en", 0, cb_cp_data_to_udisk);
    FS_REGISTER_INT(update_lidbg_out_dir_en, "update_lidbg_out_dir_en", 0, update_lidbg_out_dir);
    FS_REGISTER_INT(delete_out_dir_after_update, "delete_out_dir_after_update", 0, NULL);
    FS_REGISTER_INT(loop_warning_en, "loop_warning_en", 0, NULL);

    FS_REGISTER_KEY( "cmdstring", cb_kv_cmd);
    FS_REGISTER_KEY( "wifiadb_en", cb_kv_wifiadb);
    FS_REGISTER_KEY( "app_install_en", cb_kv_app_install);

    fs_register_filename_list("/data/kmsg.txt", true);
    fs_register_filename_list("/data/top.txt", true);
    fs_register_filename_list("/data/ps.txt", true);
    fs_register_filename_list("/data/df.txt", true);
    fs_register_filename_list("/data/machine.txt", true);
    fs_register_filename_list("/data/dumpsys.txt", true);
    fs_register_filename_list("/data/screenshot.png", true);
    fs_register_filename_list(LIDBG_LOG_DIR"lidbg_mem_log.txt", true);

    if(1 == logcat_en)
        logcat_lunch(NULL, NULL);

    CREATE_KTHREAD(thread_reboot, NULL);

    LIDBG_WARN("<==OUT==>\n\n");
    LIDBG_MODULE_LOG;
    return 0;
}


static int __init lidbg_misc_init(void)
{

    DUMP_FUN;
    CREATE_KTHREAD(misc_init, NULL);
    return 0;
}

static void __exit lidbg_misc_exit(void)
{
}

module_init(lidbg_misc_init);
module_exit(lidbg_misc_exit);

EXPORT_SYMBOL(lidbg_loop_warning);


MODULE_AUTHOR("futengfei");
MODULE_DESCRIPTION("misc zone");
MODULE_LICENSE("GPL");

