/*
    pdp1 machine code

    includes emulation for I/O devices (with the IOT opcode) and control panel functions

    TODO:
    * typewriter out should overwrite the typewriter buffer
    * improve emulation of the internals of tape reader?
    * improve puncher timing?
*/

#include <stdio.h>

#include "emu.h"
#include "cpu/pdp1/pdp1.h"
#include "includes/pdp1.h"


/* This flag makes the emulated pdp-1 trigger a sequence break request when a character has been
typed on the typewriter keyboard.  It is useful in order to test sequence break, but we need
to emulate a connection box in which we can connect each wire to any interrupt line.  Also,
we need to determine the exact relationship between the status register and the sequence break
system (both standard and type 20). */
#define USE_SBS 0

#define LOG_IOT 0
#define LOG_IOT_EXTRA 0

/* IOT completion may take much more than the 5us instruction time.  A possible programming
error would be to execute an IOT before the last IOT of the same kind is over: such a thing
would confuse the targeted IO device, which might ignore the latter IOT, execute either
or both IOT incompletely, or screw up completely.  I insist that such an error can be caused
by a pdp-1 programming error, even if there is no emulator error. */
#define LOG_IOT_OVERLAP 0


static int tape_read(pdp1_state *state, UINT8 *reply);
static TIMER_CALLBACK(reader_callback);
static TIMER_CALLBACK(puncher_callback);
static TIMER_CALLBACK(tyo_callback);
static TIMER_CALLBACK(dpy_callback);
static void pdp1_machine_stop(running_machine &machine);

static void pdp1_tape_read_binary(device_t *device);
static void pdp1_io_sc_callback(device_t *device);

