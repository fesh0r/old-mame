#include "emu.h"
#include "includes/tigeroad.h"


WRITE16_MEMBER(tigeroad_state::tigeroad_videoram_w)
{
	UINT16 *videoram = m_videoram;
	COMBINE_DATA(&videoram[offset]);
	m_fg_tilemap->mark_tile_dirty(offset);
}

WRITE16_MEMBER(tigeroad_state::tigeroad_videoctrl_w)
{
	int bank;

	if (ACCESSING_BITS_8_15)
	{
		data = (data >> 8) & 0xff;

		/* bit 1 flips screen */

		if (flip_screen() != (data & 0x02))
		{
			flip_screen_set(data & 0x02);
			machine().tilemap().mark_all_dirty();
		}

		/* bit 2 selects bg char bank */

		bank = (data & 0x04) >> 2;

		if (m_bgcharbank != bank)
		{
			m_bgcharbank = bank;
			m_bg_tilemap->mark_all_dirty();
		}

		/* bits 4-5 are coin lockouts */

		coin_lockout_w(machine(), 0, !(data & 0x10));
		coin_lockout_w(machine(), 1, !(data & 0x20));

		/* bits 6-7 are coin counters */

		coin_counter_w(machine(), 0, data & 0x40);
		coin_counter_w(machine(), 1, data & 0x80);
	}
}

WRITE16_MEMBER(tigeroad_state::tigeroad_scroll_w)
{
	int scroll = 0;

	COMBINE_DATA(&scroll);

	switch (offset)
	{
	case 0:
		m_bg_tilemap->set_scrollx(0, scroll);
		break;
	case 1:
		m_bg_tilemap->set_scrolly(0, -scroll - 32 * 8);
		break;
	}
}

static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int priority )
{
	tigeroad_state *state = machine.driver_data<tigeroad_state>();
	UINT16 *source = &state->m_spriteram->buffer()[state->m_spriteram->bytes()/2] - 4;
	UINT16 *finish = state->m_spriteram->buffer();

	// TODO: The Track Map should probably be drawn on top of the background tilemap...
	//       Also convert the below into a for loop!

	while (source >= finish)
	{
		int tile_number = source[0];

		if (tile_number != 0xfff) {
			int attr = source[1];
			int sy = source[2] & 0x1ff;
			int sx = source[3] & 0x1ff;

			int flipx = attr & 0x02;
			int flipy = attr & 0x01;
			int color = (attr >> 2) & 0x0f;

			if (sx > 0x100) sx -= 0x200;
			if (sy > 0x100) sy -= 0x200;

			if (state->flip_screen())
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx_transpen(bitmap, cliprect,
				machine.gfx[2],
				tile_number,
				color,
				flipx, flipy,
				sx, 240 - sy, 15);
		}

		source -= 4;
	}
}

TILE_GET_INFO_MEMBER(tigeroad_state::get_bg_tile_info)
{
	UINT8 *tilerom = memregion("gfx4")->base();

	int data = tilerom[tile_index];
	int attr = tilerom[tile_index + 1];
	int code = data + ((attr & 0xc0) << 2) + (m_bgcharbank << 10);
	int color = attr & 0x0f;
	int flags = (attr & 0x20) ? TILE_FLIPX : 0;

	SET_TILE_INFO_MEMBER(1, code, color, flags);
	tileinfo.group = (attr & 0x10) ? 1 : 0;
}

TILE_GET_INFO_MEMBER(tigeroad_state::get_fg_tile_info)
{
	UINT16 *videoram = m_videoram;
	int data = videoram[tile_index];
	int attr = data >> 8;
	int code = (data & 0xff) + ((attr & 0xc0) << 2) + ((attr & 0x20) << 5);
	int color = attr & 0x0f;
	int flags = (attr & 0x10) ? TILE_FLIPY : 0;

	SET_TILE_INFO_MEMBER(0, code, color, flags);
}

TILEMAP_MAPPER_MEMBER(tigeroad_state::tigeroad_tilemap_scan)
{
	/* logical (col,row) -> memory offset */
	return 2 * (col % 8) + 16 * ((127 - row) % 8) + 128 * (col / 8) + 2048 * ((127 - row) / 8);
}

void tigeroad_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tigeroad_state::get_bg_tile_info),this), tilemap_mapper_delegate(FUNC(tigeroad_state::tigeroad_tilemap_scan),this),
		 32, 32, 128, 128);

	m_fg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tigeroad_state::get_fg_tile_info),this), TILEMAP_SCAN_ROWS,
		 8, 8, 32, 32);

	m_bg_tilemap->set_transmask(0, 0xffff, 0);
	m_bg_tilemap->set_transmask(1, 0x1ff, 0xfe00);

	m_fg_tilemap->set_transparent_pen(3);
}

UINT32 tigeroad_state::screen_update_tigeroad(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER1, 0);
	draw_sprites(machine(), bitmap, cliprect, 0);
	m_bg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER0, 1);
	//draw_sprites(machine(), bitmap, cliprect, 1); draw priority sprites?
	m_fg_tilemap->draw(bitmap, cliprect, 0, 2);
	return 0;
}
