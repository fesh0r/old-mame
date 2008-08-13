/* Galxaia

Galaxia by Zaccaria (1979)

Taken from an untested board.

1K byte files were 2708 or equivalent.
512 byte file is the 82S130 colour PROM.

This is not a direct pirate of Galaxians as you might think from the name.
The game uses a Signetics 2650A CPU with three 40-pin 2636 chips. I have
no idea what 2636's are but I am hoping they are something to do with the
sound since the board has no apparent sound circuitry. The video hardware
looks like it's similar to Galaxians (2 x 2114, 2 x 2101, 2 x EPROM) but
there is no attack RAM and the graphics EPROMS are 2708. The graphics EPROMS
do contain Galaxian-like graphics...

---

TS 2008.08.12:
- fixed rom loading
- added preliminary video emulation

*/

#include "driver.h"
#include "video/s2636.h"
#include "cpu/s2650/s2650.h"

static UINT8 *galaxia_video;
static UINT8 *galaxia_color;

static s2636_t *s2636_0, *s2636_1, *s2636_2;
static UINT8 *galaxia_s2636_0_ram;
static UINT8 *galaxia_s2636_1_ram;
static UINT8 *galaxia_s2636_2_ram;

static VIDEO_START( galaxia )
{
	int width = video_screen_get_width(machine->primary_screen);
	int height = video_screen_get_height(machine->primary_screen);

	/* configure the S2636 chips */
	s2636_0 = s2636_config(galaxia_s2636_0_ram, height, width,  3, -27);
	s2636_1 = s2636_config(galaxia_s2636_1_ram, height, width,  3, -27);
	s2636_2 = s2636_config(galaxia_s2636_1_ram, height, width,  3, -27);
}

static VIDEO_UPDATE( galaxia )
{
	int x,y, count;

	bitmap_t *s2636_0_bitmap;
	bitmap_t *s2636_1_bitmap;
	bitmap_t *s2636_2_bitmap;

	count = 0;

	for (y=0;y<256/8;y++)
	{
		for (x=0;x<256/8;x++)
		{
			int tile = galaxia_video[count];
			drawgfx(bitmap,screen->machine->gfx[0],tile,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_NONE,0);
			count++;
		}
	}

	s2636_0_bitmap = s2636_update(s2636_0, cliprect);
	s2636_1_bitmap = s2636_update(s2636_1, cliprect);
	s2636_2_bitmap = s2636_update(s2636_2, cliprect);

	/* copy the S2636 images into the main bitmap */
	{
		int y;

		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			int x;

			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int pixel0 = *BITMAP_ADDR16(s2636_0_bitmap, y, x);
				int pixel1 = *BITMAP_ADDR16(s2636_1_bitmap, y, x);
				int pixel2 = *BITMAP_ADDR16(s2636_2_bitmap, y, x);

				if (S2636_IS_PIXEL_DRAWN(pixel0))
					*BITMAP_ADDR16(bitmap, y, x) = S2636_PIXEL_COLOR(pixel0);

				if (S2636_IS_PIXEL_DRAWN(pixel1))
					*BITMAP_ADDR16(bitmap, y, x) = S2636_PIXEL_COLOR(pixel1);

				if (S2636_IS_PIXEL_DRAWN(pixel2))
					*BITMAP_ADDR16(bitmap, y, x) = S2636_PIXEL_COLOR(pixel2);
			}
		}
	}
	return 0;
}

static WRITE8_HANDLER(galaxia_video_w)
{
	if (activecpu_get_reg(S2650_FO))
	{
		galaxia_video[offset]=data;
	}
	else
	{
		galaxia_color[offset]=data;
	}
}

static READ8_HANDLER(galaxia_video_r)
{
	return galaxia_video[offset];
}

