/**********************************************************************

    Hudson/NEC HuC6202 Video Priority Controller

**********************************************************************/

#include "emu.h"
#include "huc6270.h"
#include "huc6202.h"


const device_type HUC6202 = &device_creator<huc6202_device>;


void huc6202_device::device_config_complete()
{
	const huc6202_interface *intf = reinterpret_cast<const huc6202_interface *>(static_config());

	if ( intf != NULL )
	{
		*static_cast<huc6202_interface *>(this) = *intf;
	}
	else
	{
		memset(&m_next_pixel_0, 0, sizeof(m_next_pixel_0));
		memset(&m_get_time_til_next_event_0, 0, sizeof(m_get_time_til_next_event_0));
		memset(&m_hsync_changed_0, 0, sizeof(m_hsync_changed_0));
		memset(&m_vsync_changed_0, 0, sizeof(m_vsync_changed_0));
		memset(&m_read_0, 0, sizeof(m_read_0));
		memset(&m_write_0, 0, sizeof(m_write_0));
		memset(&m_next_pixel_1, 0, sizeof(m_next_pixel_1));
		memset(&m_get_time_til_next_event_1, 0, sizeof(m_get_time_til_next_event_1));
		memset(&m_hsync_changed_1, 0, sizeof(m_hsync_changed_1));
		memset(&m_vsync_changed_1, 0, sizeof(m_vsync_changed_1));
		memset(&m_read_1, 0, sizeof(m_read_1));
		memset(&m_write_1, 0, sizeof(m_write_1));
	}
}


huc6202_device::huc6202_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, HUC6202, "HuC6202", tag, owner, clock)
{
}


READ16_MEMBER( huc6202_device::next_pixel )
{
	UINT16 data = huc6270_device::HUC6270_BACKGROUND;

	UINT16 data_0 = m_next_pixel_0( 0, 0xffff );
	UINT16 data_1 = m_next_pixel_1( 0, 0xffff );

	if ( data_0 == huc6270_device::HUC6270_SPRITE && data_1 == huc6270_device::HUC6270_SPRITE )
	{
		m_map_index = 0;
		if ( m_map_dirty )
		{
			int i;

			for ( i = 0; i < 512; i++ )
			{
				m_prio_map[ i ] = 0;
				if ( m_window1 < 0x40 || i > m_window1 )
				{
					m_prio_map [ i ] |= 1;
				}
				if ( m_window2 < 0x40 || i > m_window2 )
				{
					m_prio_map[ i ] |= 2;
				}
			}
			m_map_dirty = 0;
		}
	}
	else
	{
		UINT8	prio_type = m_prio[ m_prio_map[ m_map_index ] ].prio_type;

		if ( m_prio[ prio_type ].dev0_enabled && data_0 != huc6270_device::HUC6270_SPRITE )
		{
			if ( m_prio[ prio_type ].dev1_enabled && data_1 != huc6270_device::HUC6270_SPRITE )
			{
				switch ( prio_type )
				{
				case 0:		/* Back - BG1 SP1 BG0 SP0 - Front */
					data = ( data_0 & 0x0F ) ? data_0 : data_1;
					break;

				case 1:		/* Back - BG1 BG0 SP1 SP0 - Front */
					if ( data_0 > huc6270_device::HUC6270_SPRITE )
					{
						/* Device 0 sprite */
						data = data_0;
					}
					else if ( data_1 > huc6270_device::HUC6270_SPRITE )
					{
						/* Device 1 sprite */
						data = data_1;
					}
					else
					{
						/* Device 0 and 1 backgrounds */
						data = ( data_0 & 0x0F ) ? data_0 : data_1;
					}
					break;

				case 2:		/* Back - BG0 + SP1 => BG0 - Front
                                      BG0 + BG1 => BG0
                                      BG1 + SP0 => BG1
                                      SP0 + SP1 => SP0
                            */
					if ( data_1 > huc6270_device::HUC6270_SPRITE )
					{
						if ( data_0 > huc6270_device::HUC6270_SPRITE )
						{
							/* Device 1 sprite, device 0 sprite */
							data = data_0;
						}
						else
						{
							/* Device 1 sprite, device 0 background */
							data = ( data_0 & 0x0F ) ? data_0 : data_1;
						}
					}
					else
					{
						if ( data_0 > huc6270_device::HUC6270_SPRITE )
						{
							/* Device 1 background, device 0 sprite */
							data = data_1;
						}
						else
						{
							/* Device 1 background, device 0 background */
							data = ( data_0 & 0x0F ) ? data_0 : data_1;
						}
					}
					break;

				case 3:		/* ?? */
					break;
				}
			}
			else
			{
				/* Only device 0 is enabled */
				data = data_0;
			}
		}
		else
		{
			/* Only device 1 is enabled */
			if ( m_prio[ prio_type ].dev1_enabled && data_1 != huc6270_device::HUC6270_SPRITE )
			{
				data = data_1;
			}
		}
		m_map_index += 1;
	}
	return data;
}


