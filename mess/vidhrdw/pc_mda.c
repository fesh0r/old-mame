/***************************************************************************

  Monochrom Display Adapter (MDA) section

***************************************************************************/
#include "vidhrdw/generic.h"
#include "includes/crtc6845.h"

#include "includes/pc_mda.h"
#include "includes/pc_cga.h" //for the cga palette hack
#include "includes/state.h"

#define VERBOSE_MDA 0		/* MDA (Monochrome Display Adapter) */

#if VERBOSE_MDA
#define MDA_LOG(N,M,A) \
	if(VERBOSE_MDA>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define MDA_LOG(n,m,a)
#endif

unsigned char mda_palette[4][3] = {
	{ 0x00,0x00,0x00 },
	{ 0x00,0x55,0x00 }, 
	{ 0x00,0xaa,0x00 }, 
	{ 0x00,0xff,0x00 }
};

struct GfxLayout pc_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8,
	  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16384+0*8, 16384+1*8, 16384+2*8, 16384+3*8,
	  16384+4*8, 16384+5*8, 16384+6*8, 16384+7*8 },
	8*8 					/* every char takes 8 bytes (upper half) */
};

struct GfxLayout pc_mda_gfxlayout_1bpp =
{
	8,1,					/* 8 x 32 graphics */
	256,					/* 256 codes */
	1,						/* 1 bit per pixel */
	{ 0 },					/* no bit planes */
    /* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets (we only use one byte to build the block) */
	{ 0 },
	8						/* every code takes 1 byte */
};

struct GfxDecodeInfo pc_mda_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &pc_mda_charlayout,		0, 256 },
	{ 1, 0x1000, &pc_mda_gfxlayout_1bpp,256*2,	 1 },	/* 640x400x1 gfx */
    { -1 } /* end of array */
};

/* to be done:
   only 2 digital color lines to mda/hercules monitor
   (maximal 4 colors) */
unsigned short mda_colortable[] = {
     0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 2, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
     2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 2,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     0, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2, 0,10, 0, 0,10,10,10,10,10,10,10,10,10,10,10,10, 0,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 2,10, 2,10, 2,10, 2,10, 2,10, 2,10,10,10, 0,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
    10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10, 0,10,10,
/* the two colors for HGC graphics */
     0, 10
};

/* Initialise the mda palette */
void pc_mda_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
//    memcpy(sys_palette,mda_palette,sizeof(mda_palette));
    memcpy(sys_palette,cga_palette,sizeof(cga_palette));
    memcpy(sys_colortable,mda_colortable,sizeof(mda_colortable));
}

static struct { 
	struct _CRTC6845 *crtc;

	UINT8 status;

	int full_refresh;

	int pc_blink;
	int pc_framecnt;

	UINT8 mode_control, configuration_switch; //hercules

	struct GfxElement *gfx_char;
	struct GfxElement *gfx_graphic;
} mda = { 0 };

/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
void pc_mda_blink_textcolors(int on)
{
	int i, offs, size;

	if (mda.pc_blink == on) return;

    mda.pc_blink = on;
	offs = (crtc6845_get_start(mda.crtc)*2)% videoram_size;
	size = crtc6845_get_char_lines(mda.crtc)*crtc6845_get_char_columns(mda.crtc);

	for (i = 0; i < size; i++)
	{
		if( videoram[offs+1] & 0x80 )
			dirtybuffer[offs+1] = 1;
		if( (offs += 2) == videoram_size )
			offs = 0;
    }
}

extern void pc_mda_timer(void)
{
	if( ((++mda.pc_framecnt & 63) == 63) ) {
		pc_mda_blink_textcolors(mda.pc_framecnt&64);
	}
}

void pc_mda_cursor(CRTC6845_CURSOR *cursor)
{
	dirtybuffer[cursor->pos*2]=1;
}

static CRTC6845_CONFIG config= { 14318180 /*?*/, pc_mda_cursor };

extern void pc_mda_init_video(struct _CRTC6845 *crtc)
{
	int i;
	mda.gfx_char=Machine->gfx[0];
	mda.gfx_graphic=Machine->gfx[1];

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < Machine->gfx[0]->height; y++ )
				Machine->gfx[0]->gfxdata[(i * Machine->gfx[0]->height + y) * Machine->gfx[0]->width + 8] = 0;
		}
	}
	mda.crtc=crtc6845;
}

