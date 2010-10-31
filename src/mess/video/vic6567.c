/***************************************************************************

    Video Interface Chip (6567R8 for NTSC system and 6569 for PAL system)

    original emulation by PeT (mess@utanet.at)

    A part of the code (cycle routine and drawing routines) is a modified version of the vic ii emulation used in
    commodore 64 emulator "frodo" by Christian Bauer

    http://frodo.cebix.net/
    The rights on the source code remain at the author.
    It may not - not even in parts - used for commercial purposes without explicit written permission by the author.
    Permission to use it for non-commercial purposes is hereby granted als long as my copyright notice remains in the program.
    You are not allowed to use the source to create and distribute a modified version of Frodo.

    2010-02: converted to be a device and split vic III emulation (which still uses old cycle & drawing routines)

    TODO:
      - plenty of cleanups
      - emulate variants of the vic chip
      - update vic III to use new code for the vic II compatibility

***************************************************************************/
/* mos videochips
  vic (6560 NTSC, 6561 PAL)
  used in commodore vic20

  vic II
   6566 NTSC
    no dram refresh?
   6567 NTSC
   6569 PAL-B
   6572 PAL-N
   6573 PAL-M
   8562 NTSC
   8565 PAL
  used in commodore c64
  complete different to vic

  ted
   7360/8360 (NTSC-M, PAL-B by same chip ?)
   8365 PAL-N
   8366 PAL-M
  used in c16 c116 c232 c264 plus4 c364
  based on the vic II
  but no sprites and not register compatible
  includes timers, input port, sound generators
  memory interface, dram refresh, clock generation

  vic IIe
   8564 NTSC-M
   8566 PAL-B
   8569 PAL-N
  used in commodore c128
  vic II with some additional features
   3 programmable output pins k0 k1 k2

  vic III
   4567
  used in commodore c65 prototype
  vic II compatible (mode only?)
  several additional features
   different resolutions, more colors, ...
   (maybe like in the amiga graphic chip docu)

  vdc
   8563
   8568 (composite video and composite sync)
  second graphic chip in c128
  complete different to above chips
*/


#include "emu.h"
#include "video/vic6567.h"

static UINT8 rdy_cycles = 0;

typedef struct _vic2_state  vic2_state;
struct _vic2_state
{
	vic2_type  type;

	screen_device *screen;			// screen which sets bitmap properties
	running_device *cpu;

	UINT8 reg[0x80];

	int on;								/* rastering of the screen */

	UINT16 chargenaddr, videoaddr, bitmapaddr;

	bitmap_t *bitmap;

	UINT16 colors[4], spritemulti[4];

	int rasterline;
	UINT64 cycles_counter;
	UINT8 cycle;
	UINT16 raster_x;
	UINT16 graphic_x;

	/* convert multicolor byte to background/foreground for sprite collision */
	UINT16 expandx[256];
	UINT16 expandx_multi[256];

	/* Display */
	UINT16 dy_start;
	UINT16 dy_stop;

	/* GFX */
	UINT8 draw_this_line;
	UINT8 is_bad_line;
	UINT8 bad_lines_enabled;
	UINT8 display_state;
	UINT8 char_data;
	UINT8 gfx_data;
	UINT8 color_data;
	UINT8 last_char_data;
	UINT8 matrix_line[40];						// Buffer for video line, read in Bad Lines
	UINT8 color_line[40];						// Buffer for color line, read in Bad Lines
	UINT8 vblanking;
	UINT16 ml_index;
	UINT8 rc;
	UINT16 vc;
	UINT16 vc_base;
	UINT8 ref_cnt;

	/* Sprites */
	UINT8 spr_coll_buf[0x400];					// Buffer for sprite-sprite collisions and priorities
	UINT8 fore_coll_buf[0x400];					// Buffer for foreground-sprite collisions and priorities
	UINT8 spr_draw_data[8][4];					// Sprite data for drawing
	UINT8 spr_exp_y;
	UINT8 spr_dma_on;
	UINT8 spr_draw;
	UINT8 spr_disp_on;
	UINT16 spr_ptr[8];
	UINT8 spr_data[8][4];
	UINT16 mc_base[8];						// Sprite data counter bases
	UINT16 mc[8];							// Sprite data counters

	/* Border */
	UINT8 border_on;
	UINT8 ud_border_on;
	UINT8 border_on_sample[5];
	UINT8 border_color_sample[0x400 / 8];			// Samples of border color at each "displayed" cycle

	/* Cycles */
	UINT64 first_ba_cycle;
	UINT8 cpu_suspended;

	/* DMA */
	vic2_dma_read          dma_read;
	vic2_dma_read_color    dma_read_color;

	/* IRQ */
	vic2_irq               interrupt;

	/* RDY */
	vic2_rdy_callback      rdy_workaround_cb;

	/* lightpen */
	vic2_lightpen_button_callback lightpen_button_cb;
	vic2_lightpen_x_callback lightpen_x_cb;
	vic2_lightpen_y_callback lightpen_y_cb;
};


/*****************************************************************************
    CONSTANTS
*****************************************************************************/

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	do { \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time(machine)), (char*) M ); \
			logerror A; \
		} \
	} while (0)

#define ROW25_YSTART      0x33
#define ROW25_YSTOP       0xfb
#define ROW24_YSTART      0x37
#define ROW24_YSTOP       0xf7

#define RASTERLINE_2_C64(a)		(a)
#define C64_2_RASTERLINE(a)		(a)
#define XPOS				(VIC2_STARTVISIBLECOLUMNS + (VIC2_VISIBLECOLUMNS - VIC2_HSIZE) / 2)
#define YPOS				(VIC2_STARTVISIBLELINES /* + (VIC2_VISIBLELINES - VIC2_VSIZE) / 2 */)
#define FIRSTCOLUMN			50

/* 2008-05 FP: lightpen code needs to read input port from c64.c and cbmb.c */

#define LIGHTPEN_BUTTON		(vic2->lightpen_button_cb(machine))
#define LIGHTPEN_X_VALUE	(vic2->lightpen_x_cb(machine))
#define LIGHTPEN_Y_VALUE	(vic2->lightpen_y_cb(machine))

/* lightpen delivers values from internal counters; they do not start with the visual area or frame area */
#define VIC2_MAME_XPOS			0
#define VIC2_MAME_YPOS			0
#define VIC6567_X_BEGIN			38
#define VIC6567_Y_BEGIN			-6			   /* first 6 lines after retrace not for lightpen! */
#define VIC6569_X_BEGIN			38
#define VIC6569_Y_BEGIN			-6
#define VIC2_X_BEGIN			((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_X_BEGIN : VIC6567_X_BEGIN)
#define VIC2_Y_BEGIN			((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_Y_BEGIN : VIC6567_Y_BEGIN)
#define VIC2_X_VALUE			((LIGHTPEN_X_VALUE / 1.3) + 12)
#define VIC2_Y_VALUE			((LIGHTPEN_Y_VALUE      ) + 10)

#define VIC2E_K0_LEVEL			(vic2->reg[0x2f] & 0x01)
#define VIC2E_K1_LEVEL			(vic2->reg[0x2f] & 0x02)
#define VIC2E_K2_LEVEL			(vic2->reg[0x2f] & 0x04)


/* sprites 0 .. 7 */
#define SPRITEON(nr)			(vic2->reg[0x15] & (1 << nr))
#define SPRITE_Y_EXPAND(nr)		(vic2->reg[0x17] & (1 << nr))
#define SPRITE_Y_SIZE(nr)		(SPRITE_Y_EXPAND(nr) ? 2 * 21 : 21)
#define SPRITE_X_EXPAND(nr)		(vic2->reg[0x1d] & (1 << nr))
#define SPRITE_X_SIZE(nr)		(SPRITE_X_EXPAND(nr) ? 2 * 24 : 24)
#define SPRITE_X_POS(nr)		(vic2->reg[(nr) * 2] | (vic2->reg[0x10] & (1 << (nr)) ? 0x100 : 0))
#define SPRITE_Y_POS(nr)		(vic2->reg[1 + 2 * (nr)])
#define SPRITE_MULTICOLOR(nr)		(vic2->reg[0x1c] & (1 << nr))
#define SPRITE_PRIORITY(nr)		(vic2->reg[0x1b] & (1 << nr))
#define SPRITE_MULTICOLOR1		(vic2->reg[0x25] & 0x0f)
#define SPRITE_MULTICOLOR2		(vic2->reg[0x26] & 0x0f)
#define SPRITE_COLOR(nr)		(vic2->reg[0x27+nr] & 0x0f)
#define SPRITE_ADDR(nr)			(vic2->videoaddr | 0x3f8 | nr)
#define SPRITE_COLL			(vic2->reg[0x1e])
#define SPRITE_BG_COLL			(vic2->reg[0x1f])

#define GFXMODE				((vic2->reg[0x11] & 0x60) | (vic2->reg[0x16] & 0x10)) >> 4
#define SCREENON				(vic2->reg[0x11] & 0x10)
#define VERTICALPOS			(vic2->reg[0x11] & 0x07)
#define HORIZONTALPOS			(vic2->reg[0x16] & 0x07)
#define ECMON				(vic2->reg[0x11] & 0x40)
#define HIRESON				(vic2->reg[0x11] & 0x20)
#define COLUMNS40				(vic2->reg[0x16] & 0x08)		   /* else 38 Columns */

