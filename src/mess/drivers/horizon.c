/***************************************************************************

        NorthStar Horizon

        07/12/2009 Skeleton driver.

        It appears these machines say nothing until a floppy disk is
        succesfully loaded. The memory range EA00-EB40 appears to be
        used by devices, particularly the FDC.

    http://www.hartetechnologies.com/manuals/Northstar/

****************************************************************************/

/*

    TODO:

    - connect to S-100 bus
    - USARTs
    - parallel I/O
    - motherboard ports
    - RTC
    - RAM boards
    - floppy boards
    - floating point board
    - SOROC IQ 120 CRT terminal
    - NEC 5530-2 SPINWRITER printer
    - Anadex DP-8000 printer

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/i8251.h"
#include "machine/serial.h"
#include "machine/s100.h"
#include "machine/s100_nsmdsa.h"
#include "machine/s100_nsmdsad.h"

#define Z80_TAG         "z80"
#define I8251_L_TAG     "3a"
#define I8251_R_TAG     "4a"
#define RS232_A_TAG     "rs232a"
#define RS232_B_TAG     "rs232b"

class horizon_state : public driver_device
{
public:
	horizon_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, Z80_TAG),
			m_usart_l(*this, I8251_L_TAG),
			m_usart_r(*this, I8251_L_TAG)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<i8251_device> m_usart_l;
	required_device<i8251_device> m_usart_r;
};



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( horizon_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( horizon_mem, AS_PROGRAM, 8, horizon_state )
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( horizon_io )
//-------------------------------------------------

static ADDRESS_MAP_START( horizon_io, AS_IO, 8, horizon_state )
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( sage2 )
//-------------------------------------------------

static INPUT_PORTS_START( horizon )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  i8251_interface usart_l_intf
//-------------------------------------------------

static const i8251_interface usart_l_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, serial_port_device, rx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, serial_port_device, tx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, rs232_port_device, dsr_r),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, rs232_port_device, dtr_w),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, rs232_port_device, rts_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  i8251_interface usart_r_intf
//-------------------------------------------------

static const i8251_interface usart_r_intf =
{
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, serial_port_device, rx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, serial_port_device, tx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, rs232_port_device, dsr_r),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, rs232_port_device, dtr_w),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, rs232_port_device, rts_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  rs232_port_interface rs232a_intf
//-------------------------------------------------

static DEVICE_INPUT_DEFAULTS_START( terminal )
	DEVICE_INPUT_DEFAULTS( "TERM_FRAME", 0x0f, 0x06 ) // 9600
	DEVICE_INPUT_DEFAULTS( "TERM_FRAME", 0x30, 0x00 ) // 8N1
DEVICE_INPUT_DEFAULTS_END

static const rs232_port_interface rs232a_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  rs232_port_interface rs232b_intf
//-------------------------------------------------

static const rs232_port_interface rs232b_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  S100_INTERFACE( s100_intf )
//-------------------------------------------------

static SLOT_INTERFACE_START( horizon_s100_cards )
	SLOT_INTERFACE("mdsa", S100_MDS_A)
	SLOT_INTERFACE("mdsad", S100_MDS_AD)
	//SLOT_INTERFACE("hram", S100_HRAM)
	//SLOT_INTERFACE("ram32a", S100_RAM32A)
	//SLOT_INTERFACE("ram16a", S100_RAM16A)
	//SLOT_INTERFACE("fpb", S100_FPB)
SLOT_INTERFACE_END

static S100_INTERFACE( s100_intf )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(Z80_TAG, Z80_INPUT_LINE_WAIT),
	DEVCB_NULL,
	DEVCB_NULL
};



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( horizon )
//-------------------------------------------------

static MACHINE_CONFIG_START( horizon, horizon_state )
	// basic machine hardware
	MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(horizon_mem)
	MCFG_CPU_IO_MAP(horizon_io)

	// devices
	MCFG_I8251_ADD(I8251_L_TAG, usart_l_intf)
	MCFG_I8251_ADD(I8251_R_TAG, usart_r_intf)
	MCFG_RS232_PORT_ADD(RS232_A_TAG, rs232a_intf, default_rs232_devices, "serial_terminal", terminal)
	MCFG_RS232_PORT_ADD(RS232_B_TAG, rs232b_intf, default_rs232_devices, NULL, NULL)

	// S-100
	MCFG_S100_BUS_ADD(s100_intf)
	//MCFG_S100_SLOT_ADD("s100_1", horizon_s100_cards, NULL, NULL) // CPU
	MCFG_S100_SLOT_ADD("s100_2", horizon_s100_cards, NULL, NULL) // RAM
	MCFG_S100_SLOT_ADD("s100_3", horizon_s100_cards, "mdsad", NULL) // MDS
	MCFG_S100_SLOT_ADD("s100_4", horizon_s100_cards, NULL, NULL) // FPB
	MCFG_S100_SLOT_ADD("s100_5", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_6", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_7", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_8", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_9", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_10", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_11", horizon_s100_cards, NULL, NULL)
	MCFG_S100_SLOT_ADD("s100_12", horizon_s100_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "horizon")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( nshrz )
//-------------------------------------------------

ROM_START( nshrz )
	ROM_REGION( 0x400, Z80_TAG, 0 )
	ROM_LOAD( "option.prom", 0x000, 0x400, NO_DUMP )
ROM_END


//-------------------------------------------------
//  ROM( vector1 )
//-------------------------------------------------

ROM_START( vector1 ) // This one have different I/O
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASEFF )
	ROM_LOAD( "horizon.bin", 0xe800, 0x0100, CRC(7aafa134) SHA1(bf1552c4818f30473798af4f54e65e1957e0db48))
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                             FULLNAME    FLAGS
COMP( 1976, nshrz,   0,       0,    horizon,   horizon, driver_device, 0,  "North Star Computers", "Horizon (North Star Computers)", GAME_NOT_WORKING | GAME_NO_SOUND_HW )
COMP( 1979, vector1,  nshrz, 0,    horizon,   horizon, driver_device, 0,  "Vector Graphic", "Vector 1+ (DD drive)", GAME_NOT_WORKING | GAME_NO_SOUND_HW )
