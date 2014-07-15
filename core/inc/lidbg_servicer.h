#ifndef _LIGDBG_SERVICER__
#define _LIGDBG_SERVICER__



#define SERVICER_DONOTHING  (0)
#define LOG_DMESG  (1)
#define LOG_LOGCAT (2)
#define LOG_CLEAR_LOGCAT_KMSG (5)
#define LOG_SHELL_TOP_DF_PS (6)


#if 0

#define LOG_DMESG  (1)
#define LOG_LOGCAT (2)
#define LOG_ALL (3)
#define LOG_CONT    (4)
#define WAKEUP_KERNEL (10)
#define SUSPEND_KERNEL (11)

#define USB_RST_ACK (88)

#define LOG_DVD_RESET (64)
#define LOG_CAP_TS_GT811 (65)
#define LOG_CAP_TS_FT5X06 (66)
#define LOG_CAP_TS_FT5X06_SKU7 (67)
#define LOG_CAP_TS_RMI (68)
#define LOG_CAP_TS_GT801 (69)
#define LOG_CAP_TS_GT911 (71)
#define CMD_FAST_POWER_OFF (70)
#define LOG_CAP_TS_GT910 (72)
#define UMOUNT_USB (80)


#define UBLOX_EXIST   (88)
#define CMD_ACC_OFF_PROPERTY_SET (89)
#endif
void lidbg_servicer_main(int argc, char **argv);

void k2u_write(int cmd);
int k2u_read(void);
int u2k_read(void);


#endif

