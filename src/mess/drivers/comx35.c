/*

    TODO:

    - unreliable DOS commands?
    - tape input/output
    - PL-80 plotter
    - serial printer
    - thermal printer

*/

#include "includes/comx35.h"
#include "formats/imageutl.h"


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

enum
{
	COMX_TYPE_BINARY = 1,
	COMX_TYPE_BASIC,
	COMX_TYPE_BASIC_FM,
	COMX_TYPE_RESERVED,
	COMX_TYPE_DATA
};

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    image_fread_memory - read image to memory
-------------------------------------------------*/

void comx35_state::image_fread_memory(device_image_interface &image, UINT16 addr, UINT32 count)
{
	UINT8 *ram = m_ram->pointer() + (addr - 0x4000);

	image.fread(ram, count);
}

/*-------------------------------------------------
    QUICKLOAD_LOAD_MEMBER( comx35_state, comx35_comx )
-------------------------------------------------*/

QUICKLOAD_LOAD_MEMBER( comx35_state, comx35_comx )
{
	address_space &program = m_maincpu->space(AS_PROGRAM);

	UINT8 header[16] = {0};
	int size = image.length();

	if (size > m_ram->size())
	{
		return IMAGE_INIT_FAIL;
	}

	image.fread( header, 5);

	if (header[1] != 'C' || header[2] != 'O' || header[3] != 'M' || header[4] != 'X' )
	{
		return IMAGE_INIT_FAIL;
	}

	switch (header[0])
	{
	case COMX_TYPE_BINARY:
		/*

		    Type 1: pure machine code (i.e. no basic)

		    Byte 0 to 4: 1 - 'COMX'
		    Byte 5 and 6: Start address (1802 way; see above)
		    Byte 6 and 7: End address
		    Byte 9 and 10: Execution address

		    Byte 11 to Eof, should be stored in ram from start to end; execution address
		    'xxxx' for the CALL (@xxxx) basic statement to actually run the code.

		*/
		{
			UINT16 start_address, end_address, run_address;

			image.fread(header, 6);

			start_address = pick_integer_be(header, 0, 2);
			end_address = pick_integer_be(header, 2, 2);
			run_address = pick_integer_be(header, 4, 2);

			image_fread_memory(image, start_address, end_address - start_address);

			popmessage("Type CALL (@%04x) to start program", run_address);
		}
		break;

	case COMX_TYPE_BASIC:
		/*

		    Type 2: Regular basic code or machine code followed by basic

		    Byte 0 to 4: 2 - 'COMX'
		    Byte 5 and 6: DEFUS value, to be stored on 0x4281 and 0x4282
		    Byte 7 and 8: EOP value, to be stored on 0x4283 and 0x4284
		    Byte 9 and 10: End array, start string to be stored on 0x4292 and 0x4293
		    Byte 11 and 12: start array to be stored on 0x4294 and 0x4295
		    Byte 13 and 14: EOD and end string to be stored on 0x4299 and 0x429A

		    Byte 15 to Eof to be stored on 0x4400 and onwards

		    Byte 0x4281-0x429A (or at least the ones above) should be set otherwise
		    BASIC won't 'see' the code.

		*/

		image_fread_memory(image, 0x4281, 4);
		image_fread_memory(image, 0x4292, 4);
		image_fread_memory(image, 0x4299, 2);
		image_fread_memory(image, 0x4400, size);
		break;

	case COMX_TYPE_BASIC_FM:
		/*

		    Type 3: F&M basic load

		    Not the most important! But we designed our own basic extension, you can
		    find it in the F&M basic folder as F&M Basic.comx. When you run this all
		    basic code should start at address 0x6700 instead of 0x4400 as from
		    0x4400-0x6700 the F&M basic stuff is loaded. So format is identical to Type
		    2 except Byte 15 to Eof should be stored on 0x6700 instead. .comx files of
		    this format can also be found in the same folder as the F&M basic.comx file.

		*/

		image_fread_memory(image, 0x4281, 4);
		image_fread_memory(image, 0x4292, 4);
		image_fread_memory(image, 0x4299, 2);
		image_fread_memory(image, 0x6700, size);
		break;

	case COMX_TYPE_RESERVED:
		/*

		    Type 4: Incorrect DATA format, I suggest to forget this one as it won't work
		    in most cases. Instead I left this one reserved and designed Type 5 instead.

		*/
		break;

	case COMX_TYPE_DATA:
		/*

		    Type 5: Data load

		    Byte 0 to 4: 5 - 'COMX'
		    Byte 5 and 6: Array length
		    Byte 7 to Eof: Basic 'data'

		    To load this first get the 'start array' from the running COMX, i.e. address
		    0x4295/0x4296. Calculate the EOD as 'start array' + length of the data (i.e.
		    file length - 7). Store the EOD back on 0x4299 and ox429A. Calculate the
		    'Start String' as 'start array' + 'Array length' (Byte 5 and 6). Store the
		    'Start String' on 0x4292/0x4293. Load byte 7 and onwards starting from the
		    'start array' value fetched from 0x4295/0x4296.

		*/
		{
			UINT16 start_array, end_array, start_string, array_length;

			image.fread(header, 2);

			array_length = pick_integer_be(header, 0, 2);
			start_array = (program.read_byte(0x4295) << 8) | program.read_byte(0x4296);
			end_array = start_array + (size - 7);

			program.write_byte(0x4299, end_array >> 8);
			program.write_byte(0x429a, end_array & 0xff);

			start_string = start_array + array_length;

			program.write_byte(0x4292, start_string >> 8);
			program.write_byte(0x4293, start_string & 0xff);

			image_fread_memory(image, start_array, size);
		}
		break;
	}

	return IMAGE_INIT_PASS;
}

