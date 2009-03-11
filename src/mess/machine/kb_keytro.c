/***************************************************************************

TODO:
- Add support for PF01 - PF24

Emulation file for Keytronic KB3270/PC keyboard by Wilbert Pol.

This keyboard supports both the PC/XT and the PC/AT keyboard protocols. The
desired protocol can be selected by a switch on the keybaord.

Keyboard matrix.

Z5 = XR 22-00950-001
Z6 = SN74LS132N
Z7 = XR 22-00950-001
Z8 = SN74LS374N
Z11 = XR 22-908-03B


  |           Z11 pin 8
  |           |                Z11 pin 7
  |           |                |        Z11 pin 6
  |           |                |        |        Z11 pin 5
  |           |                |        |        |        Z11 pin 4
  |           |                |        |        |        |
  F1 -------- F2 ------------- F3 ----- F4 ----- F5 ----- F6 -------- J2 pin 2 - Z5 pin 19
  |           |                |        |        |        |
  F7 -------- F8 ------------- LShift - < ------ Z ------ X --------- J2 pin 1 - Z5 pin 18
  |           |                |        |        |        |
  F9 -------- F10 ------------ LCtrl -- LAlt --- Space -- RAlt ------ J1 pin 9 - Z5 pin 17
  |           |                |        |        |        |
  1 --------- ` -------------- Q ------ TAB ---- A ------ Caps Lock - J1 pin 8 - Z5 pin 16
  |           |                |        |        |        |
  2 --------- 3 -------------- W ------ E ------ S ------ D --------- J1 pin 7 - Z5 pin 15
  |           |                |        |        |        |
  5 --------- 4 -------------- T ------ R ------ G ------ F --------- J2 pin 3 - Z7 pin 20
  |           |                |        |        |        |
  N --------- M -------------- B ------ V ------ C ------ , -------------------- Z7 pin 19
  |           |                |        |        |        |
  6 --------- 7 -------------- Y ------ U ------ H ------ J -------------------- Z7 pin 18
  |           |                |        |        |        |
  9 --------- 8 -------------- O ------ I ------ L ------ K -------------------- Z7 pin 17
  |           |                |        |        |        |
  Pad 2 ----- Pad 1 ---------- Unused - Unused - Down --- Enter ---------------- Z7 pin 16
  |           |                |        |        |        |
  |           / -------------- Rshift - Rshift - Left --- . --------- J9 pin 8 - Z7 pin 15
  |           |                |        |        |        |
  0 --------- - -------------- P ------ [ ------ ; ------ ' --------- J9 pin 7 - Z7 pin 14
  |           |                |        |        |        |
  Backspace - = -------------- Return - \ ------ Return - ] --------- J9 pin 6 - Z7 pin 13
  |           |                |        |        |        |
  Backspace - PA1/Dup -------- |<-- --- /a\ ---- Pad + -- Pad + ----- J9 pin 5 - Z7 pin 12
  |           |                |        |        |        |
  Esc ------- NumLock -------- Pad 7 -- Pad 8 -- Pad 4 -- Pad 5 ------J9 pin 3 - Z7 pin 2
  |           |                |        |        |        |
  SysReq ---- ScrLk ---------- -->| --- Pad 9 -- Pad - -- Pad 6 ---------------- goes under Z8 between pins 5 and 6 to Z8 pin 15?
  |           |                |        |        |        |
  Pad 3 ----- Pad 0 ---------- Pad 0 -- Pad . -- Unused - Up ------------------- Z7 pin 7
  |           |                |        |        |        |
  PrtSc * --- PA2/Field Mark - Right -- /a ----- Center - Unused ---- J9 pin 2 - Z7 pin 3
  |           |                |        |        |        |

The 'Return', 'Backspace, 'Pad 0', 'Right Shift', 'Pad +'  keys on the keyboard
are attached to two switches. The keys appear twice in the keyboard matrix.

***************************************************************************/

