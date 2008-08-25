/***************************************************************************

Apple II

This family of computers bank-switches everything up the wazoo.

Remarkable features
-------------------

Apple II (original model)
-------------------------

RAM: 4/8/12/16/20/24/32/36/48 KB (according to the manual)

ROM: 8 KB mapped to $E000-$FFFF
Empty ROM sockets mapped at $D000-$D7FF (usually occupied by Programmer's
Aid #1 chip) and $D800-$DFFF (usually empty, but a couple of 3rd party
chips were produced)

HI-RES Palette has only 4 colors: 0 - black, 1 - green, 2 - purple,
3 - white

Due to an hardware bug, green/purple artifacts are present in text mode
too!

No 80 columns
No Open/Solid Apple keys
No Up/Down arrows key

Users often connected the SHIFT key to the paddle #2 button (mapped to
$C063) in order to inform properly written software that characters were
to be intended upper/lower case

*** TODO: Should MESS emulate this via a dipswitch?

Integer BASIC in ROM, AppleSoft must be loaded from disk or tape

No AutoStart ROM: once the machine was switched on, the user had to manually
perform the reset cycle pressing, guess what, RESET ;)

Apple II+
---------

RAM: 16/32/48 KB + extra 16 KB at $C000 if using Apple Language Card
ROM: 12 KB mapped to $D000-$DFFF

HI-RES Palette has four more entries: 4 - black (again), 5 - orange,
6 - blue, 7 - white (again)

No more artifact bug in text mode

No 80 columns
No Open/Solid Apple keys
NO Up/Down arrows keys

Users still did the SHIFT key mod

AppleSoft BASIC in ROM

AutoStart ROM - no more need to press RESET after switching the machine on

Apple IIe
---------

RAM: 64 KB + optional bank of 64 KB (see 80 columns card)
ROM: 16 KB

80 columns card: this card was available in two versions - one equipped
with 1 KB of memory to provide the extra RAM for the display, the other
equipped with full 64 KB of RAM - the 80 columns card is not included
in the standard configuration and is available as add-on.

Open/Solid Apple keys mapped to buttons 0 and 1 of the paddle #1
Up/Down arrows keys
Connector for an optional numeric keypad

Apple begins manufacturing its machines with the SHIFT key mod

Revision A motherboards cannot handle double-hires graphics, Revision B can

*** TODO: Should MESS emulate this via a dipswitch?

Apple IIe (enhanced)
--------------------

The enhancement consists in bugfix of the ROM code, a 65c02 instead of the
6502 and a change in the character generator ROM which now includes the
so called "MouseText" characters (thus, no flashing characters in 80
columns mode)

Double hi-res mode is supported

Apple IIe (Platinum)
--------------------

Identical to IIe enhanced except for:

The numerical keypad is integrated into the main keyboard (although the
internal connector is still present)

The CLEAR key on the keypad generates the same character of the ESC key,
but some users did an hardware modification so that it generates CTRL-X

*** TODO: Should MESS emulate this via a dipswitch?

The 64 KB 80 columns card is built in

Due to the SHIFT key mod, if the user press both SHIFT and the paddle
button where the shift key was connected, a short circuit is caused
and the power supply is shut down!

Apple IIc
---------

Same as IIe enhanced (Rev B) except for:

There are no slots in hardware. The machine however sees (for compatibility
reasons):

Two Super Serial Cards in slots 1-2
80 columns card (64 KB version) in slot 3
Mouse in slot 4
Easter Egg in slot 5 (!)
Disk II in slot 6
External 5.25 drive in slot 7

MouseText characters

No numerical keypad

Switchables keyboard layouts (the user, via an external switch, can choose
between two layouts, i.e. US and German, and in the USA QWERTY and Dvorak)

*** TODO: Should MESS emulate this?

Apple IIc (UniDisk 3.5)
-----------------------

Identical to IIc except for:

ROM: 32 KB

The disk firmware can handle up to four 3.5 disk drives or three 3.5 drives
and a 5.25 drive

Preliminary support (but not working and never completed) for AppleTalk
network in slot 7

Apple IIc (Original Memory Expansion)
-------------------------------------

ROMSET NOT DUMPED

Identical to IIc except for:

Support for Memory Expansion Board (mapped to slot 4)
This card can provide up to 1 MB of RAM in increments of 256 MB
The firmware in ROM sees the extra RAM as a RAMdisk

Since the expansion is mapped to slot 4, mouse is now mapped to slot 7

Apple IIc (Revised Memory Expansion)
------------------------------------

ROMSET NOT DUMPED

Identical to IIc (OME) except for bugfixes

Apple IIc Plus
--------------

Identical to IIc (RME) except for:

The 65c02 works at 4MHz

The machine has an internal "Apple 3.5" drive (which is DIFFERENT from the
UniDisk 3.5 drive!)

The external drive port supports not only 5.25 drives but also UniDisk and
Apple 3.5 drives, allowing via daisy-chaining any combination of UniDisk,
Apple 3.5 and Apple 5.25 drives - up to three devices

***************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "devices/appldriv.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "machine/ap2_slot.h"
#include "machine/ap2_lang.h"
#include "machine/applefdc.h"
#include "machine/mockngbd.h"
#include "sound/ay8910.h"



/***************************************************************************
    PARAMETERS
***************************************************************************/

