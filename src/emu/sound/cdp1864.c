/**********************************************************************

    RCA CDP1864C COS/MOS PAL Compatible Color TV Interface

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - interlace mode
    - PAL output, currently using RGB
    - cpu synchronization

        SC1 and SC0 are used to provide CDP1864C-to-CPU synchronization for a jitter-free display.
        During every horizontal sync the CDP1864C samples SC0 and SC1 for SC0 = 1 and SC1 = 0
        (CDP1800 execute state). Detection of a fetch cycle causes the CDP1864C to skip cycles to
        attain synchronization. (i.e. picture moves 8 pixels to the right)

*/


#include "emu.h"
#include "cdp1864.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define CDP1864_DEFAULT_LATCH		0x35

#define CDP1864_CYCLES_DMA_START	2*8
#define CDP1864_CYCLES_DMA_ACTIVE	8*8
#define CDP1864_CYCLES_DMA_WAIT		6*8

static const int CDP1864_BACKGROUND_COLOR_SEQUENCE[] = { 2, 0, 1, 4 };



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// devices
const device_type CDP1864 = cdp1864_device_config::static_alloc_device_config;



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************


//-------------------------------------------------
//  cdp1864_device_config - constructor
//-------------------------------------------------

cdp1864_device_config::cdp1864_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
	: device_config(mconfig, static_alloc_device_config, "CDP1864", tag, owner, clock),
	  device_config_sound_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  static_alloc_device_config - allocate a new
//  configuration object
//-------------------------------------------------

device_config *cdp1864_device_config::static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock)
{
	return global_alloc(cdp1864_device_config(mconfig, tag, owner, clock));
}


//-------------------------------------------------
//  alloc_device - allocate a new device object
//-------------------------------------------------

device_t *cdp1864_device_config::alloc_device(running_machine &machine) const
{
	return auto_alloc(machine, cdp1864_device(machine, *this));
}


//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void cdp1864_device_config::device_config_complete()
{
	// inherit a copy of the static data
	const cdp1864_interface *intf = reinterpret_cast<const cdp1864_interface *>(static_config());
	if (intf != NULL)
		*static_cast<cdp1864_interface *>(this) = *intf;

	// or initialize to defaults if none provided
	else
	{
		memset(&m_in_inlace_func, 0, sizeof(m_in_inlace_func));
		memset(&m_in_rdata_func, 0, sizeof(m_in_rdata_func));
		memset(&m_in_bdata_func, 0, sizeof(m_in_bdata_func));
		memset(&m_in_gdata_func, 0, sizeof(m_in_gdata_func));
		memset(&m_out_int_func, 0, sizeof(m_out_int_func));
		memset(&m_out_dmao_func, 0, sizeof(m_out_dmao_func));
		memset(&m_out_efx_func, 0, sizeof(m_out_efx_func));
		memset(&m_out_hsync_func, 0, sizeof(m_out_hsync_func));
	}
}



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  initialize_palette -
//-------------------------------------------------

