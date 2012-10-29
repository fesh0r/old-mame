/**********************************************************************

    Simons' BASIC cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __SIMONS_BASIC__
#define __SIMONS_BASIC__


#include "emu.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_simons_basic_cartridge_device

class c64_simons_basic_cartridge_device : public device_t,
										  public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_simons_basic_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	// device-level overrides
	virtual void device_config_complete() { m_shortname = "c64_simons_basic"; }
	virtual void device_start();
	virtual void device_reset();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);
};


// device type definition
extern const device_type C64_SIMONS_BASIC;


#endif