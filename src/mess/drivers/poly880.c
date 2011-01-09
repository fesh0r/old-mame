/***************************************************************************

    Poly-Computer 880

    12/05/2009 Skeleton driver.

    http://www.kc85-museum.de/books/poly880/index.html

****************************************************************************/

#include "emu.h"
#include "includes/poly880.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cassette.h"
#include "machine/z80pio.h"
#include "machine/z80ctc.h"
#include "devices/messram.h"
#include "poly880.lh"

/*

    TODO:

    - SEND/SCON
    - MCYCL (activate single stepping)
    - CYCL (single step)
    - layout LEDs (address bus, data bus, command bus, MCYCL)
    - RAM expansion

*/

/* Read/Write Handlers */

static void update_display(poly880_state *state)
{
	int i;

	for (i = 0; i < 8; i++)
	{
		if (BIT(state->digit, i)) output_set_digit_value(7 - i, state->segment);
	}
}

static WRITE8_HANDLER( cldig_w )
{
	poly880_state *state = space->machine->driver_data<poly880_state>();

	state->digit = data;

	update_display(state);
}

/* Memory Maps */

static ADDRESS_MAP_START( poly880_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0x0c00) AM_ROM
	AM_RANGE(0x1000, 0x13ff) AM_MIRROR(0x0c00) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_MIRROR(0x0c00) AM_ROM
	AM_RANGE(0x3000, 0x33ff) AM_MIRROR(0x0c00) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x3c00) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK("bank1")
ADDRESS_MAP_END

static ADDRESS_MAP_START( poly880_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xaf)
	AM_RANGE(0x80, 0x83) AM_DEVREADWRITE(Z80PIO1_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x84, 0x87) AM_DEVREADWRITE(Z80PIO2_TAG, z80pio_ba_cd_r, z80pio_ba_cd_w)
	AM_RANGE(0x88, 0x8b) AM_DEVREADWRITE(Z80CTC_TAG, z80ctc_r, z80ctc_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x0f) AM_WRITE(cldig_w)
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

