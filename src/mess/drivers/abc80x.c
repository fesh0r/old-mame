/*
    abc800.c

    MESS Driver by Curt Coder

    Luxor ABC 800C
    --------------
    (c) 1981 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 1 KB frame buffer, 16 KB high-resolution videoram (800C/HR)
    CRTC:           6845
    Resolution:     240x240
    Colors:         8

    Luxor ABC 800M
    --------------
    (c) 1981 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 2 KB frame buffer, 16 KB high-resolution videoram (800M/HR)
    CRTC:           6845
    Resolution:     480x240, 240x240 (HR)
    Colors:         2

    Luxor ABC 802
    -------------
    (c) 1983 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            16 KB, 2 KB frame buffer, 16 KB ram-floppy
    CRTC:           6845
    Resolution:     480x240
    Colors:         2

    Luxor ABC 806
    -------------
    (c) 1983 Luxor Datorer AB, Sweden

    CPU:            Z80 @ 3 MHz
    ROM:            32 KB
    RAM:            32 KB, 4 KB frame buffer, 128 KB scratch pad ram
    CRTC:           6845
    Resolution:     240x240, 256x240, 512x240
    Colors:         8

    http://www.devili.iki.fi/Computers/Luxor/
    http://hem.passagen.se/mani/abc/
*/

/*

    TODO:

    - ABC77 keyboard ROM dump is needed!
	- refactor ABC77/99 keyboards into devices?
	- rewrite Z80DART for bit level serial I/O
	- ABC806 memory banking for 0x7000-0x7fff
    - keyboard NE556 discrete beeper
    - COM port DIP switch
    - floppy controller board
	- TeleDisk (td0) image support
    - Facit DTC (recased ABC-800?)
    - hard disks (ABC-850 10MB, ABC-852 20MB, ABC-856 60MB)

*/

/* Core includes */
#include "driver.h"
#include "includes/abc80x.h"

/* Components */
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "cpu/i8039/i8039.h"
#include "machine/centroni.h"
#include "includes/serial.h"
#include "machine/z80ctc.h"
#include "machine/z80sio.h"
#include "machine/z80dart.h"
#include "machine/abcbus.h"
#include "machine/e0516.h"
#include "video/mc6845.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"

static emu_timer *abc800_ctc_timer;

/* Read/Write Handlers */

// ABC 800

static WRITE8_HANDLER( abc800_ram_ctrl_w )
{
}

// ABC 806

static UINT8 abc806_bank[16] = { 0 };

static READ8_HANDLER( abc806_bankswitch_r )
{
	int bank = offset >> 12;

	return abc806_bank[bank];
}

static WRITE8_HANDLER( abc806_bankswitch_w )
{
	/*

		bit		description
		0		physical block address bit 0
		1		physical block address bit 1
		2		physical block address bit 2
		3		physical block address bit 3
		4		physical block address bit 4
		5
		6
		7		allocate block

		- extended 128KB memory is divided into 32 physical blocks of 4KB each
		- logical CPU address space is divided into 16 logical blocks of 4KB each
		- extended blocks can be allocated to the logical blocks
		- logical address is provided by the offset (e.g write to port 0xc034 -> address 0xc000)

	*/

	int bank = offset >> 12;
	int block = data & 0x3f;

	abc806_bank[bank] = data;

	if (bank != 7)
	{
		UINT16 bank_start = offset & 0xf000;
		UINT16 bank_end = bank_start + 0xfff;

		if (BIT(data, 7))
		{
			UINT16 block_start = block * 0x1000;
			UINT16 block_end = block_start + 0xfff;

			logerror("ABC806 allocating extended block %u (%04x-%04x) to %04x-%04x\n", block, block_start, block_end, bank_start, bank_end);

			// allocate to extended memory block
			memory_set_bank(bank + 1, block + 1);
		}
		else
		{
			// deallocate back to "main"
			memory_set_bank(bank + 1, 0);

			if (bank < 7)
			{
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, bank_start, bank_end, 0, 0, SMH_UNMAP);
				logerror("ABC806 deallocating %04x-%04x back to ROM\n", bank_start, bank_end);
			}
			else
			{
				memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, bank_start, bank_end, 0, 0, (write8_machine_func)(FPTR)(STATIC_BANK1 + bank));
				logerror("ABC806 deallocating %04x-%04x back to RAM\n", bank_start, bank_end);
			}
		}
	}
}

// Z80 SIO/2

static READ8_HANDLER( sio2_r )
{
	switch (offset)
	{
	case 0:
		return z80sio_d_r(0, 0);
	case 1:
		return z80sio_c_r(0, 0);
	case 2:
		return z80sio_d_r(0, 1);
	case 3:
		return z80sio_c_r(0, 1);
	}

	return 0;
}

