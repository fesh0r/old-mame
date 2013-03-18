/**********************************************************************

    Commodore Plus/4 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

                    GND       1      A       GND
                    +5V       2      B       C1 LOW
                    +5V       3      C       _BRESET
                   _IRQ       4      D       _RAS
                   R/_W       5      E       phi0
                C1 HIGH       6      F       A15
                 C2 LOW       7      H       A14
                C2 HIGH       8      J       A13
                   _CS1       9      K       A12
                   _CS0      10      L       A11
                   _CAS      11      M       A10
                    MUX      12      N       A9
                     BA      13      P       A8
                     D7      14      R       A7
                     D6      15      S       A6
                     D5      16      T       A5
                     D4      17      U       A4
                     D3      18      V       A3
                     D2      19      W       A2
                     D1      20      X       A1
                     D0      21      Y       A0
                    AEC      22      Z       N.C. (RAMEN)
              EXT AUDIO      23      AA      N.C.
                   phi2      24      BB      N.C.
                    GND      25      CC      GND

**********************************************************************/

#pragma once

#ifndef __PLUS4_EXPANSION_SLOT__
#define __PLUS4_EXPANSION_SLOT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define PLUS4_EXPANSION_SLOT_TAG        "exp"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_PLUS4_EXPANSION_SLOT_ADD(_tag, _clock, _slot_intf, _def_slot, _def_inp, _irq) \
	MCFG_DEVICE_ADD(_tag, PLUS4_EXPANSION_SLOT, _clock) \
	downcast<plus4_expansion_slot_device *>(device)->set_irq_callback(DEVCB2_##_irq); \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, _def_inp, false)

#define MCFG_PLUS4_PASSTHRU_EXPANSION_SLOT_ADD() \
	MCFG_PLUS4_EXPANSION_SLOT_ADD(PLUS4_EXPANSION_SLOT_TAG, 0, plus4_expansion_cards, NULL, NULL, DEVWRITELINE(DEVICE_SELF_OWNER, plus4_expansion_slot_device, irq_w)) \
	MCFG_PLUS4_EXPANSION_SLOT_DMA_CALLBACKS(DEVREAD8(DEVICE_SELF_OWNER, plus4_expansion_slot_device, dma_cd_r), DEVWRITE8(DEVICE_SELF_OWNER, plus4_expansion_slot_device, dma_cd_w), DEVWRITELINE(DEVICE_SELF_OWNER, plus4_expansion_slot_device, aec_w))


#define MCFG_PLUS4_EXPANSION_SLOT_DMA_CALLBACKS(_read, _write, _aec) \
	downcast<plus4_expansion_slot_device *>(device)->set_dma_callbacks(DEVCB2_##_read, DEVCB2_##_write, DEVCB2_##_aec);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> plus4_expansion_slot_device

class device_plus4_expansion_card_interface;

class plus4_expansion_slot_device : public device_t,
									public device_slot_interface,
									public device_image_interface
{
public:
	// construction/destruction
	plus4_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _irq> void set_irq_callback(_irq irq) { m_write_irq.set_callback(irq); }

	template<class _read, class _write, class _aec> void set_dma_callbacks(_read read, _write write, _aec aec) {
		m_read_dma_cd.set_callback(read);
		m_write_dma_cd.set_callback(write);
		m_write_aec.set_callback(aec);
	}

	// computer interface
	UINT8 cd_r(address_space &space, offs_t offset, UINT8 data, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h);
	void cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h);

	// cartridge interface
	DECLARE_READ8_MEMBER( dma_cd_r ) { return m_read_dma_cd(offset); }
	DECLARE_WRITE8_MEMBER( dma_cd_w ) { m_write_dma_cd(offset, data); }
	DECLARE_WRITE_LINE_MEMBER( irq_w ) { m_write_irq(state); }
	DECLARE_WRITE_LINE_MEMBER( aec_w ) { m_write_aec(state); }
	int phi2() { return clock(); }

protected:
	// device-level overrides
	virtual void device_config_complete() { update_names(); }
	virtual void device_start();
	virtual void device_reset();

	// image-level overrides
	virtual bool call_load();
	virtual bool call_softlist_load(char *swlist, char *swname, rom_entry *start_entry);

	virtual iodevice_t image_type() const { return IO_CARTSLOT; }

	virtual bool is_readable()  const { return 1; }
	virtual bool is_writeable() const { return 0; }
	virtual bool is_creatable() const { return 0; }
	virtual bool must_be_loaded() const { return 0; }
	virtual bool is_reset_on_load() const { return 1; }
	virtual const char *image_interface() const { return "plus4_cart"; }
	virtual const char *file_extensions() const { return "rom,bin"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	// slot interface overrides
	virtual const char * get_default_card_software(const machine_config &config, emu_options &options);

	devcb2_write_line   m_write_irq;
	devcb2_read8        m_read_dma_cd;
	devcb2_write8       m_write_dma_cd;
	devcb2_write_line   m_write_aec;

	device_plus4_expansion_card_interface *m_card;
};


// ======================> device_plus4_expansion_card_interface

class device_plus4_expansion_card_interface : public device_slot_card_interface
{
	friend class plus4_expansion_slot_device;

public:
	// construction/destruction
	device_plus4_expansion_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_plus4_expansion_card_interface();

	// initialization
	virtual UINT8* plus4_c1l_pointer(running_machine &machine, size_t size);
	virtual UINT8* plus4_c1h_pointer(running_machine &machine, size_t size);
	virtual UINT8* plus4_c2l_pointer(running_machine &machine, size_t size);
	virtual UINT8* plus4_c2h_pointer(running_machine &machine, size_t size);
	virtual UINT8* plus4_ram_pointer(running_machine &machine, size_t size);
	virtual UINT8* plus4_nvram_pointer(running_machine &machine, size_t size);

	// runtime
	virtual UINT8 plus4_cd_r(address_space &space, offs_t offset, UINT8 data, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h) { return data; };
	virtual void plus4_cd_w(address_space &space, offs_t offset, UINT8 data, int ba, int cs0, int c1l, int c2l, int cs1, int c1h, int c2h) { };

protected:
	plus4_expansion_slot_device *m_slot;

	UINT8 *m_c1l;
	UINT8 *m_c1h;
	UINT8 *m_c2l;
	UINT8 *m_c2h;
	UINT8 *m_ram;
	UINT8 *m_nvram;

	size_t m_nvram_size;

	size_t m_c1l_mask;
	size_t m_c1h_mask;
	size_t m_c2l_mask;
	size_t m_c2h_mask;
	size_t m_ram_mask;
	size_t m_nvram_mask;
};


// device type definition
extern const device_type PLUS4_EXPANSION_SLOT;



#endif
