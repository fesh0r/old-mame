/*

Sega SG-1000

PCB Layout
----------

171-5078 (C) SEGA 1983
171-5046 REV. A (C) SEGA 1983

|---------------------------|                              |----------------------------|
|   SW1     CN2             |   |------|---------------|   |    SW2     CN4             |
|                           |---|         CN3          |---|                            |
|  CN1                                                                              CN5 |
|                                                                                       |
|   10.738635MHz            |------------------------------|                7805        |
|   |---|                   |------------------------------|                            |
|   |   |                                 CN6                                           |
|   | 9 |                                                                               |
|   | 9 |                                                                       LS32    |
|   | 1 |       |---------|                                                             |
|   | 8 |       | TMM2009 |                                                     LS139   |
|   | A |       |---------|             |------------------|                            |
|   |   |                               |       Z80        |                            |
|   |---|                               |------------------|                            |
|                                                                                       |
|                                                                                       |
|       MB8118  MB8118  MB8118  MB8118              SN76489A            SW3             |
|           MB8118  MB8118  MB8118  MB8118                          LS257   LS257       |
|---------------------------------------------------------------------------------------|

Notes:
    All IC's shown.

    Z80     - NEC D780C-1 / Zilog Z8400A (REV.A) Z80A CPU @ 3.579545
    TMS9918A- Texas Instruments TMS9918ANL Video Display Processor @ 10.738635MHz
    MB8118  - Fujitsu MB8118-12 16K x 1 Dynamic RAM
    TMM2009 - Toshiba TMM2009P-A / TMM2009P-B (REV.A)
    SN76489A- Texas Instruments SN76489AN Digital Complex Sound Generator @ 3.579545
    CN1     - player 1 joystick connector
    CN2     - RF video connector
    CN3     - keyboard connector
    CN4     - power connector (+9VDC)
    CN5     - player 2 joystick connector
    CN6     - cartridge connector
    SW1     - TV channel select switch
    SW2     - power switch
    SW3     - hold switch

*/

/*

    TODO:

    - SC-3000 return instruction referenced by R when reading ports 60-7f,e0-ff
    - connect PSG /READY signal to Z80 WAIT
    - accurate video timing
    - SP-400 serial printer
    - SH-400 racing controller
    - SF-7000 serial comms

*/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/sg1000.h"
#include "devices/flopdrv.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "devices/printer.h"
#include "formats/basicdsk.h"
#include "machine/ctronics.h"
#include "machine/i8255a.h"
#include "machine/msm8251.h"
#include "machine/serial.h"
#include "machine/upd765.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "crsshair.h"

/***************************************************************************
    READ/WRITE HANDLERS
***************************************************************************/

/*

    Terebi Oekaki (TV Draw)

    Address Access  Bits
                    7       6   5   4   3   2   1   0
    $6000   W       -       -   -   -   -   -   -   AXIS
    $8000   R       BUSY    -   -   -   -   -   -   PRESS
    $A000   R/W     DATA

    AXIS: write 0 to select X axis, 1 to select Y axis.
    BUSY: reads 1 when graphic board is busy sampling position, else 0.
    PRESS: reads 0 when pen is touching graphic board, else 1.
    DATA: when pen is touching graphic board, return 8-bit sample position for currently selected axis (X is in the 0-255 range, Y in the 0-191 range). Else, return 0.

*/

/*-------------------------------------------------
    tvdraw_axis_w - TV Draw axis select
-------------------------------------------------*/

static WRITE8_HANDLER( tvdraw_axis_w )
{
	sg1000_state *state = (sg1000_state *)space->machine->driver_data;

	if (data & 0x01)
	{
		state->tvdraw_data = input_port_read(space->machine, "TVDRAW_X");

		if (state->tvdraw_data < 4) state->tvdraw_data = 4;
		if (state->tvdraw_data > 251) state->tvdraw_data = 251;
	}
	else
	{
		state->tvdraw_data = input_port_read(space->machine, "TVDRAW_Y") + 32;
	}
}

/*-------------------------------------------------
    tvdraw_status_r - TV Draw status read
-------------------------------------------------*/

