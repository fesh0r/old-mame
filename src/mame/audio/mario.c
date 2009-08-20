#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/mcs48/mcs48.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "sound/discrete.h"

#include "includes/mario.h"

/****************************************************************
 *
 * Defines and Macros
 *
 ****************************************************************/

/* FIXME: Capacitor aging - only in for calibration now        */
/* Adjustments are needed to bring this close to recordings    */

#define CAP_AGE(c, f)	((c) * (f))

/* FIXME: Awaiting verification. VCO range inputs are shown    */
/* as connected to +5V on schematics. This does not match      */
/* recordings                                                  */

#define RUN_VCO_VOLTAGE		(0.0)	/* 5 in schematics */

#define USE_8039	(0)			/* set to 1 to try 8039 hack */

#define ACTIVELOW_PORT_BIT(P,A,D)   ((P & (~(1 << A))) | ((D ^ 1) << A))
#define ACTIVEHIGH_PORT_BIT(P,A,D)   ((P & (~(1 << A))) | (D << A))

#define I8035_T_R(M,N) ((soundlatch2_r(M,0) >> (N)) & 1)
#define I8035_T_W_AH(M,N,D) do { state->portT = ACTIVEHIGH_PORT_BIT(state->portT,N,D); soundlatch2_w(M, 0, state->portT); } while (0)

#define I8035_P1_R(M) (soundlatch3_r(M,0))
#define I8035_P2_R(M) (soundlatch4_r(M,0))
#define I8035_P1_W(M,D) soundlatch3_w(M,0,D)

#if (USE_8039)
#define I8035_P2_W(M,D) do { soundlatch4_w(M,0,D); } while (0)
#else
#define I8035_P2_W(M,D) do { set_ea(M, ((D) & 0x20) ? 0 : 1);  soundlatch4_w(M,0,D); } while (0)
#endif

#define I8035_P1_W_AH(M,B,D) I8035_P1_W(M,ACTIVEHIGH_PORT_BIT(I8035_P1_R(M),B,(D)))
#define I8035_P2_W_AH(M,B,D) I8035_P2_W(M,ACTIVEHIGH_PORT_BIT(I8035_P2_R(M),B,(D)))

/****************************************************************
 *
 * Discrete Sound defines
 *
 ****************************************************************/

/* Discrete sound inputs */

#define DS_SOUND0_INV		NODE_01
#define DS_SOUND1_INV		NODE_02
#define DS_SOUND7_INV		NODE_05
#define DS_DAC				NODE_07

#define DS_SOUND0			NODE_208
#define DS_SOUND1			NODE_209
#define DS_SOUND7			NODE_212

#define DS_OUT_SOUND0		NODE_241
#define DS_OUT_SOUND1		NODE_242
#define DS_OUT_SOUND7		NODE_248
#define DS_OUT_DAC			NODE_250

/* Input definitions for write handlers */

#define DS_SOUND0_INP		DS_SOUND0_INV
#define DS_SOUND1_INP		DS_SOUND1_INV
#define DS_SOUND7_INP		DS_SOUND7_INV

/* General defines */

#define VSS					5.0
#define TTL_HIGH			4.0
#define GND					0.0

/****************************************************************
 *
 * Mario Discrete Sound Interface
 *
 * verified against picture of a TMA1-04-CPU Board. Some
 * of the values were hard to read, though.
 *
 ****************************************************************/

#define MR_R6		RES_K(4.7)		/* verified                             */
#define MR_R7		RES_K(4.7)		/* verified                             */
#define MR_R17		RES_K(27)		/* 20 according to parts list           */
									/* 27 verified, 30K in schematics       */
#define MR_R18		RES_K(27)		/* 20 according to parts list           */
									/* 27 verified, 30K in schematics       */
#define MR_R19		RES_K(22)		/* verified                             */
#define MR_R20		RES_K(22)		/* verified                             */
#define MR_R40		RES_K(22)		/* verified                             */
#define MR_R41		RES_K(100)		/* verified, hard to read               */
#define MR_R42		RES_K(43)
#define MR_R43		RES_K(100)
#define MR_R61		RES_K(47)		/* verified, hard to read               */
#define MR_R64		RES_K(20)		/* verified                             */
#define MR_R65		RES_K(10)		/* verified                             */

