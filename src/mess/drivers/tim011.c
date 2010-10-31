/***************************************************************************
   
        TIM-011

        04/09/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z180/z180.h"

static ADDRESS_MAP_START(tim011_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x01fff) AM_ROM 
	AM_RANGE(0x40000, 0x7ffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tim011_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x7f) AM_NOP	/* Z180 internal registers */		
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( tim011 )
INPUT_PORTS_END


static MACHINE_RESET(tim011) 
{	
}

static VIDEO_START( tim011 )
{
}

static VIDEO_UPDATE( tim011 )
{
    return 0;
}

static MACHINE_CONFIG_START( tim011,driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z180, XTAL_12_288MHz / 2)
    MDRV_CPU_PROGRAM_MAP(tim011_mem)
    MDRV_CPU_IO_MAP(tim011_io)	

    MDRV_MACHINE_RESET(tim011)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(512, 256)
    MDRV_SCREEN_VISIBLE_AREA(0, 512-1, 0, 256-1)
    MDRV_PALETTE_LENGTH(4)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(tim011)
    MDRV_VIDEO_UPDATE(tim011)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( tim011 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "sys_tim011.bin", 0x0000, 0x2000, CRC(5b4f1300) SHA1(d324991c4292d7dcde8b8d183a57458be8a2be7b))
	ROM_REGION( 0x10000, "keyboard", ROMREGION_ERASEFF )
	ROM_LOAD( "keyb_tim011.bin", 0x0000, 0x1000, CRC(a99c40a6) SHA1(d6d505271d91df4e079ec3c0a4abbe75ae9d649b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1987, tim011,  0,       0, 	tim011, 	tim011, 	 0,  "Mihajlo Pupin Institute",   "TIM-011",		GAME_NOT_WORKING | GAME_NO_SOUND)

