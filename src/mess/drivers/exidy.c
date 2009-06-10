/******************************************************************************

  Exidy Sorcerer system driver

    The UART controls rs232 and cassette i/o. The chip is a AY-3-1014A or AY-3-1015.


    port fc:
    ========
    input/output:
        uart data

    port fd:
    ========
    input: uart status
        bit 4: parity error (RPE)
        bit 3: framing error (RFE)
        bit 2: over-run (RDP)
        bit 1: data available (RDA)
        bit 0: transmit buffer empty (TPMT)

    output:
        bit 4: no parity (NPB)
        bit 3: parity type (POE)
        bit 2: number of stop bits (NSB)
        bit 1: number of bits per char bit 2 (NDB2)
        bit 0: number of bits per char bit 1 (NDB1)

    port fe:
    ========

    output:

        bit 7: rs232 enable (1=rs232, 0=cassette)
        bit 6: baud rate (1=1200, 0=300)
        bit 5: cassette motor 2
        bit 4: cassette motor 1
        bit 3..0: keyboard line select

    input:
        bit 7..6: parallel control (not emulated)
                7: must be 1 to read data from parallel port via PARIN
                6: must be 1 to send data out of parallel port via PAROUT
        bit 5: vsync
        bit 4..0: keyboard line data

    port ff:
    ========
      parallel port in/out

    -------------------------------------------------------------------------------------

    When cassette is selected, it is connected to the uart data input via the cassette
    interface hardware.

    The cassette interface hardware converts square-wave pulses into bits which the uart receives.

    1. the cassette format: "frequency shift" is converted
    into the uart data format "non-return to zero"

    2. on cassette a 1 data bit is stored as a high frequency
    and a 0 data bit as a low frequency
    - At 1200 baud, a logic 1 is 1 cycle of 1200 Hz and a logic 0 is 1/2 cycle of 600 Hz.
    - At 300 baud, a logic 1 is 8 cycles of 2400 Hz and a logic 0 is 4 cycles of 1200 Hz.

    Attenuation is applied to the signal and the square wave edges are rounded.

    A manchester encoder is used. A flip-flop synchronises input
    data on the positive-edge of the clock pulse.

    Interestingly the data on cassette is stored in xmodem-checksum.


	Due to bugs in the hardware and software of a real Sorcerer, the serial
	interface misbehaves.
	1. Sorcerer I had a hardware problem causing rs232 idle to be a space (+9v)
	instead of mark (-9v). Fixed in Sorcerer II.
	2. When you select a different baud for rs232, it was "remembered" but not
	sent to port fe. It only gets sent when motor on was requested. Motor on is
	only meaningful in a cassette operation.
	3. The monitor software always resets the device to cassette whenever the
	keyboard is scanned, motors altered, or an error occurred.
	4. The above problems make rs232 communication impractical unless you write
	your own routines or create a corrected monitor rom.

    Sound:

    external speaker connected to the parallel port
    speaker is connected to all pins. All pins need to be toggled at the same time.


    Kevin Thacker [MESS driver]

 ******************************************************************************

    The CPU clock speed is 2.106 MHz, which was increased to 4.0 MHz on the last production runs.

    The Sorcerer has a bus connection for S100 equipment. This allows the connection
    of disk drives, provided that suitable driver/boot software is loaded.

    The driver "exidy" emulates a Sorcerer with 4 floppy disk drives fitted, and 32k of ram.

    The driver "exidyd" emulates the most common form, a cassette-based system with 48k of ram.

*******************************************************************************


Emulation status:

exidy: 32k disk-based system. Disks not tested. Optional S-100 interface not implemented.
exidyd: 48k cassette-based system. Disks and S-100 are not options on this system.

Real machines had optional RAM sizes of 4k, 16k, 32k, 48k (officially).
Unofficially, the cart could hold another 8k of static RAM (4x 6116), giving 56k total.

On the back of the machine is a 50-pin expansion port, which could be hooked to the
expansion unit.

Also is a 25-pin parallel port, allowing inwards and outwards communication.
It was often hooked to a printer, a joystick, a music card, or a speaker.

We emulate the printer and the speaker.

Another 25-pin port provided two-way serial communications. Only two speeds are
available - 300 baud and 1200 baud. There is no handshaking. This protocol is
currently not emulated.

Other pins on this connector provided for two cassette players. The connections
for cassette unit 1 are duplicated on a set of phono plugs.

We emulate the use of two cassette units. An option allows you to hear the sound
of the tape during playback.

The sorcerer has a UART device used by the serial interface and the cassette system.


********************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "includes/exidy.h"
#include "machine/ctronics.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/z80bin.h"
#include "machine/ay31015.h"


static const device_config *exidy_ay31015;


static Z80BIN_EXECUTE( exidy );

static DEVICE_IMAGE_LOAD( exidy_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* not correct */
		basicdsk_set_geometry(image, 80, 2, 9, 512, 1, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}


