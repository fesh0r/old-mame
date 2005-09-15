/***************************************************************************

  gb.c

  Driver file to handle emulation of the Nintendo Gameboy.
  By:

  Hans de Goede               1998
  Anthony Kruize              2002
  Wilbert Pol                 2004 (Megaduck/Cougar Boy)

  Todo list:
  Done entries kept for historical reasons, besides that it's nice to see
  what is already done instead of what has to be done.

Priority:  Todo:                                                  Done:
  2        Replace Marat's  vidhrdw/gb.c  by Playboy code           *
  2        Clean & speed up vidhrdw/gb.c                            *
  2        Replace Marat's  Z80gb/Z80gb.c by Playboy code           *
  2        Transform Playboys Z80gb.c to big case method            *
  2        Clean up Z80gb.c                                         *
  2        Fix / optimise halt instruction                          *
  2        Do correct lcd stat timing                               In Progress
  2        Generate lcd stat interrupts                             *
  2        Replace Marat's code in machine/gb.c by Playboy code     ?
  1        Check, and fix if needed flags bug which troubles ffa    ?
  1        Save/restore battery backed ram                          *
  1        Add sound                                                *
  0        Add supergb support                                      *
  0        Add palette editting, save & restore
  0        Add somekind of backdrop support
  0        Speedups if remotly possible

  2 = has to be done before first public release
  1 = should be added later on
  0 = bells and whistles

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/gb.h"
#include "devices/cartslot.h"

/* Initial value of the cpu registers */
static UINT16 dmg_cpu_reset[6] = { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };	/* GameBoy                          */
static UINT16 sgb_cpu_reset[6] = { 0x01B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };    /* Super GameBoy                    */
static UINT16 gbp_cpu_reset[6] = { 0xFFB0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };	/* GameBoy Pocket / Super GameBoy 2 */
static UINT16 gbc_cpu_reset[6] = { 0x11B0, 0x0013, 0x00D8, 0x014D, 0xFFFE, 0x0100 };	/* GameBoy Color  / Gameboy Advance */
static UINT16 megaduck_cpu_reset[6] = { 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFE, 0x0000 };	/* Megaduck */

static ADDRESS_MAP_START(gb_readmem_map, ADDRESS_SPACE_PROGRAM, 8)
//	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)				/* 16k fixed ROM bank */
	AM_RANGE(0x0000, 0x00ff) AM_READ(MRA8_BANK5)
	AM_RANGE(0x0100, 0x3fff) AM_READ(MRA8_BANK10)
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)			/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_RAM)				/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_BANK2)			/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xfe9f) AM_READ(MRA8_RAM)				/* 8k low RAM, echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_READ(MRA8_NOP)				/* unusable */
	AM_RANGE(0xff00, 0xff7f) AM_READ(gb_io_r)				/* I/O */
	AM_RANGE(0xff80, 0xffff) AM_READ(MRA8_RAM)				/* 127 bytes high RAM, interrupt enable io */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gb_writemem_map, ADDRESS_SPACE_PROGRAM, 8)
/*	AM_RANGE(0x0000, 0x1fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM enable */
/*	AM_RANGE(0x2000, 0x3fff) AM_WRITE(INSTALL AT RUNTIME)	   ROM bank select */
/*	AM_RANGE(0x4000, 0x5fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM bank select */
/*	AM_RANGE(0x6000, 0x7fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM/ROM mode select */
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_RAM)				/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_BANK2)			/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xfe9f) AM_WRITE(MWA8_RAM)				/* 8k low RAM, echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_WRITE(MWA8_NOP)				/* Unusable */
	AM_RANGE(0xff00, 0xff0f) AM_WRITE(gb_io_w)				/* General I/O */
	AM_RANGE(0xff10, 0xff26) AM_WRITE(gb_sound_w)			/* Sound controller */
	AM_RANGE(0xff27, 0xff2f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff30, 0xff3f) AM_WRITE(MWA8_RAM)				/* 16 bytes Wave pattern RAM */
	AM_RANGE(0xff40, 0xff4b) AM_WRITE(gb_video_w)			/* Video controller */
	AM_RANGE(0xff4c, 0xff4f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff50, 0xff50) AM_WRITE(gb_bios_w)				/* BIOS disable */
	AM_RANGE(0xff51, 0xff7f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff80, 0xfffe) AM_WRITE(MWA8_RAM)				/* 127 bytes high RAM */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(gb_ie_w)				/* Interrupt enable */
