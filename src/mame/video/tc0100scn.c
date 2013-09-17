/*
Taito TC0100SCN
---------
Tilemap generator. The front tilemap fetches gfx data from RAM,
the others use ROMs as usual.

Standard memory layout (three 64x64 tilemaps with 8x8 tiles)

0000-3fff BG0
4000-5fff FG0
6000-6fff gfx data for FG0
7000-7fff unused (probably)
8000-bfff BG1
c000-c3ff BG0 rowscroll (second half unused*)
c400-c7ff BG1 rowscroll (second half unused*)
c800-dfff unused (probably)
e000-e0ff BG0 colscroll [see info below]
e100-ffff unused (probably)

Double width tilemaps memory layout (two 128x64 tilemaps, one 128x32 tilemap)

00000-07fff BG0 (128x64)
08000-0ffff BG1 (128x64)
10000-103ff BG0 rowscroll (second half unused*)
10400-107ff BG1 rowscroll (second half unused*)
10800-108ff BG0 colscroll [evidenced by Warriorb inits from $1634]
10900-10fff unused (probably)
11000-11fff gfx data for FG0
12000-13fff FG0 (128x32)

* Perhaps Taito wanted potential for double height tilemaps on the
  TC0100SCN. The inits state the whole area is "linescroll".

Control registers

000-001 BG0 scroll X
002-003 BG1 scroll X
004-005 FG0 scroll X
006-007 BG0 scroll Y
008-009 BG1 scroll Y
00a-00b FG0 scroll Y
00c-00d ---------------x BG0 disable
        --------------x- BG1 disable
        -------------x-- FG0 disable
        ------------x--- change priority order from BG0-BG1-FG0 to BG1-BG0-FG0
        -----------x---- double width tilemaps + different memory map
                              (cameltru and all the multi-screen games)
        ----------x----- unknown (set in most of the TaitoZ games and Cadash)
00e-00f ---------------x flip screen
        ----------x----- this TC0100SCN is subsidiary [= not the main one]
                              (Multi-screen games only. Could it mean: "write
                               through what is written into main TC0100SCN" ?)
        --x------------- unknown (thunderfox)


Colscroll [standard layout]
=========

The e000-ff area is not divided into two halves, it appears to refer only
to bg1 - the top most layer unless bg0/1 are flipped.

128 words are available in 0xe0?? area. Every word scrolls 8
pixels.

Growl
-----
This uses column scroll in the boat scene [that's just after you have
disposed of the fat men in fezzes] and in the underground lava cavern
scene.

Boat scene: code from $2eb58 appears to be doing see-saw motion for
water layer under boat. $e08c is highest word written, it oscillates
between fffa and 0005. Going back towards $e000 a middle point is reached
which sticks at zero. By $e000 written values oscillate again.

A total of 80 words are being written to [some below 0xe000, I think those
won't do anything, sloppy coding...]

Cavern scene: code from $3178a moves a sequence of 0s, 1s and 0x1ffs
along. These words equate to 0, +1, -1 so will gently ripple bg 0
up and down adding to the shimmering heat effect.

Ninja Kids
----------
This uses column scroll in the fat flame boss scene [that's the end of
round 2] and in the last round in the final confrontation with Satan scene.

Fat flame boss: code at $8eee moves a sequence of 1s and 0s along. This
is similar to the heat shimmer in Growl cavern scene.

Final boss: code at $a024 moves a sine wave of values 0-4 along. When
you are close to getting him the range of values expands to 0-10.

Gunfront
--------
In demo mode when the boss appears with the clouds, a sequence of 40 words
forming sine wave between 0xffd0 and ffe0 is moved along. Bg0 has been
given priority over bg1 so it's the foreground (clouds) affected.

The 40 words will affect 40 8-pixel columns [rows, as this game is
rotated] i.e. what is visible on screen at any point.

Galmedes
--------
Towards end of first round in empty starfield area, about three big ship
sprites cross the screen (scrolling down with the starfield). 16 starfield
columns [rows, as the game is rotated] scroll across with the ship.
$84fc0 and neighbouring routines poke col scroll area.

*/

#include "emu.h"
#include "drawgfxm.h"
#include "tc0100scn.h"

#define TC0100SCN_RAM_SIZE        0x14000   /* enough for double-width tilemaps */
#define TC0100SCN_TOTAL_CHARS     256
#define XOR(a) WORD_XOR_BE(a)

