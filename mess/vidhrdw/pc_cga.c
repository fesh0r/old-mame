/***************************************************************************

  Color Graphics Adapter (CGA) section


  Notes on Port 3D8
  (http://www.clipx.net/ng/interrupts_and_ports/ng2d045.php)

	Port 3D8  -  Color/VGA Mode control register

			xx1x xxxx  Attribute bit 7. 0=blink, 1=Intesity
			xxx1 xxxx  640x200 mode
			xxxx 1xxx  Enable video signal
			xxxx x1xx  Select B/W mode
			xxxx xx1x  Select graphics
			xxxx xxx1  80x25 text


	The usage of the above control register for various modes is:
			xx10 1100  40x25 alpha B/W
			xx10 1000  40x25 alpha color
			xx10 1101  80x25 alpha B/W
			xx10 1001  80x25 alpha color
			xxx0 1110  320x200 graph B/W
			xxx0 1010  320x200 graph color
			xxx1 1110  640x200 graph B/W


	PC1512 display notes

	The PC1512 built-in display adaptor is an emulation of IBM's CGA.  Unlike a
	real CGA, it is not built around a real MC6845 controller, and so attempts
	to get custom video modes out of it may not work as expected. Its 640x200
	CGA mode can be set up to be a 16-color mode rather than mono.

	If you program it with BIOS calls, the PC1512 behaves just like a real CGA,
	except:

	- The 'greyscale' text modes (0 and 2) behave just like the 'color'
	  ones (1 and 3). On a color monitor both are in color; on a mono
	  monitor both are in greyscale.
	- Mode 5 (the 'greyscale' graphics mode) displays in color, using
	  an alternative color palette: Cyan, Red and White.
	- The undocumented 160x100x16 "graphics" mode works correctly.

	(source John Elliott http://www.seasip.info/AmstradXT/pc1512disp.html)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "state.h"

#include "includes/crtc6845.h"
#include "includes/pc_cga.h"
#include "includes/pc_video.h"
#include "mscommon.h"

#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */

#if VERBOSE_CGA
#define CGA_LOG(N,M,A) \
	if(VERBOSE_CGA>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define CGA_LOG(n,m,a)
#endif

/***************************************************************************

	Static declarations

***************************************************************************/

static VIDEO_START( pc_cga );
static PALETTE_INIT( pc_cga );

/***************************************************************************

	MachineDriver stuff

	Note - two character ROMs needed:

		// cutted from some aga char rom
		// 256 8x8 thick chars
		// 256 8x8 thin chars
		ROM_LOAD("cga.chr",     0x00000, 0x01000, 0x42009069)
		// first font of above
		ROM_LOAD("cga2.chr", 0x00000, 0x800, 0xa362ffe6)

***************************************************************************/

unsigned char cga_palette[16 * CGA_PALETTE_SETS][3] = {
/*  normal colors */
    { 0x00,0x00,0x00 },
	{ 0x00,0x00,0x7f }, 
	{ 0x00,0x7f,0x00 }, 
	{ 0x00,0x7f,0x7f },
    { 0x7f,0x00,0x00 }, 
	{ 0x7f,0x00,0x7f }, 
	{ 0x7f,0x7f,0x00 }, 
	{ 0x7f,0x7f,0x7f },
/*  light colors */
    { 0x3f,0x3f,0x3f }, 
	{ 0x00,0x00,0xff }, 
	{ 0x00,0xff,0x00 }, 
	{ 0x00,0xff,0xff },
    { 0xff,0x00,0x00 }, 
	{ 0xff,0x00,0xff }, 
	{ 0xff,0xff,0x00 }, 
	{ 0xff,0xff,0xff },
/*  normal greys */
    { 0x00,0x00,0x00 },
	{ 0x11,0x11,0x11 }, 
	{ 0x44,0x44,0x44 }, 
	{ 0x55,0x55,0x55 },
    { 0x22,0x22,0x22 }, 
	{ 0x33,0x33,0x33 }, 
	{ 0x66,0x66,0x66 }, 
	{ 0x77,0x77,0x77 },
/*  light greys */
    { 0x88,0x88,0x88 }, 
	{ 0x99,0x99,0x99 }, 
	{ 0xcc,0xcc,0xcc }, 
	{ 0xdd,0xdd,0xdd },
    { 0xaa,0xaa,0xaa }, 
	{ 0xbb,0xbb,0xbb }, 
	{ 0xee,0xee,0xee }, 
	{ 0xff,0xff,0xff }
};


unsigned short cga_colortable[] =
{
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,
/* the color sets for 1bpp graphics mode */
	 0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	 0,8, 0,9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
/* the color sets for 2bpp graphics mode. There are 32*3 = 96 of these,
 * though only the first 64 apply on a real CGA. */
/* Red/Green/Yellow set */
      0, 2, 4, 6,  1, 2, 4, 6,  2, 2, 4, 6,  3, 2, 4, 6, 
      4, 2, 4, 6,  5, 2, 4, 6,  6, 2, 4, 6,  7, 2, 4, 6, 
      8, 2, 4, 6,  9, 2, 4, 6, 10, 2, 4, 6, 11, 2, 4, 6, 
     12, 2, 4, 6, 13, 2, 4, 6, 14, 2, 4, 6, 15, 2, 4, 6, 
      0,10,12,14,  1,10,12,14,  2,10,12,14,  3,10,12,14,
      4,10,12,14,  5,10,12,14,  6,10,12,14,  7,10,12,14,
      8,10,12,14,  9,10,12,14, 10,10,12,14, 11,10,12,14,
     12,10,12,14, 13,10,12,14, 14,10,12,14, 15,10,12,14,
/* Magenta/Cyan/White set */
      0, 3, 5, 7,  1, 3, 5, 7,  2, 3, 5, 7,  3, 3, 5, 7, 
      4, 3, 5, 7,  5, 3, 5, 7,  6, 3, 5, 7,  7, 3, 5, 7, 
      8, 3, 5, 7,  9, 3, 5, 7, 10, 3, 5, 7, 11, 3, 5, 7, 
     12, 3, 5, 7, 13, 3, 5, 7, 14, 3, 5, 7, 15, 3, 5, 7, 
      0,11,13,15,  1,11,13,15,  2,11,13,15,  3,11,13,15,
      4,11,13,15,  5,11,13,15,  6,11,13,15,  7,11,13,15,
      8,11,13,15,  9,11,13,15, 10,11,13,15, 11,11,13,15,
     12,11,13,15, 13,11,13,15, 14,11,13,15, 15,11,13,15,
/* Red/Cyan/White set (PC1512 only) */
      0, 3, 4, 7,  1, 3, 4, 7,  2, 3, 4, 7,  3, 3, 4, 7, 
      4, 3, 4, 7,  5, 3, 4, 7,  6, 3, 4, 7,  7, 3, 4, 7, 
      8, 3, 4, 7,  9, 3, 4, 7, 10, 3, 4, 7, 11, 3, 4, 7, 
     12, 3, 4, 7, 13, 3, 4, 7, 14, 3, 4, 7, 15, 3, 4, 7, 
      0,11,12,15,  1,11,12,15,  2,11,12,15,  3,11,12,15,
      4,11,12,15,  5,11,12,15,  6,11,12,15,  7,11,12,15,
      8,11,12,15,  9,11,12,15, 10,11,12,15, 11,11,12,15,
     12,11,12,15, 13,11,12,15, 14,11,12,15, 15,11,12,15,
};

struct GfxLayout CGA_charlayout =
{
	8,16,					/* 8 x 16 characters */
    256,                    /* 256 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8,
	  0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

struct GfxLayout CGA_gfxlayout_1bpp =
{
    8,1,                   /* 8 x 32 graphics */
    256,                    /* 256 codes */
    1,                      /* 1 bit per pixel */
    { 0 },                  /* no bit planes */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
    8                       /* every code takes 1 byte */
};

struct GfxLayout CGA_gfxlayout_2bpp =
{
	4,1,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
    2,                      /* 2 bits per pixel */
	{ 0, 1 },				/* adjacent bit planes */
    /* x offsets */
    { 0,2,4,6  },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
    8                       /* every code takes 1 byte */
};

static struct GfxDecodeInfo CGA_gfxdecodeinfo[] =
{
/* Support up to four CGA fonts */
	{ 1, 0x0000, &CGA_charlayout,              0, 256 },   /* Font 0 */
	{ 1, 0x0800, &CGA_charlayout,              0, 256 },   /* Font 1 */
	{ 1, 0x1000, &CGA_charlayout,              0, 256 },   /* Font 2 */
	{ 1, 0x1800, &CGA_charlayout,              0, 256 },   /* Font 3*/
	{ 1, 0x8000, &CGA_gfxlayout_1bpp,      256*2,  16 },   /* 640x400x1 gfx */
	{ 1, 0x8000, &CGA_gfxlayout_2bpp, 256*2+16*2,  96 },   /* 320x200x4 gfx */
    { -1 } /* end of array */
};

MACHINE_DRIVER_START( pcvideo_cga )
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(80*8, 25*8)
	MDRV_VISIBLE_AREA(0,80*8-1, 0,25*8-1)
	MDRV_GFXDECODE(CGA_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / (CGA_PALETTE_SETS * sizeof(cga_palette[0])))
	MDRV_COLORTABLE_LENGTH(sizeof(cga_colortable) / sizeof(cga_colortable[0]))
	MDRV_PALETTE_INIT(pc_cga)

	MDRV_VIDEO_START(pc_cga)
	MDRV_VIDEO_UPDATE(pc_video)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pcvideo_pc1512 )
	MDRV_IMPORT_FROM( pcvideo_cga )
	MDRV_VIDEO_START( pc1512 )