#define VIDEOADDR				((vic2->reg[0x18] & 0xf0) << (10 - 4))
#define CHARGENADDR			((vic2->reg[0x18] & 0x0e) << 10)
#define BITMAPADDR			((data & 0x08) << 10)

#define RASTERLINE			(((vic2->reg[0x11] & 0x80) << 1) | vic2->reg[0x12])

#define FRAMECOLOR			(vic2->reg[0x20] & 0x0f)
#define BACKGROUNDCOLOR			(vic2->reg[0x21] & 0x0f)
#define MULTICOLOR1			(vic2->reg[0x22] & 0x0f)
#define MULTICOLOR2			(vic2->reg[0x23] & 0x0f)
#define FOREGROUNDCOLOR			(vic2->reg[0x24] & 0x0f)

#define VIC2_LINES		((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_LINES : VIC6567_LINES)
#define VIC2_FIRST_DMA_LINE	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_FIRST_DMA_LINE : VIC6567_FIRST_DMA_LINE)
#define VIC2_LAST_DMA_LINE	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_LAST_DMA_LINE : VIC6567_LAST_DMA_LINE)
#define VIC2_FIRST_DISP_LINE	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_FIRST_DISP_LINE : VIC6567_FIRST_DISP_LINE)
#define VIC2_LAST_DISP_LINE	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_LAST_DISP_LINE : VIC6567_LAST_DISP_LINE)
#define VIC2_RASTER_2_EMU(a)	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_RASTER_2_EMU(a) : VIC6567_RASTER_2_EMU(a))
#define VIC2_FIRSTCOLUMN	((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_FIRSTCOLUMN : VIC6567_FIRSTCOLUMN)
#define VIC2_X_2_EMU(a)		((vic2->type == VIC6569 || vic2->type == VIC8566) ? VIC6569_X_2_EMU(a) : VIC6567_X_2_EMU(a))

/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE vic2_state *get_safe_token( running_device *device )
{
	assert(device != NULL);
	assert(device->type() == VIC2);

	return (vic2_state *)downcast<legacy_device_base *>(device)->token();
}

INLINE const vic2_interface *get_interface( running_device *device )
{
	assert(device != NULL);
	assert((device->type() == VIC2));
	return (const vic2_interface *) device->baseconfig().static_config();
}

/*****************************************************************************
    IMPLEMENTATION
*****************************************************************************/

static void vic2_set_interrupt( running_machine *machine, int mask, vic2_state *vic2 )
{
	if (((vic2->reg[0x19] ^ mask) & vic2->reg[0x1a] & 0xf))
	{
		if (!(vic2->reg[0x19] & 0x80))
		{
			DBG_LOG(2, "vic2", ("irq start %.2x\n", mask));
			vic2->reg[0x19] |= 0x80;
			vic2->interrupt(machine, 1);
		}
	}
	vic2->reg[0x19] |= mask;
}

static void vic2_clear_interrupt( running_machine *machine, int mask, vic2_state *vic2 )
{
	vic2->reg[0x19] &= ~mask;
	if ((vic2->reg[0x19] & 0x80) && !(vic2->reg[0x19] & vic2->reg[0x1a] & 0xf))
	{
		DBG_LOG(2, "vic2", ("irq end %.2x\n", mask));
		vic2->reg[0x19] &= ~0x80;
		vic2->interrupt(machine, 0);
	}
}

void vic2_lightpen_write( running_device *device, int level )
{
	/* calculate current position, write it and raise interrupt */
}

static TIMER_CALLBACK( vic2_timer_timeout )
{
	vic2_state *vic2 = (vic2_state *)ptr;
	int which = param;

	DBG_LOG(3, "vic2 ", ("timer %d timeout\n", which));

	switch (which)
	{
		case 1:						   /* light pen */
			/* and diode must recognize light */
			if (1)
			{
				vic2->reg[0x13] = VIC2_X_VALUE;
				vic2->reg[0x14] = VIC2_Y_VALUE;
			}
			vic2_set_interrupt(machine, 8, vic2);
			break;
	}
}


// modified VIC II emulation by Christian Bauer starts here...

// Idle access
INLINE void vic2_idle_access( running_machine *machine, vic2_state *vic2 )
{
	vic2->dma_read(machine, 0x3fff);
}

// Fetch sprite data pointer
INLINE void vic2_spr_ptr_access( running_machine *machine, vic2_state *vic2, int num )
{
	vic2->spr_ptr[num] = vic2->dma_read(machine, SPRITE_ADDR(num)) << 6;
}

// Fetch sprite data, increment data counter
INLINE void vic2_spr_data_access( running_machine *machine, vic2_state *vic2, int num, int bytenum )
{
	if (vic2->spr_dma_on & (1 << num))
	{
		vic2->spr_data[num][bytenum] = vic2->dma_read(machine, (vic2->mc[num] & 0x3f) | vic2->spr_ptr[num]);
		vic2->mc[num]++;
	}
	else
		if (bytenum == 1)
			vic2_idle_access(machine, vic2);
}

// Turn on display if Bad Line
INLINE void vic2_display_if_bad_line( vic2_state *vic2 )
{
	if (vic2->is_bad_line)
		vic2->display_state = 1;
}

// Suspend CPU
INLINE void vic2_suspend_cpu( running_machine *machine, vic2_state *vic2 )
{
	if (vic2->cpu_suspended == 0)
	{
		vic2->first_ba_cycle = vic2->cycles_counter;
		if ((vic2->rdy_workaround_cb != NULL) && (vic2->rdy_workaround_cb(machine) != 7 ))
		{
//          cpu_suspend(machine->firstcpu, SUSPEND_REASON_SPIN, 0);
		}
		vic2->cpu_suspended = 1;
	}
}

// Resume CPU
INLINE void vic2_resume_cpu( running_machine *machine, vic2_state *vic2 )
{
	if (vic2->cpu_suspended == 1)
	{
		if ((vic2->rdy_workaround_cb != NULL))
		{
//  cpu_resume(machine->firstcpu, SUSPEND_REASON_SPIN);
		}
		vic2->cpu_suspended = 0;
	}
}

// Refresh access
INLINE void vic2_refresh_access( running_machine *machine, vic2_state *vic2 )
{
	vic2->dma_read(machine, 0x3f00 | vic2->ref_cnt--);
}


INLINE void vic2_fetch_if_bad_line( vic2_state *vic2 )
{
	if (vic2->is_bad_line)
		vic2->display_state = 1;
}


// Turn on display and matrix access and reset RC if Bad Line
INLINE void vic2_rc_if_bad_line( vic2_state *vic2 )
{
	if (vic2->is_bad_line)
	{
		vic2->display_state = 1;
		vic2->rc = 0;
	}
}

// Sample border color and increment vic2->graphic_x
INLINE void vic2_sample_border( vic2_state *vic2 )
{
	if (vic2->draw_this_line)
	{
		if (vic2->border_on)
			vic2->border_color_sample[vic2->cycle - 13] = FRAMECOLOR;
		vic2->graphic_x += 8;
	}
}


// Turn on sprite DMA if necessary
INLINE void vic2_check_sprite_dma( vic2_state *vic2 )
{
	int i;
	UINT8 mask = 1;

	for (i = 0; i < 8; i++, mask <<= 1)
		if (SPRITEON(i) && ((vic2->rasterline & 0xff) == SPRITE_Y_POS(i)))
		{
			vic2->spr_dma_on |= mask;
			vic2->mc_base[i] = 0;
			if (SPRITE_Y_EXPAND(i))
				vic2->spr_exp_y &= ~mask;
		}
}

// Video matrix access
INLINE void vic2_matrix_access( running_machine *machine, vic2_state *vic2 )
{
//  if (vic2->cpu_suspended == 1)
	{
		if ((vic2->cycles_counter - vic2->first_ba_cycle) < 0)
			vic2->matrix_line[vic2->ml_index] = vic2->color_line[vic2->ml_index] = 0xff;
		else
		{
			UINT16 adr = (vic2->vc & 0x03ff) | VIDEOADDR;
			vic2->matrix_line[vic2->ml_index] = vic2->dma_read(machine, adr); \
			vic2->color_line[vic2->ml_index] = vic2->dma_read_color(machine, (adr & 0x03ff)); \
		}
	}
}

// Graphics data access
INLINE void vic2_graphics_access( running_machine *machine, vic2_state *vic2 )
{
	if (vic2->display_state == 1)
	{
		UINT16 adr;
		if (HIRESON)
			adr = ((vic2->vc & 0x03ff) << 3) | vic2->bitmapaddr | vic2->rc;
		else
			adr = (vic2->matrix_line[vic2->ml_index] << 3) | vic2->chargenaddr | vic2->rc;
		if (ECMON)
			adr &= 0xf9ff;
		vic2->gfx_data = vic2->dma_read(machine, adr);
		vic2->char_data = vic2->matrix_line[vic2->ml_index];
		vic2->color_data = vic2->color_line[vic2->ml_index];
		vic2->ml_index++;
		vic2->vc++;
	}
	else
	{
		vic2->gfx_data = vic2->dma_read(machine, (ECMON ? 0x39ff : 0x3fff));
		vic2->char_data = 0;
	}
}

