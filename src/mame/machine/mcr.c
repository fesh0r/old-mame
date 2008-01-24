/***************************************************************************

    Midway MCR system

***************************************************************************/

#include "driver.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "audio/mcr.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "cpu/z80/z80daisy.h"
#include "mcr.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)


/*************************************
 *
 *  Global variables
 *
 *************************************/

attotime mcr68_timing_factor;

UINT8 mcr_cocktail_flip;

UINT32 mcr_cpu_board;
UINT32 mcr_sprite_board;
UINT32 mcr_ssio_board;



/*************************************
 *
 *  Statics
 *
 *************************************/

static UINT8 m6840_status;
static UINT8 m6840_status_read_since_int;
static UINT8 m6840_msb_buffer;
static UINT8 m6840_lsb_buffer;
static struct counter_state
{
	UINT8			control;
	UINT16			latch;
	UINT16			count;
	emu_timer *	timer;
	UINT8			timer_active;
	attotime		period;
} m6840_state[3];

/* MCR/68k interrupt states */
static UINT8 m6840_irq_state;
static UINT8 m6840_irq_vector;
static UINT8 v493_irq_state;
static UINT8 v493_irq_vector;

static timer_callback v493_callback;

static UINT8 zwackery_sound_data;

static attotime m6840_counter_periods[3];
static attotime m6840_internal_counter_period;	/* 68000 CLK / 10 */

static emu_timer *ipu_watchdog_timer;



/*************************************
 *
 *  Function prototypes
 *
 *************************************/

static void subtract_from_counter(int counter, int count);

static TIMER_CALLBACK( mcr68_493_callback );
static TIMER_CALLBACK( zwackery_493_callback );

static WRITE8_HANDLER( zwackery_pia_2_w );
static WRITE8_HANDLER( zwackery_pia_3_w );
static WRITE8_HANDLER( zwackery_ca2_w );
static void zwackery_pia_irq(int state);

static void reload_count(int counter);
static TIMER_CALLBACK( counter_fired_callback );
static TIMER_CALLBACK( ipu_watchdog_reset );
static WRITE8_HANDLER( ipu_break_changed );



/*************************************
 *
 *  Graphics declarations
 *
 *************************************/

const gfx_layout mcr_bg_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ STEP2(RGN_FRAC(1,2),1), STEP2(RGN_FRAC(0,2),1) },
	{ STEP8(0,2) },
	{ STEP8(0,16) },
	16*8
};


const gfx_layout mcr_sprite_layout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{ STEP4(0,1) },
	{ STEP2(RGN_FRAC(0,4)+0,4), STEP2(RGN_FRAC(1,4)+0,4), STEP2(RGN_FRAC(2,4)+0,4), STEP2(RGN_FRAC(3,4)+0,4),
	  STEP2(RGN_FRAC(0,4)+8,4), STEP2(RGN_FRAC(1,4)+8,4), STEP2(RGN_FRAC(2,4)+8,4), STEP2(RGN_FRAC(3,4)+8,4),
	  STEP2(RGN_FRAC(0,4)+16,4), STEP2(RGN_FRAC(1,4)+16,4), STEP2(RGN_FRAC(2,4)+16,4), STEP2(RGN_FRAC(3,4)+16,4),
	  STEP2(RGN_FRAC(0,4)+24,4), STEP2(RGN_FRAC(1,4)+24,4), STEP2(RGN_FRAC(2,4)+24,4), STEP2(RGN_FRAC(3,4)+24,4) },
	{ STEP32(0,32) },
	32*32
};



/*************************************
 *
 *  6821 PIA declarations
 *
 *************************************/

READ8_HANDLER( zwackery_port_2_r );


static READ8_HANDLER( zwackery_port_1_r )
{
	UINT8 ret = readinputport(1);

	pia_set_port_a_z_mask(3, ret);

	return ret;
}


static READ8_HANDLER( zwackery_port_3_r )
{
	UINT8 ret = readinputport(3);

	pia_set_port_a_z_mask(4, ret);

	return ret;
}


