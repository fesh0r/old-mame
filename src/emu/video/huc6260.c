/**********************************************************************

    Hudson/NEC HuC6260 Video Colour Encoder

    The HuC6260 takes a stream of pixel data, looks up the correct
    palette data and outputs a video stream.

    The HuC6260 generates the tv control signals. A full line lasts
    1365 "master" cycles (typically at 21.47727Mhz).

    HSync is low for 237 and high for 1128 master cycles.
    VSync is low for 4095 master cycles (3 lines).
    VSync changes 30 master cycles after HSync would go low.

**********************************************************************/

#include "emu.h"
#include "profiler.h"
#include "huc6260.h"

#define LOG 0

#define HUC6260_HSYNC_LENGTH    237
#define HUC6260_HSYNC_START     ( HUC6260_WPF - HUC6260_HSYNC_LENGTH )


PALETTE_INIT( huc6260 )
{
	int i;

	for ( i = 0; i < 512; i++ )
	{
		int r = ( ( i >> 3 ) & 7 ) << 5;
		int g = ( ( i >> 6 ) & 7 ) << 5;
		int b = ( ( i      ) & 7 ) << 5;
		int y = ( ( 66 * r + 129 * g + 25 * b + 128 ) >> 8 ) + 16;

		palette_set_color_rgb( machine, i, r, g, b );
		palette_set_color_rgb( machine, 512 + i, y, y, y );
	}
}


const device_type HUC6260 = &device_creator<huc6260_device>;


void huc6260_device::device_config_complete()
{
	const huc6260_interface *intf = reinterpret_cast<const huc6260_interface *>(static_config());

	if ( intf != NULL )
	{
		*static_cast<huc6260_interface *>(this) = *intf;
	}
	else
	{
		memset(&m_get_next_pixel_data, 0, sizeof(m_get_next_pixel_data));
		memset(&m_get_time_til_next_event, 0, sizeof(m_get_time_til_next_event));
		memset(&m_hsync_changed, 0, sizeof(m_hsync_changed));
		memset(&m_vsync_changed, 0, sizeof(m_vsync_changed));
	}
}


huc6260_device::huc6260_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, HUC6260, "HuC6260", tag, owner, clock)
{
}


void huc6260_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	int vpos = m_screen->vpos();
	int hpos = m_screen->hpos();
	int h = m_last_h;
	int v = m_last_v;
	UINT16 *bitmap_line = &m_bmp->pix16(v);

	while ( h != hpos || v != vpos )
	{
		if ( m_pixel_clock == 0 )
		{
			g_profiler.start( PROFILER_VIDEO );
			/* Get next pixel information */
			m_pixel_data = m_get_next_pixel_data( 0, 0xffff );
			g_profiler.stop();
			if ( m_greyscales )
			{
				m_pixel_data += 512;
			}
		}

		bitmap_line[ h ] = m_palette[ m_pixel_data ];
		m_pixel_clock = ( m_pixel_clock + 1 ) % m_pixels_per_clock;
		h = ( h + 1 ) % HUC6260_WPF;

		switch( h )
		{
		case HUC6260_HSYNC_START:       /* Start of HSync */
			m_hsync_changed( 0 );
//          if ( v == 0 )
//          {
//              /* Check if the screen should be resized */
//              m_height = HUC6260_LPF - ( m_blur ? 1 : 0 );
//              if ( m_height != video_screen_get_height( m_screen ) )
//              {
//                  rectangle visible_area;
//
//                  /* TODO: Set proper visible area parameters */
//                  visible_area.min_x = 64;
//                  visible_area.min_y = 18;
//                  visible_area.max_x = 64 + 1024 + 64 - 1;
//                  visible_area.max_y = 18 + 242 - 1;
//
//                  video_screen_configure( m_screen, HUC6260_WPF, m_height, &visible_area, HZ_TO_ATTOSECONDS( device->clock / ( HUC6260_WPF * m_height ) ) );
//              }
//          }
			break;

		case 0:     /* End of HSync */
			m_hsync_changed( 1 );
			m_pixel_clock = 0;
			v = ( v + 1 ) % m_height;
			bitmap_line = &m_bmp->pix16(v);
			break;

		case HUC6260_HSYNC_START + 30:      /* End/Start of VSync */
			if ( v>= m_height - 4 )
			{
				m_vsync_changed( ( v >= m_height - 4 && v < m_height - 1 ) ? 0 : 1 );
			}
			break;
		}
	}

	m_last_h = h;
	m_last_v = v;

	/* Reschedule timer */
	if ( m_last_h < HUC6260_HSYNC_START )
	{
		/* Next event is start of HSync signal */
		v = m_last_v;
		h = HUC6260_HSYNC_START;
	}
	else if ( ( m_last_v == m_height - 4 || m_last_v == m_height - 1 ) && m_last_h < HUC6260_HSYNC_START + 30 )
	{
		/* Next event is start/end of VSync signal */
		v = m_last_v;
		h = HUC6260_HSYNC_START + 30;
	}
	else
	{
		/* Next event is end of HSync signal */
		v = ( m_last_v + 1 ) % m_height;
		h = 0;
	}

	/* Ask our slave device for time until next possible event */
	{
		UINT16 next_event_clocks = m_get_time_til_next_event( 0, 0xffff );
		int event_hpos, event_vpos;

		/* Adjust for pixel clocks per pixel */
		next_event_clocks *= m_pixels_per_clock;

		/* Adjust for clocks left to go for current pixel */
		next_event_clocks += ( m_pixels_per_clock - ( m_pixel_clock + 1 ) );

		event_hpos = hpos + next_event_clocks;
		event_vpos = vpos;
		while ( event_hpos > HUC6260_WPF )
		{
			event_vpos += 1;
			event_hpos -= HUC6260_WPF;
		}

		if ( event_vpos < v || ( event_vpos == v && event_hpos <= h ) )
		{
			if ( event_vpos > vpos || ( event_vpos == vpos && event_hpos > hpos ) )
			{
				v = event_vpos;
				h = event_hpos;
			}
		}
	}

	m_timer->adjust( m_screen->time_until_pos( v, h ) );
}


