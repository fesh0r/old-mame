/***************************************************************************

  ISA 16 bit IDE controller

***************************************************************************/

#include "emu.h"
#include "isa_ide.h"
#include "machine/idectrl.h"
#include "imagedev/harddriv.h"

READ16_MEMBER(isa16_ide_device::ide16_r)
{
	return ide_controller16_r(m_ide, space, 0x1f0/2 + offset, mem_mask);
}

WRITE16_MEMBER(isa16_ide_device::ide16_w)
{
	ide_controller16_w(m_ide, space, 0x1f0/2 + offset, data, mem_mask);
}

READ8_MEMBER(isa16_ide_device::ide16_alt_r )
{
	return ide_controller16_r(m_ide, space, 0x3f6/2, 0x00ff);
}

WRITE8_MEMBER(isa16_ide_device::ide16_alt_w )
{
	ide_controller16_w(m_ide, space, 0x3f6/2, data, 0x00ff);
}

DEVICE_ADDRESS_MAP_START(map, 16, isa16_ide_device)
	AM_RANGE(0x0, 0x7) AM_READWRITE(ide16_r, ide16_w)
ADDRESS_MAP_END

DEVICE_ADDRESS_MAP_START(alt_map, 8, isa16_ide_device)
	AM_RANGE(0x6, 0x6) AM_READWRITE(ide16_alt_r, ide16_alt_w)
ADDRESS_MAP_END

WRITE_LINE_MEMBER(isa16_ide_device::ide_interrupt)
{
	if (is_primary())
	{
		m_isa->irq14_w(state);
	}
	else
	{
		m_isa->irq15_w(state);
	}
}

static MACHINE_CONFIG_FRAGMENT( ide )
	MCFG_IDE_CONTROLLER_ADD("ide", ide_image_devices, "hdd", "hdd", false)
	MCFG_IDE_CONTROLLER_IRQ_HANDLER(DEVWRITELINE(DEVICE_SELF, isa16_ide_device, ide_interrupt))
MACHINE_CONFIG_END

static INPUT_PORTS_START( ide )
	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x00, "IDE Configuration")
	PORT_DIPSETTING(    0x00, "Primary" )
	PORT_DIPSETTING(    0x01, "Secondary" )
INPUT_PORTS_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type ISA16_IDE = &device_creator<isa16_ide_device>;

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor isa16_ide_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ide );
}

//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor isa16_ide_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( ide );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  isa16_ide_device - constructor
//-------------------------------------------------

isa16_ide_device::isa16_ide_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
		: device_t(mconfig, ISA16_IDE, "IDE Fixed Drive Adapter", tag, owner, clock),
		device_isa16_card_interface( mconfig, *this ),
		m_is_primary(true),
		m_ide(*this, "ide")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void isa16_ide_device::device_start()
{
	set_isa_device();
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void isa16_ide_device::device_reset()
{
	m_is_primary = (ioport("DSW")->read() & 1) ? false : true;
	if (m_is_primary) {
		m_isa->install_device(0x01f0, 0x01f7, *this, &isa16_ide_device::map, 16);
		m_isa->install_device(0x03f0, 0x03f7, *this, &isa16_ide_device::alt_map);
	} else {
		m_isa->install_device(0x0170, 0x0177, *this, &isa16_ide_device::map, 16);
		m_isa->install_device(0x0370, 0x0377, *this, &isa16_ide_device::alt_map);
	}
}