static void iot_rpa(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_rpb(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_rrb(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_ppa(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_ppb(device_t *device, int op2, int nac, int mb, int *io, int ac);

static void iot_tyo(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_tyi(device_t *device, int op2, int nac, int mb, int *io, int ac);

static void iot_dpy(device_t *device, int op2, int nac, int mb, int *io, int ac);

static void iot_dia(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_dba(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_dcc(device_t *device, int op2, int nac, int mb, int *io, int ac);
static void iot_dra(device_t *device, int op2, int nac, int mb, int *io, int ac);

static void iot_011(device_t *device, int op2, int nac, int mb, int *io, int ac);

static void iot_cks(device_t *device, int op2, int nac, int mb, int *io, int ac);


/*
    devices which are known to generate a completion pulse (source: maintainance manual 9-??,
    and 9-20, 9-21):
    emulated:
    * perforated tape reader
    * perforated tape punch
    * typewriter output
    * CRT display
    unemulated:
    * card punch (pac: 43)
    * line printer (lpr, lsp, but NOT lfb)

    This list should probably include additional optional devices (card reader comes to mind).
*/

/* IO status word */

/* defines for io_status bits */
enum
{
	io_st_pen = 0400000,	/* light pen: light has hit the pen */
	io_st_ptr = 0200000,	/* perforated tape reader: reader buffer full */
	io_st_tyo = 0100000,	/* typewriter out: device ready */
	io_st_tyi = 0040000,	/* typewriter in: new character in buffer */
	io_st_ptp = 0020000		/* perforated tape punch: device ready */
};







/* crt display timer */

/* light pen config */



#define PARALLEL_DRUM_WORD_TIME attotime::from_nsec(8500)
#define PARALLEL_DRUM_ROTATION_TIME attotime::from_nsec(8500*4096)


pdp1_reset_param_t pdp1_reset_param =
{
	{	/* external iot handlers.  NULL means that the iot is unimplemented, unless there are
        parentheses around the iot name, in which case the iot is internal to the cpu core. */
		/* I put a ? when the source is the handbook, since a) I have used the maintainance manual
        as the primary source (as it goes more into details) b) the handbook and the maintainance
        manual occasionnally contradict each other. */
		/* dia, dba, dcc, dra are documented in MIT PDP-1 COMPUTER MODIFICATION
        BULLETIN no. 2 (drumInstrWriteup.bin/drumInstrWriteup.txt), and are
        similar to IOT documented in Parallel Drum Type 23 Instruction Manual. */
	/*  (iot)       rpa         rpb         tyo         tyi         ppa         ppb         dpy */
		NULL,		iot_rpa,	iot_rpb,	iot_tyo,	iot_tyi,	iot_ppa,	iot_ppb,	iot_dpy,
	/*              spacewar                                                                 */
		NULL,		iot_011,	NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*                          lag                                             glf?/jsp?   gpl?/gpr?/gcf? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*  rrb         rcb?        rcc?        cks         mcs         mes         mel          */
		iot_rrb,	NULL,		NULL,		iot_cks,	NULL,		NULL,		NULL,		NULL,
	/*  cad?        rac?        rbc?        pac                     lpr/lfb/lsp swc/sci/sdf?/shr?   scv? */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*  (dsc)       (asc)       (isb)       (cac)       (lsm)       (esm)       (cbs)        */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	/*  icv?        dia         dba         dcc         dra                     mri|rlc?    mrf/inr?/ccr? */
		NULL,		iot_dia,	iot_dba,	iot_dcc,	iot_dra,	NULL,		NULL,		NULL,
	/*  mcb|dur?    mwc|mtf?    mrc|sfc?... msm|cgo?    (eem/lem)   mic         muf          */
		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,		NULL,
	},
	pdp1_tape_read_binary,
	pdp1_io_sc_callback,
	0,	/* extend mode support defined in input ports and pdp1_init_machine */
	0,	/* hardware multiply/divide support defined in input ports and pdp1_init_machine */
	0	/* type 20 sequence break system support defined in input ports and pdp1_init_machine */
};

MACHINE_RESET( pdp1 )
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int cfg;

	cfg = input_port_read(machine, "CFG");
	pdp1_reset_param.extend_support = (cfg >> pdp1_config_extend_bit) & pdp1_config_extend_mask;
	pdp1_reset_param.hw_mul_div = (cfg >> pdp1_config_hw_mul_div_bit) & pdp1_config_hw_mul_div_mask;
	pdp1_reset_param.type_20_sbs = (cfg >> pdp1_config_type_20_sbs_bit) & pdp1_config_type_20_sbs_mask;

	/* reset device state */
	state->m_tape_reader.rcl = state->m_tape_reader.rc = 0;
	state->m_io_status = io_st_tyo | io_st_ptp;
	state->m_lightpen.active = state->m_lightpen.down = 0;
	state->m_lightpen.x = state->m_lightpen.y = 0;
	state->m_lightpen.radius = 10;	/* ??? */
	pdp1_update_lightpen_state(machine, &state->m_lightpen);
}


static void pdp1_machine_stop(running_machine &machine)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	/* the core will take care of freeing the timers, BUT we must set the variables
    to NULL if we don't want to risk confusing the tape image init function */
	state->m_tape_reader.timer = state->m_tape_puncher.timer = state->m_typewriter.tyo_timer = state->m_dpy_timer = NULL;
}


/*
    driver init function

    Set up the pdp1_memory pointer, and generate font data.
*/
MACHINE_START( pdp1 )
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	UINT8 *dst;

	static const unsigned char fontdata6x8[pdp1_fontdata_size] =
	{	/* ASCII characters */
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00,
		0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xf8,0x50,0xf8,0x50,0x00,0x00,
		0x20,0x70,0xc0,0x70,0x18,0xf0,0x20,0x00,0x40,0xa4,0x48,0x10,0x20,0x48,0x94,0x08,
		0x60,0x90,0xa0,0x40,0xa8,0x90,0x68,0x00,0x10,0x20,0x40,0x00,0x00,0x00,0x00,0x00,
		0x20,0x40,0x40,0x40,0x40,0x40,0x20,0x00,0x10,0x08,0x08,0x08,0x08,0x08,0x10,0x00,
		0x20,0xa8,0x70,0xf8,0x70,0xa8,0x20,0x00,0x00,0x20,0x20,0xf8,0x20,0x20,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xf8,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x00,0x08,0x10,0x20,0x40,0x80,0x00,0x00,
		0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,0x10,0x30,0x10,0x10,0x10,0x10,0x10,0x00,
		0x70,0x88,0x08,0x10,0x20,0x40,0xf8,0x00,0x70,0x88,0x08,0x30,0x08,0x88,0x70,0x00,
		0x10,0x30,0x50,0x90,0xf8,0x10,0x10,0x00,0xf8,0x80,0xf0,0x08,0x08,0x88,0x70,0x00,
		0x70,0x80,0xf0,0x88,0x88,0x88,0x70,0x00,0xf8,0x08,0x08,0x10,0x20,0x20,0x20,0x00,
		0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00,0x70,0x88,0x88,0x88,0x78,0x08,0x70,0x00,
		0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00,0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x60,
		0x10,0x20,0x40,0x80,0x40,0x20,0x10,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,
		0x40,0x20,0x10,0x08,0x10,0x20,0x40,0x00,0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00,
		0x70,0x88,0xb8,0xa8,0xb8,0x80,0x70,0x00,0x70,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0xf0,0x00,0x70,0x88,0x80,0x80,0x80,0x88,0x70,0x00,
		0xf0,0x88,0x88,0x88,0x88,0x88,0xf0,0x00,0xf8,0x80,0x80,0xf0,0x80,0x80,0xf8,0x00,
		0xf8,0x80,0x80,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x80,0x98,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0xf8,0x88,0x88,0x88,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
		0x08,0x08,0x08,0x08,0x88,0x88,0x70,0x00,0x88,0x90,0xa0,0xc0,0xa0,0x90,0x88,0x00,
		0x80,0x80,0x80,0x80,0x80,0x80,0xf8,0x00,0x88,0xd8,0xa8,0x88,0x88,0x88,0x88,0x00,
		0x88,0xc8,0xa8,0x98,0x88,0x88,0x88,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0xf0,0x88,0x88,0xf0,0x80,0x80,0x80,0x00,0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x08,
		0xf0,0x88,0x88,0xf0,0x88,0x88,0x88,0x00,0x70,0x88,0x80,0x70,0x08,0x88,0x70,0x00,
		0xf8,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00,
		0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00,0x88,0x88,0x88,0x88,0xa8,0xd8,0x88,0x00,
		0x88,0x50,0x20,0x20,0x20,0x50,0x88,0x00,0x88,0x88,0x88,0x50,0x20,0x20,0x20,0x00,
		0xf8,0x08,0x10,0x20,0x40,0x80,0xf8,0x00,0x30,0x20,0x20,0x20,0x20,0x20,0x30,0x00,
		0x40,0x40,0x20,0x20,0x10,0x10,0x08,0x08,0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00,
		0x20,0x50,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfc,
		0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00,
		0x80,0x80,0xf0,0x88,0x88,0x88,0xf0,0x00,0x00,0x00,0x70,0x88,0x80,0x80,0x78,0x00,
		0x08,0x08,0x78,0x88,0x88,0x88,0x78,0x00,0x00,0x00,0x70,0x88,0xf8,0x80,0x78,0x00,
		0x18,0x20,0x70,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70,
		0x80,0x80,0xf0,0x88,0x88,0x88,0x88,0x00,0x20,0x00,0x20,0x20,0x20,0x20,0x20,0x00,
		0x20,0x00,0x20,0x20,0x20,0x20,0x20,0xc0,0x80,0x80,0x90,0xa0,0xe0,0x90,0x88,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0xf0,0xa8,0xa8,0xa8,0xa8,0x00,
		0x00,0x00,0xb0,0xc8,0x88,0x88,0x88,0x00,0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00,
		0x00,0x00,0xf0,0x88,0x88,0xf0,0x80,0x80,0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08,
		0x00,0x00,0xb0,0xc8,0x80,0x80,0x80,0x00,0x00,0x00,0x78,0x80,0x70,0x08,0xf0,0x00,
		0x20,0x20,0x70,0x20,0x20,0x20,0x18,0x00,0x00,0x00,0x88,0x88,0x88,0x98,0x68,0x00,
		0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00,0x00,0x00,0xa8,0xa8,0xa8,0xa8,0x50,0x00,
		0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00,0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70,
		0x00,0x00,0xf8,0x10,0x20,0x40,0xf8,0x00,0x08,0x10,0x10,0x20,0x10,0x10,0x08,0x00,
		0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x40,0x20,0x20,0x10,0x20,0x20,0x40,0x00,
		0x00,0x68,0xb0,0x00,0x00,0x00,0x00,0x00,0x20,0x50,0x20,0x50,0xa8,0x50,0x00,0x00,

		/* non-spacing middle dot */
		0x00,
		0x00,
		0x00,
		0x20,
		0x00,
		0x00,
		0x00,
		0x00,
		/* non-spacing overstrike */
		0x00,
		0x00,
		0x00,
		0x00,
		0xfc,
		0x00,
		0x00,
		0x00,
		/* implies */
		0x00,
		0x00,
		0x70,
		0x08,
		0x08,
		0x08,
		0x70,
		0x00,
		/* or */
		0x88,
		0x88,
		0x88,
		0x50,
		0x50,
		0x50,
		0x20,
		0x00,
		/* and */
		0x20,
		0x50,
		0x50,
		0x50,
		0x88,
		0x88,
		0x88,
		0x00,
		/* up arrow */
		0x20,
		0x70,
		0xa8,
		0x20,
		0x20,
		0x20,
		0x20,
		0x00,
		/* right arrow */
		0x00,
		0x20,
		0x10,
		0xf8,
		0x10,
		0x20,
		0x00,
		0x00,
		/* multiply */
		0x00,
		0x88,
		0x50,
		0x20,
		0x50,
		0x88,
		0x00,
		0x00,
	};

	/* set up our font */
	dst = machine.region("gfx1")->base();
	memcpy(dst, fontdata6x8, pdp1_fontdata_size);

	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(pdp1_machine_stop),&machine));

	state->m_tape_reader.timer = machine.scheduler().timer_alloc(FUNC(reader_callback));
	state->m_tape_puncher.timer = machine.scheduler().timer_alloc(FUNC(puncher_callback));
	state->m_typewriter.tyo_timer = machine.scheduler().timer_alloc(FUNC(tyo_callback));
	state->m_dpy_timer = machine.scheduler().timer_alloc(FUNC(dpy_callback));
}


/*
    perforated tape handling
*/

void pdp1_get_open_mode(int id, unsigned int *readable, unsigned int *writeable, unsigned int *creatable)
{
	/* unit 0 is read-only, unit 1 is write-only */
	if (id)
	{
		*readable = 0;
		*writeable = 1;
		*creatable = 1;
	}
	else
	{
		*readable = 1;
		*writeable = 1;
		*creatable = 1;
	}
}


#if 0
DEVICE_START( pdp1_tape )
{
}



/*
    Open a perforated tape image

    unit 0 is reader (read-only), unit 1 is puncher (write-only)
*/
DEVICE_IMAGE_LOAD( pdp1_tape )
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		state->m_tape_reader.fd = image;

		/* start motor */
		state->m_tape_reader.motor_on = 1;

		/* restart reader IO when necessary */
		/* note that this function may be called before pdp1_init_machine, therefore
        before state->m_tape_reader.timer is allocated.  It does not matter, as the clutch is never
        down at power-up, but we must not call timer_enable with a NULL parameter! */
		if (state->m_tape_reader.timer)
		{
			if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
			{
				/* delay is approximately 1/400s */
				state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
			}
			else
			{
				state->m_tape_reader.timer->enable(0);
			}
		}
		break;

	case 1:
		/* punch unit */
		state->m_tape_puncher.fd = image;
		break;
	}

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( pdp1_tape )
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	int id = image_index_in_device(image);

	switch (id)
	{
	case 0:
		/* reader unit */
		state->m_tape_reader.fd = NULL;

		/* stop motor */
		state->m_tape_reader.motor_on = 0;

		if (state->m_tape_reader.timer)
			state->m_tape_reader.timer->enable(0);
		break;

	case 1:
		/* punch unit */
		state->m_tape_puncher.fd = NULL;
		break;
	}
}
#endif
/*
    Read a byte from perforated tape
*/
static int tape_read(pdp1_state *state, UINT8 *reply)
{
	if (state->m_tape_reader.fd && (state->m_tape_reader.fd->fread(reply, 1) == 1))
		return 0;	/* unit OK */
	else
		return 1;	/* unit not ready */
}

