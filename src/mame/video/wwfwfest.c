/*******************************************************************************
 WWF Wrestlefest (C) 1991 Technos Japan  (video/wwfwfest.c)
********************************************************************************
 driver by David Haywood

 see (drivers/wwfwfest.c) for more notes
*******************************************************************************/

#include "emu.h"
#include "includes/wwfwfest.h"


/*******************************************************************************
 Write Handlers
********************************************************************************
 for writes to Video Ram
*******************************************************************************/

WRITE16_MEMBER(wwfwfest_state::wwfwfest_fg0_videoram_w)
{
	/* Videoram is 8 bit, upper & lower byte writes end up in the same place */
	if (ACCESSING_BITS_8_15 && ACCESSING_BITS_0_7) {
		COMBINE_DATA(&m_fg0_videoram[offset]);
	} else if (ACCESSING_BITS_8_15) {
		m_fg0_videoram[offset]=(data>>8)&0xff;
	} else {
		m_fg0_videoram[offset]=data&0xff;
	}

	m_fg0_tilemap->mark_tile_dirty(offset/2);
}

WRITE16_MEMBER(wwfwfest_state::wwfwfest_bg0_videoram_w)
{
	COMBINE_DATA(&m_bg0_videoram[offset]);
	m_bg0_tilemap->mark_tile_dirty(offset/2);
}

WRITE16_MEMBER(wwfwfest_state::wwfwfest_bg1_videoram_w)
{
	COMBINE_DATA(&m_bg1_videoram[offset]);
	m_bg1_tilemap->mark_tile_dirty(offset);
}

/*******************************************************************************
 Tilemap Related Functions
*******************************************************************************/
TILE_GET_INFO_MEMBER(wwfwfest_state::get_fg0_tile_info)
{
	/*- FG0 RAM Format -**

	  4 bytes per tile

	  ---- ----  tttt tttt  ---- ----  ???? TTTT

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)

	  other bits unknown / unused

	  basically the same as WWF Superstar's FG0 Ram but
	  more of it and the used bytes the other way around

	**- End of Comments -*/

	UINT16 *tilebase;
	int tileno;
	int colbank;
	tilebase =  &m_fg0_videoram[tile_index*2];
	tileno =  (tilebase[0] & 0x00ff) | ((tilebase[1] & 0x000f) << 8);
	colbank = (tilebase[1] & 0x00f0) >> 4;
	SET_TILE_INFO_MEMBER(
			0,
			tileno,
			colbank,
			0);
}

TILE_GET_INFO_MEMBER(wwfwfest_state::get_bg0_tile_info)
{
	/*- BG0 RAM Format -**

	  4 bytes per tile

	  ---- ----  fF-- CCCC  ---- TTTT tttt tttt

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)
	  f = Flip Y
	  F = Flip X

	  other bits unknown / unused

	**- End of Comments -*/

	UINT16 *tilebase;
	int tileno,colbank;

	tilebase =  &m_bg0_videoram[tile_index*2];
	tileno =  (tilebase[1] & 0x0fff);
	colbank = (tilebase[0] & 0x000f);
	SET_TILE_INFO_MEMBER(
			2,
			tileno,
			colbank,
			TILE_FLIPYX((tilebase[0] & 0x00c0) >> 6));
}

TILE_GET_INFO_MEMBER(wwfwfest_state::get_bg1_tile_info)
{
	/*- BG1 RAM Format -**

	  2 bytes per tile

	  CCCC TTTT tttt tttt

	  C = Colour Bank (0-15)
	  T = Tile Number (0 - 4095)

	**- End of Comments -*/

	UINT16 *tilebase;
	int tileno;
	int colbank;
	tilebase =  &m_bg1_videoram[tile_index];
	tileno =  (tilebase[0] & 0x0fff);
	colbank = (tilebase[0] & 0xf000) >> 12;
	SET_TILE_INFO_MEMBER(
			3,
			tileno,
			colbank,
			0);
}

/*******************************************************************************
 Sprite Related Functions
********************************************************************************
 sprite drawing could probably be improved a bit
*******************************************************************************/