static WRITE8_HANDLER( sio2_w )
{
	switch (offset)
	{
	case 0:
		z80sio_d_w(0, 0, data);
		break;
	case 1:
		z80sio_c_w(0, 0, data);
		break;
	case 2:
		z80sio_d_w(0, 1, data);
		break;
	case 3:
		z80sio_c_w(0, 1, data);
		break;
	}
}

// Z80 DART

static READ8_HANDLER( dart_r )
{
	switch (offset)
	{
	case 0:
		return z80dart_d_r(0, 0);
	case 1:
		return z80dart_c_r(0, 0);
	case 2:
		return z80dart_d_r(0, 1);
	case 3:
		return z80dart_c_r(0, 0); // this gets ABC806 booting, should be z80dart_c_r(0, 1)
	}

	return 0xff;
}

static WRITE8_HANDLER( dart_w )
{
	switch (offset)
	{
	case 0:
		z80dart_d_w(0, 0, data);
		break;
	case 1:
		z80dart_c_w(machine, 0, 0, data);
		break;
	case 2:
		z80dart_d_w(0, 1, data);
		break;
	case 3:
		z80dart_c_w(machine, 0, 1, data);
		break;
	}
}

// ABC-77 keyboard

/*

    Keyboard connector pinout

    pin     description     connected to
    -------------------------------------------
    1       TxD             i8035 pin 36 (P25)
    2       GND
    3       RxD             i8035 pin 6 (_INT)
    4       CLOCK           i8035 pin 39 (T1)
    5       _KEYDOWN        i8035 pin 37 (P26)
    6       +12V
    7       _RESET          NE556 pin 8 (2TRIG)

*/

static int abc77_keylatch, abc77_keydown, abc77_clock, abc77_beep, abc77_hys, abc77_txd;

static READ8_HANDLER( abc77_clock_r )
{
	return abc77_clock;
}

static READ8_HANDLER( abc77_data_r )
{
	static const char *keynames[] = { "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7", "X8", "X9", "X10", "X11" };

	return input_port_read(machine, keynames[abc77_keylatch]);
}

static WRITE8_HANDLER( abc77_data_w )
{
	abc77_keylatch = data & 0x0f;

	if (abc77_keylatch == 1)
	{
		watchdog_reset(machine);
	}

	abc77_beep = BIT(data, 4);
	abc77_txd = BIT(data, 5);
	abc77_keydown = BIT(data, 6);
	abc77_hys = BIT(data, 7);

	z80dart_set_dcd(0, 0, abc77_keydown);
	//z80dart_set_cts(0, 0, abc77_keydown); on ABC802 only

	// write abc77_txd to Z80DART TxD
}

static READ8_HANDLER( abc77_ea_r )
{
	return input_port_read(machine, "DSW") & 0x01;
}

/* Memory Maps */

// ABC 77 keyboard

static ADDRESS_MAP_START( abc77_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x000, 0xfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc77_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x3f) AM_RAM
	AM_RANGE(I8039_p1, I8039_p1) AM_READ(abc77_data_r)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(abc77_data_w)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(abc77_clock_r)
	AM_RANGE(I8039_bus, I8039_bus) AM_READ_PORT("DSW")
	AM_RANGE(I8039_ea, I8039_ea) AM_READ(abc77_ea_r)
ADDRESS_MAP_END

// ABC 800M

static ADDRESS_MAP_START( abc800m_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800m_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0x18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x18) AM_WRITE(abc800m_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_READWRITE(abcbus_reset_r, abc800m_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(abc800_ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_MIRROR(0x1c) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 800C

static ADDRESS_MAP_START( abc800c_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x0000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7bff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x7800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc800c_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0x18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0x18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0x18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0x18) AM_WRITE(abc800c_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0x18) AM_READWRITE(abcbus_reset_r, abc800c_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0x0c) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x30, 0x32) AM_WRITE(abc800_ram_ctrl_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0x06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0x06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x43) AM_MIRROR(0x1c) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x50, 0x53) AM_MIRROR(0x1c) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 802

static ADDRESS_MAP_START( abc802_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x77ff) AM_RAMBANK(1)
	AM_RANGE(0x7800, 0x7fff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc802_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x07, 0x07) AM_READ(abcbus_reset_r)
	AM_RANGE(0x20, 0x23) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x32, 0x35) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x38, 0x38) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x60, 0x63) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0xff) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

// ABC 806

