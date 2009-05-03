/************************************************************************
Nascom Memory map

    CPU: z80
        0000-03ff   ROM (Nascom1 Monitor)
        0400-07ff   ROM (Nascom2 Monitor extension)
        0800-0bff   RAM (Screen)
        0c00-0c7f   RAM (OS workspace)
        0c80-0cff   RAM (extended OS workspace)
        0d00-0f7f   RAM (Firmware workspace)
        0f80-0fff   RAM (Stack space)
        1000-8fff   RAM (User space)
        9000-97ff   RAM (Programmable graphics RAM/User space)
        9800-afff   RAM (Colour graphics RAM/User space)
        b000-b7ff   ROM (OS extensions)
        b800-bfff   ROM (WP/Naspen software)
        c000-cfff   ROM (Disassembler/colour graphics software)
        d000-dfff   ROM (Assembler/Basic extensions)
        e000-ffff   ROM (Nascom2 Basic)

    Interrupts:

    Ports:
        OUT (00)    0:  Increment keyboard scan
                1:  Reset keyboard scan
                2:
                3:  Read from cassette
                4:
                5:
                6:
                7:
        IN  (00)    Read keyboard
        OUT (01)    Write to cassette/serial
        IN  (01)    Read from cassette/serial
        OUT (02)    Unused
        IN  (02)    ?

    Monitors:
        Nasbug1     1K  Original Nascom1
        Nasbug2         1K
        Nasbug3     Probably non existing
        Nasbug4     2K
        Nassys1     2K  Original Nascom2
        Nassys2     Probably non existing
        Nassys3     2K
        Nassys4     2K
        T4      2K

************************************************************************/

/* Core includes */
#include "driver.h"
#include "includes/nascom1.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/wd17xx.h"
#include "machine/ay31015.h"
#include "machine/z80pio.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"



/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( nascom1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x0bff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x0c00, 0x0fff) AM_RAM
	AM_RANGE(0x1000, 0x13ff) AM_RAM	/* 1Kb */
	AM_RANGE(0x1400, 0x4fff) AM_RAM	/* 16Kb */
	AM_RANGE(0x5000, 0x8fff) AM_RAM	/* 32Kb */
	AM_RANGE(0x9000, 0xafff) AM_RAM	/* 40Kb */
	AM_RANGE(0xb000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( nascom1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x0F)
	AM_RANGE(0x00, 0x00) AM_READWRITE(nascom1_port_00_r, nascom1_port_00_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(nascom1_port_01_r, nascom1_port_01_w)
	AM_RANGE(0x02, 0x02) AM_READ(nascom1_port_02_r)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w )
ADDRESS_MAP_END


static ADDRESS_MAP_START( nascom2_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(nascom1_port_00_r, nascom1_port_00_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(nascom1_port_01_r, nascom1_port_01_w)
	AM_RANGE(0x02, 0x02) AM_READ(nascom1_port_02_r)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w )
	AM_RANGE(0xe0, 0xe3) AM_DEVREADWRITE("wd1793", wd17xx_r, wd17xx_w)
	AM_RANGE(0xe4, 0xe4) AM_READWRITE(nascom2_fdc_select_r, nascom2_fdc_select_w)
	AM_RANGE(0xe5, 0xe5) AM_READ(nascom2_fdc_status_r)
ADDRESS_MAP_END



/*************************************
 *
 *  GFX layouts
 *
 *************************************/

static const gfx_layout nascom1_charlayout =
{
	8, 16,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
	8 * 16
};


static GFXDECODE_START( nascom1 )
	GFXDECODE_ENTRY("gfx1", 0x0000, nascom1_charlayout, 0, 1)
GFXDECODE_END


static const gfx_layout nascom2_charlayout =
{
	8, 14,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8,  3*8,  4*8,  5*8,  6*8,
	  7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8 },
	8 * 16
};


static GFXDECODE_START( nascom2 )
	GFXDECODE_ENTRY("gfx1", 0x0000, nascom2_charlayout, 0, 1)
GFXDECODE_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( nascom1 )
	PORT_START("KEY0")
	PORT_BIT(0x6f, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("KEY1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)  PORT_CHAR('H') PORT_CHAR('h')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)  PORT_CHAR('B') PORT_CHAR('b')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)  PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)  PORT_CHAR('F') PORT_CHAR('f')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)  PORT_CHAR('X') PORT_CHAR('x')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)  PORT_CHAR('T') PORT_CHAR('t')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("KEY2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)    PORT_CHAR('J') PORT_CHAR('j')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)    PORT_CHAR('N') PORT_CHAR('n')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)    PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)    PORT_CHAR('D') PORT_CHAR('d')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)    PORT_CHAR('Z') PORT_CHAR('z')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)    PORT_CHAR('Y') PORT_CHAR('y')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("KEY3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)    PORT_CHAR('K') PORT_CHAR('k')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)    PORT_CHAR('M') PORT_CHAR('m')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)    PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)    PORT_CHAR('E') PORT_CHAR('e')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)    PORT_CHAR('S') PORT_CHAR('s')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)    PORT_CHAR('U') PORT_CHAR('u')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))

	PORT_START("KEY4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)     PORT_CHAR('L') PORT_CHAR('l')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)     PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)     PORT_CHAR('W') PORT_CHAR('w')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)     PORT_CHAR('A') PORT_CHAR('a')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)     PORT_CHAR('I') PORT_CHAR('i')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("KEY5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)     PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)     PORT_CHAR('3') PORT_CHAR('\xA3')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)     PORT_CHAR('Q') PORT_CHAR('q')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)     PORT_CHAR('O') PORT_CHAR('o')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)     PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)     PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)     PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)     PORT_CHAR('P') PORT_CHAR('p')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)     PORT_CHAR('G') PORT_CHAR('g')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)     PORT_CHAR('V') PORT_CHAR('v')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)     PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)     PORT_CHAR('C') PORT_CHAR('c')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)     PORT_CHAR('R') PORT_CHAR('r')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace ClearScreen") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(8)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("New Line")              PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(13)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x58, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR(UCHAR_SHIFT_2) PORT_CHAR('@')
