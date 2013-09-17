#include "emu.h"
#include "includes/othldrby.h"


#define VIDEORAM_SIZE      0x1c00
#define SPRITERAM_START    0x1800
#define SPRITERAM_SIZE     (VIDEORAM_SIZE - SPRITERAM_START)


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

inline void othldrby_state::get_tile_info( tile_data &tileinfo, int tile_index, int plane )
{
	UINT16 attr;

	tile_index = 2 * tile_index + 0x800 * plane;
	attr = m_vram[tile_index];
	SET_TILE_INFO_MEMBER(
			1,
			m_vram[tile_index + 1],
			attr & 0x7f,
			0);
	tileinfo.category = (attr & 0x0600) >> 9;
}

TILE_GET_INFO_MEMBER(othldrby_state::get_tile_info0)
{
	get_tile_info(tileinfo, tile_index, 0);
}

TILE_GET_INFO_MEMBER(othldrby_state::get_tile_info1)
{
	get_tile_info(tileinfo, tile_index, 1);
}

TILE_GET_INFO_MEMBER(othldrby_state::get_tile_info2)
{
	get_tile_info(tileinfo, tile_index, 2);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void othldrby_state::video_start()
{
	m_bg_tilemap[0] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(othldrby_state::get_tile_info0),this), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
	m_bg_tilemap[1] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(othldrby_state::get_tile_info1),this), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);
	m_bg_tilemap[2] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(othldrby_state::get_tile_info2),this), TILEMAP_SCAN_ROWS, 16, 16, 32, 32);

	m_vram = auto_alloc_array(machine(), UINT16, VIDEORAM_SIZE);
	m_buf_spriteram = auto_alloc_array(machine(), UINT16, 2 * SPRITERAM_SIZE);
	m_buf_spriteram2 = m_buf_spriteram + SPRITERAM_SIZE;

	m_bg_tilemap[0]->set_transparent_pen(0);
	m_bg_tilemap[1]->set_transparent_pen(0);
	m_bg_tilemap[2]->set_transparent_pen(0);

	save_pointer(NAME(m_vram), VIDEORAM_SIZE);
	save_pointer(NAME(m_buf_spriteram), 2 * SPRITERAM_SIZE);
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_MEMBER(othldrby_state::othldrby_videoram_addr_w)
{
	m_vram_addr = data;
}

READ16_MEMBER(othldrby_state::othldrby_videoram_r)
{
	if (m_vram_addr < VIDEORAM_SIZE)
		return m_vram[m_vram_addr++];
	else
	{
		popmessage("GFXRAM OUT OF BOUNDS %04x", m_vram_addr);
		return 0;
	}
}

WRITE16_MEMBER(othldrby_state::othldrby_videoram_w)
{
	if (m_vram_addr < VIDEORAM_SIZE)
	{
		if (m_vram_addr < SPRITERAM_START)
			m_bg_tilemap[m_vram_addr / 0x800]->mark_tile_dirty((m_vram_addr & 0x7ff) / 2);
		m_vram[m_vram_addr++] = data;
	}
	else
		popmessage("GFXRAM OUT OF BOUNDS %04x", m_vram_addr);
}

WRITE16_MEMBER(othldrby_state::othldrby_vreg_addr_w)
{
	m_vreg_addr = data & 0x7f;  /* bit 7 is set when screen is flipped */
}

WRITE16_MEMBER(othldrby_state::othldrby_vreg_w)
{
	if (m_vreg_addr < OTHLDRBY_VREG_SIZE)
		m_vreg[m_vreg_addr++] = data;
	else
		popmessage("%06x: VREG OUT OF BOUNDS %04x", space.device().safe_pc(), m_vreg_addr);
}



/***************************************************************************

  Display refresh

***************************************************************************/

void othldrby_state::draw_sprites( bitmap_ind16 &bitmap, const rectangle &cliprect, int priority )
{
	int offs;

	for (offs = 0; offs < SPRITERAM_SIZE; offs += 4)
	{
		int x, y, color, code, sx, sy, flipx, flipy, sizex, sizey, pri;

		pri = (m_buf_spriteram[offs] & 0x0600) >> 9;
		if (pri != priority)
			continue;

		flipx = m_buf_spriteram[offs] & 0x1000;
		flipy = 0;
		color = (m_buf_spriteram[offs] & 0x01fc) >> 2;
		code = m_buf_spriteram[offs + 1] | ((m_buf_spriteram[offs] & 0x0003) << 16);
		sx = (m_buf_spriteram[offs + 2] >> 7);
		sy = (m_buf_spriteram[offs + 3] >> 7);
		sizex = (m_buf_spriteram[offs + 2] & 0x000f) + 1;
		sizey = (m_buf_spriteram[offs + 3] & 0x000f) + 1;

		if (flip_screen())
		{
			flipx = !flipx;
			flipy = !flipy;
			sx = 246 - sx;
			sy = 16 - sy;
		}

		for (y = 0; y < sizey; y++)
		{
			for (x = 0; x < sizex; x++)
			{
				drawgfx_transpen(bitmap,cliprect,machine().gfx[0],
						code + x + sizex * y,
						color,
						flipx,flipy,
						(sx + (flipx ? (-8*(x+1)+1) : 8*x) - m_vreg[6]+44) & 0x1ff,(sy + (flipy ? (-8*(y+1)+1) : 8*y) - m_vreg[7]-9) & 0x1ff,0);
			}
		}
	}
}

UINT32 othldrby_state::screen_update_othldrby(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	int layer;

	flip_screen_set(m_vreg[0x0f] & 0x80);

	for (layer = 0; layer < 3; layer++)
	{
		if (flip_screen())
		{
			m_bg_tilemap[layer]->set_scrollx(0, m_vreg[2 * layer] + 59);
			m_bg_tilemap[layer]->set_scrolly(0, m_vreg[2 * layer + 1] + 248);
		}
		else
		{
			m_bg_tilemap[layer]->set_scrollx(0, m_vreg[2 * layer] - 58);
			m_bg_tilemap[layer]->set_scrolly(0, m_vreg[2 * layer+1] + 9);
		}
	}

	screen.priority().fill(0, cliprect);

	bitmap.fill(0, cliprect);

	for (layer = 0; layer < 3; layer++)
		m_bg_tilemap[layer]->draw(screen, bitmap, cliprect, 0, 0);
	draw_sprites(bitmap, cliprect, 0);

	for (layer = 0; layer < 3; layer++)
		m_bg_tilemap[layer]->draw(screen, bitmap, cliprect, 1, 0);
	draw_sprites(bitmap, cliprect, 1);

	for (layer = 0; layer < 3; layer++)
		m_bg_tilemap[layer]->draw(screen, bitmap, cliprect, 2, 0);
	draw_sprites(bitmap, cliprect, 2);

	for (layer = 0; layer < 3; layer++)
		m_bg_tilemap[layer]->draw(screen, bitmap, cliprect, 3, 0);
	draw_sprites(bitmap, cliprect, 3);

	return 0;
}

void othldrby_state::screen_eof_othldrby(screen_device &screen, bool state)
{
	// rising edge
	if (state)
	{
		/* sprites need to be delayed two frames */
		memcpy(m_buf_spriteram, m_buf_spriteram2, SPRITERAM_SIZE * sizeof(m_buf_spriteram[0]));
		memcpy(m_buf_spriteram2, &m_vram[SPRITERAM_START], SPRITERAM_SIZE * sizeof(m_buf_spriteram[0]));
	}
}
