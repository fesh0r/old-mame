/*

	TODO:

	- unreliable DOS commands?
	- tape input/output
	- PL-80 plotter
	- serial printer
	- thermal printer

*/

#include "driver.h"
#include "includes/comx35.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/cdp1869.h"
#include "sound/wave.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "devices/snapquik.h"
#include "machine/cdp1871.h"
#include "machine/wd17xx.h"
#include "video/mc6845.h"

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, CASSETTE_TAG);
}

/* Memory Maps */

static ADDRESS_MAP_START( comx35_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x1000, 0x17ff) AM_ROMBANK(2)
	AM_RANGE(0x1800, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK(1)
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK(3)
	AM_RANGE(0xf400, 0xf7ff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_charram_r, cdp1869_charram_w)
	AM_RANGE(0xf800, 0xffff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_pageram_r, cdp1869_pageram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( comx35_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_WRITE(comx35_bank_select_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(comx35_io_r, comx35_io_w)
	AM_RANGE(0x03, 0x03) AM_DEVREAD(CDP1871_TAG, cdp1871_data_r) AM_DEVWRITE(CDP1869_TAG, cdp1869_out3_w)
	AM_RANGE(0x04, 0x04) AM_READ(comx35_io2_r) AM_DEVWRITE(CDP1869_TAG, cdp1869_out4_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CDP1869_TAG, cdp1869_out5_w)
	AM_RANGE(0x06, 0x06) AM_DEVWRITE(CDP1869_TAG, cdp1869_out6_w)
	AM_RANGE(0x07, 0x07) AM_DEVWRITE(CDP1869_TAG, cdp1869_out7_w)
ADDRESS_MAP_END

/* Input Ports */

#define COMX35_DEVICES \
	PORT_CONFSETTING( 0x00, DEF_STR( None ) ) \
	PORT_CONFSETTING( 0x01, "DOS Card" ) \
	PORT_CONFSETTING( 0x02, "Parallel Output Interface" ) \
	PORT_CONFSETTING( 0x03, "Parallel Output Interface (F&M)" ) \
	PORT_CONFSETTING( 0x04, "RS-232C Serial Output Interface" ) \
	PORT_CONFSETTING( 0x05, "Thermal Printer Interface" ) \
	PORT_CONFSETTING( 0x06, "F&M Joycard" ) \
	PORT_CONFSETTING( 0x07, "80-Column Card" ) \
	PORT_CONFSETTING( 0x08, "RAM Card" )

static INPUT_PORTS_START( comx35 )
	PORT_START("D1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0 \xE2\x96\xA0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('�')

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
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RT") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10)) PORT_CHANGED(comx35_reset, NULL)

	PORT_START("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("JOY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("EXPANSION")
	PORT_CONFNAME( 0x0f, 0x00, "Expansion Slot")
	COMX35_DEVICES

	PORT_START("SLOT1")
	PORT_CONFNAME( 0x0f, 0x01, "Expansion Box Slot 1")
	COMX35_DEVICES

	PORT_START("SLOT2")
	PORT_CONFNAME( 0x0f, 0x02, "Expansion Box Slot 2")
	COMX35_DEVICES

	PORT_START("SLOT3")
	PORT_CONFNAME( 0x0f, 0x07, "Expansion Box Slot 3")
	COMX35_DEVICES

	PORT_START("SLOT4")
	PORT_CONFNAME( 0x0f, 0x08, "Expansion Box Slot 4")
	COMX35_DEVICES

	PORT_START("PRINTER")
	PORT_CONFNAME( 0x04, 0x00, "Printer")
	PORT_CONFSETTING( 0x00, "Parallel" )
	PORT_CONFSETTING( 0x01, "Serial" )
	PORT_CONFSETTING( 0x02, "Thermal" )
	PORT_CONFSETTING( 0x03, "COMX PL-80 Plotter" )
INPUT_PORTS_END

/* CDP1802 Interface */

static CDP1802_MODE_READ( comx35_mode_r )
{
	comx35_state *state = device->machine->driver_data;

	return state->cdp1802_mode;
}

static CDP1802_EF_READ( comx35_ef_r )
{
	comx35_state *state = device->machine->driver_data;

	int flags = 0x0f;

	/*
        EF1     predis
        EF2     on power up: 1=PAL/0=NTSC, after toggle Q: _EFXB
        EF3     _EFXA
        EF4     cassette in (ear)
    */

	// CDP1869 predisplay
	if (!state->cdp1869_prd) flags -= EF1;

	if (state->iden)
	{
		// interrupts disabled: PAL/NTSC
		if (!state->pal_ntsc) flags -= EF2;
	}
	else
	{
		// interrupts enabled: keyboard repeat
		if (!state->cdp1871_efxb) flags -= EF2;
	}

	// keyboard data available
	if (!state->cdp1871_efxa) flags -= EF3;

	// cassette input, expansion device flag
	if ((cassette_input(cassette_device_image(device->machine)) < +0.0) || !state->cdp1802_ef4) flags -= EF4;

	return flags;
}

static CDP1802_SC_WRITE( comx35_sc_w )
{
	comx35_state *driver_state = device->machine->driver_data;

	switch (state)
	{
	case CDP1802_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case CDP1802_STATE_CODE_S1_EXECUTE:
		// every other S1 triggers a DMAOUT request
		if (driver_state->dma)
		{
			driver_state->dma = 0;

			if (!driver_state->iden)
			{
				cputag_set_input_line(device->machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT, HOLD_LINE);
			}
		}
		else
		{
			driver_state->dma = 1;
		}
		break;

	case CDP1802_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		cputag_set_input_line(device->machine, CDP1802_TAG, CDP1802_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;

	case CDP1802_STATE_CODE_S3_INTERRUPT:
		// interrupt acknowledge clears the INT request
		cputag_set_input_line(device->machine, CDP1802_TAG, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
		break;
	}
}

static CDP1802_Q_WRITE( comx35_q_w )
{
	comx35_state *state = device->machine->driver_data;

	state->cdp1802_q = level;

	if (state->iden && level)
	{
		// enable interrupts
		state->iden = 0;
	}

	// cassette output
	cassette_output(cassette_device_image(device->machine), level ? +1.0 : -1.0);
}

static CDP1802_INTERFACE( comx35_cdp1802_config )
{
	comx35_mode_r,
	comx35_ef_r,
	comx35_sc_w,
	comx35_q_w,
	NULL,
	NULL
};

/* CDP1871 Interface */

static WRITE_LINE_DEVICE_HANDLER( comx35_da_w )
{
	comx35_state *driver_state = device->machine->driver_data;

	driver_state->cdp1871_efxa = state;
}

static WRITE_LINE_DEVICE_HANDLER( comx35_rpt_w )
{
	comx35_state *driver_state = device->machine->driver_data;

	driver_state->cdp1871_efxb = state;
}

static READ_LINE_DEVICE_HANDLER( comx35_shift_r )
{
	return BIT(input_port_read(device->machine, "MODIFIERS"), 0);
}

static READ_LINE_DEVICE_HANDLER( comx35_control_r )
{
	return BIT(input_port_read(device->machine, "MODIFIERS"), 1);
}

static CDP1871_INTERFACE( comx35_cdp1871_intf )
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
	DEVCB_LINE(comx35_shift_r),
	DEVCB_LINE(comx35_control_r),
	DEVCB_NULL,
	DEVCB_LINE(comx35_da_w),
	DEVCB_LINE(comx35_rpt_w)
};

/* Machine Drivers */

static const cassette_config comx35_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( comx35_pal )
	MDRV_DRIVER_DATA(comx35_state)

	/* basic system hardware */
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_PAL)
	MDRV_CPU_PROGRAM_MAP(comx35_map)
	MDRV_CPU_IO_MAP(comx35_io_map)
	MDRV_CPU_CONFIG(comx35_cdp1802_config)

	MDRV_MACHINE_START(comx35p)
	MDRV_MACHINE_RESET(comx35)

	/* sound and video hardware */
	MDRV_IMPORT_FROM(comx35_pal_video)

	/* peripheral hardware */
	MDRV_CDP1871_ADD(CDP1871_TAG, comx35_cdp1871_intf, CDP1869_CPU_CLK_PAL / 8)
	MDRV_WD1770_ADD(WD1770_TAG, comx35_wd17xx_interface )	
	MDRV_QUICKLOAD_ADD("quickload", comx35, "comx", 0)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, comx35_cassette_config)
	MDRV_PRINTER_ADD("printer")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( comx35_ntsc )
	MDRV_DRIVER_DATA(comx35_state)

	/* basic system hardware */
	MDRV_CPU_ADD(CDP1802_TAG, CDP1802, CDP1869_CPU_CLK_NTSC)
	MDRV_CPU_PROGRAM_MAP(comx35_map)
	MDRV_CPU_IO_MAP(comx35_io_map)
	MDRV_CPU_CONFIG(comx35_cdp1802_config)

	MDRV_MACHINE_START(comx35n)
	MDRV_MACHINE_RESET(comx35)

	/* sound and video hardware */
	MDRV_IMPORT_FROM(comx35_ntsc_video)

	/* peripheral hardware */
	MDRV_CDP1871_ADD(CDP1871_TAG, comx35_cdp1871_intf, CDP1869_CPU_CLK_NTSC / 8)
	MDRV_WD1770_ADD(WD1770_TAG, comx35_wd17xx_interface )	
	MDRV_QUICKLOAD_ADD("quickload", comx35, "comx", 0)
	MDRV_CASSETTE_ADD(CASSETTE_TAG, comx35_cassette_config)
	MDRV_PRINTER_ADD("printer")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( comx35p )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_DEFAULT_BIOS( "basic100e" )

	ROM_SYSTEM_BIOS( 0, "basic100", "COMX BASIC V1.00" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic100e", "COMX BASIC V1.00 with Expansion Box" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(2) )
	ROMX_LOAD( "expansion.e5",			0xe000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "fmbasic31", "F&M BASIC V3.1 with Expansion Box" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(3) )
	ROMX_LOAD( "f&m.expansion.3.1.e5",	0xe000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "basic101", "COMX BASIC V1.01" )
	ROMX_LOAD( "comx_11.u21",			0x0000, 0x4000, CRC(609d89cd) SHA1(799646810510d8236fbfafaff7a73d5170990f16), ROM_BIOS(4) )

	ROM_REGION( 0x2000, "fdc", 0 )
	ROM_LOAD( "fdc.f4",					0x0000, 0x2000, CRC(cf4ecd2e) SHA1(290e19bdc89e3c8059e63d5ae3cca4daa194e1fe) )

	ROM_REGION( 0x2000, "printer", 0 )
	ROM_LOAD( "printer.bin",			0x0000, 0x0800, CRC(3bbc2b2e) SHA1(08bf7ea4174713ab24969c553affd5c1401876b8) )

	ROM_REGION( 0x2000, "printer_fm", 0 )
	ROM_LOAD( "f&m.printer.1.2.bin",	0x0000, 0x1000, CRC(2feb997d) SHA1(ee9cb91042696c88ff5f2f44d2f702dc93369ba0) )

	ROM_REGION( 0x2000, "rs232", 0 )
	ROM_LOAD( "rs232.bin",				0x0000, 0x0800, CRC(926ff2d1) SHA1(be02bd388bba0211ea72d4868264a63308e4318d) )

	ROM_REGION( 0x2000, "thermal", 0 )
	ROM_LOAD( "thermal.bin",			0x0000, 0x1000, CRC(41a72ba8) SHA1(3a8760c78bd8c7bec2dbf26657b930c9a6814803) )

	ROM_REGION( 0x2000, "80column", 0 )
	ROM_LOAD( "80column.u3",			0x0000, 0x0800, CRC(b417d30a) SHA1(d428b0467945ecb9aec884211d0f4b1d8d56d738) )

	ROM_REGION( 0x800, "chargen", 0 )
	ROM_LOAD( "chargen.bin",			0x0000, 0x0800, CRC(69dd7b07) SHA1(71d368adbb299103d165eab8359a97769e463e26) )
ROM_END

#define rom_comx35n rom_comx35p

/* System Configuration */

static void comx35_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(comx35_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( comx35 )
	CONFIG_RAM_DEFAULT	(32 * 1024)
	CONFIG_DEVICE(comx35_floppy_getinfo)
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT  COMPAT	MACHINE		INPUT     INIT	CONFIG    COMPANY						FULLNAME			FLAGS
COMP( 1983, comx35p,	0,		0,		comx35_pal,	comx35,   0, 	comx35,   "Comx World Operations Ltd",	"COMX 35 (PAL)",	GAME_IMPERFECT_SOUND )
COMP( 1983, comx35n,	comx35p,0,		comx35_ntsc,comx35,   0, 	comx35,   "Comx World Operations Ltd",	"COMX 35 (NTSC)",	GAME_IMPERFECT_SOUND )