inline void cdp1864_device::initialize_palette()
{
	double res_total = m_config.m_res_r + m_config.m_res_g + m_config.m_res_b + m_config.m_res_bkg;

	int weight_r = (m_config.m_res_r / res_total) * 100;
	int weight_g = (m_config.m_res_g / res_total) * 100;
	int weight_b = (m_config.m_res_b / res_total) * 100;
	int weight_bkg = (m_config.m_res_bkg / res_total) * 100;

	for (int i = 0; i < 16; i++)
	{
		int luma = 0;

		luma += (i & 4) ? weight_r : 0;
		luma += (i & 1) ? weight_g : 0;
		luma += (i & 2) ? weight_b : 0;
		luma += (i & 8) ? 0 : weight_bkg;

		luma = (luma * 0xff) / 100;

		int r = (i & 4) ? luma : 0;
		int g = (i & 1) ? luma : 0;
		int b = (i & 2) ? luma : 0;

		palette_set_color_rgb(machine(), i, r, g, b);
	}
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cdp1864_device - constructor
//-------------------------------------------------

cdp1864_device::cdp1864_device(running_machine &_machine, const cdp1864_device_config &config)
    : device_t(_machine, config),
	  device_sound_interface(_machine, config, *this),
	  m_disp(0),
	  m_dmaout(0),
	  m_bgcolor(0),
	  m_con(0),
	  m_aoe(0),
	  m_latch(CDP1864_DEFAULT_LATCH),
      m_config(config)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cdp1864_device::device_start()
{
	// resolve callbacks
	devcb_resolve_read_line(&m_in_inlace_func, &m_config.m_in_inlace_func, this);
	devcb_resolve_read_line(&m_in_rdata_func, &m_config.m_in_rdata_func, this);
	devcb_resolve_read_line(&m_in_bdata_func, &m_config.m_in_bdata_func, this);
	devcb_resolve_read_line(&m_in_gdata_func, &m_config.m_in_gdata_func, this);
	devcb_resolve_write_line(&m_out_int_func, &m_config.m_out_int_func, this);
	devcb_resolve_write_line(&m_out_dmao_func, &m_config.m_out_dmao_func, this);
	devcb_resolve_write_line(&m_out_efx_func, &m_config.m_out_efx_func, this);
	devcb_resolve_write_line(&m_out_hsync_func, &m_config.m_out_hsync_func, this);

	// initialize palette
	initialize_palette();

	// create sound stream
	m_stream = machine().sound().stream_alloc(*this, 0, 1, machine().sample_rate());

	// allocate timers
	m_int_timer = timer_alloc(TIMER_INT);
	m_efx_timer = timer_alloc(TIMER_EFX);
	m_dma_timer = timer_alloc(TIMER_DMA);
	m_hsync_timer = timer_alloc(TIMER_HSYNC);

	// find devices
	m_cpu = machine().device<cpu_device>(m_config.m_cpu_tag);
	m_screen = machine().device<screen_device>(m_config.m_screen_tag);
	m_bitmap = auto_bitmap_alloc(machine(), m_screen->width(), m_screen->height(), m_screen->format());

	// register for state saving
	save_item(NAME(m_disp));
	save_item(NAME(m_dmaout));
	save_item(NAME(m_bgcolor));
	save_item(NAME(m_con));
	save_item(NAME(m_aoe));
	save_item(NAME(m_latch));
	save_item(NAME(m_signal));
	save_item(NAME(m_incr));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void cdp1864_device::device_reset()
{
	m_int_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_INT_START, 0));
	m_efx_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_EFX_TOP_START, 0));
	m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1864_CYCLES_DMA_START));

	m_disp = 0;
	m_dmaout = 0;

	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
}


//-------------------------------------------------
//  device_timer - handle timer events
//-------------------------------------------------

void cdp1864_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	int scanline = m_screen->vpos();

	switch (id)
	{
	case TIMER_INT:
		if (scanline == CDP1864_SCANLINE_INT_START)
		{
			if (m_disp)
			{
				devcb_call_write_line(&m_out_int_func, ASSERT_LINE);
			}

			m_int_timer->adjust(m_screen->time_until_pos( CDP1864_SCANLINE_INT_END, 0));
		}
		else
		{
			if (m_disp)
			{
				devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
			}

			m_int_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_INT_START, 0));
		}
		break;

	case TIMER_EFX:
		switch (scanline)
		{
		case CDP1864_SCANLINE_EFX_TOP_START:
			devcb_call_write_line(&m_out_efx_func, ASSERT_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_EFX_TOP_END, 0));
			break;

		case CDP1864_SCANLINE_EFX_TOP_END:
			devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_EFX_BOTTOM_START, 0));
			break;

		case CDP1864_SCANLINE_EFX_BOTTOM_START:
			devcb_call_write_line(&m_out_efx_func, ASSERT_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_EFX_BOTTOM_END, 0));
			break;

		case CDP1864_SCANLINE_EFX_BOTTOM_END:
			devcb_call_write_line(&m_out_efx_func, CLEAR_LINE);
			m_efx_timer->adjust(m_screen->time_until_pos(CDP1864_SCANLINE_EFX_TOP_START, 0));
			break;
		}
		break;

	case TIMER_DMA:
		if (m_dmaout)
		{
			if (m_disp)
			{
				if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
				{
					devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);
				}
			}

			m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1864_CYCLES_DMA_WAIT));

			m_dmaout = 0;
		}
		else
		{
			if (m_disp)
			{
				if (scanline >= CDP1864_SCANLINE_DISPLAY_START && scanline < CDP1864_SCANLINE_DISPLAY_END)
				{
					devcb_call_write_line(&m_out_dmao_func, ASSERT_LINE);
				}
			}

			m_dma_timer->adjust(m_cpu->cycles_to_attotime(CDP1864_CYCLES_DMA_ACTIVE));

			m_dmaout = 1;
		}
		break;
	}
}


