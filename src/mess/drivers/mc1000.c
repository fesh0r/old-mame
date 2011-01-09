/***************************************************************************

    MC 1000

    12/05/2009 Skeleton driver.

    http://ensjo.wikispaces.com/MC-1000+on+JEMU
    http://ensjo.blogspot.com/2006/11/color-artifacting-no-mc-1000.html

****************************************************************************/

/*

    TODO:

    - xtal frequency?
    - Z80 wait at 0x0000-0x1fff when !hsync & !vsync
    - 80-column card (MC6845) character generator ROM
    - Charlemagne / GEM-1000 / Junior Computer ROMs

*/

#include "emu.h"
#include "includes/mc1000.h"
#include "cpu/z80/z80.h"
#include "devices/cassette.h"
#include "video/m6847.h"
#include "sound/ay8910.h"
#include "machine/ctronics.h"
#include "machine/rescap.h"
#include "devices/messram.h"

/* Memory Banking */

static void mc1000_bankswitch(running_machine *machine)
{
	mc1000_state *state = machine->driver_data<mc1000_state>();
	address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* MC6845 video RAM */
	memory_set_bank(machine, "bank2", state->mc6845_bank);

	/* extended RAM */
	if (messram_get_size(machine->device("messram")) > 16*1024)
	{
		memory_install_readwrite_bank(program, 0x4000, 0x7fff, 0, 0, "bank3");
	}
	else
	{
		memory_unmap_readwrite(program, 0x4000, 0x7fff, 0, 0);
	}

	/* MC6847 video RAM */
	if (state->mc6847_bank)
	{
		if (messram_get_size(machine->device("messram")) > 16*1024)
		{
			memory_install_readwrite_bank(program, 0x8000, 0x97ff, 0, 0, "bank4");
		}
		else
		{
			memory_unmap_readwrite(program, 0x8000, 0x97ff, 0, 0);
		}
	}
	else
	{
		memory_install_readwrite_bank(program, 0x8000, 0x97ff, 0, 0, "bank4");
	}

	memory_set_bank(machine, "bank4", state->mc6847_bank);

	/* extended RAM */
	if (messram_get_size(machine->device("messram")) > 16*1024)
	{
		memory_install_readwrite_bank(program, 0x9800, 0xbfff, 0, 0, "bank5");
	}
	else
	{
		memory_unmap_readwrite(program, 0x9800, 0xbfff, 0, 0);
	}
}

/* Read/Write Handlers */

static READ8_HANDLER( printer_r )
{
	mc1000_state *state = space->machine->driver_data<mc1000_state>();

	return centronics_busy_r(state->centronics);
}

static WRITE8_HANDLER( printer_w )
{
	mc1000_state *state = space->machine->driver_data<mc1000_state>();

	centronics_strobe_w(state->centronics, BIT(data, 0));
}

static WRITE8_HANDLER( mc6845_ctrl_w )
{
	mc1000_state *state = space->machine->driver_data<mc1000_state>();

	state->mc6845_bank = BIT(data, 0);

	mc1000_bankswitch(space->machine);
}

static WRITE8_HANDLER( mc6847_attr_w )
{
	/*

        bit     description

        0       enable CPU video RAM access
        1       CSS
        2       GM0
        3       GM1
        4       GM2
        5       _INT/EXT
        6       _A/S
        7       _A/G

    */

	mc1000_state *state = space->machine->driver_data<mc1000_state>();

	state->mc6847_bank = BIT(data, 0);
	mc6847_css_w(state->mc6847, BIT(data, 1));
	mc6847_gm0_w(state->mc6847, BIT(data, 2));
	mc6847_gm1_w(state->mc6847, BIT(data, 3));
	mc6847_gm2_w(state->mc6847, BIT(data, 4));
	mc6847_intext_w(state->mc6847, BIT(data, 5));
	mc6847_as_w(state->mc6847, BIT(data, 6));
	mc6847_ag_w(state->mc6847, BIT(data, 7));

	mc1000_bankswitch(space->machine);
}

/* Memory Maps */

