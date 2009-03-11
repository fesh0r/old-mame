/***************************************************************************
    microbee.c

    system driver
    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

    Brett Selwood, Andrew Davies (technical assistance)

    Microbee Standard / Plus memory map

        0000-7FFF RAM
        8000-BFFF SYSTEM roms
        C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
        E000-EFFF Telcom 1.2 (netrom.ic34; optional)
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics)

    Microbee IC memory map (preliminary)

        0000-7FFF RAM
        8000-BFFF SYSTEM roms (bas522a.rom, bas522b.rom)
        C000-DFFF Edasm or WBee (edasm.rom or wbeee12.rom, optional)
        E000-EFFF Telcom (tecl321.rom; optional)
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Microbee 56KB ROM memory map (preliminary)

        0000-DFFF RAM
        E000-EFFF ROM 56kb.rom CP/M bootstrap loader
        F000-F7FF Video RAM
        F800-FFFF PCG RAM (graphics), Colour RAM (banked)

    Microbee 32 came in three versions:
        IC: features a terminal emulator mapped at $E000 - type NET to run

        PC: features an editor/assembler - type EDASM to run

        PC85: features the WordBee wordprocessor - type EDASM to run
              (maybe the ROM was patched to use another keyword?)


    These early colour computers have a PROM to create the foreground palette.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "machine/z80pio.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "devices/snapquik.h"
#include "devices/basicdsk.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "devices/z80bin.h"

static const device_config *cassette_device_image(running_machine *machine)
{
	return devtag_get_device(machine, "cassette");
}

static READ8_DEVICE_HANDLER(z80pio_alt_r)
{
	int channel = BIT(offset, 1);

	return (offset & 1) ? z80pio_c_r(device, channel) : z80pio_d_r(device, channel);
}

static WRITE8_DEVICE_HANDLER(z80pio_alt_w)
{
	int channel = BIT(offset, 1);

	if (offset & 1)
		z80pio_c_w(device, channel, data);
	else
		z80pio_d_w(device, channel, data);
}

static QUICKLOAD_LOAD( mbee );
static Z80BIN_EXECUTE( mbee );

static size_t mbee_size;

static ADDRESS_MAP_START(mbee_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_SIZE(&mbee_size)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_WRITENOP	/* Needed because quickload to here will crash MESS otherwise */
	AM_RANGE(0x8000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(SMH_BANK2, mbee_videoram_w) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(SMH_BANK3, mbee_pcg_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeic_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x7fff) AM_SIZE(&mbee_size)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(SMH_BANK2, mbee_videoram_w) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(SMH_BANK3, mbee_pcg_color_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbee56_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xdfff) AM_SIZE(&mbee_size)
	AM_RANGE(0x0000, 0x0fff) AM_RAMBANK(1)
	AM_RANGE(0x1000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(SMH_BANK2, mbee_videoram_w) AM_SIZE(&videoram_size)
	AM_RANGE(0xf800, 0xffff) AM_READWRITE(SMH_BANK3, mbee_pcg_color_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(mbee_ports, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START(mbeeic_ports, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00, 0x03) AM_MIRROR(0x10) AM_DEVREADWRITE("z80pio", z80pio_alt_r, z80pio_alt_w)
	AM_RANGE(0x08, 0x08) AM_MIRROR(0x10) AM_READWRITE(mbee_pcg_color_latch_r, mbee_pcg_color_latch_w)
	// AM_RANGE(0x09, 0x09) AM_MIRROR(0x10)  Listed as "Colour Wait Off" or "USART 2651" but doesn't appear in the schematics
	AM_RANGE(0x0a, 0x0a) AM_MIRROR(0x10) AM_READWRITE(mbee_color_bank_r, mbee_color_bank_w)
	AM_RANGE(0x0b, 0x0b) AM_MIRROR(0x10) AM_READWRITE(mbee_video_bank_r, mbee_video_bank_w)
	AM_RANGE(0x0c, 0x0c) AM_MIRROR(0x10) AM_READWRITE(m6545_status_r, m6545_index_w)
	AM_RANGE(0x0d, 0x0d) AM_MIRROR(0x10) AM_READWRITE(m6545_data_r, m6545_data_w)
	AM_RANGE(0x44, 0x44) AM_DEVREADWRITE("wd179x", wd17xx_status_r, wd17xx_command_w)
	AM_RANGE(0x45, 0x45) AM_DEVREADWRITE("wd179x", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0x46, 0x46) AM_DEVREADWRITE("wd179x", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0x47, 0x47) AM_DEVREADWRITE("wd179x", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0x48, 0x48) AM_READWRITE(mbee_fdc_status_r, mbee_fdc_motor_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( mbee )
    PORT_START("LINE0") /* IN0 KEY ROW 0 [000] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("@") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('@') PORT_CHAR('`')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A') PORT_CHAR(0x01)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B') PORT_CHAR(0x02)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C') PORT_CHAR(0x03)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D') PORT_CHAR(0x04)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E') PORT_CHAR(0x05)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F') PORT_CHAR(0x06)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G') PORT_CHAR(0x07)

    PORT_START("LINE1") /* IN1 KEY ROW 1 [080] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H') PORT_CHAR(0x08)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I') PORT_CHAR(0x09)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J') PORT_CHAR(0x0a)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K') PORT_CHAR(0x0b)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L') PORT_CHAR(0x0c)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M') PORT_CHAR(0x0d)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N') PORT_CHAR(0x0e)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O') PORT_CHAR(0x0f)

    PORT_START("LINE2") /* IN2 KEY ROW 2 [100] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P') PORT_CHAR(0x10)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q') PORT_CHAR(0x11)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R') PORT_CHAR(0x12)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S') PORT_CHAR(0x13)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T') PORT_CHAR(0x14)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U') PORT_CHAR(0x15)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V') PORT_CHAR(0x16)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W') PORT_CHAR(0x17)

    PORT_START("LINE3") /* IN3 KEY ROW 3 [180] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X') PORT_CHAR(0x18)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('u') PORT_CHAR('Y') PORT_CHAR(0x19)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z') PORT_CHAR(0x1a)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{') PORT_CHAR(0x1b)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHAR(0x1c)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}') PORT_CHAR(0x1d)
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("^") PORT_CODE(KEYCODE_TILDE) PORT_CHAR('^') PORT_CHAR('~') PORT_CHAR(0x1e)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(8) PORT_CHAR(0x5f) PORT_CHAR(0x1f)	// port_char not working - hijacked

    PORT_START("LINE4") /* IN4 KEY ROW 4 [200] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("6 &") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 '") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')

    PORT_START("LINE5") /* IN5 KEY ROW 5 [280] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("8 (") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("9 )") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')

    PORT_START("LINE6") /* IN6 KEY ROW 6 [300] */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Linefeed") PORT_CODE(KEYCODE_HOME) PORT_CHAR(10)
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
    PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
    PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Break") PORT_CODE(KEYCODE_END) PORT_CHAR(3)
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

    PORT_START("LINE7") /* IN7 KEY ROW 7 [380] */
    PORT_BIT (0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL)
    PORT_BIT (0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT (0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
    PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)

    PORT_START("EXTRA") /* IN8 extra keys */
    PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Up)") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
    PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Down)") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
    PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Left)") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
    PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Right)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
    PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("(Insert)") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
    PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	PORT_BIT( 0x6, 0x6, IPT_UNUSED )
//	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
//	PORT_CONFSETTING(    0x08, DEF_STR(On))
//	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

/* GFX not used by video update - for documentation only */
static const gfx_layout mbee_charlayout =
{
    8,16,                   /* 8 x 16 characters */
    257,                    /* 256 characters + cursor */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    /* y offsets triple height: use each line three times */
    {  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
       8*8,  9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
    8*16                    /* every char takes 16 bytes */
};

static GFXDECODE_START( mbee )
	GFXDECODE_ENTRY( "maincpu", 0x11000, mbee_charlayout, 0, 1 )
GFXDECODE_END

static GFXDECODE_START( mbeeic )
	GFXDECODE_ENTRY( "maincpu", 0x11000, mbee_charlayout, 0, 48 )
GFXDECODE_END

static PALETTE_INIT( mbeeic )
{
	UINT16 i;
	UINT8 r, b, g, k; 
	UINT8 level[] = { 0, 0x80, 0xff, 0xff };	/* off, half, full intensity */

	/* set up background palette (00-63) */
	for (i = 0; i < 64; i++)
	{
		r = level[((i>>0)&1)|((i>>2)&2)];
		g = level[((i>>1)&1)|((i>>3)&2)];
		b = level[((i>>2)&1)|((i>>4)&2)];
		palette_set_color(machine, i, MAKE_RGB(r, g, b));
	}

	/* set up foreground palette (64-95) by reading the prom */
	for (i = 0; i < 32; i++)
	{
		k = color_prom[i];
		r = level[((k>>2)&1)|((k>>4)&2)];
		g = level[((k>>1)&1)|((k>>3)&2)];
		b = level[((k>>0)&1)|((k>>2)&2)];
		palette_set_color(machine, i|64, MAKE_RGB(r, g, b));
	}
}

static int mbee_vsync;

static void mbee_pio_interrupt(const device_config *device, int state)
{
	cpu_set_input_line(device->machine->cpu[0], 0, state);
}

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	/* PIO B data bits
	 * 0	cassette data (input)
	 * 1	cassette data (output)
	 * 2	rs232 clock or DTR line
	 * 3	rs232 CTS line (0: clear to send)
	 * 4	rs232 input (0: mark)
	 * 5	rs232 output (1: mark)
	 * 6	speaker
	 * 7	network interrupt
	 */

	UINT8 data = 0;

	if (cassette_input(cassette_device_image(device->machine)) > 0.03)
		data |= 0x01;

	data |= mbee_vsync << 7;

	if (mbee_vsync) mbee_vsync = 0;

	return data;
};

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
	/* PIO B data bits
	 * 0	cassette data (input)
	 * 1	cassette data (output)
	 * 2	rs232 clock or DTR line
	 * 3	rs232 CTS line (0: clear to send)
	 * 4	rs232 input (0: mark)
	 * 5	rs232 output (1: mark)
	 * 6	speaker
	 * 7	network interrupt
	 */
	const device_config *speaker = devtag_get_device(device->machine, "speaker");

	cassette_output(cassette_device_image(device->machine), (data & 0x02) ? -1.0 : +1.0);

	speaker_level_w(speaker, (data & 0x40) ? 1 : 0);
};

static const z80pio_interface mbee_z80pio_intf =
{
	mbee_pio_interrupt,	/* callback when change interrupt status */
	NULL,
	pio_port_b_r,
	NULL,
	pio_port_b_w,
	NULL,
	NULL
};

static const z80_daisy_chain mbee_daisy_chain[] =
{
	{ "z80pio" },
	{ NULL }
};

static INTERRUPT_GEN( mbee_interrupt )
{
	/* once per frame, pulse the PIO B bit 7 - only needed for networked bees */
	mbee_vsync = 1;
}

static MACHINE_DRIVER_START( mbee_cartslot )
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(mbee_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mbee )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_12MHz / 6)         /* 2 MHz */
	MDRV_CPU_PROGRAM_MAP(mbee_mem, 0)
	MDRV_CPU_IO_MAP(mbee_ports, 0)
	MDRV_CPU_CONFIG(mbee_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", mbee_interrupt)

	MDRV_MACHINE_RESET( mbee )

	MDRV_Z80PIO_ADD( "z80pio", mbee_z80pio_intf )

	MDRV_GFXDECODE(mbee)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(250)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*8, 19*16)			/* need at least 17 lines for NET */
	MDRV_SCREEN_VISIBLE_AREA(0*8, 64*8-1, 0, 19*16-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(mbee)
	MDRV_VIDEO_UPDATE(mbee)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", mbee, "mwb,com", 2)
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload2", mbee, 2)

	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
	
	/* cartridge */
	MDRV_IMPORT_FROM(mbee_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbeeic )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3375000)         /* 3.37500 MHz */
	MDRV_CPU_PROGRAM_MAP(mbeeic_mem, 0)
	MDRV_CPU_IO_MAP(mbeeic_ports, 0)
	MDRV_CPU_CONFIG(mbee_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", mbee_interrupt)

	MDRV_MACHINE_RESET( mbee )

	MDRV_Z80PIO_ADD( "z80pio", mbee_z80pio_intf )

	MDRV_GFXDECODE(mbeeic)
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(250)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(70*8, 310)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 70*8-1, 0, 19*16-1)
	MDRV_PALETTE_LENGTH(96)
	MDRV_PALETTE_INIT(mbeeic)

	MDRV_VIDEO_START(mbeeic)
	MDRV_VIDEO_UPDATE(mbeeic)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_QUICKLOAD_ADD("quickload", mbee, "mwb,com", 2)
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload2", mbee, 2)

	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
	
	MDRV_WD179X_ADD("wd179x", mbee_wd17xx_interface )

	/* cartridge */
	MDRV_IMPORT_FROM(mbee_cartslot)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mbee56 )
	MDRV_IMPORT_FROM( mbeeic )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP(mbee56_mem, 0)
