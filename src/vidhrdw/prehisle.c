/***************************************************************************

	Prehistoric Isle video routines

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *pf1_bitmap, *pf2_bitmap;
data16_t *prehisle_video16;
static data16_t vid_control16[7];
static int dirty_back;
static int dirty_front;

/******************************************************************************/

void prehisle_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int offs, mx, my, color, tile, i;
	int colmask[0x80], code, pal_base, tile_base;
	int scrollx, scrolly;
	UINT8 *tilemap = memory_region(REGION_GFX5);
	static int old_base = 0xfffff, old_front = 0xfffff;

	/* Build the dynamic palette */
	palette_init_used_colors();

	/* Text layer */
	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0; color < 16; color++) colmask[color] = 0;
	for (offs = 0; offs < (0x800 >> 1); offs++)
	{
		code = videoram16[offs];
		color = code >> 12;
		if (code == 0xff20)
			continue;
		colmask[color] |= Machine->gfx[0]->pen_usage[code & 0xfff];
	}

	for (color = 0;color < 16;color++)
	{
		for (i = 0; i < 15; i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	/* Tiles - bottom layer */
	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (offs = 0; offs < 256; offs++)
		palette_used_colors[pal_base + offs] = PALETTE_COLOR_USED;

	/* Tiles - top layer */
	pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0; color < 16; color++) colmask[color] = 0;
	for (offs = 0x0000; offs < (0x4000 >> 1); offs++ )
	{
		code = prehisle_video16[offs];
		color = code >> 12;
		colmask[color] |= Machine->gfx[2]->pen_usage[code & 0x7ff];
	}
	for (color = 0; color < 16; color++)
	{
		for (i = 0; i < 15; i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}

/* kludge */
palette_used_colors[pal_base + 16 * color +15] = PALETTE_COLOR_TRANSPARENT;
palette_change_color(pal_base + 16 * color +15 ,0,0,0);

	}

	/* Sprites */
	pal_base = Machine->drv->gfxdecodeinfo[3].color_codes_start;
	for (color = 0; color < 16; color++) colmask[color] = 0;
	for (offs = 0; offs < (0x400 >> 1); offs += 4)
	{
		code = spriteram16[offs+2] & 0x1fff;
		color = spriteram16[offs+3] >> 12;
		if (code > 0x13ff) code=0x13ff;
		colmask[color] |= Machine->gfx[3]->pen_usage[code];
	}
	for (color = 0;color < 16;color++)
	{
		for (i = 0;i < 15;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		dirty_back = 1;
		dirty_front = 1;
	}

	/* Calculate tilebase for background, 32 words per column */
	tile_base = ((vid_control16[3] >> 4) & 0x3ff) * 32;
	if (old_base != tile_base)
		dirty_back = 1; 	/* Redraw */
	old_base = tile_base;

	/* Back layer, taken from tilemap rom */
	if (dirty_back)
	{
		tile_base &= 0x7fff; /* Safety */
		dirty_back = 0;

		for (mx = 0; mx < 17; mx++)
		{
			for (my = 0; my < 32; my++)
			{
				tile = tilemap[2*tile_base+1] + (tilemap[2*tile_base] << 8);
				color = tile >> 12;
				drawgfx(pf1_bitmap,Machine->gfx[1],
							(tile & 0x7ff) | 0x800,
							color,
							tile & 0x800,0,
							16*mx,16*my,
							0,TRANSPARENCY_NONE,0);
				if (++tile_base == 0x8000)
					tile_base = 0;	/* Wraparound */
			}
		}
	}

	scrollx = -(vid_control16[3] & 15);
	scrolly = -vid_control16[2];
	copyscrollbitmap(bitmap,pf1_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Calculate tilebase for background, 32 words per column */
	tile_base = ((vid_control16[1] >> 4) & 0xff) * 32;
	if (old_front != tile_base)
		dirty_front = 1;	/* Redraw */
	old_front = tile_base;

	/* Back layer, taken from tilemap rom */
	if (dirty_front)
	{
		tile_base &= 0x1fff; /* Safety */
		dirty_front = 0;

		for (mx = 0; mx < 17; mx++)
		{
			for (my = 0; my < 32; my++)
			{
				tile = prehisle_video16[tile_base];
				color = tile >> 12;
				drawgfx(pf2_bitmap,Machine->gfx[2],
							tile & 0x7ff,
							color,
							0,tile&0x800,
							16*mx,16*my,
							0,TRANSPARENCY_NONE,0);
				if (++tile_base == 0x2000)
					tile_base = 0; /* Wraparound */
			}
		}
	}

	scrollx = -(vid_control16[1] & 15);
	scrolly = -vid_control16[0];
	copyscrollbitmap(bitmap,pf2_bitmap,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Sprites */
	for (offs = 0; offs < (0x800 >> 1); offs += 4)
	{
		int x,y,sprite,colour,fx,fy;

		y = spriteram16[offs+0];
		if (y>254)	/* Speedup */
			continue;
		x = spriteram16[offs+1];
		if (x&0x200) x=-(0xff-(x&0xff));
		if (x>256)	/* Speedup */
			continue;

		sprite = spriteram16[offs+2];
		colour = spriteram16[offs+3] >> 12;

		fy = sprite & 0x8000;
		fx = sprite & 0x4000;

		sprite = sprite & 0x1fff;

		if (sprite > 0x13ff)
			sprite = 0x13ff;

		drawgfx(bitmap,Machine->gfx[3],
				sprite,
				colour,fx,fy,x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}

	/* Text layer */
	mx = -1;
	my = 0;
	for (offs = 0x000; offs < (0x800 >> 1); offs++)
	{
		mx++;
		if (mx == 32)
		{
			mx = 0;
			my++;
		}
		tile = videoram16[offs];
		if ((tile & 0xff) == 0x20)
			continue;
		color = tile >> 12;
		drawgfx(bitmap,Machine->gfx[0],
				tile & 0xfff,
				color,
				0,0,
				8*mx,8*my,
				&Machine->visible_area,TRANSPARENCY_PEN,15);
	}
}

/******************************************************************************/

int prehisle_vh_start (void)
{
	pf1_bitmap = bitmap_alloc(256+16,512);
	pf2_bitmap = bitmap_alloc(256+16,512);

	if (!pf1_bitmap || !pf2_bitmap)
		return 1;

	return 0;
}

void prehisle_vh_stop (void)
{
	bitmap_free(pf1_bitmap);
	bitmap_free(pf2_bitmap);
}

WRITE16_HANDLER( prehisle_video16_w )
{
	data16_t oldword = prehisle_video16[offset];
	COMBINE_DATA(&prehisle_video16[offset]);

	if (oldword != prehisle_video16[offset])
		dirty_front = 1;	/* Redraw */
}

static int controls_invert;

READ16_HANDLER( prehisle_control16_r )
{
	switch (offset)
	{
	case 0x08: /* Player 2 */
		return readinputport(1);

	case 0x10: /* Coins, tilt, service */
		return readinputport(2);

	case 0x20: /* Player 1 */
		return readinputport(0) ^ controls_invert;

	case 0x21: /* Dips */
		return readinputport(3);

	case 0x22: /* Dips + VBL */
		return readinputport(4);

	default:
		logerror("%06x: read unknown control %02x\n",cpu_get_pc(),offset);
		return 0;
	}
}

WRITE16_HANDLER( prehisle_control16_w )
{
	switch (offset)
	{
	case 0:
		COMBINE_DATA(&vid_control16[0]);
		break;

	case 0x08:
		COMBINE_DATA(&vid_control16[1]);
		break;

	case 0x10:
		COMBINE_DATA(&vid_control16[2]);
		break;

	case 0x18:
		COMBINE_DATA(&vid_control16[3]);
		break;

	case 0x23:
		controls_invert = data ? 0xff : 0x00;
		break;

	case 0x28:
		COMBINE_DATA(&vid_control16[4]);
		break;

	case 0x29:
		COMBINE_DATA(&vid_control16[5]);
		break;

	case 0x30:
		COMBINE_DATA(&vid_control16[6]);
		break;

	default:
		logerror("%06x: write unknown control %02x\n",cpu_get_pc(),offset);
		break;
	}
}

