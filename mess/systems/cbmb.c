/***************************************************************************
	commodore b series computer

	PeT mess@utanet.at

	documentation
	 vice emulator
     www.funet.fi
***************************************************************************/

/*
CBM B Series:   6509 @ 2MHz, 6545/6845 Video, 6526 CIA, 6581 SID, BASIC 4.0+
                (Sometimes called BASIC 4.5)
                Commodore differentiated between the HP (High Profile) and
                LP (Low Profile) series by naming all HP machine CBM.
                (B128-80HP was CBM128-80).  Also, any machine with optional
                8088 CPU card had 'X' after B or CBM (BX128-80).
* CBM B128-80HP 128kB, Detached Keyboard, Cream.                            GP
* CBM B128-80LP 128kB, One-Piece, Cream, New Keyboard.                      GP
* CBM B256-80HP 256kB, Detached Keyboard, Cream.
* CBM B256-80LP 256kB, One-Piece, Cream.                                    GP
* CBM B128-40   6567, 6581, 6509, 6551, 128kB.  In B128-80LP case.
  CBM B256-40   6567, 6581, 6509, 6551, 256kB.  In B128-80LP case.
* CBM B500
* CBM B500      256kB. board same as B128-80.                               GP

CBM 500 Series: 6509, 6567, 6581, 6551.
                Sometimes called PET II series.
* CBM 500       256kB. (is this the 500, or should it 515?)                 EC
* CBM 505       64kB.
* CBM 510       128kB.
* CBM P500      64kB                                                        GP

CBM 600 Series: Same as B series LP
* CBM 610       B128-80 LP                                                  CS
* CBM 620       B256-80 LP                                                  CS

CBM 700 Series: Same as B series HP.  Also named PET 700 Series
* CBM 700       B128-80 LP (Note this unit is out of place here)
* CBM 710       B128-80 HP                                                  SL
* CBM 720       B256-80 HP                                                  GP
* CBM 730       720 with 8088 coprocessor card

CBM B or II Series
B128-80LP/610/B256-80LP/620
Pet700 Series B128-80HP/710/B256-80HP/720/BX256-80HP/730
---------------------------
M6509 2 MHZ
CRTC6545 6845 video chip
RS232 Port/6551
TAPE Port
IEEE488 Port
ROM Port
Monitor Port
Audio
reset
internal user port
internal processor/dram slot
 optional 8088 cpu card
2 internal system bus slots

LP/600 series
-------------
case with integrated powersupply

HP/700 series
-------------
separated keyboard, integrated monitor, no monitor port
no standard monitor with tv frequencies, 25 character lines
with 14 lines per character (like hercules/pc mda)

B128-80LP/610
-------------
128 KB RAM

B256-80LP/620
-------------
256 KB RAM

B128-80HP/710
-------------
128 KB RAM

B256-80HP/720
-------------
256 KB RAM

BX256-80LP/730
--------------
(720 with cpu upgrade)
8088 upgrade CPU

CBM Pet II Series
500/505/515/P500/B128-40/B256-40
--------------------------------
LP/600 case
videochip vic6567
2 gameports
m6509 clock 1? MHZ

CBM 500
-------
256 KB RAM

CBM 505/P500
-------
64 KB RAM

CBM 510
-------
128 KB RAM


state
-----
keyboard
no sound
no tape support
 no system roms supporting build in tape connector found
no ieee488 support
 no floppy support
no internal userport support
no internal slot1/slot2 support
no internal cpu/dram slot support
preliminary quickloader

state 600/700
-------------
dirtybuffer based video system
 no lightpen support
 no rasterline

state 500
-----
rasterline based video system
 no lightpen support
 no rasterline support
 memory access not complete
no gameport a
 no paddles 1,2
 no joystick 1
 no 2 button joystick/mouse joystick emulation
 no mouse
 no lightpen
no gameport b
 paddles 3,4
 joystick 2
 2 button joystick/mouse joystick emulation
 no mouse

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

when problems start with -log and look into error.log file
 */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6509.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/cia6526.h"
#include "includes/tpi6525.h"
#include "includes/vic6567.h"
#include "includes/crtc6845.h"
#include "includes/sid6581.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"

#include "includes/cbmb.h"

