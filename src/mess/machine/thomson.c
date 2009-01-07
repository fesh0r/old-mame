/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#include "driver.h"
#include "devices/thomflop.h"
#include "formats/thom_dsk.h"
#include "includes/thomson.h"

#define VERBOSE       0
#define VERBOSE_IRQ   0
#define VERBOSE_KBD   0  /* TO8 / TO9 / TO9+ keyboard */
#define VERBOSE_BANK  0
#define VERBOSE_VIDEO 0  /* video & lightpen */
#define VERBOSE_IO    0  /* serial & parallel I/O */
#define VERBOSE_MIDI  0

#define PRINT(x) mame_printf_info x

#define LOG(x)	do { if (VERBOSE) logerror x; } while (0)
#define VLOG(x)	do { if (VERBOSE > 1) logerror x; } while (0)
#define LOG_IRQ(x) do { if (VERBOSE_IRQ) logerror x; } while (0)
#define LOG_KBD(x) do { if (VERBOSE_KBD) logerror x; } while (0)
#define LOG_BANK(x) do { if (VERBOSE_BANK) logerror x; } while (0)
#define LOG_VIDEO(x) do { if (VERBOSE_VIDEO) logerror x; } while (0)
#define LOG_IO(x) do { if (VERBOSE_IO) logerror x; } while (0)
#define LOG_MIDI(x) do { if (VERBOSE_MIDI) logerror x; } while (0)

/* This set to 1 handle the .k7 files without passing through .wav */
/* It must be set accordingly in formats/thom_cas.c */
#define K7_SPEED_HACK 0

/********************* common cassette code ***************************/

INLINE const device_config* thom_cassette_img( running_machine *machine )
{ return device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette" ); }

/*-------------- TO7 ------------*/


/* On the TO7 & compatible (TO7/70,TO8,TO9, but not MO5,MO6), bits are coded
   in FM format with a 1.1 ms period (900 bauds):
   - 0 is 5 periods at 4.5 kHz
   - 1 is 7 periods at 6.3 kHz

   Moreover, a byte is represented using 11 bits:
   - one 0 start bit
   - eight data bits (low bit first)
   - two 1 stop bits

   There are also long (1 s) sequences of 1 bits to re-synchronize the
   cassette at places the motor can be cut off and back on (e.g., between
   files).

   The computer outputs a modulated wave that is directly put on the cassette.
   However, the input is demodulated by the cassette-reader before being
   sent to the computer: we got 0 when the signal is around 4.5 kHz and
   1 when the signal is around 6.3 kHz.
*/

#define TO7_BIT_LENGTH 0.001114

/* buffer storing demodulated bits, only for k7 and with speed hack */
static UINT32 to7_k7_bitsize;
static UINT8* to7_k7_bits;


/* 1-bit cassette input to the computer
   inside the controller, two frequency filters (adjusted to 6.3 and 4.5 kHz)
   and a comparator demodulate the raw signal into 0s and 1s.
*/
static int to7_get_cassette ( running_machine *machine )
{
	const device_config* img = thom_cassette_img(machine);

	if ( image_exists( img ) )
	{
		cassette_image* cass = cassette_get_image( img );
		cassette_state state = cassette_get_state( img );
		double pos = cassette_get_position( img );
		int bitpos = pos / TO7_BIT_LENGTH;

		if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED )
			return 1;

		if ( K7_SPEED_HACK && to7_k7_bits )
		{
			/* hack, feed existing bits */
			if ( bitpos >= to7_k7_bitsize )
				bitpos = to7_k7_bitsize -1;
			VLOG (( "$%04x %f to7_get_cassette: state=$%X pos=%f samppos=%i bit=%i\n",
				cpu_get_previouspc( machine->cpu[0] ), attotime_to_double(timer_get_time(machine)), state, pos, bitpos,
				to7_k7_bits[ bitpos ] ));
			return to7_k7_bits[ bitpos ];
		}
		else
		{
			/* demodulate wave signal on-the-fly */
			/* we simply count sign changes... */
			int k, chg;
			INT8 data[40];
			cassette_get_samples( cass, 0, pos, TO7_BIT_LENGTH * 15. / 14., 40, 1, data, 0 );

			for ( k = 1, chg = 0; k < 40; k++ )
			{
				if ( data[ k - 1 ] >= 0 && data[ k ] < 0 )
					chg++;
				if ( data[ k - 1 ] <= 0 && data[ k ] > 0 )
					chg++;
			}
			k = ( chg >= 13 ) ? 1 : 0;
			VLOG (( "$%04x %f to7_get_cassette: state=$%X pos=%f samppos=%i bit=%i (%i)\n",
				cpu_get_previouspc( machine->cpu[0] ), attotime_to_double(timer_get_time(machine)), state, pos, bitpos,
				k, chg ));
			return k;
		}

	}
	else
		return 0;
}



/* 1-bit cassette output */
static void to7_set_cassette ( running_machine *machine, int data )
{
	const device_config* img = thom_cassette_img(machine);
	cassette_output( img, data ? 1. : -1. );
}



static WRITE8_HANDLER ( to7_set_cassette_motor )
{
	const device_config* img = thom_cassette_img( space->machine );
	cassette_state state = cassette_get_state( img );
	double pos = cassette_get_position(img);

	LOG (( "$%04x %f to7_set_cassette_motor: cassette motor %s bitpos=%i\n",
	       cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(img->machine)), data ? "off" : "on",
	       (int) (pos / TO7_BIT_LENGTH) ));

	if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED && !data && pos > 0.3 )
	{
		/* rewind a little before starting the motor */
		cassette_seek( img, -0.3, SEEK_CUR );
	}

	cassette_change_state( img, data ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR );
}



/*-------------- MO5 ------------*/


/* Each byte is represented as 8 bits without start or stop bit (unlike TO7).
   Bits are coded in MFM, and the MFM signal is directly fed to the
   computer which has to decode it in software (unlike TO7).
   A 1 bit is one period at 1200 Hz; a 0 bit is one half-period at 600 Hz.
   Bit-order is most significant bit first (unlike TO7).

   Double-density MO6 cassettes follow the exact same mechanism, but with
   at double frequency (perdiods at 2400 Hz, and half-perdios at 1200 Hz).
*/


#define MO5_BIT_LENGTH   0.000833
#define MO5_HBIT_LENGTH (MO5_BIT_LENGTH / 2.)


static int mo5_get_cassette ( running_machine *machine )
{
	const device_config* img = thom_cassette_img(machine);

	if ( image_exists( img ) )
	{
		cassette_image* cass = cassette_get_image( img );
		cassette_state state = cassette_get_state( img );
		double pos = cassette_get_position( img );
		INT32 hbit;

		if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED )
			return 1;

		cassette_get_sample( cass, 0, pos, 0, &hbit );
		hbit = hbit >= 0;

		VLOG (( "$%04x %f mo5_get_cassette: state=$%X pos=%f hbitpos=%i hbit=%i\n",
			cpu_get_previouspc( machine->cpu[0] ), attotime_to_double(timer_get_time(machine)), state, pos,
			(int) (pos / MO5_HBIT_LENGTH), hbit ));
		return hbit;
	}
	else
		return 0;
}



static void mo5_set_cassette ( running_machine *machine, int data )
{
	const device_config* img = thom_cassette_img(machine);
	cassette_output( img, data ? 1. : -1. );
}



static WRITE8_HANDLER ( mo5_set_cassette_motor )
{
	const device_config* img = thom_cassette_img( space->machine );
	cassette_state state = cassette_get_state( img );
	double pos = cassette_get_position(img);

	LOG (( "$%04x %f mo5_set_cassette_motor: cassette motor %s hbitpos=%i\n",
	       cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ? "off" : "on",
	       (int) (pos / MO5_HBIT_LENGTH) ));

	if ( (state & CASSETTE_MASK_MOTOR) == CASSETTE_MOTOR_DISABLED &&  !data && pos > 0.3 )
	{
		/* rewind a little before starting the motor */
		cassette_seek( img, -0.3, SEEK_CUR );
	}

	cassette_change_state( img, data ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR );
}




/*************************** utilities ********************************/



/* ------------ IRQs ------------ */


/* several devices on the same irqs */

static UINT8 thom_irq;

static UINT8 thom_firq;



static void thom_set_irq ( running_machine *machine, int line, int state )
{
	int old = thom_irq;

	if ( state )
		thom_irq |= 1 << line;
	else
		thom_irq &= ~(1 << line);

	if ( !old && thom_irq )
		LOG_IRQ(( "%f thom_set_irq: irq line up %i\n", attotime_to_double(timer_get_time(machine)), line ));
	if ( old && !thom_irq )
		LOG_IRQ(( "%f thom_set_irq: irq line down %i\n", attotime_to_double(timer_get_time(machine)), line ));

	cpu_set_input_line(machine->cpu[0], M6809_IRQ_LINE, thom_irq ? ASSERT_LINE : CLEAR_LINE );
}



static void thom_set_firq ( running_machine *machine, int line, int state )
{
	int old = thom_irq;

	if ( state )
		thom_firq |= 1 << line;
	else
		thom_firq &= ~(1 << line);

	if ( !old && thom_firq )
		LOG_IRQ(( "%f thom_set_firq: firq line up %i\n", attotime_to_double(timer_get_time(machine)), line ));
	if ( old && !thom_firq )
		LOG_IRQ(( "%f thom_set_firq: firq line down %i\n", attotime_to_double(timer_get_time(machine)), line ));

	cpu_set_input_line( machine->cpu[0], M6809_FIRQ_LINE, thom_firq ? ASSERT_LINE : CLEAR_LINE );
}



static void thom_irq_reset ( running_machine *machine )
{
	thom_irq = 0;
	thom_firq = 0;
	cpu_set_input_line( machine->cpu[0], M6809_IRQ_LINE, CLEAR_LINE );
	cpu_set_input_line( machine->cpu[0], M6809_FIRQ_LINE, CLEAR_LINE );
}



static void thom_irq_init ( running_machine *machine )
{
	state_save_register_global( machine, thom_irq );
	state_save_register_global( machine, thom_firq );
}



static void thom_irq_0  ( running_machine *machine, int state )
{
	thom_set_irq  ( machine, 0, state );
}

static void thom_dev_irq_0  ( const device_config *device, int state )
{
	thom_irq_0( device->machine, state );
}



static void thom_irq_1  ( running_machine *machine, int state )
{
	thom_set_irq  ( machine, 1, state );
}

static void thom_irq_3  ( running_machine *machine, int state )
{
	thom_set_irq  ( machine, 3, state );
}

static void thom_firq_1 ( running_machine *machine, int state )
{
	thom_set_firq ( machine, 1, state );
}

static void thom_firq_2 ( running_machine *machine, int state )
{
	thom_set_firq ( machine, 2, state );
}

#ifdef CHARDEV
static void thom_irq_4  ( running_machine *machine, int state )
{
	thom_set_irq  ( machine, 4, state );
}
#endif


/*
   current IRQ usage:

   line 0 => 6846 interrupt
   line 1 => 6821 interrupts (shared for all 6821)
   line 2 => TO8 lightpen interrupt (from gate-array)
   line 3 => TO9 keyboard interrupt (from 6850 ACIA)
   line 4 => MIDI interrupt (from 6850 ACIA)
*/



/* ------------ LED ------------ */



static void thom_set_caps_led ( running_machine *machine, int led )
{
	output_set_value( "led0", led );
}



/* ------------ serial (not functionnal yet) ------------ */



static struct serial_connection to7_io_line;



DEVICE_START( thom_serial )
{
	const device_config *acia = device_list_find_by_tag(device->machine->config->devicelist,
		ACIA6551,
		"acia");
	int idx = image_index_in_device(device);

	DEVICE_START_CALL(serial_device);

	switch ( idx ) {

	case THOM_SERIAL_CC90323:
		LOG(( "thom_serial_init: init CD 90-320 RS232 device\n" ));
		serial_device_connect( device, &to7_io_line );
		break;

	case THOM_SERIAL_RF57232:
		LOG(( "thom_serial_init: init RF 57-232 RS232 device\n" ));
		acia_6551_connect_to_serial_device( acia, device );
		break;

	case THOM_SERIAL_MODEM:
		LOG(( "thom_serial_init: init MODEM (MD 90-120) device\n" ));
		break;

	default:
		fatalerror( "thom_serial_init: unknown serial device index %i\n", idx );
		break;
	}

	serial_device_setup( device, 2400, 7, 2, SERIAL_PARITY_NONE ); /* default */
	serial_device_set_protocol( device, SERIAL_PROTOCOL_NONE );
	serial_device_set_transmit_state( device, 1 );
	return DEVICE_START_OK;
}



DEVICE_IMAGE_UNLOAD( thom_serial )
{
	device_unload_serial_device( image );
}



DEVICE_IMAGE_LOAD( thom_serial )
{
	int idx = image_index_in_device( image );

	if ( device_load_serial_device( image ) != INIT_PASS )
	{
		logerror( "thom_serial_load: could not load serial image in device %i\n", idx );
		return INIT_FAIL;
	}

	return INIT_PASS;
}



/* ------------ 6850 defines ------------ */

#define ACIA_6850_RDRF  0x01    /* Receive data register full */
#define ACIA_6850_TDRE  0x02    /* Transmit data register empty */
#define ACIA_6850_dcd   0x04    /* Data carrier detect, active low */
#define ACIA_6850_cts   0x08    /* Clear to send, active low */
#define ACIA_6850_FE    0x10    /* Framing error */
#define ACIA_6850_OVRN  0x20    /* Receiver overrun */
#define ACIA_6850_PE    0x40    /* Parity error */
#define ACIA_6850_irq   0x80    /* Interrupt request, active low */



/***************************** TO7 / T9000 *************************/



/* ------------ cartridge ------------ */

static UINT8 thom_cart_nb_banks; /* number of 16 KB banks (up to 4) */

static UINT8 thom_cart_bank;     /* current bank */


DEVICE_IMAGE_LOAD( to7_cartridge )
{
	int i,j;
	UINT8* pos = memory_region(image->machine,  "main" ) + 0x10000;
	offs_t size = image_length ( image );
	char name[129];

	/* get size & number of 16-KB banks */
	if ( size <= 0x04000 )
		thom_cart_nb_banks = 1;
	else if ( size == 0x08000 )
		thom_cart_nb_banks = 2;
	else if ( size == 0x10000 )
		thom_cart_nb_banks = 4;
	else
	{
		logerror( "to7_cartridge_load: invalid cartridge size %i\n", size );
		return INIT_FAIL;
	}

	if ( image_fread( image, pos, size ) != size )
	{
		logerror( "to7_cartridge_load: read error\n" );
		return INIT_FAIL;
	}

	/* extract name */
	for ( i = 0; i < size && pos[i] != ' '; i++ );
	for ( i++, j = 0; i + j < size && j < 128 && pos[i+j] >= 0x20; j++)
		name[j] = pos[i+j];
	name[j] = 0;

	/* sanitize name */
	for ( i = 0; name[i]; i++)
	{
		if ( name[i] < ' ' || name[i] >= 127 )
			name[i] = '?';
	}

	PRINT (( "to7_cartridge_load: cartridge \"%s\" banks=%i, size=%i\n", name, thom_cart_nb_banks, size ));

	return INIT_PASS;
}



static void to7_update_cart_bank(running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	static int old_bank = -1;
	int bank = 0;
	if ( thom_cart_nb_banks )
	{
		bank = thom_cart_bank % thom_cart_nb_banks;
		memory_install_read8_handler(space, 0x0000, 0x0003, 0, 0, to7_cartridge_r );
	}

	if ( bank != old_bank )
		LOG_BANK(( "to7_update_cart_bank: CART is cartridge bank %i\n", bank ));

	memory_set_bank( machine, THOM_CART_BANK, bank );
	old_bank = bank;
}



static STATE_POSTLOAD( to7_update_cart_bank_postload )
{
	to7_update_cart_bank(machine);
}



/* write signal to 0000-1fff generates a bank switch */
WRITE8_HANDLER ( to7_cartridge_w )
{
	if ( offset >= 0x2000 )
		return;

	thom_cart_bank = offset & 3;
	to7_update_cart_bank(space->machine);
}



/* read signal to 0000-0003 generates a bank switch */
READ8_HANDLER ( to7_cartridge_r )
{
	UINT8* pos = memory_region( space->machine, "main" ) + 0x10000;
	UINT8 data = pos[offset + (thom_cart_bank % thom_cart_nb_banks) * 0x4000];
	thom_cart_bank = offset & 3;
	to7_update_cart_bank(space->machine);
	return data;
}



/* ------------ 6846 (timer, I/O) ------------ */



static WRITE8_DEVICE_HANDLER ( to7_timer_port_out )
{
	thom_set_mode_point( device->machine, data & 1 );          /* bit 0: video bank switch */
	thom_set_caps_led( device->machine, (data & 8) ? 1 : 0 ) ; /* bit 3: keyboard led */
	thom_set_border_color( device->machine, 
			       ((data & 0x10) ? 1 : 0) |           /* bits 4-6: border color */
			       ((data & 0x20) ? 2 : 0) |
			       ((data & 0x40) ? 4 : 0) );
}



static WRITE8_DEVICE_HANDLER ( to7_timer_cp2_out )
{
	dac_data_w( THOM_SOUND_BUZ, data ? 0x80 : 0); /* 1-bit buzzer */
}



static READ8_DEVICE_HANDLER ( to7_timer_port_in )
{
	int lightpen = (input_port_read(device->machine, "lightpen_button") & 1) ? 2 : 0;
	int cass = to7_get_cassette(device->machine) ? 0x80 : 0;
	return lightpen | cass;
}



static WRITE8_DEVICE_HANDLER ( to7_timer_tco_out )
{
	/* 1-bit cassette output */
	to7_set_cassette( device->machine, data );
}



mc6846_interface to7_timer =
{
	to7_timer_port_out, NULL, to7_timer_cp2_out,
	to7_timer_port_in, to7_timer_tco_out,
	thom_dev_irq_0
};



/* ------------ lightpen automaton ------------ */



static UINT8 to7_lightpen_step;

static UINT8 to7_lightpen;



static void to7_lightpen_cb ( running_machine *machine, int step )
{
	if ( ! to7_lightpen )
		return;

	LOG_VIDEO(( "%f to7_lightpen_cb: step=%i\n", attotime_to_double(timer_get_time(machine)), step ));
	pia_set_input_cb1( THOM_PIA_SYS, 1 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );
	to7_lightpen_step = step;
}



/* ------------ video ------------ */



static void to7_set_init ( running_machine *machine, int init )
{
	/* INIT signal wired to system PIA 6821 */

	LOG_VIDEO(( "%f to7_set_init: init=%i\n", attotime_to_double(timer_get_time(machine)), init ));
	pia_set_input_ca1( THOM_PIA_SYS, init );
}



/* ------------ system PIA 6821 ------------ */



static WRITE8_HANDLER ( to7_sys_cb2_out )
{
	to7_lightpen = !data;
}



static WRITE8_HANDLER ( to7_sys_portb_out )
{
	/* value fetched in to7_sys_porta_in */
}



#define TO7_LIGHTPEN_DECAL 17 /* horizontal lightpen shift, stored in $60D2 */



