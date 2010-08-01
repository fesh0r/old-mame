/***************************************************************************

    Canon Cat driver by Miodrag Milanovic

    12/06/2009 Skeleton driver.
    15/06/2009 Working driver

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/68681.h"

class cat_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, cat_state(machine)); }

	cat_state(running_machine &machine) { }

	UINT16 *video_ram;

	UINT8 duart_inp;// = 0x0e;

	UINT16 *sram;
	UINT8 video_enable;
	UINT16 pr_cont;
	UINT8 keyboard_line;
	emu_timer *keyboard_timer;
};

static WRITE16_HANDLER( cat_video_status_w )
{
	cat_state *state = (cat_state *)space->machine->driver_data;

	state->video_enable = BIT( data, 3 );
}

static WRITE16_HANDLER( cat_test_mode_w )
{
}

static READ16_HANDLER( cat_modem_r )
{
	return 0;
}

static WRITE16_HANDLER( cat_modem_w )
{
}

static READ16_HANDLER( cat_battery_r )
{
	/* just return that battery is full */
	return 0x7fff;
}
static WRITE16_HANDLER( cat_printer_w )
{
	cat_state *state = (cat_state *)space->machine->driver_data;

	state->pr_cont = data;
}
static READ16_HANDLER( cat_floppy_r )
{
	return 0x00;
}
static WRITE16_HANDLER( cat_floppy_w )
{
}

static READ16_HANDLER( cat_keyboard_r )
{
	cat_state *state = (cat_state *)space->machine->driver_data;
	UINT16 retVal = 0;
	// Read country code
	if (state->pr_cont == 0x0900)
	{
		retVal = input_port_read(space->machine, "DIPSW1");
	}
	// Regular keyboard read
	if (state->pr_cont == 0x0800 || state->pr_cont == 0x0a00)
	{
		retVal=0xff00;
		switch(state->keyboard_line)
		{
			case 0x01: retVal = input_port_read(space->machine, "LINE0") << 8; break;
			case 0x02: retVal = input_port_read(space->machine, "LINE1") << 8; break;
			case 0x04: retVal = input_port_read(space->machine, "LINE2") << 8; break;
			case 0x08: retVal = input_port_read(space->machine, "LINE3") << 8; break;
			case 0x10: retVal = input_port_read(space->machine, "LINE4") << 8; break;
			case 0x20: retVal = input_port_read(space->machine, "LINE5") << 8; break;
			case 0x40: retVal = input_port_read(space->machine, "LINE6") << 8; break;
			case 0x80: retVal = input_port_read(space->machine, "LINE7") << 8; break;
		}
	}
	return retVal;
}
static WRITE16_HANDLER( cat_keyboard_w )
{
	cat_state *state = (cat_state *)space->machine->driver_data;

	state->keyboard_line = data >> 8;
}

static WRITE16_HANDLER( cat_video_w )
{
/*
 006500AE ,          ( HSS HSync Strart    89 )
 006480C2 ,          ( HST End HSync   96 )
 006400CE ,          ( HSE End H Line    104 )
 006180B0 ,          ( VDE Active Lines    344 )
 006100D4 ,          ( VSS VSync Start   362 )
 006080F4 ,          ( VST End of VSync    378 )
 00600120 ,          ( VSE End of Frame    400 )
 006581C0 ,          ( VOC Video Control Normal Syncs )
 */
}
static READ16_HANDLER( cat_something_r )
{
	return 0x00ff;
}

static ADDRESS_MAP_START(cat_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0003ffff) AM_ROM // 256 KB ROM
	AM_RANGE(0x00040000, 0x00043fff) AM_RAM AM_BASE_MEMBER(cat_state,sram) // SRAM powered by batery
	AM_RANGE(0x00200000, 0x0027ffff) AM_ROM AM_REGION("svrom",0x0000) // SV ROM
	AM_RANGE(0x00400000, 0x0047ffff) AM_RAM AM_BASE_MEMBER(cat_state,video_ram) // 512 KB RAM
	AM_RANGE(0x00600000, 0x0065ffff) AM_WRITE(cat_video_w) // Video chip
	AM_RANGE(0x00800000, 0x00800001) AM_READWRITE(cat_floppy_r, cat_floppy_w)
	AM_RANGE(0x00800002, 0x00800003) AM_WRITE(cat_keyboard_w)
	AM_RANGE(0x00800008, 0x00800009) AM_READ(cat_something_r)
	AM_RANGE(0x0080000a, 0x0080000b) AM_READ(cat_keyboard_r)
	AM_RANGE(0x0080000e, 0x0080000f) AM_READWRITE(cat_battery_r,cat_printer_w)
	AM_RANGE(0x00810000, 0x0081001f) AM_DEVREADWRITE8( "duart68681", duart68681_r, duart68681_w, 0xff )
	AM_RANGE(0x00820000, 0x008200ff) AM_READWRITE(cat_modem_r, cat_modem_w)// modem
	AM_RANGE(0x00840000, 0x00840001) AM_WRITE(cat_video_status_w) // Video status
	AM_RANGE(0x00860000, 0x00860001) AM_WRITE(cat_test_mode_w) // Test mode
