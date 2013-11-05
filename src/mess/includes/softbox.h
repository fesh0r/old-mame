// license:BSD-3-Clause
// copyright-holders:Curt Coder, Mike Naberezny
#pragma once

#ifndef __SOFTBOX__
#define __SOFTBOX__

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/harddriv.h"
#include "includes/corvushd.h"
#include "machine/com8116.h"
#include "machine/i8251.h"
#include "machine/i8255.h"
#include "bus/ieee488/ieee488.h"
#include "machine/imi5000h.h"
#include "machine/serial.h"

#define Z80_TAG         "z80"
#define I8251_TAG       "ic15"
#define I8255_0_TAG     "ic17"
#define I8255_1_TAG     "ic16"
#define COM8116_TAG     "ic14"
#define RS232_TAG       "rs232"

class softbox_state : public driver_device
{
public:
	softbox_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, Z80_TAG),
			m_dbrg(*this, COM8116_TAG),
			m_ieee(*this, IEEE488_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<com8116_device> m_dbrg;
	required_device<ieee488_device> m_ieee;

	virtual void machine_start();
	virtual void device_reset_after_children();

	// device_ieee488_interface overrides
	virtual void ieee488_ifc(int state);

	DECLARE_WRITE8_MEMBER( dbrg_w );

	DECLARE_READ8_MEMBER( ppi0_pa_r );
	DECLARE_WRITE8_MEMBER( ppi0_pb_w );

	DECLARE_READ8_MEMBER( ppi1_pa_r );
	DECLARE_WRITE8_MEMBER( ppi1_pb_w );
	DECLARE_READ8_MEMBER( ppi1_pc_r );
	DECLARE_WRITE8_MEMBER( ppi1_pc_w );

	enum
	{
		LED_A,
		LED_B,
		LED_READY
	};

	int m_ifc;  // Tracks previous state of IEEE-488 IFC line
};

#endif