#define JOYSTICK_DELTA			80
#define JOYSTICK_SENSITIVITY	50
#define JOYSTICK_AUTOCENTER     80
#define PADDLE_DELTA            10
#define PADDLE_SENSITIVITY      10
#define PADDLE_AUTOCENTER       0



/***************************************************************************
    ADDRESS MAP
***************************************************************************/

static ADDRESS_MAP_START( apple2_map, ADDRESS_SPACE_PROGRAM, 8 )
	/* nothing in the address map - everything is added dynamically */
ADDRESS_MAP_END



/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( apple2_joystick )
	PORT_START("joystick_1_x")		/* Joystick 1 X Axis */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X) PORT_NAME("P1 Joystick X")
	PORT_SENSITIVITY(JOYSTICK_SENSITIVITY)
	PORT_KEYDELTA(JOYSTICK_DELTA)
	PORT_CENTERDELTA(JOYSTICK_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(1)
	PORT_CODE_DEC(KEYCODE_4_PAD)	PORT_CODE_INC(KEYCODE_6_PAD)
	PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH)	PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START("joystick_1_y")		/* Joystick 1 Y Axis */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y) PORT_NAME("P1 Joystick Y")
	PORT_SENSITIVITY(JOYSTICK_SENSITIVITY)
	PORT_KEYDELTA(JOYSTICK_DELTA)
	PORT_CENTERDELTA(JOYSTICK_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(1)
	PORT_CODE_DEC(KEYCODE_8_PAD)	PORT_CODE_INC(KEYCODE_2_PAD)
	PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH)		PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)

	PORT_START("joystick_2_x")		/* Joystick 2 X Axis */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X) PORT_NAME("P2 Joystick X")
	PORT_SENSITIVITY(JOYSTICK_SENSITIVITY)
	PORT_KEYDELTA(JOYSTICK_DELTA)
	PORT_CENTERDELTA(JOYSTICK_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(2)
	PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH)	PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START("joystick_2_y")		/* Joystick 2 Y Axis */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y) PORT_NAME("P2 Joystick Y")
	PORT_SENSITIVITY(JOYSTICK_SENSITIVITY)
	PORT_KEYDELTA(JOYSTICK_DELTA)
	PORT_CENTERDELTA(JOYSTICK_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(2)
	PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH)		PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)

	PORT_START("joystick_buttons")
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1)  PORT_PLAYER(1)			PORT_CODE(KEYCODE_0_PAD)	PORT_CODE(JOYCODE_BUTTON1)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2)  PORT_PLAYER(1)			PORT_CODE(KEYCODE_ENTER_PAD)PORT_CODE(JOYCODE_BUTTON2)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1)  PORT_PLAYER(2)			PORT_CODE(JOYCODE_BUTTON1)
INPUT_PORTS_END

static INPUT_PORTS_START( apple2_paddle )
	PORT_START("paddle_0")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_NAME("P1 Paddle 0")
	PORT_SENSITIVITY(PADDLE_SENSITIVITY)
	PORT_KEYDELTA(PADDLE_DELTA)
	PORT_CENTERDELTA(PADDLE_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(1)
	PORT_CODE_DEC(KEYCODE_4_PAD)	PORT_CODE_INC(KEYCODE_6_PAD)
	PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH)	PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START("paddle_1")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_NAME("P1 Paddle 1")
	PORT_SENSITIVITY(PADDLE_SENSITIVITY)
	PORT_KEYDELTA(PADDLE_DELTA)
	PORT_CENTERDELTA(PADDLE_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(1)
	PORT_CODE_DEC(KEYCODE_8_PAD)	PORT_CODE_INC(KEYCODE_2_PAD)
	PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH)		PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)

	PORT_START("paddle_2")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_NAME("P2 Paddle 0")
	PORT_SENSITIVITY(PADDLE_SENSITIVITY)
	PORT_KEYDELTA(PADDLE_DELTA)
	PORT_CENTERDELTA(PADDLE_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(2)
	PORT_CODE_DEC(JOYCODE_X_LEFT_SWITCH)	PORT_CODE_INC(JOYCODE_X_RIGHT_SWITCH)

	PORT_START("paddle_3")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE) PORT_NAME("P2 Paddle 1")
	PORT_SENSITIVITY(PADDLE_SENSITIVITY)
	PORT_KEYDELTA(PADDLE_DELTA)
	PORT_CENTERDELTA(PADDLE_AUTOCENTER)
	PORT_MINMAX(0,0xff) PORT_PLAYER(2)
	PORT_CODE_DEC(JOYCODE_Y_UP_SWITCH)		PORT_CODE_INC(JOYCODE_Y_DOWN_SWITCH)