static ADDRESS_MAP_START( abc806_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0x1fff) AM_RAMBANK(2)
	AM_RANGE(0x2000, 0x2fff) AM_RAMBANK(3)
	AM_RANGE(0x3000, 0x3fff) AM_RAMBANK(4)
	AM_RANGE(0x4000, 0x4fff) AM_RAMBANK(5)
	AM_RANGE(0x5000, 0x5fff) AM_RAMBANK(6)
	AM_RANGE(0x6000, 0x6fff) AM_RAMBANK(7)
	//
	AM_RANGE(0x7000, 0x77ff) AM_ROM
	AM_RANGE(0x7800, 0x7fff) AM_READWRITE(abc806_videoram_r, abc806_videoram_w) AM_BASE(&videoram)
	//
	AM_RANGE(0x8000, 0x8fff) AM_RAMBANK(9)
	AM_RANGE(0x9000, 0x9fff) AM_RAMBANK(10)
	AM_RANGE(0xa000, 0xafff) AM_RAMBANK(11)
	AM_RANGE(0xb000, 0xbfff) AM_RAMBANK(12)
	AM_RANGE(0xc000, 0xcfff) AM_RAMBANK(13)
	AM_RANGE(0xd000, 0xdfff) AM_RAMBANK(14)
	AM_RANGE(0xe000, 0xefff) AM_RAMBANK(15)
	AM_RANGE(0xf000, 0xffff) AM_RAMBANK(16)
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc806_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff1f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff18) AM_READWRITE(abcbus_data_r, abcbus_data_w)
	AM_RANGE(0x01, 0x01) AM_MIRROR(0xff18) AM_READWRITE(abcbus_status_r, abcbus_channel_w)
	AM_RANGE(0x02, 0x05) AM_MIRROR(0xff18) AM_WRITE(abcbus_command_w)
	AM_RANGE(0x06, 0x06) AM_MIRROR(0xff18) AM_WRITE(abc806_hrs_w)
	AM_RANGE(0x07, 0x07) AM_MIRROR(0xff18) AM_READWRITE(abcbus_reset_r, abc806_hrc_w)
	AM_RANGE(0x20, 0x23) AM_MIRROR(0xff0c) AM_READWRITE(dart_r, dart_w)
	AM_RANGE(0x31, 0x31) AM_MIRROR(0xff06) AM_DEVREAD(MC6845, MC6845_TAG, mc6845_register_r)
	AM_RANGE(0x34, 0x34) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_READWRITE(abc806_bankswitch_r, abc806_bankswitch_w)
	AM_RANGE(0x35, 0x35) AM_MIRROR(0xff00) AM_READWRITE(abc806_colorram_r, abc806_colorram_w)
	AM_RANGE(0x36, 0x36) AM_MIRROR(0xff00) AM_WRITE(abc806_fgctlprom_w)
	AM_RANGE(0x37, 0x37) AM_MIRROR(0xff00) AM_READWRITE(abc806_fgctlprom_r, abc806_sync_w)
	AM_RANGE(0x38, 0x38) AM_MIRROR(0xff06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_address_w)
	AM_RANGE(0x39, 0x39) AM_MIRROR(0xff06) AM_DEVWRITE(MC6845, MC6845_TAG, mc6845_register_w)
	AM_RANGE(0x40, 0x41) AM_MIRROR(0xff1c) AM_READWRITE(sio2_r, sio2_w)
	AM_RANGE(0x60, 0x63) AM_MIRROR(0xff1c) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0xff7f) AM_READWRITE(abcbus_strobe_r, abcbus_strobe_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( abc77 )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CAPS LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('<') PORT_CHAR('>')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR('\r')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR(0x00E9) PORT_CHAR(0x00C9)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('+') PORT_CHAR('?')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR(0x00E5) PORT_CHAR(0x00C5)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(0x00FC) PORT_CHAR(0x00DC)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(0x00E4) PORT_CHAR(0x00C4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\'') PORT_CHAR('*')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('=')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(0x00F6) PORT_CHAR(0x00D6)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('-') PORT_CHAR('_')

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('/')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR(0x00A4)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')

	PORT_START("X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')

	PORT_START("X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 9") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD +") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 6") PORT_CODE(KEYCODE_6_PAD) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD -") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 3") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD RETURN") PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD .") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))

	PORT_START("X10")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 7") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 8") PORT_CODE(KEYCODE_8_PAD) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 4") PORT_CODE(KEYCODE_4_PAD) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 5") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 1") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 2") PORT_CODE(KEYCODE_2_PAD) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("KEYPAD 0") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("X11")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF5") PORT_CODE(KEYCODE_F5) PORT_CHAR(UCHAR_MAMEKEY(F5))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF6") PORT_CODE(KEYCODE_F6) PORT_CHAR(UCHAR_MAMEKEY(F6))
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF7") PORT_CODE(KEYCODE_F7) PORT_CHAR(UCHAR_MAMEKEY(F7))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("PF8") PORT_CODE(KEYCODE_F8) PORT_CHAR(UCHAR_MAMEKEY(F8))

	PORT_START("DSW")
	PORT_DIPNAME( 0x01, 0x01, "Keyboard Program" )
	PORT_DIPSETTING(    0x00, "Internal (8048)" )
	PORT_DIPSETTING(    0x01, "External PROM" )
	PORT_DIPNAME( 0x02, 0x00, "Character Set" )
	PORT_DIPSETTING(    0x00, "Swedish" )
	PORT_DIPSETTING(    0x02, "US ASCII" )
	PORT_DIPNAME( 0x04, 0x04, "External PROM" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "Danish" )
	PORT_DIPSETTING(    0x10, DEF_STR( French ) )
	PORT_DIPSETTING(    0x08, DEF_STR( German ) )
	PORT_DIPSETTING(    0x18, DEF_STR( Spanish ) )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START( abc800 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

static INPUT_PORTS_START( abc802 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

static INPUT_PORTS_START( abc806 )
	PORT_INCLUDE(abc77)
INPUT_PORTS_END

/* Machine Initialization */

static WRITE8_HANDLER( abc800_ctc_z2_w )
{
	// write to DART channel A RxC/TxC
}

static z80ctc_interface abc800_ctc_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,              		/* timer disables */
	0,				  		/* interrupt handler */
	0,						/* ZC/TO0 callback */
	0,              		/* ZC/TO1 callback */
	abc800_ctc_z2_w    		/* ZC/TO2 callback */
};

static WRITE8_HANDLER( sio_serial_transmit )
{
}

static int sio_serial_receive(int ch)
{
	return -1;
}

static z80sio_interface abc800_sio_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	sio_serial_transmit,	/* transmit handler */
	sio_serial_receive		/* receive handler */
};

