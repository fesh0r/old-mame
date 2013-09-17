/**********************************************************************

    Mator Systems SHARK Intelligent Winchester Disc Subsystem emulation

    35MB PRIAM DISKOS 3450 8" Winchester Hard Disk (-chs 525,5,? -ss ?)

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __SHARK__
#define __SHARK__

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "imagedev/harddriv.h"
#include "machine/cbmipt.h"
#include "machine/ieee488.h"
#include "machine/serial.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> shark_device

class shark_device :  public device_t,
						public device_ieee488_interface
{
public:
	// construction/destruction
	shark_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

protected:
	// device-level overrides
	virtual void device_start();

private:
	required_device<cpu_device> m_maincpu;
};


// device type definition
extern const device_type SHARK;



#endif
