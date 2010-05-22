/*

    Tandy Radio Shack TRS-80 Model II/12/16/16B/6000

    http://home.iae.nl/users/pb0aia/cm/modelii.html

*/

/*

    TODO:

	- wd17xx.c thinks the drive is always ready
    - keyboard CPU ROM
    - keyboard layout
    - graphics board

*/

#include "emu.h"
#include "includes/trs80m2.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/mcs48/mcs48.h"
#include "cpu/m68000/m68000.h"
#include "devices/flopdrv.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "machine/wd17xx.h"
#include "machine/z80ctc.h"
#include "machine/z80dma.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "video/mc6845.h"

/* Read/Write Handlers */

static WRITE8_DEVICE_HANDLER( drvslt_w )
{
	/*

        bit     signal

        0       DS1
        1       DS2
        2       DS3
        3       DS4
        4
        5
        6       SDSEL
        7       FM/MFM

    */

	/* drive select */
	if (!BIT(data, 0)) wd17xx_set_drive(device, 0);
	if (!BIT(data, 1)) wd17xx_set_drive(device, 1);
	if (!BIT(data, 2)) wd17xx_set_drive(device, 2);
	if (!BIT(data, 3)) wd17xx_set_drive(device, 3);

	/* side select */
	wd17xx_set_side(device, !BIT(data, 6));

	/* FM/MFM */
	wd17xx_dden_w(device, BIT(data, 7));
}

static void bankswitch(running_machine *machine)
{
	trs80m2_state *state = (trs80m2_state *)machine->driver_data;

	const address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);
	running_device *messram = devtag_get_device(machine, "messram");
	UINT8 *rom = memory_region(machine, Z80_TAG);
	UINT8 *ram = messram_get_ptr(messram);
	int last_page = (messram_get_size(messram) / 0x8000) - 1;

	if (state->boot_rom)
	{
		/* enable BOOT ROM */
		memory_install_rom(program, 0x0000, 0x07ff, 0, 0, rom);
		memory_install_ram(program, 0x0800, 0x7fff, 0, 0, ram + 0x800);
	}
	else
	{
		/* enable RAM */
		memory_install_ram(program, 0x0000, 0x7fff, 0, 0, ram);
	}

	if (state->bank > last_page)
	{
		memory_unmap_readwrite(program, 0x8000, 0xffff, 0, 0);
	}
	else
	{
		/* enable RAM */
		memory_install_ram(program, 0x8000, 0xffff, 0, 0, ram + (state->bank * 0x8000));
	}

	if (state->msel)
	{
		/* enable video RAM */
		memory_install_ram(program, 0xf800, 0xffff, 0, 0, state->video_ram);
	}
}

static WRITE8_HANDLER( rom_enable_w )
{
	/*

        bit     description

        0       BOOT ROM
        1
        2
        3
        4
        5
        6
        7

    */

	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	state->boot_rom = BIT(data, 0);
	bankswitch(space->machine);
}

static READ8_HANDLER( keyboard_r )
{
	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	/* clear keyboard interrupt */
	state->kbirq = 1;
	z80ctc_trg3_w(state->z80ctc, state->kbirq);

	state->key_bit = 0;

	return state->key_data;
}

static READ8_HANDLER( rtc_r )
{
	/* clear RTC interrupt */
	cputag_set_input_line(space->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);

	return 0;
}

static READ8_HANDLER( nmi_r )
{
	/*

        bit     signal              description

        0
        1
        2
        3
        4       80/40 CHAR EN       80/40 character mode
        5       ENABLE RTC INT      RTC interrupt enable
        6       DE                  display enabled
        7       KBIRQ               keyboard interrupt

    */

	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	UINT8 data = 0;

	/* 80/40 character mode*/
	data |= state->_80_40_char_en << 4;

	/* RTC interrupt enable */
	data |= state->enable_rtc_int << 5;

	/* display enabled */
	data |= state->de << 6;

	/* keyboard interrupt */
	data |= state->kbirq << 7;

	return data;
}