static ADDRESS_MAP_START( mem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x13ff) AM_ROM
	AM_RANGE(0x1400, 0x14ff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x1500, 0x15ff) AM_MIRROR(0x6000) AM_RAM AM_BASE(&galaxia_s2636_0_ram)
	AM_RANGE(0x1600, 0x16ff) AM_MIRROR(0x6000) AM_RAM AM_BASE(&galaxia_s2636_1_ram)
	AM_RANGE(0x1700, 0x17ff) AM_MIRROR(0x6000) AM_RAM AM_BASE(&galaxia_s2636_2_ram)
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0x6000) AM_READWRITE(galaxia_video_r, galaxia_video_w)  AM_BASE(&galaxia_video)
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0x6000) AM_RAM
	AM_RANGE(0x2000, 0x33ff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x02, 0x02) AM_READ_PORT("IN0")
ADDRESS_MAP_END

static INPUT_PORTS_START( galaxia )
	PORT_START("IN0")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( galaxia )
	GFXDECODE_ENTRY( "gfx1", 0, tiles8x8_layout, 0, 16 )
GFXDECODE_END


static INTERRUPT_GEN( galaxia_interrupt )
{
	cpunum_set_input_line_and_vector(machine, 0, 0, HOLD_LINE, 0x03);
}

static MACHINE_DRIVER_START( galaxia )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", S2650,2000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(mem_map, 0)
	MDRV_CPU_IO_MAP(io_map, 0)
	MDRV_CPU_VBLANK_INT("main", galaxia_interrupt)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 0, 256-1)

	MDRV_GFXDECODE(galaxia)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(galaxia)
	MDRV_VIDEO_UPDATE(galaxia)
MACHINE_DRIVER_END


ROM_START( galaxia )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "08h.bin", 0x00000, 0x0400, CRC(f3b4ffde) SHA1(15b004e7821bfc145158b1e9435f061c524f6b86) )
	ROM_LOAD( "10h.bin", 0x00400, 0x0400, CRC(6d07fdd4) SHA1(d7d4b345a055275d59951788569db370bccd5195) )
	ROM_LOAD( "11h.bin", 0x00800, 0x0400, CRC(1520eb3d) SHA1(3683174da701e1124af0f9c2ee4a9a84f3fea33a) )
	ROM_LOAD( "13h.bin", 0x00c00, 0x0400, CRC(c4482770) SHA1(aee983cc3d80989f49aea4138961bb623039484a) )
	ROM_LOAD( "08i.bin", 0x01000, 0x0400, CRC(45b88599) SHA1(3b79c21db1aa9d80fac81ac5a554e438805febd1) )
	ROM_LOAD( "10i.bin", 0x02000, 0x0400, CRC(c0baa654) SHA1(80e0880c32ad285fbce0f7f552268b964b97cab3) )
	ROM_LOAD( "11i.bin", 0x02400, 0x0400, CRC(4456808a) SHA1(f9e8cfdde0e17f13f1be297b2b4503ccc959b33c) )
	ROM_LOAD( "13i.bin", 0x02800, 0x0400, CRC(cf653b9a) SHA1(fef5943de60cb5ba2459fc6ae7419e29c96a76cd) )
	ROM_LOAD( "11l.bin", 0x02c00, 0x0400, CRC(50c6a645) SHA1(46638907bc393df6be25fc7461d73047d1746ffc) )
	ROM_LOAD( "13l.bin", 0x03000, 0x0400, CRC(3a9c38c7) SHA1(d1e934092b69c0f3f9636eba05a1d8a6d9588e6b) )

	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "01d.bin", 0x00000, 0x0400, CRC(2dd50aab) SHA1(758d7a5383c9a1ee134d99e3f7025819cfbe0e0f) )
	ROM_LOAD( "03d.bin", 0x00400, 0x0400, CRC(1dc30185) SHA1(e3c75eecb80b376ece98f602e1b9587487841824) )

	ROM_REGION( 0x80000, "proms", 0 )
	ROM_LOAD( "11o", 0x00000, 0x0200, CRC(ae816417) SHA1(9497857d13c943a2735c3b85798199054e613b2c) )
ROM_END


static DRIVER_INIT(galaxia)
{
	galaxia_color=auto_malloc(0x400);
}

GAME( 1979, galaxia, 0, galaxia, galaxia, galaxia, ROT90, "Zaccaria", "Galaxia", GAME_NOT_WORKING|GAME_NO_SOUND )
