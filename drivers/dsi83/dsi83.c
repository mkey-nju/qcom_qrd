
#include "dsi83.h"

//struct work_struct  dsi83_work = NULL;
static struct delayed_work dsi83_work;
static struct workqueue_struct *dsi83_workqueue;


struct i2c_adapter *adap_glb = NULL;

static int32_t i2c_rw(unsigned char chipaddr,unsigned int sub_addr,int mode,
	 unsigned char *buf, int size)
{
	int32_t ret =0;
	struct i2c_adapter *adap = NULL;  //=i2c_get_adapter(DSI83_I2C_BUS);
	uint16_t saddr =chipaddr;
//	printk("adap name=%s\n",adap->name);

	 if(adap_glb)
	 {
		 adap = adap_glb;
	 }
	 else
	 {
		 adap = i2c_get_adapter(DSI83_I2C_BUS);
		 if(adap == NULL || adap->name==NULL){
			 printk(KERN_CRIT "dsi83:%s():i2c_get_adapter(%d) error!",__func__,DSI83_I2C_BUS);
			 return 0;
		 }
	 }

	 switch (mode)
    	{
    	case I2C_API_XFER_MODE_SEND:
   	 {
        struct i2c_msg msg;

        msg.addr = saddr ;
        msg.flags = 0;
        msg.len = size;
        msg.buf = buf;

        ret = i2c_transfer(adap, &msg, 1);
        break;
    }

    case I2C_API_XFER_MODE_RECV:
    {

        struct i2c_msg msg[2];
        char subaddr;
        subaddr = sub_addr & 0xff;

        msg[0].addr =saddr ;
        msg[0].flags = 0;
        msg[0].len = 1;
        msg[0].buf = &subaddr;

        msg[1].addr = saddr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = size;
        msg[1].buf = buf;


        ret = i2c_transfer(adap, msg, 2);
        break;
    }
   case I2C_API_XFER_MODE_RECV_SUBADDR_2BYTES:
    {

        struct i2c_msg msg[2];
        char SubAddr[2];

        SubAddr[0] = (sub_addr >> 8) & 0xff;
        SubAddr[1] = sub_addr & 0xff;

        msg[0].addr = saddr ;
        msg[0].flags = 0;
        msg[0].len = 2;
        msg[0].buf = SubAddr;

        msg[1].addr = saddr ;
        msg[1].flags = I2C_M_RD;
        msg[1].len = size;
        msg[1].buf = buf;

        ret = i2c_transfer(adap, msg, 2);
        break;

    }
	default:
        return -EINVAL;
    }
	return ret;
}

static int SN65_register_read(unsigned char sub_addr,char *buf)
{int ret;
	ret = i2c_rw(DSI83_I2C_ADDR,sub_addr,I2C_API_XFER_MODE_RECV,buf,1);
return ret;
}

static int SN65_register_write( char *buf)
{int ret;
	ret = i2c_rw(DSI83_I2C_ADDR,0,I2C_API_XFER_MODE_SEND,buf,2);
return ret;
}

static int SN65_Sequence_seq4(void)
{
	int ret = 0,i;
	char buf2[2];
	char *buf_piont = NULL;
	buf_piont = dsi83_conf;
	buf2[0]=0x00;
	buf2[1]=0x00;
	printk(KERN_CRIT "dsi83:Sequence 4\n");
	for(i=0;buf_piont[i] !=0xff ;i+=2)
	{
		ret = SN65_register_write(&buf_piont[i]);
		buf2[0] = buf_piont[i];
		ret = SN65_register_read(buf2[0],&buf2[1]);
		lidbg_dsi83("register 0x%x=0x%x\n", buf_piont[i], buf_piont[i+1]);

		if(buf2[1] != buf_piont[i+1])
		{
			printk(KERN_CRIT "Warning regitster(0x%.2x),write(0x%.2x) and read back(0x%.2x) Unequal\n",\
buf_piont[i],buf_piont[i+1],buf2[1]);
		}
	}
return ret;
}

static int SN65_Sequence_seq6(void)
{
	int ret;
	char buf2[2];
	buf2[0]=0x0d;
	buf2[1]=0x01;
	printk(KERN_CRIT "dsi83:Sequence 6\n");
	ret = SN65_register_write(buf2);
return ret;
}
static int SN65_Sequence_seq7(void)
{
	int ret;
	char buf2[2];
	printk(KERN_CRIT "dsi83:Sequence 7\n");

	buf2[0]=0x0a;
	buf2[1]=0x00;
	ret = SN65_register_read(buf2[0],&buf2[1]);
	lidbg_dsi83("read(0x0a) = 0x%.2x\n",buf2[1]);

	{unsigned char k,i=0;
		k = buf2[1]&0x80;
		while(!k)
		{
			ret = SN65_register_read(buf2[0],&buf2[1]);
			k = buf2[1]&0x80;
			printk(KERN_CRIT "dsi83:Wait for %d,r = 0x%.2x\n",i,buf2[1]);
			msleep(5);
			i++;
			if(i>10)
			{
				printk(KERN_CRIT "dsi83:Warning wait time out .. break\n");
				break;
			}
			mdelay(1000);
		}
	}
return ret;
}