READ16_MEMBER( huc6202_device::time_until_next_event )
{
	UINT16 next_event_clocks_0 = m_get_time_til_next_event_0( 0, 0xffff  );
	UINT16 next_event_clocks_1 = m_get_time_til_next_event_1( 0, 0xffff );

	return MIN( next_event_clocks_0, next_event_clocks_1 );
}


WRITE_LINE_MEMBER( huc6202_device::vsync_changed )
{
	m_vsync_changed_0( state );
	m_vsync_changed_1( state );
}


WRITE_LINE_MEMBER( huc6202_device::hsync_changed )
{
	m_hsync_changed_0( state );
	m_hsync_changed_1( state );
}


READ8_MEMBER( huc6202_device::read )
{
	UINT8 data = 0xFF;

	switch ( offset & 7 )
	{
		case 0x00:	/* Priority register #0 */
			data = ( m_prio[0].prio_type << 2 ) |
				( m_prio[0].dev0_enabled ? 0x01 : 0 ) |
				( m_prio[0].dev1_enabled ? 0x02 : 0 ) |
				( m_prio[1].prio_type << 6 ) |
				( m_prio[1].dev0_enabled ? 0x10 : 0 ) |
				( m_prio[1].dev1_enabled ? 0x20 : 0 );
			break;

		case 0x01:	/* Priority register #1 */
			data = ( m_prio[2].prio_type << 2 ) |
				( m_prio[2].dev0_enabled ? 0x01 : 0 ) |
				( m_prio[2].dev1_enabled ? 0x02 : 0 ) |
				( m_prio[3].prio_type << 6 ) |
				( m_prio[3].dev0_enabled ? 0x10 : 0 ) |
				( m_prio[3].dev1_enabled ? 0x20 : 0 );
			break;

		case 0x02:	/* Window 1 LSB */
			data = m_window1 & 0xFF;
			break;

		case 0x03:	/* Window 1 MSB */
			data = ( m_window1 >> 8 ) & 0xFF;
			break;

		case 0x04:	/* Window 2 LSB */
			data = m_window2 & 0xFF;
			break;

		case 0x05:	/* Window 2 MSB */
			data = ( m_window2 >> 8 ) & 0xFF;
			break;
	}

	return data;
}


WRITE8_MEMBER( huc6202_device::write )
{
	switch ( offset & 7 )
	{
		case 0x00:	/* Priority register #0 */
			m_prio[0].dev0_enabled = data & 0x01;
			m_prio[0].dev1_enabled = data & 0x02;
			m_prio[0].prio_type = ( data >> 2 ) & 0x03;
			m_prio[1].dev0_enabled = data & 0x10;
			m_prio[1].dev1_enabled = data & 0x20;
			m_prio[1].prio_type = ( data >> 6 ) & 0x03;
			break;

		case 0x01:	/* Priority register #1 */
			m_prio[2].dev0_enabled = data & 0x01;
			m_prio[2].dev1_enabled = data & 0x02;
			m_prio[2].prio_type = ( data >> 2 ) & 0x03;
			m_prio[3].dev0_enabled = data & 0x10;
			m_prio[3].dev1_enabled = data & 0x20;
			m_prio[3].prio_type = ( data >> 6 ) & 0x03;
			break;

		case 0x02:	/* Window 1 LSB */
			m_window1 = ( m_window1 & 0xFF00 ) | data;
			m_map_dirty = 1;
			break;

		case 0x03:	/* Window 1 MSB */
			m_window1 = ( ( m_window1 & 0x00FF ) | ( data << 8 ) ) & 0x3FF;
			m_map_dirty = 1;
			break;

		case 0x04:	/* Window 2 LSB */
			m_window2 = ( m_window2 & 0xFF00 ) | data;
			m_map_dirty = 1;
			break;

		case 0x05:	/* Window 2 MSB */
			m_window2 = ( ( m_window2 & 0x00FF ) | ( data << 8 ) ) & 0x3FF;
			m_map_dirty = 1;
			break;

		case 0x06:	/* I/O select */
			m_io_device = data & 0x01;
			break;
	}
}