extern void pc_mda_europc_init(struct _CRTC6845 *crtc)
{
	int i;
	mda.gfx_char=Machine->gfx[3];
	mda.gfx_graphic=Machine->gfx[4];

    /* remove pixel column 9 for character codes 0 - 175 and 224 - 255 */
	for( i = 0; i < 256; i++)
	{
		if( i < 176 || i > 223 )
		{
			int y;
			for( y = 0; y < Machine->gfx[3]->height; y++ )
				Machine->gfx[3]->gfxdata[(i * Machine->gfx[3]->height + y) * Machine->gfx[3]->width + 8] = 0;
		}
	}
	mda.crtc=crtc6845;
}

int pc_mda_vh_start(void)
{
	pc_mda_init_video(crtc6845);
	crtc6845_init(mda.crtc, &config);

    return generic_vh_start();
}

void pc_mda_vh_stop(void)
{
    generic_vh_stop();
}

WRITE_HANDLER ( pc_mda_videoram_w )
{
	if (videoram[offset] == data) return;
	videoram[offset] = data;
	dirtybuffer[offset] = 1;
}

/*
 *	rW	MDA mode control register (see #P138)
 */
void hercules_mode_control_w(int data)
{
	MDA_LOG(1,"MDA_mode_control_w",(errorlog, "$%02x: colums %d, gfx %d, enable %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>3)&1, (data>>5)&1));
	mda.mode_control = data;
	mda.full_refresh=1;
}

/*	R-	CRT status register (see #P139)
 *		(EGA/VGA) input status 1 register
 *		7	 HGC vertical sync in progress
 *		6-4  adapter 000  hercules
 *					 001  hercules+
 *					 101  hercules InColor
 *					 else unknown
 *		3	 pixel stream (0 black, 1 white)
 *		2-1  reserved
 *		0	 horizontal drive enable
 */
int pc_mda_status_r(void)
{
    int data = (input_port_0_r(0) & 0x80) | 0x08 | mda.status;
	mda.status ^= 0x01;
	return data;
}

void hercules_config_w(int data)
{
	MDA_LOG(1,"HGC_config_w",(errorlog, "$%02x\n", data));
    mda.configuration_switch = data;
	mda.full_refresh=1;
}

/*************************************************************************
 *
 *		MDA
 *		monochrome display adapter
 *
 *************************************************************************/
WRITE_HANDLER ( pc_MDA_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			crtc6845_port_w(mda.crtc, 0, data);
			break;
		case 1: case 3: case 5: case 7:
			crtc6845_port_w(mda.crtc, 1, data);
			break;
		case 8:
			hercules_mode_control_w(data);
			break;
		case 15:
			hercules_config_w(data);
			break;
	}
}

READ_HANDLER ( pc_MDA_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = crtc6845_port_r(mda.crtc,0);
			break;
		case 1: case 3: case 5: case 7:
			data = crtc6845_port_r(mda.crtc,1);
			break;
		case 10:
			data = pc_mda_status_r();
			break;
		/* 12, 13, 14  are the LPT1 ports */
    }
	return data;
}

/***************************************************************************
  Draw text mode with 80x25 characters (default) and intense background.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/
static void mda_text_inten(struct mame_bitmap *bitmap)
{
	int sx, sy;
	int	offs = crtc6845_get_start(mda.crtc)*2;
	int lines = crtc6845_get_char_lines(mda.crtc);
	int height = crtc6845_get_char_height(mda.crtc);
	int columns = crtc6845_get_char_columns(mda.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(mda.crtc);
	crtc6845_get_cursor(mda.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

		for (sx=0, r.min_x=0, r.max_x=8; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=9, r.max_x+=9) {
			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
				
				drawgfx(bitmap, mda.gfx_char, videoram[offs], videoram[offs+1], 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(mda.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 9, k, Machine->pens[2/*?*/]);
				}

				dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}


