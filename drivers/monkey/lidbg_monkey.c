#include "lidbg.h"

LIDBG_DEFINE;

int gpio_pin;

struct monkey_dev
{
    struct completion monkey_wait;
    int monkey_enable;
    int random_on_en;
    int random_off_en;
    int on_ms;
    int off_ms;
    int sleep_ms;
    bool (*callback)(struct monkey_dev *g_monkey_dev, bool on_off);
};
struct monkey_dev private_dev;
struct monkey_dev *g_monkey_dev;


bool monkey_cmn_callback(struct monkey_dev *dev, bool on_off)
{
    bool abort_monkey = true;
    if(on_off)
    {
        SOC_IO_Output(0, gpio_pin, 1);
    }
    else
    {
        SOC_IO_Output(0, gpio_pin, 0);
    }
    msleep(dev->sleep_ms);
    return abort_monkey;
}

void monkey_work_func(struct monkey_dev *dev)
{
    //on
    dev->sleep_ms = dev->on_ms;
    if(dev->random_on_en)
        dev->sleep_ms = lidbg_get_random_number(dev->on_ms);
    LIDBG_WARN("==on==%d=====\n", dev->sleep_ms);
    if(!dev->callback(dev, true))
        monkey_run(false);

    //off
    dev->sleep_ms = dev->off_ms;
    if(dev->random_off_en)
        dev->sleep_ms = lidbg_get_random_number(dev->off_ms);
    LIDBG_WARN("==off==%d=====\n", dev->sleep_ms);
    if(!dev->callback(dev, false))
        monkey_run(false);

}
int monkey_work(void *data)
{
    while(1)
    {
        if(g_monkey_dev->monkey_enable)
            monkey_work_func(g_monkey_dev);
        else
            wait_for_completion_interruptible(&g_monkey_dev->monkey_wait);
    }
    return 0;
}

void monkey_run(int enable)
{
    LIDBG_WARN("<%d>\n", enable);
    if(!enable)
        g_monkey_dev->monkey_enable = false;
    else
    {
        g_monkey_dev->monkey_enable = true;
        complete(&g_monkey_dev->monkey_wait);
    }
}
void monkey_config(int gpio, int on_en, int off_en, int on_ms, int off_ms)
{
    LIDBG_WARN("<%d,%d,%d,%d,%d>\n",  gpio, on_en, off_en, on_ms, off_ms);
    gpio_pin = gpio;
    g_monkey_dev->random_on_en = on_en;
    g_monkey_dev->random_off_en = off_en;
    g_monkey_dev->on_ms = on_ms;
    g_monkey_dev->off_ms = off_ms;
}
void cb_kv_monkey_enable(char *key, char *value)
{
    monkey_run(g_monkey_dev->monkey_enable);
}

int monkey_init(void *data)
{
	g_monkey_dev = &private_dev;

    init_completion(&g_monkey_dev->monkey_wait);
    g_monkey_dev->callback = monkey_cmn_callback;

    FS_REGISTER_INT(g_monkey_dev->monkey_enable, "monkey_work_en", 0, cb_kv_monkey_enable);
    FS_REGISTER_INT(g_monkey_dev->random_on_en, "random_on_en", 0, NULL);
    FS_REGISTER_INT(g_monkey_dev->random_off_en, "random_off_en", 0, NULL);
    FS_REGISTER_INT(g_monkey_dev->on_ms, "on_ms", 0, NULL);
    FS_REGISTER_INT(g_monkey_dev->off_ms, "off_ms", 0, NULL);

    FS_REGISTER_INT(gpio_pin, "gpio", 0 , NULL);

    LIDBG_WARN("[%d,%d,%d,%d,%d]\n\n", gpio_pin, g_monkey_dev->random_on_en, g_monkey_dev->random_off_en, g_monkey_dev->on_ms, g_monkey_dev->off_ms);
    CREATE_KTHREAD(monkey_work, NULL);
    return 0;
}


static int __init lidbg_monkey_init(void)
{
    DUMP_FUN;
    LIDBG_GET;
    CREATE_KTHREAD(monkey_init, NULL);
    return 0;
}

static void __exit lidbg_monkey_exit(void)
{
}

module_init(lidbg_monkey_init);
module_exit(lidbg_monkey_exit);

MODULE_LICENSE("GPL");


EXPORT_SYMBOL(monkey_run);
EXPORT_SYMBOL(monkey_config);

