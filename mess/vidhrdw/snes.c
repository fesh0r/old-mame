/***************************************************************************

  snes.c

  Video file to handle emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original code by Lee Hammerton (aka Savoury Snax)

  Some notes on the snes video hardware:

  Object Attribute Memory(OAM) is made up of 128 blocks of 32 bits, followed
  by 128 blocks of 2 bits. The format for each block is:
  -First Block----------------------------------------------------------------
  | x pos  | y pos  |char no.| v flip | h flip |priority|palette |char no msb|
  +--------+--------+--------+--------+--------+--------+--------+-----------+
  | 8 bits | 8 bits | 8 bits | 1 bit  | 1 bit  | 2 bits | 3 bits | 1 bit     |
  -Second Block---------------------------------------------------------------
  | size  | x pos msb |
  +-------+-----------+
  | 1 bit | 1 bit     |
  ---------------------

  Video RAM contains information for character data and screen maps.
  Screen maps are made up of 32 x 32 blocks of 16 bits each.
  The format for each block is:
  ----------------------------------------------
  | v flip | x flip |priority|palette |char no.|
  +--------+--------+--------+--------+--------+
  | 1 bit  | 1 bit  | 1 bit  | 3 bits |10 bits |
  ----------------------------------------------
  Mode 7 is stored differently. Character data and screen map are interleaved.
  There are two formats:
  -Normal-----------------  -EXTBG-----------------------------
  | char data | char no. |  | priority | char data | char no. |
  +-----------+----------+  +----------+-----------+----------+
  | 8 bits    | 8 bits   |  | 1 bit    | 7 bits    | 8 bits   |
  ------------------------  -----------------------------------

  The screen layers are drawn with the following priorities:
  (highest to lowest)

  Mainscreens
          (BG3:1 - If BG3 priority set)
           OBJ:3
           BG1:1
           BG2:1
           OBJ:2
           BG1:0
           BG2:0
           OBJ:1
          (BG3:1 - if BG3 priority not set)
           BG4:1
           OBJ:0
           BG3:0
           BG4:0
           Background Color
  Subscreens
          (BG3:1 - If BG3 priority set)
           OBJ:3
           BG1:1
           BG2:1
           OBJ:2
           BG1:0
           BG2:0
           OBJ:1
          (BG3:1 - if BG3 priority not set)
           BG4:1
           OBJ:0
           BG3:0
           BG4:0

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"

#define MAINSCREEN      0
#define SUBSCREEN       1
#define SNES_BLEND_NONE 0
#define SNES_BLEND_ADD  1
#define SNES_BLEND_SUB  2
#define SNES_CLIP_ALL   0
#define SNES_CLIP_IN    1
#define SNES_CLIP_OUT   2
#define SNES_CLIP_ALL2  3

#ifdef MAME_DEBUG
struct DEBUGOPTS
{
	UINT8 input_count;
	UINT8 bg_disabled[6];
	UINT8 draw_subscreen;
};
static struct DEBUGOPTS debug_options  = {5, {0,0,0,0,0,0}, 0};
/*                                    red   green  blue    purple  yellow cyan    grey    white */
/*static UINT16 dbg_mode_colours[8] = { 0x1f, 0x3e0, 0x7c00, 0x7c1f, 0x3ff, 0x7fe0, 0x4210, 0x7fff };*/
/*static UINT16 dbg_pty_colours[12] = { 0x1084, 0x18c6, 0x2529, 0x2d6b, 0x39ce, 0x4210, 0x4e73, 0x56b5, 0x6318, 0x6b5a, 0x77bd, 0x7fff };*/
#endif

UINT8 ppu_obj_size[2];				/* Object sizes */

/* Lookup tables */
static const UINT8  table_bgd_pty[4][2] = { {7,10}, {6,9}, {1,4}, {0,3} };
static const UINT8  table_obj_pty[4]    = { 2, 5, 8, 11 };
static const UINT16 table_obj_offset[8][8] =
{
	{ (0*32),   (0*32)+32,   (0*32)+64,   (0*32)+96,   (0*32)+128,   (0*32)+160,   (0*32)+192,   (0*32)+224 },
	{ (16*32),  (16*32)+32,  (16*32)+64,  (16*32)+96,  (16*32)+128,  (16*32)+160,  (16*32)+192,  (16*32)+224 },
	{ (32*32),  (32*32)+32,  (32*32)+64,  (32*32)+96,  (32*32)+128,  (32*32)+160,  (32*32)+192,  (32*32)+224 },
	{ (48*32),  (48*32)+32,  (48*32)+64,  (48*32)+96,  (48*32)+128,  (48*32)+160,  (48*32)+192,  (48*32)+224 },
	{ (64*32),  (64*32)+32,  (64*32)+64,  (64*32)+96,  (64*32)+128,  (64*32)+160,  (64*32)+192,  (64*32)+224 },
	{ (80*32),  (80*32)+32,  (80*32)+64,  (80*32)+96,  (80*32)+128,  (80*32)+160,  (80*32)+192,  (80*32)+224 },
	{ (96*32),  (96*32)+32,  (96*32)+64,  (96*32)+96,  (96*32)+128,  (96*32)+160,  (96*32)+192,  (96*32)+224 },
	{ (112*32), (112*32)+32, (112*32)+64, (112*32)+96, (112*32)+128, (112*32)+160, (112*32)+192, (112*32)+224 }
};
static const UINT16 table_hscroll[4][4] = { {0,0,0,0}, {0,0x800,0,0x800}, {0,0,0,0}, {0,0x800,0,0x800} };
static const UINT16 table_vscroll[4][4] = { {0,0,0,0}, {0,0,0,0}, {0,0x800,0,0x800}, {0,0x1000,0,0x1000} };

struct SCANLINE
{
	UINT16 buffer[SNES_SCR_WIDTH + 8];
	UINT8  zbuf[SNES_SCR_WIDTH + 8];
};

static struct SCANLINE scanlines[2];
struct SNES_PPU_STRUCT snes_ppu;

/* Routine for additive/subtractive blending between the main and sub screens */
INLINE void snes_draw_blend( UINT16 offset, UINT16 *colour, UINT8 mode, UINT8 clip )
{
	if( (clip == SNES_CLIP_ALL || clip == SNES_CLIP_ALL2) ||
		(clip == SNES_CLIP_IN  && snes_ppu.clipmasks[5][offset]) ||
		(clip == SNES_CLIP_OUT && !snes_ppu.clipmasks[5][offset]) )
	{
		UINT16 r, g, b;
		if( mode == SNES_BLEND_ADD )
		{
			if( snes_ram[CGWSEL] & 0x2 ) /* Subscreen*/
			{
				r = (*colour & 0x1f) + (scanlines[SUBSCREEN].buffer[offset] & 0x1f);
				g = ((*colour & 0x3e0) >> 5) + ((scanlines[SUBSCREEN].buffer[offset] & 0x3e0) >> 5);
				b = ((*colour & 0x7c00) >> 10) + ((scanlines[SUBSCREEN].buffer[offset] & 0x7c00) >> 10);
				if( (snes_ram[CGADSUB] & 0x40) && (scanlines[SUBSCREEN].zbuf[offset]) ) /* FIXME: We shouldn't half for the back colour */
				{
					r >>= 1;
					g >>= 1;
					b >>= 1;
				}
			}
			else /* Fixed colour */
			{
				r = (*colour & 0x1f) + (Machine->remapped_colortable[FIXED_COLOUR] & 0x1f);
				g = ((*colour & 0x3e0) >> 5) + ((Machine->remapped_colortable[FIXED_COLOUR] & 0x3e0) >> 5);
				b = ((*colour & 0x7c00) >> 10) + ((Machine->remapped_colortable[FIXED_COLOUR] & 0x7c00) >> 10);
				if( snes_ram[CGADSUB] & 0x40 ) /* FIXME: We shouldn't half for the back colour */
				{
					r >>= 1;
					g >>= 1;
					b >>= 1;
				}
			}
			if( r > 0x1f ) r = 0x1f;
			if( g > 0x1f ) g = 0x1f;
			if( b > 0x1f ) b = 0x1f;
			*colour = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
		}
		else if( mode == SNES_BLEND_SUB )
		{
			if( snes_ram[CGWSEL] & 0x2 ) /* Subscreen */
			{
				r = (*colour & 0x1f) - (scanlines[SUBSCREEN].buffer[offset] & 0x1f);
				g = ((*colour & 0x3e0) >> 5) - ((scanlines[SUBSCREEN].buffer[offset] & 0x3e0) >> 5);
				b = ((*colour & 0x7c00) >> 10) - ((scanlines[SUBSCREEN].buffer[offset] & 0x7c00) >> 10);
				if( r > 0x1f ) r = 0;
				if( g > 0x1f ) g = 0;
				if( b > 0x1f ) b = 0;
				if( (snes_ram[CGADSUB] & 0x40) && (scanlines[SUBSCREEN].zbuf[offset]) ) /* FIXME: We shouldn't half for the back colour */
				{
					r >>= 1;
					g >>= 1;
					b >>= 1;
				}
			}
			else /* Fixed colour */
			{
				r = (*colour & 0x1f) - (Machine->remapped_colortable[FIXED_COLOUR] & 0x1f);
				g = ((*colour & 0x3e0) >> 5) - ((Machine->remapped_colortable[FIXED_COLOUR] & 0x3e0) >> 5);
				b = ((*colour & 0x7c00) >> 10) - ((Machine->remapped_colortable[FIXED_COLOUR] & 0x7c00) >> 10);
				if( r > 0x1f ) r = 0;
				if( g > 0x1f ) g = 0;
				if( b > 0x1f ) b = 0;
				if( snes_ram[CGADSUB] & 0x40 ) /* FIXME: We shouldn't half for the back colour */
				{
					r >>= 1;
					g >>= 1;
					b >>= 1;
				}
			}
			*colour = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
		}
	}
}

