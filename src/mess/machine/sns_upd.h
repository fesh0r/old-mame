#ifndef __SNS_UPD_H
#define __SNS_UPD_H

#include "machine/sns_slot.h"
#include "machine/sns_rom.h"
#include "machine/sns_rom21.h"
#include "cpu/upd7725/upd7725.h"

// ======================> sns_rom_necdsp_device

class sns_rom20_necdsp_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom20_necdsp_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	sns_rom20_necdsp_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "sns_rom_necdsp"; }
	virtual machine_config_constructor device_mconfig_additions() const;

	required_device<upd7725_device> m_upd7725;

	// additional reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);
	virtual DECLARE_WRITE8_MEMBER(chip_write);

	virtual DECLARE_READ32_MEMBER(necdsp_prg_r);
	virtual DECLARE_READ16_MEMBER(necdsp_data_r);
};

// ======================> sns_rom21_necdsp_device

class sns_rom21_necdsp_device : public sns_rom21_device
{
public:
	// construction/destruction
	sns_rom21_necdsp_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);
	sns_rom21_necdsp_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "sns_rom21_necdsp"; }
	virtual machine_config_constructor device_mconfig_additions() const;

	required_device<upd7725_device> m_upd7725;

	// additional reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);
	virtual DECLARE_WRITE8_MEMBER(chip_write);

	virtual DECLARE_READ32_MEMBER(necdsp_prg_r);
	virtual DECLARE_READ16_MEMBER(necdsp_data_r);
};

// ======================> sns_rom_setadsp_device

class sns_rom_setadsp_device : public sns_rom_device
{
public:
	// construction/destruction
	sns_rom_setadsp_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_start();
	virtual void device_config_complete() { m_shortname = "sns_rom_setadsp"; }

	required_device<upd96050_device> m_upd96050;

	// additional reading and writing
	virtual DECLARE_READ8_MEMBER(chip_read);
	virtual DECLARE_WRITE8_MEMBER(chip_write);

	virtual DECLARE_READ32_MEMBER(setadsp_prg_r);
	virtual DECLARE_READ16_MEMBER(setadsp_data_r);
};

// ======================> sns_rom_seta10dsp_device

class sns_rom_seta10dsp_device : public sns_rom_setadsp_device
{
public:
	// construction/destruction
	sns_rom_seta10dsp_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "sns_rom_seta10"; }
	virtual machine_config_constructor device_mconfig_additions() const;
};

// ======================> sns_rom_seta11dsp_device [Faster CPU than ST010]

class sns_rom_seta11dsp_device : public sns_rom_setadsp_device
{
public:
	// construction/destruction
	sns_rom_seta11dsp_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "sns_rom_seta11"; }
	virtual machine_config_constructor device_mconfig_additions() const;
};


// device type definition
extern const device_type SNS_LOROM_NECDSP;
extern const device_type SNS_HIROM_NECDSP;
extern const device_type SNS_LOROM_SETA10;
extern const device_type SNS_LOROM_SETA11;




// Devices including DSP dumps to support faulty .sfc dumps missing DSP data

class sns_rom20_necdsp1_legacy_device : public sns_rom20_necdsp_device
{
public:
	// construction/destruction
	sns_rom20_necdsp1_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp1leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom20_necdsp1b_legacy_device : public sns_rom20_necdsp_device
{
public:
	// construction/destruction
	sns_rom20_necdsp1b_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp1bleg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom20_necdsp2_legacy_device : public sns_rom20_necdsp_device
{
public:
	// construction/destruction
	sns_rom20_necdsp2_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp2leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom20_necdsp3_legacy_device : public sns_rom20_necdsp_device
{
public:
	// construction/destruction
	sns_rom20_necdsp3_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp3leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom20_necdsp4_legacy_device : public sns_rom20_necdsp_device
{
public:
	// construction/destruction
	sns_rom20_necdsp4_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp4leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom21_necdsp1_legacy_device : public sns_rom21_necdsp_device
{
public:
	// construction/destruction
	sns_rom21_necdsp1_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "dsp1leg_hi"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom_seta10dsp_legacy_device : public sns_rom_setadsp_device
{
public:
	// construction/destruction
	sns_rom_seta10dsp_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "seta10leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

class sns_rom_seta11dsp_legacy_device : public sns_rom_setadsp_device
{
public:
	// construction/destruction
	sns_rom_seta11dsp_legacy_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	// device-level overrides
	virtual void device_config_complete() { m_shortname = "seta11leg"; }
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;
};

extern const device_type SNS_LOROM_NECDSP1_LEG;
extern const device_type SNS_LOROM_NECDSP1B_LEG;
extern const device_type SNS_LOROM_NECDSP2_LEG;
extern const device_type SNS_LOROM_NECDSP3_LEG;
extern const device_type SNS_LOROM_NECDSP4_LEG;
extern const device_type SNS_HIROM_NECDSP1_LEG;
extern const device_type SNS_LOROM_SETA10_LEG;
extern const device_type SNS_LOROM_SETA11_LEG;

#endif