static READ8_HANDLER ( to7_sys_porta_in )
{
	if ( to7_lightpen )
	{
		/* lightpen hi */
		return to7_lightpen_gpl( space->machine, TO7_LIGHTPEN_DECAL, to7_lightpen_step ) >> 8;
	}
	else
	{
		/* keyboard  */
		int keyline = pia_get_output_b( THOM_PIA_SYS );
		UINT8 val = 0xff;
		int i;
		static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", 
											"keyboard_4", "keyboard_5", "keyboard_6", "keyboard_7" };
		
		for ( i = 0; i < 8; i++ )
		{
			if ( ! (keyline & (1 << i)) )
				val &= input_port_read(space->machine, keynames[i]);
		}
		return val;
	}
}



static READ8_HANDLER ( to7_sys_portb_in )
{
	/* lightpen low */
	return to7_lightpen_gpl( space->machine, TO7_LIGHTPEN_DECAL, to7_lightpen_step ) & 0xff;
}



static const pia6821_interface to7_sys =
{
	to7_sys_porta_in, to7_sys_portb_in,
	NULL, NULL, NULL, NULL,
	NULL, to7_sys_portb_out, to7_set_cassette_motor, to7_sys_cb2_out,
	thom_firq_1, thom_firq_1
};



/* ------------ CC 90-232 I/O extension ------------ */

/* Features:
   - 6821 PIA
   - serial RS232: bit-banging?
   - parallel CENTRONICS: a printer (-prin) is emulated
   - usable on TO7(/70), MO5(E) only; not on TO9 and higher

   Note: it seems impossible to connect both a serial & a parallel device
   because the Data Transmit Ready bit is shared in an incompatible way!
*/



typedef enum
{
	TO7_IO_NONE,
	TO7_IO_CENTRONICS,
	TO7_IO_RS232
} to7_io_dev;

static const device_config *printer_device(running_machine *machine)
{
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, "printer");
}

/* test whether a parallel or a serial device is connected: both cannot
   be exploited at the same time!
*/
static to7_io_dev to7_io_mode( running_machine *machine )
{
	if ( printer_is_ready(printer_device(machine) ) )
		return TO7_IO_CENTRONICS;
	else if ( image_exists( image_from_devtype_and_index( machine, IO_SERIAL, THOM_SERIAL_CC90323 ) ) )
		return TO7_IO_RS232;
	return TO7_IO_NONE;
}



static void to7_io_ack ( running_machine *machine, int n, int data, int mask )
{
	/* acknowledge from centronics printer to PIA */
	int ack = (data & CENTRONICS_ACKNOWLEDGE) ? 1 : 0;
	//LOG_IO (( "%f to7_io_ack: CENTRONICS new state $%02X (ack=%i)\n", attotime_to_double(timer_get_time(machine)), data, ack ));
	pia_set_input_cb1( THOM_PIA_IO, ack );
}



static WRITE8_HANDLER ( to7_io_porta_out )
{
	int tx  = data & 1;
	int dtr = ( data & 2 ) ? 1 : 0;

	LOG_IO(( "$%04x %f to7_io_porta_out: tx=%i, dtr=%i\n",  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), tx, dtr ));
	if ( dtr )
		to7_io_line.State |=  SERIAL_STATE_DTR;
	else
		to7_io_line.State &= ~SERIAL_STATE_DTR;

	set_out_data_bit( to7_io_line.State, tx );
	serial_connection_out( space->machine, &to7_io_line );
}