static WRITE8_HANDLER( nmi_w )
{
	/*

        bit     signal              description

        0                           memory bank select bit 0
        1                           memory bank select bit 1
        2                           memory bank select bit 2
        3                           memory bank select bit 3
        4       80/40 CHAR EN       80/40 character mode
        5       ENABLE RTC INT      RTC interrupt enable
        6       BLNKVID             video display enable
        7                           video RAM enable

    */

	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	/* memory bank select */
	state->bank = data & 0x0f;

	/* 80/40 character mode */
	state->_80_40_char_en = BIT(data, 4);
	mc6845_set_clock(state->mc6845, state->_80_40_char_en ? XTAL_12_48MHz/16 : XTAL_12_48MHz/8);

	/* RTC interrupt enable */
	state->enable_rtc_int = BIT(data, 5);

	if (state->enable_rtc_int && state->rtc_int)
	{
		/* trigger RTC interrupt */
		cputag_set_input_line(space->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
	}

	/* video display enable */
	state->blnkvid = BIT(data, 6);

	/* video RAM enable */
	state->msel = BIT(data, 7);
	bankswitch(space->machine);
}

static READ8_HANDLER( keyboard_busy_r )
{
	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	return state->kbirq;
}

static READ8_HANDLER( keyboard_data_r )
{
	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	static const char *const KEY_ROW[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7", "ROW8", "ROW9", "ROW10", "ROW11" };

	return input_port_read(space->machine, KEY_ROW[state->key_latch]);
}

static WRITE8_HANDLER( keyboard_ctrl_w )
{
	/*

        bit     description

        0       DATA
        1       CLOCK
        2       LED
        3
        4       LED
        5
        6
        7

    */

	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	int kbdata = BIT(data, 0);
	int kbclk = BIT(data, 1);

	if (state->key_bit == 8)
	{
		if (!state->kbdata && kbdata)
		{
			/* trigger keyboard interrupt */
			state->kbirq = 0;
			z80ctc_trg3_w(state->z80ctc, state->kbirq);
		}
	}
	else
	{
		if (!state->kbclk && kbclk)
		{
			/* shift in keyboard data bit */
			state->key_data <<= 1;
			state->key_data |= kbdata;
			state->key_bit++;
		}
	}

	state->kbdata = kbdata;
	state->kbclk = kbclk;
}

static WRITE8_HANDLER( keyboard_latch_w )
{
	/*

        bit     description

        0       D
        1       C
        2       B
        3       A
        4
        5
        6
        7

    */

	trs80m2_state *state = (trs80m2_state *)space->machine->driver_data;

	state->key_latch = BITSWAP8(data, 7, 6, 5, 4, 0, 1, 2, 3) & 0x0f;
}

/* Memory Maps */

static ADDRESS_MAP_START( z80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE_MEMBER(trs80m2_state, video_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( z80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xe0, 0xe3) AM_DEVREADWRITE(Z80PIO_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0xe4, 0xe7) AM_DEVREADWRITE(FD1791_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0xef, 0xef) AM_DEVWRITE(FD1791_TAG, drvslt_w)
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xf4, 0xf7) AM_DEVREADWRITE(Z80SIO_TAG, z80sio_cd_ba_r, z80sio_cd_ba_w)
	AM_RANGE(0xf8, 0xf8) AM_DEVREADWRITE(Z80DMA_TAG, z80dma_r, z80dma_w)
	AM_RANGE(0xf9, 0xf9) AM_WRITE(rom_enable_w)
	AM_RANGE(0xfc, 0xfc) AM_READ(keyboard_r) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
	AM_RANGE(0xfd, 0xfd) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0xfe, 0xfe) AM_READ(rtc_r)
	AM_RANGE(0xff, 0xff) AM_READWRITE(nmi_r, nmi_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trs80m2_keyboard_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(keyboard_busy_r)
	AM_RANGE(MCS48_PORT_P0, MCS48_PORT_P0) AM_READ(keyboard_data_r)
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_WRITE(keyboard_ctrl_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_WRITE(keyboard_latch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( m68000_mem, ADDRESS_SPACE_PROGRAM, 16 )
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( trs80m2 )
	PORT_START("ROW0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 0") PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW1")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REPEAT")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Right SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Left SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LOCK")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW3")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("cannot read label")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad .") PORT_CODE(KEYCODE_DEL_PAD)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))

	PORT_START("ROW4")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACKSPACE") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 5") PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("ROW5")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("HOLD")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 2") PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START("ROW6")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad ENTER") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 7") PORT_CODE(KEYCODE_7_PAD)

	PORT_START("ROW7")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 4") PORT_CODE(KEYCODE_4_PAD)

	PORT_START("ROW8")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 8") PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Keypad 1") PORT_CODE(KEYCODE_1_PAD)

	PORT_START("ROW9")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW10")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW11")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Video */