static UINT8 exidy_fe = 0xff;
static UINT8 exidy_keyboard_line;

static WRITE8_HANDLER(exidy_fe_port_w);

#if 0

The serial code (which was never connected to the outside) is disabled for now.

/* timer for exidy serial chip transmit and receive */
//static emu_timer *serial_timer;

//static TIMER_CALLBACK(exidy_serial_timer_callback)
//{
	/* if rs232 is enabled, uart is connected to clock defined by bit6 of port fe.
    Transmit and receive clocks are connected to the same clock */

	/* if rs232 is disabled, receive clock is linked to cassette hardware */
//	if (exidy_fe & 0x80)
//	{
		connect to rs232
//	}
//}
#endif


/* timer to read cassette waveforms */
static emu_timer *cassette_timer;


static const device_config *cassette_device_image(running_machine *machine)
{
	if (exidy_fe & 0x20)
		return devtag_get_device(machine, "cassette2");
	else
		return devtag_get_device(machine, "cassette1");
}


static struct {
	struct {
		int length;		/* time cassette level is at input.level */
		int level;		/* cassette level */
		int bit;		/* bit being read */
	} input;
	struct {
		int length;		/* time cassette level is at output.level */
		int level;		/* cassette level */
		int bit;		/* bit to to output */
	} output;
} cass_data;


static TIMER_CALLBACK(exidy_cassette_tc)
{
	UINT8 cass_ws = 0;
	switch (exidy_fe & 0xc0)		/*/ bit 7 low indicates cassette */
	{
		case 0x00:				/* Cassette 300 baud */

			/* loading a tape - this is basically the same as the super80.
				We convert the 1200/2400 Hz signal to a 0 or 1, and send it to the uart. */

			cass_data.input.length++;

			cass_ws = (cassette_input(cassette_device_image(machine)) > +0.02) ? 1 : 0;

			if (cass_ws != cass_data.input.level)
			{
				cass_data.input.level = cass_ws;
				cass_data.input.bit = ((cass_data.input.length < 0x6) || (cass_data.input.length > 0x20)) ? 1 : 0;
				cass_data.input.length = 0;
				ay31015_set_input_pin( exidy_ay31015, AY31015_SI, cass_data.input.bit );
			}

			/* saving a tape - convert the serial stream from the uart, into 1200 and 2400 Hz frequencies.
				Synchronisation of the frequency pulses to the uart is extremely important. */

			cass_data.output.length++;
			if (!(cass_data.output.length & 0x1f))
			{
				cass_ws = ay31015_get_output_pin( exidy_ay31015, AY31015_SO );
				if (cass_ws != cass_data.output.bit)
				{
					cass_data.output.bit = cass_ws;
					cass_data.output.length = 0;
				}
			}

			if (!(cass_data.output.length & 3))
			{
				if (!((cass_data.output.bit == 0) && (cass_data.output.length & 4)))
				{
					cass_data.output.level ^= 1;			// toggle output state, except on 2nd half of low bit
					cassette_output(cassette_device_image(machine), cass_data.output.level ? -1.0 : +1.0);
				}
			}
			return;

		case 0x40:			/* Cassette 1200 baud */
			/* loading a tape */
			cass_data.input.length++;

			cass_ws = (cassette_input(cassette_device_image(machine)) > +0.02) ? 1 : 0;

			if (cass_ws != cass_data.input.level || cass_data.input.length == 10)
			{
				cass_data.input.bit = ((cass_data.input.length < 10) || (cass_data.input.length > 0x20)) ? 1 : 0;
				if ( cass_ws != cass_data.input.level )
				{
					cass_data.input.length = 0;
					cass_data.input.level = cass_ws;
				}
				ay31015_set_input_pin( exidy_ay31015, AY31015_SI, cass_data.input.bit );
			}

			/* saving a tape - convert the serial stream from the uart, into 600 and 1200 Hz frequencies. */

			cass_data.output.length++;
			if (!(cass_data.output.length & 0x07))
			{
				cass_ws = ay31015_get_output_pin( exidy_ay31015, AY31015_SO );
				if (cass_ws != cass_data.output.bit)
				{
					cass_data.output.bit = cass_ws;
					cass_data.output.length = 0;
				}
			}

			if (!(cass_data.output.length & 7))
			{
				if (!((cass_data.output.bit == 0) && (cass_data.output.length & 8)))
				{
					cass_data.output.level ^= 1;			// toggle output state, except on 2nd half of low bit
					cassette_output(cassette_device_image(machine), cass_data.output.level ? -1.0 : +1.0);
				}
			}
			return;
	}
}


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( exidy_reset )
{
	memory_set_bank(machine, 1, 0);
}