MACHINE_DRIVER_END

static DRIVER_INIT( mbee )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0x8000);
	memory_configure_bank(machine, 2, 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, 3, 0, 2, &RAM[0x11800], 0x4000);
	memory_set_bank(machine, 2, 1);
	memory_set_bank(machine, 3, 0);
}

static DRIVER_INIT( mbee56 )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0xe000);
	memory_configure_bank(machine, 2, 0, 2, &RAM[0x11000], 0x4000);
	memory_configure_bank(machine, 3, 0, 2, &RAM[0x11800], 0x4000);
	memory_set_bank(machine, 2, 1);
	memory_set_bank(machine, 3, 0);
}

ROM_START( mbee )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("bas510a.ic25",  0x8000, 0x1000, CRC(2ca47c36) SHA1(f36fd0afb3f1df26edc67919e78000b762b6cbcb) )
	ROM_LOAD("bas510b.ic27",  0x9000, 0x1000, CRC(a07a0c51) SHA1(dcbdd9df78b4b6b2972de2e4050dabb8ae9c3f5a) )
	ROM_LOAD("bas510c.ic28",  0xa000, 0x1000, CRC(906ac00f) SHA1(9b46458e5755e2c16cdb191a6a70df6de9fe0271) )
	ROM_LOAD("bas510d.ic30",  0xb000, 0x1000, CRC(61727323) SHA1(c0fea9fd0e25beb9faa7424db8efd07cf8d26c1b) )
	ROM_LOAD("edasma.ic31",   0xc000, 0x1000, CRC(120c3dea) SHA1(32c9bb6e54dd50d5218bb43cc921885a0307161d) )
	ROM_LOAD("edasmb.ic33",   0xd000, 0x1000, CRC(a23bf3c8) SHA1(73a57c2800a1c744b527d0440b170b8b03351753) )
	ROM_LOAD("telcom11.rom",  0xe000, 0x1000, CRC(15516499) SHA1(2d4953f994b66c5d3b1d457b8c92d9a0a69eb8b8) )
	ROM_LOAD("charrom.ic13",  0x11000, 0x0800, CRC(b149737b) SHA1(a3cd4f5d0d3c71137cd1f0f650db83333a2e3597) )
	ROM_RELOAD( 0x17000, 0x0800 )
	ROM_RELOAD( 0x17800, 0x0800 )

	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0000, 0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeeic )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("bas522a.rom",   0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
	ROM_LOAD("bas522b.rom",   0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
	ROM_LOAD("edasm.rom",     0xc000, 0x2000, CRC(1af1b3a9) SHA1(d035a997c2dbbb3918b3395a3a5a1076aa203ee5))
	ROM_LOAD("telcom12.rom",  0xe000, 0x1000, CRC(0231bda3) SHA1(be7b32499034f985cc8f7865f2bc2b78c485585c) )
	ROM_LOAD("charrom.bin",   0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",   0x0000, 0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020, 0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeepc85 )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("bas522a.rom",   0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
	ROM_LOAD("bas522b.rom",   0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
	ROM_LOAD("wbee12.rom",    0xc000, 0x2000, CRC(0fc21cb5) SHA1(33b3995988fc51ddef1568e160dfe699867adbd5))
	ROM_LOAD("charrom.bin",   0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",   0x0000, 0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020, 0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbeepc )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("bas522a.rom",   0x8000, 0x2000, CRC(7896a696) SHA1(a158f7803296766160e1f258dfc46134735a9477))
	ROM_LOAD("bas522b.rom",   0xa000, 0x2000, CRC(b21d9679) SHA1(332844433763331e9483409cd7da3f90ac58259d))
	/* This telcom rom is banked between its 2 halves, hooked to port 0A -  not emulated yet */
	ROM_LOAD("telc321.rom",   0xe000, 0x2000, CRC(b07eefaa) SHA1(5dab90b2c232673282d215845c9947cc5bdd50c8))
	ROM_LOAD("charrom.bin",   0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",   0x0000, 0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020, 0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

ROM_START( mbee56 )
	ROM_REGION(0x18000,"maincpu",0)
	ROM_LOAD("56kb.rom",      0xe000, 0x1000, CRC(28211224) SHA1(b6056339402a6b2677b0e6c57bd9b78a62d20e4f))
	ROM_LOAD("charrom.bin",   0x11000, 0x1000, CRC(1f9fcee4) SHA1(e57ac94e03638075dde68a0a8c834a4f84ba47b0))
	ROM_RELOAD( 0x17000, 0x1000 )

	ROM_REGION( 0x0040, "proms", 0 )
	ROM_LOAD( "82s123.ic7",   0x0000, 0x0020, CRC(61b9c16c) SHA1(0ee72377831c21339360c376f7248861d476dc20) )
	ROM_LOAD_OPTIONAL( "82s123.ic16", 0x0020, 0x0020, CRC(4e779985) SHA1(cd2579cf65032c30b3fe7d6d07b89d4633687481) )	/* video switching prom, not needed for emulation purposes */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

static Z80BIN_EXECUTE( mbee )
{
	const device_config *cpu = cputag_get_cpu(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_write_word_16le(space, 0xa6, execute_address);			/* fix the EXEC command */

	if (autorun)
	{
		memory_write_word_16le(space, 0xa2, execute_address);		/* fix warm-start vector to get around some copy-protections */
		cpu_set_reg(cpu, REG_GENPC, execute_address);
	}
	else
	{
		memory_write_word_16le(space, 0xa2, 0x8517);
	}
}

static QUICKLOAD_LOAD( mbee )
{
	const device_config *cpu = cputag_get_cpu(image->machine, "maincpu");
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 i, j;
	UINT8 data, sw = input_port_read(image->machine, "CONFIG") & 1;	/* reading the dipswitch: 1 = autorun */

	if (!mame_stricmp(image_filetype(image), "mwb"))
	{
		/* mwb files - standard basic files */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x8c0 + i;

			if (image_fread(image, &data, 1) != 1) return INIT_FAIL;

			if ((j < mbee_size) || (j > 0xefff))
				memory_write_byte(space, j, data);
			else
				return INIT_FAIL;
		}

		if (sw)
		{
			memory_write_word_16le(space, 0xa2,0x801e);	/* fix warm-start vector to get around some copy-protections */
			cpu_set_reg(cpu, REG_GENPC, 0x801e);
		}
		else
			memory_write_word_16le(space, 0xa2,0x8517);
	}
	else if (!mame_stricmp(image_filetype(image), "com"))
	{
		/* com files - most com files are just machine-language games with a wrapper and don't need cp/m to be present */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x100 + i;

			if (image_fread(image, &data, 1) != 1) return INIT_FAIL;

			if ((j < mbee_size) || (j > 0xefff))
				memory_write_byte(space, j, data);
			else
				return INIT_FAIL;
		}

		if (sw) cpu_set_reg(cpu, REG_GENPC, 0x100);
	}

	return INIT_PASS;
}

static void mbee_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:		info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:		info->load = DEVICE_IMAGE_LOAD_NAME(basicdsk_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy(info->s = device_temp_str(), "dsk"); break;

		default:				legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}



static SYSTEM_CONFIG_START(mbeeic)
	CONFIG_DEVICE(mbee_floppy_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG    COMPANY   FULLNAME */
COMP( 1982, mbee,     0,	0,	mbee,     mbee,     mbee,     0,			"Applied Technology",  "Microbee 16 Standard" , 0)
COMP( 1982, mbeeic,   mbee,	0,	mbeeic,   mbee,     mbee,     mbeeic,		"Applied Technology",  "Microbee 32 IC" , 0)
COMP( 1982, mbeepc,   mbee,	0,	mbeeic,   mbee,     mbee,     mbeeic,		"Applied Technology",  "Microbee 32 PC" , 0)
COMP( 1985?,mbeepc85, mbee,	0,	mbeeic,   mbee,     mbee,     mbeeic,		"Applied Technology",  "Microbee 32 PC85" , 0)
COMP( 1983, mbee56,   mbee,	0,	mbee56,   mbee,     mbee56,   mbeeic,		"Applied Technology",  "Microbee 56" , 0)

