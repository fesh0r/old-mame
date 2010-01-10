/***************************************************************************

        Elwro 800 Junior

        Driver by Mariusz Wojcieszek

        ToDo:
        - 8251 DTR and DTS signals are connected (with some additional logic) to NMI of Z80, this
          is not emulated
        - 8251 is used for JUNET network (a network of Elwro 800 Junior computers, allows sharing
          floppy disc drives and printers) - network is not emulated

****************************************************************************/

#include "driver.h"
#include "includes/spectrum.h"

/* Components */
#include "cpu/z80/z80.h"
#include "machine/upd765.h"	/* for floppy disc controller */
#include "machine/i8255a.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/ctronics.h"
#include "machine/msm8251.h"

/* Devices */
#include "devices/flopdrv.h"
#include "devices/cassette.h"
#include "formats/tzx_cas.h"
#include "devices/messram.h"

/*************************************
 *
 *  State/Globals
 *
 *************************************/
static UINT8 elwro800_df_on_databus = 0xdf;

typedef struct _elwro800_state elwro800_state;
struct _elwro800_state
{
	/* RAM mapped at 0 */
	UINT8 ram_at_0000;

	/* NR signal */
	UINT8 NR;
};

/*************************************
 *
 * When RAM is mapped at 0x0000 - 0x1fff (in CP/J mode), reading a location 66 with /M1=0
 * (effectively reading NMI vector) is hardwired to return 0xDF (RST #18)
 * (note that in CP/J mode address 66 is used for FCB)
 *
 *************************************/
static DIRECT_UPDATE_HANDLER(elwro800_direct_handler)
{
	elwro800_state *state = space->machine->driver_data;
	if (state->ram_at_0000 && address == 0x66)
	{
		direct->raw = direct->decrypted = &elwro800_df_on_databus;
		direct->bytemask = 0;
		return ~0;
	}
	return address;
}

/*************************************
 *
 *  UPD765/Floppy drive
 *
 *************************************/

static const struct upd765_interface elwro800jr_upd765_interface =
{
	DEVCB_NULL,
	NULL,
	NULL,
	UPD765_RDY_PIN_CONNECTED,
	{FLOPPY_0,FLOPPY_1, NULL, NULL}
};

static WRITE8_HANDLER(elwro800jr_fdc_control_w)
{
	const device_config *fdc = devtag_get_device(space->machine, "upd765");

	floppy_mon_w(floppy_get_device(space->machine, 0), !BIT(data, 0));
	floppy_mon_w(floppy_get_device(space->machine, 1), !BIT(data, 1));
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 0), 1,1);
	floppy_drive_set_ready_state(floppy_get_device(space->machine, 1), 1,1);

	upd765_tc_w(fdc, data & 0x04);

	upd765_reset_w(fdc, !(data & 0x08));
}

/*************************************
 *
 *  I/O port F7: memory mapping
 *
 *************************************/

static void elwro800jr_mmu_w(running_machine *machine, UINT8 data)
{
	UINT8 *prom = memory_region(machine, "proms") + 0x200;
	UINT8 cs;
	UINT8 ls175;
	elwro800_state *state = machine->driver_data;

	ls175 = BITSWAP8(data, 7, 6, 5, 4, 4, 5, 7, 6) & 0x0f;

	cs = prom[((0x0000 >> 10) | (ls175 << 6)) & 0x1ff];
	if (!BIT(cs,0))
	{
		// rom BAS0
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x0000); /* BAS0 ROM */
		memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0);
		state->ram_at_0000 = 0;
	}
	else if (!BIT(cs,4))
	{
		// rom BOOT
		memory_set_bankptr(machine, "bank1", memory_region(machine, "maincpu") + 0x4000); /* BOOT ROM */
		memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0);
		state->ram_at_0000 = 0;
	}
	else
	{
		// RAM
		memory_set_bankptr(machine, "bank1", messram_get_ptr(devtag_get_device(machine, "messram")));
		memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x0000, 0x1fff, 0, 0, "bank1");
		state->ram_at_0000 = 1;
	}

	cs = prom[((0x2000 >> 10) | (ls175 << 6)) & 0x1ff];
	if (!BIT(cs,1))
	{
		memory_set_bankptr(machine, "bank2", memory_region(machine, "maincpu") + 0x2000);	/* BAS1 ROM */
		memory_nop_write(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x2000, 0x3fff, 0, 0);
	}
	else
	{
		memory_set_bankptr(machine, "bank2", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x2000); /* RAM */
		memory_install_write_bank(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x2000, 0x3fff, 0, 0, "bank2");
	}

	if (BIT(ls175,2))
	{
		// relok
		spectrum_screen_location = messram_get_ptr(devtag_get_device(machine, "messram")) + 0xe000;
	}
	else
	{
		spectrum_screen_location = messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000;
	}

	state->NR = BIT(ls175,3);
	if (BIT(ls175,3))
	{
		logerror("Reading network number\n");
	}
}

