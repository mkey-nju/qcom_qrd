#ifndef _LIDBG_BP_
#define _LIDBG_BP_

//futengfei add for bpmsg_to_ap_print  2012.12.12.12:00

//up always online
#include <mach/msm_smsm.h>

#define TOTAL_LOGS 500UL
#define LOG_BYTES  32UL
typedef struct
{
    int start_pos;
    int end_pos;
    char log[TOTAL_LOGS][LOG_BYTES];
    u8 write_flag;
} smem_log_deep;
#define 	BP_MSG_POLLING_TIME	(50)

#endif