READ8_MEMBER( huc6202_device::io_read )
{
	if ( m_io_device )
	{
		return m_read_1( offset );
	}
	else
	{
		return m_read_0( offset );
	}
}


WRITE8_MEMBER( huc6202_device::io_write )
{
	if ( m_io_device )
	{
		m_write_1( offset, data );
	}
	else
	{
		m_write_0( offset, data );
	}
}


void huc6202_device::device_start()
{
	/* Resolve callbacks */
	m_next_pixel_0.resolve( device_0_next_pixel, *this );
	m_get_time_til_next_event_0.resolve( get_time_til_next_event_0, *this );
	m_hsync_changed_0.resolve( hsync_0_changed, *this );
	m_vsync_changed_0.resolve( vsync_0_changed, *this );
	m_read_0.resolve( read_0, *this );
	m_write_0.resolve( write_0, *this );

	m_next_pixel_1.resolve( device_1_next_pixel, *this );
	m_get_time_til_next_event_1.resolve( get_time_til_next_event_1, *this );
	m_hsync_changed_1.resolve( hsync_1_changed, *this );
	m_vsync_changed_1.resolve( vsync_1_changed, *this );
	m_read_1.resolve( read_1, *this );
	m_write_1.resolve( write_1, *this );

	/* We want all our callbacks to be resolved */
	assert( ! m_next_pixel_0.isnull() );
	assert( ! m_get_time_til_next_event_0.isnull() );
	assert( ! m_hsync_changed_0.isnull() );
	assert( ! m_vsync_changed_0.isnull() );
	assert( ! m_read_0.isnull() );
	assert( ! m_write_0.isnull() );
	assert( ! m_next_pixel_1.isnull() );
	assert( ! m_get_time_til_next_event_1.isnull() );
	assert( ! m_hsync_changed_1.isnull() );
	assert( ! m_vsync_changed_1.isnull() );
	assert( ! m_read_1.isnull() );
	assert( ! m_write_1.isnull() );

	/* Register save items */
	save_item(NAME(m_prio[0].prio_type));
	save_item(NAME(m_prio[0].dev0_enabled));
	save_item(NAME(m_prio[0].dev1_enabled));
	save_item(NAME(m_prio[1].prio_type));
	save_item(NAME(m_prio[1].dev0_enabled));
	save_item(NAME(m_prio[1].dev1_enabled));
	save_item(NAME(m_prio[2].prio_type));
	save_item(NAME(m_prio[2].dev0_enabled));
	save_item(NAME(m_prio[2].dev1_enabled));
	save_item(NAME(m_prio[3].prio_type));
	save_item(NAME(m_prio[3].dev0_enabled));
	save_item(NAME(m_prio[3].dev1_enabled));
	save_item(NAME(m_window1));
	save_item(NAME(m_window2));
	save_item(NAME(m_io_device));
	save_item(NAME(m_map_index));
	save_item(NAME(m_map_dirty));
	save_item(NAME(m_prio_map));
}


void huc6202_device::device_reset()
{
	m_prio[0].prio_type = 0;
	m_prio[0].dev0_enabled = 1;
	m_prio[0].dev1_enabled = 0;
	m_prio[1].prio_type = 0;
	m_prio[1].dev0_enabled = 1;
	m_prio[1].dev1_enabled = 0;
	m_prio[2].prio_type = 0;
	m_prio[2].dev0_enabled = 1;
	m_prio[2].dev1_enabled = 0;
	m_prio[3].prio_type = 0;
	m_prio[3].dev0_enabled = 1;
	m_prio[3].dev1_enabled = 0;
	m_map_dirty = 1;
	m_window1 = 0;
	m_window2 = 0;
	m_io_device = 0;
}