#include "driver.h"
#include "machine/kb_keytro.h"
#include "cpu/mcs51/mcs51.h"

#define	LOG		0

static struct {
	UINT8					p1;
	UINT8					p1_data;
	UINT8					p2;
	UINT8					p3;
	UINT8					clock_signal;
	UINT8					data_signal;
	write8_space_func		clock_callback;
	write8_space_func		data_callback;
	const device_config *	cpu;
	UINT16					last_write_addr;
} kb_keytronic;


/***************************************************************************

  Input port declaration

***************************************************************************/
INPUT_PORTS_START( kb_keytronic )
	PORT_START( "kb_keytronic_0b" )
	PORT_DIPNAME( 0x01, 0x00, "Protocol selection" )
	PORT_DIPSETTING( 0x00, "Enhanced XT, AT and PS/2 models" )
	PORT_DIPSETTING( 0x01, "Standard PC and XT" )
	PORT_DIPNAME( 0x02, 0x02, "IRMA/Native scan code set" )
	PORT_DIPSETTING( 0x00, "Native scan code set" )
	PORT_DIPSETTING( 0x02, "IRMA Emulation" )
	PORT_DIPNAME( 0x04, 0x04, "Enhanced 101/Native scan code set" )
	PORT_DIPSETTING( 0x00, "Native scan code set" )
	PORT_DIPSETTING( 0x04, "Enhanced 101 scan code set" )
	PORT_DIPNAME( 0x08, 0x08, "Enable E0" )
	PORT_DIPSETTING( 0x00, "Enable E0" )
	PORT_DIPSETTING( 0x08, "Disable E0" )
	PORT_DIPNAME( 0x10, 0x10, "Code tables" )
	PORT_DIPSETTING( 0x00, "U.S. code tables" )
	PORT_DIPSETTING( 0x10, "International code tables" )
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Key click" )
	PORT_DIPSETTING( 0x00, "No key click" )
	PORT_DIPSETTING( 0x80, "Key click" )

	PORT_START( "kb_keytronic_0f" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_5)			PORT_CHAR('5')						/* 06 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_4)			PORT_CHAR('4')						/* 05 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_T)			PORT_CHAR('T')						/* 14 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_R)			PORT_CHAR('R')						/* 13 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_G)			PORT_CHAR('G')						/* 22 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F)			PORT_CHAR('F')						/* 21 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F7)			PORT_CHAR(UCHAR_MAMEKEY(F7))		/* 41 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?6a?")					/* 6a */

	PORT_START( "kb_keytronic_30_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_N)			PORT_CHAR('N')						/* 31 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_M)			PORT_CHAR('M')						/* 32 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_B)			PORT_CHAR('B')						/* 30 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_V)			PORT_CHAR('V')						/* 2f */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_C)			PORT_CHAR('C')						/* 2e */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',')						/* 33 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_30_1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?58?")					/* 58 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?59?")					/* 59 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?5a?")					/* 5a */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?5b?")					/* 5b */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?5c?")					/* 5c */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?5d?")					/* 5d */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?6b?")					/* 6b */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F8)			PORT_CHAR(UCHAR_MAMEKEY(F8))		/* 42 */

	PORT_START( "kb_keytronic_31_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_6)			PORT_CHAR('6')						/* 07 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_7)			PORT_CHAR('7')						/* 08 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_Y)			PORT_CHAR('Y')						/* 15 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_U)			PORT_CHAR('U')						/* 16 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_H)			PORT_CHAR('H')						/* 23 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_J)			PORT_CHAR('J')						/* 24 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_31_1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?37?")					/* 37 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?5f?")					/* 5f */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LSHIFT)		PORT_NAME("LShift")					/* 2a */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("<")						/* 70 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_Z)			PORT_CHAR('Z')						/* 2c */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_X)			PORT_CHAR('X')						/* 2d */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?6c?")					/* 6c */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F9)			PORT_CHAR(UCHAR_MAMEKEY(F9))		/* 43 */

	PORT_START( "kb_keytronic_32_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_9)			PORT_CHAR('9')						/* 0a */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_8)			PORT_CHAR('8')						/* 09 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_O)			PORT_CHAR('O')						/* 18 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_I)			PORT_CHAR('I')						/* 17 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_L)			PORT_CHAR('L')						/* 26 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_K)			PORT_CHAR('K')						/* 25 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_32_1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?57?")					/* 57 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?1d?")					/* 1d */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LCONTROL)		PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))	/* 71 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LALT)			PORT_NAME("LAlt")					/* 38 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SPACE)		PORT_CHAR(' ')						/* 39 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_RALT)			PORT_NAME("RAlt")					/* 38 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?69?")					/* 69 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F6)			PORT_CHAR(UCHAR_MAMEKEY(F6))		/* 40 */

	PORT_START( "kb_keytronic_33_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_2_PAD)		PORT_NAME("KP 2")					/* 50 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_1_PAD)		PORT_NAME("KP 1")					/* 4f */
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_DOWN)			PORT_NAME("Down")					/* 55 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("Enter")					/* 75 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_33_1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_1)			PORT_CHAR('1')						/* 02 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_TILDE)		PORT_CHAR('`')						/* 29 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_Q)			PORT_CHAR('Q')						/* 10 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_TAB)			PORT_CHAR(9)						/* 0f */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_A)			PORT_CHAR('A')						/* 1e */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_CAPSLOCK)		PORT_NAME("Caps")					/* 3a */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?68?")					/* 68 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F5)			PORT_CHAR(UCHAR_MAMEKEY(F5))		/* 3f */

	PORT_START( "kb_keytronic_34_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/')						/* 35 */
	PORT_BIT( 0x0c, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_RSHIFT)		PORT_CHAR(UCHAR_MAMEKEY(RSHIFT))	/* 36 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_LEFT)			PORT_NAME("Left")					/* 56 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.')						/* 34 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_34_1" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_2)			PORT_CHAR('2')						/* 02 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_3)			PORT_CHAR('3')						/* 03 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_W)			PORT_CHAR('W')						/* 11 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_E)			PORT_CHAR('E')						/* 12 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_S)			PORT_CHAR('S')						/* 1f */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_D)			PORT_CHAR('D')						/* 20 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?67?")					/* 67 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F4)			PORT_CHAR(UCHAR_MAMEKEY(F4))		/* 3e */

	PORT_START( "kb_keytronic_35_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_0)			PORT_CHAR('0')						/* 0b */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-')						/* 0c */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_P)			PORT_CHAR('P')						/* 19 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[')						/* 1a */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';')						/* 27 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_QUOTE)		PORT_CHAR('\'')						/* 28 */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_35_1" )
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?66?")					/* 66 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F3)			PORT_CHAR(UCHAR_MAMEKEY(F3))		/* 3d */

	PORT_START( "kb_keytronic_36_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(8)						/* 0e */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_EQUALS)		PORT_CHAR('=')						/* 0d */
	PORT_BIT( 0x14, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_ENTER)		PORT_CHAR(13)						/* 1c */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('\\')						/* 2b */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']')						/* 1b */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_37_0" )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("PA1")					/* 7b */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("|<--")					/* 7e */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("/a\\") 					/* 7a */
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_PLUS_PAD)		PORT_NAME("KP +")					/* 4e */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_37_1" )
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?64?")					/* 64 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F1)			PORT_CHAR(UCHAR_MAMEKEY(F1))		/* 3b */

	PORT_START( "kb_keytronic_38_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("SysReq")					/* 54 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_SCRLOCK)		PORT_NAME("ScrLock")				/* 46 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("-->|")					/* 7c */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_9_PAD)		PORT_NAME("KP 9")					/* 49 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_MINUS_PAD)	PORT_NAME("KP -")					/* 4a */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_6_PAD)		PORT_NAME("KP 6")					/* 4d */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START( "kb_keytronic_39_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_ESC)			PORT_NAME("Esc")					/* 01 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_NUMLOCK)		PORT_NAME("NumLock")				/* 45 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_7_PAD)		PORT_NAME("KP 7")					/* 47 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_8_PAD)		PORT_NAME("KP 8")					/* 48 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_4_PAD)		PORT_NAME("KP 4")					/* 4b */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_5_PAD)		PORT_NAME("KP 5")					/* 4c */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?76?")					/* 76 */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?63?")					/* 63 */

	PORT_START( "kb_keytronic_3a_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("PrtSc *")				/* 6f */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("PA2")					/* 7f */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_RIGHT)		PORT_NAME("Right")					/* 7d */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("/a")						/* 79 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("Center")					/* 77 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?6e?")					/* 6e */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?62?")					/* 62 */

	PORT_START( "kb_keytronic_3b_0" )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_3_PAD)		PORT_NAME("KP 3")					/* 51 */
	PORT_BIT( 0x06, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_0_PAD)		PORT_NAME("KP 0")					/* 52 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_DEL_PAD)		PORT_NAME("KP .")					/* 53 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_UP)			PORT_NAME("Up")						/* 78 */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )									PORT_NAME("?6d?")					/* 6d */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )	PORT_CODE(KEYCODE_F10)			PORT_CHAR(UCHAR_MAMEKEY(F10))		/* 44 */

