
#include "lidbg.h"
#include <linux/leds.h>
extern 	struct led_classdev *backled_cdev;



unsigned int soc_pwm_set(int pwm_id, int duty_ns, int period_ns)
{
    return 1;
}

void soc_bl_init(void)
{

}

unsigned int   soc_bl_set(u32 bl_level)
{
//struct led_classdev *bl_cdev=backled_cdev;
// brightness_set(bl_cdev, bl_level);
 return 0;
}

EXPORT_SYMBOL(soc_bl_set);
EXPORT_SYMBOL(soc_pwm_set);