VIDEO_UPDATE( snes )
{
}

/*****************************************
 * snes_draw_tile_2()
 *
 * Draw tiles with 2 bit planes(4 colors)
 *****************************************/
INLINE void snes_draw_tile_2( UINT8 screen, UINT8 layer, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT16 pal )
{
	UINT8 ii, mask;
	UINT8 plane[2];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];

	if( flip )
		mask = 0x1;
	else
		mask = 0x80;

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0);
			mask <<= 1;
		}
		else
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0);
			mask >>= 1;
		}

		/* Clip to windows */
		if( (screen == MAINSCREEN && (snes_ram[TMW] & (0x1 << layer))) || (screen == SUBSCREEN && (snes_ram[TSW] & (0x1 << layer))))
			colour &= snes_ppu.clipmasks[layer][x+ii];

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( (scanlines[screen].zbuf[x+ii] <= priority) && ((x + ii) >= 0)  )
			{
				c = Machine->remapped_colortable[pal + colour];
				if( screen == MAINSCREEN )	/* Only blend main screens */
					snes_draw_blend( x+ii, &c, snes_ppu.layer[layer].blend, (snes_ram[CGWSEL] & 0x30) >> 4 );
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*****************************************
 * snes_draw_tile_4()
 *
 * Draw tiles with 4 bit planes(16 colors)
 *****************************************/
INLINE void snes_draw_tile_4( UINT8 screen, UINT8 layer, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT16 pal )
{
	UINT8 ii, mask;
	UINT8 plane[4];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];

	if( flip )
		mask = 0x1;
	else
		mask = 0x80;

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0) |
					 (plane[2] & mask ? 4 : 0) | (plane[3] & mask ? 8 : 0);
			mask <<= 1;
		}
		else
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0) |
					 (plane[2] & mask ? 4 : 0) | (plane[3] & mask ? 8 : 0);
			mask >>= 1;
		}

		/* Clip to windows */
		if( (screen == MAINSCREEN && (snes_ram[TMW] & (0x1 << layer))) || (screen == SUBSCREEN && (snes_ram[TSW] & (0x1 << layer))))
			colour &= snes_ppu.clipmasks[layer][x+ii];

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( (scanlines[screen].zbuf[x+ii] <= priority) && ((x + ii) >= 0) )
			{
				c = Machine->remapped_colortable[pal + colour];
				if( screen == MAINSCREEN )	/* Only blend main screens */
					snes_draw_blend( x+ii, &c, snes_ppu.layer[layer].blend, (snes_ram[CGWSEL] & 0x30) >> 4 );
#ifdef MAME_DEBUG
/*				switch( priority )
				{
					case 2: c = Machine->pens[0x1f]; break;
					case 5: c = Machine->pens[0x3e0]; break;
					case 8: c = Machine->pens[0x7c00]; break;
					case 11: c = Machine->pens[0x3ff]; break;
					case 20: c = Machine->pens[0x7fff]; break;
				}*/
/*				c = Machine->pens[dbg_pty_colours[priority]]; */
#endif /* MAME_DEBUG */
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*****************************************
 * snes_draw_tile_8()
 *
 * Draw tiles with 8 bit planes(256 colors)
 *****************************************/
INLINE void snes_draw_tile_8( UINT8 screen, UINT8 layer, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip )
{
	UINT8 ii, mask;
	UINT8 plane[8];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];
	plane[4] = snes_vram[tileaddr + 32];
	plane[5] = snes_vram[tileaddr + 33];
	plane[6] = snes_vram[tileaddr + 48];
	plane[7] = snes_vram[tileaddr + 49];

	if( flip )
		mask = 0x1;
	else
		mask = 0x80;

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & mask ? 1 : 0)  | (plane[1] & mask ? 2 : 0)  |
					 (plane[2] & mask ? 4 : 0)  | (plane[3] & mask ? 8 : 0)  |
					 (plane[4] & mask ? 16 : 0) | (plane[5] & mask ? 32 : 0) |
					 (plane[6] & mask ? 64 : 0) | (plane[7] & mask ? 128 : 0);
			mask <<= 1;
		}
		else
		{
			colour = (plane[0] & mask ? 1 : 0)  | (plane[1] & mask ? 2 : 0)  |
					 (plane[2] & mask ? 4 : 0)  | (plane[3] & mask ? 8 : 0)  |
					 (plane[4] & mask ? 16 : 0) | (plane[5] & mask ? 32 : 0) |
					 (plane[6] & mask ? 64 : 0) | (plane[7] & mask ? 128 : 0);
			mask >>= 1;
		}

		/* Clip to windows */
		if( (screen == MAINSCREEN && (snes_ram[TMW] & (0x1 << layer))) || (screen == SUBSCREEN && (snes_ram[TSW] & (0x1 << layer))))
			colour &= snes_ppu.clipmasks[layer][x+ii];

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( (scanlines[screen].zbuf[x+ii] <= priority) && ((x + ii) >= 0) )
			{
				c = Machine->remapped_colortable[colour];
				if( screen == MAINSCREEN )	/* Only blend main screens */
					snes_draw_blend( x+ii, &c, snes_ppu.layer[layer].blend, (snes_ram[CGWSEL] & 0x30) >> 4 );
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*****************************************
 * snes_draw_tile_object()
 *
 * Draw tiles with 4 bit planes(16 colors)
 * The same as snes_draw_tile_4() except
 * that it takes a blend parameter.
 *****************************************/
INLINE void snes_draw_tile_object( UINT8 screen, UINT16 tileaddr, INT16 x, UINT8 priority, UINT8 flip, UINT16 pal, UINT8 blend )
{
	UINT8 ii, mask;
	UINT8 plane[4];
	UINT16 c;

	plane[0] = snes_vram[tileaddr];
	plane[1] = snes_vram[tileaddr + 1];
	plane[2] = snes_vram[tileaddr + 16];
	plane[3] = snes_vram[tileaddr + 17];

	if( flip )
		mask = 0x1;
	else
		mask = 0x80;

	for( ii = 0; ii < 8; ii++ )
	{
		register UINT8 colour;
		if( flip )
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0) |
					 (plane[2] & mask ? 4 : 0) | (plane[3] & mask ? 8 : 0);
			mask <<= 1;
		}
		else
		{
			colour = (plane[0] & mask ? 1 : 0) | (plane[1] & mask ? 2 : 0) |
					 (plane[2] & mask ? 4 : 0) | (plane[3] & mask ? 8 : 0);
			mask >>= 1;
		}

		/* Clip to windows */
		if( (screen == MAINSCREEN && (snes_ram[TMW] & 0x10)) || (screen == SUBSCREEN && (snes_ram[TSW] & 0x10)))
			colour &= snes_ppu.clipmasks[4][x+ii];

		/* Only draw if we have a colour (0 == transparent) */
		if( colour )
		{
			if( (scanlines[screen].zbuf[x+ii] <= priority) && ((x + ii) >= 0) )
			{
				c = Machine->remapped_colortable[pal + colour];
				if( blend && screen == MAINSCREEN )	/* Only blend main screens */
					snes_draw_blend( x+ii, &c, snes_ppu.layer[4].blend, (snes_ram[CGWSEL] & 0x30) >> 4 );
#ifdef MAME_DEBUG
/*				switch( priority )
				{
					case 2: c = Machine->pens[0x1f]; break;
					case 5: c = Machine->pens[0x3e0]; break;
					case 8: c = Machine->pens[0x7c00]; break;
					case 11: c = Machine->pens[0x3ff]; break;
					case 20: c = Machine->pens[0x7fff]; break;
				}*/
/*				c = Machine->pens[dbg_pty_colours[priority]];*/
#endif /* MAME_DEBUG */
				scanlines[screen].buffer[x+ii] = c;
				scanlines[screen].zbuf[x+ii] = priority;
			}
		}
	}
}

