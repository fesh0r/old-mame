/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/punchout.h"


DRIVER_INIT_MEMBER(punchout_state,punchout)
{
	m_palette_reverse_top = 0x00;
	m_palette_reverse_bot = 0xff;
}

DRIVER_INIT_MEMBER(punchout_state,spnchout)
{
	m_palette_reverse_top = 0x00;
	m_palette_reverse_bot = 0xff;
}

DRIVER_INIT_MEMBER(punchout_state,spnchotj)
{
	m_palette_reverse_top = 0xff;
	m_palette_reverse_bot = 0xff;
}

DRIVER_INIT_MEMBER(punchout_state,armwrest)
{
	m_palette_reverse_top = 0x00;
	m_palette_reverse_bot = 0x00;
}




/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

TILE_GET_INFO_MEMBER(punchout_state::top_get_info)
{
	int attr = m_bg_top_videoram[tile_index*2 + 1];
	int code = m_bg_top_videoram[tile_index*2] + ((attr & 0x03) << 8);
	int color = ((attr & 0x7c) >> 2);
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(0, code, color, flipx ? TILE_FLIPX : 0);
}

TILE_GET_INFO_MEMBER(punchout_state::armwrest_top_get_info)
{
	int attr = m_bg_top_videoram[tile_index*2 + 1];
	int code = m_bg_top_videoram[tile_index*2] + ((attr & 0x03) << 8) + ((attr & 0x80) << 3);
	int color = ((attr & 0x7c) >> 2);
	SET_TILE_INFO_MEMBER(0, code, color, 0);
}

TILE_GET_INFO_MEMBER(punchout_state::bot_get_info)
{
	int attr = m_bg_bot_videoram[tile_index*2 + 1];
	int code = m_bg_bot_videoram[tile_index*2] + ((attr & 0x03) << 8);
	int color = ((attr & 0x7c) >> 2);
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(1, code, color, flipx ? TILE_FLIPX : 0);
}

TILE_GET_INFO_MEMBER(punchout_state::armwrest_bot_get_info)
{
	int attr = m_bg_bot_videoram[tile_index*2 + 1];
	int code = m_bg_bot_videoram[tile_index*2] + ((attr & 0x03) << 8);
	int color = ((attr & 0x7c) >> 2) + 0x40;
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(0, code, color, flipx ? TILE_FLIPX : 0);
}

TILE_GET_INFO_MEMBER(punchout_state::bs1_get_info)
{
	int attr = m_spr1_videoram[tile_index*4 + 3];
	int code = m_spr1_videoram[tile_index*4] + ((m_spr1_videoram[tile_index*4 + 1] & 0x1f) << 8);
	int color = attr & 0x1f;
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(2, code, color, flipx ? TILE_FLIPX : 0);
}

TILE_GET_INFO_MEMBER(punchout_state::bs2_get_info)
{
	int attr = m_spr2_videoram[tile_index*4 + 3];
	int code = m_spr2_videoram[tile_index*4] + ((m_spr2_videoram[tile_index*4 + 1] & 0x0f) << 8);
	int color = attr & 0x3f;
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(3, code, color, flipx ? TILE_FLIPX : 0);
}

TILE_GET_INFO_MEMBER(punchout_state::armwrest_fg_get_info)
{
	int attr = m_armwrest_fg_videoram[tile_index*2 + 1];
	int code = m_armwrest_fg_videoram[tile_index*2] + 256 * (attr & 0x07);
	int color = ((attr & 0xf8) >> 3);
	int flipx = attr & 0x80;
	SET_TILE_INFO_MEMBER(1, code, color, flipx ? TILE_FLIPX : 0);
}

TILEMAP_MAPPER_MEMBER(punchout_state::armwrest_bs1_scan)
{
	int halfcols = num_cols/2;
	return (col/halfcols)*(halfcols*num_rows) + row*halfcols + col%halfcols;
}

TILEMAP_MAPPER_MEMBER(punchout_state::armwrest_bs1_scan_flipx)
{
	int halfcols = num_cols/2;
	col ^=0x10;
	return (col/halfcols)*(halfcols*num_rows) + row*halfcols + col%halfcols;
}


void punchout_state::video_start()
{
	m_bg_top_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::top_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 32,32);
	m_bg_bot_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bot_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 64,32);
	m_bg_bot_tilemap->set_scroll_rows(32);

	m_spr1_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bs1_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 16,32);
	m_spr2_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bs2_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 16,32);

	m_fg_tilemap = NULL;

	m_spr1_tilemap->set_transparent_pen(0x07);
	m_spr2_tilemap->set_transparent_pen(0x03);
}


