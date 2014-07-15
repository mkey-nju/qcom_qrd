#ifndef __TW9912_H__
#define __TW9912_H__

#include "i2c_io.h"

//#ifdef DFLY_DEBUG
#define DEBUG_TW9912
//#endif
//#define DEBUG_PLOG_TW9912
//#define DEBUG_TW9912_CHECK_WRITE
#define I2C_US_IO_1
#define Progressive_Output_Format 0x12
#define Interlaced_Output_Format 0x02
#define TW9912_RESET 43
#define TW9912_I2C_ChipAdd 0x44 //SIAD = 0-->0x44  SIAD =1-->0x45


extern u8 tw9912_signal_unstabitily_for_tw9912_config_array_flag;
#if 1
#define tw9912_dbg(msg...)  do { lidbg( "TW9912: " msg); }while(0)
#else
#define tw9912_dbg(msg...)  do {}while(0)
#endif
#define tw9912_dbg_err(msg...)  do { lidbgerr( "TW9912: " msg); }while(0)

#if 1

//#ifdef BOARD_V1
//#define tw9912_RESX_UP do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 1);}while(0)//34
//#define tw9912_RESX_DOWN do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 0);msleep(100);}while(0)//9912 reset output, active low
//#elif BOARD_V2
#ifdef BOARD_V2
#define tw9912_RESX_UP do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 0);}while(0)//34
#define tw9912_RESX_DOWN do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 1);}while(0)//9912 reset output, active low 
#else //BOARD_V3
#define tw9912_RESX_UP do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 1);}while(0)//34
#define tw9912_RESX_DOWN do{i2c_io_config(TW9912_RESET,GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA,0);gpio_set_value(TW9912_RESET, 0);}while(0)//9912 reset output, active low 
#endif
#else
#define tw9912_RESX_UP do{}while(0)
#define tw9912_RESX_DOWN do{}while(0)
#endif

typedef enum
{
    YIN0 = 0, //not use
    YIN1,//not use
    YIN2,// DVD
    YIN3,//now is use AUX/BACK_CVBS
    SEPARATION,//DVD
    NOTONE,
} vedio_channel_t;
typedef enum
{
    AUX_4KO = 0,
    TV_4KO,
    ASTREN_4KO,
    DVD_4KO,
    OTHER_CHANNEL_4KO,
} vedio_channel_t_2;
typedef enum
{
    NTSC_I = 1,
    PAL_I,
    NTSC_P,
    PAL_P,
    STOP_VIDEO,
    COLORBAR,
    OTHER,
} vedio_format_t;
typedef enum
{
    BRIGHTNESS = 1,//ok
    CONTRAST,//ok
    SHARPNESS,
    CHROMA_U,//ok
    CHROMA_V,
    HUE,//ok
    //Positive value results in red hue and negative value gives green hue.
    //These bits control the color hue. It is in 2\u201fs complement form with 0 being the center 00 value.
} Vedio_Effect;

typedef enum
{
    source_60Hz = 0, //NTSC
    source_50Hz,//PAL
    source_other,
} Source_in;
typedef struct
{
    vedio_channel_t Channel;
    vedio_format_t Format;
    Source_in vedio_source;
} tw9912_signal_t;
/**************************/
typedef struct
{
    unsigned char index;	//register index
    unsigned char valu;		//resister valu
} TW9912_reg;

typedef struct
{
    TW9912_reg chip_status1;//index 0x01
    TW9912_reg chip_status2;//0x31
    TW9912_reg standard_selet;//0x1c
    TW9912_reg component_video_format;//0x1e
    TW9912_reg macrovision_detection;//0x30
    TW9912_reg input_detection;//0xc1
} tw9912_input_info_t;
/**************************/
typedef enum
{
    TW9912_initall_not = 0,
    TW9912_initall_yes,
} TW9912_initall_flag;
typedef struct
{
    TW9912_initall_flag flag;
    vedio_channel_t Channel;
    vedio_format_t format;
} tw9912_initall_status_t;
typedef struct
{
    u8 run;
    vedio_format_t format;
} tw9912_run_flag_t;
typedef struct
{
    vedio_channel_t Channel;
    vedio_format_t format;
} last_config_t;
typedef struct
{
    unsigned char reg;
    unsigned char reg_val;
    bool sta;//true is find black line;
    bool flag;//true is neet again find the black line;
    bool this_is_first_open;//true is first
} tw9912info_t;
/**************************/
int tw9912_config_array( vedio_channel_t Channel);

vedio_format_t camera_open_video_signal_test_in_2(void);
vedio_format_t Tw9912_appoint_pin_tw9912_cvbs_signal_tsting(vedio_channel_t Channel);
vedio_format_t tw9912_cvbs_signal_tsting(vedio_channel_t Channel);
i2c_ack tw9912_write(char *buf );
int tw9912_read_chips_status(u8 cmd);
i2c_ack Correction_Parameter_fun(vedio_format_t format);
int tw9912_config_array_agin(void);
vedio_format_t tw9912_yuv_signal_testing(void);
//void Disabel_video_data_out(void);
//void Enabel_video_data_out(void);
vedio_format_t tw9912_testing_channal_signal(vedio_channel_t Channel);
void tw9912_hardware_reset(void);
int Tw9912_YIN3ToYUV_init_agin(void);
int read_tw9912_chips_status_fast(u8 *valu);
int tw9912_config_array_PALi(void);
int tw9912_config_array_NTSCp(void);
#endif
