/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  R. Belmont
  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Fix additional sound bugs
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58Mhz at the appropriate time.
    - I'm sure there's lots more ...

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "includes/snes.h"
#include "machine/snescart.h"


/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( snes_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x2fffff) AM_READWRITE(snes_r_bank1, snes_w_bank1)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x300000, 0x3fffff) AM_READWRITE(snes_r_bank2, snes_w_bank2)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(snes_r_bank3, SMH_ROM)		/* ROM (and reserved in Mode 20) */
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(snes_r_bank4, snes_w_bank4)	/* used by Mode 20 DSP-1 */
	AM_RANGE(0x700000, 0x7dffff) AM_READWRITE(snes_r_bank5, snes_w_bank5)
	AM_RANGE(0x7e0000, 0x7fffff) AM_RAM					/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	AM_RANGE(0x800000, 0xbfffff) AM_READWRITE(snes_r_bank6, snes_w_bank6)	/* Mirror and ROM */
	AM_RANGE(0xc00000, 0xffffff) AM_READWRITE(snes_r_bank7, snes_w_bank7)	/* Mirror and ROM */
ADDRESS_MAP_END

static READ8_HANDLER( spc_ram_100_r )
{
	return spc_ram_r(machine, offset + 0x100);
}

static WRITE8_HANDLER( spc_ram_100_w )
{
	spc_ram_w(machine, offset + 0x100, data);
}

static ADDRESS_MAP_START( spc_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ef) AM_READWRITE(spc_ram_r, spc_ram_w) AM_BASE(&spc_ram)   	/* lower 32k ram */
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE(spc_io_r, spc_io_w)   	/* spc io */
	AM_RANGE(0x0100, 0xffff) AM_WRITE(spc_ram_100_w)
	AM_RANGE(0x0100, 0xffbf) AM_READ(spc_ram_100_r)
	AM_RANGE(0xffc0, 0xffff) AM_READ(spc_ipl_r)
ADDRESS_MAP_END



/***************************************************************************

  Input ports

***************************************************************************/

static INPUT_PORTS_START( snes )
	PORT_START("PAD1L")		/* Joypad 1 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P1 Button A")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P1 Button X")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P1 Button L")  PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P1 Button R")  PORT_PLAYER(1)

	PORT_START("PAD1H")		/* Joypad 1 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Button B")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Button Y")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)

	PORT_START("PAD2L")		/* Joypad 2 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P2 Button A")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P2 Button X")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P2 Button L")  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P2 Button R")  PORT_PLAYER(2)

	PORT_START("PAD2H")		/* Joypad 2 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P2 Button B")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P2 Button Y")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START("PAD3L")		/* Joypad 3 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P3 Button A")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P3 Button X")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P3 Button L")  PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P3 Button R")  PORT_PLAYER(3)

	PORT_START("PAD3H")		/* Joypad 3 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P3 Button B")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P3 Button Y")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)

	PORT_START("PAD4L")		/* Joypad 4 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_NAME("P4 Button A")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_NAME("P4 Button X")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5 ) PORT_NAME("P4 Button L")  PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6 ) PORT_NAME("P4 Button R")  PORT_PLAYER(4)

	PORT_START("PAD4H")		/* Joypad 4 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P4 Button B")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P4 Button Y")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT ) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START ) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)

	PORT_START("INTERNAL")	/* Configuration */
	PORT_CONFNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_CONFSETTING(   0x0, DEF_STR( No ) )
	PORT_CONFSETTING(   0x1, DEF_STR( Yes ) )

#ifdef MAME_DEBUG
	PORT_START("DEBUG1")	/* debug switches */
	PORT_DIPNAME( 0x3, 0x0, "Browse tiles" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, "2bpl" )
	PORT_DIPSETTING(   0x2, "4bpl" )
	PORT_DIPSETTING(   0x3, "8bpl" )
	PORT_DIPNAME( 0xc, 0x0, "Browse maps" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, "2bpl" )
	PORT_DIPSETTING(   0x8, "4bpl" )
	PORT_DIPSETTING(   0xc, "8bpl" )

	PORT_START("DEBUG2")	/* debug switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Toggle BG 1")  PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("Toggle BG 2")  PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Toggle BG 3")  PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Toggle BG 4")  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Toggle Objects")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8 ) PORT_NAME("Toggle Main/Sub")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Toggle Back col")  PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Toggle Windows")  PORT_PLAYER(3)

	PORT_START("DEBUG3")	/* IN 11 : debug input */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON9 ) PORT_NAME("Pal prev")
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON10 ) PORT_NAME("Pal next")
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON7 ) PORT_NAME("Toggle Transparency")  PORT_PLAYER(4)
#endif
INPUT_PORTS_END


/***************************************************************************

  Sound interface

***************************************************************************/

static const custom_sound_interface snes_sound_interface =
{ snes_sh_start, 0, 0 };



/***************************************************************************

  Machine driver

***************************************************************************/

static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", G65816, MCLK_NTSC/6)	/* 2.68Mhz, also 3.58Mhz */
	MDRV_CPU_PROGRAM_MAP(snes_map, 0)

	MDRV_CPU_ADD("sound", SPC700, 1024000)	/* 1.024 Mhz */
	MDRV_CPU_PROGRAM_MAP(spc_map, 0)
	MDRV_CPU_VBLANK_INT_HACK(NULL, 0)

	MDRV_INTERLEAVE(800)

	MDRV_MACHINE_START( snes_mess )
	MDRV_MACHINE_RESET( snes )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)

	MDRV_SCREEN_RAW_PARAMS(DOTCLK_NTSC, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_NTSC, 0, SNES_SCR_HEIGHT_NTSC)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(snes_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.00)
	MDRV_SOUND_ROUTE(1, "right", 1.00)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_CPU_REPLACE("main", G65816, MCLK_PAL/6)

	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_RAW_PARAMS(DOTCLK_PAL, SNES_HTOTAL, 0, SNES_SCR_WIDTH, SNES_VTOTAL_PAL, 0, SNES_SCR_HEIGHT_PAL)
MACHINE_DRIVER_END


/***************************************************************************

  ROM definition(s)

***************************************************************************/

ROM_START(snes)
	ROM_REGION(0x100,           "user5", 0)		/* IPL ROM */
	ROM_LOAD("spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0))	/* boot rom */
	ROM_REGION(0x800,           "user6", 0)		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD("dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1))
ROM_END

ROM_START(snespal)
	ROM_REGION(0x100,           "user5", 0)		/* IPL ROM */
	ROM_LOAD("spc700.rom", 0, 0x40, CRC(44bb3a40) SHA1(97e352553e94242ae823547cd853eecda55c20f0))	/* boot rom */
	ROM_REGION(0x800,           "user6", 0)		/* add-on chip ROMs (DSP, SFX, etc) */
	ROM_LOAD("dsp1data.bin", 0x000000, 0x000800, CRC(4b02d66d) SHA1(1534f4403d2a0f68ba6e35186fe7595d33de34b1))
ROM_END



/***************************************************************************

  System configuration(s)

***************************************************************************/

static SYSTEM_CONFIG_START(snes)
	CONFIG_DEVICE(snes_cartslot_getinfo)
SYSTEM_CONFIG_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY     FULLNAME                                      FLAGS */
CONS( 1989, snes,    0,      0,      snes,    snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (NTSC)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
CONS( 1991, snespal, snes,   0,      snespal, snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
