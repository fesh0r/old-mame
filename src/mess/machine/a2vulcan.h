/*********************************************************************

    a2vulcan.h

    Applied Engineering Vulcan IDE controller

*********************************************************************/

#ifndef __A2BUS_VULCAN__
#define __A2BUS_VULCAN__

#include "emu.h"
#include "machine/a2bus.h"
#include "machine/ataintf.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class a2bus_vulcanbase_device:
	public device_t,
	public device_a2bus_card_interface
{
public:
	// construction/destruction
	a2bus_vulcanbase_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const;
	virtual const rom_entry *device_rom_region() const;

protected:
	virtual void device_start();
	virtual void device_reset();

	// overrides of standard a2bus slot functions
	virtual UINT8 read_c0nx(address_space &space, UINT8 offset);
	virtual void write_c0nx(address_space &space, UINT8 offset, UINT8 data);
	virtual UINT8 read_cnxx(address_space &space, UINT8 offset);
	virtual UINT8 read_c800(address_space &space, UINT16 offset);
	virtual void write_c800(address_space &space, UINT16 offset, UINT8 data);

	required_device<ata_interface_device> m_ata;

	UINT8 *m_rom;
	UINT8 m_ram[8*1024];

private:
	UINT16 m_lastdata;
	int m_rombank, m_rambank;
	bool m_last_read_was_0;
};

class a2bus_vulcan_device : public a2bus_vulcanbase_device
{
public:
	a2bus_vulcan_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

protected:
};

// device type definition
extern const device_type A2BUS_VULCAN;

#endif /* __A2BUS_VULCAN__ */
