struct cmd_item
{
    char *cmd;
    void (*func)(int argc, char **argv);
};

struct cmd_item lidbg_cmd_item[] =
{
    {"wakelock", lidbg_wakelock_stat},
    {"mem", lidbg_mem_main},
    {"i2c", mod_i2c_main},
    {"io", mod_io_main},
    //{"spi",mod_spi_main},
    {"display", lidbg_display_main},
    {"key", lidbg_key_main},
    {"touch", lidbg_touch_main},
    {"soc", lidbg_soc_main},
    {"uart", lidbg_uart_main},
    {"servicer", lidbg_servicer_main},
    {"cmm", mod_cmn_main},
    {"file", lidbg_fileserver_main},
    {"trace_msg", trace_msg_main},
    {"mem_log", mem_log_main},
#ifndef USE_CALL_USERHELPER
    {"uevent", lidbg_uevent_main},
#endif
};

void parse_cmd(char *pt)
{
    int argc = 0;
    int i = 0;

    char *argv[32] = {NULL};
	argc = lidbg_token_string(pt, " ", argv);
	
    if(debug_mask)
    {
        lidbg("cmd:");
        while(i < argc)
        {
            printk(KERN_CRIT"%s ", argv[i]);
            i++;
        }
        lidbg("\n");
    }
	
    if (!strcmp(argv[0], "c"))
    {
        int new_argc, i;
        char **new_argv;
        new_argc = argc - 2;
        new_argv = argv + 2;

        if(argv[1] == NULL)
            return;

        for(i = 0; i < SIZE_OF_ARRAY(lidbg_cmd_item); i++)
        {
            if (!strcmp(argv[1], lidbg_cmd_item[i].cmd))
            {
                lidbg_cmd_item[i].func(new_argc, new_argv);
                break;
            }
        }
    }
}