static ADDRESS_MAP_START( mc1000_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAMBANK("bank1")
	AM_RANGE(0x2000, 0x27ff) AM_RAMBANK("bank2") AM_BASE_MEMBER(mc1000_state, mc6845_video_ram)
	AM_RANGE(0x2800, 0x3fff) AM_RAM
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank3")
	AM_RANGE(0x8000, 0x97ff) AM_RAMBANK("bank4") AM_BASE_MEMBER(mc1000_state, mc6847_video_ram)
	AM_RANGE(0x9800, 0xbfff) AM_RAMBANK("bank5")
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mc1000_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x04, 0x04) AM_READWRITE(printer_r, printer_w)
	AM_RANGE(0x05, 0x05) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
//  AM_RANGE(0x10, 0x10) AM_DEVWRITE(MC6845_TAG, mc6845_address_w)
//  AM_RANGE(0x11, 0x11) AM_DEVREADWRITE(MC6845_TAG, mc6845_register_r, mc6845_register_w)
	AM_RANGE(0x12, 0x12) AM_WRITE(mc6845_ctrl_w)
	AM_RANGE(0x20, 0x20) AM_DEVWRITE(AY8910_TAG, ay8910_address_w)
	AM_RANGE(0x40, 0x40) AM_DEVREAD(AY8910_TAG, ay8910_r)
	AM_RANGE(0x60, 0x60) AM_DEVWRITE(AY8910_TAG, ay8910_data_w)
	AM_RANGE(0x80, 0x80) AM_WRITE(mc6847_attr_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mc1000 )
	PORT_START("JOYA") /* Player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    /* = 'I' */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  /* = 'Q' */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  /* = 'Y' */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) /* = '1' */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )        /* = '9' */
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOYB") /* Player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)        /* = '@' */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)    /* = 'H' */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)  /* = 'P' */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)  /* = 'X' */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) /* = '0' */
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('@')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RUBOUT") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('^')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RESET")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))

	PORT_START("JOYAKEYMAP")
	PORT_CONFNAME( 0x01, 0x00, "JOYSTICK A (P1) keyboard mapping" )
	PORT_CONFSETTING( 0x00, DEF_STR( Off ) )
	PORT_CONFSETTING( 0x01, DEF_STR( On ) )

	PORT_START("JOYBKEYMAP")
	PORT_CONFNAME( 0x01, 0x00, "JOYSTICK B (P2) keyboard mapping" )
	PORT_CONFSETTING( 0x00, DEF_STR( Off ) )
	PORT_CONFSETTING( 0x01, DEF_STR( On ) )

	PORT_INCLUDE( m6847_artifacting )
INPUT_PORTS_END

/* Video */

static WRITE_LINE_DEVICE_HANDLER( mc1000_mc6847_fs_w )
{
	mc1000_state *mc1000 = device->machine->driver_data<mc1000_state>();
	mc1000->vsync = state;
}

static WRITE_LINE_DEVICE_HANDLER( mc1000_mc6847_hs_w )
{
	mc1000_state *mc1000 = device->machine->driver_data<mc1000_state>();
	mc1000->hsync = state;
}

static READ8_DEVICE_HANDLER( mc1000_mc6847_videoram_r )
{
	mc1000_state *state = device->machine->driver_data<mc1000_state>();

	mc6847_inv_w(device, BIT(state->mc6847_video_ram[offset], 7));

	return state->mc6847_video_ram[offset];
}

static VIDEO_UPDATE( mc1000 )
{
	mc1000_state *state = screen->machine->driver_data<mc1000_state>();
	return mc6847_update(state->mc6847, bitmap, cliprect);
}

UINT8 mc1000_get_char_rom(running_machine *machine, UINT8 ch,int line)
{
	return ch;
}

/* AY-3-8910 Interface */

static WRITE8_DEVICE_HANDLER( keylatch_w )
{
	mc1000_state *state = device->machine->driver_data<mc1000_state>();

	state->keylatch = data;

	cassette_output(state->cassette, BIT(data, 7) ? -1.0 : +1.0);
}

