
#include "lidbg.h"
#define QPNP_LIDBG_DRIVER_NAME "qcom,qpnp-lidbg"
struct qpnp_vadc_chip		*vadc_chip;
unsigned int  soc_ad_read(unsigned int channel)  //temp-alarm channel is 8  ,name is temp_alarm
{
	struct qpnp_vadc_result adc_result;
    qpnp_vadc_read(vadc_chip, channel, &adc_result);
	//printk("result:%lld",adc_result.physical);
	return adc_result.physical;
}

static int __devinit qpnp_lidbg_probe(struct spmi_device *spmi)
{
	int rc = 0;
	vadc_chip= qpnp_get_vadc(&spmi->dev,"lidbg");
	return rc;
}

static struct of_device_id qpnp_lidbg_match_table[] = {
	{ .compatible = QPNP_LIDBG_DRIVER_NAME, },
	{}
};

static const struct spmi_device_id qpnp_lidbg_id[] = {
	{ QPNP_LIDBG_DRIVER_NAME, 0 },
	{}
};

static struct spmi_driver qpnp_lidbg_driver = {
	.driver = {
		.name		= QPNP_LIDBG_DRIVER_NAME,
		.of_match_table	= qpnp_lidbg_match_table,
		.owner		= THIS_MODULE,
	},
	.probe	  = qpnp_lidbg_probe,
	.id_table = qpnp_lidbg_id,
};

int __init qpnp_lidbg_init(void)
{
	int ret;
	ret = spmi_driver_register(&qpnp_lidbg_driver);
	pr_info("spmi register return %d\n", ret);
	return ret;
}

static void __exit qpnp_lidbg_exit(void)
{
	spmi_driver_unregister(&qpnp_lidbg_driver);
}

module_init(qpnp_lidbg_init);
module_exit(qpnp_lidbg_exit);

MODULE_DESCRIPTION("QPNP LIDBG DRIVER");
MODULE_LICENSE("GPL v2");



EXPORT_SYMBOL(soc_ad_read);