INPUT_PORTS_END


/* Write handler which is called when the clock signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_clock_signal )
{
	kb_keytronic.clock_signal = data;
	cpu_set_input_line( kb_keytronic.cpu, MCS51_INT0_LINE, data ? ASSERT_LINE : CLEAR_LINE );
}


/* Write handler which is called when the data signal may have changed */
WRITE8_HANDLER( kb_keytronic_set_data_signal )
{
	kb_keytronic.data_signal = data;
	cpu_set_input_line( kb_keytronic.cpu, MCS51_T0_LINE, data ? ASSERT_LINE : CLEAR_LINE );
}


void kb_keytronic_set_host_interface( running_machine *machine, write8_space_func clock_cb, write8_space_func data_cb )
{
	kb_keytronic.clock_callback = clock_cb;
	kb_keytronic.data_callback = data_cb;
	kb_keytronic.p3 = 0xff;
	kb_keytronic.cpu = cputag_get_cpu( machine, KEYTRONIC_KB3270PC_CPU );
}


static READ8_HANDLER( kb_keytronic_p1_r )
{
	return kb_keytronic.p1 & kb_keytronic.p1_data;
}


static WRITE8_HANDLER( kb_keytronic_p1_w )
{
	if (LOG)
		logerror( "kb_keytronic_p1_w(): write %02x\n", data );

	kb_keytronic.p1 = data;
}