static READ8_DEVICE_HANDLER( keydata_r )
{
	mc1000_state *state = device->machine->driver_data<mc1000_state>();

	UINT8 data = 0xff;

	if (!BIT(state->keylatch, 0))
	{
	                              data &= input_port_read(device->machine, "ROW0");
		if (input_port_read(device->machine, "JOYBKEYMAP"))
	                              data &= input_port_read(device->machine, "JOYB");
	}
	if (!BIT(state->keylatch, 1))
	{
	                              data &= input_port_read(device->machine, "ROW1");
		if (input_port_read(device->machine, "JOYAKEYMAP"))
	                              data &= input_port_read(device->machine, "JOYA");
	}
	if (!BIT(state->keylatch, 2)) data &= input_port_read(device->machine, "ROW2");
	if (!BIT(state->keylatch, 3)) data &= input_port_read(device->machine, "ROW3");
	if (!BIT(state->keylatch, 4)) data &= input_port_read(device->machine, "ROW4");
	if (!BIT(state->keylatch, 5)) data &= input_port_read(device->machine, "ROW5");
	if (!BIT(state->keylatch, 6)) data &= input_port_read(device->machine, "ROW6");
	if (!BIT(state->keylatch, 7)) data &= input_port_read(device->machine, "ROW7");
	
	data = (input_port_read(device->machine, "MODIFIERS") & 0xc0) | (data & 0x3f);

	if (cassette_input(state->cassette) < +0.0)	data &= 0x7f;

	return data;
}

static const ay8910_interface ay8910_intf =
{
	AY8910_SINGLE_OUTPUT,
	{ RES_K(2.2), 0, 0 },
	DEVCB_NULL,
	DEVCB_HANDLER(keydata_r),
	DEVCB_HANDLER(keylatch_w),
	DEVCB_NULL
};

/* Machine Initialization */

static MACHINE_START( mc1000 )
{
	mc1000_state *state = machine->driver_data<mc1000_state>();
	address_space *program = cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM);

	/* find devices */
	state->mc6845 = machine->device(MC6845_TAG);
	state->mc6847 = machine->device(MC6847_TAG);
	state->centronics = machine->device(CENTRONICS_TAG);
	state->cassette = machine->device(CASSETTE_TAG);

	/* setup memory banking */
	memory_install_readwrite_bank(program, 0x0000, 0x1fff, 0, 0, "bank1");
	memory_configure_bank(machine, "bank1", 0, 1, machine->region(Z80_TAG)->base(), 0);
	memory_configure_bank(machine, "bank1", 1, 1, machine->region(Z80_TAG)->base() + 0xc000, 0);
	memory_set_bank(machine, "bank1", 1);

	state->rom0000 = 1;

	memory_install_readwrite_bank(program, 0x2000, 0x27ff, 0, 0, "bank2");
	memory_configure_bank(machine, "bank2", 0, 1, machine->region(Z80_TAG)->base() + 0x2000, 0);
	memory_configure_bank(machine, "bank2", 1, 1, state->mc6845_video_ram, 0);
	memory_set_bank(machine, "bank2", 0);

	memory_configure_bank(machine, "bank3", 0, 1, machine->region(Z80_TAG)->base() + 0x4000, 0);
	memory_set_bank(machine, "bank3", 0);

	memory_configure_bank(machine, "bank4", 0, 1, state->mc6847_video_ram, 0);
	memory_configure_bank(machine, "bank4", 1, 1, machine->region(Z80_TAG)->base() + 0x8000, 0);
	memory_set_bank(machine, "bank4", 0);

	memory_configure_bank(machine, "bank5", 0, 1, machine->region(Z80_TAG)->base() + 0x9800, 0);
	memory_set_bank(machine, "bank5", 0);

	mc1000_bankswitch(machine);

	/* register for state saving */
	state_save_register_global(machine, state->rom0000);
	state_save_register_global(machine, state->mc6845_bank);
	state_save_register_global(machine, state->mc6847_bank);
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->hsync);
	state_save_register_global(machine, state->vsync);
}

static MACHINE_RESET( mc1000 )
{
	mc1000_state *state = machine->driver_data<mc1000_state>();

	memory_set_bank(machine, "bank1", 1);

	state->rom0000 = 1;
}

/* Machine Driver */

/*
 
 Interrupt generator:
 NE555 chip in astable circuit.
 
  +---------*---*---o V+
  |         |   |
 +-+        |   |
 | |309K    |   |
 | |R17     |8  |4
 +-+      +-------+
  |      7|       |3
  *-------|       |-------> /INT (Z80)
  |       |       |
  |       |       |
 +-+R16  2| IC 28 |
 | |1K +--|       |
 | |   |  |  555  |
 +-+   |  |       |
  |    | 6|       |5
  *----*�-|       |---+
  |       |       |   |
 ---C30   +-------+  ---C29
 ---103       |1     ---103
 _|_         _|_     _|_
 ///         ///     ///
 
 Calculated properties:
 
 * 99.74489795918367 Duty Cycle Percentage
 * 368.1126130105722 Frequency in Hertz
 * 0.00000693 Seconds Low
 * 0.0027096299999999998 Seconds High
 
 */

