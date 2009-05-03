/******************************************************************************
 *	Sharp MZ700
 *
 *	machine driver
 *
 *	Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Reference: http://sharpmz.computingmuseum.com
 *
 *****************************************************************************/

#include "driver.h"
#include "includes/mz700.h"
#include "cpu/z80/z80.h"
#include "machine/pit8253.h"
#include "machine/8255ppi.h"
#include "machine/z80pio.h"
#include "machine/74145.h"
#include "machine/ctronics.h"
#include "sound/speaker.h"
#include "devices/cassette.h"


#ifndef VERBOSE
#define VERBOSE 1
#endif

#define LOG(N,M,A,mac)	\
	do { \
		if(VERBOSE>=N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s",attotime_to_double(timer_get_time(mac)), (const char*)M ); \
			logerror A; \
		} \
	} while (0)


static READ8_DEVICE_HANDLER ( pio_port_b_r );
static READ8_DEVICE_HANDLER ( pio_port_c_r );
static WRITE8_DEVICE_HANDLER ( pio_port_a_w );
static WRITE8_DEVICE_HANDLER ( pio_port_c_w );

const ppi8255_interface mz700_ppi8255_interface =
{
	DEVCB_NULL,
	DEVCB_DEVICE_HANDLER("ls145", pio_port_b_r),
	DEVCB_HANDLER(pio_port_c_r),
	DEVCB_DEVICE_HANDLER("ls145", pio_port_a_w),
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_c_w)
};


static PIT8253_OUTPUT_CHANGED( pit_out0_changed );
static PIT8253_OUTPUT_CHANGED( pit_out1_changed );
static PIT8253_OUTPUT_CHANGED( pit_irq_2 );

const struct pit8253_config mz700_pit8253_config =
{
	{
		/* clockin	           callback */
		{ XTAL_17_73447MHz/20, pit_out0_changed },
		{	          15611.0, pit_out1_changed },
		{		            0, pit_irq_2        },
	}
};

const struct pit8253_config mz800_pit8253_config =
{
	{
		/* clockin	           callback */
		{ XTAL_17_73447MHz/16, pit_out0_changed },
		{	          15611.0, pit_out1_changed },
		{		            0, pit_irq_2        },
	}
};


/***************************************************************************
    INITIALIZATIoN
***************************************************************************/

DRIVER_INIT( mz700 )
{
	mz_state *mz = machine->driver_data;
	mz->mz700 = TRUE;
	mz->mz700_mode = TRUE;

	videoram_size = 0x800;
	videoram = auto_malloc(videoram_size);
	colorram = auto_malloc(0x800);
}

DRIVER_INIT( mz800 )
{
	mz_state *mz = machine->driver_data;
	mz->mz700 = FALSE;
	mz->mz700_mode = FALSE;

	/* video ram */
	videoram_size = 0x4000;
	videoram = auto_malloc(videoram_size);
	colorram = videoram + 0x800;

	/* character generator ram */
	mz->cgram = auto_malloc(0x1000);
}

MACHINE_START( mz700 )
{
	mz_state *mz = machine->driver_data;

	mz->pit = devtag_get_device(machine, "pit8253");
	mz->ppi = devtag_get_device(machine, "ppi8255");

	/* reset memory map to defaults */
	mz_bank_4_w(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, 0);
}


/***************************************************************************
    MMIO
***************************************************************************/

static READ8_HANDLER( mz700_e008_r )
{
	mz_state *mz = space->machine->driver_data;
	UINT8 data = 0;

	data |= mz->other_timer;
	data |= input_port_read(space->machine, "JOY");
	data |= video_screen_get_hblank(space->machine->primary_screen);

	LOG(1, "mz700_e008_r", ("%02X\n", data), space->machine);

	return data;
}


/***************************************************************************
    BANK SWITCHING
***************************************************************************/

READ8_HANDLER( mz800_bank_0_r )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	/* switch in cgrom */
	memory_install_readwrite8_handler(spc, 0x1000, 0x1fff, 0, 0, SMH_BANK(2), SMH_NOP);
	memory_set_bankptr(space->machine, 2, memory_region(space->machine, "monitor") + 0x1000);

	if (mz->mz700_mode)
	{
		/* cgram from 0xc000 to 0xcfff */
		memory_install_readwrite8_handler(spc, 0xc000, 0xcfff, 0, 0, SMH_BANK(6), mz800_cgram_w);
		memory_set_bankptr(space->machine, 6, mz->cgram);
	}
	else
	{
		if (mz->hires_mode)
		{
			/* vram from 0x8000 to 0xbfff */
			memory_install_readwrite8_handler(spc, 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
			memory_set_bankptr(space->machine, 4, videoram);
		}
		else
		{
			/* vram from 0x8000 to 0x9fff */
			memory_install_readwrite8_handler(spc, 0x8000, 0x9fff, 0, 0, SMH_BANK(4), SMH_BANK(4));
			memory_set_bankptr(space->machine, 4, videoram);

			/* ram from 0xa000 to 0xbfff */
			memory_install_readwrite8_handler(spc, 0xa000, 0xbfff, 0, 0, SMH_BANK(5), SMH_BANK(5));
			memory_set_bankptr(space->machine, 5, mess_ram + 0xa000);
		}
	}

	return 0xff;
}

WRITE8_HANDLER( mz700_bank_0_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_readwrite8_handler(spc, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_set_bankptr(space->machine, 1, mess_ram);
}

WRITE8_HANDLER( mz800_bank_0_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_readwrite8_handler(spc, 0x0000, 0x7fff, 0, 0, SMH_BANK(1), SMH_BANK(1));
	memory_set_bankptr(space->machine, 1, mess_ram);
}

READ8_HANDLER( mz800_bank_1_r )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	/* switch in ram from 0x1000 to 0x1fff */
	memory_install_readwrite8_handler(spc, 0x1000, 0x1fff, 0x1000, 0, SMH_BANK(2), SMH_BANK(2));
	memory_set_bankptr(space->machine, 2, mess_ram + 0x1000);

	if (mz->mz700_mode)
	{
		/* ram from 0xc000 to 0xcfff */
		memory_install_readwrite8_handler(spc, 0xc000, 0xcfff, 0, 0, SMH_BANK(6), SMH_BANK(6));
		memory_set_bankptr(space->machine, 6, mess_ram + 0xc000);
	}
	else
	{
		/* ram from 0x8000 to 0xbfff */
		memory_install_readwrite8_handler(spc, 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
		memory_set_bankptr(space->machine, 4, mess_ram + 0x8000);
	}

	return 0xff;
}

WRITE8_HANDLER( mz_bank_1_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	if (mz->mz700_mode)
	{
		/* switch in ram when not locked */
		if (!mz->mz700_ram_lock)
		{
			memory_install_readwrite8_handler(spc, 0xd000, 0xffff, 0, 0, SMH_BANK(7), SMH_BANK(7));
			memory_set_bankptr(space->machine, 7, mess_ram + 0xd000);
			mz->mz700_ram_vram = FALSE;
		}
	}
	else
	{
		/* switch in ram when not locked */
		if (!mz->mz800_ram_lock)
		{
			memory_install_readwrite8_handler(spc, 0xe000, 0xffff, 0, 0, SMH_BANK(8), SMH_BANK(8));
			memory_set_bankptr(space->machine, 8, mess_ram + 0xe000);
			mz->mz800_ram_monitor = FALSE;
		}
	}
}

WRITE8_HANDLER( mz_bank_2_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_install_readwrite8_handler(spc, 0x0000, 0x0fff, 0, 0, SMH_BANK(1), SMH_NOP);
	memory_set_bankptr(space->machine, 1, memory_region(space->machine, "monitor"));
}

WRITE8_HANDLER( mz_bank_3_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	if (mz->mz700_mode)
	{
		if (!mz->mz700_ram_lock)
		{
			/* switch in videoram */
			memory_install_readwrite8_handler(spc, 0xd000, 0xd7ff, 0, 0, SMH_BANK(7), SMH_BANK(7));
			memory_set_bankptr(space->machine, 7, videoram);

			/* switch in colorram */
			memory_install_readwrite8_handler(spc, 0xd800, 0xdfff, 0, 0, SMH_BANK(9), SMH_BANK(9));
			memory_set_bankptr(space->machine, 9, colorram);

			mz->mz700_ram_vram = TRUE;

			/* switch in memory mapped i/o devices */
			if (mz->mz700)
			{
				memory_install_readwrite8_device_handler(spc, mz->ppi, 0xe000, 0xfff3, 0, 0x1ff0, ppi8255_r, ppi8255_w);
				memory_install_readwrite8_device_handler(spc, mz->pit, 0xe004, 0xfff7, 0, 0x1ff0, pit8253_r, pit8253_w);
				memory_install_read8_handler(spc, 0xe008, 0xfff8, 0, 0x1ff0, mz700_e008_r);
				memory_install_write8_device_handler(spc, mz->pit, 0xe008, 0xfff8, 0, 0x1ff0, pit8253_gate_w);
			}
			else
			{
				memory_install_readwrite8_device_handler(spc, mz->ppi, 0xe000, 0xe003, 0, 0, ppi8255_r, ppi8255_w);
				memory_install_readwrite8_device_handler(spc, mz->pit, 0xe004, 0xe007, 0, 0, pit8253_r, pit8253_w);
				memory_install_read8_handler(spc, 0xe008, 0xe008, 0, 0, mz700_e008_r);
				memory_install_write8_device_handler(spc, mz->pit, 0xe008, 0xe008, 0, 0, pit8253_gate_w);
			}
		}
	}
	else
	{
		if (!mz->mz800_ram_lock)
		{
			/* switch in mz800 monitor rom if not locked */
			memory_install_readwrite8_handler(spc, 0xe000, 0xffff, 0, 0, SMH_BANK(8), SMH_NOP);
			memory_set_bankptr(space->machine, 8, memory_region(space->machine, "monitor") + 0x2000);
			mz->mz800_ram_monitor = TRUE;
		}
	}
}

WRITE8_HANDLER( mz_bank_4_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	if (mz->mz700_mode)
	{
		mz->mz700_ram_lock = FALSE;		/* reset lock */
		mz_bank_2_w(space, 0, 0);	/* switch in monitor rom */
		mz_bank_3_w(space, 0, 0);	/* switch in videoram, colorram, and mmio */

		/* rest is ram is always ram in mz700 mode */
		memory_install_readwrite8_handler(spc, 0x1000, 0xcfff, 0, 0, SMH_BANK(2), SMH_BANK(2));
		memory_set_bankptr(space->machine, 2, mess_ram + 0x1000);
	}
	else
	{
		/* monitor rom and cgrom */
		memory_install_readwrite8_handler(spc, 0x0000, 0x1fff, 0, 0, SMH_BANK(1), SMH_NOP);
		memory_set_bankptr(space->machine, 1, memory_region(space->machine, "monitor"));

		/* ram from 0x2000 to 0x7fff */
		memory_install_readwrite8_handler(spc, 0x2000, 0x7fff, 0, 0, SMH_BANK(3), SMH_BANK(3));
		memory_set_bankptr(space->machine, 3, mess_ram);

		if (mz->hires_mode)
		{
			/* vram from 0x8000 to 0xbfff */
			memory_install_readwrite8_handler(spc, 0x8000, 0xbfff, 0, 0, SMH_BANK(4), SMH_BANK(4));
			memory_set_bankptr(space->machine, 4, videoram);
		}
		else
		{
			/* vram from 0x8000 to 0x9fff */
			memory_install_readwrite8_handler(spc, 0x8000, 0x9fff, 0, 0, SMH_BANK(4), SMH_BANK(4));
			memory_set_bankptr(space->machine, 4, videoram);

			/* ram from 0xa000 to 0xbfff */
			memory_install_readwrite8_handler(spc, 0xa000, 0xbfff, 0, 0, SMH_BANK(5), SMH_BANK(5));
			memory_set_bankptr(space->machine, 5, mess_ram + 0xa000);
		}

		/* ram from 0xc000 to 0xdfff */
		memory_install_readwrite8_handler(spc, 0xc000, 0xdfff, 0, 0, SMH_BANK(6), SMH_BANK(6));
		memory_set_bankptr(space->machine, 6, mess_ram + 0xc000);

		/* mz800 monitor rom from 0xe000 to 0xffff */
		memory_install_readwrite8_handler(spc, 0xe000, 0xffff, 0, 0, SMH_BANK(8), SMH_NOP);
		memory_set_bankptr(space->machine, 8, memory_region(space->machine, "monitor") + 0x2000);
		mz->mz800_ram_monitor = TRUE;

		mz->mz800_ram_lock = FALSE; /* reset lock? */
	}
}

WRITE8_HANDLER( mz_bank_5_w )
{
	const address_space *spc = cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	mz_state *mz = space->machine->driver_data;

	if (mz->mz700_mode)
	{
		/* prevent access from 0xd000 to 0xffff */
		mz->mz700_ram_lock = TRUE;
		memory_install_readwrite8_handler(spc, 0xd000, 0xffff, 0, 0, SMH_NOP, SMH_NOP);
	}
	else
	{
		/* prevent access from 0xe000 to 0xffff */
		mz->mz800_ram_lock = TRUE;
		memory_install_readwrite8_handler(spc, 0xe000, 0xffff, 0, 0, SMH_NOP, SMH_NOP);
	}
}

WRITE8_HANDLER( mz_bank_6_w )
{
	mz_state *mz = space->machine->driver_data;

	if (mz->mz700_mode)
	{
		mz->mz700_ram_lock = FALSE;

		/* restore access */
		if (mz->mz700_ram_vram)
			mz_bank_3_w(space, 0, 0);
		else
			mz_bank_1_w(space, 0, 0);
	}
	else
	{
		mz->mz800_ram_lock = FALSE;

		/* restore access from 0xe000 to 0xffff */
		if (mz->mz800_ram_monitor)
			mz_bank_3_w(space, 0, 0);
		else
			mz_bank_1_w(space, 0, 0);
	}
}


/************************ PIT ************************************************/

/* Timer 0 is the clock for the speaker output */
static PIT8253_OUTPUT_CHANGED( pit_out0_changed )
{
	const device_config *speaker = devtag_get_device(device->machine, "speaker");
	speaker_level_w(speaker, state ? 1 : 0);
}


/* Timer 1 is the clock for timer 2 clock input */
static PIT8253_OUTPUT_CHANGED( pit_out1_changed )
{
	pit8253_set_clock_signal(device, 2, state);
}


/* timer 2 is the AM/PM (12 hour) interrupt */
static PIT8253_OUTPUT_CHANGED( pit_irq_2 )
{
	mz_state *mz = device->machine->driver_data;

	if (mz->intmsk)
		cputag_set_input_line(device->machine, "maincpu", 0, ASSERT_LINE);
}


/***************************************************************************
    8255 PPI
***************************************************************************/

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	int key_line = ttl74145_r(device, 0, 0);

	switch (key_line)
	{
	case 1 << 0: return input_port_read(device->machine, "ROW0");
	case 1 << 1: return input_port_read(device->machine, "ROW1");
	case 1 << 2: return input_port_read(device->machine, "ROW2");
	case 1 << 3: return input_port_read(device->machine, "ROW3");
	case 1 << 4: return input_port_read(device->machine, "ROW4");
	case 1 << 5: return input_port_read(device->machine, "ROW5");
	case 1 << 6: return input_port_read(device->machine, "ROW6");
	case 1 << 7: return input_port_read(device->machine, "ROW7");
	case 1 << 8: return input_port_read(device->machine, "ROW8");
	case 1 << 9: return input_port_read(device->machine, "ROW9");
	}

	/* should never reach this */
    return 0xff;
}

/*
 * bit 7 in     vertical blank
 * bit 6 in     NE556 output
 * bit 5 in     tape data (RDATA)
 * bit 4 in     motor (1 = on)
 */
static READ8_DEVICE_HANDLER( pio_port_c_r )
{
	const device_config *cas = devtag_get_device(device->machine, "cassette");
	mz_state *mz = device->machine->driver_data;
	UINT8 data = 0;

	/* note: this is actually connected to Q output of the motor-control flip-flop (see below) */
	if ((cassette_get_state(cas) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data |= 0x10;

	if (cassette_input(cas) > 0.0038)
		data |= 0x20;       /* set the RDATA status */

	data |= mz->cursor_timer << 6;
	data |= video_screen_get_vblank(device->machine->primary_screen) << 7;

	LOG(2,"mz700_pio_port_c_r",("%02X\n", data),device->machine);

	return data;
}


static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	const device_config *timer = devtag_get_device(device->machine, "cursor");

	LOG(2,"mz700_pio_port_a_w",("%02X\n", data),device->machine);

	/* the ls145 is connected to PA0-PA3 */
	ttl74145_w(device, 0, data & 0x07);

	/* ne556 reset is connected to PA7 */
	timer_device_enable(timer, BIT(data, 7));
}


static WRITE8_DEVICE_HANDLER( pio_port_c_w )
{
    /*
     * bit 3 out    motor control (0 = on)
     * bit 2 out    INTMSK
     * bit 1 out    tape data (WDATA)
     * bit 0 out    unused
     */

//	UINT8 state = cassette_get_state(devtag_get_device(device->machine, "cassette"));
//	UINT8 action = ((~pio_port_c_output & 8) & (data & 8));		/* detect low-to-high transition */

	/* The motor control circuit consists of a resistor, capacitor, invertor, nand-gate, and D flip-flop.
		The sense input from the cassette player goes low whenever play, rewind or fast-forward is pressed.
		This connects to most of the above components.
		The Q output enables the motor, and also connects to Bit 4 (input).
		Bit 3 outputs a string of pulses to the Clock pin, and therefore cannot be used to control
		the motor directly.
		For the moment, the user can use the UI to select play, stop, etc.
		If you load from the command-line or the software-picker, type in L <enter> immediately.

	cassette_change_state(
		devtag_get_device(device->machine, "cassette"),
		((data & 0x08) && mz700_motor_on) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MOTOR_DISABLED);

	*/

	LOG(2,"mz700_pio_port_c_w",("%02X\n", data),device->machine);

	cassette_output(devtag_get_device(device->machine, "cassette"), (data & 0x02) ? +1.0 : -1.0);
}


/******************************************************************************
 *	Sharp MZ800
 *
 *
 ******************************************************************************/

static UINT16 mz800_ramaddr = 0;
//static UINT8 mz800_display_mode = 0;
//static UINT8 mz800_port_e8 = 0;
static UINT8 mz800_palette[4];
static UINT8 mz800_palette_bank;


/***************************************************************************
    Z80 PIO
***************************************************************************/

static void mz800_z80pio_irq(const device_config *device, int which)
{
	cputag_set_input_line(device->machine, "maincpu", 0, which);
}

static READ8_DEVICE_HANDLER( mz800_z80pio_port_a_r )
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	UINT8 result = 0;

	result |= centronics_busy_r(printer);
	result |= centronics_pe_r(printer) << 1;
	result |= video_screen_get_hblank(device->machine->primary_screen) << 5;

	return result;
}

static WRITE8_DEVICE_HANDLER( mz800_z80pio_port_a_w )
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");

	centronics_prime_w(printer, BIT(data, 6));
	centronics_strobe_w(printer, BIT(data, 7));
}

