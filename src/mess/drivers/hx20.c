/***************************************************************************

        Epson HX20

        29/09/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"

static ADDRESS_MAP_START(ehx20_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_RAM // I/O
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ehx20 )
INPUT_PORTS_END


static MACHINE_RESET(ehx20)
{
}

static VIDEO_START( ehx20 )
{
}

static VIDEO_UPDATE( ehx20 )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout hx20_1_charlayout =
{
	5, 8,					/* 5 x 8 characters */
	102,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 8, 16, 24, 32 },
	/* y offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	5*8					/* every char is 5 bits of 8 bytes */
};

static const gfx_layout hx20_2_charlayout =
{
	6, 8,					/* 6 x 8 characters */
	27,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 8, 16, 24, 32, 40 },
	/* y offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	6*8					/* every char is 6 bits of 8 bytes */
};

static const gfx_layout hx20_3_charlayout =
{
	5, 8,					/* 5 x 8 characters */
	34,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 16, 24, 32, 40, 48 },
	/* y offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	5*8					/* every char is 5 bits of 8 bytes */
};

static GFXDECODE_START( hx20 )
	GFXDECODE_ENTRY( "maincpu", 0xfb82, hx20_1_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0xfd80, hx20_2_charlayout, 0, 1 )
	GFXDECODE_ENTRY( "maincpu", 0xfe20, hx20_3_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_DRIVER_START( ehx20 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",HD63701, 614000) // HD6301
    MDRV_CPU_PROGRAM_MAP(ehx20_mem)

    MDRV_MACHINE_RESET(ehx20)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_GFXDECODE(hx20)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(ehx20)
    MDRV_VIDEO_UPDATE(ehx20)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( ehx20 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hx20.rom", 0x8000, 0x8000, CRC(ef84dce4) SHA1(84e259ca537f163e2faafb4adf3fec766590e1b4))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, ehx20,  0,       0, 	ehx20, 	ehx20, 	 0,  	  	 "Epson",   "HX20",		GAME_NOT_WORKING)