static WRITE8_HANDLER( dart_serial_transmit )
{
}

static int dart_serial_receive(int ch)
{
	return -1;
}

static const z80dart_interface abc800_dart_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	0,						/* DTR changed handler */
	0,						/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static WRITE8_HANDLER( abc802_dart_dtr_w )
{
	if (offset == 1)
	{
		memory_set_bank(1, data);
	}
}

static WRITE8_HANDLER( abc802_dart_rts_w )
{
	if (offset == 1)
	{
		abc802_mux80_40_w(!BIT(data, 0));
	}
}

static const z80dart_interface abc802_dart_intf =
{
	ABC800_X01/2/2,			/* clock */
	0,						/* interrupt handler */
	abc802_dart_dtr_w,		/* DTR changed handler */
	abc802_dart_rts_w,		/* RTS changed handler */
	0,						/* BREAK changed handler */
	dart_serial_transmit,	/* transmit handler */
	dart_serial_receive		/* receive handler */
};

static const struct z80_irq_daisy_chain abc800_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 },
	{ z80sio_reset, z80sio_irq_state, z80sio_irq_ack, z80sio_irq_reti, 0 },
	{ z80dart_reset, z80dart_irq_state, z80dart_irq_ack, z80dart_irq_reti, 0 },
	{ 0, 0, 0, 0, -1 }
};

static TIMER_CALLBACK(abc800_ctc_tick)
{
	z80ctc_trg_w(machine, 0, 0, 1);
	z80ctc_trg_w(machine, 0, 0, 0);
	z80ctc_trg_w(machine, 0, 1, 1);
	z80ctc_trg_w(machine, 0, 1, 0);
	z80ctc_trg_w(machine, 0, 2, 1);
	z80ctc_trg_w(machine, 0, 2, 0);
}

static MACHINE_START( abc800 )
{
	state_save_register_global(abc77_keylatch);
	state_save_register_global(abc77_clock);

	abc800_ctc_timer = timer_alloc(abc800_ctc_tick, NULL);
	timer_adjust_periodic(abc800_ctc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(ABC800_X01/2/2/2));

	z80ctc_init(0, &abc800_ctc_intf);
	z80sio_init(0, &abc800_sio_intf);
	z80dart_init(0, &abc800_dart_intf);
}

static MACHINE_START( abc802 )
{
	MACHINE_START_CALL(abc800);

	z80dart_init(0, &abc802_dart_intf);

	memory_configure_bank(1, 0, 1, memory_region(machine, "main"), 0);
	memory_configure_bank(1, 1, 1, mess_ram, 0);
}

static MACHINE_RESET( abc802 )
{
	memory_set_bank(1, 0);
}

