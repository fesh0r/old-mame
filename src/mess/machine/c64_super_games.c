/**********************************************************************

    Commodore Super Games cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c64_super_games.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C64_SUPER_GAMES = &device_creator<c64_super_games_cartridge_device>;



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c64_super_games_cartridge_device - constructor
//-------------------------------------------------

c64_super_games_cartridge_device::c64_super_games_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, C64_SUPER_GAMES, "C64 Super Games cartridge", tag, owner, clock),
	device_c64_expansion_card_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c64_super_games_cartridge_device::device_start()
{
	// state saving
	save_item(NAME(m_bank));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void c64_super_games_cartridge_device::device_reset()
{
	m_bank = 0;
}


//-------------------------------------------------
//  c64_cd_r - cartridge data read
//-------------------------------------------------

UINT8 c64_super_games_cartridge_device::c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2)
{
	UINT8 data = 0;

	if (!roml || !romh)
	{
		offs_t addr = (m_bank << 14) | (offset & 0x3fff);
		data = m_roml[addr];
	}

	return data;
}


//-------------------------------------------------
//  c64_cd_w - cartridge data write
//-------------------------------------------------

void c64_super_games_cartridge_device::c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2)
{
	if (!io2)
	{
		m_bank = data & 0x03;

		m_game = BIT(data, 2);
		m_exrom = BIT(data, 3);
	}
}
