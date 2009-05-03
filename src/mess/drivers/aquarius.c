/*

	TODO:

	- vsync
	- hand controllers
	- scramble RAM also
	- CAQ tape support
	- memory mapper
	- proper video timings
	- PAL mode

	- floppy support
	- modem
	- "old" version of BASIC ROM
	- Aquarius II

*/

#include "driver.h"
#include "includes/aquarius.h"
#include "cpu/z80/z80.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "sound/ay8910.h"
#include "sound/speaker.h"

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}

/* Read/Write Handlers */

static UINT8 *decrypt_rom;
static UINT8 scrambler;

static READ8_HANDLER( cassette_r )
{
	/*

		To read stored data from cassette the program should look at bit zero of 
		the cassette input port and measure the time difference between leading 
		edges or trailing edges. This is to prevent DC level shifting from altering 
		pulse width of data. The program should then look for sync bytes for data 
		synchronisation before data block transfer. If there is any task that must
		be performed during cassette loading, the maximum allowable time to do the 
		job after one byte from cassette, must be less than 80% of the period of a 
		mark cycle. Control must be returned at that time to the cassette routine 
		in order to maintain data integrity.
	
	*/

	return (cassette_input(cassette_device_image(space->machine)) < +0.0) ? 0 : 1;
}

static WRITE8_HANDLER( cassette_w )
{
	/*

		Sound and cassette port use a common pin. Therefore the signal to cassette
		will appear on audio output. Sound port is a simple one bit I/O and therefore
		it must be toggled at a specific rate under software control.

	*/

	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	speaker_level_w(speaker, data & 0x01);

	cassette_output(cassette_device_image(space->machine), (data & 0x01) ? +1.0 : -1.0);
}

static READ8_HANDLER( vsync_r )
{
	/*

		The current state of the vertical sync will appear on bit 0 during a read of 
		this port. The waveform and timing spec is shown as follows:

			|<-    Active scan period   ->|V.sync |<-
			|      12.8 ms                |3.6 ms (PAL)
			|                             |2.8 ms (NTSC)
			+++++++++++++++++++++++++++++++       ++++++++++++
			+                             +       +
			+                             +       +
		+++++                             +++++++++

	*/

	return 0;
}

static WRITE8_HANDLER( mapper_w )
{
	/*

		Bit D0 of this port controls the swapping of the lower 16K block in the memory 
		map with the upper 16K. A 1 in this bit indicates swapping. This bit is reset 
		after power up initialization.

	*/
}

static READ8_HANDLER( printer_r )
{
	/*

		Printer handshaking port (read) Port 0xFE when read, presents the clear 
		to send status from PRNHASK pin at bit D0. A 1 indicates printer is ready, 
		0 means not ready.

	*/

	return 1; // ready
}

static WRITE8_HANDLER( printer_w )
{
	/*

		This is a single bit I/O at D0, it will perform as a serial output 
		port under software control. Since timing is done by software the 
		baudrate is variable. In BASIC this is a 1200 baud printer port for 
		the 40 column thermal printer.

	*/
}

static READ8_HANDLER( keyboard_r )
{
	/*

		This port is 6 bits wide, when read, it returns the row data from the 
		keyboard matrix. The keyboard is usually scanned in the following manner:

		The keyboard is a 6 row by 8 column matrix. The column is connected to
		the higher order address bus A15-A8. When Z80 executes its input 
		instruction sets, either the current content of the accumulator (A) or 
		the content of register (B) will go to the higher order address bus. 
		Therefore the keyboard can be scanned by placing a specific scanning 
		pattern in (A) or (B) and reading the result returned on rows. 

	*/

	UINT8 keylatch = offset >> 8;
	int row;
	static const char *const rownames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", "ROW5", "ROW6", "ROW7" };

	if (keylatch == 0) return 0;

	for (row = 0; row < 8; row++)
	{
		if (!BIT(keylatch, row))
			return input_port_read(space->machine, rownames[row]);
	}

	return 0xff;
}

