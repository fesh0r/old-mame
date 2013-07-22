#include "emu.h"
#include "includes/88games.h"


/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

void _88games_tile_callback( running_machine &machine, int layer, int bank, int *code, int *color, int *flags, int *priority )
{
	_88games_state *state = machine.driver_data<_88games_state>();

	*code |= ((*color & 0x0f) << 8) | (bank << 12);
	*color = state->m_layer_colorbase[layer] + ((*color & 0xf0) >> 4);
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

void _88games_sprite_callback( running_machine &machine, int *code, int *color, int *priority, int *shadow )
{
	_88games_state *state = machine.driver_data<_88games_state>();

	*priority = (*color & 0x20) >> 5;   /* ??? */
	*color = state->m_sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

void _88games_zoom_callback( running_machine &machine, int *code, int *color, int *flags )
{
	_88games_state *state = machine.driver_data<_88games_state>();

	*flags = (*color & 0x40) ? TILE_FLIPX : 0;
	*code |= ((*color & 0x07) << 8);
	*color = state->m_zoom_colorbase + ((*color & 0x38) >> 3) + ((*color & 0x80) >> 4);
}

/***************************************************************************

  Display refresh

***************************************************************************/

UINT32 _88games_state::screen_update_88games(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_k052109->tilemap_update();

	if (m_k88games_priority)
	{
		m_k052109->tilemap_draw(bitmap, cliprect, 0, TILEMAP_DRAW_OPAQUE, 0);   // tile 0
		m_k051960->k051960_sprites_draw(bitmap,cliprect, 1, 1);
		m_k052109->tilemap_draw(bitmap, cliprect, 2, 0, 0); // tile 2
		m_k052109->tilemap_draw(bitmap, cliprect, 1, 0, 0); // tile 1
		m_k051960->k051960_sprites_draw(bitmap, cliprect, 0, 0);
		m_k051316->zoom_draw(bitmap, cliprect, 0, 0);
	}
	else
	{
		m_k052109->tilemap_draw(bitmap, cliprect, 2, TILEMAP_DRAW_OPAQUE, 0);   // tile 2
		m_k051316->zoom_draw(bitmap, cliprect, 0, 0);
		m_k051960->k051960_sprites_draw(bitmap, cliprect, 0, 0);
		m_k052109->tilemap_draw(bitmap, cliprect, 1, 0, 0); // tile 1
		m_k051960->k051960_sprites_draw(bitmap, cliprect, 1, 1);
		m_k052109->tilemap_draw(bitmap, cliprect, 0, 0, 0); // tile 0
	}

	return 0;
}