/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking characters.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/
static void mda_text_blink(struct mame_bitmap *bitmap)
{
	int sx, sy;
	int	offs = crtc6845_get_start(mda.crtc)*2;
	int lines = crtc6845_get_char_lines(mda.crtc);
	int height = crtc6845_get_char_height(mda.crtc);
	int columns = crtc6845_get_char_columns(mda.crtc);
	struct rectangle r;
	CRTC6845_CURSOR cursor;

	crtc6845_time(mda.crtc);
	crtc6845_get_cursor(mda.crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

		for (sx=0, r.min_x=0, r.max_x=8; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=9, r.max_x+=9) {

			if (dirtybuffer[offs] || dirtybuffer[offs+1]) {
				
				int attr = videoram[offs+1];
				
				if (attr & 0x80)	/* blinking ? */
				{
					if (mda.pc_blink)
						attr = (attr & 0x70) | ((attr & 0x70) >> 4);
					else
						attr = attr & 0x7f;
				}

				drawgfx(bitmap, mda.gfx_char, videoram[offs], attr, 
						0,0,r.min_x,r.min_y,&r,TRANSPARENCY_NONE,0);

//				if ((cursor.on)&&(offs==cursor.pos*2)) {
				if (cursor.on&&(mda.pc_framecnt&32)&&(offs==cursor.pos*2)) {
					int k=height-cursor.top;
					struct rectangle rect2=r;
					rect2.min_y+=cursor.top; 
					if (cursor.bottom<height) k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(Machine->scrbitmap, r.min_x, 
								 r.min_y+cursor.top, 
								 9, k, Machine->pens[2/*?*/]);
				}

				dirtybuffer[offs]=dirtybuffer[offs+1]=0;
			}
		}
	}
}

/***************************************************************************
  Draw graphics with 720x384 pixels (default); so called Hercules gfx.
  The memory layout is divided into 4 banks where of size 0x2000.
  Every bank holds data for every n'th scanline, 8 pixels per byte,
  bit 7 being the leftmost.
***************************************************************************/
static void hercules_gfx(struct mame_bitmap *bitmap)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(mda.crtc)*2;
	int lines = crtc6845_get_char_lines(mda.crtc);
	int height = crtc6845_get_char_height(mda.crtc);
	int columns = crtc6845_get_char_columns(mda.crtc)*2;
	UINT8 *vram=videoram, *dbuffer=dirtybuffer;
	if (mda.mode_control&0x80) { vram+=0x8000, dbuffer+=0x8000; }

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++) { // char line 0 used as a12 line in graphic mode
			switch(sh&3) { // char line 0 used as a12 line in graphic mode
			case 0:
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dbuffer[i]) {
						drawgfx(bitmap, mda.gfx_graphic, vram[i], 0, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dbuffer[i]=0;
					}
				}
				break;
			case 1:
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dbuffer[i]) {
						drawgfx(bitmap, mda.gfx_graphic, vram[i], 0, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dbuffer[i]=0;
					}
				}
				break;
			case 2:
				for (i=offs|0x4000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x4000) {
					if (dbuffer[i]) {
						drawgfx(bitmap, mda.gfx_graphic, vram[i], 0, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dbuffer[i]=0;
					}
				}
				break;
			case 3:
				for (i=offs|0x6000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x6000) {
					if (dbuffer[i]) {
						drawgfx(bitmap, mda.gfx_graphic, vram[i], 0, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dbuffer[i]=0;
					}
				}
				break;
			}
		}
	}
}

/***************************************************************************
  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
 ***************************************************************************/
void pc_mda_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	static int video_active = 0;
	static int width=0, height=0;
	int w,h;

    /* draw entire scrbitmap because of usrintrf functions
	   called osd_clearbitmap or attr change / scanline change */
	if( crtc6845_do_full_refresh(mda.crtc)||full_refresh||mda.full_refresh )
	{
		mda.full_refresh=0;
		memset(dirtybuffer, 1, videoram_size);
		fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);
		video_active = 0;
    }

	h=crtc6845_get_char_height(mda.crtc)*crtc6845_get_char_lines(mda.crtc);
	w=crtc6845_get_char_columns(mda.crtc);

	switch (mda.mode_control & 0x2a) /* text and gfx modes */
	{
		case 0x08: video_active = 10; mda_text_inten(bitmap);w*=9; break;
		case 0x28: video_active = 10; mda_text_blink(bitmap);w*=9; break;
		case 0x0a: video_active = 10; hercules_gfx(bitmap);w*=8; break;
		case 0x2a: video_active = 10; hercules_gfx(bitmap);w*=8; break;

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
		if ((width>100)&&(height>100))
			osd_set_visible_area(0,width-1,0, height-1);
		else logerror("video %d %d\n",width, height);
	}

//	state_display(bitmap);
}

