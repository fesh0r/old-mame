/**********************************************************************

    Epyx Fast Load cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __EPYX_FAST_LOAD__
#define __EPYX_FAST_LOAD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "machine/c64exp.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_epyx_fast_load_cartridge_device

class c64_epyx_fast_load_cartridge_device : public device_t,
										    public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_epyx_fast_load_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(address_space &space, offs_t offset, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int roml, int romh, int io1, int io2);

private:
	emu_timer *m_exrom_timer;
};


// device type definition
extern const device_type C64_EPYX_FAST_LOAD;


#endif
