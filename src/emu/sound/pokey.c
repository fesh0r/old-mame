/*****************************************************************************
 *
 *  POKEY chip emulator 4.51
 *  Copyright Nicola Salmoria and the MAME Team
 *
 *  Based on original info found in Ron Fries' Pokey emulator,
 *  with additions by Brad Oliver, Eric Smith and Juergen Buchmueller,
 *  paddle (a/d conversion) details from the Atari 400/800 Hardware Manual.
 *  Polynome algorithms according to info supplied by Perry McFarlane.
 *
 *  This code is subject to the MAME license, which besides other
 *  things means it is distributed as is, no warranties whatsoever.
 *  For more details read mame.txt that comes with MAME.
 *
 *  4.51:
 *  - changed to use the attotime datatype
 *  4.5:
 *  - changed the 9/17 bit polynomial formulas such that the values
 *    required for the Tempest Pokey protection will be found.
 *    Tempest expects the upper 4 bits of the RNG to appear in the
 *    lower 4 bits after four cycles, so there has to be a shift
 *    of 1 per cycle (which was not the case before). Bits #6-#13 of the
 *    new RNG give this expected result now, bits #0-7 of the 9 bit poly.
 *  - reading the RNG returns the shift register contents ^ 0xff.
 *    That way resetting the Pokey with SKCTL (which resets the
 *    polynome shifters to 0) returns the expected 0xff value.
 *  4.4:
 *  - reversed sample values to make OFF channels produce a zero signal.
 *    actually de-reversed them; don't remember that I reversed them ;-/
 *  4.3:
 *  - for POT inputs returning zero, immediately assert the ALLPOT
 *    bit after POTGO is written, otherwise start trigger timer
 *    depending on SK_PADDLE mode, either 1-228 scanlines or 1-2
 *    scanlines, depending on the SK_PADDLE bit of SKCTL.
 *  4.2:
 *  - half volume for channels which are inaudible (this should be
 *    close to the real thing).
 *  4.1:
 *  - default gain increased to closely match the old code.
 *  - random numbers repeat rate depends on POLY9 flag too!
 *  - verified sound output with many, many Atari 800 games,
 *    including the SUPPRESS_INAUDIBLE optimizations.
 *  4.0:
 *  - rewritten from scratch.
 *  - 16bit stream interface.
 *  - serout ready/complete delayed interrupts.
 *  - reworked pot analog/digital conversion timing.
 *  - optional non-indexing pokey update functions.
 *
 *****************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "cpuintrf.h"
#include "cpuexec.h"
#include "pokey.h"

/*
 * Defining this produces much more (about twice as much)
 * but also more efficient code. Ideally this should be set
 * for processors with big code cache and for healthy compilers :)
 */
#ifndef BIG_SWITCH
#ifndef HEAVY_MACRO_USAGE
#define HEAVY_MACRO_USAGE   1
#endif
#else
#define HEAVY_MACRO_USAGE	BIG_SWITCH
#endif

#define SUPPRESS_INAUDIBLE	1

/* Four channels with a range of 0..32767 and volume 0..15 */
//#define POKEY_DEFAULT_GAIN (32767/15/4)

/*
 * But we raise the gain and risk clipping, the old Pokey did
 * this too. It defined POKEY_DEFAULT_GAIN 6 and this was
 * 6 * 15 * 4 = 360, 360/256 = 1.40625
 * I use 15/11 = 1.3636, so this is a little lower.
 */
#define POKEY_DEFAULT_GAIN (32767/11/4)

#define VERBOSE 		0
#define VERBOSE_SOUND	0
#define VERBOSE_TIMER	0
#define VERBOSE_POLY	0
#define VERBOSE_RAND	0

#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define LOG_SOUND(x) do { if (VERBOSE_SOUND) logerror x; } while (0)

#define LOG_TIMER(x) do { if (VERBOSE_TIMER) logerror x; } while (0)

#define LOG_POLY(x) do { if (VERBOSE_POLY) logerror x; } while (0)

#define LOG_RAND(x) do { if (VERBOSE_RAND) logerror x; } while (0)

#define CHAN1	0
#define CHAN2	1
#define CHAN3	2
#define CHAN4	3

#define TIMER1	0
#define TIMER2	1
#define TIMER4	2

/* values to add to the divisors for the different modes */
#define DIVADD_LOCLK		1
#define DIVADD_HICLK		4
#define DIVADD_HICLK_JOINED 7

/* AUDCx */
#define NOTPOLY5	0x80	/* selects POLY5 or direct CLOCK */
#define POLY4		0x40	/* selects POLY4 or POLY17 */
#define PURE		0x20	/* selects POLY4/17 or PURE tone */
#define VOLUME_ONLY 0x10	/* selects VOLUME OUTPUT ONLY */
#define VOLUME_MASK 0x0f	/* volume mask */

/* AUDCTL */
#define POLY9		0x80	/* selects POLY9 or POLY17 */
#define CH1_HICLK	0x40	/* selects 1.78979 MHz for Ch 1 */
#define CH3_HICLK	0x20	/* selects 1.78979 MHz for Ch 3 */
#define CH12_JOINED 0x10	/* clocks channel 1 w/channel 2 */
#define CH34_JOINED 0x08	/* clocks channel 3 w/channel 4 */
#define CH1_FILTER	0x04	/* selects channel 1 high pass filter */
#define CH2_FILTER	0x02	/* selects channel 2 high pass filter */
#define CLK_15KHZ	0x01	/* selects 15.6999 kHz or 63.9211 kHz */

/* IRQEN (D20E) */
#define IRQ_BREAK	0x80	/* BREAK key pressed interrupt */
#define IRQ_KEYBD	0x40	/* keyboard data ready interrupt */
#define IRQ_SERIN	0x20	/* serial input data ready interrupt */
#define IRQ_SEROR	0x10	/* serial output register ready interrupt */
#define IRQ_SEROC	0x08	/* serial output complete interrupt */
#define IRQ_TIMR4	0x04	/* timer channel #4 interrupt */
#define IRQ_TIMR2	0x02	/* timer channel #2 interrupt */
#define IRQ_TIMR1	0x01	/* timer channel #1 interrupt */

/* SKSTAT (R/D20F) */
#define SK_FRAME	0x80	/* serial framing error */
#define SK_OVERRUN	0x40	/* serial overrun error */
#define SK_KBERR	0x20	/* keyboard overrun error */
#define SK_SERIN	0x10	/* serial input high */
#define SK_SHIFT	0x08	/* shift key pressed */
#define SK_KEYBD	0x04	/* keyboard key pressed */
#define SK_SEROUT	0x02	/* serial output active */

/* SKCTL (W/D20F) */
#define SK_BREAK	0x80	/* serial out break signal */
#define SK_BPS		0x70	/* bits per second */
#define SK_FM		0x08	/* FM mode */
#define SK_PADDLE	0x04	/* fast paddle a/d conversion */
#define SK_RESET	0x03	/* reset serial/keyboard interface */

#define DIV_64		28		 /* divisor for 1.78979 MHz clock to 63.9211 kHz */
#define DIV_15		114 	 /* divisor for 1.78979 MHz clock to 15.6999 kHz */

