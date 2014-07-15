#ifndef _LIGDBG_TOUCH__
#define _LIGDBG_TOUCH__


#define TOUCH_RELEASED    0
#define TOUCH_PRESSED      1
#define TOUCH_PRESSED_RELEASED    2

struct lidbg_ts_data
{
	s32 x[10];
	s32 y[10];
	s32 w[10];
	s32 id[10];
	u8 touch_num;
	u16 touch_index;
	u16 *pre_touch;
};
struct lidbg_input_data
{
	u16 abs_x_max;
	u16 abs_y_max;
	struct input_dev *input_dev;
};

void lidbg_touch_main(int argc, char **argv);
void lidbg_touch_report(struct lidbg_ts_data * pdata);
int lidbg_init_input(struct lidbg_input_data *pinput);

#endif