MACHINE_DRIVER_END

/***************************************************************************

	Methods

***************************************************************************/

/* Initialise the cga palette */
PALETTE_INIT( pc_cga )
{
	int i;
	for(i = 0; i < (sizeof(cga_palette) / (3 * CGA_PALETTE_SETS)); i++)
		palette_set_color(i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2]);
	memcpy(colortable,cga_colortable,sizeof(cga_colortable));
}

static struct
{ 
	UINT8 mode_control;	/* wo 0x3d8 */
	UINT8 color_select;	/* wo 0x3d9 */
	UINT8 status;		/* ro 0x3da */
	UINT8 plantronics;	/* wo 0x3dd, ATI chipset only */
	
	int pc_blink;
	int pc_framecnt;
	int palette;
} cga;

void pc_cga_cursor(struct crtc6845_cursor *cursor)
{
	if (dirtybuffer && (videoram_size > cursor->pos*2))
		dirtybuffer[cursor->pos*2]=1;
}

static struct crtc6845_config config= { 14318180 /*?*/, pc_cga_cursor };

VIDEO_START( pc_cga )
{
	if (!pc_video_start(&config, pc_cga_choosevideomode, 0x8000))
		return 1;

	state_save_register_UINT8("pccga", 0, "mode_control",	&cga.mode_control, 1);
	state_save_register_UINT8("pccga", 0, "color_select",	&cga.color_select, 1);
	state_save_register_UINT8("pccga", 0, "status",			&cga.status, 1);
	state_save_register_UINT8("pccga", 0, "plantronics",	&cga.plantronics, 1);
	return 0;
}

static void pc_cga_check_palette(void)
{
	int i, p = 0;

	switch(CGA_MONITOR)
	{
		case CGA_MONITOR_COMPOSITE: 
			if (cga.mode_control & 4)
				p = 16;
			break;
		case CGA_MONITOR_MONO:
		case CGA_MONITOR_LCD:
			p = 16;
			break;
	}
	if (p != cga.palette)
	{
		for(i = 0; i < (sizeof(cga_palette) / (3 * CGA_PALETTE_SETS)); i++)
			palette_set_color(i, cga_palette[i + p][0], 
					     cga_palette[i + p][1],
					     cga_palette[i + p][2]);
		cga.palette = p;
		schedule_full_refresh();
		if (dirtybuffer) memset(dirtybuffer, 1, videoram_size);
	}
}