INLINE void vic2_draw_background( vic2_state *vic2 )
{
	if (vic2->draw_this_line)
	{
		UINT8 c;

		switch (GFXMODE)
		{
			case 0:
			case 1:
			case 3:
				c = vic2->colors[0];
				break;
			case 2:
				c = vic2->last_char_data & 0x0f;
				break;
			case 4:
				if (vic2->last_char_data & 0x80)
					if (vic2->last_char_data & 0x40)
						c = vic2->colors[3];
					else
						c = vic2->colors[2];
				else
					if (vic2->last_char_data & 0x40)
						c = vic2->colors[1];
					else
						c = vic2->colors[0];
				break;
			default:
				c = 0;
				break;
		}
		plot_box(vic2->bitmap, vic2->graphic_x, VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, c);
	}
}

INLINE void vic2_draw_mono( vic2_state *vic2, UINT16 p, UINT8 c0, UINT8 c1 )
{
	UINT8 c[2];
	UINT8 data = vic2->gfx_data;

	c[0] = c0;
	c[1] = c1;

	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 7) = c[data & 1];
	vic2->fore_coll_buf[p + 7] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 6) = c[data & 1];
	vic2->fore_coll_buf[p + 6] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 5) = c[data & 1];
	vic2->fore_coll_buf[p + 5] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 4) = c[data & 1];
	vic2->fore_coll_buf[p + 4] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 3) = c[data & 1];
	vic2->fore_coll_buf[p + 3] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 2) = c[data & 1];
	vic2->fore_coll_buf[p + 2] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 1) = c[data & 1];
	vic2->fore_coll_buf[p + 1] = data & 1; data >>= 1;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 0) = c[data];
	vic2->fore_coll_buf[p + 0] = data & 1;
}

INLINE void vic2_draw_multi( vic2_state *vic2, UINT16 p, UINT8 c0, UINT8 c1, UINT8 c2, UINT8 c3 )
{
	UINT8 c[4];
	UINT8 data = vic2->gfx_data;

	c[0] = c0;
	c[1] = c1;
	c[2] = c2;
	c[3] = c3;

	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 7) = c[data & 3];
	vic2->fore_coll_buf[p + 7] = data & 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 6) = c[data & 3];
	vic2->fore_coll_buf[p + 6] = data & 2; data >>= 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 5) = c[data & 3];
	vic2->fore_coll_buf[p + 5] = data & 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 4) = c[data & 3];
	vic2->fore_coll_buf[p + 4] = data & 2; data >>= 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 3) = c[data & 3];
	vic2->fore_coll_buf[p + 3] = data & 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 2) = c[data & 3];
	vic2->fore_coll_buf[p + 2] = data & 2; data >>= 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 1) = c[data];
	vic2->fore_coll_buf[p + 1] = data & 2;
	*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 0) = c[data];
	vic2->fore_coll_buf[p + 0] = data & 2;
}

// Graphics display (8 pixels)
static void vic2_draw_graphics( vic2_state *vic2 )
{
	if (vic2->draw_this_line == 0)
	{
		UINT16 p = vic2->graphic_x + HORIZONTALPOS;
		vic2->fore_coll_buf[p + 7] = 0;
		vic2->fore_coll_buf[p + 6] = 0;
		vic2->fore_coll_buf[p + 5] = 0;
		vic2->fore_coll_buf[p + 4] = 0;
		vic2->fore_coll_buf[p + 3] = 0;
		vic2->fore_coll_buf[p + 2] = 0;
		vic2->fore_coll_buf[p + 1] = 0;
		vic2->fore_coll_buf[p + 0] = 0;
	}
	else if (vic2->ud_border_on)
	{
		UINT16 p = vic2->graphic_x + HORIZONTALPOS;
		vic2->fore_coll_buf[p + 7] = 0;
		vic2->fore_coll_buf[p + 6] = 0;
		vic2->fore_coll_buf[p + 5] = 0;
		vic2->fore_coll_buf[p + 4] = 0;
		vic2->fore_coll_buf[p + 3] = 0;
		vic2->fore_coll_buf[p + 2] = 0;
		vic2->fore_coll_buf[p + 1] = 0;
		vic2->fore_coll_buf[p + 0] = 0;
		vic2_draw_background(vic2);
	}
	else
	{
		UINT8 tmp_col;
		UINT16 p = vic2->graphic_x + HORIZONTALPOS;
		switch (GFXMODE)
		{
			case 0:
				vic2_draw_mono(vic2, p, vic2->colors[0], vic2->color_data & 0x0f);
				break;
			case 1:
				if (vic2->color_data & 0x08)
					vic2_draw_multi(vic2, p, vic2->colors[0], vic2->colors[1], vic2->colors[2], vic2->color_data & 0x07);
				else
					vic2_draw_mono(vic2, p, vic2->colors[0], vic2->color_data & 0x0f);
				break;
			case 2:
				vic2_draw_mono(vic2, p, vic2->char_data & 0x0f, vic2->char_data >> 4);
				break;
			case 3:
				vic2_draw_multi(vic2, p, vic2->colors[0], vic2->char_data >> 4, vic2->char_data & 0x0f, vic2->color_data & 0x0f);
				break;
			case 4:
				if (vic2->char_data & 0x80)
					if (vic2->char_data & 0x40)
						tmp_col = vic2->colors[3];
					else
						tmp_col = vic2->colors[2];
				else
					if (vic2->char_data & 0x40)
						tmp_col = vic2->colors[1];
					else
						tmp_col = vic2->colors[0];
				vic2_draw_mono(vic2, p, tmp_col, vic2->color_data & 0x0f);
				break;
			case 5:
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 7) = 0;
				vic2->fore_coll_buf[p + 7] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 6) = 0;
				vic2->fore_coll_buf[p + 6] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 5) = 0;
				vic2->fore_coll_buf[p + 5] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 4) = 0;
				vic2->fore_coll_buf[p + 4] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 3) = 0;
				vic2->fore_coll_buf[p + 3] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 2) = 0;
				vic2->fore_coll_buf[p + 2] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 1) = 0;
				vic2->fore_coll_buf[p + 1] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 0) = 0;
				vic2->fore_coll_buf[p + 0] = 0;
				break;
			case 6:
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 7) = 0;
				vic2->fore_coll_buf[p + 7] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 6) = 0;
				vic2->fore_coll_buf[p + 6] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 5) = 0;
				vic2->fore_coll_buf[p + 5] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 4) = 0;
				vic2->fore_coll_buf[p + 4] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 3) = 0;
				vic2->fore_coll_buf[p + 3] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 2) = 0;
				vic2->fore_coll_buf[p + 2] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 1) = 0;
				vic2->fore_coll_buf[p + 1] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 0) = 0;
				vic2->fore_coll_buf[p + 0] = 0;
				break;
			case 7:
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 7) = 0;
				vic2->fore_coll_buf[p + 7] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 6) = 0;
				vic2->fore_coll_buf[p + 6] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 5) = 0;
				vic2->fore_coll_buf[p + 5] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 4) = 0;
				vic2->fore_coll_buf[p + 4] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 3) = 0;
				vic2->fore_coll_buf[p + 3] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 2) = 0;
				vic2->fore_coll_buf[p + 2] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 1) = 0;
				vic2->fore_coll_buf[p + 1] = 0;
				*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + 0) = 0;
				vic2->fore_coll_buf[p + 0] = 0;
				break;
		}
	}
}