void huc6260_device::video_update( bitmap_ind16 &bitmap, const rectangle &cliprect )
{
	copybitmap( bitmap, *m_bmp, 0, 0, 0, 0, cliprect );
}


READ8_MEMBER( huc6260_device::read )
{
	UINT8 data = 0xFF;

	switch ( offset & 7 )
	{
		case 0x04:  /* Color table data LSB */
			data = m_palette[ m_address ] & 0xFF;
			break;

		case 0x05:  /* Color table data MSB */
			data = 0xFE | ( m_palette[ m_address ] >> 8 );

			/* Increment internal address */
			m_address = ( m_address + 1 ) & 0x1FF;
			break;
	}

	return data;
}


WRITE8_MEMBER( huc6260_device::write )
{
	switch ( offset & 7 )
	{
		case 0x00:  /* Control register */
			m_greyscales = data & 0x80;
			m_blur = data & 0x04;
			m_pixels_per_clock = ( data & 0x02 ) ? 2 : ( ( data & 0x01 ) ? 3 : 4 );
			break;

		case 0x02:  /* Color table address LSB */
			m_address = ( ( m_address & 0xFF00 ) | data ) & 0x1FF;
			break;

		case 0x03:  /* Color table address MSB */
			m_address = ( ( m_address & 0x00FF ) | ( data << 8 ) ) & 0x1FF;
			break;

		case 0x04:  /* Color table data LSB */
			m_palette[ m_address ] = ( ( m_palette[ m_address ] & 0xFF00 ) | data ) & 0x1FF;
			break;

		case 0x05:  /* Color table data MSB */
			m_palette[ m_address ] = ( ( m_palette[ m_address ] & 0x00FF ) | ( data << 8 ) ) & 0x1FF;

			/* Increment internal address */
			m_address = ( m_address + 1 ) & 0x1FF;
			break;
	}
}


void huc6260_device::device_start()
{
	/* Make sure we are supplied a screen tag */
	assert( screen_tag != NULL );

	m_timer = timer_alloc();
	m_screen = machine().device<screen_device>( screen_tag );
	m_bmp = auto_bitmap_ind16_alloc( machine(), HUC6260_WPF, HUC6260_LPF );

	/* Resolve callbacks */
	m_hsync_changed.resolve( hsync_changed, *this );
	m_vsync_changed.resolve( vsync_changed, *this );
	m_get_next_pixel_data.resolve( get_next_pixel_data, *this );
	m_get_time_til_next_event.resolve( get_time_til_next_event, *this );

	/* We want to have a valid screen and valid callbacks */
	assert( m_screen != NULL );
	assert( ! m_hsync_changed.isnull() );
	assert( ! m_vsync_changed.isnull() );
	assert( ! m_get_next_pixel_data.isnull() );
	assert( ! m_get_time_til_next_event.isnull() );

	save_item(NAME(m_last_h));
	save_item(NAME(m_last_v));
	save_item(NAME(m_height));
	save_item(NAME(m_palette));
	save_item(NAME(m_address));
	save_item(NAME(m_greyscales));
	save_item(NAME(m_blur));
	save_item(NAME(m_pixels_per_clock));
	save_item(NAME(m_pixel_data));
	save_item(NAME(m_pixel_clock));
}


void huc6260_device::device_reset()
{
	m_address = 0;
	m_greyscales = 0;
	m_blur = 0;
	m_pixels_per_clock = 4;
	m_height = 263;

	m_last_v = m_screen->vpos();
	m_last_h = m_screen->hpos();
	m_timer->adjust( m_screen->time_until_pos( ( m_screen->vpos() + 1 ) % 263, 0 ) );
}