INPUT_PORTS_END

static INPUT_PORTS_START( apple2_gameport )
	PORT_INCLUDE( apple2_joystick )
	//PORT_INCLUDE( apple2_paddle )
INPUT_PORTS_END

static INPUT_PORTS_START( apple2_common )
    PORT_START("keyb_0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90")		PORT_CODE(KEYCODE_LEFT)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return")	PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92")		PORT_CODE(KEYCODE_RIGHT)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc")		PORT_CODE(KEYCODE_ESC)		PORT_CHAR(27)

    PORT_START("keyb_1")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR(':') PORT_CHAR('*')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.') PORT_CHAR('>')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/') PORT_CHAR('?')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)		PORT_CHAR('0')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')

    PORT_START("keyb_2")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)	PORT_CHAR('2') PORT_CHAR('\"')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)	PORT_CHAR('3') PORT_CHAR('#')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)	PORT_CHAR('4') PORT_CHAR('$')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)	PORT_CHAR('5') PORT_CHAR('%')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)	PORT_CHAR('6') PORT_CHAR('&')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR('\'')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)	PORT_CHAR('8') PORT_CHAR('(')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR(')')

    PORT_START("keyb_3")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)	PORT_CHAR(';') PORT_CHAR('+')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)	PORT_CHAR('-') PORT_CHAR('=')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)		PORT_CHAR('A')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)		PORT_CHAR('B')

    PORT_START("keyb_4")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)	PORT_CHAR('C')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)	PORT_CHAR('D')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)	PORT_CHAR('E')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)	PORT_CHAR('F')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)	PORT_CHAR('G')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)	PORT_CHAR('H')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)	PORT_CHAR('I')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)	PORT_CHAR('J')

    PORT_START("keyb_5")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)	PORT_CHAR('K')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)	PORT_CHAR('L')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)	PORT_CHAR('M') PORT_CHAR(']')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)	PORT_CHAR('N') PORT_CHAR('^')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)	PORT_CHAR('O')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)	PORT_CHAR('P') PORT_CHAR('@')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)	PORT_CHAR('R')

    PORT_START("keyb_6")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 	PORT_CHAR('S')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 	PORT_CHAR('T')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 	PORT_CHAR('U')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 	PORT_CHAR('V')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 	PORT_CHAR('W')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 	PORT_CHAR('X')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 	PORT_CHAR('Y')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)	PORT_CHAR('Z')

    PORT_START("keyb_special")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Shift")	PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Shift")	PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control")		PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_SHIFT_2)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RESET")		PORT_CODE(KEYCODE_F12)
INPUT_PORTS_END

static INPUT_PORTS_START( apple2 )
	PORT_INCLUDE(apple2_common)

	PORT_START("keyb_repeat")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("REPT")			PORT_CODE(KEYCODE_BACKSLASH)PORT_CHAR('\\')

	/* other devices */
	PORT_INCLUDE( apple2_gameport )
INPUT_PORTS_END

static INPUT_PORTS_START( apple2p )
	PORT_INCLUDE( apple2 )

	PORT_START("reset_dip")
	PORT_DIPNAME( 0x01, 0x01, "Reset" )
	PORT_DIPSETTING( 0x01, "CTRL-RESET" )
	PORT_DIPSETTING( 0x00, "RESET" )
INPUT_PORTS_END

