
#include "lidbg.h"


unsigned int soc_pwm_set(int pwm_id, int duty_ns, int period_ns)
{
    return 1;
}

void soc_bl_init(void)
{
#if (defined(BOARD_V1) || defined(BOARD_V2))

#else
    pmapp_disp_backlight_init();
#endif

}

unsigned int   soc_bl_set(u32 bl_level)
{
#if (defined(BOARD_V1) || defined(BOARD_V2))
    if (p_fly_smem == NULL)
    {
        lidbg( "p_fly_smem == NULL\n");
        return 0;
    }
    SMEM_BL = (int)bl_level;

#else
    int err;
    err = pmapp_disp_backlight_set_brightness(bl_level);
    if(err)
    {
        lidbg("Backlight set brightness failed !\n");
        return 0;
    }

#endif
    lidbg("soc_bl_set:%d\n", bl_level);
    return 1;
}

EXPORT_SYMBOL(soc_bl_set);
EXPORT_SYMBOL(soc_pwm_set);