static MACHINE_START( exidyd )
{
//	serial_timer = timer_alloc(machine, exidy_serial_timer_callback, NULL);
	cassette_timer = timer_alloc(machine, exidy_cassette_tc, NULL);
}

static MACHINE_START( exidy )
{
	MACHINE_START_CALL( exidyd );
}

static MACHINE_RESET( exidyd )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	/* Initialize cassette interface */
	cass_data.output.length = 0;
	cass_data.output.level = 1;
	cass_data.input.length = 0;
	cass_data.input.bit = 1;

	exidy_ay31015 = devtag_get_device(machine, "ay_3_1015");

	exidy_fe_port_w(space, 0, 0);

	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, exidy_reset);
	memory_set_bank(machine, 1, 1);
}

static MACHINE_RESET( exidy )
{
	floppy_drive_set_geometry(image_from_devtype_and_index(machine, IO_FLOPPY, 0), FLOPPY_DRIVE_DS_80);
	MACHINE_RESET_CALL( exidyd );
}


static  READ8_HANDLER ( exidy_wd179x_r )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	switch (offset & 0x03)
	{
	case 0:
		return wd17xx_status_r(fdc, offset);
	case 1:
		return wd17xx_track_r(fdc, offset);
	case 2:
		return wd17xx_sector_r(fdc, offset);
	case 3:
		return wd17xx_data_r(fdc, offset);
	default:
		return 0xff;
	}
}

static WRITE8_HANDLER ( exidy_wd179x_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "wd179x");
	switch (offset & 0x03)
	{
	case 0:
		wd17xx_command_w(fdc, offset, data);
		return;
	case 1:
		wd17xx_track_w(fdc, offset, data);
		return;
	case 2:
		wd17xx_sector_w(fdc, offset, data);
		return;
	case 3:
		wd17xx_data_w(fdc, offset, data);
		return;
	default:
		break;
	}
}


static WRITE8_HANDLER(exidy_fc_port_w)
{
	ay31015_set_transmit_data( exidy_ay31015, data );
}


static WRITE8_HANDLER(exidy_fd_port_w)
{
	/* Translate data to control signals */

	ay31015_set_input_pin( exidy_ay31015, AY31015_CS, 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_NB1, ( data & 0x01 ) ? 1 : 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_NB2, ( data & 0x02 ) ? 1 : 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_TSB, ( data & 0x04 ) ? 1 : 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_EPS, ( data & 0x08 ) ? 1 : 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_NP,  ( data & 0x10 ) ? 1 : 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_CS, 1 );
}

#define EXIDY_CASSETTE_MOTOR_MASK ((1<<4)|(1<<5))


