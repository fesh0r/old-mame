/**********************************************************************

    Commodore VIC-1011A/B RS-232C Adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_vic1011.h"



//**************************************************************************
//  MACROS/CONSTANTS
//**************************************************************************

#define RS232_TAG       "rs232"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_VIC1011 = &device_creator<c64_vic1011_device>;


//-------------------------------------------------
//  rs232_port_interface rs232_intf
//-------------------------------------------------

static const rs232_port_interface rs232_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(DEVICE_SELF_OWNER, c64_vic1011_device, rxd_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  MACHINE_DRIVER( vic1011 )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( vic1011 )
	MCFG_RS232_PORT_ADD(RS232_TAG, rs232_intf, default_rs232_devices, NULL)
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c64_vic1011_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( vic1011 );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_vic1011_device - constructor
//-------------------------------------------------

c64_vic1011_device::c64_vic1011_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, C64_VIC1011, "C64 VIC1011", tag, owner, clock, "c64_vic1011", __FILE__),
		device_c64_user_port_interface(mconfig, *this),
		m_rs232(*this, RS232_TAG)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_vic1011_device::device_start()
{
}


//-------------------------------------------------
//  c64_pb_r - port B read
//-------------------------------------------------

UINT8 c64_vic1011_device::c64_pb_r(address_space &space, offs_t offset)
{
	/*

	    bit     description

	    0       Sin
	    1
	    2
	    3
	    4       DCDin
	    5
	    6       CTS
	    7       DSR

	*/

	UINT8 data = 0;

	data |= !m_rs232->rx();
	data |= m_rs232->dcd_r() << 4;
	data |= m_rs232->cts_r() << 6;
	data |= m_rs232->dsr_r() << 7;

	return data;
}


//-------------------------------------------------
//  c64_pb_w - port B write
//-------------------------------------------------

void c64_vic1011_device::c64_pb_w(address_space &space, offs_t offset, UINT8 data)
{
	/*

	    bit     description

	    0
	    1       RTS
	    2       DTR
	    3
	    4
	    5       DCDout
	    6
	    7

	*/

	m_rs232->rts_w(BIT(data, 1));
	m_rs232->dtr_w(BIT(data, 2));
}


//-------------------------------------------------
//  c64_pa2_w - PA2 write
//-------------------------------------------------

void c64_vic1011_device::c64_pa2_w(int state)
{
	m_rs232->tx(!state);
}


//-------------------------------------------------
//  rxd_w -
//-------------------------------------------------

WRITE_LINE_MEMBER( c64_vic1011_device::rxd_w )
{
	m_slot->cia_flag2_w(!state);
}
