/*
 * PlayStation Serial I/O emulator
 *
 * Copyright 2003-2011 smf
 *
 */

#include "sio.h"

#define VERBOSE_LEVEL ( 0 )

INLINE void ATTR_PRINTF(3,4) verboselog( running_machine& machine, int n_level, const char *s_fmt, ... )
{
	if( VERBOSE_LEVEL >= n_level )
	{
		va_list v;
		char buf[ 32768 ];
		va_start( v, s_fmt );
		vsprintf( buf, s_fmt, v );
		va_end( v );
		logerror( "%s: %s", machine.describe_context(), buf );
	}
}

const device_type PSX_SIO0 = &device_creator<psxsio0_device>;
const device_type PSX_SIO1 = &device_creator<psxsio1_device>;

psxsio0_device::psxsio0_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	psxsio_device(mconfig, PSX_SIO0, tag, owner, clock)
{
}

psxsio1_device::psxsio1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	psxsio_device(mconfig, PSX_SIO1, tag, owner, clock)
{
}

psxsio_device::psxsio_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, type, "PSX SIO", tag, owner, clock),
	m_irq_handler(*this)
{
}

void psxsio_device::device_post_load()
{
	sio_timer_adjust();
}

void psxsio_device::device_start()
{
	m_irq_handler.resolve_safe();

	m_timer = timer_alloc( 0 );
	m_status = SIO_STATUS_TX_EMPTY | SIO_STATUS_TX_RDY;
	m_mode = 0;
	m_control = 0;
	m_baud = 0;
	m_tx = 0;
	m_rx = 0;
	m_tx_prev = 0;
	m_rx_prev = 0;
	m_rx_data = 0;
	m_tx_data = 0;
	m_rx_shift = 0;
	m_tx_shift = 0;
	m_rx_bits = 0;
	m_tx_bits = 0;

	save_item( NAME( m_status ) );
	save_item( NAME( m_mode ) );
	save_item( NAME( m_control ) );
	save_item( NAME( m_baud ) );
	save_item( NAME( m_tx ) );
	save_item( NAME( m_rx ) );
	save_item( NAME( m_tx_prev ) );
	save_item( NAME( m_rx_prev ) );
	save_item( NAME( m_rx_data ) );
	save_item( NAME( m_tx_data ) );
	save_item( NAME( m_rx_shift ) );
	save_item( NAME( m_tx_shift ) );
	save_item( NAME( m_rx_bits ) );
	save_item( NAME( m_tx_bits ) );

	deviceCount = 0;

	for( device_t *device = first_subdevice(); device != NULL; device = device->next() )
	{
		psxsiodev_device *psxsiodev = dynamic_cast<psxsiodev_device *>(device);
		if( psxsiodev != NULL )
		{
			devices[ deviceCount++ ] = psxsiodev;
			psxsiodev->m_psxsio = this;
		}
	}

	input_update();
}

void psxsio_device::sio_interrupt()
{
	verboselog( machine(), 1, "sio_interrupt( %s )\n", tag() );
	m_status |= SIO_STATUS_IRQ;
	m_irq_handler(1);
}

void psxsio_device::sio_timer_adjust()
{
	attotime n_time;

	if( ( m_status & SIO_STATUS_TX_EMPTY ) == 0 || m_tx_bits != 0 )
	{
		int n_prescaler;

		switch( m_mode & 3 )
		{
		case 1:
			n_prescaler = 1;
			break;
		case 2:
			n_prescaler = 16;
			break;
		case 3:
			n_prescaler = 64;
			break;
		default:
			n_prescaler = 0;
			break;
		}

		if( m_baud != 0 && n_prescaler != 0 )
		{
			n_time = attotime::from_hz(33868800) * (n_prescaler * m_baud);
			verboselog( machine(), 2, "sio_timer_adjust( %s ) = %s ( %d x %d )\n", tag(), n_time.as_string(), n_prescaler, m_baud );
		}
		else
		{
			n_time = attotime::never;
			verboselog( machine(), 0, "sio_timer_adjust( %s ) invalid baud rate ( %d x %d )\n", tag(), n_prescaler, m_baud );
		}
	}
	else
	{
		n_time = attotime::never;
		verboselog( machine(), 2, "sio_timer_adjust( %s ) finished\n", tag() );
	}

	m_timer->adjust( n_time );
}