/*
    Write a byte to perforated tape
*/
static void tape_write(pdp1_state *state, UINT8 data)
{
	if (state->m_tape_puncher.fd)
		state->m_tape_puncher.fd->fwrite(& data, 1);
}

/*
    common code for tape read commands (RPA, RPB, and read-in mode)
*/
static void begin_tape_read(pdp1_state *state, int binary, int nac)
{
	state->m_tape_reader.rb = 0;
	state->m_tape_reader.rcl = 1;
	state->m_tape_reader.rc = (binary) ? 1 : 3;
	state->m_tape_reader.rby = (binary) ? 1 : 0;
	state->m_tape_reader.rcp = nac;

	if (LOG_IOT_OVERLAP)
	{
		if (state->m_tape_reader.timer->enable(0))
			logerror("Error: overlapped perforated tape reads (Read-in mode, RPA/RPB instruction)\n");
	}
	/* set up delay if tape is advancing */
	if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
	{
		/* delay is approximately 1/400s */
		state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
	}
	else
	{
		state->m_tape_reader.timer->enable(0);
	}
}

/*
    timer callback to simulate reader IO
*/
static TIMER_CALLBACK(reader_callback)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int not_ready;
	UINT8 data;

	if (state->m_tape_reader.rc)
	{
		not_ready = tape_read(state, & data);
		if (not_ready)
		{
			state->m_tape_reader.motor_on = 0;	/* let us stop the motor */
		}
		else
		{
			if ((! state->m_tape_reader.rby) || (data & 0200))
			{
				state->m_tape_reader.rb |= (state->m_tape_reader.rby) ? (data & 077) : data;

				if (state->m_tape_reader.rc != 3)
				{
					state->m_tape_reader.rb <<= 6;
				}

				state->m_tape_reader.rc = (state->m_tape_reader.rc+1) & 3;

				if (state->m_tape_reader.rc == 0)
				{	/* IO complete */
					state->m_tape_reader.rcl = 0;
					if (state->m_tape_reader.rcp)
					{
						cpu_set_reg(machine.device("maincpu"), PDP1_IO, state->m_tape_reader.rb);	/* transfer reader buffer to IO */
						pdp1_pulse_iot_done(machine.device("maincpu"));
					}
					else
						state->m_io_status |= io_st_ptr;
				}
			}
		}
	}

	if (state->m_tape_reader.motor_on && state->m_tape_reader.rcl)
		/* delay is approximately 1/400s */
		state->m_tape_reader.timer->adjust(attotime::from_usec(2500));
	else
		state->m_tape_reader.timer->enable(0);
}

