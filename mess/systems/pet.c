/***************************************************************************
	commodore pet series computer

	PeT mess@utanet.at

	documentation
	 vice emulator
     www.funet.fi
	 andre fachat (vice emulator, docu, web site, excellent keyboard pictures)

***************************************************************************/

/*
PET 2000 Series:Renamed to CBM 20XX, XX = RAM, when Philips forbid PET use.
                Most CBM renamed units powered up in lowercase and had a
                different keyboard config, while the PET machines booted in
                uppercase. B and N notation alternately put after RAM amount
                in name (PET 2001B-32 = PET 2001-32B)
                Black (B) or Blue (N) Trim, 9" (9) or 12" (2) screen,
                Built-In Cassette with Chiclet Keys (C),
                Business Style Keyboard with No Graphics on Keys (K), or
                Home Computer with Number Keys and Graphics on Keys (H),
                Green/White screen (G) or Black/White screen (W)
* PET 2001-4K   4kB, CB                                                     GP
* PET 2001-8K   8kB, CN9                                                    GP
* PET 2001-8C   8kB, CN9W, SN#0620733, No "WAIT 6502,X"                     GL
* PET 2001-8C   8kB, CB9G, SN#0629836, No "WAIT 6502,X"                     GL
  PET 2001-16K  16kB, CN9
  PET 2001-32K  32kB, CN9
  PET 2001B-8   8kB, K2
  PET 2001B-16  16kB, K2
  PET 2001B-32  32kB, BK9W, boots in lowercase                              RB
  PET 2001B-32  32kB, K2
  PET 2001N-8   8kB, H2
* PET 2001N-16  16kB, H9                                                    CH
  PET 2001N-16  16kB, H2
* PET 2001N-32  32kB, H, BASIC 4.0,                                         CS
* PET 2001NT    Teacher's PET.  Same as 2001N, just rebadged
* MDS 6500      Modified 2001N-32 with matching 2040 drive.  500 made.      GP

CBM 3000 Series: 40 Col. Screen, BASIC 2.0-2.3, Same Board as Thin 4000
                 3001 series in Germany were just 2001's with big Keyboard.
* CBM 3008      8kB, 9" Screen.                                             EG
* CBM 3016      16kB
* CBM 3032      32kB.                                                       SL

CBM 4000 Thin Series: 9" Screen, 40 Column Only, Basic 4.0.
CBM 4000 Fat Series:  12" Screen, Upgradeable to 80 Column, When upgraded
                      to 80 Columns, the systems were 8000's.
  CBM 4004      4kB, One Piece.
* CBM 4008      8kB, One Piece.                                             SF
* CBM 4016      16kB, One Piece.                                            KK
* CBM 4032      32kB, One Piece                                             JB
* CBM 4064      Educator 64 in 40XX case. green screen (no Fat option)      GP
CBM 8000 Series:12" Screen, 80 Column, BASIC 4.0
                SK means "SoftKey", or "Separated Keyboard"  All -SK and d
                units were enclosed in CBM 700/B series HP cases.
  CBM 8008      8kB, One Piece
  CBM 8016      16kB, One Piece
* CBM 8032      32kB, One Piece                                             GP
* CBM 8032-32 B 8032 in Higher Profile case (HP).  Could install LP drives. GP
* CBM 8032 SK   32kB, Detached Keyboard, SK = SoftKey or Separated Keyboard.EG
  CBM 8096      96kB, 8032 with 64kB ram card
* CBM 8096 SK   96kB, Detached Keyboard.
* CBM 8096d     8096 + 8250LP                                               SL
* CBM 8296      128kB, Detached Keyboard, Brown like 64, LOS-96 OS          TL
* CBM 8296d     8296 + 8250LP                                               SL
* "CASSIE"      Synergistics Inc. rebadged 8032                             AH

SuperPet Series:Sold in Germany as MMF (MicroMainFrame) 9000
                Machines sold in Italy had 134kB of RAM.
* CBM SP9000    Dual uP 6502/6809, 96kB RAM, business keyboard.             GP

CBM 200 Series
* CBM 200       CBM 8032 SK                                                 VM
  CBM 210       ???
* CBM 220       CBM 8096 SK



basically 3 types of motherboards
no crtc (only 60 hz?)
crtc 40 columns ( 50 or 60 hz )
crtc 80 columns ( 60 or 60 hz )
(board version able to do 40 and 80 columns)

3 types of basic roms
basic 1 (only 40 columns, no crtc, with graphics)
basic 2 (only 40 columns version, no crtc)
basic 4

2 types of keyboard and roms
normal (with graphic) (80 columns versions only by 3 parties)
business
different mapping/system roms!

state
-----
keyboard
no sound (were available)
no tape drives
no ieee488 interface
 no floppy disk support
quickloader

Keys
----
Some PC-Keyboards does not behave well when special two or more keys are
pressed at the same time
(with my keyboard printscreen clears the pressed pause key!)

when problems start with -log and look into error.log file
 */

#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "vidhrdw/generic.h"
#include "includes/pet.h"
#include "includes/crtc6845.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
/*#include "includes/vc1541.h" */

static MEMORY_READ_START( pet_readmem )
	{0x0000, 0x7fff, MRA_RAM},
	{0x8000, 0x83ff, MRA_RAM },
	{0xa000, 0xe7ff, MRA_ROM },
	{0xe810, 0xe813, pia_0_r },
	{0xe820, 0xe823, pia_1_r },
	{0xe840, 0xe84f, via_0_r },