static const pia6821_interface zwackery_pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, input_port_0_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zwackery_pia_2_w, 0, 0, 0,
	/*irqs   : A/B             */ zwackery_pia_irq, zwackery_pia_irq
};

static const pia6821_interface zwackery_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zwackery_port_1_r, zwackery_port_2_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zwackery_pia_3_w, 0, zwackery_ca2_w, 0,
	/*irqs   : A/B             */ 0, 0
};

static const pia6821_interface zwackery_pia_4_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ zwackery_port_3_r, input_port_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};



/*************************************
 *
 *  Generic MCR CTC interface
 *
 *************************************/

static void ctc_interrupt(int state)
{
	cpunum_set_input_line(Machine, 0, 0, state);
}


static void ipu_ctc_interrupt(int state)
{
	cpunum_set_input_line(Machine, 3, 0, state);
}


const struct z80_irq_daisy_chain mcr_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 0 }, /* CTC number 0 */
	{ 0, 0, 0, 0, -1 }		/* end mark */
};


const struct z80_irq_daisy_chain mcr_ipu_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_irq_state, z80ctc_irq_ack, z80ctc_irq_reti, 1 }, /* CTC number 1 */
	{ z80pio_reset, z80pio_irq_state, z80pio_irq_ack, z80pio_irq_reti, 1 }, /* PIO number 1 */
	{ z80sio_reset, z80sio_irq_state, z80sio_irq_ack, z80sio_irq_reti, 0 }, /* SIO number 0 */
	{ z80pio_reset, z80pio_irq_state, z80pio_irq_ack, z80pio_irq_reti, 0 }, /* PIO number 0 */
	{ 0, 0, 0, 0, -1 }		/* end mark */
};


static z80ctc_interface ctc_intf =
{
	0,              	/* clock (filled in from the CPU 0 clock) */
	0,              	/* timer disables */
	ctc_interrupt,  	/* interrupt handler */
	z80ctc_0_trg1_w,	/* ZC/TO0 callback */
	0,              	/* ZC/TO1 callback */
	0               	/* ZC/TO2 callback */
};


static z80ctc_interface nflfoot_ctc_intf =
{
	0,                  /* clock (filled in from the CPU 3 clock) */
	0,                  /* timer disables */
	ipu_ctc_interrupt,  /* interrupt handler */
	0,					/* ZC/TO0 callback */
	0,              	/* ZC/TO1 callback */
	0               	/* ZC/TO2 callback */
};


static const z80pio_interface nflfoot_pio_intf =
{
	ipu_ctc_interrupt,
	0,
	0
};


static z80sio_interface nflfoot_sio_intf =
{
	0,                  /* clock (filled in from the CPU 3 clock) */
	ipu_ctc_interrupt,	/* interrupt handler */
	0,					/* DTR changed handler */
	0,					/* RTS changed handler */
	ipu_break_changed,	/* BREAK changed handler */
	mcr_ipu_sio_transmit/* transmit handler */
};



/*************************************
 *
 *  Generic MCR machine initialization
 *
 *************************************/

MACHINE_START( mcr )
{
	/* initialize the CTC */
	ctc_intf.baseclock = cpunum_get_clock(0);
	z80ctc_init(0, &ctc_intf);

	state_save_register_global(mcr_cocktail_flip);
}


MACHINE_START( nflfoot )
{
	/* initialize the CTC */
	ctc_intf.baseclock = cpunum_get_clock(0);
	z80ctc_init(0, &ctc_intf);

	nflfoot_ctc_intf.baseclock = cpunum_get_clock(3);
	z80ctc_init(1, &nflfoot_ctc_intf);

	z80pio_init(0, &nflfoot_pio_intf);
	z80pio_init(1, &nflfoot_pio_intf);

	nflfoot_sio_intf.baseclock = cpunum_get_clock(3);
	z80sio_init(0, &nflfoot_sio_intf);

	/* allocate a timer for the IPU watchdog */
	ipu_watchdog_timer = timer_alloc(ipu_watchdog_reset, NULL);
}


