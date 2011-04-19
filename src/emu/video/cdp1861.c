/**********************************************************************

    RCA CDP1861 Video Display Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "emu.h"
#include "cdp1861.h"
#include "machine/devhelpr.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CDP1861_CYCLES_DMA_START	2*8
#define CDP1861_CYCLES_DMA_ACTIVE	8*8
#define CDP1861_CYCLES_DMA_WAIT		6*8



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type CDP1861 = cdp1861_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

GENERIC_DEVICE_CONFIG_SETUP(cdp1861, "CDP1861")


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cdp1861_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const cdp1861_interface *intf = reinterpret_cast<const cdp1861_interface *>(static_config());
	if (intf != NULL)
		*static_cast<cdp1861_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
		memset(&m_out_dmao_func, 0, sizeof(m_out_dmao_func));
		memset(&m_out_efx_func, 0, sizeof(m_out_efx_func));
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cdp1861_device - constructor
//-------------------------------------------------

cdp1861_device::cdp1861_device(running_machine &_machine, const cdp1861_device_config &config)
    : device_t(_machine, config),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cdp1861_device::device_start()
{
	// resolve callbacks
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);
	devcb_resolve_write_line(&m_out_dmao_func, &m_config.m_out_dmao_func, this);
	devcb_resolve_write_line(&m_out_efx_func, &m_config.m_out_efx_func, this);

	// allocate timers
	m_int_timer = timer_alloc(TIMER_INT);
	m_efx_timer = timer_alloc(TIMER_EFX);
	m_dma_timer = timer_alloc(TIMER_DMA);

	// find devices
	m_cpu = machine().device<cpu_device>(m_config.m_cpu_tag);
	m_screen =  machine().device<screen_device>(m_config.m_screen_tag);
	m_bitmap = auto_bitmap_alloc(machine(), m_screen->width(), m_screen->height(), m_screen->format());

	// register for state saving
	save_item(NAME(m_disp));
	save_item(NAME(m_dispon));
	save_item(NAME(m_dispoff));
	save_item(NAME(m_dmaout));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cdp1861_device::device_reset()
{
	m_int_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_INT_START, 0));
	m_efx_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_START, 0));
	m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_START));

	m_disp = 0;
	m_dmaout = 0;

	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
}


//-------------------------------------------------
//  device_timer - handle timer events
//-------------------------------------------------

void cdp1861_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	int scanline = m_screen->vpos();

	switch (id)
	{
	case TIMER_INT:
		if (scanline == CDP1861_SCANLINE_INT_START)
		{
			if (m_disp)
			{
				devcb_call_write_line(&m_out_int_func, ASSERT_LINE);
			}

			m_int_timer->adjust(m_screen->time_until_pos( CDP1861_SCANLINE_INT_END, 0));
		}
		else
		{
			if (m_disp)
			{
				devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
			}

			m_int_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_INT_START, 0));
		}
		break;

	case TIMER_EFX:
		switch (scanline)
		{
		case CDP1861_SCANLINE_EFX_TOP_START:
			devcb_call_write_line(&m_out_efx_func, ASSERT_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_END, 0));
			break;

		case CDP1861_SCANLINE_EFX_TOP_END:
			devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_EFX_BOTTOM_START, 0));
			break;

		case CDP1861_SCANLINE_EFX_BOTTOM_START:
			devcb_call_write_line(&m_out_efx_func, ASSERT_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_EFX_BOTTOM_END, 0));
			break;

		case CDP1861_SCANLINE_EFX_BOTTOM_END:
			devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1861_SCANLINE_EFX_TOP_START, 0));
			break;
		}
		break;

	case TIMER_DMA:
		if (m_dmaout)
		{
			if (m_disp)
			{
				if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
				{
					devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);
				}
			}

			m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_WAIT));

			m_dmaout = 0;
		}
		else
		{
			if (m_disp)
			{
				if (scanline >= CDP1861_SCANLINE_DISPLAY_START && scanline < CDP1861_SCANLINE_DISPLAY_END)
				{
					devcb_call_write_line(&m_out_dmao_func, ASSERT_LINE);
				}
			}

			m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1861_CYCLES_DMA_ACTIVE));

			m_dmaout = 1;
		}
		break;
	}
}


//-------------------------------------------------
//  dma_w -
//-------------------------------------------------

WRITE8_MEMBER( cdp1861_device::dma_w )
{
	int sx = m_screen->hpos() + 4;
	int y = m_screen->vpos();
	int x;

	for (x = 0; x < 8; x++)
	{
		int color = BIT(data, 7);
		*BITMAP_ADDR16(m_bitmap, y, sx + x) = color;
		data <<= 1;
	}
}


//-------------------------------------------------
//  disp_on_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( cdp1861_device::disp_on_w )
{
	if (!m_dispon && state) m_disp = 1;

	m_dispon = state;
}


//-------------------------------------------------
//  disp_off_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( cdp1861_device::disp_off_w )
{
	if (!m_dispon && !m_dispoff && state) m_disp = 0;

	m_dispoff = state;

	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);
}


//-------------------------------------------------
//  update_screen -
//-------------------------------------------------

void cdp1861_device::update_screen(bitmap_t *bitmap, const rectangle *cliprect)
{
	if (m_disp)
	{
		copybitmap(bitmap, m_bitmap, 0, 0, 0, 0, cliprect);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(machine()));
	}
}