void wwfwfest_state::draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	/*- SPR RAM Format -**

	  16 bytes per sprite

	  ---- ----  yyyy yyyy  ---- ----  lllF fXYE  ---- ----  nnnn nnnn  ---- ----  NNNN NNNN
	  ---- ----  ---- CCCC  ---- ----  xxxx xxxx  ---- ----  ---- ----  ---- ----  ---- ----

	  Yy = sprite Y Position
	  Xx = sprite X Position
	  C  = colour bank
	  f  = flip Y
	  F  = flip X
	  l  = chain sprite
	  E  = sprite enable
	  Nn = Sprite Number

	  other bits unused

	**- End of Comments -*/

	UINT16 *buffered_spriteram16 = m_spriteram->buffer();
	gfx_element *gfx = machine().gfx[1];
	UINT16 *source = buffered_spriteram16;
	UINT16 *finish = source + 0x2000/2;

	while( source<finish )
	{
		int xpos, ypos, colourbank, flipx, flipy, chain, enable, number, count;

		enable = (source[1] & 0x0001);

		if (enable) {
			xpos = +(source[5] & 0x00ff) | (source[1] & 0x0004) << 6;
			if (xpos>512-16) xpos -=512;
			xpos += m_sprite_xoff;
			ypos = (source[0] & 0x00ff) | (source[1] & 0x0002) << 7;
			ypos = (256 - ypos) & 0x1ff;
			ypos -= 16 ;
			flipx = (source[1] & 0x0010) >> 4;
			flipy = (source[1] & 0x0008) >> 3;
			chain = (source[1] & 0x00e0) >> 5;
			chain += 1;
			number = (source[2] & 0x00ff) | (source[3] & 0x00ff) << 8;
			colourbank = (source[4] & 0x000f);

			if (flip_screen()) {
				if (flipy) flipy=0; else flipy=1;
				if (flipx) flipx=0; else flipx=1;
				ypos=240-ypos-m_sprite_xoff;
				xpos=304-xpos;
			}

			for (count=0;count<chain;count++) {
				if (flip_screen()) {
					if (!flipy) {
						drawgfx_transpen(bitmap,cliprect,gfx,number+count,colourbank,flipx,flipy,xpos,ypos+(16*(chain-1))-(16*count),0);
					} else {
						drawgfx_transpen(bitmap,cliprect,gfx,number+count,colourbank,flipx,flipy,xpos,ypos+16*count,0);
					}
				} else {
					if (flipy) {
						drawgfx_transpen(bitmap,cliprect,gfx,number+count,colourbank,flipx,flipy,xpos,ypos-(16*(chain-1))+(16*count),0);
					} else {
						drawgfx_transpen(bitmap,cliprect,gfx,number+count,colourbank,flipx,flipy,xpos,ypos-16*count,0);
					}
				}
			}
		}
	source+=8;
	}
}

/*******************************************************************************
 Video Start and Refresh Functions
********************************************************************************
 Draw Order / Priority seems to affect where the scroll values are used also.
*******************************************************************************/

void wwfwfest_state::video_start()
{
	state_save_register_global(machine(), m_pri);
	state_save_register_global(machine(), m_bg0_scrollx);
	state_save_register_global(machine(), m_bg0_scrolly);
	state_save_register_global(machine(), m_bg1_scrollx);
	state_save_register_global(machine(), m_bg1_scrolly);

	m_fg0_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(wwfwfest_state::get_fg0_tile_info),this),TILEMAP_SCAN_ROWS, 8, 8,64,32);
	m_bg1_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(wwfwfest_state::get_bg1_tile_info),this),TILEMAP_SCAN_ROWS, 16, 16,32,32);
	m_bg0_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(wwfwfest_state::get_bg0_tile_info),this),TILEMAP_SCAN_ROWS, 16, 16,32,32);

	m_fg0_tilemap->set_transparent_pen(0);
	m_bg1_tilemap->set_transparent_pen(0);
	m_bg0_tilemap->set_transparent_pen(0);

	m_sprite_xoff = m_bg0_dx = m_bg1_dx[0] = m_bg1_dx[1] = 0;
}

VIDEO_START_MEMBER(wwfwfest_state,wwfwfstb)
{
	wwfwfest_state::video_start();

	m_sprite_xoff = 2;
	m_bg0_dx = m_bg1_dx[0] = -4;
	m_bg1_dx[1] = -2;
}

UINT32 wwfwfest_state::screen_update_wwfwfest(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	if (m_pri == 0x0078) {
		m_bg0_tilemap->set_scrolly(0, m_bg0_scrolly  );
		m_bg0_tilemap->set_scrollx(0, m_bg0_scrollx  + m_bg0_dx);
		m_bg1_tilemap->set_scrolly(0, m_bg1_scrolly  );
		m_bg1_tilemap->set_scrollx(0, m_bg1_scrollx  + m_bg1_dx[0]);
	} else {
		m_bg1_tilemap->set_scrolly(0, m_bg0_scrolly  );
		m_bg1_tilemap->set_scrollx(0, m_bg0_scrollx  + m_bg1_dx[1]);
		m_bg0_tilemap->set_scrolly(0, m_bg1_scrolly  );
		m_bg0_tilemap->set_scrollx(0, m_bg1_scrollx  + m_bg0_dx);
	}

	/* todo : which bits of pri are significant to the order */

	if (m_pri == 0x007b) {
		m_bg0_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_OPAQUE,0);
		m_bg1_tilemap->draw(bitmap, cliprect, 0,0);
		draw_sprites(bitmap,cliprect);
		m_fg0_tilemap->draw(bitmap, cliprect, 0,0);
	}

	if (m_pri == 0x007c) {
		m_bg0_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_OPAQUE,0);
		draw_sprites(bitmap,cliprect);
		m_bg1_tilemap->draw(bitmap, cliprect, 0,0);
		m_fg0_tilemap->draw(bitmap, cliprect, 0,0);
	}

	if (m_pri == 0x0078) {
		m_bg1_tilemap->draw(bitmap, cliprect, TILEMAP_DRAW_OPAQUE,0);
		m_bg0_tilemap->draw(bitmap, cliprect, 0,0);
		draw_sprites(bitmap,cliprect);
		m_fg0_tilemap->draw(bitmap, cliprect, 0,0);
	}
	return 0;
}