void psxsio_device::output( int data, int mask )
{
	int new_outputdata = ( m_outputdata & ~mask ) | ( data & mask );
	int new_mask = m_outputdata ^ new_outputdata;

	if( new_mask != 0 )
	{
		m_outputdata = new_outputdata;

		for( int i = 0; i < deviceCount; i++ )
		{
			devices[ i ]->data_in( m_outputdata, new_mask );
		}
	}
}

void psxsio_device::device_timer(emu_timer &timer, device_timer_id tid, int param, void *ptr)
{
	verboselog( machine(), 2, "sio tick\n" );

	if( m_tx_bits == 0 &&
		( m_control & SIO_CONTROL_TX_ENA ) != 0 &&
		( m_status & SIO_STATUS_TX_EMPTY ) == 0 )
	{
		m_tx_bits = 8;
		m_tx_shift = m_tx_data;

		if( type() == PSX_SIO0 )
		{
			m_rx_bits = 8;
			m_rx_shift = 0;
		}

		m_status |= SIO_STATUS_TX_EMPTY;
		m_status |= SIO_STATUS_TX_RDY;
	}

	if( m_tx_bits != 0 )
	{
		m_tx = ( m_tx & ~PSX_SIO_OUT_DATA ) | ( ( m_tx_shift & 1 ) * PSX_SIO_OUT_DATA );
		m_tx_shift >>= 1;
		m_tx_bits--;

		if( type() == PSX_SIO0 )
		{
			m_tx &= ~PSX_SIO_OUT_CLOCK;
			output( m_tx, PSX_SIO_OUT_CLOCK | PSX_SIO_OUT_DATA );
			m_tx |= PSX_SIO_OUT_CLOCK;
		}

		output( m_tx, PSX_SIO_OUT_CLOCK | PSX_SIO_OUT_DATA );

		if( m_tx_bits == 0 &&
			( m_control & SIO_CONTROL_TX_IENA ) != 0 )
		{
			sio_interrupt();
		}
	}

	if( m_rx_bits != 0 )
	{
		m_rx_shift = ( m_rx_shift >> 1 ) | ( ( ( m_rx & PSX_SIO_IN_DATA ) / PSX_SIO_IN_DATA ) << 7 );
		m_rx_bits--;

		if( m_rx_bits == 0 )
		{
			if( ( m_status & SIO_STATUS_RX_RDY ) != 0 )
			{
				m_status |= SIO_STATUS_OVERRUN;
			}
			else
			{
				m_rx_data = m_rx_shift;
				m_status |= SIO_STATUS_RX_RDY;
			}

			if( ( m_control & SIO_CONTROL_RX_IENA ) != 0 )
			{
				sio_interrupt();
			}
		}
	}

	sio_timer_adjust();
}

