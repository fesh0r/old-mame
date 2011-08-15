/***************************************************************************

  Asante MC3B NuBus Ethernet card
 
  Based on National Semiconductor DP83902 "ST-NIC"
 
  Fs*D0000 - 64k RAM buffer (used as DP83902 DMA target)
  Fs*E0000 - DP83902 registers
 
***************************************************************************/

#include "emu.h"
#include "machine/nubus_asntmc3b.h"

#define ASNTMC3B_ROM_REGION  "asntm3b_rom"
#define ASNTMC3B_DP83902  "dp83902"

MACHINE_CONFIG_FRAGMENT( asntm3b )
	MCFG_DP8390X_ADD(ASNTMC3B_DP83902, 10000000)
MACHINE_CONFIG_END

ROM_START( asntm3b )
	ROM_REGION(0x4000, ASNTMC3B_ROM_REGION, 0)
	ROM_LOAD( "asante_mc3b.bin", 0x000000, 0x004000, CRC(4f86d451) SHA1(d0a41df667e6b51fbc63f9251d593f4fc49104ba) )
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type NUBUS_ASNTMC3B = &device_creator<nubus_asntm3b_device>;


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor nubus_asntm3b_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( asntm3b );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *nubus_asntm3b_device::device_rom_region() const
{
	return ROM_NAME( asntm3b );
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  nubus_asntm3b_device - constructor
//-------------------------------------------------

nubus_asntm3b_device::nubus_asntm3b_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, NUBUS_ASNTMC3B, "Asante MC3B Ethernet card", tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this),
		m_dp83902(*this, ASNTMC3B_DP83902)
{
	m_shortname = "nb_amc3b";
}

nubus_asntm3b_device::nubus_asntm3b_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock) :
        device_t(mconfig, type, name, tag, owner, clock),
		device_nubus_card_interface(mconfig, *this),
		device_slot_card_interface(mconfig, *this),
		m_dp83902(*this, ASNTMC3B_DP83902)
{
	m_shortname = "nb_amc3b";
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void nubus_asntm3b_device::device_start()
{
	UINT32 slotspace;

	// set_nubus_device makes m_slot valid
	set_nubus_device();
	install_declaration_rom(this, ASNTMC3B_ROM_REGION, true);

	slotspace = get_slotspace();

//	printf("[ASNTMC3B %p] slotspace = %x\n", this, slotspace);

	m_nubus->install_device(slotspace+0xd0000, slotspace+0xdffff, read8_delegate(FUNC(nubus_asntm3b_device::asntm3b_ram_r), this), write8_delegate(FUNC(nubus_asntm3b_device::asntm3b_ram_w), this));
	m_nubus->install_device(slotspace+0xbd0000, slotspace+0xbdffff, read8_delegate(FUNC(nubus_asntm3b_device::asntm3b_ram_r), this), write8_delegate(FUNC(nubus_asntm3b_device::asntm3b_ram_w), this));
	m_nubus->install_device(slotspace+0xe0000, slotspace+0xe003f, read32_delegate(FUNC(nubus_asntm3b_device::en_r), this), write32_delegate(FUNC(nubus_asntm3b_device::en_w), this));
	m_nubus->install_device(slotspace+0xbe0000, slotspace+0xbe003f, read32_delegate(FUNC(nubus_asntm3b_device::en_r), this), write32_delegate(FUNC(nubus_asntm3b_device::en_w), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void nubus_asntm3b_device::device_reset()
{
}

WRITE8_MEMBER( nubus_asntm3b_device::asntm3b_ram_w )
{
	m_ram[offset] = data; 
}

READ8_MEMBER( nubus_asntm3b_device::asntm3b_ram_r )
{
	return m_ram[offset];
}

WRITE32_MEMBER( nubus_asntm3b_device::en_w )
{
    if (mem_mask == 0xff000000)
    {
        m_dp83902->reg_w(space, offset, data>>24);
    }
    else if (mem_mask == 0xffff0000)
    {
        m_dp83902->reg_w(space, offset, data>>24);
        m_dp83902->reg_w(space, offset+1, (data>>16)&0xff);
    }
    else
    {
        fatalerror("asntm3b: write %08x to DP83902 @ %x with unhandled mask %08x (PC=%x)\n", data, offset, mem_mask, cpu_get_pc(&space.device()));
    }
}

READ32_MEMBER( nubus_asntm3b_device::en_r )
{
    if (mem_mask == 0xff000000)
    {
        return (m_dp83902->reg_r(space, offset)<<24);
    }
    else
    {
        fatalerror("asntm3b: read DP83902 @ %x with unhandled mask %08x (PC=%x)\n", offset, mem_mask, cpu_get_pc(&space.device()));
    }

    return 0;
}

