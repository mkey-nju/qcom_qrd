#ifndef __TC358746XBG_CONFIG__
#define __TC358746XBG_CONFIG__
#include "TC358746XBG.h"

const struct tc358746xbg_register_t tc358746_id[] =
{
    {	0x0000	,	0x4400	,	register_value_width_16		},
};
const struct tc358746xbg_register_t PALp_init_tab[] =
{

    //	add		value(dec)		register width			value(hex)

    //{	0x0002	,	1 	,	register_value_width_16		},   //	0001

    //{	0x0002	,	0 	,	register_value_width_16		},   //	0000

    {	0x0016	,	4363 	,	register_value_width_16		},   //	110B

    {	0x0018	,	2579 	,	register_value_width_16		},   //	0A13

    {	0x0006	,	176 	,	register_value_width_16		},   //	0b0

    {	0x0008	,	96 	,	register_value_width_16		},   //	0060

    {	0x0022	,	1469 	,	register_value_width_16		},   //	5A0

    {0x0100, 0x203, register_value_width_32}, //clck current
    {0x0104, 0x203, register_value_width_32}, //lane0 current
    {0x0108, 0x203, register_value_width_32}, //lane1 current


    {	0x0140	,	0 	,	register_value_width_32		},   //	00000000

    {	0x0144	,	0 	,	register_value_width_32		},   //	00000000

    {	0x0148	,	0 	,	register_value_width_32		},   //	00000000

    {	0x014c	,	1 	,	register_value_width_32		},   //	00000001

    {	0x0150	,	1 	,	register_value_width_32		},   //	00000001

    {	0x0210	,	5120 	,	register_value_width_32		},   //	0001400

    {	0x0214	,	2 	,	register_value_width_32		},   //	00000002

    {	0x0218	,	4098 	,	register_value_width_32		},   //	00001002

    {	0x021C	,	0 	,	register_value_width_32		},   //	00000000

    {	0x0220	,	2 	,	register_value_width_32		},   //	00000002

    {	0x0224	,	17152 	,	register_value_width_32		},   //	00004300

    {	0x0228	,	8 	,	register_value_width_32		},   //	00000008

    {	0x022C	,	0 	,	register_value_width_32		},   //	00000000

    {	0x0234	,	7 	,	register_value_width_32		},   //	0007

    {	0x0238	,	1 	,	register_value_width_32		},   //	0001

    {	0x0204	,	1 	,	register_value_width_32		},   //	00000001

    {	0x0518	,	1 	,	register_value_width_32		},   //	00000001

    {	0x0500	,	0xA30080A2 	,	register_value_width_32		},   //	A30080A2

    {	0x0004	,	4164 	,	register_value_width_16		},   //	1044

    {0xffff},
};

const struct tc358746xbg_register_t NTSCp_init_tab[] =
{
    //	add		value(dec)		register width			value(hex)
    //0x0002	,	1 	,	register_value_width_16		},   //	0001
    //0x0002	,	0 	,	register_value_width_16		},   //	0000
    {	0x0016	,	4395 	,	register_value_width_16		},   //	112B
    {	0x0018	,	2579 	,	register_value_width_16		},   //	0A13
    {	0x0006	,	196 	,	register_value_width_16		},   //	00C4
    {	0x0008	,	96 	,	register_value_width_16		},   //	0060
    {	0x0022	,	1500 	,	register_value_width_16		},   //	5A0
    {0x0100, 0x200, register_value_width_32}, //clck current
    {0x0104, 0x200, register_value_width_32}, //lane0 current
    {0x0108, 0x200, register_value_width_32}, //lane1 current
    {	0x0140	,	0 	,	register_value_width_32		},   //	00000000
    {	0x0144	,	0 	,	register_value_width_32		},   //	00000000
    {	0x0148	,	0 	,	register_value_width_32		},   //	00000000
    {	0x014c	,	1 	,	register_value_width_32		},   //	00000001
    {	0x0150	,	1 	,	register_value_width_32		},   //	00000001
    {	0x0210	,	5760 	,	register_value_width_32		},   //	0001680
    {	0x0214	,	2 	,	register_value_width_32		},   //	00000002
    {	0x0218	,	4098 	,	register_value_width_32		},   //	00001002
    {	0x021C	,	0 	,	register_value_width_32		},   //	00000000
    {	0x0220	,	2 	,	register_value_width_32		},   //	00000002
    {	0x0224	,	20224 	,	register_value_width_32		},   //	00004f00
    {	0x0228	,	7 	,	register_value_width_32		},   //	00000007
    {	0x022C	,	1 	,	register_value_width_32		},   //	00000001
    {	0x0234	,	7 	,	register_value_width_32		},   //	0007
    {	0x0238	,	1 	,	register_value_width_32		},   //	0001
    {	0x0204	,	1 	,	register_value_width_32		},   //	00000001
    {	0x0518	,	1 	,	register_value_width_32		},   //	00000001
    {	0x0500	,	0xa30080a2 	,	register_value_width_32		},   //	A30080A2
    {	0x0004	,	4164 	,	register_value_width_16		},   //	1044
    {0xffff},
};