VIDEO_START_MEMBER(punchout_state,armwrest)
{
	m_bg_top_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::armwrest_top_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 32,32);
	m_bg_bot_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::armwrest_bot_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 32,32);

	m_spr1_tilemap =       &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bs1_get_info),this), tilemap_mapper_delegate(FUNC(punchout_state::armwrest_bs1_scan),this),  8,8, 32,16);
	m_spr1_tilemap_flipx = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bs1_get_info),this), tilemap_mapper_delegate(FUNC(punchout_state::armwrest_bs1_scan_flipx),this),  8,8, 32,16);
	m_spr2_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::bs2_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 16,32);
	m_fg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(punchout_state::armwrest_fg_get_info),this), TILEMAP_SCAN_ROWS,  8,8, 32,32);

	m_spr1_tilemap->set_transparent_pen(0x07);
	m_spr1_tilemap_flipx->set_transparent_pen(0x07);
	m_spr2_tilemap->set_transparent_pen(0x03);
	m_fg_tilemap->set_transparent_pen(0x07);
}



WRITE8_MEMBER(punchout_state::punchout_bg_top_videoram_w)
{
	m_bg_top_videoram[offset] = data;
	m_bg_top_tilemap->mark_tile_dirty(offset/2);
}

WRITE8_MEMBER(punchout_state::punchout_bg_bot_videoram_w)
{
	m_bg_bot_videoram[offset] = data;
	m_bg_bot_tilemap->mark_tile_dirty(offset/2);
}

WRITE8_MEMBER(punchout_state::armwrest_fg_videoram_w)
{
	m_armwrest_fg_videoram[offset] = data;
	m_fg_tilemap->mark_tile_dirty(offset/2);
}

WRITE8_MEMBER(punchout_state::punchout_spr1_videoram_w)
{
	m_spr1_videoram[offset] = data;
	m_spr1_tilemap->mark_tile_dirty(offset/4);
	if (m_spr1_tilemap_flipx)
		m_spr1_tilemap_flipx->mark_tile_dirty(offset/4);
}

WRITE8_MEMBER(punchout_state::punchout_spr2_videoram_w)
{
	m_spr2_videoram[offset] = data;
	m_spr2_tilemap->mark_tile_dirty(offset/4);
}



static void draw_big_sprite(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int palette)
{
	punchout_state *state = machine.driver_data<punchout_state>();
	int zoom;

	zoom = state->m_spr1_ctrlram[0] + 256 * (state->m_spr1_ctrlram[1] & 0x0f);
	if (zoom)
	{
		int sx,sy;
		UINT32 startx,starty;
		int incxx,incyy;

		sx = 4096 - (state->m_spr1_ctrlram[2] + 256 * (state->m_spr1_ctrlram[3] & 0x0f));
		if (sx > 4096-4*127) sx -= 4096;

		sy = -(state->m_spr1_ctrlram[4] + 256 * (state->m_spr1_ctrlram[5] & 1));
		if (sy <= -256 + zoom/0x40) sy += 512;
		sy += 12;

		incxx = zoom << 6;
		incyy = zoom << 6;

		startx = -sx * 0x4000;
		starty = -sy * 0x10000;
		startx += 3740 * zoom;  /* adjustment to match the screen shots */
		starty -= 178 * zoom;   /* and make the hall of fame picture nice */

		if (state->m_spr1_ctrlram[6] & 1)   /* flip x */
		{
			startx = ((16 * 8) << 16) - startx - 1;
			incxx = -incxx;
		}

		state->m_spr1_tilemap->set_palette_offset(0x100 * palette);

		state->m_spr1_tilemap->draw_roz(bitmap, cliprect,
			startx,starty + 0x200*(2) * zoom,
			incxx,0,0,incyy,    /* zoom, no rotation */
			0,  /* no wraparound */
			0,0);
	}
}


static void armwrest_draw_big_sprite(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect, int palette)
{
	punchout_state *state = machine.driver_data<punchout_state>();
	int zoom;

	zoom = state->m_spr1_ctrlram[0] + 256 * (state->m_spr1_ctrlram[1] & 0x0f);
	if (zoom)
	{
		int sx,sy;
		UINT32 startx,starty;
		int incxx,incyy;
		tilemap_t *_tilemap;

		sx = 4096 - (state->m_spr1_ctrlram[2] + 256 * (state->m_spr1_ctrlram[3] & 0x0f));
		if (sx > 2048) sx -= 4096;

		sy = -(state->m_spr1_ctrlram[4] + 256 * (state->m_spr1_ctrlram[5] & 1));
		if (sy <= -256 + zoom/0x40) sy += 512;
		sy += 12;

		incxx = zoom << 6;
		incyy = zoom << 6;

		startx = -sx * 0x4000;
		starty = -sy * 0x10000;
		startx += 3740 * zoom;  /* adjustment to match the screen shots */
		starty -= 178 * zoom;   /* and make the hall of fame picture nice */

		if (state->m_spr1_ctrlram[6] & 1)   /* flip x */
		{
			_tilemap = state->m_spr1_tilemap_flipx;
			startx = ((32 * 8) << 16) - startx - 1;
			incxx = -incxx;
		}
		else
			_tilemap = state->m_spr1_tilemap;

		_tilemap->set_palette_offset(0x100 * palette);

		_tilemap->draw_roz(bitmap, cliprect,
			startx,starty + 0x200*(2) * zoom,
			incxx,0,0,incyy,    /* zoom, no rotation */
			0,  /* no wraparound */
			0,0);
	}
}