MACHINE_RESET( mcr )
{
	/* reset cocktail flip */
	mcr_cocktail_flip = 0;

	/* initialize the sound */
	mcr_sound_reset();
}



/*************************************
 *
 *  Generic MCR/68k machine initialization
 *
 *************************************/

MACHINE_START( mcr68 )
{
	int i;

	for (i = 0; i < 3; i++)
	{
		struct counter_state *m6840 = &m6840_state[i];

		m6840->timer = timer_alloc(counter_fired_callback, NULL);

		state_save_register_item("m6840", i, m6840->control);
		state_save_register_item("m6840", i, m6840->latch);
		state_save_register_item("m6840", i, m6840->count);
		state_save_register_item("m6840", i, m6840->timer_active);
	}

	state_save_register_global(m6840_status);
	state_save_register_global(m6840_status_read_since_int);
	state_save_register_global(m6840_msb_buffer);
	state_save_register_global(m6840_lsb_buffer);
	state_save_register_global(m6840_irq_state);
	state_save_register_global(v493_irq_state);
	state_save_register_global(zwackery_sound_data);

	state_save_register_global(mcr_cocktail_flip);
}


static void mcr68_common_init(void)
{
	int i;

	/* reset the 6840's */
	m6840_counter_periods[0] = ATTOTIME_IN_HZ(30);			/* clocked by /VBLANK */
	m6840_counter_periods[1] = attotime_never;					/* grounded */
	m6840_counter_periods[2] = ATTOTIME_IN_HZ(512 * 30);	/* clocked by /HSYNC */

	m6840_status = 0x00;
	m6840_status_read_since_int = 0x00;
	m6840_msb_buffer = m6840_lsb_buffer = 0;
	for (i = 0; i < 3; i++)
	{
		struct counter_state *m6840 = &m6840_state[i];

		m6840->control = 0x00;
		m6840->latch = 0xffff;
		m6840->count = 0xffff;
		timer_enable(m6840->timer, FALSE);
		m6840->timer_active = 0;
		m6840->period = m6840_counter_periods[i];
	}

	/* initialize the clock */
	m6840_internal_counter_period = ATTOTIME_IN_HZ(cpunum_get_clock(0) / 10);

	/* reset cocktail flip */
	mcr_cocktail_flip = 0;

	/* initialize the sound */
	mcr_sound_reset();
}


MACHINE_RESET( mcr68 )
{
	/* for the most part all MCR/68k games are the same */
	mcr68_common_init();
	v493_callback = mcr68_493_callback;

	/* vectors are 1 and 2 */
	v493_irq_vector = 1;
	m6840_irq_vector = 2;
}


MACHINE_START( zwackery )
{
	/* append our PIA state onto the existing one and reinit */
	pia_config(2, &zwackery_pia_2_intf);
	pia_config(3, &zwackery_pia_3_intf);
	pia_config(4, &zwackery_pia_4_intf);

	MACHINE_START_CALL(mcr68);
}


MACHINE_RESET( zwackery )
{
	/* for the most part all MCR/68k games are the same */
	mcr68_common_init();
	v493_callback = zwackery_493_callback;

	/* append our PIA state onto the existing one and reinit */
	pia_reset();

	/* vectors are 5 and 6 */
	v493_irq_vector = 5;
	m6840_irq_vector = 6;
}



/*************************************
 *
 *  Generic MCR interrupt handler
 *
 *************************************/

INTERRUPT_GEN( mcr_interrupt )
{
	/* CTC line 2 is connected to VBLANK, which is once every 1/2 frame */
	/* for the 30Hz interlaced display */
	z80ctc_0_trg2_w(0, 1);
	z80ctc_0_trg2_w(0, 0);

	/* CTC line 3 is connected to 493, which is signalled once every */
	/* frame at 30Hz */
	if (cpu_getiloops() == 0)
	{
		z80ctc_0_trg3_w(0, 1);
		z80ctc_0_trg3_w(0, 0);
	}
}