/*
 *	rW	CGA mode control register (see #P138)
 */
static void pc_cga_mode_control_w(int data)
{
	CGA_LOG(1,"CGA_mode_control_w",(errorlog, "$%02x: columns %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));

	if ((cga.mode_control ^ data) & 0x3b)    /* text/gfx/width change */
	{
		schedule_full_refresh();
		if (dirtybuffer) memset(dirtybuffer, 1, videoram_size);
	}
	cga.mode_control = data;
}

/*
 *	?W	reserved for color select register on color adapter
 */
static void pc_cga_color_select_w(int data)
{
	CGA_LOG(1,"CGA_color_select_w",(errorlog, "$%02x\n", data));
	if( cga.color_select == data )
		return;
	cga.color_select = data;
	schedule_full_refresh();
	if (dirtybuffer) memset(dirtybuffer, 1, videoram_size);
}

/* 
 * Select Plantronics modes 
 */
static void pc_cga_plantronics_w(int data)
{
	CGA_LOG(1,"CGA_plantronics_w",(errorlog, "$%02x\n", data));

	if (CGA_CHIPSET != CGA_CHIPSET_ATI) return;

	data &= 0x70;	/* Only bits 6-4 are used */
	if (cga.plantronics == data) return;
	cga.plantronics = data;
	schedule_full_refresh();	
	if (dirtybuffer) memset(dirtybuffer, 1, videoram_size);
}



/*	Bitfields for CGA status register:
 *	Bit(s)	Description (Table P179)
 *	7-6 not used
 *	5-4 color EGA, color ET4000: diagnose video display feedback, select
 *		from color plane enable
 *	3	in vertical retrace
 *	2	(CGA,color EGA) light pen switch is off
 *		(MCGA,color ET4000) reserved (0)
 *	1	(CGA,color EGA) positive edge from light pen has set trigger
 *		(MCGA,color ET4000) reserved (0)
 *	0	horizontal retrace in progress
 *		=0	do not use memory
 *		=1	memory access without interfering with display
 *		(Genoa SuperEGA) horizontal or vertical retrace
 */
static int pc_cga_status_r(void)
{
	int data = (input_port_0_r(0) & 0x08) | cga.status;
	cga.status ^= 0x01;
	return data;
}

/*************************************************************************
 *
 *		CGA
 *		color graphics adapter
 *
 *************************************************************************/
WRITE_HANDLER ( pc_CGA_w )
{
	switch(offset) {
	case 0: case 2: case 4: case 6:
	case 1: case 3: case 5: case 7:
		crtc6845_port_w(crtc6845, offset, data);
		break;
	case 8:
		pc_cga_mode_control_w(data);
		break;
	case 9:
		pc_cga_color_select_w(data);
		break;
	case 0x0d:
		pc_cga_plantronics_w(data);
		break;
	}
}

READ_HANDLER ( pc_CGA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = crtc6845_port_r(crtc6845, 0);
			break;
		case 1: case 3: case 5: case 7:
			data = crtc6845_port_r(crtc6845, 1);
			break;
		case 10:
			data = pc_cga_status_r();
			break;
    }
	return data;
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/
static void cga_text_inten(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc) * 2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	struct rectangle r;
	struct crtc6845_cursor cursor;

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);
	pc_cga_check_palette();

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height)
	{
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				drawgfx(bitmap, Machine->gfx[CGA_FONT], videoram[offs], videoram[offs+1], 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2))
				{
					int k=height-cursor.top;
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
					{
						plot_box(bitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
					}
				}

				if (dirtybuffer)
					dirtybuffer[offs] = dirtybuffer[offs+1] = 0;
			}
		}
	}
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/
static void cga_text_inten_alt(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc) * 2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	struct rectangle r;
	struct crtc6845_cursor cursor;

	if (CGA_CHIPSET != CGA_CHIPSET_PC200) 
	{
		cga_text_inten(bitmap, crtc);
		return;
	}

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);
	pc_cga_check_palette();

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height)
	{
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				drawgfx(bitmap, Machine->gfx[CGA_FONT], videoram[offs], (videoram[offs+1] & 15), 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2))
				{
					int k=height-cursor.top;
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
					{
						plot_box(bitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
					}
				}

				if (dirtybuffer)
					dirtybuffer[offs] = dirtybuffer[offs+1] = 0;
			}
		}
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/
static void cga_text_blink(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	struct rectangle r;
	struct crtc6845_cursor cursor;

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);
	pc_cga_check_palette();

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height)
	{
		if (r.min_y >= Machine->scrbitmap->height)
			break;

		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				
				int attr = videoram[offs+1];
				
				if (attr & 0x80)	/* blinking ? */
				{
					if (cga.pc_blink)
						attr = (attr & 0x70) | ((attr & 0x70) >> 4);
					else
						attr = attr & 0x7f;
				}

				drawgfx(bitmap, Machine->gfx[CGA_FONT], videoram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2))
				{
					int k=height-cursor.top;
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k > 0)
					{
						plot_box(bitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
					}
				}

				if (dirtybuffer)
					dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/
