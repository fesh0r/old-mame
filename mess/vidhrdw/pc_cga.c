/***************************************************************************

  Color Graphics Adapter (CGA) section

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
//#include "includes/pc.h"

#include "includes/crtc6845.h"
#include "includes/pc_cga.h"
#include "includes/state.h"

#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */

#if VERBOSE_CGA
#define CGA_LOG(N,M,A) \
	if(VERBOSE_CGA>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define CGA_LOG(n,m,a)
#endif

/* window resizing with dirtybuffering traping in xmess window */
//#define GFX_ZOOMING


unsigned char cga_palette[16][3] = {
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
	{ 0xff,0xff,0xff }
};

unsigned short cga_colortable[] = {
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
/* the color sets for 2bpp graphics mode */
     /*0, 2, 4, 6,*/  0,10,12,14,
     /*0, 3, 5, 7,*/  0,11,13,15 // only 2 sets!?
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

struct GfxDecodeInfo CGA_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &CGA_charlayout,			  0, 256 },   /* single width */
	{ 1, 0x1000, &CGA_gfxlayout_1bpp,	  256*2,  16 },   /* 640x400x1 gfx */
	{ 1, 0x1000, &CGA_gfxlayout_2bpp, 256*2+16*2,   2 },   /* 320x200x4 gfx */
    { -1 } /* end of array */
};

/* Initialise the cga palette */
void pc_cga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,cga_palette,sizeof(cga_palette));
	memcpy(sys_colortable,cga_colortable,sizeof(cga_colortable));
}

typedef enum { TYPE_CGA, TYPE_PC1512 } TYPE;

static struct { 
	TYPE type;
	struct _CRTC6845 *crtc;

	UINT8 mode_control,  // wo 0x3d8
		color_select, //wo 0x3d9
		status; //ro 0x3da
	
	int full_refresh;

	int pc_blink;
	int pc_framecnt;
} cga= { TYPE_CGA };

void pc_cga_cursor(CRTC6845_CURSOR *cursor)
{
	dirtybuffer[cursor->pos*2]=1;
}

static CRTC6845_CONFIG config= { 14318180 /*?*/, pc_cga_cursor };

extern void pc_cga_init_video(struct _CRTC6845 *crtc)
{
	cga.crtc=crtc;
}

int pc_cga_vh_start(void)
{
	cga.crtc=crtc6845;
	crtc6845_init(cga.crtc, &config);

    return generic_vh_start();
}

void pc_cga_vh_stop(void)
{
    generic_vh_stop();
}