//**************************************************************************
//  MEMORY ACCESS
//**************************************************************************

//-------------------------------------------------
//  mem_r - memory read
//-------------------------------------------------

READ8_MEMBER( comx35_state::mem_r )
{
	int extrom = 1;

	UINT8 data = m_exp->mrd_r(space, offset, &extrom);

	if (offset < 0x4000)
	{
		if (extrom) data = m_rom->base()[offset & 0x3fff];
	}
	else if (offset >= 0x4000 && offset < 0xc000)
	{
		data = m_ram->pointer()[offset - 0x4000];
	}
	else if (offset >= 0xf400 && offset < 0xf800)
	{
		data = m_vis->char_ram_r(space, offset & 0x3ff);
	}

	return data;
}


//-------------------------------------------------
//  mem_w - memory write
//-------------------------------------------------

WRITE8_MEMBER( comx35_state::mem_w )
{
	m_exp->mwr_w(space, offset, data);

	if (offset >= 0x4000 && offset < 0xc000)
	{
		m_ram->pointer()[offset - 0x4000] = data;
	}
	else if (offset >= 0xf400 && offset < 0xf800)
	{
		m_vis->char_ram_w(space, offset & 0x3ff, data);
	}
	else if (offset >= 0xf800)
	{
		m_vis->page_ram_w(space, offset & 0x3ff, data);
	}
}


//-------------------------------------------------
//  io_r - I/O read
//-------------------------------------------------

READ8_MEMBER( comx35_state::io_r )
{
	UINT8 data = m_exp->io_r(space, offset);

	if (offset == 3)
	{
		data = m_kbe->data_r(space, 0);
	}

	return data;
}


//-------------------------------------------------
//  io_w - I/O write
//-------------------------------------------------