const struct tc358746xbg_register_t tc358746xbg_show_colorbar_config_tab[] =
{
    //80 pixel of black
    {0x00e8, 0x7f, register_value_width_16},
    {0x00e8, 0x7f, register_value_width_16},
    //80 pixel of blue
    {0x00e8, 0xff, register_value_width_16},
    {0x00e8, 0x0, register_value_width_16},
    //80 pixel of red
    {0x00e8, 0x0, register_value_width_16},
    {0x00e8, 0xff, register_value_width_16},
    //80 pixel of pink
    {0x00e8, 0x7fff, register_value_width_16},
    {0x00e8, 0x7ff0, register_value_width_16},
    //80 pixel of green
    {0x00e8, 0x7f00, register_value_width_16},
    {0x00e8, 0x7f00, register_value_width_16},
    //80 pixel of light blue
    {0x00e8, 0xc0ff, register_value_width_16},
    {0x00e8, 0xc000, register_value_width_16},
    //80 pixel of yellow
    {0x00e8, 0xff00, register_value_width_16},
    {0x00e8, 0xffff, register_value_width_16},
    //80 pixel of white
    {0x00e8, 0xff7f, register_value_width_16},
    {0x00e8, 0xff7f, register_value_width_16},
};

const struct tc358746xbg_register_t toshiba_initall_list[] =
{
    {0x00e0, 0x0, register_value_width_16},
    //	{0x0002,0xf,register_value_width_16},
    //	{0x0002,0x0,register_value_width_16},

    {0x0016, 0x10ff, register_value_width_16},
    {0x0018, 0x0a13, register_value_width_16},

    {0x0100, 0x202, register_value_width_32},
    //  {0x0102,0x0,register_value_width_16},

    {0x0104, 0x242, register_value_width_32},
    //  {0x0106,0x0,register_value_width_16},

    {0x0108, 0x242, register_value_width_32},
    //  {0x010a,0x0,register_value_width_16},

    {0x010c, 0x242, register_value_width_32},
    //  {0x010e,0x0,register_value_width_16},

    {0x0110, 0x242, register_value_width_32},
    // {0x0112,0x0,register_value_width_16},


    {0x0140, 0x0, register_value_width_32},
    //	{0x0142,0x0,register_value_width_32},

    {0x0150, 0x0, register_value_width_32},
    //	{0x0152,0x0,register_value_width_32},

    {0x014c, 0x0, register_value_width_32},
    //	{0x014e,0x0,register_value_width_32},
    {0x0148, 0x0, register_value_width_32},
    //	{0x014a,0x0,register_value_width_32},
    {0x0144, 0x0, register_value_width_32},
    //	{0x0146,0x0,register_value_width_32},


    {0x0210, 0x1300, register_value_width_32},
    //	{0x0212,0x0,register_value_width_32},

    {0x0214, 0x2, register_value_width_32},
    //	{0x0216,0x0,register_value_width_32},

    {0x0218, 0xd01, register_value_width_32},
    //	{0x021a,0x0,register_value_width_32},

    {0x021c, 0x0, register_value_width_32},
    //	{0x021e,0x0,register_value_width_32},

    {0x0220, 0x2, register_value_width_32},
    //	{0x0222,0x0,register_value_width_32},

    {0x0228, 0x7, register_value_width_32},
    //	{0x022a,0x0,register_value_width_32},

    {0x022c, 0x0, register_value_width_32},
    //	{0x022e,0x0,register_value_width_32},

    {0x0234, 0x1f, register_value_width_32},
    //	{0x0236,0x0,register_value_width_32},

    {0x0238, 0x0, register_value_width_32},
    //	{0x023a,0x0,register_value_width_32},

    {0x0204, 0x1, register_value_width_32},
    //{0x0206,0x0,register_value_width_32},

    {0x0518, 0x1, register_value_width_32},
    //	{0x051a,0x0,register_value_width_32},

    {0x0500, 0xa30080a2, register_value_width_32},

    {0x0008, 0x60, register_value_width_16},
    {0x0022, 0x5a0, register_value_width_16},
    //COLORBAR Logics
    {0x00e0, 0x8200, register_value_width_16},
    {0x00e2, 0x600, register_value_width_16},
    {0x00e4, 0x40, register_value_width_16},

    //{0x00e0,0xc1df,register_value_width_16},
    //{0x0004,0x1076,register_value_width_16},
    {0xffff},
};

const struct tc358746xbg_register_t Stop_tab[] =
{
    {0x0002, 0x2, register_value_width_16}, //sleep
    {0xffff},
};

#endif
