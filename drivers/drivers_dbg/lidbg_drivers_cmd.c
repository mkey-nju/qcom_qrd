#ifdef USE_CALL_USERHELPER
void lidbg_uevent_shell(char *shell_cmd)
{
}
#endif


static bool encode = false;
int thread_dump_log(void *data)
{
    msleep(7000);
    fs_cp_data_to_udisk(encode);
    lidbg_domineering_ack();
    return 0;
}
static bool logcat_enabled = false;
int thread_enable_logcat(void *data)
{
    char cmd[128] = {0};
    char logcat_file_name[256] = {0};
    char time_buf[32] = {0};

    if(logcat_enabled)
        goto out;
    logcat_enabled = true;

    lidbg("\n\n\nthread_enable_logcat:logcat+\n");

    lidbg_get_current_time(time_buf, NULL);
    sprintf(logcat_file_name, "logcat_%d_%s.txt", get_machine_id(), time_buf);

    sprintf(cmd, "date >/data/%s", logcat_file_name);
    lidbg_uevent_shell(cmd);
    memset(cmd, '\0', sizeof(cmd));
    ssleep(1);
    lidbg_uevent_shell("chmod 777 /data/logcat*");
    ssleep(1);
    sprintf(cmd, "logcat  -v time>> /data/%s &", logcat_file_name);
    lidbg_uevent_shell(cmd);
    lidbg("logcat-\n");
    return 0;
out:
    lidbg("logcat.skip\n");
    return 0;
}

static bool dmesg_enabled = false;
int thread_enable_dmesg(void *data)
{
    char cmd[128] = {0};
    char dmesg_file_name[256] = {0};
    char time_buf[32] = {0};

    if(dmesg_enabled)
        goto out;
    dmesg_enabled = true;

    lidbg("\n\n\nthread_enable_dmesg:kmsg+\n");

    lidbg_trace_msg_disable(1);
    lidbg_get_current_time(time_buf, NULL);
    sprintf(dmesg_file_name, "kmsg_%d_%s.txt", get_machine_id(), time_buf);

    sprintf(cmd, "date >/data/%s", dmesg_file_name);
    lidbg_uevent_shell(cmd);
    memset(cmd, '\0', sizeof(cmd));
    ssleep(1);
    lidbg_uevent_shell("chmod 777 /data/kmsg*");
    ssleep(1);
    sprintf(cmd, "cat /proc/kmsg >> /data/%s &", dmesg_file_name);
    lidbg_uevent_shell(cmd);
    lidbg("kmsg-\n");
    return 0;
out:
    lidbg("kmsg.skip\n");
    return 0;
}

