/***************************************************************************

	Atari Dominos hardware

	driver by Mike Balfour

	Games supported:
		* Sprint 1
		* Sprint 2

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
			0000-03FF		RAM
			0400-07FF		DISPLAY RAM
			0800-0BFF	R	SWITCH
			0C00-0FFF	R	SYNC
			0C00-0C0F	W	ATTRACT
			0C10-0C1F	W	TUMBLE
			0C30-0C3F	W	LAMP2
			0C40-0C4F	W	LAMP1
			3000-37FF		Program ROM1
			3800-3FFF		Program ROM2
		   (F800-FFFF)		Program ROM2 - only needed for the 6502 vectors

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "dominos.h"



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x00, 0x01,
	0x00, 0x02
};

static PALETTE_INIT( dominos )
{
	palette_set_color(0,0x80,0x80,0x80); /* LT GREY */
	palette_set_color(1,0x00,0x00,0x00); /* BLACK */
	palette_set_color(2,0xff,0xff,0xff); /* WHITE */
	palette_set_color(3,0x55,0x55,0x55); /* DK GREY */
	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM }, /* RAM */
	{ 0x0400, 0x07ff, MRA_RAM }, /* RAM */
	{ 0x0800, 0x083f, dominos_port_r }, /* SWITCH */
	{ 0x0840, 0x087f, input_port_3_r }, /* SWITCH */
	{ 0x0900, 0x093f, dominos_port_r }, /* SWITCH */
	{ 0x0940, 0x097f, input_port_3_r }, /* SWITCH */
	{ 0x0a00, 0x0a3f, dominos_port_r }, /* SWITCH */
	{ 0x0a40, 0x0a7f, input_port_3_r }, /* SWITCH */
	{ 0x0b00, 0x0b3f, dominos_port_r }, /* SWITCH */
	{ 0x0b40, 0x0b7f, input_port_3_r }, /* SWITCH */
	{ 0x0c00, 0x0fff, dominos_sync_r }, /* SYNC */
	{ 0x3000, 0x3fff, MRA_ROM }, /* ROM1-ROM2 */
	{ 0xfff0, 0xffff, MRA_ROM }, /* ROM2 for 6502 vectors */
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x07ff, videoram_w, &videoram, &videoram_size }, /* DISPLAY */
	{ 0x0c00, 0x0c0f, dominos_attract_w }, /* ATTRACT */
	{ 0x0c10, 0x0c1f, dominos_tumble_w }, /* TUMBLE */
	{ 0x0c30, 0x0c3f, dominos_lamp2_w }, /* LAMP2 */
	{ 0x0c40, 0x0c4f, dominos_lamp1_w }, /* LAMP1 */
	{ 0x0c80, 0x0cff, MWA_NOP }, /* TIMER RESET */
	{ 0x3000, 0x3fff, MWA_ROM }, /* ROM1-ROM2 */
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( dominos )
	PORT_START		/* DSW - fake port, gets mapped to Dominos ports */
	PORT_DIPNAME( 0x03, 0x01, "Points To Win" )
	PORT_DIPSETTING(	0x03, "6" )
	PORT_DIPSETTING(	0x02, "5" )
	PORT_DIPSETTING(	0x01, "4" )
	PORT_DIPSETTING(	0x00, "3" )
	PORT_DIPNAME( 0x0C, 0x08, "Cost" )
	PORT_DIPSETTING(	0x0C, "2 players/coin" )
	PORT_DIPSETTING(	0x08, "1 coin/player" )
	PORT_DIPSETTING(	0x04, "2 coins/player" )
	PORT_DIPSETTING(	0x00, "2 coins/player" ) /* not a typo */

	PORT_START		/* IN0 - fake port, gets mapped to Dominos ports */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_PLAYER2)
	PORT_BIT (0xF0, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START		/* IN1 - fake port, gets mapped to Dominos ports */
	PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT (0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT (0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_SERVICE | IPF_TOGGLE, "Self Test", KEYCODE_F2, IP_JOY_NONE )

	PORT_START		/* IN2 */
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START		/* IN3 */
	PORT_BIT ( 0x0F, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT ( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* ATTRACT */
	PORT_BIT ( 0x20, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VRESET */
	PORT_BIT ( 0x40, IP_ACTIVE_HIGH, IPT_VBLANK )	/* VBLANK* */
	PORT_BIT ( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Alternating signal? */
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 4, 5, 6, 7, 0x200*8 + 4, 0x200*8 + 5, 0x200*8 + 6, 0x200*8 + 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0x00, 0x02 }, /* offset into colors, # of colors */
	{ -1 }
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( dominos )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 750000)	   /* 750 kHz ???? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))
	
	MDRV_PALETTE_INIT(dominos)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(dominos)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( dominos )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "7352-02.d1",   0x3000, 0x0800, 0x738b4413 )
	ROM_LOAD( "7438-02.e1",   0x3800, 0x0800, 0xc84e54e2 )
	ROM_RELOAD( 			  0xf800, 0x0800 )

	ROM_REGION( 0x800, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "7439-01.p4",   0x0000, 0x0200, 0x4f42fdd6 )
	ROM_LOAD( "7440-01.r4",   0x0200, 0x0200, 0x957dd8df )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAMEX( 1977, dominos, 0, dominos, dominos, 0, ROT0, "Atari", "Dominos", GAME_NO_SOUND )