static void vic2_draw_sprites( running_machine *machine, vic2_state *vic2 )
{
	int i;
	UINT8 snum, sbit;
	UINT8 spr_coll = 0, gfx_coll = 0;
	UINT32 plane0_l, plane0_r, plane1_l, plane1_r;
	UINT32 sdata_l = 0, sdata_r = 0;

	for (i = 0; i < 0x400; i++)
		vic2->spr_coll_buf[i] = 0;

	for (snum = 0, sbit = 1; snum < 8; snum++, sbit <<= 1)
	{
		if ((vic2->spr_draw & sbit) && (SPRITE_X_POS(snum) <= (403 - (VIC2_FIRSTCOLUMN + 1))))
		{
			UINT16 p = SPRITE_X_POS(snum) + VIC2_X_2_EMU(0) + 8;
			UINT8 color = SPRITE_COLOR(snum);
			UINT32 sdata = (vic2->spr_draw_data[snum][0] << 24) | (vic2->spr_draw_data[snum][1] << 16) | (vic2->spr_draw_data[snum][2] << 8);

			if (SPRITE_X_EXPAND(snum))
			{
				if (SPRITE_X_POS(snum) > (403 - 24 - (VIC2_FIRSTCOLUMN + 1)))
					continue;

				if (SPRITE_MULTICOLOR(snum))
				{
					sdata_l = (vic2->expandx_multi[(sdata >> 24) & 0xff] << 16) | vic2->expandx_multi[(sdata >> 16) & 0xff];
					sdata_r = vic2->expandx_multi[(sdata >> 8) & 0xff] << 16;
					plane0_l = (sdata_l & 0x55555555) | (sdata_l & 0x55555555) << 1;
					plane1_l = (sdata_l & 0xaaaaaaaa) | (sdata_l & 0xaaaaaaaa) >> 1;
					plane0_r = (sdata_r & 0x55555555) | (sdata_r & 0x55555555) << 1;
					plane1_r = (sdata_r & 0xaaaaaaaa) | (sdata_r & 0xaaaaaaaa) >> 1;
					for (i = 0; i < 32; i++, plane0_l <<= 1, plane1_l <<= 1)
					{
						UINT8 col;

						if (plane1_l & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}
							if (plane0_l & 0x80000000)
								col = vic2->spritemulti[3];
							else
								col = color;
						}
						else
						{
							if (plane0_l & 0x80000000)
							{
								if (vic2->fore_coll_buf[p + i])
								{
									gfx_coll |= sbit;
								}
								col = vic2->spritemulti[1];
							}
							else
								continue;
						}

						if (vic2->spr_coll_buf[p + i])
							spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
						else
						{
							if (SPRITE_PRIORITY(snum))
							{
								if (vic2->fore_coll_buf[p + i] == 0)
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
							else
							{
								*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
						}
					}

					for (; i < 48; i++, plane0_r <<= 1, plane1_r <<= 1)
					{
						UINT8 col;

						if(plane1_r & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}

							if (plane0_r & 0x80000000)
								col = vic2->spritemulti[3];
							else
								col = color;
						}
						else
						{
							if (plane0_r & 0x80000000)
							{
								if (vic2->fore_coll_buf[p + i])
								{
									gfx_coll |= sbit;
								}
								col =  vic2->spritemulti[1];
							}
							else
								continue;
						}

						if (vic2->spr_coll_buf[p + i])
							spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
						else
						{
							if (SPRITE_PRIORITY(snum))
							{
								if (vic2->fore_coll_buf[p + i] == 0)
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
							else
							{
								*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
						}
					}
				}
				else
				{
					sdata_l = (vic2->expandx[(sdata >> 24) & 0xff] << 16) | vic2->expandx[(sdata >> 16) & 0xff];
					sdata_r = vic2->expandx[(sdata >> 8) & 0xff] << 16;

					for (i = 0; i < 32; i++, sdata_l <<= 1)
						if (sdata_l & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}

							if (vic2->spr_coll_buf[p + i])
								spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
							else
							{
								if (SPRITE_PRIORITY(snum))
								{
									if (vic2->fore_coll_buf[p + i] == 0)
										*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
								else
								{
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
							}
						}

					for (; i < 48; i++, sdata_r <<= 1)
						if (sdata_r & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}

							if (vic2->spr_coll_buf[p + i])
								spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
							else
							{
								if (SPRITE_PRIORITY(snum))
								{
									if (vic2->fore_coll_buf[p + i] == 0)
										*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
								else
								{
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
							}
						}
				}
			}
			else
			{
				if (SPRITE_MULTICOLOR(snum))
				{
					UINT32 plane0 = (sdata & 0x55555555) | (sdata & 0x55555555) << 1;
					UINT32 plane1 = (sdata & 0xaaaaaaaa) | (sdata & 0xaaaaaaaa) >> 1;

					for (i = 0; i < 24; i++, plane0 <<= 1, plane1 <<= 1)
					{
						UINT8 col;

						if (plane1 & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}

							if (plane0 & 0x80000000)
								col = vic2->spritemulti[3];
							else
								col = color;
						}
						else
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}

							if (plane0 & 0x80000000)
								col = vic2->spritemulti[1];
							else
								continue;
						}

						if (vic2->spr_coll_buf[p + i])
							spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
						else
						{
							if (SPRITE_PRIORITY(snum))
							{
								if (vic2->fore_coll_buf[p + i] == 0)
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
							else
							{
								*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = col;
								vic2->spr_coll_buf[p + i] = sbit;
							}
						}
					}
				}
				else
				{
					for (i = 0; i < 24; i++, sdata <<= 1)
					{
						if (sdata & 0x80000000)
						{
							if (vic2->fore_coll_buf[p + i])
							{
								gfx_coll |= sbit;
							}
							if (vic2->spr_coll_buf[p + i])
							{
								spr_coll |= vic2->spr_coll_buf[p + i] | sbit;
							}
							else
							{
								if (SPRITE_PRIORITY(snum))
								{
									if (vic2->fore_coll_buf[p + i] == 0)
										*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
								else
								{
									*BITMAP_ADDR16(vic2->bitmap, VIC2_RASTER_2_EMU(vic2->rasterline), p + i) = color;
									vic2->spr_coll_buf[p + i] = sbit;
								}
							}
						}
					}
				}
			}
		}
	}

	if (SPRITE_COLL)
		SPRITE_COLL |= spr_coll;
	else
	{
		SPRITE_COLL = spr_coll;
		if (SPRITE_COLL)
			vic2_set_interrupt(machine, 4, vic2);
	}

	if (SPRITE_BG_COLL)
		SPRITE_BG_COLL |= gfx_coll;
	else
	{
		SPRITE_BG_COLL = gfx_coll;
		if (SPRITE_BG_COLL)
			vic2_set_interrupt(machine, 2, vic2);
	}
}


static TIMER_CALLBACK( pal_timer_callback )
{
	vic2_state *vic2 = (vic2_state *)ptr;
	int i;
	UINT8 mask;
	static int adjust[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

	UINT8 cpu_cycles = machine->device<cpu_device>("maincpu")->total_cycles() & 0xff;
	UINT8 vic_cycles = (vic2->cycles_counter + 1) & 0xff;
	vic2->cycles_counter++;

//  printf("%02x %02x %02x\n",cpu_cycles,vic_cycles,rdy_cycles);
/*
if (input_code_pressed(machine, KEYCODE_X))
{
if (input_code_pressed_once(machine, KEYCODE_Q)) adjust[1]++;
if (input_code_pressed_once(machine, KEYCODE_W)) adjust[2]++;
if (input_code_pressed_once(machine, KEYCODE_E)) adjust[3]++;
if (input_code_pressed_once(machine, KEYCODE_R)) adjust[4]++;
if (input_code_pressed_once(machine, KEYCODE_T)) adjust[5]++;
if (input_code_pressed_once(machine, KEYCODE_Y)) adjust[6]++;
if (input_code_pressed_once(machine, KEYCODE_U)) adjust[7]++;
if (input_code_pressed_once(machine, KEYCODE_I)) adjust[8]++;
if (input_code_pressed_once(machine, KEYCODE_A)) adjust[1]--;
if (input_code_pressed_once(machine, KEYCODE_S)) adjust[2]--;
if (input_code_pressed_once(machine, KEYCODE_D)) adjust[3]--;
if (input_code_pressed_once(machine, KEYCODE_F)) adjust[4]--;
if (input_code_pressed_once(machine, KEYCODE_G)) adjust[5]--;
if (input_code_pressed_once(machine, KEYCODE_H)) adjust[6]--;
if (input_code_pressed_once(machine, KEYCODE_J)) adjust[7]--;
if (input_code_pressed_once(machine, KEYCODE_K)) adjust[8]--;
if (input_code_pressed_once(machine, KEYCODE_C)) adjust[0]++;
if (input_code_pressed_once(machine, KEYCODE_V)) adjust[0]--;
if (input_code_pressed_once(machine, KEYCODE_Z)) printf("b:%02x 1:%02x 2:%02x 3:%02x 4:%02x 5:%02x 6:%02x 7:%02x 8:%02x\n",
                                adjust[0],adjust[1],adjust[2],adjust[3],adjust[4],adjust[5],adjust[6],adjust[7],adjust[8]);
}
*/

	switch(vic2->cycle)
	{

	// Sprite 3, raster counter, raster IRQ, bad line
	case 1:
		if (vic2->rasterline == (VIC2_LINES - 1))
		{
			vic2->vblanking = 1;

//          if (LIGHTPEN_BUTTON)
			{
				/* lightpen timer start */
				timer_set(machine, attotime_make(0, 0), vic2, 1, vic2_timer_timeout);
			}
		}
		else
		{
			vic2->rasterline++;

			if (vic2->rasterline == VIC2_FIRST_DMA_LINE)
				vic2->bad_lines_enabled = SCREENON;

			vic2->is_bad_line = ((vic2->rasterline >= VIC2_FIRST_DMA_LINE) && (vic2->rasterline <= VIC2_LAST_DMA_LINE) &&
						((vic2->rasterline & 0x07) == VERTICALPOS) && vic2->bad_lines_enabled);

			vic2->draw_this_line =	((VIC2_RASTER_2_EMU(vic2->rasterline) >= VIC2_RASTER_2_EMU(VIC2_FIRST_DISP_LINE)) &&
						(VIC2_RASTER_2_EMU(vic2->rasterline ) <= VIC2_RASTER_2_EMU(VIC2_LAST_DISP_LINE)));
		}

		vic2->border_on_sample[0] = vic2->border_on;
		vic2_spr_ptr_access(machine, vic2, 3);
		vic2_spr_data_access(machine, vic2, 3, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x08)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x08) rdy_cycles += (2 + adjust[1]);

		vic2->cycle++;
		break;

	// Sprite 3
	case 2:
		if (vic2->vblanking)
		{
			// Vertical blank, reset counters
			vic2->rasterline = vic2->vc_base = 0;
			vic2->ref_cnt = 0xff;
			vic2->vblanking = 0;

			// Trigger raster IRQ if IRQ in line 0
			if (RASTERLINE == 0)
			{
				vic2_set_interrupt(machine, 1, vic2);
			}
		}

		if (vic2->rasterline == RASTERLINE)
		{
			vic2_set_interrupt(machine, 1, vic2);
		}

		vic2->graphic_x = VIC2_X_2_EMU(0);

		vic2_spr_data_access(machine, vic2, 3, 1);
		vic2_spr_data_access(machine, vic2, 3, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 4
	case 3:
		vic2_spr_ptr_access(machine, vic2, 4);
		vic2_spr_data_access(machine, vic2, 4, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x10)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x10) rdy_cycles += (2 + adjust[2]);

		vic2->cycle++;
		break;

	// Sprite 4
	case 4:
		vic2_spr_data_access(machine, vic2, 4, 1);
		vic2_spr_data_access(machine, vic2, 4, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 5
	case 5:
		vic2_spr_ptr_access(machine, vic2, 5);
		vic2_spr_data_access(machine, vic2, 5, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x20)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x20) rdy_cycles += (2 + adjust[3]);

		vic2->cycle++;
		break;

	// Sprite 5
	case 6:
		vic2_spr_data_access(machine, vic2, 5, 1);
		vic2_spr_data_access(machine, vic2, 5, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 6
	case 7:
		vic2_spr_ptr_access(machine, vic2, 6);
		vic2_spr_data_access(machine, vic2, 6, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x40)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x40) rdy_cycles += (2 + adjust[4]);

		vic2->cycle++;
		break;

	// Sprite 6
	case 8:
		vic2_spr_data_access(machine, vic2, 6, 1);
		vic2_spr_data_access(machine, vic2, 6, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 7
	case 9:
		vic2_spr_ptr_access(machine, vic2, 7);
		vic2_spr_data_access(machine, vic2, 7, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x80)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x80) rdy_cycles += (2 + adjust[5]);

		vic2->cycle++;
		break;

	// Sprite 7
	case 10:
		vic2_spr_data_access(machine, vic2, 7, 1);
		vic2_spr_data_access(machine, vic2, 7, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Refresh
	case 11:
		vic2_refresh_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line
	case 12:
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, raster_x
	case 13:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		vic2->raster_x = 0xfffc;

		if ((vic2->rdy_workaround_cb(machine) == 0 ) && (vic2->is_bad_line))
			rdy_cycles += (43+adjust[0]);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, RC, VC
	case 14:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_rc_if_bad_line(vic2);

		vic2->vc = vic2->vc_base;

		if ((vic2->rdy_workaround_cb(machine) == 1 ) && (vic2->is_bad_line))
			rdy_cycles += (42+adjust[0]);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, sprite y expansion
	case 15:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		for (i = 0; i < 8; i++)
			if (vic2->spr_exp_y & (1 << i))
				vic2->mc_base[i] += 2;

		vic2->ml_index = 0;
		vic2_matrix_access(machine, vic2);

		if ((vic2->rdy_workaround_cb(machine) == 2 ) && (vic2->is_bad_line))
			rdy_cycles += (41+adjust[0]);

		vic2->cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 16:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			if (vic2->spr_exp_y & mask)
				vic2->mc_base[i]++;
			if ((vic2->mc_base[i] & 0x3f) == 0x3f)
				vic2->spr_dma_on &= ~mask;
		}

		vic2_matrix_access(machine, vic2);

		if ((vic2->rdy_workaround_cb(machine) == 3 ) && (vic2->is_bad_line))
			rdy_cycles += (40+adjust[0]);

		vic2->cycle++;
		break;

	// Graphics, check border
	case 17:
		if (COLUMNS40)
		{
			if (vic2->rasterline == vic2->dy_stop)
				vic2->ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2->rasterline == vic2->dy_start)
						vic2->border_on = vic2->ud_border_on = 0;
					else
						if (vic2->ud_border_on == 0)
							vic2->border_on = 0;
				} else
					if (vic2->ud_border_on == 0)
						vic2->border_on = 0;
			}
		}

		// Second sample of border state
		vic2->border_on_sample[1] = vic2->border_on;

		vic2_draw_background(vic2);
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);
		vic2_matrix_access(machine, vic2);

		if ((vic2->rdy_workaround_cb(machine) == 4 ) && (vic2->is_bad_line))
			rdy_cycles += (40+adjust[0]);

		vic2->cycle++;
		break;

	// Check border
	case 18:
		if (!COLUMNS40)
		{
			if (vic2->rasterline == vic2->dy_stop)
				vic2->ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2->rasterline == vic2->dy_start)
						vic2->border_on = vic2->ud_border_on = 0;
					else
						if (vic2->ud_border_on == 0)
							vic2->border_on = 0;
				} else
					if (vic2->ud_border_on == 0)
						vic2->border_on = 0;
			}
		}

		// Third sample of border state
		vic2->border_on_sample[2] = vic2->border_on;

	// Graphics

	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);
		vic2_matrix_access(machine, vic2);
		vic2->last_char_data = vic2->char_data;

		vic2->cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 55:
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		// sprite y expansion
		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if (SPRITE_Y_EXPAND (i))
				vic2->spr_exp_y ^= mask;

		vic2_check_sprite_dma(vic2);

		vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Check border, sprite DMA
	case 56:
		if (!COLUMNS40)
			vic2->border_on = 1;

		// Fourth sample of border state
		vic2->border_on_sample[3] = vic2->border_on;

		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);
		vic2_check_sprite_dma(vic2);

		vic2->cycle++;
		break;

	// Check border, sprites
	case 57:
		if (COLUMNS40)
			vic2->border_on = 1;

		// Fifth sample of border state
		vic2->border_on_sample[4] = vic2->border_on;

		// Sample spr_disp_on and spr_data for sprite drawing
		vic2->spr_draw = vic2->spr_disp_on;
		if (vic2->spr_draw)
			memcpy(vic2->spr_draw_data, vic2->spr_data, 8 * 4);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if ((vic2->spr_disp_on & mask) && !(vic2->spr_dma_on & mask))
				vic2->spr_disp_on &= ~mask;

		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 0, sprite DMA, MC, RC
	case 58:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			vic2->mc[i] = vic2->mc_base[i];
			if ((vic2->spr_dma_on & mask) && ((vic2->rasterline & 0xff) == SPRITE_Y_POS(i)))
				vic2->spr_disp_on |= mask;
		}

		vic2_spr_ptr_access(machine, vic2, 0);
		vic2_spr_data_access(machine, vic2, 0, 0);

		if (vic2->rc == 7)
		{
			vic2->vc_base = vic2->vc;
			vic2->display_state = 0;
		}

		if (vic2->is_bad_line || vic2->display_state)
		{
			vic2->display_state = 1;
			vic2->rc = (vic2->rc + 1) & 7;
		}

		if (vic2->spr_dma_on & 0x01)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x01) rdy_cycles += (2 + adjust[6]);

		vic2->cycle++;
		break;

	// Sprite 0
	case 59:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_spr_data_access(machine, vic2, 0, 1);
		vic2_spr_data_access(machine, vic2, 0, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 1, draw
	case 60:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);

		if (vic2->draw_this_line)
		{
			vic2_draw_sprites(machine, vic2);

			if (vic2->border_on_sample[0])
				for (i = 0; i < 4; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);

			if (vic2->border_on_sample[1])
				plot_box(vic2->bitmap, VIC2_X_2_EMU(4 * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[4]);

			if (vic2->border_on_sample[2])
				for (i = 5; i < 43; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);

			if (vic2->border_on_sample[3])
				plot_box(vic2->bitmap, VIC2_X_2_EMU(43 * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[43]);

			if (vic2->border_on_sample[4])
			{
				for (i = 44; i < 48; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);
				for (i = 48; i < 51; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[47]);
			}
		}

		vic2_spr_ptr_access(machine, vic2, 1);
		vic2_spr_data_access(machine, vic2, 1, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x02)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x02) rdy_cycles += (2 + adjust[7]);

		vic2->cycle++;
		break;

	// Sprite 1
	case 61:
		vic2_spr_data_access(machine, vic2, 1, 1);
		vic2_spr_data_access(machine, vic2, 1, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 2
	case 62:
		vic2_spr_ptr_access(machine, vic2, 2);
		vic2_spr_data_access(machine, vic2, 2, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x04)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		if (vic2->spr_dma_on & 0x04) rdy_cycles += (2 + adjust[8]);

		vic2->cycle++;
		break;

	// Sprite 2
	case 63:
		vic2_spr_data_access(machine, vic2, 2, 1);
		vic2_spr_data_access(machine, vic2, 2, 2);
		vic2_display_if_bad_line(vic2);

		if (vic2->rasterline == vic2->dy_stop)
			vic2->ud_border_on = 1;
		else
			if (SCREENON && (vic2->rasterline == vic2->dy_start))
				vic2->ud_border_on = 0;

		// Last cycle
		vic2->cycle = 1;
	}

	if ((cpu_cycles == vic_cycles) && (rdy_cycles > 0))
	{
		device_spin_until_time (machine->firstcpu, machine->device<cpu_device>("maincpu")->cycles_to_attotime(rdy_cycles));
		rdy_cycles = 0;
	}

	vic2->raster_x += 8;
	timer_set(machine, machine->device<cpu_device>("maincpu")->cycles_to_attotime(1), vic2, 0, pal_timer_callback);
}

