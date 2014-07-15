static bool encode = false;
int thread_dump_log(void *data)
{
    msleep(7000);
    fs_cp_data_to_udisk(encode);
    lidbg_domineering_ack();
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
            k2u_write(LOG_LOGCAT);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#002"))
        {
            lidbg_chmod("/data");
            k2u_write(LOG_DMESG);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#003"))
        {
            k2u_write(LOG_CLEAR_LOGCAT_KMSG);
            lidbg_domineering_ack();
        }
        else if (!strcmp(argv[1], "*158#004"))
        {
            k2u_write(LOG_SHELL_TOP_DF_PS);
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

}