WRITE_HANDLER ( pc_cga_videoram_w )
{
	if (videoram[offset] == data) return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

/*
 *	rW	CGA mode control register (see #P138)
 */
void pc_cga_mode_control_w(int data)
{
	CGA_LOG(1,"CGA_mode_control_w",(errorlog, "$%02x: colums %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
	if ((cga.mode_control ^ data) & 0x3b)    /* text/gfx/width change */
		cga.full_refresh=1;
    cga.mode_control = data;
}

/*
 *	?W	reserved for color select register on color adapter
 */
void pc_cga_color_select_w(int data)
{
	CGA_LOG(1,"CGA_color_select_w",(errorlog, "$%02x\n", data));
	if( cga.color_select == data )
		return;
	cga.color_select = data;
	cga.full_refresh=1;
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
int pc_cga_status_r(void)
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
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			crtc6845_port_w(cga.crtc,0,data);
			break;
		case 1: case 3: case 5: case 7:
			crtc6845_port_w(cga.crtc,1,data);
			break;
		case 8:
			pc_cga_mode_control_w(data);
			break;
		case 9:
			pc_cga_color_select_w(data);
			break;
	}
}

READ_HANDLER ( pc_CGA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = crtc6845_port_r(cga.crtc,0);
			break;
		case 1: case 3: case 5: case 7:
			data = crtc6845_port_r(cga.crtc,1);
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
#ifndef GFX_ZOOMING
static void cga_text_inten(struct mame_bitmap *bitmap)
#else
static void cga_text_inten(struct mame_bitmap *bitmap, int width)
#endif
{
	int sx, sy;
	int	offs = crtc6845_get_start(cga.crtc)*2;
	int lines = crtc6845_get_char_lines(cga.crtc);
	int height = crtc6845_get_char_height(cga.crtc);
	int columns = crtc6845_get_char_columns(cga.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(cga.crtc);
	crtc6845_get_cursor(cga.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

#ifndef GFX_ZOOMING
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8) {
#else
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8*width, r.max_x+=8*width) {
#endif
			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
#ifndef GFX_ZOOMING
				drawgfx(bitmap, Machine->gfx[0], videoram[offs], videoram[offs+1], 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
#else
				drawgfxzoom(bitmap, Machine->gfx[0], videoram[offs], videoram[offs+1], 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_PEN,16,0x10000*width,1<<16);
#endif
//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0) {
#ifndef GFX_ZOOMING
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
#else
						plot_box(Machine->scrbitmap, r.min_x*width, 
								 r.min_y+cursor.top, 
								 8*width, k, Machine->pens[7]);
#endif
					}
				}

				dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/
#ifndef GFX_ZOOMING
static void cga_text_blink(struct mame_bitmap *bitmap)
#else
static void cga_text_blink(struct mame_bitmap *bitmap, int width)
#endif
{
	int sx, sy;
	int	offs = crtc6845_get_start(cga.crtc)*2;
	int lines = crtc6845_get_char_lines(cga.crtc);
	int height = crtc6845_get_char_height(cga.crtc);
	int columns = crtc6845_get_char_columns(cga.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(cga.crtc);
	crtc6845_get_cursor(cga.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

#ifndef GFX_ZOOMING
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8) {
#else
		for (sx=0, r.min_x=0, r.max_x=8*width-1; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8*width, r.max_x+=8*width) {
#endif
			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
				
				int attr = videoram[offs+1];
				
				if (attr & 0x80)	/* blinking ? */
				{
					if (cga.pc_blink)
						attr = (attr & 0x70) | ((attr & 0x70) >> 4);
					else
						attr = attr & 0x7f;
				}

#ifndef GFX_ZOOMING
				drawgfx(bitmap, Machine->gfx[0], videoram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);
#else
				drawgfxzoom(bitmap, Machine->gfx[0], videoram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_PEN,16,0x10000*width,1<<16);
#endif

//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(cga.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0) {
#ifndef GFX_ZOOMING
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8, k, Machine->pens[7]);
#else
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 8*width, k, Machine->pens[7]);
#endif
					}
				}

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
static void cga_gfx_2bpp(struct mame_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(cga.crtc)*2;
	int lines = crtc6845_get_char_lines(cga.crtc);
	int height = crtc6845_get_char_height(cga.crtc);
	int columns = crtc6845_get_char_columns(cga.crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++) { 
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
#ifndef GFX_ZOOMING
						drawgfx(bitmap, Machine->gfx[2], videoram[i], (cga.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
#else
						drawgfxzoom(bitmap, Machine->gfx[2], videoram[i], (cga.color_select&0x20?1:0),
								0,0,sx*8,sy*height+sh, 0,TRANSPARENCY_PEN,16,0x10000*2,1<<16);
#endif
						dirtybuffer[i]=0;
					}
				}
			} else {
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
#ifndef GFX_ZOOMING
						drawgfx(bitmap, Machine->gfx[2], videoram[i], (cga.color_select&0x20?1:0),
								0,0,sx*4,sy*height+sh, 0,TRANSPARENCY_NONE,0);
#else
						drawgfxzoom(bitmap, Machine->gfx[2], videoram[i], (cga.color_select&0x20?1:0),
								0,0,sx*8,sy*height+sh, 0,TRANSPARENCY_PEN,16,0x10000*2,1<<16);
#endif
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
static void cga_gfx_1bpp(struct mame_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(cga.crtc)*2;
	int lines = crtc6845_get_char_lines(cga.crtc);
	int height = crtc6845_get_char_height(cga.crtc);
	int columns = crtc6845_get_char_columns(cga.crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++, offs|=0x2000) { // char line 0 used as a12 line in graphic mode
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[1], videoram[i], cga.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
			} else {
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[1], videoram[i], cga.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
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

	values[0]=videoram[offs|videoram_offset[0]]; // red
	values[1]=videoram[offs|videoram_offset[1]]<<1; // green
	values[2]=videoram[offs|videoram_offset[2]]<<2; // blue
	values[3]=videoram[offs|videoram_offset[3]]<<3; // intensity

	for (i=7; i>=0; i--) {
		color=(values[0]&1)|(values[1]&2)|(values[2]&4)|(values[3]&8);
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
static void pc1512_gfx_4bpp(struct mame_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(cga.crtc);
	int lines = crtc6845_get_char_lines(cga.crtc);
	int height = crtc6845_get_char_height(cga.crtc);
	int columns = crtc6845_get_char_columns(cga.crtc);

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++, offs|=0x2000) { // char line 0 used as a12 line in graphic mode

			for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
				if (dirtybuffer[i]) {
					pc1512_plot_unit(bitmap, sx*8, sy*height+sh, i);
					dirtybuffer[i]=0;
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
	offs = (crtc6845_get_start(cga.crtc)*2) & 0x3fff;
	size = crtc6845_get_char_lines(cga.crtc)*crtc6845_get_char_columns(cga.crtc);

	for (i = 0; i < size; i++)
	{
		if (videoram[offs+1] & 0x80) dirtybuffer[offs+1] = 1;
		offs=(offs+2)&0x3fff;
    }
}

extern void pc_cga_timer(void)
{
	if( ((++cga.pc_framecnt & 63) == 63) ) {
		pc_cga_blink_textcolors(cga.pc_framecnt&64);
	}
}
	

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
void pc_cga_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	static int video_active = 0;
	static int width=0, height=0;
	int w,h;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	if( crtc6845_do_full_refresh(cga.crtc)||full_refresh||cga.full_refresh )
	{
		cga.full_refresh=0;
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
		video_active = 0;
    }

	w=crtc6845_get_char_columns(cga.crtc)*8;
	h=crtc6845_get_char_height(cga.crtc)*crtc6845_get_char_lines(cga.crtc);
	switch( cga.mode_control & 0x3b )	/* text and gfx modes */
	{
		case 0x08: // half frequency
			video_active = 10; 
#ifndef GFX_ZOOMING
			cga_text_inten(bitmap);
#else
			cga_text_inten(bitmap,2);
#endif
			break;
		case 0x09:
			video_active = 10;
#ifndef GFX_ZOOMING
			cga_text_inten(bitmap);
#else
			cga_text_inten(bitmap,1);
#endif
			break;
		case 0x28: // half frequency
			video_active = 10; 
#ifndef GFX_ZOOMING
			cga_text_blink(bitmap);
#else
			cga_text_blink(bitmap,2);
#endif
			break;
		case 0x29:
			video_active = 10;
#ifndef GFX_ZOOMING
			cga_text_blink(bitmap);
#else
			cga_text_blink(bitmap,1);
#endif
			break;
        case 0x0a: case 0x0b: case 0x2a: case 0x2b:
			video_active = 10;
			cga_gfx_2bpp(bitmap);
			break;
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x38: case 0x39: case 0x3a: case 0x3b:
			video_active = 10;
			if (cga.type==TYPE_PC1512) {
				pc1512_gfx_4bpp(bitmap);
			} else {
				cga_gfx_1bpp(bitmap);w*=2;
			}
			break;

        default:
			if (video_active && --video_active == 0)
				fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
    }

	if ( (width!=w)||(height!=h) ) 
	{
		width=w;
		height=h;
		if (width>Machine->visible_area.max_x) width=Machine->visible_area.max_x+1;
		if (height>Machine->visible_area.max_y) height=Machine->visible_area.max_y+1;
#ifndef GFX_ZOOMING
		if ((width>100)&&(height>100))
			osd_set_visible_area(0,width-1,0, height-1);
		else logerror("video %d %d\n",width, height);
#endif
	}
//	state_display(bitmap);
}

static struct {
	UINT8 write, read;
} pc1512;

extern WRITE_HANDLER ( pc1512_w )
{
	switch (offset) {
	case 0xd: pc1512.write=data;break;
	case 0xe: pc1512.read=data;cpu_setbank(1,videoram+videoram_offset[data&3]);break;
	default: pc_CGA_w(offset,data);
	}
}

extern READ_HANDLER ( pc1512_r )
{
	int data;
	switch (offset) {
	case 0xd: data=pc1512.write;break;
	case 0xe: data=pc1512.read;break;
	default: data=pc_CGA_r(offset);
	}
	return data;
}


extern WRITE_HANDLER ( pc1512_videoram_w )
{
	if (pc1512.write&1) videoram[offset+videoram_offset[0]]=data; //blue plane
	if (pc1512.write&2) videoram[offset+videoram_offset[1]]=data; //green
	if (pc1512.write&4) videoram[offset+videoram_offset[2]]=data; //red
	if (pc1512.write&8) videoram[offset+videoram_offset[3]]=data; //intensity (text, 4color)
	dirtybuffer[offset]=1;
}

extern int	pc1512_vh_start(void)
{
	cga.type=TYPE_PC1512;	
	videoram=(UINT8*)malloc(0x10000);
	if (videoram==0) return 1;
	videoram_size=0x4000; //! used in cga this way, size of plain memory in 1 bank
	cpu_setbank(1,videoram+videoram_offset[0]);
	pc1512.write=0xf;
	pc1512.read=0;
	return pc_cga_vh_start();
}

extern void pc1512_vh_stop(void)
{
	free(videoram);
	pc_cga_vh_stop();
}

extern void pc1512_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	pc_cga_vh_screenrefresh(bitmap, full_refresh);
}