INPUT_PORTS_END

static INPUT_PORTS_START( nascom2 )
	PORT_INCLUDE(nascom1)

	PORT_MODIFY("KEY6")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                            PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('[') PORT_CHAR('\\')

	PORT_MODIFY("KEY7")
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD)                            PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(']') PORT_CHAR('_')

	PORT_MODIFY("KEY8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Back CS")       PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(8)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter  Escape") PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(13)  PORT_CHAR(27)
INPUT_PORTS_END


/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static const ay31015_config nascom1_ay31015_config =
{
	AY_3_1015,
	( XTAL_16MHz / 16 ) / 256,
	( XTAL_16MHz / 16 ) / 256,
	nascom1_hd6402_si,
	nascom1_hd6402_so,
	NULL
};


static const z80pio_interface nascom1_z80pio_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


static MACHINE_DRIVER_START( nascom1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(nascom1_mem, 0)
	MDRV_CPU_IO_MAP(nascom1_io, 0)

	MDRV_MACHINE_RESET( nascom1 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(48 * 8, 16 * 16)
	MDRV_SCREEN_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 16 - 1)
	MDRV_GFXDECODE(nascom1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE(nascom1)

	MDRV_AY31015_ADD( "hd6402", nascom1_ay31015_config )

	MDRV_Z80PIO_ADD( "z80pio", nascom1_z80pio_intf )

	/* devices */
	MDRV_SNAPSHOT_ADD("snapshot", nascom1, "nas", 0.5)

	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nascom2 )
	MDRV_IMPORT_FROM(nascom1)
	MDRV_CPU_REPLACE("maincpu", Z80, XTAL_16MHz/8)
	MDRV_CPU_IO_MAP(nascom2_io, 0)

	/* video hardware */
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(48 * 8, 16 * 14)
	MDRV_SCREEN_VISIBLE_AREA(0, 48 * 8 - 1, 0, 16 * 14 - 1)
	MDRV_GFXDECODE(nascom2)
	MDRV_VIDEO_UPDATE(nascom2)
	
	MDRV_WD1793_ADD("wd1793", nascom2_wd17xx_interface )
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START(nascom1)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_SYSTEM_BIOS(0, "T4", "NasBug T4")
	ROMX_LOAD("nasbugt4.rom", 0x0000, 0x0800, CRC(f391df68) SHA1(00218652927afc6360c57e77d6a4fd32d4e34566), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "T1", "NasBug T1")
	ROMX_LOAD("nasbugt1.rom", 0x0000, 0x0400, CRC(8ea07054) SHA1(3f9a8632826003d6ea59d2418674d0fb09b83a4c), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "T2", "NasBug T2")
	ROMX_LOAD("nasbugt2.rom", 0x0000, 0x0400, CRC(e371b58a) SHA1(485b20a560b587cf9bb4208ba203b12b3841689b), ROM_BIOS(3))
	ROM_REGION(0x0800, "gfx1", 0)
	ROM_LOAD("nascom1.chr",   0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
ROM_END


ROM_START(nascom2)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_SYSTEM_BIOS( 0, "NS3", "NasSys 3")
	ROMX_LOAD("nassys3.rom", 0x0000, 0x0800, CRC(3da17373) SHA1(5fbda15765f04e4cd08cf95c8d82ce217889f240), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "NS1", "NasSys 1")
	ROMX_LOAD("nassys1.rom", 0x0000, 0x0800, CRC(b6300716) SHA1(29da7d462ba3f569f70ed3ecd93b981f81c7adfa), ROM_BIOS(2))
	ROM_LOAD("nasdos.rom",   0xd000, 0x1000, CRC(54a36f6d) SHA1(1d063d04be5024f128bd589e6edc066e9a63fc1b))
	ROM_LOAD("basic.rom",    0xe000, 0x2000, CRC(5cb5197b) SHA1(c41669c2b6d6dea808741a2738426d97bccc9b07))
	ROM_REGION(0x1000, "gfx1", 0)
	ROM_LOAD("nascom1.chr",  0x0000, 0x0800, CRC(33e92a04) SHA1(be6e1cc80e7f95a032759f7df19a43c27ff93a52))
	ROM_LOAD("nasgra.chr",   0x0800, 0x0800, CRC(2bc09d32) SHA1(d384297e9b02cbcb283c020da51b3032ff62b1ae))
ROM_END



/*************************************
 *
 *  System configs
 *
 *************************************/

//static void nascom1_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
//{
//	/* cassette */
//	switch(state)
//	{
//		/* --- the following bits of info are returned as 64-bit signed integers --- */
//		case MESS_DEVINFO_INT_TYPE:							info->i = IO_CASSETTE; break;
//		case MESS_DEVINFO_INT_READABLE:						info->i = 1; break;
//		case MESS_DEVINFO_INT_WRITEABLE:						info->i = 0; break;
//		case MESS_DEVINFO_INT_CREATABLE:						info->i = 0; break;
//		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;
//
//		/* --- the following bits of info are returned as pointers to data or functions --- */
//		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(nascom1_cassette); break;
//		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(nascom1_cassette); break;
//
//		/* --- the following bits of info are returned as NULL-terminated strings --- */
//		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cas"); break;
//	}
//}


static void nascom2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(nascom2_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}


static SYSTEM_CONFIG_START( nascom1 )
	CONFIG_RAM(1 * 1024)
	CONFIG_RAM(16 * 1024)
	CONFIG_RAM(32 * 1024)
	CONFIG_RAM_DEFAULT(40 * 1024)
//	CONFIG_DEVICE(nascom1_cassette_getinfo)
SYSTEM_CONFIG_END


static SYSTEM_CONFIG_START( nascom2 )
	CONFIG_IMPORT_FROM(nascom1)
	CONFIG_DEVICE(nascom2_floppy_getinfo)
SYSTEM_CONFIG_END



/*************************************
 *
 *  Driver definitions
 *
 *************************************/

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY                     FULLNAME        FLAGS */
COMP( 1978, nascom1,    0,          0,      nascom1,    nascom1,    nascom1,    nascom1,    "Nascom Microcomputers",    "Nascom 1",     0 )
COMP( 1979, nascom2,    nascom1,    0,      nascom2,    nascom2,    nascom1,    nascom2,    "Nascom Microcomputers",    "Nascom 2",     0 )
