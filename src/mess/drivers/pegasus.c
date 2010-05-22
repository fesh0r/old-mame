/***************************************************************************

    Aamber Pegasus computer (New Zealand)

    http://web.mac.com/lord_philip/aamber_pegasus/Aamber_Pegasus.html

    Each copy of the monitor rom was made for an individual machine.
    The early bios versions checked that it was running on that
    particular computer.

    This computer has no sound.

    The usual way of loading a new rom was to plug it into the board.
    We have replaced this with cartslots, to save having to recompile
    whenever a new rom is found. Single rom programs will usually work in
    any slot (if it is going to work at all). A working rom will appear
    in the menu. Press the first letter to run it.

    If a machine language program is loaded via cassette, do it in the
    Monitor (L command), when loaded press Enter, and it will be in the
    menu.

    Basic cassettes are loaded in the usual way, that is, start Basic,
    type LOAD, press Enter. When done, RUN or LIST as needed.

    TO DO:
    - Identify which rom slots the multi-rom programs should be fitted to.
    - Work on the other non-working programs

****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "includes/pegasus.h"

static UINT8 pegasus_kbd_row = 0;
static UINT8 pegasus_kbd_irq = 1;
UINT8 pegasus_control_bits = 0;
static running_device *pegasus_cass;
static UINT8 *FNT;

static TIMER_DEVICE_CALLBACK( pegasus_firq )
{
	running_device *cpu = devtag_get_device( timer->machine, "maincpu" );
	cpu_set_input_line(cpu, M6809_FIRQ_LINE, HOLD_LINE);
}

static WRITE_LINE_DEVICE_HANDLER( pegasus_firq_clr )
{
	cputag_set_input_line(device->machine, "maincpu", M6809_FIRQ_LINE, CLEAR_LINE);
}

static READ8_DEVICE_HANDLER( pegasus_keyboard_r )
{
	static const char *const keynames[] = { "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7" };
	UINT8 bit,data = 0xff;
	for (bit = 0; bit < 8; bit++)
		if (!BIT(pegasus_kbd_row, bit)) data &= input_port_read(device->machine, keynames[bit]);

	pegasus_kbd_irq = (data == 0xff) ? 1 : 0;
	if (pegasus_control_bits & 8) data<<=4;
	return data;
}

static WRITE8_DEVICE_HANDLER( pegasus_keyboard_w )
{
	pegasus_kbd_row = data;
}

static WRITE8_DEVICE_HANDLER( pegasus_controls )
{
/*  d0,d2 - not emulated
    d0 - Blank - Video blanking
    d1 - Char - select character rom or ram
    d2 - Page - enables writing to video ram
    d3 - Asc - Select which half of the keyboard to read
*/

	pegasus_control_bits = data;
}

static READ_LINE_DEVICE_HANDLER( pegasus_keyboard_irq )
{
	return pegasus_kbd_irq;
}

static READ_LINE_DEVICE_HANDLER( pegasus_cassette_r )
{
	return cassette_input(pegasus_cass);
}

static WRITE_LINE_DEVICE_HANDLER( pegasus_cassette_w )
{
	cassette_output(pegasus_cass, state ? 1 : -1);
}

static READ8_HANDLER( pegasus_pcg_r )
{
	UINT8 code = pegasus_video_ram[offset] & 0x7f;
	return FNT[(code << 4) | (~pegasus_kbd_row & 15)];
}

static WRITE8_HANDLER( pegasus_pcg_w )
{
//  if (pegasus_control_bits & 2)
	{
		UINT8 code = pegasus_video_ram[offset] & 0x7f;
		FNT[(code << 4) | (~pegasus_kbd_row & 15)] = data;
	}
}

/* Must return the A register except when it is doing a rom search */
static READ8_HANDLER( pegasus_protection_r )
{
	UINT8 data = cpu_get_reg(devtag_get_device(space->machine, "maincpu"), M6809_A);
	if (data == 0x20) data = 0xff;
	return data;
}

