/******************************************************************************

 Mathis Rosenhauer
 Nate Woods

 TODO:
   - Support configurable RAM sizes:
     - coco & cocoe should allow 4k/16k/32k/64k
	 - coco2 & coco2b should allow 16k/64k
	 - coco3 & coco3h should allow 128k/512k (or even 2mb/8mb to show the hacks)
   - All systems capable of disk support (i.e. - not original coco) should also
     support DECB 1.0
 ******************************************************************************/
#include "driver.h"
#include "machine/6821pia.h"
#include "vidhrdw/m6847.h"
#include "includes/6883sam.h"
#include "includes/rstrtrck.h"
#include "includes/dragon.h"
#include "formats/dmkdsk.h"
#include "includes/basicdsk.h"
#include "printer.h"

static MEMORY_READ_START( dragon32_readmem )
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xfeff, MRA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, coco_pia_1_r },
	{ 0xff40, 0xff5f, coco_cartridge_r },
	{ 0xff60, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
MEMORY_END

static MEMORY_WRITE_START( dragon32_writemem )
	{ 0x0000, 0x7fff, coco_ram_w },
	{ 0x8000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xfeff, MWA_ROM }, /* cart area */
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff5f, coco_cartridge_w },
	{ 0xff60, 0xffbf, MWA_NOP },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP },
MEMORY_END

static MEMORY_READ_START( d64_readmem )
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0xfeff, MRA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, coco_pia_1_r },
	{ 0xff40, 0xff8f, coco_cartridge_r },
	{ 0xff90, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, dragon_mapped_irq_r },
MEMORY_END

static MEMORY_WRITE_START( d64_writemem )
	{ 0x0000, 0x7fff, coco_ram_w},
	{ 0x8000, 0xfeff, MWA_BANK1 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff8f, coco_cartridge_w },
	{ 0xff90, 0xffbf, MWA_NOP },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP},
MEMORY_END

static MEMORY_READ_START( coco3_readmem )
	{ 0x0000, 0x1fff, MRA_BANK1 },
	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x5fff, MRA_BANK3 },
	{ 0x6000, 0x7fff, MRA_BANK4 },
	{ 0x8000, 0x9fff, MRA_BANK5 },
	{ 0xa000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, MRA_BANK7 },
	{ 0xe000, 0xfdff, MRA_BANK8 },
	{ 0xfe00, 0xfeff, MRA_BANK9 },
	{ 0xff00, 0xff1f, pia_0_r },
	{ 0xff20, 0xff3f, coco3_pia_1_r },
	{ 0xff40, 0xff8f, coco_cartridge_r },
	{ 0xff90, 0xff97, coco3_gime_r },
	{ 0xff98, 0xff9f, coco3_gimevh_r },
	{ 0xffa0, 0xffaf, coco3_mmu_r },
	{ 0xffb0, 0xffbf, paletteram_r },
	{ 0xffc0, 0xffef, MRA_NOP },
	{ 0xfff0, 0xffff, coco3_mapped_irq_r },
MEMORY_END

/* Note that the CoCo 3 doesn't use the SAM VDG mode registers
 *
 * Also, there might be other SAM registers that are ignored in the CoCo 3;
 * I am not sure which ones are...
 *
 * Tepolt implies that $FFD4-$FFD7 and $FFDA-$FFDD are ignored on the CoCo 3,
 * which would make sense, but I'm not sure.
 */