static TIMER_CALLBACK( ntsc_timer_callback )
{
	vic2_state *vic2 = (vic2_state *)ptr;
	int i;
	UINT8 mask;
	vic2->cycles_counter++;

	switch (vic2->cycle)
	{

	// Sprite 3, raster counter, raster IRQ, bad line
	case 1:
		if (vic2->rasterline == (VIC2_LINES - 1))
		{
			vic2->vblanking = 1;

//          if (LIGHTPEN_BUTTON)
			{
				/* lightpen timer starten */
				timer_set(machine, attotime_make(0, 0), vic2, 1, vic2_timer_timeout);
			}
		}
		else
		{
			vic2->rasterline++;

			if (vic2->rasterline == VIC2_FIRST_DMA_LINE)
				vic2->bad_lines_enabled = SCREENON;

			vic2->is_bad_line = ((vic2->rasterline >= VIC2_FIRST_DMA_LINE) && (vic2->rasterline <= VIC2_LAST_DMA_LINE) &&
						((vic2->rasterline & 0x07) == VERTICALPOS) && vic2->bad_lines_enabled);

			vic2->draw_this_line = ((VIC2_RASTER_2_EMU(vic2->rasterline) >= VIC2_RASTER_2_EMU(VIC2_FIRST_DISP_LINE)) &&
						(VIC2_RASTER_2_EMU(vic2->rasterline ) <= VIC2_RASTER_2_EMU(VIC2_LAST_DISP_LINE)));
		}

		vic2->border_on_sample[0] = vic2->border_on;
		vic2_spr_ptr_access(machine, vic2, 3);
		vic2_spr_data_access(machine, vic2, 3, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x08)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 3
	case 2:
		if (vic2->vblanking)
		{
			// Vertical blank, reset counters
			vic2->rasterline = vic2->vc_base = 0;
			vic2->ref_cnt = 0xff;
			vic2->vblanking = 0;

			// Trigger raster IRQ if IRQ in line 0
			if (RASTERLINE == 0)
			{
				vic2_set_interrupt(machine, 1, vic2);
			}
		}

		if (vic2->rasterline == RASTERLINE)
		{
			vic2_set_interrupt(machine, 1, vic2);
		}

		vic2->graphic_x = VIC2_X_2_EMU(0);

		vic2_spr_data_access(machine, vic2, 3, 1);
		vic2_spr_data_access(machine, vic2, 3, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 4
	case 3:
		vic2_spr_ptr_access(machine, vic2, 4);
		vic2_spr_data_access(machine, vic2, 4, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x10)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 4
	case 4:
		vic2_spr_data_access(machine, vic2, 4, 1);
		vic2_spr_data_access(machine, vic2, 4, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 5
	case 5:
		vic2_spr_ptr_access(machine, vic2, 5);
		vic2_spr_data_access(machine, vic2, 5, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x20)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 5
	case 6:
		vic2_spr_data_access(machine, vic2, 5, 1);
		vic2_spr_data_access(machine, vic2, 5, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 6
	case 7:
		vic2_spr_ptr_access(machine, vic2, 6);
		vic2_spr_data_access(machine, vic2, 6, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x40)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 6
	case 8:
		vic2_spr_data_access(machine, vic2, 6, 1);
		vic2_spr_data_access(machine, vic2, 6, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 7
	case 9:
		vic2_spr_ptr_access(machine, vic2, 7);
		vic2_spr_data_access(machine, vic2, 7, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x80)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 7
	case 10:
		vic2_spr_data_access(machine, vic2, 7, 1);
		vic2_spr_data_access(machine, vic2, 7, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Refresh
	case 11:
		vic2_refresh_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line
	case 12:
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, raster_x
	case 13:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		vic2->raster_x = 0xfffc;

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, RC, VC
	case 14:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_rc_if_bad_line(vic2);

		vic2->vc = vic2->vc_base;

		vic2->cycle++;
		break;

	// Refresh, fetch if bad line, sprite y expansion
	case 15:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_refresh_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		for (i = 0; i < 8; i++)
			if (vic2->spr_exp_y & (1 << i))
				vic2->mc_base[i] += 2;

		if (vic2->is_bad_line)
			vic2_suspend_cpu(machine, vic2);

		vic2->ml_index = 0;
		vic2_matrix_access(machine, vic2);

		vic2->cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 16:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			if (vic2->spr_exp_y & mask)
				vic2->mc_base[i]++;
			if ((vic2->mc_base[i] & 0x3f) == 0x3f)
				vic2->spr_dma_on &= ~mask;
		}

		vic2_matrix_access(machine, vic2);

		vic2->cycle++;
		break;

	// Graphics, check border
	case 17:
		if (COLUMNS40)
		{
			if (vic2->rasterline == vic2->dy_stop)
				vic2->ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2->rasterline == vic2->dy_start)
						vic2->border_on = vic2->ud_border_on = 0;
					else
						if (vic2->ud_border_on == 0)
							vic2->border_on = 0;
				}
				else
					if (vic2->ud_border_on == 0)
						vic2->border_on = 0;
			}
		}

		// Second sample of border state
		vic2->border_on_sample[1] = vic2->border_on;

		vic2_draw_background(vic2);
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);
		vic2_matrix_access(machine, vic2);

		vic2->cycle++;
		break;

	// Check border
	case 18:
		if (!COLUMNS40)
		{
			if (vic2->rasterline == vic2->dy_stop)
				vic2->ud_border_on = 1;
			else
			{
				if (SCREENON)
				{
					if (vic2->rasterline == vic2->dy_start)
						vic2->border_on = vic2->ud_border_on = 0;
					else
						if (vic2->ud_border_on == 0)
							vic2->border_on = 0;
				}
				else
					if (vic2->ud_border_on == 0)
						vic2->border_on = 0;
			}
		}

		// Third sample of border state
		vic2->border_on_sample[2] = vic2->border_on;

	// Graphics

	case 19:
	case 20:
	case 21:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_fetch_if_bad_line(vic2);
		vic2_matrix_access(machine, vic2);
		vic2->last_char_data = vic2->char_data;

		vic2->cycle++;
		break;

	// Graphics, sprite y expansion, sprite DMA
	case 55:
		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_graphics_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		// sprite y expansion
		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if (SPRITE_Y_EXPAND (i))
				vic2->spr_exp_y ^= mask;

		vic2_check_sprite_dma(vic2);

		vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Check border, sprite DMA
	case 56:
		if (!COLUMNS40)
			vic2->border_on = 1;

		// Fourth sample of border state
		vic2->border_on_sample[3] = vic2->border_on;

		vic2_draw_graphics(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);
		vic2_check_sprite_dma(vic2);

		vic2->cycle++;
		break;

	// Check border, sprites
	case 57:
		if (COLUMNS40)
			vic2->border_on = 1;

		// Fifth sample of border state
		vic2->border_on_sample[4] = vic2->border_on;

		// Sample spr_disp_on and spr_data for sprite drawing
		vic2->spr_draw = vic2->spr_disp_on;
		if (vic2->spr_draw)
			memcpy(vic2->spr_draw_data, vic2->spr_data, 8 * 4);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
			if ((vic2->spr_disp_on & mask) && !(vic2->spr_dma_on & mask))
				vic2->spr_disp_on &= ~mask;

		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// for NTSC 6567R8
	case 58:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// for NTSC 6567R8
	case 59:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_idle_access(machine, vic2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 0, sprite DMA, MC, RC
	case 60:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);

		mask = 1;
		for (i = 0; i < 8; i++, mask <<= 1)
		{
			vic2->mc[i] = vic2->mc_base[i];
			if ((vic2->spr_dma_on & mask) && ((vic2->rasterline & 0xff) == SPRITE_Y_POS(i)))
				vic2->spr_disp_on |= mask;
		}

		vic2_spr_ptr_access(machine, vic2, 0);
		vic2_spr_data_access(machine, vic2, 0, 0);

		if (vic2->rc == 7)
		{
			vic2->vc_base = vic2->vc;
			vic2->display_state = 0;
		}

		if (vic2->is_bad_line || vic2->display_state)
		{
			vic2->display_state = 1;
			vic2->rc = (vic2->rc + 1) & 7;
		}

		if (vic2->spr_dma_on & 0x01)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 0
	case 61:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);
		vic2_spr_data_access(machine, vic2, 0, 1);
		vic2_spr_data_access(machine, vic2, 0, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 1, draw
	case 62:
		vic2_draw_background(vic2);
		vic2_sample_border(vic2);

		if (vic2->draw_this_line)
		{
			vic2_draw_sprites(machine, vic2);

			if (vic2->border_on_sample[0])
				for (i = 0; i < 4; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);

			if (vic2->border_on_sample[1])
				plot_box(vic2->bitmap, VIC2_X_2_EMU(4 * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[4]);

			if (vic2->border_on_sample[2])
				for (i = 5; i < 43; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);

			if (vic2->border_on_sample[3])
				plot_box(vic2->bitmap, VIC2_X_2_EMU(43 * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[43]);

			if (vic2->border_on_sample[4])
			{
				for (i = 44; i < 48; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[i]);
				for (i = 48; i < 53; i++)
					plot_box(vic2->bitmap, VIC2_X_2_EMU(i * 8), VIC2_RASTER_2_EMU(vic2->rasterline), 8, 1, vic2->border_color_sample[47]);
			}
		}

		vic2_spr_ptr_access(machine, vic2, 1);
		vic2_spr_data_access(machine, vic2, 1, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x02)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 1
	case 63:
		vic2_spr_data_access(machine, vic2, 1, 1);
		vic2_spr_data_access(machine, vic2, 1, 2);
		vic2_display_if_bad_line(vic2);

		vic2->cycle++;
		break;

	// Sprite 2
	case 64:
		vic2_spr_ptr_access(machine, vic2, 2);
		vic2_spr_data_access(machine, vic2, 2, 0);
		vic2_display_if_bad_line(vic2);

		if (vic2->spr_dma_on & 0x04)
			vic2_suspend_cpu(machine, vic2);
		else
			vic2_resume_cpu(machine, vic2);

		vic2->cycle++;
		break;

	// Sprite 2
	case 65:
		vic2_spr_data_access(machine, vic2, 2, 1);
		vic2_spr_data_access(machine, vic2, 2, 2);
		vic2_display_if_bad_line(vic2);

		if (vic2->rasterline == vic2->dy_stop)
			vic2->ud_border_on = 1;
		else
			if (SCREENON && (vic2->rasterline == vic2->dy_start))
				vic2->ud_border_on = 0;

		// Last cycle
		vic2->cycle = 1;
	}

	vic2->raster_x += 8;
	timer_set(machine, machine->device<cpu_device>("maincpu")->cycles_to_attotime(1), vic2, 0, ntsc_timer_callback);
}


/*****************************************************************************
    I/O HANDLERS
*****************************************************************************/

void vic2_set_rastering( running_device *device, int onoff )
{
	vic2_state *vic2 = get_safe_token(device);
	vic2->on = onoff;
}

int vic2e_k0_r( running_device *device )
{
	vic2_state *vic2 = get_safe_token(device);
	return VIC2E_K0_LEVEL;
}

int vic2e_k1_r( running_device *device )
{
	vic2_state *vic2 = get_safe_token(device);
	return VIC2E_K1_LEVEL;
}

int vic2e_k2_r( running_device *device )
{
	vic2_state *vic2 = get_safe_token(device);
	return VIC2E_K2_LEVEL;
}


WRITE8_DEVICE_HANDLER( vic2_port_w )
{
	vic2_state *vic2 = get_safe_token(device);
	running_machine *machine = device->machine;

	DBG_LOG(2, "vic write", ("%.2x:%.2x\n", offset, data));
	offset &= 0x3f;

	switch (offset)
	{
	case 0x01:
	case 0x03:
	case 0x05:
	case 0x07:
	case 0x09:
	case 0x0b:
	case 0x0d:
	case 0x0f:
		vic2->reg[offset] = data;		/* sprite y positions */
		break;

	case 0x00:
	case 0x02:
	case 0x04:
	case 0x06:
	case 0x08:
	case 0x0a:
	case 0x0c:
	case 0x0e:
		vic2->reg[offset] = data;		/* sprite x positions */
		break;

	case 0x10:
		vic2->reg[offset] = data;		/* sprite x positions */
		break;

	case 0x17:							/* sprite y size */
		vic2->spr_exp_y |= ~data;
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x1d:							/* sprite x size */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x1b:							/* sprite background priority */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x1c:							/* sprite multicolor mode select */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
									/* sprite colors */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x25:							/* sprite multicolor */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->spritemulti[1] = SPRITE_MULTICOLOR1;
		}
		break;

	case 0x26:							/* sprite multicolor */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->spritemulti[3] = SPRITE_MULTICOLOR2;
		}
		break;

	case 0x19:
		vic2_clear_interrupt(machine, data & 0x0f, vic2);
		break;

	case 0x1a:							/* irq mask */
		vic2->reg[offset] = data;
		vic2_set_interrupt(machine, 0, vic2);	// beamrider needs this
		break;

	case 0x11:
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			if (data & 8)
			{
				vic2->dy_start = ROW25_YSTART;
				vic2->dy_stop = ROW25_YSTOP;
			}
			else
			{
				vic2->dy_start = ROW24_YSTART;
				vic2->dy_stop = ROW24_YSTOP;
			}
		}
		break;

	case 0x12:
		if (data != vic2->reg[offset])
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x16:
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x18:
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->videoaddr = VIDEOADDR;
			vic2->chargenaddr = CHARGENADDR;
			vic2->bitmapaddr = BITMAPADDR;
		}
		break;

	case 0x21:							/* background color */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->colors[0] = BACKGROUNDCOLOR;
		}
		break;

	case 0x22:							/* background color 1 */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->colors[1] = MULTICOLOR1;
		}
		break;

	case 0x23:							/* background color 2 */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->colors[2] = MULTICOLOR2;
		}
		break;

	case 0x24:							/* background color 3 */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
			vic2->colors[3] = FOREGROUNDCOLOR;
		}
		break;

	case 0x20:							/* framecolor */
		if (vic2->reg[offset] != data)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x2f:
		if (vic2->type == VIC8564 || vic2->type == VIC8566)
		{
			DBG_LOG(2, "vic write", ("%.2x:%.2x\n", offset, data));
			vic2->reg[offset] = data;
		}
		break;

	case 0x30:
		if (vic2->type == VIC8564 || vic2->type == VIC8566)
		{
			vic2->reg[offset] = data;
		}
		break;

	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:
		vic2->reg[offset] = data;
		DBG_LOG(2, "vic write", ("%.2x:%.2x\n", offset, data));
		break;

	default:
		vic2->reg[offset] = data;
		break;
	}
}