static MEMORY_READ_START( cbmb_readmem )
	{0x00000, 0x00000, m6509_read_00000 },
	{0x00001, 0x00001, m6509_read_00001 },
	{0x00002, 0x0ffff, MRA_RAM},
	{0x10000, 0x10000, m6509_read_00000 },
	{0x10001, 0x10001, m6509_read_00001 },
	{0x10002, 0x1ffff, MRA_RAM},
	{0x20000, 0x20000, m6509_read_00000 },
	{0x20001, 0x20001, m6509_read_00001 },
	{0x20002, 0x2ffff, MRA_RAM},
	{0x30000, 0x30000, m6509_read_00000 },
	{0x30001, 0x30001, m6509_read_00001 },
	{0x30002, 0x3ffff, MRA_RAM},
	{0x40000, 0x40000, m6509_read_00000 },
	{0x40001, 0x40001, m6509_read_00001 },
	{0x40002, 0x4ffff, MRA_RAM},
	{0x50000, 0x50000, m6509_read_00000 },
	{0x50001, 0x50001, m6509_read_00001 },
	{0x50002, 0x5ffff, MRA_RAM},
	{0x60000, 0x60000, m6509_read_00000 },
	{0x60001, 0x60001, m6509_read_00001 },
	{0x60002, 0x6ffff, MRA_RAM},
	{0x70000, 0x70000, m6509_read_00000 },
	{0x70001, 0x70001, m6509_read_00001 },
	{0x70002, 0x7ffff, MRA_RAM},
	{0x80000, 0x80000, m6509_read_00000 },
	{0x80001, 0x80001, m6509_read_00001 },
	{0x80002, 0x8ffff, MRA_RAM},
	{0x90000, 0x90000, m6509_read_00000 },
	{0x90001, 0x90001, m6509_read_00001 },
	{0x90002, 0x9ffff, MRA_RAM},
	{0xa0000, 0xa0000, m6509_read_00000 },
	{0xa0001, 0xa0001, m6509_read_00001 },
	{0xa0002, 0xaffff, MRA_RAM},
	{0xb0000, 0xb0000, m6509_read_00000 },
	{0xb0001, 0xb0001, m6509_read_00001 },
	{0xb0002, 0xbffff, MRA_RAM},
	{0xc0000, 0xc0000, m6509_read_00000 },
	{0xc0001, 0xc0001, m6509_read_00001 },
	{0xc0002, 0xcffff, MRA_RAM},
	{0xd0000, 0xd0000, m6509_read_00000 },
	{0xd0001, 0xd0001, m6509_read_00001 },
	{0xd0002, 0xdffff, MRA_RAM},
	{0xe0000, 0xe0000, m6509_read_00000 },
	{0xe0001, 0xe0001, m6509_read_00001 },
	{0xe0002, 0xeffff, MRA_RAM},
	{0xf0000, 0xf0000, m6509_read_00000 },
	{0xf0001, 0xf0001, m6509_read_00001 },
	{0xf0002, 0xf07ff, MRA_RAM },
#if 0
	{0xf0800, 0xf0fff, MRA_ROM },