static const e0516_interface abc806_e0516_intf =
{
	ABC806_X02
};

static MACHINE_START( abc806 )
{
	UINT8 *mem;
	int bank;

	MACHINE_START_CALL(abc800);

	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x0fff, 0, 0, SMH_BANK1, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x1000, 0x1fff, 0, 0, SMH_BANK2, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, 0, SMH_BANK3, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, 0, SMH_BANK4, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x4fff, 0, 0, SMH_BANK5, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x5fff, 0, 0, SMH_BANK6, SMH_UNMAP);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, SMH_BANK7, SMH_UNMAP);

//	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x7fff, 0, 0, SMH_BANK8, SMH_BANK8);

	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, SMH_BANK9, SMH_BANK9);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, 0, SMH_BANK10, SMH_BANK10);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, SMH_BANK11, SMH_BANK11);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, 0, SMH_BANK12, SMH_BANK12);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xcfff, 0, 0, SMH_BANK13, SMH_BANK13);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, SMH_BANK14, SMH_BANK14);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xefff, 0, 0, SMH_BANK15, SMH_BANK15);
	memory_install_readwrite8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffff, 0, 0, SMH_BANK16, SMH_BANK16);

	mem = memory_region(machine, "main");
	for (bank = 1; bank < 17; bank++)
	{
		if (bank != 8)
		{
			memory_configure_bank(bank, 0, 1, mem + ((bank - 1) * 0x1000), 0);
			memory_configure_bank(bank, 1, 32, mess_ram, 0x1000);

			memory_set_bank(bank, 0);
		}
	}
}

static MACHINE_RESET( abc806 )
{
	int bank;

	for (bank = 1; bank < 17; bank++)
	{
		if (bank != 8)
		{
			memory_set_bank(bank, 0);
		}
	}
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc800m )
	// basic machine hardware
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)	// 3 MHz
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800m_map, 0)
	MDRV_CPU_IO_MAP(abc800m_io_map, 0)

	// ABC77 keyboard
	MDRV_CPU_ADD("keyboard", I8035, 4608000)
	MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_MACHINE_START(abc800)

	MDRV_IMPORT_FROM(abc800m_video)

	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc800c )
	// basic machine hardware
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)	// 3 MHz
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc800c_map, 0)
	MDRV_CPU_IO_MAP(abc800c_io_map, 0)

	// ABC77 keyboard
	MDRV_CPU_ADD("keyboard", I8035, 4608000)
	MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_MACHINE_START(abc800)

	MDRV_IMPORT_FROM(abc800c_video)

	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc802 )
	// basic machine hardware
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)	// 3 MHz
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc802_map, 0)
	MDRV_CPU_IO_MAP(abc802_io_map, 0)

	// ABC77 keyboard
	MDRV_CPU_ADD("keyboard", I8035, 4608000)
	MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_MACHINE_START(abc802)
	MDRV_MACHINE_RESET(abc802)

	MDRV_IMPORT_FROM(abc802_video)

	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( abc806 )
	// basic machine hardware
	MDRV_CPU_ADD("main", Z80, ABC800_X01/2/2)	// 3 MHz
	MDRV_CPU_CONFIG(abc800_daisy_chain)
	MDRV_CPU_PROGRAM_MAP(abc806_map, 0)
	MDRV_CPU_IO_MAP(abc806_io_map, 0)

	// ABC77 keyboard
	MDRV_CPU_ADD("keyboard", I8035, 4608000)
	MDRV_CPU_PROGRAM_MAP(abc77_map, 0)
	MDRV_CPU_IO_MAP(abc77_io_map, 0)

	MDRV_MACHINE_START(abc806)
	MDRV_MACHINE_RESET(abc806)

	MDRV_IMPORT_FROM(abc806_video)

	MDRV_DEVICE_ADD("printer", PRINTER)

	MDRV_DEVICE_ADD(E0516_TAG, E0516)
	MDRV_DEVICE_CONFIG(abc806_e0516_intf)
MACHINE_DRIVER_END

/* ROMs */

/*

    ABC800 DOS ROMs

    Label       Drive Type
    ----------------------------
    ABC 6-1X    ABC830,DD82,DD84
    800 8"      DD88
    ABC 6-2X    ABC832
    ABC 6-3X    ABC838
    UFD 6.XX    Winchester

*/