static MEMORY_WRITE_START( coco3_writemem )
	{ 0x0000, 0x1fff, MWA_BANK1 },
	{ 0x2000, 0x3fff, MWA_BANK2 },
	{ 0x4000, 0x5fff, MWA_BANK3 },
	{ 0x6000, 0x7fff, MWA_BANK4 },
	{ 0x8000, 0x9fff, MWA_BANK5 },
	{ 0xa000, 0xbfff, MWA_BANK6 },
	{ 0xc000, 0xdfff, MWA_BANK7 },
	{ 0xe000, 0xfdff, MWA_BANK8 },
	{ 0xfe00, 0xfeff, MWA_BANK9 },
	{ 0xff00, 0xff1f, pia_0_w },
	{ 0xff20, 0xff3f, pia_1_w },
	{ 0xff40, 0xff8f, coco_cartridge_w },
	{ 0xff90, 0xff97, coco3_gime_w },
	{ 0xff98, 0xff9f, coco3_gimevh_w },
	{ 0xffa0, 0xffaf, coco3_mmu_w },
	{ 0xffb0, 0xffbf, coco3_palette_w },
	{ 0xffc0, 0xffdf, sam_w },
	{ 0xffe0, 0xffff, MWA_NOP },
MEMORY_END

/* Dragon keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk N/c N/c N/c N/c Shift
  PA5: X   Y   Z   Up  Dwn Lft Rgt Space
  PA4: P   Q   R   S   T   U   V   W
  PA3: H   I   J   K   L   M   N   O
  PA2: @   A   B   C   D   E   F   G
  PA1: 8   9   :   ;   ,   -   .   /
  PA0: 0   1   2   3   4   5   6   7
 */
INPUT_PORTS_START( dragon32 )
	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0   ", KEYCODE_0, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !", KEYCODE_1, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"", KEYCODE_2, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #", KEYCODE_3, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $", KEYCODE_4, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %", KEYCODE_5, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &", KEYCODE_6, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '", KEYCODE_7, CODE_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (", KEYCODE_8, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )", KEYCODE_9, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":  *", KEYCODE_COLON, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";  +", KEYCODE_QUOTE, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",  <", KEYCODE_COMMA, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =", KEYCODE_MINUS, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  >", KEYCODE_STOP, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/  ?", KEYCODE_SLASH, CODE_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, CODE_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, CODE_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, CODE_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, CODE_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, CODE_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), CODE_NONE, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, JOYCODE_2_BUTTON1)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
INPUT_PORTS_END

/* CoCo keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk N/c N/c N/c N/c Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   Up  Dwn Lft Rgt Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */
INPUT_PORTS_START( coco )
	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, CODE_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, CODE_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, CODE_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, CODE_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0   ", KEYCODE_0, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !", KEYCODE_1, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"", KEYCODE_2, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #", KEYCODE_3, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $", KEYCODE_4, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %", KEYCODE_5, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &", KEYCODE_6, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '", KEYCODE_7, CODE_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (", KEYCODE_8, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )", KEYCODE_9, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":  *", KEYCODE_COLON, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";  +", KEYCODE_QUOTE, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",  <", KEYCODE_COMMA, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =", KEYCODE_MINUS, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  >", KEYCODE_STOP, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/  ?", KEYCODE_SLASH, CODE_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, CODE_NONE)
	PORT_BITX(0x78, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), CODE_NONE, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button", KEYCODE_RALT, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button", KEYCODE_LALT, JOYCODE_2_BUTTON1)

	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x00, "RAM Selector Switch" )
	PORT_DIPSETTING(    0x00, "32/64K" )
	PORT_DIPSETTING(    0x08, "16K" )
	PORT_DIPSETTING(    0x10, "4K" )
	
	PORT_START /* 13 */
	PORT_DIPNAME( 0x03, 0x00, "Real Time Clock" )
	PORT_DIPSETTING(    0x00, "Disto" )
	PORT_DIPSETTING(    0x01, "Cloud-9" )

INPUT_PORTS_END

/* CoCo 3 keyboard

	   PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ent Clr Brk Alt Ctr F1  F2 Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   Up  Dwn Lft Rgt Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */
INPUT_PORTS_START( coco3 )
	PORT_START /* KEY ROW 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_ASTERISK, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, CODE_NONE)

	PORT_START /* KEY ROW 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, CODE_NONE)

	PORT_START /* KEY ROW 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, CODE_NONE)

	PORT_START /* KEY ROW 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, KEYCODE_BACKSPACE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, CODE_NONE)

	PORT_START /* KEY ROW 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0   ", KEYCODE_0, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  !", KEYCODE_1, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  \"", KEYCODE_2, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  #", KEYCODE_3, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  $", KEYCODE_4, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  %", KEYCODE_5, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  &", KEYCODE_6, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  '", KEYCODE_7, CODE_NONE)

	PORT_START /* KEY ROW 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  (", KEYCODE_8, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  )", KEYCODE_9, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":  *", KEYCODE_COLON, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";  +", KEYCODE_QUOTE, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",  <", KEYCODE_COMMA, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "-  =", KEYCODE_MINUS, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  >", KEYCODE_STOP, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "/  ?", KEYCODE_SLASH, CODE_NONE)

	PORT_START /* KEY ROW 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, CODE_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLEAR", KEYCODE_HOME, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_END, CODE_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK (Esc)", KEYCODE_ESC, CODE_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "ALT",   KEYCODE_LALT, CODE_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL",  KEYCODE_LCONTROL, CODE_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F1",    KEYCODE_F1, CODE_NONE)
	PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "F2",    KEYCODE_F2, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "L-SHIFT", KEYCODE_LSHIFT, CODE_NONE)
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "R-SHIFT", KEYCODE_RSHIFT, CODE_NONE)

	PORT_START /* 7 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_1_LEFT, JOYCODE_1_RIGHT)
	PORT_START /* 8 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER1, 100, 10, 0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_1_UP, JOYCODE_1_DOWN)
	PORT_START /* 9 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_X | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_LEFT, KEYCODE_RIGHT, JOYCODE_2_LEFT, JOYCODE_2_RIGHT)
	PORT_START /* 10 */
	PORT_ANALOGX( 0xff, 0x80,  IPT_AD_STICK_Y | IPF_PLAYER2, 100, 10, 0x0, 0xff, KEYCODE_UP, KEYCODE_DOWN, JOYCODE_2_UP, JOYCODE_2_DOWN)

	PORT_START /* 11 */
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1, "Right Button 1", KEYCODE_RCONTROL, JOYCODE_1_BUTTON1)
	PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1, "Right Button 2", CODE_NONE,        JOYCODE_1_BUTTON2)
	PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2, "Left Button 1",  KEYCODE_RALT,     JOYCODE_2_BUTTON1)
	PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2, "Left Button 2",  CODE_NONE,        JOYCODE_2_BUTTON2)


	PORT_START /* 12 */
	PORT_DIPNAME( 0x03, 0x01, "Artifacting" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, "Red" )
	PORT_DIPSETTING(    0x02, "Blue" )
	PORT_DIPNAME( 0x04, 0x00, "Autocenter Joysticks" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Video type" )
	PORT_DIPSETTING(	0x00, "Composite" )
	PORT_DIPSETTING(	0x08, "RGB" )
	PORT_DIPNAME( 0x30, 0x00, "Joystick Type" )
	PORT_DIPSETTING(	0x00, "Normal" )
	PORT_DIPSETTING(	0x10, "Hi-Res Interface" )
	PORT_DIPSETTING(	0x30, "Hi-Res Interface (CoCoMax 3 Style)" )
	
	PORT_START /* 13 */
	PORT_DIPNAME( 0x03, 0x00, "Real Time Clock" )
	PORT_DIPSETTING(    0x00, "Disto" )
	PORT_DIPSETTING(    0x01, "Cloud-9" )
INPUT_PORTS_END

static struct DACinterface d_dac_interface =
{
	1,
	{ 100 }
};

static struct Wave_interface d_wave_interface = {
	1,			/* number of waves */
	{ 25 }		/* mixing levels */
};

static struct MachineDriver machine_driver_dragon32 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			COCO_CPU_SPEED_HZ,
			dragon32_readmem,dragon32_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	dragon32_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			COCO_CPU_SPEED_HZ,
			d64_readmem,d64_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	coco_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco2 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			COCO_CPU_SPEED_HZ,
			d64_readmem,d64_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	coco2_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	dragon_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco2b =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			COCO_CPU_SPEED_HZ,
			d64_readmem,d64_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	coco2_init_machine,
	dragon_stop_machine,

	/* video hardware */
	320,					/* screen width */
	240,					/* screen height (pixels doubled) */
	{ 0, 319, 0, 239 },		/* visible_area */
	0,						/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	coco2b_vh_start,
	m6847_vh_stop,
	m6847_vh_update,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco3 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6809,
			COCO_CPU_SPEED_HZ,
			coco3_readmem,coco3_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	coco3_init_machine,
	dragon_stop_machine,

	/* video hardware */
	640,							/* screen width */
	240,							/* screen height (pixels doubled) */
	{ 0, 639, 0, 239 },				/* visible_area */
	0,								/* graphics decode info */
	64+M6847_ARTIFACT_COLOR_COUNT,	/* 64 colors + artifact colors */
	0,
	NULL,							/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	coco3_vh_start,
	coco3_vh_stop,
	coco3_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

static struct MachineDriver machine_driver_coco3h =
{
	/* basic machine hardware */
	{
		{
			CPU_HD6309,
			COCO_CPU_SPEED_HZ,
			coco3_readmem,coco3_writemem,
			0, 0,
			m6847_vh_interrupt, M6847_INTERRUPTS_PER_FRAME,
			0, 0,
		},
	},
	COCO_FRAMES_PER_SECOND, 0,		 /* frames per second, vblank duration */
	0,
	coco3_init_machine,
	dragon_stop_machine,

	/* video hardware */
	640,							/* screen width */
	240,							/* screen height (pixels doubled) */
	{ 0, 639, 0, 239 },				/* visible_area */
	0,								/* graphics decode info */
	64+M6847_ARTIFACT_COLOR_COUNT,	/* 64 colors + artifact colors */
	0,
	NULL,							/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	0,
	coco3_vh_start,
	coco3_vh_stop,
	coco3_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_DAC,
			&d_dac_interface
		},
        {
			SOUND_WAVE,
            &d_wave_interface
        }
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(dragon32)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD(           "d32.rom",      0x8000,  0x4000, 0xe3879310)
	ROM_LOAD_OPTIONAL(  "ddos10.bin",   0xc000,  0x2000, 0xb44536f6)
ROM_END

ROM_START(coco)
     ROM_REGION(0x18000,REGION_CPU1,0)
     ROM_LOAD(			"bas10.rom",	0x12000, 0x2000, 0x73316e3e)
ROM_END

ROM_START(cocoe)
     ROM_REGION(0x18000,REGION_CPU1,0)
     ROM_LOAD(			"bas11.rom",	0x12000, 0x2000, 0x6270955a)
     ROM_LOAD(	        "extbas10.rom",	0x10000, 0x2000, 0x6111a086)
     ROM_LOAD_OPTIONAL(	"disk10.rom",	0x14000, 0x2000, 0xb4f9968e)
ROM_END

ROM_START(coco2)
     ROM_REGION(0x18000,REGION_CPU1,0)
     ROM_LOAD(			"bas12.rom",	0x12000, 0x2000, 0x54368805)
     ROM_LOAD(      	"extbas11.rom",	0x10000, 0x2000, 0xa82a6254)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x14000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco2b)
     ROM_REGION(0x18000,REGION_CPU1,0)
     ROM_LOAD(			"bas13.rom",	0x12000, 0x2000, 0xd8f4d15e)
     ROM_LOAD(      	"extbas11.rom",	0x10000, 0x2000, 0xa82a6254)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x14000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco3)
     ROM_REGION(0x90000,REGION_CPU1,0)
	 ROM_LOAD(			"coco3.rom",	0x80000, 0x8000, 0xb4c88d6c)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x8C000, 0x2000, 0x0b9c5415)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x8E000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(coco3p)
     ROM_REGION(0x90000,REGION_CPU1,0)
	 ROM_LOAD(			"coco3p.rom",	0x80000, 0x8000, 0xff050d80)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x8C000, 0x2000, 0x0b9c5415)
     ROM_LOAD_OPTIONAL(	"disk11.rom",	0x8E000, 0x2000, 0x0b9c5415)