static MC6845_UPDATE_ROW( trs80m2_update_row )
{
	trs80m2_state *state = (trs80m2_state *)device->machine->driver_data;

	for (int column = 0; column < x_count; column++)
	{
		int bit;

		UINT16 address = (state->video_ram[(ma + column) & 0x7ff] << 4) | (ra & 0x0f);
		UINT8 data = state->char_rom[address & 0x7ff];

		if (column == cursor_x)
		{
			data = 0xff;
		}

		for (bit = 0; bit < 8; bit++)
		{
			int x = (column * 8) + bit;

			*BITMAP_ADDR16(bitmap, y, x) = BIT(data, 7);

			data <<= 1;
		}
	}
}

static WRITE_LINE_DEVICE_HANDLER( de_w )
{
	trs80m2_state *driver_state = (trs80m2_state *)device->machine->driver_data;

	driver_state->de = state;
}

static WRITE_LINE_DEVICE_HANDLER( vsync_w )
{
	trs80m2_state *driver_state = (trs80m2_state *)device->machine->driver_data;

	if (state)
	{
		driver_state->rtc_int = !driver_state->rtc_int;

		if (driver_state->enable_rtc_int && driver_state->rtc_int)
		{
			/* trigger RTC interrupt */
			cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
		}
	}
}

static const mc6845_interface mc6845_intf =
{
	SCREEN_TAG,
	8,
	NULL,
	trs80m2_update_row,
	NULL,
	DEVCB_LINE(de_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(vsync_w),
	NULL
};

static VIDEO_START( trs80m2 )
{
	trs80m2_state *state = (trs80m2_state *)machine->driver_data;

	/* find devices */
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);

	/* find memory regions */
	state->char_rom = memory_region(machine, MC6845_TAG);
}

static VIDEO_UPDATE( trs80m2 )
{
	trs80m2_state *state = (trs80m2_state *)screen->machine->driver_data;

	if (state->blnkvid)
	{
		bitmap_fill(bitmap, cliprect, get_black_pen(screen->machine));
	}
	else
	{
		mc6845_update(state->mc6845, bitmap, cliprect);
	}

	return 0;
}

/* Z80-DMA Interface */

static Z80DMA_INTERFACE( dma_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_HALT),
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, PROGRAM, memory_write_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_read_byte),
	DEVCB_MEMORY_HANDLER(Z80_TAG, IO, memory_write_byte)
};

/* Z80-PIO Interface */

static READ8_DEVICE_HANDLER( pio_pa_r )
{
	/*

        bit     signal      description

        0       INTRQ       FDC INT request
        1       _TWOSID     2-sided diskette
        2       _DSKCHG     disk change
        3       PRIME       prime
        4       FAULT       printer fault
        5       PSEL        printer select
        6       PE          paper empty
        7       BUSY        printer busy

    */

	trs80m2_state *state = (trs80m2_state *)device->machine->driver_data;

	UINT8 data = 0;

	/* floppy interrupt */
	data |= state->fdc_intrq;

	/* 2-sided diskette */
	data |= floppy_twosid_r(state->floppy) << 1;

	/* disk change */
	data |= floppy_dskchg_r(state->floppy) << 2;

	/* printer fault */
	data |= centronics_fault_r(state->centronics) << 4;

	/* paper empty */
	data |= !centronics_pe_r(state->centronics) << 6;

	/* printer busy */
	data |= centronics_busy_r(state->centronics) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( pio_pa_w )
{
	/*

        bit     signal      description

        0       INTRQ       FDC INT request
        1       _TWOSID     2-sided diskette
        2       _DSKCHG     disk change
        3       PRIME       prime
        4       FAULT       printer fault
        5       PSEL        printer select
        6       PE          paper empty
        7       BUSY        printer busy

    */

	trs80m2_state *state = (trs80m2_state *)device->machine->driver_data;

	/* prime */
	centronics_prime_w(state->centronics, BIT(data, 3));
}

static WRITE_LINE_DEVICE_HANDLER( strobe_w )
{
	centronics_strobe_w(device, !state);
}

static Z80PIO_INTERFACE( pio_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),				/* interrupt callback */
	DEVCB_HANDLER(pio_pa_r),									/* port A read callback */
	DEVCB_HANDLER(pio_pa_w),									/* port A write callback */
	DEVCB_NULL,													/* port A ready callback */
	DEVCB_NULL,													/* port B read callback */
	DEVCB_DEVICE_HANDLER(CENTRONICS_TAG, centronics_data_w),	/* port B write callback */
	DEVCB_DEVICE_LINE(CENTRONICS_TAG, strobe_w)					/* port B ready callback */
};