READ8_DEVICE_HANDLER( vic2_port_r )
{
	vic2_state *vic2 = get_safe_token(device);
	running_machine *machine = device->machine;
	int val = 0;

	offset &= 0x3f;

	switch (offset)
	{
	case 0x11:
		val = (vic2->reg[offset] & ~0x80) | ((vic2->rasterline & 0x100) >> 1);
		break;

	case 0x12:
		val = vic2->rasterline & 0xff;
		break;

	case 0x16:
		val = vic2->reg[offset] | 0xc0;
		break;

	case 0x18:
		val = vic2->reg[offset] | 0x01;
		break;

	case 0x19:							/* interrupt flag register */
		/* vic2_clear_interrupt(0xf); */
		val = vic2->reg[offset] | 0x70;
		break;

	case 0x1a:
		val = vic2->reg[offset] | 0xf0;
		break;

	case 0x1e:							/* sprite to sprite collision detect */
		val = vic2->reg[offset];
		vic2->reg[offset] = 0;
		vic2_clear_interrupt(machine, 4, vic2);
		break;

	case 0x1f:							/* sprite to background collision detect */
		val = vic2->reg[offset];
		vic2->reg[offset] = 0;
		vic2_clear_interrupt(machine, 2, vic2);
		break;

	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
		val = vic2->reg[offset];
		break;

	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0a:
	case 0x0b:
	case 0x0c:
	case 0x0d:
	case 0x0e:
	case 0x0f:
	case 0x10:
	case 0x17:
	case 0x1b:
	case 0x1c:
	case 0x1d:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2a:
	case 0x2b:
	case 0x2c:
	case 0x2d:
	case 0x2e:
		val = vic2->reg[offset];
		break;

	case 0x2f:
	case 0x30:
		if (vic2->type == VIC8564 || vic2->type == VIC8566)
		{
			val = vic2->reg[offset];
			DBG_LOG(2, "vic read", ("%.2x:%.2x\n", offset, val));
		}
		else
			val = 0xff;
		break;

	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3a:
	case 0x3b:
	case 0x3c:
	case 0x3d:
	case 0x3e:
	case 0x3f:							/* not used */
		// val = vic2->reg[offset]; //
		val = 0xff;
		DBG_LOG(2, "vic read", ("%.2x:%.2x\n", offset, val));
		break;

	default:
		val = vic2->reg[offset];
	}

	if ((offset != 0x11) && (offset != 0x12))
		DBG_LOG(2, "vic read", ("%.2x:%.2x\n", offset, val));
	return val;
}

