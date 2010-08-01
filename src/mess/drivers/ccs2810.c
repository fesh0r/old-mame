/***************************************************************************

        CCS Model 2810

        11/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ins8250.h"

static ADDRESS_MAP_START(ccs2810_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xefff) AM_RAM
	AM_RANGE(0xf000, 0xf7ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ccs2810_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x20, 0x27) AM_DEVREADWRITE("ins8250", ins8250_r, ins8250_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ccs2810 )
INPUT_PORTS_END


static MACHINE_RESET(ccs2810)
{
	cpu_set_reg(machine->device("maincpu"), Z80_PC, 0xf000);
}

static VIDEO_START( ccs2810 )
{
}

static VIDEO_UPDATE( ccs2810 )
{
    return 0;
}

static const ins8250_interface ccs2810_com_interface =
{
	1843200,
	NULL,
	NULL,
	NULL,
	NULL
};

static MACHINE_DRIVER_START( ccs2810 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(ccs2810_mem)
    MDRV_CPU_IO_MAP(ccs2810_io)

    MDRV_MACHINE_RESET(ccs2810)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ccs2810)
    MDRV_VIDEO_UPDATE(ccs2810)

	MDRV_INS8250_ADD( "ins8250", ccs2810_com_interface )
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( ccs2810 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ccs2810_u8.bin", 0xf000, 0x0800, CRC(0c3054ea) SHA1(c554b7c44a61af13decb2785f3c9b33c6fc2bfce))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1980, ccs2810,  0,       0,	ccs2810,	ccs2810,	 0,   "California Computer Systems",   "CCS Model 2810",		GAME_NOT_WORKING | GAME_NO_SOUND)