static void drawbs2(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	punchout_state *state = machine.driver_data<punchout_state>();
	int sx,sy;
	int incxx;

	sx = 512 - (state->m_spr2_ctrlram[0] + 256 * (state->m_spr2_ctrlram[1] & 1));
	if (sx > 512-127) sx -= 512;
	sx -= 55;   /* adjustment to match the screen shots */

	sy = -state->m_spr2_ctrlram[2] + 256 * (state->m_spr2_ctrlram[3] & 1);
	sy += 3;    /* adjustment to match the screen shots */

	sx = -sx << 16;
	sy = -sy << 16;

	if (state->m_spr2_ctrlram[4] & 1)   /* flip x */
	{
		sx = ((16 * 8) << 16) - sx - 1;
		incxx = -1;
	}
	else
		incxx = 1;

	// this tilemap doesn't actually zoom, but draw_roz is the only way to draw it without wraparound
	state->m_spr2_tilemap->draw_roz(bitmap, cliprect,
		sx, sy, incxx << 16, 0, 0, 1 << 16,
		0, 0, 0);
}



static void punchout_copy_top_palette(running_machine &machine, int bank)
{
	punchout_state *state = machine.driver_data<punchout_state>();
	int i;
	const UINT8 *color_prom = state->memregion("proms")->base();

	// top monitor palette
	for (i = 0; i < 0x100; i++)
	{
		int base = 0x100 * bank;
		int r, g, b;

		r = 255 - pal4bit(color_prom[i + 0x000 + base]);
		g = 255 - pal4bit(color_prom[i + 0x200 + base]);
		b = 255 - pal4bit(color_prom[i + 0x400 + base]);

		palette_set_color(machine, i ^ state->m_palette_reverse_top, MAKE_RGB(r, g, b));
	}
}

static void punchout_copy_bot_palette(running_machine &machine, int bank)
{
	punchout_state *state = machine.driver_data<punchout_state>();
	int i;
	const UINT8 *color_prom = state->memregion("proms")->base() + 0x600;

	// bottom monitor palette
	for (i = 0; i < 0x100; i++)
	{
		int base = 0x100 * bank;
		int r, g, b;

		r = 255 - pal4bit(color_prom[i + 0x000 + base]);
		g = 255 - pal4bit(color_prom[i + 0x200 + base]);
		b = 255 - pal4bit(color_prom[i + 0x400 + base]);

		palette_set_color(machine, (i ^ state->m_palette_reverse_bot) + 0x100, MAKE_RGB(r, g, b));
	}
}


UINT32 punchout_state::screen_update_punchout_top(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{

	punchout_copy_top_palette(machine(), BIT(*m_palettebank,1));

	m_bg_top_tilemap->draw(bitmap, cliprect, 0, 0);

	if (m_spr1_ctrlram[7] & 1)  /* display in top monitor */
		draw_big_sprite(machine(), bitmap, cliprect, 0);

	return 0;
}

UINT32 punchout_state::screen_update_punchout_bottom(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int offs;

	punchout_copy_bot_palette(machine(), BIT(*m_palettebank,0));

	/* copy the character mapped graphics */
	for (offs = 0;offs < 32;offs++)
		m_bg_bot_tilemap->set_scrollx(offs, 58 + m_bg_bot_videoram[2*offs] + 256 * (m_bg_bot_videoram[2*offs + 1] & 0x01));

	m_bg_bot_tilemap->draw(bitmap, cliprect, 0, 0);

	if (m_spr1_ctrlram[7] & 2)  /* display in bottom monitor */
		draw_big_sprite(machine(), bitmap, cliprect, 1);
	drawbs2(machine(), bitmap, cliprect);

	return 0;
}


UINT32 punchout_state::screen_update_armwrest_top(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{

	punchout_copy_top_palette(machine(), BIT(*m_palettebank,1));

	m_bg_top_tilemap->draw(bitmap, cliprect, 0, 0);

	if (m_spr1_ctrlram[7] & 1)  /* display in top monitor */
		armwrest_draw_big_sprite(machine(), bitmap, cliprect, 0);

	return 0;
}

UINT32 punchout_state::screen_update_armwrest_bottom(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{

	punchout_copy_bot_palette(machine(), BIT(*m_palettebank,0));

	m_bg_bot_tilemap->draw(bitmap, cliprect, 0, 0);

	if (m_spr1_ctrlram[7] & 2)  /* display in bottom monitor */
		armwrest_draw_big_sprite(machine(), bitmap, cliprect, 1);
	drawbs2(machine(), bitmap, cliprect);

	m_fg_tilemap->draw(bitmap, cliprect, 0, 0);

	return 0;
}