static READ8_HANDLER( tvdraw_status_r )
{
	return input_port_read(space->machine, "TVDRAW_PEN");
}

/*-------------------------------------------------
    tvdraw_data_r - TV Draw data read
-------------------------------------------------*/

static READ8_HANDLER( tvdraw_data_r )
{
	sg1000_state *state = (sg1000_state *)space->machine->driver_data;

	return state->tvdraw_data;
}

/*-------------------------------------------------
    sg1000_joysel_r -
-------------------------------------------------*/

static READ8_HANDLER( sg1000_joysel_r )
{
	return 0x80;
}

/***************************************************************************
    MEMORY MAPS
***************************************************************************/

/*-------------------------------------------------
    ADDRESS_MAP( sg1000_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sg1000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank1")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sg1000_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sg1000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_DEVWRITE(SN76489A_TAG, sn76496_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x81, 0x81) AM_MIRROR(0x3e) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdc) AM_READ_PORT("PA7")
	AM_RANGE(0xdd, 0xdd) AM_READ_PORT("PB7")
	AM_RANGE(0xde, 0xde) AM_READ(sg1000_joysel_r) AM_WRITENOP
	AM_RANGE(0xdf, 0xdf) AM_NOP
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( omv_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( omv_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank1")
	AM_RANGE(0xc000, 0xc7ff) AM_MIRROR(0x3800) AM_RAM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( omv_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( omv_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x40, 0x40) AM_MIRROR(0x3f) AM_DEVWRITE(SN76489A_TAG, sn76496_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x3e) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x81, 0x81) AM_MIRROR(0x3e) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x38) AM_READ_PORT("C0")
	AM_RANGE(0xc1, 0xc1) AM_MIRROR(0x38) AM_READ_PORT("C1")
	AM_RANGE(0xc2, 0xc2) AM_MIRROR(0x38) AM_READ_PORT("C2")
	AM_RANGE(0xc3, 0xc3) AM_MIRROR(0x38) AM_READ_PORT("C3")
	AM_RANGE(0xc4, 0xc4) AM_MIRROR(0x3a) AM_READ_PORT("C4")
	AM_RANGE(0xc5, 0xc5) AM_MIRROR(0x3a) AM_READ_PORT("C5")
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sc3000_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sc3000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_RAMBANK("bank1")
	AM_RANGE(0xc000, 0xffff) AM_RAMBANK("bank2")
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sc3000_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sc3000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x7f, 0x7f) AM_DEVWRITE(SN76489A_TAG, sn76496_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdf) AM_DEVREADWRITE(UPD9255_TAG, i8255a_r, i8255a_w)
ADDRESS_MAP_END

/* This is how the I/O ports are really mapped, but MAME does not support overlapping ranges
static ADDRESS_MAP_START( sc3000_io_map, ADDRESS_SPACE_IO, 8 )
    ADDRESS_MAP_GLOBAL_MASK(0xff)
    AM_RANGE(0x00, 0x00) AM_MIRROR(0xdf) AM_DEVREADWRITE(UPD9255_TAG, i8255a_r, i8255a_w)
    AM_RANGE(0x00, 0x00) AM_MIRROR(0x7f) AM_DEVWRITE(SN76489A_TAG, sn76496_w)
    AM_RANGE(0x00, 0x00) AM_MIRROR(0xae) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
    AM_RANGE(0x01, 0x01) AM_MIRROR(0xae) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
    AM_RANGE(0x60, 0x60) AM_MIRROR(0x9f) AM_READ(sc3000_r_r)
ADDRESS_MAP_END
*/

