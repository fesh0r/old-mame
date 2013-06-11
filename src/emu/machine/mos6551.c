/**********************************************************************

    MOS Technology 6551 Asynchronous Communication Interface Adapter

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

    - receiver disable
    - IRQ on DCD/DSR change
    - parity
    - framing error

*/

#include "mos6551.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define LOG 0


const int mos6551_device::brg_divider[] = {
	0, 2304, 1536, 1048, 856, 768, 384, 192, 96, 64, 48, 32, 24, 16, 12, 6
};



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

// device type definition
const device_type MOS6551 = &device_creator<mos6551_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  mos6551_device - constructor
//-------------------------------------------------

mos6551_device::mos6551_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, MOS6551, "MOS6551", tag, owner, clock),
		device_serial_interface(mconfig, *this),
		m_write_irq(*this),
		m_read_rxd(*this),
		m_write_txd(*this),
		m_write_rts(*this),
		m_write_dtr(*this),
		m_ctrl(0),
		m_cmd(CMD_RIE),
		m_st(ST_TDRE),
		m_ext_rxc(0),
		m_cts(1),
		m_dsr(1),
		m_dcd(1)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void mos6551_device::device_start()
{
	// resolve callbacks
	m_write_irq.resolve_safe();
	m_read_rxd.resolve_safe(1);
	m_write_txd.resolve_safe();
	m_write_rts.resolve_safe();
	m_write_dtr.resolve_safe();

	// state saving
	save_item(NAME(m_ctrl));
	save_item(NAME(m_cmd));
	save_item(NAME(m_st));
	save_item(NAME(m_tdr));
	save_item(NAME(m_ext_rxc));
	save_item(NAME(m_cts));
	save_item(NAME(m_dsr));
	save_item(NAME(m_dcd));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void mos6551_device::device_reset()
{
	m_ctrl = 0;
	m_cmd = CMD_RIE;

	transmit_register_reset();
	receive_register_reset();

	update_serial();
}


//-------------------------------------------------
//  tra_callback -
//-------------------------------------------------

void mos6551_device::tra_callback()
{
	if (m_write_txd.isnull())
		transmit_register_send_bit();
	else
		m_write_txd(transmit_register_get_data_bit());
}


//-------------------------------------------------
//  tra_complete -
//-------------------------------------------------

void mos6551_device::tra_complete()
{
	if (!(m_st & ST_TDRE))
	{
		transmit_register_setup(m_tdr);
		m_st |= ST_TDRE;

		if ((m_cmd & CMD_TC_MASK) == CMD_TC_TIE_RTS_LO)
		{
			m_st |= ST_IRQ;
			m_write_irq(ASSERT_LINE);
		}
	}
}


//-------------------------------------------------
//  rcv_callback -
//-------------------------------------------------

void mos6551_device::rcv_callback()
{
	if (m_read_rxd.isnull())
		receive_register_update_bit(get_in_data_bit());
	else
		receive_register_update_bit(m_read_rxd());
}


//-------------------------------------------------
//  rcv_complete -
//-------------------------------------------------

void mos6551_device::rcv_complete()
{
	if (m_st & ST_RDRF)
	{
		m_st |= ST_OR;
	}

	m_st &= ~(ST_FE | ST_PE);

	m_st |= ST_RDRF;

	if (!(m_cmd & CMD_RIE))
	{
		m_st |= ST_IRQ;
		m_write_irq(ASSERT_LINE);
	}
}


//-------------------------------------------------
//  input_callback -
//-------------------------------------------------

void mos6551_device::input_callback(UINT8 state)
{
	m_input_state = state;
}


//-------------------------------------------------
//  update_serial -
//-------------------------------------------------

void mos6551_device::update_serial()
{
	int brg = m_ctrl & CTRL_BRG_MASK;

	if (brg == CTRL_BRG_16X_EXTCLK)
	{
		set_rcv_rate(m_ext_rxc / 16);
		set_tra_rate(m_ext_rxc / 16);
	}
	else
	{
		int baud = clock() / brg_divider[brg] / 16;

		set_tra_rate(baud);

		if (m_ctrl & CTRL_RXC_BRG)
		{
			set_rcv_rate(baud);
		}
		else
		{
			set_rcv_rate(m_ext_rxc / 16);
		}

		int num_data_bits = 8;
		int stop_bit_count = 1;
		int parity_code = SERIAL_PARITY_NONE;

		switch (m_ctrl & CTRL_WL_MASK)
		{
		case CTRL_WL_8: num_data_bits = 8; break;
		case CTRL_WL_7: num_data_bits = 7; break;
		case CTRL_WL_6: num_data_bits = 6; break;
		case CTRL_WL_5: num_data_bits = 5; break;
		}

		set_data_frame(num_data_bits, stop_bit_count, parity_code);
	}

	if (m_cmd & CMD_DTR)
		m_connection_state |= SERIAL_STATE_DTR;
	else
		m_connection_state &= ~SERIAL_STATE_DTR;

	m_write_dtr((m_connection_state & SERIAL_STATE_DTR) ? 0 : 1);

	if ((m_cmd & CMD_TC_MASK) == CMD_TC_RTS_HI)
		m_connection_state &= ~SERIAL_STATE_RTS;
	else
		m_connection_state |= SERIAL_STATE_RTS;

	m_write_rts((m_connection_state & SERIAL_STATE_RTS) ? 0 : 1);

	serial_connection_out();
}


//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( mos6551_device::read )
{
	UINT8 data = 0;

	switch (offset & 0x03)
	{
	case 0:
		if (is_receive_register_full())
		{
			receive_register_extract();
			data = get_received_char();
		}

		m_st &= ~(ST_RDRF | ST_OR | ST_FE | ST_PE);
		break;

	case 1:
		data = (m_dsr << 6) | (m_dcd << 5) | m_st;
		m_st &= ~ST_IRQ;
		m_write_irq(CLEAR_LINE);
		break;

	case 2:
		data = m_cmd;
		break;

	case 3:
		data = m_ctrl;
		break;
	}

	return data;
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( mos6551_device::write )
{
	switch (offset & 0x03)
	{
	case 0:
		m_tdr = data;
		m_st &= ~ST_TDRE;

		if (is_transmit_register_empty())
		{
			transmit_register_setup(m_tdr);
			m_st |= ST_TDRE;

			if ((m_cmd & CMD_TC_MASK) == CMD_TC_TIE_RTS_LO)
			{
				m_st |= ST_IRQ;
				m_write_irq(ASSERT_LINE);
			}
		}
		break;

	case 1:
		// programmed reset
		m_cmd = (m_cmd & 0xe0) | CMD_RIE;
		m_st &= ~ST_OR;
		update_serial();
		break;

	case 2:
		m_cmd = data;
		update_serial();
		break;

	case 3:
		m_ctrl = data;
		update_serial();
		break;
	}
}


//-------------------------------------------------
//  set_rxc - set external receiver clock
//-------------------------------------------------

void mos6551_device::set_rxc(int clock)
{
	m_ext_rxc = clock;

	update_serial();
}


//-------------------------------------------------
//  rxd_w - receive data write
//-------------------------------------------------

WRITE_LINE_MEMBER( mos6551_device::rxd_w )
{
	check_for_start(state);
}


//-------------------------------------------------
//  rxc_w - receive clock write
//-------------------------------------------------

WRITE_LINE_MEMBER( mos6551_device::rxc_w )
{
	rcv_clock();
	tra_clock();
}


//-------------------------------------------------
//  cts_w - clear to send write
//-------------------------------------------------

WRITE_LINE_MEMBER( mos6551_device::cts_w )
{
	m_cts = state;
}


//-------------------------------------------------
//  dsr_w - data set ready write
//-------------------------------------------------

WRITE_LINE_MEMBER( mos6551_device::dsr_w )
{
	if (m_dsr != state)
	{
		m_st |= ST_IRQ;
		m_write_irq(ASSERT_LINE);
	}

	m_dsr = state;
}


//-------------------------------------------------
//  dcd_w - data carrier detect write
//-------------------------------------------------

WRITE_LINE_MEMBER( mos6551_device::dcd_w )
{
	if (m_dcd != state)
	{
		m_st |= ST_IRQ;
		m_write_irq(ASSERT_LINE);
	}

	m_dcd = state;
}