ADDRESS_MAP_END

static ADDRESS_MAP_START(sgb_writemem_map, ADDRESS_SPACE_PROGRAM, 8)
/*	AM_RANGE(0x0000, 0x1fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM enable */
/*	AM_RANGE(0x2000, 0x3fff) AM_WRITE(INSTALL AT RUNTIME)	   ROM bank select */
/*	AM_RANGE(0x4000, 0x5fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM bank select */
/*	AM_RANGE(0x6000, 0x7fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM/ROM mode select */
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_RAM)				/* 8k VRAM */
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_BANK2)			/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xfe9f) AM_WRITE(MWA8_RAM)				/* 8k low RAM, echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_WRITE(MWA8_NOP)				/* Unusable */
	AM_RANGE(0xff00, 0xff0f) AM_WRITE(sgb_io_w)				/* General I/O */
	AM_RANGE(0xff10, 0xff26) AM_WRITE(gb_sound_w)			/* Sound controller */
	AM_RANGE(0xff27, 0xff2f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff30, 0xff3f) AM_WRITE(MWA8_RAM)				/* 16 bytes Wave pattern RAM */
	AM_RANGE(0xff40, 0xff4b) AM_WRITE(gb_video_w)			/* Video controller */
	AM_RANGE(0xff4c, 0xff7f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff80, 0xfffe) AM_WRITE(MWA8_RAM)				/* 127 bytes high RAM */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(gb_ie_w)				/* Interrupt enable */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gbc_readmem_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)				/* 16k fixed ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)			/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK4)			/* 8k switched VRAM bank */
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_BANK2)			/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xcfff) AM_READ(MRA8_RAM)				/* 4k fixed RAM bank */
	AM_RANGE(0xd000, 0xdfff) AM_READ(MRA8_BANK3)			/* 4k switched RAM bank */
	AM_RANGE(0xe000, 0xfe9f) AM_READ(MRA8_RAM)				/* echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_READ(MRA8_NOP)				/* unusable */
	AM_RANGE(0xff00, 0xff7f) AM_READ(gb_io_r)				/* I/O */
	AM_RANGE(0xff80, 0xffff) AM_READ(MRA8_RAM)				/* 127 bytes high RAM, interrupt enable io */
ADDRESS_MAP_END

static ADDRESS_MAP_START(gbc_writemem_map, ADDRESS_SPACE_PROGRAM, 8)
/*	AM_RANGE(0x0000, 0x1fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM enable */
/*	AM_RANGE(0x2000, 0x3fff) AM_WRITE(INSTALL AT RUNTIME)	   ROM bank select */
/*	AM_RANGE(0x4000, 0x5fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM bank select */
/*	AM_RANGE(0x6000, 0x7fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM/ROM mode select */
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_BANK4)			/* 8k switched VRAM bank */
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(MWA8_BANK2)			/* 8k switched RAM bank (on cartridge) */
	AM_RANGE(0xc000, 0xcfff) AM_WRITE(MWA8_RAM)				/* 4k fixed RAM bank */
	AM_RANGE(0xd000, 0xdfff) AM_WRITE(MWA8_BANK3)			/* 4k switched RAM bank */
	AM_RANGE(0xe000, 0xfeff) AM_WRITE(MWA8_RAM)				/* echo RAM, OAM RAM */
	AM_RANGE(0xff00, 0xff0f) AM_WRITE(gb_io_w)				/* General I/O */
	AM_RANGE(0xff10, 0xff26) AM_WRITE(gb_sound_w)			/* Sound controller */
	AM_RANGE(0xff27, 0xff2f) AM_WRITE(MWA8_NOP)				/* Unused registers */
	AM_RANGE(0xff30, 0xff3f) AM_WRITE(MWA8_RAM)				/* 16 bytes Wave pattern RAM */
	AM_RANGE(0xff40, 0xff7f) AM_WRITE(gbc_video_w)			/* Video controller */
	AM_RANGE(0xff80, 0xfffe) AM_WRITE(MWA8_RAM)				/* 127b high RAM */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(gb_ie_w)				/* Interrupt enable */