UINT32 vic2_video_update( running_device *device, bitmap_t *bitmap, const rectangle *cliprect )
{
	vic2_state *vic2 = get_safe_token(device);

	if (vic2->on)
		copybitmap(bitmap, vic2->bitmap, 0, 0, 0, 0, cliprect);
	return 0;
}

/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( vic2 )
{
	vic2_state *vic2 = get_safe_token(device);
	const vic2_interface *intf = (vic2_interface *)device->baseconfig().static_config();
	int width, height;
	int i;

	vic2->cpu = device->machine->device(intf->cpu);

	vic2->screen = device->machine->device<screen_device>(intf->screen);
	width = vic2->screen->width();
	height = vic2->screen->height();

	vic2->bitmap = auto_bitmap_alloc(device->machine, width, height, BITMAP_FORMAT_INDEXED16);

	vic2->type = intf->type;

	vic2->dma_read = intf->dma_read;
	vic2->dma_read_color = intf->dma_read_color;
	vic2->interrupt = intf->irq;

	vic2->rdy_workaround_cb = intf->rdy_cb;

	vic2->lightpen_button_cb = intf->button_cb;
	vic2->lightpen_x_cb = intf->x_cb;
	vic2->lightpen_y_cb = intf->y_cb;

	// immediately call the timer to handle the first line
	if (vic2->type == VIC6569 || vic2->type == VIC8566)
		timer_set(device->machine, downcast<cpu_device *>(vic2->cpu)->cycles_to_attotime(0), vic2, 0, pal_timer_callback);
	else
		timer_set(device->machine, downcast<cpu_device *>(vic2->cpu)->cycles_to_attotime(0), vic2, 0, ntsc_timer_callback);

	for (i = 0; i < 256; i++)
	{
		vic2->expandx[i] = 0;
		if (i & 1)
			vic2->expandx[i] |= 3;
		if (i & 2)
			vic2->expandx[i] |= 0xc;
		if (i & 4)
			vic2->expandx[i] |= 0x30;
		if (i & 8)
			vic2->expandx[i] |= 0xc0;
		if (i & 0x10)
			vic2->expandx[i] |= 0x300;
		if (i & 0x20)
			vic2->expandx[i] |= 0xc00;
		if (i & 0x40)
			vic2->expandx[i] |= 0x3000;
		if (i & 0x80)
			vic2->expandx[i] |= 0xc000;
	}

	for (i = 0; i < 256; i++)
	{
		vic2->expandx_multi[i] = 0;
		if (i & 1)
			vic2->expandx_multi[i] |= 5;
		if (i & 2)
			vic2->expandx_multi[i] |= 0xa;
		if (i & 4)
			vic2->expandx_multi[i] |= 0x50;
		if (i & 8)
			vic2->expandx_multi[i] |= 0xa0;
		if (i & 0x10)
			vic2->expandx_multi[i] |= 0x500;
		if (i & 0x20)
			vic2->expandx_multi[i] |= 0xa00;
		if (i & 0x40)
			vic2->expandx_multi[i] |= 0x5000;
		if (i & 0x80)
			vic2->expandx_multi[i] |= 0xa000;
	}

	state_save_register_device_item_array(device, 0, vic2->reg);

	state_save_register_device_item(device, 0, vic2->on);

	state_save_register_device_item_bitmap(device, 0, vic2->bitmap);

	state_save_register_device_item(device, 0, vic2->chargenaddr);
	state_save_register_device_item(device, 0, vic2->videoaddr);
	state_save_register_device_item(device, 0, vic2->bitmapaddr);

	state_save_register_device_item_array(device, 0, vic2->colors);
	state_save_register_device_item_array(device, 0, vic2->spritemulti);

	state_save_register_device_item(device, 0, vic2->rasterline);
	state_save_register_device_item(device, 0, vic2->cycles_counter);
	state_save_register_device_item(device, 0, vic2->cycle);
	state_save_register_device_item(device, 0, vic2->raster_x);
	state_save_register_device_item(device, 0, vic2->graphic_x);

	state_save_register_device_item(device, 0, vic2->dy_start);
	state_save_register_device_item(device, 0, vic2->dy_stop);

	state_save_register_device_item(device, 0, vic2->draw_this_line);
	state_save_register_device_item(device, 0, vic2->is_bad_line);
	state_save_register_device_item(device, 0, vic2->bad_lines_enabled);
	state_save_register_device_item(device, 0, vic2->display_state);
	state_save_register_device_item(device, 0, vic2->char_data);
	state_save_register_device_item(device, 0, vic2->gfx_data);
	state_save_register_device_item(device, 0, vic2->color_data);
	state_save_register_device_item(device, 0, vic2->last_char_data);
	state_save_register_device_item_array(device, 0, vic2->matrix_line);
	state_save_register_device_item_array(device, 0, vic2->color_line);
	state_save_register_device_item(device, 0, vic2->vblanking);
	state_save_register_device_item(device, 0, vic2->ml_index);
	state_save_register_device_item(device, 0, vic2->rc);
	state_save_register_device_item(device, 0, vic2->vc);
	state_save_register_device_item(device, 0, vic2->vc_base);
	state_save_register_device_item(device, 0, vic2->ref_cnt);

	state_save_register_device_item_array(device, 0, vic2->spr_coll_buf);
	state_save_register_device_item_array(device, 0, vic2->fore_coll_buf);
	state_save_register_device_item(device, 0, vic2->spr_exp_y);
	state_save_register_device_item(device, 0, vic2->spr_dma_on);
	state_save_register_device_item(device, 0, vic2->spr_draw);
	state_save_register_device_item(device, 0, vic2->spr_disp_on);
	state_save_register_device_item_array(device, 0, vic2->spr_ptr);
	state_save_register_device_item_array(device, 0, vic2->mc_base);
	state_save_register_device_item_array(device, 0, vic2->mc);

	for (i = 0; i < 8; i++)
	{
		state_save_register_device_item_array(device, i, vic2->spr_data[i]);
		state_save_register_device_item_array(device, i, vic2->spr_draw_data[i]);
	}

	state_save_register_device_item(device, 0, vic2->border_on);
	state_save_register_device_item(device, 0, vic2->ud_border_on);
	state_save_register_device_item_array(device, 0, vic2->border_on_sample);
	state_save_register_device_item_array(device, 0, vic2->border_color_sample);

	state_save_register_device_item(device, 0, vic2->first_ba_cycle);
	state_save_register_device_item(device, 0, vic2->cpu_suspended);
}