static ADDRESS_MAP_START(pegasus_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0xb000, 0xbdff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM AM_BASE(&pegasus_video_ram)
	AM_RANGE(0xc000, 0xdfff) AM_ROM AM_WRITENOP
	AM_RANGE(0xe000, 0xe1ff) AM_READ(pegasus_protection_r)
	AM_RANGE(0xe200, 0xe3ff) AM_READWRITE(pegasus_pcg_r,pegasus_pcg_w)
	AM_RANGE(0xe400, 0xe403) AM_MIRROR(0x1fc) AM_DEVREADWRITE("pegasus_pia_u", pia6821_r, pia6821_w)
	AM_RANGE(0xe600, 0xe603) AM_MIRROR(0x1fc) AM_DEVREADWRITE("pegasus_pia_s", pia6821_r, pia6821_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START(pegasusm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x5000, 0xbdff) AM_RAM
	AM_RANGE(0xbe00, 0xbfff) AM_RAM AM_BASE(&pegasus_video_ram)
	AM_RANGE(0xc000, 0xdfff) AM_ROM AM_WRITENOP
	AM_RANGE(0xe000, 0xe1ff) AM_READ(pegasus_protection_r)
	AM_RANGE(0xe200, 0xe3ff) AM_READWRITE(pegasus_pcg_r,pegasus_pcg_w)
	AM_RANGE(0xe400, 0xe403) AM_MIRROR(0x1fc) AM_DEVREADWRITE("pegasus_pia_u", pia6821_r, pia6821_w)
	AM_RANGE(0xe600, 0xe603) AM_MIRROR(0x1fc) AM_DEVREADWRITE("pegasus_pia_s", pia6821_r, pia6821_w)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pegasus )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 )") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BackSpace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR(8)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Break") PORT_CODE(KEYCODE_NUMLOCK) PORT_CHAR(20)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l') PORT_CHAR(13)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p') PORT_CHAR(16)

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("[ ]") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR(']')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(127) PORT_CHAR('_')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\' \"") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i') PORT_CHAR(9)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k') PORT_CHAR(11)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o') PORT_CHAR(15)

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 *") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("` ~") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j') PORT_CHAR(10)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u') PORT_CHAR(21)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g') PORT_CHAR(7)

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t') PORT_CHAR(20)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 &") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )	// outputs a space
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ShiftR") PORT_NAME("ShiftR") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h') PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y') PORT_CHAR(25)

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r') PORT_CHAR(18)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w') PORT_CHAR(23)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )	// outputs a space
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c') PORT_CHAR(3)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f') PORT_CHAR(6)

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e') PORT_CHAR(5)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q') PORT_CHAR(17)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )	// REPEAT key which is disconnected - outputs a space
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LineFeed") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(10)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m') PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d') PORT_CHAR(4)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n') PORT_CHAR(14)

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v') PORT_CHAR(22)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a') PORT_CHAR(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x') PORT_CHAR(24)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BlankL") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(16)

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b') PORT_CHAR(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s') PORT_CHAR(19)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CapsLock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(19)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ShiftL") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BlankR") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(21)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z') PORT_CHAR(26)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("{ }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('{') PORT_CHAR('}')
INPUT_PORTS_END

/* System - for keyboard, video, general housekeeping */
static const pia6821_interface pegasus_pia_s_intf=
{
	DEVCB_NULL,						/* port A input */
	DEVCB_HANDLER(pegasus_keyboard_r),			/* port B input */
	DEVCB_LINE(pegasus_cassette_r),				/* CA1 input */
	DEVCB_LINE(pegasus_keyboard_irq),			/* CB1 input */
	DEVCB_NULL,						/* CA2 input */
	DEVCB_NULL,						/* CB2 input */
	DEVCB_HANDLER(pegasus_keyboard_w),			/* port A output */
	DEVCB_HANDLER(pegasus_controls),			/* port B output */
	DEVCB_LINE(pegasus_cassette_w),				/* CA2 output */
	DEVCB_DEVICE_LINE("maincpu", pegasus_firq_clr),		/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE),	/* IRQA output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE)		/* IRQB output */
};

/* User interface - for connection of external equipment */
static const pia6821_interface pegasus_pia_u_intf=
{
	DEVCB_NULL,		/* port A input */
	DEVCB_NULL,		/* port B input */
	DEVCB_NULL,		/* CA1 input */
	DEVCB_NULL,		/* CB1 input */
	DEVCB_NULL,		/* CA2 input */
	DEVCB_NULL,		/* CB2 input */
	DEVCB_NULL,		/* port A output */
	DEVCB_NULL,		/* port B output */
	DEVCB_NULL,		/* CA2 output */
	DEVCB_NULL,		/* CB2 output */
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE),
	DEVCB_CPU_INPUT_LINE("maincpu", M6809_IRQ_LINE)
};

static const cassette_config pegasus_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED|CASSETTE_MOTOR_ENABLED)
};

/* An encrypted single rom starts with 02, decrypted with 20. Not sure what
    multipart roms will have. */
static void pegasus_decrypt_rom( running_machine *machine, UINT16 addr )
{
	UINT8 b, *ROM = memory_region(machine, "maincpu");
	UINT16 i, j;
	UINT8 buff[0x1000];
	if (ROM[addr] == 0x02)
	{
		for (i = 0; i < 0x1000; i++)
		{
			b = ROM[addr|i];
			j = BITSWAP16( i, 15, 14, 13, 12, 11, 10, 9, 8, 0, 1, 2, 3, 4, 5, 6, 7 );
			b = BITSWAP8( b, 3, 2, 1, 0, 7, 6, 5, 4 );
			buff[j&0xfff] = b;
		}
		for (i = 0; i < 0x1000; i++)
			ROM[addr|i] = buff[i];
	}
}

static DEVICE_IMAGE_LOAD( pegasus_cart_1 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0x0000, 0x1000);
	pegasus_decrypt_rom( image->machine, 0x0000 );

	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( pegasus_cart_2 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0x1000, 0x1000);
	pegasus_decrypt_rom( image->machine, 0x1000 );

	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( pegasus_cart_3 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0x2000, 0x1000);
	pegasus_decrypt_rom( image->machine, 0x2000 );

	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( pegasus_cart_4 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0xc000, 0x1000);
	pegasus_decrypt_rom( image->machine, 0xc000 );

	return INIT_PASS;
}

static DEVICE_IMAGE_LOAD( pegasus_cart_5 )
{
	image_fread(image, memory_region(image->machine, "maincpu") + 0xd000, 0x1000);
	pegasus_decrypt_rom( image->machine, 0xd000 );

	return INIT_PASS;
}

static MACHINE_START( pegasus )
{
	pegasus_cass = devtag_get_device(machine, "cassette");
	FNT = memory_region(machine, "pcg");
}

static MACHINE_RESET( pegasus )
{
	pegasus_kbd_row = 0;
	pegasus_kbd_irq = 1;
	pegasus_control_bits = 0;
}

static DRIVER_INIT( pegasus )
{
	pegasus_decrypt_rom( machine, 0xf000 );
}

static MACHINE_DRIVER_START( pegasus )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6809E, XTAL_4MHz)	// actually a 6809C
	MDRV_CPU_PROGRAM_MAP(pegasus_mem)

	MDRV_TIMER_ADD_PERIODIC("pegasus_firq", pegasus_firq, HZ(400))	// controls accuracy of the clock (ctrl-P)
	MDRV_MACHINE_START(pegasus)
	MDRV_MACHINE_RESET(pegasus)
	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0, 32*8-1, 0, 16*16-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(pegasus)

	/* devices */
	MDRV_PIA6821_ADD( "pegasus_pia_s", pegasus_pia_s_intf )
	MDRV_PIA6821_ADD( "pegasus_pia_u", pegasus_pia_u_intf )
	MDRV_CARTSLOT_ADD("cart1")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_1)
	MDRV_CARTSLOT_ADD("cart2")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_2)
	MDRV_CARTSLOT_ADD("cart3")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_3)
	MDRV_CARTSLOT_ADD("cart4")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_4)
	MDRV_CARTSLOT_ADD("cart5")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_LOAD(pegasus_cart_5)
	MDRV_CASSETTE_ADD( "cassette", pegasus_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pegasusm )
	MDRV_IMPORT_FROM(pegasus)
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(pegasusm_mem)
MACHINE_DRIVER_END


/* ROM definition */
ROM_START( pegasus )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "11r2674", "Monitor 1.1 r2674")
	ROMX_LOAD( "mon11_2674.bin", 0xf000, 0x1000, CRC(1640ff7e) SHA1(8199643749fb40fb8be05e9f311c75620ca939b1), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS(1, "10r2569", "Monitor 1.0 r2569")
	ROMX_LOAD( "mon10_2569.bin", 0xf000, 0x1000, CRC(910fc930) SHA1(a4f6bbe5def0268cc49ee7045616a39017dd8052), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS(2, "11r2569", "Monitor 1.1 r2569")
	ROMX_LOAD( "mon11_2569.bin", 0xf000, 0x1000, CRC(07b92002) SHA1(3c434601120870c888944ecd9ade5186432ddbc2), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS(3, "11r2669", "Monitor 1.1 r2669")
	ROMX_LOAD( "mon11_2669.bin", 0xf000, 0x1000, CRC(f3ee23c8) SHA1(3ac96935668f5e53799c90db5140393c2ef9ce36), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS(4, "22r2856", "Monitor 2.2 r2856")
	ROMX_LOAD( "mon22_2856.bin", 0xf000, 0x1000, CRC(5f5f688a) SHA1(3719eecc347e158dd027ea7aa8a068cdafc00d9b), ROM_BIOS(5) )
	ROM_SYSTEM_BIOS(5, "22br2856", "Monitor 2.2B r2856")
	ROMX_LOAD( "mon22b_2856.bin", 0xf000, 0x1000, CRC(a47b0308) SHA1(f215e51aa8df6aed99c10f3df6a3589cb9f63d46), ROM_BIOS(6) )
	ROM_SYSTEM_BIOS(6, "23r2601", "Monitor 2.3 r2601")
	ROMX_LOAD( "mon23_2601.bin", 0xf000, 0x1000, CRC(0e024222) SHA1(9950cba08996931b9d5a3368b44c7309638b4e08), ROM_BIOS(7) )
	ROM_SYSTEM_BIOS(7, "23ar2569", "Monitor 2.3A r2569")
	ROMX_LOAD( "mon23a_2569.bin", 0xf000, 0x1000, CRC(248e62c9) SHA1(adbde27e69b38b29ff89bacf28d0240a8e5d90f3), ROM_BIOS(8) )

	ROM_REGION( 0x800, "pcg", ROMREGION_ERASEFF )
ROM_END

#define rom_pegasusm rom_pegasus

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, pegasus,  0,     0,      pegasus,	pegasus, pegasus, "Technosys",   "Aamber Pegasus", GAME_NO_SOUND_HW )
COMP( 1981, pegasusm, pegasus, 0,    pegasusm,	pegasus, pegasus, "Technosys",   "Aamber Pegasus with RAM expansion unit", GAME_NO_SOUND_HW )

