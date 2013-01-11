/***************************************************************************

    Midway MCR-68k system

***************************************************************************/

#include "emu.h"
#include "includes/mcr.h"
#include "includes/mcr68.h"


#define LOW_BYTE(x) ((x) & 0xff)


/*************************************
 *
 *  Tilemap callbacks
 *
 *************************************/

TILE_GET_INFO_MEMBER(mcr68_state::get_bg_tile_info)
{
	UINT16 *videoram = m_videoram;
	int data = LOW_BYTE(videoram[tile_index * 2]) | (LOW_BYTE(videoram[tile_index * 2 + 1]) << 8);
	int code = (data & 0x3ff) | ((data >> 4) & 0xc00);
	int color = (~data >> 12) & 3;
	SET_TILE_INFO_MEMBER(0, code, color, TILE_FLIPYX((data >> 10) & 3));
	if (machine().gfx[0]->elements() < 0x1000)
		tileinfo.category = (data >> 15) & 1;
}


TILE_GET_INFO_MEMBER(mcr68_state::zwackery_get_bg_tile_info)
{
	UINT16 *videoram = m_videoram;
	int data = videoram[tile_index];
	int color = (data >> 13) & 7;
	SET_TILE_INFO_MEMBER(0, data & 0x3ff, color, TILE_FLIPYX((data >> 11) & 3));
}


TILE_GET_INFO_MEMBER(mcr68_state::zwackery_get_fg_tile_info)
{
	UINT16 *videoram = m_videoram;
	int data = videoram[tile_index];
	int color = (data >> 13) & 7;
	SET_TILE_INFO_MEMBER(2, data & 0x3ff, color, TILE_FLIPYX((data >> 11) & 3));
	tileinfo.category = (color != 0);
}



/*************************************
 *
 *  Video startup
 *
 *************************************/

VIDEO_START_MEMBER(mcr68_state,mcr68)
{
	/* initialize the background tilemap */
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(mcr68_state::get_bg_tile_info),this), TILEMAP_SCAN_ROWS,  16,16, 32,32);
	m_bg_tilemap->set_transparent_pen(0);
}


VIDEO_START_MEMBER(mcr68_state,zwackery)
{
	const UINT8 *colordatabase = (const UINT8 *)memregion("gfx3")->base();
	gfx_element *gfx0 = machine().gfx[0];
	gfx_element *gfx2 = machine().gfx[2];
	UINT8 *srcdata0, *dest0;
	UINT8 *srcdata2, *dest2;
	int code, y, x;

	/* initialize the background tilemap */
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(mcr68_state::zwackery_get_bg_tile_info),this), TILEMAP_SCAN_ROWS,  16,16, 32,32);

	/* initialize the foreground tilemap */
	m_fg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(mcr68_state::zwackery_get_fg_tile_info),this), TILEMAP_SCAN_ROWS,  16,16, 32,32);
	m_fg_tilemap->set_transparent_pen(0);

	/* allocate memory for the assembled gfx data */
	srcdata0 = auto_alloc_array(machine(), UINT8, gfx0->elements() * gfx0->width() * gfx0->height());
	srcdata2 = auto_alloc_array(machine(), UINT8, gfx2->elements() * gfx2->width() * gfx2->height());

	/* "colorize" each code */
	dest0 = srcdata0;
	dest2 = srcdata2;
	for (code = 0; code < gfx0->elements(); code++)
	{
		const UINT8 *coldata = colordatabase + code * 32;
		const UINT8 *gfxdata0 = gfx0->get_data(code);
		const UINT8 *gfxdata2 = gfx2->get_data(code);

		/* assume 16 rows */
		for (y = 0; y < 16; y++)
		{
			const UINT8 *gd0 = gfxdata0;
			const UINT8 *gd2 = gfxdata2;

			/* 16 columns */
			for (x = 0; x < 16; x++, gd0++, gd2++)
			{
				int coloffs = (y & 0x0c) | ((x >> 2) & 0x03);
				int pen0 = coldata[coloffs * 2 + 0];
				int pen1 = coldata[coloffs * 2 + 1];
				int tp0, tp1;

				/* every 4 pixels gets its own foreground/background colors */
				*dest0++ = *gd0 ? pen1 : pen0;

				/* for gfx 2, we convert all low-priority pens to 0 */
				tp0 = (pen0 & 0x80) ? pen0 : 0;
				tp1 = (pen1 & 0x80) ? pen1 : 0;
				*dest2++ = *gd2 ? tp1 : tp0;
			}

			/* advance */
			gfxdata0 += gfx0->rowbytes();
			gfxdata2 += gfx2->rowbytes();
		}
	}

	/* make the assembled data our new source data */
	gfx0->set_raw_layout(srcdata0, gfx0->width(), gfx0->height(), gfx0->elements(), 8 * gfx0->width(), 8 * gfx0->width() * gfx0->height());
	gfx2->set_raw_layout(srcdata2, gfx2->width(), gfx2->height(), gfx2->elements(), 8 * gfx2->width(), 8 * gfx2->width() * gfx2->height());
}



/*************************************
 *
 *  Palette RAM writes
 *
 *************************************/

WRITE16_MEMBER(mcr68_state::mcr68_paletteram_w)
{
	int newword;

	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	newword = m_generic_paletteram_16[offset];
	palette_set_color_rgb(machine(), offset, pal3bit(newword >> 6), pal3bit(newword >> 0), pal3bit(newword >> 3));
}


WRITE16_MEMBER(mcr68_state::zwackery_paletteram_w)
{
	int newword;

	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	newword = m_generic_paletteram_16[offset];
	palette_set_color_rgb(machine(), offset, pal5bit(~newword >> 10), pal5bit(~newword >> 0), pal5bit(~newword >> 5));
}



