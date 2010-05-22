/***************************************************************************

        Intel iPB

        17/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(ipb_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe800, 0xefff) AM_ROM
	AM_RANGE(0xf800, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipb_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ipb )
INPUT_PORTS_END


static MACHINE_RESET(ipb)
{
	cpu_set_reg(devtag_get_device(machine, "maincpu"), I8085_PC, 0xE800);
}

static VIDEO_START( ipb )
{
}

static VIDEO_UPDATE( ipb )
{
    return 0;
}

static MACHINE_DRIVER_START( ipb )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8080, XTAL_19_6608MHz / 4)
    MDRV_CPU_PROGRAM_MAP(ipb_mem)
    MDRV_CPU_IO_MAP(ipb_io)

    MDRV_MACHINE_RESET(ipb)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ipb)
    MDRV_VIDEO_UPDATE(ipb)
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( ipb )
    ROM_REGION( 0x100000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipb_e8_v1.3.bin", 0xe800, 0x0800, CRC(fc9d4703) SHA1(2ce078e1bcd8b24217830c54bcf04c5d146d1b76))
	ROM_LOAD( "ipb_f8_v1.3.bin", 0xf800, 0x0800, CRC(966ba421) SHA1(d6a904c7d992a05ed0f451d7d34c1fc8de9547ee))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, ipb,  0,    	 0, 		ipb,	ipb,	 0,		 "Intel",   "iPB",		GAME_NOT_WORKING | GAME_NO_SOUND)

