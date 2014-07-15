/* Copyright (c) 2012, swlee
 *
 */
#include "lidbg.h"

LIDBG_DEFINE;
static bool lpc_work_en = true;


#define  I2_ID  (0)

#ifdef SOC_msm8x25
#define  MCU_WP_GPIO 	  (29)
#define  MCU_IIC_REQ_GPIO (30)
#elif defined(SOC_msm8x26)
#define  MCU_WP_GPIO      (107)
#define  MCU_IIC_REQ_GPIO (108)
#endif



#define MCU_WP_GPIO_SET  do{SOC_IO_Output(0, MCU_WP_GPIO, 1); }while(0)


#define DATA_BUFF_LENGTH_FROM_MCU   (128)

#define BYTE u8
#define UINT u32
#define UINT32 u32
#define BOOL bool
#define ULONG u32

#define FALSE 0
#define TRUE 1

#define LPC_SYSTEM_TYPE 0x00

typedef struct _FLY_IIC_INFO
{
    struct work_struct iic_work;
} FLY_IIC_INFO, *P_FLY_IIC_INFO;



struct fly_hardware_info
{

    FLY_IIC_INFO FlyIICInfo;

    BYTE buffFromMCU[DATA_BUFF_LENGTH_FROM_MCU];
    BYTE buffFromMCUProcessorStatus;
    UINT buffFromMCUFrameLength;
    UINT buffFromMCUFrameLengthMax;
    BYTE buffFromMCUCRC;
    BYTE buffFromMCUBak[DATA_BUFF_LENGTH_FROM_MCU];

};

#define  MCU_ADDR_W  0xA0
#define  MCU_ADDR_R  0xA1


//#define LPC_DEBUG_LOG

#ifdef CONFIG_HAS_EARLYSUSPEND
static void lpc_early_suspend(struct early_suspend *handler);
static void lpc_late_resume(struct early_suspend *handler);
struct early_suspend early_suspend;
#endif

struct fly_hardware_info GlobalHardwareInfo;
struct fly_hardware_info *pGlobalHardwareInfo;

int thread_lpc(void *data)
{
    while(1)
    {
        if(lpc_work_en)
            LPC_CMD_NO_RESET;
        msleep(5000);
    }
    return 0;
}

void LPCCombinDataStream(BYTE *p, UINT len)
{
    UINT i = 0;
    BYTE checksum = 0;
    BYTE bufData[16];
    BYTE *buf;
    bool bMalloc = FALSE;

    if(!lpc_work_en)
        return;

    if (3 + len + 1 > 16)
    {
        buf = (BYTE *)kmalloc(sizeof(BYTE) * (4 + len), GFP_KERNEL);
        if (buf == NULL)
        {
            LIDBG_ERR("kmalloc.\n");
        }
        bMalloc = TRUE;
    }
    else
    {
        buf = bufData;
    }

    buf[0] = 0xFF;
    buf[1] = 0x55;
    buf[2] = len + 1;
    checksum = buf[2];
    for (i = 0; i < len; i++)
    {
        buf[3 + i] = p[i];
        checksum += p[i];
    }

    buf[3 + i] = checksum;
#ifdef LPC_DEBUG_LOG
    lidbg("ToMCU:%x %x %x\n", p[0], p[1], p[2]);
#endif
    SOC_I2C_Send(I2_ID, MCU_ADDR_W >> 1, buf, 3 + i + 1);

    if (bMalloc)
    {
        kfree(buf);
        buf = NULL;
    }
}


static void LPCdealReadFromMCUAll(BYTE *p, UINT length)
{
#if 1
#ifdef LPC_DEBUG_LOG
    {
        u32 i;
        lidbg("From LPC:");//mode ,command,para
        for(i = 0; i < length; i++)
        {
            printk(KERN_CRIT"%x ", p[i]);

        }
        lidbg("\n");
    }
#endif

    switch (p[0])
    {
    case LPC_SYSTEM_TYPE:
        break;
    case 0x96:
        switch (p[2])
        {
        case 0x7f:
#ifdef LPC_DEBUG_LOG
            lidbg("LPC ping return!\n");
#endif
            break;
        }
    default:
        break;
    }
#endif
}