WRITE8_MEMBER( comx35_state::io_w )
{
	m_exp->io_w(space, offset, data);

	if (offset >= 3)
	{
		cdp1869_w(space, offset, data);
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( comx35_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( comx35_mem, AS_PROGRAM, 8, comx35_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(mem_r, mem_w)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( comx35_io )
//-------------------------------------------------

static ADDRESS_MAP_START( comx35_io, AS_IO, 8, comx35_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x07) AM_READWRITE(io_r, io_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_CHANGED_MEMBER( comx35_reset )
//-------------------------------------------------

INPUT_CHANGED_MEMBER( comx35_state::trigger_reset )
{
	if (newval && BIT(m_d6->read(), 7))
	{
		machine_reset();
	}
}


//-------------------------------------------------
//  INPUT_PORTS( comx35 )
//-------------------------------------------------

static INPUT_PORTS_START( comx35 )
	PORT_START("D1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0 \xE2\x96\xA0") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('?')

	PORT_START("D2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('[')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('<') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('=') PORT_CHAR('^')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('>') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('\\') PORT_CHAR('_')

	PORT_START("D3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("D4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("D5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("D6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('+') PORT_CHAR('{')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('-') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('*') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR(8)

	PORT_START("D7")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CR") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D9")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D10")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D11")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CNTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RT") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10)) PORT_CHANGED_MEMBER(DEVICE_SELF, comx35_state, trigger_reset, 0)
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  COSMAC_INTERFACE( cosmac_intf )
//-------------------------------------------------

void comx35_state::check_interrupt()
{
	m_maincpu->set_input_line(COSMAC_INPUT_LINE_INT, m_cr1 || m_int);
}

READ_LINE_MEMBER( comx35_state::clear_r )
{
	return m_clear;
}

READ_LINE_MEMBER( comx35_state::ef2_r )
{
	if (m_iden)
	{
		// interrupts disabled: PAL/NTSC
		return m_vis->pal_ntsc_r();
	}
	else
	{
		// interrupts enabled: keyboard repeat
		return m_kbe->rpt_r();
	}
}

READ_LINE_MEMBER( comx35_state::ef4_r )
{
	return m_exp->ef4_r(); // | (m_cassette->input() > 0.0f);
}

static COSMAC_SC_WRITE( comx35_sc_w )
{
	comx35_state *state = device->machine().driver_data<comx35_state>();

	switch (sc)
	{
	case COSMAC_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case COSMAC_STATE_CODE_S1_EXECUTE:
		// every other S1 triggers a DMAOUT request
		if (state->m_dma)
		{
			state->m_dma = 0;

			if (!state->m_iden)
			{
				state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_DMAOUT, ASSERT_LINE);
			}
		}
		else
		{
			state->m_dma = 1;
		}
		break;

	case COSMAC_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;

	case COSMAC_STATE_CODE_S3_INTERRUPT:
		// interrupt acknowledge clears the INT request
		state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_INT, CLEAR_LINE);
		break;
	}
}

WRITE_LINE_MEMBER( comx35_state::q_w )
{
	m_q = state;

	if (m_iden && state)
	{
		// enable interrupts
		m_iden = 0;
	}

	// cassette output
	m_cassette->output(state ? +1.0 : -1.0);

	// expansion bus
	m_exp->q_w(state);
}

static COSMAC_INTERFACE( cosmac_intf )
{
	DEVCB_LINE_VCC,                                 // wait
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, clear_r),// clear
	DEVCB_NULL,                                     // EF1
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, ef2_r),  // EF2
	DEVCB_NULL,                                     // EF3
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, ef4_r),  // EF4
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, q_w),    // Q
	DEVCB_NULL,                                     // DMA in
	DEVCB_NULL,                                     // DMA out
	comx35_sc_w,                                    // SC
	DEVCB_NULL,                                     // TPA
	DEVCB_NULL                                      // TPB
};


//-------------------------------------------------
//  CDP1871_INTERFACE( kbc_intf )
//-------------------------------------------------

READ_LINE_MEMBER( comx35_state::shift_r )
{
	return BIT(m_modifiers->read(), 0);
}

READ_LINE_MEMBER( comx35_state::control_r )
{
	return BIT(m_modifiers->read(), 1);
}

static CDP1871_INTERFACE( kbc_intf )
{
	DEVCB_INPUT_PORT("D1"),
	DEVCB_INPUT_PORT("D2"),
	DEVCB_INPUT_PORT("D3"),
	DEVCB_INPUT_PORT("D4"),
	DEVCB_INPUT_PORT("D5"),
	DEVCB_INPUT_PORT("D6"),
	DEVCB_INPUT_PORT("D7"),
	DEVCB_INPUT_PORT("D8"),
	DEVCB_INPUT_PORT("D9"),
	DEVCB_INPUT_PORT("D10"),
	DEVCB_INPUT_PORT("D11"),
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, shift_r),
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, control_r),
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_EF3),
	DEVCB_NULL // polled
};


//-------------------------------------------------
//  cassette_interface cassette_intf
//-------------------------------------------------

static const cassette_interface cassette_intf =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED),
	NULL,
	NULL
};


//-------------------------------------------------
//  COMX_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( comx35_state::int_w )
{
	m_int = state;
	check_interrupt();
}

static COMX_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, int_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static SLOT_INTERFACE_START( comx_expansion_cards )
	SLOT_INTERFACE("eb", COMX_EB)
	SLOT_INTERFACE("fd", COMX_FD)
	SLOT_INTERFACE("clm", COMX_CLM)
	SLOT_INTERFACE("ram", COMX_RAM)
	SLOT_INTERFACE("joy", COMX_JOY)
	SLOT_INTERFACE("prn", COMX_PRN)
	SLOT_INTERFACE("thm", COMX_THM)
	SLOT_INTERFACE("epr", COMX_EPR)
SLOT_INTERFACE_END



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void comx35_state::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_ID_RESET:
		m_clear = 1;
		break;
	}
}


//-------------------------------------------------
//  MACHINE_START( comx35 )
//-------------------------------------------------