ADDRESS_MAP_END

static ADDRESS_MAP_START(megaduck_readmem_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_BANK10)			/* 16k switched ROM bank */
	AM_RANGE(0x4000, 0x7fff) AM_READ(MRA8_BANK1)			/* 16k switched ROM bank */
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_RAM)				/* 8k VRAM */
	AM_RANGE(0xc000, 0xfe9f) AM_READ(MRA8_RAM)				/* 8k low RAM, echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_READ(MRA8_NOP)				/* unusable */
	AM_RANGE(0xff00, 0xff0f) AM_READ(gb_io_r)				/* I/O */
	AM_RANGE(0xff10, 0xff1f) AM_READ(megaduck_video_r)		/* Video registers read */
	AM_RANGE(0xff20, 0xff2f) AM_READ(megaduck_sound_r1)		/* Sound channel 1,2,3 registers */
	AM_RANGE(0xff30, 0xff3f) AM_READ(MRA8_RAM)				/* 6 bytes Wave pattern RAM (?) */
	AM_RANGE(0xff40, 0xff46) AM_READ(megaduck_sound_r2)		/* Sound channel 4 and mixer sound registers */
	AM_RANGE(0xff47, 0xff7f) AM_READ(MRA8_RAM)
	AM_RANGE(0xff80, 0xffff) AM_READ(MRA8_RAM)				/* 127 bytes high RAM, interrupt enable io */
ADDRESS_MAP_END

static ADDRESS_MAP_START(megaduck_writemem_map, ADDRESS_SPACE_PROGRAM, 8 )
/*	AM_RANGE(0x0000, 0x1fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM enable */
/*	AM_RANGE(0x2000, 0x3fff) AM_WRITE(INSTALL AT RUNTIME)	   ROM bank select */
/*	AM_RANGE(0x4000, 0x5fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM bank select */
/*	AM_RANGE(0x6000, 0x7fff) AM_WRITE(INSTALL AT RUNTIME)	   RAM/ROM mode select */
	AM_RANGE(0x8000, 0x9fff) AM_WRITE(MWA8_RAM)				/* 8k VRAM */
	AM_RANGE(0xc000, 0xfe9f) AM_WRITE(MWA8_RAM)				/* 8k low RAM, echo RAM, OAM RAM */
	AM_RANGE(0xfea0, 0xfeff) AM_WRITE(MWA8_NOP)				/* Unusable */
	AM_RANGE(0xff00, 0xff0f) AM_WRITE(gb_io_w)				/* General I/O */
	AM_RANGE(0xff10, 0xff1f) AM_WRITE(megaduck_video_w)		/* Video controller */
	AM_RANGE(0xff20, 0xff2f) AM_WRITE(megaduck_sound_w1)	/* Sound controller */
	AM_RANGE(0xff30, 0xff3f) AM_WRITE(MWA8_RAM)				/* 16 bytes Wave pattern RAM */
	AM_RANGE(0xff40, 0xff46) AM_WRITE(megaduck_sound_w2)	/* Sound controller */
	AM_RANGE(0xff47, 0xff7f) AM_WRITE(MWA8_RAM)				/* Unused registers */
	AM_RANGE(0xff80, 0xfffe) AM_WRITE(MWA8_RAM)				/* 127 bytes high RAM */
	AM_RANGE(0xffff, 0xffff) AM_WRITE(gb_ie_w)				/* Interrupt enable */
