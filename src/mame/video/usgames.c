#include "emu.h"
#include "includes/usgames.h"


void usgames_state::palette_init()
{
	int j;

	for (j = 0; j < 0x200; j++)
	{
		int data;
		int r, g, b, i;

		if (j & 0x01)
			data = (j >> 5) & 0x0f;
		else
			data = (j >> 1) & 0x0f;

		r = (data & 1) >> 0;
		g = (data & 2) >> 1;
		b = (data & 4) >> 2;
		i = (data & 8) >> 3;

		r = 0xff * r;
		g = 0x7f * g * (i + 1);
		b = 0x7f * b * (i + 1);

		palette_set_color(machine(),j,MAKE_RGB(r, g, b));
	}
}



TILE_GET_INFO_MEMBER(usgames_state::get_usgames_tile_info)
{
	int tileno, colour;

	tileno = m_videoram[tile_index*2];
	colour = m_videoram[tile_index*2+1];

	SET_TILE_INFO_MEMBER(0,tileno,colour,0);
}

void usgames_state::video_start()
{
	m_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(usgames_state::get_usgames_tile_info),this),TILEMAP_SCAN_ROWS, 8, 8,64,32);
	machine().gfx[0]->set_source(m_charram);
}


WRITE8_MEMBER(usgames_state::usgames_videoram_w)
{
	m_videoram[offset] = data;
	m_tilemap->mark_tile_dirty(offset/2);
}

WRITE8_MEMBER(usgames_state::usgames_charram_w)
{
	m_charram[offset] = data;
	machine().gfx[0]->mark_dirty(offset/8);
}


UINT32 usgames_state::screen_update_usgames(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_tilemap->draw(bitmap, cliprect, 0,0);
	return 0;
}