#define ROM_ABC77 \
	ROM_REGION( 0x1000, "keyboard", 0 ) \
	ROM_LOAD( "65-02486.z10", 0x0000, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller Swedish EPROM */ \
	ROM_LOAD( "keyboard.z14", 0x0800, 0x0800, NO_DUMP ) /* 2716 ABC55/77 keyboard controller non-Swedish EPROM */

#define ROM_ABC99 \
	ROM_REGION( 0x1000, "keyboard", 0 ) \
	ROM_LOAD( "abc99.bin", 0x0000, 0x0800, CRC(d48310fc) SHA1(17a2ffc0ec00d395c2b9caf3d57fed575ba2b137) )

#define ROM_ABC99_2 \
	ROM_REGION( 0x1800, "keyboard", 0 ) \
	ROM_LOAD( "10681909", 0x0000, 0x1000, CRC(ffe32a71) SHA1(fa2ce8e0216a433f9bbad0bdd6e3dc0b540f03b7) ) \
	ROM_LOAD( "10726864", 0x1000, 0x0800, CRC(e33683ae) SHA1(0c1d9e320f82df05f4804992ef6f6f6cd20623f3) )

#define ROM_KEYBOARD ROM_ABC77

ROM_START( abc800c )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abcc.1m",    0x0000, 0x1000, NO_DUMP )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_KEYBOARD

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "vuc-se.7c",  0x0000, 0x1000, NO_DUMP )

	ROM_REGION( 0x2000, "user1", 0 )
	// Fast Controller
	ROM_LOAD( "fast108.bin",  0x0000, 0x2000, CRC(229764cb) SHA1(a2e2f6f49c31b827efc62f894de9a770b65d109d) ) // Luxor v1.08
	ROM_LOAD( "fast207.bin",  0x0000, 0x2000, CRC(86622f52) SHA1(61ad271de53152c1640c0b364fce46d1b0b4c7e2) ) // DIAB v2.07

	// MyAB Turbo-Kontroller
	ROM_LOAD( "unidis5d.bin", 0x0000, 0x1000, CRC(569dd60c) SHA1(47b810bcb5a063ffb3034fd7138dc5e15d243676) ) // 5" 25-pin
	ROM_LOAD( "unidiskh.bin", 0x0000, 0x1000, CRC(5079ad85) SHA1(42bb91318f13929c3a440de3fa1f0491a0b90863) ) // 5" 34-pin
	ROM_LOAD( "unidisk8.bin", 0x0000, 0x1000, CRC(d04e6a43) SHA1(8db504d46ff0355c72bd58fd536abeb17425c532) ) // 8"

	// ABC-832
	ROM_LOAD( "micr1015.bin", 0x0000, 0x0800, CRC(a7bc05fa) SHA1(6ac3e202b7ce802c70d89728695f1cb52ac80307) ) // Micropolis 1015
	ROM_LOAD( "micr1115.bin", 0x0000, 0x0800, CRC(f2fc5ccc) SHA1(86d6baadf6bf1d07d0577dc1e092850b5ff6dd1b) ) // Micropolis 1115
	ROM_LOAD( "basf6118.bin", 0x0000, 0x0800, CRC(9ca1a1eb) SHA1(04973ad69de8da403739caaebe0b0f6757e4a6b1) ) // BASF 6118

	// ABC-850
	ROM_LOAD( "rodi202.bin",  0x0000, 0x0800, CRC(337b4dcf) SHA1(791ebeb4521ddc11fb9742114018e161e1849bdf) ) // Rodime 202
	ROM_LOAD( "basf6185.bin", 0x0000, 0x0800, CRC(06f8fe2e) SHA1(e81f2a47c854e0dbb096bee3428d79e63591059d) ) // BASF 6185

	// ABC-852
	ROM_LOAD( "nec5126.bin",  0x0000, 0x1000, CRC(17c247e7) SHA1(7339738b87751655cb4d6414422593272fe72f5d) ) // NEC 5126

	// ABC-856
	ROM_LOAD( "micr1325.bin", 0x0000, 0x0800, CRC(084af409) SHA1(342b8e214a8c4c2b014604e53c45ef1bd1c69ea3) ) // Micropolis 1325

	// unknown
	ROM_LOAD( "st4038.bin",   0x0000, 0x0800, CRC(4c803b87) SHA1(1141bb51ad9200fc32d92a749460843dc6af8953) ) // Seagate ST4038
	ROM_LOAD( "st225.bin",    0x0000, 0x0800, CRC(c9f68f81) SHA1(7ff8b2a19f71fe0279ab3e5a0a5fffcb6030360c) ) // Seagate ST225
ROM_END