/*
    timer callback to generate punch completion pulse
*/
static TIMER_CALLBACK(puncher_callback)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int nac = param;
	state->m_io_status |= io_st_ptp;
	if (nac)
	{
		pdp1_pulse_iot_done(machine.device("maincpu"));
	}
}

/*
    Initiate read of a 18-bit word in binary format from tape (used in read-in mode)
*/
void pdp1_tape_read_binary(device_t *device)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	begin_tape_read(state, 1, 1);
}

/*
    perforated tape reader iot callbacks
*/

/*
    rpa iot callback

    op2: iot command operation (equivalent to mb & 077)
    nac: nead a completion pulse
    mb: contents of the MB register
    io: pointer on the IO register
    ac: contents of the AC register
*/
/*
 * Read Perforated Tape, Alphanumeric
 * rpa Address 0001
 *
 * This instruction reads one line of tape (all eight Channels) and transfers the resulting 8-bit code to
 * the Reader Buffer. If bits 5 and 6 of the rpa function are both zero (720001), the contents of the
 * Reader Buffer must be transferred to the IO Register by executing the rrb instruction. When the Reader
 * Buffer has information ready to be transferred to the IO Register, Status Register Bit 1 is set to
 * one. If bits 5 and 6 are different (730001 or 724001) the 8-bit code read from tape is automatically
 * transferred to the IO Register via the Reader Buffer and appears as follows:
 *
 * IO Bits        10 11 12 13 14 15 16 17
 * TAPE CHANNELS  8  7  6  5  4  3  2  1
 */
