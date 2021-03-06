// license:BSD-3-Clause
// copyright-holders:Curt Coder
/**********************************************************************

    CMD Turbo232 RS-232 cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    http://ar.c64.org/wiki/Turbo232_Programming.txt

*/

#include "turbo232.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define MOS6551_TAG     "mos6551"
#define RS232_TAG       "rs232"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_TURBO232 = &device_creator<c64_turbo232_cartridge_device>;


//-------------------------------------------------
//  rs232_port_interface rs232_intf
//-------------------------------------------------

static const rs232_port_interface rs232_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(MOS6551_TAG, mos6551_device, rxd_w),
	DEVCB_DEVICE_LINE_MEMBER(MOS6551_TAG, mos6551_device, dcd_w),
	DEVCB_DEVICE_LINE_MEMBER(MOS6551_TAG, mos6551_device, dsr_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(MOS6551_TAG, mos6551_device, cts_w)
};


//-------------------------------------------------
//  MACHINE_CONFIG_FRAGMENT( c64_turbo232 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c64_turbo232 )
	MCFG_MOS6551_ADD(MOS6551_TAG, XTAL_3_6864MHz, WRITELINE(c64_turbo232_cartridge_device, acia_irq_w))
	MCFG_MOS6551_RXD_TXD_CALLBACKS(NULL, DEVWRITELINE(RS232_TAG, rs232_port_device, tx))

	MCFG_RS232_PORT_ADD(RS232_TAG, rs232_intf, default_rs232_devices, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_turbo232_cartridge_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c64_turbo232 );
}


//-------------------------------------------------
//  INPUT_PORTS( c64_turbo232 )
//-------------------------------------------------

static INPUT_PORTS_START( c64_turbo232 )
	PORT_START("CS")
	PORT_DIPNAME( 0x03, 0x01, "Base Address" )
	PORT_DIPSETTING(    0x00, "$D700 (C128)" )
	PORT_DIPSETTING(    0x01, "$DE00" )
	PORT_DIPSETTING(    0x02, "$DF00" )

	PORT_START("IRQ")
	PORT_DIPNAME( 0x01, 0x01, "Interrupt" )
	PORT_DIPSETTING(    0x00, "IRQ" )
	PORT_DIPSETTING(    0x01, "NMI" )
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor c64_turbo232_cartridge_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( c64_turbo232 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_turbo232_cartridge_device - constructor
//-------------------------------------------------

c64_turbo232_cartridge_device::c64_turbo232_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_TURBO232, "C64 Turbo232 cartridge", tag, owner, clock, "c64_turbo232", __FILE__),
	device_c64_expansion_card_interface(mconfig, *this),
	m_acia(*this, MOS6551_TAG),
	m_rs232(*this, RS232_TAG),
	m_io_cs(*this, "CS"),
	m_io_irq(*this, "IRQ")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_turbo232_cartridge_device::device_start()
{
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_turbo232_cartridge_device::device_reset()
{
	m_acia->reset();

	m_cs = m_io_cs->read();
	m_irq = m_io_irq->read();

	m_es = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_turbo232_cartridge_device::c64_cd_r(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2)
{
	if (((m_cs == DE00) && !io1) || ((m_cs == DF00) && !io2) ||
		((m_cs == D700) && ((offset & 0xff00) == 0xd700)))
	{
		if (!(offset & 0xe0))
		{
			switch (offset & 0x07)
			{
			case 0: case 1: case 2: case 3:
				data = m_acia->read(space, offset & 0x03);
				break;

			case 7:
				data = m_es;
			}
		}
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_turbo232_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2)
{
	if (((m_cs == DE00) && !io1) || ((m_cs == DF00) && !io2) ||
		((m_cs == D700) && ((offset & 0xff00) == 0xd700)))
	{
		if (!(offset & 0xe0))
		{
			switch (offset & 0x07)
			{
			case 0: case 1: case 2:
				m_acia->write(space, offset & 0x03, data);
				break;

			case 3:
				m_acia->write(space, offset & 0x03, data);

				if (data & 0x0f)
					m_es &= ~ES_M;
				else
					m_es |= ES_M;
				break;

			case 7:
				if (m_es & ES_M)
				{
					data = m_es;

					switch (m_es & ES_S_MASK)
					{
					case ES_S_230400: m_acia->set_rxc(XTAL_3_6864MHz); break;
					case ES_S_115200: m_acia->set_rxc(XTAL_3_6864MHz/2); break;
					case ES_S_57600: m_acia->set_rxc(XTAL_3_6864MHz/4); break;
					case ES_S_UNDEFINED: m_acia->set_rxc(0); break;
					}
				}
			}
		}
	}
}


//-------------------------------------------------
//  acia_irq_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_turbo232_cartridge_device::acia_irq_w )
{
	switch (m_irq)
	{
	case IRQ: m_slot->irq_w(state); break;
	case NMI: m_slot->nmi_w(state); break;
	}
}
