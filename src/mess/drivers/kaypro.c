/*************************************************************************************************


    Kaypro 2/83 computer - the very first Kaypro II - 2 full size floppy drives.
    Each disk was single sided, and could hold 191k. The computer had 2x pio
    and 1x sio. One of the sio ports communicated with the keyboard with a coiled
    telephone cord, complete with modular plug on each end. The keyboard carries
    its own Intel 8748 processor and is an intelligent device.

    There are 2 major problems preventing this driver from working

    - MESS is not capable of conducting realtime serial communications between devices

    - MAME's z80sio implementation is lacking important deatures:
    -- cannot change baud rate on the fly
    -- cannot specify different rates for each channel
    -- cannot specify different rates for Receive and Transmit
    -- the callback doesn't appear to support channel B ??


    Things that need doing:

    - Kaypro2x/4a are not booting.

    - Hard Disk not emulated.
      The controller is a WD1002 (original version, for Winchester drives).

    - Kaypro 4 plus 88 does work as a normal Kaypro, but the extra processor needs
      to be worked out.


**************************************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/ctronics.h"
#include "machine/kay_kbd.h"
#include "sound/beep.h"
#include "devices/snapquik.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "includes/kaypro.h"


static READ8_HANDLER( kaypro2x_87) { return 0x7f; }	/* to bypass unemulated HD controller */

/***********************************************************

    Address Maps

************************************************************/