#endif
	{0xf1000, 0xf1fff, MRA_ROM }, /* cartridges or ram */
	{0xf2000, 0xf3fff, MRA_ROM }, /* cartridges or ram */
	{0xf4000, 0xf5fff, MRA_ROM },
	{0xf6000, 0xf7fff, MRA_ROM },
	{0xf8000, 0xfbfff, MRA_ROM },
	/*	{0xfc000, 0xfcfff, MRA_ROM }, */
	{0xfd000, 0xfd7ff, MRA_ROM },
	{0xfd800, 0xfd8ff, crtc6845_0_port_r },
	/* disk units */
	{0xfda00, 0xfdaff, sid6581_0_port_r },
	/* db00 coprocessor */
	{0xfdc00, 0xfdcff, cia6526_0_port_r },
	/* dd00 acia */
	{0xfde00, 0xfdeff, tpi6525_0_port_r},
	{0xfdf00, 0xfdfff, tpi6525_1_port_r},
	{0xfe000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( cbmb_writemem )
	{0x00000, 0x00000, m6509_write_00000, &cbmb_memory },
	{0x00001, 0x00001, m6509_write_00001 },
	{0x00002, 0x0ffff, MWA_NOP },
	{0x10000, 0x10000, m6509_write_00000 },
	{0x10001, 0x10001, m6509_write_00001 },
	{0x10002, 0x1ffff, MWA_RAM },
	{0x20000, 0x20000, m6509_write_00000 },
	{0x20001, 0x20001, m6509_write_00001 },
	{0x20002, 0x2ffff, MWA_RAM },
	{0x30000, 0x30000, m6509_write_00000 },
	{0x30001, 0x30001, m6509_write_00001 },
	{0x30002, 0x3ffff, MWA_RAM },
	{0x40000, 0x40000, m6509_write_00000 },
	{0x40001, 0x40001, m6509_write_00001 },
	{0x40002, 0x4ffff, MWA_RAM },
	{0x50000, 0x50000, m6509_write_00000 },
	{0x50001, 0x50001, m6509_write_00001 },
	{0x50002, 0x5ffff, MWA_NOP },
	{0x60000, 0x60000, m6509_write_00000 },
	{0x60001, 0x60001, m6509_write_00001 },
	{0x60002, 0x6ffff, MWA_RAM },
	{0x70000, 0x70000, m6509_write_00000 },
	{0x70001, 0x70001, m6509_write_00001 },
	{0x70002, 0x7ffff, MWA_RAM },
	{0x80000, 0x80000, m6509_write_00000 },
	{0x80001, 0x80001, m6509_write_00001 },
	{0x80002, 0x8ffff, MWA_RAM },
	{0x90000, 0x90000, m6509_write_00000 },
	{0x90001, 0x90001, m6509_write_00001 },
	{0x90002, 0x9ffff, MWA_RAM },
	{0xa0000, 0xa0000, m6509_write_00000 },
	{0xa0001, 0xa0001, m6509_write_00001 },
	{0xa0002, 0xaffff, MWA_RAM },
	{0xb0000, 0xb0000, m6509_write_00000 },
	{0xb0001, 0xb0001, m6509_write_00001 },
	{0xb0002, 0xbffff, MWA_RAM },
	{0xc0000, 0xc0000, m6509_write_00000 },
	{0xc0001, 0xc0001, m6509_write_00001 },
	{0xc0002, 0xcffff, MWA_RAM },
	{0xd0000, 0xd0000, m6509_write_00000 },
	{0xd0001, 0xd0001, m6509_write_00001 },
	{0xd0002, 0xdffff, MWA_RAM },
	{0xe0000, 0xe0000, m6509_write_00000 },
	{0xe0001, 0xe0001, m6509_write_00001 },
	{0xe0002, 0xeffff, MWA_RAM },
	{0xf0000, 0xf0000, m6509_write_00000 },
	{0xf0001, 0xf0001, m6509_write_00001 },
	{0xf0002, 0xf07ff, MWA_RAM },
	{0xf1000, 0xf1fff, MWA_ROM }, /* cartridges */
	{0xf2000, 0xf3fff, MWA_ROM }, /* cartridges */
	{0xf4000, 0xf5fff, MWA_ROM },
	{0xf6000, 0xf7fff, MWA_ROM },
	{0xf8000, 0xfbfff, MWA_ROM, &cbmb_basic },
	{0xfd000, 0xfd7ff, videoram_w, &videoram,&videoram_size }, /* VIDEORAM */
	{0xfd800, 0xfd8ff, crtc6845_0_port_w },
	/* disk units */
	{0xfda00, 0xfdaff, sid6581_0_port_w},
	/* db00 coprocessor */
	{0xfdc00, 0xfdcff, cia6526_0_port_w},
	/* dd00 acia */
	{0xfde00, 0xfdeff, tpi6525_0_port_w},
	{0xfdf00, 0xfdfff, tpi6525_1_port_w},
	{0xfe000, 0xfffff, MWA_ROM, &cbmb_kernal },
MEMORY_END

static MEMORY_READ_START( cbm500_readmem )
	{0x00000, 0x00000, m6509_read_00000 },
	{0x00001, 0x00001, m6509_read_00001 },
	{0x00002, 0x0ffff, MRA_RAM},
	{0x10000, 0x10000, m6509_read_00000 },
	{0x10001, 0x10001, m6509_read_00001 },
	{0x10002, 0x1ffff, MRA_RAM},
	{0x20000, 0x20000, m6509_read_00000 },
	{0x20001, 0x20001, m6509_read_00001 },
	{0x20002, 0x2ffff, MRA_RAM},
	{0x30000, 0x30000, m6509_read_00000 },
	{0x30001, 0x30001, m6509_read_00001 },
	{0x30002, 0x3ffff, MRA_RAM},
	{0x40000, 0x40000, m6509_read_00000 },
	{0x40001, 0x40001, m6509_read_00001 },
	{0x40002, 0x4ffff, MRA_RAM},
	{0x50000, 0x50000, m6509_read_00000 },
	{0x50001, 0x50001, m6509_read_00001 },
	{0x50002, 0x5ffff, MRA_RAM},
	{0x60000, 0x60000, m6509_read_00000 },
	{0x60001, 0x60001, m6509_read_00001 },
	{0x60002, 0x6ffff, MRA_RAM},
	{0x70000, 0x70000, m6509_read_00000 },
	{0x70001, 0x70001, m6509_read_00001 },
	{0x70002, 0x7ffff, MRA_RAM},
	{0x80000, 0x80000, m6509_read_00000 },
	{0x80001, 0x80001, m6509_read_00001 },
	{0x80002, 0x8ffff, MRA_RAM},
	{0x90000, 0x90000, m6509_read_00000 },
	{0x90001, 0x90001, m6509_read_00001 },
	{0x90002, 0x9ffff, MRA_RAM},
	{0xa0000, 0xa0000, m6509_read_00000 },
	{0xa0001, 0xa0001, m6509_read_00001 },
	{0xa0002, 0xaffff, MRA_RAM},
	{0xb0000, 0xb0000, m6509_read_00000 },
	{0xb0001, 0xb0001, m6509_read_00001 },
	{0xb0002, 0xbffff, MRA_RAM},
	{0xc0000, 0xc0000, m6509_read_00000 },
	{0xc0001, 0xc0001, m6509_read_00001 },
	{0xc0002, 0xcffff, MRA_RAM},
	{0xd0000, 0xd0000, m6509_read_00000 },
	{0xd0001, 0xd0001, m6509_read_00001 },
	{0xd0002, 0xdffff, MRA_RAM},
	{0xe0000, 0xe0000, m6509_read_00000 },
	{0xe0001, 0xe0001, m6509_read_00001 },
	{0xe0002, 0xeffff, MRA_RAM},
	{0xf0000, 0xf0000, m6509_read_00000 },
	{0xf0001, 0xf0001, m6509_read_00001 },
	{0xf0002, 0xf07ff, MRA_RAM },
#if 0
	{0xf0800, 0xf0fff, MRA_ROM },
#endif
	{0xf1000, 0xf1fff, MRA_ROM }, /* cartridges or ram */
	{0xf2000, 0xf3fff, MRA_ROM }, /* cartridges or ram */
	{0xf4000, 0xf5fff, MRA_ROM },
	{0xf6000, 0xf7fff, MRA_ROM },
	{0xf8000, 0xfbfff, MRA_ROM },
	/*	{0xfc000, 0xfcfff, MRA_ROM }, */
	{0xfd000, 0xfd3ff, MRA_RAM }, /* videoram */
	{0xfd400, 0xfd7ff, MRA_RAM }, /* colorram */
	{0xfd800, 0xfd8ff, vic2_port_r },
	/* disk units */
	{0xfda00, 0xfdaff, sid6581_0_port_r },
	/* db00 coprocessor */
	{0xfdc00, 0xfdcff, cia6526_0_port_r },
	/* dd00 acia */
	{0xfde00, 0xfdeff, tpi6525_0_port_r},
	{0xfdf00, 0xfdfff, tpi6525_1_port_r},
	{0xfe000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( cbm500_writemem )
	{0x00000, 0x00000, m6509_write_00000, &cbmb_memory },
	{0x00001, 0x00001, m6509_write_00001 },
	{0x00002, 0x0ffff, MWA_RAM },
	{0x10000, 0x10000, m6509_write_00000 },
	{0x10001, 0x10001, m6509_write_00001 },
	{0x10002, 0x1ffff, MWA_RAM },
	{0x20000, 0x20000, m6509_write_00000 },
	{0x20001, 0x20001, m6509_write_00001 },
	{0x20002, 0x2ffff, MWA_NOP },
	{0x30000, 0x30000, m6509_write_00000 },
	{0x30001, 0x30001, m6509_write_00001 },
	{0x30002, 0x3ffff, MWA_RAM },
	{0x40000, 0x40000, m6509_write_00000 },
	{0x40001, 0x40001, m6509_write_00001 },
	{0x40002, 0x4ffff, MWA_RAM },
	{0x50000, 0x50000, m6509_write_00000 },
	{0x50001, 0x50001, m6509_write_00001 },
	{0x50002, 0x5ffff, MWA_RAM },
	{0x60000, 0x60000, m6509_write_00000 },
	{0x60001, 0x60001, m6509_write_00001 },
	{0x60002, 0x6ffff, MWA_RAM },
	{0x70000, 0x70000, m6509_write_00000 },
	{0x70001, 0x70001, m6509_write_00001 },
	{0x70002, 0x7ffff, MWA_RAM },
	{0x80000, 0x80000, m6509_write_00000 },
	{0x80001, 0x80001, m6509_write_00001 },
	{0x80002, 0x8ffff, MWA_NOP },
	{0x90000, 0x90000, m6509_write_00000 },
	{0x90001, 0x90001, m6509_write_00001 },
	{0x90002, 0x9ffff, MWA_RAM },
	{0xa0000, 0xa0000, m6509_write_00000 },
	{0xa0001, 0xa0001, m6509_write_00001 },
	{0xa0002, 0xaffff, MWA_RAM },
	{0xb0000, 0xb0000, m6509_write_00000 },
	{0xb0001, 0xb0001, m6509_write_00001 },
	{0xb0002, 0xbffff, MWA_RAM },
	{0xc0000, 0xc0000, m6509_write_00000 },
	{0xc0001, 0xc0001, m6509_write_00001 },
	{0xc0002, 0xcffff, MWA_RAM },
	{0xd0000, 0xd0000, m6509_write_00000 },
	{0xd0001, 0xd0001, m6509_write_00001 },
	{0xd0002, 0xdffff, MWA_RAM },
	{0xe0000, 0xe0000, m6509_write_00000 },
	{0xe0001, 0xe0001, m6509_write_00001 },
	{0xe0002, 0xeffff, MWA_RAM },
	{0xf0000, 0xf0000, m6509_write_00000 },
	{0xf0001, 0xf0001, m6509_write_00001 },
	{0xf0002, 0xf07ff, MWA_RAM },
	{0xf1000, 0xf1fff, MWA_ROM }, /* cartridges */
	{0xf2000, 0xf3fff, MWA_ROM }, /* cartridges */
	{0xf4000, 0xf5fff, MWA_ROM },
	{0xf6000, 0xf7fff, MWA_ROM },
	{0xf8000, 0xfbfff, MWA_ROM, &cbmb_basic },
	{0xfd000, 0xfd3ff, MWA_RAM, &cbmb_videoram },
	{0xfd400, 0xfd7ff, cbmb_colorram_w, &cbmb_colorram },
	{0xfd800, 0xfd8ff, vic2_port_w },
	/* disk units */
	{0xfda00, 0xfdaff, sid6581_0_port_w},
	/* db00 coprocessor */
	{0xfdc00, 0xfdcff, cia6526_0_port_w},
	/* dd00 acia */
	{0xfde00, 0xfdeff, tpi6525_0_port_w},
	{0xfdf00, 0xfdfff, tpi6525_1_port_w},
	{0xfe000, 0xfffff, MWA_ROM, &cbmb_kernal },
MEMORY_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define CBMB_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "ESC", KEYCODE_ESC)\
	DIPS_HELPER( 0x4000, "1 !", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 At-Sign", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 Arrow-Up", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 &", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 *", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 (", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0 )", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "-", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "= +", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "Arrow-Left Pound", KEYCODE_TILDE)\
	DIPS_HELPER( 0x0002, "DEL INS", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0001, "TAB", KEYCODE_TAB)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x4000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x2000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x1000, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0800, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0400, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0200, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0100, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0080, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0040, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0020, "[", KEYCODE_OPENBRACE)\
        DIPS_HELPER( 0x0010, "]", KEYCODE_CLOSEBRACE)\
        DIPS_HELPER( 0x0008, "RETURN",KEYCODE_ENTER)\
	PORT_BITX( 0x0004, 0, IPT_DIPSWITCH_NAME|IPF_TOGGLE,\
		     "(shift)SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE)\
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )\
	 PORT_DIPSETTING(4, DEF_STR( On ) )\
	DIPS_HELPER( 0x0002, "A", KEYCODE_A)\
	DIPS_HELPER( 0x0001, "S", KEYCODE_S)\
	PORT_START \
	DIPS_HELPER( 0x8000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x4000, "F", KEYCODE_F)\
	DIPS_HELPER( 0x2000, "G", KEYCODE_G)\
	DIPS_HELPER( 0x1000, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0800, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0400, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0200, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0100, "; :", KEYCODE_COLON)\
	DIPS_HELPER( 0x0080, "' \"", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0040, "PI", KEYCODE_BACKSLASH)\
	DIPS_HELPER( 0x0020, "(shift)Left-Shift", KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x0010, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x0008, "X", KEYCODE_X)\
	DIPS_HELPER( 0x0004, "C", KEYCODE_C)\
	DIPS_HELPER( 0x0002, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0001, "B", KEYCODE_B)\
	PORT_START \
	DIPS_HELPER( 0x8000, "N", KEYCODE_N)\
	DIPS_HELPER( 0x4000, "M", KEYCODE_M)\
	DIPS_HELPER( 0x2000, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x1000, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0800, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0400, "(shift)Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0200, "CBM", KEYCODE_RALT)\
	DIPS_HELPER( 0x0100, "CTRL", KEYCODE_RCONTROL)\
	DIPS_HELPER( 0x0080, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0040, "f1", KEYCODE_F1)\
	DIPS_HELPER( 0x0020, "f2", KEYCODE_F2)\
	DIPS_HELPER( 0x0010, "f3", KEYCODE_F3)\
	DIPS_HELPER( 0x0008, "f4", KEYCODE_F4)\
	DIPS_HELPER( 0x0004, "f5", KEYCODE_F5)\
	DIPS_HELPER( 0x0002, "f6", KEYCODE_F6)\
	DIPS_HELPER( 0x0001, "f7", KEYCODE_F7)\
	PORT_START \
	DIPS_HELPER( 0x8000, "f8", KEYCODE_F8)\
	DIPS_HELPER( 0x4000, "f9", KEYCODE_F9)\
	DIPS_HELPER( 0x2000, "f10", KEYCODE_F10)\
	DIPS_HELPER( 0x1000, "CRSR Down", KEYCODE_DOWN)\
	DIPS_HELPER( 0x0800, "CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0400, "CRSR Left", KEYCODE_LEFT)\
	DIPS_HELPER( 0x0200, "CRSR Right", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0100, "HOME CLR", KEYCODE_INSERT)

#define CBMB_KEYBOARD2 \
	DIPS_HELPER( 0x0020, "STOP RUN", KEYCODE_DEL)\
	DIPS_HELPER( 0x0010, "NUM ?", KEYCODE_END)\
	DIPS_HELPER( 0x0008, "NUM CE", KEYCODE_PGDN)\
	DIPS_HELPER( 0x0004, "NUM *", KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x0002, "NUM /", KEYCODE_SLASH_PAD)\
	DIPS_HELPER( 0x0001, "NUM 7", KEYCODE_7_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 8", KEYCODE_8_PAD)\
	DIPS_HELPER( 0x4000, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x2000, "NUM -", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x1000, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x0800, "NUM 5", KEYCODE_5_PAD)\
	DIPS_HELPER( 0x0400, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0200, "NUM +", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0100, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x0080, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0040, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0020, "NUM Enter", KEYCODE_ENTER_PAD)\
	DIPS_HELPER( 0x0010, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0008, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0004, "NUM 00", KEYCODE_NUMLOCK)\

INPUT_PORTS_START (cbm600)
     CBMB_KEYBOARD
     DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	 DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	 CBMB_KEYBOARD2
     PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	 PORT_BIT (0x200, 0x200, IPT_UNUSED) /* ntsc */
	 PORT_BIT (0x100, 0x000, IPT_UNUSED) /* cbm600 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm600pal)
     CBMB_KEYBOARD
     DIPS_HELPER( 0x0080, "GRAPH NORM", KEYCODE_HOME)
	 DIPS_HELPER( 0x0040, "ASCII DIN", KEYCODE_PGUP)
	 CBMB_KEYBOARD2
     PORT_START
	 DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	 PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	 PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	 PORT_DIPSETTING(  0, DEF_STR( Off ) )
	 PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	 DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	 DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	 DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	 PORT_BIT (0x200, 0x000, IPT_UNUSED) /* pal */
	 PORT_BIT (0x100, 0x000, IPT_UNUSED) /* cbm600 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm700)
	CBMB_KEYBOARD
    DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	CBMB_KEYBOARD2
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* not used */
	PORT_BIT (0x100, 0x100, IPT_UNUSED) /* cbm700 */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
INPUT_PORTS_END

INPUT_PORTS_START (cbm500)
	CBMB_KEYBOARD
    DIPS_HELPER( 0x0080, "RVS OFF", KEYCODE_HOME)
	DIPS_HELPER( 0x0040, "GRAPH NORM", KEYCODE_PGUP)
	CBMB_KEYBOARD2
    PORT_START
    DIPS_HELPER( 0x8000, "Quickload", KEYCODE_F8)
#ifdef PET_TEST_CODE
	PORT_DIPNAME   ( 0x4000, 0x4000, "Tape Drive/Device 1")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x4000, DEF_STR( On ) )
	PORT_DIPNAME   ( 0x2000, 0x00, " Tape Sound")
	PORT_DIPSETTING(  0, DEF_STR( Off ) )
	PORT_DIPSETTING(0x2000, DEF_STR( On ) )
	DIPS_HELPER( 0x1000, "Tape Drive Play",       KEYCODE_F5)
	DIPS_HELPER( 0x0800, "Tape Drive Record",     KEYCODE_F6)
	DIPS_HELPER( 0x0400, "Tape Drive Stop",       KEYCODE_F7)
#endif
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* not used */
	PORT_BIT (0x100, 0x000, IPT_UNUSED) /* not used */
#ifdef PET_TEST_CODE
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
#endif
	/*C64_DIPS */
INPUT_PORTS_END

unsigned char cbm700_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static unsigned short cbmb_colortable[] = {
	0, 1
};

static struct GfxLayout cbm600_charlayout =
{
	8,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static struct GfxLayout cbm700_charlayout =
{
	9,16,
	256,                                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 }, // 8.column will be cleared in cbm700_vh_start
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

static struct GfxDecodeInfo cbm600_gfxdecodeinfo[] = {
	{ 1, 0x0000, &cbm600_charlayout, 0, 1 },
	{ 1, 0x1000, &cbm600_charlayout, 0, 1 },
    { -1 } /* end of array */
};

static struct GfxDecodeInfo cbm700_gfxdecodeinfo[] = {
	{ 1, 0x0000, &cbm700_charlayout, 0, 1 },
	{ 1, 0x1000, &cbm700_charlayout, 0, 1 },
    { -1 } /* end of array */
};

static void cbm500_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, vic2_palette, sizeof (vic2_palette));
}

static void cbm700_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, cbm700_palette, sizeof (cbm700_palette));
    memcpy(sys_colortable,cbmb_colortable,sizeof(cbmb_colortable));
}