/*********************************************
 * snes_update_line_2()
 *
 * Update an entire line of 2 bit plane tiles.
 *********************************************/
static void snes_update_line_2( UINT8 screen, UINT8 layer, UINT16 curline, UINT8 bg3_pty )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll, vtilescroll;
	UINT8 vshift, hshift, tile_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Handle Mosaic effects */
	if( snes_ram[MOSAIC] & (1 << layer) )
		curline -= (curline % ((snes_ram[MOSAIC] >> 4) + 1));

	/* Find the size of the tiles (8x8 or 16x16) */
	tile_size = snes_ppu.layer[layer].tile_size;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> (3 + tile_size);
	vshift = bg_voffset[layer] & ((8 << tile_size) - 1);
	hscroll = (bg_hoffset[layer] & 0x3ff) >> (3 + tile_size);
	hshift = bg_hoffset[layer] & ((8 << tile_size) - 1);

	/* Find vertical scroll amount */
	vtilescroll = vscroll + (curline >> (3 + tile_size));
	/* figure out which line to draw */
	line = (curline % (8 << tile_size)) + vshift;
	if( line > ((8 << tile_size) - 1) )	/* scrolled into the next tile */
	{
		vtilescroll++;	/* pretend we scrolled by 1 tile line */
		line -= (8 << tile_size);
	}
	if( vtilescroll >= 128 )
		vtilescroll -= 128;

	/* Jump to base map address */
	tilemap = snes_ppu.layer[layer].map;
	/* Offset vertically */
	tilemap += table_vscroll[snes_ppu.layer[layer].map_size][vtilescroll >> 5];
	/* Scroll vertically */
	tilemap += (vtilescroll & 0x1f) << 6;
	/* Remember this position */
	basevmap = tilemap;
	/* Offset horizontally */
	tilemap += table_hscroll[snes_ppu.layer[layer].map_size][hscroll >> 5];
	/* Scroll horizontally */
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < (66 >> tile_size); ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[snes_ppu.layer[layer].map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* 8 palettes of 4 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		if( line > 7 )
		{
			tile += 32;
			tile_line = line - 8;
		}
		else
		{
			tile_line = line;
		}
		if( vflip )
			tile_line = -tile_line + 7;
		tile_line <<= 1;

		/* Special case for bg3 */
		if( layer == 2 && bg3_pty && (snes_vram[tilemap + ii + 1] & 0x20) )
			priority = table_obj_pty[3] + 1;		/* We want to have the highest priority here */

		snes_draw_tile_2( screen, layer, snes_ppu.layer[layer].data + (tile << 4) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift, priority, hflip, pal );
		if( tile_size )
			snes_draw_tile_2( screen, layer, snes_ppu.layer[layer].data + (tile << 4) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift + 8, priority, hflip, pal );
	}
}

/*********************************************
 * snes_update_line_4()
 *
 * Update an entire line of 4 bit plane tiles.
 *********************************************/
static void snes_update_line_4( UINT8 screen, UINT8 layer, UINT16 curline )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll, vtilescroll;
	UINT8 vshift, hshift, tile_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Handle Mosaic effects */
	if( snes_ram[MOSAIC] & (1 << layer) )
		curline -= (curline % ((snes_ram[MOSAIC] >> 4) + 1));

	/* Find the size of the tiles (8x8 or 16x16) */
	tile_size = snes_ppu.layer[layer].tile_size;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> (3 + tile_size);
	vshift = bg_voffset[layer] & ((8 << tile_size) - 1);
	hscroll = (bg_hoffset[layer] & 0x3ff) >> (3 + tile_size);
	hshift = bg_hoffset[layer] & ((8 << tile_size) - 1);

	/* Find vertical scroll amount */
	vtilescroll = vscroll + (curline >> (3 + tile_size));
	/* figure out which line to draw */
	line = (curline % (8 << tile_size)) + vshift;
	if( line > ((8 << tile_size) - 1) )	/* scrolled into the next tile */
	{
		vtilescroll++;	/* pretend we scrolled by 1 tile line */
		line -= (8 << tile_size);
	}
	if( vtilescroll >= 128 )
		vtilescroll -= 128;

	/* Jump to base map address */
	tilemap = snes_ppu.layer[layer].map;
	/* Offset vertically */
	tilemap += table_vscroll[snes_ppu.layer[layer].map_size][vtilescroll >> 5];
	/* Scroll vertically */
	tilemap += (vtilescroll & 0x1f) << 6;
	/* Remember this position */
	basevmap = tilemap;
	/* Offset horizontally */
	tilemap += table_hscroll[snes_ppu.layer[layer].map_size][hscroll >> 5];
	/* Scroll horizontally */
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < (66 >> tile_size); ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[snes_ppu.layer[layer].map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = snes_vram[tilemap + ii + 1] & 0x80;
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];		/* is this even right??? */
		pal = (snes_vram[tilemap + ii + 1] & 0x1c) << 2;	/* 8 palettes of 16 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		if( line > 7 )
		{
			tile += 16;
			tile_line = line - 8;
		}
		else
		{
			tile_line = line;
		}
		if( vflip )
			tile_line = -tile_line + 7;
		tile_line <<= 1;

		snes_draw_tile_4( screen, layer, snes_ppu.layer[layer].data + (tile << 5) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift, priority, hflip, pal );
		if( tile_size )
			snes_draw_tile_4( screen, layer, snes_ppu.layer[layer].data + ((tile+1) << 5) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift + 8, priority, hflip, pal );
	}
}

/*********************************************
 * snes_update_line_8()
 *
 * Update an entire line of 8 bit plane tiles.
 *********************************************/
static void snes_update_line_8( UINT8 screen, UINT8 layer, UINT16 curline )
{
	UINT32 tilemap, tile;
	UINT16 ii, vflip, hflip, pal;
	INT8 line, tile_line;
	UINT8 priority;
	/* scrolling */
	UINT32 basevmap;
	UINT16 vscroll, hscroll, vtilescroll;
	UINT8 vshift, hshift, tile_size;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[layer] )
	{
		return;
	}
