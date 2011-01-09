/***************************************************************************

    C-80

    12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "includes/c80.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "machine/z80pio.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "c80.lh"

/* Memory Maps */

static ADDRESS_MAP_START( c80_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_ROM
	AM_RANGE(0x0800, 0x0bff) AM_MIRROR(0x400) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( c80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x7c, 0x7f) AM_DEVREADWRITE(Z80PIO2_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
	AM_RANGE(0xbc, 0xbf) AM_DEVREADWRITE(Z80PIO1_TAG, z80pio_cd_ba_r, z80pio_cd_ba_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_CHANGED( trigger_reset )
{
	cputag_set_input_line(field->port->machine, Z80_TAG, INPUT_LINE_RESET, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_CHANGED( trigger_nmi )
{
	cputag_set_input_line(field->port->machine, Z80_TAG, INPUT_LINE_NMI, newval ? CLEAR_LINE : ASSERT_LINE);
}

static INPUT_PORTS_START( c80 )
	PORT_START("ROW0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("REG") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("GO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("FCN") PORT_CODE(KEYCODE_F1)

	PORT_START("ROW1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("+") PORT_CODE(KEYCODE_UP)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("-") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')

	PORT_START("ROW2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("MEM") PORT_CODE(KEYCODE_M)

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("RES") PORT_CODE(KEYCODE_F10) PORT_CHANGED(trigger_reset, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYPAD ) PORT_NAME("BRK") PORT_CODE(KEYCODE_ESC) PORT_CHANGED(trigger_nmi, 0)
INPUT_PORTS_END

/* Z80-PIO Interface */

static READ8_DEVICE_HANDLER( pio1_port_a_r )
{
	/*

        bit     description

        PA0     keyboard row 0 input
        PA1     keyboard row 1 input
        PA2     keyboard row 2 input
        PA3
        PA4     _BSTB input
        PA5     display enable output (0=enabled, 1=disabled)
        PA6     tape output
        PA7     tape input

    */

	c80_state *state = device->machine->driver_data<c80_state>();

	UINT8 data = !state->pio1_brdy << 4 | 0x07;

	int i;

	for (i = 0; i < 8; i++)
	{
		if (!BIT(state->keylatch, i))
		{
			if (!BIT(input_port_read(device->machine, "ROW0"), i)) data &= ~0x01;
			if (!BIT(input_port_read(device->machine, "ROW1"), i)) data &= ~0x02;
			if (!BIT(input_port_read(device->machine, "ROW2"), i)) data &= ~0x04;
		}
	}

	data |= (cassette_input(state->cassette) < +0.0) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( pio1_port_a_w )
{
	/*

        bit     description

        PA0     keyboard row 0 input
        PA1     keyboard row 1 input
        PA2     keyboard row 2 input
        PA3
        PA4     _BSTB input
        PA5     display enable output (0=enabled, 1=disabled)
        PA6     tape output
        PA7     tape input

    */

	c80_state *state = device->machine->driver_data<c80_state>();

	state->pio1_a5 = BIT(data, 5);

	if (!BIT(data, 5))
	{
		state->digit = 0;
	}

	cassette_output(state->cassette, BIT(data, 6) ? +1.0 : -1.0);
}

static WRITE8_DEVICE_HANDLER( pio1_port_b_w )
{
	/*

        bit     description

        PB0     VQD30 segment A
        PB1     VQD30 segment B
        PB2     VQD30 segment C
        PB3     VQD30 segment D
        PB4     VQD30 segment E
        PB5     VQD30 segment F
        PB6     VQD30 segment G
        PB7     VQD30 segment P

    */

	c80_state *state = device->machine->driver_data<c80_state>();

	if (!state->pio1_a5)
	{
		output_set_digit_value(state->digit, data);
	}

	state->keylatch = data;
}

static WRITE_LINE_DEVICE_HANDLER( pio1_brdy_w )
{
	c80_state *driver_state = device->machine->driver_data<c80_state>();

	driver_state->pio1_brdy = state;

	if (state)
	{
		if (!driver_state->pio1_a5)
		{
			driver_state->digit++;
		}

		z80pio_bstb_w(device, 1);
		z80pio_bstb_w(device, 0);
	}
}

static Z80PIO_INTERFACE( pio1_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_HANDLER(pio1_port_a_r),	/* port A read callback */
	DEVCB_HANDLER(pio1_port_a_w),	/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_HANDLER(pio1_port_b_w),	/* port B write callback */
	DEVCB_LINE(pio1_brdy_w)			/* portB ready active callback */
};

static Z80PIO_INTERFACE( pio2_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_NULL,						/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_NULL,						/* port B read callback */
	DEVCB_NULL,						/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
};

/* Z80 Daisy Chain */

static const z80_daisy_config c80_daisy_chain[] =
{
	{ Z80PIO1_TAG },
	{ Z80PIO2_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( c80 )
{
	c80_state *state = machine->driver_data<c80_state>();

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->keylatch);
	state_save_register_global(machine, state->digit);
	state_save_register_global(machine, state->pio1_a5);
	state_save_register_global(machine, state->pio1_brdy);
}

/* Machine Driver */

static const cassette_config c80_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( c80, c80_state )

	/* basic machine hardware */
    MCFG_CPU_ADD(Z80_TAG, Z80, 2500000) /* U880D */
    MCFG_CPU_PROGRAM_MAP(c80_mem)
    MCFG_CPU_IO_MAP(c80_io)
	MCFG_CPU_CONFIG(c80_daisy_chain)

    MCFG_MACHINE_START(c80)

    /* video hardware */
	MCFG_DEFAULT_LAYOUT( layout_c80 )

	/* devices */
	MCFG_Z80PIO_ADD(Z80PIO1_TAG, 2500000, pio1_intf)
	MCFG_Z80PIO_ADD(Z80PIO2_TAG, 2500000, pio2_intf)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, c80_cassette_config)

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("1K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( c80 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "c80.d3", 0x0000, 0x0400, CRC(ad2b3296) SHA1(14f72cb73a4068b7a5d763cc0e254639c251ce2e) )
ROM_END


/* System Drivers */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT  COMPANY             FULLNAME    FLAGS */
COMP( 1986, c80,	0,		0,		c80,	c80,	0,	"Joachim Czepa",	"C-80",		GAME_SUPPORTS_SAVE | GAME_NO_SOUND)
