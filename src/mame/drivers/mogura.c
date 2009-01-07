/* Mogura Desse */

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/dac.h"

static UINT8 *mogura_tileram;
static UINT8* mogura_gfxram;
static tilemap *mogura_tilemap;

static PALETTE_INIT( mogura )
{
	int i,j;

	j = 0;
	for (i = 0;i < 0x20;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(machine,j,MAKE_RGB(r,g,b));
		j+=4;
		if (j>31) j-=31;
	}
}


static TILE_GET_INFO( get_mogura_tile_info )
{
	int code = mogura_tileram[tile_index];
	int attr = mogura_tileram[tile_index+0x800];

	SET_TILE_INFO(
			0,
			code,
			(attr>>1)&7,
			0);
}


static VIDEO_START( mogura )
{
	mogura_tilemap = tilemap_create(machine, get_mogura_tile_info,tilemap_scan_rows,8,8,64, 32);
}

static VIDEO_UPDATE( mogura )
{
	const rectangle *visarea = video_screen_get_visible_area(screen);

	/* tilemap layout is a bit strange ... */
	rectangle clip;
	clip.min_x = visarea->min_x;
	clip.max_x = 256-1;
	clip.min_y = visarea->min_y;
	clip.max_y = visarea->max_y;
	tilemap_set_scrollx(mogura_tilemap,0, 256);
	tilemap_draw(bitmap,&clip,mogura_tilemap,0,0);

	clip.min_x = 256;
	clip.max_x = 512-1;
	clip.min_y = visarea->min_y;
	clip.max_y = visarea->max_y;
	tilemap_set_scrollx(mogura_tilemap,0, -128);
	tilemap_draw(bitmap,&clip,mogura_tilemap,0,0);

	return 0;
}

static WRITE8_HANDLER( mogura_tileram_w )
{
	mogura_tileram[offset] = data;
	tilemap_mark_tile_dirty(mogura_tilemap,offset&0x7ff);
}

static WRITE8_HANDLER(dac_w)
{
	dac_0_data_w(space, 0, data & 0xf0 );	/* left */
	dac_1_data_w(space, 0, (data & 0x0f)<<4 );	/* right */
}

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_WRITE(SMH_NOP)	// ??
	AM_RANGE(0x08, 0x08) AM_READ_PORT("SYSTEM")
	AM_RANGE(0x0c, 0x0c) AM_READ_PORT("P1")
	AM_RANGE(0x0d, 0x0d) AM_READ_PORT("P2")
	AM_RANGE(0x0e, 0x0e) AM_READ_PORT("P3")
	AM_RANGE(0x0f, 0x0f) AM_READ_PORT("P4")
	AM_RANGE(0x10, 0x10) AM_READ_PORT("SERVICE")
	AM_RANGE(0x14, 0x14) AM_WRITE(dac_w)	/* 4 bit DAC x 2. MSB = left, LSB = right */
ADDRESS_MAP_END


static WRITE8_HANDLER ( mogura_gfxram_w )
{
	mogura_gfxram[offset] = data ;

	decodechar(space->machine->gfx[0], offset/16, mogura_gfxram);

	tilemap_mark_all_tiles_dirty(mogura_tilemap);
}


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(SMH_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_READ(SMH_RAM) // main ram
	AM_RANGE(0xe000, 0xefff) AM_READ(SMH_RAM) // ram based characters
	AM_RANGE(0xf000, 0xffff) AM_READ(SMH_RAM) // tilemap
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(SMH_ROM)
	AM_RANGE(0xc000, 0xdfff) AM_WRITE(SMH_RAM)
	AM_RANGE(0xe000, 0xefff) AM_WRITE(mogura_gfxram_w) AM_BASE(&mogura_gfxram)
	AM_RANGE(0xf000, 0xffff) AM_WRITE(mogura_tileram_w) AM_BASE(&mogura_tileram)
ADDRESS_MAP_END


static INPUT_PORTS_START( mogura )
	PORT_START("SYSTEM")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE4 )

	PORT_START("P1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START("P2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START("P3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START("P4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3) PORT_PLAYER(4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START("SERVICE")
	PORT_SERVICE_NO_TOGGLE( 0x01, IP_ACTIVE_LOW)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static const gfx_layout tiles8x8_layout =
{
	8,8,
	0x1000*8/(16*8),
	2,
	{ 0, 1 },
	{ 0, 2, 4, 6, 8, 10, 12, 14 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static GFXDECODE_START( mogura )
	GFXDECODE_ENTRY( NULL, 0, tiles8x8_layout, 0, 8 )
GFXDECODE_END

static MACHINE_DRIVER_START( mogura )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80,3000000)		 /* 3 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_IO_MAP(io_map,0)
	MDRV_CPU_VBLANK_INT("main", irq0_line_hold)

	MDRV_GFXDECODE(mogura)

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60) // ?
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 256-1)

	MDRV_PALETTE_LENGTH(32)

	MDRV_PALETTE_INIT(mogura)
	MDRV_VIDEO_START(mogura)
	MDRV_VIDEO_UPDATE(mogura)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD("dac1", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.50)

	MDRV_SOUND_ADD("dac2", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.50)
MACHINE_DRIVER_END

ROM_START( mogura )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "gx141.5n", 0x00000, 0x08000, CRC(98e6120d) SHA1(45cdb2d78224a7c44fff8cd3487f33c57669a06c)  )

	ROM_REGION( 0x20, "proms", 0 )
	ROM_LOAD( "gx141.7j", 0x00, 0x20,  CRC(b21c5d5f) SHA1(6913c840dd69a7d4687f4c4cbe3ff12300f62bc2) )
ROM_END

GAME( 1991, mogura, 0, mogura, mogura, 0, ROT0, "Konami", "Mogura Desse", 0 )

