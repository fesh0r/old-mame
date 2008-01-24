/*
Funai / Gakken Esh's Aurunmilla laserdisc hardware
Driver by Andrew Gardner with help from Daphne Source

Notes:
    (dumper's note) Esh's Aurunmilla can be played in an Interstellar cabinet by swapping the
                    main pcb and the laserdisc. Sound & video pcbs are the same. Control panel is different however.
                    Esh's Aurunmilla can be a 2 player game, while Interstellar is a 1 player only game.
    Hold down TEST while resetting the machine - pops up test mode where...
    The DIPs are software-controlled.
    Two joysticks appear in the IO TEST, but the photos of the control panel I've seen show only 1.
    Eshb has some junk in the IO TEST screen.  Maybe a bad dump?

Todo:
    - LD TROUBLE message pops up after each cycle in attract.  NMI-related?
    - Convert to tilemaps (see next ToDo for feasibility).
    - Apparently some tiles blink (in at least two different ways).
    - 0xfe and 0xff are pretty obviously not NMI enables.  They're likely LED's.  Do the NMI right (somehow).
    - Rumor has it there's an analog beep hanging off 0xf5?  Implement it and finish off 0xf5 bits.
    - NVRAM range 0xe000-0xe800 might be too large.  It doesn't seem to write past 0xe600...
    - Maybe some of the IPT_UNKNOWNs do something?
    - Hook up LED's to the MAME lamp system.
*/

#include "driver.h"
#include "render.h"
#include "machine/laserdsc.h"

/* From daphne */
#define PCB_CLOCK (18432000)


/* Misc variables */
static laserdisc_info *discinfo;

static UINT8 *tile_ram;
static UINT8 *tile_control_ram;

static UINT8 ld_video_visible;

/* VIDEO GOODS */
static VIDEO_UPDATE( esh )
{
	int charx, chary;

	/* clear */
	fillbitmap(bitmap, 0, cliprect);

	/* display disc information */
	if (discinfo != NULL && ld_video_visible)
		popmessage("%s", laserdisc_describe_state(discinfo));

	/* Draw tiles */
	for (charx = 0; charx < 32; charx++)
	{
		for (chary = 0; chary < 32; chary++)
		{
			int current_screen_character = (chary*32) + charx;

			int palIndex  = (tile_control_ram[current_screen_character] & 0x0f);
			int tileOffs  = (tile_control_ram[current_screen_character] & 0x10) >> 4;
			//int blinkLine = (tile_control_ram[current_screen_character] & 0x40) >> 6;
			//int blinkChar = (tile_control_ram[current_screen_character] & 0x80) >> 7;

			drawgfx(bitmap, machine->gfx[0],
					tile_ram[current_screen_character] + (0x100 * tileOffs),
					palIndex,
					0, 0, charx*8, chary*8, cliprect, TRANSPARENCY_PEN, 0);
		}
	}

	/* Draw sprites */
	return 0;
}



/* MEMORY HANDLERS */
static READ8_HANDLER(ldp_read)
{
	return laserdisc_data_r(discinfo);
}

static WRITE8_HANDLER(ldp_write)
{
	laserdisc_data_w(discinfo,data);
}

static WRITE8_HANDLER(misc_write)
{
	/* Bit 0 unknown */

	if (data & 0x02)
		logerror("BEEP!\n");

	/* Bit 2 unknown */
	ld_video_visible = !((data & 0x08) >> 3);

	/* Bits 4-7 unknown */
	/* They cycle through a repeating pattern though */
}

static WRITE8_HANDLER(led_writes)
{
	switch(offset)
	{
	case 0x00:
		logerror("WRITING 0x%x to P1's START LED\n", data);
		break;
	case 0x01:
		logerror("WRITING 0x%x to P2's START LED\n", data);
		break;
	case 0x02:
		logerror("WRITING 0x%x to P1's BUTTON1 LED\n", data);
		break;
	case 0x03:
		logerror("WRITING 0x%x to P1's BUTTON2 LED\n", data);
		break;
	case 0x04:
		logerror("WRITING 0x%x to P2's BUTTON1 LED\n", data);
		break;
	case 0x05:
		logerror("WRITING 0x%x to P2's BUTTON2 LED\n", data);
		break;
	case 0x06:
		/* Likely coming soon */
		break;
	case 0x07:
		/* Likely coming soon */
		break;
	}
}

static WRITE8_HANDLER(nmi_line_w)
{
	if (data == 0x00)
		cpunum_set_input_line(Machine, 0, INPUT_LINE_NMI, ASSERT_LINE);
	if (data == 0x01)
		cpunum_set_input_line(Machine, 0, INPUT_LINE_NMI, CLEAR_LINE);

	if (data != 0x00 && data != 0x01)
		logerror("NMI line got a weird value!\n");
}


/* PROGRAM MAPS */
static ADDRESS_MAP_START( z80_0_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000,0x3fff) AM_ROM
	AM_RANGE(0xe000,0xe7ff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf000,0xf3ff) AM_RAM AM_BASE(&tile_ram)
	AM_RANGE(0xf400,0xf7ff) AM_RAM AM_BASE(&tile_control_ram)
