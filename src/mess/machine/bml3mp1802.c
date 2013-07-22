/*********************************************************************

    bml3mp1802.c

    Hitachi MP-1802 floppy disk controller card for the MB-6890
    Hitachi MP-3550 floppy drive is attached

*********************************************************************/

#include "emu.h"
#include "bml3mp1802.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type BML3BUS_MP1802 = &device_creator<bml3bus_mp1802_device>;

static const floppy_interface bml3_mp1802_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	NULL
};

static WRITE_LINE_DEVICE_HANDLER( bml3_wd17xx_intrq_w )
{
	bml3bus_mp1802_device *fdc = dynamic_cast<bml3bus_mp1802_device *>(device->owner());
	if (state) {
		fdc->m_bml3bus->set_nmi_line(PULSE_LINE);
	}
}

const wd17xx_interface bml3_wd17xx_interface =
{
	DEVCB_NULL,
	DEVCB_LINE(bml3_wd17xx_intrq_w),
	DEVCB_NULL,
	{FLOPPY_0, FLOPPY_1}
};


#define MP1802_ROM_REGION  "mp1802_rom"

ROM_START( mp1802 )
	ROM_REGION(0x10000, MP1802_ROM_REGION, 0)
	// MP-1802 disk controller ROM, which replaces part of the system ROM
	ROM_LOAD( "mp1802.rom", 0xf800, 0x800, BAD_DUMP CRC(8d0dc101) SHA1(92f7d1cebecafa7472e45c4999520de5c01c6dbc))
ROM_END

MACHINE_CONFIG_FRAGMENT( mp1802 )
	MCFG_MB8866_ADD("wd17xx", bml3_wd17xx_interface )
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(bml3_mp1802_floppy_interface)
MACHINE_CONFIG_END

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor bml3bus_mp1802_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( mp1802 );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const rom_entry *bml3bus_mp1802_device::device_rom_region() const
{
	return ROM_NAME( mp1802 );
}

READ8_MEMBER( bml3bus_mp1802_device::bml3_mp1802_r)
{
	return wd17xx_drq_r(m_wd17xx) ? 0x00 : 0x80;
}

WRITE8_MEMBER( bml3bus_mp1802_device::bml3_mp1802_w)
{
	int drive = data & 0x03;
	int side = BIT(data, 4);
	int motor = BIT(data, 3);
	const char *floppy_name = NULL;
	switch (drive) {
	case 0:
		floppy_name = FLOPPY_0;
		break;
	case 1:
		floppy_name = FLOPPY_1;
		break;
	case 2:
		floppy_name = FLOPPY_2;
		break;
	case 3:
		floppy_name = FLOPPY_3;
		break;
	}
	device_t *floppy = subdevice(floppy_name);
	wd17xx_set_drive(m_wd17xx,drive);
	floppy_mon_w(floppy, !motor);
	floppy_drive_set_ready_state(floppy, ASSERT_LINE, 0);
	wd17xx_set_side(m_wd17xx,side);
}


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

bml3bus_mp1802_device::bml3bus_mp1802_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, BML3BUS_MP1802, "Hitachi MP-1802 Floppy Controller Card", tag, owner, clock, "bml3mp1802", __FILE__),
	device_bml3bus_card_interface(mconfig, *this),
	m_wd17xx(*this, "wd17xx")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void bml3bus_mp1802_device::device_start()
{
	// set_bml3bus_device makes m_slot valid
	set_bml3bus_device();

	m_rom = memregion(MP1802_ROM_REGION)->base();

	// install into memory
	address_space &space_prg = machine().firstcpu->space(AS_PROGRAM);
	space_prg.install_legacy_readwrite_handler(m_wd17xx, 0xff00, 0xff03, FUNC(wd17xx_r), FUNC(wd17xx_w));
	space_prg.install_readwrite_handler(0xff04, 0xff04, read8_delegate( FUNC(bml3bus_mp1802_device::bml3_mp1802_r), this), write8_delegate(FUNC(bml3bus_mp1802_device::bml3_mp1802_w), this) );
	// overwriting the main ROM (rather than using e.g. install_rom) should mean that bank switches for RAM expansion still work...
	UINT8 *mainrom = device().machine().root_device().memregion("maincpu")->base();
	memcpy(mainrom + 0xf800, m_rom + 0xf800, 0x800);
}

void bml3bus_mp1802_device::device_reset()
{
}
