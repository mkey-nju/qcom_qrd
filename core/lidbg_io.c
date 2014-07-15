
//http://blog.chinaunix.net/uid-20543672-id-3076669.html

#include "lidbg.h"
//extern void *global_lidbg_devp;

irqreturn_t io_test_irq(int irq, void *dev_id)
{

    lidbg("io_test_irq: %d \n", irq);
    return IRQ_HANDLED;

}

void mod_io_main(int argc, char **argv)
{
    if(argc < 2)
    {
        lidbg("Usage:\n");
        lidbg("w index status\n");
        lidbg("r index \n");
        lidbg("i\n");
        return;
    }

    if(!strcmp(argv[0], "r"))
    {
        u32 index;
        bool status;
        index = simple_strtoul(argv[1], 0, 0);

        IO_CONFIG_INPUT(0, index);
        status = soc_io_input(index);

        lidbg("read: %x\n", status);
        // *(int *)(((struct lidbg_dev *)global_lidbg_devp)->mem) = (int)status;

    }
    else if(!strcmp(argv[0], "w"))
    {
        u32 index;
        bool status;

        if(argc < 3)
        {
            lidbg("Usage:\n");
            lidbg("w index status\n");
            lidbg("r index \n");
            lidbg("i\n");
            return;
        }

        index = simple_strtoul(argv[1], 0, 0);
        status = simple_strtoul(argv[2], 0, 0);

        IO_CONFIG_OUTPUT(0, index);
        soc_io_output(0, index, status);

    }
    else if(!strcmp(argv[0], "i"))
    {
        if(!strcmp(argv[1], "request"))
        {

            struct io_int_config   *pio_int_config = NULL;
            pio_int_config = kmalloc(sizeof(struct io_int_config), GFP_KERNEL);
            if (pio_int_config == NULL)
            {
                LIDBG_ERR("kmalloc.\n");
            }

            pio_int_config->ext_int_num = simple_strtoul(argv[2], 0, 0);
            lidbg("request ext_int_num: %d\n", pio_int_config->ext_int_num);
            pio_int_config->ext_int_num = GPIO_TO_INT(pio_int_config->ext_int_num);
            lidbg("request ext_int_num: %d\n", pio_int_config->ext_int_num);

            pio_int_config->irqflags =  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT;

            pio_int_config->pisr = io_test_irq;
            pio_int_config->dev = (void *)pio_int_config;
            soc_io_irq(pio_int_config);
        }
        else if(!strcmp(argv[1], "enable"))
        {
            int irq = simple_strtoul(argv[2], 0, 0);
            lidbg("enable irq: %d\n", irq);
            soc_irq_enable(GPIO_TO_INT(irq));
        }
        else if(!strcmp(argv[1], "disable"))
        {
            int irq = simple_strtoul(argv[2], 0, 0);
            lidbg("disable irq: %d\n", irq);
            soc_irq_disable(GPIO_TO_INT(irq));
        }
    }

}

static int __init io_init(void)
{
    LIDBG_MODULE_LOG;
    return 0;
}

static void __exit io_exit(void)
{}

module_init(io_init);
module_exit(io_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudio Inc.");


EXPORT_SYMBOL(mod_io_main);

