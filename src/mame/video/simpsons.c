#include "emu.h"

#include "includes/simpsons.h"

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

void simpsons_tile_callback( running_machine &machine, int layer, int bank, int *code, int *color, int *flags, int *priority )
{
	simpsons_state *state = machine.driver_data<simpsons_state>();
	*code |= ((*color & 0x3f) << 8) | (bank << 14);
	*color = state->m_layer_colorbase[layer] + ((*color & 0xc0) >> 6);
}


/***************************************************************************

  Callbacks for the K053247

***************************************************************************/

void simpsons_sprite_callback( running_machine &machine, int *code, int *color, int *priority_mask )
{
	simpsons_state *state = machine.driver_data<simpsons_state>();
	int pri = (*color & 0x0f80) >> 6;   /* ??????? */

	if (pri <= state->m_layerpri[2])
		*priority_mask = 0;
	else if (pri > state->m_layerpri[2] && pri <= state->m_layerpri[1])
		*priority_mask = 0xf0;
	else if (pri > state->m_layerpri[1] && pri <= state->m_layerpri[0])
		*priority_mask = 0xf0 | 0xcc;
	else
		*priority_mask = 0xf0 | 0xcc | 0xaa;

	*color = state->m_sprite_colorbase + (*color & 0x001f);
}


/***************************************************************************

  Extra video banking

***************************************************************************/

READ8_MEMBER(simpsons_state::simpsons_k052109_r)
{
	return m_k052109->read(space, offset + 0x2000);
}

WRITE8_MEMBER(simpsons_state::simpsons_k052109_w)
{
	m_k052109->write(space, offset + 0x2000, data);
}

READ8_MEMBER(simpsons_state::simpsons_k053247_r)
{
	int offs;

	if (offset < 0x1000)
	{
		offs = offset >> 1;

		if (offset & 1)
			return(m_spriteram[offs] & 0xff);
		else
			return(m_spriteram[offs] >> 8);
	}
	else
		return m_xtraram[offset - 0x1000];
}

WRITE8_MEMBER(simpsons_state::simpsons_k053247_w)
{
	int offs;

	if (offset < 0x1000)
	{
		UINT16 *spriteram = m_spriteram;
		offs = offset >> 1;

		if (offset & 1)
			spriteram[offs] = (spriteram[offs] & 0xff00) | data;
		else
			spriteram[offs] = (spriteram[offs] & 0x00ff) | (data << 8);
	}
	else m_xtraram[offset - 0x1000] = data;
}

void simpsons_state::simpsons_video_banking( int bank )
{
	address_space &space = m_maincpu->space(AS_PROGRAM);

	if (bank & 1)
	{
		space.install_read_bank(0x0000, 0x0fff, "bank5");
		space.install_write_handler(0x0000, 0x0fff, write8_delegate(FUNC(simpsons_state::paletteram_xBBBBBGGGGGRRRRR_byte_be_w), this));
		membank("bank5")->set_base(m_generic_paletteram_8);
	}
	else
		space.install_readwrite_handler(0x0000, 0x0fff, read8_delegate(FUNC(k052109_device::read), (k052109_device*)m_k052109), write8_delegate(FUNC(k052109_device::write), (k052109_device*)m_k052109));

	if (bank & 2)
		space.install_readwrite_handler(0x2000, 0x3fff, read8_delegate(FUNC(simpsons_state::simpsons_k053247_r),this), write8_delegate(FUNC(simpsons_state::simpsons_k053247_w),this));
	else
		space.install_readwrite_handler(0x2000, 0x3fff, read8_delegate(FUNC(simpsons_state::simpsons_k052109_r),this), write8_delegate(FUNC(simpsons_state::simpsons_k052109_w),this));
}



/***************************************************************************

  Display refresh

***************************************************************************/

UINT32 simpsons_state::screen_update_simpsons(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int layer[3], bg_colorbase;

	bg_colorbase = m_k053251->get_palette_index(K053251_CI0);
	m_sprite_colorbase = m_k053251->get_palette_index(K053251_CI1);
	m_layer_colorbase[0] = m_k053251->get_palette_index(K053251_CI2);
	m_layer_colorbase[1] = m_k053251->get_palette_index(K053251_CI3);
	m_layer_colorbase[2] = m_k053251->get_palette_index(K053251_CI4);

	m_k052109->tilemap_update();

	layer[0] = 0;
	m_layerpri[0] = m_k053251->get_priority(K053251_CI2);
	layer[1] = 1;
	m_layerpri[1] = m_k053251->get_priority(K053251_CI3);
	layer[2] = 2;
	m_layerpri[2] = m_k053251->get_priority(K053251_CI4);

	konami_sortlayers3(layer, m_layerpri);

	screen.priority().fill(0, cliprect);
	bitmap.fill(16 * bg_colorbase, cliprect);
	m_k052109->tilemap_draw(screen, bitmap, cliprect, layer[0], 0, 1);
	m_k052109->tilemap_draw(screen, bitmap, cliprect, layer[1], 0, 2);
	m_k052109->tilemap_draw(screen, bitmap, cliprect, layer[2], 0, 4);

	m_k053246->k053247_sprites_draw(bitmap, cliprect);
	return 0;
}
