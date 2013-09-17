#include "emu.h"
#include "includes/sidepckt.h"


void sidepckt_state::palette_init()
{
	const UINT8 *color_prom = memregion("proms")->base();
	int i;

	for (i = 0;i < machine().total_colors();i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 4) & 0x01;
		bit1 = (color_prom[i] >> 5) & 0x01;
		bit2 = (color_prom[i] >> 6) & 0x01;
		bit3 = (color_prom[i] >> 7) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + machine().total_colors()] >> 0) & 0x01;
		bit1 = (color_prom[i + machine().total_colors()] >> 1) & 0x01;
		bit2 = (color_prom[i + machine().total_colors()] >> 2) & 0x01;
		bit3 = (color_prom[i + machine().total_colors()] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(machine(),i,MAKE_RGB(r,g,b));
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

TILE_GET_INFO_MEMBER(sidepckt_state::get_tile_info)
{
	UINT8 attr = m_colorram[tile_index];
	SET_TILE_INFO_MEMBER(
			0,
			m_videoram[tile_index] + ((attr & 0x07) << 8),
			((attr & 0x10) >> 3) | ((attr & 0x20) >> 5),
			TILE_FLIPX);
	tileinfo.group = (attr & 0x80) >> 7;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void sidepckt_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(sidepckt_state::get_tile_info),this),TILEMAP_SCAN_ROWS,8,8,32,32);

	m_bg_tilemap->set_transmask(0,0xff,0x00); /* split type 0 is totally transparent in front half */
	m_bg_tilemap->set_transmask(1,0x01,0xfe); /* split type 1 has pen 0 transparent in front half */

	machine().tilemap().set_flip_all(TILEMAP_FLIPX);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_MEMBER(sidepckt_state::sidepckt_videoram_w)
{
	m_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(sidepckt_state::sidepckt_colorram_w)
{
	m_colorram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(sidepckt_state::sidepckt_flipscreen_w)
{
	int flipscreen = data;
	machine().tilemap().set_flip_all(flipscreen ? TILEMAP_FLIPY : TILEMAP_FLIPX);
}


/***************************************************************************

  Display refresh

***************************************************************************/

void sidepckt_state::draw_sprites(bitmap_ind16 &bitmap,const rectangle &cliprect)
{
	UINT8 *spriteram = m_spriteram;
	int offs;

	for (offs = 0;offs < m_spriteram.bytes(); offs += 4)
	{
		int sx,sy,code,color,flipx,flipy;

		code = spriteram[offs+3] + ((spriteram[offs+1] & 0x03) << 8);
		color = (spriteram[offs+1] & 0xf0) >> 4;

		sx = spriteram[offs+2]-2;
		sy = spriteram[offs];

		flipx = spriteram[offs+1] & 0x08;
		flipy = spriteram[offs+1] & 0x04;

		drawgfx_transpen(bitmap,cliprect,machine().gfx[1],
				code,
				color,
				flipx,flipy,
				sx,sy,0);
		/* wraparound */
		drawgfx_transpen(bitmap,cliprect,machine().gfx[1],
				code,
				color,
				flipx,flipy,
				sx-256,sy,0);
	}
}


UINT32 sidepckt_state::screen_update_sidepckt(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER1,0);
	draw_sprites(bitmap,cliprect);
	m_bg_tilemap->draw(screen, bitmap, cliprect, TILEMAP_DRAW_LAYER0,0);
	return 0;
}