void parse_cmd(char *pt)
{
    int argc = 0;
    int i = 0;

    char *argv[32] = {NULL};
    argc = lidbg_token_string(pt, " ", argv);

    lidbg("cmd:");
    while(i < argc)
    {
        printk(KERN_CRIT"%s ", argv[i]);
        i++;
    }
    lidbg("\n");

    if (!strcmp(argv[0], "appcmd"))
    {
        lidbg("%s:[%s]\n", argv[0], argv[1]);

        if (!strcmp(argv[1], "*158#000"))
        {
            //*#*#158999#*#*
            lidbg_chmod("/data");
            fs_mem_log("*158#999--fs_call_apk\n");
            fs_mem_log("*158#001--LOG_LOGCAT\n");
            fs_mem_log("*158#002--LOG_DMESG\n");
            fs_mem_log("*158#003--LOG_CLEAR_LOGCAT_KMSG\n");
            fs_mem_log("*158#004--LOG_SHELL_TOP_DF_PS\n");
            fs_mem_log("*158#010--USB_ID_LOW_HOST\n");
            fs_mem_log("*158#011--USB_ID_HIGH_DEV\n");
            fs_mem_log("*158#012--lidbg_trace_msg_disable\n");
            fs_mem_log("*158#013--dump log and copy to udisk\n");
        }

        if (!strcmp(argv[1], "*158#999"))
        {
            is_fs_work_enable = true;
            g_is_te_enable = 1;
            fs_call_apk();
        }
        else if (!strcmp(argv[1], "*158#001"))
        {
            lidbg_chmod("/data");

#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_LOGCAT);
#else
            CREATE_KTHREAD(thread_enable_logcat, NULL);
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#002"))
        {
            lidbg_chmod("/data");

#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_DMESG);
#else
            CREATE_KTHREAD(thread_enable_dmesg, NULL);
#endif
            lidbg_domineering_ack();

        }
        else if (!strcmp(argv[1], "*158#003"))
        {
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_CLEAR_LOGCAT_KMSG);
#else
            lidbg("clear+logcat*&&kmsg*\n");
            lidbg_uevent_shell("rm /data/logcat*");
            lidbg_uevent_shell("rm /data/kmsg*");
            lidbg("clear-logcat*&&kmsg*\n");
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#004"))
        {
#ifdef USE_CALL_USERHELPER
            k2u_write(LOG_SHELL_TOP_DF_PS);
#else
            lidbg("\n\nLOG_SHELL_TOP_DF_PS+\n");
            lidbg_uevent_shell("date > /data/machine.txt");
            lidbg_uevent_shell("cat /proc/cmdline >> /data/machine.txt");
            lidbg_uevent_shell("getprop fly.version.mcu >> /data/machine.txt");
            lidbg_uevent_shell("top -n 3 -t >/data/top.txt &");
            lidbg_uevent_shell("screencap -p /data/screenshot.png &");
            lidbg_uevent_shell("ps > /data/ps.txt");
            lidbg_uevent_shell("df > /data/df.txt");
            lidbg_uevent_shell("chmod 777 /data/*.txt");
            lidbg_uevent_shell("chmod 777 /data/*.png");
            lidbg("\n\nLOG_SHELL_TOP_DF_PS-\n");
#endif
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#010"))
        {
            lidbg("USB_ID_LOW_HOST\n");
            USB_ID_LOW_HOST;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#011"))
        {
            lidbg("USB_ID_HIGH_DEV\n");
            USB_ID_HIGH_DEV;
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#012"))
        {
            lidbg_trace_msg_disable(1);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#013"))
        {
            encode = false;
            lidbg_chmod("/data");
            lidbg_fifo_get(glidbg_msg_fifo, LIDBG_LOG_DIR"lidbg_mem_log.txt", 0);
            CREATE_KTHREAD(thread_dump_log, NULL);
        }
        else if (!strcmp(argv[1], "*168#001"))
        {
            encode = true;
            lidbg_chmod("/data");
            lidbg_fifo_get(glidbg_msg_fifo, LIDBG_LOG_DIR"lidbg_mem_log.txt", 0);
            CREATE_KTHREAD(thread_dump_log, NULL);
        }

    }
    else if(!strcmp(argv[0], "monkey") )
    {
        int enable, gpio, on_en, off_en, on_ms, off_ms;
        enable = simple_strtoul(argv[1], 0, 0);
        gpio = simple_strtoul(argv[2], 0, 0);
        on_en = simple_strtoul(argv[3], 0, 0);
        off_en = simple_strtoul(argv[4], 0, 0);
        on_ms = simple_strtoul(argv[5], 0, 0);
        off_ms = simple_strtoul(argv[6], 0, 0);
        monkey_run(enable);
        monkey_config(gpio, on_en, off_en, on_ms, off_ms);
    }
#ifndef SOC_msm8x25
    else if(!strcmp(argv[0], "pm") )
    {
        int enable;
        enable = simple_strtoul(argv[1], 0, 0);
        SOC_PM_STEP((fly_pm_stat_step)enable, NULL);
    }
    else if(!strcmp(argv[0], "pm1") )
    {
        int enable;
        enable = simple_strtoul(argv[1], 0, 0);
        LINUX_TO_LIDBG_TRANSFER((linux_to_lidbg_transfer_t)enable, NULL);
    }
    else if (!strcmp(argv[0], "lpc"))
    {
    		int para_count = argc -1;
		u8 lpc_buf[10]={0};
		for(i=0; i<para_count; i++)
		{
			lpc_buf[i] = simple_strtoul(argv[i+1], 0, 0);
			lidbg("%d ", lpc_buf[i]);
		}
		lidbg("para_count = %d\n", para_count);
		SOC_LPC_Send(lpc_buf, para_count);
    }	
#endif

#ifdef SOC_msm8x25
    else if(!strcmp(argv[0], "video"))
    {
        int new_argc;
        char **new_argv;
        new_argc = argc - 1;
        new_argv = argv + 1;
        lidbg_video_main(new_argc, new_argv);
    }
#endif
}