WRITE32_MEMBER( psxsio_device::write )
{
	switch( offset % 4 )
	{
	case 0:
		verboselog( machine(), 1, "psx_sio_w %s data %02x (%08x)\n", tag(), data, mem_mask );
		m_tx_data = data;
		m_status &= ~( SIO_STATUS_TX_RDY );
		m_status &= ~( SIO_STATUS_TX_EMPTY );
		sio_timer_adjust();
		break;
	case 1:
		verboselog( machine(), 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	case 2:
		if( ACCESSING_BITS_0_15 )
		{
			m_mode = data & 0xffff;
			verboselog( machine(), 1, "psx_sio_w %s mode %04x\n", tag(), data & 0xffff );
		}
		if( ACCESSING_BITS_16_31 )
		{
			verboselog( machine(), 1, "psx_sio_w %s control %04x\n", tag(), data >> 16 );
			m_control = data >> 16;

			if( ( m_control & SIO_CONTROL_RESET ) != 0 )
			{
				verboselog( machine(), 1, "psx_sio_w reset\n" );
				m_status |= SIO_STATUS_TX_EMPTY | SIO_STATUS_TX_RDY;
				m_status &= ~( SIO_STATUS_RX_RDY | SIO_STATUS_OVERRUN | SIO_STATUS_IRQ );

				// toggle DTR to reset controllers, Star Ocean 2, at least, requires it
				// the precise mechanism of the reset is unknown
				// maybe it's related to the bottom 2 bits of control which are usually set
				output( m_tx ^ PSX_SIO_OUT_DTR, PSX_SIO_OUT_DTR );
			}
			if( ( m_control & SIO_CONTROL_IACK ) != 0 )
			{
				verboselog( machine(), 1, "psx_sio_w iack\n" );
				m_status &= ~( SIO_STATUS_IRQ );
				m_control &= ~( SIO_CONTROL_IACK );
			}
			if( ( m_control & SIO_CONTROL_DTR ) != 0 )
			{
				m_tx |= PSX_SIO_OUT_DTR;
			}
			else
			{
				m_tx &= ~PSX_SIO_OUT_DTR;
			}

			output( m_tx, PSX_SIO_OUT_DTR );

			m_tx_prev = m_tx;

		}
		break;
	case 3:
		if( ACCESSING_BITS_0_15 )
		{
			verboselog( machine(), 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		}
		if( ACCESSING_BITS_16_31 )
		{
			m_baud = data >> 16;
			verboselog( machine(), 1, "psx_sio_w %s baud %04x\n", tag(), data >> 16 );
		}
		break;
	default:
		verboselog( machine(), 0, "psx_sio_w( %08x, %08x, %08x )\n", offset, data, mem_mask );
		break;
	}
}

READ32_MEMBER( psxsio_device::read )
{
	UINT32 data;

	switch( offset % 4 )
	{
	case 0:
		data = m_rx_data;
		m_status &= ~( SIO_STATUS_RX_RDY );
		m_rx_data = 0xff;
		verboselog( machine(), 1, "psx_sio_r %s data %02x (%08x)\n", tag(), data, mem_mask );
		break;
	case 1:
		data = m_status;
		if( ACCESSING_BITS_0_15 )
		{
			verboselog( machine(), 1, "psx_sio_r %s status %04x\n", tag(), data & 0xffff );
		}
		if( ACCESSING_BITS_16_31 )
		{
			verboselog( machine(), 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		}
		break;
	case 2:
		data = ( m_control << 16 ) | m_mode;
		if( ACCESSING_BITS_0_15 )
		{
			verboselog( machine(), 1, "psx_sio_r %s mode %04x\n", tag(), data & 0xffff );
		}
		if( ACCESSING_BITS_16_31 )
		{
			verboselog( machine(), 1, "psx_sio_r %s control %04x\n", tag(), data >> 16 );
		}
		break;
	case 3:
		data = m_baud << 16;
		if( ACCESSING_BITS_0_15 )
		{
			verboselog( machine(), 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		}
		if( ACCESSING_BITS_16_31 )
		{
			verboselog( machine(), 1, "psx_sio_r %s baud %04x\n", tag(), data >> 16 );
		}
		break;
	default:
		data = 0;
		verboselog( machine(), 0, "psx_sio_r( %08x, %08x ) %08x\n", offset, mem_mask, data );
		break;
	}
	return data;
}

void psxsio_device::input_update()
{
	int data = 0;

	for( int i = 0; i < deviceCount; i++ )
	{
		data |= devices[ i ]->m_dataout;
	}

	int mask = data ^ m_rx;

	verboselog( machine(), 1, "input_update( %s, %02x, %02x )\n", tag(), mask, data );

	m_rx = data;

	if( ( m_rx & PSX_SIO_IN_DSR ) != 0 )
	{
		m_status |= SIO_STATUS_DSR;
		if( ( m_rx_prev & PSX_SIO_IN_DSR ) == 0 &&
			( m_control & SIO_CONTROL_DSR_IENA ) != 0 )
		{
			sio_interrupt();
		}
	}
	else
	{
		m_status &= ~SIO_STATUS_DSR;
	}
	m_rx_prev = m_rx;
}