static WRITE8_HANDLER( scrambler_w )
{
	/*

		Software lock: Writing this port with an 8 bit value will set the software 
		scrambler pattern. The data that appears on the output side will be the 
		result of the input bus EX-ORed with this pattern, bit by bit. The software 
		lock is a scrambler built between the CPU and external interface. The 
		scrambling pattern is contained in port 0xFF and is not readable. CPU data 
		output to external bus will be XORed with this pattern in a bit by bit 
		fashion to generate the real data on external bus. By the same mechanism, 
		data from external bus is also XORed with this pattern and read by CPU. 

		Therefore it the external device is RAM, the software lock simply has no 
		effect as long as the scrambling pattern remains unchanged. For I/O 
		operation the pattern is gated to 0 and thus scrambling action is nullified.

		In BASIC operation the scrambling pattern is generated by a random number 
		routine. For game cartridge the lock pattern is generated from data in the
		game cartridge itself.

	*/

	UINT8 *ROM = memory_region(space->machine, "maincpu") + 0xc000;
	UINT16 addr;

	scrambler = data;

	for (addr = 0; addr < 16*1024; addr++)
	{
		decrypt_rom[addr] = ROM[addr] ^ scrambler;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( aquarius_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x3000, 0x33ff) AM_RAM_WRITE(aquarius_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x3400, 0x37ff) AM_RAM_WRITE(aquarius_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0x3800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0xbfff) AM_RAMBANK(1)
	AM_RANGE(0xc000, 0xffff) AM_ROMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( aquarius_io, ADDRESS_SPACE_IO, 8)
//	AM_RANGE(0x7e, 0x7f) AM_MIRROR(0xff00) AM_READWRITE(modem_r, modem_w)
//	AM_RANGE(0xe0, 0xef) AM_MIRROR(0xff00) AM_READWRITE(floppy_r, floppy_w)
	AM_RANGE(0xf6, 0xf6) AM_MIRROR(0xff00) AM_DEVREADWRITE("ay8910", ay8910_r, ay8910_data_w)
	AM_RANGE(0xf7, 0xf7) AM_MIRROR(0xff00) AM_DEVWRITE("ay8910", ay8910_address_w)
	AM_RANGE(0xfc, 0xfc) AM_MIRROR(0xff00) AM_READWRITE(cassette_r, cassette_w)
	AM_RANGE(0xfd, 0xfd) AM_MIRROR(0xff00) AM_READWRITE(vsync_r, mapper_w)
	AM_RANGE(0xfe, 0xfe) AM_MIRROR(0xff00) AM_READWRITE(printer_r, printer_w)
	AM_RANGE(0xff, 0xff) AM_MIRROR(0xff00) AM_MASK(0xff00) AM_READWRITE(keyboard_r, scrambler_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( aquarius_reset )
{
	cputag_set_input_line(field->port->machine, "maincpu", INPUT_LINE_RESET, (input_port_read(field->port->machine, "RESET") ? CLEAR_LINE : ASSERT_LINE));
}

static INPUT_PORTS_START(aquarius)
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("= +\tNEXT") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90 \\") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8) PORT_CHAR('\\')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(": *\tPEEK") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RTN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("; @\tPOKE") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('@')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". >\tVAL") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("- _\tFOR") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("/ ^") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('/') PORT_CHAR('^')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 ?") PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR('?')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR(16)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("L\tPOINT") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR(12)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(", <\tSTR$") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9 )\tCOPY") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(15)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("K\tPRESET") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR(11)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("N\tRIGHT$") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(14)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("J\tPSET") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR(10)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 (\tRETURN") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(9)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 '\tGOSUB") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(21)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR(8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("B\tMID$") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR(2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 &\tON") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y') PORT_CHAR(25)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("G\tBELL") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR(7)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("V\tLEFT$") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR(22)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("C\tSTOP") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F\tDATA") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR(6)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 %\tGOTO") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("T\tINPUT") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(20)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 $\tTHEN") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("R\tRETYP") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR(18)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("D\tREAD") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(4)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("X\tDELINE") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(24)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3 #\tIF") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("E\tDIM") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(5)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("S\tSTPLST") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(19)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("Z\tCLOAD") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(26)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE\tCHR$") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(32)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("A\tCSAVE") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR(1)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2 \"\tLIST") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("W\tREM") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR(23)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 !\tRUN") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR(17)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RST") PORT_CODE(KEYCODE_F10) PORT_CHANGED(aquarius_reset, 0)

	PORT_START("LEFT")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START("RIGHT")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* Character Layout */

static const gfx_layout aquarius_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8,
	  4*8, 5*8, 6*8, 7*8, },
	8 * 8
};

/* Graphics Decode Information */

static GFXDECODE_START( aquarius )
	GFXDECODE_ENTRY( "gfx1", 0x0000, aquarius_charlayout, 0, 256 )
GFXDECODE_END

/* Interrupt Generator */

static INTERRUPT_GEN( aquarius_interrupt )
{
	cpu_set_input_line(device, INPUT_LINE_IRQ0, HOLD_LINE);
}

/* Sound Interface */