#endif

	/* Handle Mosaic effects */
	if( snes_ram[MOSAIC] & (1 << layer) )
		curline -= (curline % ((snes_ram[MOSAIC] >> 4) + 1));

	/* Find the size of the tiles (8x8 or 16x16) */
	tile_size = snes_ppu.layer[layer].tile_size;
	/* Find scroll info */
	vscroll = (bg_voffset[layer] & 0x3ff) >> (3 + tile_size);
	vshift = bg_voffset[layer] & ((8 << tile_size) - 1);
	hscroll = (bg_hoffset[layer] & 0x3ff) >> (3 + tile_size);
	hshift = bg_hoffset[layer] & ((8 << tile_size) - 1);

	/* Find vertical scroll amount */
	vtilescroll = vscroll + (curline >> (3 + tile_size));
	/* figure out which line to draw */
	line = (curline % (8 << tile_size)) + vshift;
	if( line > ((8 << tile_size) - 1) )	/* scrolled into the next tile */
	{
		vtilescroll++;	/* pretend we scrolled by 1 tile line */
		line -= (8 << tile_size);
	}
	if( vtilescroll >= 128 )
		vtilescroll -= 128;

	/* Jump to base map address */
	tilemap = snes_ppu.layer[layer].map;
	/* Offset vertically */
	tilemap += table_vscroll[snes_ppu.layer[layer].map_size][vtilescroll >> 5];
	/* Scroll vertically */
	tilemap += (vtilescroll & 0x1f) << 6;
	/* Remember this position */
	basevmap = tilemap;
	/* Offset horizontally */
	tilemap += table_hscroll[snes_ppu.layer[layer].map_size][hscroll >> 5];
	/* Scroll horizontally */
	tilemap += (hscroll & 0x1f) << 1;

	for( ii = 0; ii < (66 >> tile_size); ii += 2 )
	{
		/* FIXME: A quick hack to stop memory overwrite.... */
		if( tilemap >= 0x20000 )
			continue;

		/* Have we scrolled into the next map? */
		if( hscroll && ((ii >> 1) >= 32 - (hscroll & 0x1f)) )
		{
			tilemap = basevmap + table_hscroll[snes_ppu.layer[layer].map_size][(hscroll >> 5) + 1];
			tilemap -= ii;
			hscroll = 0;	/* Make sure we don't do this again */
		}
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		priority = table_bgd_pty[layer][(snes_vram[tilemap + ii + 1] & 0x20) >> 5];
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* what does this do for 8 bit screen? */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		if( line > 7 )
		{
			tile += 8;
			tile_line = line - 8;
		}
		else
		{
			tile_line = line;
		}
		if( vflip )
			tile_line = -tile_line + 7;
		tile_line <<= 1;

		snes_draw_tile_8( screen, layer, snes_ppu.layer[layer].data + (tile << 6) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift, priority, hflip );
		if( tile_size )
			snes_draw_tile_8( screen, layer, snes_ppu.layer[layer].data + (tile << 6) + tile_line, ((ii >> 1) * (8 << tile_size)) - hshift + 8, priority, hflip );
	}
}

/*********************************************
 * snes_update_line_mode7()
 *
 * Update an entire line of mode7 tiles.
 *********************************************/
static void snes_update_line_mode7( UINT8 screen, UINT16 curline )
{
	UINT32 tiled;
	INT16 ma,mb,mc,md;
	INT16 xc, yc, tx, ty, sx, sy, hs, vs, xpos, xdir;
	register UINT8 colour = 0;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[0] )
	{
		return;
	}
#endif

	ma = mode7_data[0];
	mb = -mode7_data[1];
	mc = -mode7_data[2];
	md = mode7_data[3];
	xc = mode7_data[4];
	yc = mode7_data[5];
	hs = bg_hoffset[0];
	vs = bg_voffset[0];

	/* Sign extend */
	xc <<= 3;
	xc >>= 3;
	yc <<= 3;
	yc >>= 3;
	hs <<= 3;
	hs >>= 3;
	vs <<= 3;
	vs >>= 3;

	/* Vertical flip */
	if( snes_ram[M7SEL] & 0x2 )
		sy = 255 - curline;
	else
		sy = curline;

	/* Horizontal flip */
	if( snes_ram[M7SEL] & 0x1 )
	{
		xpos = 255;
		xdir = -1;
	}
	else
	{
		xpos = 0;
		xdir = 1;
	}

	/* Let's do some mode7 drawing huh? */
	for( sx = 0; sx < 256; sx++, xpos += xdir )
	{
		tx = (((ma * ((sx + hs) - xc)) + (mc * ((sy + vs) - yc))) >> 8) + xc;
		ty = (((mb * ((sx + hs) - xc)) + (md * ((sy + vs) - yc))) >> 8) + yc;
		switch( snes_ram[M7SEL] & 0xc0 )
		{
			case 0x00:	/* Repeat if outside screen area */
				tx &= 0x3ff;
				ty &= 0x3ff;
				tiled = snes_vram[((tx >> 3) * 2) + ((ty >> 3) * 128 * 2)] << 7;
				colour = snes_vram[tiled + ((tx & 0x7) * 2) + ((ty & 0x7) * 16) + 1];
				break;
			case 0x80:	/* Single colour backdrop screen if outside screen area */
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
				{
					tiled = snes_vram[((tx >> 3) * 2) + ((ty >> 3) * 128 * 2)] << 7;
					colour = snes_vram[tiled + ((tx & 0x7) * 2) + ((ty & 0x7) * 16) + 1];
				}
				else
				{
					colour = 0;
				}
				break;
			case 0xC0:	/* Character 0x00 repeat if outside screen area */
				if( (tx & 0x7fff) < 1024 && (ty & 0x7fff) < 1024 )
				{
					tiled = snes_vram[(((tx & 0x3ff) >> 3) * 2) + (((ty & 0x3ff) >> 3) * 128 * 2)] << 7;
					colour = snes_vram[tiled + ((tx & 0x7) * 2) + ((ty & 0x7) * 16) + 1];
				}
				else
				{
					/* FIXME: should we be rotating this? */
					colour = snes_vram[((tx & 0x7) * 2) + ((ty & 0x7) * 16) + 1];
				}
				break;
		}

		colour &= snes_ppu.clipmasks[0][xpos];

		/* Draw pixel if appropriate */
		if( scanlines[screen].zbuf[xpos] < table_bgd_pty[0][0] && colour > 0 )
		{
			UINT16 clr = Machine->remapped_colortable[colour];
			if( screen == MAINSCREEN )	/* Only blend main screens */
				snes_draw_blend( xpos, &clr, snes_ppu.layer[0].blend, (snes_ram[CGWSEL] & 0x30) >> 4 );		/* FIXME: Need to support clip mode */
			scanlines[screen].buffer[xpos] = clr;
			scanlines[screen].zbuf[xpos] = table_bgd_pty[0][0];
		}
	}
}

/*********************************************
 * snes_update_objects()
 *
 * Update an entire line of sprites.
 * FIXME: We need to support high priority
 *********************************************/
static void snes_update_objects( UINT8 screen, UINT16 curline )
{
	INT8 xs, ys;
	UINT8 line;
	UINT16 oam_extra, extra;
	INT16 oam;
	UINT8 range_over = 0, time_over = 0;
	UINT8 size, vflip, hflip, priority, pal, blend;
	UINT16 tile;
	INT16 i, x, y;
	UINT8 *oamram = (UINT8 *)snes_oam;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[4] )
	{
		return;
	}
#endif

	oam = 0x1ff;
	oam_extra = oam + 0x20;
	extra = 0;
	for( i = 128; i > 0; i-- )
	{
		if( (i % 4) == 0 )
			extra = oamram[oam_extra--];

		vflip = (oamram[oam] & 0x80) >> 7;
		hflip = (oamram[oam] & 0x40) >> 6;
		priority = table_obj_pty[(oamram[oam] & 0x30) >> 4];
		pal = 128 + ((oamram[oam] & 0xE) << 3);
		tile = (oamram[oam--] & 0x1) << 8;
		tile |= oamram[oam--];
		y = oamram[oam--] + 1;	/* We seem to need to add one here.... */
		x = oamram[oam--];
		size = (extra & 0x80) >> 7;
		extra <<= 1;
		x |= ((extra & 0x80) << 1);
		extra <<= 1;

		/* Adjust if past maximum position */
		if( y > 239 )
			y -= 256;
		if( x > 255 )
			x -= 512;

		/* Only sprites using palettes 4-7 can be transparent */
		blend = (pal < 192) ? 0 : 1;

		/* Draw sprite if it intersects the current line */
		if( curline >= y && curline < (y + (ppu_obj_size[size] << 3)) )
		{
			ys = (curline - y) >> 3;
			line = (curline - y) % 8;
			if( vflip )
			{
				ys = ppu_obj_size[size] - ys - 1;
				line = (-1 * line) + 7;
			}
			line <<= 1;
			tile <<= 5;
			if( hflip )
			{
				UINT8 count = 0;
				for( xs = (ppu_obj_size[size] - 1); xs >= 0; xs-- )
				{
					if( (x + (count << 3) < SNES_SCR_WIDTH + 8) )
						snes_draw_tile_object( screen, snes_ppu.layer[4].data + tile + table_obj_offset[ys][xs] + line, x + (count++ << 3), priority, hflip, pal, blend );
					time_over++;	/* Increase time_over. Should we stop drawing if exceeded 34 tiles? */
				}
			}
			else
			{
				for( xs = 0; xs < ppu_obj_size[size]; xs++ )
				{
					if( (x + (xs << 3) < SNES_SCR_WIDTH + 8) )
						snes_draw_tile_object( screen, snes_ppu.layer[4].data + tile + table_obj_offset[ys][xs] + line, x + (xs << 3), priority, hflip, pal, blend );
					time_over++;	/* Increase time_over. Should we stop drawing if exceeded 34 tiles? */
				}
			}

			/* Increase range_over. 
			 * Stop drawing if exceeded 32 objects and
			 * enforcing that limit is enabled */
			range_over++;
			if( range_over == 32 && (readinputport( 16 ) & 0x10) )
			{
				/* Set the flag in STAT77 register */
				snes_ram[STAT77] |= 0x40;
				/* FIXME: This stops the SNESTest rom from drawing the object
				 * test properly.  Maybe we shouldn't stop drawing? */
				/* return; */
			}
		}
	}

	if( time_over >= 34 )
	{
		/* Set the flag in STAT77 register */
		snes_ram[STAT77] |= 0x80;
	}
}

