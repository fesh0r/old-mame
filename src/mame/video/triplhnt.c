/***************************************************************************

Atari Triple Hunt video emulation

***************************************************************************/

#include "emu.h"
#include "includes/triplhnt.h"


TILE_GET_INFO_MEMBER(triplhnt_state::get_tile_info)
{
	int code = m_playfield_ram[tile_index] & 0x3f;

	SET_TILE_INFO_MEMBER(2, code, code == 0x3f ? 1 : 0, 0);
}


void triplhnt_state::video_start()
{
	machine().primary_screen->register_screen_bitmap(m_helper);

	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(triplhnt_state::get_tile_info),this), TILEMAP_SCAN_ROWS, 16, 16, 16, 16);
}


TIMER_CALLBACK_MEMBER(triplhnt_state::triplhnt_hit_callback)
{
	triplhnt_set_collision(param);
}


void triplhnt_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int i;

	int hit_line = 999;
	int hit_code = 999;

	for (i = 0; i < 16; i++)
	{
		rectangle rect;

		int j = (m_orga_ram[i] & 15) ^ 15;

		/* software sorts sprites by x and stores order in orga RAM */

		int hpos = m_hpos_ram[j] ^ 255;
		int vpos = m_vpos_ram[j] ^ 255;
		int code = m_code_ram[j] ^ 255;

		if (hpos == 255)
			continue;

		/* sprite placement might be wrong */

		if (m_sprite_zoom)
		{
			rect.set(hpos - 16, hpos - 16 + 63, 196 - vpos, 196 - vpos + 63);
		}
		else
		{
			rect.set(hpos - 16, hpos - 16 + 31, 224 - vpos, 224 - vpos + 31);
		}

		/* render sprite to auxiliary bitmap */

		drawgfx_opaque(m_helper, cliprect, machine().gfx[m_sprite_zoom],
			2 * code + m_sprite_bank, 0, code & 8, 0,
			rect.min_x, rect.min_y);

		rect &= cliprect;

		/* check for collisions and copy sprite */

		{
			int x;
			int y;

			for (x = rect.min_x; x <= rect.max_x; x++)
			{
				for (y = rect.min_y; y <= rect.max_y; y++)
				{
					pen_t a = m_helper.pix16(y, x);
					pen_t b = bitmap.pix16(y, x);

					if (a == 2 && b == 7)
					{
						hit_code = j;
						hit_line = y;
					}

					if (a != 1)
						bitmap.pix16(y, x) = a;
				}
			}
		}
	}

	if (hit_line != 999 && hit_code != 999)
		machine().scheduler().timer_set(machine().primary_screen->time_until_pos(hit_line), timer_expired_delegate(FUNC(triplhnt_state::triplhnt_hit_callback),this), hit_code);
}


UINT32 triplhnt_state::screen_update_triplhnt(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	device_t *discrete = machine().device("discrete");

	m_bg_tilemap->mark_all_dirty();

	m_bg_tilemap->draw(bitmap, cliprect, 0, 0);

	draw_sprites(bitmap, cliprect);

	address_space &space = machine().driver_data()->generic_space();
	discrete_sound_w(discrete, space, TRIPLHNT_BEAR_ROAR_DATA, m_playfield_ram[0xfa] & 15);
	discrete_sound_w(discrete, space, TRIPLHNT_SHOT_DATA, m_playfield_ram[0xfc] & 15);
	return 0;
}