static WRITE8_DEVICE_HANDLER( mz800_printer_data_w )
{
	const device_config *printer = devtag_get_device(device->machine, "centronics");
	centronics_data_w(printer, 0, data);
}

const z80pio_interface mz800_z80pio_config =
{
	DEVCB_LINE(mz800_z80pio_irq),
	DEVCB_HANDLER(mz800_z80pio_port_a_r),
	DEVCB_NULL,
	DEVCB_HANDLER(mz800_z80pio_port_a_w),
	DEVCB_HANDLER(mz800_printer_data_w),
	DEVCB_NULL,
	DEVCB_NULL,
};




/* port CE */
READ8_HANDLER( mz800_crtc_r )
{
	UINT8 data = 0x00;
	LOG(1,"mz800_crtc_r",("%02X\n",data),space->machine);
    return data;
}


/* port EA */
 READ8_HANDLER( mz800_ramdisk_r )
{
	UINT8 *mem = memory_region(space->machine, "user1");
	UINT8 data = mem[mz800_ramaddr];
	LOG(2,"mz800_ramdisk_r",("[%04X] -> %02X\n", mz800_ramaddr, data),space->machine);
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_r",("address wrap 0000\n"),space->machine);
    return data;
}

/* port CC */
WRITE8_HANDLER( mz800_write_format_w )
{
	LOG(1,"mz800_write_format_w",("%02X\n", data),space->machine);
}

