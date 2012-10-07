/***************************************************************************

  video.c

  Functions to emulate the video hardware of early Toaplan hardware.

***************************************************************************/

#include "emu.h"
#include "includes/slapfght.h"


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

TILE_GET_INFO_MEMBER(slapfght_state::get_pf_tile_info)/* For Performan only */
{
	int tile,color;

	tile=m_slapfight_videoram[tile_index] + ((m_slapfight_colorram[tile_index] & 0x03) << 8);
	color=(m_slapfight_colorram[tile_index] >> 3) & 0x0f;
	SET_TILE_INFO_MEMBER(
			0,
			tile,
			color,
			0);
}

TILE_GET_INFO_MEMBER(slapfght_state::get_pf1_tile_info)
{
	int tile,color;

	tile=m_slapfight_videoram[tile_index] + ((m_slapfight_colorram[tile_index] & 0x0f) << 8);
	color=(m_slapfight_colorram[tile_index] & 0xf0) >> 4;

	SET_TILE_INFO_MEMBER(
			1,
			tile,
			color,
			0);
}

TILE_GET_INFO_MEMBER(slapfght_state::get_fix_tile_info)
{
	int tile,color;

	tile=m_slapfight_fixvideoram[tile_index] + ((m_slapfight_fixcolorram[tile_index] & 0x03) << 8);
	color=(m_slapfight_fixcolorram[tile_index] & 0xfc) >> 2;

	SET_TILE_INFO_MEMBER(
			0,
			tile,
			color,
			0);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START_MEMBER(slapfght_state,perfrman)
{
	m_pf1_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(slapfght_state::get_pf_tile_info),this),TILEMAP_SCAN_ROWS,8,8,64,32);

	m_pf1_tilemap->set_transparent_pen(0);
}

