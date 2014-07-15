/*
 * =====================================================================================
 *
 *       Filename:  lidb_kmsg.c
 *
 *       Description:  Diplay kmsg on the products screen .
 *
 *       Version:  1.0
 *       Created:  2013-09-29 10:00:11
 *       Revision:  none
 *       Compiler:  gcc
 *
 *       Author:  Jeson.Joy
 *       Company:  FlyAudio
 *
 * =====================================================================================
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <pthread.h>
#include "debug_includes.h"

#define LOG_BYTES  32UL

#define LIDBG_PRINT(msg...) do{\
	int fd;\
	char s[LOG_BYTES];\
	sprintf(s, "lidbg_msg: " msg);\
	s[LOG_BYTES - 1] = '\0';\
	 fd = open("/dev/lidbg_msg", O_RDWR);\
	 if((fd == 0)||(fd == (int)0xfffffffe)|| (fd == (int)0xffffffff))break;\
	 write(fd, s, /*sizeof(msg)*/strlen(s)/*LOG_BYTES*/);\
	 close(fd);\
}while(0)

//global var
static struct fb_var_screeninfo vinfo;
static struct gui_info guiInfo;

void GUI_PutPixel(U32 x, U32 y, U32 color)
{
    U8 a, r, g , b;
    long location;
    int i;

    switch(vinfo.bits_per_pixel)
    {
    case 32:
        location = x * 4 + y * vinfo.xres * 4;
        if(color == SCREEN_CLEAN_COLOR)
        {
            for(i = 0; i < 3; i++)
            {
                *((unsigned int *)(guiInfo.sysFbBaseAddr + guiInfo.outBuffSize * i + location)) = color;
            }
        }
        *((unsigned int *)(guiInfo.outBuff + location)) = color;
        break;

    case 16:
        location = x * 2  + y * vinfo.xres * 2;

        r = (color >> 19) & 0xff;
        g = (color >> 10) & 0xff;
        b = (color >> 3) & 0xff;

        color = (r << 11) | (g << 5) | b;
        *((unsigned short *)(guiInfo.outBuff + location)) = color;
        break;

    default:
        printf("Don't support the bpp!\n");
        break;
    }

}

U8 GUI_GetPixel(U8 x, U8 y, U32 *ret)
{
    return 0;
}

void  GUI_DrawHline(U8 x0, U8 y0, U8 x1, U32 color)
{
    int x = 0, y = 0;
    int i;

    for(x = x0; x < x1; x++)
        for(y = y0;;)
        {
            long location = x * 4 + y * vinfo.xres * 4;
            *((unsigned int *)(guiInfo.outBuff + location)) = color;
        }

}

void  GUI_DrawVline(U8 x0, U8 y0, U8 y1, U32 color)
{
    int x = 0, y = 0;
    int i;

    for(x = x0;;)
        for(y = y0; y < y1; y++)
        {
            long location = x  + y * vinfo.xres ;
            *((unsigned int *)(guiInfo.outBuff + location)) = color;
        }

}

static void *lidbg_gui_thread_funs(void *data)
{
    size_t rdSize;
    int i, j, k = 0;
    U32 x, y;
    long location;

    struct gui_info *arg = data;

    while(1)
    {
#if 1
        rdSize = read(arg->inFd, arg->rdBuff, RD_BUFF_SIZE);
        printf("Read rdSize:%d\n", rdSize);
        if(rdSize == 0)
            lseek(arg->inFd, 0, SEEK_SET);

        if(rdSize < 0)
            printf("Read file error !\n");

        for(i = 0; i < rdSize; i++)
            lidbg_ui("%c", arg->rdBuff[i]);
#endif
#if 0
        k++;
        lidbg_ui("k = %d\n", k);

#endif
#if 0
        //copy outBuff data to 3-system framebuffer
        for(j = 0; j < 3; j++)

            memcpy(arg->sysFbBaseAddr + arg->outBuffSize * j, arg->outBuff, arg->outBuffSize);
#endif
#if 1
        for(j = 0; j < 3; j++)
            for(y = 0; y < GUI_LCM_YMAX; y++)
                for(x = 0; x < GUI_LCM_XMAX; x++)
                {
                    location = x * 4 + y * vinfo.xres * 4;

                    if(((*((unsigned int *)(arg->outBuff + location))) == DISP_COLOR)/* ||((*((unsigned int *)(arg->outBuff + location))) == SCREEN_CLEAN_COLOR)*/)
                        *((unsigned int *)(arg->sysFbBaseAddr + arg->outBuffSize * j + location)) = *((unsigned int *)(arg->outBuff + location));

                }
#endif
        if(((arg->interval) > 499) && ((arg->interval) < 1001))
            usleep(arg->interval * 1000);	//500~1000ms
        else
            sleep(arg->interval);
    }

    return ((void *) 0);
}

int main(int argc, char **argv)
{
    int fbdp = 0;
    unsigned long screensize = 0;
    int x = 0, y = 0;
    char path[30];
    int pathLen;
    pthread_t ntid;
    int err;

    if(argc < 3)
    {
        printf("Usage: lidbg_gui path interval\n");
        return 1;
    }

    LIDBG_PRINT("lidbg_kmsg +\n");
    //open fb device
    fbdp = open("/dev/graphics/fb0", O_RDWR);
    if (!fbdp)
    {
        printf("Open framebuffer failed !\n");
        exit(1);
    }

    printf("Open framebuffer successed !\n");

    //get fb_var_screeninfo
    if (ioctl(fbdp, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Read fb_var_screeninfo failed !\n");
        exit(1);
    }

    printf("x:%d y:%d, bpp:%d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    guiInfo.outBuffSize = vinfo.xres * vinfo.yres * (vinfo.bits_per_pixel / 8);
    guiInfo.outBuff = (char *)malloc(guiInfo.outBuffSize);
    guiInfo.rdBuff = (char *)malloc(RD_BUFF_SIZE);

    //figure out the size of the screen
    screensize = guiInfo.outBuffSize * SYS_FB_NUM;

    //map the device to memory
    guiInfo.sysFbBaseAddr = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbdp, 0);
    if ((int) guiInfo.sysFbBaseAddr == -1)
    {
        printf("Map framebuffer to memory failed !\n");
        exit(1);
    }

    printf("The framebuffer was mapped to memory successed !\n");
    pathLen = strcpy(path, argv[1]);
    guiInfo.inFd = open(path, O_RDONLY);

    if (!(guiInfo.inFd))
    {
        printf("Open %s failed !\n", path);
        exit(1);
    }

    guiInfo.interval = atoi(argv[2]);
    if (guiInfo.interval  == 0)
    {
        printf("The interval is zero.\n");
        exit(1);
    }


    GUI_SetColor(DISP_COLOR, BACK_COLOR);


    err = pthread_create(&ntid, NULL, lidbg_gui_thread_funs, (void *)&guiInfo);

    if (err != 0)
    {
        printf("can't create thread: %s\n", strerror(err));
        exit(1);
    }

    while(1)
        sleep(60);

    munmap(guiInfo.sysFbBaseAddr, screensize);
    close(fbdp);
    LIDBG_PRINT("lidbg_kmsg -\n");

    return 0;
}