INTERRUPT_GEN( mcr_ipu_interrupt )
{
	/* CTC line 3 is connected to 493, which is signalled once every */
	/* frame at 30Hz */
	if (cpu_getiloops() == 0)
	{
		z80ctc_1_trg3_w(0, 1);
		z80ctc_1_trg3_w(0, 0);
	}
}


INTERRUPT_GEN( mcr68_interrupt )
{
	/* update the 6840 VBLANK clock */
	if (!m6840_state[0].timer_active)
		subtract_from_counter(0, 1);

	logerror("--- VBLANK ---\n");

	/* also set a timer to generate the 493 signal at a specific time before the next VBLANK */
	/* the timing of this is crucial for Blasted and Tri-Sports, which check the timing of */
	/* VBLANK and 493 using counter 2 */
	timer_set(attotime_sub(ATTOTIME_IN_HZ(30), mcr68_timing_factor), NULL, 0, v493_callback);
}



/*************************************
 *
 *  MCR/68k interrupt central
 *
 *************************************/

static void update_mcr68_interrupts(void)
{
	int newstate = 0;

	/* all interrupts go through an LS148, which gives priority to the highest */
	if (v493_irq_state)
		newstate = v493_irq_vector;
	if (m6840_irq_state)
		newstate = m6840_irq_vector;

	/* set the new state of the IRQ lines */
	if (newstate)
		cpunum_set_input_line(Machine, 0, newstate, ASSERT_LINE);
	else
		cpunum_set_input_line(Machine, 0, 7, CLEAR_LINE);
}


static TIMER_CALLBACK( mcr68_493_off_callback )
{
	v493_irq_state = 0;
	update_mcr68_interrupts();
}


static TIMER_CALLBACK( mcr68_493_callback )
{
	v493_irq_state = 1;
	update_mcr68_interrupts();
	timer_set(video_screen_get_scan_period(0), NULL, 0, mcr68_493_off_callback);
	logerror("--- (INT1) ---\n");
}



/*************************************
 *
 *  Generic MCR port write handlers
 *
 *************************************/

WRITE8_HANDLER( mcr_control_port_w )
{
	/*
        Bit layout is as follows:
            D7 = n/c
            D6 = cocktail flip
            D5 = red LED
            D4 = green LED
            D3 = n/c
            D2 = coin meter 3
            D1 = coin meter 2
            D0 = coin meter 1
    */

	coin_counter_w(0, (data >> 0) & 1);
	coin_counter_w(1, (data >> 1) & 1);
	coin_counter_w(2, (data >> 2) & 1);
	mcr_cocktail_flip = (data >> 6) & 1;
}


WRITE8_HANDLER( mcrmono_control_port_w )
{
	/*
        Bit layout is as follows:
            D7 = n/c
            D6 = cocktail flip
            D5 = n/c
            D4 = n/c
            D3 = n/c
            D2 = n/c
            D1 = n/c
            D0 = coin meter 1
    */

	coin_counter_w(0, (data >> 0) & 1);
	mcr_cocktail_flip = (data >> 6) & 1;
}


WRITE8_HANDLER( mcr_scroll_value_w )
{
	switch (offset)
	{
		case 0:
			/* low 8 bits of horizontal scroll */
			spyhunt_scrollx = (spyhunt_scrollx & ~0xff) | data;
			break;

		case 1:
			/* upper 3 bits of horizontal scroll and upper 1 bit of vertical scroll */
			spyhunt_scrollx = (spyhunt_scrollx & 0xff) | ((data & 0x07) << 8);
			spyhunt_scrolly = (spyhunt_scrolly & 0xff) | ((data & 0x80) << 1);
			break;

		case 2:
			/* low 8 bits of vertical scroll */
			spyhunt_scrolly = (spyhunt_scrolly & ~0xff) | data;
			break;
	}
}



/*************************************
 *
 *  Zwackery-specific interfaces
 *
 *************************************/

WRITE8_HANDLER( zwackery_pia_2_w )
{
	/* bit 7 is the watchdog */
	if (!(data & 0x80)) watchdog_reset_w(offset, data);

	/* bits 5 and 6 control hflip/vflip */
	/* bits 3 and 4 control coin counters? */
	/* bits 0, 1 and 2 control meters? */
}