static void iot_rpa(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("Warning, RPA instruction not fully emulated: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	begin_tape_read(state, 0, nac);
}

/*
    rpb iot callback
*/
/*
 * Read Perforated Tape, Binary rpb
 * Address 0002
 *
 * The instruction reads three lines of tape (six Channels per line) and assembles
 * the resulting 18-bit word in the Reader Buffer.  For a line to be recognized by
 * this instruction Channel 8 must be punched (lines with Channel 8 not punched will
 * be skipped over).  Channel 7 is ignored.  The instruction sub 5137, for example,
 * appears on tape and is assembled by rpb as follows:
 *
 * Channel 8 7 6 5 4 | 3 2 1
 * Line 1  X   X     |   X
 * Line 2  X   X   X |     X
 * Line 3  X     X X | X X X
 * Reader Buffer 100 010 101 001 011 111
 *
 * (Vertical dashed line indicates sprocket holes and the symbols "X" indicate holes
 * punched in tape).
 *
 * If bits 5 and 6 of the rpb instruction are both zero (720002), the contents of
 * the Reader Buffer must be transferred to the IO Register by executing
 * a rrb instruction.  When the Reader Buffer has information ready to be transferred
 * to the IO Register, Status Register Bit 1 is set to one.  If bits 5 and 6 are
 * different (730002 or 724002) the 18-bit word read from tape is automatically
 * transferred to the IO Register via the Reader Buffer.
 */
static void iot_rpb(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("Warning, RPB instruction not fully emulated: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	begin_tape_read(state, 1, nac);
}

/*
    rrb iot callback
*/
static void iot_rrb(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("RRB instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	*io = state->m_tape_reader.rb;
	state->m_io_status &= ~io_st_ptr;
}

/*
    perforated tape punch iot callbacks
*/

/*
    ppa iot callback
*/
/*
 * Punch Perforated Tape, Alphanumeric
 * ppa Address 0005
 *
 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
 * Bit 17 conditions Hole 1. Bit 16 conditions Hole 2, etc. Bit 10 conditions Hole 8
 */
static void iot_ppa(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("PPA instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	tape_write(state, *io & 0377);
	state->m_io_status &= ~io_st_ptp;
	/* delay is approximately 1/63.3 second */
	if (LOG_IOT_OVERLAP)
	{
		if (state->m_tape_puncher.timer->enable(0))
			logerror("Error: overlapped PPA/PPB instructions: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());
	}

	state->m_tape_puncher.timer->adjust(attotime::from_usec(15800), nac);
}

/*
    ppb iot callback
*/
/*
 * Punch Perforated Tape, Binary
 * ppb Addres 0006
 *
 * For each In-Out Transfer instruction one line of tape is punched. In-Out Register
 * Bit 5 conditions Hole 1. Bit 4 conditions Hole 2, etc. Bit 0 conditions Hole 6.
 * Hole 7 is left blank. Hole 8 is always punched in this mode.
 */
static void iot_ppb(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("PPB instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	tape_write(state, (*io >> 12) | 0200);
	state->m_io_status &= ~io_st_ptp;
	/* delay is approximately 1/63.3 second */
	if (LOG_IOT_OVERLAP)
	{
		if (state->m_tape_puncher.timer->enable(0))
			logerror("Error: overlapped PPA/PPB instructions: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());
	}
	state->m_tape_puncher.timer->adjust(attotime::from_usec(15800), nac);
}


/*
    Typewriter handling

    The alphanumeric on-line typewriter is a standard device on pdp-1: it
    can both handle keyboard input and print output text.
*/

/*
    Open a file for typewriter output
*/
DEVICE_IMAGE_LOAD(pdp1_typewriter)
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	/* open file */
	state->m_typewriter.fd = &image;

	state->m_io_status |= io_st_tyo;

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(pdp1_typewriter)
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	state->m_typewriter.fd = NULL;
}

/*
    Write a character to typewriter
*/
static void typewriter_out(running_machine &machine, UINT8 data)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("typewriter output %o\n", data);

	pdp1_typewriter_drawchar(machine, data);
	if (state->m_typewriter.fd)
#if 1
		state->m_typewriter.fd->fwrite(& data, 1);
#else
	{
		static const char ascii_table[2][64] =
		{	/* n-s = non-spacing */
			{	/* lower case */
				' ',				'1',				'2',				'3',
				'4',				'5',				'6',				'7',
				'8',				'9',				'*',				'*',
				'*',				'*',				'*',				'*',
				'0',				'/',				's',				't',
				'u',				'v',				'w',				'x',
				'y',				'z',				'*',				',',
				'*',/*black*/		'*',/*red*/			'*',/*Tab*/			'*',
				'\200',/*n-s middle dot*/'j',			'k',				'l',
				'm',				'n',				'o',				'p',
				'q',				'r',				'*',				'*',
				'-',				')',				'\201',/*n-s overstrike*/'(',
				'*',				'a',				'b',				'c',
				'd',				'e',				'f',				'g',
				'h',				'i',				'*',/*Lower Case*/	'.',
				'*',/*Upper Case*/	'*',/*Backspace*/	'*',				'*'/*Carriage Return*/
			},
			{	/* upper case */
				' ',				'"',				'\'',				'~',
				'\202',/*implies*/	'\203',/*or*/		'\204',/*and*/		'<',
				'>',				'\205',/*up arrow*/	'*',				'*',
				'*',				'*',				'*',				'*',
				'\206',/*right arrow*/'?',				'S',				'T',
				'U',				'V',				'W',				'X',
				'Y',				'Z',				'*',				'=',
				'*',/*black*/		'*',/*red*/			'\t',/*Tab*/		'*',
				'_',/*n-s???*/		'J',				'K',				'L',
				'M',				'N',				'O',				'P',
				'Q',				'R',				'*',				'*',
				'+',				']',				'|',/*n-s???*/		'[',
				'*',				'A',				'B',				'C',
				'D',				'E',				'F',				'G',
				'H',				'I',				'*',/*Lower Case*/	'\207',/*multiply*/
				'*',/*Upper Case*/	'\b',/*Backspace*/	'*',				'*'/*Carriage Return*/
			}
		};

		data &= 0x3f;

		switch (data)
		{
		case 034:
			/* Black: ignore */
			//color = color_typewriter_black;
			{
				static const char black[5] = { '\033', '[', '3', '0', 'm' };
				image_fwrite(state->m_typewriter.fd, black, sizeof(black));
			}
			break;

		case 035:
			/* Red: ignore */
			//color = color_typewriter_red;
			{
				static const char red[5] = { '\033', '[', '3', '1', 'm' };
				image_fwrite(state->m_typewriter.fd, red, sizeof(red));
			}
			break;

		case 072:
			/* Lower case */
			state->m_case_shift = 0;
			break;

		case 074:
			/* Upper case */
			state->m_case_shift = 1;
			break;

		case 077:
			/* Carriage Return */
			{
				static const char line_end[2] = { '\r', '\n' };
				image_fwrite(state->m_typewriter.fd, line_end, sizeof(line_end));
			}
			break;

		default:
			/* Any printable character... */

			if ((data != 040) && (data != 056))	/* 040 and 056 are non-spacing characters: don't try to print right now */
				/* print character (lookup ASCII equivalent in table) */
				image_fwrite(state->m_typewriter.fd, & ascii_table[state->m_case_shift][data], 1);

			break;
		}
	}
#endif
}

/*
    timer callback to generate typewriter completion pulse
*/
static TIMER_CALLBACK(tyo_callback)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int nac = param;
	state->m_io_status |= io_st_tyo;
	if (nac)
	{
		pdp1_pulse_iot_done(machine.device("maincpu"));
	}
}

/*
    tyo iot callback
*/
static void iot_tyo(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	int ch, delay;

	if (LOG_IOT_EXTRA)
		logerror("Warning, TYO instruction not fully emulated: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	ch = (*io) & 077;

	typewriter_out(device->machine(), ch);
	state->m_io_status &= ~io_st_tyo;

	/* compute completion delay (source: maintainance manual 9-12, 9-13 and 9-14) */
	switch (ch)
	{
	case 072:	/* lower-case */
	case 074:	/* upper-case */
	case 034:	/* black */
	case 035:	/* red */
		delay = 175;	/* approximately 175ms (?) */
		break;

	case 077:	/* carriage return */
		delay = 205;	/* approximately 205ms (?) */
		break;

	default:
		delay = 105;	/* approximately 105ms */
		break;
	}
	if (LOG_IOT_OVERLAP)
	{
		if (state->m_typewriter.tyo_timer->enable(0))
			logerror("Error: overlapped TYO instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());
	}

	state->m_typewriter.tyo_timer->adjust(attotime::from_msec(delay), nac);
}

/*
    tyi iot callback
*/
/* When a typewriter key is struck, the code for the struck key is placed in the
 * typewriter buffer, Program Flag 1 is set, and the type-in status bit is set to
 * one. A program designed to accept typed-in data would periodically check
 * Program Flag 1, and if found to be set, an In-Out Transfer Instruction with
 * address 4 could be executed for the information to be transferred to the
 * In-Out Register. This In-Out Transfer should not use the optional in-out wait.
 * The information contained in the typewriter buffer is then transferred to the
 * right six bits of the In-Out Register. The tyi instruction automatically
 * clears the In-Out Register before transferring the information and also clears
 * the type-in status bit.
 */
static void iot_tyi(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("Warning, TYI instruction not fully emulated: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	*io = state->m_typewriter.tb;
	if (! (state->m_io_status & io_st_tyi))
		*io |= 0100;
	else
	{
		state->m_io_status &= ~io_st_tyi;
		if (USE_SBS)
			cputag_set_input_line_and_vector(device->machine(), "maincpu", 0, CLEAR_LINE, 0);	/* interrupt it, baby */
	}
}


/*
 * PRECISION CRT DISPLAY (TYPE 30)
 *
 * This sixteen-inch cathode ray tube display is intended to be used as an on-line output device for the
 * PDP-1. It is useful for high speed presentation of graphs, diagrams, drawings, and alphanumerical
 * information. The unit is solid state, self-powered and completely buffered. It has magnetic focus and
 * deflection.
 *
 * Display characteristics are as follows:
 *
 *     Random point plotting
 *     Accuracy of points +/-3 per cent of raster size
 *     Raster size 9.25 by 9.25 inches
 *     1024 by 1024 addressable locations
 *     Fixed origin at center of CRT
 *     Ones complement binary arithmetic
 *     Plots 20,000 points per second
 *
 * Resolution is such that 512 points along each axis are discernable on the face of the tube.
 *
 * One instruction is added to the PDP-1 with the installation of this display:
 *
 * Display One Point On CRT
 * dpy Address 0007
 *
 * This instruction clears the light pen status bit and displays one point using bits 0 through 9 of the
 * AC to represent the (signed) X coordinate of the point and bits 0 through 9 of the IO as the (signed)
 * Y coordinate.
 *
 * Many variations of the Type 30 Display are available. Some of these include hardware for line and
 * curve generation.
 */


/*
    timer callback to generate crt completion pulse
*/
static TIMER_CALLBACK(dpy_callback)
{
	pdp1_pulse_iot_done(machine.device("maincpu"));
}


/*
    dpy iot callback

    Light on one pixel on CRT
*/
static void iot_dpy(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	int x;
	int y;


	x = ((ac+0400000) & 0777777) >> 8;
	y = (((*io)+0400000) & 0777777) >> 8;
	pdp1_plot(device->machine(), x, y);

	/* light pen 32 support */
	state->m_io_status &= ~io_st_pen;

	if (state->m_lightpen.down && ((x-state->m_lightpen.x)*(x-state->m_lightpen.x)+(y-state->m_lightpen.y)*(y-state->m_lightpen.y) < state->m_lightpen.radius*state->m_lightpen.radius))
	{
		state->m_io_status |= io_st_pen;

		cpu_set_reg(device->machine().device("maincpu"), PDP1_PF3, 1);
	}

	if (nac)
	{
		/* 50us delay */
		if (LOG_IOT_OVERLAP)
		{
			/* note that overlap detection is incomplete: it will only work if both DPY
            instructions require a completion pulse */
			if (state->m_dpy_timer->enable(0))
				logerror("Error: overlapped DPY instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());
		}
		state->m_dpy_timer->adjust(attotime::from_usec(50));
	}
}



/*
    MIT parallel drum (variant of type 23)
*/

static void parallel_drum_set_il(pdp1_state *state, int il)
{
	attotime il_phase;

	state->m_parallel_drum.il = il;

	il_phase = ((PARALLEL_DRUM_WORD_TIME * il) - state->m_parallel_drum.rotation_timer->elapsed());
	if (il_phase < attotime::zero)
		il_phase = il_phase + PARALLEL_DRUM_ROTATION_TIME;
	state->m_parallel_drum.il_timer->adjust(il_phase, 0, PARALLEL_DRUM_ROTATION_TIME);
}

#ifdef UNUSED_FUNCTION
static TIMER_CALLBACK(il_timer_callback)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	if (state->m_parallel_drum.dba)
	{
		/* set break request and status bit 5 */
		/* ... */
	}
}

static void parallel_drum_init(pdp1_state *state)
{
	state->m_parallel_drum.rotation_timer = machine.scheduler().timer_alloc();
	state->m_parallel_drum.rotation_timer->adjust(PARALLEL_DRUM_ROTATION_TIME, 0, PARALLEL_DRUM_ROTATION_TIME);

	state->m_parallel_drum.il_timer = machine.scheduler().timer_alloc(FUNC(il_timer_callback));
	parallel_drum_set_il(0);
}
#endif

/*
    Open a file for drum
*/
DEVICE_IMAGE_LOAD(pdp1_drum)
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	/* open file */
	state->m_parallel_drum.fd = &image;

	return IMAGE_INIT_PASS;
}

DEVICE_IMAGE_UNLOAD(pdp1_drum)
{
	pdp1_state *state = image.device().machine().driver_data<pdp1_state>();
	state->m_parallel_drum.fd = NULL;
}

static void iot_dia(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	state->m_parallel_drum.wfb = ((*io) & 0370000) >> 12;
	parallel_drum_set_il(state, (*io) & 0007777);

	state->m_parallel_drum.dba = 0;	/* right? */
}

static void iot_dba(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	state->m_parallel_drum.wfb = ((*io) & 0370000) >> 12;
	parallel_drum_set_il(state, (*io) & 0007777);

	state->m_parallel_drum.dba = 1;
}

/*
    Read a word from drum
*/
static UINT32 drum_read(pdp1_state *state, int field, int position)
{
	int offset = (field*4096+position)*3;
	UINT8 buf[3];

	if (state->m_parallel_drum.fd && (!state->m_parallel_drum.fd->fseek(offset, SEEK_SET)) && (state->m_parallel_drum.fd->fread( buf, 3) == 3))
		return ((buf[0] << 16) | (buf[1] << 8) | buf[2]) & 0777777;

	return 0;
}

/*
    Write a word to drum
*/
static void drum_write(pdp1_state *state, int field, int position, UINT32 data)
{
	int offset = (field*4096+position)*3;
	UINT8 buf[3];

	if (state->m_parallel_drum.fd)
	{
		buf[0] = data >> 16;
		buf[1] = data >> 8;
		buf[2] = data;

		state->m_parallel_drum.fd->fseek(offset, SEEK_SET);
		state->m_parallel_drum.fd->fwrite( buf, 3);
	}
}

static void iot_dcc(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	attotime delay;
	int dc;

	state->m_parallel_drum.rfb = ((*io) & 0370000) >> 12;
	state->m_parallel_drum.wc = - ((*io) & 0007777);

	state->m_parallel_drum.wcl = ac & 0177777/*0007777???*/;

	state->m_parallel_drum.dba = 0;	/* right? */
	/* clear status bit 5... */

	/* do transfer */
	delay = state->m_parallel_drum.il_timer->remaining();
	dc = state->m_parallel_drum.il;
	do
	{
		if ((state->m_parallel_drum.wfb >= 1) && (state->m_parallel_drum.wfb <= 22))
		{
			drum_write(state, state->m_parallel_drum.wfb-1, dc, (signed)device->machine().device("maincpu")->memory().space(AS_PROGRAM)->read_dword(state->m_parallel_drum.wcl<<2));
		}

		if ((state->m_parallel_drum.rfb >= 1) && (state->m_parallel_drum.rfb <= 22))
		{
			device->machine().device("maincpu")->memory().space(AS_PROGRAM)->write_dword(state->m_parallel_drum.wcl<<2, drum_read(state, state->m_parallel_drum.rfb-1, dc));
		}

		state->m_parallel_drum.wc = (state->m_parallel_drum.wc+1) & 07777;
		state->m_parallel_drum.wcl = (state->m_parallel_drum.wcl+1) & 0177777/*0007777???*/;
		dc = (dc+1) & 07777;
		if (state->m_parallel_drum.wc)
			delay = delay + PARALLEL_DRUM_WORD_TIME;
	} while (state->m_parallel_drum.wc);
	device_adjust_icount(device->machine().device("maincpu"),-device->machine().device<cpu_device>("maincpu")->attotime_to_cycles(delay));
	/* if no error, skip */
	cpu_set_reg(device->machine().device("maincpu"), PDP1_PC, cpu_get_reg(device->machine().device("maincpu"), PDP1_PC)+1);
}

static void iot_dra(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	(*io) = (state->m_parallel_drum.rotation_timer->elapsed() *
		(ATTOSECONDS_PER_SECOND / (PARALLEL_DRUM_WORD_TIME.as_attoseconds()))).seconds & 0007777;

	/* set parity error and timing error... */
}



/*
    iot 11 callback

    Read state of Spacewar! controllers

    Not documented, except a few comments in the Spacewar! source code:
        it should leave the control word in the io as follows.
        high order 4 bits, rotate ccw, rotate cw, (both mean hyperspace)
        fire rocket, and fire torpedo. low order 4 bits, same for
        other ship. routine is entered by jsp cwg.
*/
static void iot_011(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	int key_state = input_port_read(device->machine(), "SPACEWAR");
	int reply;


	reply = 0;

	if (key_state & ROTATE_RIGHT_PLAYER2)
		reply |= 0400000;
	if (key_state & ROTATE_LEFT_PLAYER2)
		reply |= 0200000;
	if (key_state & THRUST_PLAYER2)
		reply |= 0100000;
	if (key_state & FIRE_PLAYER2)
		reply |= 0040000;
	if (key_state & HSPACE_PLAYER2)
		reply |= 0600000;

	if (key_state & ROTATE_RIGHT_PLAYER1)
		reply |= 010;
	if (key_state & ROTATE_LEFT_PLAYER1)
		reply |= 004;
	if (key_state & THRUST_PLAYER1)
		reply |= 002;
	if (key_state & FIRE_PLAYER1)
		reply |= 001;
	if (key_state & HSPACE_PLAYER1)
		reply |= 014;

	*io = reply;
}


/*
    cks iot callback

    check IO status
*/
static void iot_cks(device_t *device, int op2, int nac, int mb, int *io, int ac)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	if (LOG_IOT_EXTRA)
		logerror("CKS instruction: mb=0%06o, (%s)\n", (unsigned) mb, device->machine().describe_context());

	*io = state->m_io_status;
}


/*
    callback which is called when Start Clear is pulsed

    IO devices should reset
*/
void pdp1_io_sc_callback(device_t *device)
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	state->m_tape_reader.rcl = state->m_tape_reader.rc = 0;
	if (state->m_tape_reader.timer)
		state->m_tape_reader.timer->enable(0);

	if (state->m_tape_puncher.timer)
		state->m_tape_puncher.timer->enable(0);

	if (state->m_typewriter.tyo_timer)
		state->m_typewriter.tyo_timer->enable(0);

	if (state->m_dpy_timer)
		state->m_dpy_timer->enable(0);

	state->m_io_status = io_st_tyo | io_st_ptp;
}


/*
    typewriter keyboard handler
*/
static void pdp1_keyboard(running_machine &machine)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int i;
	int j;

	int typewriter_keys[4];

	int typewriter_transitions;
	static const char *const twrnames[] = { "TWR0", "TWR1", "TWR2", "TWR3" };


	for (i=0; i<4; i++)
	{
		typewriter_keys[i] = input_port_read(machine, twrnames[i]);
	}

	for (i=0; i<4; i++)
	{
		typewriter_transitions = typewriter_keys[i] & (~ state->m_old_typewriter_keys[i]);
		if (typewriter_transitions)
		{
			for (j=0; (((typewriter_transitions >> j) & 1) == 0) /*&& (j<16)*/; j++)
				;
			state->m_typewriter.tb = (i << 4) + j;
			state->m_io_status |= io_st_tyi;
			#if USE_SBS
				cputag_set_input_line_and_vector(machine, "maincpu", 0, ASSERT_LINE, 0);	/* interrupt it, baby */
			#endif
			cpu_set_reg(machine.device("maincpu"), PDP1_PF1, 1);
			pdp1_typewriter_drawchar(machine, state->m_typewriter.tb);	/* we want to echo input */
			break;
		}
	}

	for (i=0; i<4; i++)
		state->m_old_typewriter_keys[i] = typewriter_keys[i];
}

static void pdp1_lightpen(running_machine &machine)
{
	pdp1_state *state = machine.driver_data<pdp1_state>();
	int x_delta, y_delta;
	int current_state;

	state->m_lightpen.active = (input_port_read(machine, "CFG") >> pdp1_config_lightpen_bit) & pdp1_config_lightpen_mask;

	current_state = input_port_read(machine, "LIGHTPEN");

	/* update pen down state */
	state->m_lightpen.down = state->m_lightpen.active && (current_state & pdp1_lightpen_down);

	/* update size of pen tip hole */
	if ((current_state & ~state->m_old_lightpen) & pdp1_lightpen_smaller)
	{
		state->m_lightpen.radius --;
		if (state->m_lightpen.radius < 0)
			state->m_lightpen.radius = 0;
	}
	if ((current_state & ~state->m_old_lightpen) & pdp1_lightpen_larger)
	{
		state->m_lightpen.radius ++;
		if (state->m_lightpen.radius > 32)
			state->m_lightpen.radius = 32;
	}

	state->m_old_lightpen = current_state;

	/* update pen position */
	x_delta = input_port_read(machine, "LIGHTX");
	y_delta = input_port_read(machine, "LIGHTY");

	if (x_delta >= 0x80)
		x_delta -= 0x100;
	if (y_delta >= 0x80)
		y_delta -= 256;

	state->m_lightpen.x += x_delta;
	state->m_lightpen.y += y_delta;

	if (state->m_lightpen.x < 0)
		state->m_lightpen.x = 0;
	if (state->m_lightpen.x > 1023)
		state->m_lightpen.x = 1023;
	if (state->m_lightpen.y < 0)
		state->m_lightpen.y = 0;
	if (state->m_lightpen.y > 1023)
		state->m_lightpen.y = 1023;

	pdp1_update_lightpen_state(machine, &state->m_lightpen);
}

/*
    Not a real interrupt - just handle keyboard input
*/
INTERRUPT_GEN( pdp1_interrupt )
{
	pdp1_state *state = device->machine().driver_data<pdp1_state>();
	running_machine &machine = device->machine();
	int control_keys;
	int tw_keys;
	int ta_keys;

	int control_transitions;
	int tw_transitions;
	int ta_transitions;


	cpu_set_reg(device, PDP1_SS, input_port_read(device->machine(), "SENSE"));

	/* read new state of control keys */
	control_keys = input_port_read(device->machine(), "CSW");

	if (control_keys & pdp1_control)
	{
		/* compute transitions */
		control_transitions = control_keys & (~ state->m_old_control_keys);

		if (control_transitions & pdp1_extend)
		{
			cpu_set_reg(device, PDP1_EXTEND_SW, ! cpu_get_reg(device, PDP1_EXTEND_SW));
		}
		if (control_transitions & pdp1_start_nobrk)
		{
			pdp1_pulse_start_clear(device);	/* pulse Start Clear line */
			cpu_set_reg(device, PDP1_EXD, cpu_get_reg(device, PDP1_EXTEND_SW));
			cpu_set_reg(device, PDP1_SBM, (UINT64)0);
			cpu_set_reg(device, PDP1_OV, (UINT64)0);
			cpu_set_reg(device, PDP1_PC, cpu_get_reg(device, PDP1_TA));
			cpu_set_reg(device, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_start_brk)
		{
			pdp1_pulse_start_clear(device);	/* pulse Start Clear line */
			cpu_set_reg(device, PDP1_EXD, cpu_get_reg(device, PDP1_EXTEND_SW));
			cpu_set_reg(device, PDP1_SBM, 1);
			cpu_set_reg(device, PDP1_OV, (UINT64)0);
			cpu_set_reg(device, PDP1_PC, cpu_get_reg(device, PDP1_TA));
			cpu_set_reg(device, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_stop)
		{
			cpu_set_reg(device, PDP1_RUN, (UINT64)0);
			cpu_set_reg(device, PDP1_RIM, (UINT64)0);	/* bug : we stop after reading an even-numbered word
                                            (i.e. data), whereas a real pdp-1 stops after reading
                                            an odd-numbered word (i.e. dio instruciton) */
		}
		if (control_transitions & pdp1_continue)
		{
			cpu_set_reg(device, PDP1_RUN, 1);
		}
		if (control_transitions & pdp1_examine)
		{
			pdp1_pulse_start_clear(device);	/* pulse Start Clear line */
			cpu_set_reg(device, PDP1_PC, cpu_get_reg(device, PDP1_TA));
			cpu_set_reg(device, PDP1_MA, cpu_get_reg(device, PDP1_PC));
			cpu_set_reg(device, PDP1_IR, LAC);	/* this instruction is actually executed */

			cpu_set_reg(device, PDP1_MB, (signed)device->memory().space(AS_PROGRAM)->read_dword(PDP1_MA<<2));
			cpu_set_reg(device, PDP1_AC, cpu_get_reg(device, PDP1_MB));
		}
		if (control_transitions & pdp1_deposit)
		{
			pdp1_pulse_start_clear(device);	/* pulse Start Clear line */
			cpu_set_reg(device, PDP1_PC, cpu_get_reg(device, PDP1_TA));
			cpu_set_reg(device, PDP1_MA, cpu_get_reg(device, PDP1_PC));
			cpu_set_reg(device, PDP1_AC, cpu_get_reg(device, PDP1_TW));
			cpu_set_reg(device, PDP1_IR, DAC);	/* this instruction is actually executed */

			cpu_set_reg(device, PDP1_MB, cpu_get_reg(device, PDP1_AC));
			device->memory().space(AS_PROGRAM)->write_dword(cpu_get_reg(device, PDP1_MA)<<2, cpu_get_reg(device, PDP1_MB));
		}
		if (control_transitions & pdp1_read_in)
		{	/* set cpu to read instructions from perforated tape */
			pdp1_pulse_start_clear(device);	/* pulse Start Clear line */
			cpu_set_reg(device, PDP1_PC, (  cpu_get_reg(device, PDP1_TA) & 0170000)
										|  (cpu_get_reg(device, PDP1_PC) & 0007777));	/* transfer ETA to EPC */
			/*cpu_set_reg(machine.device("maincpu"), PDP1_MA, cpu_get_reg(machine.device("maincpu"), PDP1_PC));*/
			cpu_set_reg(device, PDP1_EXD, cpu_get_reg(device, PDP1_EXTEND_SW));
			cpu_set_reg(device, PDP1_OV, (UINT64)0);		/* right??? */
			cpu_set_reg(device, PDP1_RUN, (UINT64)0);
			cpu_set_reg(device, PDP1_RIM, 1);
		}
		if (control_transitions & pdp1_reader)
		{
		}
		if (control_transitions & pdp1_tape_feed)
		{
		}
		if (control_transitions & pdp1_single_step)
		{
			cpu_set_reg(device, PDP1_SNGL_STEP, ! cpu_get_reg(device, PDP1_SNGL_STEP));
		}
		if (control_transitions & pdp1_single_inst)
		{
			cpu_set_reg(device, PDP1_SNGL_INST, ! cpu_get_reg(device, PDP1_SNGL_INST));
		}

		/* remember new state of control keys */
		state->m_old_control_keys = control_keys;


		/* handle test word keys */
		tw_keys = (input_port_read(machine, "TWDMSB") << 16) | input_port_read(machine, "TWDLSB");

		/* compute transitions */
		tw_transitions = tw_keys & (~ state->m_old_tw_keys);

		if (tw_transitions)
			cpu_set_reg(device, PDP1_TW, cpu_get_reg(device, PDP1_TW) ^ tw_transitions);

		/* remember new state of test word keys */
		state->m_old_tw_keys = tw_keys;


		/* handle address keys */
		ta_keys = input_port_read(machine, "TSTADD");

		/* compute transitions */
		ta_transitions = ta_keys & (~ state->m_old_ta_keys);

		if (ta_transitions)
			cpu_set_reg(device, PDP1_TA, cpu_get_reg(device, PDP1_TA) ^ ta_transitions);

		/* remember new state of test word keys */
		state->m_old_ta_keys = ta_keys;

	}
	else
	{
		state->m_old_control_keys = 0;
		state->m_old_tw_keys = 0;
		state->m_old_ta_keys = 0;

		pdp1_keyboard(machine);
	}

	pdp1_lightpen(machine);
}
