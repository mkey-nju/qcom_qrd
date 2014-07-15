
#include "lidbg.h"

//struct fb_info *p_registered_fb;
//int num_registered_fb = 0;

//#define GETFBIO  0x4777

//确认fbmem.c已经存在
//EXPORT_SYMBOL(num_registered_fb);
//EXPORT_SYMBOL(registered_fb);


bool display_init_fbi(void)
{
#if 0
    struct fb_fix_screeninfo fb_fixinfo;
    struct fb_var_screeninfo fb_varinfo;
    int fbidx;
    lidbg("display_init_fbi +\n");

    lidbg("\num_registered_fb = %d \n", num_registered_fb);

    for(fbidx = 0; fbidx < num_registered_fb; fbidx++)
    {
        struct fb_info *info = registered_fb[fbidx];

        memcpy(&fb_fixinfo, &(info->fix), sizeof(fb_fixinfo));
        memcpy(&fb_varinfo, &(info->var), sizeof(fb_varinfo));

        lidbg("\nfbi_num = %d \n", fbidx);
        lidbg("id=%s\n", fb_fixinfo.id);
        lidbg("smem_start=%#x\n", (unsigned int)fb_fixinfo.smem_start);
        lidbg("mem_len=%d\n", fb_fixinfo.smem_len);
        lidbg("type=%d\n", fb_fixinfo.type);
        lidbg("type_aux=%d\n", fb_fixinfo.type_aux);
        lidbg("visual=%d\n", fb_fixinfo.visual);
        lidbg("xpanstep=%d\n", fb_fixinfo.xpanstep);
        lidbg("ypanstep=%d\n", fb_fixinfo.ypanstep);
        lidbg("ywrapstep=%d\n", fb_fixinfo.ywrapstep);
        lidbg("line_length=%d\n", fb_fixinfo.line_length);
        lidbg("mmio_start=%#x\n", (unsigned int)fb_fixinfo.mmio_start);
        lidbg("mmio_len=%#x\n", fb_fixinfo.mmio_len);
        lidbg("accel=%d\n", fb_fixinfo.accel);
        lidbg("reserved[0]=%d\n", fb_fixinfo.reserved[0]);
        lidbg("reserved[1]=%d\n", fb_fixinfo.reserved[1]);
        lidbg("reserved[2]=%d\n", fb_fixinfo.reserved[2]);

        lidbg("\nioctl FBIOGET_VSCREENINFO ok\n");
        lidbg("xres=%d\n", fb_varinfo.xres);
        lidbg("yres=%d\n", fb_varinfo.yres);
        lidbg("xres_virtual=%d\n", fb_varinfo.xres_virtual);
        lidbg("yres_virtual=%d\n", fb_varinfo.yres_virtual);
        lidbg("xoffset=%d\n", fb_varinfo.xoffset);
        lidbg("yoffset=%d\n", fb_varinfo.yoffset);
        lidbg("bits_per_pixel=%d\n", fb_varinfo.bits_per_pixel);
        lidbg("grayscale=%d\n", fb_varinfo.grayscale);
        lidbg("red=%#x\n", fb_varinfo.red);
        lidbg("green=%#x\n", fb_varinfo.green);
        lidbg("blue=%#x\n", fb_varinfo.blue);
        lidbg("transp=%d\n", fb_varinfo.transp);
        lidbg("nonstd=%d\n", fb_varinfo.nonstd);
        lidbg("activate=%d\n", fb_varinfo.activate);
        lidbg("height=%d\n", fb_varinfo.height);
        lidbg("width=%d\n", fb_varinfo.width);
        lidbg("accel_flags=%d\n", fb_varinfo.accel_flags);
        lidbg("pixclock=%d\n", fb_varinfo.pixclock);
        lidbg("left_margin=%d\n", fb_varinfo.left_margin);
        lidbg("right_margin=%d\n", fb_varinfo.right_margin);
        lidbg("upper_margin=%d\n", fb_varinfo.upper_margin);
        lidbg("lower_margin=%d\n", fb_varinfo.lower_margin);
        lidbg("hsync_len=%d\n", fb_varinfo.hsync_len);
        lidbg("vsync_len=%d\n", fb_varinfo.vsync_len);
        lidbg("sync=%d\n", fb_varinfo.sync);
        lidbg("vmode=%d\n", fb_varinfo.vmode);
        lidbg("rotate=%d\n", fb_varinfo.rotate);
        lidbg("reserved[0]=%d\n", fb_varinfo.reserved[0]);
        lidbg("reserved[1]=%d\n", fb_varinfo.reserved[1]);
        lidbg("reserved[2]=%d\n", fb_varinfo.reserved[2]);
        lidbg("reserved[3]=%d\n", fb_varinfo.reserved[3]);
        lidbg("reserved[4]=%d\n", fb_varinfo.reserved[4]);

        lidbg("\n\n");

    }

    lidbg("display_init_fbi-.\n");
#endif
    return 1;
}