WRITE8_HANDLER( zwackery_pia_3_w )
{
	zwackery_sound_data = (data >> 4) & 0x0f;
}


WRITE8_HANDLER( zwackery_ca2_w )
{
	csdeluxe_data_w(offset, (data << 4) | zwackery_sound_data);
}


static void zwackery_pia_irq(int state)
{
	v493_irq_state = pia_get_irq_a(2) | pia_get_irq_b(2);
	update_mcr68_interrupts();
}


static TIMER_CALLBACK( zwackery_493_off_callback )
{
	pia_2_ca1_w(0, 0);
}


static TIMER_CALLBACK( zwackery_493_callback )
{
	pia_2_ca1_w(0, 1);
	timer_set(video_screen_get_scan_period(0), NULL, 0, zwackery_493_off_callback);
}



/*************************************
 *
 *  M6840 timer utilities
 *
 *************************************/

INLINE void update_interrupts(void)
{
	m6840_status &= ~0x80;

	if ((m6840_status & 0x01) && (m6840_state[0].control & 0x40)) m6840_status |= 0x80;
	if ((m6840_status & 0x02) && (m6840_state[1].control & 0x40)) m6840_status |= 0x80;
	if ((m6840_status & 0x04) && (m6840_state[2].control & 0x40)) m6840_status |= 0x80;

	m6840_irq_state = m6840_status >> 7;
	update_mcr68_interrupts();
}


static void subtract_from_counter(int counter, int count)
{
	/* dual-byte mode */
	if (m6840_state[counter].control & 0x04)
	{
		int lsb = m6840_state[counter].count & 0xff;
		int msb = m6840_state[counter].count >> 8;

		/* count the clocks */
		lsb -= count;

		/* loop while we're less than zero */
		while (lsb < 0)
		{
			/* borrow from the MSB */
			lsb += (m6840_state[counter].latch & 0xff) + 1;
			msb--;

			/* if MSB goes less than zero, we've expired */
			if (msb < 0)
			{
				m6840_status |= 1 << counter;
				m6840_status_read_since_int &= ~(1 << counter);
				update_interrupts();
				msb = (m6840_state[counter].latch >> 8) + 1;
				LOG(("** Counter %d fired\n", counter));
			}
		}

		/* store the result */
		m6840_state[counter].count = (msb << 8) | lsb;
	}

	/* word mode */
	else
	{
		int word = m6840_state[counter].count;

		/* count the clocks */
		word -= count;

		/* loop while we're less than zero */
		while (word < 0)
		{
			/* borrow from the MSB */
			word += m6840_state[counter].latch + 1;

			/* we've expired */
			m6840_status |= 1 << counter;
			m6840_status_read_since_int &= ~(1 << counter);
			update_interrupts();
			LOG(("** Counter %d fired\n", counter));
		}

		/* store the result */
		m6840_state[counter].count = word;
	}
}


static TIMER_CALLBACK( counter_fired_callback )
{
	int count = param >> 2;
	int counter = param & 3;

	/* reset the timer */
	m6840_state[counter].timer_active = 0;

	/* subtract it all from the counter; this will generate an interrupt */
	subtract_from_counter(counter, count);
}


static void reload_count(int counter)
{
	attotime period;
	attotime total_period;
	int count;

	/* copy the latched value in */
	m6840_state[counter].count = m6840_state[counter].latch;

	/* counter 0 is self-updating if clocked externally */
	if (counter == 0 && !(m6840_state[counter].control & 0x02))
	{
		timer_adjust(m6840_state[counter].timer, attotime_never, 0, attotime_zero);
		m6840_state[counter].timer_active = 0;
		return;
	}

	/* determine the clock period for this timer */
	if (m6840_state[counter].control & 0x02)
		period = m6840_internal_counter_period;
	else
		period = m6840_counter_periods[counter];

	/* determine the number of clock periods before we expire */
	count = m6840_state[counter].count;
	if (m6840_state[counter].control & 0x04)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		count = count + 1;

	/* set the timer */
	total_period = attotime_make(0, attotime_to_attoseconds(period) * count);
LOG(("reload_count(%d): period = %f  count = %d\n", counter, attotime_to_double(period), count));
	timer_adjust(m6840_state[counter].timer, total_period, (count << 2) + counter, attotime_zero);
	m6840_state[counter].timer_active = 1;
}