static BOOL readFromMCUProcessor(BYTE *p, UINT length)
{
    UINT i;

    for (i = 0; i < length; i++)
    {
        switch (pGlobalHardwareInfo->buffFromMCUProcessorStatus)
        {
        case 0:
            if (0xFF == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
            }
            break;
        case 1:
            if (0xFF == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 1;
            }
            else if (0x55 == p[i])
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 2;
            }
            else
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
            }
            break;
        case 2:
            pGlobalHardwareInfo->buffFromMCUProcessorStatus = 3;
            pGlobalHardwareInfo->buffFromMCUFrameLength = 0;
            pGlobalHardwareInfo->buffFromMCUFrameLengthMax = p[i];
            pGlobalHardwareInfo->buffFromMCUCRC = p[i];
            break;
        case 3:
            if (pGlobalHardwareInfo->buffFromMCUFrameLength < (pGlobalHardwareInfo->buffFromMCUFrameLengthMax - 1))
            {
                pGlobalHardwareInfo->buffFromMCU[pGlobalHardwareInfo->buffFromMCUFrameLength] = p[i];
                pGlobalHardwareInfo->buffFromMCUCRC += p[i];
                pGlobalHardwareInfo->buffFromMCUFrameLength++;
            }
            else
            {
                pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
                if (pGlobalHardwareInfo->buffFromMCUCRC == p[i])
                {
                    LPCdealReadFromMCUAll(pGlobalHardwareInfo->buffFromMCU, pGlobalHardwareInfo->buffFromMCUFrameLengthMax - 1);
                }
                else
                {
                    lidbg("\nRead From MCU CRC Error");
                }
            }
            break;
        default:
            pGlobalHardwareInfo->buffFromMCUProcessorStatus = 0;
            break;
        }
    }

    if (pGlobalHardwareInfo->buffFromMCUProcessorStatus > 1)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL actualReadFromMCU(BYTE *p, UINT length)
{

    if(!lpc_work_en)
        return FALSE;

    SOC_I2C_Rec_Simple(I2_ID, MCU_ADDR_R >> 1, p, length);
    if (readFromMCUProcessor(p, length))
    {

#ifdef LPC_DEBUG_LOG
        lidbg("More ");
#endif
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//MCUµÄIIC¶Á´¦Àí
irqreturn_t MCUIIC_isr(int irq, void *dev_id)
{
    schedule_work(&pGlobalHardwareInfo->FlyIICInfo.iic_work);
    return IRQ_HANDLED;
}

static void workFlyMCUIIC(struct work_struct *work)
{
    BYTE buff[16];
    BYTE iReadLen = 12;

    while (SOC_IO_Input(MCU_IIC_REQ_GPIO, MCU_IIC_REQ_GPIO, 0) == 0)
    {
        actualReadFromMCU(buff, iReadLen);
        iReadLen = 16;
    }
}


void mcuFirstInit(void)
{
    pGlobalHardwareInfo = &GlobalHardwareInfo;
    INIT_WORK(&pGlobalHardwareInfo->FlyIICInfo.iic_work, workFlyMCUIIC);
    MCU_WP_GPIO_SET;

    //let i2c_c high
    while (SOC_IO_Input(0, MCU_IIC_REQ_GPIO, GPIO_CFG_PULL_UP) == 0)
    {
        u8 buff[32];
        static int count = 0;
        count++;
        WHILE_ENTER;
        actualReadFromMCU(buff, 32);
        if(count > 100)
        {
            lidbg("exit mcuFirstInit!\n");
            break;
        }
        msleep(100);
    }
    SOC_IO_ISR_Add(MCU_IIC_REQ_GPIO, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, MCUIIC_isr, pGlobalHardwareInfo);

    CREATE_KTHREAD(thread_lpc, NULL);

}


void LPCSuspend(void)
{

    SOC_IO_ISR_Disable(MCU_IIC_REQ_GPIO);
}

void LPCResume(void)
{
    BYTE buff[16];
    BYTE iReadLen = 12;

    SOC_IO_ISR_Enable(MCU_IIC_REQ_GPIO);

    //clear lpc i2c buffer
    while (SOC_IO_Input(MCU_IIC_REQ_GPIO, MCU_IIC_REQ_GPIO, 0) == 0)
    {
        WHILE_ENTER;
        actualReadFromMCU(buff, iReadLen);
        iReadLen = 16;
    }
}


static int  lpc_probe(struct platform_device *pdev)
{
    DUMP_FUN;

    if(g_var.is_fly)
    {
        lidbg("lpc_init do nothing\n");
        return 0;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    early_suspend.suspend = lpc_early_suspend;
    early_suspend.resume = lpc_late_resume;
    register_early_suspend(&early_suspend);
#endif

    mcuFirstInit();
    return 0;
}


static int  lpc_remove(struct platform_device *pdev)
{
    return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void lpc_early_suspend(struct early_suspend *handler) {}
static void lpc_late_resume(struct early_suspend *handler) {}
#endif

static int thread_lpc_delay_en(void *data)
{
    msleep(1000);
    lpc_work_en=true;
    return 1;
}

#ifdef CONFIG_PM
static int lpc_suspend(struct device *dev)
{
    DUMP_FUN;
    lpc_work_en = false;
    return 0;
}

static int lpc_resume(struct device *dev)
{
    DUMP_FUN;
    CREATE_KTHREAD(thread_lpc_delay_en, NULL);
    return 0;
}

static struct dev_pm_ops lpc_pm_ops =
{
    .suspend	= lpc_suspend,
    .resume		= lpc_resume,
};
#endif


static struct platform_device lidbg_lpc =
{
    .name               = "lidbg_lpc",
    .id                 = -1,
};

static struct platform_driver lpc_driver =
{
    .probe		= lpc_probe,
    .remove     = lpc_remove,
    .driver         = {
        .name = "lidbg_lpc",
        .owner = THIS_MODULE,
#ifdef CONFIG_PM
        .pm = &lpc_pm_ops,
#endif
    },
};


static void set_func_tbl(void)
{
#ifdef SOC_msm8x25
    ((struct lidbg_hal *)plidbg_dev)->soc_func_tbl.pfnSOC_LPC_Send = LPCCombinDataStream;
#else
    ((struct lidbg_interface *)plidbg_dev)->soc_func_tbl.pfnSOC_LPC_Send = LPCCombinDataStream;
#endif

}

static int __init lpc_init(void)
{
    DUMP_BUILD_TIME;
    LIDBG_GET;
    set_func_tbl();
    platform_device_register(&lidbg_lpc);
    platform_driver_register(&lpc_driver);
    return 0;
}

static void __exit lpc_exit(void)
{
    platform_driver_unregister(&lpc_driver);
}


module_init(lpc_init);
module_exit(lpc_exit);


MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("lidbg lpc driver");