#if 0

bool display_init_fbi()
{
    struct fb_info *tmp = NULL;

    struct file *file = NULL;
    char *fbdev = NULL;
    ssize_t result;
    mm_segment_t old_fs;
    u32 fbi_addr = 0;


    struct fb_fix_screeninfo fb_fixinfo;
    struct fb_var_screeninfo fb_varinfo;

    lidbg("display_init_fbi+.\n");


    fbdev = "/dev/graphics/fb0";

    lidbg("open %s\n", fbdev);

    file = filp_open(fbdev, O_RDWR, 0);

    if(IS_ERR(file))
    {
        lidbg("open FrameBuffer device failed.\n");

        return 0;
    }

    lidbg("open FrameBuffer device successfully!\n");

    BEGIN_KMEM;

    result = file->f_op->ioctl(file->f_dentry->d_inode, file, GETFBIO, (long unsigned int)&fbi_addr);

    END_KMEM;

    filp_close(file, 0);

    lidbg("fbi_addr=0x%x\n", fbi_addr);

    registered_fb = (struct fb_info *)fbi_addr;
    tmp = registered_fb;

    while(tmp)
    {
        memcpy(&fb_fixinfo, &(tmp->fix), sizeof(fb_fixinfo));
        memcpy(&fb_varinfo, &(tmp->var), sizeof(fb_varinfo));


        lidbg("\nfbi_num = %d \n", num_registered_fb);
        lidbg("id=%s\n", fb_fixinfo.id);
        lidbg("smem_start=%#x\n", (unsigned int)fb_fixinfo.smem_start);
        lidbg("mem_len=%d\n", fb_fixinfo.smem_len);
        lidbg("type=%d\n", fb_fixinfo.type);
        lidbg("type_aux=%d\n", fb_fixinfo.type_aux);
        lidbg("visual=%d\n", fb_fixinfo.visual);
        lidbg("xpanstep=%d\n", fb_fixinfo.xpanstep);
        lidbg("ypanstep=%d\n", fb_fixinfo.ypanstep);
        lidbg("ywrapstep=%d\n", fb_fixinfo.ywrapstep);
        lidbg("line_length=%d\n", fb_fixinfo.line_length);
        lidbg("mmio_start=%#x\n", (unsigned int)fb_fixinfo.mmio_start);
        lidbg("mmio_len=%#x\n", fb_fixinfo.mmio_len);
        lidbg("accel=%d\n", fb_fixinfo.accel);
        lidbg("reserved[0]=%d\n", fb_fixinfo.reserved[0]);
        lidbg("reserved[1]=%d\n", fb_fixinfo.reserved[1]);
        lidbg("reserved[2]=%d\n", fb_fixinfo.reserved[2]);

        lidbg("\nioctl FBIOGET_VSCREENINFO ok\n");
        lidbg("xres=%d\n", fb_varinfo.xres);
        lidbg("yres=%d\n", fb_varinfo.yres);
        lidbg("xres_virtual=%d\n", fb_varinfo.xres_virtual);
        lidbg("yres_virtual=%d\n", fb_varinfo.yres_virtual);
        lidbg("xoffset=%d\n", fb_varinfo.xoffset);
        lidbg("yoffset=%d\n", fb_varinfo.yoffset);
        lidbg("bits_per_pixel=%d\n", fb_varinfo.bits_per_pixel);
        lidbg("grayscale=%d\n", fb_varinfo.grayscale);
        lidbg("red=%#x\n", fb_varinfo.red);
        lidbg("green=%#x\n", fb_varinfo.green);
        lidbg("blue=%#x\n", fb_varinfo.blue);
        lidbg("transp=%d\n", fb_varinfo.transp);
        lidbg("nonstd=%d\n", fb_varinfo.nonstd);
        lidbg("activate=%d\n", fb_varinfo.activate);
        lidbg("height=%d\n", fb_varinfo.height);
        lidbg("width=%d\n", fb_varinfo.width);
        lidbg("accel_flags=%d\n", fb_varinfo.accel_flags);
        lidbg("pixclock=%d\n", fb_varinfo.pixclock);
        lidbg("left_margin=%d\n", fb_varinfo.left_margin);
        lidbg("right_margin=%d\n", fb_varinfo.right_margin);
        lidbg("upper_margin=%d\n", fb_varinfo.upper_margin);
        lidbg("lower_margin=%d\n", fb_varinfo.lower_margin);
        lidbg("hsync_len=%d\n", fb_varinfo.hsync_len);
        lidbg("vsync_len=%d\n", fb_varinfo.vsync_len);
        lidbg("sync=%d\n", fb_varinfo.sync);
        lidbg("vmode=%d\n", fb_varinfo.vmode);
        lidbg("rotate=%d\n", fb_varinfo.rotate);
        lidbg("reserved[0]=%d\n", fb_varinfo.reserved[0]);
        lidbg("reserved[1]=%d\n", fb_varinfo.reserved[1]);
        lidbg("reserved[2]=%d\n", fb_varinfo.reserved[2]);
        lidbg("reserved[3]=%d\n", fb_varinfo.reserved[3]);
        lidbg("reserved[4]=%d\n", fb_varinfo.reserved[4]);

        lidbg("\n\n");


        num_registered_fb++;
        tmp++;

    }

    lidbg("display_init_fbi-.\n");

    return 1;
}
#endif






