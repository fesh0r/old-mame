/* Fit of Fighting Video Hardware */

#include "emu.h"
#include "includes/fitfight.h"


static void draw_sprites( running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int layer )
{
	fitfight_state *state = machine.driver_data<fitfight_state>();
	gfx_element *gfx = machine.gfx[3];
	UINT16 *source = state->m_spriteram;
	UINT16 *finish = source + 0x800 / 2;

	while (source < finish)
	{
		int xpos, ypos, number, xflip, yflip, end, colr, prio;

		ypos = source[0];
		xpos = source[3];
		number = source[2];
		xflip = (source[1] & 0x0001) ^ 0x0001;
		yflip = (source[1] & 0x0002);
		prio = (source[1] & 0x0400) >> 10;
		colr = (source[1] & 0x00fc) >> 2;

		if (state->m_bbprot_kludge == 1)
			colr = (source[1] & 0x00f8) >> 3;

		end = source[0] & 0x8000;

		ypos = 0xff - ypos;

		xpos -= 38;//48;
		ypos -= 14;//16;

		if (end) break;
		if (prio == layer)
			drawgfx_transpen(bitmap, cliprect, gfx, number, colr, xflip, yflip, xpos, ypos, 0);

		source += 4;
	}
}

TILE_GET_INFO_MEMBER(fitfight_state::get_fof_bak_tile_info)
{
	int code = m_fof_bak_tileram[tile_index * 2 + 1];
	int colr = m_fof_bak_tileram[tile_index * 2] & 0x1f;
	int xflip = (m_fof_bak_tileram[tile_index * 2] & 0x0020) >> 5;
	xflip ^= 1;

	SET_TILE_INFO_MEMBER(2, code, colr, TILE_FLIPYX(xflip));
}

WRITE16_MEMBER(fitfight_state::fof_bak_tileram_w)
{

	COMBINE_DATA(&m_fof_bak_tileram[offset]);
	m_fof_bak_tilemap->mark_tile_dirty(offset / 2);
}


TILE_GET_INFO_MEMBER(fitfight_state::get_fof_mid_tile_info)
{
	int code = m_fof_mid_tileram[tile_index * 2 + 1];
	int colr = m_fof_mid_tileram[tile_index * 2] & 0x1f;
	int xflip = (m_fof_mid_tileram[tile_index * 2] & 0x0020) >> 5;
	xflip ^= 1;

	SET_TILE_INFO_MEMBER(1, code, colr, TILE_FLIPYX(xflip));
}

WRITE16_MEMBER(fitfight_state::fof_mid_tileram_w)
{

	COMBINE_DATA(&m_fof_mid_tileram[offset]);
	m_fof_mid_tilemap->mark_tile_dirty(offset / 2);
}

TILE_GET_INFO_MEMBER(fitfight_state::get_fof_txt_tile_info)
{
	int code = m_fof_txt_tileram[tile_index * 2 + 1];
	int colr = m_fof_txt_tileram[tile_index * 2] & 0x1f;
	int xflip = (m_fof_txt_tileram[tile_index * 2] & 0x0020) >> 5;
	xflip ^= 1;

	SET_TILE_INFO_MEMBER(0, code, colr, TILE_FLIPYX(xflip));
}

WRITE16_MEMBER(fitfight_state::fof_txt_tileram_w)
{

	COMBINE_DATA(&m_fof_txt_tileram[offset]);
	m_fof_txt_tilemap->mark_tile_dirty(offset / 2);
}

/* video start / update */

void fitfight_state::video_start()
{
	m_fof_bak_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(fitfight_state::get_fof_bak_tile_info),this), TILEMAP_SCAN_COLS, 8, 8, 128, 32);
	/* opaque */

	m_fof_mid_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(fitfight_state::get_fof_mid_tile_info),this), TILEMAP_SCAN_COLS, 8, 8, 128, 32);
	m_fof_mid_tilemap->set_transparent_pen(0);

	m_fof_txt_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(fitfight_state::get_fof_txt_tile_info),this), TILEMAP_SCAN_COLS, 8, 8, 128, 32);
	m_fof_txt_tilemap->set_transparent_pen(0);
}

UINT32 fitfight_state::screen_update_fitfight(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{

	/* scroll isn't right */

	int vblank;
	int scrollbak, scrollmid;

	vblank = (m_fof_700000[0] & 0x8000);

	if (vblank > 0)
		bitmap.fill(get_black_pen(machine()), cliprect);
	else {
//      if (machine().input().code_pressed(KEYCODE_Q))
//          scrollbak = ((m_fof_a00000[0] & 0xff00) >> 5) - ((m_fof_700000[0] & 0x0038) >> 3);
//      else if (machine().input().code_pressed(KEYCODE_W))
//          scrollbak = ((m_fof_a00000[0] & 0xff00) >> 5) + ((m_fof_700000[0] & 0x01c0) >> 6);
//      else if (machine().input().code_pressed(KEYCODE_E))
//          scrollbak = ((m_fof_a00000[0] & 0xff00) >> 5) - ((m_fof_700000[0] & 0x01c0) >> 6);
//      else if (machine().input().code_pressed(KEYCODE_R))
//          scrollbak = ((m_fof_a00000[0] & 0xff00) >> 5) + ((m_fof_700000[0] & 0x0038) >> 3);
//      else
		scrollbak = ((m_fof_a00000[0] & 0xffe0) >> 5);
		m_fof_bak_tilemap->set_scrollx(0, scrollbak );
		m_fof_bak_tilemap->set_scrolly(0, m_fof_a00000[0] & 0xff);
		m_fof_bak_tilemap->draw(bitmap, cliprect, 0, 0);

		draw_sprites(machine(), bitmap, cliprect, 0);

//      if (machine().input().code_pressed(KEYCODE_A))
//          scrollmid = ((m_fof_900000[0] & 0xff00) >> 5) - ((m_fof_700000[0] & 0x01c0) >> 6);
//      else if (machine().input().code_pressed(KEYCODE_S))
//          scrollmid = ((m_fof_900000[0] & 0xff00) >> 5) + ((m_fof_700000[0] & 0x0038) >> 3);
//      else if (machine().input().code_pressed(KEYCODE_D))
//          scrollmid = ((m_fof_900000[0] & 0xff00) >> 5) - ((m_fof_700000[0] & 0x0038) >> 3);
//      else if (machine().input().code_pressed(KEYCODE_F))
//          scrollmid = ((m_fof_900000[0] & 0xff00) >> 5) + ((m_fof_700000[0] & 0x01c0) >> 6);
//      else
		scrollmid = ((m_fof_900000[0] & 0xffe0) >> 5);
		m_fof_mid_tilemap->set_scrollx(0, scrollmid );
		m_fof_mid_tilemap->set_scrolly(0, m_fof_900000[0] & 0xff);
//      if (!machine().input().code_pressed(KEYCODE_F))
		m_fof_mid_tilemap->draw(bitmap, cliprect, 0, 0);

		draw_sprites(machine(), bitmap, cliprect, 1);

		m_fof_txt_tilemap->draw(bitmap, cliprect, 0, 0);
	}
/*  popmessage ("Regs %04x %04x %04x %04x %04x %04x",
            m_fof_100000[0], m_fof_600000[0], m_fof_700000[0],
            m_fof_800000[0], m_fof_900000[0],
            m_fof_a00000[0] );
*/
	return 0;
}
