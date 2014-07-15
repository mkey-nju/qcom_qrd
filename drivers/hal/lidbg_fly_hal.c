/* Copyright (c) 2012, swlee
 *
 */
#define LIDBG_FLY_HAL

#include "lidbg.h"
bool recovery_mode = 0;
LIDBG_DEFINE;
#define HAL_SO "/flysystem/lib/hw/flyfa.default.so"

//struct task_struct *soc_task;

int SOC_Get_CpuFreq(void);

char *insmod_list[] =
{
    "lidbg_lpc.ko",
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
    "lidbg_fastboot.ko",
#else
    "lidbg_acc.ko",
#endif
    "lidbg_devices.ko",
    "lidbg_bpmsg.ko",
    "lidbg_gps.ko",
    "lidbg_videoin.ko",
#if (defined(BOARD_V1) || defined(BOARD_V2) || defined(BOARD_V3))
    "lidbg_ts_probe.ko",
#else
    "lidbg_ts_probe_new.ko",
#endif
    "lidbg_monkey.ko",
    "lidbg_drivers_dbg.ko",
    NULL,
};

char *insmod_path[] =
{
    "/system/lib/modules/out/",
    "/flysystem/lib/out/",
    NULL,
};


void hal_func_tbl_default(void)
{
    lidbgerr("hal_func_tbl_default:this func not ready!\n");
    //print who call this
    dump_stack();

}

int loader_thread(void *data)
{
    int i, j;
    char path[100];
#if (defined(BOARD_V1) || defined(BOARD_V2))
    lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", NULL, "600000", sizeof("600000") - 1);
#else
    lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", NULL, "700800", sizeof("700800") - 1);
#endif
    SOC_Get_CpuFreq();
    if( fs_is_file_exist(RECOVERY_MODE_DIR))
        recovery_mode = 1;
    else
        recovery_mode = 0;
    for(i = 0; insmod_path[i] != NULL; i++)
    {
        for(j = 0; insmod_list[j] != NULL; j++)
        {
            sprintf(path, "%s%s", insmod_path[i], insmod_list[j]);
            //lidbg("load %s\n",path);
            lidbg_insmod(path);
        }
    }

#if (defined(BOARD_V1) || defined(BOARD_V2)  || defined(BOARD_V4))

#else
    msleep(1000);
    if(lidbg_exe("/flysystem/lib/out/lidbg_servicer", NULL, NULL, NULL, NULL, NULL, NULL) < 0)
        lidbg_exe("/system/lib/modules/out/lidbg_servicer", NULL, NULL, NULL, NULL, NULL, NULL);
#endif
    return 0;
}



bool SOC_IO_ISR_Add(u32 irq, u32  interrupt_type, pinterrupt_isr func, void *dev)
{
    bool ret = 0;
    struct io_int_config io_int_config1;

    io_int_config1.ext_int_num = MSM_GPIO_TO_INT(irq);
    io_int_config1.irqflags = interrupt_type;
    io_int_config1.pisr = func;
    io_int_config1.dev = dev;

    lidbg("ext_int_num:%d \n", irq);

    ret =  soc_io_irq(&io_int_config1);
    return ret;
}

bool SOC_IO_ISR_Enable(u32 irq)
{
    soc_irq_enable(MSM_GPIO_TO_INT(irq));
    return 1;
}


bool SOC_IO_ISR_Disable(u32 irq)
{
    soc_irq_disable(MSM_GPIO_TO_INT(irq));
    return 1;
}
bool SOC_IO_ISR_Del (u32 irq)
{
    free_irq(MSM_GPIO_TO_INT(irq), NULL);
    return 1;
}


bool SOC_IO_Config(u32 index, bool direction, u32 pull, u32 drive_strength)
{
    return soc_io_config( index,  direction, pull, drive_strength, 1);
}

void SOC_IO_Output_Ext(u32 group, u32 index, bool status, u32 pull, u32 drive_strength)
{
    soc_io_config( index,  GPIO_CFG_OUTPUT, pull, drive_strength, 1);
    soc_io_output(group, index,  status);
}

void SOC_IO_Output(u32 group, u32 index, bool status)
{
    soc_io_config( index,  GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA, 0);
    soc_io_output(group, index,  status);
}

bool SOC_IO_Input(u32 group, u32 index, u32 pull)
{
    soc_io_config( index,  GPIO_CFG_INPUT, pull/*GPIO_CFG_NO_PULL*/, GPIO_CFG_16MA, 0);
    return soc_io_input(index);
}

bool SOC_ADC_Get (u32 channel , u32 *value)
{
    *value = 0xffffffff;

    *value = soc_ad_read(channel);

    if(*value == 0xffffffff)
        return 0;
    return 1;
}

void SOC_Key_Report(u32 key_value, u32 type)
{
    lidbg_key_report(key_value, type);
}

