
#include "lidbg.h"

struct spi_api
{
    struct list_head list;
    struct spi_device *spi;
};

#define SPI_MINORS    32
#define SPI_MODE_MASK (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
        | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
        | SPI_NO_CS | SPI_READY)

static LIST_HEAD(spi_api_list);
static DEFINE_SPINLOCK(spi_api_list_lock);

static struct spi_api *get_spi_api(int bus_id)
{
    struct spi_api *spi_api;

    spin_lock(&spi_api_list_lock);
    list_for_each_entry(spi_api, &spi_api_list, list)
    {
        if (spi_api->spi->master->bus_num == bus_id)
            goto found;
    }
    spi_api = NULL;

found:
    spin_unlock(&spi_api_list_lock);
    return spi_api;
}

static struct spi_api *add_spi_api(struct spi_device *spi)
{
    struct spi_api *spi_api;

    if (spi->master->bus_num >= SPI_MINORS)
    {
        lidbg(KERN_ERR "spi_api: Out of device minors (%d)\n",
              spi->master->bus_num);
        return NULL;
    }

    spi_api = kzalloc(sizeof(*spi_api), GFP_KERNEL);
    if (!spi_api)
        return NULL;
    spi_api->spi = spi;

    spin_lock(&spi_api_list_lock);
    list_add_tail(&spi_api->list, &spi_api_list);
    spin_unlock(&spi_api_list_lock);
    return spi_api;
}

static void del_spi_api(struct spi_api *spi_api)
{
    spin_lock(&spi_api_list_lock);
    list_del(&spi_api->list);
    spin_unlock(&spi_api_list_lock);
    kfree(spi_api);
}

int spi_api_do_set(int bus_id,
                   u8 mode,
                   u8 bits_per_word,
                   u8 max_speed_hz)
{
    struct spi_device *spi;
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;

    spi = spi_api->spi;
    //spi->mode &= ~SPI_MODE_MASK;
    spi->mode &= SPI_MODE_MASK;
    spi->mode |= mode;
    spi->bits_per_word = bits_per_word;
    spi->max_speed_hz = max_speed_hz;
    return spi_setup(spi);
}

int spi_api_do_write(int bus_id, const u8 *buf, size_t len)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_write(spi_api->spi, buf, len);
}

int spi_api_do_read(int bus_id, u8 *buf, size_t len)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_read(spi_api->spi, buf, len);
}

int spi_api_do_write_then_read(int bus_id,
                               const u8 *txbuf, unsigned n_tx,
                               u8 *rxbuf, unsigned n_rx)
{
    struct spi_api *spi_api = get_spi_api(bus_id);
    if (!spi_api)
        return -ENODEV;
    return spi_write_then_read(spi_api->spi, txbuf, n_tx, rxbuf, n_rx);
}

static int spi_api_probe(struct spi_device *spi)
{
    add_spi_api(spi);
    lidbg(KERN_INFO "spi_api_probe device[%d]\n", spi->master->bus_num);
    return 0;
}

static int spi_api_remove(struct spi_device *spi)
{
    struct spi_api *spi_api = get_spi_api(spi->master->bus_num);
    if (spi_api)
        del_spi_api(spi_api);
    return 0;
}

static struct spi_driver spi_api_driver =
{
    .driver = {
        .name = "SPI-API",
        .owner = THIS_MODULE,
    },
    .probe = spi_api_probe,
    .remove = spi_api_remove,
};

static int __init spi_api_init(void)
{
    /*
    SPI设备的驱动程序通过spi_register_driver注册进SPI子系统，驱动类型为struct spi_driver。
    因为spi总线不支持SPI设备的自动检测，所以一般在spi的probe函数中不会检测设备是否存在，
    而是做一些spi设备的初始化工作。
    */

    int ret = spi_register_driver(&spi_api_driver);



    if (ret)
    {
        lidbg(KERN_ERR "[%s] Driver registration failed, module not inserted.\n", __func__);
        return ret;
    }
    lidbg("spi_api_init\n");
    DUMP_BUILD_TIME;
    return 0 ;
}

static void __exit spi_api_exit(void)
{

    spi_unregister_driver(&spi_api_driver);
}


void mod_spi_main(int argc, char **argv)
{

    if(argc < 5)
    {
        lidbg("Usage:\n");
        lidbg("r bus_id dev_addr start_reg num\n");
        lidbg("w bus_id dev_addr num data1 data2 ...\n");
        return;

    }
    //...


}


MODULE_AUTHOR("Loon, <sepnic@gmail.com>");
MODULE_DESCRIPTION("SPI spi_api Driver");
MODULE_LICENSE("GPL");

module_init(spi_api_init);
module_exit(spi_api_exit);

EXPORT_SYMBOL(spi_api_do_set);
EXPORT_SYMBOL(spi_api_do_write);
EXPORT_SYMBOL(spi_api_do_read);
EXPORT_SYMBOL(spi_api_do_write_then_read);
EXPORT_SYMBOL(mod_spi_main);


