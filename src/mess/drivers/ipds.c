/***************************************************************************

        Intel iPDS

        17/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "video/i8275.h"

static ADDRESS_MAP_START(ipds_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ipds_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( ipds )
INPUT_PORTS_END


I8275_DISPLAY_PIXELS(ipds_display_pixels)
{
	int i;
	bitmap_t *bitmap = device->machine->generic.tmpbitmap;
	UINT8 *charmap = memory_region(device->machine, "gfx1");
	UINT8 pixels = charmap[(linecount & 7) + (charcode << 3)] ^ 0xff;
	if (vsp) {
		pixels = 0;
	}
	if (lten) {
		pixels = 0xff;
	}
	if (rvv) {
		pixels ^= 0xff;
	}
	for(i=0;i<6;i++) {
		*BITMAP_ADDR16(bitmap, y, x + i) = (pixels >> (5-i)) & 1 ? (hlgt ? 2 : 1) : 0;
	}
}

const i8275_interface ipds_i8275_interface = {
	"screen",
	6,
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	ipds_display_pixels
};

static MACHINE_RESET(ipds)
{
	cpu_set_reg(machine->device("maincpu"), I8085_PC, (UINT64)0x0000);
}

static VIDEO_UPDATE( ipds )
{
    running_device *devconf = screen->machine->device("i8275");
	i8275_update( devconf, bitmap, cliprect);
	VIDEO_UPDATE_CALL ( generic_bitmapped );
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout ipds_charlayout =
{
	8, 11,					/* 8 x 11 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( ipds )
	GFXDECODE_ENTRY( "gfx1", 0x0000, ipds_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( ipds, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8085A, XTAL_19_6608MHz / 4)
    MDRV_CPU_PROGRAM_MAP(ipds_mem)
    MDRV_CPU_IO_MAP(ipds_io)

    MDRV_MACHINE_RESET(ipds)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_GFXDECODE(ipds)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

	MDRV_I8275_ADD	( "i8275", ipds_i8275_interface)

    MDRV_VIDEO_START(generic_bitmapped)
    MDRV_VIDEO_UPDATE(ipds)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( ipds )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "164180.bin", 0x0000, 0x0800, CRC(10ba23ce) SHA1(67a9d03a08cdd7c70a867fb7e069703c7a0b9094))
	ROM_REGION( 0x10000, "slavecpu", ROMREGION_ERASEFF )
	ROM_LOAD( "164196.bin", 0x0000, 0x1000, CRC(d3e18bc6) SHA1(01bc233be154bdfea9e8015839c5885cdaa08f11))
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD( "163642.bin", 0x0000, 0x0800, CRC(3d81b2d1) SHA1(262a42920f15f1156ad0717c876cc0b2ed947ec5))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 1982, ipds,  0,       0,	     ipds,	ipds,	 0, 	"Intel",   "iPDS",	GAME_NOT_WORKING | GAME_NO_SOUND)