ADDRESS_MAP_END

static ADDRESS_MAP_START(swyft_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM // 64 KB ROM
	AM_RANGE(0x00040000, 0x000fffff) AM_RAM AM_BASE_MEMBER(cat_state,video_ram)
ADDRESS_MAP_END

/* Input ports */

/* 2009-07 FP
   FIXME: Natural keyboard does not catch all the Shifted chars. No idea of the reason!  */
static INPUT_PORTS_START( cat )
	PORT_START("DIPSW1")
	PORT_DIPNAME( 0x8000, 0x8000, "Mode" )
	PORT_DIPSETTING(	0x8000, DEF_STR( Normal ) )
	PORT_DIPSETTING(	0x0000, "Diagnostic" )
	PORT_DIPNAME( 0x7f00,0x7f00, "Country code" )
	PORT_DIPSETTING(	0x7f00, "United States" )
	PORT_DIPSETTING(	0x7e00, "Canada" )
	PORT_DIPSETTING(	0x7d00, "United Kingdom" )
	PORT_DIPSETTING(	0x7c00, "Norway" )
	PORT_DIPSETTING(	0x7b00, "France" )
	PORT_DIPSETTING(	0x7a00, "Denmark" )
	PORT_DIPSETTING(	0x7900, "Sweden" )
	PORT_DIPSETTING(	0x7800, DEF_STR(Japan) )
	PORT_DIPSETTING(	0x7700, "West Germany" )
	PORT_DIPSETTING(	0x7600, "Netherlands" )
	PORT_DIPSETTING(	0x7500, "Spain" )
	PORT_DIPSETTING(	0x7400, "Italy" )
	PORT_DIPSETTING(	0x7300, "Latin America" )
	PORT_DIPSETTING(	0x7200, "South Africa" )
	PORT_DIPSETTING(	0x7100, "Switzerland" )
	PORT_DIPSETTING(	0x7000, "ASCII" )

	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('\xa2')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')

	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('n') PORT_CHAR('B')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')

	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')

	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left USE FRONT") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right USE FRONT") PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_F12) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')

	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('\xbd') PORT_CHAR('\xbc')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')

	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left LEAP") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('[')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB) PORT_CHAR('\t')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Leap") PORT_CODE(KEYCODE_RALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Page") PORT_CODE(KEYCODE_PGUP) PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("UNDO") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('\xb1') PORT_CHAR('\xb0')
INPUT_PORTS_END

static INPUT_PORTS_START( swyft )
INPUT_PORTS_END


static TIMER_CALLBACK(keyboard_callback)
{
	cputag_set_input_line(machine, "maincpu", M68K_IRQ_1, ASSERT_LINE);
}

static IRQ_CALLBACK(cat_int_ack)
{
	cputag_set_input_line(device->machine, "maincpu",M68K_IRQ_1,CLEAR_LINE);
	return M68K_INT_ACK_AUTOVECTOR;
}

static MACHINE_START(cat)
{
	cat_state *state = (cat_state *)machine->driver_data;

	state->duart_inp = 0x0e;
	state->keyboard_timer = timer_alloc(machine, keyboard_callback, NULL);
}

static MACHINE_RESET(cat)
{
	cat_state *state = (cat_state *)machine->driver_data;
	cpu_set_irq_callback(machine->device("maincpu"), cat_int_ack);
	timer_adjust_periodic(state->keyboard_timer, attotime_zero, 0, ATTOTIME_IN_HZ(120));
}

static VIDEO_START( cat )
{
}

static VIDEO_UPDATE( cat )
{
	cat_state *state = (cat_state *)screen->machine->driver_data;
	UINT16 code;
	int y, x, b;

	int addr = 0;
	if (state->video_enable == 1)
	{
		for (y = 0; y < 344; y++)
		{
			int horpos = 0;
			for (x = 0; x < 42; x++)
			{
				code = state->video_ram[addr++];
				for (b = 15; b >= 0; b--)
				{
					*BITMAP_ADDR16(bitmap, y, horpos++) = (code >> b) & 0x01;
				}
			}
		}
	} else {
		rectangle black_area = {0, 672 - 1, 0, 344 - 1};
		bitmap_fill(bitmap, &black_area, 0);
	}
	return 0;
}

static TIMER_CALLBACK( swyft_reset )
{
	memset(memory_get_read_ptr(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0xe2341), 0xff, 1);
}

static MACHINE_START(swyft)
{
}

static MACHINE_RESET(swyft)
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, swyft_reset);
}