#define MR_C3		CAP_U(10)		/* verified                             */
#define MR_C4		CAP_U(4.7)		/* illegible, 4.7 or 47 pcb/schematics  */
									/* 4.7 in parts list                    */
#define MR_C5		CAP_N(39)		/* illegible on pcb                     */
#define MR_C6		CAP_N(3.9)		/* illegible on pcb                     */
#define MR_C14		CAP_U(4.7)		/* verified                             */
#define MR_C15		CAP_U(4.7)		/* verified                             */
#define MR_C16		CAP_N(6.8)		/* verified                             */
#define MR_C17		CAP_N(22)		/* illegible on pcb                     */
#define MR_C31		CAP_U(0.022)	/* not found                            */
#define MR_C32		CAP_U(1)		/* illegible on pcb                     */
#define MR_C39		CAP_N(4.7)		/* not found                            */
#define MR_C40		CAP_N(22)		/* verified                             */
//#define MR_C41        CAP_U(4.7)      /* verified, hard to read           */
#define MR_C41		CAP_U(4.7)
#define MR_C43		CAP_U(3.3)		/* verified                             */
#define MR_C44		CAP_U(3.3)		/* verified                             */

#define MR_MIXER_RPAR  RES_4_PARALLEL(MR_R20, MR_R19, MR_R41, MR_R40)


/* KT = 0.25 for diode circuit, 0.33 else */

#define DISCRETE_LS123(_N, _T, _R, _C) \
	DISCRETE_ONESHOTR(_N, 0, _T, TTL_HIGH, (0.25 * (_R) * (_C) * (1.0+700./(_R))), DISC_ONESHOT_RETRIG | DISC_ONESHOT_REDGE)
#define DISCRETE_LS123_INV(_N, _T, _R, _C) \
	DISCRETE_ONESHOTR(_N, 0, _T, TTL_HIGH, (0.25 * (_R) * (_C) * (1.0+700./(_R))), DISC_ONESHOT_RETRIG | DISC_ONESHOT_REDGE | DISC_OUT_ACTIVE_LOW)

/* The following formula was derived from figures 2 and 3 in LS624 datasheet. Coefficients
 * where calculated using least square approximation.
 * This approach gives a bit better results compared to the first approach.
 */
#define LS624_F(_C, _VI, _VR)	pow(10, -0.912029404 * log10(_C) + 0.243264328 * (_VI) \
		          - 0.091695877 * (_VR) -0.014110946 * (_VI) * (_VR) - 3.207072925)

/************************************************************************
 *
 * Custom mario run
 *
 * Two VCO with XOR'd signal
 *
 * input[0]    - Enable / Amplitude
 * input[1]    - In1 VCO Control 1
 * input[2]    - In2 VCO Control 2
 * input[3]    - C1 VCO 1 Cap
 * input[4]    - C2 VCO 2 CAP
 * input[5]    - R1
 * input[6]    - C3
 *
 * Vout >--------------------------
 *                                 |
 *             C1                  |
 *            -||-                 |
 *           |    |                 -- +---
 *           ------          ---       |AND|    R1
 *  In1 >---+   Y1 +--------+   |      |(*)+---ZZZ--+------> Out
 *          |      |        |XOR+------+---         |
 *          |      |     ---+   |                   |
 *          |      |    |    ---                   ---
 *  In2 >---+   Y2 +----                           --- C3
 *           ------                                 |
 *           |    |                                Gnd
 *            -||-
 *             C2
 ************************************************************************/
#define MARIO_CUSTOM_VOUT		(*(node->input[0]))
#define MARIO_CUSTOM_IN1		(*(node->input[1]))
#define MARIO_CUSTOM_IN2		(*(node->input[2]))
#define MARIO_CUSTOM_C1			(*(node->input[3]))
#define MARIO_CUSTOM_C2			(*(node->input[4]))
#define MARIO_CUSTOM_R1			(*(node->input[5]))
#define MARIO_CUSTOM_C3			(*(node->input[6]))