static READ8_HANDLER ( to7_io_porta_in )
{
	int cts = 1;
	int dsr = ( to7_io_line.input_state & SERIAL_STATE_DSR ) ? 0 : 1;
	int rd  = get_in_data_bit( to7_io_line.input_state );

	if ( to7_io_mode(space->machine) == TO7_IO_RS232 )
		cts = to7_io_line.input_state & SERIAL_STATE_CTS ? 0 : 1;
	else
		cts = ( centronics_read_handshake( space->machine, 0 ) & CENTRONICS_NOT_BUSY ) ? 1 : 0;

	LOG_IO(( "$%04x %f to7_io_porta_in: mode=%i cts=%i, dsr=%i, rd=%i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), to7_io_mode(space->machine), cts, dsr, rd ));

	return (dsr ? 0x20 : 0) | (cts ? 0x40 : 0) | (rd ? 0x80: 0);
}



static WRITE8_HANDLER ( to7_io_portb_out )
{
	/* set 8-bit data */

	LOG_IO(( "$%04x %f to7_io_portb_out: CENTRONICS set data=$%02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));

	centronics_write_data( space->machine, 0, data );
}



static WRITE8_HANDLER ( to7_io_cb2_out )
{
	/* send STROBE to printer */

	LOG_IO(( "$%04x %f to7_io_cb2_out: CENTRONICS set strobe=%i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));

	centronics_write_handshake( space->machine, 0, data ? CENTRONICS_STROBE : 0, CENTRONICS_STROBE );
}



static void to7_io_in_cb ( running_machine *machine, int id, unsigned long state )
{
	/* our peer's state has changed */
	to7_io_line.input_state = state;

	LOG_IO(( "%f to7_io_in_callback:  cts=%i dsr=%i rd=%i\n", attotime_to_double(timer_get_time(machine)), (state & SERIAL_STATE_CTS) ? 1 : 0, (state & SERIAL_STATE_DSR) ? 1 : 0, (int)get_in_data_bit( state ) ));
}



static const pia6821_interface to7_io =
{
	to7_io_porta_in, NULL, NULL, NULL, NULL, NULL,
	to7_io_porta_out, to7_io_portb_out, NULL, to7_io_cb2_out,
	thom_firq_1, thom_firq_1
};



static const CENTRONICS_CONFIG to7_centronics = { PRINTER_CENTRONICS, to7_io_ack };



static void thom_centronics_reset( running_machine *machine )
{
	centronics_write_handshake
		( machine, 0,
		  CENTRONICS_SELECT | CENTRONICS_NOT_BUSY | CENTRONICS_STROBE,
		  CENTRONICS_SELECT | CENTRONICS_NO_RESET | CENTRONICS_NOT_BUSY |
		  CENTRONICS_STROBE | CENTRONICS_ACKNOWLEDGE );

	centronics_write_handshake ( machine, 0, CENTRONICS_NO_RESET, CENTRONICS_NO_RESET );
}



static void to7_io_reset( running_machine *machine )
{
	LOG (( "to7_io_reset called\n" ));
	thom_centronics_reset(machine);
	/* pia_reset() is called in MACHINE_RESET */
	pia_set_port_a_z_mask( THOM_PIA_IO, 0x03 );
	to7_io_line.input_state = SERIAL_STATE_CTS;
	to7_io_line.State &= ~SERIAL_STATE_DTR;
	to7_io_line.State |=  SERIAL_STATE_RTS;  /* always ready to send */
	set_out_data_bit( to7_io_line.State, 1 );
	serial_connection_out( machine, &to7_io_line );
}



static void to7_io_init( running_machine *machine )
{
	LOG (( "to7_io_init: CC 90-323 serial / parallel extension\n" ));
	pia_config( machine, THOM_PIA_IO, &to7_io );
	centronics_config( machine, 0, &to7_centronics );
	serial_connection_init( machine, &to7_io_line );
	serial_connection_set_in_callback( machine, &to7_io_line, to7_io_in_cb );
}



/* ------------ RF 57-932 RS232 extension ------------ */

/* Features:
   - SY 6551 ACIA.
   - higher transfer rates than the CC 90-232
   - usable on all computer, including TO9 and higher
 */



/* ------------  MD 90-120 MODEM extension (not functional) ------------ */

/* Features:
   - 6850 ACIA
   - 6821 PIA
   - asymetric 1200/ 75 bauds (reversable)

   TODO!
 */



static UINT8 to7_modem_rx;

static UINT8 to7_modem_tx;



static void to7_modem_cb( const device_config *device, int state )
{
	LOG(( "to7_modem_cb: called %i\n", state ));
}



static const pia6821_interface to7_pia_modem =
{
	NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL
};



acia6850_interface to7_modem =
{
	1200, 1200, /* 1200 bauds, might be divided by 16 */
	&to7_modem_rx, &to7_modem_tx,
	NULL, NULL, NULL,
	to7_modem_cb
};



static void to7_modem_reset( running_machine *machine )
{
	LOG (( "to7_modem_reset called\n" ));
	to7_modem_rx = 0;
	to7_modem_tx = 0;
	/* pia_reset() is called in MACHINE_RESET */
	/* acia_6850 has no reset (?) */
}



static void to7_modem_init( running_machine *machine )
{
	LOG (( "to7_modem_init: MODEM not implemented!\n" ));
	pia_config( machine, THOM_PIA_MODEM, &to7_pia_modem );
	state_save_register_global( machine, to7_modem_rx );
	state_save_register_global( machine, to7_modem_tx );
}



/* ------------  dispatch MODEM / speech extension ------------ */


mea8000_interface to7_speech = { THOM_SOUND_SPEECH, NULL };


READ8_HANDLER ( to7_modem_mea8000_r )
{
	if ( input_port_read(space->machine, "mconfig") & 1 ) 
	{
		const device_config* device = devtag_get_device(space->machine, MEA8000, "mea8000" );
		return mea8000_r( device, offset );
	}
	else
	{	
		const device_config* device = devtag_get_device(space->machine, ACIA6850, "acia6850" );
		switch (offset) {
		case 0: return acia6850_stat_r( device, offset );
		case 1: return acia6850_data_r( device, offset );
		default: return 0;
		}
	}
}



WRITE8_HANDLER ( to7_modem_mea8000_w )
{
	if ( input_port_read(space->machine, "mconfig") & 1 )
	{
		const device_config* device = devtag_get_device(space->machine, MEA8000, "mea8000" );
		mea8000_w( device, offset, data );
	}
	else
	{
		const device_config* device = devtag_get_device(space->machine, ACIA6850, "acia6850" );		
		switch (offset) {
		case 0: acia6850_ctrl_w( device, offset, data );
		case 1: acia6850_data_w( device, offset, data );
		}
	}
}



/* ------------ SX 90-018 (model 2) music & game extension ------------ */

/* features:
   - 6821 PIA
   - two 8-position, 2-button game pads
   - 2-button mouse (exclusive with pads)
     do not confuse with the TO9-specific mouse
   - 6-bit DAC sound

   extends the CM 90-112 (model 1) with one extra button per pad and a mouse
*/



#define TO7_GAME_POLL_PERIOD  ATTOTIME_IN_USEC( 500 )



/* calls to7_game_update_cb periodically */
static emu_timer* to7_game_timer;

static UINT8 to7_game_sound;
static UINT8 to7_game_mute;


/* The mouse is a very simple phase quadrature one.
   Each axis provides two 1-bit signals, A and B, that are toggled by the
   axis rotation. The two signals are not in phase, so that whether A is
   toggled before B or B before A gives the direction of rotation.
   This is similar Atari & Amiga mouses.
   Returns: 0 0 0 0 0 0 YB XB YA XA
 */
static UINT8 to7_get_mouse_signal( running_machine *machine )
{
	UINT8 xa, xb, ya, yb;
	UINT16 dx = input_port_read(machine, "mouse_x"); /* x axis */
	UINT16 dy = input_port_read(machine, "mouse_y"); /* y axis */
	xa = ((dx + 1) & 3) <= 1;
	xb = (dx & 3) <= 1;
	ya = ((dy + 1) & 3) <= 1;
	yb = (dy & 3) <= 1;
	return xa | (ya << 1) | (xb << 2) | (yb << 3);
}



static void to7_game_sound_update ( running_machine *machine )
{
	dac_data_w( THOM_SOUND_GAME, to7_game_mute ? 0 : (to7_game_sound << 2) );
}



static READ8_HANDLER ( to7_game_porta_in )
{
	UINT8 data;
	if ( input_port_read(space->machine, "config") & 1 )
	{
		/* mouse */
		data = to7_get_mouse_signal(space->machine) & 0x0c;             /* XB, YB */
		data |= input_port_read(space->machine, "mouse_button") & 3; /* buttons */
	}
	else
	{
		/* joystick */
		data = input_port_read(space->machine, "game_port_directions");
		/* bit 0=0 => P1 up      bit 4=0 => P2 up
		   bit 1=0 => P1 down    bit 5=0 => P2 down
		   bit 2=0 => P1 left    bit 6=0 => P2 left
		   bit 3=0 => P1 right   bit 7=0 => P2 right
		*/
		/* remove impossible combinations: up+down, left+right */
		if ( ! ( data & 0x03 ) )
			data |= 0x03;
		if ( ! ( data & 0x0c ) )
			data |= 0x0c;
		if ( ! ( data & 0x30 ) )
			data |= 0x30;
		if ( ! ( data & 0xc0 ) )
			data |= 0xc0;
		if ( ! ( data & 0x03 ) )
			data |= 0x03;
		if ( ! ( data & 0x0c ) )
			data |= 0x0c;
		if ( ! ( data & 0x30 ) )
			data |= 0x30;
		if ( ! ( data & 0xc0 ) )
			data |= 0xc0;
	}
	return data;
}



static READ8_HANDLER ( to7_game_portb_in )
{
	UINT8 data;
	if ( input_port_read(space->machine, "config") & 1 )
	{
		/* mouse */
		UINT8 mouse =  to7_get_mouse_signal(space->machine);
		data = 0;
		if ( mouse & 1 )
			data |= 0x04; /* XA */
		if ( mouse & 2 )
			data |= 0x40; /* YA */
	}
	else
	{
		/* joystick */
		/* bits 6-7: action buttons A (0=pressed) */
		/* bits 2-3: action buttons B (0=pressed) */
		/* bits 4-5: unused (ouput) */
		/* bits 0-1: unknown! */
		data = input_port_read(space->machine, "game_port_buttons");
	}
	return data;
}



static WRITE8_HANDLER ( to7_game_portb_out )
{
	/* 6-bit DAC sound */
	to7_game_sound = data & 0x3f;
	to7_game_sound_update(space->machine);
}



static WRITE8_HANDLER ( to7_game_cb2_out )
{
	/* undocumented */
	/* some TO8 games (e.g.: F15) seem to write here a lot */
}



static const pia6821_interface to7_game =
{
	to7_game_porta_in, to7_game_portb_in,
	NULL, NULL, NULL, NULL,
	NULL, to7_game_portb_out, NULL, to7_game_cb2_out,
	thom_irq_1, thom_irq_1
};



/* this should be called periodically */
static TIMER_CALLBACK(to7_game_update_cb)
{
	if ( input_port_read(machine, "config") & 1 )
	{
		/* mouse */
		UINT8 mouse = to7_get_mouse_signal(machine);
		pia_set_input_ca1( THOM_PIA_GAME, (mouse & 1) ? 1 : 0 ); /* XA */
		pia_set_input_ca2( THOM_PIA_GAME, (mouse & 2) ? 1 : 0 ); /* YA */
	}
	else
	{
		/* joystick */
		UINT8 in = input_port_read(machine, "game_port_buttons");
		pia_set_input_cb2( THOM_PIA_GAME, (in & 0x80) ? 1 : 0 ); /* P2 action A */
		pia_set_input_ca2( THOM_PIA_GAME, (in & 0x40) ? 1 : 0 ); /* P1 action A */
		pia_set_input_cb1( THOM_PIA_GAME, (in & 0x08) ? 1 : 0 ); /* P2 action B */
		pia_set_input_ca1( THOM_PIA_GAME, (in & 0x04) ? 1 : 0 ); /* P1 action B */
		/* TODO:
		   it seems that CM 90-112 behaves differently
		   - ca1 is P1 action A, i.e., in & 0x40
		   - ca2 is P2 action A, i.e., in & 0x80
		   - cb1, cb2 are not connected (should not be a problem)
		*/
		/* Note: the MO6 & MO5NR have slightly different connections
		   (see mo6_game_update_cb)
		*/
	}
}



static void to7_game_init ( running_machine *machine )
{
	LOG (( "to7_game_init called\n" ));
	pia_config( machine, THOM_PIA_GAME, &to7_game );
	to7_game_timer = timer_alloc( machine, to7_game_update_cb , NULL);
	timer_adjust_periodic(to7_game_timer, TO7_GAME_POLL_PERIOD, 0, TO7_GAME_POLL_PERIOD);
	state_save_register_global( machine, to7_game_sound );
	state_save_register_global( machine, to7_game_mute );
}



static void to7_game_reset ( running_machine *machine )
{
	LOG (( "to7_game_reset called\n" ));
	pia_set_input_ca1( THOM_PIA_GAME, 0 );
	to7_game_sound = 0;
	to7_game_mute = 0;
	to7_game_sound_update( machine );
}



/* ------------ MIDI extension ------------ */

/* IMPORTANT NOTE:
   The following is experimental and not compiled in by default.
   It relies on the existence of an hypothetical "character device" API able
   to transmit bytes between the MESS driver and the outside world
   (using, e.g., character device special files on some UNIX).
*/

#ifdef CHARDEV

#include "devices/chardev.h"

/* Features an EF 6850 ACIA

   MIDI protocol is a serial asynchronous protocol
   Each 8-bit byte is transmitted as:
   - 1 start bit
   - 8 data bits
   - 1 stop bits
   320 us per transmitted byte => 31250 baud

   Emulation is based on the Motorola 6850 documentation, not EF 6850.

   We do not emulate the seral line but pass bytes directly between the
   6850 registers and the MIDI device.
*/


static UINT8 to7_midi_status;   /* 6850 status word */
static UINT8 to7_midi_overrun;  /* pending overrun */
static UINT8 to7_midi_intr;     /* enabled interrupts */

static chardev* to7_midi_chardev;



static void to7_midi_update_irq ( running_machine *machine )
{
	if ( (to7_midi_intr & 4) && (to7_midi_status & ACIA_6850_RDRF) )
		to7_midi_status |= ACIA_6850_irq; /* byte received interrupt */

	if ( (to7_midi_intr & 4) && (to7_midi_status & ACIA_6850_OVRN) )
		to7_midi_status |= ACIA_6850_irq; /* overrun interrupt */

	if ( (to7_midi_intr & 3) == 1 && (to7_midi_status & ACIA_6850_TDRE) )
		to7_midi_status |= ACIA_6850_irq; /* ready to transmit interrupt */

	thom_irq_4( machine, to7_midi_status & ACIA_6850_irq );
}



static void to7_midi_byte_received_cb( running_machine *machine, chardev_err s )
{
	to7_midi_status |= ACIA_6850_RDRF;
	if ( s == CHARDEV_OVERFLOW )
		to7_midi_overrun = 1;
	to7_midi_update_irq( machine );
}



static void to7_midi_ready_to_send_cb( running_machine *machine )
{
	to7_midi_status |= ACIA_6850_TDRE;
	to7_midi_update_irq( machine );
}



READ8_HANDLER ( to7_midi_r )
{
	/* ACIA 6850 registers */

	switch ( offset )
	{

	case 0: /* get status */
		/* bit 0:     data received */
		/* bit 1:     ready to transmit data */
		/* bit 2:     data carrier detect (ignored) */
		/* bit 3:     clear to send (ignored) */
		/* bit 4:     framing error (ignored) */
		/* bit 5:     overrun */
		/* bit 6:     parity error (ignored) */
		/* bit 7:     interrupt */
		LOG_MIDI(( "$%04x %f to7_midi_r: status $%02X (rdrf=%i, tdre=%i, ovrn=%i, irq=%i)\n",
			  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), to7_midi_status,
			  (to7_midi_status & ACIA_6850_RDRF) ? 1 : 0,
			  (to7_midi_status & ACIA_6850_TDRE) ? 1 : 0,
			  (to7_midi_status & ACIA_6850_OVRN) ? 1 : 0,
			  (to7_midi_status & ACIA_6850_irq) ? 1 : 0 ));
		return to7_midi_status;


	case 1: /* get input data */
	{
		UINT8 data = chardev_in( to7_midi_chardev );
		to7_midi_status &= ~(ACIA_6850_irq | ACIA_6850_RDRF);
		if ( to7_midi_overrun )
			to7_midi_status |= ACIA_6850_OVRN;
		else
			to7_midi_status &= ~ACIA_6850_OVRN;
		to7_midi_overrun = 0;
		LOG_MIDI(( "$%04x %f to7_midi_r: read data $%02X\n",
			  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
		to7_midi_update_irq( space->machine );
		return data;
	}


	default:
		logerror( "$%04x to7_midi_r: invalid offset %i\n",
			  cpu_get_previouspc( space->machine->cpu[0] ),  offset );
		return 0;
	}
}



WRITE8_HANDLER ( to7_midi_w )
{
	/* ACIA 6850 registers */

	switch ( offset )
	{

	case 0: /* set control */
		/* bits 0-1: clock divide (ignored) or reset */
		if ( (data & 3) == 3 )
		{
			/* reset */
			LOG_MIDI(( "$%04x %f to7_midi_w: reset (data=$%02X)\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
			to7_midi_overrun = 0;
			to7_midi_status = 2;
			to7_midi_intr = 0;
			chardev_reset( to7_midi_chardev );
		}
		else
		{
			/* bits 2-4: parity  */
			/* bits 5-6: interrupt on transmit */
			/* bit 7:    interrupt on receive */
			to7_midi_intr = data >> 5;
			{
				static const int bits[8] = { 7,7,7,7,8,8,8,8 };
				static const int stop[8] = { 2,2,1,1,2,1,1,1 };
				static const char parity[8] = { 'e','o','e','o','-','-','e','o' };
				LOG_MIDI(( "$%04x %f to7_midi_w: set control to $%02X (bits=%i, stop=%i, parity=%c, intr in=%i out=%i)\n",
					  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
					  data,
					  bits[ (data >> 2) & 7 ],
					  stop[ (data >> 2) & 7 ],
					  parity[ (data >> 2) & 7 ],
					  to7_midi_intr >> 2,
					  (to7_midi_intr & 3) ? 1 : 0));
			}
		}
		to7_midi_update_irq( space->machine );
		break;


	case 1: /* output data */
		LOG_MIDI(( "$%04x %f to7_midi_w: write data $%02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
		if ( data == 0x55 )
			/* cable-detect: shortcut */
			chardev_fake_in( to7_midi_chardev, 0x55 );
		else
		{
			/* send to MIDI */
			to7_midi_status &= ~(ACIA_6850_irq | ACIA_6850_TDRE);
			chardev_out( to7_midi_chardev, data );
		}
		break;


	default:
		logerror( "$%04x to7_midi_w: invalid offset %i (data=$%02X) \n", cpu_get_previouspc( space->machine->cpu[0] ), offset, data );
	}
}



static const chardev_interface to7_midi_interface =
{
	to7_midi_byte_received_cb,
	to7_midi_ready_to_send_cb,
};



static void to7_midi_reset( running_machine *machine )
{
	LOG (( "to7_midi_reset called\n" ));
	to7_midi_overrun = 0;
	to7_midi_status = 0;
	to7_midi_intr = 0;
	chardev_reset( to7_midi_chardev );
}



static void to7_midi_init( running_machine *machine )
{
	LOG (( "to7_midi_init\n" ));
	to7_midi_chardev = chardev_open( machine, "/dev/snd/midiC1D0", "/dev/snd/midiC1D1", &to7_midi_interface );
	state_save_register_global( machine, to7_midi_status );
	state_save_register_global( machine, to7_midi_overrun );
	state_save_register_global( machine, to7_midi_intr );
}



#else



READ8_HANDLER ( to7_midi_r )
{
	logerror( "to7_midi_r: not implemented\n" );
	return 0;
}



WRITE8_HANDLER ( to7_midi_w )
{
	logerror( "to7_midi_w: not implemented\n" );
}



static void to7_midi_reset( running_machine *machine )
{
	logerror( "to7_midi_reset: not implemented\n" );
}



static void to7_midi_init( running_machine *machine )
{
	logerror( "to7_midi_init: not implemented\n" );
}



#endif



/* ------------ init / reset ------------ */



MACHINE_RESET ( to7 )
{
	LOG (( "to7: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	to7_game_reset(machine);
	to7_floppy_reset(machine);
	to7_io_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_TO770 );
	thom_set_init_callback( machine, to7_set_init );
	thom_set_lightpen_callback( machine, 3, to7_lightpen_cb );
	thom_set_mode_point( machine, 0 );
	thom_set_border_color( machine, 0 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );

	/* memory */
	to7_update_cart_bank(machine);
	/* thom_cart_bank not reset */

	/* lightpen */
	to7_lightpen = 0;
}



MACHINE_START ( to7 )
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	UINT8* mem = memory_region(machine, "main");

	LOG (( "to7: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &to7_sys );
	to7_game_init(machine);
	to7_floppy_init(machine, mem + 0x20000);
	to7_io_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);

	/* memory */
	thom_cart_bank = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_BASE_BANK, 0, 1, mess_ram + 0x4000, 0x2000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0, 2, thom_vram, 0x2000 );
	memory_configure_bank( machine, THOM_CART_BANK, 0, 4, mem + 0x10000, 0x4000 );
	memory_set_bank( machine, THOM_BASE_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );

	if ( mess_ram_size > 24*1024 )
	{
		/* install 16 KB or 16 KB + 8 KB memory extensions */
		/* BASIC instruction to see free memory: ?FRE(0) */
		int extram = mess_ram_size - 24*1024;
		memory_install_write8_handler(space, 0x8000, 0x8000 + extram - 1, 0, 0, (write8_space_func)(STATIC_BANK1 + THOM_RAM_BANK - 1) );
		memory_install_read8_handler(space, 0x8000, 0x8000 + extram - 1, 0, 0, (read8_space_func)(STATIC_BANK1 + THOM_RAM_BANK - 1) );
		memory_configure_bank( machine, THOM_RAM_BANK,  0, 1, mess_ram + 0x6000, extram );
		memory_set_bank( machine, THOM_RAM_BANK, 0 );
	}

	/* force 2 topmost color bits to 1 */
	memset( thom_vram + 0x2000, 0xc0, 0x2000 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  to7_update_cart_bank_postload, NULL );
}



/***************************** TO7/70 *************************/



/* ------------ system PIA 6821 ------------ */



static WRITE8_HANDLER ( to770_sys_cb2_out )
{
	/* video overlay: black pixels are transparent and show TV image underneath */
	LOG(( "$%04x to770_sys_cb2_out: video overlay %i\n", cpu_get_previouspc( space->machine->cpu[0] ), data ));
}



static READ8_HANDLER ( to770_sys_porta_in )
{
	/* keyboard */
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", 
										"keyboard_4", "keyboard_5", "keyboard_6", "keyboard_7" };
	int keyline = pia_get_output_b( THOM_PIA_SYS ) & 7;
	
	return input_port_read(space->machine, keynames[7 - keyline]);
}



static void to770_update_ram_bank(running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	UINT8 portb = pia_get_port_b_z_mask( THOM_PIA_SYS );
	static int old_bank = -1;
	int bank;

	switch (portb & 0xf8)
	{
		/* 2 * 16 KB internal RAM */
	case 0xf0: bank = 0; break;
	case 0xe8: bank = 1; break;

		/* 4 * 16 KB extended RAM */
	case 0x18: bank = 2; break;
	case 0x98: bank = 3; break;
	case 0x58: bank = 4; break;
	case 0xd8: bank = 5; break;

		/* none selected */
	case 0xf8: return;

	default:
		logerror( "to770_update_ram_bank unknown bank $%02X\n", portb & 0xf8 );
		return;
	}

	if ( bank != old_bank )
		LOG_BANK(( "to770_update_ram_bank: RAM bank change %i\n", bank ));

	if ( mess_ram_size == 128*1024 || bank < 2 )
	{
		memory_set_bank( machine, THOM_RAM_BANK, bank );
		memory_install_write8_handler(space, 0xa000, 0xdfff, 0, 0, (write8_space_func)(STATIC_BANK1 + THOM_RAM_BANK - 1) );
	}
	else
	{
		memory_install_write8_handler(space, 0xa000, 0xdfff, 0, 0, SMH_UNMAP );
	}

	old_bank = bank;
}



static STATE_POSTLOAD ( to770_update_ram_bank_postload )
{
	to770_update_ram_bank(machine);
}



static WRITE8_HANDLER ( to770_sys_portb_out )
{
	to770_update_ram_bank(space->machine);
}



static const pia6821_interface to770_sys =
{
	to770_sys_porta_in, NULL,
	NULL, NULL, NULL, NULL,
	NULL, to770_sys_portb_out, to7_set_cassette_motor, to770_sys_cb2_out,
	thom_firq_1, thom_firq_1
};



/* ------------ 6846 (timer, I/O) ------------ */



static WRITE8_DEVICE_HANDLER ( to770_timer_port_out )
{
	thom_set_mode_point( device->machine, data & 1 );          /* bit 0: video bank switch */
	thom_set_caps_led( device->machine, (data & 8) ? 1 : 0 ) ; /* bit 3: keyboard led */
	thom_set_border_color( device->machine, 
			       ((data & 0x10) ? 1 : 0) |          /* 4-bit border color */
			       ((data & 0x20) ? 2 : 0) |
			       ((data & 0x40) ? 4 : 0) |
			       ((data & 0x04) ? 0 : 8) );
}



mc6846_interface to770_timer =
{
	to770_timer_port_out, NULL, to7_timer_cp2_out,
	to7_timer_port_in, to7_timer_tco_out,
	thom_dev_irq_0
};



/* ------------ gate-array ------------ */



READ8_HANDLER ( to770_gatearray_r )
{
	struct thom_vsignal v = thom_get_vsignal( space->machine );
	struct thom_vsignal l = thom_get_lightpen_vsignal( space->machine, TO7_LIGHTPEN_DECAL, to7_lightpen_step - 1, 0 );
	int count, inil, init, lt3;
	count = to7_lightpen ? l.count : v.count;
	inil  = to7_lightpen ? l.inil  : v.inil;
	init  = to7_lightpen ? l.init  : v.init;
	lt3   = to7_lightpen ? l.lt3   : v.lt3;

	switch ( offset )
	{
	case 0: return (count >> 8) & 0xff;
	case 1: return count & 0xff;
	case 2: return (lt3 << 7) | (inil << 6);
	case 3: return (init << 7);
	default:
		logerror( "$%04x to770_gatearray_r: invalid offset %i\n", cpu_get_previouspc( space->machine->cpu[0] ), offset );
		return 0;
	}
}



WRITE8_HANDLER ( to770_gatearray_w )
{
	if ( ! offset )
		to7_lightpen = data & 1;
}



/* ------------ init / reset ------------ */



MACHINE_RESET( to770 )
{
	LOG (( "to770: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	to7_game_reset(machine);
	to7_floppy_reset(machine);
	to7_io_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_TO770 );
	thom_set_init_callback( machine, to7_set_init );
	thom_set_lightpen_callback( machine, 3, to7_lightpen_cb );
	thom_set_mode_point( machine, 0 );
	thom_set_border_color( machine, 8 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );

	/* memory */
	to7_update_cart_bank(machine);
	to770_update_ram_bank(machine);
	/* thom_cart_bank not reset */

	/* lightpen */
	to7_lightpen = 0;
}



MACHINE_START ( to770 )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "to770: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &to770_sys );
	to7_game_init(machine);
	to7_floppy_init( machine, mem + 0x20000 );
	to7_io_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);

	/* memory */
	thom_cart_bank = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_BASE_BANK, 0, 1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, THOM_RAM_BANK,  0, 6, mess_ram + 0x8000, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0, 2, thom_vram, 0x2000 );
	memory_configure_bank( machine, THOM_CART_BANK, 0, 4, mem + 0x10000, 0x4000 );
	memory_set_bank( machine, THOM_BASE_BANK, 0 );
	memory_set_bank( machine, THOM_RAM_BANK,  0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  to770_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  to7_update_cart_bank_postload, NULL );
}



/***************************** MO5 *************************/



/* ------------ lightpen automaton ------------ */



static void mo5_lightpen_cb ( running_machine *machine, int step )
{
	/* MO5 signals ca1 (TO7 signals cb1) */
	if ( ! to7_lightpen )
		return;

	pia_set_input_ca1( THOM_PIA_SYS, 1 );
	pia_set_input_ca1( THOM_PIA_SYS, 0 );
	to7_lightpen_step = step;
}


/* ------------ periodic interrupt ------------ */

/* the MO5 & MO6 do not have a MC 6846 timer,
   they have a fixed 50 Hz timer instead
*/



static emu_timer* mo5_periodic_timer;



static TIMER_CALLBACK(mo5_periodic_cb)
{
	/* pulse */
	pia_set_input_cb1( THOM_PIA_SYS, 1 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );
}



static void mo5_init_timer(running_machine *machine)
{
	/* time is a little faster than 50 Hz to match video framerate */
	timer_adjust_periodic(mo5_periodic_timer, attotime_zero, 0, ATTOTIME_IN_USEC( 19968 ));
}



/* ------------ system PIA 6821 ------------ */



static WRITE8_HANDLER ( mo5_sys_porta_out )
{
	thom_set_mode_point( space->machine, data & 1 );		/* bit 0: video bank switch */
	thom_set_border_color( space->machine, (data >> 1) & 15 );	/* bit 1-4: border color */
	mo5_set_cassette( space->machine, (data & 0x40) ? 1 : 0 );	/* bit 6: cassette output */
}



static READ8_HANDLER ( mo5_sys_porta_in )
{
	return
		(mo5_get_cassette(space->machine) ? 0x80 : 0) |     /* bit 7: cassette input */
		((input_port_read(space->machine, "lightpen_button") & 1) ? 0x20 : 0)
		/* bit 5: lightpen button */;
}



static WRITE8_HANDLER ( mo5_sys_portb_out )
{
	dac_data_w( THOM_SOUND_BUZ, (data & 1) ? 0x80 : 0); /* 1-bit buzzer */
}



static READ8_HANDLER ( mo5_sys_portb_in )
{
	UINT8 portb = pia_get_output_b( THOM_PIA_SYS );
	int col = (portb >> 1) & 7;       /* key column */
	int lin = 7 - ((portb >> 4) & 7); /* key line */
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", 
										"keyboard_4", "keyboard_5", "keyboard_6", "keyboard_7" };
	
	return ( input_port_read(space->machine, keynames[lin]) & (1 << col) ) ? 0x80 : 0;
}



static const pia6821_interface mo5_sys =
{
	mo5_sys_porta_in, mo5_sys_portb_in,
	NULL, NULL, NULL, NULL,
	mo5_sys_porta_out, mo5_sys_portb_out, mo5_set_cassette_motor, NULL,
	thom_firq_1, thom_irq_1 /* WARNING: differs from TO7 ! */
};



/* ------------ gate-array ------------ */



#define MO5_LIGHTPEN_DECAL 12



READ8_HANDLER ( mo5_gatearray_r )
{
	struct thom_vsignal v = thom_get_vsignal( space->machine );
	struct thom_vsignal l = thom_get_lightpen_vsignal( space->machine, MO5_LIGHTPEN_DECAL, to7_lightpen_step - 1, 0 );
	int count, inil, init, lt3;
	count = to7_lightpen ? l.count : v.count;
	inil  = to7_lightpen ? l.inil  : v.inil;
	init  = to7_lightpen ? l.init  : v.init;
	lt3   = to7_lightpen ? l.lt3   : v.lt3;

	switch ( offset ) {
	case 0: return (count >> 8) & 0xff;
	case 1: return count & 0xff;
	case 2: return (lt3 << 7) | (inil << 6);
	case 3: return (init << 7);
	default:
		logerror( "$%04x mo5_gatearray_r: invalid offset %i\n",  cpu_get_previouspc( space->machine->cpu[0] ), offset );
		return 0;
	}
}



WRITE8_HANDLER ( mo5_gatearray_w )
{
	if ( ! offset )
		to7_lightpen = data & 1;
}



/* ------------ cartridge / extended RAM ------------ */



static UINT8 mo5_reg_cart; /* 0xa7cb bank switch */



DEVICE_IMAGE_LOAD( mo5_cartridge )
{
	UINT8* pos = memory_region(image->machine, "main") + 0x10000;
	UINT64 size = image_length ( image );
	int i,j;
	char name[129];

	/* get size & number of 16-KB banks */
	if ( size > 32 && size <= 0x04000 )
		thom_cart_nb_banks = 1;
	else if ( size == 0x08000 )
		thom_cart_nb_banks = 2;
	else if ( size == 0x10000 )
		thom_cart_nb_banks = 4;
	else
	{
		logerror( "mo5_cartridge_load: invalid cartridge size %u\n", (unsigned) size );
		return INIT_FAIL;
	}

	if ( image_fread( image, pos, size ) != size )
	{
		logerror( "mo5_cartridge_load: read error\n" );
		return INIT_FAIL;
	}

	/* extract name */
	i = size - 32;
	while ( i < size && !pos[i] ) i++;
	for ( j = 0; i < size && pos[i] >= 0x20; j++, i++)
		name[j] = pos[i];
	name[j] = 0;

	/* sanitize name */
	for ( i = 0; name[i]; i++)
	{
		if ( name[i] < ' ' || name[i] >= 127 ) name[i] = '?';
	}

	PRINT (( "mo5_cartridge_load: cartridge \"%s\" banks=%i, size=%u\n", name, thom_cart_nb_banks, (unsigned) size ));

	return INIT_PASS;
}



static void mo5_update_cart_bank(running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	static int old_bank = -1;
	int rom_is_ram = mo5_reg_cart & 4;
	int bank = 0;

	memory_install_read8_handler( space, 0xb000, 0xefff, 0, 0, (read8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) );

	if ( rom_is_ram && thom_cart_nb_banks == 4 )
	{
		/* 64 KB ROM from "JANE" cartridge */
		memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, SMH_NOP );
		bank = mo5_reg_cart & 3;
		if ( bank != old_bank )
			LOG_BANK(( "mo5_update_cart_bank: CART is cartridge bank %i (A7CB style)\n", bank ));
	}
	else if ( rom_is_ram )
	{
		/* 64 KB RAM from network extension */
		int write_enable = mo5_reg_cart & 8;
		bank = 4 + ( mo5_reg_cart & 3 );
		memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, write_enable ? (write8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) :  SMH_NOP );
		if ( bank != old_bank )
			LOG_BANK(( "mo5_update_cart_bank: CART is nanonetwork RAM bank %i (write-enable=%i)\n", mo5_reg_cart & 3, write_enable ? 1 : 0 ));
	}
	else
	{
		/* regular cartridge bank switch */
		memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, mo5_cartridge_w );
		if ( thom_cart_nb_banks )
		{
			bank = thom_cart_bank % thom_cart_nb_banks;
			memory_install_read8_handler( space, 0xbffc, 0xbfff, 0, 0, mo5_cartridge_r );
		}
		if ( bank != old_bank )
			LOG_BANK(( "mo5_update_cart_bank: CART is internal / cartridge bank %i\n", thom_cart_bank ));
	}
	memory_set_bank( machine, THOM_CART_BANK, bank );
	old_bank = bank;
}



static STATE_POSTLOAD( mo5_update_cart_bank_postload )
{
	mo5_update_cart_bank(machine);
}



/* write signal to b000-cfff generates a bank switch */
WRITE8_HANDLER ( mo5_cartridge_w )
{
	if ( offset >= 0x2000 )
		return;

	thom_cart_bank = offset & 3;
	mo5_update_cart_bank(space->machine);
}



/* read signal to bffc-bfff generates a bank switch */
READ8_HANDLER ( mo5_cartridge_r )
{
	UINT8* pos = memory_region( space->machine, "main" ) + 0x10000;
	UINT8 data = pos[offset + 0xbffc + (thom_cart_bank % thom_cart_nb_banks) * 0x4000];
	thom_cart_bank = offset & 3;
	mo5_update_cart_bank(space->machine);
	return data;
}



/* 0xa7cb bank-switch register */
WRITE8_HANDLER ( mo5_ext_w )
{
	mo5_reg_cart = data;
	mo5_update_cart_bank(space->machine);
}



/* ------------ init / reset ------------ */



MACHINE_RESET( mo5 )
{
	LOG (( "mo5: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0x5f );
	to7_game_reset(machine);
	to7_floppy_reset(machine);
	to7_io_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);
	mo5_init_timer(machine);

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_MO5 );
	thom_set_lightpen_callback( machine, 3, mo5_lightpen_cb );
	thom_set_mode_point( machine, 0 );
	thom_set_border_color( machine, 0 );
	pia_set_input_ca1( THOM_PIA_SYS, 0 );

	/* memory */
	mo5_update_cart_bank(machine);
	/* mo5_reg_cart not reset */
	/* thom_cart_bank not reset */

	/* lightpen */
	to7_lightpen = 0;
}



MACHINE_START ( mo5 )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "mo5: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &mo5_sys );
	to7_game_init(machine);
	to7_floppy_init( machine, mem + 0x20000 );
	to7_io_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);
	mo5_periodic_timer = timer_alloc( machine, mo5_periodic_cb , NULL);

	/* memory */
	thom_cart_bank = 0;
	mo5_reg_cart = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_BASE_BANK, 0, 1, mess_ram + 0x4000, 0x8000 );
	memory_configure_bank( machine, THOM_CART_BANK, 0, 4, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 4, 4, mess_ram + 0xc000, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0, 2, thom_vram, 0x2000 );
	memory_set_bank( machine, THOM_BASE_BANK, 0 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, mo5_reg_cart );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  mo5_update_cart_bank_postload, NULL );
}