// 7bit i2c sub_addr
int SOC_I2C_Send(int bus_id, char chip_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_send( bus_id,  chip_addr,  0,  buf,  size);
}
int SOC_I2C_Rec(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int SOC_I2C_Rec_2B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_sub_addr_2bytes( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int SOC_I2C_Rec_3B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_sub_addr_3bytes( bus_id,  chip_addr,  sub_addr, buf,  size);
}

int SOC_I2C_Rec_Simple(int bus_id, char chip_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_no_sub_addr( bus_id,  chip_addr,  0, buf,  size);
}

int i2c_api_do_recv_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);
int i2c_api_do_recv_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size);

int SOC_I2C_Rec_SAF7741(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_SAF7741( bus_id,  chip_addr, sub_addr, buf,  size);
}

int SOC_I2C_Send_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_send_TEF7000( bus_id,  chip_addr, sub_addr, buf,  size);
}

int SOC_I2C_Rec_TEF7000(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
{
    return  i2c_api_do_recv_TEF7000( bus_id,  chip_addr, sub_addr, buf,  size);
}

void SOC_PWM_Set(int pwm_id, int duty_ns, int period_ns)
{
    soc_pwm_set(pwm_id, duty_ns, period_ns);
}

//0~255
int SOC_BL_Set( u32 bl_level)
{
    soc_bl_set(bl_level);
    return 1;

}

void SOC_LCD_Reset(void)
{
#if 0
    SOC_BL_Set(BL_MIN);
    msleep(20);// bp loop time
    LCD_RESET;
    msleep(20);//wait lcd ready
    SOC_BL_Set(BL_MAX);
#endif
}



void SOC_Write_Servicer(int cmd)
{
    k2u_write(cmd);
}


int SOC_Display_Get_Res(u32 *screen_x, u32 *screen_y)
{
    return soc_get_screen_res(screen_x, screen_y);
}

void SOC_Mic_Enable( bool enable)
{}
//I2c_Rate	 i2c_api_set_rate(int  bus_id, int rate)
//int (*pfnSOC_I2C_Set_Rate)(int  bus_id, int rate);
int SOC_I2C_Set_Rate(int  bus_id, int rate)
{
    return		 i2c_api_set_rate(bus_id, rate);
}

void SOC_IO_Uart_Cfg(u32 baud)
{
    soc_io_uart_cfg(baud);
}

void SOC_IO_Uart_Send( u32 baud, const char *fmt, ... )
{
    va_list args;
    int n;
    char printbuffer[256];

    va_start ( args, fmt );
    n = vsprintf ( printbuffer, (const char *)fmt, args );
    va_end ( args );
    soc_io_uart_send(baud, (unsigned char *)printbuffer);

}


struct fly_smem *SOC_Get_Share_Mem(void)
{
    return p_fly_smem;
}


void SOC_System_Status(FLY_SYSTEM_STATUS status)
{
    lidbg("SOC_System_Status=%d\n", status);
    g_var.system_status = status;

    lidbg_notifier_call_chain(NOTIFIER_VALUE(NOTIFIER_MAJOR_SYSTEM_STATUS_CHANGE, g_var.system_status));
#if 0
    if(g_var.system_status == FLY_ACC_OFF)
    {
        // stop gps i2c read
        lidbg_readwrite_file("/sys/module/lidbg_gps/parameters/work_en", NULL, "0", sizeof("0") - 1);
    }
    else if(g_var.system_status == FLY_ACC_ON)
    {
        lidbg_readwrite_file("/sys/module/lidbg_gps/parameters/work_en", NULL, "1", sizeof("1") - 1);
    }
#endif
}


int SOC_Get_CpuFreq(void)
{
    char buf[16];
    int cpu_freq;
    lidbg_readwrite_file("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq", buf, NULL, 16);
    cpu_freq = simple_strtoul(buf, 0, 0);
    g_var.cpu_freq = cpu_freq;
    lidbg("cpufreq=%d\n", cpu_freq);

    return cpu_freq;
}




static void set_func_tbl(void)
{
    //io
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Output = SOC_IO_Output;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Input = SOC_IO_Input;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Output_Ext = SOC_IO_Output_Ext;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Config = SOC_IO_Config;

    //i2c
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send = SOC_I2C_Send;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec = SOC_I2C_Rec;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_Simple = SOC_I2C_Rec_Simple;
    //add by huangzongqiang	SOC_I2C_Rec_2B_SubAddr(int bus_id, char chip_addr, unsigned int sub_addr, char *buf, unsigned int size)
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_2B_SubAddr = SOC_I2C_Rec_2B_SubAddr;
    //I2c_Rate	 i2c_api_set_rate(int  bus_id, int rate)
    //int (*pfnSOC_I2C_Set_Rate)(int  bus_id, int rate);
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Set_Rate = SOC_I2C_Set_Rate;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_SAF7741 = SOC_I2C_Rec_SAF7741;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send_TEF7000 = SOC_I2C_Send_TEF7000;
    plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_TEF7000 = SOC_I2C_Rec_TEF7000;

    //io-irq
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Add = SOC_IO_ISR_Add;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Enable = SOC_IO_ISR_Enable;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Disable = SOC_IO_ISR_Disable;
    plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Del = SOC_IO_ISR_Del;

    //ad
    plidbg_dev->soc_func_tbl.pfnSOC_ADC_Get = SOC_ADC_Get;

    //key
    plidbg_dev->soc_func_tbl.pfnSOC_Key_Report = SOC_Key_Report;

    //bl
    plidbg_dev->soc_func_tbl.pfnSOC_BL_Set = SOC_BL_Set;

    //
    plidbg_dev->soc_func_tbl.pfnSOC_Write_Servicer = SOC_Write_Servicer;
    //video

    //display/touch
    plidbg_dev->soc_func_tbl.pfnSOC_Display_Get_Res = SOC_Display_Get_Res;

    //mic
    plidbg_dev->soc_func_tbl.pfnSOC_Mic_Enable = SOC_Mic_Enable;

    // uart
    plidbg_dev->soc_func_tbl.pfnSOC_IO_Uart_Send = SOC_IO_Uart_Send;

    plidbg_dev->soc_func_tbl.pfnSOC_Get_Share_Mem = SOC_Get_Share_Mem;
    plidbg_dev->soc_func_tbl.pfnSOC_System_Status = SOC_System_Status;

    plidbg_dev->soc_func_tbl.pfnSOC_Get_CpuFreq = SOC_Get_CpuFreq;

    plidbg_dev->soc_func_tbl.pfnSOC_LCD_Reset = SOC_LCD_Reset;

    plidbg_dev->soc_func_tbl.pfnSOC_WakeLock_Stat = lidbg_wakelock_register;


    plidbg_dev->soc_func_tbl.pfnNotifier_Call = lidbg_notifier_call_chain;

}

int hal_open(struct inode *inode, struct file *filp)
{
    return 0;
}

int hal_release(struct inode *inode, struct file *filp)
{
    return 0;
}


ssize_t hal_read(struct file *filp, char __user *buf, size_t size,
                 loff_t *ppos)
{
    unsigned int count = 4;
    int ret = 0;
    u32 read_value = 0;
    read_value = (u32)plidbg_dev;

    lidbg("hal_read:read_value=%x,read_count=%d\n", (u32)read_value, count);
    if (copy_to_user(buf, &read_value, count))
    {
        ret =  - EFAULT;
    }
    else
    {
        ret = count;
    }

    return count;
}


#include "cmd.c"
static ssize_t hal_write(struct file *filp, const char __user *buf,
                         size_t size, loff_t *ppos)
{
    char cmd_buf[512];
    memset(cmd_buf, '\0', 512);

    if(copy_from_user(cmd_buf, buf, size))
    {
        lidbg("copy_from_user ERR\n");
    }
    parse_cmd(cmd_buf);
    return size;
}


#define DEVICE_NAME "lidbg_hal"

static struct file_operations dev_fops =
{
    .owner	=	THIS_MODULE,
    .open   =   hal_open,
    .read   =   hal_read,
    .write   =   hal_write,
    .release =  hal_release,
};


static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,

};