ROM_START (cbm610)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, 0xb0dcb56d)
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, 0xde04ea4f)
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, 0x09a5667e)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, 0x1acf5098)
ROM_END

ROM_START (cbm620)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, 0x5c1f3347)
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, 0x72aa44e1)
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, 0x09a5667e)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901237.01", 0x0000, 0x1000, 0x1acf5098)
ROM_END

ROM_START (cbm620hu)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("610u60.bin", 0xf8000, 0x4000, 0x8eed0d7e)
	ROM_LOAD ("kernhun.bin", 0xfe000, 0x2000, 0x0ea8ca4d)
	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD ("charhun.bin", 0x0000, 0x2000, 0x1fb5e596)
ROM_END

ROM_START (cbm710)
	ROM_REGION (0x100000, REGION_CPU1, 0)
	ROM_LOAD ("901243.04a", 0xf8000, 0x2000, 0xb0dcb56d)
	ROM_LOAD ("901242.04a", 0xfa000, 0x2000, 0xde04ea4f)
	ROM_LOAD ("901244.04a", 0xfe000, 0x2000, 0x09a5667e)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, 0x3a350bc3)
ROM_END

ROM_START (cbm720)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, 0x5c1f3347)
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, 0x72aa44e1)
    ROM_LOAD ("901244.04a", 0xfe000, 0x2000, 0x09a5667e)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901232.01", 0x0000, 0x1000, 0x3a350bc3)