struct mario_custom_run_context
{
	int		state1;
	int		state2;
	double	remain1;
	double  remain2;
	double  vc3;
};

static DISCRETE_STEP( mario_custom_run )
{
	struct mario_custom_run_context *context = (struct mario_custom_run_context *)node->context;

	double	t1	= 0.5 / LS624_F(MARIO_CUSTOM_C1, MARIO_CUSTOM_IN1, RUN_VCO_VOLTAGE);
	double	t2	= 0.5 / LS624_F(MARIO_CUSTOM_C2, MARIO_CUSTOM_IN2, RUN_VCO_VOLTAGE);
	double  sample_t = disc_info->sample_time;
	double  vn, t;

	//if (MARIO_CUSTOM_VOUT)
	//  printf("%f %f %f %f\n", MARIO_CUSTOM_IN1, MARIO_CUSTOM_IN2, 0.5 / t1, 0.5 / t2);
	while (sample_t > 0.0f)
	{
		/* state before time advance */
		vn = (double) (context->state1 ^ context->state2);
		vn *= MARIO_CUSTOM_VOUT;
		if (context->remain1 < context->remain2)
		{
			if (context->remain1 < sample_t)
			{
				t = context->remain1;
				context->state1 ^= 1;
				sample_t -= context->remain1;
				context->remain2 -= context->remain1;
				context->remain1 = t1;
			}
			else
			{
				t = sample_t;
				context->remain1 -= sample_t;
				context->remain2 -= sample_t;
				sample_t = 0.0f;
			}
		}
		else
		{
			if (context->remain2 < sample_t)
			{
				t = context->remain2;
				context->state2 ^= 1;
				sample_t -= context->remain2;
				context->remain1 -= context->remain2;
				context->remain2 = t2;
			}
			else
			{
				t = sample_t;
				context->remain1 -= sample_t;
				context->remain2 -= sample_t;
				sample_t = 0.0f;
			}
		}

		context->vc3 += (vn - context->vc3) * (1.0 - exp(- t / (MARIO_CUSTOM_R1 * MARIO_CUSTOM_C3)));
	}
	node->output[0] = context->vc3;
}

static DISCRETE_RESET( mario_custom_run )
{
	struct mario_custom_run_context *context = (struct mario_custom_run_context *)node->context;

	context->remain1 = 0.0;
	context->remain2 = 0.0;
	context->state1 = 0;
	context->state2 = 0;
	context->vc3 = 0;
	node->output[0] = 0;
}

static const discrete_custom_info mario_custom_run_info =
{
	DISCRETE_RESET_NAME( mario_custom_run ),
	DISCRETE_STEP_NAME( mario_custom_run ),
	sizeof(struct mario_custom_run_context),
	NULL
};

