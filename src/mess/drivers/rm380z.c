
/*

Research Machines 380Z (aka "RML 380Z" or "RM 380Z")
Microcomputer produced by Research Machines Limited, Oxford, UK
1978-1985
MESS driver by Wilbert Pol and friol
Driver started on 22/12/2011

===

Memory map from sevice manual:

PAGE SEL bit in PORT0 set to 0:

  0000-3BFF - CPU RAM row 1 (15KB!)
  3C00-7BFF - CPU RAM row 2 (16KB)
  7C00-BBFF - Add-on RAM row 1 or HRG RAM (16KB)
  BC00-DFFF - Add-on RAM row 2 (9KB!)
  E000-EFFF - ROM (COS)
  F000-F5FF - VUD and HRG Video RAM
  F600-F9FF - ROM (monitor extension)
  FA00-FAFF - Reserved, RAM?
  FB00-FBFF - Memory-mapped ports (FBFC-FBFF)
  FC00-FFFF - RAM

PAGE SEL bit in PORT0 set to 1:
  0000-0FFF - ROM (COS mirror from E000)
  1B00-1BFF - Memory-mapped ports (1BFC-1BFF)
  1C00-1DFF - ROM
  4000-7FFF - CPU RAM row 1 (16KB!, this RAM normally appears at 0000)
  8000-BFFF - ???
  C000-DFFF - ???
  E000-EFFF - ROM (COS)
  F000-F5FF - VDU and HRG Video RAM
  F600-F9FF - ROM (monitor extension)
  FA00-FAFF - Reserved, RAM?
  FB00-FBFF - Memory-mapped ports (FBFC-FBFF)
  FC00-FFFF - RAM

40x24 - 6 pixels wide (5 + spacing), 9 pixels high = 240x216
Video input clock is 4MHz (8MHz/2)

According to the manuals, VDU-1 chargen is Texas 74LS262.

===

Notes on COS 4.0 disassembly:

- routine at 0xe438 is called at startup in COS 4.0 and it sets the RST vectors in RAM
- routine at 0xe487 finds "top" of system RAM and stores it in 0x0006 and 0x000E
- 0xeca0 - outputs a string (null terminated) to screen (?)
- 0xff18 - does char output to screen (char in A?)

*/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "imagedev/flopdrv.h"
#include "machine/terminal.h"
#include "machine/wd17xx.h"
#include "includes/rm380z.h"


static ADDRESS_MAP_START(rm380z_mem, AS_PROGRAM, 8, rm380z_state)
	AM_RANGE( 0xe000, 0xefff ) AM_ROM AM_REGION(MAINCPU_TAG, 0)
	AM_RANGE( 0xf000, 0xf5ff ) AM_READWRITE(videoram_read,videoram_write)
	AM_RANGE( 0xf600, 0xf9ff ) AM_ROM AM_REGION(MAINCPU_TAG, 0x1000)		/* Extra ROM space for COS4.0 */
	AM_RANGE( 0xfa00, 0xfbfb ) AM_RAM
	AM_RANGE( 0xfbfc, 0xfbff ) AM_READWRITE( port_read, port_write )
	AM_RANGE( 0xfc00, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( rm380z_io , AS_IO, 8, rm380z_state)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	//AM_RANGE(0x0000, 0xffff) AM_READWRITE(rm380z_io_r, rm380z_io_w)
	AM_RANGE(0xc0, 0xc0) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_status_r, wd17xx_command_w)
	AM_RANGE(0xc1, 0xc1) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0xc2, 0xc2) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0xc3, 0xc3) AM_DEVREADWRITE_LEGACY("wd1771", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0xc4, 0xc4) AM_WRITE(disk_0_control)
	AM_RANGE(0xcc, 0xcc) AM_NOP // CTC (?)
ADDRESS_MAP_END

static GENERIC_TERMINAL_INTERFACE( terminal_intf )
{
	DEVCB_DRIVER_MEMBER(rm380z_state, keyboard_put)
};

INPUT_PORTS_START( rm380z )
INPUT_PORTS_END

//
//
//

static const floppy_interface rm380z_floppy_interface =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSSD,
	LEGACY_FLOPPY_OPTIONS_NAME(default),
	"floppy0",
	NULL
};

static SCREEN_UPDATE( rm380z )
{
	rm380z_state *state = screen.machine().driver_data<rm380z_state>();
	state->update_screen(bitmap);
	return 0;
}

static MACHINE_CONFIG_START( rm380z, rm380z_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(rm380z_mem)
	MCFG_CPU_IO_MAP(rm380z_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE((screencols*chdimx), (screenrows*chdimy))
	MCFG_SCREEN_VISIBLE_AREA(0, (screencols*chdimx)-1, 0, (screenrows*chdimy)-1)
	MCFG_SCREEN_UPDATE(rm380z)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	/* RAM configurations */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("56K")

	/* floppy disk */
	MCFG_FD1771_ADD("wd1771", default_wd17xx_interface)
	MCFG_LEGACY_FLOPPY_DRIVE_ADD(FLOPPY_0, rm380z_floppy_interface)

	/* keyboard */
	MCFG_GENERIC_TERMINAL_ADD(TERMINAL_TAG, terminal_intf)

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( rm380z )
	ROM_REGION( 0x10000, MAINCPU_TAG, 0 )
	ROM_LOAD( "cos40b-m.bin", 0x0000, 0x1000, CRC(1f0b3a5c) SHA1(0b29cb2a3b7eaa3770b34f08c4fd42844f42700f))
	ROM_LOAD( "cos40b-m_f600-f9ff.bin", 0x1000, 0x400, CRC(e3397d9d) SHA1(490a0c834b0da392daf782edc7d51ca8f0668b1a))
	ROM_LOAD( "cos40b-m_1c00-1dff.bin", 0x1400, 0x200, CRC(0f759f44) SHA1(9689c1c1faa62c56def999cbedbbb0c8d928dcff))
	// chargen ROM is undumped, afaik
	ROM_REGION( 0x1680, "gfx", 0 )
	ROM_LOAD( "ch3.raw", 0x0000, 0x1680, BAD_DUMP CRC(e0b10221) SHA1(95304ccad9a7af49d79a349feb2bc23035f90abc))
ROM_END

/* Driver */

COMP(1978, rm380z, 0,      0,       rm380z,    rm380z,  0,    "Research Machines", "RM-380Z", GAME_NO_SOUND_HW)