ROM_END

ROM_START (cbm720se)
	ROM_REGION (0x100000, REGION_CPU1, 0)
    ROM_LOAD ("901241.03", 0xf8000, 0x2000, 0x5c1f3347)
    ROM_LOAD ("901240.03", 0xfa000, 0x2000, 0x72aa44e1)
    ROM_LOAD ("901244.03", 0xfe000, 0x2000, 0x87bc142b)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901233.03", 0x0000, 0x1000, 0x09518b19)
ROM_END


ROM_START (cbm500)
	ROM_REGION (0x101000, REGION_CPU1, 0)
	ROM_LOAD ("901236.02", 0xf8000, 0x2000, 0xc62ab16f)
	ROM_LOAD ("901235.02", 0xfa000, 0x2000, 0x20b7df33)
	ROM_LOAD ("901234.02", 0xfe000, 0x2000, 0xf46bbd2b)
	ROM_LOAD ("901225.01", 0x100000, 0x1000, 0xec4272ee)
ROM_END

#if 0
/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

    /* scrap */
	 /* 0000 1fff --> 0000
                      inverted 2000
        2000 3fff --> 4000
		              inverted 6000 */

	 /* 128 kb basic version */
    ROM_LOAD ("b128-8000.901243-02b.bin", 0xf8000, 0x2000, 0x9d0366f9)
    ROM_LOAD ("b128-a000.901242-02b.bin", 0xfa000, 0x2000, 0x837978b5)
	 /* merged df83bbb9 */

    ROM_LOAD ("b128-8000.901243-04a.bin", 0xf8000, 0x2000, 0xb0dcb56d)
    ROM_LOAD ("b128-a000.901242-04a.bin", 0xfa000, 0x2000, 0xde04ea4f)
	 /* merged a8ff9372 */

	 /* some additions to 901242-04a */
    ROM_LOAD ("b128-a000.901242-04_3f.bin", 0xfa000, 0x2000, 0x5a680d2a)

     /* 256 kbyte basic version */
    ROM_LOAD ("b256-8000.610u60.bin", 0xf8000, 0x4000, 0x8eed0d7e)

    ROM_LOAD ("b256-8000.901241-03.bin", 0xf8000, 0x2000, 0x5c1f3347)
    ROM_LOAD ("b256-a000.901240-03.bin", 0xfa000, 0x2000, 0x72aa44e1)
	 /* merged 5db15870 */

     /* monitor instead of tape */
    ROM_LOAD ("kernal.901244-03b.bin", 0xfe000, 0x2000, 0x4276dbba)
     /* modified 03b for usage of vc1541 on tape port ??? */
    ROM_LOAD ("kernelnew", 0xfe000, 0x2000, 0x19bf247e)
    ROM_LOAD ("kernal.901244-04a.bin", 0xfe000, 0x2000, 0x09a5667e)
    ROM_LOAD ("kernal.hungarian.bin", 0xfe000, 0x2000, 0x0ea8ca4d)


	 /* 600 8x16 chars for 8x8 size
        128 ascii, 128 ascii graphics
		inversion logic in hardware */
    ROM_LOAD ("characters.901237-01.bin", 0x0000, 0x1000, 0x1acf5098)
	 /* packing 128 national, national graphics, ascii, ascii graphics */
    ROM_LOAD ("characters-hungarian.bin", 0x0000, 0x2000, 0x1fb5e596)
	 /* 700 8x16 chars for 9x14 size*/
    ROM_LOAD ("characters.901232-01.bin", 0x0000, 0x1000, 0x3a350bc3)

    ROM_LOAD ("vt52emu.bin", 0xf4000, 0x2000, 0xb3b6173a)
	 /* load address 0xf4000? */
    ROM_LOAD ("moni.bin", 0xfe000, 0x2000, 0x43b08d1f)

    ROM_LOAD ("profitext.bin", 0xf2000, 0x2000, 0xac622a2b)
	 /* address ?*/
    ROM_LOAD ("sfd1001-copy-u59.bin", 0xf1000, 0x1000, 0x1c0fd916)

    ROM_LOAD ("", 0xfe000, 0x2000, 0x)

	 /* 500 */
	 /* 128 basic, other colors than cbmb series basic */
    ROM_LOAD ("basic-lo.901236-02.bin", 0xf8000, 0x2000, 0xc62ab16f)
    ROM_LOAD ("basic-hi.901235-02.bin", 0xfa000, 0x2000, 0x20b7df33)
     /* monitor instead of tape */
    ROM_LOAD ("kernal.901234-02.bin", 0xfe000, 0x2000, 0xf46bbd2b)
    ROM_LOAD ("characters.901225-01.bin", 0x100000, 0x1000, 0xec4272ee)