static WRITE8_HANDLER(exidy_fe_port_w)
{
	UINT8 changed_bits = (exidy_fe ^ data) & 0xf0;
	exidy_fe = data;

	/* bits 0..3 */
	exidy_keyboard_line = data & 0x0f;

	if (!changed_bits) return;

	/* bits 4..5 */
	/* does user want to hear the sound? */
	if ((input_port_read(space->machine, "CONFIG") & 8) && (data & EXIDY_CASSETTE_MOTOR_MASK))
	{
		if (data & 0x20)
		{
			cassette_change_state(devtag_get_device(space->machine, "cassette1"), CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
			cassette_change_state(devtag_get_device(space->machine, "cassette2"), CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
		}
		else
		{
			cassette_change_state(devtag_get_device(space->machine, "cassette2"), CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
			cassette_change_state(devtag_get_device(space->machine, "cassette1"), CASSETTE_SPEAKER_ENABLED, CASSETTE_MASK_SPEAKER);
		}
	}
	else
	{
		cassette_change_state(devtag_get_device(space->machine, "cassette2"), CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
		cassette_change_state(devtag_get_device(space->machine, "cassette1"), CASSETTE_SPEAKER_MUTED, CASSETTE_MASK_SPEAKER);
	}

	/* cassette 1 motor */
	cassette_change_state(devtag_get_device(space->machine, "cassette1"),
		(data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	/* cassette 2 motor */
	cassette_change_state(devtag_get_device(space->machine, "cassette2"),
		(data & 0x20) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);

	if ((data & EXIDY_CASSETTE_MOTOR_MASK) && (~data & 0x80))
		timer_adjust_periodic(cassette_timer, attotime_zero, 0, ATTOTIME_IN_HZ(19200));
	else
		timer_adjust_oneshot(cassette_timer, attotime_zero, 0);

	/* bit 6 */
	if (changed_bits & 0x40)
	{
		ay31015_set_receiver_clock( exidy_ay31015, (data & 0x40) ? 19200.0 : 4800.0);
		ay31015_set_transmitter_clock( exidy_ay31015, (data & 0x40) ? 19200.0 : 4800.0);
	}

	/* bit 7 */
	if (changed_bits & 0x80)
	{
		if (data & 0x80)
		{
		/* connect to serial device (not yet emulated) */
//			timer_adjust_periodic(serial_timer, attotime_zero, 0, ATTOTIME_IN_HZ(baud_rate));
		}
		else
		{
		/* connect to tape */
//			hd6402_connect(&cassette_serial_connection);
		}
	}
}

static WRITE8_HANDLER(exidy_ff_port_w)
{
	const device_config *printer = devtag_get_device(space->machine, "centronics");
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	/* reading the config switch */
	switch (input_port_read(space->machine, "CONFIG") & 0x06)
	{
		case 0: /* speaker */
			speaker_level_w(speaker, (data) ? 1 : 0);
			break;

		case 2: /* Centronics 7-bit printer */
			/* bit 7 = strobe, bit 6..0 = data */
			centronics_strobe_w(printer, (~data>>7) & 0x01);
			centronics_data_w(printer, 0, data & 0x7f);
			break;

		case 4: /* 8-bit parallel output */
			/* hardware strobe driven from port select, bit 7..0 = data */
			centronics_strobe_w(printer, 1);
			centronics_data_w(printer, 0, data);
			centronics_strobe_w(printer, 0);
			break;
	}
}

static READ8_HANDLER(exidy_fc_port_r)
{
	UINT8 data = ay31015_get_received_data( exidy_ay31015 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_RDAV, 0 );
	ay31015_set_input_pin( exidy_ay31015, AY31015_RDAV, 1 );
	return data;
}

static READ8_HANDLER(exidy_fd_port_r)
{
	/* set unused bits high */
	UINT8 data = 0xe0;

	ay31015_set_input_pin( exidy_ay31015, AY31015_SWE, 0 );
	data |= ay31015_get_output_pin( exidy_ay31015, AY31015_TBMT ) ? 0x01 : 0;
	data |= ay31015_get_output_pin( exidy_ay31015, AY31015_DAV  ) ? 0x02 : 0;
	data |= ay31015_get_output_pin( exidy_ay31015, AY31015_OR   ) ? 0x04 : 0;
	data |= ay31015_get_output_pin( exidy_ay31015, AY31015_FE   ) ? 0x08 : 0;
	data |= ay31015_get_output_pin( exidy_ay31015, AY31015_PE   ) ? 0x10 : 0;
	ay31015_set_input_pin( exidy_ay31015, AY31015_SWE, 1 );

	return data;
}

static READ8_HANDLER(exidy_fe_port_r)
{
	/* bits 6..7
     - hardware handshakes from user port
     - not emulated
     - tied high, allowing PARIN and PAROUT bios routines to run */

	UINT8 data = 0xc0;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
										"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15" };

	/* bit 5 - vsync (inverted) */
	data |= (((~input_port_read(space->machine, "VS")) & 0x01)<<5);

	/* bits 4..0 - keyboard data */
	data |= (input_port_read(space->machine, keynames[exidy_keyboard_line]) & 0x1f);

	return data;
}

static READ8_HANDLER(exidy_ff_port_r)
{
	/* The use of the parallel port as a general purpose port is not emulated.
	Currently the only use is to read the printer status in the Centronics CENDRV bios routine.
	This uses bit 7. The other bits have been set high (=nothing plugged in).
	This fixes those games that use a joystick. */

	const device_config *printer = devtag_get_device(space->machine, "centronics");
	UINT8 data=0x7f;

	/* bit 7 = printer busy
	0 = printer is not busy */

	data |= centronics_busy_r(printer) << 7;

	return data;
}


static READ8_HANDLER( exidy_read_ff ) { return 0xff; }

static ADDRESS_MAP_START( exidy_mem , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK(1)
	AM_RANGE(0x0800, 0x7fff) AM_RAM		/* ram 32k machine */
	AM_RANGE(0x8000, 0xbbff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xbc00, 0xbcff) AM_ROM		/* disk bios */
	AM_RANGE(0xbd00, 0xbdff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xbe00, 0xbe03) AM_READWRITE(exidy_wd179x_r, exidy_wd179x_w)
	AM_RANGE(0xbe04, 0xbfff) AM_READWRITE(exidy_read_ff, SMH_NOP)
	AM_RANGE(0xc000, 0xefff) AM_ROM		/* rom pac and bios */
	AM_RANGE(0xf000, 0xf7ff) AM_RAM		/* screen ram */
	AM_RANGE(0xf800, 0xfbff) AM_ROM		/* char rom */
	AM_RANGE(0xfc00, 0xffff) AM_RAM		/* programmable chars */
ADDRESS_MAP_END

static ADDRESS_MAP_START( exidyd_mem , ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x07ff) AM_RAMBANK(1)
	AM_RANGE(0x0800, 0xbfff) AM_RAM		/* ram 48k cassette-based machine */
	AM_RANGE(0xc000, 0xefff) AM_ROM		/* rom pac and bios */
	AM_RANGE(0xf000, 0xf7ff) AM_RAM		/* screen ram */
	AM_RANGE(0xf800, 0xfbff) AM_ROM		/* char rom */
	AM_RANGE(0xfc00, 0xffff) AM_RAM		/* programmable chars */
ADDRESS_MAP_END

static ADDRESS_MAP_START( exidy_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0xfc, 0xfc) AM_READWRITE( exidy_fc_port_r, exidy_fc_port_w )
	AM_RANGE(0xfd, 0xfd) AM_READWRITE( exidy_fd_port_r, exidy_fd_port_w )
	AM_RANGE(0xfe, 0xfe) AM_READWRITE( exidy_fe_port_r, exidy_fe_port_w )
	AM_RANGE(0xff, 0xff) AM_READWRITE( exidy_ff_port_r, exidy_ff_port_w )
ADDRESS_MAP_END

static INPUT_PORTS_START(exidy)
	PORT_START("VS")
	/* vblank */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK)

	/* line 0 */
	PORT_START("LINE0")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Control") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Graphic") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Stop") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	/* line 1 */
	PORT_START("LINE1")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(Sel)") PORT_CODE(KEYCODE_F2) PORT_CHAR(27)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Skip") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Repeat") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_F5) PORT_CHAR(12)
	/* line 2 */
	PORT_START("LINE2")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) PORT_CHAR('Q') PORT_CHAR('q') PORT_CHAR(0x11)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A') PORT_CHAR('a') PORT_CHAR(0x01)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z') PORT_CHAR('z') PORT_CHAR(0x1a)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) PORT_CHAR('X') PORT_CHAR('x') PORT_CHAR(0x18)
	/* line 3 */
	PORT_START("LINE3")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) PORT_CHAR('W') PORT_CHAR('w') PORT_CHAR(0x17)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S') PORT_CHAR('s') PORT_CHAR(0x13)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) PORT_CHAR('D') PORT_CHAR('d') PORT_CHAR(0x04)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) PORT_CHAR('C') PORT_CHAR('c') PORT_CHAR(0x03)
	/* line 4 */
	PORT_START("LINE4")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('E') PORT_CHAR('e') PORT_CHAR(0x05)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R') PORT_CHAR('r') PORT_CHAR(0x12)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) PORT_CHAR('F') PORT_CHAR('f') PORT_CHAR(0x06)
	/* line 5 */
	PORT_START("LINE5")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T') PORT_CHAR('t') PORT_CHAR(0x14)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) PORT_CHAR('G') PORT_CHAR('g') PORT_CHAR(0x07)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) PORT_CHAR('V') PORT_CHAR('v') PORT_CHAR(0x16)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B') PORT_CHAR('b') PORT_CHAR(0x02)
	/* line 6 */
	PORT_START("LINE6")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) PORT_CHAR('Y') PORT_CHAR('y') PORT_CHAR(0x19)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) PORT_CHAR('H') PORT_CHAR('h') PORT_CHAR(0x08)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N') PORT_CHAR('n') PORT_CHAR(0x0e)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M') PORT_CHAR('m') PORT_CHAR(0x0d)
	/* line 7 */
	PORT_START("LINE7")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U') PORT_CHAR('u') PORT_CHAR(0x15)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) PORT_CHAR('J') PORT_CHAR('j') PORT_CHAR(0x0a)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) PORT_CHAR('I') PORT_CHAR('i') PORT_CHAR(0x09)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) PORT_CHAR('K') PORT_CHAR('k') PORT_CHAR(0x0b)
	/* line 8 */
	PORT_START("LINE8")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) PORT_CHAR('O') PORT_CHAR('o') PORT_CHAR(0x0f)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) PORT_CHAR('L') PORT_CHAR('l') PORT_CHAR(0x0c)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	/* line 9 */
	PORT_START("LINE9")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P') PORT_CHAR('p') PORT_CHAR(0x10)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	/* line 10 */
	PORT_START("LINE10")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_QUOTE) PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[')  PORT_CHAR('{')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')  PORT_CHAR('}')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@ `") PORT_CODE(KEYCODE_F7) PORT_CHAR('@') PORT_CHAR('`')
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\')  PORT_CHAR('|')
	/* line 11 */
	PORT_START("LINE11")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("^ ~") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('^') PORT_CHAR('~')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LINE FEED") PORT_CODE(KEYCODE_F6) PORT_CHAR(10)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("_ Rub") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR('_') PORT_CHAR(8)
	/* line 12 */
	PORT_START("LINE12")
	PORT_BIT (0x10, 0x10, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("- (PAD)") PORT_CODE(KEYCODE_MINUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(MINUS_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/ (PAD)") PORT_CODE(KEYCODE_SLASH_PAD)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("* (PAD)") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+ (PAD)") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR(UCHAR_MAMEKEY(PLUS_PAD))
	/* line 13 */
	PORT_START("LINE13")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7 (PAD)") PORT_CODE(KEYCODE_7_PAD) PORT_CHAR(UCHAR_MAMEKEY(7_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8 (PAD)") PORT_CODE(KEYCODE_8_PAD) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4 (PAD)") PORT_CODE(KEYCODE_4_PAD) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT)) PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1 (PAD)") PORT_CODE(KEYCODE_1_PAD) PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0 (PAD)") PORT_CODE(KEYCODE_0_PAD) PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	/* line 14 */
	PORT_START("LINE14")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9 (PAD)") PORT_CODE(KEYCODE_9_PAD) PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6 (PAD)") PORT_CODE(KEYCODE_6_PAD) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5 (PAD)") PORT_CODE(KEYCODE_5_PAD) PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2 (PAD)") PORT_CODE(KEYCODE_2_PAD) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN)) PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". (PAD)") PORT_CODE(KEYCODE_DEL_PAD) PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	/* line 15 */
	PORT_START("LINE15")
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3 (PAD)") PORT_CODE(KEYCODE_3_PAD) PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= (PAD)") PORT_CODE(KEYCODE_NUMLOCK)
	PORT_BIT (0x04, 0x04, IPT_UNUSED)
	PORT_BIT (0x02, 0x02, IPT_UNUSED)
	PORT_BIT (0x01, 0x01, IPT_UNUSED)

	/* Enhanced options not available on real hardware */
	PORT_START("CONFIG")
	PORT_CONFNAME( 0x01, 0x01, "Autorun on Quickload")
	PORT_CONFSETTING(    0x00, DEF_STR(No))
	PORT_CONFSETTING(    0x01, DEF_STR(Yes))
	/* hardware connected to printer port */
	PORT_CONFNAME( 0x06, 0x00, "Parallel port" )
	PORT_CONFSETTING(    0x00, "Speaker" )
	PORT_CONFSETTING(    0x02, "Printer (7-bit)" )
	PORT_CONFSETTING(    0x04, "Printer (8-bit)" )
	PORT_CONFNAME( 0x08, 0x08, "Cassette Speaker")
	PORT_CONFSETTING(    0x08, DEF_STR(On))
	PORT_CONFSETTING(    0x00, DEF_STR(Off))