const device_type TC0100SCN = &device_creator<tc0100scn_device>;

tc0100scn_device::tc0100scn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, TC0100SCN, "Taito TC0100SCN", tag, owner, clock, "tc0100scn", __FILE__),
	m_ram(NULL),
	m_bg_ram(NULL),
	m_fg_ram(NULL),
	m_tx_ram(NULL),
	m_char_ram(NULL),
	m_bgscroll_ram(NULL),
	m_fgscroll_ram(NULL),
	m_colscroll_ram(NULL),
	m_bgscrollx(0),
	m_bgscrolly(0),
	m_fgscrollx(0),
	m_fgscrolly(0),
	m_bg_col_mult(0),
	m_bg_tilemask(0),
	m_tx_col_mult(0),
	m_gfxbank(0),
	m_colbank(0),
	m_bg0_colbank(0),
	m_bg1_colbank(0),
	m_tx_colbank(0),
	m_dblwidth(0)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void tc0100scn_device::device_config_complete()
{
	// inherit a copy of the static data
	const tc0100scn_interface *intf = reinterpret_cast<const tc0100scn_interface *>(static_config());
	if (intf != NULL)
	*static_cast<tc0100scn_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
	}
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void tc0100scn_device::device_start()
{
	static const gfx_layout tc0100scn_charlayout =
	{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,  /* 2 bits per pixel */
	{ XOR(0)*4, XOR(2)*4 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8    /* every sprite takes 16 consecutive bytes */
	};

	int xd, yd;

	/* Set up clipping for multi-TC0100SCN games. We assume
	   this code won't ever affect single screen games:
	   Thundfox is the only one of those with two chips, and
	   we're safe as it uses single width tilemaps. */

	/* Single width versions */
	m_tilemap[0][0] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_bg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);
	m_tilemap[1][0] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_fg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);
	m_tilemap[2][0] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_tx_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 64, 64);

	/* Double width versions */
	m_tilemap[0][1] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_bg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 128, 64);
	m_tilemap[1][1] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_fg_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 128, 64);
	m_tilemap[2][1] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tc0100scn_device::get_tx_tile_info),this), TILEMAP_SCAN_ROWS, 8, 8, 128, 32);

	m_tilemap[0][0]->set_transparent_pen(0);
	m_tilemap[1][0]->set_transparent_pen(0);
	m_tilemap[2][0]->set_transparent_pen(0);

	m_tilemap[0][1]->set_transparent_pen(0);
	m_tilemap[1][1]->set_transparent_pen(0);
	m_tilemap[2][1]->set_transparent_pen(0);

	/* Standard width tilemaps. I'm setting the optional chip #2
	   7 bits higher and 2 pixels to the left than chip #1 because
	   that's how thundfox wants it. */

	xd = (m_multiscrn_hack == 0) ?  (-m_x_offset) : (-m_x_offset - 2);
	yd = (m_multiscrn_hack == 0) ?  (8 - m_y_offset) : (1 - m_y_offset);

	m_tilemap[0][0]->set_scrolldx(xd - 16, -m_flip_xoffs - xd - 16);
	m_tilemap[0][0]->set_scrolldy(yd,      -m_flip_yoffs - yd);
	m_tilemap[1][0]->set_scrolldx(xd - 16, -m_flip_xoffs - xd - 16);
	m_tilemap[1][0]->set_scrolldy(yd,      -m_flip_yoffs - yd);
	m_tilemap[2][0]->set_scrolldx(xd - 16, -m_flip_text_xoffs - xd - 16 - 7);
	m_tilemap[2][0]->set_scrolldy(yd,      -m_flip_text_yoffs - yd);

	/* Double width tilemaps. We must correct offsets for
	   extra chips, as MAME sees offsets from LHS of whole
	   display not from the edges of individual screens.
	   NB flipscreen tilemap offsets are based on Cameltry */

	xd = -m_x_offset - m_multiscrn_xoffs;
	yd = 8 - m_y_offset;

	m_tilemap[0][1]->set_scrolldx(xd - 16, -m_flip_xoffs - xd - 16);
	m_tilemap[0][1]->set_scrolldy(yd,      -m_flip_yoffs - yd);
	m_tilemap[1][1]->set_scrolldx(xd - 16, -m_flip_xoffs - xd - 16);
	m_tilemap[1][1]->set_scrolldy(yd,      -m_flip_yoffs - yd);
	m_tilemap[2][1]->set_scrolldx(xd - 16, -m_flip_text_xoffs - xd - 16 - 7);
	m_tilemap[2][1]->set_scrolldy(yd,      -m_flip_text_yoffs - yd);

	m_tilemap[0][0]->set_scroll_rows(512);
	m_tilemap[1][0]->set_scroll_rows(512);
	m_tilemap[0][1]->set_scroll_rows(512);
	m_tilemap[1][1]->set_scroll_rows(512);

	m_bg_tilemask = 0xffff;    /* Mjnquest has 0x7fff tilemask */

	m_bg_col_mult = 1; /* multiplier for when bg gfx != 4bpp */
	m_tx_col_mult = 1; /* multiplier needed when bg gfx is 6bpp */

	if (machine().gfx[m_gfxnum]->granularity() == 2)    /* Yuyugogo, Yesnoj */
		m_bg_col_mult = 8;

	if (machine().gfx[m_gfxnum]->granularity() == 0x40) /* Undrfire */
		m_tx_col_mult = 4;