#if 0
bool display_get_fbi(u32 layer_num, struct fb_info *pfbi)
{

    struct file *file = NULL;
    char *fbdev = NULL;
    ssize_t result;
    mm_segment_t old_fs;
    u32 fbi_addr = 0;

    struct fb_fix_screeninfo fb_fixinfo;
    struct fb_var_screeninfo fb_varinfo;


    lidbg("display_get_fbi+.\n");


    switch (layer_num)
    {
    case 0:
        fbdev = "/dev/graphics/fb0";
        break;

    case 1:
        fbdev = "/dev/graphics/fb1";
        break;

    case 2:
        fbdev = "/dev/graphics/fb2";
        break;

    case 3:
        fbdev = "/dev/graphics/fb3";
        break;

    case 4:
        fbdev = "/dev/graphics/fb4";
        break;

    case 5:
        fbdev = "/dev/graphics/fb5";
        break;
    default:
        return 0;

    }

    lidbg("open %s\n", fbdev);


    file = filp_open(fbdev, O_RDWR, 0);

    if(IS_ERR(file))
    {
        lidbg("open FrameBuffer device failed.\n");

        return 0;
    }

    lidbg("open FrameBuffer device successfully!\n");

    BEGIN_KMEM;

    result = file->f_op->ioctl(file->f_dentry->d_inode, file, GETFBIO, (long unsigned int)&fbi_addr);

    END_KMEM;

    filp_close(file, 0);

    lidbg("fbi_addr=0x%x\n", fbi_addr);

    pfbi = (struct fb_info *)fbi_addr;

    memcpy(&fb_fixinfo, &(pfbi->fix), sizeof(fb_fixinfo));
    memcpy(&fb_varinfo, &(pfbi->var), sizeof(fb_varinfo));


    lidbg("\nioctl GETFBIO ok\n");
    lidbg("id=%s\n", fb_fixinfo.id);
    lidbg("smem_start=%#x\n", (unsigned int)fb_fixinfo.smem_start);
    lidbg("mem_len=%d\n", fb_fixinfo.smem_len);
    lidbg("type=%d\n", fb_fixinfo.type);
    lidbg("type_aux=%d\n", fb_fixinfo.type_aux);
    lidbg("visual=%d\n", fb_fixinfo.visual);
    lidbg("xpanstep=%d\n", fb_fixinfo.xpanstep);
    lidbg("ypanstep=%d\n", fb_fixinfo.ypanstep);
    lidbg("ywrapstep=%d\n", fb_fixinfo.ywrapstep);
    lidbg("line_length=%d\n", fb_fixinfo.line_length);
    lidbg("mmio_start=%#x\n", (unsigned int)fb_fixinfo.mmio_start);
    lidbg("mmio_len=%#x\n", fb_fixinfo.mmio_len);
    lidbg("accel=%d\n", fb_fixinfo.accel);
    lidbg("reserved[0]=%d\n", fb_fixinfo.reserved[0]);
    lidbg("reserved[1]=%d\n", fb_fixinfo.reserved[1]);
    lidbg("reserved[2]=%d\n", fb_fixinfo.reserved[2]);

    lidbg("\nioctl FBIOGET_VSCREENINFO ok\n");
    lidbg("xres=%d\n", fb_varinfo.xres);
    lidbg("yres=%d\n", fb_varinfo.yres);
    lidbg("xres_virtual=%d\n", fb_varinfo.xres_virtual);
    lidbg("yres_virtual=%d\n", fb_varinfo.yres_virtual);
    lidbg("xoffset=%d\n", fb_varinfo.xoffset);
    lidbg("yoffset=%d\n", fb_varinfo.yoffset);
    lidbg("bits_per_pixel=%d\n", fb_varinfo.bits_per_pixel);
    lidbg("grayscale=%d\n", fb_varinfo.grayscale);
    lidbg("red=%#x\n", fb_varinfo.red);
    lidbg("green=%#x\n", fb_varinfo.green);
    lidbg("blue=%#x\n", fb_varinfo.blue);
    lidbg("transp=%d\n", fb_varinfo.transp);
    lidbg("nonstd=%d\n", fb_varinfo.nonstd);
    lidbg("activate=%d\n", fb_varinfo.activate);
    lidbg("height=%d\n", fb_varinfo.height);
    lidbg("width=%d\n", fb_varinfo.width);
    lidbg("accel_flags=%d\n", fb_varinfo.accel_flags);
    lidbg("pixclock=%d\n", fb_varinfo.pixclock);
    lidbg("left_margin=%d\n", fb_varinfo.left_margin);
    lidbg("right_margin=%d\n", fb_varinfo.right_margin);
    lidbg("upper_margin=%d\n", fb_varinfo.upper_margin);
    lidbg("lower_margin=%d\n", fb_varinfo.lower_margin);
    lidbg("hsync_len=%d\n", fb_varinfo.hsync_len);
    lidbg("vsync_len=%d\n", fb_varinfo.vsync_len);
    lidbg("sync=%d\n", fb_varinfo.sync);
    lidbg("vmode=%d\n", fb_varinfo.vmode);
    lidbg("rotate=%d\n", fb_varinfo.rotate);
    lidbg("reserved[0]=%d\n", fb_varinfo.reserved[0]);
    lidbg("reserved[1]=%d\n", fb_varinfo.reserved[1]);
    lidbg("reserved[2]=%d\n", fb_varinfo.reserved[2]);
    lidbg("reserved[3]=%d\n", fb_varinfo.reserved[3]);
    lidbg("reserved[4]=%d\n", fb_varinfo.reserved[4]);

    lidbg("display_get_fbi-.\n");

    return 1;
}