static void snes_update_backplane(void)
{
	UINT16 ii;

#ifdef MAME_DEBUG
	if( debug_options.bg_disabled[5] )
	{
		return;
	}
#endif

	if( snes_ram[CGADSUB] & 0x20 )
	{
		for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
		{
			snes_draw_blend( ii, &scanlines[MAINSCREEN].buffer[ii], (snes_ram[CGADSUB] & 0x80)?SNES_BLEND_SUB:SNES_BLEND_ADD, (snes_ram[CGWSEL] & 0x30) >> 4 );
		}
	}
}

/* An example of how windows work:
 * Win1: ...#####......
 * Win2: ......#####...
 *             IN                 OUT
 * OR:   ...########...     ###........###
 * AND:  ......##......     ######..######
 * XOR:  ...###..###...     ###...##...###
 * XNOR: ###...##...###     ...###..###...
 */

static void snes_update_windowmasks(void)
{
	UINT16 ii;
	INT8 w1, w2;

	snes_ppu.update_windows = 0;		/* reset the flag */

	for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
	{
		/* update bg 1 */
		snes_ppu.clipmasks[0][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[W12SEL] & 0x2 )
		{
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			if( snes_ram[W12SEL] & 0x1 )
				w1 = !w1;
		}
		if( snes_ram[W12SEL] & 0x8 )
		{
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			if( snes_ram[W12SEL] & 0x4 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WBGLOG] & 0x3 )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[0][ii] = w1 | w2 ? 0x00 : 0xff;
					break;
				case 0x1:	/* AND */
					snes_ppu.clipmasks[0][ii] = w1 & w2 ? 0x00 : 0xff;
					break;
				case 0x2:	/* XOR */
					snes_ppu.clipmasks[0][ii] = w1 ^ w2 ? 0x00 : 0xff;
					break;
				case 0x3:	/* XNOR */
					snes_ppu.clipmasks[0][ii] = !(w1 ^ w2) ? 0x00 : 0xff;
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[0][ii] = w1 ? 0x00 : 0xff;
		else if( w2 >= 0 )
			snes_ppu.clipmasks[0][ii] = w2 ? 0x00 : 0xff;

		/* update bg 2 */
		snes_ppu.clipmasks[1][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[W12SEL] & 0x20 )
		{
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			if( snes_ram[W12SEL] & 0x10 )
				w1 = !w1;
		}
		if( snes_ram[W12SEL] & 0x80 )
		{
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			if( snes_ram[W12SEL] & 0x40 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WBGLOG] & 0xc )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[1][ii] = w1 | w2 ? 0x00 : 0xff;
					break;
				case 0x4:	/* AND */
					snes_ppu.clipmasks[1][ii] = w1 & w2 ? 0x00 : 0xff;
					break;
				case 0x2:	/* XOR */
					snes_ppu.clipmasks[1][ii] = w1 ^ w2 ? 0x00 : 0xff;
					break;
				case 0xc:	/* XNOR */
					snes_ppu.clipmasks[1][ii] = !(w1 ^ w2) ? 0x00 : 0xff;
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[1][ii] = w1 ? 0x00 : 0xff;
		else if( w2 >= 0 )
			snes_ppu.clipmasks[1][ii] = w2 ? 0x00 : 0xff;

		/* update bg 3 */
		snes_ppu.clipmasks[2][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[W34SEL] & 0x2 )
		{
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			if( snes_ram[W34SEL] & 0x1 )
				w1 = !w1;
		}
		if( snes_ram[W34SEL] & 0x8 )
		{
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			if( snes_ram[W34SEL] & 0x4 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WBGLOG] & 0x30 )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[2][ii] = w1 | w2 ? 0x00 : 0xff;
					break;
				case 0x10:	/* AND */
					snes_ppu.clipmasks[2][ii] = w1 & w2 ? 0x00 : 0xff;
					break;
				case 0x20:	/* XOR */
					snes_ppu.clipmasks[2][ii] = w1 ^ w2 ? 0x00 : 0xff;
					break;
				case 0x30:	/* XNOR */
					snes_ppu.clipmasks[2][ii] = !(w1 ^ w2) ? 0x00 : 0xff;
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[2][ii] = w1 ? 0x00 : 0xff;
		else if( w2 >= 0 )
			snes_ppu.clipmasks[2][ii] = w2 ? 0x00 : 0xff;

		/* update bg 4 */
		snes_ppu.clipmasks[3][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[W34SEL] & 0x20 )
		{
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			if( snes_ram[W34SEL] & 0x10 )
				w1 = !w1;
		}
		if( snes_ram[W34SEL] & 0x80 )
		{
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			if( snes_ram[W34SEL] & 0x40 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WBGLOG] & 0xc0 )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[3][ii] = w1 | w2 ? 0x00 : 0xff;
					break;
				case 0x40:	/* AND */
					snes_ppu.clipmasks[3][ii] = w1 & w2 ? 0x00 : 0xff;
					break;
				case 0x80:	/* XOR */
					snes_ppu.clipmasks[3][ii] = w1 ^ w2 ? 0x00 : 0xff;
					break;
				case 0xc0:	/* XNOR */
					snes_ppu.clipmasks[3][ii] = !(w1 ^ w2) ? 0x00 : 0xff;
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[3][ii] = w1 ? 0x00 : 0xff;
		else if( w2 >= 0 )
			snes_ppu.clipmasks[3][ii] = w2 ? 0x00 : 0xff;

		/* update objects */
		snes_ppu.clipmasks[4][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[WOBJSEL] & 0x2 )
		{
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			if( snes_ram[WOBJSEL] & 0x1 )
				w1 = !w1;
		}
		if( snes_ram[WOBJSEL] & 0x8 )
		{
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			if( snes_ram[WOBJSEL] & 0x4 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WOBJLOG] & 0x3 )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[4][ii] = w1 | w2 ? 0x00 : 0xff;
					break;
				case 0x1:	/* AND */
					snes_ppu.clipmasks[4][ii] = w1 & w2 ? 0x00 : 0xff;
					break;
				case 0x2:	/* XOR */
					snes_ppu.clipmasks[4][ii] = w1 ^ w2 ? 0x00 : 0xff;
					break;
				case 0x3:	/* XNOR */
					snes_ppu.clipmasks[4][ii] = !(w1 ^ w2) ? 0x00 : 0xff;
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[4][ii] = w1 ? 0x00 : 0xff;
		else if( w2 >= 0 )
			snes_ppu.clipmasks[4][ii] = w2 ? 0x00 : 0xff;

		/* update colour window */
		snes_ppu.clipmasks[5][ii] = 0xff;
		w1 = w2 = -1;
		if( snes_ram[WOBJSEL] & 0x20 )
		{
			/* Default to mask area inside */
			if( (ii < snes_ram[WH0]) || (ii > snes_ram[WH1]) )
				w1 = 0;
			else
				w1 = 1;
			/* If mask area is outside then swap */
			if( snes_ram[WOBJSEL] & 0x10 )
				w1 = !w1;
		}
		if( snes_ram[WOBJSEL] & 0x80 )
		{
			/* Default to mask area inside */
			if( (ii < snes_ram[WH2]) || (ii > snes_ram[WH3]) )
				w2 = 0;
			else
				w2 = 1;
			/* If mask area is outside then swap */
			if( snes_ram[WOBJSEL] & 0x40 )
				w2 = !w2;
		}
		if( w1 >= 0 && w2 >= 0 )
		{
			switch( snes_ram[WOBJLOG] & 0xc )
			{
				case 0x0:	/* OR */
					snes_ppu.clipmasks[5][ii] = w1 | w2 ? 0xff : 0x00;
/*					snes_ppu.clipmasks[5][ii] = w1 | w2 ? 0x00 : 0xff;*/
					break;
				case 0x4:	/* AND */
					snes_ppu.clipmasks[5][ii] = w1 & w2 ? 0xff : 0x00;
/*					snes_ppu.clipmasks[5][ii] = w1 & w2 ? 0x00 : 0xff;*/
					break;
				case 0x8:	/* XOR */
					snes_ppu.clipmasks[5][ii] = w1 ^ w2 ? 0xff : 0x00;
/*					snes_ppu.clipmasks[5][ii] = w1 ^ w2 ? 0x00 : 0xff;*/
					break;
				case 0xc:	/* XNOR */
					snes_ppu.clipmasks[5][ii] = !(w1 ^ w2) ? 0xff : 0x00;
/*					snes_ppu.clipmasks[5][ii] = !(w1 ^ w2) ? 0x00 : 0xff;*/
					break;
			}
		}
		else if( w1 >= 0 )
			snes_ppu.clipmasks[5][ii] = w1 ? 0xff : 0x00;
/*			snes_ppu.clipmasks[5][ii] = w1 ? 0x00 : 0xff;*/
		else if( w2 >= 0 )
			snes_ppu.clipmasks[5][ii] = w2 ? 0xff : 0x00;
/*			snes_ppu.clipmasks[5][ii] = w2 ? 0x00 : 0xff;*/
	}
}

