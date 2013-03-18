#ifndef __SNS_ROM_H
#define __SNS_ROM_H

#include "machine/sns_slot.h"


// ======================> sns_rom_device

class sns_rom_device : public device_t,
						public device_sns_cart_interface
{
public:
	// construction/destruction
	sns_rom_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	sns_rom_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "sns_rom"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_l);
	virtual DECLARE_READ8_MEMBER(read_h);
};

// ======================> sns_rom_obc1_device

class sns_rom_obc1_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_obc1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_obc1"; }

	// additional reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);
	virtual DECLARE_WRITE8_MEMBER(chip_write);

	int m_address;
	int m_offset;
	int m_shift;
	UINT8 m_ram[0x2000];
};



// ======================> sns_rom_pokemon_device

class sns_rom_pokemon_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_pokemon_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_pokemon"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);    // protection device
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // protection device
	UINT8 m_latch;
};

// ======================> sns_rom_tekken2_device

class sns_rom_tekken2_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_tekken2_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_tekken2"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);    // protection device
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // protection device

	void update_prot(UINT32 offset);

	// bit0-3 prot value, bit4 direction, bit5 function
	// reads must return (prot value) +1/-1/<<1/>>1 depending on bit4 & bit5
	UINT8 m_prot;
};

// ======================> sns_rom_soulblad_device

class sns_rom_soulblad_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_soulblad_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "sns_rom_soulblad"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);    // protection device
};

// ======================> sns_rom_mcpirate1_device

class sns_rom_mcpirate1_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_mcpirate1_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_mcpirate1"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_l);
	virtual DECLARE_READ8_MEMBER(read_h);
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // bankswitch device
	UINT8 m_base_bank;
};

// ======================> sns_rom_mcpirate2_device

class sns_rom_mcpirate2_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_mcpirate2_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_mcpirate2"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_l);
	virtual DECLARE_READ8_MEMBER(read_h);
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // bankswitch device
	UINT8 m_base_bank;
};

// ======================> sns_rom_20col_device

class sns_rom_20col_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_20col_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "sns_rom_20col"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(read_l);
	virtual DECLARE_READ8_MEMBER(read_h);
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // bankswitch device
	UINT8 m_base_bank;
};

// ======================> sns_rom_banana_device

class sns_rom_banana_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_banana_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
//  virtual void device_start();
//  virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_banana"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);    // protection device
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // protection device
	UINT8 m_latch[16];
};

// ======================> sns_rom_bugs_device

class sns_rom_bugs_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_bugs_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_config_complete() { m_shortname = "sns_rom_bugslife"; }

	// reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);    // protection device
	virtual DECLARE_WRITE8_MEMBER(chip_write);  // protection device
	UINT8 m_latch[0x800];
};


// device type definition
extern const device_type SNS_LOROM;
extern const device_type SNS_LOROM_OBC1;
extern const device_type SNS_LOROM_POKEMON;
extern const device_type SNS_LOROM_TEKKEN2;
extern const device_type SNS_LOROM_SOULBLAD;
extern const device_type SNS_LOROM_MCPIR1;
extern const device_type SNS_LOROM_MCPIR2;
extern const device_type SNS_LOROM_20COL;
extern const device_type SNS_LOROM_BANANA;
extern const device_type SNS_LOROM_BUGSLIFE;

#endif