#endif

#if 0
bool display_get_screen_info(u32 layer_num, struct fb_fix_screeninfo *pfb_fixinfo, struct fb_var_screeninfo *pfb_varinfo)
{
    struct file *file = NULL;
    char *fbdev = NULL;
    ssize_t result;
    mm_segment_t old_fs;

    struct fb_fix_screeninfo fb_fixinfo;
    struct fb_var_screeninfo fb_varinfo;


    lidbg("display_get_screen_info+.\n");


    switch (layer_num)
    {
    case 0:
        fbdev = "/dev/graphics/fb0";
        break;

    case 1:
        fbdev = "/dev/graphics/fb1";
        break;

    case 2:
        fbdev = "/dev/graphics/fb2";
        break;

    case 3:
        fbdev = "/dev/graphics/fb3";
        break;

    case 4:
        fbdev = "/dev/graphics/fb4";
        break;

    case 5:
        fbdev = "/dev/graphics/fb5";
        break;
    default:
        return 0;

    }

    lidbg("open %s\n", fbdev);


    file = filp_open(fbdev, O_RDWR, 0);

    if(IS_ERR(file))
    {
        lidbg("open FrameBuffer device failed.\n");

        return 0;
    }

    lidbg("open FrameBuffer device successfully!\n");

    BEGIN_KMEM;

    result = file->f_op->ioctl(file->f_dentry->d_inode, file, FBIOGET_FSCREENINFO, (long unsigned int)&fb_fixinfo);
    result = file->f_op->ioctl(file->f_dentry->d_inode, file, FBIOGET_VSCREENINFO, (long unsigned int)&fb_varinfo);

    END_KMEM;

    filp_close(file, 0);

    lidbg("\nioctl FBIOGET_FSCREENINFO ok\n");
    lidbg("id=%s\n", fb_fixinfo.id);
    lidbg("smem_start=%#x\n", (unsigned int)fb_fixinfo.smem_start);
    lidbg("mem_len=%d\n", fb_fixinfo.smem_len);
    lidbg("type=%d\n", fb_fixinfo.type);
    lidbg("type_aux=%d\n", fb_fixinfo.type_aux);
    lidbg("visual=%d\n", fb_fixinfo.visual);
    lidbg("xpanstep=%d\n", fb_fixinfo.xpanstep);
    lidbg("ypanstep=%d\n", fb_fixinfo.ypanstep);
    lidbg("ywrapstep=%d\n", fb_fixinfo.ywrapstep);
    lidbg("line_length=%d\n", fb_fixinfo.line_length);
    lidbg("mmio_start=%#x\n", (unsigned int)fb_fixinfo.mmio_start);
    lidbg("mmio_len=%#x\n", fb_fixinfo.mmio_len);
    lidbg("accel=%d\n", fb_fixinfo.accel);
    lidbg("reserved[0]=%d\n", fb_fixinfo.reserved[0]);
    lidbg("reserved[1]=%d\n", fb_fixinfo.reserved[1]);
    lidbg("reserved[2]=%d\n", fb_fixinfo.reserved[2]);

    lidbg("\nioctl FBIOGET_VSCREENINFO ok\n");
    lidbg("xres=%d\n", fb_varinfo.xres);
    lidbg("yres=%d\n", fb_varinfo.yres);
    lidbg("xres_virtual=%d\n", fb_varinfo.xres_virtual);
    lidbg("yres_virtual=%d\n", fb_varinfo.yres_virtual);
    lidbg("xoffset=%d\n", fb_varinfo.xoffset);
    lidbg("yoffset=%d\n", fb_varinfo.yoffset);
    lidbg("bits_per_pixel=%d\n", fb_varinfo.bits_per_pixel);
    lidbg("grayscale=%d\n", fb_varinfo.grayscale);
    lidbg("red=%#x\n", fb_varinfo.red);
    lidbg("green=%#x\n", fb_varinfo.green);
    lidbg("blue=%#x\n", fb_varinfo.blue);
    lidbg("transp=%d\n", fb_varinfo.transp);
    lidbg("nonstd=%d\n", fb_varinfo.nonstd);
    lidbg("activate=%d\n", fb_varinfo.activate);
    lidbg("height=%d\n", fb_varinfo.height);
    lidbg("width=%d\n", fb_varinfo.width);
    lidbg("accel_flags=%d\n", fb_varinfo.accel_flags);
    lidbg("pixclock=%d\n", fb_varinfo.pixclock);
    lidbg("left_margin=%d\n", fb_varinfo.left_margin);
    lidbg("right_margin=%d\n", fb_varinfo.right_margin);
    lidbg("upper_margin=%d\n", fb_varinfo.upper_margin);
    lidbg("lower_margin=%d\n", fb_varinfo.lower_margin);
    lidbg("hsync_len=%d\n", fb_varinfo.hsync_len);
    lidbg("vsync_len=%d\n", fb_varinfo.vsync_len);
    lidbg("sync=%d\n", fb_varinfo.sync);
    lidbg("vmode=%d\n", fb_varinfo.vmode);
    lidbg("rotate=%d\n", fb_varinfo.rotate);
    lidbg("reserved[0]=%d\n", fb_varinfo.reserved[0]);
    lidbg("reserved[1]=%d\n", fb_varinfo.reserved[1]);
    lidbg("reserved[2]=%d\n", fb_varinfo.reserved[2]);
    lidbg("reserved[3]=%d\n", fb_varinfo.reserved[3]);
    lidbg("reserved[4]=%d\n", fb_varinfo.reserved[4]);


    memcpy(pfb_fixinfo, &fb_fixinfo, sizeof(fb_fixinfo));
    memcpy(pfb_varinfo, &fb_varinfo, sizeof(fb_varinfo));
    lidbg("display_get_screen_info-.\n");
    return 1;

}
#endif