//-------------------------------------------------
//  sound_stream_update - handle update requests for
//  our sound stream
//-------------------------------------------------

void cdp1864_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	// reset the output stream
	memset(outputs[0], 0, samples * sizeof(*outputs[0]));

	INT16 signal = m_signal;
	stream_sample_t *buffer = outputs[0];

	memset( buffer, 0, samples * sizeof(*buffer) );

	if (m_aoe)
	{
		double frequency = m_cpu->unscaled_clock() / 8 / 4 / (m_latch + 1) / 2;
		int rate = machine().sample_rate() / 2;

		/* get progress through wave */
		int incr = m_incr;

		if (signal < 0)
		{
			signal = -0x7fff;
		}
		else
		{
			signal = 0x7fff;
		}

		while( samples-- > 0 )
		{
			*buffer++ = signal;
			incr -= frequency;
			while( incr < 0 )
			{
				incr += rate;
				signal = -signal;
			}
		}

		/* store progress through wave */
		m_incr = incr;
		m_signal = signal;
	}
}


//-------------------------------------------------
//  dispon_r -
//-------------------------------------------------

READ8_MEMBER( cdp1864_device::dispon_r )
{
	m_disp = 1;

	return 0xff;
}


//-------------------------------------------------
//  dispoff_r -
//-------------------------------------------------

READ8_MEMBER( cdp1864_device::dispoff_r )
{
	m_disp = 0;

	devcb_call_write_line(&m_out_int_func, CLEAR_LINE);
	devcb_call_write_line(&m_out_dmao_func, CLEAR_LINE);

	return 0xff;
}


//-------------------------------------------------
//  step_bgcolor_w -
//-------------------------------------------------

WRITE8_MEMBER( cdp1864_device::step_bgcolor_w )
{
	m_disp = 1;

	m_bgcolor++;

	if (m_bgcolor > 3)
	{
		m_bgcolor = 0;
	}
}


//-------------------------------------------------
//  tone_latch_w -
//-------------------------------------------------

WRITE8_MEMBER( cdp1864_device::tone_latch_w )
{
	m_latch = data;
}


//-------------------------------------------------
//  dma_w -
//-------------------------------------------------

WRITE8_MEMBER( cdp1864_device::dma_w )
{
	int rdata = 1, bdata = 1, gdata = 1;
	int sx = m_screen->hpos() + 4;
	int y = m_screen->vpos();

	if (!m_con)
	{
		rdata = devcb_call_read_line(&m_in_rdata_func);
		bdata = devcb_call_read_line(&m_in_bdata_func);
		gdata = devcb_call_read_line(&m_in_gdata_func);
	}

	for (int x = 0; x < 8; x++)
	{
		int color = CDP1864_BACKGROUND_COLOR_SEQUENCE[m_bgcolor] + 8;

		if (BIT(data, 7))
		{
			color = (gdata << 2) | (bdata << 1) | rdata;
		}

		*BITMAP_ADDR16(m_bitmap, y, sx + x) = color;

		data <<= 1;
	}
}


//-------------------------------------------------
//  con_w - color on write
//-------------------------------------------------

WRITE_LINE_MEMBER( cdp1864_device::con_w )
{
	if (!state)
	{
		m_con = 0;
	}
}


//-------------------------------------------------
//  aoe_w - audio output enable write
//-------------------------------------------------

WRITE_LINE_MEMBER( cdp1864_device::aoe_w )
{
	if (!state)
	{
		m_latch = CDP1864_DEFAULT_LATCH;
	}

	m_aoe = state;
}


//-------------------------------------------------
//  evs_w - external vertical sync write
//-------------------------------------------------

WRITE_LINE_MEMBER( cdp1864_device::evs_w )
{
}


//-------------------------------------------------
//  update_screen -
//-------------------------------------------------

void cdp1864_device::update_screen(bitmap_t *bitmap, const rectangle *cliprect)
{
	if (m_disp)
	{
		copybitmap(bitmap, m_bitmap, 0, 0, 0, 0, cliprect);
		bitmap_fill(m_bitmap, cliprect, CDP1864_BACKGROUND_COLOR_SEQUENCE[m_bgcolor] + 8);
	}
	else
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(machine()));
	}
}
