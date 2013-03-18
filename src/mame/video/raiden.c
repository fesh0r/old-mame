#include "emu.h"
#include "includes/raiden.h"


/******************************************************************************/

WRITE16_MEMBER(raiden_state::raiden_background_w)
{
	COMBINE_DATA(&m_back_data[offset]);
	m_bg_layer->mark_tile_dirty(offset);
}

WRITE16_MEMBER(raiden_state::raiden_foreground_w)
{
	COMBINE_DATA(&m_fore_data[offset]);
	m_fg_layer->mark_tile_dirty(offset);
}

WRITE16_MEMBER(raiden_state::raiden_text_w)
{
	UINT16 *videoram = m_videoram;
	COMBINE_DATA(&videoram[offset]);
	m_tx_layer->mark_tile_dirty(offset);
}

TILE_GET_INFO_MEMBER(raiden_state::get_back_tile_info)
{
	int tile=m_back_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO_MEMBER(
			1,
			tile,
			color,
			0);
}

TILE_GET_INFO_MEMBER(raiden_state::get_fore_tile_info)
{
	int tile=m_fore_data[tile_index];
	int color=tile >> 12;

	tile=tile&0xfff;

	SET_TILE_INFO_MEMBER(
			2,
			tile,
			color,
			0);
}

TILE_GET_INFO_MEMBER(raiden_state::get_text_tile_info)
{
	UINT16 *videoram = m_videoram;
	int tiledata = videoram[tile_index];
	int tile = (tiledata & 0xff) | ((tiledata >> 6) & 0x300);
	int color = (tiledata >> 8) & 0x0f;

	SET_TILE_INFO_MEMBER(
			0,
			tile,
			color,
			0);
}

void raiden_state::video_start()
{
	m_bg_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_back_tile_info),this),TILEMAP_SCAN_COLS,     16,16,32,32);
	m_fg_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_fore_tile_info),this),TILEMAP_SCAN_COLS,16,16,32,32);
	m_tx_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_text_tile_info),this),TILEMAP_SCAN_COLS,8,8,32,32);
	m_alternate=0;

	m_fg_layer->set_transparent_pen(15);
	m_tx_layer->set_transparent_pen(15);
}

VIDEO_START_MEMBER(raiden_state,raidena)
{
	m_bg_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_back_tile_info),this),TILEMAP_SCAN_COLS,     16,16,32,32);
	m_fg_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_fore_tile_info),this),TILEMAP_SCAN_COLS,16,16,32,32);
	m_tx_layer = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(raiden_state::get_text_tile_info),this),TILEMAP_SCAN_ROWS,8,8,32,32);
	m_alternate=1;

	m_fg_layer->set_transparent_pen(15);
	m_tx_layer->set_transparent_pen(15);
}

WRITE16_MEMBER(raiden_state::raiden_control_w)
{
	/* All other bits unknown - could be playfield enables */

	/* Flipscreen */
	if (offset==3 && ACCESSING_BITS_0_7) {
		m_flipscreen=data&0x2;
		machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}
}

WRITE16_MEMBER(raiden_state::raidena_control_w)
{
	/* raidena uses 0x40 instead of 0x02 */

	/* Flipscreen */
	if (offset==3 && ACCESSING_BITS_0_7) {
		m_flipscreen=data&0x40;
		machine().tilemap().set_flip_all(m_flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
	}
}

void raiden_state::draw_sprites(bitmap_ind16 &bitmap,const rectangle &cliprect,int pri_mask)
{
	UINT16 *buffered_spriteram16 = m_spriteram->buffer();
	int offs,fx,fy,x,y,color,sprite;

	for (offs = 0x1000/2-4;offs >= 0;offs -= 4)
	{
		if (!(pri_mask&(buffered_spriteram16[offs+2]>>8))) continue;

		fx    = buffered_spriteram16[offs+0] & 0x2000;
		fy    = buffered_spriteram16[offs+0] & 0x4000;
		color = (buffered_spriteram16[offs+0] & 0x0f00) >> 8;
		y = buffered_spriteram16[offs+0] & 0x00ff;

		sprite = buffered_spriteram16[offs+1];
		sprite &= 0x0fff;

		x = buffered_spriteram16[offs+2] & 0xff;
		if (buffered_spriteram16[offs+2] & 0x100) x=0-(0x100-x);

		if (m_flipscreen) {
			x=240-x;
			y=240-y;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
		}

		drawgfx_transpen(bitmap,cliprect,machine().gfx[3],
				sprite,
				color,fx,fy,x,y,15);
	}
}

UINT32 raiden_state::screen_update_raiden(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	/* Setup the tilemaps, alternate version has different scroll positions */
	if (!m_alternate) {
		m_bg_layer->set_scrollx(0, m_scroll_ram[0]);
		m_bg_layer->set_scrolly(0, m_scroll_ram[1]);
		m_fg_layer->set_scrollx(0, m_scroll_ram[2]);
		m_fg_layer->set_scrolly(0, m_scroll_ram[3]);
	}
	else {
		m_bg_layer->set_scrolly(0, ((m_scroll_ram[0x01]&0x30)<<4)+((m_scroll_ram[0x02]&0x7f)<<1)+((m_scroll_ram[0x02]&0x80)>>7) );
		m_bg_layer->set_scrollx(0, ((m_scroll_ram[0x09]&0x30)<<4)+((m_scroll_ram[0x0a]&0x7f)<<1)+((m_scroll_ram[0x0a]&0x80)>>7) );
		m_fg_layer->set_scrolly(0, ((m_scroll_ram[0x11]&0x30)<<4)+((m_scroll_ram[0x12]&0x7f)<<1)+((m_scroll_ram[0x12]&0x80)>>7) );
		m_fg_layer->set_scrollx(0, ((m_scroll_ram[0x19]&0x30)<<4)+((m_scroll_ram[0x1a]&0x7f)<<1)+((m_scroll_ram[0x1a]&0x80)>>7) );
	}

	m_bg_layer->draw(bitmap, cliprect, 0,0);

	/* Draw sprites underneath foreground */
	draw_sprites(bitmap,cliprect,0x40);
	m_fg_layer->draw(bitmap, cliprect, 0,0);

	/* Rest of sprites */
	draw_sprites(bitmap,cliprect,0x80);

	/* Text layer */
	m_tx_layer->draw(bitmap, cliprect, 0,0);
	return 0;
}