#define MC1000_NE555_FREQ       (368) /* Hz */
#define MC1000_NE555_DUTY_CYCLE (99.745) /* % */

static TIMER_DEVICE_CALLBACK( ne555_tick )
{
	mc1000_state *state = timer.machine->driver_data<mc1000_state>();

	// (state->ne555_int not needed anymore and can be done with?)
	state->ne555_int = param;

	cputag_set_input_line(timer.machine, Z80_TAG, INPUT_LINE_IRQ0, param);
}

static const cassette_config mc1000_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED),
	NULL
};

static const mc6847_interface mc1000_mc6847_intf =
{
	DEVCB_HANDLER(mc1000_mc6847_videoram_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(mc1000_mc6847_fs_w),
	DEVCB_LINE(mc1000_mc6847_hs_w),
	DEVCB_NULL
};

static MACHINE_CONFIG_START( mc1000, mc1000_state )

	/* basic machine hardware */
	MCFG_CPU_ADD(Z80_TAG, Z80, 3579545)
	MCFG_CPU_PROGRAM_MAP(mc1000_mem)
	MCFG_CPU_IO_MAP(mc1000_io)

	MCFG_MACHINE_START(mc1000)
	MCFG_MACHINE_RESET(mc1000)

	/* timers */
	MCFG_TIMER_ADD_PERIODIC("ne555clear", ne555_tick, HZ(MC1000_NE555_FREQ))
	MCFG_TIMER_PARAM(CLEAR_LINE)

	MCFG_TIMER_ADD_PERIODIC("ne555assert", ne555_tick, HZ(MC1000_NE555_FREQ))
	MCFG_TIMER_START_DELAY(HZ(MC1000_NE555_FREQ * 100 / MC1000_NE555_DUTY_CYCLE))
	MCFG_TIMER_PARAM(ASSERT_LINE)

	/* video hardware */
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MCFG_SCREEN_SIZE(320, 25+192+26)
	MCFG_SCREEN_VISIBLE_AREA(0, 319, 1, 239)
	MCFG_PALETTE_LENGTH(16)

	MCFG_VIDEO_UPDATE(mc1000)

	MCFG_MC6847_ADD(MC6847_TAG, mc1000_mc6847_intf)
	MCFG_MC6847_TYPE(M6847_VERSION_ORIGINAL_NTSC)
	MCFG_MC6847_CHAR_ROM(mc1000_get_char_rom)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(AY8910_TAG, AY8910, 3579545/2)
	MCFG_SOUND_CONFIG(ay8910_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MCFG_CASSETTE_ADD(CASSETTE_TAG, mc1000_cassette_config)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("16K")
	MCFG_RAM_EXTRA_OPTIONS("48K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( mc1000 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "mc1000.ic17", 0xc000, 0x2000, CRC(8e78d80d) SHA1(9480270e67a5db2e7de8bc5c8b9e0bb210d4142b) )
	ROM_LOAD( "mc1000.ic12", 0xe000, 0x2000, CRC(750c95f0) SHA1(fd766f5ea4481ef7fd4df92cf7d8397cc2b5a6c4) )
ROM_END


/* Driver Initialization */

DIRECT_UPDATE_HANDLER( mc1000_direct_update_handler )
{
	mc1000_state *state = machine->driver_data<mc1000_state>();

	if (state->rom0000)
	{
		if (address >= 0xc000)
		{
			memory_set_bank(machine, "bank1", 0);
			state->rom0000 = 0;
		}
	}

	return address;
}

static DRIVER_INIT( mc1000 )
{
	cputag_get_address_space(machine, Z80_TAG, ADDRESS_SPACE_PROGRAM)->set_direct_update_handler(direct_update_delegate_create_static(mc1000_direct_update_handler, *machine));
}

/* System Drivers */

/*    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT        COMPANY             FULLNAME        FLAGS */
COMP( 1985,	mc1000,		0,			0,		mc1000,		mc1000,		mc1000,		"CCE",				"MC-1000",		GAME_IMPERFECT_GRAPHICS | GAME_SUPPORTS_SAVE )
