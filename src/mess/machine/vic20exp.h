/**********************************************************************

    Commodore VIC-20 Expansion Port emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************

                    GND       1      A       GND
                    CD0       2      B       CA0
                    CD1       3      C       CA1
                    CD2       4      D       CA2
                    CD3       5      E       CA3
                    CD4       6      F       CA4
                    CD5       7      H       CA5
                    CD6       8      J       CA6
                    CD7       9      K       CA7
                  _BLK1      10      L       CA8
                  _BLK2      11      M       CA9
                  _BLK3      12      N       CA10
                  _BLK5      13      P       CA11
                  _RAM1      14      R       CA12
                  _RAM2      15      S       CA13
                  _RAM3      16      T       _I/O2
                   VR/W      17      U       _I/O3
                   CR/W      18      V       Sphi2
                   _IRQ      19      W       _NMI
                   N.C.      20      X       _RES
                    +5V      21      Y       N.C.
                    GND      22      Z       GND

**********************************************************************/

#pragma once

#ifndef __VIC20_EXPANSION_SLOT__
#define __VIC20_EXPANSION_SLOT__

#include "emu.h"



//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define VIC20_EXPANSION_SLOT_TAG        "exp"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_VIC20_EXPANSION_SLOT_ADD(_tag, _clock, _slot_intf, _def_slot) \
	MCFG_DEVICE_ADD(_tag, VIC20_EXPANSION_SLOT, _clock) \
	MCFG_DEVICE_SLOT_INTERFACE(_slot_intf, _def_slot, false)

#define MCFG_VIC20_PASSTHRU_EXPANSION_SLOT_ADD(_tag) \
	MCFG_VIC20_EXPANSION_SLOT_ADD(_tag, 0, vic20_expansion_cards, NULL) \
	MCFG_VIC20_EXPANSION_SLOT_IRQ_CALLBACKS(DEVWRITELINE(DEVICE_SELF_OWNER, vic20_expansion_slot_device, irq_w), DEVWRITELINE(DEVICE_SELF_OWNER, vic20_expansion_slot_device, nmi_w), DEVWRITELINE(DEVICE_SELF_OWNER, vic20_expansion_slot_device, res_w))


#define MCFG_VIC20_EXPANSION_SLOT_IRQ_CALLBACKS(_irq, _nmi, _res) \
	downcast<vic20_expansion_slot_device *>(device)->set_irq_callbacks(DEVCB2_##_irq, DEVCB2_##_nmi, DEVCB2_##_res);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> vic20_expansion_slot_device

class device_vic20_expansion_card_interface;

class vic20_expansion_slot_device : public device_t,
									public device_slot_interface,
									public device_image_interface
{
public:
	// construction/destruction
	vic20_expansion_slot_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _irq, class _nmi, class _res> void set_irq_callbacks(_irq irq, _nmi nmi, _res res) {
		m_write_irq.set_callback(irq);
		m_write_nmi.set_callback(nmi);
		m_write_res.set_callback(res);
	}

	// computer interface
	UINT8 cd_r(address_space &space, offs_t offset, UINT8 data, int ram1, int ram2, int ram3, int blk1, int blk2, int blk3, int blk5, int io2, int io3);
	void cd_w(address_space &space, offs_t offset, UINT8 data, int ram1, int ram2, int ram3, int blk1, int blk2, int blk3, int blk5, int io2, int io3);

	// cartridge interface
	DECLARE_WRITE_LINE_MEMBER( irq_w ) { m_write_irq(state); }
	DECLARE_WRITE_LINE_MEMBER( nmi_w ) { m_write_nmi(state); }
	DECLARE_WRITE_LINE_MEMBER( res_w ) { m_write_res(state); }

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
	virtual const char *image_interface() const { return "vic1001_cart"; }
	virtual const char *file_extensions() const { return "20,40,60,70,a0,b0,crt"; }
	virtual const option_guide *create_option_guide() const { return NULL; }

	// slot interface overrides
	virtual const char * get_default_card_software(const machine_config &config, emu_options &options);

	devcb2_write_line   m_write_irq;
	devcb2_write_line   m_write_nmi;
	devcb2_write_line   m_write_res;

	device_vic20_expansion_card_interface *m_card;
};


// ======================> device_vic20_expansion_card_interface

// class representing interface-specific live vic20_expansion card
class device_vic20_expansion_card_interface : public device_slot_card_interface
{
	friend class vic20_expansion_slot_device;

public:
	// construction/destruction
	device_vic20_expansion_card_interface(const machine_config &mconfig, device_t &device);
	virtual ~device_vic20_expansion_card_interface();

protected:
	// initialization
	virtual UINT8* vic20_blk1_pointer(running_machine &machine, size_t size);
	virtual UINT8* vic20_blk2_pointer(running_machine &machine, size_t size);
	virtual UINT8* vic20_blk3_pointer(running_machine &machine, size_t size);
	virtual UINT8* vic20_blk5_pointer(running_machine &machine, size_t size);
	virtual UINT8* vic20_ram_pointer(running_machine &machine, size_t size);
	virtual UINT8* vic20_nvram_pointer(running_machine &machine, size_t size);

	// runtime
	virtual UINT8 vic20_cd_r(address_space &space, offs_t offset, UINT8 data, int ram1, int ram2, int ram3, int blk1, int blk2, int blk3, int blk5, int io2, int io3) { return data; };
	virtual void vic20_cd_w(address_space &space, offs_t offset, UINT8 data, int ram1, int ram2, int ram3, int blk1, int blk2, int blk3, int blk5, int io2, int io3) { };

	vic20_expansion_slot_device *m_slot;

	UINT8 *m_blk1;
	UINT8 *m_blk2;
	UINT8 *m_blk3;
	UINT8 *m_blk5;
	UINT8 *m_ram;
	UINT8 *m_nvram;

	size_t m_nvram_size;

	size_t m_nvram_mask;
};


// device type definition
extern const device_type VIC20_EXPANSION_SLOT;



#endif