static UINT16 compute_counter(int counter)
{
	attotime period;
	int remaining;

	/* if there's no timer, return the count */
	if (!m6840_state[counter].timer_active)
		return m6840_state[counter].count;

	/* determine the clock period for this timer */
	if (m6840_state[counter].control & 0x02)
		period = m6840_internal_counter_period;
	else
		period = m6840_counter_periods[counter];

	/* see how many are left */
	remaining = attotime_to_attoseconds(timer_timeleft(m6840_state[counter].timer)) / attotime_to_attoseconds(period);

	/* adjust the count for dual byte mode */
	if (m6840_state[counter].control & 0x04)
	{
		int divisor = (m6840_state[counter].count & 0xff) + 1;
		int msb = remaining / divisor;
		int lsb = remaining % divisor;
		remaining = (msb << 8) | lsb;
	}

	return remaining;
}



/*************************************
 *
 *  M6840 timer I/O
 *
 *************************************/

static WRITE8_HANDLER( mcr68_6840_w_common )
{
	int i;

	/* offsets 0 and 1 are control registers */
	if (offset < 2)
	{
		int counter = (offset == 1) ? 1 : (m6840_state[1].control & 0x01) ? 0 : 2;
		UINT8 diffs = data ^ m6840_state[counter].control;

		m6840_state[counter].control = data;

		/* reset? */
		if (counter == 0 && (diffs & 0x01))
		{
			/* holding reset down */
			if (data & 0x01)
			{
				for (i = 0; i < 3; i++)
				{
					timer_adjust(m6840_state[i].timer, attotime_never, 0, attotime_zero);
					m6840_state[i].timer_active = 0;
				}
			}

			/* releasing reset */
			else
			{
				for (i = 0; i < 3; i++)
					reload_count(i);
			}

			m6840_status = 0;
			update_interrupts();
		}

		/* changing the clock source? (needed for Zwackery) */
		if (diffs & 0x02)
			reload_count(counter);

		LOG(("%06X:Counter %d control = %02X\n", activecpu_get_previouspc(), counter, data));
	}

	/* offsets 2, 4, and 6 are MSB buffer registers */
	else if ((offset & 1) == 0)
	{
		LOG(("%06X:MSB = %02X\n", activecpu_get_previouspc(), data));
		m6840_msb_buffer = data;
	}

	/* offsets 3, 5, and 7 are Write Timer Latch commands */
	else
	{
		int counter = (offset - 2) / 2;
		m6840_state[counter].latch = (m6840_msb_buffer << 8) | (data & 0xff);

		/* clear the interrupt */
		m6840_status &= ~(1 << counter);
		update_interrupts();

		/* reload the count if in an appropriate mode */
		if (!(m6840_state[counter].control & 0x10))
			reload_count(counter);

		LOG(("%06X:Counter %d latch = %04X\n", activecpu_get_previouspc(), counter, m6840_state[counter].latch));
	}
}


static READ16_HANDLER( mcr68_6840_r_common )
{
	/* offset 0 is a no-op */
	if (offset == 0)
		return 0;

	/* offset 1 is the status register */
	else if (offset == 1)
	{
		LOG(("%06X:Status read = %04X\n", activecpu_get_previouspc(), m6840_status));
		m6840_status_read_since_int |= m6840_status & 0x07;
		return m6840_status;
	}

	/* offsets 2, 4, and 6 are Read Timer Counter commands */
	else if ((offset & 1) == 0)
	{
		int counter = (offset - 2) / 2;
		int result = compute_counter(counter);

		/* clear the interrupt if the status has been read */
		if (m6840_status_read_since_int & (1 << counter))
			m6840_status &= ~(1 << counter);
		update_interrupts();

		m6840_lsb_buffer = result & 0xff;

		LOG(("%06X:Counter %d read = %04X\n", activecpu_get_previouspc(), counter, result));
		return result >> 8;
	}

	/* offsets 3, 5, and 7 are LSB buffer registers */
	else
		return m6840_lsb_buffer;
}