static INPUT_PORTS_START( apple2e_common )
    PORT_START("keyb_0")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Delete")	PORT_CODE(KEYCODE_BACKSPACE)PORT_CHAR(8)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x90")		PORT_CODE(KEYCODE_LEFT)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab")		PORT_CODE(KEYCODE_TAB)		PORT_CHAR(9)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x93")		PORT_CODE(KEYCODE_DOWN)		PORT_CHAR(10)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91")		PORT_CODE(KEYCODE_UP)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Return")	PORT_CODE(KEYCODE_ENTER)	PORT_CHAR(13)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92")		PORT_CODE(KEYCODE_RIGHT)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc")		PORT_CODE(KEYCODE_ESC)		PORT_CHAR(27)

    PORT_START("keyb_1")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)	PORT_CHAR(' ')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE)	PORT_CHAR('\'') PORT_CHAR('\"')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA)	PORT_CHAR(',') PORT_CHAR('<')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS)	PORT_CHAR('-') PORT_CHAR('_')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)	PORT_CHAR('.') PORT_CHAR('>')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH)	PORT_CHAR('/') PORT_CHAR('?')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)		PORT_CHAR('0') PORT_CHAR(')')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)		PORT_CHAR('1') PORT_CHAR('!')

    PORT_START("keyb_2")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)	PORT_CHAR('2') PORT_CHAR('@')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)	PORT_CHAR('3') PORT_CHAR('#')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)	PORT_CHAR('4') PORT_CHAR('$')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)	PORT_CHAR('5') PORT_CHAR('%')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)	PORT_CHAR('6') PORT_CHAR('^')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR('&')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)	PORT_CHAR('8') PORT_CHAR('*')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR('(')

    PORT_START("keyb_3")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';') PORT_CHAR(':')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=') PORT_CHAR('+')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[') PORT_CHAR('{')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\') PORT_CHAR('|')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']') PORT_CHAR('}')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('`') PORT_CHAR('~')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)			PORT_CHAR('A') PORT_CHAR('a')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)			PORT_CHAR('B') PORT_CHAR('b')

    PORT_START("keyb_4")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)	PORT_CHAR('C') PORT_CHAR('c')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)	PORT_CHAR('D') PORT_CHAR('d')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)	PORT_CHAR('E') PORT_CHAR('e')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)	PORT_CHAR('F') PORT_CHAR('f')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)	PORT_CHAR('G') PORT_CHAR('g')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)	PORT_CHAR('H') PORT_CHAR('h')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)	PORT_CHAR('I') PORT_CHAR('i')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)	PORT_CHAR('J') PORT_CHAR('j')

    PORT_START("keyb_5")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)	PORT_CHAR('K') PORT_CHAR('k')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)	PORT_CHAR('L') PORT_CHAR('l')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)	PORT_CHAR('M') PORT_CHAR('m')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)	PORT_CHAR('N') PORT_CHAR('n')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)	PORT_CHAR('O') PORT_CHAR('o')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)	PORT_CHAR('P') PORT_CHAR('p')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)	PORT_CHAR('Q') PORT_CHAR('q')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)	PORT_CHAR('R') PORT_CHAR('r')

    PORT_START("keyb_6")
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) 	PORT_CHAR('S') PORT_CHAR('s')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) 	PORT_CHAR('T') PORT_CHAR('t')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) 	PORT_CHAR('U') PORT_CHAR('u')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) 	PORT_CHAR('V') PORT_CHAR('v')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) 	PORT_CHAR('W') PORT_CHAR('w')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) 	PORT_CHAR('X') PORT_CHAR('x')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) 	PORT_CHAR('Y') PORT_CHAR('y')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)	PORT_CHAR('Z') PORT_CHAR('z')

	PORT_START("keyb_special")
    PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_KEYBOARD) PORT_NAME("Caps Lock")	PORT_CODE(KEYCODE_CAPSLOCK)	PORT_TOGGLE
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Shift")	PORT_CODE(KEYCODE_LSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Shift")	PORT_CODE(KEYCODE_RSHIFT)	PORT_CHAR(UCHAR_SHIFT_1)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Control")		PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_SHIFT_2)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Open Apple")	PORT_CODE(KEYCODE_LALT)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Solid Apple")	PORT_CODE(KEYCODE_RALT)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("RESET")		PORT_CODE(KEYCODE_F12)
INPUT_PORTS_END

static INPUT_PORTS_START( apple2_keypad )
	PORT_START("keypad_1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0_PAD)	PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1_PAD)	PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2_PAD)	PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3_PAD)	PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4_PAD)	PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5_PAD)	PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6_PAD)	PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7_PAD)	PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("keypad_2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8_PAD)		PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9_PAD)		PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH_PAD)	PORT_CHAR(UCHAR_MAMEKEY(SLASH_PAD))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ASTERISK)	PORT_CHAR(UCHAR_MAMEKEY(ASTERISK))
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS_PAD)	PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_PLUS_PAD) 	PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL_PAD)		PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD)	PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
INPUT_PORTS_END

static INPUT_PORTS_START( apple2e )
	PORT_INCLUDE( apple2e_common )
	PORT_INCLUDE( apple2_gameport )
INPUT_PORTS_END

INPUT_PORTS_START( apple2ep )
	PORT_INCLUDE( apple2e_common )
	PORT_INCLUDE( apple2_gameport )
	PORT_INCLUDE( apple2_keypad )
INPUT_PORTS_END

/* according to Steve Nickolas (author of Dapple), our original palette would
 * have been more appropriate for an Apple IIgs.  So we've substituted in the
 * Robert Munafo palette instead, which is more accurate on 8-bit Apples
 */
static const rgb_t apple2_palette[] =
{
	RGB_BLACK,
	MAKE_RGB(0xE3, 0x1E, 0x60),	/* Dark Red */
	MAKE_RGB(0x60, 0x4E, 0xBD),	/* Dark Blue */
	MAKE_RGB(0xFF, 0x44, 0xFD),	/* Purple */
	MAKE_RGB(0x00, 0xA3, 0x60),	/* Dark Green */
	MAKE_RGB(0x9C, 0x9C, 0x9C),	/* Dark Gray */
	MAKE_RGB(0x14, 0xCF, 0xFD),	/* Medium Blue */
	MAKE_RGB(0xD0, 0xC3, 0xFF),	/* Light Blue */
	MAKE_RGB(0x60, 0x72, 0x03),	/* Brown */
	MAKE_RGB(0xFF, 0x6A, 0x3C),	/* Orange */
	MAKE_RGB(0x9C, 0x9C, 0x9C),	/* Light Grey */
	MAKE_RGB(0xFF, 0xA0, 0xD0),	/* Pink */
	MAKE_RGB(0x14, 0xF5, 0x3C),	/* Light Green */
	MAKE_RGB(0xD0, 0xDD, 0x8D),	/* Yellow */
	MAKE_RGB(0x72, 0xFF, 0xD0),	/* Aquamarine */
	RGB_WHITE
};

/* Initialize the palette */
PALETTE_INIT( apple2 )
{
	palette_set_colors(machine, 0, apple2_palette, ARRAY_LENGTH(apple2_palette));
}

static const ay8910_interface apple2_ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	NULL
};

static MACHINE_DRIVER_START( apple2_common )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6502, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
	MDRV_CPU_PROGRAM_MAP(apple2_map, 0)
	MDRV_CPU_VBLANK_INT_HACK(apple2_interrupt, 192/8)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START( apple2 )

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(280*2, 192)
	MDRV_SCREEN_VISIBLE_AREA(0, (280*2)-1,0,192-1)
	MDRV_PALETTE_LENGTH(ARRAY_LENGTH(apple2_palette))
	MDRV_PALETTE_INIT(apple2)

	MDRV_VIDEO_START(apple2)
	MDRV_VIDEO_UPDATE(apple2)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("A2DAC", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("ay8913.1", AY8913, 1022727)
	MDRV_SOUND_CONFIG(apple2_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("ay8913.2", AY8913, 1022727)
	MDRV_SOUND_CONFIG(apple2_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* slot devices */
	MDRV_DEVICE_ADD("langcard", APPLE2_LANGCARD)
	MDRV_DEVICE_ADD("mockingboard", MOCKINGBOARD)
	MDRV_DEVICE_ADD("fdc", APPLEFDC)
	MDRV_DEVICE_CONFIG(apple2_fdc_interface)

	/* slots */
	MDRV_APPLE2_SLOT_ADD(0, "langcard", apple2_langcard_r, apple2_langcard_w)
	MDRV_APPLE2_SLOT_ADD(4, "mockingboard", mockingboard_r, mockingboard_w)
	MDRV_APPLE2_SLOT_ADD(6, "fdc", applefdc_r, applefdc_w)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2 )
	MDRV_IMPORT_FROM( apple2_common )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2p )
	MDRV_IMPORT_FROM( apple2_common )
	MDRV_VIDEO_START(apple2p)
MACHINE_DRIVER_END

ROM_START(las3000)
	ROM_REGION(0x0800,"gfx1",0)
	ROM_LOAD ( "a2.chr", 0x0000, 0x0800, CRC(64f415c6) SHA1(f9d312f128c9557d9d6ac03bfad6c3ddf83e5659))

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD ( "las3000.rom", 0x0000, 0x8000, CRC(9C7AEB09) SHA1(3302ADF41E258CF50210C19736948C8FA65E91DE))
	ROM_LOAD ( "l3kdisk.rom", 0x0500, 0x0100, CRC(2D4B1584) SHA1(989780B77E100598124DF7B72663E5A31A3339C0))
ROM_END

MACHINE_DRIVER_START( apple2e )
	MDRV_IMPORT_FROM( apple2_common )
	MDRV_VIDEO_START(apple2e)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2ee )
	MDRV_IMPORT_FROM( apple2e )
	MDRV_CPU_REPLACE("main", M65C02, 1021800)		/* close to actual CPU frequency of 1.020484 MHz */
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2c )
	MDRV_IMPORT_FROM( apple2ee )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( apple2c_iwm )
	MDRV_IMPORT_FROM( apple2c )

	/* replace the old-style FDC with an IWM */
	MDRV_DEVICE_REMOVE("fdc", APPLEFDC)
	MDRV_DEVICE_ADD("fdc", IWM)
	MDRV_DEVICE_CONFIG(apple2_fdc_interface)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2)
	ROM_REGION(0x0800,"gfx1",0)
	ROM_LOAD ( "a2.chr", 0x0000, 0x0800, CRC(64f415c6) SHA1(f9d312f128c9557d9d6ac03bfad6c3ddf83e5659))

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD_OPTIONAL ( "341-0016.rom", 0x1000, 0x0800, CRC(4234e88a) SHA1(c9a81d704dc2f0c3416c20f9c4ab71fedda937ed))

/* The area $D800-$DFFF in Apple II is reserved for 3rd party add-ons:
   Maybe MESS should map this space to a CARTSLOT device?              */

	ROM_LOAD ( "a2.e0", 0x2000, 0x0800, CRC(c0a4ad3b) SHA1(bf32195efcb34b694c893c2d342321ec3a24b98f))
	ROM_LOAD ( "a2.e8", 0x2800, 0x0800, CRC(a99c2cf6) SHA1(9767d92d04fc65c626223f25564cca31f5248980))
	ROM_LOAD ( "a2.f0", 0x3000, 0x0800, CRC(62230d38) SHA1(f268022da555e4c809ca1ae9e5d2f00b388ff61c))
	ROM_LOAD ( "a2.f8", 0x3800, 0x0800, CRC(020a86d0) SHA1(52a18bd578a4694420009cad7a7a5779a8c00226))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2p)
	ROM_REGION(0x0800,"gfx1",0)
	ROM_LOAD ( "341-0036.chr", 0x0000, 0x0800, CRC(64f415c6) SHA1(f9d312f128c9557d9d6ac03bfad6c3ddf83e5659))

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ( "341-0011.d0", 0x1000, 0x0800, CRC(6f05f949) SHA1(0287ebcef2c1ce11dc71be15a99d2d7e0e128b1e))
	ROM_LOAD ( "341-0012.d8", 0x1800, 0x0800, CRC(1f08087c) SHA1(a75ce5aab6401355bf1ab01b04e4946a424879b5))
	ROM_LOAD ( "341-0013.e0", 0x2000, 0x0800, CRC(2b8d9a89) SHA1(8d82a1da63224859bd619005fab62c4714b25dd7))
	ROM_LOAD ( "341-0014.e8", 0x2800, 0x0800, CRC(5719871a) SHA1(37501be96d36d041667c15d63e0c1eff2f7dd4e9))
	ROM_LOAD ( "341-0015.f0", 0x3000, 0x0800, CRC(9a04eecf) SHA1(e6bf91ed28464f42b807f798fc6422e5948bf581))
	ROM_LOAD ( "341-0020.f8", 0x3800, 0x0800, CRC(079589c4) SHA1(a28852ff997b4790e53d8d0352112c4b1a395098))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2jp)
	ROM_REGION(0x0800,"gfx1",0)
	ROM_LOAD ( "a2jp.chr", 0x0000, 0x0800, CRC(487104b5) SHA1(0a382be58db5215c4a3de53b19a72fab660d5da2)) // not confirmed as the actual rom on motherboard

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ( "a2p.d0", 0x1000, 0x0800, CRC(6f05f949) SHA1(0287ebcef2c1ce11dc71be15a99d2d7e0e128b1e)) // not confirmed as the actual rom on motherboard
	ROM_LOAD ( "a2p.d8", 0x1800, 0x0800, CRC(1f08087c) SHA1(a75ce5aab6401355bf1ab01b04e4946a424879b5)) // not confirmed as the actual rom on motherboard
	ROM_LOAD ( "a2p.e0", 0x2000, 0x0800, CRC(2b8d9a89) SHA1(8d82a1da63224859bd619005fab62c4714b25dd7)) // not confirmed as the actual rom on motherboard
	ROM_LOAD ( "a2p.e8", 0x2800, 0x0800, CRC(5719871a) SHA1(37501be96d36d041667c15d63e0c1eff2f7dd4e9)) // not confirmed as the actual rom on motherboard
	ROM_LOAD ( "a2p.f0", 0x3000, 0x0800, CRC(9a04eecf) SHA1(e6bf91ed28464f42b807f798fc6422e5948bf581)) // not confirmed as the actual rom on motherboard
	ROM_LOAD ( "a2jp.f8", 0x3800, 0x0800, CRC(6ea8379b) SHA1(00a75ae3b58e1917ad640249366f654608589cf4))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(ace100)
	ROM_REGION(0x0800,"gfx1",0)
	ROM_LOAD ( "ace100.chr", 0x0000, 0x0800, BAD_DUMP CRC(64f415c6) SHA1(f9d312f128c9557d9d6ac03bfad6c3ddf83e5659)) // copy of a2.chr - real Ace chr is undumped

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ( "ace100.rom", 0x1000, 0x3000, CRC(9d5ec94f) SHA1(8f2b3f2561788bebc7a805f620ec9e7ade973460))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2e)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3420133a.chr", 0x0000, 0x1000,CRC(b081df66) SHA1(7060de104046736529c1e8a687a0dd7b84f8c51b))
	ROM_LOAD ( "3420133a.chr", 0x1000, 0x1000,CRC(b081df66) SHA1(7060de104046736529c1e8a687a0dd7b84f8c51b))

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ( "3420135b.64", 0x0000, 0x2000, CRC(e248835e) SHA1(523838c19c79f481fa02df56856da1ec3816d16e))
	ROM_LOAD ( "3420134a.64", 0x2000, 0x2000, CRC(fc3d59d8) SHA1(8895a4b703f2184b673078f411f4089889b61c54))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ee)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3420265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3420265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ( "3420304a.64", 0x0000, 0x2000, CRC(443aa7c4) SHA1(3aecc56a26134df51e65e17f33ae80c1f1ac93e6))
	ROM_LOAD ( "3420303a.64", 0x2000, 0x2000, CRC(95e10034) SHA1(afb09bb96038232dc757d40c0605623cae38088e))
	ROM_LOAD ( "3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2ep)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3420265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3420265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x4700,"main",0)
	ROM_LOAD ("320349b.128", 0x0000, 0x4000, CRC(1d70b193) SHA1(b8ea90abe135a0031065e01697c4a3a20d51198b))
	ROM_LOAD ("3410027a.rom", 0x4500, 0x0100, CRC(ce7144f6) SHA1(d4181c9f046aafc3fb326b381baac809d9e38d16)) /* Disk II ROM - DOS 3.3 version */
ROM_END

ROM_START(apple2c)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x4000,"main",0)
	ROM_LOAD ( "a2c.128", 0x0000, 0x4000, CRC(f0edaa1b) SHA1(1a9b8aca5e32bb702ddb7791daddd60a89655729))
ROM_END

ROM_START(apple2c0)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("3420033a.256", 0x0000, 0x8000, CRC(c8b979b3) SHA1(10767e96cc17bad0970afda3a4146564e6272ba1))
ROM_END

ROM_START(apple2c3)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("3420445a.256", 0x0000, 0x8000, CRC(bc5a79ff) SHA1(5338d9baa7ae202457b6500fde5883dbdc86e5d3))
ROM_END

ROM_START(apple2c4)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("3410445b.256", 0x0000, 0x8000, CRC(06f53328) SHA1(015061597c4cda7755aeb88b735994ffd2f235ca))
ROM_END

ROM_START(laser128)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10)) // need to dump real laser rom
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10)) // need to dump real laser rom

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("laser128.256", 0x0000, 0x8000, CRC(39E59ED3) SHA1(CBD2F45C923725BFD57F8548E65CC80B13BC18DA))
ROM_END

ROM_START(las128ex)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10)) // need to dump real laser rom
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10)) // need to dump real laser rom

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("las128ex.256", 0x0000, 0x8000, CRC(B67C8BA1) SHA1(8BD5F82A501B1CF9D988C7207DA81E514CA254B0))
ROM_END


ROM_START(apple2cp)
	ROM_REGION(0x2000,"gfx1",0)
	ROM_LOAD ( "3410265a.chr", 0x0000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))
	ROM_LOAD ( "3410265a.chr", 0x1000, 0x1000,CRC(2651014d) SHA1(b2b5d87f52693817fc747df087a4aa1ddcdb1f10))

	ROM_REGION(0x8700,"main",0)
	ROM_LOAD("3410625a.256", 0x0000, 0x8000, CRC(0b996420) SHA1(1a27ae26966bbafd825d08ad1a24742d3e33557c))
ROM_END


static void apple2_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND:	info->i = 15; break;
		case MESS_DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR:	info->i = 16; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_NAME+0:						strcpy(info->s = device_temp_str(), "slot6disk1"); break;
		case MESS_DEVINFO_STR_NAME+1:						strcpy(info->s = device_temp_str(), "slot6disk2"); break;
		case MESS_DEVINFO_STR_SHORT_NAME+0:					strcpy(info->s = device_temp_str(), "s6d1"); break;
		case MESS_DEVINFO_STR_SHORT_NAME+1:					strcpy(info->s = device_temp_str(), "s6d2"); break;
		case MESS_DEVINFO_STR_DESCRIPTION+0:					strcpy(info->s = device_temp_str(), "Slot 6 Disk #1"); break;
		case MESS_DEVINFO_STR_DESCRIPTION+1:					strcpy(info->s = device_temp_str(), "Slot 6 Disk #2"); break;

		default:										apple525_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(apple2_common)
	CONFIG_DEVICE(apple2_floppy_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(apple2)
	CONFIG_IMPORT_FROM( apple2_common )
	CONFIG_RAM				(4  * 1024)
	CONFIG_RAM				(8  * 1024)
	CONFIG_RAM				(12 * 1024)
	CONFIG_RAM				(16 * 1024)
	CONFIG_RAM				(20 * 1024)
	CONFIG_RAM				(24 * 1024)
	CONFIG_RAM				(32 * 1024)
	CONFIG_RAM				(36 * 1024)
	CONFIG_RAM				(48 * 1024)
	CONFIG_RAM_DEFAULT		(64 * 1024)	/* At the moment the RAM bank $C000-$FFFF is available only if you choose   */
										/* default configuration: on real machine is present also in configurations */
										/* with less memory, provided that the language card is installed           */
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(apple2p)
	CONFIG_IMPORT_FROM( apple2_common )
	CONFIG_RAM				(16 * 1024)
	CONFIG_RAM				(32 * 1024)
	CONFIG_RAM				(48 * 1024)
	CONFIG_RAM_DEFAULT		(64 * 1024)	/* At the moment the RAM bank $C000-$FFFF is available only if you choose   */
										/* default configuration: on real machine is present also in configurations */
										/* with less memory, provided that the language card is installed           */
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(apple2e)
	CONFIG_IMPORT_FROM( apple2_common )
	CONFIG_RAM				(64  * 1024)
	CONFIG_RAM_DEFAULT		(128 * 1024)
SYSTEM_CONFIG_END



/*    YEAR  NAME      PARENT    COMPAT      MACHINE       INPUT     INIT CONFIG     COMPANY            FULLNAME */
COMP( 1977, apple2,   0,        0,			apple2,       apple2,   0,   apple2,	"Apple Computer", "Apple ][" , 0)
COMP( 1979, apple2p,  apple2,   0,			apple2p,      apple2p,  0,   apple2p,	"Apple Computer", "Apple ][+" , 0)
COMP( 1980, apple2jp, apple2,   0,			apple2p,      apple2p,  0,   apple2p,	"Apple Computer", "Apple ][j+" , 0)
COMP( 1982, ace100,   apple2,   0,			apple2,	      apple2e,  0,   apple2,	"Franklin Computer", "Franklin Ace 100" , 0)
COMP( 1983, apple2e,  0,        apple2,		apple2e,      apple2e,  0,   apple2e,	"Apple Computer", "Apple //e" , 0)
COMP( 1985, apple2ee, apple2e,  0,			apple2ee,     apple2e,  0,   apple2e,	"Apple Computer", "Apple //e (enhanced)" , 0)
COMP( 1987, apple2ep, apple2e,  0,			apple2ee,     apple2ep, 0,   apple2e,	"Apple Computer", "Apple //e (Platinum)" , 0)
COMP( 1984, apple2c,  0,        apple2,		apple2c,      apple2e,  0,   apple2e,	"Apple Computer", "Apple //c" , 0)
COMP( 1983, las3000,  apple2,   0,			apple2p,      apple2p,  0,   apple2p,	"Video Technology", "Laser 3000",		GAME_NOT_WORKING )
COMP( 1987, laser128, 0,        apple2c0,	apple2c,      apple2e,  0,   apple2e,	"Video Technology", "Laser 128 (rev 4)",		GAME_NOT_WORKING )
COMP( 1987, las128ex, apple2c,  0,			apple2c,      apple2e,  0,   apple2e,	"Video Technology", "Laser 128ex (rev 4a)",		GAME_NOT_WORKING )
COMP( 1985, apple2c0, apple2c,  0,			apple2c_iwm,  apple2e,  0,   apple2e,	"Apple Computer", "Apple //c (UniDisk 3.5)" , 0)
COMP( 1986, apple2c3, apple2c,  0,			apple2c_iwm,  apple2e,  0,	 apple2e,	"Apple Computer", "Apple //c (Original Memory Expansion)" , 0)
COMP( 1986, apple2c4, apple2c,  0,			apple2c_iwm,  apple2e,  0,	 apple2e,	"Apple Computer", "Apple //c (rev 4)" , GAME_NOT_WORKING )
COMP( 1988, apple2cp, apple2c,  0,			apple2c_iwm,  apple2e,  0,	 apple2e,	"Apple Computer", "Apple //c Plus" , 0)
