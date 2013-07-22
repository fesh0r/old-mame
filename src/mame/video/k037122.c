/*
Konami 037122
*/

#include "emu.h"
#include "k037122.h"
#include "konami_helper.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define K037122_NUM_TILES       16384

const device_type K037122 = &device_creator<k037122_device>;

k037122_device::k037122_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, K037122, "Konami 0371222", tag, owner, clock, "k037122", __FILE__),
	m_screen(NULL),
	m_tile_ram(NULL),
	m_char_ram(NULL),
	m_reg(NULL)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void k037122_device::device_config_complete()
{
	// inherit a copy of the static data
	const k037122_interface *intf = reinterpret_cast<const k037122_interface *>(static_config());
	if (intf != NULL)
		*static_cast<k037122_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		m_screen_tag = "";
		m_gfx_index = 0;
	}
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void k037122_device::device_start()
{
	static const gfx_layout k037122_char_layout =
	{
	8, 8,
	K037122_NUM_TILES,
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 1*16, 0*16, 3*16, 2*16, 5*16, 4*16, 7*16, 6*16 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128 },
	8*128
	};

	m_screen = machine().device<screen_device>(m_screen_tag);

	m_char_ram = auto_alloc_array_clear(machine(), UINT32, 0x200000 / 4);
	m_tile_ram = auto_alloc_array_clear(machine(), UINT32, 0x20000 / 4);
	m_reg = auto_alloc_array_clear(machine(), UINT32, 0x400 / 4);

	m_layer[0] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(k037122_device::tile_info_layer0),this), TILEMAP_SCAN_ROWS, 8, 8, 256, 64);
	m_layer[1] = &machine().tilemap().create(tilemap_get_info_delegate(FUNC(k037122_device::tile_info_layer1),this), TILEMAP_SCAN_ROWS, 8, 8, 128, 64);

	m_layer[0]->set_transparent_pen(0);
	m_layer[1]->set_transparent_pen(0);

	machine().gfx[m_gfx_index] = auto_alloc_clear(machine(), gfx_element(machine(), k037122_char_layout, (UINT8*)m_char_ram, machine().total_colors() / 16, 0));

	save_pointer(NAME(m_reg), 0x400 / 4);
	save_pointer(NAME(m_char_ram), 0x200000 / 4);
	save_pointer(NAME(m_tile_ram), 0x20000 / 4);

}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void k037122_device::device_reset()
{
	memset(m_char_ram, 0, 0x200000);
	memset(m_tile_ram, 0, 0x20000);
	memset(m_reg, 0, 0x400);
}

/*****************************************************************************
    DEVICE HANDLERS
*****************************************************************************/

TILE_GET_INFO_MEMBER(k037122_device::tile_info_layer0)
{
	UINT32 val = m_tile_ram[tile_index + (0x8000/4)];
	int color = (val >> 17) & 0x1f;
	int tile = val & 0x3fff;
	int flags = 0;

	if (val & 0x400000)
		flags |= TILE_FLIPX;
	if (val & 0x800000)
		flags |= TILE_FLIPY;

	SET_TILE_INFO_MEMBER(m_gfx_index, tile, color, flags);
}

TILE_GET_INFO_MEMBER(k037122_device::tile_info_layer1)
{
	UINT32 val = m_tile_ram[tile_index];
	int color = (val >> 17) & 0x1f;
	int tile = val & 0x3fff;
	int flags = 0;

	if (val & 0x400000)
		flags |= TILE_FLIPX;
	if (val & 0x800000)
		flags |= TILE_FLIPY;

	SET_TILE_INFO_MEMBER(m_gfx_index, tile, color, flags);
}


void k037122_device::tile_draw( bitmap_rgb32 &bitmap, const rectangle &cliprect )
{
	const rectangle &visarea = m_screen->visible_area();

	if (m_reg[0xc] & 0x10000)
	{
		m_layer[1]->set_scrolldx(visarea.min_x, visarea.min_x);
		m_layer[1]->set_scrolldy(visarea.min_y, visarea.min_y);
		m_layer[1]->draw(bitmap, cliprect, 0, 0);
	}
	else
	{
		m_layer[0]->set_scrolldx(visarea.min_x, visarea.min_x);
		m_layer[0]->set_scrolldy(visarea.min_y, visarea.min_y);
		m_layer[0]->draw(bitmap, cliprect, 0, 0);
	}
}

void k037122_device::update_palette_color( UINT32 palette_base, int color )
{
	UINT32 data = m_tile_ram[(palette_base / 4) + color];

	palette_set_color_rgb(machine(), color, pal5bit(data >> 6), pal6bit(data >> 0), pal5bit(data >> 11));
}

READ32_MEMBER( k037122_device::sram_r )
{
	return m_tile_ram[offset];
}

WRITE32_MEMBER( k037122_device::sram_w )
{
	COMBINE_DATA(m_tile_ram + offset);

	if (m_reg[0xc] & 0x10000)
	{
		if (offset < 0x8000 / 4)
		{
			m_layer[1]->mark_tile_dirty(offset);
		}
		else if (offset >= 0x8000 / 4 && offset < 0x18000 / 4)
		{
			m_layer[0]->mark_tile_dirty(offset - (0x8000 / 4));
		}
		else if (offset >= 0x18000 / 4)
		{
			update_palette_color(0x18000, offset - (0x18000 / 4));
		}
	}
	else
	{
		if (offset < 0x8000 / 4)
		{
			update_palette_color(0, offset);
		}
		else if (offset >= 0x8000 / 4 && offset < 0x18000 / 4)
		{
			m_layer[0]->mark_tile_dirty(offset - (0x8000 / 4));
		}
		else if (offset >= 0x18000 / 4)
		{
			m_layer[1]->mark_tile_dirty(offset - (0x18000 / 4));
		}
	}
}


READ32_MEMBER( k037122_device::char_r )
{
	int bank = m_reg[0x30 / 4] & 0x7;

	return m_char_ram[offset + (bank * (0x40000 / 4))];
}

WRITE32_MEMBER( k037122_device::char_w )
{
	int bank = m_reg[0x30 / 4] & 0x7;
	UINT32 addr = offset + (bank * (0x40000/4));

	COMBINE_DATA(m_char_ram + addr);
	space.machine().gfx[m_gfx_index]->mark_dirty(addr / 32);
}

READ32_MEMBER( k037122_device::reg_r )
{
	switch (offset)
	{
		case 0x14/4:
		{
			return 0x000003fa;
		}
	}
	return m_reg[offset];
}

WRITE32_MEMBER( k037122_device::reg_w )
{
	COMBINE_DATA(m_reg + offset);
}
