#pragma once

#ifndef __SOFTBOX__
#define __SOFTBOX__

#include "emu.h"
#include "cpu/z80/z80.h"
#include "imagedev/harddriv.h"
#include "includes/corvushd.h"
#include "machine/cbmipt.h"
#include "machine/com8116.h"
#include "machine/i8251.h"
#include "machine/i8255.h"
#include "machine/ieee488.h"
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
			m_dbrg(*this, COM8116_TAG),
			m_ieee(*this, IEEE488_TAG)
	{ }

	required_device<com8116_device> m_dbrg;
	required_device<ieee488_device> m_ieee;

	virtual void machine_start();

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
};

#endif