/*************************************
 *
 *  Video RAM writes
 *
 *************************************/

WRITE16_MEMBER(mcr68_state::mcr68_videoram_w)
{
	UINT16 *videoram = m_videoram;
	COMBINE_DATA(&videoram[offset]);
	m_bg_tilemap->mark_tile_dirty(offset / 2);
}


WRITE16_MEMBER(mcr68_state::zwackery_videoram_w)
{
	UINT16 *videoram = m_videoram;
	COMBINE_DATA(&videoram[offset]);
	m_bg_tilemap->mark_tile_dirty(offset);
	m_fg_tilemap->mark_tile_dirty(offset);
}


WRITE16_MEMBER(mcr68_state::zwackery_spriteram_w)
{
	/* yech -- Zwackery relies on the upper 8 bits of a spriteram read being $ff! */
	/* to make this happen we always write $ff in the upper 8 bits */
	COMBINE_DATA(&m_spriteram[offset]);
	m_spriteram[offset] |= 0xff00;
}



/*************************************
 *
 *  Sprite update
 *
 *************************************/

static void mcr68_update_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int priority)
{
	mcr68_state *state = machine.driver_data<mcr68_state>();
	rectangle sprite_clip = machine.primary_screen->visible_area();
	UINT16 *spriteram = state->m_spriteram;
	int offs;

	/* adjust for clipping */
	sprite_clip.min_x += state->m_sprite_clip;
	sprite_clip.max_x -= state->m_sprite_clip;
	sprite_clip &= cliprect;

	machine.priority_bitmap.fill(1, sprite_clip);

	/* loop over sprite RAM */
	for (offs = state->m_spriteram.bytes() / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, flags;

		flags = LOW_BYTE(spriteram[offs + 1]);
		code = LOW_BYTE(spriteram[offs + 2]) + 256 * ((flags >> 3) & 0x01) + 512 * ((flags >> 6) & 0x03);

		/* skip if zero */
		if (code == 0)
			continue;

		/* also skip if this isn't the priority we're drawing right now */
		if (((flags >> 2) & 1) != priority)
			continue;

		/* extract the bits of information */
		color = ~flags & 0x03;
		flipx = flags & 0x10;
		flipy = flags & 0x20;
		x = LOW_BYTE(spriteram[offs + 3]) * 2 + state->m_sprite_xoffset;
		y = (241 - LOW_BYTE(spriteram[offs])) * 2;

		/* allow sprites to clip off the left side */
		if (x > 0x1f0) x -= 0x200;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
		    The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx_transmask(bitmap, sprite_clip, machine.gfx[1], code, color, flipx, flipy, x, y,
				machine.priority_bitmap, 0x00, 0x0101);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx_transmask(bitmap, sprite_clip, machine.gfx[1], code, color, flipx, flipy, x, y,
				machine.priority_bitmap, 0x02, 0xfeff);
	}
}


static void zwackery_update_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int priority)
{
	mcr68_state *state = machine.driver_data<mcr68_state>();
	UINT16 *spriteram = state->m_spriteram;
	int offs;

	machine.priority_bitmap.fill(1, cliprect);

	/* loop over sprite RAM */
	for (offs = state->m_spriteram.bytes() / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, flags;

		/* get the code and skip if zero */
		code = LOW_BYTE(spriteram[offs + 2]);
		if (code == 0)
			continue;

		/* extract the flag bits and determine the color */
		flags = LOW_BYTE(spriteram[offs + 1]);
		color = ((~flags >> 2) & 0x0f) | ((flags & 0x02) << 3);

		/* for low priority, draw everything but color 7 */
		if (!priority)
		{
			if (color == 7)
				continue;
		}

		/* for high priority, only draw color 7 */
		else
		{
			if (color != 7)
				continue;
		}

		/* determine flipping and coordinates */
		flipx = ~flags & 0x40;
		flipy = flags & 0x80;
		x = (231 - LOW_BYTE(spriteram[offs + 3])) * 2;
		y = (241 - LOW_BYTE(spriteram[offs])) * 2;

		if (x <= -32) x += 512;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
		    The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx_transmask(bitmap, cliprect, machine.gfx[1], code, color, flipx, flipy, x, y,
				machine.priority_bitmap, 0x00, 0x0101);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx_transmask(bitmap, cliprect, machine.gfx[1], code, color, flipx, flipy, x, y,
				machine.priority_bitmap, 0x02, 0xfeff);
	}
}



/*************************************
 *
 *  General MCR/68k update
 *
 *************************************/

UINT32 mcr68_state::screen_update_mcr68(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	/* draw the background */
	m_bg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_OPAQUE | TILEMAP_DRAW_ALL_CATEGORIES, 0);

	/* draw the low-priority sprites */
	mcr68_update_sprites(machine(), bitmap, cliprect, 0);

	/* redraw tiles with priority over sprites */
	m_bg_tilemap->draw(bitmap, cliprect, 1, 0);

	/* draw the high-priority sprites */
	mcr68_update_sprites(machine(), bitmap, cliprect, 1);
	return 0;
}


UINT32 mcr68_state::screen_update_zwackery(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	/* draw the background */
	m_bg_tilemap->draw(bitmap, cliprect, 0, 0);

	/* draw the low-priority sprites */
	zwackery_update_sprites(machine(), bitmap, cliprect, 0);

	/* redraw tiles with priority over sprites */
	m_fg_tilemap->draw(bitmap, cliprect, 1, 0);

	/* draw the high-priority sprites */
	zwackery_update_sprites(machine(), bitmap, cliprect, 1);
	return 0;
}