void snes_update_palette(void)
{
	UINT8 r, g, b, fade;
	UINT16 ii;
	UINT32 col;

	/* Reset the flag */
	snes_ppu.update_palette = 0;

	/* Modify the palette to fade out the colours */
	fade = (snes_ram[INIDISP] & 0xf) + 1;
	for( ii = 0; ii <= 256; ii++ )
	{
		col = Machine->pens[snes_cgram[ii] & 0x7fff];
		r = ((col & 0x1f) * fade) >> 4;
		g = (((col & 0x3e0) >> 5) * fade) >> 4;
		b = (((col & 0x7c00) >> 10) * fade) >> 4;
		Machine->remapped_colortable[ii] = ((r & 0x1f) | ((g & 0x1f) << 5) | ((b & 0x1f) << 10));
	}
}

#ifdef SNES_DBG_VIDHRDW
/*********************************************
 * snes_dbg_draw_maps()
 *
 * Debug to display maps
 * Doesn't handle vflip.
 *********************************************/
void snes_dbg_draw_maps( struct mame_bitmap *bitmap, UINT32 tilemap, UINT8 bpl, UINT16 curline, UINT8 layer )
{
	UINT32 tile, addr = tilemap;
	UINT16 ii, vflip, hflip, pal;
	INT8 line;
	char str[50];
	UINT8 clipmask[SNES_SCR_WIDTH + 8];

	memset( clipmask, 0xff, SNES_SCR_WIDTH + 8 );

	tilemap += (curline >> 3) * 64;
	for( ii = 0; ii < 64; ii += 2 )
	{
		vflip = (snes_vram[tilemap + ii + 1] & 0x80);
		hflip = snes_vram[tilemap + ii + 1] & 0x40;
		pal = (snes_vram[tilemap + ii + 1] & 0x1c);		/* 8 palettes of 4 colours */
		tile = (snes_vram[tilemap + ii + 1] & 0x3) << 8;
		tile |= snes_vram[tilemap + ii];
		line = curline % 8;
		if( vflip )
			line = -line + 7;

		if( tile != 0 )
		{
			switch( bpl )
			{
				case 1:
					snes_draw_tile_2( MAINSCREEN, layer, snes_ppu.layer[layer].data + (tile << 4) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip, pal );
					break;
				case 2:
					pal <<= 2;
					snes_draw_tile_4( MAINSCREEN, layer, snes_ppu.layer[layer].data + (tile << 5) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip, pal );
					break;
				case 4:
					snes_draw_tile_8( MAINSCREEN, layer, snes_ppu.layer[layer].data + (tile << 6) + ((curline % 8) * 2), (ii >> 1) * 8, 255, hflip );
					break;
			}
		}
	}

	sprintf( str, "%d : %8X  ", layer, addr );
	ui_text( bitmap, str, 300, 227 );
}

/*********************************************
 * snes_dbg_draw_all_tiles()
 *
 * Debug to display everything in VRAM.
 *********************************************/
static void snes_dbg_draw_all_tiles( struct mame_bitmap *bitmap, UINT32 tileaddr, UINT8 bpl, UINT16 pal )
{
	UINT16 ii, jj, kk;
	UINT32 addr = tileaddr;
	char str[50];
	UINT8 clipmask[SNES_SCR_WIDTH + 8];

	memset( clipmask, 0xff, SNES_SCR_WIDTH + 8 );
	for( jj = 0; jj < 32; jj++ )
	{
		addr = tileaddr + (jj * bpl * 16 * 32);
		for( kk = 0; kk < 8; kk++ )
		{
			/* Clear buffers */
			memset( scanlines[MAINSCREEN].buffer, 0, SNES_SCR_WIDTH * 2 );
			memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
			for( ii = 0; ii < 32; ii++ )
			{
				switch( bpl )
				{
					case 1:
						snes_draw_tile_2( MAINSCREEN, 0, addr, ii * 8, 255, 0, pal );
						break;
					case 2:
						snes_draw_tile_4( MAINSCREEN, 0, addr, ii * 8, 255, 0, pal );
						break;
					case 4:
						snes_draw_tile_8( MAINSCREEN, 0, addr, ii * 8, 255, 0 );
						break;
				}
				addr += (bpl * 16);
			}
			draw_scanline16( bitmap, 0, jj * 8 + kk, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, 200 );
			addr -= (32 * (bpl * 16)) - 2;
		}
	}

	sprintf( str, "  %8X  ", tileaddr );
	ui_text( bitmap, str, 300, 227 );
}
#endif /* SNES_DBG_VIDHRDW */

/*********************************************
 * snes_refresh_scanline()
 *
 * Redraw the current line.
 *********************************************/