ROM_START( abc800m )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abcm.1m",    0x0000, 0x1000, CRC(f85b274c) SHA1(7d0f5639a528d8d8130a22fe688d3218c77839dc) )
	ROM_LOAD( "abc1-12.1l", 0x1000, 0x1000, CRC(1e99fbdc) SHA1(ec6210686dd9d03a5ed8c4a4e30e25834aeef71d) )
	ROM_LOAD( "abc2-12.1k", 0x2000, 0x1000, CRC(ac196ba2) SHA1(64fcc0f03fbc78e4c8056e1fa22aee12b3084ef5) )
	ROM_LOAD( "abc3-12.1j", 0x3000, 0x1000, CRC(3ea2b5ee) SHA1(5a51ac4a34443e14112a6bae16c92b5eb636603f) )
	ROM_LOAD( "abc4-12.2m", 0x4000, 0x1000, CRC(695cb626) SHA1(9603ce2a7b2d7b1cbeb525f5493de7e5c1e5a803) )
	ROM_LOAD( "abc5-12.2l", 0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_LOAD( "abc6-13.2k", 0x6000, 0x1000, CRC(6fa71fb6) SHA1(b037dfb3de7b65d244c6357cd146376d4237dab6) )
	ROM_LOAD( "abc7-21.2j", 0x7000, 0x1000, CRC(fd137866) SHA1(3ac914d90db1503f61397c0ea26914eb38725044) )

	ROM_KEYBOARD

	ROM_REGION( 0x0800, "chargen", 0 )
	ROM_LOAD( "vum-se.7c",  0x0000, 0x0800, CRC(f9152163) SHA1(997313781ddcbbb7121dbf9eb5f2c6b4551fc799) )
ROM_END

ROM_START( abc802 )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD(  "abc02-11.9f",  0x0000, 0x2000, CRC(b86537b2) SHA1(4b7731ef801f9a03de0b5acd955f1e4a1828355d) )
	ROM_LOAD(  "abc12-11.11f", 0x2000, 0x2000, CRC(3561c671) SHA1(f12a7c0fe5670ffed53c794d96eb8959c4d9f828) )
	ROM_LOAD(  "abc22-11.12f", 0x4000, 0x2000, CRC(8dcb1cc7) SHA1(535cfd66c84c0370fd022d6edf702d3d1ad1b113) )
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v6.19" )
	ROMX_LOAD( "abc32-21.14f", 0x6000, 0x2000, CRC(57050b98) SHA1(b977e54d1426346a97c98febd8a193c3e8259574), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v6.20" )
	ROMX_LOAD( "abc32-31.14f", 0x6000, 0x2000, CRC(fc8be7a8) SHA1(a1d4cb45cf5ae21e636dddfa70c99bfd2050ad60), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v6.20" )
	ROMX_LOAD( "mica820.14f",  0x6000, 0x2000, CRC(edf998af) SHA1(daae7e1ff6ef3e0ddb83e932f324c56f4a98f79b), ROM_BIOS(3) )

	ROM_KEYBOARD

	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "abct2-11.3g",  0x0000, 0x2000, CRC(e21601ee) SHA1(2e838ebd7692e5cb9ba4e80fe2aa47ea2584133a) )

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "abcp2-11.2g", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
ROM_END

ROM_START( abc806 )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "abc06-11.1m",  0x0000, 0x1000, CRC(27083191) SHA1(9b45592273a5673e4952c6fe7965fc9398c49827) )
	ROM_LOAD( "abc16-11.1l",  0x1000, 0x1000, CRC(eb0a08fd) SHA1(f0b82089c5c8191fbc6a3ee2c78ce730c7dd5145) )
	ROM_LOAD( "abc26-11.1k",  0x2000, 0x1000, CRC(97a95c59) SHA1(013bc0a2661f4630c39b340965872bf607c7bd45) )
	ROM_LOAD( "abc36-11.1j",  0x3000, 0x1000, CRC(b50e418e) SHA1(991a59ed7796bdcfed310012b2bec50f0b8df01c) )
	ROM_LOAD( "abc46-11.2m",  0x4000, 0x1000, CRC(17a87c7d) SHA1(49a7c33623642b49dea3d7397af5a8b9dde8185b) )
	ROM_LOAD( "abc56-11.2l",  0x5000, 0x1000, CRC(b4b02358) SHA1(95338efa3b64b2a602a03bffc79f9df297e9534a) )
	ROM_SYSTEM_BIOS( 0, "v19",		"UDF-DOS v.19" )
	ROMX_LOAD( "abc66-21.2k", 0x6000, 0x1000, CRC(c311b57a) SHA1(4bd2a541314e53955a0d53ef2f9822a202daa485), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v20",		"UDF-DOS v.20" )
	ROMX_LOAD( "abc66-31.2k", 0x6000, 0x1000, CRC(a2e38260) SHA1(0dad83088222cb076648e23f50fec2fddc968883), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "mica",		"MICA DOS v.20" )
	ROMX_LOAD( "mica2006.2k", 0x6000, 0x1000, CRC(58bc2aa8) SHA1(0604bd2396f7d15fcf3d65888b4b673f554037c0), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "catnet",	"CAT-NET" )
	ROMX_LOAD( "cmd8_5.2k",	  0x6000, 0x1000, CRC(25430ef7) SHA1(03a36874c23c215a19b0be14ad2f6b3b5fb2c839), ROM_BIOS(4) )
	ROM_LOAD( "abc76-11.2j",  0x7000, 0x1000, CRC(3eb5f6a1) SHA1(02d4e38009c71b84952eb3b8432ad32a98a7fe16) ) // 6490238-02

	ROM_KEYBOARD

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "abct6-11.7c",   0x0000, 0x1000, CRC(b17c51c5) SHA1(e466e80ec989fbd522c89a67d274b8f0bed1ff72) ) // 6490243-01

	ROM_REGION( 0x620, "proms", 0 )
	ROM_LOAD( "rad.9b",		 0x0000, 0x0200, NO_DUMP ) // 7621/7643 (82S131/82S137)
	ROM_LOAD( "hrui.6e",	 0x0200, 0x0020, NO_DUMP ) // 7603 (82S123)
	ROM_LOAD( "hruii.12g",	 0x0220, 0x0200, NO_DUMP ) // 7621 (82S131)
	ROM_LOAD( "v50.7e",		 0x0420, 0x0200, NO_DUMP ) // 7621 (82S131)

	ROM_REGION( 0x400, "plds", 0 )
	ROM_LOAD( "atthand.11c", 0x0000, 0x0400, NO_DUMP ) // 40033A (?)
	ROM_LOAD( "abcp3-11.1b", 0x0000, 0x0400, NO_DUMP ) // PAL16R4
	ROM_LOAD( "abcp4-11.2d", 0x0000, 0x0400, NO_DUMP ) // PAL16L8
ROM_END

/* System Configuration */

static void abc800_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void abc800_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(abc_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static DEVICE_IMAGE_LOAD( abc800_serial )
{
	/* filename specified */
	if (device_load_serial_device(image)==INIT_PASS)
	{
		/* setup transmit parameters */
		serial_device_setup(image, 9600 >> input_port_read(image->machine, "BAUD"), 8, 1, SERIAL_PARITY_NONE);

		serial_device_set_protocol(image, SERIAL_PROTOCOL_NONE);

		/* and start transmit */
		serial_device_set_transmit_state(image, 1);

		return INIT_PASS;
	}

	return INIT_FAIL;
}

static void abc800_serial_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* serial */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:							info->i = IO_SERIAL; break;
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:							info->start = DEVICE_START_NAME(serial_device); break;
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(abc800_serial); break;
		case MESS_DEVINFO_PTR_UNLOAD:						info->unload = DEVICE_IMAGE_UNLOAD_NAME(serial_device); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "txt"); break;
	}
}