#endif

static SID6581_interface sid_sound_interface =
{
	{
		sid6581_custom_start,
		sid6581_custom_stop,
		sid6581_custom_update
	},
	1,
	{
		{
			MIXER(50, MIXER_PAN_CENTER),
			MOS6581,
			1000000,
			NULL
		}
	}
};


static struct MachineDriver machine_driver_cbm600 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6509,
			2000000,
			cbmb_readmem, cbmb_writemem,
			0, 0,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	cbmb_init_machine,
	cbmb_shutdown_machine,

  /* video hardware */
	640,							   /* screen width */
	200,							   /* screen height */
	{0, 640 - 1, 0, 200 - 1},		   /* visible_area */
	cbm600_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3,
	sizeof (cbmb_colortable) / sizeof(cbmb_colortable[0]),
	cbm700_init_palette,				   /* convert color prom */
#ifdef PET_TEST_CODE
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_PIXEL_ASPECT_RATIO_1_2|VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#endif
	0,
	generic_vh_start,
	generic_vh_stop,
	cbmb_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid_sound_interface },
		{ 0 }
	}
};

static struct MachineDriver machine_driver_cbm600pal =
{
  /* basic machine hardware */
	{
		{
			CPU_M6509,
			2000000,
			cbmb_readmem, cbmb_writemem,
			0, 0,
			0, 0,
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	cbmb_init_machine,
	cbmb_shutdown_machine,

  /* video hardware */
	640,							   /* screen width */
	200,							   /* screen height */
	{0, 640 - 1, 0, 200 - 1},		   /* visible_area */
	cbm600_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3,
	sizeof (cbmb_colortable) / sizeof(cbmb_colortable[0]),
	cbm700_init_palette,				   /* convert color prom */
#ifdef PET_TEST_CODE
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_PIXEL_ASPECT_RATIO_1_2|VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#endif
	0,
	generic_vh_start,
	generic_vh_stop,
	cbmb_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid_sound_interface },
		{ 0 }
	}
};