typedef enum
{
    RGB565 = 2,
    RGB888 = 4,
    //ARGB888 = 5,
}
RGB_OUTPUT_TYPE;


void color_degree(struct fb_fix_screeninfo *pfb_fixinfo, struct fb_var_screeninfo *pfb_varinfo)
{
#if 0
    u32 lcd_phy_buffer;
    u32 *plcd_vir_buffer;

    RGB_OUTPUT_TYPE rgb_type;
    u32 R_value, G_value, B_value, i, j;
    u8 *p1;
    u8 *p2;
    u32 DEGREE, PIX_DEGREE, LCD_WIDTH, LCD_HEIGHT, LINE_BYTE_FULL, RGBBYTE, PER_COLOR_HEIGHT;
    char *uiLCDBufferAddress;

    lidbg(" ColorDegree+\n");

    if((pfb_fixinfo->smem_start == 0) || (pfb_fixinfo->smem_len == 0))
        return;

    lcd_phy_buffer = pfb_fixinfo->smem_start;
    plcd_vir_buffer = ioremap(lcd_phy_buffer, pfb_varinfo->xres * pfb_varinfo->yres * pfb_varinfo->bits_per_pixel);
    uiLCDBufferAddress = (char *)(plcd_vir_buffer);

    lidbg("lcd_phy_buffer:0x%x\n", lcd_phy_buffer);
    lidbg("lcd_vir_buffer:0x%x\n", (u32)plcd_vir_buffer);


    LCD_WIDTH = pfb_varinfo->xres;
    LCD_HEIGHT = pfb_varinfo->yres;
    PER_COLOR_HEIGHT = (LCD_HEIGHT / 4);

    lidbg( "LCD_WIDTH %d\n", LCD_WIDTH);
    lidbg("LCD_HEIGHT %d\n", LCD_HEIGHT);

    rgb_type = (pfb_varinfo->bits_per_pixel) / 8;

    if(rgb_type == RGB565)
    {
        typedef  u16  PIX_BYTE;
        RGBBYTE = 2;
        DEGREE = 32;
        PIX_DEGREE = (LCD_WIDTH / DEGREE);
        LINE_BYTE_FULL = (LCD_WIDTH * RGBBYTE);

        lidbg (" RGB565\n");

#if  1
        //R
        R_value = G_value = B_value = 0;

        for(i = 0; i < DEGREE; i++)
        {

            for(j = 0; j < PIX_DEGREE; j ++)

            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB555(R_value, G_value, B_value);
            }
            R_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress);

        for(j = 0; j < 120; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }
#if  1

        //G
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB555(R_value, G_value, B_value);
            }
            G_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }


        //B
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT * 2) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB555(R_value, G_value, B_value);
            }
            B_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT * 2);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }

#if  1

        //BGB	灰阶
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT * 3) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB555(R_value, G_value, B_value);
            }
            R_value ++;
            G_value ++;
            B_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT * 3);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }
#endif

#endif
#endif
    }


    else if(rgb_type == RGB888)
    {
        typedef  u32  PIX_BYTE;
        RGBBYTE = 4;
        DEGREE = 256;
        PIX_DEGREE = (LCD_WIDTH / DEGREE);
        LINE_BYTE_FULL = (LCD_WIDTH * RGBBYTE);
        lidbg (" RGB888\n");

#if  1
        //R
        R_value = G_value = B_value = 0;

        for(i = 0; i < DEGREE; i++)
        {

            for(j = 0; j < PIX_DEGREE; j ++)

            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB888(R_value, G_value, B_value);
            }
            R_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress);

        for(j = 0; j < 120; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }
#if  1

        //G
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB888(R_value, G_value, B_value);
            }
            G_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }


        //B
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT * 2) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB888(R_value, G_value, B_value);
            }
            B_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT * 2);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }

#if  1

        //BGB	灰阶
        R_value = G_value = B_value = 0;
        for(i = 0; i < DEGREE; i++)
        {
            for(j = 0; j < PIX_DEGREE; j ++)
            {
                *(  (PIX_BYTE *)(uiLCDBufferAddress) + LCD_WIDTH * (PER_COLOR_HEIGHT * 3) + (i * PIX_DEGREE + j)  ) = (PIX_BYTE)GET_COLOR_RGB888(R_value, G_value, B_value);
            }
            R_value ++;
            G_value ++;
            B_value ++;
        }

        p1 = p2 = (u8 *)(uiLCDBufferAddress) + LINE_BYTE_FULL * (PER_COLOR_HEIGHT * 3);

        for(j = 0; j < PER_COLOR_HEIGHT; j++)
        {
            p2 = p2 + LINE_BYTE_FULL;
            for(i = 0; i < LINE_BYTE_FULL; i++)
            {
                *(p2 + i) = *(p1 + i);
            }

        }
