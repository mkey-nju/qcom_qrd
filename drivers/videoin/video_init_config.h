#ifndef __VIDEO_INIT_CONFIG_H__
#define __VIDEO_INIT_CONFIG_H__
#include "i2c_io.h"
#include "tw9912.h"
#include "TC358746XBG.h"
#define CARD_TYPE_HONDA_XIYU
#if 1
#define video_config_debug(msg...)  do { lidbg( "flyvideo: " msg); }while(0)
#else
#define video_config_debug(msg...)  do {}while(0)
#endif
typedef struct
{
    Vedio_Effect cmd;	//register index
    unsigned char valu;		//resister valu
} TW9912_Image_Parameter;
void chips_config_begin(vedio_format_t config_pramat);
void video_io_i2c_init_in(void);
int flyVideoInitall_in(u8 Channel);
vedio_format_t flyVideoTestSignalPin_in(u8 Channel);
vedio_format_t camera_open_video_signal_test_in(void);
int flyVideoImageQualityConfig_in(Vedio_Effect cmd , u8 valu);
void tc358746xbg_show_color(u8 color_flag);
int read_chips_signal_status(u8 cmd);
int IfInputSignalNotStable(void);
void chips_hardware_reset(void);
int read_chips_signal_status_fast(u8 *valu);
#endif

