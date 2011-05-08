#pragma once

#ifndef __INTERPOD__
#define __INTERPOD__

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/6850acia.h"
#include "machine/cbmiec.h"
#include "machine/ieee488.h"



//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define INTERPOD_TAG			"interpod"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_INTERPOD_ADD(_daisy) \
    MCFG_DEVICE_ADD(INTERPOD_TAG, INTERPOD, 0) \
	MCFG_IEEE488_CONFIG_ADD(_daisy, interpod_ieee488_intf)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> interpod_device

class interpod_device :  public device_t,
					     public device_cbm_iec_interface
{
public:
    // construction/destruction
    interpod_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;

protected:
    // device-level overrides
    virtual void device_start();
	virtual void device_reset();
    virtual void device_config_complete();

	required_device<cpu_device> m_maincpu;
	required_device<via6522_device> m_via;
	required_device<riot6532_device> m_riot;
	required_device<acia6850_device> m_acia;
	required_device<cbm_iec_device> m_iec;
	required_device<ieee488_device> m_ieee;
};


// device type definition
extern const device_type INTERPOD;


// IEEE-488 interface
extern const ieee488_stub_interface interpod_ieee488_intf;



#endif
