


#ifndef _lidbg_display_h
#define _lidbg_display_h


#define GET_COLOR_RGB555(r,g,b)  ((((r)&0x1F)<<11)|(((g)&0x3F)<<6)|((b)&0x1F))//555
#define GET_COLOR_RGB565(r,g,b)  ((((r)&0x1F)<<11)|(((g)&0x3F)<<5)|((b)&0x1F))//565
#define GET_COLOR_RGB888(r,g,b)  ((((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF))//888
#define GET_COLOR_ARGB888(a,r,g,b)  ((((a)&0xFF)<<24)|(((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF))//A888

#define RGB888_565(r,g,b)  ((((r>>3)&0x1F)<<11)|(((g>>2)&0x3F)<<5)|((b>>3)&0x1F))//888-->565 取高位对齐


void lidbg_display_main(int argc, char **argv);

int  soc_get_screen_res(u32 *screen_x, u32 *screen_y);



#endif

