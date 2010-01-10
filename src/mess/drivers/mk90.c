/***************************************************************************

        Elektronika MK-90

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/t11/t11.h"

static ADDRESS_MAP_START(mk90_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x3fff ) AM_RAM // RAM
	AM_RANGE( 0x4000, 0x7fff ) AM_ROM // Extension ROM
	AM_RANGE( 0x8000, 0xffff ) AM_ROM // Main ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( mk90 )
INPUT_PORTS_END


static MACHINE_RESET(mk90)
{
}

static VIDEO_START( mk90 )
{
}

static VIDEO_UPDATE( mk90 )
{
    return 0;
}

static const struct t11_setup t11_data =
{
	1 << 13			/* start from 8000 */
};

static MACHINE_DRIVER_START( mk90 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",T11, XTAL_4MHz)
    MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(mk90_mem)

    MDRV_MACHINE_RESET(mk90)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mk90)
    MDRV_VIDEO_UPDATE(mk90)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( mk90 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "bas1", "Basic 1")
    ROMX_LOAD( "mk90ro10.bin",  0x8000, 0x8000, CRC(fac18038) SHA1(639f09a1be5f781f897603d0f799f7c6efd1b67f), ROM_BIOS(1))
    ROM_SYSTEM_BIOS(1, "bas2", "Basic 2")
    ROMX_LOAD( "mk90ro20.bin",  0x8000, 0x8000, CRC(d8b3a5f5) SHA1(8f7ab2d97c7466392b6354c0ea7017531c2133ae), ROM_BIOS(2))
    ROMX_LOAD( "mk90ro20t.bin", 0x4000, 0x4000, CRC(0f4b9434) SHA1(c74bbde6d201913c9e67ef8e2abe14b784187f8d), ROM_BIOS(2))	// Expansion ROM
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1988, mk90,  0,       0, 			mk90, 	mk90, 	 0,  	 "Elektronika",   "MK-90",		GAME_NOT_WORKING)