struct POKEYregisters
{
	INT32 counter[4];		/* channel counter */
	INT32 divisor[4];		/* channel divisor (modulo value) */
	UINT32 volume[4];		/* channel volume - derived */
	UINT8 output[4];		/* channel output signal (1 active, 0 inactive) */
	UINT8 audible[4];		/* channel plays an audible tone/effect */
	UINT32 samplerate_24_8; /* sample rate in 24.8 format */
	UINT32 samplepos_fract; /* sample position fractional part */
	UINT32 samplepos_whole; /* sample position whole part */
	UINT32 polyadjust;		/* polynome adjustment */
	UINT32 p4;              /* poly4 index */
	UINT32 p5;              /* poly5 index */
	UINT32 p9;              /* poly9 index */
	UINT32 p17;             /* poly17 index */
	UINT32 r9;				/* rand9 index */
	UINT32 r17;             /* rand17 index */
	UINT32 clockmult;		/* clock multiplier */
	const device_config *device;
	sound_stream * channel; /* streams channel */
	emu_timer *timer[3]; 	/* timers for channel 1,2 and 4 events */
	attotime timer_period[3];	/* computed periods for these timers */
	int timer_param[3];		/* computed parameters for these timers */
	emu_timer *rtimer;     /* timer for calculating the random offset */
	emu_timer *ptimer[8];	/* pot timers */
	read8_space_func pot_r[8];
	read8_space_func allpot_r;
	read8_space_func serin_r;
	write8_space_func serout_w;
	void (*interrupt_cb)(running_machine *machine, int mask);
	UINT8 AUDF[4];          /* AUDFx (D200, D202, D204, D206) */
	UINT8 AUDC[4];			/* AUDCx (D201, D203, D205, D207) */
	UINT8 POTx[8];			/* POTx   (R/D200-D207) */
	UINT8 AUDCTL;			/* AUDCTL (W/D208) */
	UINT8 ALLPOT;			/* ALLPOT (R/D208) */
	UINT8 KBCODE;			/* KBCODE (R/D209) */
	UINT8 RANDOM;			/* RANDOM (R/D20A) */
	UINT8 SERIN;			/* SERIN  (R/D20D) */
	UINT8 SEROUT;			/* SEROUT (W/D20D) */
	UINT8 IRQST;			/* IRQST  (R/D20E) */
	UINT8 IRQEN;			/* IRQEN  (W/D20E) */
	UINT8 SKSTAT;			/* SKSTAT (R/D20F) */
	UINT8 SKCTL;			/* SKCTL  (W/D20F) */
	pokey_interface intf;
	attotime clock_period;
	attotime ad_time_fast;
	attotime ad_time_slow;
	int index;

	UINT8 poly4[0x0f];
	UINT8 poly5[0x1f];
	UINT8 poly9[0x1ff];
	UINT8 poly17[0x1ffff];

	UINT8 rand9[0x1ff];
	UINT8 rand17[0x1ffff];
};


#define P4(chip)  chip->poly4[chip->p4]
#define P5(chip)  chip->poly5[chip->p5]
#define P9(chip)  chip->poly9[chip->p9]
#define P17(chip) chip->poly17[chip->p17]

static TIMER_CALLBACK( pokey_timer_expire );
static TIMER_CALLBACK( pokey_pot_trigger );


#define SAMPLE	-1

#define ADJUST_EVENT(chip)												\
	chip->counter[CHAN1] -= event;										\
	chip->counter[CHAN2] -= event;										\
	chip->counter[CHAN3] -= event;										\
	chip->counter[CHAN4] -= event;										\
	chip->samplepos_whole -= event;										\
	chip->polyadjust += event

#if SUPPRESS_INAUDIBLE

#define PROCESS_CHANNEL(chip,ch)                                        \
	int toggle = 0; 													\
	ADJUST_EVENT(chip); 												\
	/* reset the channel counter */ 									\
	if( chip->audible[ch] )												\
		chip->counter[ch] = chip->divisor[ch];							\
	else																\
		chip->counter[ch] = 0x7fffffff;									\
	chip->p4 = (chip->p4+chip->polyadjust)%0x0000f;						\
	chip->p5 = (chip->p5+chip->polyadjust)%0x0001f;						\
	chip->p9 = (chip->p9+chip->polyadjust)%0x001ff;						\
	chip->p17 = (chip->p17+chip->polyadjust)%0x1ffff; 					\
	chip->polyadjust = 0; 												\
	if( (chip->AUDC[ch] & NOTPOLY5) || P5(chip) ) 						\
	{																	\
		if( chip->AUDC[ch] & PURE )										\
			toggle = 1; 												\
		else															\
		if( chip->AUDC[ch] & POLY4 )									\
			toggle = chip->output[ch] == !P4(chip);						\
		else															\
		if( chip->AUDCTL & POLY9 )										\
			toggle = chip->output[ch] == !P9(chip);						\
		else															\
			toggle = chip->output[ch] == !P17(chip);					\
	}																	\
	if( toggle )														\
	{																	\
		if( chip->audible[ch] )											\
		{																\
			if( chip->output[ch] )										\
				sum -= chip->volume[ch];								\
			else														\
				sum += chip->volume[ch];								\
		}																\
		chip->output[ch] ^= 1;											\
	}																	\
	/* is this a filtering channel (3/4) and is the filter active? */	\
	if( chip->AUDCTL & ((CH1_FILTER|CH2_FILTER) & (0x10 >> ch)) ) 		\
    {                                                                   \
		if( chip->output[ch-2] )										\
        {                                                               \
			chip->output[ch-2] = 0;										\
			if( chip->audible[ch] )										\
				sum -= chip->volume[ch-2];								\
        }                                                               \
    }                                                                   \

#else

#define PROCESS_CHANNEL(chip,ch)                                        \
	int toggle = 0; 													\
	ADJUST_EVENT(chip); 												\
	/* reset the channel counter */ 									\
	chip->counter[ch] = p[chip].divisor[ch];							\
	chip->p4 = (chip->p4+chip->polyadjust)%0x0000f;						\
	chip->p5 = (chip->p5+chip->polyadjust)%0x0001f;						\
	chip->p9 = (chip->p9+chip->polyadjust)%0x001ff;						\
	chip->p17 = (chip->p17+chip->polyadjust)%0x1ffff; 					\
	chip->polyadjust = 0; 												\
	if( (chip->AUDC[ch] & NOTPOLY5) || P5(chip) ) 						\
	{																	\
		if( chip->AUDC[ch] & PURE )										\
			toggle = 1; 												\
		else															\
		if( chip->AUDC[ch] & POLY4 )									\
			toggle = chip->output[ch] == !P4(chip);						\
		else															\
		if( chip->AUDCTL & POLY9 )										\
			toggle = chip->output[ch] == !P9(chip);						\
		else															\
			toggle = chip->output[ch] == !P17(chip);					\
	}																	\
	if( toggle )														\
	{																	\
		if( chip->output[ch] )											\
			sum -= chip->volume[ch];									\
		else															\
			sum += chip->volume[ch];									\
		chip->output[ch] ^= 1;											\
	}																	\
	/* is this a filtering channel (3/4) and is the filter active? */	\
	if( chip->AUDCTL & ((CH1_FILTER|CH2_FILTER) & (0x10 >> ch)) ) 		\
    {                                                                   \
		if( chip->output[ch-2] )										\
        {                                                               \
			chip->output[ch-2] = 0;										\
			sum -= chip->volume[ch-2];									\
        }                                                               \
    }                                                                   \