static DISCRETE_SOUND_START(mario)

	/************************************************/
	/* Input register mapping for mario             */
	/************************************************/

	/* DISCRETE_INPUT_DATA */
    DISCRETE_INPUT_PULSE(DS_SOUND0_INV, 1)
    DISCRETE_INPUT_PULSE(DS_SOUND1_INV, 1)
    DISCRETE_INPUT_NOT(DS_SOUND7_INV)
    DISCRETE_INPUT_DATA(DS_DAC)

	/************************************************/
	/* SOUND0                                       */
	/************************************************/

	DISCRETE_LS123(NODE_10, DS_SOUND0_INV, MR_R17, CAP_AGE(MR_C14, 0.7))
	DISCRETE_RCFILTER(NODE_11, 1, NODE_10, MR_R6, CAP_AGE(MR_C3, 0.5) )
	DISCRETE_CUSTOM7(NODE_12, NODE_10, NODE_11, NODE_11, MR_C6, MR_C17,
			MR_MIXER_RPAR, MR_C31, &mario_custom_run_info)
	DISCRETE_MULTIPLY(DS_OUT_SOUND0, 1, NODE_12, MR_MIXER_RPAR / MR_R20)

	/************************************************/
	/* SOUND1                                       */
	/************************************************/

	DISCRETE_LS123(NODE_20, DS_SOUND1_INV, MR_R18, CAP_AGE(MR_C15, 0.7))
	DISCRETE_RCFILTER(NODE_21, 1, NODE_20, MR_R7, CAP_AGE(MR_C4, 0.5) )
	DISCRETE_CUSTOM7(NODE_22, NODE_20, NODE_21, NODE_21, MR_C5, MR_C16,
		MR_MIXER_RPAR, MR_C31, &mario_custom_run_info)
	DISCRETE_MULTIPLY(DS_OUT_SOUND1, 1, NODE_22, MR_MIXER_RPAR / MR_R19)

	/************************************************/
	/* SOUND7                                       */
	/************************************************/

	DISCRETE_COUNTER(NODE_100,1,0,NODE_118,0xFFFF,DISC_COUNT_UP,0,DISC_CLK_BY_COUNT)

	DISCRETE_BIT_DECODE(NODE_102, NODE_100, 3, 1) 	//LS157 2B
	DISCRETE_BIT_DECODE(NODE_104, NODE_100, 11, TTL_HIGH) //LS157 3B

	DISCRETE_LS123(NODE_110, DS_SOUND7_INV, MR_R61, CAP_AGE(MR_C41, 0.6))
	DISCRETE_TRANSFORM2(NODE_111, NODE_110, TTL_HIGH, "0!1*")
	DISCRETE_RCFILTER(NODE_112, 1, NODE_111, MR_R65, MR_C44)
	DISCRETE_74LS624(NODE_113, 1, NODE_112, VSS, MR_C40, DISC_LS624_OUT_LOGIC)

	DISCRETE_LOGIC_XOR(NODE_115, 1, NODE_102, NODE_113)

	DISCRETE_RCFILTER(NODE_117, 1, NODE_104, MR_R64, MR_C43)
	DISCRETE_74LS624(NODE_118, 1, NODE_117, VSS, MR_C39, DISC_LS624_OUT_COUNT_F)

	DISCRETE_LOGIC_AND(NODE_120, 1, NODE_115, NODE_110)
	DISCRETE_MULTIPLY(NODE_121, 1, NODE_120, TTL_HIGH * MR_MIXER_RPAR / MR_R41)
	DISCRETE_RCFILTER(DS_OUT_SOUND7, 1, NODE_121, MR_MIXER_RPAR, MR_C31)

	/************************************************/
	/* DAC                                          */
	/************************************************/

	/* following the resistor DAC are two opamps. The first is a 1:1 amplifier, the second
     * is a filter circuit. Simulation in LTSPICE shows, that the following is equivalent:
     */

	DISCRETE_MULTIPLY(NODE_170, 1, DS_DAC, TTL_HIGH/256.0)
	DISCRETE_RCFILTER(NODE_171, 1, NODE_170, RES_K(750), CAP_P(200))

	DISCRETE_MULTIPLY(NODE_172, 1, NODE_171, MR_MIXER_RPAR / MR_R40)
	DISCRETE_RCFILTER(DS_OUT_DAC, 1, NODE_172, MR_MIXER_RPAR, MR_C31)


	/************************************************/
	/* MIXER                                        */
	/************************************************/

	DISCRETE_ADDER4(NODE_295, 1, DS_OUT_SOUND0, DS_OUT_SOUND1, DS_OUT_SOUND7, DS_OUT_DAC)

	/* Amplifier: internal amplifier
     * Just a 1:n amplifier without filters and no output capacitor
     */
	DISCRETE_CRFILTER(NODE_296, 1, NODE_295, RES_2_PARALLEL(MR_R42, MR_R43), MR_C32)
	/* EZV20 (Monitor) ouput capacitor / Speaker */
	DISCRETE_CRFILTER(NODE_297, 1, NODE_296, 6, CAP_U(22))
	DISCRETE_OUTPUT(NODE_297, 32767.0/5.0 * 10)
	//DISCRETE_WAVELOG1(DS_OUT_SOUND0, 32767/5.0)
	//DISCRETE_WAVELOG2(NODE_296, 32767/5.0 * 2, DS_SOUND7_INV, 10000)

