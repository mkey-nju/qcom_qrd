#ifndef __LIDBG_FASTBOOT_
#define __LIDBG_FASTBOOT_

#if 0
typedef enum
{
    PM_STATUS_EARLY_SUSPEND_PENDING,
    PM_STATUS_SUSPEND_PENDING,
    PM_STATUS_RESUME_OK,
    PM_STATUS_LATE_RESUME_OK,
} LIDBG_FAST_PWROFF_STATUS;

#endif

//int fastboot_get_status(void);
void fastboot_pwroff(void);


#endif