/*************************************
 *
 *  8255: joystick and Centronics printer connections
 *
 *************************************/

static READ8_DEVICE_HANDLER(i8255_port_c_r)
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	return (centronics_ack_r(printer) << 2);
}

static WRITE8_DEVICE_HANDLER(i8255_port_c_w)
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	centronics_strobe_w(printer, (data >> 7) & 0x01);
}

static I8255A_INTERFACE(elwro800jr_ppi8255_interface)
{
	DEVCB_INPUT_PORT("JOY"),
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_r),
	DEVCB_HANDLER(i8255_port_c_r),
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("centronics", centronics_data_w),
	DEVCB_HANDLER(i8255_port_c_w)
};

static const centronics_interface elwro800jr_centronics_interface =
{
	FALSE,
	DEVCB_DEVICE_LINE("ppi8255", i8255a_pc2_w),
	DEVCB_NULL,
	DEVCB_NULL
};

/*************************************
 *
 *  I/O reads and writes
 *
 *  I/O accesses are decoded by prom which uses 8 low address lines (A0-A8) as input
 *  and outputs chip select signals for system components. Standard adresses are:
 *
 *  0x1F: 8255 port A (joystick)
 *  0xBE: 8251 data (Junet network)
 *  0xBF: 8251 control/status (Junet network)
 *  0xDC: 8255 control
 *  0xDD: 8255 port C (centronics 7-strobe, 2-ack)
 *  0xDE: 8255 port B (centronics data)
 *  0xDF: 8255 port A (joystick)
 *  0xEE: FDC 765A status
 *  0xEF: FDC 765A command and data
 *  0xF1: FDC control (motor on/off)
 *  0xF7: memory banking
 *  0xFE (write): border color, speaker and tape (as in Spectrum)
 *  0x??FE, 0x??7F, 0x??7B (read): keyboard reading
 *************************************/

static READ8_HANDLER(elwro800jr_io_r)
{
	UINT8 *prom = memory_region(space->machine, "proms");
	UINT8 cs = prom[offset & 0x1ff];
	elwro800_state *state = space->machine->driver_data;

	if (!BIT(cs,0))
	{
		// CFE
		int mask = 0x8000;
		int data = 0xff;
		int i;
		char port_name[6] = "LINE0";

		if ( !state->NR )
		{
			for (i = 0; i < 9; mask >>= 1, i++)
			{
				if (!(offset & mask))
				{
					if (i == 8)
					{
						port_name[4] = '8';
					}
					else
					{
						port_name[4] = '0' + (7 - i);
					}
					data &= (input_port_read(space->machine, port_name));
				}
			}

			if ((offset & 0xff) == 0xfb)
			{
				data &= input_port_read(space->machine, "LINE9");
			}

			/* cassette input from wav */
			if (cassette_input(devtag_get_device(space->machine, "cassette")) > 0.0038 )
			{
				data &= ~0x40;
			}
		}
		else
		{
			data = input_port_read(space->machine, "NETWORK ID");
		}

		return data;
	}
	else if (!BIT(cs,1))
	{
		// CF7
	}
	else if (!BIT(cs,2))
	{
		// CS55
		const device_config *ppi = devtag_get_device(space->machine, "ppi8255");
		return i8255a_r(ppi, (offset & 0x03) ^ 0x03);
	}
	else if (!BIT(cs,3))
	{
		// CSFDC
		const device_config *fdc = devtag_get_device(space->machine, "upd765");
		if (offset & 1)
		{
			return upd765_data_r(fdc,0);
		}
		else
		{
			return upd765_status_r(fdc,0);
		}
	}
	else if (!BIT(cs,4))
	{
		// CS51
		const device_config *usart = devtag_get_device(space->machine, "msm8251");
		if (offset & 1)
		{
			return msm8251_status_r(usart, 0);
		}
		else
		{
			return msm8251_data_r(usart, 0);
		}
	}
	else if (!BIT(cs,5))
	{
		// CF1
	}
	else
	{
		logerror("Unmapped I/O read: %04x\n", offset);
	}
	return 0x00;
}