VIDEO_START_MEMBER(slapfght_state,slapfight)
{
	m_pf1_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(slapfght_state::get_pf1_tile_info),this),TILEMAP_SCAN_ROWS,8,8,64,32);
	m_fix_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(slapfght_state::get_fix_tile_info),this),TILEMAP_SCAN_ROWS,8,8,64,32);

	m_fix_tilemap->set_transparent_pen(0);
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_MEMBER(slapfght_state::slapfight_videoram_w)
{
	m_slapfight_videoram[offset]=data;
	m_pf1_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(slapfght_state::slapfight_colorram_w)
{
	m_slapfight_colorram[offset]=data;
	m_pf1_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(slapfght_state::slapfight_fixram_w)
{
	m_slapfight_fixvideoram[offset]=data;
	m_fix_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(slapfght_state::slapfight_fixcol_w)
{
	m_slapfight_fixcolorram[offset]=data;
	m_fix_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(slapfght_state::slapfight_flipscreen_w)
{
	logerror("Writing %02x to flipscreen\n",offset);
	if (offset==0) m_flipscreen=1; /* Port 0x2 is flipscreen */
	else m_flipscreen=0; /* Port 0x3 is normal */
}

WRITE8_MEMBER(slapfght_state::slapfight_palette_bank_w)
{
	m_slapfight_palette_bank = offset;
}

static void slapfght_log_vram(running_machine &machine)
{
#ifdef MAME_DEBUG
	slapfght_state *state = machine.driver_data<slapfght_state>();
	if ( machine.input().code_pressed_once(KEYCODE_B) )
	{
		int i;
		for (i=0; i<0x800; i++)
		{
			logerror("Offset:%03x   TileRAM:%02x   AttribRAM:%02x   SpriteRAM:%02x\n",i, state->m_slapfight_videoram[i],state->m_slapfight_colorram[i],state->m_spriteram->live()[i]);
		}
	}
#endif
}

/***************************************************************************

  Render the Sprites

***************************************************************************/
static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int priority_to_display )
{
	slapfght_state *state = machine.driver_data<slapfght_state>();
	UINT8 *buffered_spriteram = state->m_spriteram->buffer();
	int offs;

	for (offs = 0;offs < state->m_spriteram->bytes();offs += 4)
	{
		int sx, sy;

		if ((buffered_spriteram[offs+2] & 0x80) == priority_to_display)
		{
			if (state->m_flipscreen)
			{
				sx = 265 - buffered_spriteram[offs+1];
				sy = 239 - buffered_spriteram[offs+3];
				sy &= 0xff;
			}
			else
			{
				sx = buffered_spriteram[offs+1] + 3;
				sy = buffered_spriteram[offs+3] - 1;
			}
			drawgfx_transpen(bitmap,cliprect,machine.gfx[1],
				buffered_spriteram[offs],
				((buffered_spriteram[offs+2] >> 1) & 3)	|
					((buffered_spriteram[offs+2] << 2) & 4) | (state->m_slapfight_palette_bank << 3),
				state->m_flipscreen, state->m_flipscreen,
				sx, sy,0);
		}
	}
}


UINT32 slapfght_state::screen_update_perfrman(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_pf1_tilemap->set_flip(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	m_pf1_tilemap ->set_scrolly(0 , 0 );
	if (m_flipscreen) {
		m_pf1_tilemap ->set_scrollx(0 , 264 );
	}
	else {
		m_pf1_tilemap ->set_scrollx(0 , -16 );
	}

	m_pf1_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_OPAQUE,0);
	draw_sprites(machine(), bitmap,cliprect,0);
	m_pf1_tilemap->draw(bitmap, cliprect, 0,0);
	draw_sprites(machine(), bitmap,cliprect,0x80);

	slapfght_log_vram(machine());
	return 0;
}


UINT32 slapfght_state::screen_update_slapfight(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	UINT8 *buffered_spriteram = m_spriteram->buffer();
	int offs;

	machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	if (m_flipscreen) {
		m_fix_tilemap->set_scrollx(0,296);
		m_pf1_tilemap->set_scrollx(0,(*m_slapfight_scrollx_lo + 256 * *m_slapfight_scrollx_hi)+296 );
		m_pf1_tilemap->set_scrolly(0, (*m_slapfight_scrolly)+15 );
		m_fix_tilemap->set_scrolly(0, -1 ); /* Glitch in Tiger Heli otherwise */
	}
	else {
		m_fix_tilemap->set_scrollx(0,0);
		m_pf1_tilemap->set_scrollx(0,(*m_slapfight_scrollx_lo + 256 * *m_slapfight_scrollx_hi) );
		m_pf1_tilemap->set_scrolly(0, (*m_slapfight_scrolly)-1 );
		m_fix_tilemap->set_scrolly(0, -1 ); /* Glitch in Tiger Heli otherwise */
	}

	m_pf1_tilemap->draw(bitmap, cliprect, 0,0);

	/* Draw the sprites */
	for (offs = 0;offs < m_spriteram->bytes();offs += 4)
	{
		if (m_flipscreen)
			drawgfx_transpen(bitmap,cliprect,machine().gfx[2],
				buffered_spriteram[offs] + ((buffered_spriteram[offs+2] & 0xc0) << 2),
				(buffered_spriteram[offs+2] & 0x1e) >> 1,
				1,1,
				288-(buffered_spriteram[offs+1] + ((buffered_spriteram[offs+2] & 0x01) << 8)) +18,240-buffered_spriteram[offs+3],0);
		else
			drawgfx_transpen(bitmap,cliprect,machine().gfx[2],
				buffered_spriteram[offs] + ((buffered_spriteram[offs+2] & 0xc0) << 2),
				(buffered_spriteram[offs+2] & 0x1e) >> 1,
				0,0,
				(buffered_spriteram[offs+1] + ((buffered_spriteram[offs+2] & 0x01) << 8)) - 13,buffered_spriteram[offs+3],0);
	}

	m_fix_tilemap->draw(bitmap, cliprect, 0,0);

	slapfght_log_vram(machine());
	return 0;
}