#ifdef DSI83_DEBUG
static void dsi83_dump_reg(void)
{
	int i;
	unsigned char reg;
	
	//int ret;
	char buf1[2];
	buf1[0]=0xe5;
	buf1[1]=0xff;

	for (i = 0; i < 0x3d; i++) {
		SN65_register_read(i, &reg);
		printk(KERN_CRIT "dsi83:Read reg-0x%x=0x%x\n", i, reg);
	}

/*
	SN65_register_read(0xe1, &reg);
	printk(KERN_CRIT "[LSH]:reg-0xE1=0x%x.\n",reg);
	SN65_register_read(0xe5, &reg);
	printk(KERN_CRIT "[LSH]:reg-0xE5=0x%x.\n",reg);
	printk(KERN_CRIT "*****************************\n");

	for(i = 0; i < 20; ++i)
	{
		
		ret = SN65_register_write(buf1);
		if(!ret)
			printk(KERN_CRIT "[LSH]:write reg 0xe5 error.\n");
		msleep(50);
		
		SN65_register_read(0xe1, &reg);
		printk(KERN_CRIT "[LSH]:reg-0xE1=0x%x.\n",reg);
		SN65_register_read(0xe5, &reg);
		printk(KERN_CRIT "[LSH]:reg-0xE5=0x%x.\n",reg);

		printk(KERN_CRIT "------------------------\n");
		msleep(10);
	}
*/

}
#endif

static int SN65_Sequence_seq8(void)  /*seq 8 the bit must be set after the CSR`s are updated*/
{
	int ret;
	char buf2[2];
	
#ifdef DSI83_DEBUG
	dsi83_dump_reg();
#endif
	printk(KERN_CRIT "dsi83:Sequence 8\n");
	buf2[0]=0x09;
	buf2[1]=0x01;
	ret = SN65_register_write(buf2);
	lidbg_dsi83("write(0x09) = 0x%.2x\n",buf2[1]);
	
#ifdef DSI83_DEBUG
	mdelay(100);
	dsi83_dump_reg();
#endif	
	return ret;
}