#endif

#define PROCESS_SAMPLE(chip)                                            \
    ADJUST_EVENT(chip);                                                 \
    /* adjust the sample position */                                    \
	chip->samplepos_whole++;											\
	/* store sum of output signals into the buffer */					\
	*buffer++ = (sum > 0x7fff) ? 0x7fff : sum;							\
	samples--

#if HEAVY_MACRO_USAGE

/*
 * This version of PROCESS_POKEY repeats the search for the minimum
 * event value without using an index to the channel. That way the
 * PROCESS_CHANNEL macros can be called with fixed values and expand
 * to much more efficient code
 */

#define PROCESS_POKEY(chip) 											\
	UINT32 sum = 0; 													\
	if( chip->output[CHAN1] ) 											\
		sum += chip->volume[CHAN1];										\
	if( chip->output[CHAN2] ) 											\
		sum += chip->volume[CHAN2];										\
	if( chip->output[CHAN3] ) 											\
		sum += chip->volume[CHAN3];										\
	if( chip->output[CHAN4] ) 											\
		sum += chip->volume[CHAN4];										\
    while( samples > 0 )                                                 \
	{																	\
		if( chip->counter[CHAN1] < chip->samplepos_whole )				\
		{																\
			if( chip->counter[CHAN2] <  chip->counter[CHAN1] ) 			\
			{															\
				if( chip->counter[CHAN3] <  chip->counter[CHAN2] )		\
				{														\
					if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 	\
					{													\
						UINT32 event = chip->counter[CHAN4];			\
                        PROCESS_CHANNEL(chip,CHAN4);                    \
					}													\
					else												\
					{													\
						UINT32 event = chip->counter[CHAN3];			\
                        PROCESS_CHANNEL(chip,CHAN3);                    \
					}													\
				}														\
				else													\
				if( chip->counter[CHAN4] < chip->counter[CHAN2] )  		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
                else                                                    \
				{														\
					UINT32 event = chip->counter[CHAN2];				\
                    PROCESS_CHANNEL(chip,CHAN2);                        \
				}														\
            }                                                           \
			else														\
			if( chip->counter[CHAN3] < chip->counter[CHAN1] ) 			\
			{															\
				if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
                else                                                    \
				{														\
					UINT32 event = chip->counter[CHAN3];				\
                    PROCESS_CHANNEL(chip,CHAN3);                        \
				}														\
            }                                                           \
			else														\
			if( chip->counter[CHAN4] < chip->counter[CHAN1] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
            else                                                        \
			{															\
				UINT32 event = chip->counter[CHAN1];					\
                PROCESS_CHANNEL(chip,CHAN1);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN2] < chip->samplepos_whole )				\
		{																\
			if( chip->counter[CHAN3] < chip->counter[CHAN2] ) 			\
			{															\
				if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 		\
				{														\
					UINT32 event = chip->counter[CHAN4];				\
                    PROCESS_CHANNEL(chip,CHAN4);                        \
				}														\
				else													\
				{														\
					UINT32 event = chip->counter[CHAN3];				\
                    PROCESS_CHANNEL(chip,CHAN3);                        \
				}														\
			}															\
			else														\
			if( chip->counter[CHAN4] < chip->counter[CHAN2] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
			else														\
			{															\
				UINT32 event = chip->counter[CHAN2];					\
                PROCESS_CHANNEL(chip,CHAN2);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN3] < chip->samplepos_whole )				\
        {                                                               \
			if( chip->counter[CHAN4] < chip->counter[CHAN3] ) 			\
			{															\
				UINT32 event = chip->counter[CHAN4];					\
                PROCESS_CHANNEL(chip,CHAN4);                            \
			}															\
			else														\
			{															\
				UINT32 event = chip->counter[CHAN3];					\
                PROCESS_CHANNEL(chip,CHAN3);                            \
			}															\
		}																\
		else															\
		if( chip->counter[CHAN4] < chip->samplepos_whole )				\
		{																\
			UINT32 event = chip->counter[CHAN4];						\
            PROCESS_CHANNEL(chip,CHAN4);                                \
        }                                                               \
		else															\
		{																\
			UINT32 event = chip->samplepos_whole; 						\
			PROCESS_SAMPLE(chip);										\
		}																\
	}																	\
	timer_adjust_oneshot(chip->rtimer, attotime_never, 0)

#else   /* no HEAVY_MACRO_USAGE */
/*
 * And this version of PROCESS_POKEY uses event and channel variables
 * so that the PROCESS_CHANNEL macro needs to index memory at runtime.
 */

#define PROCESS_POKEY(chip)                                             \
	UINT32 sum = 0; 													\
	if( chip->output[CHAN1] ) 											\
		sum += chip->volume[CHAN1];										\
	if( chip->output[CHAN2] ) 											\
		sum += chip->volume[CHAN2];										\
	if( chip->output[CHAN3] ) 											\
		sum += chip->volume[CHAN3];										\
	if( chip->output[CHAN4] ) 											\
        sum += chip->volume[CHAN4];                                     \
	while( samples > 0 )                                                 \
	{																	\
		UINT32 event = chip->samplepos_whole; 							\
		UINT32 channel = SAMPLE;										\
		if( chip->counter[CHAN1] < event )								\
        {                                                               \
			event = chip->counter[CHAN1]; 								\
			channel = CHAN1;											\
		}																\
		if( chip->counter[CHAN2] < event )								\
        {                                                               \
			event = chip->counter[CHAN2]; 								\
			channel = CHAN2;											\
        }                                                               \
		if( chip->counter[CHAN3] < event )								\
        {                                                               \
			event = chip->counter[CHAN3]; 								\
			channel = CHAN3;											\
        }                                                               \
		if( chip->counter[CHAN4] < event )								\
        {                                                               \
			event = chip->counter[CHAN4]; 								\
			channel = CHAN4;											\
        }                                                               \
        if( channel == SAMPLE )                                         \
		{																\
            PROCESS_SAMPLE(chip);                                       \
        }                                                               \
		else															\
		{																\
			PROCESS_CHANNEL(chip,channel);								\
		}																\
	}																	\
	timer_adjust_oneshot(chip->rtimer, attotime_never, 0)

#endif


static STREAM_UPDATE( pokey_update )
{
	struct POKEYregisters *chip = param;
	stream_sample_t *buffer = outputs[0];
	PROCESS_POKEY(chip);
}


static void poly_init(UINT8 *poly, int size, int left, int right, int add)
{
	int mask = (1 << size) - 1;
    int i, x = 0;

	LOG_POLY(("poly %d\n", size));
	for( i = 0; i < mask; i++ )
	{
		*poly++ = x & 1;
		LOG_POLY(("%05x: %d\n", x, x&1));
        /* calculate next bit */
		x = ((x << left) + (x >> right) + add) & mask;
	}
}

static void rand_init(UINT8 *rng, int size, int left, int right, int add)
{
    int mask = (1 << size) - 1;
    int i, x = 0;

	LOG_RAND(("rand %d\n", size));
    for( i = 0; i < mask; i++ )
	{
		if (size == 17)
			*rng = x >> 6;	/* use bits 6..13 */
		else
			*rng = x;		/* use bits 0..7 */
        LOG_RAND(("%05x: %02x\n", x, *rng));
        rng++;
        /* calculate next bit */
		x = ((x << left) + (x >> right) + add) & mask;
	}
}


static void register_for_save(struct POKEYregisters *chip, const device_config *device)
{
	state_save_register_device_item_array(device, 0, chip->counter);
	state_save_register_device_item_array(device, 0, chip->divisor);
	state_save_register_device_item_array(device, 0, chip->volume);
	state_save_register_device_item_array(device, 0, chip->output);
	state_save_register_device_item_array(device, 0, chip->audible);
	state_save_register_device_item(device, 0, chip->samplepos_fract);
	state_save_register_device_item(device, 0, chip->samplepos_whole);
	state_save_register_device_item(device, 0, chip->polyadjust);
	state_save_register_device_item(device, 0, chip->p4);
	state_save_register_device_item(device, 0, chip->p5);
	state_save_register_device_item(device, 0, chip->p9);
	state_save_register_device_item(device, 0, chip->p17);
	state_save_register_device_item(device, 0, chip->r9);
	state_save_register_device_item(device, 0, chip->r17);
	state_save_register_device_item(device, 0, chip->clockmult);
	state_save_register_device_item(device, 0, chip->timer_period[0].seconds);
	state_save_register_device_item(device, 0, chip->timer_period[0].attoseconds);
	state_save_register_device_item(device, 0, chip->timer_period[1].seconds);
	state_save_register_device_item(device, 0, chip->timer_period[1].attoseconds);
	state_save_register_device_item(device, 0, chip->timer_period[2].seconds);
	state_save_register_device_item(device, 0, chip->timer_period[2].attoseconds);
	state_save_register_device_item_array(device, 0, chip->timer_param);
	state_save_register_device_item_array(device, 0, chip->AUDF);
	state_save_register_device_item_array(device, 0, chip->AUDC);
	state_save_register_device_item_array(device, 0, chip->POTx);
	state_save_register_device_item(device, 0, chip->AUDCTL);
	state_save_register_device_item(device, 0, chip->ALLPOT);
	state_save_register_device_item(device, 0, chip->KBCODE);
	state_save_register_device_item(device, 0, chip->RANDOM);
	state_save_register_device_item(device, 0, chip->SERIN);
	state_save_register_device_item(device, 0, chip->SEROUT);
	state_save_register_device_item(device, 0, chip->IRQST);
	state_save_register_device_item(device, 0, chip->IRQEN);
	state_save_register_device_item(device, 0, chip->SKSTAT);
	state_save_register_device_item(device, 0, chip->SKCTL);
}


static SND_START( pokey )
{
	struct POKEYregisters *chip;
	int sample_rate = clock;
	int i;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	if (config)
		memcpy(&chip->intf, config, sizeof(pokey_interface));
	chip->device = device;
	chip->clock_period = ATTOTIME_IN_HZ(clock);
	chip->index = sndindex;

	/* calculate the A/D times
     * In normal, slow mode (SKCTL bit SK_PADDLE is clear) the conversion
     * takes N scanlines, where N is the paddle value. A single scanline
     * takes approximately 64us to finish (1.78979MHz clock).
     * In quick mode (SK_PADDLE set) the conversion is done very fast
     * (takes two scanlines) but the result is not as accurate.
     */
	chip->ad_time_fast = attotime_div(attotime_mul(ATTOTIME_IN_NSEC(64000*2/228), FREQ_17_EXACT), clock);
	chip->ad_time_slow = attotime_div(attotime_mul(ATTOTIME_IN_NSEC(64000      ), FREQ_17_EXACT), clock);

	/* initialize the poly counters */
	poly_init(chip->poly4,   4, 3, 1, 0x00004);
	poly_init(chip->poly5,   5, 3, 2, 0x00008);
	poly_init(chip->poly9,   9, 8, 1, 0x00180);
	poly_init(chip->poly17, 17,16, 1, 0x1c000);

	/* initialize the random arrays */
	rand_init(chip->rand9,   9, 8, 1, 0x00180);
	rand_init(chip->rand17, 17,16, 1, 0x1c000);

	chip->samplerate_24_8 = (clock << 8) / sample_rate;
	chip->divisor[CHAN1] = 4;
	chip->divisor[CHAN2] = 4;
	chip->divisor[CHAN3] = 4;
	chip->divisor[CHAN4] = 4;
	chip->clockmult = DIV_64;
	chip->KBCODE = 0x09;		 /* Atari 800 'no key' */
	chip->SKCTL = SK_RESET;	 /* let the RNG run after reset */
	chip->rtimer = timer_alloc(device->machine,  NULL, NULL);

	chip->timer[0] = timer_alloc(device->machine, pokey_timer_expire, chip);
	chip->timer[1] = timer_alloc(device->machine, pokey_timer_expire, chip);
	chip->timer[2] = timer_alloc(device->machine, pokey_timer_expire, chip);

	for (i=0; i<8; i++)
	{
		chip->ptimer[i] = timer_alloc(device->machine, pokey_pot_trigger, chip);
		chip->pot_r[i] = chip->intf.pot_r[i];
	}
	chip->allpot_r = chip->intf.allpot_r;
	chip->serin_r = chip->intf.serin_r;
	chip->serout_w = chip->intf.serout_w;
	chip->interrupt_cb = chip->intf.interrupt_cb;

	chip->channel = stream_create(device, 0, 1, sample_rate, chip, pokey_update);

	register_for_save(chip, device);

    return chip;
}

static TIMER_CALLBACK( pokey_timer_expire )
{
	struct POKEYregisters *p = ptr;
	int timers = param;

	LOG_TIMER(("POKEY #%p timer %d with IRQEN $%02x\n", p, timers, p->IRQEN));

    /* check if some of the requested timer interrupts are enabled */
	timers &= p->IRQEN;

    if( timers )
    {
		/* set the enabled timer irq status bits */
		p->IRQST |= timers;
        /* call back an application supplied function to handle the interrupt */
		if( p->interrupt_cb )
			(*p->interrupt_cb)(machine, timers);
    }
}

static char *audc2str(int val)
{
	static char buff[80];
	if( val & NOTPOLY5 )
	{
		if( val & PURE )
			strcpy(buff,"pure");
		else
		if( val & POLY4 )
			strcpy(buff,"poly4");
		else
			strcpy(buff,"poly9/17");
	}
	else
	{
		if( val & PURE )
			strcpy(buff,"poly5");
		else
		if( val & POLY4 )
			strcpy(buff,"poly4+poly5");
		else
			strcpy(buff,"poly9/17+poly5");
    }
	return buff;
}

static char *audctl2str(int val)
{
	static char buff[80];
	if( val & POLY9 )
		strcpy(buff,"poly9");
	else
		strcpy(buff,"poly17");
	if( val & CH1_HICLK )
		strcat(buff,"+ch1hi");
	if( val & CH3_HICLK )
		strcat(buff,"+ch3hi");
	if( val & CH12_JOINED )
		strcat(buff,"+ch1/2");
	if( val & CH34_JOINED )
		strcat(buff,"+ch3/4");
	if( val & CH1_FILTER )
		strcat(buff,"+ch1filter");
	if( val & CH2_FILTER )
		strcat(buff,"+ch2filter");
	if( val & CLK_15KHZ )
		strcat(buff,"+clk15");
    return buff;
}

static TIMER_CALLBACK( pokey_serin_ready )
{
	struct POKEYregisters *p = ptr;
    if( p->IRQEN & IRQ_SERIN )
	{
		/* set the enabled timer irq status bits */
		p->IRQST |= IRQ_SERIN;
		/* call back an application supplied function to handle the interrupt */
		if( p->interrupt_cb )
			(*p->interrupt_cb)(machine, IRQ_SERIN);
	}
}

static TIMER_CALLBACK( pokey_serout_ready )
{
	struct POKEYregisters *p = ptr;
    if( p->IRQEN & IRQ_SEROR )
	{
		p->IRQST |= IRQ_SEROR;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(machine, IRQ_SEROR);
	}
}

static TIMER_CALLBACK( pokey_serout_complete )
{
	struct POKEYregisters *p = ptr;
    if( p->IRQEN & IRQ_SEROC )
	{
		p->IRQST |= IRQ_SEROC;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(machine, IRQ_SEROC);
	}
}

static TIMER_CALLBACK( pokey_pot_trigger )
{
	struct POKEYregisters *p = ptr;
	int pot = param;
	LOG(("POKEY #%p POT%d triggers after %dus\n", p, pot, (int)(1000000 * attotime_to_double(timer_timeelapsed(p->ptimer[pot])))));
	p->ALLPOT &= ~(1 << pot);	/* set the enabled timer irq status bits */
}

#define AD_TIME  ((p->SKCTL & SK_PADDLE) ? p->ad_time_fast : p->ad_time_slow)

static void pokey_potgo(const address_space *space, struct POKEYregisters *p)
{
    int pot;

	LOG(("POKEY #%p pokey_potgo\n", p));

    p->ALLPOT = 0xff;

    for( pot = 0; pot < 8; pot++ )
	{
		p->POTx[pot] = 0xff;
		if( p->pot_r[pot] )
		{
			int r = (*p->pot_r[pot])(space, pot);

			LOG(("POKEY #%d pot_r(%d) returned $%02x\n", p->index, pot, r));
			if( r != -1 )
			{
				if (r > 228)
                    r = 228;

                /* final value */
                p->POTx[pot] = r;
				timer_adjust_oneshot(p->ptimer[pot], attotime_mul(AD_TIME, r), pot);
			}
		}
	}
}

static int pokey_register_r(int chip, int offs)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
	/* temporary hack until this is converted to a device */
	const address_space *space;
	int data = 0, pot;
	UINT32 adjust = 0;

#ifdef MAME_DEBUG
	if( !p )
	{
		logerror("POKEY #%d is >= number of Pokeys!\n", chip);
		return data;
	}
#endif

	space = memory_find_address_space(p->device->machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	switch (offs & 15)
	{
	case POT0_C: case POT1_C: case POT2_C: case POT3_C:
	case POT4_C: case POT5_C: case POT6_C: case POT7_C:
		pot = offs & 7;
		if( p->pot_r[pot] )
		{
			/*
             * If the conversion is not yet finished (ptimer running),
             * get the current value by the linear interpolation of
             * the final value using the elapsed time.
             */
			if( p->ALLPOT & (1 << pot) )
			{
				data = timer_timeelapsed(p->ptimer[pot]).attoseconds / AD_TIME.attoseconds;
				LOG(("POKEY #%d read POT%d (interpolated) $%02x\n", chip, pot, data));
            }
			else
			{
				data = p->POTx[pot];
				LOG(("POKEY #%d read POT%d (final value)  $%02x\n", chip, pot, data));
			}
		}
		else
			logerror("%s: warning - read p[chip] #%d POT%d\n", cpuexec_describe_context(p->device->machine), chip, pot);
		break;

    case ALLPOT_C:
		/****************************************************************
         * If the 2 least significant bits of SKCTL are 0, the ALLPOTs
         * are disabled (SKRESET). Thanks to MikeJ for pointing this out.
         ****************************************************************/
		if( (p->SKCTL & SK_RESET) == 0)
		{
			data = 0;
			LOG(("POKEY #%d ALLPOT internal $%02x (reset)\n", chip, data));
		}
		else if( p->allpot_r )
		{
			data = (*p->allpot_r)(space, offs);
			LOG(("POKEY #%d ALLPOT callback $%02x\n", chip, data));
		}
		else
		{
			data = p->ALLPOT;
			LOG(("POKEY #%d ALLPOT internal $%02x\n", chip, data));
		}
		break;

	case KBCODE_C:
		data = p->KBCODE;
		break;

	case RANDOM_C:
		/****************************************************************
         * If the 2 least significant bits of SKCTL are 0, the random
         * number generator is disabled (SKRESET). Thanks to Eric Smith
         * for pointing out this critical bit of info! If the random
         * number generator is enabled, get a new random number. Take
         * the time gone since the last read into account and read the
         * new value from an appropriate offset in the rand17 table.
         ****************************************************************/
		if( p->SKCTL & SK_RESET )
		{
			adjust = attotime_to_double(timer_timeelapsed(p->rtimer)) / attotime_to_double(p->clock_period);
			p->r9 = (p->r9 + adjust) % 0x001ff;
			p->r17 = (p->r17 + adjust) % 0x1ffff;
		}
		else
		{
			adjust = 1;
			p->r9 = 0;
			p->r17 = 0;
            LOG_RAND(("POKEY #%d rand17 frozen (SKCTL): $%02x\n", chip, p->RANDOM));
		}
		if( p->AUDCTL & POLY9 )
		{
			p->RANDOM = p->rand9[p->r9];
			LOG_RAND(("POKEY #%d adjust %u rand9[$%05x]: $%02x\n", chip, adjust, p->r9, p->RANDOM));
		}
		else
		{
			p->RANDOM = p->rand17[p->r17];
			LOG_RAND(("POKEY #%d adjust %u rand17[$%05x]: $%02x\n", chip, adjust, p->r17, p->RANDOM));
		}
		if (adjust > 0)
			timer_adjust_oneshot(p->rtimer, attotime_never, 0);
		data = p->RANDOM ^ 0xff;
		break;

	case SERIN_C:
		if( p->serin_r )
			p->SERIN = (*p->serin_r)(space, offs);
		data = p->SERIN;
		LOG(("POKEY #%d SERIN  $%02x\n", chip, data));
		break;

	case IRQST_C:
		/* IRQST is an active low input port; we keep it active high */
		/* internally to ease the (un-)masking of bits */
		data = p->IRQST ^ 0xff;
		LOG(("POKEY #%d IRQST  $%02x\n", chip, data));
		break;

	case SKSTAT_C:
		/* SKSTAT is also an active low input port */
		data = p->SKSTAT ^ 0xff;
		LOG(("POKEY #%d SKSTAT $%02x\n", chip, data));
		break;

	default:
		LOG(("POKEY #%d register $%02x\n", chip, offs));
        break;
    }
    return data;
}

READ8_HANDLER( pokey1_r )
{
	return pokey_register_r(0, offset);
}

READ8_HANDLER( pokey2_r )
{
	return pokey_register_r(1, offset);
}

READ8_HANDLER( pokey3_r )
{
	return pokey_register_r(2, offset);
}

READ8_HANDLER( pokey4_r )
{
	return pokey_register_r(3, offset);
}

READ8_HANDLER( quad_pokey_r )
{
	int pokey_num = (offset >> 3) & ~0x04;
	int control = (offset & 0x20) >> 2;
	int pokey_reg = (offset % 8) | control;

	return pokey_register_r(pokey_num, pokey_reg);
}


static void pokey_register_w(int chip, int offs, int data)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
	/* temporary hack until this is converted to a device */
	const address_space *space = memory_find_address_space(p->device->machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	int ch_mask = 0, new_val;

#ifdef MAME_DEBUG
	if( !p )
	{
		logerror("POKEY #%d is >= number of Pokeys!\n", chip);
		return;
	}
#endif
	stream_update(p->channel);

    /* determine which address was changed */
	switch (offs & 15)
    {
    case AUDF1_C:
		if( data == p->AUDF[CHAN1] )
            return;
		LOG_SOUND(("POKEY #%d AUDF1  $%02x\n", chip, data));
		p->AUDF[CHAN1] = data;
        ch_mask = 1 << CHAN1;
		if( p->AUDCTL & CH12_JOINED )		/* if ch 1&2 tied together */
            ch_mask |= 1 << CHAN2;    /* then also change on ch2 */
        break;

    case AUDC1_C:
		if( data == p->AUDC[CHAN1] )
            return;
		LOG_SOUND(("POKEY #%d AUDC1  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN1] = data;
        ch_mask = 1 << CHAN1;
        break;

    case AUDF2_C:
		if( data == p->AUDF[CHAN2] )
            return;
		LOG_SOUND(("POKEY #%d AUDF2  $%02x\n", chip, data));
		p->AUDF[CHAN2] = data;
        ch_mask = 1 << CHAN2;
        break;

    case AUDC2_C:
		if( data == p->AUDC[CHAN2] )
            return;
		LOG_SOUND(("POKEY #%d AUDC2  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN2] = data;
        ch_mask = 1 << CHAN2;
        break;

    case AUDF3_C:
		if( data == p->AUDF[CHAN3] )
            return;
		LOG_SOUND(("POKEY #%d AUDF3  $%02x\n", chip, data));
		p->AUDF[CHAN3] = data;
        ch_mask = 1 << CHAN3;

		if( p->AUDCTL & CH34_JOINED )	/* if ch 3&4 tied together */
            ch_mask |= 1 << CHAN4;  /* then also change on ch4 */
        break;

    case AUDC3_C:
		if( data == p->AUDC[CHAN3] )
            return;
		LOG_SOUND(("POKEY #%d AUDC3  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN3] = data;
        ch_mask = 1 << CHAN3;
        break;

    case AUDF4_C:
		if( data == p->AUDF[CHAN4] )
            return;
		LOG_SOUND(("POKEY #%d AUDF4  $%02x\n", chip, data));
		p->AUDF[CHAN4] = data;
        ch_mask = 1 << CHAN4;
        break;

    case AUDC4_C:
		if( data == p->AUDC[CHAN4] )
            return;
		LOG_SOUND(("POKEY #%d AUDC4  $%02x (%s)\n", chip, data, audc2str(data)));
		p->AUDC[CHAN4] = data;
        ch_mask = 1 << CHAN4;
        break;

    case AUDCTL_C:
		if( data == p->AUDCTL )
            return;
		LOG_SOUND(("POKEY #%d AUDCTL $%02x (%s)\n", chip, data, audctl2str(data)));
		p->AUDCTL = data;
        ch_mask = 15;       /* all channels */
        /* determine the base multiplier for the 'div by n' calculations */
		p->clockmult = (p->AUDCTL & CLK_15KHZ) ? DIV_15 : DIV_64;
        break;

    case STIMER_C:
        /* first remove any existing timers */
		LOG_TIMER(("POKEY #%d STIMER $%02x\n", chip, data));

		timer_adjust_oneshot(p->timer[TIMER1], attotime_never, p->timer_param[TIMER1]);
		timer_adjust_oneshot(p->timer[TIMER2], attotime_never, p->timer_param[TIMER2]);
		timer_adjust_oneshot(p->timer[TIMER4], attotime_never, p->timer_param[TIMER4]);

        /* reset all counters to zero (side effect) */
		p->polyadjust = 0;
		p->counter[CHAN1] = 0;
		p->counter[CHAN2] = 0;
		p->counter[CHAN3] = 0;
		p->counter[CHAN4] = 0;

        /* joined chan#1 and chan#2 ? */
		if( p->AUDCTL & CH12_JOINED )
        {
			if( p->divisor[CHAN2] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer1+2 after %d clocks\n", chip, p->divisor[CHAN2]));
				/* set timer #1 _and_ #2 event after timer_div clocks of joined CHAN1+CHAN2 */
				p->timer_period[TIMER2] = attotime_mul(p->clock_period, p->divisor[CHAN2]);
				p->timer_param[TIMER2] = (chip<<3)|IRQ_TIMR2|IRQ_TIMR1;
				timer_adjust_periodic(p->timer[TIMER2], p->timer_period[TIMER2], p->timer_param[TIMER2], p->timer_period[TIMER2]);
			}
        }
        else
        {
			if( p->divisor[CHAN1] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer1 after %d clocks\n", chip, p->divisor[CHAN1]));
				/* set timer #1 event after timer_div clocks of CHAN1 */
				p->timer_period[TIMER1] = attotime_mul(p->clock_period, p->divisor[CHAN1]);
				p->timer_param[TIMER1] = (chip<<3)|IRQ_TIMR1;
				timer_adjust_periodic(p->timer[TIMER1], p->timer_period[TIMER1], p->timer_param[TIMER1], p->timer_period[TIMER1]);
			}

			if( p->divisor[CHAN2] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer2 after %d clocks\n", chip, p->divisor[CHAN2]));
				/* set timer #2 event after timer_div clocks of CHAN2 */
				p->timer_period[TIMER2] = attotime_mul(p->clock_period, p->divisor[CHAN2]);
				p->timer_param[TIMER2] = (chip<<3)|IRQ_TIMR2;
				timer_adjust_periodic(p->timer[TIMER2], p->timer_period[TIMER2], p->timer_param[TIMER2], p->timer_period[TIMER2]);
			}
        }

		/* Note: p[chip] does not have a timer #3 */

		if( p->AUDCTL & CH34_JOINED )
        {
            /* not sure about this: if audc4 == 0000xxxx don't start timer 4 ? */
			if( p->AUDC[CHAN4] & 0xf0 )
            {
				if( p->divisor[CHAN4] > 4 )
				{
					LOG_TIMER(("POKEY #%d timer4 after %d clocks\n", chip, p->divisor[CHAN4]));
					/* set timer #4 event after timer_div clocks of CHAN4 */
					p->timer_period[TIMER4] = attotime_mul(p->clock_period, p->divisor[CHAN4]);
					p->timer_param[TIMER4] = (chip<<3)|IRQ_TIMR4;
					timer_adjust_periodic(p->timer[TIMER4], p->timer_period[TIMER4], p->timer_param[TIMER4], p->timer_period[TIMER4]);
				}
            }
        }
        else
        {
			if( p->divisor[CHAN4] > 4 )
			{
				LOG_TIMER(("POKEY #%d timer4 after %d clocks\n", chip, p->divisor[CHAN4]));
				/* set timer #4 event after timer_div clocks of CHAN4 */
				p->timer_period[TIMER4] = attotime_mul(p->clock_period, p->divisor[CHAN4]);
				p->timer_param[TIMER4] = (chip<<3)|IRQ_TIMR4;
				timer_adjust_periodic(p->timer[TIMER4], p->timer_period[TIMER4], p->timer_param[TIMER4], p->timer_period[TIMER4]);
			}
        }

		timer_enable(p->timer[TIMER1], p->IRQEN & IRQ_TIMR1);
		timer_enable(p->timer[TIMER2], p->IRQEN & IRQ_TIMR2);
		timer_enable(p->timer[TIMER4], p->IRQEN & IRQ_TIMR4);
        break;

    case SKREST_C:
        /* reset SKSTAT */
		LOG(("POKEY #%d SKREST $%02x\n", chip, data));
		p->SKSTAT &= ~(SK_FRAME|SK_OVERRUN|SK_KBERR);
        break;

    case POTGO_C:
		LOG(("POKEY #%d POTGO  $%02x\n", chip, data));
		pokey_potgo(space, p);
        break;

    case SEROUT_C:
		LOG(("POKEY #%d SEROUT $%02x\n", chip, data));
		if (p->serout_w)
			(*p->serout_w)(space, offs, data);
		p->SKSTAT |= SK_SEROUT;
        /*
         * These are arbitrary values, tested with some custom boot
         * loaders from Ballblazer and Escape from Fractalus
         * The real times are unknown
         */
        timer_set(space->machine, ATTOTIME_IN_USEC(200), p, 0, pokey_serout_ready);
        /* 10 bits (assumption 1 start, 8 data and 1 stop bit) take how long? */
        timer_set(space->machine, ATTOTIME_IN_USEC(2000), p, 0, pokey_serout_complete);
        break;

    case IRQEN_C:
		LOG(("POKEY #%d IRQEN  $%02x\n", chip, data));

        /* acknowledge one or more IRQST bits ? */
		if( p->IRQST & ~data )
        {
            /* reset IRQST bits that are masked now */
			p->IRQST &= data;
        }
        else
        {
			/* enable/disable timers now to avoid unneeded
               breaking of the CPU cores for masked timers */
			if( p->timer[TIMER1] && ((p->IRQEN^data) & IRQ_TIMR1) )
				timer_enable(p->timer[TIMER1], data & IRQ_TIMR1);
			if( p->timer[TIMER2] && ((p->IRQEN^data) & IRQ_TIMR2) )
				timer_enable(p->timer[TIMER2], data & IRQ_TIMR2);
			if( p->timer[TIMER4] && ((p->IRQEN^data) & IRQ_TIMR4) )
				timer_enable(p->timer[TIMER4], data & IRQ_TIMR4);
        }
		/* store irq enable */
		p->IRQEN = data;
        break;

    case SKCTL_C:
		if( data == p->SKCTL )
            return;
		LOG(("POKEY #%d SKCTL  $%02x\n", chip, data));
		p->SKCTL = data;
        if( !(data & SK_RESET) )
        {
            pokey_register_w(chip, IRQEN_C,  0);
            pokey_register_w(chip, SKREST_C, 0);
        }
        break;
    }

	/************************************************************
     * As defined in the manual, the exact counter values are
     * different depending on the frequency and resolution:
     *    64 kHz or 15 kHz - AUDF + 1
     *    1.79 MHz, 8-bit  - AUDF + 4
     *    1.79 MHz, 16-bit - AUDF[CHAN1]+256*AUDF[CHAN2] + 7
     ************************************************************/

    /* only reset the channels that have changed */

    if( ch_mask & (1 << CHAN1) )
    {
        /* process channel 1 frequency */
		if( p->AUDCTL & CH1_HICLK )
			new_val = p->AUDF[CHAN1] + DIVADD_HICLK;
        else
			new_val = (p->AUDF[CHAN1] + DIVADD_LOCLK) * p->clockmult;

		LOG_SOUND(("POKEY #%d chan1 %d\n", chip, new_val));

		p->volume[CHAN1] = (p->AUDC[CHAN1] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
        p->divisor[CHAN1] = new_val;
		if( new_val < p->counter[CHAN1] )
			p->counter[CHAN1] = new_val;
		if( p->interrupt_cb && p->timer[TIMER1] )
			timer_adjust_periodic(p->timer[TIMER1], attotime_mul(p->clock_period, new_val), p->timer_param[TIMER1], p->timer_period[TIMER1]);
		p->audible[CHAN1] = !(
			(p->AUDC[CHAN1] & VOLUME_ONLY) ||
			(p->AUDC[CHAN1] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN1] & PURE) && new_val < (p->samplerate_24_8 >> 8)));
		if( !p->audible[CHAN1] )
		{
			p->output[CHAN1] = 1;
			p->counter[CHAN1] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
            p->volume[CHAN1] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN2) )
    {
        /* process channel 2 frequency */
		if( p->AUDCTL & CH12_JOINED )
        {
			if( p->AUDCTL & CH1_HICLK )
				new_val = p->AUDF[CHAN2] * 256 + p->AUDF[CHAN1] + DIVADD_HICLK_JOINED;
            else
				new_val = (p->AUDF[CHAN2] * 256 + p->AUDF[CHAN1] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan1+2 %d\n", chip, new_val));
        }
        else
		{
			new_val = (p->AUDF[CHAN2] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan2 %d\n", chip, new_val));
		}

		p->volume[CHAN2] = (p->AUDC[CHAN2] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN2] = new_val;
		if( new_val < p->counter[CHAN2] )
			p->counter[CHAN2] = new_val;
		if( p->interrupt_cb && p->timer[TIMER2] )
			timer_adjust_periodic(p->timer[TIMER2], attotime_mul(p->clock_period, new_val), p->timer_param[TIMER2], p->timer_period[TIMER2]);
		p->audible[CHAN2] = !(
			(p->AUDC[CHAN2] & VOLUME_ONLY) ||
			(p->AUDC[CHAN2] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN2] & PURE) && new_val < (p->samplerate_24_8 >> 8)));
		if( !p->audible[CHAN2] )
		{
			p->output[CHAN2] = 1;
			p->counter[CHAN2] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN2] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN3) )
    {
        /* process channel 3 frequency */
		if( p->AUDCTL & CH3_HICLK )
			new_val = p->AUDF[CHAN3] + DIVADD_HICLK;
        else
			new_val = (p->AUDF[CHAN3] + DIVADD_LOCLK) * p->clockmult;

		LOG_SOUND(("POKEY #%d chan3 %d\n", chip, new_val));

		p->volume[CHAN3] = (p->AUDC[CHAN3] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN3] = new_val;
		if( new_val < p->counter[CHAN3] )
			p->counter[CHAN3] = new_val;
		/* channel 3 does not have a timer associated */
		p->audible[CHAN3] = !(
			(p->AUDC[CHAN3] & VOLUME_ONLY) ||
			(p->AUDC[CHAN3] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN3] & PURE) && new_val < (p->samplerate_24_8 >> 8))) ||
			(p->AUDCTL & CH1_FILTER);
		if( !p->audible[CHAN3] )
		{
			p->output[CHAN3] = 1;
			p->counter[CHAN3] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN3] >>= 1;
        }
    }

    if( ch_mask & (1 << CHAN4) )
    {
        /* process channel 4 frequency */
		if( p->AUDCTL & CH34_JOINED )
        {
			if( p->AUDCTL & CH3_HICLK )
				new_val = p->AUDF[CHAN4] * 256 + p->AUDF[CHAN3] + DIVADD_HICLK_JOINED;
            else
				new_val = (p->AUDF[CHAN4] * 256 + p->AUDF[CHAN3] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan3+4 %d\n", chip, new_val));
        }
        else
		{
			new_val = (p->AUDF[CHAN4] + DIVADD_LOCLK) * p->clockmult;
			LOG_SOUND(("POKEY #%d chan4 %d\n", chip, new_val));
		}

		p->volume[CHAN4] = (p->AUDC[CHAN4] & VOLUME_MASK) * POKEY_DEFAULT_GAIN;
		p->divisor[CHAN4] = new_val;
		if( new_val < p->counter[CHAN4] )
			p->counter[CHAN4] = new_val;
		if( p->interrupt_cb && p->timer[TIMER4] )
			timer_adjust_periodic(p->timer[TIMER4], attotime_mul(p->clock_period, new_val), p->timer_param[TIMER4], p->timer_period[TIMER4]);
		p->audible[CHAN4] = !(
			(p->AUDC[CHAN4] & VOLUME_ONLY) ||
			(p->AUDC[CHAN4] & VOLUME_MASK) == 0 ||
			((p->AUDC[CHAN4] & PURE) && new_val < (p->samplerate_24_8 >> 8))) ||
			(p->AUDCTL & CH2_FILTER);
		if( !p->audible[CHAN4] )
		{
			p->output[CHAN4] = 1;
			p->counter[CHAN4] = 0x7fffffff;
			/* 50% duty cycle should result in half volume */
			p->volume[CHAN4] >>= 1;
        }
    }
}

WRITE8_HANDLER( pokey1_w )
{
	pokey_register_w(0,offset,data);
}

WRITE8_HANDLER( pokey2_w )
{
    pokey_register_w(1,offset,data);
}

WRITE8_HANDLER( pokey3_w )
{
    pokey_register_w(2,offset,data);
}

WRITE8_HANDLER( pokey4_w )
{
    pokey_register_w(3,offset,data);
}

WRITE8_HANDLER( quad_pokey_w )
{
    int pokey_num = (offset >> 3) & ~0x04;
    int control = (offset & 0x20) >> 2;
    int pokey_reg = (offset % 8) | control;

    pokey_register_w(pokey_num, pokey_reg, data);
}

void pokey1_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 0);
	timer_set(p->device->machine, attotime_mul(p->clock_period, after), p, 0, pokey_serin_ready);
}

void pokey2_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 1);
	timer_set(p->device->machine, attotime_mul(p->clock_period, after), p, 0, pokey_serin_ready);
}