static READ8_HANDLER( kb_keytronic_p2_r )
{
	UINT8 data = kb_keytronic.p2;

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p2_w )
{
	if (LOG)
		logerror( "kb_keytronic_p2_w(): write %02x\n", data );

	kb_keytronic.p2 = data;
}


static READ8_HANDLER( kb_keytronic_p3_r )
{
	UINT8 data = kb_keytronic.p3;

	data &= ~ 0x14;

	/* -INT0 signal */
	data |= ( kb_keytronic.clock_signal ? 0x04 : 0x00 );

	/* T0 signal */
	data |= ( kb_keytronic.data_signal ? 0x00 : 0x10 );

	return data;
}


static WRITE8_HANDLER( kb_keytronic_p3_w )
{
	if (LOG)
		logerror( "kb_keytronic_p3_w(): write %02x\n", data );

	kb_keytronic.p3 = data;
}


static READ8_HANDLER( kb_keytronic_data_r )
{
	if (LOG)
		logerror( "kb_keytronic_data_r(): read from %04x\n", offset );

	kb_keytronic.data_signal = ( offset & 0x0100 ) ? 1 : 0;
	kb_keytronic.clock_signal = ( offset & 0x0200 ) ? 1 : 0;

	kb_keytronic.data_callback( space, 0, kb_keytronic.data_signal );
	kb_keytronic.clock_callback( space, 0, kb_keytronic.clock_signal );

	return 0xFF;
}