ADDRESS_MAP_END


/* IO MAPS */
static ADDRESS_MAP_START( z80_0_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xf0,0xf0) AM_READ_PORT("IN0")
	AM_RANGE(0xf1,0xf1) AM_READ_PORT("IN1")
	AM_RANGE(0xf2,0xf2) AM_READ_PORT("IN2")
	AM_RANGE(0xf3,0xf3) AM_READ_PORT("IN3")
	AM_RANGE(0xf4,0xf4) AM_READWRITE(ldp_read,ldp_write)
	AM_RANGE(0xf5,0xf5) AM_WRITE(misc_write)	/* Continuously writes repeating patterns */
	AM_RANGE(0xf8,0xfd) AM_WRITE(led_writes)
	AM_RANGE(0xfe,0xfe) AM_WRITE(nmi_line_w)	/* Both 0xfe and 0xff flip quickly between 0 and 1 */
	AM_RANGE(0xff,0xff) AM_NOP					/*   (they're probably not NMI enables - likely LED's like their neighbors :) */
ADDRESS_MAP_END									/*   (someday 0xf8-0xff will probably be a single handler) */


/* PORTS */
static INPUT_PORTS_START( esh )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME( "TEST" ) PORT_CODE( KEYCODE_T )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

static PALETTE_INIT( esh )
{
	int i;

	/* Oddly enough, the top 4 bits of each byte is 0 */
	for (i = 0; i < machine->drv->total_colors; i++)
	{
		int r,g,b;
		int bit0,bit1,bit2;

		/* Presumably resistor values would help here */

		/* red component */
		bit0 = (color_prom[i+0x100] >> 0) & 0x01;
		bit1 = (color_prom[i+0x100] >> 1) & 0x01;
		bit2 = (color_prom[i+0x100] >> 2) & 0x01;
		r = (0x97 * bit2) + (0x47 * bit1) + (0x21 * bit0);

		/* green component */
		bit0 = 0;
		bit1 = (color_prom[i+0x100] >> 3) & 0x01;
		bit2 = (color_prom[i+0x100] >> 4) & 0x01;
		g = (0x97 * bit2) + (0x47 * bit1) + (0x21 * bit0);

		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i+0x100] >> 5) & 0x01;
		bit2 = (color_prom[i+0x100] >> 6) & 0x01;
		b = (0x97 * bit2) + (0x47 * bit1) + (0x21 * bit0);

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
	}
}

static const gfx_layout esh_gfx_layout =
{
	8,8,
	0x1000/8,
	3,
	{ 0, 0x1000*8, 0x2000*8 },
	{ 0,1,2,3,4,5,6,7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static GFXDECODE_START( esh )
	GFXDECODE_ENTRY(REGION_GFX1, 0, esh_gfx_layout, 0x0, 0x100)
GFXDECODE_END

static MACHINE_START( esh )
{
	discinfo = laserdisc_init(LASERDISC_TYPE_LDV1000, get_disk_handle(0), 0);
	return;
}

static TIMER_CALLBACK( irq_stop )
{
	cpunum_set_input_line(machine, 0, 0, CLEAR_LINE);
}

static INTERRUPT_GEN( vblank_callback_esh )
{
	// IRQ
	cpunum_set_input_line(machine, 0, 0, ASSERT_LINE);
	timer_set(ATTOTIME_IN_USEC(50), NULL, 0, irq_stop);

	laserdisc_vsync(discinfo);
}


/* DRIVER */
static MACHINE_DRIVER_START( esh )
/*  main cpu */
	MDRV_CPU_ADD(Z80, PCB_CLOCK/6)						/* The denominator is a Daphne guess based on PacMan's hardware */
	MDRV_CPU_PROGRAM_MAP(z80_0_mem,0)
	MDRV_CPU_IO_MAP(z80_0_io,0)
	MDRV_CPU_VBLANK_INT(vblank_callback_esh, 1)

	MDRV_MACHINE_START(esh)
	MDRV_NVRAM_HANDLER(generic_0fill)

/*  video */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)

	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(esh)

	MDRV_GFXDECODE(esh)
	MDRV_VIDEO_UPDATE(esh)

/*  sound */
MACHINE_DRIVER_END


ROM_START( esh )
	/* Main program CPU */
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	ROM_LOAD( "is1.h8", 0x0000, 0x2000, CRC(114c912b) SHA1(7c033a102d046199f3e2c6787579dac5b5295d50) )
	ROM_LOAD( "is2.f8", 0x2000, 0x2000, CRC(0e3b6e62) SHA1(5e8160180e20705e727329f9d70305fcde176a25) )

	/* Tiles */
	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a.m3", 0x0000, 0x1000, CRC(a04736d8) SHA1(3b642b5d7168cf4a09328eee54c532be815d2bcf) )
	ROM_LOAD( "b.l3", 0x1000, 0x1000, CRC(9366dde7) SHA1(891db65384d47d13355b2eea37f57c34bc775c8f) )
	ROM_LOAD( "c.k3", 0x2000, 0x1000, CRC(a936ef01) SHA1(bcacb281ccb72ceb57fb6a79380cc3a9688743c4) )

	/* Color (+other) PROMs */
	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "rgb.j1", 0x000, 0x200, CRC(1e9f795f) SHA1(61a58694929fa39b2412bc9244e5681d65a0eacb) )
	ROM_LOAD( "h.c5",   0x200, 0x100, CRC(abde5e4b) SHA1(9dd3a7fd523b519ac613b9f08ae9cc962992cf5d) )	/* Video timing? */
	ROM_LOAD( "v.c6",   0x300, 0x100, CRC(7157ba22) SHA1(07355f30efe46196d216356eda48a59fc622e43f) )
