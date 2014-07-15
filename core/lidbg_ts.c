
#include "lidbg.h"

//#define LIDBG_MULTI_TOUCH_SUPPORT
#define GTP_MAX_TOUCH	5

#define TOUCH_X_MIN  (0)
#define TOUCH_X_MAX  (RESOLUTION_X-1)
#define TOUCH_Y_MIN  (0)
#define TOUCH_Y_MAX  (RESOLUTION_Y-1)
struct input_dev *input = NULL;
struct input_dev *input_dev = NULL;
int lidbg_init_input(struct lidbg_input_data *pinput)
{
    	int ret;
	char phys[32];
	lidbg("%s:----------------wsx-------------------\n",__FUNCTION__);
	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		lidbg("Failed to allocate input device.\n");
		return -ENOMEM;
	}
	input_dev->evbit[0] =
		BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ; 
	set_bit(BTN_TOOL_FINGER, (input_dev)->keybit);
	__set_bit(INPUT_PROP_DIRECT, (input_dev)->propbit);
	
	input_mt_init_slots(input_dev, 5);/* in case of "out of memory" */ 
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
				0, pinput->abs_x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
				0, pinput->abs_y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR,
				0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
				0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,
				0, 255, 0, 0);
	snprintf(phys, 32, "input/ts");
	//strcpy(input_dev->name,"Goodix-CTP");
	input_dev->name = "Goodix-CTP";
	input_dev->phys = phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->id.vendor = 0xDEAD;
	input_dev->id.product = 0xBEEF;
	input_dev->id.version = 10427;
	lidbg("before register\n");
	ret = input_register_device(input_dev);

	if (ret) {
		lidbg("---------wsx------input device failed.\n");
		input_free_device(input_dev);
		input_dev = NULL;
		return ret;
	}
	lidbg("after register\n");
	pinput->input_dev = input_dev;
	return 0;

}

void lidbg_touch_report(struct lidbg_ts_data *pdata)
{
    	int i;
	lidbg("%s:-----------wsx-----------",__FUNCTION__);
	for (i = 0; i < GTP_MAX_TOUCH; i++)  
    	{  
		input_mt_slot(input_dev, pdata->id[i]);  
            if ((pdata->touch_index) & (0x01<<i)) 
            	{  	   
                      input_mt_slot(input_dev, pdata->id[i]);
		        input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, true);
		        input_report_abs(input_dev, ABS_MT_POSITION_X, pdata->x[i]);
		        input_report_abs(input_dev, ABS_MT_POSITION_Y, pdata->y[i]);
		        input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, pdata->w[i]);
			 input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, pdata->w[i]);
		        lidbg("%d,%d[%d,%d];\n", pdata->touch_num,pdata->id[i], pdata->x[i], pdata->y[i]);
			*(pdata->pre_touch) |= 0x01 << i;
			pdata->touch_index |= (0x01 << pdata->id[i+1]);  
			
        	}  
       		else  
        	{  
			lidbg("release\n");
           	    	input_mt_slot(input_dev, i);
		    	input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, false);
		    	*(pdata->pre_touch) &= ~(0x01 << i);
        	}  
		
	}  
  		input_sync(input_dev);
      
}

extern void  touch_event_init(void);
int lidbg_touch_init(void)
{
    int error;
    u32 RESOLUTION_X = 1024;
    u32 RESOLUTION_Y = 600;

    input = input_allocate_device();
    if (!input)
    {
        lidbg("input_allocate_device err!\n");
        goto fail;
    }
    input->name = "lidbg-touch_n/input0";
    input->phys = "lidbg-touch_p/input0";

    input->id.bustype = BUS_HOST;
    input->id.vendor = 0x0001;
    input->id.product = 0x0011;
    input->id.version = 0x0101;

    soc_get_screen_res(&RESOLUTION_X, &RESOLUTION_Y);

#ifdef LIDBG_MULTI_TOUCH_SUPPORT
    set_bit(EV_SYN, input->evbit);
    set_bit(EV_KEY, input->evbit);
    set_bit(EV_ABS, input->evbit);
    set_bit(BTN_TOUCH, input->keybit);
    set_bit(BTN_2, input->keybit);

    input_set_abs_params(ts->input_dev, ABS_X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_Y, 0, RESOLUTION_Y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, GOODIX_TOUCH_WEIGHT_MAX, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, GOODIX_TOUCH_WEIGHT_MAX, 15, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_HAT0X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_HAT0Y, 0, RESOLUTION_Y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, RESOLUTION_X, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, RESOLUTION_Y, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 150, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 150, 0, 0);

#else

    input->evbit[0] = BIT(EV_SYN) | BIT_MASK(EV_ABS) | BIT_MASK(EV_KEY);
    input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
    input_set_abs_params(input, ABS_X, TOUCH_X_MIN, TOUCH_X_MAX, 0, 0);
    input_set_abs_params(input, ABS_Y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 0);
    input_set_abs_params(input, ABS_PRESSURE, 0, 1, 0, 0);  //\u538b\u529b
#endif

    error = input_register_device(input);
    if (error)
    {
        lidbg("input_register_device err!\n");

        goto fail;
    }

    touch_event_init();
	
    LIDBG_MODULE_LOG;

    return 0;
fail:
    input_free_device(input);
    return 0;

}


void lidbg_touch_deinit(void)
{
    input_unregister_device(input);
}


void lidbg_touch_main(int argc, char **argv)
{
    struct lidbg_ts_data *posion_t = kzalloc(sizeof(*posion_t), GFP_KERNEL);;
    if(argc < 2)
    {
        lidbg("Usage:\n");
        lidbg("x y num\n");
        return;
    }
    posion_t->x[0] = simple_strtoul(argv[0], 0, 0);
    posion_t->y[0] = simple_strtoul(argv[1], 0, 0);
    posion_t->touch_num = simple_strtoul(argv[2], 0, 0);

  //  lidbg_touch_report(input,posion_t);

}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Flyaudad Inc.");

EXPORT_SYMBOL(lidbg_init_input);
EXPORT_SYMBOL(lidbg_touch_main);
EXPORT_SYMBOL(lidbg_touch_report);

module_init(lidbg_touch_init);
module_exit(lidbg_touch_deinit);