static VIDEO_START( swyft )
{
}

static VIDEO_UPDATE( swyft )
{
	cat_state *state = (cat_state *)screen->machine->driver_data;
	UINT16 code;
	int y, x, b;

	int addr = 0;
	for (y = 0; y < 242; y++)
	{
		int horpos = 0;
		for (x = 0; x < 20; x++)
		{
			code = state->video_ram[addr++];
			for (b = 15; b >= 0; b--)
			{
				*BITMAP_ADDR16(bitmap, y, horpos++) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}

static void duart_irq_handler(running_device *device, UINT8 vector)
{
	logerror("duart_irq_handler\n");
}

static void duart_tx(running_device *device, int channel, UINT8 data)
{
}

static UINT8 duart_input(running_device *device)
{
	cat_state *state = (cat_state *)device->machine->driver_data;

	if (state->duart_inp != 0)
	{
		state->duart_inp = 0;
		return 0x0e;
	}
	else
	{
		state->duart_inp = 0x0e;
		return 0x00;
	}
}

static const duart68681_config cat_duart68681_config =
{
	duart_irq_handler,
	duart_tx,
	duart_input,
	NULL
};

static NVRAM_HANDLER( cat )
{
	cat_state *state = (cat_state *)machine->driver_data;

	if (read_or_write)
	{
		mame_fwrite(file, state->sram, 0x4000);
	}
	else
	{
		if (file)
		{
			mame_fread(file, state->sram, 0x4000);
		}
	}
}

static MACHINE_DRIVER_START( cat )

	MDRV_DRIVER_DATA( cat_state )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M68000, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(cat_mem)

	MDRV_MACHINE_START(cat)
	MDRV_MACHINE_RESET(cat)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(672, 344)
	MDRV_SCREEN_VISIBLE_AREA(0, 672-1, 0, 344-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(cat)
	MDRV_VIDEO_UPDATE(cat)

	MDRV_DUART68681_ADD( "duart68681", XTAL_5MHz, cat_duart68681_config )

	MDRV_NVRAM_HANDLER( cat )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( swyft )

	MDRV_DRIVER_DATA( cat_state )

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",M68000, XTAL_5MHz)
	MDRV_CPU_PROGRAM_MAP(swyft_mem)

	MDRV_MACHINE_START(swyft)
	MDRV_MACHINE_RESET(swyft)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(320, 242)
	MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 242-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(swyft)
	MDRV_VIDEO_UPDATE(swyft)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( swyft )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "infoapp.lo", 0x0000, 0x8000, CRC(52c1bd66) SHA1(b3266d72970f9d64d94d405965b694f5dcb23bca) )
	ROM_LOAD( "infoapp.hi", 0x8000, 0x8000, CRC(83505015) SHA1(693c914819dd171114a8c408f399b56b470f6be0) )
ROM_END

ROM_START( cat )
	ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	// SYS ROM
	ROM_LOAD16_BYTE( "r240l0.bin", 0x00001, 0x10000, CRC(1b89bdc4) SHA1(39c639587dc30f9d6636b46d0465f06272838432) )
	ROM_LOAD16_BYTE( "r240h0.bin", 0x00000, 0x10000, CRC(94f89b8c) SHA1(6c336bc30636a02c625d31f3057ec86bf4d155fc) )
	ROM_LOAD16_BYTE( "r240l1.bin", 0x20001, 0x10000, CRC(1a73be4f) SHA1(e2de2cb485f78963368fb8ceba8fb66ca56dba34) )
	ROM_LOAD16_BYTE( "r240h1.bin", 0x20000, 0x10000, CRC(898dd9f6) SHA1(93e791dd4ed7e4afa47a04df6fdde359e41c2075) )

	ROM_REGION( 0x80000, "svrom", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME  PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1985, swyft,0,       0,		swyft,		swyft,	 0, 	"Information Applicance Inc.",   "Swyft",		GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1987, cat,  swyft,   0,		cat,		cat,	 0, 	"Canon",   "Cat",		GAME_NOT_WORKING | GAME_NO_SOUND)