/***************************** TO9 *************************/



/* ------------ IEEE extension ------------ */



/* TODO: figure out what this extension is... IEEE-488 ??? */



WRITE8_HANDLER ( to9_ieee_w )
{
	logerror( "$%04x %f to9_ieee_w: unhandled write $%02X to register %i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data, offset );
}



READ8_HANDLER  ( to9_ieee_r )
{
	logerror( "$%04x %f to9_ieee_r: unhandled read from register %i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), offset );
	return 0;
}



/* ------------ system gate-array ------------ */



#define TO9_LIGHTPEN_DECAL 8



READ8_HANDLER ( to9_gatearray_r )
{
	struct thom_vsignal v = thom_get_vsignal( space->machine );
	struct thom_vsignal l = thom_get_lightpen_vsignal( space->machine, TO9_LIGHTPEN_DECAL, to7_lightpen_step - 1, 0 );
	int count, inil, init, lt3;
	count = to7_lightpen ? l.count : v.count;
	inil  = to7_lightpen ? l.inil  : v.inil;
	init  = to7_lightpen ? l.init  : v.init;
	lt3   = to7_lightpen ? l.lt3   : v.lt3;

	switch ( offset )
	{
	case 0: return (count >> 8) & 0xff;
	case 1: return count & 0xff;
	case 2: return (lt3 << 7) | (inil << 6);
	case 3: return (v.init << 7) | (init << 6); /* != TO7/70 */
	default:
		logerror( "$%04x to9_gatearray_r: invalid offset %i\n", cpu_get_previouspc( space->machine->cpu[0] ), offset );
		return 0;
	}
}



WRITE8_HANDLER ( to9_gatearray_w )
{
	if ( ! offset )
		to7_lightpen = data & 1;
}



/* ------------ video gate-array ------------ */



static UINT8 to9_palette_data[32];

static UINT8 to9_palette_idx;


/* style: 0 => TO9, 1 => TO8/TO9, 2 => MO6 */
static void to9_set_video_mode( running_machine *machine, UINT8 data, int style )
{
	switch ( data & 0x7f )
	{
	case 0x00:
		if ( style == 2 )
			thom_set_video_mode( machine, THOM_VMODE_MO5 );
		else if ( style == 1 )
			thom_set_video_mode( machine, THOM_VMODE_TO770 );
		else
			thom_set_video_mode( machine, THOM_VMODE_TO9 );
		break;

	case 0x21: thom_set_video_mode( machine, THOM_VMODE_BITMAP4 );     break;

	case 0x41: thom_set_video_mode( machine, THOM_VMODE_BITMAP4_ALT ); break;

	case 0x2a:
		if ( style==0 )
			thom_set_video_mode( machine, THOM_VMODE_80_TO9 );
		else
			thom_set_video_mode( machine, THOM_VMODE_80 );
		break;

	case 0x7b: thom_set_video_mode( machine, THOM_VMODE_BITMAP16 );    break;

	case 0x24: thom_set_video_mode( machine, THOM_VMODE_PAGE1 );       break;

	case 0x25: thom_set_video_mode( machine, THOM_VMODE_PAGE2 );       break;

	case 0x26: thom_set_video_mode( machine, THOM_VMODE_OVERLAY );     break;

	case 0x3f: thom_set_video_mode( machine, THOM_VMODE_OVERLAY3 );    break;

	default:
		logerror( "to9_set_video_mode: unknown mode $%02X tr=%i phi=%i mod=%i\n", data, (data >> 5) & 3, (data >> 3) & 2, data & 7 );
	}
}



READ8_HANDLER  ( to9_vreg_r )
{
	switch ( offset )
	{

	case 0: /* palette data */
	{
		UINT8 c =  to9_palette_data[ to9_palette_idx ];
		to9_palette_idx = ( to9_palette_idx + 1 ) & 31;
		return c;
	}

	case 1: /* palette address */
		return to9_palette_idx;

	case 2:
	case 3:
		return 0;

	default:
		logerror( "to9_vreg_r: invalid read offset %i\n", offset );
		return 0;
	}
}



WRITE8_HANDLER ( to9_vreg_w )
{
	LOG_VIDEO(( "$%04x %f to9_vreg_w: off=%i ($%04X) data=$%02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), offset, 0xe7da + offset, data ));

	switch ( offset )
	{

	case 0: /* palette data */
	{
		UINT16 color, idx;
		to9_palette_data[ to9_palette_idx ] = data;
		idx = to9_palette_idx / 2;
		color = to9_palette_data[ 2 * idx + 1 ];
		color = to9_palette_data[ 2 * idx ] | (color << 8);
		thom_set_palette( space->machine, idx ^ 8, color & 0x1fff );

		to9_palette_idx = ( to9_palette_idx + 1 ) & 31;
	}
	break;

	case 1: /* palette address */
		to9_palette_idx = data & 31;
		break;

	case 2: /* video mode */
		to9_set_video_mode( space->machine, data, 0 );
		break;

	case 3: /* border color */
		thom_set_border_color( space->machine, data & 15 );
		break;

	default:
		logerror( "to9_vreg_w: invalid write offset %i data=$%02X\n", offset, data );
	}
}



static void to9_palette_init ( running_machine *machine )
{
	to9_palette_idx = 0;
	memset( to9_palette_data, 0, sizeof( to9_palette_data ) );
	state_save_register_global( machine, to9_palette_idx );
	state_save_register_global_array( machine, to9_palette_data );
}



/* ------------ RAM / ROM banking ------------ */


static UINT8 to9_soft_bank;


static void to9_update_cart_bank(running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	static int old_bank = -1;
	int bank = 0;
	int slot = ( mc6846_get_output_port(devtag_get_device(machine, MC6846, "mc6846")) >> 4 ) & 3; /* bits 4-5: ROM bank */

	/* reset cartridge read handler */
	memory_install_read8_handler( space, 0x0000, 0x0003, 0, 0, (read8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) );

	switch ( slot )
	{
	case 0:
		/* BASIC (64 KB) */
		bank = 4 + to9_soft_bank;
		if ( bank != old_bank )
			LOG_BANK(( "to9_update_cart_bank: CART is BASIC bank %i\n", to9_soft_bank ));
		break;
	case 1:
		/* software 1 (32 KB) */
		bank = 8 + (to9_soft_bank & 1);
		if ( bank != old_bank )
			LOG_BANK(( "to9_update_cart_bank: CART is software 1 bank %i\n", to9_soft_bank ));
		break;
	case 2:
		/* software 2 (32 KB) */
		bank = 10 + (to9_soft_bank & 1);
		if ( bank != old_bank )
			LOG_BANK(( "to9_update_cart_bank: CART is software 2 bank %i\n", to9_soft_bank ));
		break;
	case 3:
		/* external cartridge */
		if ( thom_cart_nb_banks )
		{
			bank = thom_cart_bank % thom_cart_nb_banks;
			memory_install_read8_handler( space, 0x0000, 0x0003, 0, 0, to7_cartridge_r );
		}
		if ( bank != old_bank )
			LOG_BANK(( "to9_update_cart_bank: CART is cartridge bank %i\n",  thom_cart_bank ));
		break;
	}

	memory_set_bank( machine, THOM_CART_BANK, bank );
	old_bank = bank;
}



static STATE_POSTLOAD( to9_update_cart_bank_postload )
{
	to9_update_cart_bank(machine);
}



/* write signal to 0000-1fff generates a bank switch */
WRITE8_HANDLER ( to9_cartridge_w )
{
	int slot = ( mc6846_get_output_port(devtag_get_device(space->machine, MC6846, "mc6846")) >> 4 ) & 3; /* bits 4-5: ROM bank */

	if ( offset >= 0x2000 )
		return;

	if ( slot == 3 )
		thom_cart_bank = offset & 3;
	else
		to9_soft_bank = offset & 3;
	to9_update_cart_bank(space->machine);
}



/* read signal to 0000-0003 generates a bank switch */
READ8_HANDLER ( to9_cartridge_r )
{
	UINT8* pos = memory_region( space->machine, "main" ) + 0x10000;
	UINT8 data = pos[offset + (thom_cart_bank % thom_cart_nb_banks) * 0x4000];
	thom_cart_bank = offset & 3;
	to9_update_cart_bank(space->machine);
	return data;
}



static void to9_update_ram_bank (running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	static int old_bank = -1;
	UINT8 port = mc6846_get_output_port(devtag_get_device(machine, MC6846, "mc6846"));
	UINT8 portb = pia_get_port_b_z_mask( THOM_PIA_SYS );
	UINT8 disk = ((port >> 2) & 1) | ((port >> 5) & 2); /* bits 6,2: RAM bank */
	int bank;

	switch ( portb & 0xf8 )
	{
		/* TO7/70 compatible */
	case 0xf0: bank = 0; break;
	case 0xe8: bank = 1; break;
	case 0x18: bank = 2; break;
	case 0x98: bank = 3; break;
	case 0x58: bank = 4; break;
	case 0xd8: bank = 5; break;

		/* 64 KB of virtual disk */
	case 0xf8: bank = 6 + disk ; break;

		/* none selected */
	case 0: return;

	default:
		logerror( "to9_update_ram_bank: unknown RAM bank pia=$%02X disk=%i\n", portb & 0xf8, disk );
		return;
	}

	if ( old_bank != bank )
		LOG_BANK(( "to9_update_ram_bank: bank %i selected (pia=$%02X disk=%i)\n", bank, portb & 0xf8, disk ));

	if ( mess_ram_size == 192*1024 || bank < 6 )
	{
		memory_set_bank( machine, THOM_RAM_BANK, bank );
		memory_install_write8_handler( space, 0xa000, 0xdfff, 0, 0, (write8_space_func)(STATIC_BANK1 + THOM_RAM_BANK - 1) );
	}
	else
	{
		memory_install_write8_handler( space, 0xa000, 0xdfff, 0, 0, SMH_NOP );
	}

	old_bank = bank;
}



static STATE_POSTLOAD( to9_update_ram_bank_postload )
{
	to9_update_ram_bank(machine);
}



/* ------------ keyboard (6850 ACIA + 6805 CPU) ------------ */

/* The 6805 chip scans the keyboard and sends ASCII codes to the 6909.
   Data between the 6809 and 6805 is serialized at 9600 bauds.
   On the 6809 side, a 6850 ACIA is used.
   We do not emulate the seral line but pass bytes directly between the
   keyboard and the 6850 registers.
   Note that the keyboard protocol uses the parity bit as an extra data bit.
*/



/* normal mode: polling interval */
#define TO9_KBD_POLL_PERIOD  ATTOTIME_IN_MSEC( 10 )

/* peripherial mode: time between two bytes, and after last byte */
#define TO9_KBD_BYTE_SPACE   ATTOTIME_IN_USEC( 300 )
#define TO9_KBD_END_SPACE    ATTOTIME_IN_USEC( 9100 )

/* first and subsequent repeat periods, in TO9_KBD_POLL_PERIOD units */
#define TO9_KBD_REPEAT_DELAY  80 /* 800 ms */
#define TO9_KBD_REPEAT_PERIOD  7 /*  70 ms */



static UINT8  to9_kbd_parity;  /* 0=even, 1=odd, 2=no parity */
static UINT8  to9_kbd_intr;    /* interrupt mode */
static UINT8  to9_kbd_in;      /* data from keyboard */
static UINT8  to9_kbd_status;  /* status */
static UINT8  to9_kbd_overrun; /* character lost */

static UINT8  to9_kbd_periph;     /* peripherial mode */
static UINT8  to9_kbd_byte_count; /* byte-count in peripherial mode */
static UINT16 to9_mouse_x, to9_mouse_y;

static UINT8  to9_kbd_last_key;  /* for key repetition */
static UINT16 to9_kbd_key_count;

static UINT8  to9_kbd_caps;  /* caps-lock */
static UINT8  to9_kbd_pad;   /* keypad outputs special codes */

static emu_timer* to9_kbd_timer;



/* quick keyboard scan */
static int to9_kbd_ktest ( running_machine *machine )
{
	int line, bit;
	UINT8 port;
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", 
										"keyboard_5", "keyboard_6", "keyboard_7", "keyboard_8", "keyboard_9" };

	for ( line = 0; line < 10; line++ )
	{
		port = input_port_read(machine, keynames[line]);

		if ( line == 7 || line == 9 )
			port |= 1; /* shift & control */

		for ( bit = 0; bit < 8; bit++ )
		{
			if ( ! (port & (1 << bit)) )
				return 1;
		}
	}
	return 0;
}



static void to9_kbd_update_irq ( running_machine *machine )
{
	if ( (to9_kbd_intr & 4) && (to9_kbd_status & ACIA_6850_RDRF) )
		to9_kbd_status |= ACIA_6850_irq; /* byte received interrupt */

	if ( (to9_kbd_intr & 4) && (to9_kbd_status & ACIA_6850_OVRN) )
		to9_kbd_status |= ACIA_6850_irq; /* overrun interrupt */

	if ( (to9_kbd_intr & 3) == 1 && (to9_kbd_status & ACIA_6850_TDRE) )
		to9_kbd_status |= ACIA_6850_irq; /* ready to transmit interrupt */

	thom_irq_3( machine, to9_kbd_status & ACIA_6850_irq );
}



READ8_HANDLER ( to9_kbd_r )
{
	/* ACIA 6850 registers */

	switch ( offset )
	{

	case 0: /* get status */
		/* bit 0:     data received */
		/* bit 1:     ready to transmit data (always 1) */
		/* bit 2:     data carrier detect (ignored) */
		/* bit 3:     clear to send (ignored) */
		/* bit 4:     framing error (ignored) */
		/* bit 5:     overrun */
		/* bit 6:     parity error */
		/* bit 7:     interrupt */

		LOG_KBD(( "$%04x %f to9_kbd_r: status $%02X (rdrf=%i, tdre=%i, ovrn=%i, pe=%i, irq=%i)\n",
			  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), to9_kbd_status,
			  (to9_kbd_status & ACIA_6850_RDRF) ? 1 : 0,
			  (to9_kbd_status & ACIA_6850_TDRE) ? 1 : 0,
			  (to9_kbd_status & ACIA_6850_OVRN) ? 1 : 0,
			  (to9_kbd_status & ACIA_6850_PE) ? 1 : 0,
			  (to9_kbd_status & ACIA_6850_irq) ? 1 : 0 ));
		return to9_kbd_status;

	case 1: /* get input data */
		to9_kbd_status &= ~(ACIA_6850_irq | ACIA_6850_PE);
		if ( to9_kbd_overrun )
			to9_kbd_status |= ACIA_6850_OVRN;
		else
			to9_kbd_status &= ~(ACIA_6850_OVRN | ACIA_6850_RDRF);
		to9_kbd_overrun = 0;
		LOG_KBD(( "$%04x %f to9_kbd_r: read data $%02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), to9_kbd_in ));
		to9_kbd_update_irq(space->machine);
		return to9_kbd_in;

	default:
		logerror( "$%04x to9_kbd_r: invalid offset %i\n", cpu_get_previouspc( space->machine->cpu[0] ),  offset );
		return 0;
	}
}



WRITE8_HANDLER ( to9_kbd_w )
{
	/* ACIA 6850 registers */

	switch ( offset )
	{

	case 0: /* set control */
		/* bits 0-1: clock divide (ignored) or reset */
		if ( (data & 3) == 3 )
		{
			/* reset */
			to9_kbd_overrun = 0;
			to9_kbd_status = ACIA_6850_TDRE;
			to9_kbd_intr = 0;
			LOG_KBD(( "$%04x %f to9_kbd_w: reset (data=$%02X)\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
		}
		else
		{
			/* bits 2-4: parity */
			if ( (data & 0x18) == 0x10 )
				to9_kbd_parity = 2;
			else
				to9_kbd_parity = (data >> 2) & 1;
			/* bits 5-6: interrupt on transmit */
			/* bit 7:    interrupt on receive */
			to9_kbd_intr = data >> 5;

			LOG_KBD(( "$%04x %f to9_kbd_w: set control to $%02X (parity=%i, intr in=%i out=%i)\n",
				  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
				  data, to9_kbd_parity, to9_kbd_intr >> 2,
				  (to9_kbd_intr & 3) ? 1 : 0 ));
		}
		to9_kbd_update_irq(space->machine);
		break;

	case 1: /* output data */
		to9_kbd_status &= ~(ACIA_6850_irq | ACIA_6850_TDRE);
		to9_kbd_update_irq(space->machine);
		/* TODO: 1 ms delay here ? */
		to9_kbd_status |= ACIA_6850_TDRE; /* data transmit ready again */
		to9_kbd_update_irq(space->machine);

		switch ( data )
		{
		case 0xF8:
			/* reset */
			to9_kbd_caps = 1;
			to9_kbd_periph = 0;
			to9_kbd_pad = 0;
			break;

		case 0xF9: to9_kbd_caps = 1;   break;
		case 0xFA: to9_kbd_caps = 0;   break;
		case 0xFB: to9_kbd_pad = 1;    break;
		case 0xFC: to9_kbd_pad = 0;    break;
		case 0xFD: to9_kbd_periph = 1; break;
		case 0xFE: to9_kbd_periph = 0; break;

		default:
			logerror( "$%04x %f to9_kbd_w: unknown kbd command %02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data );
		}

		thom_set_caps_led( space->machine, !to9_kbd_caps );

		LOG(( "$%04x %f to9_kbd_w: kbd command %02X (caps=%i, pad=%i, periph=%i)\n",
		      cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data,
		      to9_kbd_caps, to9_kbd_pad, to9_kbd_periph ));

		break;

	default:
		logerror( "$%04x to9_kbd_w: invalid offset %i (data=$%02X) \n", cpu_get_previouspc( space->machine->cpu[0] ), offset, data );
	}
}



/* send a key to the CPU, 8-bit + parity bit (0=even, 1=odd)
   note: parity is not used as a checksum but to actually transmit a 9-th bit
   of information!
*/
static void to9_kbd_send ( running_machine *machine, UINT8 data, int parity )
{
	if ( to9_kbd_status & ACIA_6850_RDRF )
	{
		/* overrun will be set when the current valid byte is read */
		to9_kbd_overrun = 1;
		LOG_KBD(( "%f to9_kbd_send: overrun => drop data=$%02X, parity=%i\n", attotime_to_double(timer_get_time(machine)), data, parity ));
	}
	else
	{
		/* valid byte */
		to9_kbd_in = data;
		to9_kbd_status |= ACIA_6850_RDRF; /* raise data received flag */
		if ( to9_kbd_parity == 2 || to9_kbd_parity == parity )
			to9_kbd_status &= ~ACIA_6850_PE; /* parity OK */
		else
			to9_kbd_status |= ACIA_6850_PE;  /* parity error */
		LOG_KBD(( "%f to9_kbd_send: data=$%02X, parity=%i, status=$%02X\n", attotime_to_double(timer_get_time(machine)), data, parity, to9_kbd_status ));
	}
	to9_kbd_update_irq(machine);
}



/* keycode => TO9 code (extended ASCII), shifted and un-shifted */
static const int to9_kbd_code[80][2] =
{
	{ 145, 150 }, { '_', '6' }, { 'Y', 'Y' }, { 'H', 'H' },
	{ 11, 11 }, { 9, 9 }, { 30, 12 }, { 'N', 'N' },

	{ 146, 151 }, { '(', '5' }, { 'T', 'T' }, { 'G', 'G' },
	{ '=', '+' }, { 8, 8 }, { 28, 28 }, { 'B', 'B' },

	{ 147, 152 }, { '\'', '4' }, { 'R', 'R' }, { 'F', 'F' },
	{ 22, 22 },  { 155, 155 }, { 29, 127 }, { 'V', 'V' },

	{ 148, 153 }, { '"', '3' }, { 'E', 'E' }, { 'D', 'D' },
	{ 161, 161 }, { 158, 158 },
	{ 154, 154 }, { 'C', 'C' },

	{ 144, 149 }, { 128, '2' }, { 'Z', 'Z' }, { 'S', 'S' },
	{ 162, 162 }, { 156, 156 },
	{ 164, 164 }, { 'X', 'X' },

	{ '#', '@' }, { '*', '1' }, { 'A', 'A' }, { 'Q', 'Q' },
	{ '[', '{' }, { 159, 159 }, { 160, 160 }, { 'W', 'W' },

	{ 2, 2 }, { 129, '7' }, { 'U', 'U' }, { 'J', 'J' },
	{ ' ', ' ' }, { 163, 163 }, { 165, 165 },
	{ ',', '?' },

	{ 0, 0 }, { '!', '8' }, { 'I', 'I' }, { 'K', 'K' },
	{ '$', '&' }, { 10, 10 }, { ']', '}' },  { ';', '.' },

	{ 0, 0 }, { 130, '9' }, { 'O', 'O' }, { 'L', 'L' },
	{ '-', '\\' }, { 132, '%' }, { 13, 13 }, { ':', '/' },

	{ 0, 0 }, { 131, '0' }, { 'P', 'P' }, { 'M', 'M' },
	{ ')', 134 }, { '^', 133 }, { 157, 157 }, { '>', '<' }
};



/* returns the ASCII code for the key, or 0 for no key */
static int to9_kbd_get_key( running_machine *machine )
{
	int control = ! (input_port_read(machine, "keyboard_7") & 1);
	int shift   = ! (input_port_read(machine, "keyboard_9") & 1);
	int key = -1, line, bit;
	UINT8 port;
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", 
										"keyboard_5", "keyboard_6", "keyboard_7", "keyboard_8", "keyboard_9" };

	for ( line = 0; line < 10; line++ )
	{
		port = input_port_read(machine, keynames[line]);

		if ( line == 7 || line == 9 )
			port |= 1; /* shift & control */

		/* TODO: correct handling of simultaneous keystokes:
		   return the new key preferably & disable repeat
		*/
		for ( bit = 0; bit < 8; bit++ )
		{
			if ( ! (port & (1 << bit)) )
				key = line * 8 + bit;
		}
	}

	if ( key == -1 )
	{
		to9_kbd_last_key = 0xff;
		to9_kbd_key_count = 0;
		return 0;
	}
	else if ( key == 64 )
	{
		/* caps lock */
		if ( to9_kbd_last_key == key )
			return 0; /* no repeat */

		to9_kbd_last_key = key;
		to9_kbd_caps = !to9_kbd_caps;
		thom_set_caps_led( machine, !to9_kbd_caps );
		return 0;
	}
	else
	{
		int asc;
		asc = to9_kbd_code[key][shift];
		if ( ! asc ) return 0;

		/* keypad */
		if ( ! to9_kbd_pad ) {
			if ( asc >= 154 && asc <= 163 )
				asc += '0' - 154;
			else if ( asc == 164 )
				asc = '.';
			else if ( asc == 165 )
				asc = 13;
		}

		/* shifted letter */
		if ( asc >= 'A' && asc <= 'Z' && ( ! to9_kbd_caps ) && ( ! shift ) )
			asc += 'a' - 'A';

		/* control */
		if ( control )
			asc &= ~0x40;

		if ( key == to9_kbd_last_key )
		{
			/* repeat */
			to9_kbd_key_count++;
			if ( to9_kbd_key_count < TO9_KBD_REPEAT_DELAY || (to9_kbd_key_count - TO9_KBD_REPEAT_DELAY) % TO9_KBD_REPEAT_PERIOD )
				return 0;
			LOG_KBD(( "to9_kbd_get_key: repeat key $%02X '%c'\n", asc, asc ));
			return asc;
		}
		else {
			to9_kbd_last_key = key;
			to9_kbd_key_count = 0;
			LOG_KBD(( "to9_kbd_get_key: key down $%02X '%c'\n", asc, asc ));
			return asc;
		}
	}
}



static TIMER_CALLBACK(to9_kbd_timer_cb)
{
	if ( to9_kbd_periph )
	{
		/* peripherial mode: every 10 ms we send 4 bytes */

		switch ( to9_kbd_byte_count )
		{

		case 0: /* key */
			to9_kbd_send( machine, to9_kbd_get_key(machine), 0 );
			break;

		case 1: /* x axis */
		{
			int newx = input_port_read(machine, "mouse_x");
			UINT8 data = ( (newx - to9_mouse_x) & 0xf ) - 8;
			to9_kbd_send( machine, data, 1 );
			to9_mouse_x = newx;
			break;
		}

		case 2: /* y axis */
		{
			int newy = input_port_read(machine, "mouse_y");
			UINT8 data = ( (newy - to9_mouse_y) & 0xf ) - 8;
			to9_kbd_send( machine, data, 1 );
			to9_mouse_y = newy;
			break;
		}

		case 3: /* axis overflow & buttons */
		{
			int b = input_port_read(machine, "mouse_button");
			UINT8 data = 0;
			if ( b & 1 ) data |= 1;
			if ( b & 2 ) data |= 4;
			to9_kbd_send( machine, data, 1 );
			break;
		}

		}

		to9_kbd_byte_count = ( to9_kbd_byte_count + 1 ) & 3;
		timer_adjust_oneshot(to9_kbd_timer, to9_kbd_byte_count ? TO9_KBD_BYTE_SPACE : TO9_KBD_END_SPACE, 0);
	}
	else
	{
		int key = to9_kbd_get_key(machine);
		/* keyboard mode: send a byte only if a key is down */
		if ( key )
			to9_kbd_send( machine, key, 0 );
		timer_adjust_oneshot(to9_kbd_timer, TO9_KBD_POLL_PERIOD, 0);
	}
}



static void to9_kbd_reset ( running_machine *machine )
{
	LOG(( "to9_kbd_reset called\n" ));
	to9_kbd_overrun = 0;  /* no byte lost */
	to9_kbd_status = ACIA_6850_TDRE;  /* clear to transmit */
	to9_kbd_intr = 0;     /* interrupt disabled */
	to9_kbd_caps = 1;
	to9_kbd_periph = 0;
	to9_kbd_pad = 0;
	to9_kbd_byte_count = 0;
	thom_set_caps_led( machine, !to9_kbd_caps );
	to9_kbd_key_count = 0;
	to9_kbd_last_key = 0xff;
	to9_kbd_update_irq(machine);
	timer_adjust_oneshot(to9_kbd_timer, TO9_KBD_POLL_PERIOD, 0);
}



static void to9_kbd_init ( running_machine *machine )
{
	LOG(( "to9_kbd_init called\n" ));
	to9_kbd_timer = timer_alloc( machine, to9_kbd_timer_cb , NULL);
	state_save_register_global( machine, to9_kbd_parity );
	state_save_register_global( machine, to9_kbd_intr );
	state_save_register_global( machine, to9_kbd_in );
	state_save_register_global( machine, to9_kbd_status );
	state_save_register_global( machine, to9_kbd_overrun );
	state_save_register_global( machine, to9_kbd_last_key );
	state_save_register_global( machine, to9_kbd_key_count );
	state_save_register_global( machine, to9_kbd_caps );
	state_save_register_global( machine, to9_kbd_periph );
	state_save_register_global( machine, to9_kbd_pad );
	state_save_register_global( machine, to9_kbd_byte_count );
	state_save_register_global( machine, to9_mouse_x );
	state_save_register_global( machine, to9_mouse_y );
}


/* ------------ system PIA 6821 ------------ */

/* afaik, P2-P7 are not connected, so, the warning about undefined 0xf0 can be safely ignored */



static void to9_update_centronics ( running_machine *machine )
{
	UINT8 a = pia_get_output_a( THOM_PIA_SYS );
	UINT8 b = pia_get_output_b( THOM_PIA_SYS );
	UINT8 data = (a & 0xfe) | (b & 1);
	centronics_write_data( machine, 0, data );
	centronics_write_handshake( machine, 0, (b & 2) ? CENTRONICS_STROBE : 0, CENTRONICS_STROBE );

	LOG_IO(( "$%04x %f to9_update_centronics: data=$%02X strobe=%i\n",
		  cpu_get_previouspc( machine->cpu[0] ), attotime_to_double(timer_get_time(machine)), data,
		  (b & 2) ? 1 : 0 ));
}



static READ8_HANDLER ( to9_sys_porta_in )
{
	UINT8 ktest = to9_kbd_ktest(space->machine);

	LOG_KBD(( "to9_sys_porta_in: ktest=%i\n", ktest ));

	return ktest;
}



static WRITE8_HANDLER ( to9_sys_porta_out )
{
	to9_update_centronics(space->machine); /* bits 1-7: printer */
}



static WRITE8_HANDLER ( to9_sys_portb_out )
{
	to9_update_centronics(space->machine); /* bits 0-1: printer */
	to9_update_ram_bank(space->machine);

	if ( data & 4 ) /* bit 2: video overlay (TODO) */
		LOG(( "to9_sys_portb_out: video overlay not handled\n" ));
}



static const pia6821_interface to9_sys =
{
	to9_sys_porta_in, NULL,
	NULL, NULL, NULL, NULL,
	to9_sys_porta_out, to9_sys_portb_out, to7_set_cassette_motor, NULL,
	NULL, thom_firq_1
};


static const CENTRONICS_CONFIG to9_centronics = { PRINTER_CENTRONICS, NULL };



/* ------------ 6846 (timer, I/O) ------------ */



static WRITE8_DEVICE_HANDLER ( to9_timer_port_out )
{
	thom_set_mode_point( device->machine, data & 1 ); /* bit 0: video bank */
	to9_update_ram_bank( device->machine );
	to9_update_cart_bank( device->machine );
}



mc6846_interface to9_timer =
{
	to9_timer_port_out, NULL, to7_timer_cp2_out,
	to7_timer_port_in, to7_timer_tco_out,
	thom_dev_irq_0
};



/* ------------ init / reset ------------ */



MACHINE_RESET ( to9 )
{
	LOG (( "to9: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0xfe );
	to7_game_reset(machine);
	to9_floppy_reset(machine);
	to9_kbd_reset(machine);
	thom_centronics_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_TO9 );
	thom_set_lightpen_callback( machine, 3, to7_lightpen_cb );
	thom_set_border_color( machine, 8 );
	thom_set_mode_point( machine, 0 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );

	/* memory */
	to9_soft_bank = 0;
	to9_update_cart_bank(machine);
	to9_update_ram_bank(machine);
	/* thom_cart_bank not reset */

	/* lightpen */
	to7_lightpen = 0;
}



MACHINE_START ( to9 )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "to9: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &to9_sys );
	centronics_config( machine, 0, &to9_centronics );
	to7_game_init(machine);
	to9_floppy_init( machine, mem + 0xe000, mem + 0x40000 );
	to9_kbd_init(machine);
	to9_palette_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);

	/* memory */
	thom_vram = mess_ram;
	thom_cart_bank = 0;
	memory_configure_bank( machine, THOM_VRAM_BANK, 0,  2, thom_vram, 0x2000 );
	memory_configure_bank( machine, THOM_CART_BANK, 0, 12, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_BASE_BANK, 0,  1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, THOM_RAM_BANK,  0, 10, mess_ram + 0x8000, 0x4000 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_BASE_BANK, 0 );
	memory_set_bank( machine, THOM_RAM_BANK,  0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, to9_soft_bank );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  to9_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  to9_update_cart_bank_postload, NULL );
}



/***************************** TO8 *************************/



UINT8 to8_data_vpage;
UINT8 to8_cart_vpage;



/* ------------ keyboard (6804) ------------ */

/* The 6804 chip scans the keyboard and sends keycodes to the 6809.
   Data is serialized using variable pulse length encoding.
   Unlike the TO9, there is no decoding chip on the 6809 side, only
   1-bit PIA ports (6821 & 6846). The 6809 does the decoding.

   We do not emulate the 6804 but pass serialized data directly through the
   PIA ports.

   Note: if we conform to the (scarce) documentation the CPU tend to lock
   waitting for keyboard input.
   The protocol documentation is pretty scarce and does not account for these
   behaviors!
   The emulation code contains many hacks (delays, timeouts, spurious
   pulses) to improve the stability.
   This works well, but is not very accurate.
*/



/* polling interval */
#define TO8_KBD_POLL_PERIOD  ATTOTIME_IN_MSEC( 1 )

/* first and subsequent repeat periods, in TO8_KBD_POLL_PERIOD units */
#define TO8_KBD_REPEAT_DELAY  800 /* 800 ms */
#define TO8_KBD_REPEAT_PERIOD  70 /*  70 ms */

/* timeout waiting for CPU */
#define TO8_KBD_TIMEOUT  ATTOTIME_IN_MSEC( 100 )



/* state */
static UINT8  to8_kbd_ack;       /* 1 = cpu inits / accepts transfers */
static UINT16 to8_kbd_data;      /* data to transmit */
static UINT16 to8_kbd_step;      /* transmission automaton state */
static UINT8  to8_kbd_last_key;  /* last key (for repetition) */
static UINT32 to8_kbd_key_count; /* keypress time (for repetition)  */
static UINT8  to8_kbd_caps;      /* caps lock */

static emu_timer* to8_kbd_timer;   /* bit-send */
static emu_timer* to8_kbd_signal;  /* signal from CPU */



/* quick keyboard scan */
static int to8_kbd_ktest ( running_machine *machine )
{
	int line, bit;
	UINT8 port;
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", 
										"keyboard_5", "keyboard_6", "keyboard_7", "keyboard_8", "keyboard_9" };

	if ( input_port_read(machine, "config") & 2 )
		return 0; /* disabled */

	for ( line = 0; line < 10; line++ )
	{
		port = input_port_read(machine, keynames[line]);

		if ( line == 7 || line == 9 )
			port |= 1; /* shift & control */

		for ( bit = 0; bit < 8; bit++ )
		{
			if ( ! (port & (1 << bit)) )
				return 1;
		}
	}

	return 0;
}



/* keyboard scan & return keycode (or -1) */
static int to8_kbd_get_key( running_machine *machine )
{
	int control = (input_port_read(machine, "keyboard_7") & 1) ? 0 : 0x100;
	int shift   = (input_port_read(machine, "keyboard_9") & 1) ? 0 : 0x080;
	int key = -1, line, bit;
	UINT8 port;
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", 
										"keyboard_5", "keyboard_6", "keyboard_7", "keyboard_8", "keyboard_9" };
	
	if ( input_port_read(machine, "config") & 2 )
		return -1; /* disabled */

	for ( line = 0; line < 10; line++ )
	{
		port = input_port_read(machine, keynames[line]);

		if ( line == 7 || line == 9 )
			port |= 1; /* shift & control */

		/* TODO: correct handling of simultaneous keystokes:
		   return the new key preferably & disable repeat
		*/
		for ( bit = 0; bit < 8; bit++ )
		{
			if ( ! (port & (1 << bit)) )
				key = line * 8 + bit;
		}
	}

	if ( key == -1 )
	{
		to8_kbd_last_key = 0xff;
		to8_kbd_key_count = 0;
		return -1;
	}
	else if ( key == 64 )
	{
		/* caps lock */
		if ( to8_kbd_last_key == key )
			return -1; /* no repeat */
		to8_kbd_last_key = key;
		to8_kbd_caps = !to8_kbd_caps;
		if ( to8_kbd_caps )
			key |= 0x080; /* auto-shift */
		thom_set_caps_led( machine, !to8_kbd_caps );
		return key;
	}
	else if ( key == to8_kbd_last_key )
	{
		/* repeat */
		to8_kbd_key_count++;
		if ( to8_kbd_key_count < TO8_KBD_REPEAT_DELAY || (to8_kbd_key_count - TO8_KBD_REPEAT_DELAY) % TO8_KBD_REPEAT_PERIOD )
			return -1;
		return key | shift | control;
	}
	else
	{
		to8_kbd_last_key = key;
		to8_kbd_key_count = 0;
		return key | shift | control;
	}
}


/* steps:
   0     = idle, key polling
   1     = wait for ack to go down (key to send)
   99-117 = key data transmit
   91-117 = signal
   255    = timeout
*/

/* keyboard automaton */
static void to8_kbd_timer_func(running_machine *machine)
{
	attotime d;

	LOG_KBD(( "%f to8_kbd_timer_cb: step=%i ack=%i data=$%03X\n", attotime_to_double(timer_get_time(machine)), to8_kbd_step, to8_kbd_ack, to8_kbd_data ));

	if( ! to8_kbd_step )
	{
		/* key polling */
		int k = to8_kbd_get_key(machine);
		/* if not in transfer, send pulse from time to time
		   (helps avoiding CPU lock)
		*/
		if ( ! to8_kbd_ack )
			mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 0 );
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 1 );

		if ( k == -1 )
			d = TO8_KBD_POLL_PERIOD;
		else
		{
			/* got key! */
			LOG_KBD(( "to8_kbd_timer_cb: got key $%03X\n", k ));
			to8_kbd_data = k;
			to8_kbd_step = 1;
			d = ATTOTIME_IN_USEC( 100 );
		}
	}
	else if ( to8_kbd_step == 255 )
	{
		/* timeout */
		to8_kbd_last_key = 0xff;
		to8_kbd_key_count = 0;
		to8_kbd_step = 0;
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 1 );
		d = TO8_KBD_POLL_PERIOD;
	}
	else if ( to8_kbd_step == 1 )
	{
		/* schedule timeout waiting for ack to go down */
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 0 );
		to8_kbd_step = 255;
		d = TO8_KBD_TIMEOUT;
	}
	else if ( to8_kbd_step == 117 )
	{
		/* schedule timeout  waiting for ack to go up */
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 0 );
		to8_kbd_step = 255;
		d = TO8_KBD_TIMEOUT;
	}
	else if ( to8_kbd_step & 1 )
	{
		/* send silence between bits */
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 0 );
		d = ATTOTIME_IN_USEC( 100 );
		to8_kbd_step++;
	}
	else
	{
		/* send bit */
		int bpos = 8 - ( (to8_kbd_step - 100) / 2);
		int bit = (to8_kbd_data >> bpos) & 1;
		mc6846_set_input_cp1( devtag_get_device(machine, MC6846, "mc6846"), 1 );
		d = ATTOTIME_IN_USEC( bit ? 56 : 38 );
		to8_kbd_step++;
	}
	timer_adjust_oneshot(to8_kbd_timer, d, 0);
}



