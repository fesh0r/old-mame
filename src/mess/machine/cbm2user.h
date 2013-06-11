/**********************************************************************

    Commodore CBM-II User Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

**********************************************************************/

#pragma once

#ifndef __CBM2_USER_PORT__
#define __CBM2_USER_PORT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define CBM2_USER_PORT_TAG       "user"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define CBM2_USER_PORT_INTERFACE(_name) \
	const cbm2_user_port_interface (_name) =


#define MCFG_CBM2_USER_PORT_ADD(_tag, _config, _slot_intf, _def_slot) \
	MCFG_DEVICE_ADD(_tag, CBM2_USER_PORT, 0) \
	MCFG_DEVICE_CONFIG(_config) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, false)



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> cbm2_user_port_interface

struct cbm2_user_port_interface
{
	devcb_write_line    m_out_irq_cb;
	devcb_write_line    m_out_sp_cb;
	devcb_write_line    m_out_cnt_cb;
	devcb_write_line    m_out_flag_cb;
};


// ======================> cbm2_user_port_device

class device_cbm2_user_port_interface;

class cbm2_user_port_device : public device_t,
								public cbm2_user_port_interface,
								public device_slot_interface
{
public:
	// construction/destruction
	cbm2_user_port_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);
	virtual ~cbm2_user_port_device();

	// computer interface
	DECLARE_READ8_MEMBER( d1_r );
	DECLARE_WRITE8_MEMBER( d1_w );
	DECLARE_READ8_MEMBER( d2_r );
	DECLARE_WRITE8_MEMBER( d2_w );
	DECLARE_READ_LINE_MEMBER( pb2_r );
	DECLARE_WRITE_LINE_MEMBER( pb2_w );
	DECLARE_READ_LINE_MEMBER( pb3_r );
	DECLARE_WRITE_LINE_MEMBER( pb3_w );
	DECLARE_WRITE_LINE_MEMBER( pc_w );
	DECLARE_WRITE_LINE_MEMBER( cnt_w );
	DECLARE_WRITE_LINE_MEMBER( sp_w );

	// cartridge interface
	DECLARE_WRITE_LINE_MEMBER( irq_w ) { m_out_irq_func(state); }
	DECLARE_WRITE_LINE_MEMBER( cia_sp_w ) { m_out_sp_func(state); }
	DECLARE_WRITE_LINE_MEMBER( cia_cnt_w ) { m_out_cnt_func(state); }
	DECLARE_WRITE_LINE_MEMBER( flag_w ) { m_out_flag_func(state); }

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();

	devcb_resolved_write_line   m_out_irq_func;
	devcb_resolved_write_line   m_out_sp_func;
	devcb_resolved_write_line   m_out_cnt_func;
	devcb_resolved_write_line   m_out_flag_func;

	device_cbm2_user_port_interface *m_card;
};


// ======================> device_cbm2_user_port_interface

// class representing interface-specific live cbm2_expansion card
class device_cbm2_user_port_interface : public device_slot_card_interface
{
public:
	// construction/destruction
	device_cbm2_user_port_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_cbm2_user_port_interface();

	virtual UINT8 cbm2_d1_r(address_space &space, offs_t offset) { return 0xff; };
	virtual void cbm2_d1_w(address_space &space, offs_t offset, UINT8 data) { };

	virtual UINT8 cbm2_d2_r(address_space &space, offs_t offset) { return 0xff; };
	virtual void cbm2_d2_w(address_space &space, offs_t offset, UINT8 data) { };

	virtual int cbm2_pb2_r() { return 1; }
	virtual void cbm2_pb2_w(int state) { };
	virtual int cbm2_pb3_r() { return 1; }
	virtual void cbm2_pb3_w(int state) { };

	virtual void cbm2_pc_w(int state) { };
	virtual void cbm2_cnt_w(int state) { };
	virtual void cbm2_sp_w(int state) { };

protected:
	cbm2_user_port_device *m_slot;
};


// device type definition
extern const device_type CBM2_USER_PORT;



#endif
