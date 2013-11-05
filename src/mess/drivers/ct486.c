/***************************************************************************

    PC/AT 486 with Chips & Technologies CS4031 chipset

    license: MAME, GPL-2.0+
    copyright-holders: Dirk Best

***************************************************************************/

#include "emu.h"
#include "cpu/i386/i386.h"
#include "machine/ram.h"
#include "machine/cs4031.h"
#include "machine/at_keybc.h"
#include "machine/pc_kbdc.h"
#include "machine/pc_keyboards.h"
#include "machine/isa.h"
#include "machine/isa_cards.h"
#include "sound/speaker.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class ct486_state : public driver_device
{
public:
	ct486_state(const machine_config &mconfig, device_type type, const char *tag) :
	driver_device(mconfig, type, tag),
	m_maincpu(*this, "maincpu"),
	m_cs4031(*this, "cs4031"),
	m_isabus(*this, "isabus"),
	m_speaker(*this, "speaker")
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<cs4031_device> m_cs4031;
	required_device<isa16_device> m_isabus;
	required_device<speaker_sound_device> m_speaker;

	virtual void machine_start();

	IRQ_CALLBACK_MEMBER( irq_callback ) { return m_cs4031->int_ack_r(); }

	DECLARE_READ16_MEMBER( cs4031_ior );
	DECLARE_WRITE16_MEMBER( cs4031_iow );
	DECLARE_WRITE_LINE_MEMBER( cs4031_hold );
	DECLARE_WRITE8_MEMBER( cs4031_tc ) { m_isabus->eop_w(offset, data); }
	DECLARE_WRITE_LINE_MEMBER( cs4031_spkr ) { m_speaker->level_w(state); }
};


//**************************************************************************
//  MACHINE EMULATION
//**************************************************************************

void ct486_state::machine_start()
{
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(ct486_state::irq_callback), this));
}

READ16_MEMBER( ct486_state::cs4031_ior )
{
	if (offset < 4)
		return m_isabus->dack_r(offset);
	else
		return m_isabus->dack16_r(offset);
}

WRITE16_MEMBER( ct486_state::cs4031_iow )
{
	if (offset < 4)
		m_isabus->dack_w(offset, data);
	else
		m_isabus->dack16_w(offset, data);
}

WRITE_LINE_MEMBER( ct486_state::cs4031_hold )
{
	// halt cpu
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	// and acknowledge hold
	m_cs4031->hlda_w(state);
}


//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

static ADDRESS_MAP_START( ct486_map, AS_PROGRAM, 32, ct486_state )
ADDRESS_MAP_END

static ADDRESS_MAP_START( ct486_io, AS_IO, 32, ct486_state )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END


//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

static const at_keyboard_controller_interface keybc_intf =
{
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, kbrst_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, gatea20_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq01_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER("pc_kbdc", pc_kbdc_device, clock_write_from_mb),
	DEVCB_DEVICE_LINE_MEMBER("pc_kbdc", pc_kbdc_device, data_write_from_mb)
};

static const pc_kbdc_interface pc_kbdc_intf =
{
	DEVCB_DEVICE_LINE_MEMBER("keybc", at_keyboard_controller_device, keyboard_clock_w),
	DEVCB_DEVICE_LINE_MEMBER("keybc", at_keyboard_controller_device, keyboard_data_w)
};

static const isa16bus_interface isabus_intf =
{
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq09_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq03_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq04_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq05_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq06_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq07_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq10_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq11_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq12_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq14_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, irq15_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq0_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq1_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq2_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq3_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq5_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq6_w),
	DEVCB_DEVICE_LINE_MEMBER("cs4031", cs4031_device, dreq7_w),
};

static MACHINE_CONFIG_START( ct486, ct486_state )
	MCFG_CPU_ADD("maincpu", I486, XTAL_25MHz)
	MCFG_CPU_PROGRAM_MAP(ct486_map)
	MCFG_CPU_IO_MAP(ct486_io)

	MCFG_CS4031_ADD("cs4031", XTAL_25MHz, "maincpu", "isa", "bios", "keybc")
	// cpu connections
	MCFG_CS4031_HOLD(WRITELINE(ct486_state, cs4031_hold));
	MCFG_CS4031_NMI(INPUTLINE("maincpu", INPUT_LINE_NMI));
	MCFG_CS4031_INTR(INPUTLINE("maincpu", INPUT_LINE_IRQ0));
	MCFG_CS4031_CPURESET(INPUTLINE("maincpu", INPUT_LINE_RESET));
	MCFG_CS4031_A20M(INPUTLINE("maincpu", INPUT_LINE_A20));
	// isa dma
	MCFG_CS4031_IOR(READ16(ct486_state, cs4031_ior))
	MCFG_CS4031_IOW(WRITE16(ct486_state, cs4031_iow))
	MCFG_CS4031_TC(WRITE8(ct486_state, cs4031_tc))
	// speaker
	MCFG_CS4031_SPKR(WRITELINE(ct486_state, cs4031_spkr))

	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4M")
	MCFG_RAM_EXTRA_OPTIONS("1M,2M,8M,16M,32M,64M")

	MCFG_AT_KEYBOARD_CONTROLLER_ADD("keybc", XTAL_12MHz, keybc_intf)
	MCFG_PC_KBDC_ADD("pc_kbdc", pc_kbdc_intf)
	MCFG_PC_KBDC_SLOT_ADD("pc_kbdc", "kbd", pc_at_keyboards, STR_KBD_MICROSOFT_NATURAL)

	MCFG_ISA16_BUS_ADD("isabus", ":maincpu", isabus_intf)
	MCFG_ISA_BUS_IOCHCK(DEVWRITELINE("cs4031", cs4031_device, iochck_w))
	MCFG_ISA16_SLOT_ADD("isabus", "board1", pc_isa16_cards, "fdcsmc", true)
	MCFG_ISA16_SLOT_ADD("isabus", "board2", pc_isa16_cards, "comat", true)
	MCFG_ISA16_SLOT_ADD("isabus", "board3", pc_isa16_cards, "ide", true)
	MCFG_ISA16_SLOT_ADD("isabus", "board4", pc_isa16_cards, "lpt", true)
	MCFG_ISA16_SLOT_ADD("isabus", "isa1", pc_isa16_cards, "svga_et4k", false)
	MCFG_ISA16_SLOT_ADD("isabus", "isa2", pc_isa16_cards, NULL, false)
	MCFG_ISA16_SLOT_ADD("isabus", "isa3", pc_isa16_cards, NULL, false)
	MCFG_ISA16_SLOT_ADD("isabus", "isa4", pc_isa16_cards, NULL, false)
	MCFG_ISA16_SLOT_ADD("isabus", "isa5", pc_isa16_cards, NULL, false)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD("speaker", SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	// video hardware
	MCFG_PALETTE_LENGTH(256) // todo: really needed?
MACHINE_CONFIG_END


//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( ct486 )
	ROM_REGION(0x40000, "isa", ROMREGION_ERASEFF)
	ROM_REGION(0x100000, "bios", 0)
	ROM_LOAD("chips_1.ami", 0xf0000, 0x10000, CRC(a14a7511) SHA1(b88d09be66905ed2deddc26a6f8522e7d2d6f9a8))
ROM_END


//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

COMP( 1993, ct486, 0, 0, ct486, 0, driver_device, 0, "<unknown>", "PC/AT 486 with CS4031 chipset", 0 )