static const ay8910_interface aquarius_ay8910_interface =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_INPUT_PORT("RIGHT"),
	DEVCB_INPUT_PORT("LEFT"),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( aquarius )
{
	decrypt_rom = auto_malloc(16*1024);
}

static MACHINE_RESET( aquarius )
{
	/* RAM expansion */

	switch (mess_ram_size)
	{
	case 4 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 8 * 1024: // 4K expansion
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0x4fff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x5000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 20 * 1024: // 16K expansion
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0x8fff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x9000, 0xbfff, 0, 0, SMH_UNMAP, SMH_UNMAP);
		break;

	case 36 * 1024: // 32K expansion (prototype)
		memory_install_readwrite8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x4000, 0xbfff, 0, 0, SMH_BANK1, SMH_BANK1);
		break;
	}

	memory_configure_bank(machine, 1, 0, 1, mess_ram, 0);
	memory_set_bank(machine, 1, 0);

	/* cartridge */

	memory_configure_bank(machine, 2, 0, 1, decrypt_rom, 0);
	memory_set_bank(machine, 2, 0);
}

static DEVICE_IMAGE_LOAD( aquarius_cart )
{
	int size = image_length(image);
	UINT8 *ptr = memory_region(image->machine, "maincpu") + 0xc000;

	if (image_fread(image, ptr, size) != size)
	{
		return INIT_FAIL;
	}

	switch (size)
	{
	case 8 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xdfff, 0, 0x2000, SMH_BANK2, SMH_UNMAP);
		break;

	case 16 * 1024:
		memory_install_readwrite8_handler(cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xc000, 0xffff, 0, 0, SMH_BANK2, SMH_UNMAP);
		break;
	}

	return INIT_PASS;
}

/* Machine Driver */
static const cassette_config aquarius_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED
};

static MACHINE_DRIVER_START( aquarius )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_3_579545MHz) // ???
	MDRV_CPU_PROGRAM_MAP(aquarius_mem, 0)
	MDRV_CPU_IO_MAP(aquarius_io, 0)
	MDRV_CPU_VBLANK_INT("screen", aquarius_interrupt)

	MDRV_MACHINE_START( aquarius )
	MDRV_MACHINE_RESET( aquarius )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40 * 8, 25 * 8)
	MDRV_SCREEN_VISIBLE_AREA(0, 40 * 8 - 1, 0 * 8, 25 * 8 - 1)
	MDRV_GFXDECODE( aquarius )
	MDRV_PALETTE_LENGTH(512)
	MDRV_PALETTE_INIT( aquarius )

	MDRV_VIDEO_START( aquarius )
	MDRV_VIDEO_UPDATE( aquarius )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_SOUND_ADD("ay8910", AY8910, XTAL_3_579545MHz/2) // ??? AY-3-8914
	MDRV_SOUND_CONFIG(aquarius_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* printer */
	MDRV_PRINTER_ADD("printer")

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", aquarius_cassette_config )

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(aquarius_cart)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( aquarius )
	ROM_REGION( 0x10000, "maincpu", 0 )
//	ROM_LOAD( "aquarius.u2", 0x0000, 0x2000, CRC(5cfa5b42) SHA1(02c8ee11e911d1aa346812492d14284b6870cb3e) )
	ROM_LOAD( "aq2.u2", 0x0000, 0x2000, CRC(a2d15bcf) SHA1(ca6ef55e9ead41453efbf5062d6a60285e9661a6) )
	
	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "aq2.u5", 0x0000, 0x0800, BAD_DUMP CRC(0b3edeed) SHA1(d2509839386b852caddcaa89cd376be647ba1492) )
ROM_END

/* System Configuration */

static DEVICE_IMAGE_LOAD( aquarius_floppy )
{
	// 128K images, 64K/side
	return INIT_FAIL;
}

static void aquarius_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(aquarius_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( aquarius )
	CONFIG_RAM_DEFAULT( 4 * 1024)
	CONFIG_RAM		  ( 8 * 1024)
	CONFIG_RAM		  (20 * 1024)
	CONFIG_RAM		  (36 * 1024)
	CONFIG_DEVICE(aquarius_floppy_getinfo)
SYSTEM_CONFIG_END

/*      YEAR    NAME        PARENT		COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME			FLAGS */
COMP(	1983,	aquarius,	0,			0,		aquarius,	aquarius,	0,		aquarius,	"Mattel",	"Aquarius (NTSC)",	0 )
//COMP(	1984,	aquariu2,	aquarius,	0,		aquarius,	aquarius,	0,		aquarius,	"Mattel",	"Aquarius II",	GAME_NOT_WORKING )