static TIMER_CALLBACK(to8_kbd_timer_cb)
{
	to8_kbd_timer_func(machine);
}



/* cpu <-> keyboard hand-check */
static void to8_kbd_set_ack ( running_machine *machine, int data )
{
	if ( data == to8_kbd_ack )
		return;
	to8_kbd_ack = data;

	if ( data )
	{
		double len = attotime_to_double(timer_timeelapsed( to8_kbd_signal )) * 1000. - 2.;
		LOG_KBD(( "%f to8_kbd_set_ack: CPU end ack, len=%f\n", attotime_to_double(timer_get_time(machine)), len ));
		if ( to8_kbd_data == 0xfff )
		{
			/* end signal from CPU */
			if ( len >= 0.6 && len <= 0.8 )
			{
				LOG (( "%f to8_kbd_set_ack: INIT signal\n", attotime_to_double(timer_get_time(machine)) ));
				to8_kbd_last_key = 0xff;
				to8_kbd_key_count = 0;
				to8_kbd_caps = 1;
				/* send back signal: TODO returned codes ? */
				to8_kbd_data = 0;
				to8_kbd_step = 0;
				timer_adjust_oneshot(to8_kbd_timer, ATTOTIME_IN_MSEC( 1 ), 0);
			}
			else
			{
				to8_kbd_step = 0;
				timer_adjust_oneshot(to8_kbd_timer, TO8_KBD_POLL_PERIOD, 0);
				if ( len >= 1.2 && len <= 1.4 )
				{
					LOG (( "%f to8_kbd_set_ack: CAPS on signal\n", attotime_to_double(timer_get_time(machine)) ));
					to8_kbd_caps = 1;
				}
				else if ( len >= 1.8 && len <= 2.0 )
				{
					LOG (( "%f to8_kbd_set_ack: CAPS off signal\n", attotime_to_double(timer_get_time(machine)) ));
					to8_kbd_caps = 0;
				}
			}
			thom_set_caps_led( machine, !to8_kbd_caps );
		}
		else
		{
			/* end key transmission */
			to8_kbd_step = 0;
			timer_adjust_oneshot(to8_kbd_timer, TO8_KBD_POLL_PERIOD, 0);
		}
	}

	else
	{
		if ( to8_kbd_step == 255 )
		{
			/* CPU accepts key */
			to8_kbd_step = 99;
			timer_adjust_oneshot(to8_kbd_timer, ATTOTIME_IN_USEC( 400 ), 0);
		}
		else
		{
			/* start signal from CPU */
			to8_kbd_data = 0xfff;
			to8_kbd_step = 91;
			timer_adjust_oneshot(to8_kbd_timer, ATTOTIME_IN_USEC( 400 ), 0);
			timer_adjust_oneshot(to8_kbd_signal, attotime_never, 0);
		}
		LOG_KBD(( "%f to8_kbd_set_ack: CPU ack, data=$%03X\n", attotime_to_double(timer_get_time(machine)), to8_kbd_data ));
	}
}



