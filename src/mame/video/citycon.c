/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/citycon.h"


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

TILEMAP_MAPPER_MEMBER(citycon_state::citycon_scan)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x60) << 5);
}

TILE_GET_INFO_MEMBER(citycon_state::get_fg_tile_info)
{
	SET_TILE_INFO_MEMBER(
			0,
			m_videoram[tile_index],
			(tile_index & 0x03e0) >> 5,	/* color depends on scanline only */
			0);
}

TILE_GET_INFO_MEMBER(citycon_state::get_bg_tile_info)
{
	UINT8 *rom = memregion("gfx4")->base();
	int code = rom[0x1000 * m_bg_image + tile_index];
	SET_TILE_INFO_MEMBER(
			3 + m_bg_image,
			code,
			rom[0xc000 + 0x100 * m_bg_image + code],
			0);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void citycon_state::video_start()
{
	m_fg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(citycon_state::get_fg_tile_info),this), tilemap_mapper_delegate(FUNC(citycon_state::citycon_scan),this), 8, 8, 128, 32);
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(citycon_state::get_bg_tile_info),this), tilemap_mapper_delegate(FUNC(citycon_state::citycon_scan),this), 8, 8, 128, 32);

	m_fg_tilemap->set_transparent_pen(0);
	m_fg_tilemap->set_scroll_rows(32);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_MEMBER(citycon_state::citycon_videoram_w)
{
	m_videoram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset);
}


WRITE8_MEMBER(citycon_state::citycon_linecolor_w)
{
	m_linecolor[offset] = data;
}


WRITE8_MEMBER(citycon_state::citycon_background_w)
{

	/* bits 4-7 control the background image */
	if (m_bg_image != (data >> 4))
	{
		m_bg_image = (data >> 4);
		m_bg_tilemap->mark_all_dirty();
	}

	/* bit 0 flips screen */
	/* it is also used to multiplex player 1 and player 2 controls */
	flip_screen_set(data & 0x01);

	/* bits 1-3 are unknown */
//  if ((data & 0x0e) != 0) logerror("background register = %02x\n", data);
}



static void draw_sprites( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	citycon_state *state = machine.driver_data<citycon_state>();
	int offs;

	for (offs = state->m_spriteram.bytes() - 4; offs >= 0; offs -= 4)
	{
		int sx, sy, flipx;

		sx = state->m_spriteram[offs + 3];
		sy = 239 - state->m_spriteram[offs];
		flipx = ~state->m_spriteram[offs + 2] & 0x10;
		if (state->flip_screen())
		{
			sx = 240 - sx;
			sy = 238 - sy;
			flipx = !flipx;
		}

		drawgfx_transpen(bitmap, cliprect, machine.gfx[state->m_spriteram[offs + 1] & 0x80 ? 2 : 1],
				state->m_spriteram[offs + 1] & 0x7f,
				state->m_spriteram[offs + 2] & 0x0f,
				flipx,state->flip_screen(),
				sx, sy, 0);
	}
}


INLINE void changecolor_RRRRGGGGBBBBxxxx( running_machine &machine, int color, int indx )
{
	citycon_state *state = machine.driver_data<citycon_state>();
	int data = state->m_generic_paletteram_8[2 * indx | 1] | (state->m_generic_paletteram_8[2 * indx] << 8);
	palette_set_color_rgb(machine, color, pal4bit(data >> 12), pal4bit(data >> 8), pal4bit(data >> 4));
}

UINT32 citycon_state::screen_update_citycon(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int offs, scroll;

	/* Update the virtual palette to support text color code changing on every scanline. */
	for (offs = 0; offs < 256; offs++)
	{
		int indx = m_linecolor[offs];
		int i;

		for (i = 0; i < 4; i++)
			changecolor_RRRRGGGGBBBBxxxx(machine(), 640 + 4 * offs + i, 512 + 4 * indx + i);
	}


	scroll = m_scroll[0] * 256 + m_scroll[1];
	m_bg_tilemap->set_scrollx(0, scroll >> 1);
	for (offs = 6; offs < 32; offs++)
		m_fg_tilemap->set_scrollx(offs, scroll);

	m_bg_tilemap->draw(bitmap, cliprect, 0, 0);
	m_fg_tilemap->draw(bitmap, cliprect, 0, 0);
	draw_sprites(machine(), bitmap, cliprect);
	return 0;
}