INPUT_PORTS_END

/**********************************************************************************************************/

static const ay31015_config exidy_ay31015_config =
{
	AY_3_1015,
	4800.0,
	4800.0,
	NULL,
	NULL,
	NULL
};


static const cassette_config exidy_cassette_config =
{
	cassette_default_formats,
	NULL,
	CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED
};

static MACHINE_DRIVER_START( exidy )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 12638000/6)
	MDRV_CPU_PROGRAM_MAP(exidy_mem)
	MDRV_CPU_IO_MAP(exidy_io)

	MDRV_MACHINE_START( exidy )
	MDRV_MACHINE_RESET( exidy )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(200))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(EXIDY_SCREEN_WIDTH, EXIDY_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, EXIDY_SCREEN_WIDTH-1, 0, EXIDY_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_UPDATE( exidy )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave.1", "cassette1")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	// cass1 speaker
	MDRV_SOUND_WAVE_ADD("wave.2", "cassette2")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)	// cass2 speaker
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)	// speaker on parallel port

	MDRV_AY31015_ADD( "ay_3_1015", exidy_ay31015_config )

	/* printer */
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)

	/* quickload */
	MDRV_Z80BIN_QUICKLOAD_ADD("quickload", exidy, 2)

	MDRV_CASSETTE_ADD( "cassette1", exidy_cassette_config )
	MDRV_CASSETTE_ADD( "cassette2", exidy_cassette_config )

	MDRV_WD179X_ADD("wd179x", default_wd17xx_interface )

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( exidyd )
	MDRV_IMPORT_FROM( exidy )

	MDRV_CPU_MODIFY("maincpu")
	MDRV_CPU_PROGRAM_MAP(exidyd_mem)

	MDRV_MACHINE_START( exidyd )
	MDRV_MACHINE_RESET( exidyd )
