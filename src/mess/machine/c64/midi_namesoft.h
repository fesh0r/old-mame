/**********************************************************************

    Namesoft MIDI Interface cartridge emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __C64_MIDI_NAMESOFT__
#define __C64_MIDI_NAMESOFT__

#include "emu.h"
#include "machine/c64/exp.h"
#include "machine/6850acia.h"
#include "machine/serial.h"
#include "machine/midiinport.h"
#include "machine/midioutport.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> c64_namesoft_midi_cartridge_device

class c64_namesoft_midi_cartridge_device : public device_t,
												public device_c64_expansion_card_interface
{
public:
	// construction/destruction
	c64_namesoft_midi_cartridge_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;

	DECLARE_WRITE_LINE_MEMBER( acia_irq_w );
	DECLARE_WRITE_LINE_MEMBER( midi_rx_w );
	DECLARE_READ_LINE_MEMBER( rx_in );
	DECLARE_WRITE_LINE_MEMBER( tx_out );

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();

	// device_c64_expansion_card_interface overrides
	virtual UINT8 c64_cd_r(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);
	virtual void c64_cd_w(address_space &space, offs_t offset, UINT8 data, int sphi2, int ba, int roml, int romh, int io1, int io2);

private:
	required_device<acia6850_device> m_acia;
	required_device<serial_port_device> m_mdout;

	int m_rx_state;
};


// device type definition
extern const device_type C64_MIDI_NAMESOFT;


#endif