void pokey3_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 2);
	timer_set(p->device->machine, attotime_mul(p->clock_period, after), p, 0, pokey_serin_ready);
}

void pokey4_serin_ready(int after)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, 3);
	timer_set(p->device->machine, attotime_mul(p->clock_period, after), p, 0, pokey_serin_ready);
}

static void pokey_break_w(int chip, int shift)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
	if( shift )                     /* shift code ? */
		p->SKSTAT |= SK_SHIFT;
	else
		p->SKSTAT &= ~SK_SHIFT;
	/* check if the break IRQ is enabled */
	if( p->IRQEN & IRQ_BREAK )
	{
		/* set break IRQ status and call back the interrupt handler */
		p->IRQST |= IRQ_BREAK;
		if( p->interrupt_cb )
			(*p->interrupt_cb)(p->device->machine, IRQ_BREAK);
	}
}

void pokey1_break_w(int shift)
{
	pokey_break_w(0, shift);
}

void pokey2_break_w(int shift)
{
	pokey_break_w(1, shift);
}

void pokey3_break_w(int shift)
{
	pokey_break_w(2, shift);
}

void pokey4_break_w(int shift)
{
	pokey_break_w(3, shift);
}

static void pokey_kbcode_w(int chip, int kbcode, int make)
{
	struct POKEYregisters *p = sndti_token(SOUND_POKEY, chip);
    /* make code ? */
	if( make )
	{
		p->KBCODE = kbcode;
		p->SKSTAT |= SK_KEYBD;
		if( kbcode & 0x40 ) 		/* shift code ? */
			p->SKSTAT |= SK_SHIFT;
		else
			p->SKSTAT &= ~SK_SHIFT;

		if( p->IRQEN & IRQ_KEYBD )
		{
			/* last interrupt not acknowledged ? */
			if( p->IRQST & IRQ_KEYBD )
				p->SKSTAT |= SK_KBERR;
			p->IRQST |= IRQ_KEYBD;
			if( p->interrupt_cb )
				(*p->interrupt_cb)(p->device->machine, IRQ_KEYBD);
		}
	}
	else
	{
		p->KBCODE = kbcode;
		p->SKSTAT &= ~SK_KEYBD;
    }
}

void pokey1_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(0, kbcode, make);
}

void pokey2_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(1, kbcode, make);
}

void pokey3_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(2, kbcode, make);
}

void pokey4_kbcode_w(int kbcode, int make)
{
	pokey_kbcode_w(3, kbcode, make);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static SND_SET_INFO( pokey )
{
	switch (state)
	{
		/* no parameters to set */
	}
}


SND_GET_INFO( pokey )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = SND_SET_INFO_NAME( pokey );	break;
		case SNDINFO_PTR_START:							info->start = SND_START_NAME( pokey );			break;
		case SNDINFO_PTR_STOP:							/* Nothing */									break;
		case SNDINFO_PTR_RESET:							/* Nothing */									break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							strcpy(info->s, "POKEY");						break;
		case SNDINFO_STR_CORE_FAMILY:					strcpy(info->s, "Atari custom");				break;
		case SNDINFO_STR_CORE_VERSION:					strcpy(info->s, "4.51");						break;
		case SNDINFO_STR_CORE_FILE:						strcpy(info->s, __FILE__);						break;
		case SNDINFO_STR_CORE_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}