//logerror("TC0100SCN bg gfx granularity %04x: multiplier %04x\n", machine().gfx[m_gfxnum]->granularity(), m_tx_col_mult);

	m_ram = auto_alloc_array_clear(machine(), UINT16, TC0100SCN_RAM_SIZE / 2);

	set_layer_ptrs();

	set_colbanks(0, 0, 0);  /* standard values, only Wgp & multiscreen games change them */
									/* we call this here, so that they can be modified at video_start*/

	/* create the char set (gfx will then be updated dynamically from RAM) */
	machine().gfx[m_txnum] = auto_alloc_clear(machine(), gfx_element(machine(), tc0100scn_charlayout, (UINT8 *)m_char_ram, 64, 0));

	save_pointer(NAME(m_ram), TC0100SCN_RAM_SIZE / 2);
	save_item(NAME(m_ctrl));
	save_item(NAME(m_dblwidth));
	save_item(NAME(m_gfxbank));
	machine().save().register_postload(save_prepost_delegate(FUNC(tc0100scn_device::postload), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void tc0100scn_device::device_reset()
{
	int i;

	m_dblwidth = 0;
	m_colbank = 0;
	m_gfxbank = 0; /* Mjnquest uniquely banks tiles */

	for (i = 0; i < 8; i++)
		m_ctrl[i] = 0;
}


/*****************************************************************************
    DEVICE HANDLERS
*****************************************************************************/

void tc0100scn_device::common_get_bg0_tile_info( tile_data &tileinfo, int tile_index, UINT16 *ram, int gfxnum, int colbank, int dblwidth )
{
	int code, attr;

	if (!dblwidth)
	{
		/* Mahjong Quest (F2 system) inexplicably has a banking feature */
		code = (ram[2 * tile_index + 1] & m_bg_tilemask) + (m_gfxbank << 15);
		attr = ram[2 * tile_index];
	}
	else
	{
		code = ram[2 * tile_index + 1] & m_bg_tilemask;
		attr = ram[2 * tile_index];
	}

	SET_TILE_INFO_MEMBER(
			gfxnum,
			code,
			(((attr * m_bg_col_mult) + m_bg0_colbank) & 0xff) + colbank,
			TILE_FLIPYX((attr & 0xc000) >> 14));
}

void tc0100scn_device::common_get_bg1_tile_info( tile_data &tileinfo, int tile_index, UINT16 *ram, int gfxnum, int colbank, int dblwidth )
{
	int code, attr;

	if (!dblwidth)
	{
		/* Mahjong Quest (F2 system) inexplicably has a banking feature */
		code = (ram[2 * tile_index + 1] & m_bg_tilemask) + (m_gfxbank << 15);
		attr = ram[2 * tile_index];
	}
	else
	{
		code = ram[2 * tile_index + 1] & m_bg_tilemask;
		attr = ram[2 * tile_index];
	}

	SET_TILE_INFO_MEMBER(
			gfxnum,
			code,
			(((attr * m_bg_col_mult) + m_bg1_colbank) & 0xff) + colbank,
			TILE_FLIPYX((attr & 0xc000) >> 14));
}

void tc0100scn_device::common_get_tx_tile_info( tile_data &tileinfo, int tile_index, UINT16 *ram, int gfxnum, int colbank, int dblwidth )
{
	int attr = ram[tile_index];

	SET_TILE_INFO_MEMBER(
			gfxnum,
			attr & 0xff,
			((((attr >> 6) & 0xfc) * m_tx_col_mult + (m_tx_colbank << 2)) & 0x3ff) + colbank * 4,
			TILE_FLIPYX((attr & 0xc000) >> 14));
}

TILE_GET_INFO_MEMBER(tc0100scn_device::get_bg_tile_info)
{
	common_get_bg0_tile_info(tileinfo, tile_index, m_bg_ram, m_gfxnum, m_colbank, m_dblwidth);
}

TILE_GET_INFO_MEMBER(tc0100scn_device::get_fg_tile_info)
{
	common_get_bg1_tile_info(tileinfo, tile_index, m_fg_ram, m_gfxnum, m_colbank, m_dblwidth);
}

TILE_GET_INFO_MEMBER(tc0100scn_device::get_tx_tile_info)
{
	common_get_tx_tile_info(tileinfo, tile_index, m_tx_ram, m_txnum, m_colbank, m_dblwidth);
}

void tc0100scn_device::set_colbank( int col )
{
	m_colbank = col;
}

void tc0100scn_device::set_colbanks( int bg0, int bg1, int tx )
{
	m_bg0_colbank = bg0;
	m_bg1_colbank = bg1;
	m_tx_colbank = tx;
}

void tc0100scn_device::set_bg_tilemask( int mask )
{
	m_bg_tilemask = mask;
}

WRITE16_MEMBER( tc0100scn_device::gfxbank_w )   /* Mjnquest banks its 2 sets of scr tiles */
{
	m_gfxbank = (data & 0x1);
}

void tc0100scn_device::set_layer_ptrs()
{
	if (!m_dblwidth)
	{
		m_bg_ram        = m_ram + 0x0;
		m_tx_ram        = m_ram + 0x4000 /2;
		m_char_ram      = m_ram + 0x6000 /2;
		m_fg_ram        = m_ram + 0x8000 /2;
		m_bgscroll_ram  = m_ram + 0xc000 /2;
		m_fgscroll_ram  = m_ram + 0xc400 /2;
		m_colscroll_ram = m_ram + 0xe000 /2;
	}
	else
	{
		m_bg_ram        = m_ram + 0x0;
		m_fg_ram        = m_ram + 0x08000 /2;
		m_bgscroll_ram  = m_ram + 0x10000 /2;
		m_fgscroll_ram  = m_ram + 0x10400 /2;
		m_colscroll_ram = m_ram + 0x10800 /2;
		m_char_ram      = m_ram + 0x11000 /2;
		m_tx_ram        = m_ram + 0x12000 /2;
	}
}

void tc0100scn_device::dirty_tilemaps()
{
	m_tilemap[0][m_dblwidth]->mark_all_dirty();
	m_tilemap[1][m_dblwidth]->mark_all_dirty();
	m_tilemap[2][m_dblwidth]->mark_all_dirty();
}

void tc0100scn_device::restore_scroll()
{
	int flip;

	m_bgscrollx = -m_ctrl[0];
	m_fgscrollx = -m_ctrl[1];
	m_tilemap[2][0]->set_scrollx(0, -m_ctrl[2]);
	m_tilemap[2][1]->set_scrollx(0, -m_ctrl[2]);

	m_bgscrolly = -m_ctrl[3];
	m_fgscrolly = -m_ctrl[4];
	m_tilemap[2][0]->set_scrolly(0, -m_ctrl[5]);
	m_tilemap[2][1]->set_scrolly(0, -m_ctrl[5]);

	flip = (m_ctrl[7] & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;
	m_tilemap[0][0]->set_flip(flip);
	m_tilemap[1][0]->set_flip(flip);
	m_tilemap[2][0]->set_flip(flip);
	m_tilemap[0][1]->set_flip(flip);
	m_tilemap[1][1]->set_flip(flip);
	m_tilemap[2][1]->set_flip(flip);
}


void tc0100scn_device::postload()
{
	set_layer_ptrs();
	restore_scroll();

	m_tilemap[0][0]->mark_all_dirty();
	m_tilemap[1][0]->mark_all_dirty();
	m_tilemap[2][0]->mark_all_dirty();
	m_tilemap[0][1]->mark_all_dirty();
	m_tilemap[1][1]->mark_all_dirty();
	m_tilemap[2][1]->mark_all_dirty();
}

READ16_MEMBER( tc0100scn_device::word_r )
{
	return m_ram[offset];
}

WRITE16_MEMBER( tc0100scn_device::word_w )
{
	COMBINE_DATA(&m_ram[offset]);
	if (!m_dblwidth)
	{
		if (offset < 0x2000)
			m_tilemap[0][0]->mark_tile_dirty(offset / 2);
		else if (offset < 0x3000)
			m_tilemap[2][0]->mark_tile_dirty((offset & 0x0fff));
		else if (offset < 0x3800)
			space.machine().gfx[m_txnum]->mark_dirty((offset - 0x3000) / 8);
		else if (offset >= 0x4000 && offset < 0x6000)
			m_tilemap[1][0]->mark_tile_dirty((offset & 0x1fff) / 2);
	}
	else    /* Double-width tilemaps have a different memory map */
	{
		if (offset < 0x4000)
			m_tilemap[0][1]->mark_tile_dirty(offset / 2);
		else if (offset >= 0x4000 && offset < 0x8000)
			m_tilemap[1][1]->mark_tile_dirty((offset & 0x3fff) / 2);
		else if (offset >= 0x8800 && offset < 0x9000)
			space.machine().gfx[m_txnum]->mark_dirty((offset - 0x8800) / 8);
		else if (offset >= 0x9000)
			m_tilemap[2][1]->mark_tile_dirty((offset & 0x0fff));
	}
}

READ16_MEMBER( tc0100scn_device::ctrl_word_r )
{
	return m_ctrl[offset];
}

WRITE16_MEMBER( tc0100scn_device::ctrl_word_w )
{
	COMBINE_DATA(&m_ctrl[offset]);

	data = m_ctrl[offset];

	switch (offset)
	{
		case 0x00:
			m_bgscrollx = -data;
			break;

		case 0x01:
			m_fgscrollx = -data;
			break;

		case 0x02:
			m_tilemap[2][0]->set_scrollx(0, -data);
			m_tilemap[2][1]->set_scrollx(0, -data);
			break;

		case 0x03:
			m_bgscrolly = -data;
			break;

		case 0x04:
			m_fgscrolly = -data;
			break;

		case 0x05:
			m_tilemap[2][0]->set_scrolly(0, -data);
			m_tilemap[2][1]->set_scrolly(0, -data);
			break;

		case 0x06:
		{
			int old_width = m_dblwidth;
			m_dblwidth = (data & 0x10) >> 4;

			if (m_dblwidth != old_width)   /* tilemap width is changing */
			{
				/* Reinitialise layer pointers */
				set_layer_ptrs();

				/* and ensure full redraw of the tilemaps */
				dirty_tilemaps();

				/* reset the pointer to the text characters (and dirty them all) */
				space.machine().gfx[m_txnum]->set_source((UINT8 *)m_char_ram);
			}

			break;
		}

		case 0x07:
		{
			int flip = (data & 0x01) ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0;

			m_tilemap[0][0]->set_flip(flip);
			m_tilemap[1][0]->set_flip(flip);
			m_tilemap[2][0]->set_flip(flip);
			m_tilemap[0][1]->set_flip(flip);
			m_tilemap[1][1]->set_flip(flip);
			m_tilemap[2][1]->set_flip(flip);

			break;
		}
	}
}


READ32_MEMBER( tc0100scn_device::ctrl_long_r )
{
	return (ctrl_word_r(space, offset * 2, 0xffff) << 16) | ctrl_word_r(space, offset * 2 + 1, 0xffff);
}

WRITE32_MEMBER( tc0100scn_device::ctrl_long_w )
{
	if (ACCESSING_BITS_16_31)
		ctrl_word_w(space, offset * 2, data >> 16, mem_mask >> 16);
	if (ACCESSING_BITS_0_15)
		ctrl_word_w(space, (offset * 2) + 1, data & 0xffff, mem_mask & 0xffff);
}

READ32_MEMBER( tc0100scn_device::long_r )
{
	return (word_r(space, offset * 2, 0xffff) << 16) | word_r(space, offset * 2 + 1, 0xffff);
}

WRITE32_MEMBER( tc0100scn_device::long_w )
{
	if (ACCESSING_BITS_16_31)
	{
		int oldword = word_r(space, offset * 2, 0xffff);
		int newword = data >> 16;
		if (!ACCESSING_BITS_16_23)
			newword |= (oldword & 0x00ff);
		if (!ACCESSING_BITS_24_31)
			newword |= (oldword & 0xff00);
		word_w(space, offset * 2, newword, 0xffff);
	}
	if (ACCESSING_BITS_0_15)
	{
		int oldword = word_r(space, (offset * 2) + 1, 0xffff);
		int newword = data& 0xffff;
		if (!ACCESSING_BITS_0_7)
			newword |= (oldword & 0x00ff);
		if (!ACCESSING_BITS_8_15)
			newword |= (oldword & 0xff00);
		word_w(space, (offset * 2) + 1, newword, 0xffff);
	}
}


void tc0100scn_device::tilemap_update()
{
	int j;

	m_tilemap[0][m_dblwidth]->set_scrolly(0, m_bgscrolly);
	m_tilemap[1][m_dblwidth]->set_scrolly(0, m_fgscrolly);

	for (j = 0; j < 256; j++)
		m_tilemap[0][m_dblwidth]->set_scrollx((j + m_bgscrolly) & 0x1ff, m_bgscrollx - m_bgscroll_ram[j]);
	for (j = 0; j < 256; j++)
		m_tilemap[1][m_dblwidth]->set_scrollx((j + m_fgscrolly) & 0x1ff, m_fgscrollx - m_fgscroll_ram[j]);
}

void tc0100scn_device::tilemap_draw_fg( screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, tilemap_t* tmap, int flags, UINT32 priority )
{
	const bitmap_ind16 &src_bitmap = tmap->pixmap();
	int width_mask, height_mask, x, y, p;
	int column_offset, src_x = 0, src_y = 0;
	int scrollx_delta = - tmap->scrolldx();
	int scrolly_delta = - tmap->scrolldy();

	width_mask = src_bitmap.width() - 1;
	height_mask = src_bitmap.height() - 1;

	src_y = (m_fgscrolly + scrolly_delta) & height_mask;
	if (m_ctrl[0x7] & 1) // Flipscreen
		src_y = (256 - src_y) & height_mask;

	//We use cliprect.max_y and cliprect.max_x to support games which use more than 1 screen

	// Row offsets are 'screen space' 0-255 regardless of Y scroll
	for (y = 0; y <= cliprect.max_y; y++)
	{
		src_x = (m_fgscrollx - m_fgscroll_ram[(y + scrolly_delta) & 0x1ff] + scrollx_delta + cliprect.min_x) & width_mask;
		if (m_ctrl[0x7] & 1) // Flipscreen
			src_x = (256 - 64 - src_x) & width_mask;

		// Col offsets are 'tilemap' space 0-511, and apply to blocks of 8 pixels at once
		for (x = 0; x < cliprect.width(); x++)
		{
			column_offset = m_colscroll_ram[(src_x & 0x3ff) / 8];
			p = src_bitmap.pix16((src_y - column_offset) & height_mask, src_x);

			if ((p & 0xf)!= 0 || (flags & TILEMAP_DRAW_OPAQUE))
			{
				bitmap.pix16(y, x + cliprect.min_x) = p;
				if (screen.priority().valid())
				{
					UINT8 *pri = &screen.priority().pix8(y);
					pri[x + cliprect.min_x] |= priority;
				}
			}
			src_x = (src_x + 1) & width_mask;
		}
		src_y = (src_y + 1) & height_mask;
	}
}

int tc0100scn_device::tilemap_draw( screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect, int layer, int flags, UINT32 priority )
{
	int disable = m_ctrl[6] & 0xf7;
	rectangle clip = cliprect;
	clip &= screen.visible_area();

#if 0
if (disable != 0 && disable != 3 && disable != 7)
	popmessage("layer disable = %x",disable);
#endif

	switch (layer)
	{
		case 0:
			if (disable & 0x01)
				return 1;
			m_tilemap[0][m_dblwidth]->draw(screen, bitmap, clip, flags, priority);
			break;
		case 1:
			if (disable & 0x02)
				return 1;
			tilemap_draw_fg(screen, bitmap, clip, m_tilemap[1][m_dblwidth], flags, priority);
			break;
		case 2:
			if (disable & 0x04)
				return 1;
			m_tilemap[2][m_dblwidth]->draw(screen, bitmap, clip, flags, priority);
			break;
	}
	return 0;
}

int tc0100scn_device::bottomlayer()
{
	return (m_ctrl[6] & 0x8) >> 3;
}