static struct MachineDriver machine_driver_cbm700 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6509,
			2000000,
			cbmb_readmem, cbmb_writemem,
			0, 0,
			0, 0,
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	cbmb_init_machine,
	cbmb_shutdown_machine,

  /* video hardware */
	720,							   /* screen width */
	350,							   /* screen height */
	{0, 720 - 1, 0, 350 - 1},		   /* visible_area */
	cbm700_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (cbm700_palette) / sizeof (cbm700_palette[0]) / 3,
	sizeof (cbmb_colortable) / sizeof(cbmb_colortable[0]),
	cbm700_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	cbm700_vh_start,
	generic_vh_stop,
	cbmb_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ SOUND_CUSTOM, &sid_sound_interface },
		{ 0 }
	}
};

static struct MachineDriver machine_driver_cbm500 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6509,
			VIC6567_CLOCK,
			cbm500_readmem, cbm500_writemem,
			0, 0,
			0, 0,
			vic2_raster_irq, VIC2_HRETRACERATE,
		},
	},
	VIC6567_VRETRACERATE, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	cbmb_init_machine,
	cbmb_shutdown_machine,

	/* video hardware */
	336,							   /* screen width */
	216,							   /* screen height */
	{0, 336 - 1, 0, 216 - 1},		   /* visible_area */
	0,								   /* graphics decode info */
	sizeof (vic2_palette) / sizeof (vic2_palette[0]) / 3,
	0,
	cbm500_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER,
	0,
	vic2_vh_start,
	vic2_vh_stop,
	vic2_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		// ad_converter wired to joystick ports
		{ SOUND_CUSTOM, &sid_sound_interface },
		{ 0 }
	}
};

