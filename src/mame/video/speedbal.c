 /****************************************************************************
 *                                                                           *
 * video.c                                                                 *
 *                                                                           *
 * Functions to emulate the video hardware of the machine.                   *
 *                                                                           *
 ****************************************************************************/

#include "emu.h"
#include "includes/speedbal.h"


TILE_GET_INFO_MEMBER(speedbal_state::get_tile_info_bg)
{
	int code = m_background_videoram[tile_index*2] + ((m_background_videoram[tile_index*2+1] & 0x30) << 4);
	int color = m_background_videoram[tile_index*2+1] & 0x0f;

	SET_TILE_INFO_MEMBER(1, code, color, 0);
	tileinfo.group = (color == 8);
}

TILE_GET_INFO_MEMBER(speedbal_state::get_tile_info_fg)
{
	int code = m_foreground_videoram[tile_index*2] + ((m_foreground_videoram[tile_index*2+1] & 0x30) << 4);
	int color = m_foreground_videoram[tile_index*2+1] & 0x0f;

	SET_TILE_INFO_MEMBER(0, code, color, 0);
	tileinfo.group = (color == 9);
}

/*************************************
 *                                   *
 *      Start-Stop                   *
 *                                   *
 *************************************/

void speedbal_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(speedbal_state::get_tile_info_bg),this), TILEMAP_SCAN_COLS_FLIP_X,  16, 16, 16, 16);
	m_fg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(speedbal_state::get_tile_info_fg),this), TILEMAP_SCAN_COLS_FLIP_X,   8,  8, 32, 32);

	m_bg_tilemap->set_transmask(0,0xffff,0x0000); /* split type 0 is totally transparent in front half */
	m_bg_tilemap->set_transmask(1,0x00f7,0x0000); /* split type 1 has pen 0-2, 4-7 transparent in front half */

	m_fg_tilemap->set_transmask(0,0xffff,0x0001); /* split type 0 is totally transparent in front half and has pen 0 transparent in back half */
	m_fg_tilemap->set_transmask(1,0x0001,0x0001); /* split type 1 has pen 0 transparent in front and back half */
}



/*************************************
 *                                   *
 *  Foreground characters RAM        *
 *                                   *
 *************************************/

WRITE8_MEMBER(speedbal_state::speedbal_foreground_videoram_w)
{
	m_foreground_videoram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset>>1);
}

/*************************************
 *                                   *
 *  Background tiles RAM             *
 *                                   *
 *************************************/

WRITE8_MEMBER(speedbal_state::speedbal_background_videoram_w)
{
	m_background_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset>>1);
}


/*************************************
 *                                   *
 *   Sprite drawing                  *
 *                                   *
 *************************************/

static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	speedbal_state *state = machine.driver_data<speedbal_state>();
	UINT8 *spriteram = state->m_spriteram;
	int x,y,code,color,offset,flipx,flipy;

	/* Drawing sprites: 64 in total */

	for (offset = 0;offset < state->m_spriteram.bytes();offset += 4)
	{
		if(!(spriteram[offset + 2] & 0x80))
			continue;

		x = 243 - spriteram[offset + 3];
		y = 239 - spriteram[offset + 0];

		code = BITSWAP8(spriteram[offset + 1],0,1,2,3,4,5,6,7) | ((spriteram[offset + 2] & 0x40) << 2);

		color = spriteram[offset + 2] & 0x0f;

		flipx = flipy = 0;

		if(state->flip_screen())
		{
			x = 246 - x;
			y = 238 - y;
			flipx = flipy = 1;
		}

		drawgfx_transpen (bitmap,cliprect,machine.gfx[2],
				code,
				color,
				flipx,flipy,
				x,y,0);
	}
}

/*************************************
 *                                   *
 *   Refresh screen                  *
 *                                   *
 *************************************/

UINT32 speedbal_state::screen_update_speedbal(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER1, 0);
	m_fg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER1, 0);
	draw_sprites(machine(), bitmap, cliprect);
	m_bg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER0, 0);
	m_fg_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_LAYER0, 0);
	return 0;
}