static void cga_text_blink_alt(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	struct rectangle r;
	struct crtc6845_cursor cursor;
	
	if (CGA_CHIPSET != CGA_CHIPSET_PC200) 
	{
		cga_text_blink(bitmap, crtc);
		return;
	}

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);
	pc_cga_check_palette();

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height)
	{
		if (r.min_y >= Machine->scrbitmap->height)
			break;

		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				
				int attr = videoram[offs+1];
				
				if ((attr & 0x80) && cga.pc_blink)	/* blinking ? */
					attr = (attr & 0x70) >> 4;
				else
					attr = attr & 0x0F;

				drawgfx(bitmap, Machine->gfx[CGA_FONT], videoram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2))
				{
					int k=height-cursor.top;
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k > 0)
					{
						plot_box(bitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
					}
				}

				if (dirtybuffer)
					dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}



/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
  cga fetches 2 byte per crtc6845 access (not modeled here)!
***************************************************************************/
static void cga_gfx_2bpp(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;
	int colorset = cga.color_select & 0x3F;

	/* Many clone chipsets use bit 2 of the mode control register to 
	 * access a third palette */
	pc_cga_check_palette();
	switch(CGA_CHIPSET)
	{
		case CGA_CHIPSET_PC1512:
		if ((colorset < 32) && (cga.mode_control & 4)) colorset += 64;
		break;

		case CGA_CHIPSET_PC200:
		case CGA_CHIPSET_ATI:
		case CGA_CHIPSET_PARADISE:
		if (cga.mode_control & 4) colorset = (colorset & 0x1F) + 64;
		break;
	}	

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++)
		{
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						drawgfx(bitmap, Machine->gfx[5], videoram[i], colorset,
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
			else
			{
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						drawgfx(bitmap, Machine->gfx[5], videoram[i], colorset,
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/
static void cga_gfx_1bpp(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff)
	{
		for (sh=0; sh<height; sh++, offs|=0x2000)
		{
			// char line 0 used as a12 line in graphic mode
			if (!(sh&1))
			{
				// char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						drawgfx(bitmap, Machine->gfx[4], videoram[i], cga.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						if (dirtybuffer)
							dirtybuffer[i] = 0;
					}
				}
			}
			else
			{
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						drawgfx(bitmap, Machine->gfx[4], videoram[i], cga.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						if (dirtybuffer)
							dirtybuffer[i] = 0;
					}
				}
			}
		}
	}
}



INLINE void pgfx_plot_unit_4bpp(struct mame_bitmap *bitmap, 
							 int x, int y, int offs)
{
	int color, values[2];
	int i;

	if (cga.plantronics & 0x40) 
	{
		values[0] = videoram[offs | 0x4000];
		values[1] = videoram[offs];
	}
	else
	{
		values[0] = videoram[offs];
		values[1] = videoram[offs | 0x4000];
	}
	for (i=3; i>=0; i--)
	{
		color = ((values[0] & 0x3) << 1) | 
			((values[1] & 2)   >> 1) |
			((values[1] & 1)   << 3);
//		fprintf(stderr, "%02x,%02x -> %02x\n",
//			values[0] & 3,
//			values[1] & 3,
//			color);
		plot_pixel(bitmap, x+i, y, Machine->pens[color]);
		values[0]>>=2;
		values[1]>>=2;
	}
}


