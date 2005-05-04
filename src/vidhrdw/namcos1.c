/*******************************************************************

Namco System 1 Video Hardware

*******************************************************************/

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"


/*
  video ram map
  0000-1fff : scroll playfield (0) : 64*64*2
  2000-3fff : scroll playfield (1) : 64*64*2
  4000-5fff : scroll playfield (2) : 64*64*2
  6000-6fff : scroll playfield (3) : 64*32*2
  7000-700f : ?
  7010-77ef : fixed playfield (4)  : 36*28*2
  77f0-77ff : ?
  7800-780f : ?
  7810-7fef : fixed playfield (5)  : 36*28*2
  7ff0-7fff : ?
*/
static data8_t *namcos1_videoram;

/*
  paletteram map (s1ram  0x0000-0x7fff)
  0000-17ff : palette page0 : sprite
  2000-37ff : palette page1 : playfield
  4000-57ff : palette page2 : playfield (shadow)
  6000-77ff : palette page3 : work RAM

  x800-x807 : CUS116 registers (visibility window)

  so there is just 3x0x2000 RAM, plus the CUS116 internal registers.
*/
static data8_t *namcos1_paletteram;
static data8_t namcos1_cus116[0x10];

/*
  spriteram map (s1ram 0x10000-0x10fff)
  0000-07ff : work ram
  0800-0fef : sprite ram    : 0x10 * 127
  0ff0-0fff : display control registers

  1000-1fff : playfield control registers
*/
static data8_t *namcos1_spriteram;

static data8_t namcos1_playfield_control[0x20];

static struct tilemap *tilemap[6];
static UINT8 *tilemap_maskdata;




/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

INLINE void bg_get_info(int tile_index,data8_t *info_vram)
{
	int code;

	tile_index <<= 1;
	code = info_vram[tile_index + 1] + ((info_vram[tile_index] & 0x3f) << 8);
	SET_TILE_INFO(0,code,0,0);
	tile_info.mask_data = &tilemap_maskdata[code << 3];
}

INLINE void fg_get_info(int tile_index,data8_t *info_vram)
{
	int code;

	tile_index <<= 1;
	code = info_vram[tile_index + 1] + ((info_vram[tile_index] & 0x3f) << 8);
	SET_TILE_INFO(0,code,0,0);
	tile_info.mask_data = &tilemap_maskdata[code << 3];
}