static SYSTEM_CONFIG_START( abc800 )
	CONFIG_RAM_DEFAULT(16 * 1024)
	CONFIG_RAM		  (32 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( abc802 )
	CONFIG_RAM_DEFAULT(64 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( abc806 )
	CONFIG_RAM_DEFAULT(128 * 1024)
	CONFIG_DEVICE(abc800_cassette_getinfo)
	CONFIG_DEVICE(abc800_floppy_getinfo)
	CONFIG_DEVICE(abc800_serial_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static OPBASE_HANDLER( abc800_opbase_handler )
{
	if (address >= 0x7800 && address < 0x8000)
	{
		opbase->rom = opbase->ram = memory_region(machine, "main");
		return ~0;
	}

	return address;
}

static DRIVER_INIT( abc800 )
{
	memory_set_opbase_handler(0, abc800_opbase_handler);
}

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT   INIT    CONFIG  COMPANY             FULLNAME    FLAGS */
COMP( 1981, abc800c,    0,          0,      abc800c,    abc800, abc800, abc800, "Luxor Datorer AB", "ABC 800 C", GAME_NOT_WORKING )
COMP( 1981, abc800m,    abc800c,    0,      abc800m,    abc800, abc800, abc800, "Luxor Datorer AB", "ABC 800 M", GAME_NOT_WORKING )
COMP( 1983, abc802,     0,          0,      abc802,     abc802, abc800, abc802, "Luxor Datorer AB", "ABC 802",  GAME_NOT_WORKING )
COMP( 1983, abc806,     0,          0,      abc806,     abc806, abc800, abc806, "Luxor Datorer AB", "ABC 806",  GAME_NOT_WORKING )