ADDRESS_MAP_END


static gfx_decode gb_gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

INPUT_PORTS_START( gameboy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT) PORT_NAME("Left") 
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_NAME("Right") 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP) PORT_NAME("Up") 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN) PORT_NAME("Down") 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_NAME("Button A") 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_NAME("Button B") 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START) PORT_NAME("Start") 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SELECT) PORT_NAME("Select") 
INPUT_PORTS_END

static unsigned char palette[] =
{
/* Simple black and white palette */
/*	0xFF,0xFF,0xFF,
	0xB0,0xB0,0xB0,
	0x60,0x60,0x60,
	0x00,0x00,0x00 */

/* Possibly needs a little more green in it */
	0xFF,0xFB,0x87,		/* Background */
	0xB1,0xAE,0x4E,		/* Light */
	0x84,0x80,0x4E,		/* Medium */
	0x4E,0x4E,0x4E,		/* Dark */

/* Palette for GameBoy Pocket/Light */
	0xC4,0xCF,0xA1,		/* Background */
	0x8B,0x95,0x6D,		/* Light      */
	0x6B,0x73,0x53,		/* Medium     */
	0x41,0x41,0x41,		/* Dark       */
};

/* Initialise the palette */
static PALETTE_INIT( gb )
{
	int ii;
	for( ii = 0; ii < 4; ii++)
	{
		palette_set_color(ii, palette[ii*3+0], palette[ii*3+1], palette[ii*3+2]);
		colortable[ii] = ii;
	}
}

static PALETTE_INIT( gbp )
{
	int ii;
	for( ii = 0; ii < 4; ii++)
	{
		palette_set_color(ii, palette[(ii + 4)*3+0], palette[(ii + 4)*3+1], palette[(ii + 4)*3+2]);
		colortable[ii] = ii;
	}
}

static PALETTE_INIT( sgb )
{
	int ii, r, g, b;

	for( ii = 0; ii < 32768; ii++ )
	{
		r = (ii & 0x1F) << 3;
		g = ((ii >> 5) & 0x1F) << 3;
		b = ((ii >> 10) & 0x1F) << 3;
		palette_set_color( ii, r, g, b );
	}

	/* Some default colours for non-SGB games */
	colortable[0] = 32767;
	colortable[1] = 21140;
	colortable[2] = 10570;
	colortable[3] = 0;
	/* The rest of the colortable can be black */
	for( ii = 4; ii < 8*16; ii++ )
		colortable[ii] = 0;
}

static PALETTE_INIT( gbc )
{
	int ii, r, g, b;

	for( ii = 0; ii < 32768; ii++ )
	{
		r = (ii & 0x1F) << 3;
		g = ((ii >> 5) & 0x1F) << 3;
		b = ((ii >> 10) & 0x1F) << 3;
		palette_set_color( ii, r, g, b );
	}

	/* Background is initialised as white */
	for( ii = 0; ii < 8*4; ii++ )
		colortable[ii] = 32767;
	/* Sprites are supposed to be uninitialized, but we'll make them black */
	for( ii = 8*4; ii < 16*4; ii++ )
		colortable[ii] = 0;
}

static struct CustomSound_interface gameboy_sound_interface =
{ gameboy_sh_start, 0, 0 };


