#ifndef _LIGDBG_FUNCDEF__
#define _LIGDBG_FUNCDEF__

extern struct lidbg_hal *plidbg_dev;

static inline int check_pt(void)
{
    while (plidbg_dev == NULL)
    {
        printk(KERN_CRIT"lidbg:check if plidbg_dev==NULL\n");
        printk(KERN_CRIT"%s,line %d\n", __FILE__, __LINE__);
        dump_stack();//provide some information
        msleep(200);
    }
    return 0;
}


//#define lidbg_version_show (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnlidbg_version_show))

#define SOC_IO_Output (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Output))
#define SOC_IO_Input  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Input))
#define SOC_IO_Output_Ext (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Output_Ext))
#define SOC_IO_Config  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Config))
//i2c
#define SOC_I2C_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send))
#define SOC_I2C_Rec   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec))
#define SOC_I2C_Rec_Simple   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_Simple))

#define SOC_I2C_Rec_SAF7741  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_SAF7741))
#define SOC_I2C_Send_TEF7000   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Send_TEF7000))
#define SOC_I2C_Rec_TEF7000   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_TEF7000))
//add by gxnuhuang SOC_I2C_Rec_2B_SubAddr
#define SOC_I2C_Rec_2B_SubAddr   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Rec_2B_SubAddr))
#define SOC_I2C_Set_Rate  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_I2C_Set_Rate))
//io-irq
#define SOC_IO_ISR_Add  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Add))
#define SOC_IO_ISR_Enable   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Enable))
#define SOC_IO_ISR_Disable  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Disable))
#define SOC_IO_ISR_Del  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_ISR_Del))
//ad
#define SOC_ADC_Get  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_ADC_Get))
//key
#define SOC_Key_Report  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Key_Report))
//bl
#define SOC_BL_Set  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_BL_Set))
//pwr
#define SOC_PWR_ShutDown  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_PWR_ShutDown))
//for kernel build
#define SOC_PWR_GetStatus  check_pt()?:(plidbg_dev->soc_func_tbl.pfnSOC_PWR_GetStatus)
#define SOC_PWR_SetStatus  check_pt()?:(plidbg_dev->soc_func_tbl.pfnSOC_PWR_SetStatus)

//
#define SOC_Write_Servicer  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Write_Servicer))

//video
//#define lidbg_video_main  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnlidbg_video_main))
#define video_io_i2c_init  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnvideo_io_i2c_init))
#define flyVideoChannelInitall  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnflyVideoInitall))
#define flyVideoSignalPinTest  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnflyVideoTestSignalPin))
#define flyVideoImageQualityConfig  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnflyVideoImageQualityConfig))
#define video_init_config check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnvideo_init_config)

//display/touch

#define SOC_Display_Get_Res  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Display_Get_Res))

//video
#define camera_open_video_signal_test check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfncamera_open_video_signal_test)
#define camera_open_video_color check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfncamera_open_video_color)
//lpc
#define  SOC_LPC_Send  (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_LPC_Send))
#define global_video_format_flag  (plidbg_dev->soc_func_tbl.pfnglobal_video_format_flag)
//look home/Fly/3060pr/R8625QSOSKQLYA3060-v2/kernel/drivers/media/video/msm/sensors/msm_sensor.c
//---int32_t msm_sensor_get_output_info(struct msm_sensor_ctrl_t *s_ctrl,
#define global_video_channel_flag  (plidbg_dev->soc_func_tbl.pfnglobal_video_channel_flag)

//dev
#define SOC_Dev_Suspend_Prepare (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Dev_Suspend_Prepare))

//wakeup
#define SOC_PWR_Ignore_Wakelock    (plidbg_dev->soc_func_tbl.pfnSOC_PWR_Ignore_Wakelock)
#define SOC_PWR_Get_WakeLock (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Get_WakeLock))

//mic
#define SOC_Mic_Enable   (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Mic_Enable))

//
#define ptasklist_lock   (plidbg_dev->soc_pvar_tbl.pvar_tasklist_lock)
#define FLAG_FOR_15S_OFF   (plidbg_dev->soc_pvar_tbl.flag_for_15s_off)

#define read_tw9912_chips_signal_status (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnread_tw9912_chips_signal_status))
//video
#define global_camera_working_status  (plidbg_dev->soc_func_tbl.pfnglobal_video_camera_working_status)
//LCD
#define SOC_F_LCD_Light_Con (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_F_LCD_Light_Con))
//fake suspend
#define SOC_Fake_Register_Early_Suspend (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Fake_Register_Early_Suspend))
//video
#define VideoReset (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnVideoReset))

//uart
#define SOC_IO_Uart_Send (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_IO_Uart_Send))


#define SOC_Get_Share_Mem (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Get_Share_Mem))
#define SOC_System_Status (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_System_Status))

#define SOC_Get_CpuFreq (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_Get_CpuFreq))

#define SOC_LCD_Reset (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_LCD_Reset))

#define SOC_WakeLock_Stat (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_WakeLock_Stat))

#define SOC_Hal_Acc_Callback (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnHal_Acc_Callback))

#define SOC_Notifier_Call (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnNotifier_Call))

#define SOC_PWR_Status_Accon (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfnSOC_PWR_Status_Accon))

#define SOC_Call_mdp_pipe_ctrl (check_pt()?NULL:(plidbg_dev->soc_func_tbl.pfn_Call_mdp_pipe_ctrl))

#endif

