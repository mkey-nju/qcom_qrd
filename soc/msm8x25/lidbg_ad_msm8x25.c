
#include "lidbg.h"

u8 soc_ad_request_log[AD_LOG_NUM];

void  soc_ad_init(void)
{
    memset(soc_ad_request_log, 0xff, AD_LOG_NUM);//no use here
}

unsigned int  soc_ad_read(unsigned int channel)
{
    u32 value;

    if(channel >= ADC_MAX_CH)
    {
        lidbg( "channel err!\n");
        return 0xffffffff;
    }

    if (p_fly_smem == NULL)
    {
        //lidbg( "p_fly_smem == NULL\n");
        return 0xffffffff;
    }
    value = SMEM_AD[channel];
    return value;
}


EXPORT_SYMBOL(soc_ad_read);


