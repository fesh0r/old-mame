/******************************************************************************

Strength & Skill (c) 1984 Sun Electronics

Video hardware driver by Uki

    19/Jun/2001 -

******************************************************************************/

#include "driver.h"


UINT8 *strnskil_xscroll;
static UINT8 strnskil_scrl_ctrl;

static tilemap *bg_tilemap;


PALETTE_INIT( strnskil )
{
	int i;

	/* allocate the colortable */
	machine->colortable = colortable_alloc(machine, 0x100);

	/* create a lookup table for the palette */
	for (i = 0; i < 0x100; i++)
	{
		int r = pal4bit(color_prom[i + 0x000]);
		int g = pal4bit(color_prom[i + 0x100]);
		int b = pal4bit(color_prom[i + 0x200]);

		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(r, g, b));
	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 0x300;

	/* sprites lookup table */
	for (i = 0; i < 0x400; i++)
	{
		UINT8 ctabentry = color_prom[i];
		colortable_entry_set_value(machine->colortable, i, ctabentry);
	}
}


WRITE8_HANDLER( strnskil_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
}

WRITE8_HANDLER( strnskil_scrl_ctrl_w )
{
	strnskil_scrl_ctrl = data >> 5;

	if (flip_screen != (data & 0x08))
	{
		flip_screen_set(data & 0x08);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static TILE_GET_INFO( get_bg_tile_info )
{
	int attr = videoram[tile_index * 2];
	int code = videoram[(tile_index * 2) + 1] + ((attr & 0x60) << 3);
	int color = (attr & 0x1f) | ((attr & 0x80) >> 2);

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( strnskil )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols,
		 8, 8, 32, 32);

	tilemap_set_scroll_rows(bg_tilemap, 32);
}

static void draw_sprites(running_machine *machine, mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs;

	for (offs = 0x60; offs < 0x100; offs += 4)
	{
		int code = spriteram[offs + 1];
		int color = spriteram[offs + 2] & 0x3f;
		int flipx = flip_screen_x;
		int flipy = flip_screen_y;

		int sx = spriteram[offs + 3];
		int sy = spriteram[offs];
		int px, py;

		if (flip_screen)
		{
			px = 240 - sx + 0; /* +2 or +0 ? */
			py = sy;
		}
		else
		{
			px = sx - 2;
			py = 240 - sy;
		}

		sx = sx & 0xff;

		if (sx > 248)
			sx = sx - 256;

		drawgfx(bitmap, machine->gfx[1],
			code, color,
			flipx, flipy,
			px, py,
			cliprect,
			TRANSPARENCY_PENS,
			colortable_get_transpen_mask(machine->colortable, machine->gfx[1], color, 0));
	}
}

VIDEO_UPDATE( strnskil )
{
	int row;

	for (row = 0; row < 32; row++)
	{
		if (strnskil_scrl_ctrl != 0x07)
		{
			switch (memory_region(REGION_USER1)[strnskil_scrl_ctrl * 32 + row])
			{
			case 2:
				tilemap_set_scrollx(bg_tilemap, row, -~strnskil_xscroll[1]);
				break;
			case 4:
				tilemap_set_scrollx(bg_tilemap, row, -~strnskil_xscroll[0]);
				break;
			}
		}
	}

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	draw_sprites(machine, bitmap, cliprect);
	return 0;
}
