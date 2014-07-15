//-------------------------------------------------------------------------
// 文件名：
// 功能：
// 备注：
//
//
//         By  李素伟
//             lisuwei@flyaudio.cn
//-------------------------------------------------------------------------



#include "debug_includes.h"

static int lcd_x = 0;
static int lcd_y = 0;
static int cur_area = 0;

void guiCleanLine(int y)
{
    int x, z;
    if((y + 8) <= GUI_LCM_YMAX)

        for(x = 0; x < GUI_LCM_XMAX; x++)
            for(z = y; z < (y + 8); z++)
                GUI_Point(x, z, SCREEN_CLEAN_COLOR);
}
void GUI_Clean()
{
    lcd_x = 0;
    lcd_y = 0;
    cur_area = 0;
    int x, y;

    for(y = 0; y < GUI_LCM_YMAX; y++)
        for(x = 0; x < GUI_LCM_XMAX; x++)
            GUI_Point(x, y, SCREEN_CLEAN_COLOR);
    //   GUI_LCDFill_COLOR(&LayerConfig[LAYER_DBGGUI_LAYER], GET_COLORKEY(DBGGUI_C_KEY_R, DBGGUI_C_KEY_G, DBGGUI_C_KEY_B));

}


void GUI_Printf(U8 ch)
{

#define AREA_NUM (1)
#define AREA_WIDTH (1024/AREA_NUM)
#define AREA_HEIGHT (600)

    if((lcd_x - (cur_area + 1)*AREA_WIDTH) > 0)
    {
        lcd_x = cur_area * AREA_WIDTH;
        lcd_y += 8;
#if CLEAN_LINE
        guiCleanLine(lcd_y);
#endif
    }
    if(lcd_y >= AREA_HEIGHT)
    {
        cur_area++;
        lcd_x = cur_area * AREA_WIDTH;
        lcd_y = 0;

        if(AREA_NUM == cur_area)
        {
            cur_area = 0;
            lcd_x = 0;
            lcd_y = 0;
#if CLEAN_LINE
            guiCleanLine(lcd_y);
#else
            GUI_Clean();
#endif
        }
    }

    if(ch == '\r')
        return;

    if(ch == '\n')
    {
        lcd_y += 8;
        lcd_x = cur_area * AREA_WIDTH;
#if CLEAN_LINE
        guiCleanLine(lcd_y);
#endif
        return;
    }

    GUI_PutChar8_8(lcd_x, lcd_y, ch);
    lcd_x += 7;

}

void lidbg_ui(const char *fmt, ... )
{
    va_list args;
    int n;
    char printbuffer[256];
    char *p;

    va_start ( args, fmt );
    n = vsprintf ( printbuffer, (const char *)fmt, args );
    va_end ( args );

    p = printbuffer;
    while (*p)
    {
        GUI_Printf((unsigned char)*p++);
    }
}