static MACHINE_DRIVER_START( gameboy )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_PROGRAM_MAP(gb_readmem_map, gb_writemem_map)
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 154 * 3)	/* 1 int each scanline ! */
	MDRV_CPU_CONFIG(dmg_cpu_reset)

	MDRV_FRAMES_PER_SECOND(DMG_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( gb )
	MDRV_MACHINE_STOP( gb )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(20*8, 18*8)
	MDRV_VISIBLE_AREA(0*8, 20*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(gb)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(gameboy_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( supergb )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_REPLACE("main", Z80GB, 4295454)	/* 4.295454 Mhz */
	MDRV_CPU_PROGRAM_MAP(gb_readmem_map, sgb_writemem_map)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_CONFIG(sgb_cpu_reset)

	MDRV_MACHINE_INIT( sgb )

	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(8*16)	/* 8 palettes of 16 colours */
	MDRV_PALETTE_INIT(sgb)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gbpocket )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_CONFIG(gbp_cpu_reset)
	MDRV_MACHINE_INIT( gbpocket )
	MDRV_PALETTE_INIT(gbp)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gbcolor )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(gbc_readmem_map, gbc_writemem_map)
	MDRV_CPU_CONFIG(gbc_cpu_reset)

	MDRV_MACHINE_INIT(gbc)

	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(16*4)	/* 16 palettes of 4 colours */
	MDRV_PALETTE_INIT(gbc)
MACHINE_DRIVER_END

static void gameboy_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "gb\0gmb\0cgb\0gbc\0sgb\0";
	dev->must_be_loaded = 1;
	dev->init = device_init_gb_cart;
	dev->load = device_load_gb_cart;
}

static void gameboy_cartslot_getinfo_gb(struct IODevice *dev)
{
	gameboy_cartslot_getinfo(dev);
	dev->must_be_loaded = 0;
}

SYSTEM_CONFIG_START(gameboy)
	CONFIG_DEVICE(gameboy_cartslot_getinfo)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(gameboy_gb)
	CONFIG_DEVICE(gameboy_cartslot_getinfo)
SYSTEM_CONFIG_END

static MACHINE_DRIVER_START( megaduck )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_PROGRAM_MAP(megaduck_readmem_map, megaduck_writemem_map)
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 154 * 3)	/* 1 int each scanline ! */
	MDRV_CPU_CONFIG(megaduck_cpu_reset)

	MDRV_FRAMES_PER_SECOND(DMG_FRAMES_PER_SECOND)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( megaduck )
	MDRV_MACHINE_STOP( gb )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(20*8, 18*8)
	MDRV_VISIBLE_AREA(0*8, 20*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gb_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT(gb)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(gameboy_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(1, "right", 0.50)
MACHINE_DRIVER_END

static void megaduck_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "bin\0";
	dev->must_be_loaded = 1;
	dev->load = device_load_megaduck_cart;
}

SYSTEM_CONFIG_START(megaduck)
	CONFIG_DEVICE(megaduck_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gameboy )
	ROM_REGION( 0x10100, REGION_CPU1, 0 )
	ROM_LOAD( "dmg_boot.bin", 0x10000, 0x0100, CRC(59c8598e) SHA1(4ed31ec6b0b175bb109c0eb5fd3d193da823339f) )
ROM_END

ROM_START( supergb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_REGION( 0x2000,  REGION_GFX1, 0 )	/* SGB border */
ROM_END

ROM_START( gbpocket )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END

ROM_START( gbcolor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END


ROM_START( megaduck )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END

/*    YEAR  NAME      PARENT   COMPAT	MACHINE   INPUT    INIT  CONFIG   COMPANY     FULLNAME */
CONS( 1990, gameboy,  0,       0,		gameboy,  gameboy, 0,    gameboy_gb, "Nintendo", "GameBoy"  )
CONS( 1994, supergb,  0,       gameboy,	supergb,  gameboy, 0,    gameboy, "Nintendo", "Super GameBoy" )
CONS( 1996, gbpocket, gameboy, 0,		gbpocket, gameboy, 0,    gameboy, "Nintendo", "GameBoy Pocket" )
CONS( 1998, gbcolor,  0,       gameboy,	gbcolor,  gameboy, 0,    gameboy, "Nintendo", "GameBoy Color" )

/* Sound is not 100% yet, it generates some sounds which could be ok. Since we're lacking a real
   system there's no way to verify. Same goes for the colors of the LCD. We are no using the default
   GameBoy green colors */
CONS( 1993, megaduck, 0,       0,       megaduck, gameboy, 0,    megaduck,"Creatronic/Videojet/Timlex/Cougar",  "MegaDuck/Cougar Boy" )