/* Centronics Interface */

static const centronics_interface centronics_intf =
{
	0,												/* is IBM PC? */
	DEVCB_DEVICE_LINE(Z80PIO_TAG, z80pio_bstb_w),	/* ACK output */
	DEVCB_NULL,										/* BUSY output */
	DEVCB_NULL										/* NOT BUSY output */
};

/* Z80-SIO/0 Interface */

static void sio_interrupt(running_device *device, int state)
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state);
}

static const z80sio_interface sio_intf =
{
	sio_interrupt,			/* interrupt handler */
	NULL,					/* DTR changed handler */
	NULL,					/* RTS changed handler */
	NULL,					/* BREAK changed handler */
	NULL,					/* transmit handler */
	NULL					/* receive handler */
};

/* Z80-CTC Interface */

static TIMER_DEVICE_CALLBACK( ctc_tick )
{
	trs80m2_state *state = (trs80m2_state *) timer->machine->driver_data;

	z80ctc_trg0_w(state->z80ctc, 1);
	z80ctc_trg0_w(state->z80ctc, 0);

	z80ctc_trg1_w(state->z80ctc, 1);
	z80ctc_trg1_w(state->z80ctc, 0);

	z80ctc_trg2_w(state->z80ctc, 1);
	z80ctc_trg2_w(state->z80ctc, 0);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              								/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_NULL, // DEVCB_DEVICE_LINE(Z80SIO_TAG, z80sio_rxca_w),    /* ZC/TO0 callback */
	DEVCB_NULL, // DEVCB_DEVICE_LINE(Z80SIO_TAG, z80sio_txca_w),    /* ZC/TO1 callback */
	DEVCB_NULL, // DEVCB_DEVICE_LINE(Z80SIO_TAG, z80sio_rxtxcb_w)   /* ZC/TO2 callback */
};

/* FD1791 Interface */

static const floppy_config trs80m2_floppy_config =
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

static WRITE_LINE_DEVICE_HANDLER( fdc_intrq_w )
{
	trs80m2_state *driver_state = (trs80m2_state *)device->machine->driver_data;

	driver_state->fdc_intrq = state;

	z80pio_pa_w(driver_state->z80pio, 0, state);
}

static const wd17xx_interface fd1791_intf =
{
	DEVCB_NULL,
	DEVCB_LINE(fdc_intrq_w),
	DEVCB_DEVICE_LINE(Z80DMA_TAG, z80dma_rdy_w),
	{ FLOPPY_0, NULL, NULL, NULL }
};

/* Z80 Daisy Chain */