static WRITE8_HANDLER(elwro800jr_io_w)
{
	UINT8 *prom = memory_region(space->machine, "proms");
	UINT8 cs = prom[offset & 0x1ff];

	if (!BIT(cs,0))
	{
		// CFE
		spectrum_port_fe_w(space, 0, data);
	}
	else if (!BIT(cs,1))
	{
		// CF7
		elwro800jr_mmu_w(space->machine, data);
	}
	else if (!BIT(cs,2))
	{
		// CS55
		const device_config *ppi = devtag_get_device(space->machine, "ppi8255");
		i8255a_w(ppi, (offset & 0x03) ^ 0x03, data);
	}
	else if (!BIT(cs,3))
	{
		// CSFDC
		const device_config *fdc = devtag_get_device(space->machine, "upd765");
		if (offset & 1)
		{
			upd765_data_w(fdc, 0, data);
		}
	}
	else if (!BIT(cs,4))
	{
		// CS51
		const device_config *usart = devtag_get_device(space->machine, "msm8251");
		if (offset & 1)
		{
			msm8251_control_w(usart, 0, data);
		}
		else
		{
			msm8251_data_w(usart, 0, data);
		}
	}
	else if (!BIT(cs,5))
	{
		// CF1
		elwro800jr_fdc_control_w(space, 0, data);
	}
	else
	{
		logerror("Unmapped I/O write: %04x %02x\n", offset, data);
	}
}

