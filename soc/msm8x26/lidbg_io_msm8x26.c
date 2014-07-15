
#include "lidbg.h"

u8 soc_io_config_log[IO_LOG_NUM];

void soc_io_init(void)
{
    memset(soc_io_config_log, 0, IO_LOG_NUM);
}


void soc_irq_disable(unsigned int irq)
{
    disable_irq(irq);

}

void soc_irq_enable(unsigned int irq)
{
    enable_irq(irq);
}


#if 0
// for test
static irqreturn_t interrupt_isr(int irq, void *dev_id)
{

    soc_irq_enable(irq);
    lidbg("come into IRQ [%d]!\n", irq);

    return 1;
}
#endif


int soc_io_irq(struct io_int_config *pio_int_config)//need set to input first?
{


    if (request_irq(pio_int_config->ext_int_num, pio_int_config->pisr, pio_int_config->irqflags /*IRQF_ONESHOT |*//*IRQF_DISABLED*/, "lidbg_irq", pio_int_config->dev ))
    {
        lidbg("request_irq err!\n");
        return 0;
    }
    return 1;
}

int soc_io_config(u32 index, bool direction, u32 pull, u32 drive_strength, bool force_reconfig)
{
    //int rc;
	struct gpiomux_setting lidbg_setting_active = {
	  .func = GPIOMUX_FUNC_GPIO, 
	  .drv = drive_strength,
	  .pull = pull,
	  .dir = direction,
	};

	struct msm_gpiomux_config lidbg_configs[] = {
	{
		 .gpio = index,
		 .settings = { 
		    [GPIOMUX_ACTIVE] = &lidbg_setting_active,
		},
		},
	};
    if(force_reconfig)
    {
       msm_gpiomux_install(lidbg_configs, ARRAY_SIZE(lidbg_configs));
    }

    if(soc_io_config_log[index] == 1)
    {
        return 1;
    }
    else
    {
        int err;

        if (!gpio_is_valid(index))
            return 0;


        lidbg("gpio_request:index %d\n" , index);

        msm_gpiomux_install(lidbg_configs, ARRAY_SIZE(lidbg_configs));
        err = gpio_request(index, "lidbg_io");
        if (err)
        {
            lidbg("\n\nerr: gpio request failed!!!!!!\n\n\n");
        }

        if(direction == GPIO_CFG_INPUT)
        {
            err = gpio_direction_input(index);
            if (err)
            {
                lidbg("gpio_direction_set failed\n");
                goto free_gpio;
            }
        }
        soc_io_config_log[index] = 1;

        return 1;

free_gpio:
        if (gpio_is_valid(index))
            gpio_free(index);
        return 0;
    }
}


int soc_io_output(u32 group, u32 index, bool status)
{
    gpio_direction_output(index, status);
    gpio_set_value(index, status);
    return 1;

}

bool soc_io_input( u32 index)
{
    return gpio_get_value(index);
}


EXPORT_SYMBOL(soc_io_output);
EXPORT_SYMBOL(soc_io_input);
EXPORT_SYMBOL(soc_io_irq);
EXPORT_SYMBOL(soc_irq_enable);
EXPORT_SYMBOL(soc_irq_disable);
EXPORT_SYMBOL(soc_io_config);
