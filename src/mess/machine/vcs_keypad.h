/**********************************************************************

    Atari Video Computer System Keypad emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __VCS_KEYPAD__
#define __VCS_KEYPAD__


#include "emu.h"
#include "machine/vcsctrl.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vcs_keypad_device

class vcs_keypad_device : public device_t,
							public device_vcs_control_port_interface
{
public:
	// construction/destruction
	vcs_keypad_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual ioport_constructor device_input_ports() const;

protected:
	// device-level overrides
	virtual void device_config_complete() { m_shortname = "vcs_keypad"; }
	virtual void device_start();

	// device_vcs_control_port_interface overrides
	virtual UINT8 vcs_joy_r();
	virtual void vcs_joy_w( UINT8 data );
	virtual UINT8 vcs_pot_x_r();
	virtual UINT8 vcs_pot_y_r();

	UINT8	m_column;
};


// device type definition
extern const device_type VCS_KEYPAD;


#endif