static WRITE8_HANDLER( kb_keytronic_data_w )
{
	if (LOG)
		logerror( "kb_keytronic_data_w(): write to offset %04x\n", offset );

	/* Check for low->high transition on AD8 */
	if ( ! ( kb_keytronic.last_write_addr & 0x0100 ) && ( offset & 0x0100 ) )
	{
		switch ( kb_keytronic.p1 )
		{
		case 0x0e:
			break;
		case 0x0f:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_0f" );
			break;
		case 0x30:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_30_0" );
			break;
		case 0x31:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_31_0" );
			break;
		case 0x32:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_32_0" );
			break;
		case 0x33:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_33_0" );
			break;
		case 0x34:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_34_0" );
			break;
		case 0x35:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_35_0" );
			break;
		case 0x36:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_36_0" );
			break;
		case 0x37:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_37_0" ) | ( input_port_read( space->machine, "kb_keytronic_36_0" ) & 0x01 );
			break;
		case 0x38:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_38_0" );
			break;
		case 0x39:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_39_0" );
			break;
		case 0x3a:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_3a_0" );
			break;
		case 0x3b:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_3b_0" );
			break;
		}
	}

	/* Check for low->high transition on AD9 */
	if ( ! ( kb_keytronic.last_write_addr & 0x0200 ) && ( offset & 0x0200 ) )
	{
		switch ( kb_keytronic.p1 )
		{
		case 0x0b:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_0b" );
			break;
		case 0x30:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_30_1" );
			break;
		case 0x31:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_31_1" );
			break;
		case 0x32:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_32_1" );
			break;
		case 0x33:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_33_1" );
			break;
		case 0x34:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_34_1" );
			break;
		case 0x35:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_35_1" );
			break;
		case 0x36:
			kb_keytronic.p1_data = 0xFF;
			break;
		case 0x37:
			kb_keytronic.p1_data = input_port_read( space->machine, "kb_keytronic_37_1" );
			break;
		case 0x38:
			kb_keytronic.p1_data = 0xFF;
			break;
		case 0x39:
			kb_keytronic.p1_data = 0xFF;
			break;
		case 0x3a:
			kb_keytronic.p1_data = 0xFF;
			break;
		}
	}

	kb_keytronic.last_write_addr = offset;
}


static ADDRESS_MAP_START( kb_keytronic_program, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0FFF )	AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( kb_keytronic_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x0000, 0xFFFF )					AM_READWRITE( kb_keytronic_data_r, kb_keytronic_data_w )
	AM_RANGE( MCS51_PORT_P1, MCS51_PORT_P1 )	AM_READWRITE( kb_keytronic_p1_r, kb_keytronic_p1_w )
	AM_RANGE( MCS51_PORT_P2, MCS51_PORT_P2 )	AM_READWRITE( kb_keytronic_p2_r, kb_keytronic_p2_w )
	AM_RANGE( MCS51_PORT_P3, MCS51_PORT_P3 )	AM_READWRITE( kb_keytronic_p3_r, kb_keytronic_p3_w )
ADDRESS_MAP_END


MACHINE_DRIVER_START( kb_keytronic )
	MDRV_CPU_ADD( KEYTRONIC_KB3270PC_CPU, I8051, 11060250 )
	MDRV_CPU_PROGRAM_MAP( kb_keytronic_program, 0 )
	MDRV_CPU_IO_MAP( kb_keytronic_io, 0 )
MACHINE_DRIVER_END