void snes_refresh_scanline( UINT16 curline )
{
	UINT16 ii;
	struct mame_bitmap *bitmap = Machine->scrbitmap;

#ifdef SNES_DBG_VIDHRDW
	/* Check if the user has enabled or disabled stuff */
	if( curline == 0 )
	{
		UINT16 y = 0;
		char t[100];
		static char WINLOGIC[4] = { '|', '&', '^', '!' };

		if( !debug_options.input_count-- )
		{
			UINT8 toggles = readinputport( 10 );
			if( toggles & 0x1 )
				debug_options.bg_disabled[0] = !debug_options.bg_disabled[0];
			if( toggles & 0x2 )
				debug_options.bg_disabled[1] = !debug_options.bg_disabled[1];
			if( toggles & 0x4 )
				debug_options.bg_disabled[2] = !debug_options.bg_disabled[2];
			if( toggles & 0x8 )
				debug_options.bg_disabled[3] = !debug_options.bg_disabled[3];
			if( toggles & 0x10 )
				debug_options.bg_disabled[4] = !debug_options.bg_disabled[4];
			if( toggles & 0x20 )
				debug_options.draw_subscreen = !debug_options.draw_subscreen;
			debug_options.input_count = 5;
		}
		/* Display some debug info on the screen */
		sprintf( t, "%s%s%s%s%s%s", debug_options.bg_disabled[0]?" ":"1", debug_options.bg_disabled[1]?" ":"2", debug_options.bg_disabled[2]?" ":"3", debug_options.bg_disabled[3]?" ":"4", debug_options.bg_disabled[4]?" ":"O", debug_options.draw_subscreen?"S":"M" );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "1 %s%s%s%s%s%c%s%s  %4X h%3d v%3d",
				(snes_ram[TM] & 0x1)?"M":" ",
				(snes_ram[TS] & 0x1)?"S":" ",
				(snes_ram[CGADSUB] & 0x1)?"B":" ",
				(snes_ram[TMW] & 0x1)?"m":" ",
				(snes_ram[TSW] & 0x1)?"s":" ",
				WINLOGIC[(snes_ram[WBGLOG] & 0x3)],
				(snes_ram[W12SEL] & 0x2)?((snes_ram[W12SEL] & 0x1)?"o":"i"):" ",
				(snes_ram[W12SEL] & 0x8)?((snes_ram[W12SEL] & 0x4)?"o":"i"):" ",
				(snes_ram[BG1SC] & 0xfc) << 9,
				(bg_hoffset[0] & 0x3ff) >> 3,
				(bg_voffset[0] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "2 %s%s%s%s%s%c%s%s  %4X h%3d v%3d",
				(snes_ram[TM] & 0x2)?"M":" ",
				(snes_ram[TS] & 0x2)?"S":" ",
				(snes_ram[CGADSUB] & 0x2)?"B":" ",
				(snes_ram[TMW] & 0x2)?"m":" ",
				(snes_ram[TSW] & 0x2)?"s":" ",
				WINLOGIC[(snes_ram[WBGLOG] & 0xc) >> 2],
				(snes_ram[W12SEL] & 0x20)?((snes_ram[W12SEL] & 0x10)?"o":"i"):" ",
				(snes_ram[W12SEL] & 0x80)?((snes_ram[W12SEL] & 0x40)?"o":"i"):" ",
				(snes_ram[BG2SC] & 0xfc) << 9,
				(bg_hoffset[1] & 0x3ff) >> 3,
				(bg_voffset[1] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "3 %s%s%s%s%s%c%s%s%s %4X h%3d v%3d",
				(snes_ram[TM] & 0x4)?"M":" ",
				(snes_ram[TS] & 0x4)?"S":" ",
				(snes_ram[CGADSUB] & 0x4)?"B":" ",
				(snes_ram[TMW] & 0x4)?"m":" ",
				(snes_ram[TSW] & 0x4)?"s":" ",
				WINLOGIC[(snes_ram[WBGLOG] & 0x30)>>4],
				(snes_ram[W34SEL] & 0x2)?((snes_ram[W34SEL] & 0x1)?"o":"i"):" ",
				(snes_ram[W34SEL] & 0x8)?((snes_ram[W34SEL] & 0x4)?"o":"i"):" ",
				(snes_ram[BGMODE] & 0x8)?"P":" ",
				(snes_ram[BG3SC] & 0xfc) << 9,
				(bg_hoffset[2] & 0x3ff) >> 3,
				(bg_voffset[2] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "4 %s%s%s%s%s%c%s%s  %4X h%3d v%3d",
				(snes_ram[TM] & 0x8)?"M":" ",
				(snes_ram[TS] & 0x8)?"S":" ",
				(snes_ram[CGADSUB] & 0x8)?"B":" ",
				(snes_ram[TMW] & 0x8)?"m":" ",
				(snes_ram[TSW] & 0x8)?"s":" ",
				WINLOGIC[(snes_ram[WBGLOG] & 0xc0)>>6],
				(snes_ram[W34SEL] & 0x20)?((snes_ram[W34SEL] & 0x10)?"o":"i"):" ",
				(snes_ram[W34SEL] & 0x80)?((snes_ram[W34SEL] & 0x40)?"o":"i"):" ",
				(snes_ram[BG4SC] & 0xfc) << 9,
				(bg_hoffset[3] & 0x3ff) >> 3,
				(bg_voffset[3] & 0x3ff) >> 3 );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "O %s%s%s%s%s%c%s%s  %4X",
				(snes_ram[TM] & 0x10)?"M":" ",
				(snes_ram[TS] & 0x10)?"S":" ",
				(snes_ram[CGADSUB] & 0x10)?"B":" ",
				(snes_ram[TMW] & 0x10)?"m":" ",
				(snes_ram[TSW] & 0x10)?"s":" ",
				WINLOGIC[(snes_ram[WOBJLOG] & 0x3)],
				(snes_ram[WOBJSEL] & 0x2)?((snes_ram[WOBJSEL] & 0x1)?"o":"i"):" ",
				(snes_ram[WOBJSEL] & 0x8)?((snes_ram[WOBJSEL] & 0x4)?"o":"i"):" ",
				(((snes_ram[OBSEL] & 0x3) * 0x2000) + (((snes_ram[OBSEL] & 0x18)>>3) * 0x1000)) * 2 );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "B   %s  %c%s%s",
				(snes_ram[CGADSUB] & 0x20)?"B":" ",
				WINLOGIC[(snes_ram[WOBJLOG] & 0xc)>>2],
				(snes_ram[WOBJSEL] & 0x20)?((snes_ram[WOBJSEL] & 0x10)?"o":"i"):" ",
				(snes_ram[WOBJSEL] & 0x80)?((snes_ram[WOBJSEL] & 0x40)?"o":"i"):" " );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "Flags: %s%s%s%s", (snes_ram[CGWSEL] & 0x2)?"S":"F", (snes_ram[CGADSUB] & 0x80)?"-":"+", (snes_ram[CGADSUB] & 0x40)?" 50%":"100%",(snes_ram[CGWSEL] & 0x1)?"D":"P" );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
		sprintf( t, "SetINI: %s %s %s %s", (snes_ram[SETINI] & 0x1)?" I":"NI", (snes_ram[SETINI] & 0x2)?"PL":"RP", (snes_ram[SETINI] & 0x4)?"239":"224",(snes_ram[SETINI] & 0x8)?"512":"256" );
		ui_text( Machine->scrbitmap, t, 300, y++ * 9 );
	}
	/* Just for testing, draw as many tiles as possible */
	{
		UINT8 adjust = readinputport( 1 );
		UINT8 dip = readinputport( 9 );
		UINT8 inp = readinputport( 11 );
		UINT8 dt = 1 << ((dip & 0x3) - 1);
		UINT8 dm = 1 << (((dip & 0xc) >> 2) - 1);
		if( dt )
		{
			static INT16 pal = 0;
			static UINT32 addr = 0;
			if( curline == 0 )
			{
				if( adjust & 0x1 ) addr += (dt * 16);
				if( adjust & 0x2 ) addr -= (dt * 16);
				if( adjust & 0x4 ) addr += (dt * 16 * 32);
				if( adjust & 0x8 ) addr -= (dt * 16 * 32);
				if( inp & 0x1 ) pal -= 1;
				if( inp & 0x2 ) pal += 1;
				if( pal < 0 ) pal = 0;
				if( pal > 8 ) pal = 8;
				for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
				{
					scanlines[MAINSCREEN].buffer[ii] = 0;
				}
				snes_dbg_draw_all_tiles( bitmap, addr, dt, pal * 16 );
			}
			return;
		}
		if( dm )
		{
			static UINT32 tmaddr = 0;
			static INT8 tmbg = 0;
			if( curline == 0 )
			{
				if( adjust & 0x1 ) tmaddr += 2;
				if( adjust & 0x2 ) tmaddr -= 2;
				if( adjust & 0x4 ) tmaddr += 64;
				if( adjust & 0x8 ) tmaddr -= 64;
				if( inp & 0x1 ) tmbg -= 1;
				if( inp & 0x2 ) tmbg += 1;
				if( tmbg < 0 ) tmbg = 0;
				if( tmbg > 3 ) tmbg = 3;
			}
			/* Clear zbuffer */
			memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
			/* Draw back colour */
			for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
				scanlines[MAINSCREEN].buffer[ii] = Machine->pens[0];
			snes_dbg_draw_maps( bitmap, tmaddr, dm, curline, tmbg );
			draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, 200 );
			return;
		}
	}