#endif

#endif
#endif

    }

    iounmap(plcd_vir_buffer);
    lidbg(" ColorDegree-\n");
#endif

}




void lidbg_display_main(int argc, char **argv)
{


    //struct fb_fix_screeninfo fb_fixinfo;
    //struct fb_var_screeninfo fb_varinfo;
    u32 fb_id;

    //struct fb_info *info = registered_fb[fb_id];
    // int result;

    if(argc < 2)
    {
        lidbg("Usage:\n");
        lidbg("color_degree fb_id\n");
        lidbg("get_fbi fb_id\n");
        return;
    }


    display_init_fbi();

    fb_id = simple_strtoul(argv[1], 0, 0);
#if 0
    result = display_get_screen_info(layer_num, &fb_fixinfo, &fb_varinfo);


    if(!result)
    {
        lidbg("display_get_fbi err!!\n");
        return;
    }
#endif
#if 0

    result = display_get_fbi(layer_num, &fbi);

    if(!result)
    {
        lidbg("display_get_fbi err!!\n");
        return;
    }
#endif



    if(!strcmp(argv[0], "color_degree"))
    {
        struct fb_info *info = registered_fb[fb_id];
        color_degree(&(info->fix), &(info->var));

    }
#if 0
    if(!strcmp(argv[0], "get_fbi"))
    {
        struct fb_info fbi;
        display_get_fbi(fb_id, &fbi);

    }
#endif
    /*
    if(!strcmp(argv[0], "fb_set_par"))
    {

    	if (registered_fb[fb_id]->fbops->fb_set_par)
    	{
    		registered_fb[fb_id]->fbops->fb_set_par(registered_fb[fb_id]);
    	}
    }
    */

}




int  soc_get_screen_res(u32 *screen_x, u32 *screen_y)
{

    int fbidx;
    struct fb_var_screeninfo fb_varinfo;

    fb_varinfo.xres = 0;
    fb_varinfo.yres = 0;

    lidbg("num_registered_fb = %d \n", num_registered_fb);

    for(fbidx = 0; fbidx < num_registered_fb; fbidx++)
    {
        struct fb_info *info = registered_fb[fbidx];
        memcpy(&fb_varinfo, &(info->var), sizeof(fb_varinfo));

        lidbg("xres=%d\n", fb_varinfo.xres);
        lidbg("yres=%d\n", fb_varinfo.yres);

        lidbg("\n");
    }
    if((fb_varinfo.xres == 0) || (fb_varinfo.yres == 0))
    {
        lidbg("soc_get_screen_res fail!!\n");
        return 0;
    }
    else
    {
        *screen_x = fb_varinfo.xres;
        *screen_y = fb_varinfo.yres;

    }
    return 1;

}


EXPORT_SYMBOL(lidbg_display_main);
EXPORT_SYMBOL(soc_get_screen_res);



