/* Common DECO video functions (general, not sorted by IC) */
/* I think most of this stuff is driver specific and really shouldn't be in a device at all.
   It was only put here because I wanted to split deco_tilegen1 to just be the device for the
   tilemap chips, and not contain all this extra unrelated stuff */


#include "emu.h"
#include "video/decocomn.h"
#include "ui.h"


const device_type DECOCOMN = &device_creator<decocomn_device>;

decocomn_device::decocomn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, DECOCOMN, "Data East Common Video Functions", tag, owner, clock, "decocomn", __FILE__),
	m_screen(NULL),
	m_dirty_palette(NULL),
	m_priority(0)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void decocomn_device::device_config_complete()
{
	// inherit a copy of the static data
	const decocomn_interface *intf = reinterpret_cast<const decocomn_interface *>(static_config());
	if (intf != NULL)
	*static_cast<decocomn_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
	m_screen_tag = "";
	}
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void decocomn_device::device_start()
{
//  int width, height;

	m_screen = machine().device<screen_device>(m_screen_tag);
//  width = m_screen->width();
//  height = m_screen->height();

	m_dirty_palette = auto_alloc_array_clear(machine(), UINT8, 4096);

	save_item(NAME(m_priority));
	save_pointer(NAME(m_dirty_palette), 4096);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void decocomn_device::device_reset()
{
	m_priority = 0;
}

/*****************************************************************************
    DEVICE HANDLERS
*****************************************************************************/

/* Later games have double buffered paletteram - the real palette ram is
only updated on a DMA call */

WRITE16_MEMBER( decocomn_device::nonbuffered_palette_w )
{
	driver_device *state = space.machine().driver_data();

	int r,g,b;

	COMBINE_DATA(&state->m_generic_paletteram_16[offset]);
	if (offset&1) offset--;

	b = (state->m_generic_paletteram_16[offset] >> 0) & 0xff;
	g = (state->m_generic_paletteram_16[offset + 1] >> 8) & 0xff;
	r = (state->m_generic_paletteram_16[offset + 1] >> 0) & 0xff;

	palette_set_color(space.machine(), offset / 2, MAKE_RGB(r,g,b));
}

WRITE16_MEMBER( decocomn_device::buffered_palette_w )
{
	driver_device *state = space.machine().driver_data();

	COMBINE_DATA(&state->m_generic_paletteram_16[offset]);

	m_dirty_palette[offset / 2] = 1;
}

WRITE16_MEMBER( decocomn_device::palette_dma_w )
{
	driver_device *state = space.machine().driver_data();

	const int m = space.machine().total_colors();
	int r, g, b, i;

	for (i = 0; i < m; i++)
	{
		if (m_dirty_palette[i])
		{
			m_dirty_palette[i] = 0;

			b = (state->m_generic_paletteram_16[i * 2] >> 0) & 0xff;
			g = (state->m_generic_paletteram_16[i * 2 + 1] >> 8) & 0xff;
			r = (state->m_generic_paletteram_16[i * 2 + 1] >> 0) & 0xff;

			palette_set_color(space.machine(), i, MAKE_RGB(r,g,b));
		}
	}
}

/*****************************************************************************************/

/* */
READ16_MEMBER( decocomn_device::d_71_r )
{
	return 0xffff;
}

WRITE16_MEMBER( decocomn_device::priority_w )
{
	m_priority = data;
}

READ16_MEMBER( decocomn_device::priority_r )
{
	return m_priority;
}
