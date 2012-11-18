/**********************************************************************

    Coleco Adam floppy disk controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __ADAM_FDC__
#define __ADAM_FDC__

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "formats/adam_dsk.h"
#include "formats/hxcmfm_dsk.h"
#include "formats/mfi_dsk.h"
#include "imagedev/floppy.h"
#include "machine/adamnet.h"
#include "machine/wd1772.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> adam_fdc_device

class adam_fdc_device :  public device_t,
							public device_adamnet_card_interface
{
public:
	// construction/destruction
	adam_fdc_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual const rom_entry *device_rom_region() const;
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual ioport_constructor device_input_ports() const;

	// not really public
	DECLARE_READ8_MEMBER( p1_r );
	DECLARE_WRITE8_MEMBER( p1_w );
	DECLARE_READ8_MEMBER( p2_r );
	DECLARE_WRITE8_MEMBER( p2_w );

	void fdc_intrq_w(bool state);

	static const floppy_format_type floppy_formats[];

protected:
	// device-level overrides
	virtual void device_config_complete() { m_shortname = "adam_fdc"; }
	virtual void device_start();

	// device_adamnet_card_interface overrides
	virtual void adamnet_reset_w(int state);

	required_device<cpu_device> m_maincpu;
	required_device<wd2793_t> m_fdc;
	required_device<floppy_connector> m_floppy0;
	required_shared_ptr<UINT8> m_ram;

	floppy_image_device *m_image0;
};


// device type definition
extern const device_type ADAM_FDC;



#endif