/***************************************************************************
  Draw graphics mode with 640x200 pixels (default) with 2 bits/pixel.
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
  Second plane at CGA_base + 0x4000 / 0x6000 
***************************************************************************/
static void cga_pgfx_4bpp(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) 
	{
		for (sh=0; sh<height; sh++, offs|=0x2000) 
		{ 
			// char line 0 used as a12 line in graphic mode
			if (!(sh & 1))
			{
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) 
				{
					if (!dirtybuffer || dirtybuffer[i]) 
					{
						pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
			else
			{
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) 
				{
					if (!dirtybuffer || dirtybuffer[i]) 
					{
						pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}



INLINE void pgfx_plot_unit_2bpp(struct mame_bitmap *bitmap, 
					 int x, int y, int colorset, int offs)
{
	int i;
	UINT8 bmap[2], values[2];

	if (cga.plantronics & 0x40) 
	{
		values[0] = videoram[offs];
		values[1] = videoram[offs | 0x4000];
	}
	else
	{
		values[0] = videoram[offs | 0x4000];
		values[1] = videoram[offs];
	}
	bmap[0] = bmap[1] = 0;
//	fprintf(stderr, "%02x %02x -> ", values[0], values[1]);
	for (i=3; i>=0; i--)
	{
		bmap[0] = bmap[0] << 1; if (values[0] & 0x80) bmap[0] |= 1; 
		bmap[0] = bmap[0] << 1; if (values[1] & 0x80) bmap[0] |= 1; 
		bmap[1] = bmap[1] << 1; if (values[0] & 0x08) bmap[1] |= 1; 
		bmap[1] = bmap[1] << 1; if (values[1] & 0x08) bmap[1] |= 1; 
		values[0] = values[0] << 1;	
		values[1] = values[1] << 1;	
	}
//	fprintf(stderr, "%02x %02x\n", bmap[0], bmap[1]);
	drawgfx(bitmap, Machine->gfx[5], bmap[0], colorset,
		0,0, x, y, 0,TRANSPARENCY_NONE,0);
	drawgfx(bitmap, Machine->gfx[5], bmap[1], colorset,
		0,0, x+4, y, 0,TRANSPARENCY_NONE,0);
}

/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
  cga fetches 2 byte per crtc6845 access (not modeled here)!
***************************************************************************/
static void cga_pgfx_2bpp(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;
	int colorset = cga.color_select & 0x3F;

	/* Many clone chipsets use bit 2 of the mode control register to 
	 * access a third palette */
	pc_cga_check_palette();
	switch(CGA_CHIPSET)
	{
		case CGA_CHIPSET_PC1512:
		if ((colorset < 32) && (cga.mode_control & 4)) colorset += 64;
		break;

		case CGA_CHIPSET_PC200:
		case CGA_CHIPSET_ATI:
		case CGA_CHIPSET_PARADISE:
		if (cga.mode_control & 4) colorset = (colorset & 0x1F) + 64;
		break;
	}	

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++)
		{
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, colorset, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
			else
			{
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
				{
					if (!dirtybuffer || dirtybuffer[i])
					{
						pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, colorset, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}

// amstrad pc1512 video hardware
// mapping of the 4 planes into videoram
// (text data should be readable at videoram+0)
static const int videoram_offset[4]= { 0xc000, 0x8000, 0x4000, 0 };
INLINE void pc1512_plot_unit(struct mame_bitmap *bitmap, 
							 int x, int y, int offs)
{
	int color, values[4];
	int i;

	for (i = 0; i < 4; i++) values[i] = 0;

	if (cga.color_select & 1) values[0]=videoram[offs|videoram_offset[0]]; // red
	if (cga.color_select & 2) values[1]=videoram[offs|videoram_offset[1]]<<1; // green
	if (cga.color_select & 4) values[2]=videoram[offs|videoram_offset[2]]<<2; // blue
	if (cga.color_select & 8) values[3]=videoram[offs|videoram_offset[3]]<<3; // intensity

	for (i=7; i>=0; i--)
	{
		color = (values[0]&1)|(values[1]&2)|(values[2]&4)|(values[3]&8);
		plot_pixel(bitmap, x+i, y, Machine->pens[color]);
		values[0]>>=1;
		values[1]>>=1;
		values[2]>>=1;
		values[3]>>=1;
	}
}

/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/
static void pc1512_gfx_4bpp(struct mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) 
	{
		for (sh=0; sh<height; sh++, offs|=0x2000) 
		{ 
			// char line 0 used as a12 line in graphic mode
			if (!(sh & 1))
			{
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) 
				{
					if (!dirtybuffer || dirtybuffer[i]) 
					{
						pc1512_plot_unit(bitmap, sx*8, sy*height+sh, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
			else
			{
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) 
				{
					if (!dirtybuffer || dirtybuffer[i]) 
					{
						pc1512_plot_unit(bitmap, sx*8, sy*height+sh, i);
						if (dirtybuffer)
							dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}

/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
static void pc_cga_blink_textcolors(int on)
{
	int i, offs, size;

	if (cga.pc_blink == on) return;

    cga.pc_blink = on;
	offs = (crtc6845_get_start(crtc6845)*2) & 0x3fff;
	size = crtc6845_get_char_lines(crtc6845)*crtc6845_get_char_columns(crtc6845);

	if (dirtybuffer)
	{
		for (i = 0; i < size; i++)
		{
			if (videoram[offs+1] & 0x80) dirtybuffer[offs+1] = 1;
			offs=(offs+2)&0x3fff;
	    }
	}
}

extern void pc_cga_timer(void)
{
	if( ((++cga.pc_framecnt & 63) == 63) ) {
		pc_cga_blink_textcolors(cga.pc_framecnt&64);
	}
}
	

/***************************************************************************
  Choose the appropriate video mode
***************************************************************************/
pc_video_update_proc pc_cga_choosevideomode(int *width, int *height, struct crtc6845 *crtc)
{
	pc_video_update_proc proc = NULL;
	const pc_video_update_proc *procarray;
	UINT8 mode;

	static const pc_video_update_proc videoprocs_cga[] =
	{
		/* 0x08 - 0x0f */
		cga_text_inten,		cga_text_inten,		cga_gfx_2bpp,		cga_gfx_2bpp,
		cga_text_inten,		cga_text_inten,		cga_gfx_2bpp,		cga_gfx_2bpp,

		/* 0x10 - 0x1f */
		cga_text_inten_alt,	cga_text_inten_alt,	cga_gfx_1bpp,		cga_gfx_1bpp,
		cga_text_inten_alt,	cga_text_inten_alt,	cga_gfx_1bpp,		cga_gfx_1bpp,

		/* 0x20 - 0x2f */
		cga_text_blink,		cga_text_blink,		cga_gfx_2bpp,		cga_gfx_2bpp,
		cga_text_blink,		cga_text_blink,		cga_gfx_2bpp,		cga_gfx_2bpp,

		/* 0x30 - 0x3f */
		cga_text_blink_alt,	cga_text_blink_alt,	cga_gfx_1bpp,		cga_gfx_1bpp,
		cga_text_blink_alt,	cga_text_blink_alt,	cga_gfx_1bpp,		cga_gfx_1bpp,
	};

	static const pc_video_update_proc videoprocs_pc1512[] =
	{
		/* 0x08 - 0x0f */
		cga_text_inten,		cga_text_inten,		cga_gfx_2bpp,		cga_gfx_2bpp,
		cga_text_inten,		cga_text_inten,		cga_gfx_2bpp,		cga_gfx_2bpp,

		/* 0x10 - 0x1f */
		cga_text_inten,		cga_text_inten,		pc1512_gfx_4bpp,	pc1512_gfx_4bpp,
		cga_text_inten,		cga_text_inten,		pc1512_gfx_4bpp,	pc1512_gfx_4bpp,

		/* 0x20 - 0x2f */
		cga_text_blink,		cga_text_blink,		cga_gfx_2bpp,		cga_gfx_2bpp,
		cga_text_blink,		cga_text_blink,		cga_gfx_2bpp,		cga_gfx_2bpp,

		/* 0x30 - 0x3f */
		cga_text_blink,		cga_text_blink,		pc1512_gfx_4bpp,	pc1512_gfx_4bpp,
		cga_text_blink,		cga_text_blink,		pc1512_gfx_4bpp,	pc1512_gfx_4bpp
	};

	static const pc_video_update_proc *videoprocs[] =
	{
		videoprocs_cga,		/* M6845_PERSONALITY_GENUINE */
		videoprocs_pc1512	/* M6845_PERSONALITY_PC1512 */
	};

	if (cga.mode_control & 0x08)
	{
		mode = (cga.mode_control & 0x07) | ((cga.mode_control & 0x30) / 2);

		procarray = videoprocs[crtc6845_get_personality(crtc)];
		proc = procarray[mode];

		/* Plantronics high-res */
		if ((cga.mode_control & 2) && (cga.plantronics & 0x20))
			proc = cga_pgfx_2bpp;
		/* Plantronics low-res */
		if ((cga.mode_control & 2) && (cga.plantronics & 0x10))
			proc = cga_pgfx_4bpp;

		if (proc == cga_gfx_1bpp || proc == pc1512_gfx_4bpp || proc == cga_pgfx_2bpp)
			*width *= 16;
		else
			*width *= 8;

	}
	return proc;
}

static struct
{
	UINT8 write;
	UINT8 read;
} pc1512;

WRITE_HANDLER ( pc1512_w )
{
	UINT8 char_height;

	switch (offset) {
	case 0xd:
		pc1512.write = data;
		break;

	case 0xe:
		pc1512.read = data;
		cpu_setbank(1, videoram + videoram_offset[data & 3]);
		break;

	/* The PC1512 doesn't have a full 6845; writes to the first 9 6845
	 * registers are ignored, and screen resolution is entirely controlled
	 * by the mode control register. Since MESS does actually use the
	 * character columns register, program it manually. */

	case 0x8:
		char_height = crtc6845_get_char_height(crtc6845);
		if (data & 2) /* Graphics */
		{
			crtc6845_set_char_columns(crtc6845, 40);
		}
		else
		{
			if (data & 1) /* 80-column */
			{
				crtc6845_set_char_columns(crtc6845, 80);
			}
			else
			{
				crtc6845_set_char_columns(crtc6845, 40);
			}
		}
		crtc6845_set_char_lines(crtc6845, 200 / char_height);
		pc_CGA_w(offset, data);
		break;

	default:
		pc_CGA_w(offset,data);
		break;
	}
}

READ_HANDLER ( pc1512_r )
{
	data8_t data;
	switch (offset) {
	case 0xd:
		data = pc1512.write;
		break;

	case 0xe:
		data = pc1512.read;
		break;

	default:
		data = pc_CGA_r(offset);
		break;
	}
	return data;
}


WRITE_HANDLER ( pc1512_videoram_w )
{
	if (pc1512.write & 1)
		videoram[offset+videoram_offset[0]] = data; /* blue plane */
	if (pc1512.write & 2)
		videoram[offset+videoram_offset[1]] = data; /* green */
	if (pc1512.write & 4)
		videoram[offset+videoram_offset[2]] = data; /* red */
	if (pc1512.write & 8)
		videoram[offset+videoram_offset[3]] = data; /* intensity (text, 4color) */

	if (dirtybuffer)
		dirtybuffer[offset] = 1;
}

VIDEO_START( pc1512 )
{
	videoram = (UINT8*) auto_malloc(0x10000);
	if (videoram == 0)
		return 1;

	videoram_size = 0x4000; //! used in cga this way, size of plain memory in 1 bank
	cpu_setbank(1,videoram + videoram_offset[0]);
	pc1512.write = 0xf;
	pc1512.read = 0;
	config.personality = M6845_PERSONALITY_PC1512;	/* PC1512 cut-down 6845 */
	return video_start_pc_cga();
}