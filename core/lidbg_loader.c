
#define NOT_USE_MEM_LOG

#include "lidbg.h"

int load_modules_count = 0;

char *insmod_soc_list[] =
{
    SOC_KO,
	NULL,
};

char *insmod_list[] =
{
    "lidbg_mem_log.ko",
    "lidbg_common.ko",
    "lidbg_fileserver.ko",
    "lidbg_trace_msg.ko",
    "lidbg_wakelock_stat.ko",
    "lidbg_msg.ko",
    "lidbg_servicer.ko",
    "lidbg_touch.ko",
    "lidbg_key.ko",
    "lidbg_i2c.ko",
    "lidbg_io.ko",
    "lidbg_uart.ko",
    "lidbg_main.ko",
    "lidbg_misc.ko",
    INTERFACE_KO,
    NULL,
};

char *insmod_path[] =
{
    "/sdcard/out/",
    "/system/lib/modules/out/",
    "/flysystem/lib/out/",
    NULL,
};
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

void lidbg_insmod( char argv1[])
{
#ifdef USE_CALL_USERHELPER
    static char *bin_path = NULL;
    if(bin_path == NULL )
    {
        if( is_file_exist(RECOVERY_MODE_DIR))
            bin_path = "/sbin/insmod";
        else
            bin_path = "/system/bin/insmod";
    }

    {
        int ret;
        static char *envp[] = { "HOME=/", "TERM=linux", "PATH=/system/bin:/sbin", NULL };
        char *argv[] = { bin_path, argv1, NULL };
        ret = call_usermodehelper(bin_path, argv, envp, UMH_WAIT_PROC);
    }
#else
    char shell_cmd[256];
    sprintf(shell_cmd, "insmod %s", argv1 == NULL ? " " : argv1);
    lidbg_uevent_shell(shell_cmd);
#endif
}

int thread_check_restart(void *data)
{
    DUMP_FUN_ENTER;
    msleep(5000);
    LIDBG_WARN("load_modules_count=%d\n", load_modules_count);
    if(load_modules_count == 0)
    {
        LIDBG_ERR("load_modules_count err,call kernel_restart!\n");
        kernel_restart(NULL);
    }
    DUMP_FUN_LEAVE;
    return 0;
}

int thread_loader(void *data)
{
    int i, j;
    char path[100];
	int tmp;
    DUMP_FUN_ENTER;
    CREATE_KTHREAD(thread_check_restart, NULL);

    for(i = 0; insmod_path[i] != NULL; i++)
    {
    	sprintf(path, "%slidbg_loader.ko", insmod_path[i]);
    	if(!is_file_exist(path))continue;

		for(j = 0; insmod_soc_list[j] != NULL; j++)
        {
            sprintf(path, "%s%s", insmod_path[i], insmod_soc_list[j]);
            lidbg_insmod(path);
        }
		
        for(j = 0; insmod_list[j] != NULL; j++)
        {
        	tmp = load_modules_count;
            sprintf(path, "%s%s", insmod_path[i], insmod_list[j]);
            //LIDBG_WARN("load %s\n",path);
            lidbg_insmod(path);
			while(tmp == load_modules_count) msleep(50);
        }
    }

    DUMP_FUN_LEAVE;
    return 0;

}

int __init loader_init(void)
{
    DUMP_BUILD_TIME;
    CREATE_KTHREAD(thread_loader, NULL);
    return 0;
}

void __exit loader_exit(void)
{}

module_init(loader_init);
module_exit(loader_exit);

EXPORT_SYMBOL(load_modules_count);
EXPORT_SYMBOL(lidbg_insmod);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");
