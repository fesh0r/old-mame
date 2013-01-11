/*
*   Video Driver for Tank Busters
*/

#include "emu.h"
#include "includes/tankbust.h"


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/* background tilemap :

0xc000-0xc7ff:  xxxx xxxx tile code: 8 lsb bits

0xc800-0xcfff:  .... .xxx tile code: 3 msb bits
                .... x... tile priority ON TOP of sprites (roofs and part of the rails)
                .xxx .... tile color code
                x... .... ?? set on *all* roofs (a different bg/sprite priority ?)

note:
 - seems that the only way to get color test right is to swap bit 1 and bit 0 of color code

*/

TILE_GET_INFO_MEMBER(tankbust_state::get_bg_tile_info)
{
	int code = m_videoram[tile_index];
	int attr = m_colorram[tile_index];

	int color = ((attr>>4) & 0x07);

	code |= (attr&0x07) * 256;


#if 0
	if (attr&0x08)  //priority bg/sprites (1 = this bg tile on top of sprites)
	{
		color = ((int)rand()) & 0x0f;
	}
	if (attr&0x80)  //al the roofs of all buildings have this bit set. What's this ???
	{
		color = ((int)rand()) & 0x0f;
	}
#endif

	/* priority bg/sprites (1 = this bg tile on top of sprites) */
	tileinfo.category = (attr & 0x08) >> 3;

	SET_TILE_INFO_MEMBER(   1,
			code,
			(color&4) | ((color&2)>>1) | ((color&1)<<1),
			0);
}

TILE_GET_INFO_MEMBER(tankbust_state::get_txt_tile_info)
{
	int code = m_txtram[tile_index];
	int color = ((code>>6) & 0x03);

	SET_TILE_INFO_MEMBER(   2,
			code & 0x3f,
			((color&2)>>1) | ((color&1)<<1),
			0);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void tankbust_state::video_start()
{
	/* not scrollable */
	m_txt_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tankbust_state::get_txt_tile_info),this), TILEMAP_SCAN_ROWS,  8, 8, 64, 32);

	/* scrollable */
	m_bg_tilemap = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(tankbust_state::get_bg_tile_info),this), TILEMAP_SCAN_ROWS,  8, 8, 64, 32);


	m_txt_tilemap->set_transparent_pen(0);
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE8_MEMBER(tankbust_state::tankbust_background_videoram_w)
{
	m_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}
READ8_MEMBER(tankbust_state::tankbust_background_videoram_r)
{
	return m_videoram[offset];
}

WRITE8_MEMBER(tankbust_state::tankbust_background_colorram_w)
{
	m_colorram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}
READ8_MEMBER(tankbust_state::tankbust_background_colorram_r)
{
	return m_colorram[offset];
}

WRITE8_MEMBER(tankbust_state::tankbust_txtram_w)
{
	m_txtram[offset] = data;
	m_txt_tilemap->mark_tile_dirty(offset);
}
READ8_MEMBER(tankbust_state::tankbust_txtram_r)
{
	return m_txtram[offset];
}



WRITE8_MEMBER(tankbust_state::tankbust_xscroll_w)
{
	if( m_xscroll[offset] != data )
	{
		int x;

		m_xscroll[offset] = data;

		x = m_xscroll[0] + 256 * (m_xscroll[1]&1);
		if (x>=0x100) x-=0x200;
		m_bg_tilemap->set_scrollx(0, x );
	}
//popmessage("x=%02x %02x", m_xscroll[0], m_xscroll[1]);
}


WRITE8_MEMBER(tankbust_state::tankbust_yscroll_w)
{
	if( m_yscroll[offset] != data )
	{
		int y;

		m_yscroll[offset] = data;
		y = m_yscroll[0];
		if (y>=0x80) y-=0x100;
		m_bg_tilemap->set_scrolly(0, y );
	}
//popmessage("y=%02x %02x", m_yscroll[0], m_yscroll[1]);
}

/***************************************************************************

  Display refresh

***************************************************************************/
/*
spriteram format (4 bytes per sprite):

    offset  0   x.......    flip X
    offset  0   .x......    flip Y
    offset  0   ..xxxxxx    gfx code (6 bits)

    offset  1   xxxxxxxx    y position

    offset  2   ??????..    not used ?
    offset  2   ......?.    used but unknown ??? (color code ? or x ?)
    offset  2   .......x    x position (1 MSB bit)

    offset  3   xxxxxxxx    x position (8 LSB bits)
*/

static void draw_sprites(running_machine &machine, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	tankbust_state *state = machine.driver_data<tankbust_state>();
	UINT8 *spriteram = state->m_spriteram;
	int offs;

	for (offs = 0; offs < state->m_spriteram.bytes(); offs += 4)
	{
		int code,color,sx,sy,flipx,flipy;

		code  = spriteram[offs+0] & 0x3f;
		flipy = spriteram[offs+0] & 0x40;
		flipx = spriteram[offs+0] & 0x80;

		sy = (240- spriteram[offs+1]) - 14;
		sx = (spriteram[offs+2] & 0x01) * 256 + spriteram[offs+3] - 7;

		color = 0;

		//0x02 - dont know (most of the time this bit is set in tank sprite and others but not all and not always)
		//0x04 - not used
		//0x08 - not used
		//0x10 - not used
		//0x20 - not used
		//0x40 - not used
		//0x80 - not used
#if 0
		if ((spriteram[offs+2] & 0x02))
		{
			code = ((int)rand()) & 63;
		}
#endif

		if ((spriteram[offs+1]!=4)) //otherwise - ghost sprites
		{

			drawgfx_transpen(bitmap,cliprect,machine.gfx[0],
				code, color,
				flipx,flipy,
				sx,sy,0);
		}
	}
}


UINT32 tankbust_state::screen_update_tankbust(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
#if 0
	int i;

	for (i=0; i<0x800; i++)
	{
		int tile_attrib = m_colorram[i];

		if ( (tile_attrib&8) || (tile_attrib&0x80) )
		{
			m_bg_tilemap->mark_tile_dirty(i);
		}
	}
#endif

	m_bg_tilemap->draw(bitmap, cliprect, 0, 0);
	draw_sprites(machine(), bitmap, cliprect);
	m_bg_tilemap->draw(bitmap, cliprect, 1, 0);

	m_txt_tilemap->draw(bitmap, cliprect, 0,0);
	return 0;
}