#endif /* SNES_DBG_VIDHRDW */

	profiler_mark(PROFILER_VIDEO);

	if( snes_ram[INIDISP] & 0x80 ) /* screen is forced blank */
	{
		struct rectangle r = Machine->visible_area;
		r.min_y = r.max_y = curline;
		fillbitmap(bitmap, Machine->pens[0], &r);
	}
	else
	{
		/* Blending stuff */
		snes_ppu.layer[0].blend = SNES_BLEND_NONE;
		snes_ppu.layer[1].blend = SNES_BLEND_NONE;
		snes_ppu.layer[2].blend = SNES_BLEND_NONE;
		snes_ppu.layer[3].blend = SNES_BLEND_NONE;
		snes_ppu.layer[4].blend = SNES_BLEND_NONE;
		if( snes_ram[CGADSUB] & 0x80 )
		{
			if( snes_ram[CGADSUB] & 0x1 )
				snes_ppu.layer[0].blend = SNES_BLEND_SUB;
			if( snes_ram[CGADSUB] & 0x2 )
				snes_ppu.layer[1].blend = SNES_BLEND_SUB;
			if( snes_ram[CGADSUB] & 0x4 )
				snes_ppu.layer[2].blend = SNES_BLEND_SUB;
			if( snes_ram[CGADSUB] & 0x8 )
				snes_ppu.layer[3].blend = SNES_BLEND_SUB;
			if( snes_ram[CGADSUB] & 0x10 )
				snes_ppu.layer[4].blend = SNES_BLEND_SUB;
		}
		else
		{
			if( snes_ram[CGADSUB] & 0x1 )
				snes_ppu.layer[0].blend = SNES_BLEND_ADD;
			if( snes_ram[CGADSUB] & 0x2 )
				snes_ppu.layer[1].blend = SNES_BLEND_ADD;
			if( snes_ram[CGADSUB] & 0x4 )
				snes_ppu.layer[2].blend = SNES_BLEND_ADD;
			if( snes_ram[CGADSUB] & 0x8 )
				snes_ppu.layer[3].blend = SNES_BLEND_ADD;
			if( snes_ram[CGADSUB] & 0x10 )
				snes_ppu.layer[4].blend = SNES_BLEND_ADD;
		}

		/* Update clip window masks if necessary */
		if( snes_ppu.update_windows )
			snes_update_windowmasks();
		/* Update the palette if necessary */
		if( snes_ppu.update_palette )
			snes_update_palette();

		/* Clear zbuffers */
		memset( scanlines[MAINSCREEN].zbuf, 0, SNES_SCR_WIDTH );
		memset( scanlines[SUBSCREEN].zbuf, 0, SNES_SCR_WIDTH );

		/* Clear subscreen and draw back colour */
		for( ii = 0; ii < SNES_SCR_WIDTH; ii++ )
		{
			/* Not sure if this is correct behaviour, but a few games seem to
			 * require it. (SMW, Zelda etc) */
			scanlines[SUBSCREEN].buffer[ii] = Machine->remapped_colortable[FIXED_COLOUR];
			/* Draw back colour */
			scanlines[MAINSCREEN].buffer[ii] = Machine->remapped_colortable[0];
		}

		/* Draw the subscreen sprites */
		if( snes_ram[TS] & 0x10 )
			snes_update_objects( SUBSCREEN, curline );

		/* Draw backgrounds */
		switch( snes_ram[BGMODE] & 0x7 )
		{
			case 0:			/* Mode 0 - 4 layers - 4 / 4 / 4 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_2( SUBSCREEN, 0, curline, 0 );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				if( snes_ram[TS] & 0x4 ) snes_update_line_2( SUBSCREEN, 2, curline, 0 );
				if( snes_ram[TS] & 0x8 ) snes_update_line_2( SUBSCREEN, 3, curline, 0 );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_2( MAINSCREEN, 0, curline, 0 );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				if( snes_ram[TM] & 0x4 ) snes_update_line_2( MAINSCREEN, 2, curline, 0 );
				if( snes_ram[TM] & 0x8 ) snes_update_line_2( MAINSCREEN, 3, curline, 0 );
				break;
			case 1:			/* Mode 1 - 3 layers - 16 / 16 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				if( snes_ram[TS] & 0x4 )
				{
					if( snes_ram[BGMODE] & 0x8 )	/* BG3 can have highest priority if bit 3 if BGMODE is set */
						snes_update_line_2( SUBSCREEN, 2, curline, 1 );
					else
						snes_update_line_2( SUBSCREEN, 2, curline, 0 );
				}
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				if( snes_ram[TM] & 0x4 )
				{
					if( snes_ram[BGMODE] & 0x8 )	/* BG3 can have highest priority if bit 3 if BGMODE is set */
						snes_update_line_2( MAINSCREEN, 2, curline, 1 );
					else
						snes_update_line_2( MAINSCREEN, 2, curline, 0 );
				}
				break;
			case 2:			/* Mode 2 - 2 layers - 16 / 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				break;
			case 3:			/* Mode 3 - 2 layers - 256 / 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_8( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_4( SUBSCREEN, 1, curline );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_8( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_4( MAINSCREEN, 1, curline );
				break;
			case 4:			/* Mode 4 - 2 layers - 256 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_8( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_8( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				break;
			case 5:			/* Mode 5 - 2 layers - 16 / 4 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				if( snes_ram[TS] & 0x2 ) snes_update_line_2( SUBSCREEN, 1, curline, 0 );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				if( snes_ram[TM] & 0x2 ) snes_update_line_2( MAINSCREEN, 1, curline, 0 );
				break;
			case 6:			/* Mode 6 - 1 layer - 16 colours */
				/* Draw Subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_4( SUBSCREEN, 0, curline );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_4( MAINSCREEN, 0, curline );
				break;
			case 7:			/* Mode 7 - 1 layer - 256 colours - matrix math stuff */
				/* Draw the subscreen */
				if( snes_ram[TS] & 0x1 ) snes_update_line_mode7( SUBSCREEN, curline );
				/* Draw the back plane */
				snes_update_backplane();
				/* Draw Mainscreen */
				if( snes_ram[TM] & 0x1 ) snes_update_line_mode7( MAINSCREEN, curline );
				break;
		}

		/* Draw the mainscreen sprites */
		if( snes_ram[TM] & 0x10 )
			snes_update_objects( MAINSCREEN, curline );

#ifdef SNES_DBG_VIDHRDW
		{
			/* Draw some useful information about the back/fixed colours and current bg mode etc. */
			plot_pixel( bitmap, 274, curline, Machine->pens[dbg_mode_colours[(snes_ram[CGWSEL] & 0xc0) >> 6]] );
			plot_pixel( bitmap, 276, curline, Machine->pens[dbg_mode_colours[(snes_ram[CGWSEL] & 0x30) >> 4]] );
			plot_pixel( bitmap, 278, curline, Machine->pens[dbg_mode_colours[snes_ram[BGMODE] & 0x7]] );
			plot_pixel( bitmap, 288, curline, Machine->pens[32767] );
			plot_pixel( bitmap, 298, curline, Machine->pens[32767] );
			for( ii = 0; ii < 5; ii++ )
			{
				plot_pixel( bitmap, 281 + ii, curline, Machine->remapped_colortable[0] );
				plot_pixel( bitmap, 291 + ii, curline, Machine->remapped_colortable[FIXED_COLOUR] );
			}
			/* Draw window positions */
			scanlines[MAINSCREEN].buffer[snes_ram[WH0]] = Machine->pens[dbg_mode_colours[0]];
			scanlines[MAINSCREEN].buffer[snes_ram[WH1]] = Machine->pens[dbg_mode_colours[0]];
			scanlines[MAINSCREEN].buffer[snes_ram[WH2]] = Machine->pens[dbg_mode_colours[2]];
			scanlines[MAINSCREEN].buffer[snes_ram[WH3]] = Machine->pens[dbg_mode_colours[2]];
		}
#endif /* SNES_DBG_VIDHRDW */
#ifdef MAME_DEBUG
		/* Toggle drawing of subscreen or mainscreen */
		if( debug_options.draw_subscreen )
			draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanlines[SUBSCREEN].buffer, Machine->pens, -1 );
		else
#endif /* MAME_DEBUG */
		/* Phew! Draw the line to screen */
		draw_scanline16( bitmap, 0, curline, SNES_SCR_WIDTH, scanlines[MAINSCREEN].buffer, Machine->pens, -1 );
	}

	profiler_mark(PROFILER_END);
}