static INPUT_PORTS_START( poly880 )
	PORT_START("KI1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("GO") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("EXEC") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("BACK") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("REG") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("FCT") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("STEP") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MEM") PORT_CODE(KEYCODE_M)

	PORT_START("KI2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')

	PORT_START("KI3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')

	PORT_START("SPECIAL")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RES") PORT_CODE(KEYCODE_F1) PORT_CHANGED(trigger_reset, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MON") PORT_CODE(KEYCODE_F2) PORT_CHANGED(trigger_nmi, 0)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("MCYCL") PORT_CODE(KEYCODE_F3)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CYCL") PORT_CODE(KEYCODE_F4)
INPUT_PORTS_END

/* Z80-CTC Interface */

static WRITE_LINE_DEVICE_HANDLER( ctc_z0_w )
{
	// SEND
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z1_w )
{
}

static WRITE_LINE_DEVICE_HANDLER( ctc_z2_w )
{
	z80ctc_trg3_w(device, state);
}

static Z80CTC_INTERFACE( ctc_intf )
{
	0,              	/* timer disables */
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* interrupt handler */
	DEVCB_LINE(ctc_z0_w),	/* ZC/TO0 callback */
	DEVCB_LINE(ctc_z1_w),	/* ZC/TO1 callback */
	DEVCB_LINE(ctc_z2_w)    /* ZC/TO2 callback */
};

/* Z80-PIO Interface */

static WRITE8_DEVICE_HANDLER( pio1_port_a_w )
{
	/*

        bit     signal  description

        PA0     SD0     segment E
        PA1     SD1     segment D
        PA2     SD2     segment C
        PA3     SD3     segment P
        PA4     SD4     segment G
        PA5     SD5     segment A
        PA6     SD6     segment F
        PA7     SD7     segment B

    */

	poly880_state *state = device->machine->driver_data<poly880_state>();

	state->segment = BITSWAP8(data, 3, 4, 6, 0, 1, 2, 7, 5);

	update_display(state);
}

static READ8_DEVICE_HANDLER( pio1_port_b_r )
{
	/*

        bit     signal  description

        PB0     TTY
        PB1     MIN     tape input
        PB2     MOUT    tape output
        PB3
        PB4     KI1     key row 1 input
        PB5     KI2     key row 2 input
        PB6     SCON
        PB7     KI3     key row 3 input

    */

	poly880_state *state = device->machine->driver_data<poly880_state>();

	UINT8 data = 0xf0 | ((cassette_input(state->cassette) < +0.0) << 1);
	int i;

	for (i = 0; i < 8; i++)
	{
		if (BIT(state->digit, i))
		{
			if (!BIT(input_port_read(device->machine, "KI1"), i)) data &= ~0x10;
			if (!BIT(input_port_read(device->machine, "KI2"), i)) data &= ~0x20;
			if (!BIT(input_port_read(device->machine, "KI3"), i)) data &= ~0x80;
		}
	}

	return data;
}

static WRITE8_DEVICE_HANDLER( pio1_port_b_w )
{
	/*

        bit     signal  description

        PB0     TTY     teletype serial output
        PB1     MIN
        PB2     MOUT    tape output
        PB3
        PB4     KI1     key row 1 input
        PB5     KI2     key row 2 input
        PB6     SCON
        PB7     KI3     key row 3 input

    */

	poly880_state *state = device->machine->driver_data<poly880_state>();

	/* tape output */
	cassette_output(state->cassette, BIT(data, 2) ? +1.0 : -1.0);
}

static Z80PIO_INTERFACE( pio1_intf )
{
	DEVCB_CPU_INPUT_LINE(Z80_TAG, INPUT_LINE_IRQ0),	/* callback when change interrupt status */
	DEVCB_NULL,						/* port A read callback */
	DEVCB_HANDLER(pio1_port_a_w),	/* port A write callback */
	DEVCB_NULL,						/* portA ready active callback */
	DEVCB_HANDLER(pio1_port_b_r),	/* port B read callback */
	DEVCB_HANDLER(pio1_port_b_w),	/* port B write callback */
	DEVCB_NULL						/* portB ready active callback */
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

static const z80_daisy_config poly880_daisy_chain[] =
{
	{ Z80PIO1_TAG },
	{ Z80PIO2_TAG },
	{ Z80CTC_TAG },
	{ NULL }
};

/* Machine Initialization */

static MACHINE_START( poly880 )
{
	poly880_state *state = machine->driver_data<poly880_state>();

	/* find devices */
	state->cassette = machine->device(CASSETTE_TAG);

	/* register for state saving */
	state_save_register_global(machine, state->digit);
	state_save_register_global(machine, state->segment);
}

/* Machine Driver */

static const cassette_config poly880_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED),
	NULL
};

static MACHINE_CONFIG_START( poly880, poly880_state )

	/* basic machine hardware */
    MCFG_CPU_ADD(Z80_TAG, Z80, XTAL_7_3728MHz/8)
    MCFG_CPU_PROGRAM_MAP(poly880_mem)
    MCFG_CPU_IO_MAP(poly880_io)

    MCFG_MACHINE_START(poly880)

	/* video hardware */
	MCFG_DEFAULT_LAYOUT( layout_poly880 )

	/* devices */
	MCFG_Z80CTC_ADD(Z80CTC_TAG, XTAL_7_3728MHz/16, ctc_intf)
	MCFG_Z80PIO_ADD(Z80PIO1_TAG, XTAL_7_3728MHz/16, pio1_intf)
	MCFG_Z80PIO_ADD(Z80PIO2_TAG, XTAL_7_3728MHz/16, pio2_intf)

	MCFG_CASSETTE_ADD(CASSETTE_TAG, poly880_cassette_config)

	/* internal ram */
	MCFG_RAM_ADD("messram")
	MCFG_RAM_DEFAULT_SIZE("1K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( poly880 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "poly880.i5", 0x0000, 0x0400, CRC(b1c571e8) SHA1(85bfe53d39d6690e79999a1e1240789497e72db0) )
	ROM_LOAD( "poly880.i6", 0x1000, 0x0400, CRC(9efddf5b) SHA1(6ffa2f80b2c6f8ec9e22834f739c82f9754272b8) )
ROM_END

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT    COMPANY             FULLNAME                FLAGS */
COMP( 1983, poly880,	0,		0,		poly880,	poly880,	0,		"VEB Polytechnik",	"Poly-Computer 880",	GAME_SUPPORTS_SAVE | GAME_NO_SOUND)