DISCRETE_SOUND_END

/****************************************************************
 *
 * EA / Banking
 *
 ****************************************************************/

static void set_ea(const address_space *space, int ea)
{
	mario_state	*state = (mario_state *)space->machine->driver_data;
	//printf("ea: %d\n", ea);
	//cputag_set_input_line(machine, "audiocpu", MCS48_INPUT_EA, (ea) ? ASSERT_LINE : CLEAR_LINE);
	if (state->eabank != 0)
		memory_set_bank(space->machine, state->eabank, ea);
}

/****************************************************************
 *
 * Initialization
 *
 ****************************************************************/

static SOUND_START( mario )
{
	mario_state	*state = (mario_state *)machine->driver_data;
	const device_config *audiocpu = cputag_get_cpu(machine, "audiocpu");
#if USE_8039
	UINT8 *SND = memory_region(machine, "audiocpu");

	SND[0x1001] = 0x01;
#endif

	state->eabank = 0;
	if (audiocpu != NULL && cpu_get_type(audiocpu) != CPU_Z80)
	{
		state->eabank = 1;
		memory_install_read8_handler(cpu_get_address_space(audiocpu, ADDRESS_SPACE_PROGRAM), 0x000, 0x7ff, 0, 0, (read8_space_func)SMH_BANK(1));
		memory_configure_bank(machine, 1, 0, 1, memory_region(machine, "audiocpu"), 0);
	    memory_configure_bank(machine, 1, 1, 1, memory_region(machine, "audiocpu") + 0x1000, 0x800);
	}

    state_save_register_global(machine, state->last);
	state_save_register_global(machine, state->portT);
}

static SOUND_RESET( mario )
{
	mario_state	*state = (mario_state *)machine->driver_data;
	const address_space *space = cputag_get_address_space(machine, "audiocpu", ADDRESS_SPACE_PROGRAM);

#if USE_8039
    set_ea(machine, 1);
#endif

    /* FIXME: convert to latch8 */
	soundlatch_clear_w(space,0,0);
	soundlatch2_clear_w(space,0,0);
	soundlatch3_clear_w(space,0,0);
	soundlatch4_clear_w(space,0,0);
	I8035_P1_W(space,0x00); /* Input port */
	I8035_P2_W(space,0xff); /* Port is in high impedance state after reset */

	state->last = 0;
}

/****************************************************************
 *
 * I/O Handlers - static
 *
 ****************************************************************/

static READ8_HANDLER( mario_sh_p1_r )
{
	return I8035_P1_R(space);
}

static READ8_HANDLER( mario_sh_p2_r )
{
	return I8035_P2_R(space) & 0xEF; /* Bit 4 connected to GND! */
}

static READ8_HANDLER( mario_sh_t0_r )
{
	return I8035_T_R(space,0);
}

static READ8_HANDLER( mario_sh_t1_r )
{
	return I8035_T_R(space,1);
}

static READ8_HANDLER( mario_sh_tune_r )
{
	UINT8 *SND = memory_region(space->machine, "audiocpu");
	UINT16 mask = memory_region_length(space->machine, "audiocpu")-1;
	UINT8 p2 = I8035_P2_R(space);

	if ((p2 >> 7) & 1)
		return soundlatch_r(space,offset);
	else
		return (SND[(0x1000 + (p2 & 0x0f)*256+offset) & mask]);
}

static WRITE8_DEVICE_HANDLER( mario_sh_sound_w )
{
	discrete_sound_w(device,DS_DAC,data);
}

static WRITE8_HANDLER( mario_sh_p1_w )
{
	I8035_P1_W(space,data);
}

static WRITE8_HANDLER( mario_sh_p2_w )
{
	I8035_P2_W(space,data);
}

/****************************************************************
 *
 * I/O Handlers - global
 *
 ****************************************************************/