#if 1
int dsi83_io_config(u32 gpio_num, char *label)
{
	int rc = 0;
	
	if (!gpio_is_valid(gpio_num)) 
	{
		printk(KERN_CRIT "dsi83:gpio-%u isn't valid!\n",gpio_num);
		return -ENODEV;
	}
	
	rc = gpio_request(gpio_num, label);
	if (rc) 
	{
		printk(KERN_CRIT "dsi83:request gpio-%u failed, rc=%d\n", gpio_num, rc);
		//gpio_free(gpio_num);
		return -ENODEV;
	}
	rc = gpio_tlmm_config(GPIO_CFG(
			gpio_num, 0,
			GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	if (rc) 
	{
		printk(KERN_CRIT "dsi83:unable to config tlmm = %d\n", gpio_num);
		gpio_free(gpio_num);
		return -ENODEV;
	}

	return 0;
}
#endif

static int dsi83_enable(void)
{
    return gpio_direction_output(DSI83_GPIO_EN, 1);
}

/*
static int dsi83_disable(void)
{
    return gpio_direction_output(DSI83_GPIO_EN, 0);
}
*/

static int SN65_devices_read_id(void)
{
	// addr:0x08 - 0x00
	uint8_t id[9] = {0x01, 0x20, 0x20, 0x20, 0x44, 0x53, 0x49, 0x38, 0x35};
	uint8_t chip_id[9];
	int i, j;

	for (i = 0x8, j = 0; i >= 0; i--, j++) 
	{
		SN65_register_read(i,&chip_id[j]);
		lidbg_dsi83("%s():reg 0x%x = 0x%x", __func__, i, chip_id[j]);
	}

	return memcmp(id, chip_id, 9);

}

static void T123_reset(void)
{
	gpio_direction_output(T123_GPIO_RST, 0);
	mdelay(300);
	gpio_direction_output(T123_GPIO_RST, 1);
	mdelay(20);
}


static void panel_reset(void)
{
	gpio_direction_output(PANEL_GPIO_RESET, 0);
	mdelay(10);
	gpio_direction_output(PANEL_GPIO_RESET, 1);
	mdelay(20);
}

#if defined(CONFIG_FB)
void dsi83_suspend(void)
{
	//printk(KERN_CRIT "[LSH-dsi83]:enter %s().\n", __func__);
}

void dsi83_resume(void)
{
	//printk(KERN_CRIT "[LSH-dsi83]:enter %s().\n", __func__);
	queue_delayed_work(dsi83_workqueue, &dsi83_work, 0);
}

static int dsi83_fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	
	//printk(KERN_CRIT "[LSH-dsi83]:enter %s().\n", __func__);
	if (evdata && evdata->data && event == FB_EVENT_BLANK) 
	{
		blank = evdata->data;
		
		if (*blank == FB_BLANK_UNBLANK)
			dsi83_resume();
		else if (*blank == FB_BLANK_POWERDOWN)
			dsi83_suspend();
	}

	return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)

#endif

static void dsi83_work_func(struct work_struct *work)
{
    int ret = 0;
	
	ret = SN65_devices_read_id();
	if (ret)
	{
		printk(KERN_CRIT "dsi83:DSI83 match ID falied!\n");
	}

	printk(KERN_CRIT "dsi83:DSI83 match ID success!\n");

	SN65_Sequence_seq4();
	
	ret = SN65_Sequence_seq6();
	if(ret < 0)
		printk(KERN_CRIT "dsi83:SN65_Sequence_seq6(),err,ret = %d.\n", ret);
	
	SN65_Sequence_seq7();
	
	ret = SN65_Sequence_seq8();
	if(ret < 0)
		printk(KERN_CRIT "dsi83:SN65_Sequence_seq8(),err,ret = %d.\n", ret);

}


static int dsi83_probe(struct platform_device *pdev)
{
	int ret = 0;
	
	lidbg_dsi83("%s:enter\n", __func__);

	adap_glb = i2c_get_adapter(DSI83_I2C_BUS);

	if(adap_glb == NULL || adap_glb->name==NULL)
	{
		printk(KERN_CRIT "dsi83:%s():i2c_get_adapter(%d) error!",__func__,DSI83_I2C_BUS);
		return -ENODEV;
	}
/*	
    ret = dsi83_io_config(DSI83_GPIO_EN, "dsi83_en");
	if(ret)
	{
		return ret;
	}


	ret = dsi83_io_config(PANEL_GPIO_RESET, "lcd_reset");
	if(ret)
	{
		gpio_free(DSI83_GPIO_EN);
		return ret;
	}
*/	
	ret = dsi83_io_config(T123_GPIO_RST, "T123_reset");
	if(ret)
	{
		//gpio_free(PANEL_GPIO_RESET);
		//gpio_free(DSI83_GPIO_EN);
		return ret;
	}

	//INIT_WORK(&dsi83_work, dsi83_work_func);
	INIT_DELAYED_WORK(&dsi83_work, dsi83_work_func);
	dsi83_workqueue = create_workqueue("dsi83");

	dsi83_enable();
	panel_reset();
	//mdelay(100);
	T123_reset();

	//schedule_work(&dsi83_work);
	queue_delayed_work(dsi83_workqueue, &dsi83_work, DSI83_DELAY_TIME);

#if 0
	ret = SN65_devices_read_id();
	if (ret)
	{
		printk(KERN_CRIT "dsi83:DSI83 match ID falied!\n");
		return ret;
	}

	printk(KERN_CRIT "dsi83:DSI83 match ID success!\n");

	SN65_Sequence_seq4();
	
	ret = SN65_Sequence_seq6();
	if(ret < 0)
		printk(KERN_CRIT "dsi83:SN65_Sequence_seq6(),err,ret = %d.\n", ret);
	
	SN65_Sequence_seq7();
	
	ret = SN65_Sequence_seq8();
	if(ret < 0)
		printk(KERN_CRIT "dsi83:SN65_Sequence_seq8(),err,ret = %d.\n", ret);
#endif

#if defined(CONFIG_FB)
		dsi83_fb_notif.notifier_call = dsi83_fb_notifier_callback;
		ret = fb_register_client(&dsi83_fb_notif);
		if (ret)
				lidbg("Unable to register dsi83_fb_notif: %d\n",ret);
#elif defined(CONFIG_HAS_EARLYSUSPEND)

#endif


	return 0;

}

static int dsi83_remove(struct platform_device *pdev)
{
	cancel_work_sync(dsi83_workqueue);
	flush_workqueue(dsi83_workqueue);
	destroy_workqueue(dsi83_workqueue);

	gpio_free(T123_GPIO_RST);
    //gpio_free(PANEL_GPIO_RESET);
    //gpio_free(DSI83_GPIO_EN);
    return 0;
}


/*
static struct miscdevice misc =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &dev_fops,
};
*/

static struct platform_device dsi83_devices =
{
    .name			= "dsi83",
    .id 			= 0,
};

static struct platform_driver dsi83_driver =
{
    .probe = dsi83_probe,
    .remove = dsi83_remove,
    .driver = {
        .name = "dsi83",
        .owner = THIS_MODULE,
    },
};

static int __devinit dsi83_init(void)
{
    lidbg_dsi83("%s:enter\n", __func__);
    platform_device_register(&dsi83_devices);
    platform_driver_register(&dsi83_driver);
    //misc_register(&misc);
    return 0;

}

static void __exit dsi83_exit(void)
{

}


module_init(dsi83_init);
module_exit(dsi83_exit);
MODULE_LICENSE("GPL");