ROM_END

ROM_START(cp400)
     ROM_REGION(0x18000,REGION_CPU1,0)
     ROM_LOAD("cp400bas.rom",  0x10000, 0x4000, 0x878396a5)
     ROM_LOAD("cp400dsk.rom",  0x14000, 0x2000, 0xe9ad60a0)
ROM_END

#define rom_coco3h	rom_coco3

static const struct IODevice io_coco[] = {
	IO_CARTRIDGE_COCO(dragon64_rom_load),
	IO_SNAPSHOT_COCOPAK(dragon64_pak_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
	IO_BITBANGER_PORT,
    { IO_END }
};

static const struct IODevice io_dragon32[] = {
	IO_CARTRIDGE_COCO(dragon32_rom_load),
	IO_SNAPSHOT_COCOPAK(dragon32_pak_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
	IO_BITBANGER_PORT,
    { IO_END }
};

static const struct IODevice io_cp400[] = {
	IO_CARTRIDGE_COCO(dragon64_rom_load),
	IO_SNAPSHOT_COCOPAK(dragon64_pak_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
	IO_BITBANGER_PORT,
    { IO_END }
};

static const struct IODevice io_coco3[] = {
	IO_CARTRIDGE_COCO(coco3_rom_load),
	IO_SNAPSHOT_COCOPAK(coco3_pak_load),
	IO_CASSETTE_WAVE(1, "cas\0wav\0", NULL, coco_cassette_init, coco_cassette_exit),
	IO_FLOPPY_COCO,
	IO_BITBANGER_PORT,
    { IO_END }
};

#define io_cocoe io_coco
#define io_coco2 io_coco
#define io_coco2b io_coco
#define io_coco3p io_coco3
#define io_coco3h io_coco3

/*     YEAR  NAME       PARENT  MACHINE    INPUT     INIT     COMPANY               FULLNAME */
COMP(  1980, coco,      0,		coco,      coco,     0,		  "Tandy Radio Shack",  "Color Computer" )
COMP(  1981, cocoe,     coco,	coco,      coco,     0,		  "Tandy Radio Shack",  "Color Computer (Extended BASIC 1.0)" )
COMP(  198?, coco2,     coco,	coco2,     coco,     0,		  "Tandy Radio Shack",  "Color Computer 2" )
COMP(  198?, coco2b,    coco,	coco2b,    coco,     0,		  "Tandy Radio Shack",  "Color Computer 2B" )
COMP(  1986, coco3,     coco, 	coco3,	   coco3,    0,		  "Tandy Radio Shack",  "Color Computer 3 (NTSC)" )
COMP(  1986, coco3p,    coco, 	coco3,	   coco3,    0,		  "Tandy Radio Shack",  "Color Computer 3 (PAL)" )
COMPX( 19??, coco3h,	coco,	coco3h,    coco3,	 0, 	  "Tandy Radio Shack",  "Color Computer 3 (NTSC; HD6309)", GAME_COMPUTER_MODIFIED|GAME_ALIAS)
COMP(  1982, dragon32,  coco, 	dragon32,  dragon32, 0,		  "Dragon Data Ltd",    "Dragon 32" )
COMP(  1984, cp400,     coco, 	coco,      coco,     0,		  "Prologica",          "CP400" )
