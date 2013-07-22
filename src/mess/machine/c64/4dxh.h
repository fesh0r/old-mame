/**********************************************************************

    The Digital Excess & Hitmen 4-Player Joystick adapter emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_4DXH__
#define __C64_4DXH__


#include "emu.h"
#include "machine/c64/user.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_4dxh_device

class c64_4dxh_device : public device_t,
						public device_c64_user_port_interface
{
public:
	// construction/destruction
	c64_4dxh_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual ioport_constructor device_input_ports() const;

protected:
	// device-level overrides
	virtual void device_start();

	// device_c64_user_port_interface overrides
	virtual UINT8 c64_pb_r(address_space &space, offs_t offset);
	virtual int c64_pa2_r();
	virtual void c64_cnt1_w(int level);

private:
	required_ioport m_pb;
	required_ioport m_pa2;
};


// device type definition
extern const device_type C64_4DXH;


#endif