static const struct IODevice io_cbmb[] =
{
	IODEVICE_CBMB_QUICK,
	IODEVICE_CBM_ROM("crt\00010\00020\00040\00060\0"),
	/* monitor OR tape routine in kernal */
#ifdef PET_TEST_CODE
	IODEVICE_CBM_DRIVE,
#endif
	{IO_END}
};

static const struct IODevice io_cbm500[] =
{
	IODEVICE_CBM500_QUICK,
	IODEVICE_CBM_ROM("crt\00010\00020\00040\00060\0"),
#ifdef PET_TEST_CODE
	IODEVICE_CBM_DRIVE,
#endif
	/* monitor OR tape routine in kernal */
	{IO_END}
};

#define init_cbm500 cbm500_driver_init
#define init_cbm600 cbm600_driver_init
#define init_cbm600hu cbm600hu_driver_init
#define init_cbm600pal cbm600pal_driver_init
#define init_cbm700 cbm700_driver_init

#define io_cbm710 io_cbmb
#define io_cbm720 io_cbmb
#define io_cbm720se io_cbmb
#define io_cbm610 io_cbmb
#define io_cbm620 io_cbmb
#define io_cbm620hu io_cbmb

#if 0
#define rom_cbm730 rom_cbmb256hp
#endif

/*     YEAR		NAME	PARENT	MACHINE		INPUT		INIT		COMPANY								FULLNAME */
COMPX (1983,	cbm500,	0,		cbm500,		cbm500,		cbm500,		"Commodore Business Machines Co.",	"Commodore B128-40/Pet-II/P500 60Hz",		GAME_NOT_WORKING)
COMPX (1983,	cbm610, 0,		cbm600, 	cbm600, 	cbm600, 	"Commodore Business Machines Co.",  "Commodore B128-80LP/610 60Hz",             GAME_NOT_WORKING)
COMPX (1983,	cbm620,	cbm610,	cbm600pal,	cbm600pal,	cbm600pal,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 50Hz",	GAME_NOT_WORKING)
COMPX (1983,	cbm620hu,	cbm610,	cbm600pal,	cbm600pal,	cbm600hu,	"Commodore Business Machines Co.",	"Commodore B256-80LP/620 Hungarian 50Hz",	GAME_NOT_WORKING)
COMPX (1983,	cbm710, cbm610, cbm700, 	cbm700, 	cbm700, 	"Commodore Business Machines Co.",  "Commodore B128-80HP/710",                  GAME_NOT_WORKING)
COMPX (1983,	cbm720,	cbm610,	cbm700,		cbm700,		cbm700,		"Commodore Business Machines Co.",	"Commodore B256-80HP/720",					GAME_NOT_WORKING)
COMPX (1983,	cbm720se,	cbm610,	cbm700,	cbm700,		cbm700,		"Commodore Business Machines Co.",	"Commodore B256-80HP/720 Swedish/Finnish",	GAME_NOT_WORKING)
#if 0
COMPX (1983,	cbm730, cbm610, cbmbx, 		cbmb, 		cbmb, 		"Commodore Business Machines Co.",	"Commodore BX128-80HP/BX256-80HP/730", GAME_NOT_WORKING)
#endif

#ifdef RUNTIME_LOADER
extern void cbmb_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"cbm500")==0) drivers[i]=&driver_cbm500;
		if ( strcmp(drivers[i]->name,"cbm610")==0) drivers[i]=&driver_cbm610;
		if ( strcmp(drivers[i]->name,"cbm620")==0) drivers[i]=&driver_cbm620;
		if ( strcmp(drivers[i]->name,"cbm620hu")==0) drivers[i]=&driver_cbm620hu;
		if ( strcmp(drivers[i]->name,"cbm710")==0) drivers[i]=&driver_cbm710;
		if ( strcmp(drivers[i]->name,"cbm720")==0) drivers[i]=&driver_cbm720;
		if ( strcmp(drivers[i]->name,"cbm720se")==0) drivers[i]=&driver_cbm720se;
	}
}
#endif
