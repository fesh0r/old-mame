/**********************************************************************

    Seiko-Epson SED1330 LCD Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#pragma once

#ifndef __SED1330__
#define __SED1330__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_SED1330_ADD(_tag, _clock, _screen_tag, _map) \
	MCFG_DEVICE_ADD(_tag, SED1330, _clock) \
	MCFG_DEVICE_ADDRESS_MAP(AS_0, _map) \
	sed1330_device_config::static_set_config(device, _screen_tag);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> sed1330_device_config

class sed1330_device_config :   public device_config,
								public device_config_memory_interface
{
    friend class sed1330_device;

    // construction/destruction
    sed1330_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);

public:
    // allocators
    static device_config *static_alloc_device_config(const machine_config &mconfig, const char *tag, const device_config *owner, UINT32 clock);
    virtual device_t *alloc_device(running_machine &machine) const;

	// inline configuration helpers
	static void static_set_config(device_config *device, const char *screen_tag);

protected:
	// optional information overrides
	virtual const rom_entry *device_rom_region() const;

	// device_config_memory_interface overrides
	virtual const address_space_config *memory_space_config(address_spacenum spacenum = AS_0) const;

    // address space configurations
	const address_space_config		m_space_config;

private:
	const char *m_screen_tag;
};



// ======================> sed1330_device

class sed1330_device :	public device_t,
						public device_memory_interface
{
    friend class sed1330_device_config;

    // construction/destruction
    sed1330_device(running_machine &_machine, const sed1330_device_config &_config);

public:
    DECLARE_READ8_MEMBER( status_r );
    DECLARE_WRITE8_MEMBER( command_w );

    DECLARE_READ8_MEMBER( data_r );
    DECLARE_WRITE8_MEMBER( data_w );

	void update_screen(bitmap_t *bitmap, const rectangle *cliprect);

protected:
    // device-level overrides
    virtual void device_start();
    virtual void device_reset();

	inline UINT8 readbyte(offs_t address);
	inline void writebyte(offs_t address, UINT8 m_data);
	inline void increment_csr();

	void draw_text_scanline(bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 va);
	void draw_graphics_scanline(bitmap_t *bitmap, const rectangle *cliprect, int y, UINT16 va);
	void update_graphics(bitmap_t *bitmap, const rectangle *cliprect);
	void update_text(bitmap_t *bitmap, const rectangle *cliprect);

private:
	int m_bf;					// busy flag

	UINT8 m_ir;					// instruction register
	UINT8 m_dor;				// data output register
	int m_pbc;					// parameter byte counter

	int m_d;					// display enabled
	int m_sleep;				// sleep mode

	UINT16 m_sag;				// character generator RAM start address
	int m_m0;					// character generator ROM (0=internal, 1=external)
	int m_m1;					// character generator RAM D6 correction (0=no, 1=yes)
	int m_m2;					// height of character bitmaps (0=8, 1=16 pixels)
	int m_ws;					// LCD drive method (0=single, 1=dual panel)
	int m_iv;					// screen origin compensation for inverse display (0=yes, 1=no)
	int m_wf;					// AC frame drive waveform period (0=16-line, 1=2-frame)

	int m_fx;					// character width in pixels
	int m_fy;					// character height in pixels
	int m_cr;					// visible line width in characters
	int m_tcr;					// total line width in characters (including horizontal blanking)
	int m_lf;					// frame height in lines
	UINT16 m_ap;				// virtual screen line width in characters

	UINT16 m_sad1;				// display page 1 start address
	UINT16 m_sad2;				// display page 2 start address
	UINT16 m_sad3;				// display page 3 start address
	UINT16 m_sad4;				// display page 4 start address
	int m_sl1;					// display block 1 height in lines
	int m_sl2;					// display block 2 height in lines
	int m_hdotscr;				// horizontal dot scroll in pixels
	int m_fp;					// display page flash control

	UINT16 m_csr;				// cursor address register
	int m_cd;					// cursor increment direction
	int m_crx;					// cursor width
	int m_cry;					// cursor height or location
	int m_cm;					// cursor shape (0=underscore, 1=block)
	int m_fc;					// cursor flash control

	int m_mx;					// screen layer composition method
	int m_dm;					// display mode for pages 1, 3
	int m_ov;					// graphics mode layer composition

	// devices
	screen_device *m_screen;

	const sed1330_device_config &m_config;
};


// device type definition
extern const device_type SED1330;



#endif