static ADDRESS_MAP_START( kaypro_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM AM_REGION("maincpu", 0x0000)
	AM_RANGE(0x3000, 0x3fff) AM_RAM AM_REGION("maincpu", 0x3000) AM_BASE_SIZE_GENERIC(videoram)
	AM_RANGE(0x4000, 0xffff) AM_RAM AM_REGION("rambank", 0x4000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kayproii_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_WRITE(kaypro_baud_a_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80sio", kaypro_sio_r, kaypro_sio_w)
	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE("z80pio_g", z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x0c, 0x0f) AM_WRITE(kayproii_baud_b_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE("wd1793", wd17xx_r, wd17xx_w)
	AM_RANGE(0x1c, 0x1f) AM_DEVREADWRITE("z80pio_s", z80pio_ba_cd_r, z80pio_ba_cd_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kaypro2x_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_WRITE(kaypro_baud_a_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE("z80sio", kaypro_sio_r, kaypro_sio_w)
	AM_RANGE(0x08, 0x0b) AM_WRITE(kaypro2x_baud_a_w)
	AM_RANGE(0x0c, 0x0f) AM_DEVREADWRITE("z80sio_2x", z80sio_cd_ba_r, z80sio_cd_ba_w)
	AM_RANGE(0x10, 0x13) AM_DEVREADWRITE("wd1793", wd17xx_r, wd17xx_w)
	AM_RANGE(0x14, 0x17) AM_READWRITE(kaypro2x_system_port_r,kaypro2x_system_port_w)
	AM_RANGE(0x18, 0x1b) AM_DEVWRITE("centronics", centronics_data_w)
	AM_RANGE(0x1c, 0x1c) AM_READWRITE(kaypro2x_status_r,kaypro2x_index_w)
	AM_RANGE(0x1d, 0x1d) AM_DEVREAD("crtc", mc6845_register_r) AM_WRITE(kaypro2x_register_w)
	AM_RANGE(0x1f, 0x1f) AM_READWRITE(kaypro2x_videoram_r,kaypro2x_videoram_w)

	/* The below are not emulated */
/*  AM_RANGE(0x20, 0x23) AM_DEVREADWRITE("z80pio", kaypro2x_pio_r, kaypro2x_pio_w) - for RTC and Modem
    AM_RANGE(0x24, 0x27) communicate with MM58167A RTC. Modem uses TMS99531 and TMS99532 chips.
    AM_RANGE(0x80, 0x80) Hard drive controller card I/O port - 10MB hard drive only fitted to the Kaypro 10
    AM_RANGE(0x81, 0x81) Hard Drive READ error register, WRITE precomp
    AM_RANGE(0x82, 0x82) Hard Drive Sector register count I/O
    AM_RANGE(0x83, 0x83) Hard Drive Sector register number I/O
    AM_RANGE(0x84, 0x84) Hard Drive Cylinder low register I/O
    AM_RANGE(0x85, 0x85) Hard Drive Cylinder high register I/O
    AM_RANGE(0x86, 0x86) Hard Drive Size / Drive / Head register I/O
    AM_RANGE(0x87, 0x87) Hard Drive READ status register, WRITE command register */
	AM_RANGE(0x20, 0x86) AM_NOP
	AM_RANGE(0x87, 0x87) AM_READ(kaypro2x_87)
ADDRESS_MAP_END


/***************************************************************

    F4 CHARACTER DISPLAYER

****************************************************************/
static const gfx_layout kayproii_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static const gfx_layout kaypro2x_charlayout =
{
	8, 16,					/* 8 x 16 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( kayproii )
	GFXDECODE_ENTRY( "gfx1", 0x0000, kayproii_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( kaypro2x )
	GFXDECODE_ENTRY( "gfx1", 0x0000, kaypro2x_charlayout, 0, 1 )
GFXDECODE_END

/***************************************************************

    Interfaces

****************************************************************/

static const z80_daisy_chain kayproii_daisy_chain[] =
{
	{ "z80sio" },		/* sio */
	{ "z80pio_s" },		/* System pio */
	{ "z80pio_g" },		/* General purpose pio */
	{ NULL }
};

static const z80_daisy_chain kaypro2x_daisy_chain[] =
{
	{ "z80sio" },		/* sio for RS232C and keyboard */
	{ "z80sio_2x" },	/* sio for serial printer and inbuilt modem */
	{ NULL }
};

static const mc6845_interface kaypro2x_crtc = {
	"screen",			/* name of screen */
	7,				/* number of dots per character */
	NULL,
	kaypro2x_update_row,		/* handler to display a scanline */
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};


/***********************************************************

    Machine Driver

************************************************************/
static FLOPPY_OPTIONS_START(kayproii)
	FLOPPY_OPTION(kayproii, "dsk", "Kaypro II disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static FLOPPY_OPTIONS_START(kaypro2x)
	FLOPPY_OPTION(kaypro2x, "dsk", "Kaypro 2x disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([0]))
FLOPPY_OPTIONS_END

static const floppy_config kayproii_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(kayproii),
	DO_NOT_KEEP_GEOMETRY
};
static const floppy_config kaypro2x_floppy_config =
{
	DEVCB_LINE(wd17xx_idx_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(kaypro2x),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( kayproii )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 2500000)	/* 2.5 MHz */
	MDRV_CPU_PROGRAM_MAP(kaypro_map)
	MDRV_CPU_IO_MAP(kayproii_io)
	MDRV_CPU_VBLANK_INT("screen", kay_kbd_interrupt)	/* this doesn't actually exist, it is to run the keyboard */
	MDRV_CPU_CONFIG(kayproii_daisy_chain)

	MDRV_MACHINE_START( kayproii )
	MDRV_MACHINE_RESET( kayproii )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*7, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*7-1,0,24*10-1)
	MDRV_GFXDECODE(kayproii)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(kaypro)

	MDRV_VIDEO_START( kaypro )
	MDRV_VIDEO_UPDATE( kayproii )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", kayproii, "com,cpm", 3)
	MDRV_WD1793_ADD("wd1793", kaypro_wd1793_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_Z80PIO_ADD( "z80pio_g", 2500000, kayproii_pio_g_intf )
	MDRV_Z80PIO_ADD( "z80pio_s", 2500000, kayproii_pio_s_intf )
	MDRV_Z80SIO_ADD( "z80sio", 4800, kaypro_sio_intf )	/* start at 300 baud */

	MDRV_FLOPPY_2_DRIVES_ADD(kayproii_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kaypro4 )
	MDRV_IMPORT_FROM(kayproii)

	MDRV_DEVICE_REMOVE("z80pio_s")
	MDRV_Z80PIO_ADD( "z80pio_s", 2500000, kaypro4_pio_s_intf )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kaypro2x )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)	/* 4 MHz */
	MDRV_CPU_PROGRAM_MAP(kaypro_map)
	MDRV_CPU_IO_MAP(kaypro2x_io)
	MDRV_CPU_VBLANK_INT("screen", kay_kbd_interrupt)
	MDRV_CPU_CONFIG(kaypro2x_daisy_chain)

	MDRV_MACHINE_START( kaypro2x )
	MDRV_MACHINE_RESET( kaypro2x )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*16)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,25*16-1)
	MDRV_GFXDECODE(kaypro2x)
	MDRV_PALETTE_LENGTH(3)
	MDRV_PALETTE_INIT(kaypro)

	MDRV_MC6845_ADD("crtc", MC6845, 2000000, kaypro2x_crtc) /* comes out of ULA - needs to be measured */

	MDRV_VIDEO_START( kaypro )
	MDRV_VIDEO_UPDATE( kaypro2x )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", kaypro2x, "com,cpm", 3)
	MDRV_WD1793_ADD("wd1793", kaypro_wd1793_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_Z80SIO_ADD( "z80sio", 4800, kaypro_sio_intf )
	MDRV_Z80SIO_ADD( "z80sio_2x", 4800, kaypro_sio_intf )	/* extra sio for modem and printer */

	MDRV_FLOPPY_2_DRIVES_ADD(kaypro2x_floppy_config)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( omni2 )
	MDRV_IMPORT_FROM( kaypro4 )
	MDRV_VIDEO_UPDATE( omni2 )
MACHINE_DRIVER_END

/***********************************************************

    Game driver

************************************************************/

/* The detested bios "universal rom" is part number 81-478 */

ROM_START(kayproii)
	/* The board could take a 2716 or 2732 */
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-149.u47",   0x0000, 0x0800, CRC(28264bc1) SHA1(a12afb11a538fc0217e569bc29633d5270dfa51b) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("81-146.u43",   0x0000, 0x0800, CRC(4cc7d206) SHA1(5cb880083b94bd8220aac1f87d537db7cfeb9013) )
ROM_END

ROM_START(kaypro4)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-232.u47",   0x0000, 0x1000, CRC(4918fb91) SHA1(cd9f45cc3546bcaad7254b92c5d501c40e2ef0b2) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("81-146.u43",   0x0000, 0x0800, CRC(4cc7d206) SHA1(5cb880083b94bd8220aac1f87d537db7cfeb9013) )
ROM_END

ROM_START(kaypro4p88) // "KAYPRO-88" board has 128k or 256k of its own ram on it
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-232.u47",   0x0000, 0x1000, CRC(4918fb91) SHA1(cd9f45cc3546bcaad7254b92c5d501c40e2ef0b2) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("81-146.u43",   0x0000, 0x0800, CRC(4cc7d206) SHA1(5cb880083b94bd8220aac1f87d537db7cfeb9013) )

	ROM_REGION(0x1000, "8088cpu",0)
	ROM_LOAD("81-356.u29",   0x0000, 0x1000, CRC(948556db) SHA1(6e779866d099cc0dc8c6369bdfb37a923ac448a4) )
ROM_END

ROM_START(omni2)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("omni2.u47",    0x0000, 0x1000, CRC(2883f9e0) SHA1(d98c784e62853582d298bf7ca84c75872847ac9b) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD("omni2.u43",    0x0000, 0x0800, CRC(049b3381) SHA1(46f1d4f038747ba9048b075dc617361be088f82a) )
ROM_END

ROM_START(kaypro2x)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-292.u34",   0x0000, 0x2000, CRC(5eb69aec) SHA1(525f955ca002976e2e30ac7ee37e4a54f279fe96) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD("81-817.u9",    0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7) )
ROM_END

ROM_START(kaypro4a)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-292.u34",   0x0000, 0x2000, CRC(5eb69aec) SHA1(525f955ca002976e2e30ac7ee37e4a54f279fe96) )

	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD("81-817.u9",    0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7) )
ROM_END

ROM_START(kaypro10)
	ROM_REGION(0x4000, "maincpu",0)
	ROM_LOAD("81-302.u42",   0x0000, 0x1000, CRC(3f9bee20) SHA1(b29114a199e70afe46511119b77a662e97b093a0) )
//Note: the older rom 81-187 is also allowed here for kaypro 10, but we don't have a dump of it yet.
	ROM_REGION(0x10000, "rambank",0)
	ROM_FILL( 0, 0x10000, 0xff)

	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD("81-817.u31",   0x0000, 0x1000, CRC(5f72da5b) SHA1(8a597000cce1a7e184abfb7bebcb564c6bf24fb7) )
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT    INIT    COMPANY  FULLNAME */
COMP( 1982, kayproii,   0,        0,    kayproii, kay_kbd, 0,      "Non Linear Systems",  "Kaypro II - 2/83" , 0 )
COMP( 1983, kaypro4,    kayproii, 0,    kaypro4,  kay_kbd, 0,      "Non Linear Systems",  "Kaypro 4 - 4/83" , 0 ) // model 81-004
COMP( 1983, kaypro4p88, kayproii, 0,    kaypro4,  kay_kbd, 0,      "Non Linear Systems",  "Kaypro 4 plus88 - 4/83" , GAME_NOT_WORKING ) // model 81-004 with an added 8088 daughterboard and rom
COMP( 198?, omni2,      kayproii, 0,    omni2,    kay_kbd, 0,      "Non Linear Systems",  "Omni II" , 0 )
COMP( 1984, kaypro2x,   0,        0,    kaypro2x, kay_kbd, 0,      "Non Linear Systems",  "Kaypro 2x" , GAME_NOT_WORKING ) // model 81-025
COMP( 1984, kaypro4a,   0,        0,    kaypro2x, kay_kbd, 0,      "Non Linear Systems",  "Kaypro 4 - 4/84" , GAME_NOT_WORKING ) // model 81-015
// Kaypro 4/84 plus 88 goes here, model 81-015 with an added 8088 daughterboard and rom
COMP( 1983, kaypro10,   0,        0,    kaypro2x, kay_kbd, 0,      "Non Linear Systems",  "Kaypro 10" , 0 ) // model 81-005
