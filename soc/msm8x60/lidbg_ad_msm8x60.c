
#include "lidbg.h"

u8 soc_ad_request_log[AD_LOG_NUM];

void  soc_ad_init(void)
{
    memset(soc_ad_request_log, 0xff, AD_LOG_NUM);//no use here
}

unsigned int  soc_ad_read(unsigned int channel)
{

    return 0xffffffff;
}


EXPORT_SYMBOL(soc_ad_read);


