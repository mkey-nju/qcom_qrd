
#include "lidbg.h"

LIDBG_DEFINE;

static int ts_scan_delayms = 500;
static int is_warned = 0;
#define TS_I2C_BUS (1)

#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
#define FLYHAL_CONFIG_PATH "/flydata/flyhalconfig"
#else
#define FLYHAL_CONFIG_PATH "/flysystem/flyconfig/default/lidbgconfig/flylidbgconfig.txt"
#endif

static LIST_HEAD(flyhal_config_list);
int ts_should_revert = -1;

struct probe_device
{
    char chip_addr;
    unsigned int sub_addr;
    int cmd;
    char *name;
};

struct probe_device ts_probe_dev[] =
{
    {0x5d, 0x00, LOG_CAP_TS_GT811, "gt811.ko"},
    {0x55, 0x00, LOG_CAP_TS_GT801, "gt801.ko"},
    {0x14, 0x00, LOG_CAP_TS_GT911, "gt911.ko"},
    //{0x5d, 0x00, LOG_CAP_TS_GT910,"gt910new.ko"}, //flycar
};

bool scan_on = 1;
bool is_ts_load = 0;

/*add 2 variable to make  logic of update 801*/
unsigned int shutdown_flag_ts = 0;
unsigned int shutdown_flag_probe = 0;
unsigned int shutdown_flag_gt811 = 0;
unsigned int irq_signal = 0;

void ts_scan(void)
{
    static unsigned int loop = 0;
    int i;
    int32_t rc1, rc2;
    u8 tmp;
    char path[100];

    for(i = 0; i < SIZE_OF_ARRAY(ts_probe_dev); i++)
    {

        rc1 = SOC_I2C_Rec_Simple(TS_I2C_BUS, ts_probe_dev[i].chip_addr, &tmp, 1 );
        rc2 = SOC_I2C_Rec(TS_I2C_BUS, ts_probe_dev[i].chip_addr, ts_probe_dev[i].sub_addr, &tmp, 1 );

        lidbg("rc1=%x,rc2=%x,ts_scan_delayms=%d\n", rc1, rc2, ts_scan_delayms);

        if ((rc1 < 0) && (rc2 < 0))
            lidbg("i2c_addr 0x%x probe fail!\n", ts_probe_dev[i].chip_addr);
        else
        {
            lidbg("i2c_addr 0x%x found!\n", ts_probe_dev[i].chip_addr);
            scan_on = 0;
            SOC_I2C_Rec(TS_I2C_BUS, 0x12, 0x00, &tmp, 1 ); //let i2c bus release


            sprintf(path, "/system/lib/modules/out/%s", ts_probe_dev[i].name);
            lidbg_insmod( path );

            sprintf(path, "/flysystem/lib/out/%s", ts_probe_dev[i].name);
            lidbg_insmod( path );

            //in V3+,check ts revert and save the ts sate.
            if(0 == is_warned)
            {
                is_warned = 1;
                fs_mem_log("loadts=%s\n", ts_probe_dev[i].name);
                lidbg_fs_log(TS_LOG_PATH, "loadts=%s\n", ts_probe_dev[i].name);
                ts_should_revert = fs_find_string(&flyhal_config_list, "TSMODE_XYREVERT");
                if(ts_should_revert > 0)
                    lidbg("[futengfei]=======================TS.XY will revert\n");
                else
                    lidbg("[futengfei]=======================TS.XY will normal\n");
            }
            if (!strcmp(ts_probe_dev[i].name, "gt801.ko"))
            {
                lidbg_insmod("/system/lib/modules/out/gt80x_update.ko");
                lidbg_insmod("/flysystem/lib/out/gt80x_update.ko");
            }
            break;
        }
    }

    loop++;
    lidbg("ts_scan_time:%d\n", loop);

#if (defined(BOARD_V1) || defined(BOARD_V2))
    if(loop == 10)
    {
        lidbg_insmod("/system/lib/modules/out/gt80x_update.ko");
        lidbg_insmod("/flysystem/lib/out/gt80x_update.ko");
    }
#endif

    //if(loop > 10) {scan_on=0;}
}

int ts_probe_thread(void *data)
{

    fs_fill_list(FLYHAL_CONFIG_PATH, FS_CMD_FILE_LISTMODE, &flyhal_config_list);
    fs_get_intvalue(&lidbg_drivers_list, "ts_scan_delayms", &ts_scan_delayms, NULL);
    if(ts_scan_delayms < 100)
        ts_scan_delayms = 100;

    lidbg_insmod("/system/lib/modules/out/lidbg_ts_to_recov.ko");
    lidbg_insmod("/flysystem/lib/out/lidbg_ts_to_recov.ko");

    while(1)
    {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if(kthread_should_stop())
        {
            break;
        }

        if(shutdown_flag_probe == 0)
        {
            lidbg("[wang]:======begin to probe ts_driver.\n");
            if(scan_on == 1)
            {
                SOC_IO_Output(0, 27, 1);
                msleep(ts_scan_delayms);
                ts_scan();
                if(scan_on == 1)
                {
                    SOC_IO_Output(0, 27, 0);
                    msleep(ts_scan_delayms);
                    ts_scan();
                    if(scan_on == 0)
                        shutdown_flag_gt811 = 1;
                    else
                        shutdown_flag_gt811 = 0;
                }

            }
            else
            {
                ssleep(6);//3//6s later,if ts not load and scan again.
                if(is_ts_load == 0)
                {
                    scan_on = 1;
                    lidbg("ts not load,scan again...\n");
                }
                else
                    break;
            }
        }
        else
        {
            //lidbg("[wang]:=========is in updating.\n");
            shutdown_flag_probe = 2;
            msleep(ts_scan_delayms);

        }
    }
    return 0;
}

static int ts_probe_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
    CREATE_KTHREAD(ts_probe_thread, NULL);

    return 0;
}

static void ts_probe_exit(void) {}

module_init(ts_probe_init);
module_exit(ts_probe_exit);

EXPORT_SYMBOL(is_ts_load);
EXPORT_SYMBOL(ts_should_revert);
/******************************************************
 		add Logic for goodix801 update
*******************************************************/
EXPORT_SYMBOL(shutdown_flag_ts);
EXPORT_SYMBOL(shutdown_flag_probe);
EXPORT_SYMBOL(shutdown_flag_gt811);
EXPORT_SYMBOL(irq_signal);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("lsw.");