/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START(elwro800_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank1")
	AM_RANGE(0x2000, 0x3fff) AM_RAMBANK("bank2")
	AM_RANGE(0x4000, 0xffff) AM_RAMBANK("bank3")
ADDRESS_MAP_END

static ADDRESS_MAP_START(elwro800_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(elwro800jr_io_r, elwro800jr_io_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( elwro800 )
	PORT_START("LINE0") /* 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)		PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)		PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)		PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)		PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":    *") PORT_CODE(KEYCODE_ASTERISK)		PORT_CHAR(':')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";    +") PORT_CODE(KEYCODE_COLON)		PORT_CHAR(';')

	PORT_START("LINE1") /* 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)		PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)		PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)		PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)		PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)		PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-    =") PORT_CODE(KEYCODE_MINUS)		PORT_CHAR('-')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[    {") PORT_CODE(KEYCODE_OPENBRACE)	PORT_CHAR('[')

	PORT_START("LINE2") /* 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)	PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)	PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)	PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)	PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)	PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".    >") PORT_CODE(KEYCODE_STOP)			PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",    <") PORT_CODE(KEYCODE_COMMA)		PORT_CHAR(',')

	PORT_START("LINE3") /* 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1	!") PORT_CODE(KEYCODE_1)	PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2	@") PORT_CODE(KEYCODE_2)	PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3	#") PORT_CODE(KEYCODE_3)	PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4	$") PORT_CODE(KEYCODE_4)	PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5	%") PORT_CODE(KEYCODE_5)	PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/	?") PORT_CODE(KEYCODE_SLASH)		PORT_CHAR('/')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@	\\") PORT_CODE(KEYCODE_BACKSLASH)	PORT_CHAR('@')

	PORT_START("LINE4") /* 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0	_") PORT_CODE(KEYCODE_0)	PORT_CHAR('0') PORT_CHAR('_')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9	)") PORT_CODE(KEYCODE_9)	PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8	(") PORT_CODE(KEYCODE_8)	PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7	'") PORT_CODE(KEYCODE_7)	PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6	&") PORT_CODE(KEYCODE_6)	PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL") PORT_CODE(KEYCODE_DEL)		PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]	}") PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_CHAR(']')

	PORT_START("LINE5") /* 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)	PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)	PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)	PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)	PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)	PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)				PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BS") PORT_CODE(KEYCODE_BACKSPACE)		PORT_CHAR(UCHAR_MAMEKEY(BACKSPACE))

	PORT_START("LINE6") /* 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)	PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)	PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)	PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)	PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)					PORT_CHAR('\t')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT LOCK") PORT_CODE(KEYCODE_CAPSLOCK)		PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))

	PORT_START("LINE7") /* 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)	PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)	PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)	PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)	PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^    -") PORT_CODE(KEYCODE_TILDE)	PORT_CHAR('^') PORT_CHAR('-')

	PORT_START("LINE8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC4\x98") PORT_CODE(KEYCODE_0_PAD) // LATIN CAPITAL LETTER E WITH OGONEK
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC3\x93") PORT_CODE(KEYCODE_1_PAD) // LATIN CAPITAL LETTER O WITH ACUTE
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC5\x9A") PORT_CODE(KEYCODE_2_PAD) // LATIN CAPITAL LETTER S WITH ACUTE
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC5\x81") PORT_CODE(KEYCODE_3_PAD) // LATIN CAPITAL LETTER L WITH STROKE
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC5\xB9") PORT_CODE(KEYCODE_4_PAD) // LATIN CAPITAL LETTER Z WITH ACUTE
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC5\x83") PORT_CODE(KEYCODE_5_PAD) // LATIN CAPITAL LETTER N WITH ACUTE
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC4\x84") PORT_CODE(KEYCODE_6_PAD) // LATIN CAPITAL LETTER A WITH OGONEK

	PORT_START("LINE9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DIR") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Down") PORT_CODE(KEYCODE_DOWN)	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Up") PORT_CODE(KEYCODE_UP)		PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Right") PORT_CODE(KEYCODE_RIGHT)	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor Left") PORT_CODE(KEYCODE_LEFT)	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC4\x86") PORT_CODE(KEYCODE_7_PAD) // LATIN CAPITAL LETTER C WITH ACUTE
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xC5\xBB") PORT_CODE(KEYCODE_8_PAD) // LATIN CAPITAL LETTER Z WITH DOT ABOVE

	PORT_START("JOY")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Right") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Left") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Down") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Up") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P1 Joystick Fire") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START("NETWORK ID")
	PORT_DIPNAME( 0x3f, 0x01, "Computer network ID" )
	PORT_DIPSETTING( 0x01, "1" )
	PORT_DIPSETTING( 0x10, "16" )
	PORT_DIPSETTING( 0x11, "17" )

INPUT_PORTS_END

/*************************************
 *
 *  Machine
 *
 *************************************/

static MACHINE_RESET(elwro800)
{
	memset(messram_get_ptr(devtag_get_device(machine, "messram")), 0, 64*1024);

	memory_set_bankptr(machine, "bank3", messram_get_ptr(devtag_get_device(machine, "messram")) + 0x4000);

	// this is a reset of ls175 in mmu
	elwro800jr_mmu_w(machine, 0);

	memory_set_direct_update_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), elwro800_direct_handler);
}

static const cassette_config elwro800jr_cassette_config =
{
	tzx_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};

static INTERRUPT_GEN( elwro800jr_interrupt )
{
	cpu_set_input_line(device, 0, HOLD_LINE);
}

static const floppy_config elwro800jr_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

/* F4 Character Displayer */
static const gfx_layout elwro800_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	128,					/* 128 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( elwro800 )
	GFXDECODE_ENTRY( "maincpu", 0x3c00, elwro800_charlayout, 0, 8 )
GFXDECODE_END


static MACHINE_DRIVER_START( elwro800 )

	MDRV_DRIVER_DATA(elwro800_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",Z80, 3500000)	/* 3.5 MHz */
	MDRV_CPU_PROGRAM_MAP(elwro800_mem)
	MDRV_CPU_IO_MAP(elwro800_io)
	MDRV_CPU_VBLANK_INT("screen", elwro800jr_interrupt)

	MDRV_MACHINE_RESET(elwro800)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SPEC_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT( spectrum )
	MDRV_GFXDECODE(elwro800)

	MDRV_VIDEO_START( spectrum )
	MDRV_VIDEO_UPDATE( spectrum )
	MDRV_VIDEO_EOF( spectrum )

	MDRV_UPD765A_ADD("upd765", elwro800jr_upd765_interface)
	MDRV_I8255A_ADD( "ppi8255", elwro800jr_ppi8255_interface)

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", elwro800jr_centronics_interface)

	MDRV_MSM8251_ADD("msm8251", default_msm8251_interface)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_CASSETTE_ADD( "cassette", elwro800jr_cassette_config )

	MDRV_FLOPPY_2_DRIVES_ADD(elwro800jr_floppy_config)

	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("64K")
MACHINE_DRIVER_END

/*************************************
 *
 *  ROM definition
 *
 *************************************/

ROM_START( elwro800 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bas04.epr", 0x0000, 0x2000, CRC(6ab16f36) SHA1(49a19b279f311279c7fed3d2b3f207732d674c26) )
	ROM_LOAD( "bas14.epr", 0x2000, 0x2000, CRC(a743eb80) SHA1(3a300550838535b4adfe6d05c05fe0b39c47df16) )
	ROM_LOAD( "bootv.epr", 0x4000, 0x2000, CRC(de5fa37d) SHA1(4f203efe53524d84f69459c54b1a0296faa83fd9) )

	ROM_REGION(0x0400, "proms", 0 )
	ROM_LOAD( "junior_io_prom.bin", 0x0000, 0x0200,  CRC(c6a777c4) SHA1(41debc1b4c3bd4eef7e0e572327c759e0399a49c))
	ROM_LOAD( "junior_mem_prom.bin", 0x0200, 0x0200, CRC(0f745f42) SHA1(360ec23887fb6d7e19ee85d2bb30d9fa57f4936e))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1986, elwro800,  0,       0, 	elwro800, 	elwro800, 	 0,  "Elwro",   "800 Junior",		0)