/*	{0xe900, 0xe91f, cbm_ieee_state }, // for debugging */
	{0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pet_writemem )
	{0x0000, 0x7fff, MWA_RAM, &pet_memory},
	{0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{0xa000, 0xe7ff, MWA_ROM },
	{0xe810, 0xe813, pia_0_w },
	{0xe820, 0xe823, pia_1_w },
	{0xe840, 0xe84f, via_0_w },
	{0xf000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pet40_readmem )
	{0x0000, 0x7fff, MRA_RAM},
	{0x8000, 0x83ff, MRA_RAM },
	{0xa000, 0xe7ff, MRA_ROM },
	{0xe810, 0xe813, pia_0_r },
	{0xe820, 0xe823, pia_1_r },
	{0xe840, 0xe84f, via_0_r },
	{0xe880, 0xe881, crtc6845_0_port_r },
	{0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( pet40_writemem )
	{0x0000, 0x7fff, MWA_RAM, &pet_memory},
	{0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{0xa000, 0xe7ff, MWA_ROM },
	{0xe810, 0xe813, pia_0_w },
	{0xe820, 0xe823, pia_1_w },
	{0xe840, 0xe84f, via_0_w },
	{0xe880, 0xe881, crtc6845_0_port_w },
	{0xf000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( pet80_readmem )
	{0x0000, 0x7fff, MRA_RAM },
	{0x8000, 0x8fff, MRA_BANK1 },
	{0x9000, 0x9fff, MRA_BANK2 },
	{0xa000, 0xafff, MRA_BANK3 },
	{0xb000, 0xbfff, MRA_BANK4 },
	{0xc000, 0xe7ff, MRA_BANK6 },
#if 1
	{0xe800, 0xefff, MRA_BANK7 },
#else
	{0xe810, 0xe813, pia_0_r },
	{0xe820, 0xe823, pia_1_r },
	{0xe840, 0xe84f, via_0_r },
	{0xe880, 0xe881, crtc6845_0_port_r },
#endif
	{0xf000, 0xffff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( pet80_writemem )
	{0x0000, 0x7fff, MWA_RAM, &pet_memory},
	{0x8000, 0x8fff, MWA_BANK1, &videoram },
	{0x9000, 0x9fff, MWA_BANK2 },
	{0xa000, 0xafff, MWA_BANK3 },
	{0xb000, 0xbfff, MWA_BANK4 },
	{0xc000, 0xe7ff, MWA_BANK6 },
#if 1
	{0xe800, 0xefff, MWA_BANK7 },
#else
	{0xe810, 0xe813, pia_0_w },
	{0xe820, 0xe823, pia_1_w },
	{0xe840, 0xe84f, via_0_w },
	{0xe880, 0xe881, crtc6845_0_port_w },
#endif
	{0xf000, 0xffef, MWA_BANK8 },
    {0xfff1, 0xffff, MWA_BANK9 },
MEMORY_END


/* 0xe880 crtc
   0xefe0 6702 encoder
   0xeff0 acia6551

   0xeff8 super pet system latch
61432        SuperPET system latch
        bit 0    1=6502, 0=6809
        bit 1    0=read only
        bit 3    diagnostic sense: set to 1 to switch to 6502

61436        SuperPET bank select latch
        bit 0-3  bank
        bit 7    1=enable system latch

*/
static MEMORY_READ_START( superpet_readmem )
	{0x0000, 0x7fff, MRA_RAM},
	{0x8000, 0x87ff, MRA_RAM },
	{0xa000, 0xe7ff, MRA_ROM },
	{0xe810, 0xe813, pia_0_r },
	{0xe820, 0xe823, pia_1_r },
	{0xe840, 0xe84f, via_0_r },
	{0xe880, 0xe881, crtc6845_0_port_r },
	/* 0xefe0, 0xefe3, mos 6702 */
	/* 0xeff0, 0xeff3, acia6551 */
	{0xeff8, 0xefff, superpet_r },
	{0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( superpet_writemem )
	{0x0000, 0x7fff, MWA_RAM, &pet_memory},
	{0x8000, 0x87ff, videoram_w, &videoram, &videoram_size },
	{0xa000, 0xe7ff, MWA_ROM },
	{0xe810, 0xe813, pia_0_w },
	{0xe820, 0xe823, pia_1_w },
	{0xe840, 0xe84f, via_0_w },
	{0xe880, 0xe881, crtc6845_0_port_w },
	{0xeff8, 0xefff, superpet_w },
	{0xf000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START( superpet_m6809_readmem)
	{0x0000, 0x7fff, MRA_BANK1,},
	{0x8000, 0x87ff, MRA_BANK2 },
    {0x9000, 0x9fff, MRA_BANK3 },
	{0xa000, 0xe7ff, MRA_ROM },
	{0xe810, 0xe813, pia_0_r },
	{0xe820, 0xe823, pia_1_r },
	{0xe840, 0xe84f, via_0_r },
	{0xe880, 0xe881, crtc6845_0_port_r },
	{0xeff8, 0xefff, superpet_r },
	{0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( superpet_m6809_writemem )
	{0x0000, 0x7fff, MWA_BANK1 }, /* same memory as m6502 */
	{0x8000, 0x87ff, videoram_w }, /* same memory as m6502 */
    {0x9000, 0x9fff, MWA_BANK3 }, /* 64 kbyte ram turned in */
	{0xa000, 0xe7ff, MWA_ROM },
	{0xe810, 0xe813, pia_0_w },
	{0xe820, 0xe823, pia_1_w },
	{0xe840, 0xe84f, via_0_w },
	{0xe880, 0xe881, crtc6845_0_port_w },
	{0xeff8, 0xefff, superpet_w },
	{0xf000, 0xffff, MWA_ROM },
MEMORY_END

#define DIPS_HELPER(bit, name, keycode) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, IP_JOY_NONE)

#define PET_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "At", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "!", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "\"", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "#", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "$", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "%", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "'", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "&", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "\\", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "(", KEYCODE_9)\
	DIPS_HELPER( 0x0020, ")", KEYCODE_0)\
	DIPS_HELPER( 0x0010, "Arrow-Left", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "[", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "]", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0002, "RVS OFF", KEYCODE_TAB)\
	DIPS_HELPER( 0x0001, "Q", KEYCODE_Q)\
	PORT_START \
	DIPS_HELPER( 0x8000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x4000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x2000, "R", KEYCODE_R)\
	DIPS_HELPER( 0x1000, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0800, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0400, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0200, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0100, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0080, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0040, "Arrow-Up Pi", KEYCODE_OPENBRACE)\
    DIPS_HELPER( 0x0020, "<", KEYCODE_CLOSEBRACE)\
    DIPS_HELPER( 0x0010, ">", KEYCODE_BACKSLASH)\
	PORT_BITX( 0x0008, IP_ACTIVE_HIGH, IPF_TOGGLE,\
		     "(left shift)SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE)\
	DIPS_HELPER( 0x0004, "A", KEYCODE_A)\
	DIPS_HELPER( 0x0002, "S", KEYCODE_S)\
	DIPS_HELPER( 0x0001, "D", KEYCODE_D)\
	PORT_START \
	DIPS_HELPER( 0x8000, "F", KEYCODE_F)\
	DIPS_HELPER( 0x4000, "G", KEYCODE_G)\
	DIPS_HELPER( 0x2000, "H", KEYCODE_H)\
	DIPS_HELPER( 0x1000, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0800, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0400, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0200, ":", KEYCODE_COLON)\
	DIPS_HELPER( 0x0100, "STOP RUN", KEYCODE_QUOTE)\
    DIPS_HELPER( 0x0080, "RETURN",KEYCODE_ENTER)\
	DIPS_HELPER( 0x0040, "Left-Shift", KEYCODE_LSHIFT)\
	DIPS_HELPER( 0x0020, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x0010, "X", KEYCODE_X)\
	DIPS_HELPER( 0x0008, "C", KEYCODE_C)\
	DIPS_HELPER( 0x0004, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0002, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0001, "N", KEYCODE_N)\
	PORT_START \
	DIPS_HELPER( 0x8000, "M", KEYCODE_M)\
	DIPS_HELPER( 0x4000, ",", KEYCODE_COMMA)\
	DIPS_HELPER( 0x2000, ";", KEYCODE_STOP)\
	DIPS_HELPER( 0x1000, "?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0800, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0400, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0200, "HOME CLR", KEYCODE_HOME)\
	DIPS_HELPER( 0x0100, "DOWN UP", KEYCODE_DOWN)\
	DIPS_HELPER( 0x0080, "RIGHT LEFT", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0040, "DEL INST", KEYCODE_DEL)\
	DIPS_HELPER( 0x0020, "NUM 7", KEYCODE_7_PAD)\
	DIPS_HELPER( 0x0010, "NUM 8", KEYCODE_8_PAD)\
	DIPS_HELPER( 0x0008, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x0004, "NUM /", KEYCODE_SLASH_PAD)\
	DIPS_HELPER( 0x0002, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x0001, "NUM 5", KEYCODE_5_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x4000, "NUM *", KEYCODE_ASTERISK)\
	DIPS_HELPER( 0x2000, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x1000, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0800, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0400, "NUM +", KEYCODE_PLUS_PAD)\
	DIPS_HELPER( 0x0200, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0100, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0080, "NUM -", KEYCODE_MINUS_PAD)\
	DIPS_HELPER( 0x0040, "NUM =", KEYCODE_ENTER_PAD)\
	DIPS_HELPER( 0x0020, "(right-shift cursor-down)Special CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0010, "(right-shift cursor-right)Special CRSR Left", KEYCODE_LEFT)\

#define PET_B_KEYBOARD \
	PORT_START \
	DIPS_HELPER( 0x8000, "Arrow-Left", KEYCODE_TILDE)\
	DIPS_HELPER( 0x4000, "1 !", KEYCODE_1)\
	DIPS_HELPER( 0x2000, "2 \"", KEYCODE_2)\
	DIPS_HELPER( 0x1000, "3 #", KEYCODE_3)\
	DIPS_HELPER( 0x0800, "4 $", KEYCODE_4)\
	DIPS_HELPER( 0x0400, "5 %", KEYCODE_5)\
	DIPS_HELPER( 0x0200, "6 &", KEYCODE_6)\
	DIPS_HELPER( 0x0100, "7 '", KEYCODE_7)\
	DIPS_HELPER( 0x0080, "8 (", KEYCODE_8)\
	DIPS_HELPER( 0x0040, "9 )", KEYCODE_9)\
	DIPS_HELPER( 0x0020, "0", KEYCODE_0)\
	DIPS_HELPER( 0x0010, ": *", KEYCODE_MINUS)\
	DIPS_HELPER( 0x0008, "- =", KEYCODE_EQUALS)\
	DIPS_HELPER( 0x0004, "Arrow-Up", KEYCODE_BACKSPACE)\
	DIPS_HELPER( 0x0002, "CRSR RIGHT LEFT", KEYCODE_RIGHT)\
	DIPS_HELPER( 0x0001, "STOP RUN", KEYCODE_END)\
	PORT_START \
	DIPS_HELPER( 0x8000, "TAB", KEYCODE_TAB)\
	DIPS_HELPER( 0x4000, "Q", KEYCODE_Q)\
	DIPS_HELPER( 0x2000, "W", KEYCODE_W)\
	DIPS_HELPER( 0x1000, "E", KEYCODE_E)\
	DIPS_HELPER( 0x0800, "R", KEYCODE_R)\
	DIPS_HELPER( 0x0400, "T", KEYCODE_T)\
	DIPS_HELPER( 0x0200, "Y", KEYCODE_Y)\
	DIPS_HELPER( 0x0100, "U", KEYCODE_U)\
	DIPS_HELPER( 0x0080, "I", KEYCODE_I)\
	DIPS_HELPER( 0x0040, "O", KEYCODE_O)\
	DIPS_HELPER( 0x0020, "P", KEYCODE_P)\
	DIPS_HELPER( 0x0010, "[", KEYCODE_OPENBRACE)\
    DIPS_HELPER( 0x0008, "\\", KEYCODE_CLOSEBRACE)\
    DIPS_HELPER( 0x0004, "CRSR DOWN UP", KEYCODE_DOWN)\
    DIPS_HELPER( 0x0002, "DEL INST", KEYCODE_DEL)\
    DIPS_HELPER( 0x0001, "ESC",KEYCODE_ESC)\
	PORT_START \
	PORT_BITX( 0x8000, IP_ACTIVE_HIGH, IPF_TOGGLE,\
		     "(left shift)SHIFT-LOCK (switch)", KEYCODE_CAPSLOCK, IP_JOY_NONE)\
	DIPS_HELPER( 0x4000, "A", KEYCODE_A)\
	DIPS_HELPER( 0x2000, "S", KEYCODE_S)\
	DIPS_HELPER( 0x1000, "D", KEYCODE_D)\
	DIPS_HELPER( 0x0800, "F", KEYCODE_F)\
	DIPS_HELPER( 0x0400, "G", KEYCODE_G)\
	DIPS_HELPER( 0x0200, "H", KEYCODE_H)\
	DIPS_HELPER( 0x0100, "J", KEYCODE_J)\
	DIPS_HELPER( 0x0080, "K", KEYCODE_K)\
	DIPS_HELPER( 0x0040, "L", KEYCODE_L)\
	DIPS_HELPER( 0x0020, "; +", KEYCODE_COLON)\
	DIPS_HELPER( 0x0010, "At", KEYCODE_QUOTE)\
	DIPS_HELPER( 0x0008, "]", KEYCODE_BACKSLASH)\
    DIPS_HELPER( 0x0004, "RETURN",KEYCODE_ENTER)\
    DIPS_HELPER( 0x0002, "RVS Off",KEYCODE_INSERT)\
	DIPS_HELPER( 0x0001, "Left-Shift", KEYCODE_LSHIFT)\
	PORT_START \
	DIPS_HELPER( 0x8000, "Z", KEYCODE_Z)\
	DIPS_HELPER( 0x4000, "X", KEYCODE_X)\
	DIPS_HELPER( 0x2000, "C", KEYCODE_C)\
	DIPS_HELPER( 0x1000, "V", KEYCODE_V)\
	DIPS_HELPER( 0x0800, "B", KEYCODE_B)\
	DIPS_HELPER( 0x0400, "N", KEYCODE_N)\
	DIPS_HELPER( 0x0200, "M", KEYCODE_M)\
	DIPS_HELPER( 0x0100, ", <", KEYCODE_COMMA)\
	DIPS_HELPER( 0x0080, ". >", KEYCODE_STOP)\
	DIPS_HELPER( 0x0040, "/ ?", KEYCODE_SLASH)\
	DIPS_HELPER( 0x0020, "Right-Shift", KEYCODE_RSHIFT)\
	DIPS_HELPER( 0x0010, "REPEAT", KEYCODE_LALT)\
	DIPS_HELPER( 0x0008, "HOME CLR", KEYCODE_HOME)\
	DIPS_HELPER( 0x0004, "Space", KEYCODE_SPACE)\
	DIPS_HELPER( 0x0002, "NUM 7", KEYCODE_7_PAD)\
	DIPS_HELPER( 0x0001, "NUM 8", KEYCODE_8_PAD)\
	PORT_START \
	DIPS_HELPER( 0x8000, "NUM 9", KEYCODE_9_PAD)\
	DIPS_HELPER( 0x4000, "NUM 4", KEYCODE_4_PAD)\
	DIPS_HELPER( 0x2000, "NUM 5", KEYCODE_5_PAD)\
	DIPS_HELPER( 0x1000, "NUM 6", KEYCODE_6_PAD)\
	DIPS_HELPER( 0x0800, "NUM 1", KEYCODE_1_PAD)\
	DIPS_HELPER( 0x0400, "NUM 2", KEYCODE_2_PAD)\
	DIPS_HELPER( 0x0200, "NUM 3", KEYCODE_3_PAD)\
	DIPS_HELPER( 0x0100, "NUM 0", KEYCODE_0_PAD)\
	DIPS_HELPER( 0x0080, "NUM .", KEYCODE_DEL_PAD)\
	DIPS_HELPER( 0x0040, "(right-shift cursor-down)Special CRSR Up", KEYCODE_UP)\
	DIPS_HELPER( 0x0020, "(right-shift cursor-right)Special CRSR Left", KEYCODE_LEFT)\

INPUT_PORTS_START (pet)
	PET_KEYBOARD
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
	PORT_BIT (0x200, 0x000, IPT_UNUSED) /* normal keyboard/bios */
	PORT_DIPNAME   ( 0x180, 0x180, "Memory")
	PORT_DIPSETTING(  0, "4KByte" )
	PORT_DIPSETTING(  0x80, "8KByte" )
	PORT_DIPSETTING(  0x100, "16KByte" )
	PORT_DIPSETTING(  0x180, "32KByte" )
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (petb)
	PET_B_KEYBOARD
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
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_DIPNAME   ( 0x180, 0x180, "Memory")
	PORT_DIPSETTING(  0, "4KByte" )
	PORT_DIPSETTING(  0x80, "8KByte" )
	PORT_DIPSETTING(  0x100, "16KByte" )
	PORT_DIPSETTING(  0x180, "32KByte" )
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (cbm8096)
	PET_B_KEYBOARD
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
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_BIT (0x180, 0x180, IPT_UNUSED) /* 32 kb Memory */
	PORT_DIPNAME ( 0x08, 0x08, "CBM8096, 8296 Expansion Memory")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x08, DEF_STR( Yes ) )
	PORT_BIT (4, 0, IPT_UNUSED) /* no superpet switch */
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START (superpet)
	PET_B_KEYBOARD
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
	PORT_BIT (0x200, 0x200, IPT_UNUSED) /* business keyboard/bios */
	PORT_BIT (0x180, 0x180, IPT_UNUSED) /* 32KByte */
	PORT_BIT (8, 0, IPT_UNUSED) /* no 8096 hardware */
	PORT_DIPNAME   ( 4, 4, "CPU Select")
	PORT_DIPSETTING( 0, "M6502" )
	PORT_DIPSETTING( 4, "M6809" )
	PORT_DIPNAME ( 0x02, 0x02, "IEEE488 Bus/Dev 8/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(0x02, DEF_STR( Yes ) )
	PORT_DIPNAME ( 0x01, 0x00, "IEEE488 Bus/Dev 9/Floppy Sim")
	PORT_DIPSETTING(  0, DEF_STR( No ) )
	PORT_DIPSETTING(  1, DEF_STR( Yes ) )
INPUT_PORTS_END

unsigned char pet_palette[] =
{
	0,0,0, /* black */
	0,0x80,0, /* green */
};

static unsigned short pet_colortable[][2] = {
	{ 0, 1 },
	/* reverse */
	{ 1, 0 }
};

static struct GfxLayout pet_charlayout =
{
        8,8,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,1,2,3,4,5,6,7 },
        /* y offsets */
        { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
        },
        8*8
};

static struct GfxLayout pet80_charlayout =
{
        8,16,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 0,1,2,3,4,5,6,7 },
        /* y offsets */
        {
			0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
        },
        8*16
};

static struct GfxDecodeInfo pet_gfxdecodeinfo[] = {
	{ 1, 0x0000, &pet_charlayout,                     0, 1 },
	{ 1, 0x0800, &pet_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static struct GfxDecodeInfo pet80_gfxdecodeinfo[] = {
	{ 1, 0x0000, &pet80_charlayout,                     0, 1 },
	{ 1, 0x1000, &pet80_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static struct GfxDecodeInfo superpet_gfxdecodeinfo[] = {
	{ 2, 0x0000, &pet80_charlayout,                     0, 1 },
	{ 2, 0x1000, &pet80_charlayout,                     0, 1 },
	{ 2, 0x2000, &pet80_charlayout,                     0, 1 },
	{ 2, 0x3000, &pet80_charlayout,                     0, 1 },
    { -1 } /* end of array */
};

static void pet_init_palette (unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, pet_palette, sizeof (pet_palette));
    memcpy(sys_colortable,pet_colortable,sizeof(pet_colortable));
}

/* basic 1 */
ROM_START (pet)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901447.09", 0xc000, 0x800, 0x03cf16d0)
    ROM_LOAD ("901447.02", 0xc800, 0x800, 0x69fd8a8f)
    ROM_LOAD ("901447.03", 0xd000, 0x800, 0xd349f2d4)
    ROM_LOAD ("901447.04", 0xd800, 0x800, 0x850544eb)
    ROM_LOAD ("901447.05", 0xe000, 0x800, 0x9e1c5cea)
    ROM_LOAD ("901447.06", 0xf000, 0x800, 0x661a814a)
    ROM_LOAD ("901447.07", 0xf800, 0x800, 0xc4f47ad1)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, 0x54f32f45)
ROM_END

/* basic 2 */
ROM_START (pet2)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.01", 0xc000, 0x1000, 0x63a7fe4a)
    ROM_LOAD ("901465.02", 0xd000, 0x1000, 0xae4cb035)
    ROM_LOAD ("901447.24", 0xe000, 0x800, 0xe459ab32)
    ROM_LOAD ("901465.03", 0xf000, 0x1000, 0xf02238e2)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, 0x54f32f45)
ROM_END

/* basic 2 business */
ROM_START (pet2b)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.01", 0xc000, 0x1000, 0x63a7fe4a)
    ROM_LOAD ("901465.02", 0xd000, 0x1000, 0xae4cb035)
    ROM_LOAD ("901474.01", 0xe000, 0x800, 0x05db957e)
    ROM_LOAD ("901465.03", 0xf000, 0x1000, 0xf02238e2)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, 0xd8408674)
ROM_END

/* basic 4 business */
ROM_START (pet4b)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901474.02", 0xe000, 0x800, 0x75ff4af7)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, 0xd8408674)
ROM_END

/* basic 4 crtc*/
ROM_START (pet4)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901499.01", 0xe000, 0x800, 0x5f85bdf8)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, 0x54f32f45)
ROM_END

/* basic 4 crtc 50 hz */
ROM_START (pet4pal)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901498.01", 0xe000, 0x800, 0x3370e359)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x1000, REGION_GFX1, 0)
    ROM_LOAD ("901447.08", 0x0000, 0x800, 0x54f32f45)
ROM_END

/* basic 4 business 80 columns */
ROM_START (pet80)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901474.03", 0xe000, 0x800, 0x5674dd5e)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, 0xd8408674)
ROM_END

/* basic 4 business 80 columns 50 hz */
ROM_START (pet80pal)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901474.04", 0xe000, 0x800, 0xabb000e7)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.10", 0x0000, 0x800, 0xd8408674)
ROM_END

ROM_START (cbm80ger)
	ROM_REGION (0x20000, REGION_CPU1, 0)
	ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
	ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
	ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
	ROM_LOAD ("german.bin", 0xe000, 0x800, 0x1c1e597d)
	ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x2000, REGION_GFX1, 0)
	ROM_LOAD ("chargen.de", 0x0000, 0x800, 0x3bb8cb87)
ROM_END

ROM_START (cbm80swe)
	ROM_REGION (0x20000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("editswe.bin", 0xe000, 0x800, 0x75901dd7)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x2000, REGION_GFX1, 0)
    ROM_LOAD ("901447.14", 0x0000, 0x800, 0x48c77d29)
ROM_END

ROM_START (superpet)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("901474.04", 0xe000, 0x800, 0xabb000e7)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x20000, REGION_CPU2, 0)
    ROM_LOAD ("901898.01", 0xa000, 0x1000, 0x728a998b)
    ROM_LOAD ("901898.02", 0xb000, 0x1000, 0x6beb7c62)
    ROM_LOAD ("901898.03", 0xc000, 0x1000, 0x5db4983d)
    ROM_LOAD ("901898.04", 0xd000, 0x1000, 0xf55fc559)
    ROM_LOAD ("901897.01", 0xe000, 0x800, 0xb2cee903)
    ROM_LOAD ("901898.05", 0xf000, 0x1000, 0xf42df0cb)
	ROM_REGION (0x4000, REGION_GFX1, 0)
    ROM_LOAD ("901640.01", 0x0000, 0x1000, 0xee8229c4)
ROM_END

/* swedish m6809 roms needed */
ROM_START (mmf9000)
	ROM_REGION (0x10000, REGION_CPU1, 0)
    ROM_LOAD ("901465.23", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("901465.20", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("901465.21", 0xd000, 0x1000, 0x36d91855)
    ROM_LOAD ("editswe.bin", 0xe000, 0x800, 0x75901dd7)
    ROM_LOAD ("901465.22", 0xf000, 0x1000, 0xcc5298a1)
	ROM_REGION (0x20000, REGION_CPU2, 0)
    ROM_LOAD ("901898.01", 0xa000, 0x1000, 0x728a998b)
    ROM_LOAD ("901898.02", 0xb000, 0x1000, 0x6beb7c62)
    ROM_LOAD ("901898.03", 0xc000, 0x1000, 0x5db4983d)
    ROM_LOAD ("901898.04", 0xd000, 0x1000, 0xf55fc559)
    ROM_LOAD ("901897.01", 0xe000, 0x800, 0xb2cee903)
    ROM_LOAD ("901898.05", 0xf000, 0x1000, 0xf42df0cb)
	ROM_REGION (0x4000, REGION_GFX1, 0)
    ROM_LOAD ("charswe.bin", 0x0000, 0x1000, 0xda1cd630)
ROM_END

#if 0
/* in c16 and some other commodore machines:
   cbm version in kernel at 0xff80 (offset 0x3f80)
   0x80 means pal version */

    /* 901447-09 + 901447-02 + 901447-03 + 901447-04 */
    ROM_LOAD ("basic1", 0xc000, 0x2000, 0xaff78300)
    /* same as 901439-01, maybe same as 6540-011 */
    ROM_LOAD ("rom-1-c000.901447-01.bin", 0xc000, 0x800, 0xa055e33a)
    /* same as 901439-09, 6540-019 */
    ROM_LOAD ("rom-1-c000.901447-09.bin", 0xc000, 0x800, 0x03cf16d0)
    /* same as 901439-05, 6540-012 */
    ROM_LOAD ("rom-1-c800.901447-02.bin", 0xc800, 0x800, 0x69fd8a8f)
    /* same as 901439-02, 6540-013 */
    ROM_LOAD ("rom-1-d000.901447-03.bin", 0xd000, 0x800, 0xd349f2d4)
    /* same as 901439-06, 6540-014 */
    ROM_LOAD ("rom-1-d800.901447-04.bin", 0xd800, 0x800, 0x850544eb)

	/* 901465-01 + 901465-02 */
    ROM_LOAD ("basic2", 0xc000, 0x2000, 0xcf35e68b)
    /* 6540-020 + 6540-021 */
    ROM_LOAD ("basic-2-c000.901465-01.bin", 0xc000, 0x1000, 0x63a7fe4a)
    /* 6540-022 + 6540-023 */
    ROM_LOAD ("basic-2-d000.901465-02.bin", 0xd000, 0x1000, 0xae4cb035)

	/* 901465-23 901465-20 901465-21 */
    ROM_LOAD ("basic4", 0xb000, 0x3000, 0x2a940f0a)
    ROM_LOAD ("basic-4-b000.901465-19.bin", 0xb000, 0x1000, 0x3a5f5721)
    ROM_LOAD ("basic-4-b000.901465-23.bin", 0xb000, 0x1000, 0xae3deac0)
    ROM_LOAD ("basic-4-c000.901465-20.bin", 0xc000, 0x1000, 0x0fc17b9c)
    ROM_LOAD ("basic-4-d000.901465-21.bin", 0xd000, 0x1000, 0x36d91855)

    /* same as 901439-03, 6540-015 */
    ROM_LOAD ("rom-1-e000.901447-05.bin", 0xe000, 0x800, 0x9e1c5cea)

    ROM_LOAD ("edit-2-b.901474-01.bin", 0xe000, 0x800, 0x05db957e)
    /* same as 6540-024 */
    ROM_LOAD ("edit-2-n.901447-24.bin", 0xe000, 0x800, 0xe459ab32)

    ROM_LOAD ("edit-4-40-n-50hz.901498-01.bin", 0xe000, 0x800, 0x3370e359)
    ROM_LOAD ("edit-4-40-n-60hz.901499-01.bin", 0xe000, 0x800, 0x5f85bdf8)
    ROM_LOAD ("edit-4-b.901474-02.bin", 0xe000, 0x800, 0x75ff4af7)

    ROM_LOAD ("edit-4-80-b-60hz.901474-03.bin", 0xe000, 0x800, 0x5674dd5e)
    /* week 36 year 81 */
    ROM_LOAD ("edit-4-80-b-50hz.901474-04-3681.bin", 0xe000, 0x800, 0xc1ffca3a)
    ROM_LOAD ("edit-4-80-b-50hz.901474-04.bin", 0xe000, 0x800, 0xabb000e7)
    ROM_LOAD ("edit-4-80-b-50hz.901474-04?.bin", 0xe000, 0x800, 0x845a44e6)
    ROM_LOAD ("edit-4-80-b-50hz.german.bin", 0xe000, 0x800, 0x1c1e597d)
    ROM_LOAD ("edit-4-80-b-50hz.swedish.bin", 0xe000, 0x800, 0x75901dd7)

	/* 901447-06 + 901447-07 */
    ROM_LOAD ("kernal1", 0xf000, 0x1000, 0xf0186492)
    /* same as 901439-04, 6540-016 */
    ROM_LOAD ("rom-1-f000.901447-06.bin", 0xf000, 0x800, 0x661a814a)
    /* same as 904139-07, 6540-018 */
    ROM_LOAD ("rom-1-f800.901447-07.bin", 0xf800, 0x800, 0xc4f47ad1)

    ROM_LOAD ("kernal-2.901465-03.bin", 0xf000, 0x1000, 0xf02238e2)

    ROM_LOAD ("kernal-4.901465-22.bin", 0xf000, 0x1000, 0xcc5298a1)

	/* graphics */
    /* 6540-010 = 901439-08 */

    ROM_LOAD ("characters-1.901447-08.bin", 0x0000, 0x800, 0x54f32f45)
	/* business */
	/* vice chargen */
    ROM_LOAD ("characters-2.901447-10.bin", 0x0000, 0x800, 0xd8408674)
    ROM_LOAD ("chargen.de", 0x0000, 0x800, 0x3bb8cb87)
    ROM_LOAD ("characters-hungarian.bin", 0x0000, 0x800, 0xa02d8122)
    ROM_LOAD ("characters-swedish.901447-14.bin", 0x0000, 0x800, 0x48c77d29)

    ROM_LOAD ("", 0xe000, 0x800, 0x)

	/* editor rom */
    ROM_LOAD ("Execudesk.bin", 0xe000, 0x1000, 0xbef0eaa1)

    ROM_LOAD ("PaperClip.bin", 0xa000, 0x1000, 0x8fb11d4b)

	/* superpet */
    ROM_LOAD ("waterloo-a000.901898-01.bin", 0xa000, 0x1000, 0x728a998b)
    ROM_LOAD ("waterloo-b000.901898-02.bin", 0xb000, 0x1000, 0x6beb7c62)
    ROM_LOAD ("waterloo-c000.901898-03.bin", 0xc000, 0x1000, 0x5db4983d)
    ROM_LOAD ("waterloo-d000.901898-04.bin", 0xd000, 0x1000, 0xf55fc559)
    ROM_LOAD ("waterloo-e000.901897-01.bin", 0xe000, 0x800, 0xb2cee903)
    ROM_LOAD ("waterloo-f000.901898-05.bin", 0xf000, 0x1000, 0xf42df0cb)
    /* 256 chars commodore pet, 256 chars ascii m6809 */
    ROM_LOAD ("characters.901640-01.bin", 0x0000, 0x1000, 0xee8229c4)
    /* 901447-14 and the 256 chars ascii from 901640-01 */
    ROM_LOAD ("characters.swedish.bin", 0x0000, 0x1000, 0xda1cd630)

	/* scrap */
	/* fixed bits */
    ROM_LOAD ("324878-01.bin", 0x?000, 0x2000, 0xd262bacd)
    ROM_LOAD ("324878-02.bin", 0x?000, 0x2000, 0x5e00476d)
#endif

static struct MachineDriver machine_driver_pet =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			pet_readmem, pet_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	320,							   /* screen width */
	200,							   /* screen height */
	{0, 320 - 1, 0, 200 - 1},		   /* visible_area */
	pet_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	pet_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static struct MachineDriver machine_driver_pet40 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			pet40_readmem, pet40_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	320,							   /* screen width */
	200,							   /* screen height */
	{0, 320 - 1, 0, 200 - 1},		   /* visible_area */
	pet_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	pet40_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static struct MachineDriver machine_driver_pet40pal =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			pet40_readmem, pet40_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	320,							   /* screen width */
	200,							   /* screen height */
	{0, 320 - 1, 0, 200 - 1},		   /* visible_area */
	pet_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	pet40_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static struct MachineDriver machine_driver_pet80 =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			pet80_readmem, pet80_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	640,							   /* screen width */
	250,							   /* screen height */
	{0, 640 - 1, 0, 250 - 1},		   /* visible_area */
	pet80_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
#ifdef PET_TEST_CODE
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_PIXEL_ASPECT_RATIO_1_2|VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#endif
	0,
	generic_vh_start,
	generic_vh_stop,
	pet80_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static struct MachineDriver machine_driver_pet80pal =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			pet80_readmem, pet80_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	640,							   /* screen width */
	250,							   /* screen height */
	{0, 640 - 1, 0, 250 - 1},		   /* visible_area */
	pet80_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
#ifdef PET_TEST_CODE
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_PIXEL_ASPECT_RATIO_1_2|VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#endif
	0,
	generic_vh_start,
	generic_vh_stop,
	pet80_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static struct MachineDriver machine_driver_superpet =
{
  /* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,
			superpet_readmem, superpet_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
		{
			CPU_M6809,
			1000000,
			superpet_m6809_readmem, superpet_m6809_writemem,
			0, 0,
			pet_frame_interrupt, 1,
		},
	},
	50, DEFAULT_REAL_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	0,
	pet_init_machine,
	pet_shutdown_machine,

  /* video hardware */
	640,							   /* screen width */
	250,							   /* screen height */
	{0, 640 - 1, 0, 250 - 1},		   /* visible_area */
	superpet_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (pet_palette) / sizeof (pet_palette[0]) / 3,
	sizeof (pet_colortable) / sizeof(pet_colortable[0][0]),
	pet_init_palette,				   /* convert color prom */
#ifdef PET_TEST_CODE
	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#else
	VIDEO_PIXEL_ASPECT_RATIO_1_2|VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
#endif
	0,
	generic_vh_start,
	generic_vh_stop,
	pet80_vh_screenrefresh,

  /* sound hardware */
	0, 0, 0, 0,
	{
		{ 0 }
	}
};

static const struct IODevice io_pet[] =
{
	IODEVICE_CBM_PET1_QUICK,
	IODEVICE_CBM_ROM("crt\0a0\0b0\0"),
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

static const struct IODevice io_pet2[] =
{
	IODEVICE_CBM_PET_QUICK,
	IODEVICE_CBM_ROM("crt\0a0\0b0\0"),
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

static const struct IODevice io_pet4[] =
{
	IODEVICE_CBM_PET_QUICK,
	IODEVICE_CBM_ROM("crt\0a0\0"),
	IODEVICE_CBM_DRIVE,
	{IO_END}
};

#define init_pet1 pet_basic1_driver_init
#define init_pet pet_driver_init
#define init_pet40 pet40_driver_init
#define init_cbm80 cbm80_driver_init
#define init_superpet superpet_driver_init

#define io_cbm30 io_pet2
#define io_cbm30b io_pet2
#define io_cbm40 io_pet4
#define io_cbm40pal io_pet4
#define io_cbm40b io_pet4
#define io_cbm80 io_pet4
#define io_cbm80pal io_pet4
#define io_cbm80ger io_pet4
#define io_cbm80swe io_pet4
#define io_superpet io_pet4
#define io_mmf9000 io_pet4

#define rom_cbm30 rom_pet2
#define rom_cbm30 rom_pet2
#define rom_cbm30b rom_pet2b
#define rom_cbm40 rom_pet4
#define rom_cbm40pal rom_pet4pal
#define rom_cbm40b rom_pet4b
#define rom_cbm80 rom_pet80
#define rom_cbm80pal rom_pet80pal

/*	   YEAR 	NAME		PARENT	MACHINE 	INPUT	 INIT	  COMPANY							  FULLNAME */
COMPX (1977,	pet,		0,		pet,		pet,	 pet1,	  "Commodore Business Machines Co.",  "PET2001/CBM20xx Series (Basic 1)",            GAME_NO_SOUND)
COMPX (1979,	cbm30,		pet,	pet,		pet,	 pet,	  "Commodore Business Machines Co.",  "Commodore 30xx (Basic 2)",                    GAME_NO_SOUND)
COMPX (1979,	cbm30b, 	pet,	pet,		petb,	 pet,	  "Commodore Business Machines Co.",  "Commodore 30xx (Basic 2) (business keyboard)",GAME_NO_SOUND)
COMPX (198?,	cbm40,		pet,	pet40,		pet,	 pet40,   "Commodore Business Machines Co.",  "Commodore 40xx FAT (CRTC) 60Hz",              GAME_NO_SOUND)
COMPX (198?,	cbm40pal,	pet,	pet40pal,	pet,	 pet40,   "Commodore Business Machines Co.",  "Commodore 40xx FAT (CRTC) 50Hz",              GAME_NO_SOUND)
COMPX (198?,	cbm40b, 	pet,	pet,		petb,	 pet,	  "Commodore Business Machines Co.",  "Commodore 40xx THIN (business keyboard)",     GAME_NO_SOUND)
COMPX (1980,	cbm80,		pet,	pet80,		cbm8096, cbm80,   "Commodore Business Machines Co.",  "Commodore 80xx 60Hz",                         GAME_NO_SOUND)
COMPX (198?,	cbm80pal,	pet,	pet80pal,	cbm8096, cbm80,   "Commodore Business Machines Co.",  "Commodore 80xx 50Hz",                         GAME_NO_SOUND)
COMPX (198?,	cbm80ger,	pet,	pet80pal,	cbm8096, cbm80,   "Commodore Business Machines Co.",  "Commodore 80xx German (50Hz)",                GAME_NO_SOUND)
COMPX (198?,	cbm80swe,	pet,	pet80pal,	cbm8096, cbm80,   "Commodore Business Machines Co.",  "Commodore 80xx Swedish (50Hz)",               GAME_NO_SOUND)
COMPX (198?,	superpet,	pet,	superpet,	superpet,superpet,"Commodore Business Machines Co.",  "Commodore SP9000/MMF9000 (50Hz)",             GAME_NO_SOUND|GAME_NOT_WORKING)

// please leave the following as testdriver only
COMP (198?, 	mmf9000,	pet,	superpet,	superpet,superpet,"Commodore Business Machines Co.",  "MMF9000 (50Hz) Swedish")

#ifdef RUNTIME_LOADER
extern void pet_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"pet")==0) drivers[i]=&driver_pet;
		if ( strcmp(drivers[i]->name,"cbm30")==0) drivers[i]=&driver_cbm30;
		if ( strcmp(drivers[i]->name,"cbm30b")==0) drivers[i]=&driver_cbm30b;
		if ( strcmp(drivers[i]->name,"cbm40")==0) drivers[i]=&driver_cbm40;
		if ( strcmp(drivers[i]->name,"cbm40pal")==0) drivers[i]=&driver_cbm40pal;
		if ( strcmp(drivers[i]->name,"cbm40b")==0) drivers[i]=&driver_cbm40b;
		if ( strcmp(drivers[i]->name,"cbm80")==0) drivers[i]=&driver_cbm80;
		if ( strcmp(drivers[i]->name,"cbm80pal")==0) drivers[i]=&driver_cbm80pal;
		if ( strcmp(drivers[i]->name,"cbm80ger")==0) drivers[i]=&driver_cbm80ger;
		if ( strcmp(drivers[i]->name,"cbm80swe")==0) drivers[i]=&driver_cbm80swe;
		if ( strcmp(drivers[i]->name,"superpet")==0) drivers[i]=&driver_superpet;
		if ( strcmp(drivers[i]->name,"mmf9000")==0) drivers[i]=&driver_mmf9000;
	}
}
#endif