MACHINE_DRIVER_END

static DRIVER_INIT( exidy )
{
	UINT8 *RAM = memory_region(machine, "maincpu");
	memory_configure_bank(machine, 1, 0, 2, &RAM[0x0000], 0xe000);
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(exidy)
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("exmo1-1.dat", 0xe000, 0x0800, CRC(ac924f67) SHA1(72fcad6dd1ed5ec0527f967604401284d0e4b6a1) ) /* monitor roms */
	ROM_LOAD("exmo1-2.dat", 0xe800, 0x0800, CRC(ead1d0f6) SHA1(c68bed7344091bca135e427b4793cc7d49ca01be) )
	ROM_LOAD("exchr-1.dat", 0xf800, 0x0400, CRC(4a7e1cdd) SHA1(2bf07a59c506b6e0c01ec721fb7b747b20f5dced) ) /* char rom */
	ROM_LOAD_OPTIONAL("diskboot.dat",0xbc00, 0x0100, BAD_DUMP CRC(d82a40d6) SHA1(cd1ef5fb0312cd1640e0853d2442d7d858bc3e3b))
	ROM_CART_LOAD("cart", 0xc000, 0x2000, ROM_FILL_FF | ROM_OPTIONAL)

	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD_OPTIONAL("bruce.dat",   0x0000, 0x0020, CRC(fae922cb) SHA1(470a86844cfeab0d9282242e03ff1d8a1b2238d1)) /* video prom */
ROM_END

ROM_START(exidyd)
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("exmo1-1.dat", 0xe000, 0x0800, CRC(ac924f67) SHA1(72fcad6dd1ed5ec0527f967604401284d0e4b6a1) ) /* monitor roms */
	ROM_LOAD("exmo1-2.dat", 0xe800, 0x0800, CRC(ead1d0f6) SHA1(c68bed7344091bca135e427b4793cc7d49ca01be) )
	ROM_LOAD("exchr-1.dat", 0xf800, 0x0400, CRC(4a7e1cdd) SHA1(2bf07a59c506b6e0c01ec721fb7b747b20f5dced) ) /* char rom */
	ROM_CART_LOAD("cart", 0xc000, 0x2000, ROM_FILL_FF | ROM_OPTIONAL)

	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD_OPTIONAL("bruce.dat",   0x0000, 0x0020, CRC(fae922cb) SHA1(470a86844cfeab0d9282242e03ff1d8a1b2238d1)) /* video prom */
ROM_END

static Z80BIN_EXECUTE( exidy )
{
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if ((execute_address >= 0xc000) && (execute_address <= 0xdfff) && (memory_read_byte(space, 0xdffa) != 0xc3))
		return;					/* can't run a program if the cartridge isn't in */

	/* Since Exidy Basic is by Microsoft, it needs some preprocessing before it can be run.
	1. A start address of 01D5 indicates a basic program which needs its pointers fixed up.
	2. If autorunning, jump to C689 (command processor), else jump to C3DD (READY prompt).
	Important addresses:
		01D5 = start (load) address of a conventional basic program
		C858 = an autorun basic program will have this exec address on the tape
		C3DD = part of basic that displays READY and lets user enter input */

	if ((start_address == 0x1d5) || (execute_address == 0xc858))
	{
		UINT8 i, data[]={
			0xcd, 0x26, 0xc4,	// CALL C426	;set up other pointers
			0x21, 0xd4, 1,		// LD HL,01D4	;start of program address (used by C689)
			0x36, 0,		// LD (HL),00	;make sure dummy end-of-line is there
			0xc3, 0x89, 0xc6,};	// JP C689	;run program

		for (i = 0; i < ARRAY_LENGTH(data); i++)
			memory_write_byte(space, 0xf01f + i, data[i]);

		if (!autorun)
			memory_write_word_16le(space, 0xf028,0xc3dd);

		/* tell BASIC where program ends */
		memory_write_byte(space, 0x1b7, (end_address >> 0) & 0xff);
		memory_write_byte(space, 0x1b8, (end_address >> 8) & 0xff);

		if ((execute_address != 0xc858) && autorun)
			memory_write_word_16le(space, 0xf028, execute_address);

		cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_GENPC, 0xf01f);
	}
	else
	{
		if (autorun)
			cpu_set_reg(cputag_get_cpu(machine, "maincpu"), REG_GENPC, execute_address);
	}
}

static void exidy_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:			info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:			info->load = DEVICE_IMAGE_LOAD_NAME(exidy_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "dsk"); break;

		default:					legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}


static SYSTEM_CONFIG_START(exidy)
	CONFIG_DEVICE(exidy_floppy_getinfo)
SYSTEM_CONFIG_END


/*    YEAR  NAME    PARENT  COMPAT      MACHINE INPUT   INIT    CONFIG  COMPANY        FULLNAME */
COMP(1979, exidy,   0,		0,	exidy,	exidy,	exidy,	exidy,	"Exidy Inc", "Sorcerer", 0 )
COMP(1979, exidyd,  exidy,	0,	exidyd,	exidy,	exidy,	0,	"Exidy Inc", "Sorcerer (Cassette only)", 0 )

