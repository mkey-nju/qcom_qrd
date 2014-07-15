
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include <utils/Log.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#define GUI_LCM_XMAX 1024
#define GUI_LCM_YMAX 600

#define SYS_FB_NUM 3
#define RD_BUFF_SIZE (10 * 1024)

// 1:clean one line, 0:clean all screen
#define CLEAN_LINE 0

//color
#define DISP_COLOR 0x0000ff00
#define BACK_COLOR 0x00000000

#if CLEAN_LINE
#define SCREEN_CLEAN_COLOR 0xffaa00aa
#else
#define SCREEN_CLEAN_COLOR 0xff000000
#endif

struct gui_info
{
    int inFd;	 //where opened message from
    char *rdBuff;  //buffer to store info read from inFd
    char *outBuff;  //an own buffer to store info for use
    unsigned int outBuffSize;
    char *sysFbBaseAddr;
    int interval;	//interval delay to clean the screen

    int *cleanScreenFlag;
};

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */
typedef unsigned char           U8, *PU8;
typedef /*signed*/ char         S8, *PS8, * *PPS8;
typedef unsigned short          U16, *PU16;
typedef signed short            S16, *PS16;
typedef unsigned long           U32, *PU32;

#include "debug_gui.h"


