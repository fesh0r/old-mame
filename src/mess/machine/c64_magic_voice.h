/**********************************************************************

    Commodore Magic Voice cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __MAGIC_VOICE__
#define __MAGIC_VOICE__

#include "emu.h"
#include "machine/6525tpi.h"
#include "machine/c64exp.h"
#include "machine/cbmipt.h"
#include "sound/t6721a.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_magic_voice_cartridge_device

class c64_magic_voice_cartridge_device : public device_t,
											public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_magic_voice_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);
	virtual int c64_game_r(offs_t offset, int sphi2, int ba, int rw, int hiram);
	virtual int c64_exrom_r(offs_t offset, int sphi2, int ba, int rw, int hiram);

private:
	required_device<t6721a_device> m_vslsi;
	required_device<tpi6525_device> m_tpi;
	required_device<c64_expansion_slot_device> m_exp;
};


// device type definition
extern const device_type C64_MAGIC_VOICE;



#endif