/*-------------------------------------------------
    ADDRESS_MAP( sf7000_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sf7000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ_BANK("bank1") AM_WRITE_BANK("bank2")
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( sf7000_io_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( sf7000_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x7f, 0x7f) AM_DEVWRITE(SN76489A_TAG, sn76496_w)
	AM_RANGE(0xbe, 0xbe) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xbf, 0xbf) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xdc, 0xdf) AM_DEVREADWRITE(UPD9255_0_TAG, i8255a_r, i8255a_w)
	AM_RANGE(0xe0, 0xe0) AM_DEVREAD(UPD765_TAG, upd765_status_r)
	AM_RANGE(0xe1, 0xe1) AM_DEVREADWRITE(UPD765_TAG, upd765_data_r, upd765_data_w)
	AM_RANGE(0xe4, 0xe7) AM_DEVREADWRITE(UPD9255_1_TAG, i8255a_r, i8255a_w)
	AM_RANGE(0xe8, 0xe8) AM_DEVREADWRITE(UPD8251_TAG, msm8251_data_r, msm8251_data_w)
	AM_RANGE(0xe9, 0xe9) AM_DEVREADWRITE(UPD8251_TAG, msm8251_status_r, msm8251_control_w)
ADDRESS_MAP_END

/***************************************************************************
    INPUT PORTS
***************************************************************************/

/*-------------------------------------------------
    INPUT_CHANGED( trigger_nmi )
-------------------------------------------------*/

static INPUT_CHANGED( trigger_nmi )
{
	cputag_set_input_line(field->port->machine, Z80_TAG, INPUT_LINE_NMI, (input_port_read(field->port->machine, "NMI") ? CLEAR_LINE : ASSERT_LINE));
}

/*-------------------------------------------------
    INPUT_PORTS( tvdraw )
-------------------------------------------------*/