static void bg_get_info0(int tile_index) { bg_get_info(tile_index,&namcos1_videoram[0x0000]); }
static void bg_get_info1(int tile_index) { bg_get_info(tile_index,&namcos1_videoram[0x2000]); }
static void bg_get_info2(int tile_index) { bg_get_info(tile_index,&namcos1_videoram[0x4000]); }
static void bg_get_info3(int tile_index) { bg_get_info(tile_index,&namcos1_videoram[0x6000]); }
static void fg_get_info4(int tile_index) { fg_get_info(tile_index,&namcos1_videoram[0x7010]); }
static void fg_get_info5(int tile_index) { fg_get_info(tile_index,&namcos1_videoram[0x7810]); }



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( namcos1 )
{
	int i;

	tilemap_maskdata = (UINT8 *)memory_region(REGION_GFX1);

	/* allocate videoram */
	namcos1_videoram = auto_malloc(0x8000);
	namcos1_paletteram = auto_malloc(0x6000);
	namcos1_spriteram = auto_malloc(0x1000);

	/* initialize playfields */
	tilemap[0] = tilemap_create(bg_get_info0,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[1] = tilemap_create(bg_get_info1,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[2] = tilemap_create(bg_get_info2,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,64);
	tilemap[3] = tilemap_create(bg_get_info3,tilemap_scan_rows,TILEMAP_BITMASK,8,8,64,32);
	tilemap[4] = tilemap_create(fg_get_info4,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);
	tilemap[5] = tilemap_create(fg_get_info5,tilemap_scan_rows,TILEMAP_BITMASK,8,8,36,28);

	if (!tilemap[0] || !tilemap[1] || !tilemap[2] || !tilemap[3] || !tilemap[4] || !tilemap[5]
			|| !namcos1_videoram || !namcos1_paletteram)
		return 1;

	tilemap_set_scrolldx(tilemap[4],73,512-73);
	tilemap_set_scrolldx(tilemap[5],73,512-73);
	tilemap_set_scrolldy(tilemap[4],0x10,0x110);
	tilemap_set_scrolldy(tilemap[5],0x10,0x110);

	/* register videoram to the save state system (post-allocation) */
	state_save_register_UINT8("video", 0, "vram", namcos1_videoram, 0x8000);
	state_save_register_UINT8("video", 0, "paletteram", namcos1_paletteram, 0x6000);
	state_save_register_UINT8("video", 0, "cus116", namcos1_cus116, 0x10);
	state_save_register_UINT8("video", 0, "spriteram", namcos1_spriteram, 0x1000);
	state_save_register_UINT8("video", 0, "playfield", namcos1_playfield_control, 0x20);

	/* set table for sprite color == 0x7f */
	for (i = 0;i <= 15;i++)
		gfx_drawmode_table[i] = DRAWMODE_SHADOW;

	/* all palette entries are not affected by shadow sprites... */
	for (i = 0;i < 0x2000;i++)
		palette_shadow_table[Machine->pens[i]] = Machine->pens[i];
	/* ... except for tilemap colors */
	for (i = 0x0800;i < 0x1000;i++)
		palette_shadow_table[Machine->pens[i]] = Machine->pens[i + 0x0800];

	spriteram = &namcos1_spriteram[0x800];

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ8_HANDLER( namcos1_videoram_r )
{
	return namcos1_videoram[offset];
}

WRITE8_HANDLER( namcos1_videoram_w )
{
	if (namcos1_videoram[offset] != data)
	{
		namcos1_videoram[offset] = data;
		if (offset < 0x7000)
		{   /* background 0-3 */
			int layer = offset >> 13;
			int num = (offset & 0x1fff) >> 1;
			tilemap_mark_tile_dirty(tilemap[layer],num);
		}
		else
		{   /* foreground 4-5 */
			int layer = (offset >> 11 & 1) + 4;
			int num = ((offset & 0x7ff) - 0x10) >> 1;
			if (num >= 0 && num < 0x3f0)
				tilemap_mark_tile_dirty(tilemap[layer],num);
		}
	}
}


READ8_HANDLER( namcos1_paletteram_r )
{
	if ((offset & 0x1800) != 0x1800)
	{
		int color = ((offset & 0x6000) >> 2) | (offset & 0x7ff);
		int component = (offset & 0x1800) >> 11;

		return namcos1_paletteram[color + 0x2000 * component];
	}
	else
	{
		offset &= 0x0f;
		return namcos1_cus116[offset];
	}
}

WRITE8_HANDLER( namcos1_paletteram_w )
{
	if ((offset & 0x1800) != 0x1800)
	{
		int r,g,b;
		int color = ((offset & 0x6000) >> 2) | (offset & 0x7ff);
		int component = (offset & 0x1800) >> 11;

		namcos1_paletteram[color + 0x2000 * component] = data;

		r = namcos1_paletteram[color];
		g = namcos1_paletteram[color + 0x2000];
		b = namcos1_paletteram[color + 0x4000];
		palette_set_color(color,r,g,b);
	}
	else
	{
		offset &= 0x0f;
		namcos1_cus116[offset] = data;
	}
}



static int copy_sprites;

READ8_HANDLER( namcos1_spriteram_r )
{
	/* 0000-07ff work ram */
	/* 0800-0fff sprite ram */
	if (offset < 0x1000)
		return namcos1_spriteram[offset];
	/* 1xxx playfield control ram */
	else
		return namcos1_playfield_control[offset & 0x1f];
}

WRITE8_HANDLER( namcos1_spriteram_w )
{
	/* 0000-07ff work ram */
	/* 0800-0fff sprite ram */
	if (offset < 0x1000)
	{
		namcos1_spriteram[offset] = data;

		/* a write to this offset tells the sprite chip to buffer the sprite list */
		if (offset == 0x0ff2)
			copy_sprites = 1;
	}
	/* 1xxx playfield control ram */
	else
		namcos1_playfield_control[offset & 0x1f] = data;
}



/***************************************************************************

  Display refresh

***************************************************************************/

/*
sprite format:

0-3  scratchpad RAM
4-9  CPU writes here, hardware copies from here to 10-15
10   xx------  X size (16, 8, 32, 4)
10   --x-----  X flip
10   ---xx---  X offset inside 32x32 tile
10   -----xxx  tile bank
11   xxxxxxxx  tile number
12   xxxxxxx-  color
12   -------x  X position MSB
13   xxxxxxxx  X position
14   xxx-----  priority
14   ---xx---  Y offset inside 32x32 tile
14   -----xx-  Y size (16, 8, 32, 4)
14   -------x  Y flip
15   xxxxxxxx  Y position
*/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	const data8_t *source = &spriteram[0x0800-0x20];	/* the last is NOT a sprite */
	const data8_t *finish = &spriteram[0];
	struct GfxElement *gfx = Machine->gfx[1];
	struct GfxElement mygfx = *gfx;

	int sprite_xoffs = spriteram[0x07f5] + ((spriteram[0x07f4] & 1) << 8);
	int sprite_yoffs = spriteram[0x07f7];

	while (source >= finish)
	{
		static const int sprite_size[4] = { 16, 8, 32, 4 };
		int attr1 = source[10];
		int attr2 = source[14];
		int color = source[12];
		int flipx = (attr1 & 0x20) >> 5;
		int flipy = (attr2 & 0x01);
		int sizex = sprite_size[(attr1 & 0xc0) >> 6];
		int sizey = sprite_size[(attr2 & 0x06) >> 1];
		int tx = (attr1 & 0x18) & (~(sizex-1));
		int ty = (attr2 & 0x18) & (~(sizey-1));
		int sx = source[13] + ((color & 0x01) << 8);
		int sy = -source[15] - sizey;
		int sprite = source[11];
		int sprite_bank = attr1 & 7;
		int priority = (source[14] & 0xe0) >> 5;
		int pri_mask = (0xff << (priority + 1)) & 0xff;

		sprite += sprite_bank * 256;
		color = color >> 1;

		sx += sprite_xoffs;
		sy -= sprite_yoffs;

		if (flip_screen)
		{
			sx = -sx - sizex;
			sy = -sy - sizey;
			flipx ^= 1;
			flipy ^= 1;
		}

		sy++;	/* sprites are buffered and delayed by one scanline */

		/* change GfxElement parameters to draw only the needed part of the 32x32 tile */
		mygfx.width = sizex;
		mygfx.height = sizey;
		mygfx.gfxdata = gfx->gfxdata + tx + ty * gfx->line_modulo;

		pdrawgfx( bitmap, &mygfx,
				sprite,
				color,
				flipx,flipy,
				sx & 0x1ff,
				((sy + 16) & 0xff) - 16,
				cliprect,(color != 0x7f) ? TRANSPARENCY_PEN : TRANSPARENCY_PEN_TABLE,0xf, pri_mask);

		source -= 0x10;
	}
}



VIDEO_UPDATE( namcos1 )
{
	int i, j, scrollx, scrolly, priority;
	struct rectangle new_clip = *cliprect;

	/* flip screen is embedded in the sprite control registers */
	/* can't use flip_screen_set() because the visible area is asymmetrical */
	flip_screen = spriteram[0x07f6] & 1;
	tilemap_set_flip(ALL_TILEMAPS,flip_screen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);


	/* background color */
	fillbitmap(bitmap, get_black_pen(), cliprect);

	/* berabohm uses asymmetrical visibility windows to iris on the character */
	i = ((namcos1_cus116[0] << 8) | namcos1_cus116[1]) - 1;			// min x
	if (new_clip.min_x < i) new_clip.min_x = i;
	i = ((namcos1_cus116[2] << 8) | namcos1_cus116[3]) - 1 - 1;		// max x
	if (new_clip.max_x > i) new_clip.max_x = i;
	i = ((namcos1_cus116[4] << 8) | namcos1_cus116[5]) - 0x11;		// min y
	if (new_clip.min_y < i) new_clip.min_y = i;
	i = ((namcos1_cus116[6] << 8) | namcos1_cus116[7]) - 0x11 - 1;	// max y
	if (new_clip.max_y > i) new_clip.max_y = i;

	if (new_clip.max_x < new_clip.min_x || new_clip.max_y < new_clip.min_y)
		return;


	/* set palette base */
	for (i = 0;i < 6;i++)
		tilemap_set_palette_offset(tilemap[i],(namcos1_playfield_control[i + 24] & 7) * 256);

	for (i = 0;i < 4;i++)
	{
		static int disp_x[] = { 25, 27, 28, 29 };

		j = i << 2;
		scrollx = ( namcos1_playfield_control[j+1] + (namcos1_playfield_control[j+0]<<8) ) - disp_x[i];
		scrolly = ( namcos1_playfield_control[j+3] + (namcos1_playfield_control[j+2]<<8) ) + 8;

		if (flip_screen)
		{
			scrollx = -scrollx;
			scrolly = -scrolly;
		}

		tilemap_set_scrollx(tilemap[i],0,scrollx);
		tilemap_set_scrolly(tilemap[i],0,scrolly);
	}


	fillbitmap(priority_bitmap, 0, &new_clip);

	/* bit 0-2 priority */
	/* bit 3   disable  */
	for (priority = 0; priority < 8;priority++)
	{
		for (i = 0;i < 6;i++)
		{
			if (namcos1_playfield_control[16 + i] == priority)
				tilemap_draw_primask(bitmap,&new_clip,tilemap[i],0,priority,0);
		}
	}

	draw_sprites(bitmap, &new_clip);
}


VIDEO_EOF( namcos1 )
{
	if (copy_sprites)
	{
		int i,j;

		for (i = 0;i < 0x800;i += 16)
		{
			for (j = 10;j < 16;j++)
				spriteram[i+j] = spriteram[i+j - 6];
		}

		copy_sprites = 0;
	}
}