static void to8_kbd_reset ( running_machine *machine )
{
	to8_kbd_last_key = 0xff;
	to8_kbd_key_count = 0;
	to8_kbd_step = 0;
	to8_kbd_data = 0;
	to8_kbd_ack = 1;
	to8_kbd_caps = 1;
	thom_set_caps_led( machine, !to8_kbd_caps );
	to8_kbd_timer_func(machine);
}



static void to8_kbd_init ( running_machine *machine )
{
	to8_kbd_timer = timer_alloc( machine, to8_kbd_timer_cb , NULL);
	to8_kbd_signal = timer_alloc( machine, NULL , NULL);
	state_save_register_global( machine, to8_kbd_ack );
	state_save_register_global( machine, to8_kbd_data );
	state_save_register_global( machine, to8_kbd_step );
	state_save_register_global( machine, to8_kbd_last_key );
	state_save_register_global( machine, to8_kbd_key_count );
	state_save_register_global( machine, to8_kbd_caps );
}



/* ------------ RAM / ROM banking ------------ */

static UINT8  to8_reg_ram;
static UINT8  to8_reg_cart;
static UINT8  to8_reg_sys1;
static UINT8  to8_reg_sys2;
static UINT8  to8_lightpen_intr;
static UINT8  to8_soft_select;
static UINT8  to8_soft_bank;
static UINT8  to8_bios_bank;



static void to8_update_floppy_bank( running_machine *machine )
{
	static int old_bank = -1;
	int bank = (to8_reg_sys1 & 0x80) ? to7_floppy_bank : (to8_bios_bank + TO7_NB_FLOP_BANK);

	if ( bank != old_bank )
		LOG_BANK(( "to8_update_floppy_bank: floppy ROM is %s bank %i\n",
			  (to8_reg_sys1 & 0x80) ? "external" : "internal",
			  bank % TO7_NB_FLOP_BANK ));
	memory_set_bank( machine, THOM_FLOP_BANK, bank );
	old_bank = bank;
}



static STATE_POSTLOAD( to8_update_floppy_bank_postload )
{
	to8_update_floppy_bank(machine);
}



static void to8_update_ram_bank (running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	UINT8 bank = 0;

	if ( to8_reg_sys1 & 0x10 )
	{
		bank = to8_reg_ram & 31;
		if ( bank != to8_data_vpage )
			LOG_BANK(( "to8_update_ram_bank: select bank %i (new style)\n", bank ));
	}
	else
	{
		UINT8 portb = pia_get_port_b_z_mask( THOM_PIA_SYS );

		switch ( portb & 0xf8 )
		{
			/*  in compatibility mode, banks 5 and 6 are reversed wrt TO7/70 */
		case 0xf0: bank = 2; break;
		case 0xe8: bank = 3; break;
		case 0x18: bank = 4; break;
		case 0x58: bank = 5; break;
		case 0x98: bank = 6; break;
		case 0xd8: bank = 7; break;
		case 0xf8: return;
		default:
			logerror( "to8_update_ram_bank: unknown RAM bank=$%02X\n", portb & 0xf8 );
			return;
		}
		if ( bank != to8_data_vpage )
			LOG_BANK(( "to8_update_ram_bank: select bank %i (old style)\n", bank  ));
	}

	/*  due to adressing distortion, the 16 KB banked memory space is
	    split into two 8 KB spaces:
	    - 0xa000-0xbfff maps to 0x2000-0x3fff in 16 KB bank
	    - 0xc000-0xdfff maps to 0x0000-0x1fff in 16 KB bank
	    this is important if we map a bank that is also reachable by another,
	    undistorted space, such as cartridge, page 0 (video), or page 1
	*/
	to8_data_vpage = bank;
	if ( mess_ram_size == 512*1024 || to8_data_vpage < 16 )
	{
		memory_set_bank( machine, TO8_DATA_LO, to8_data_vpage );
		memory_set_bank( machine, TO8_DATA_HI, to8_data_vpage );
		memory_install_write8_handler( space, 0xa000, 0xbfff, 0, 0, to8_data_vpage <= 4 ? to8_data_lo_w : (write8_space_func)(STATIC_BANK1 + TO8_DATA_LO - 1) );
		memory_install_write8_handler( space, 0xc000, 0xdfff, 0, 0, to8_data_vpage <= 4 ? to8_data_hi_w : (write8_space_func)(STATIC_BANK1 + TO8_DATA_HI - 1) );
	}
	else
	{
		memory_install_write8_handler( space, 0xa000, 0xbfff, 0, 0, SMH_NOP );
		memory_install_write8_handler( space, 0xc000, 0xdfff, 0, 0, SMH_NOP );
	}
}



static STATE_POSTLOAD( to8_update_ram_bank_postload )
{
	to8_update_ram_bank(machine);
}



static void to8_update_cart_bank (running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	static int old_bank = -1;
	int bank = 0;

	/* reset bank switch */
	memory_install_read8_handler( space, 0x0000, 0x0003, 0, 0, (read8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) );

	if ( to8_reg_cart & 0x20 )
	{
		/* RAM space */
		to8_cart_vpage = to8_reg_cart & 31;
		bank = 8 + to8_cart_vpage;
		memory_install_write8_handler( space, 0x0000, 0x3fff, 0, 0,
					       ((to8_cart_vpage < 8 || mess_ram_size == 512*1024) && (to8_reg_cart & 0x40)) ?
					       (to8_cart_vpage <= 4) ? to8_vcart_w :
					       (write8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) :
					       SMH_NOP );
		if ( bank != old_bank )
			LOG_BANK(( "to8_update_cart_bank: CART is RAM bank %i (write-enable=%i)\n", to8_cart_vpage, (to8_reg_cart & 0x40) ? 1 : 0 ));
	}
	else
	{
		memory_install_write8_handler( space, 0x0000, 0x3fff, 0, 0, to8_cartridge_w );
		if ( to8_soft_select )
		{
			/* internal software ROM space */
			bank = 4 + to8_soft_bank;
			if ( bank != old_bank )
				LOG_BANK(( "to8_update_cart_bank: CART is internal bank %i\n",
					  to8_soft_bank ));
		}
		else
		{
			/* external cartridge ROM space */
			if ( thom_cart_nb_banks )
			{
				bank = thom_cart_bank % thom_cart_nb_banks;
				memory_install_read8_handler( space, 0x0000, 0x0003, 0, 0, to8_cartridge_r );

			}

			if ( bank != old_bank )
				LOG_BANK(( "to8_update_cart_bank: CART is external cartridge bank %i\n", thom_cart_bank ));
		}
	}

	if ( mess_ram_size == 512*1024 || bank < 16 ) 
	{
		memory_set_bank( machine, THOM_CART_BANK, bank );
	}

	old_bank = bank;
}



static STATE_POSTLOAD( to8_update_cart_bank_postload )
{
	to8_update_cart_bank(machine);
}



/* ROM bank switch */
WRITE8_HANDLER ( to8_cartridge_w )
{
	if ( offset >= 0x2000 )
		return;

	if ( to8_soft_select )
		to8_soft_bank = offset & 3;
	else
		thom_cart_bank = offset & 3;

	to8_update_cart_bank(space->machine);
}



/* read signal to 0000-0003 generates a bank switch */
READ8_HANDLER ( to8_cartridge_r )
{
	UINT8* pos = memory_region( space->machine, "main" ) + 0x10000;
	UINT8 data = pos[offset + (thom_cart_bank % thom_cart_nb_banks) * 0x4000];
	thom_cart_bank = offset & 3;
	to8_update_cart_bank(space->machine);
	return data;
}


/* ------------ floppy / network controller dispatch ------------ */



static void to8_floppy_init( running_machine *machine )
{
	UINT8* mem = memory_region(machine, "main");
	to7_floppy_init( machine, mem + 0x34000 );
}



static void to8_floppy_reset( running_machine *machine )
{
	UINT8* mem = memory_region(machine, "main");
	to7_floppy_reset(machine);
	if ( THOM_FLOPPY_INT )
		thmfc_floppy_reset(machine);
	memory_configure_bank( machine, THOM_FLOP_BANK, TO7_NB_FLOP_BANK, 2, mem + 0x30000, 0x2000 );
}



READ8_HANDLER ( to8_floppy_r )
{
	if ( (to8_reg_sys1 & 0x80) && THOM_FLOPPY_EXT )
		/* external controller */
		return to7_floppy_r( space, offset );
	else if ( ! (to8_reg_sys1 & 0x80) && THOM_FLOPPY_INT )
		/* internal controller */
		return thmfc_floppy_r( space, offset );
	else
		/* no controller */
		return 0;
}



WRITE8_HANDLER ( to8_floppy_w )
{
	if ( (to8_reg_sys1 & 0x80) && THOM_FLOPPY_EXT )
		/* external controller */
		to7_floppy_w( space, offset, data );
	else if ( ! (to8_reg_sys1 & 0x80) && THOM_FLOPPY_INT )
		/* internal controller */
		thmfc_floppy_w( space, offset, data );
}



/* ------------ system gate-array ------------ */



#define TO8_LIGHTPEN_DECAL 16



