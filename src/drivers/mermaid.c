/***************************************************************************

Mermaid

Driver by Zsolt Vasvari

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char* mermaid_background_videoram;
extern unsigned char* mermaid_foreground_videoram;
extern unsigned char* mermaid_foreground_colorram;
extern unsigned char* mermaid_background_scrollram;
extern unsigned char* mermaid_foreground_scrollram;


PALETTE_INIT( mermaid );
VIDEO_UPDATE( mermaid );
WRITE_HANDLER( mermaid_flip_screen_x_w );
WRITE_HANDLER( mermaid_flip_screen_y_w );


static unsigned char *mermaid_AY8910_enable;

static WRITE_HANDLER( mermaid_AY8910_write_port_w )
{
	if (mermaid_AY8910_enable[0])  AY8910_write_port_0_w(offset, data);
	if (mermaid_AY8910_enable[1])  AY8910_write_port_1_w(offset, data);
}

static WRITE_HANDLER( mermaid_AY8910_control_port_w )
{
	if (mermaid_AY8910_enable[0])  AY8910_control_port_0_w(offset, data);
	if (mermaid_AY8910_enable[1])  AY8910_control_port_1_w(offset, data);
}

#if 0
static READ_HANDLER( mermaid_f800_r )
{
	// collision register active LO
	// Bit 0
	// Bit 1 - Sprite - Foreground
	// Bit 2 - Sprite - Stream
	// Bit 3
	// Bit 4
	// Bit 5
	// Bit 6
	// Bit 7
	//return rand() & 0xff;
	return 0x00;
}
#endif

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x9fff, MRA_ROM },
	{ 0xc000, 0xcbff, MRA_RAM },
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0xd800, 0xd81f, MRA_RAM },
	{ 0xd840, 0xd8bf, MRA_RAM },
	{ 0xdc00, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe000, input_port_0_r },
	{ 0xe800, 0xe800, input_port_1_r },
	{ 0xf000, 0xf000, input_port_2_r },
	{ 0xf800, 0xf800, input_port_3_r },
//	{ 0xf800, 0xf800, mermaid_f800_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x9fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xc800, 0xcbff, MWA_RAM, &mermaid_background_videoram, &videoram_size },
	{ 0xd000, 0xd3ff, MWA_RAM, &mermaid_foreground_videoram },
	{ 0xd800, 0xd81f, MWA_RAM, &mermaid_background_scrollram },
	{ 0xd840, 0xd85f, MWA_RAM, &mermaid_foreground_scrollram },
	{ 0xd880, 0xd8bf, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xdc00, 0xdfff, MWA_RAM, &mermaid_foreground_colorram },
	{ 0xe000, 0xe001, MWA_RAM, &mermaid_AY8910_enable },
	{ 0xe005, 0xe005, mermaid_flip_screen_x_w },
	{ 0xe006, 0xe006, mermaid_flip_screen_y_w },
	{ 0xe007, 0xe007, interrupt_enable_w },
	{ 0xe807, 0xe807, MWA_NOP },	/* watchdog? */
	{ 0xf802, 0xf802, MWA_NOP },	/* ??? see memory map */
	{ 0xf806, 0xf806, mermaid_AY8910_write_port_w },
	{ 0xf807, 0xf807, mermaid_AY8910_control_port_w },
MEMORY_END


INPUT_PORTS_START( mermaid )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000" )
	PORT_DIPSETTING(    0x04, "30000" )
	PORT_DIPNAME( 0x08, 0x08, "Allow Continue" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_DIPSETTING(    0x30, "6" )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ))
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START		/* Fake IN for debug features (0xf800 -> 0xc01a, cpl) */
	PORT_DIPNAME( 0x01, 0x00, "0xf800 bit 0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "0xf800 bit 1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "0xf800 bit 2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "0xf800 bit 3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, "0xf800 bit 4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, "0xf800 bit 5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, "0xf800 bit 6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "0xf800 bit 7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout background_charlayout =
{
	8,8,    /* 8*8 chars */
	256,    /* 256 characters */
	1,      /* 1 bit per pixel */
	{ 0 },  /* single bitplane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout foreground_charlayout =
{
	8,8,    /* 8*8 chars */
	1024,   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, 1024*8*8 },  /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	2,		/* 2 bits per pixel */
	{ 0, 256*32*8 },	/* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &foreground_charlayout,     0, 16 },
	{ REGION_GFX1, 0, &spritelayout,              0, 16 },
	{ REGION_GFX2, 0, &background_charlayout,  4*16, 2  },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 25, 25 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static MACHINE_DRIVER_START( mermaid )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 4000000)        /* 4.00 MHz??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4*16+1)
	MDRV_COLORTABLE_LENGTH(4*16+2*2)

	MDRV_PALETTE_INIT(mermaid)
	MDRV_VIDEO_START(generic)
	MDRV_VIDEO_UPDATE(mermaid)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( mermaid )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "g960_32.15",	  0x0000, 0x1000, 0x8311f090 )
	ROM_LOAD( "g960_33.16",	  0x1000, 0x1000, 0x9f274fc4 )
	ROM_LOAD( "g960_34.17",	  0x2000, 0x1000, 0x5f910179 )
	ROM_LOAD( "g960_35.18",	  0x3000, 0x1000, 0xdb1868a1 )
	ROM_LOAD( "g960_36.19",	  0x4000, 0x1000, 0x178a3567 )
	ROM_LOAD( "g960_37.20",	  0x5000, 0x1000, 0x7d602527 )
	ROM_LOAD( "g960_38.21",	  0x6000, 0x1000, 0xbf9f623c )
	ROM_LOAD( "g960_39.22",	  0x7000, 0x1000, 0xdf0db390 )
	ROM_LOAD( "g960_40.23",	  0x8000, 0x1000, 0xfb7aba3f )
	ROM_LOAD( "g960_41.24",	  0x9000, 0x1000, 0xd022981d )

	ROM_REGION( 0x4000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "g960_45.77",	  0x0000, 0x1000, 0x1f6b735e )
	ROM_LOAD( "g960_44.76",	  0x1000, 0x1000, 0xfd76074e )
	ROM_LOAD( "g960_47.79",	  0x2000, 0x1000, 0x3b7d4ad0 )
	ROM_LOAD( "g960_46.78",	  0x3000, 0x1000, 0x50c117cd )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "g960_43.26",	  0x0000, 0x1000, 0x6f077417 )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "col_a", 	      0x0000, 0x0020, 0xef87bcd6 )
	ROM_LOAD( "col_b", 	      0x0020, 0x0020, 0xca48abdd )

	ROM_REGION( 0x1000, REGION_USER1, 0 )	/* unknown */
	ROM_LOAD( "g960_42.39",	  0x0000, 0x1000, 0x287840bb )
ROM_END


GAMEX( 1982, mermaid, 0, mermaid, mermaid, 0, ROT0, "[Sanritsu] Rock-ola", "Mermaid", GAME_NOT_WORKING )