void comx35_state::machine_start()
{
	// clear the RAM since DOS card will go crazy if RAM is not all zeroes
	UINT8 *ram = m_ram->pointer();
	memset(ram, 0, m_ram->size());

	// register for state saving
	save_item(NAME(m_clear));
	save_item(NAME(m_q));
	save_item(NAME(m_iden));
	save_item(NAME(m_dma));
	save_item(NAME(m_int));
	save_item(NAME(m_prd));
	save_item(NAME(m_cr1));
}


//-------------------------------------------------
//  MACHINE_RESET( comx35 )
//-------------------------------------------------

void comx35_state::machine_reset()
{
	m_exp->reset();

	int t = RES_K(27) * CAP_U(1) * 1000; // t = R1 * C1

	m_clear = 0;
	m_iden = 1;
	m_cr1 = 1;
	m_int = CLEAR_LINE;
	m_prd = CLEAR_LINE;

	timer_set(attotime::from_msec(t), TIMER_ID_RESET);
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( pal )
//-------------------------------------------------

static MACHINE_CONFIG_START( pal, comx35_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_PAL)
	MCFG_CPU_PROGRAM_MAP(comx35_mem)
	MCFG_CPU_IO_MAP(comx35_io)
	MCFG_CPU_CONFIG(cosmac_intf)

	// sound and video hardware
	MCFG_FRAGMENT_ADD(comx35_pal_video)

	// peripheral hardware
	MCFG_CDP1871_ADD(CDP1871_TAG, kbc_intf, CDP1869_CPU_CLK_PAL / 8)
	MCFG_QUICKLOAD_ADD("quickload", comx35_state, comx35_comx, "comx", 0)
	MCFG_CASSETTE_ADD("cassette", cassette_intf)

	// expansion bus
	MCFG_COMX_EXPANSION_SLOT_ADD(EXPANSION_TAG, expansion_intf, comx_expansion_cards, "eb")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")

	// software lists
	MCFG_SOFTWARE_LIST_ADD("flop_list", "comx35_flop")
MACHINE_CONFIG_END


//-------------------------------------------------
//  MACHINE_CONFIG( ntsc )
//-------------------------------------------------

static MACHINE_CONFIG_START( ntsc, comx35_state )
	// basic system hardware
	MCFG_CPU_ADD(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_NTSC)
	MCFG_CPU_PROGRAM_MAP(comx35_mem)
	MCFG_CPU_IO_MAP(comx35_io)
	MCFG_CPU_CONFIG(cosmac_intf)

	// sound and video hardware
	MCFG_FRAGMENT_ADD(comx35_ntsc_video)

	// peripheral hardware
	MCFG_CDP1871_ADD(CDP1871_TAG, kbc_intf, CDP1869_CPU_CLK_NTSC / 8)
	MCFG_QUICKLOAD_ADD("quickload", comx35_state, comx35_comx, "comx", 0)
	MCFG_CASSETTE_ADD("cassette", cassette_intf)

	// expansion bus
	MCFG_COMX_EXPANSION_SLOT_ADD(EXPANSION_TAG, expansion_intf, comx_expansion_cards, "eb")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")

	// software lists
	MCFG_SOFTWARE_LIST_ADD("flop_list", "comx35_flop")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( comx35p )
//-------------------------------------------------

ROM_START( comx35p )
	ROM_REGION( 0x4000, CDP1802_TAG, 0 )
	ROM_DEFAULT_BIOS( "basic100" )
	ROM_SYSTEM_BIOS( 0, "basic100", "COMX BASIC V1.00" )
	ROMX_LOAD( "comx_10.u21", 0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic101", "COMX BASIC V1.01" )
	ROMX_LOAD( "comx_11.u21", 0x0000, 0x4000, CRC(609d89cd) SHA1(799646810510d8236fbfafaff7a73d5170990f16), ROM_BIOS(2) )
ROM_END


//-------------------------------------------------
//  ROM( comx35n )
//-------------------------------------------------

#define rom_comx35n rom_comx35p



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT  COMPANY                         FULLNAME            FLAGS
COMP( 1983, comx35p,    0,      0,      pal,        comx35, driver_device,   0, "Comx World Operations Ltd",    "COMX 35 (PAL)",    GAME_IMPERFECT_SOUND )
COMP( 1983, comx35n,    comx35p,0,      ntsc,       comx35, driver_device,   0, "Comx World Operations Ltd",    "COMX 35 (NTSC)",   GAME_IMPERFECT_SOUND )