static DEVICE_RESET( vic2 )
{
	vic2_state *vic2 = get_safe_token(device);
	int i, j;

	memset(vic2->reg, 0, ARRAY_LENGTH(vic2->reg));

	for (i = 0; i < ARRAY_LENGTH(vic2->mc); i++)
		vic2->mc[i] = 63;

	// from 0 to 311 (0 first, PAL) or from 0 to 261 (? first, NTSC 6567R56A) or from 0 to 262 (? first, NTSC 6567R8)
	vic2->rasterline = 0; // VIC2_LINES - 1;

	vic2->cycles_counter = -1;
	vic2->cycle = 63;

	vic2->on = 1;

	vic2->dy_start = ROW24_YSTART;
	vic2->dy_stop = ROW24_YSTOP;

	vic2->draw_this_line = 0;
	vic2->is_bad_line = 0;
	vic2->bad_lines_enabled = 0;
	vic2->display_state = 0;
	vic2->char_data = 0;
	vic2->gfx_data = 0;
	vic2->color_data = 0;
	vic2->last_char_data = 0;
	vic2->vblanking = 0;
	vic2->ml_index = 0;
	vic2->rc = 0;
	vic2->vc = 0;
	vic2->vc_base = 0;
	vic2->ref_cnt = 0;

	vic2->spr_exp_y = 0;
	vic2->spr_dma_on = 0;
	vic2->spr_draw = 0;
	vic2->spr_disp_on = 0;


	vic2->border_on = 0;
	vic2->ud_border_on = 0;

	vic2->first_ba_cycle = 0;
	vic2->cpu_suspended = 0;

	memset(vic2->matrix_line, 0, ARRAY_LENGTH(vic2->matrix_line));
	memset(vic2->color_line, 0, ARRAY_LENGTH(vic2->color_line));

	memset(vic2->spr_coll_buf, 0, ARRAY_LENGTH(vic2->spr_coll_buf));
	memset(vic2->fore_coll_buf, 0, ARRAY_LENGTH(vic2->fore_coll_buf));
	memset(vic2->border_on_sample, 0, ARRAY_LENGTH(vic2->border_on_sample));
	memset(vic2->border_color_sample, 0, ARRAY_LENGTH(vic2->border_color_sample));

	for (i = 0; i < 8; i++)
	{
		vic2->spr_ptr[i] = 0;
		vic2->mc_base[i] = 0;
		vic2->mc[i] = 0;
		for (j = 0; j < 4; j++)
		{
			vic2->spr_draw_data[i][j] = 0;
			vic2->spr_data[i][j] = 0;
		}
	}

}


/*-------------------------------------------------
    device definition
-------------------------------------------------*/

static const char DEVTEMPLATE_SOURCE[] = __FILE__;

#define DEVTEMPLATE_ID(p,s)				p##vic2##s
#define DEVTEMPLATE_FEATURES			DT_HAS_START | DT_HAS_RESET
#define DEVTEMPLATE_NAME				"6567 / 6569 VIC II"
#define DEVTEMPLATE_FAMILY				"6567 / 6569 VIC II"
#include "devtempl.h"

DEFINE_LEGACY_DEVICE(VIC2, vic2);