WRITE16_HANDLER( mcr68_6840_upper_w )
{
	if (ACCESSING_MSB)
		mcr68_6840_w_common(offset, (data >> 8) & 0xff);
}


WRITE16_HANDLER( mcr68_6840_lower_w )
{
	if (ACCESSING_LSB)
		mcr68_6840_w_common(offset, data & 0xff);
}


READ16_HANDLER( mcr68_6840_upper_r )
{
	return (mcr68_6840_r_common(offset,0) << 8) | 0x00ff;
}


READ16_HANDLER( mcr68_6840_lower_r )
{
	return mcr68_6840_r_common(offset,0) | 0xff00;
}



/*************************************
 *
 *  NFL Football IPU board
 *
 *************************************/

static WRITE8_HANDLER( ipu_break_changed )
{
	/* channel B is connected to the CED player */
	if (offset == 1)
	{
		logerror("DTR changed -> %d\n", data);
		if (data == 1)
			z80sio_receive_data(0, 1, 0);
	}
}


READ8_HANDLER( mcr_ipu_pio_0_r )
{
	return (offset & 2) ? z80pio_c_r(0, offset & 1) : z80pio_d_r(0, offset & 1);
}


READ8_HANDLER( mcr_ipu_pio_1_r )
{
	return (offset & 2) ? z80pio_c_r(1, offset & 1) : z80pio_d_r(1, offset & 1);
}


READ8_HANDLER( mcr_ipu_sio_r )
{
	return (offset & 2) ? z80sio_c_r(0, offset & 1) : z80sio_d_r(0, offset & 1);
}


WRITE8_HANDLER( mcr_ipu_pio_0_w )
{
	if (offset & 2)
		z80pio_c_w(0, offset & 1, data);
	else
		z80pio_d_w(0, offset & 1, data);
}


WRITE8_HANDLER( mcr_ipu_pio_1_w )
{
	if (offset & 2)
		z80pio_c_w(1, offset & 1, data);
	else
		z80pio_d_w(1, offset & 1, data);
}


WRITE8_HANDLER( mcr_ipu_sio_w )
{
	if (offset & 2)
		z80sio_c_w(0, offset & 1, data);
	else
		z80sio_d_w(0, offset & 1, data);
}


WRITE8_HANDLER( mcr_ipu_laserdisk_w )
{
	/* bit 3 enables (1) LD video regardless of PIX SW */
	/* bit 2 enables (1) LD right channel audio */
	/* bit 1 enables (1) LD left channel audio */
	/* bit 0 enables (1) LD video if PIX SW == 1 */
	if (data != 0)
		logerror("%04X:mcr_ipu_laserdisk_w(%d) = %02X\n", activecpu_get_pc(), offset, data);
}


static TIMER_CALLBACK( ipu_watchdog_reset )
{
	logerror("ipu_watchdog_reset\n");
	cpunum_set_input_line(machine, 3, INPUT_LINE_RESET, PULSE_LINE);
	z80ctc_reset(1);
	z80pio_reset(0);
	z80pio_reset(1);
	z80sio_reset(0);
}


READ8_HANDLER( mcr_ipu_watchdog_r )
{
	/* watchdog counter is clocked by 7.3728MHz crystal / 16 */
	/* watchdog is tripped when 14-bit counter overflows => / 32768 = 14.0625Hz*/
	timer_adjust(ipu_watchdog_timer, ATTOTIME_IN_HZ(7372800 / 16 / 32768), 0, attotime_zero);
	return 0xff;
}


WRITE8_HANDLER( mcr_ipu_watchdog_w )
{
	mcr_ipu_watchdog_r(0);
}