READ8_HANDLER ( to8_gatearray_r )
{
	struct thom_vsignal v = thom_get_vsignal( space->machine );
	struct thom_vsignal l = thom_get_lightpen_vsignal( space->machine, TO8_LIGHTPEN_DECAL, to7_lightpen_step - 1, 6 );
	int count, inil, init, lt3;
	UINT8 res;
	count = to7_lightpen ? l.count : v.count;
	inil  = to7_lightpen ? l.inil  : v.inil;
	init  = to7_lightpen ? l.init  : v.init;
	lt3   = to7_lightpen ? l.lt3   : v.lt3;

	switch ( offset )
	{

	case 0: /* system 2 / lightpen register 1 */
		if ( to7_lightpen )
			res = (count >> 8) & 0xff;
		else
			res = to8_reg_sys2 & 0xf0;
		break;

	case 1: /* ram register / lightpen register 2 */
		if ( to7_lightpen )
		{
			thom_firq_2( space->machine, 0 );
			to8_lightpen_intr = 0;
			res = count & 0xff;
		}
		else
			res = to8_reg_ram & 0x1f;
		break;

	case 2: /* cartrige register / lightpen register 3 */
		if ( to7_lightpen )
			res = (lt3 << 7) | (inil << 6);
		else
			res = to8_reg_cart;
		break;

	case 3: /* lightpen register 4 */
		res = (v.init << 7) | (init << 6) | (v.inil << 5) | (to8_lightpen_intr << 1) | to7_lightpen;
		break;

	default:
		logerror( "$%04x to8_gatearray_r: invalid offset %i\n", cpu_get_previouspc( space->machine->cpu[0] ), offset );
		res = 0;
	}

	LOG_VIDEO(( "$%04x %f to8_gatearray_r: off=%i ($%04X) res=$%02X lightpen=%i\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xe7e4 + offset, res, to7_lightpen ));

	return res;
}



WRITE8_HANDLER ( to8_gatearray_w )
{
	LOG_VIDEO(( "$%04x %f to8_gatearray_w: off=%i ($%04X) data=$%02X\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xe7e4 + offset, data ));

	switch ( offset )
	{

	case 0: /* switch */
		to7_lightpen = data & 1;
		break;

	case 1: /* ram register */
		if ( to8_reg_sys1 & 0x10 )
		{
			to8_reg_ram = data;
			to8_update_ram_bank(space->machine);
		}
		break;

	case 2: /* cartridge register */
		to8_reg_cart = data;
		to8_update_cart_bank(space->machine);
		break;

	case 3: /* system register 1 */
		to8_reg_sys1 = data;
		to8_update_floppy_bank(space->machine);
		to8_update_ram_bank(space->machine);
		to8_update_cart_bank(space->machine);
		break;

	default:
		logerror( "$%04x to8_gatearray_w: invalid offset %i (data=$%02X)\n",
			  cpu_get_previouspc( space->machine->cpu[0] ), offset, data );
	}
}



/* ------------ video gate-array ------------ */



READ8_HANDLER  ( to8_vreg_r )
{
	/* 0xe7dc from external floppy drive aliases the video gate-array */
	if ( ( offset == 3 ) && ( to8_reg_ram & 0x80 ) && ( to8_reg_sys1 & 0x80 ) )
	{
		if ( THOM_FLOPPY_EXT )
			return to7_floppy_r( space, 0xc );
		else
			return 0;
	}

	switch ( offset )
	{

	case 0: /* palette data */
	{
		UINT8 c =  to9_palette_data[ to9_palette_idx ];
		to9_palette_idx = ( to9_palette_idx + 1 ) & 31;
		return c;
	}

	case 1: /* palette address */
		return to9_palette_idx;

	case 2:
	case 3:
		return 0;

	default:
		logerror( "to8_vreg_r: invalid read offset %i\n", offset );
		return 0;
	}
}



WRITE8_HANDLER ( to8_vreg_w )
{
	LOG_VIDEO(( "$%04x %f to8_vreg_w: off=%i ($%04X) data=$%02X\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xe7da + offset, data ));

	switch ( offset )
	{

	case 0: /* palette data */
	{
		UINT16 color, idx;
		to9_palette_data[ to9_palette_idx ] = data;
		idx = to9_palette_idx / 2;
		color = to9_palette_data[ 2 * idx + 1 ];
		color = to9_palette_data[ 2 * idx ] | (color << 8);
		thom_set_palette( space->machine, idx, color & 0x1fff );
		to9_palette_idx = ( to9_palette_idx + 1 ) & 31;
	}
	break;

	case 1: /* palette address */
		to9_palette_idx = data & 31;
		break;

	case 2: /* display register */
		to9_set_video_mode( space->machine, data, 1 );
		break;

	case 3: /* system register 2 */
		/* 0xe7dc from external floppy drive aliases the video gate-array */
		if ( ( offset == 3 ) && ( to8_reg_ram & 0x80 ) && ( to8_reg_sys1 & 0x80 ) )
		{
			if ( THOM_FLOPPY_EXT )
				to7_floppy_w( space, 0xc, data );
		}
		else
		{
			to8_reg_sys2 = data;
			thom_set_video_page( space->machine, data >> 6 );
			thom_set_border_color( space->machine, data & 15 );
		}
		break;

	default:
		logerror( "to8_vreg_w: invalid write offset %i data=$%02X\n", offset, data );
	}
}



/* ------------ system PIA 6821 ------------ */



static READ8_HANDLER ( to8_sys_porta_in )
{
	int ktest = to8_kbd_ktest (space->machine);

	LOG_KBD(( "$%04x %f: to8_sys_porta_in ktest=%i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), ktest ));

	return ktest;
}



static WRITE8_HANDLER ( to8_sys_portb_out )
{
	to9_update_centronics(space->machine); /* bits 0-1: printer */
	to8_update_ram_bank(space->machine);

	if ( data & 4 ) /* bit 2: video overlay (TODO) */
		LOG(( "to8_sys_portb_out: video overlay not handled\n" ));
}



static const pia6821_interface to8_sys =
{
	to8_sys_porta_in, NULL,
	NULL, NULL, NULL, NULL,
	to9_sys_porta_out, to8_sys_portb_out, to7_set_cassette_motor, NULL,
	NULL, thom_firq_1
};



/* ------------ 6846 (timer, I/O) ------------ */



static READ8_DEVICE_HANDLER ( to8_timer_port_in )
{
	int lightpen = (input_port_read(device->machine, "lightpen_button") & 1) ? 2 : 0;
	int cass = to7_get_cassette(device->machine) ? 0x80 : 0;
	int dtr = (centronics_read_handshake( device->machine, 0 ) & CENTRONICS_NOT_BUSY) ? 0 : 0x40;
	int lock = to8_kbd_caps ? 0 : 8; /* undocumented! */
	return lightpen | cass | dtr | lock;
}



static WRITE8_DEVICE_HANDLER ( to8_timer_port_out )
{
	int ack = (data & 0x20) ? 1 : 0;       /* bit 5: keyboard ACK */
	to8_bios_bank = (data & 0x10) ? 1 : 0; /* bit 4: BIOS bank*/
	thom_set_mode_point( device->machine, data & 1 );       /* bit 0: video bank switch */
	memory_set_bank( device->machine, TO8_BIOS_BANK, to8_bios_bank );
	to8_soft_select = (data & 0x04) ? 1 : 0; /* bit 2: internal ROM select */
	to8_update_floppy_bank(device->machine);
	to8_update_cart_bank(device->machine);
	to8_kbd_set_ack(device->machine, ack);
}



static WRITE8_DEVICE_HANDLER ( to8_timer_cp2_out )
{
	/* mute */
	to7_game_mute = data;
	to7_game_sound_update(device->machine);
}



mc6846_interface to8_timer =
{
	to8_timer_port_out, NULL, to8_timer_cp2_out,
	to8_timer_port_in, to7_timer_tco_out,
	thom_dev_irq_0
};



/* ------------ lightpen ------------ */



/* direct connection to interrupt line instead of through a PIA */
static void to8_lightpen_cb ( running_machine *machine, int step )
{
	if ( ! to7_lightpen )
		return;

	thom_firq_2( machine, 1 );
	to7_lightpen_step = step;
	to8_lightpen_intr = 1;
}



/* ------------ init / reset ------------ */



MACHINE_RESET ( to8 )
{
	LOG (( "to8: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0xfe );
	to7_game_reset(machine);
	to8_floppy_reset(machine);
	to8_kbd_reset(machine);
	thom_centronics_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);

	/* gate-array */
	to7_lightpen = 0;
	to8_reg_ram = 0;
	to8_reg_cart = 0;
	to8_reg_sys1 = 0;
	to8_reg_sys2 = 0;
	to8_lightpen_intr = 0;
	to8_soft_select = 0;

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_TO770 );
	thom_set_lightpen_callback( machine, 4, to8_lightpen_cb );
	thom_set_border_color( machine, 0 );
	thom_set_mode_point( machine, 0 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );

	/* memory */
	to8_cart_vpage = 0;
	to8_data_vpage = 0;
	to8_soft_bank = 0;
	to8_bios_bank = 0;
	to8_update_ram_bank(machine);
	to8_update_cart_bank(machine);
	to8_update_floppy_bank(machine);
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );
	/* thom_cart_bank not reset */
}



MACHINE_START ( to8 )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "to8: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &to8_sys );
	centronics_config( machine, 0, &to9_centronics );
	to7_game_init(machine);
	to8_floppy_init(machine);
	to8_kbd_init(machine);
	to9_palette_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);

	/* memory */
	thom_cart_bank = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_CART_BANK, 0,  8, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 8, 32, mess_ram, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0,  2, mess_ram, 0x2000 );
	memory_configure_bank( machine, TO8_SYS_LO,     0,  1, mess_ram + 0x6000, 0x4000 );
	memory_configure_bank( machine, TO8_SYS_HI,     0,  1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_LO,    0, 32, mess_ram + 0x2000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_HI,    0, 32, mess_ram + 0x0000, 0x4000 );
	memory_configure_bank( machine, TO8_BIOS_BANK,  0,  2, mem + 0x30800, 0x2000 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, TO8_SYS_LO,  0 );
	memory_set_bank( machine, TO8_SYS_HI,  0 );
	memory_set_bank( machine, TO8_DATA_LO, 0 );
	memory_set_bank( machine, TO8_DATA_HI, 0 );
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, to8_reg_ram );
	state_save_register_global( machine, to8_reg_cart );
	state_save_register_global( machine, to8_reg_sys1 );
	state_save_register_global( machine, to8_reg_sys2 );
	state_save_register_global( machine, to8_soft_select );
	state_save_register_global( machine, to8_soft_bank );
	state_save_register_global( machine, to8_bios_bank );
	state_save_register_global( machine, to8_lightpen_intr );
	state_save_register_global( machine, to8_data_vpage );
	state_save_register_global( machine, to8_cart_vpage );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  to8_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  to8_update_cart_bank_postload, NULL );
	state_save_register_postload( machine,  to8_update_floppy_bank_postload, NULL );
}



/***************************** TO9+ *************************/



/* ------------ system PIA 6821 ------------ */



static const pia6821_interface to9p_sys =
{
	to9_sys_porta_in, NULL,
	NULL, NULL, NULL, NULL,
	to9_sys_porta_out, to8_sys_portb_out, to7_set_cassette_motor, NULL,
	NULL, thom_firq_1
};



/* ------------ 6846 (timer, I/O) ------------ */



static READ8_DEVICE_HANDLER ( to9p_timer_port_in )
{
	int lightpen = (input_port_read(device->machine, "lightpen_button") & 1) ? 2 : 0;
	int cass = to7_get_cassette(device->machine) ? 0x80 : 0;
	int dtr = (centronics_read_handshake( device->machine, 0 ) & CENTRONICS_NOT_BUSY) ? 0 : 0x40;
	return lightpen | cass | dtr;
}



static WRITE8_DEVICE_HANDLER ( to9p_timer_port_out )
{
	int bios_bank = (data & 0x10) ? 1 : 0; /* bit 4: BIOS bank */
	thom_set_mode_point( device->machine, data & 1 );       /* bit 0: video bank switch */
	memory_set_bank( device->machine, TO8_BIOS_BANK, bios_bank );
	to8_soft_select = (data & 0x04) ? 1 : 0; /* bit 2: internal ROM select */
	to8_update_floppy_bank(device->machine);
	to8_update_cart_bank(device->machine);
}



mc6846_interface to9p_timer =
{
	to9p_timer_port_out, NULL, to8_timer_cp2_out,
	to9p_timer_port_in, to7_timer_tco_out,
	thom_dev_irq_0
};



/* ------------ init / reset ------------ */



MACHINE_RESET ( to9p )
{
	LOG (( "to9p: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0xfe );
	to7_game_reset(machine);
	to8_floppy_reset(machine);
	to9_kbd_reset(machine);
	thom_centronics_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);

	/* gate-array */
	to7_lightpen = 0;
	to8_reg_ram = 0;
	to8_reg_cart = 0;
	to8_reg_sys1 = 0;
	to8_reg_sys2 = 0;
	to8_lightpen_intr = 0;
	to8_soft_select = 0;

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_TO770 );
	thom_set_lightpen_callback( machine, 4, to8_lightpen_cb );
	thom_set_border_color( machine, 0 );
	thom_set_mode_point( machine, 0 );
	pia_set_input_cb1( THOM_PIA_SYS, 0 );

	/* memory */
	to8_cart_vpage = 0;
	to8_data_vpage = 0;
	to8_soft_bank = 0;
	to8_bios_bank = 0;
	to8_update_ram_bank(machine);
	to8_update_cart_bank(machine);
	to8_update_floppy_bank(machine);
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );
	/* thom_cart_bank not reset */
}



MACHINE_START ( to9p )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "to9p: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &to9p_sys );
	centronics_config( machine, 0, &to9_centronics );
	to7_game_init(machine);
	to8_floppy_init( machine );
	to9_kbd_init(machine);
	to9_palette_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);

	/* memory */
	thom_cart_bank = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_CART_BANK, 0,  8, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 8, 32, mess_ram, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0,  2, mess_ram, 0x2000 );
	memory_configure_bank( machine, TO8_SYS_LO,     0,  1, mess_ram + 0x6000, 0x4000 );
	memory_configure_bank( machine, TO8_SYS_HI,     0,  1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_LO,    0, 32, mess_ram + 0x2000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_HI,    0, 32, mess_ram + 0x0000, 0x4000 );
	memory_configure_bank( machine, TO8_BIOS_BANK,  0,  2, mem + 0x30800, 0x2000 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, TO8_SYS_LO,  0 );
	memory_set_bank( machine, TO8_SYS_HI,  0 );
	memory_set_bank( machine, TO8_DATA_LO, 0 );
	memory_set_bank( machine, TO8_DATA_HI, 0 );
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, to8_reg_ram );
	state_save_register_global( machine, to8_reg_cart );
	state_save_register_global( machine, to8_reg_sys1 );
	state_save_register_global( machine, to8_reg_sys2 );
	state_save_register_global( machine, to8_soft_select );
	state_save_register_global( machine, to8_soft_bank );
	state_save_register_global( machine, to8_bios_bank );
	state_save_register_global( machine, to8_lightpen_intr );
	state_save_register_global( machine, to8_data_vpage );
	state_save_register_global( machine, to8_cart_vpage );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  to8_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  to8_update_cart_bank_postload, NULL );
	state_save_register_postload( machine,  to8_update_floppy_bank_postload, NULL );
}



/****************** MO6 / Olivetti Prodest PC 128 *******************/



/* ------------ RAM / ROM banking ------------ */



static void mo6_update_ram_bank ( running_machine *machine )
{
	UINT8 bank = 0;

	if ( to8_reg_sys1 & 0x10 )
	{
		bank = to8_reg_ram & 7; /* 128 KB RAM only = 8 pages */
		if ( bank != to8_data_vpage )
			LOG_BANK(( "mo6_update_ram_bank: select bank %i (new style)\n", bank ));
	}
	to8_data_vpage = bank;
	memory_set_bank( machine, TO8_DATA_LO, to8_data_vpage );
	memory_set_bank( machine, TO8_DATA_HI, to8_data_vpage );
}



static STATE_POSTLOAD( mo6_update_ram_bank_postload )
{
	mo6_update_ram_bank(machine);
}



static void mo6_update_cart_bank (running_machine *machine)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	int b = (pia_get_output_a( THOM_PIA_SYS ) >> 5) & 1;
	static int old_bank = -1;
	int bank = 0;

	memory_install_read8_handler( space, 0xb000, 0xefff, 0, 0, (read8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) );

	if ( ( ( to8_reg_sys1 & 0x40 ) && ( to8_reg_cart & 0x20 ) ) || ( ! ( to8_reg_sys1 & 0x40 ) && ( mo5_reg_cart & 4 ) ) )
	{
		/* RAM space */
		if ( to8_reg_sys1 & 0x40 )
		{
			/* use a7e6 */
			to8_cart_vpage = to8_reg_cart & 7; /* 128 KB RAM only = 8 pages */
			bank = 8 + to8_cart_vpage;
			memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0,
						       (to8_reg_cart & 0x40) ? (to8_cart_vpage <= 4) ? to8_vcart_w :
						       (write8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) : SMH_NOP );
			if ( bank != old_bank )
				LOG_BANK(( "mo6_update_cart_bank: CART is RAM bank %i (write-enable=%i)\n", to8_cart_vpage, (to8_reg_cart & 0x40) ? 1 : 0 ));
		}
		else if ( thom_cart_nb_banks == 4 )
		{
			/* "JANE"-style cartridge bank switching */
			memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, SMH_NOP );
			bank = mo5_reg_cart & 3;
			if ( bank != old_bank )
				LOG_BANK(( "mo6_update_cart_bank: CART is external cartridge bank %i (A7CB style)\n", bank ));
		}
		else
		{
			/* RAM from MO5 network extension */
			int write_enable = mo5_reg_cart & 8;
			to8_cart_vpage = (mo5_reg_cart & 3) | 4;
			bank = 8 + to8_cart_vpage;
			memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, write_enable ?
						       (write8_space_func)(STATIC_BANK1 + THOM_CART_BANK - 1) :  SMH_NOP );
			if ( bank != old_bank )
				LOG_BANK(( "mo6_update_cart_bank: CART is RAM bank %i (write-enable=%i) (MO5 compat.)\n", to8_cart_vpage, write_enable ? 1 : 0 ));
		}
	}
	else
	{
		/* ROM space */
		memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, SMH_NOP );
		if ( to8_reg_sys2 & 0x20 )
		{
			/* internal ROM */
			if ( to8_reg_sys2 & 0x10)
				bank = b + 6; /* BASIC 128 */
			else
				bank = b + 4;                      /* BASIC 1 */
			if ( bank != old_bank )
				LOG_BANK(( "mo6_update_cart_bank: CART is internal ROM bank %i\n", b ));
		}
		else
		{
			/* cartridge */
			memory_install_write8_handler( space, 0xb000, 0xefff, 0, 0, mo6_cartridge_w );
			if ( thom_cart_nb_banks )
			{
				bank = thom_cart_bank % thom_cart_nb_banks;
				memory_install_read8_handler( space, 0xbffc, 0xbfff, 0, 0, mo6_cartridge_r );
			}
			if ( bank != old_bank )
				LOG_BANK(( "mo6_update_cart_bank: CART is external cartridge bank %i\n", bank ));
		}
	}

	old_bank = bank;
	memory_set_bank( machine, THOM_CART_BANK, bank );
	memory_set_bank( machine, TO8_BIOS_BANK, b );
}



static STATE_POSTLOAD( mo6_update_cart_bank_postload )
{
	mo6_update_cart_bank(machine);
}



/* write signal generates a bank switch */
WRITE8_HANDLER ( mo6_cartridge_w )
{
	if ( offset >= 0x2000 )
		return;

	thom_cart_bank = offset & 3;
	mo6_update_cart_bank(space->machine);
}



/* read signal generates a bank switch */
READ8_HANDLER ( mo6_cartridge_r )
{
	UINT8* pos = memory_region( space->machine, "main" ) + 0x10000;
	UINT8 data = pos[offset + 0xbffc + (thom_cart_bank % thom_cart_nb_banks) * 0x4000];
	thom_cart_bank = offset & 3;
	mo6_update_cart_bank(space->machine);
	return data;
}



WRITE8_HANDLER ( mo6_ext_w )
{
	/* MO5 network extension compatible */
	mo5_reg_cart = data;
	mo6_update_cart_bank(space->machine);
}



/* ------------ game 6821 PIA ------------ */

/* similar to SX 90-018, but with a few differences: mute, printer */