int hal_init(void *data)
{
    if(fs_is_file_exist(HAL_SO))
    {
        lidbg("=======is product=====\n");
        g_var.is_fly = true;
    }
    msleep(500);
    lidbg_chmod("/dev/lidbg_hal");
    msleep(500);

    CREATE_KTHREAD(loader_thread, NULL);
    return 0;
}


int fly_hal_init(void)
{
    int ret;
    DUMP_BUILD_TIME;
    ret = misc_register(&misc);

    plidbg_dev = kmalloc(sizeof(struct lidbg_hal), GFP_KERNEL);
    if (plidbg_dev == NULL)
    {

        LIDBG_ERR("kmalloc.\n");
        return 0;
    }
    {
        int i;
        for(i = 0; i < sizeof(plidbg_dev->soc_func_tbl) / 4; i++)
        {
            ((int *) & (plidbg_dev->soc_func_tbl))[i] = hal_func_tbl_default;

        }
    }
    memset(&(plidbg_dev->soc_pvar_tbl), (int)NULL, sizeof(struct lidbg_pvar_t));

    set_func_tbl();

    g_var.temp = 0;
    g_var.system_status = FLY_ACC_ON;
    g_var.machine_id = get_machine_id();


    g_var.is_fly = 1;
    g_var.fake_suspend = 0;
    g_var.acc_flag = 1;

    CREATE_KTHREAD(hal_init, NULL);

    return 0;
}

void fly_hal_deinit(void)
{
}

module_init(fly_hal_init);
module_exit(fly_hal_deinit);

EXPORT_SYMBOL(recovery_mode);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");