ROM_END

ROM_START( esha )
	/* Main program CPU */
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	ROM_LOAD( "is1.h8", 0x0000, 0x2000, CRC(114c912b) SHA1(7c033a102d046199f3e2c6787579dac5b5295d50) )
	ROM_LOAD( "is2.f8", 0x2000, 0x2000, CRC(7a562f49) SHA1(acfa49b3b3d96b001a5dbdee39cbb0ca80be1763) )

	/* Tiles */
	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a.m3", 0x0000, 0x1000, CRC(a04736d8) SHA1(3b642b5d7168cf4a09328eee54c532be815d2bcf) )
	ROM_LOAD( "b.l3", 0x1000, 0x1000, CRC(9366dde7) SHA1(891db65384d47d13355b2eea37f57c34bc775c8f) )
	ROM_LOAD( "c.k3", 0x2000, 0x1000, CRC(a936ef01) SHA1(bcacb281ccb72ceb57fb6a79380cc3a9688743c4) )

	/* Color (+other) PROMs */
	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "rgb.j1", 0x000, 0x200, CRC(1e9f795f) SHA1(61a58694929fa39b2412bc9244e5681d65a0eacb) )
	ROM_LOAD( "h.c5",   0x200, 0x100, CRC(abde5e4b) SHA1(9dd3a7fd523b519ac613b9f08ae9cc962992cf5d) )	/* Video timing? */
	ROM_LOAD( "v.c6",   0x300, 0x100, CRC(7157ba22) SHA1(07355f30efe46196d216356eda48a59fc622e43f) )
ROM_END

ROM_START( eshb )
	/* Main program CPU */
	ROM_REGION( 0x4000, REGION_CPU1, 0 )
	ROM_LOAD( "1.h8",   0x0000, 0x2000, CRC(8d27d363) SHA1(529d8e4283e736edb5a9193df1ed8d0164471864) )	/* Hand-written ROM label */
	ROM_LOAD( "is2.f8", 0x2000, 0x2000, CRC(0e3b6e62) SHA1(5e8160180e20705e727329f9d70305fcde176a25) )

	/* Tiles */
	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "a.m3", 0x0000, 0x1000, CRC(a04736d8) SHA1(3b642b5d7168cf4a09328eee54c532be815d2bcf) )
	ROM_LOAD( "b.l3", 0x1000, 0x1000, CRC(9366dde7) SHA1(891db65384d47d13355b2eea37f57c34bc775c8f) )
	ROM_LOAD( "c.k3", 0x2000, 0x1000, CRC(a936ef01) SHA1(bcacb281ccb72ceb57fb6a79380cc3a9688743c4) )

	/* Color (+other) PROMs */
	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "rgb.j1", 0x000, 0x200, CRC(1e9f795f) SHA1(61a58694929fa39b2412bc9244e5681d65a0eacb) )
	ROM_LOAD( "h.c5",   0x200, 0x100, CRC(abde5e4b) SHA1(9dd3a7fd523b519ac613b9f08ae9cc962992cf5d) )	/* Video timing? */
	ROM_LOAD( "v.c6",   0x300, 0x100, CRC(7157ba22) SHA1(07355f30efe46196d216356eda48a59fc622e43f) )
ROM_END


static DRIVER_INIT( esh )
{
}

/*    YEAR  NAME  PARENT   MACHINE  INPUT  INIT  MONITOR  COMPANY          FULLNAME                     FLAGS) */
GAME( 1983, esh,  0,       esh,     esh,   esh,  ROT0,    "Funai/Gakken",  "Esh's Aurunmilla",          GAME_NOT_WORKING|GAME_NO_SOUND)
GAME( 1983, esha, 0,       esh,     esh,   esh,  ROT0,    "Funai/Gakken",  "Esh's Aurunmilla (Set 2)",  GAME_NOT_WORKING|GAME_NO_SOUND)
GAME( 1983, eshb, 0,       esh,     esh,   esh,  ROT0,    "Funai/Gakken",  "Esh's Aurunmilla (Set 3)",  GAME_NOT_WORKING|GAME_NO_SOUND)