static INPUT_PORTS_START( tvdraw )
	PORT_START("TVDRAW_X")
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START("TVDRAW_Y")
	PORT_BIT( 0xff, 0x60, IPT_LIGHTGUN_Y ) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_MINMAX(0, 191) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START("TVDRAW_PEN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("Pen")
INPUT_PORTS_END

/*-------------------------------------------------
    INPUT_PORTS( sg1000 )
-------------------------------------------------*/

static INPUT_PORTS_START( sg1000 )
	PORT_START("PA7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

	PORT_START("PB7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("NMI")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START ) PORT_NAME("PAUSE") PORT_CODE(KEYCODE_P) PORT_CHANGED(trigger_nmi, 0)

	PORT_INCLUDE( tvdraw )
INPUT_PORTS_END

/*-------------------------------------------------
    INPUT_PORTS( omv )
-------------------------------------------------*/

static INPUT_PORTS_START( omv )
	PORT_START("C0")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("C1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("9 #") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("0 *") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("C2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("C3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("C4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_NAME("S-1")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("S-2")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

	PORT_START("C5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_INCLUDE( tvdraw )
INPUT_PORTS_END

/*-------------------------------------------------
    INPUT_PORTS( sk1100 )
-------------------------------------------------*/

static INPUT_PORTS_START( sk1100 )
	PORT_START("PA0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENG DIER'S") PORT_CODE(KEYCODE_RALT) PORT_CHAR(UCHAR_MAMEKEY(RALT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i')

	PORT_START("PA1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPC") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o')

	PORT_START("PA2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOME CLR") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p')

	PORT_START("PA3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INS DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xcf\x80") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x03c0)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('@') PORT_CHAR('`')

	PORT_START("PA4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('[') PORT_CHAR('{')

	PORT_START("PA5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CR") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PA7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)

	PORT_START("PB0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH2) PORT_CHAR('^')
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("PB5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xc2\xa5") PORT_CODE(KEYCODE_TILDE) PORT_CHAR(0x00a5)
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FUNC") PORT_CODE(KEYCODE_TAB)

	PORT_START("PB6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BREAK") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GRAPH") PORT_CODE(KEYCODE_LALT) PORT_CHAR(UCHAR_MAMEKEY(LALT))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("PB7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_START("NMI")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET") PORT_CODE(KEYCODE_F10) PORT_CHANGED(trigger_nmi, 0)
INPUT_PORTS_END

/*-------------------------------------------------
    INPUT_PORTS( sc3000 )
-------------------------------------------------*/

static INPUT_PORTS_START( sc3000 )
	PORT_INCLUDE( sk1100 )
	PORT_INCLUDE( tvdraw )
INPUT_PORTS_END

/*-------------------------------------------------
    INPUT_PORTS( sf7000 )
-------------------------------------------------*/

static INPUT_PORTS_START( sf7000 )
	PORT_INCLUDE( sk1100 )

	PORT_START("BAUD")
	PORT_CONFNAME( 0x05, 0x05, "Baud rate")
	PORT_CONFSETTING( 0x00, "9600 baud" )
	PORT_CONFSETTING( 0x01, "4800 baud" )
	PORT_CONFSETTING( 0x02, "2400 baud" )
	PORT_CONFSETTING( 0x03, "1200 baud" )
	PORT_CONFSETTING( 0x04, "600 baud" )
	PORT_CONFSETTING( 0x05, "300 baud" )
INPUT_PORTS_END

/***************************************************************************
    DEVICE CONFIGURATION
***************************************************************************/

/*-------------------------------------------------
    TMS9928a_interface tms9928a_interface
-------------------------------------------------*/

static INTERRUPT_GEN( sg1000_int )
{
    TMS9928A_interrupt(device->machine);
}

static void sg1000_vdp_interrupt(running_machine *machine, int state)
{
	cputag_set_input_line(machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	sg1000_vdp_interrupt
};

/*-------------------------------------------------
    I8255A_INTERFACE( sc3000_ppi_intf )
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( sc3000_ppi_pa_r )
{
	/*
        Signal  Description

        PA0     Keyboard input
        PA1     Keyboard input
        PA2     Keyboard input
        PA3     Keyboard input
        PA4     Keyboard input
        PA5     Keyboard input
        PA6     Keyboard input
        PA7     Keyboard input
    */

	sg1000_state *state = (sg1000_state *)device->machine->driver_data;

	static const char *const keynames[] = { "PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7" };

	return input_port_read(device->machine, keynames[state->keylatch]);
}

static READ8_DEVICE_HANDLER( sc3000_ppi_pb_r )
{
	/*
        Signal  Description

        PB0     Keyboard input
        PB1     Keyboard input
        PB2     Keyboard input
        PB3     Keyboard input
        PB4     /CONT input from cartridge terminal B-11
        PB5     FAULT input from printer
        PB6     BUSY input from printer
        PB7     Cassette tape input
    */

	sg1000_state *state = (sg1000_state *)device->machine->driver_data;

	static const char *const keynames[] = { "PB0", "PB1", "PB2", "PB3", "PB4", "PB5", "PB6", "PB7" };

	/* keyboard */
	UINT8 data = input_port_read(device->machine, keynames[state->keylatch]);

	/* cartridge contact */
	data |= 0x10;

	/* printer */
	data |= 0x60;

	/* tape input */
	if (cassette_input(state->cassette) > +0.0) data |= 0x80;

	return data;
}

static WRITE8_DEVICE_HANDLER( sc3000_ppi_pc_w )
{
	/*
        Signal  Description

        PC0     Keyboard raster output
        PC1     Keyboard raster output
        PC2     Keyboard raster output
        PC3     not connected
        PC4     Cassette tape output
        PC5     DATA to printer
        PC6     /RESET to printer
        PC7     /FEED to printer
    */

	sg1000_state *state = (sg1000_state *)device->machine->driver_data;

	/* keyboard */
	state->keylatch = data & 0x07;

	/* cassette */
	cassette_output(state->cassette, BIT(data, 4) ? +1.0 : -1.0);

	/* TODO printer */
}

static I8255A_INTERFACE( sc3000_ppi_intf )
{
	DEVCB_HANDLER(sc3000_ppi_pa_r),	// Port A read
	DEVCB_HANDLER(sc3000_ppi_pb_r),	// Port B read
	DEVCB_NULL,						// Port C read
	DEVCB_NULL,						// Port A write
	DEVCB_NULL,						// Port B write
	DEVCB_HANDLER(sc3000_ppi_pc_w),	// Port C write
};

/*-------------------------------------------------
    cassette_config sc3000_cassette_config
-------------------------------------------------*/

static const cassette_config sc3000_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

/*-------------------------------------------------
    I8255A_INTERFACE( sf7000_ppi_intf )
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( sf7000_ppi_pa_r )
{
	/*
        Signal  Description

        PA0     INT from FDC
        PA1     BUSY from Centronics printer
        PA2     INDEX from FDD
        PA3
        PA4
        PA5
        PA6
        PA7
    */

	sg1000_state *state = (sg1000_state *)device->machine->driver_data;
	UINT8 result = 0;

	result |= state->fdc_irq;
	result |= centronics_busy_r(state->centronics) << 1;
	result |= state->fdc_index << 2;

	return result;
}

static WRITE8_DEVICE_HANDLER( sf7000_ppi_pc_w )
{
	/*
        Signal  Description

        PC0     /INUSE signal to FDD
        PC1     /MOTOR ON signal to FDD
        PC2     TC signal to FDC
        PC3     RESET signal to FDC
        PC4     not connected
        PC5     not connected
        PC6     /ROM SEL (switch between IPL ROM and RAM)
        PC7     /STROBE to Centronics printer
    */

	sg1000_state *state = (sg1000_state *)device->machine->driver_data;

	/* floppy motor */
	floppy_mon_w(floppy_get_device(device->machine, 0), BIT(data, 1));
	floppy_drive_set_ready_state(floppy_get_device(device->machine, 0), 1, 1);

	/* FDC terminal count */
	upd765_tc_w(state->upd765, BIT(data, 2));

	/* FDC reset */
	if (BIT(data, 3))
	{
		upd765_reset(state->upd765, 0);
	}

	/* ROM selection */
	memory_set_bank(device->machine, "bank1", BIT(data, 6));

	/* printer strobe */
	centronics_strobe_w(state->centronics, BIT(data, 7));
}

static I8255A_INTERFACE( sf7000_ppi_intf )
{
	DEVCB_HANDLER(sf7000_ppi_pa_r),		// Port A read
	DEVCB_NULL,							// Port B read
	DEVCB_NULL,							// Port C read
	DEVCB_NULL,							// Port A write
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	// Port B write
	DEVCB_HANDLER(sf7000_ppi_pc_w)		// Port C write
};

/*-------------------------------------------------
    upd765_interface sf7000_upd765_interface
-------------------------------------------------*/

static WRITE_LINE_DEVICE_HANDLER( sf7000_fdc_interrupt )
{
	sg1000_state *driver_state = (sg1000_state *)device->machine->driver_data;

	driver_state->fdc_irq = state;
}

static const struct upd765_interface sf7000_upd765_interface =
{
	DEVCB_LINE(sf7000_fdc_interrupt),
	NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( sf7000 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( sf7000 )
	FLOPPY_OPTION(sf7000, "sf7", "SF7 disk image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([1])
		TRACKS([40])
		SECTORS([16])
		SECTOR_LENGTH([256])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config sf7000_floppy_config
-------------------------------------------------*/

static const floppy_config sf7000_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSHD,
	FLOPPY_OPTIONS_NAME(sf7000),
	NULL
};

/***************************************************************************
    CARTRIDGE LOADING
***************************************************************************/

/*-------------------------------------------------
    sg1000_map_cartridge_memory -
-------------------------------------------------*/

static void sg1000_map_cartridge_memory(running_machine *machine, UINT8 *ptr, int size)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	switch (size)
	{
	case 40 * 1024:
		memory_install_read_bank(program, 0x8000, 0x9fff, 0, 0, "bank1");
		memory_unmap_write(program, 0x8000, 0x9fff, 0, 0);
		memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, Z80_TAG) + 0x8000, 0);
		memory_set_bank(machine, "bank1", 0);
		break;

	case 48 * 1024:
		memory_install_read_bank(program, 0x8000, 0xbfff, 0, 0, "bank1");
		memory_unmap_write(program, 0x8000, 0xbfff, 0, 0);
		memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, Z80_TAG) + 0x8000, 0);
		memory_set_bank(machine, "bank1", 0);
		break;

	default:
		if (IS_CARTRIDGE_TV_DRAW(ptr))
		{
			memory_install_write8_handler(program, 0x6000, 0x6000, 0, 0, &tvdraw_axis_w);
			memory_install_read8_handler(program, 0x8000, 0x8000, 0, 0, &tvdraw_status_r);
			memory_install_read8_handler(program, 0xa000, 0xa000, 0, 0, &tvdraw_data_r);
			memory_nop_write(program, 0xa000, 0xa000, 0, 0);
		}
		else if (IS_CARTRIDGE_THE_CASTLE(ptr))
		{
			memory_install_readwrite_bank(program, 0x8000, 0x9fff, 0, 0, "bank1");
		}
		break;
	}
}

/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( sg1000_cart )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( sg1000_cart )
{
	const address_space *program = cputag_get_address_space(image.device().machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, Z80_TAG);

	if (image.software_entry() == NULL)
	{
		size = image.length();
		if (image.fread( ptr, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr, image.get_software_region("rom"), size);
	}

	/* cartridge ROM banking */
	sg1000_map_cartridge_memory(image.device().machine, ptr, size);

	/* work RAM banking */
	memory_install_readwrite_bank(program, 0xc000, 0xc3ff, 0, 0x3c00, "bank2");

	return IMAGE_INIT_PASS;
}

/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( omv_cart )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( omv_cart )
{
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, Z80_TAG);

	if (image.software_entry() == NULL)
	{
		size = image.length();
		if (image.fread( ptr, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr, image.get_software_region("rom"), size);
	}

	/* cartridge ROM banking */
	sg1000_map_cartridge_memory(image.device().machine, ptr, size);

	return IMAGE_INIT_PASS;
}

/*-------------------------------------------------
    sc3000_map_cartridge_memory -
-------------------------------------------------*/

static void sc3000_map_cartridge_memory(running_machine *machine, UINT8 *ptr, int size)
{
	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* include SG-1000 mapping */
	sg1000_map_cartridge_memory(machine, ptr, size);

	if (IS_CARTRIDGE_BASIC_LEVEL_III(ptr))
	{
		memory_install_readwrite_bank(program, 0x8000, 0xbfff, 0, 0, "bank1");
		memory_install_readwrite_bank(program, 0xc000, 0xffff, 0, 0, "bank2");
	}
	else if (IS_CARTRIDGE_MUSIC_EDITOR(ptr))
	{
		memory_install_readwrite_bank(program, 0x8000, 0x9fff, 0, 0, "bank1");
		memory_install_readwrite_bank(program, 0xc000, 0xc7ff, 0, 0x3800, "bank2");
	}
	else
	{
		/* regular cartridges */
		memory_install_readwrite_bank(program, 0xc000, 0xc7ff, 0, 0x3800, "bank2");
	}
}

/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( sc3000_cart )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( sc3000_cart )
{
	UINT32 size;
	UINT8 *ptr = memory_region(image.device().machine, Z80_TAG);

	if (image.software_entry() == NULL)
	{
		size = image.length();
		if (image.fread( ptr, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		size = image.get_software_region_length("rom");
		memcpy(ptr, image.get_software_region("rom"), size);
	}

	/* cartridge ROM and work RAM banking */
	sc3000_map_cartridge_memory(image.device().machine, ptr, size);

	return IMAGE_INIT_PASS;
}

/***************************************************************************
    MACHINE INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( lightgun_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( lightgun_tick )
{
	UINT8 *rom = memory_region(machine, Z80_TAG);

	if (IS_CARTRIDGE_TV_DRAW(rom))
	{
		/* enable crosshair for TV Draw */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_ALL);
	}
	else
	{
		/* disable crosshair for other cartridges */
		crosshair_set_screen(machine, 0, CROSSHAIR_SCREEN_NONE);
	}
}

/*-------------------------------------------------
    MACHINE_START( sg1000 )
-------------------------------------------------*/

static MACHINE_START( sg1000 )
{
	sg1000_state *state = (sg1000_state *)machine->driver_data;

	/* configure VDP */
	TMS9928A_configure(&tms9928a_interface);

	/* toggle light gun crosshair */
	timer_set(machine, attotime_zero, NULL, 0, lightgun_tick);

	/* register for state saving */
	state_save_register_global(machine, state->tvdraw_data);
}

/*-------------------------------------------------
    MACHINE_START( sc3000 )
-------------------------------------------------*/

static MACHINE_START( sc3000 )
{
	sg1000_state *state = (sg1000_state *)machine->driver_data;

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* configure VDP */
	TMS9928A_configure(&tms9928a_interface);

	/* toggle light gun crosshair */
	timer_set(machine, attotime_zero, NULL, 0, lightgun_tick);

	/* register for state saving */
	state_save_register_global(machine, state->tvdraw_data);
	state_save_register_global(machine, state->keylatch);
}

/*-------------------------------------------------
    sf7000_fdc_index_callback -
-------------------------------------------------*/

static void sf7000_fdc_index_callback(running_device *controller, running_device *img, int state)
{
	sg1000_state *driver_state = (sg1000_state *)img->machine->driver_data;

	driver_state->fdc_index = state;
}

/*-------------------------------------------------
    MACHINE_START( sf7000 )
-------------------------------------------------*/

static MACHINE_START( sf7000 )
{
	sg1000_state *state = (sg1000_state *)machine->driver_data;

	/* find devices */
	state->upd765 = machine->device(UPD765_TAG);
	state->cassette = machine->device(CASSETTE_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);

	/* configure VDP */
	TMS9928A_configure(&tms9928a_interface);

	/* configure FDC */
	floppy_drive_set_index_pulse_callback(floppy_get_device(machine, 0), sf7000_fdc_index_callback);

	/* configure memory banking */
	memory_configure_bank(machine, "bank1", 0, 1, memory_region(machine, Z80_TAG), 0);
	memory_configure_bank(machine, "bank1", 1, 1, messram_get_ptr(machine->device("messram")), 0);
	memory_configure_bank(machine, "bank2", 0, 1, messram_get_ptr(machine->device("messram")), 0);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->fdc_irq);
	state_save_register_global(machine, state->fdc_index);
}

/*-------------------------------------------------
    MACHINE_RESET( sf7000 )
-------------------------------------------------*/

static MACHINE_RESET( sf7000 )
{
	memory_set_bank(machine, "bank1", 0);
	memory_set_bank(machine, "bank2", 0);
}

/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

/*-------------------------------------------------
    MACHINE_DRIVER_START( sg1000 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( sg1000 )
	MDRV_DRIVER_DATA(sg1000_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_10_738635MHz/3)
	MDRV_CPU_PROGRAM_MAP(sg1000_map)
	MDRV_CPU_IO_MAP(sg1000_io_map)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, sg1000_int)

	MDRV_MACHINE_START(sg1000)

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A_TAG, SN76489A, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("sg,bin")
	MDRV_CARTSLOT_MANDATORY
	MDRV_CARTSLOT_INTERFACE("sg1000_cart")
	MDRV_CARTSLOT_LOAD(sg1000_cart)

	/* software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","sg1000")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("1K")
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER_START( omv )
-------------------------------------------------*/

static MACHINE_DRIVER_START( omv )
	MDRV_IMPORT_FROM(sg1000)

	MDRV_CPU_MODIFY(Z80_TAG)
	MDRV_CPU_PROGRAM_MAP(omv_map)
	MDRV_CPU_IO_MAP(omv_io_map)

	MDRV_CARTSLOT_MODIFY("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("sg,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(omv_cart)

	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("2K")
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER_START( sc3000 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( sc3000 )
	MDRV_DRIVER_DATA(sg1000_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_10_738635MHz/3) // LH0080A
	MDRV_CPU_PROGRAM_MAP(sc3000_map)
	MDRV_CPU_IO_MAP(sc3000_io_map)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, sg1000_int)

	MDRV_MACHINE_START(sc3000)

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A_TAG, SN76489A, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_I8255A_ADD(UPD9255_TAG, sc3000_ppi_intf)
//  MDRV_PRINTER_ADD("sp400") /* serial printer */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, sc3000_cassette_config)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("sg,sc,bin")
	MDRV_CARTSLOT_INTERFACE("sg1000_cart")
	MDRV_CARTSLOT_LOAD(sc3000_cart)

	/* software lists */
	MDRV_SOFTWARE_LIST_ADD("cart_list","sg1000")

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("2K")
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER_START( sf7000 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( sf7000 )
	MDRV_DRIVER_DATA(sg1000_state)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_10_738635MHz/3)
	MDRV_CPU_PROGRAM_MAP(sf7000_map)
	MDRV_CPU_IO_MAP(sf7000_io_map)
	MDRV_CPU_VBLANK_INT(SCREEN_TAG, sg1000_int)

	MDRV_MACHINE_START(sf7000)
	MDRV_MACHINE_RESET(sf7000)

    /* video hardware */
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY(SCREEN_TAG)
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76489A_TAG, SN76489A, XTAL_10_738635MHz/3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_I8255A_ADD(UPD9255_0_TAG, sc3000_ppi_intf)
	MDRV_I8255A_ADD(UPD9255_1_TAG, sf7000_ppi_intf)
	MDRV_MSM8251_ADD(UPD8251_TAG, default_msm8251_interface)
	MDRV_UPD765A_ADD(UPD765_TAG, sf7000_upd765_interface)
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, sf7000_floppy_config)
//  MDRV_PRINTER_ADD("sp400") /* serial printer */
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, sc3000_cassette_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/***************************************************************************
    ROMS
***************************************************************************/

ROM_START( sg1000 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )
ROM_END

#define rom_sg1000m2 rom_sg1000

ROM_START( sc3000 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )
	ROM_LOAD( "sc3000.rom", 0x0000, 0x8000, CRC(a46e3c73) SHA1(b1a8585a9afff962e8fd0e79b3305199da4e4562))
ROM_END

#define rom_sc3000h rom_sg1000

ROM_START( omv1000 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )
	ROM_LOAD( "omvbios.bin", 0x0000, 0x4000, BAD_DUMP CRC(c5a67b95) SHA1(6d7c64dd60dee4a33061d3d3a7c2ed190d895cdb) )	// The BIOS comes from a Multivision FG-2000. It is still unknown if the FG-1000 BIOS differs
ROM_END

ROM_START( omv2000 )
	ROM_REGION( 0x10000, Z80_TAG, ROMREGION_ERASE00 )
	ROM_LOAD( "omvbios.bin", 0x0000, 0x4000, CRC(c5a67b95) SHA1(6d7c64dd60dee4a33061d3d3a7c2ed190d895cdb) )
ROM_END

ROM_START( sf7000 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(d76810b8) SHA1(77339a6db2593aadc638bed77b8e9bed5d9d87e3) )
ROM_END

/***************************************************************************
    SYSTEM DRIVERS
***************************************************************************/

/*    YEAR  NAME        PARENT      COMPAT      MACHINE     INPUT       INIT    COMPANY             FULLNAME                                    FLAGS */
CONS( 1983,	sg1000,     0,          0,          sg1000,     sg1000,     0,      "Sega",             "SG-1000",                                  GAME_SUPPORTS_SAVE )
CONS( 1984,	sg1000m2,   sg1000,     0,          sc3000,     sc3000,     0,      "Sega",             "SG-1000 II",                               GAME_SUPPORTS_SAVE )
COMP( 1983,	sc3000,     0,          sg1000,     sc3000,     sc3000,     0,      "Sega",             "SC-3000",                                  GAME_SUPPORTS_SAVE )
COMP( 1983,	sc3000h,    sc3000,     0,          sc3000,     sc3000,     0,      "Sega",             "SC-3000H",                                 GAME_SUPPORTS_SAVE )
COMP( 1983,	sf7000,     sc3000,     0,          sf7000,     sf7000,     0,      "Sega",             "SC-3000/Super Control Station SF-7000",    GAME_SUPPORTS_SAVE )
CONS( 1984,	omv1000,    sg1000,     0,          omv,        omv,        0,      "Tsukuda Original", "Othello Multivision FG-1000",              GAME_SUPPORTS_SAVE )
CONS( 1984,	omv2000,    sg1000,     0,          omv,        omv,        0,      "Tsukuda Original", "Othello Multivision FG-2000",              GAME_SUPPORTS_SAVE )