/* port CD */
WRITE8_HANDLER( mz800_read_format_w )
{
	LOG(1,"mz800_read_format_w",("%02X\n", data),space->machine);
}

/* port CE
 * bit 3	1: MZ700 mode		0: MZ800 mode
 * bit 2	1: 640 horizontal	0: 320 horizontal
 * bit 1	1: 4bpp/2bpp		0: 2bpp/1bpp
 * bit 0	???
 */
WRITE8_HANDLER( mz800_display_mode_w )
{
	mz_state *mz = space->machine->driver_data;

	mz->mz700_mode = BIT(data, 3);
	mz->hires_mode = BIT(data, 2);
	mz->screen = data & 0x03;

	/* change memory maps if we switched mode */
//	if (BIT(data, 3) != mz->mz700_mode)
//	{
//		logerror("mz800_display_mode_w: switching mode to %s\n", (BIT(data, 3) ? "mz700" : "mz800"));
//		mz->mz700_mode = BIT(data, 3);
//		mz_bank_4_w(cputag_get_address_space(space->machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0, 0);
//	}
}

/* port CF */
WRITE8_HANDLER( mz800_scroll_border_w )
{
	LOG(1,"mz800_scroll_border_w",("%02X\n", data),space->machine);
}

/* port EA */
WRITE8_HANDLER( mz800_ramdisk_w )
{
	UINT8 *mem = memory_region(space->machine, "user1");
	LOG(2,"mz800_ramdisk_w",("[%04X] <- %02X\n", mz800_ramaddr, data),space->machine);
	mem[mz800_ramaddr] = data;
	if (mz800_ramaddr++ == 0)
		LOG(1,"mz800_ramdisk_w",("address wrap 0000\n"),space->machine);
}

/* port EB */
WRITE8_HANDLER( mz800_ramaddr_w )
{
	mz800_ramaddr = (cpu_get_reg(cputag_get_cpu(space->machine, "maincpu"), Z80_BC) & 0xff00) | (data & 0xff);
	LOG(1,"mz800_ramaddr_w",("%04X\n", mz800_ramaddr),space->machine);
}

/* port F0 */
WRITE8_HANDLER( mz800_palette_w )
{
	if (data & 0x40)
	{
        mz800_palette_bank = data & 3;
		LOG(1,"mz800_palette_w",("bank: %d\n", mz800_palette_bank),space->machine);
    }
	else
	{
		int idx = (data >> 4) & 3;
		int val = data & 15;
		LOG(1,"mz800_palette_w",("palette[%d] <- %d\n", idx, val),space->machine);
		mz800_palette[idx] = val;
	}
}