static const z80_daisy_chain trs80m2_daisy_chain[] =
{
	{ Z80CTC_TAG },
	{ Z80SIO_TAG },
	{ Z80DMA_TAG },
	{ Z80PIO_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( trs80m2 )
{
	trs80m2_state *state = (trs80m2_state *)machine->driver_data;

	/* find devices */
	state->z80ctc = devtag_get_device(machine, Z80CTC_TAG);
	state->z80pio = devtag_get_device(machine, Z80PIO_TAG);
	state->mc6845 = devtag_get_device(machine, MC6845_TAG);
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->floppy = devtag_get_device(machine, FLOPPY_0);

	/* Shugart SA-800 motor spins constantly */
	floppy_mon_w(state->floppy, CLEAR_LINE);

	/* register for state saving */
	state_save_register_global(machine, state->boot_rom);
	state_save_register_global(machine, state->bank);
	state_save_register_global(machine, state->msel);
	state_save_register_global(machine, state->fdc_intrq);
	state_save_register_global(machine, state->key_latch);
	state_save_register_global(machine, state->key_data);
	state_save_register_global(machine, state->key_bit);
	state_save_register_global(machine, state->kbclk);
	state_save_register_global(machine, state->kbdata);
	state_save_register_global(machine, state->kbirq);
	state_save_register_global(machine, state->blnkvid);
	state_save_register_global(machine, state->_80_40_char_en);
	state_save_register_global(machine, state->de);
	state_save_register_global(machine, state->rtc_int);
	state_save_register_global(machine, state->enable_rtc_int);
}

static MACHINE_RESET( trs80m2 )
{
	trs80m2_state *state = (trs80m2_state *)machine->driver_data;

	/* clear keyboard interrupt */
	state->kbirq = 1;

	/* enable boot ROM */
	state->boot_rom = 1;

	/* disable video RAM */
	state->msel = 0;

	bankswitch(machine);
}

/* Machine Driver */

static MACHINE_DRIVER_START( trs80m2 )
	MDRV_DRIVER_DATA(trs80m2_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_8MHz/2)
	MDRV_CPU_CONFIG(trs80m2_daisy_chain)
    MDRV_CPU_PROGRAM_MAP(z80_mem)
    MDRV_CPU_IO_MAP(z80_io)

//  MDRV_CPU_ADD(I8021_TAG, I8021, 100000)
//  MDRV_CPU_IO_MAP(trs80m2_keyboard_io)

    MDRV_MACHINE_START(trs80m2)
    MDRV_MACHINE_RESET(trs80m2)

	/* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_SCREEN_VISIBLE_AREA(0, 639, 0, 479)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(trs80m2)
	MDRV_VIDEO_UPDATE(trs80m2)

	MDRV_MC6845_ADD(MC6845_TAG, MC6845, XTAL_12_48MHz/8, mc6845_intf)

	/* devices */
	MDRV_WD179X_ADD(FD1791_TAG, fd1791_intf)
	MDRV_Z80CTC_ADD(Z80CTC_TAG, XTAL_8MHz/2, ctc_intf)
	MDRV_TIMER_ADD_PERIODIC("ctc", ctc_tick, HZ(XTAL_8MHz/2/2))
	MDRV_Z80DMA_ADD(Z80DMA_TAG, XTAL_8MHz/2, dma_intf)
	MDRV_Z80PIO_ADD(Z80PIO_TAG, XTAL_8MHz/2, pio_intf)
	MDRV_Z80SIO_ADD(Z80SIO_TAG, XTAL_8MHz/2, sio_intf)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, trs80m2_floppy_config)

	/* internal RAM */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("32K")
	MDRV_RAM_EXTRA_OPTIONS("64K,96K,128K,160K,192K,224K,256K,288K,320K,352K,384K,416K,448K,480K,512K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trs80m16 )
	MDRV_IMPORT_FROM(trs80m2)

	/* basic machine hardware */
//	MDRV_CPU_ADD(M68000_TAG, M68000, 6000000)
//	MDRV_CPU_PROGRAM_MAP(m68000_mem)

	/* video hardware */
	MDRV_PALETTE_INIT(monochrome_green)

	/* internal RAM */
	MDRV_RAM_MODIFY("messram")
	MDRV_RAM_DEFAULT_SIZE("256K")
	MDRV_RAM_EXTRA_OPTIONS("512K,768K,1M")
MACHINE_DRIVER_END

/* ROMs */

ROM_START( trs80m2 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_LOAD( "8043216.u11", 0x0000, 0x0800, CRC(7017a373) SHA1(1c7127fcc99fc351a40d3a3199ba478e783c452e) )

	ROM_REGION( 0x800, MC6845_TAG, 0 )
	ROM_LOAD( "8043316.u9",  0x0000, 0x0800, CRC(04425b03) SHA1(32a29dc202b7fcf21838289cc3bffc51ef943dab) )

	ROM_REGION( 0x400, I8021_TAG, 0 )
	ROM_LOAD( "65-1991.z4", 0x0000, 0x0400, NO_DUMP )
ROM_END

/*

    TRS-80 Model II/16 Z80 CPU Board ROM

    It would seem that every processor board I find has a different ROM on it!  It seems that the early ROMs
    don't boot directly from a hard drive.  But there seems to be many versions of ROMs.  I've placed them in
    order of serial number in the list below.  There also appears to be at least two board revisions, "C" and "D".

    cpu_c8ff.bin/hex:
    Mask Programmable PROM, Equivilant to Intel 2716 EPROM, with checksum C8FF came from a cpu board with
    serial number 120353 out of a Model II with serial number 2002102 and catalog number 26-6002.  The board
    was labeled, "Revision C".  This appears to be an early ROM and according to a very helpful fellow
    collector, Aaron in Australia, doesn't allow boot directly from a hard disk.

    cpu_9733.bin/hex:
    An actual SGS-Ates (Now STMicroelectronics) 2716 EPROM, with checksum 9733 came from a cpu board with
    serial number 161993 out of a pile of random cards that I have.  I don't know what machine it originated
    from.  The board was labeled, "Revision C".  This appears to be a later ROM in that it is able to boot
    directly from an 8MB hard disk.  The EPROM had a windows sticker on it labeled, "U54".

    cpu_2119.bin/hex:
    An actual Texas Instruments 2716 EPROM, with checksum 2119 came from a cpu board with serial number
    178892 out of a Model 16 with serial number 64014509 and catalog number 26-4002.  The board was labeled,
    "Revision D".  This appears to be a later ROM and does appear to allow boot directly from an 8MB hard disk.

    cpu_2bff.bin/hex:
    Mask Programmable PROM, Equivilant to Intel 2716 EPROM, with checksum 2BFF came from a cpu board with
    serial number 187173 our of a pile of random cards that I have.  I don't know what machine it originated
    from.  The board was labeled, "Revision D".  This appears to be a later ROM in that it is able to boot
    directly from an 8MB hard disk.

*/

ROM_START( trs80m16 )
	ROM_REGION( 0x800, Z80_TAG, 0 )
	ROM_SYSTEM_BIOS( 0, "c8ff", "S/N 120353" )
	ROMX_LOAD( "cpu_c8ff.u11",   0x0000, 0x0800, CRC(7017a373) SHA1(1c7127fcc99fc351a40d3a3199ba478e783c452e), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "9733", "S/N 161993" )
	ROMX_LOAD( "cpu_9733.u11",   0x0000, 0x0800, CRC(823924b1) SHA1(aee0625bcbd8620b28ab705e15ad9bea804c8476), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "2119", "S/N 64014509" )
	ROMX_LOAD( "cpu_2119.u11",   0x0000, 0x0800, CRC(7a663049) SHA1(f308439ce266df717bfe79adcdad6024b4faa141), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "2bff", "S/N 187173" )
	ROMX_LOAD( "cpu_2bff.u11",   0x0000, 0x0800, CRC(c6c71d8b) SHA1(7107e2cbbe769851a4460680c2deff8e76a101b5), ROM_BIOS(4) )

	ROM_REGION( 0x800, MC6845_TAG, 0 )
	ROM_LOAD( "8043316.u9",  0x0000, 0x0800, CRC(04425b03) SHA1(32a29dc202b7fcf21838289cc3bffc51ef943dab) )

	ROM_REGION( 0x400, I8021_TAG, 0 )
	ROM_LOAD( "65-1991.z4", 0x0000, 0x0400, NO_DUMP )

	ROM_REGION( 0x1000, M68000_TAG, 0 )
	ROM_LOAD( "m68000.rom", 0x0000, 0x1000, NO_DUMP )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY                 FULLNAME            FLAGS */
COMP( 1979, trs80m2,	0,			0,		trs80m2,	trs80m2,	0,		"Tandy Radio Shack",	"TRS-80 Model II",	GAME_NO_SOUND_HW | GAME_NOT_WORKING )
COMP( 1982, trs80m16,	trs80m2,	0,		trs80m16,	trs80m2,	0,		"Tandy Radio Shack",	"TRS-80 Model 16",	GAME_NO_SOUND_HW | GAME_NOT_WORKING )
//COMP( 1983, trs80m12, trs80m2,    0,      trs80m16,   trs80m2,    0,      "Tandy Radio Shack",    "TRS-80 Model 12",  GAME_NO_SOUND_HW | GAME_NOT_WORKING )
//COMP( 1984, trs80m16b,trs80m2,    0,      trs80m16,   trs80m2,    0,      "Tandy Radio Shack",    "TRS-80 Model 16B", GAME_NO_SOUND_HW | GAME_NOT_WORKING )
//COMP( 1985, tandy6k,  trs80m2,    0,      tandy6k,    trs80m2,    0,      "Tandy Radio Shack",    "Tandy 6000 HD",    GAME_NO_SOUND_HW | GAME_NOT_WORKING )