static WRITE8_HANDLER ( mo6_game_porta_out )
{
	/* centronics data */
	LOG (( "$%04x %f mo6_game_porta_out: CENTRONICS set data=$%02X\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
	centronics_write_data( space->machine, 0, data );
}



static READ8_HANDLER ( mo6_game_cb1_in )
{
	int dtr = ( centronics_read_handshake( space->machine, 0 ) & CENTRONICS_NOT_BUSY ) ? 0 : 1;
	/* note: printer busy signal replaces button */
	LOG (( "$%04x %f mo6_game_cb1_in: CENTRONICS get dtr=%i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), dtr ));
	return dtr;
}



static WRITE8_HANDLER ( mo6_game_cb2_out )
{
	/* centronics strobe */
	LOG (( "$%04x %f mo6_game_cb2_out: CENTRONICS set strobe=%i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data ));
	centronics_write_handshake( space->machine, 0, data ? CENTRONICS_STROBE : 0, CENTRONICS_STROBE );
}



static const pia6821_interface mo6_game =
{
	to7_game_porta_in, to7_game_portb_in,
	NULL, mo6_game_cb1_in, NULL, NULL,
	mo6_game_porta_out, to7_game_portb_out, NULL, mo6_game_cb2_out,
	thom_irq_1, thom_irq_1
};



static TIMER_CALLBACK(mo6_game_update_cb)
{
	/* unlike the TO8, CB1 & CB2 are not connected to buttons */
	if ( input_port_read(machine, "config") & 1 )
	{
		UINT8 mouse = to7_get_mouse_signal(machine);
		pia_set_input_ca1( THOM_PIA_GAME, mouse & 1 ); /* XA */
		pia_set_input_ca2( THOM_PIA_GAME, mouse & 2 ); /* YA */
	}
	else
	{
		/* joystick */
		UINT8 in = input_port_read(machine, "game_port_buttons");
		pia_set_input_ca1( THOM_PIA_GAME, in & 0x04 ); /* P1 action B */
		pia_set_input_ca2( THOM_PIA_GAME, in & 0x40 ); /* P1 action A */
	}
}



static void mo6_game_init ( running_machine *machine )
{
	LOG (( "mo6_game_init called\n" ));
	pia_config( machine, THOM_PIA_GAME, &mo6_game );
	to7_game_timer = timer_alloc( machine, mo6_game_update_cb , NULL);
	timer_adjust_periodic(to7_game_timer, TO7_GAME_POLL_PERIOD, 0, TO7_GAME_POLL_PERIOD);
	state_save_register_global( machine, to7_game_sound );
	state_save_register_global( machine, to7_game_mute );
}



static void mo6_game_reset ( running_machine *machine )
{
	LOG (( "mo6_game_reset called\n" ));
	pia_set_input_ca1( THOM_PIA_GAME, 0 );
	to7_game_sound = 0;
	to7_game_mute = 0;
	to7_game_sound_update(machine);
}



/* ------------ system PIA 6821 ------------ */



static READ8_HANDLER ( mo6_sys_porta_in )
{
	return
		(mo5_get_cassette(space->machine) ? 0x80 : 0) |     /* bit 7: cassette input */
		8 |                                   /* bit 3: kbd-line float up to 1 */
		((input_port_read(space->machine, "lightpen_button") & 1) ? 2 : 0);
	/* bit 1: lightpen button */;
}



static READ8_HANDLER ( mo6_sys_portb_in )
{
	/* keyboard: 9 lines of 8 keys */
	UINT8 porta = pia_get_output_a( THOM_PIA_SYS );
	UINT8 portb = pia_get_output_b( THOM_PIA_SYS );
	int col = (portb >> 4) & 7;    /* B bits 4-6: kbd column */
	int lin = (portb >> 1) & 7;    /* B bits 1-3: kbd line */
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", "keyboard_4", 
										"keyboard_5", "keyboard_6", "keyboard_7", "keyboard_8", "keyboard_9" };

	if ( ! (porta & 8) )
		lin = 8;     /* A bit 3: 9-th kbd line select */

	return
		( input_port_read(space->machine, keynames[lin]) & (1 << col) ) ?  0x80 : 0; 
	/* bit 7: key up */
}



static WRITE8_HANDLER ( mo6_sys_porta_out )
{
	thom_set_mode_point( space->machine,data & 1 );				/* bit 0: video bank switch */
	to7_game_mute = data & 4;						/* bit 2: sound mute */
	thom_set_caps_led( space->machine,(data & 16) ? 0 : 1 ) ;		/* bit 4: keyboard led */
	mo5_set_cassette( space->machine, (data & 0x40) ? 1 : 0 );	 	/* bit 6: cassette output */
	mo6_update_cart_bank(space->machine);					/* bit 5: rom bank */
	to7_game_sound_update(space->machine);
}



static WRITE8_HANDLER ( mo6_sys_portb_out )
{
	dac_data_w( THOM_SOUND_BUZ, (data & 1) ? 0x80 : 0); /* bit 0: buzzer */
}



static WRITE8_HANDLER ( mo6_sys_cb2_out )
{
	/* SCART pin 8 = slow switch (?) */
	LOG(( "mo6_sys_cb2_out: SCART slow switch set to %i\n", data ));
}



static const pia6821_interface mo6_sys =
{
	mo6_sys_porta_in, mo6_sys_portb_in,
	NULL, NULL, NULL, NULL,
	mo6_sys_porta_out, mo6_sys_portb_out, mo5_set_cassette_motor, mo6_sys_cb2_out,
	thom_firq_1, thom_irq_1 /* differs from TO */
};



/* ------------ system gate-array ------------ */

#define MO6_LIGHTPEN_DECAL 12



READ8_HANDLER ( mo6_gatearray_r )
{
	struct thom_vsignal v = thom_get_vsignal( space->machine );
	struct thom_vsignal l = thom_get_lightpen_vsignal( space->machine, MO6_LIGHTPEN_DECAL, to7_lightpen_step - 1, 6 );
	int count, inil, init, lt3;
	UINT8 res;
	count = to7_lightpen ? l.count : v.count;
	inil  = to7_lightpen ? l.inil  : v.inil;
	init  = to7_lightpen ? l.init  : v.init;
	lt3   = to7_lightpen ? l.lt3   : v.lt3;

	switch ( offset )
	{

	case 0: /* system 2 / lightpen register 1 */
		if ( to7_lightpen )
			res = (count >> 8) & 0xff;
		else
			res = to8_reg_sys2 & 0xf0;
		break;

	case 1: /* ram register / lightpen register 2 */
		if ( to7_lightpen )
		{
			thom_firq_2( space->machine, 0 );
			to8_lightpen_intr = 0;
			res =  count & 0xff;
		}
		else
			res = to8_reg_ram & 0x1f;
		break;

	case 2: /* cartrige register / lightpen register 3 */
		if ( to7_lightpen )
			res = (lt3 << 7) | (inil << 6);
		else
			res = 0;
		break;

	case 3: /* lightpen register 4 */
		res = (v.init << 7) | (init << 6) | (v.inil << 5) | (to8_lightpen_intr << 1) | to7_lightpen;
		break;

	default:
		logerror( "$%04x mo6_gatearray_r: invalid offset %i\n", cpu_get_previouspc( space->machine->cpu[0] ), offset );
		res = 0;
	}

	LOG_VIDEO(( "$%04x %f mo6_gatearray_r: off=%i ($%04X) res=$%02X lightpen=%i\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xa7e4 + offset, res, to7_lightpen ));

	return res;
}



WRITE8_HANDLER ( mo6_gatearray_w )
{
	LOG_VIDEO(( "$%04x %f mo6_gatearray_w: off=%i ($%04X) data=$%02X\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xa7e4 + offset, data ));

	switch ( offset )
	{

	case 0: /* switch */
		to7_lightpen = data & 1;
		break;

	case 1: /* ram register */
		if ( to8_reg_sys1 & 0x10 )
		{
			to8_reg_ram = data;
			mo6_update_ram_bank(space->machine);
		}
		break;

	case 2: /* cartridge register */
		to8_reg_cart = data;
		mo6_update_cart_bank(space->machine);
		break;

	case 3: /* system register 1 */
		to8_reg_sys1 = data;
		mo6_update_ram_bank(space->machine);
		mo6_update_cart_bank(space->machine);
		break;

	default:
		logerror( "$%04x mo6_gatearray_w: invalid offset %i (data=$%02X)\n", cpu_get_previouspc( space->machine->cpu[0] ), offset, data );
	}
}


/* ------------ video gate-array ------------ */



READ8_HANDLER ( mo6_vreg_r )
{
	/* 0xa7dc from external floppy drive aliases the video gate-array */
	if ( ( offset == 3 ) && ( to8_reg_ram & 0x80 ) )
		return to7_floppy_r( space, 0xc );

	switch ( offset )
	{

	case 0: /* palette data */
	case 1: /* palette address */
		return to8_vreg_r( space, offset );

	case 2:
	case 3:
		return 0;

	default:
		logerror( "mo6_vreg_r: invalid read offset %i\n", offset );
		return 0;
	}
}



WRITE8_HANDLER ( mo6_vreg_w )
{
	LOG_VIDEO(( "$%04x %f mo6_vreg_w: off=%i ($%04X) data=$%02X\n",
		  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)),
		  offset, 0xa7da + offset, data ));

	switch ( offset )
	{

	case 0: /* palette data */
	case 1: /* palette address */
		to8_vreg_w( space, offset, data );
		return;

	case 2: /* display / external floppy register */
		if ( ( to8_reg_sys1 & 0x80 ) && ( to8_reg_ram & 0x80 ) )
			to7_floppy_w( space, 0xc, data );
		else
			to9_set_video_mode( space->machine, data, 2 );
		break;

	case 3: /* system register 2 */
		/* 0xa7dc from external floppy drive aliases the video gate-array */
		if ( ( offset == 3 ) && ( to8_reg_ram & 0x80 ) )
			to7_floppy_w( space, 0xc, data );
		else
		{
			to8_reg_sys2 = data;
			thom_set_video_page( space->machine, data >> 6 );
			thom_set_border_color( space->machine, data & 15 );
			mo6_update_cart_bank( space->machine );
		}
		break;

	default:
		logerror( "mo6_vreg_w: invalid write offset %i data=$%02X\n", offset, data );
	}
}



/* ------------ init / reset ------------ */



MACHINE_RESET ( mo6 )
{
	LOG (( "mo6: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0x75 );
	mo6_game_reset(machine);
	to7_floppy_reset(machine);
	thom_centronics_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);
	mo5_init_timer(machine);

	/* gate-array */
	to7_lightpen = 0;
	to8_reg_ram = 0;
	to8_reg_cart = 0;
	to8_reg_sys1 = 0;
	to8_reg_sys2 = 0;
	to8_lightpen_intr = 0;

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_MO5 );
	thom_set_lightpen_callback( machine, 3, to8_lightpen_cb );
	thom_set_border_color( machine, 0 );
	thom_set_mode_point( machine, 0 );
	pia_set_input_ca1( THOM_PIA_SYS, 0 );

	/* memory */
	to8_cart_vpage = 0;
	to8_data_vpage = 0;
	mo6_update_ram_bank(machine);
	mo6_update_cart_bank(machine);
	/* mo5_reg_cart not reset */
	/* thom_cart_bank not reset */
}



MACHINE_START ( mo6 )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "mo6: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &mo6_sys );
	centronics_config( machine, 0, &to9_centronics );
	mo6_game_init(machine);
	to7_floppy_init( machine, mem + 0x30000 );
	to9_palette_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);
	mo5_periodic_timer = timer_alloc( machine, mo5_periodic_cb , NULL);

	/* memory */
	thom_cart_bank = 0;
	mo5_reg_cart = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_CART_BANK, 0, 4, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 4, 2, mem + 0x1f000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 6, 2, mem + 0x28000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 8, 8, mess_ram, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0, 2, mess_ram, 0x2000 );
	memory_configure_bank( machine, TO8_SYS_LO,     0, 1, mess_ram + 0x6000, 0x4000 );
	memory_configure_bank( machine, TO8_SYS_HI,     0, 1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_LO,    0, 8, mess_ram + 0x2000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_HI,    0, 8, mess_ram + 0x0000, 0x4000 );
	memory_configure_bank( machine, TO8_BIOS_BANK,  0, 2, mem + 0x23000, 0x4000 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, TO8_SYS_LO,  0 );
	memory_set_bank( machine, TO8_SYS_HI,  0 );
	memory_set_bank( machine, TO8_DATA_LO, 0 );
	memory_set_bank( machine, TO8_DATA_HI, 0 );
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, to8_reg_ram );
	state_save_register_global( machine, to8_reg_cart );
	state_save_register_global( machine, to8_reg_sys1 );
	state_save_register_global( machine, to8_reg_sys2 );
	state_save_register_global( machine, to8_lightpen_intr );
	state_save_register_global( machine, to8_data_vpage );
	state_save_register_global( machine, to8_cart_vpage );
	state_save_register_global( machine, mo5_reg_cart );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  mo6_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  mo6_update_cart_bank_postload, NULL );
}



/***************************** MO5 NR *************************/



/* ------------ network ( & external floppy) ------------ */



READ8_HANDLER ( mo5nr_net_r )
{
	if ( to7_controller_type )
		return to7_floppy_r ( space, offset );

	logerror( "$%04x %f mo5nr_net_r: read from reg %i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), offset );

	return 0;
}



WRITE8_HANDLER ( mo5nr_net_w )
{
	if ( to7_controller_type )
		to7_floppy_w ( space, offset, data );
	else
		logerror( "$%04x %f mo5nr_net_w: write $%02X to reg %i\n",
			  cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), data, offset );
}


/* ------------ printer ------------ */

/* Unlike the TO8, TO9, TO9+, MO6, the printer has its own ports and does not
   go through the 6821 PIA.
*/



READ8_HANDLER ( mo5nr_prn_r )
{
	switch ( offset )
	{

	case 1:
		return centronics_read_data( space->machine, 0 );

	case 3:
		/* TODO: understand other bits */
		if ( centronics_read_handshake( space->machine, 0 ) & CENTRONICS_NOT_BUSY )
			return 0;
		else
			return 0x80;

	default:
		logerror( "$%04x %f mo5nr_prn_r: unhandled read from reg %i\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), offset );
		return 0;
	}
}



WRITE8_HANDLER ( mo5nr_prn_w )
{
	switch ( offset )
	{

	case 1:
		centronics_write_data( space->machine, 0, data );
		break;

	case 3:
		/* TODO: understand other bits */
		centronics_write_handshake( space->machine, 0, ( data & 8 ) ? CENTRONICS_STROBE : 0, CENTRONICS_STROBE );
		break;

	default:
		logerror( "$%04x %f mo5nr_prn_w: unhandled to reg %i (data=$%02X)\n", cpu_get_previouspc( space->machine->cpu[0] ), attotime_to_double(timer_get_time(space->machine)), offset, data );
	}
}



/* ------------ system PIA 6821 ------------ */



static READ8_HANDLER ( mo5nr_sys_portb_in )
{
	/* keyboard: only 8 lines of 8 keys (MO6 has 9 lines) */
	UINT8 portb = pia_get_output_b( THOM_PIA_SYS );
	int col = (portb >> 4) & 7;    /* B bits 4-6: kbd column */
	int lin = (portb >> 1) & 7;    /* B bits 1-3: kbd line */
	static const char *const keynames[] = { "keyboard_0", "keyboard_1", "keyboard_2", "keyboard_3", 
										"keyboard_4", "keyboard_5", "keyboard_6", "keyboard_7" };
	
	return ( input_port_read(space->machine, keynames[lin]) & (1 << col) ) ? 0x80 : 0; 
	/* bit 7: key up */
}



static WRITE8_HANDLER ( mo5nr_sys_porta_out )
{
	/* no keyboard LED */
	thom_set_mode_point( space->machine, data & 1 );			/* bit 0: video bank switch */
	to7_game_mute = data & 4;						/* bit 2: sound mute */
	mo5_set_cassette( space->machine, (data & 0x40) ? 1 : 0 );		/* bit 6: cassette output */
	mo6_update_cart_bank(space->machine);					/* bit 5: rom bank */
	to7_game_sound_update(space->machine);
}



static const pia6821_interface mo5nr_sys =
{
	mo6_sys_porta_in, mo5nr_sys_portb_in,
	NULL, NULL, NULL, NULL,
	mo5nr_sys_porta_out, mo6_sys_portb_out, mo5_set_cassette_motor, mo6_sys_cb2_out,
	thom_firq_1, thom_irq_1 /* differs from TO */
};




/* ------------ game 6821 PIA ------------ */

/* similar to the MO6, without the printer */



static const pia6821_interface mo5nr_game =
{
	to7_game_porta_in, to7_game_portb_in,
	NULL, NULL, NULL, NULL,
	mo6_game_porta_out, to7_game_portb_out, NULL, NULL,
	thom_irq_1, thom_irq_1
};



static void mo5nr_game_init ( running_machine* machine )
{
	LOG (( "mo5nr_game_init called\n" ));
	pia_config( machine, THOM_PIA_GAME, &mo5nr_game );
	to7_game_timer = timer_alloc( machine, mo6_game_update_cb , NULL);
	timer_adjust_periodic( to7_game_timer, TO7_GAME_POLL_PERIOD, 0, TO7_GAME_POLL_PERIOD );
	state_save_register_global( machine, to7_game_sound );
	state_save_register_global( machine, to7_game_mute );
}



static void mo5nr_game_reset ( running_machine* machine )
{
	LOG (( "mo5nr_game_reset called\n" ));
	pia_set_input_ca1( THOM_PIA_GAME, 0 );
	to7_game_sound = 0;
	to7_game_mute = 0;
	to7_game_sound_update(machine);
}



/* ------------ init / reset ------------ */



MACHINE_RESET ( mo5nr )
{
	LOG (( "mo5nr: machine reset called\n" ));

	/* subsystems */
	thom_irq_reset(machine);
	pia_reset();
	pia_set_port_a_z_mask( THOM_PIA_SYS, 0x65 );
	mo5nr_game_reset(machine);
	to7_floppy_reset(machine);
	thom_centronics_reset(machine);
	to7_modem_reset(machine);
	to7_midi_reset(machine);
	mo5_init_timer(machine);

	/* gate-array */
	to7_lightpen = 0;
	to8_reg_ram = 0;
	to8_reg_cart = 0;
	to8_reg_sys1 = 0;
	to8_reg_sys2 = 0;
	to8_lightpen_intr = 0;

	/* video */
	thom_set_video_mode( machine, THOM_VMODE_MO5 );
	thom_set_lightpen_callback( machine, 3, to8_lightpen_cb );
	thom_set_border_color( machine, 0 );
	thom_set_mode_point( machine, 0 );
	pia_set_input_ca1( THOM_PIA_SYS, 0 );

	/* memory */
	to8_cart_vpage = 0;
	to8_data_vpage = 0;
	mo6_update_ram_bank(machine);
	mo6_update_cart_bank(machine);
	/* mo5_reg_cart not reset */
	/* thom_cart_bank not reset */
}



MACHINE_START ( mo5nr )
{
	UINT8* mem = memory_region(machine, "main");

	LOG (( "mo5nr: machine start called\n" ));

	/* subsystems */
	thom_irq_init(machine);
	pia_config( machine, THOM_PIA_SYS, &mo5nr_sys );
	centronics_config( machine, 0, &to9_centronics );
	mo5nr_game_init(machine);
	to7_floppy_init( machine, mem + 0x30000 );
	to9_palette_init(machine);
	to7_modem_init(machine);
	to7_midi_init(machine);
	mo5_periodic_timer = timer_alloc( machine, mo5_periodic_cb , NULL);

	/* memory */
	thom_cart_bank = 0;
	mo5_reg_cart = 0;
	thom_vram = mess_ram;
	memory_configure_bank( machine, THOM_CART_BANK, 0, 4, mem + 0x10000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 4, 2, mem + 0x1f000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 6, 2, mem + 0x28000, 0x4000 );
	memory_configure_bank( machine, THOM_CART_BANK, 8, 8, mess_ram, 0x4000 );
	memory_configure_bank( machine, THOM_VRAM_BANK, 0, 2, mess_ram, 0x2000 );
	memory_configure_bank( machine, TO8_SYS_LO,     0, 1, mess_ram + 0x6000, 0x4000 );
	memory_configure_bank( machine, TO8_SYS_HI,     0, 1, mess_ram + 0x4000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_LO,    0, 8, mess_ram + 0x2000, 0x4000 );
	memory_configure_bank( machine, TO8_DATA_HI,    0, 8, mess_ram + 0x0000, 0x4000 );
	memory_configure_bank( machine, TO8_BIOS_BANK,  0, 2, mem + 0x23000, 0x4000 );
	memory_set_bank( machine, THOM_CART_BANK, 0 );
	memory_set_bank( machine, THOM_VRAM_BANK, 0 );
	memory_set_bank( machine, TO8_SYS_LO,  0 );
	memory_set_bank( machine, TO8_SYS_HI,  0 );
	memory_set_bank( machine, TO8_DATA_LO, 0 );
	memory_set_bank( machine, TO8_DATA_HI, 0 );
	memory_set_bank( machine, TO8_BIOS_BANK, 0 );

	/* save-state */
	state_save_register_global( machine, thom_cart_nb_banks );
	state_save_register_global( machine, thom_cart_bank );
	state_save_register_global( machine, to7_lightpen );
	state_save_register_global( machine, to7_lightpen_step );
	state_save_register_global( machine, to8_reg_ram );
	state_save_register_global( machine, to8_reg_cart );
	state_save_register_global( machine, to8_reg_sys1 );
	state_save_register_global( machine, to8_reg_sys2 );
	state_save_register_global( machine, to8_lightpen_intr );
	state_save_register_global( machine, to8_data_vpage );
	state_save_register_global( machine, to8_cart_vpage );
	state_save_register_global( machine, mo5_reg_cart );
	state_save_register_global_pointer( machine, (mem + 0x10000), 0x10000 );
	state_save_register_postload( machine,  mo6_update_ram_bank_postload, NULL );
	state_save_register_postload( machine,  mo6_update_cart_bank_postload, NULL );
}