WRITE8_HANDLER( masao_sh_irqtrigger_w )
{
	mario_state	*state = (mario_state *)space->machine->driver_data;

	if (state->last == 1 && data == 0)
	{
		/* setting bit 0 high then low triggers IRQ on the sound CPU */
		cputag_set_input_line_and_vector(space->machine, "audiocpu",0,HOLD_LINE,0xff);
	}

	state->last = data;
}

WRITE8_HANDLER( mario_sh_tuneselect_w )
{
	soundlatch_w(space,offset,data);
}

/* Sound 0 and 1 are pulsed !*/

/* Mario running sample */
WRITE8_DEVICE_HANDLER( mario_sh1_w )
{
	discrete_sound_w(device,DS_SOUND0_INP, 0);
}

/* Luigi running sample */
WRITE8_DEVICE_HANDLER( mario_sh2_w )
{
	discrete_sound_w(device,DS_SOUND1_INP, 0);
}

/* Misc samples */
WRITE8_HANDLER( mario_sh3_w )
{
	mario_state	*state = (mario_state *)space->machine->driver_data;

	switch (offset)
	{
		case 0: /* death */
			if (data)
				cputag_set_input_line(space->machine, "audiocpu",0,ASSERT_LINE);
			else
				cputag_set_input_line(space->machine, "audiocpu",0,CLEAR_LINE);
			break;
		case 1: /* get coin */
			I8035_T_W_AH(space,0,data & 1);
			break;
		case 2: /* ice */
			I8035_T_W_AH(space,1,data & 1);
			break;
		case 3: /* crab */
			I8035_P1_W_AH(space,0,data & 1);
			break;
		case 4: /* turtle */
			I8035_P1_W_AH(space,1,data & 1);
			break;
		case 5: /* fly */
			I8035_P1_W_AH(space,2,data & 1);
			break;
		case 6: /* coin */
			I8035_P1_W_AH(space,3,data & 1);
			break;
		case 7: /* skid */
			discrete_sound_w(devtag_get_device(space->machine, "discrete"),DS_SOUND7_INP,data & 1);
			break;
	}
}

/*************************************
 *
 *  Sound CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( mario_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROMBANK(1) AM_REGION("audiocpu", 0)
	AM_RANGE(0x0800, 0x0fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mario_sound_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(mario_sh_tune_r) AM_DEVWRITE("discrete", mario_sh_sound_w)
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READWRITE(mario_sh_p1_r, mario_sh_p1_w)
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_READWRITE(mario_sh_p2_r, mario_sh_p2_w)
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_READ(mario_sh_t0_r)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(mario_sh_t1_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( masao_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x4000, 0x4000) AM_DEVREADWRITE("ay", ay8910_r, ay8910_data_w)
	AM_RANGE(0x6000, 0x6000) AM_DEVWRITE("ay", ay8910_address_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Sound Interfaces
 *
 *************************************/

static const ay8910_interface ay8910_config =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_MEMORY_HANDLER("audiocpu", PROGRAM, soundlatch_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


/*************************************
 *
 *  Machine driver
 *
 *************************************/

MACHINE_DRIVER_START( mario_audio )

#if USE_8039
	MDRV_CPU_ADD("audiocpu", I8039, I8035_CLOCK)         /* 730 kHz */
#else
	MDRV_CPU_ADD("audiocpu", M58715, I8035_CLOCK)        /* 730 kHz */
#endif
	MDRV_CPU_PROGRAM_MAP(mario_sound_map)
	MDRV_CPU_IO_MAP(mario_sound_io_map)

	MDRV_SOUND_START(mario)
	MDRV_SOUND_RESET(mario)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("discrete", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(mario)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.5)

MACHINE_DRIVER_END

MACHINE_DRIVER_START( masao_audio )

	MDRV_CPU_ADD("audiocpu", Z80,24576000/16)	/* ???? */
	MDRV_CPU_PROGRAM_MAP(masao_sound_map)

	MDRV_SOUND_START(mario)
	MDRV_SOUND_RESET(mario)

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ay", AY8910, 14318000/6)
	MDRV_SOUND_CONFIG(ay8910_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

MACHINE_DRIVER_END

