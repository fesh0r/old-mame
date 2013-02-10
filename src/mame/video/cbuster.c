/***************************************************************************

   Crude Buster Video emulation - Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "emu.h"
#include "includes/cbuster.h"
#include "video/deco16ic.h"

/******************************************************************************/

/* maybe the game should just use generic palette handling, and have a darker palette by design... */

void cbuster_state::update_24bitcol( int offset )
{
	UINT8 r, g, b; /* The highest palette value seems to be 0x8e */

	r = (UINT8)((float)((m_generic_paletteram_16[offset]  >> 0) & 0xff) * 1.75);
	g = (UINT8)((float)((m_generic_paletteram_16[offset]  >> 8) & 0xff) * 1.75);
	b = (UINT8)((float)((m_generic_paletteram2_16[offset] >> 0) & 0xff) * 1.75);

	palette_set_color(machine(), offset, MAKE_RGB(r, g, b));
}

WRITE16_MEMBER(cbuster_state::twocrude_palette_24bit_rg_w)
{
	COMBINE_DATA(&m_generic_paletteram_16[offset]);
	update_24bitcol(offset);
}

WRITE16_MEMBER(cbuster_state::twocrude_palette_24bit_b_w)
{
	COMBINE_DATA(&m_generic_paletteram2_16[offset]);
	update_24bitcol(offset);
}


/******************************************************************************/


/******************************************************************************/

void cbuster_state::video_start()
{
	m_sprgen->alloc_sprite_bitmap();
}

UINT32 cbuster_state::screen_update_twocrude(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	address_space &space = machine().driver_data()->generic_space();
	UINT16 flip = deco16ic_pf_control_r(m_deco_tilegen1, space, 0, 0xffff);

	flip_screen_set(!BIT(flip, 7));

	m_sprgen->draw_sprites(bitmap, cliprect, m_spriteram16_buffer, 0x400);


	deco16ic_pf_update(m_deco_tilegen1, m_pf1_rowscroll, m_pf2_rowscroll);
	deco16ic_pf_update(m_deco_tilegen2, m_pf3_rowscroll, m_pf4_rowscroll);

	/* Draw playfields & sprites */
	deco16ic_tilemap_2_draw(m_deco_tilegen2, bitmap, cliprect, TILEMAP_DRAW_OPAQUE, 0);
	m_sprgen->inefficient_copy_sprite_bitmap(bitmap, cliprect, 0x0800, 0x0900, 0x100, 0x0ff);
	m_sprgen->inefficient_copy_sprite_bitmap(bitmap, cliprect, 0x0900, 0x0900, 0x500, 0x0ff);

	if (m_pri)
	{
		deco16ic_tilemap_2_draw(m_deco_tilegen1, bitmap, cliprect, 0, 0);
		deco16ic_tilemap_1_draw(m_deco_tilegen2, bitmap, cliprect, 0, 0);
	}
	else
	{
		deco16ic_tilemap_1_draw(m_deco_tilegen2, bitmap, cliprect, 0, 0);
		deco16ic_tilemap_2_draw(m_deco_tilegen1, bitmap, cliprect, 0, 0);
	}

	m_sprgen->inefficient_copy_sprite_bitmap(bitmap, cliprect, 0x0000, 0x0900, 0x100, 0x0ff);
	m_sprgen->inefficient_copy_sprite_bitmap(bitmap, cliprect, 0x0100, 0x0900, 0x500, 0x0ff);
	deco16ic_tilemap_1_draw(m_deco_tilegen1, bitmap, cliprect, 0, 0);
	return 0;
}
