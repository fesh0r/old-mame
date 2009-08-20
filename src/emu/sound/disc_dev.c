/************************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@dysfunction.demon.co.uk)
 *
 *  (c) K.Wilkins 2000
 *  (c) D.Renaud 2003-2004
 *
 ************************************************************************
 *
 * DSD_555_ASTBL         - NE555 Simulation - Astable mode
 * DSD_555_MSTBL         - NE555 Simulation - Monostable mode
 * DSD_555_CC            - NE555 Constant Current VCO
 * DSD_555_VCO1          - Op-Amp linear ramp based 555 VCO
 * DSD_566               - NE566 Simulation
 *
 ************************************************************************
 *
 * You will notice that the code for a lot of these routines are similar.
 * I tried to make a common charging routine, but there are too many
 * minor differences that affect each module.
 *
 ************************************************************************/

#define DEFAULT_555_BLEED_R	RES_M(10)

struct dsd_555_astbl_context
{
	int		use_ctrlv;
	int		output_type;
	int		output_is_ac;
	double	ac_shift;			/* DC shift needed to make waveform ac */
	int		flip_flop;			/* 555 flip/flop output state */
	double	cap_voltage;		/* voltage on cap */
	double	threshold;
	double	trigger;
	double	v_out_high;			/* Logic 1 voltage level */
	double	v_charge;
	double *v_charge_node;		/* point to output of node */
	int		has_rc_nodes;
	double	exp_bleed;
	double	exp_charge;
	double	exp_discharge;
	double	t_rc_bleed;
	double	t_rc_charge;
	double	t_rc_discharge;
	double	last_r1;
	double	last_r2;
	double	last_c;
};

struct dsd_555_mstbl_context
{
	int		trig_is_logic;
	int		trig_discharges_cap;
	int		output_type;
	int		output_is_ac;
	double	ac_shift;				/* DC shift needed to make waveform ac */
	int		flip_flop;				/* 555 flip/flop output state */
	double	cap_voltage;			/* voltage on cap */
	double	threshold;
	double	trigger;
	double	v_out_high;				/* Logic 1 voltage level */
	double	v_charge;
};

struct dsd_555_cc_context
{
	unsigned int	type;			/* type of 555cc circuit */
	int				output_type;
	int				output_is_ac;
	double			ac_shift;		/* DC shift needed to make waveform ac */
	int				flip_flop;		/* 555 flip/flop output state */
	double			cap_voltage;	/* voltage on cap */
	double			threshold;
	double			trigger;
	double			v_out_high;		/* Logic 1 voltage level */
	double			v_cc_source;
	int				has_rc_nodes;
	double			exp_bleed;
	double			exp_charge;
	double			exp_discharge;
	double			exp_discharge_01;
	double			exp_discharge_no_i;
	double			t_rc_charge;
	double			t_rc_discharge;
	double			t_rc_discharge_01;
	double			t_rc_discharge_no_i;
};

struct dsd_555_vco1_context
{
	int		ctrlv_is_node;
	int		output_type;
	int		output_is_ac;
	double	ac_shift;			/* DC shift needed to make waveform ac */
	int		flip_flop;			/* flip/flop output state */
	double	v_out_high;			/* 555 high voltage */
	double	threshold;			/* falling threshold */
	double	trigger;			/* rising threshold */
	double	i_charge;			/* charge current */
	double	i_discharge;		/* discharge current */
	double	cap_voltage;		/* current capacitor voltage */
};

struct dsd_566_context
{
	int			error;
	unsigned int state[2];			/* keeps track of excess flip_flop changes during the current step */
	int			flip_flop;			/* 566 flip/flop output state */
	double		cap_voltage;		/* voltage on cap */
	double		v_charge;			/* static charge value */
	double	   *v_charge_node;		/* point to charge node */
	double		v_sqr_low;			/* voltage for a squarewave at low */
	double		v_sqr_high;			/* voltage for a squarewave at high */
	double		threshold_low;		/* falling threshold */
	double		threshold_high;		/* rising threshold */
	double		triangle_ac_offset;	/* used to shift a triangle to AC */
};

struct dsd_ls624_context
{
	int			state;
	double		remain;			/* remaining time from last step */
	int			out_type;
};


/************************************************************************
 *
 * DSD_555_ASTBL -  - 555 Astable simulation
 *
 * input[0]    - Reset value
 * input[1]    - R1 value
 * input[2]    - R2 value
 * input[3]    - C value
 * input[4]    - Control Voltage value
 *
 * also passed discrete_555_desc structure
 *
 * Jan 2004, D Renaud.
 ************************************************************************/
#define DSD_555_ASTBL__RESET	(! *(node->input[0]))
#define DSD_555_ASTBL__R1		(*(node->input[1]))
#define DSD_555_ASTBL__R2		(*(node->input[2]))
#define DSD_555_ASTBL__C		(*(node->input[3]))
#define DSD_555_ASTBL__CTRLV	(*(node->input[4]))

/* bit mask of the above RC inputs */
#define DSD_555_ASTBL_RC_MASK	0x0e

/* charge/discharge constants */
#define DSD_555_ASTBL_T_RC_BLEED		(DEFAULT_555_BLEED_R * DSD_555_ASTBL__C)
/* Use quick charge if specified. */
#define DSD_555_ASTBL_T_RC_CHARGE		((DSD_555_ASTBL__R1 + ((info->options & DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE) ? 0 : DSD_555_ASTBL__R2)) * DSD_555_ASTBL__C)
#define DSD_555_ASTBL_T_RC_DISCHARGE	(DSD_555_ASTBL__R2 * DSD_555_ASTBL__C)

static DISCRETE_STEP(dsd_555_astbl)
{
	const  discrete_555_desc     *info    = (const  discrete_555_desc *)node->custom;
	struct dsd_555_astbl_context *context = (struct dsd_555_astbl_context *)node->context;

	int		count_f = 0;
	int		count_r = 0;
	double	dt;								/* change in time */
	double	x_time  = 0;					/* time since change happened */
	double	v_cap   = context->cap_voltage;	/* Current voltage on capacitor, before dt */
	double	v_cap_next = 0;					/* Voltage on capacitor, after dt */
	double	v_charge, exponent = 0;
	int		update_exponent = 0;

	/* put commonly used stuff in local variables for speed */
	double	threshold = context->threshold;
	double	trigger   = context->trigger;

	if(DSD_555_ASTBL__RESET)
	{
		/* We are in RESET */
		node->output[0]      = 0;
		context->flip_flop   = 1;
		context->cap_voltage = 0;
		return;
	}

	/* Check: if the Control Voltage node is connected. */
	if (context->use_ctrlv)
	{
		/* If CV is less then .25V, the circuit will oscillate way out of range.
         * So we will just ignore it when it happens. */
		if (DSD_555_ASTBL__CTRLV < .25) return;
		/* If it is a node then calculate thresholds based on Control Voltage */
		threshold = DSD_555_ASTBL__CTRLV;
		trigger   = DSD_555_ASTBL__CTRLV / 2.0;
		/* Since the thresholds may have changed we need to update the FF */
		if (v_cap >= threshold)
		{
			context->flip_flop = 0;
			count_f++;
		}
		else
		if (v_cap <= trigger)
		{
			context->flip_flop = 1;
			count_r++;
		}
	}

	/* get the v_charge and update each step if it is a node */
	if (context->v_charge_node != NULL)
	{
		v_charge = *context->v_charge_node;
		if (info->options & DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE) v_charge -= 0.5;
	}
	else
		v_charge = context->v_charge;


	/* Calculate future capacitor voltage.
     * ref@ http://www.physics.rutgers.edu/ugrad/205/capacitance.html
     * The formulas from the ref pages have been modified to reflect that we are stepping the change.
     * dt = time of sample (1/sample frequency)
     * VC = Voltage across capacitor
     * VC' = Future voltage across capacitor
     * Vc = Voltage change
     * Vr = is the voltage across the resistor.  For charging it is Vcc - VC.  Discharging it is VC - 0.
     * R = R1+R2 (for charging)  R = R2 for discharging.
     * Vc = Vr*(1-exp(-dt/(R*C)))
     * VC' = VC + Vc (for charging) VC' = VC - Vc for discharging.
     *
     * We will also need to calculate the amount of time we overshoot the thresholds
     * dt = amount of time we overshot
     * Vc = voltage change overshoot
     * dt = R*C(log(1/(1-(Vc/Vr))))
     */

	dt = disc_info->sample_time;

	/* Sometimes a switching network is used to setup the capacitance.
     * These may select no capacitor, causing oscillation to stop.
     */
	if (DSD_555_ASTBL__C == 0)
	{
		context->flip_flop = 1;
		/* The voltage goes high because the cap circuit is open. */
		v_cap_next = v_charge;
		v_cap      = v_charge;
		context->cap_voltage = 0;
	}
	else
	{
		/* Update charge contstants and exponents if nodes changed */
		if (context->has_rc_nodes && (DSD_555_ASTBL__R1 != context->last_r1 || DSD_555_ASTBL__C != context->last_c || DSD_555_ASTBL__R2 != context->last_r2))
		{
			context->t_rc_bleed  = DSD_555_ASTBL_T_RC_BLEED;
			context->t_rc_charge = DSD_555_ASTBL_T_RC_CHARGE;
			context->t_rc_discharge = DSD_555_ASTBL_T_RC_DISCHARGE;
			context->exp_bleed  = RC_CHARGE_EXP(context->t_rc_bleed);
			context->exp_charge = RC_CHARGE_EXP(context->t_rc_charge);
			context->exp_discharge = RC_CHARGE_EXP(context->t_rc_discharge);
			context->last_r1 = DSD_555_ASTBL__R1;
			context->last_r2 = DSD_555_ASTBL__R2;
			context->last_c  = DSD_555_ASTBL__C;
		}
		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->flip_flop)
			{
				if (DSD_555_ASTBL__R1 == 0)
				{
					/* Oscillation disabled because there is no longer any charge resistor. */
					/* Bleed the cap due to circuit losses. */
					if (update_exponent)
						exponent = RC_CHARGE_EXP_DT(context->t_rc_bleed, dt);
					else
						exponent = context->exp_bleed;
					v_cap_next = v_cap - (v_cap * exponent);
					dt = 0;
				}
				else
				{
					/* Charging */
					if (update_exponent)
						exponent = RC_CHARGE_EXP_DT(context->t_rc_charge, dt);
					else
						exponent = context->exp_charge;
					v_cap_next = v_cap + ((v_charge - v_cap) * exponent);
					dt = 0;

					/* has it charged past upper limit? */
					if (v_cap_next >= threshold)
					{
						/* calculate the overshoot time */
						dt     = context->t_rc_charge  * log(1.0 / (1.0 - ((v_cap_next - threshold) / (v_charge - v_cap))));
						x_time = dt;
						v_cap  = threshold;
						context->flip_flop = 0;
						count_f++;
						update_exponent = 1;
					}
				}
			}
			else
			{
				/* Discharging */
				if(DSD_555_ASTBL__R2 != 0)
				{
					if (update_exponent)
						exponent = RC_CHARGE_EXP_DT(context->t_rc_discharge, dt);
					else
						exponent = context->exp_discharge;
					v_cap_next = v_cap - (v_cap * exponent);
					dt = 0;
				}
				else
				{
					/* no discharge resistor so we imediately discharge */
					v_cap_next = trigger;
				}

				/* has it discharged past lower limit? */
				if (v_cap_next <= trigger)
				{
					/* calculate the overshoot time */
					if (v_cap_next < trigger)
						dt = context->t_rc_discharge  * log(1.0 / (1.0 - ((trigger - v_cap_next) / v_cap)));
					x_time = dt;
					v_cap  = trigger;
					context->flip_flop = 1;
					count_r++;
					update_exponent = 1;
				}
			}
		} while(dt);

		context->cap_voltage = v_cap_next;
	}

	/* Convert last switch time to a ratio */
	x_time = x_time / disc_info->sample_time;

	switch (context->output_type)
	{
		case DISC_555_OUT_SQW:
			node->output[0] = context->flip_flop * context->v_out_high + context->ac_shift;
			break;
		case DISC_555_OUT_CAP:
			node->output[0] = v_cap_next;
			/* Fake it to AC if needed */
			if (context->output_is_ac)
				node->output[0] -= threshold * 3.0 /4.0;
			break;
		case DISC_555_OUT_ENERGY:
			if (x_time == 0) x_time = 1.0;
			node->output[0]  = context->v_out_high * (context->flip_flop ? x_time : (1.0 - x_time));
			node->output[0] += context->ac_shift;
			break;
		case DISC_555_OUT_LOGIC_X:
			node->output[0] = context->flip_flop + x_time;
			break;
		case DISC_555_OUT_COUNT_F_X:
			node->output[0] = count_f ? count_f + x_time : count_f;
			break;
		case DISC_555_OUT_COUNT_R_X:
			node->output[0] =  count_r ? count_r + x_time : count_r;
			break;
		case DISC_555_OUT_COUNT_F:
			node->output[0] = count_f;
			break;
		case DISC_555_OUT_COUNT_R:
			node->output[0] = count_r;
			break;
	}
}

static DISCRETE_RESET(dsd_555_astbl)
{
	const  discrete_555_desc     *info    = (const  discrete_555_desc *)node->custom;
	struct dsd_555_astbl_context *context = (struct dsd_555_astbl_context *)node->context;
	node_description *v_charge_node;

	context->use_ctrlv   = (node->input_is_node >> 4) & 1;
	context->output_type = info->options & DISC_555_OUT_MASK;

	/* Use the defaults or supplied values. */
	context->v_out_high = (info->v_out_high == DEFAULT_555_HIGH) ? info->v_pos - 1.2 : info->v_out_high;

	/* setup v_charge or node */
	v_charge_node = discrete_find_node(disc_info, info->v_charge);
	if (v_charge_node)
		context->v_charge_node = &(v_charge_node->output[NODE_CHILD_NODE_NUM(info->v_charge)]);
	else
	{
		context->v_charge   = (info->v_charge == DEFAULT_555_CHARGE) ? info->v_pos : info->v_charge;
		context->v_charge_node = NULL;

		if (info->options & DISC_555_ASTABLE_HAS_FAST_CHARGE_DIODE) context->v_charge -= 0.5;
	}

	if ((DSD_555_ASTBL__CTRLV != -1) && !context->use_ctrlv)
	{
		/* Setup based on supplied Control Voltage static value */
		context->threshold = DSD_555_ASTBL__CTRLV;
		context->trigger   = DSD_555_ASTBL__CTRLV / 2.0;
	}
	else
	{
		/* Setup based on v_pos power source */
		context->threshold = info->v_pos * 2.0 / 3.0;
		context->trigger   = info->v_pos / 3.0;
	}

	/* optimization if none of the values are nodes */
	context->has_rc_nodes = 0;
	if (node->input_is_node & DSD_555_ASTBL_RC_MASK)
		context->has_rc_nodes = 1;
	else
	{
		context->t_rc_bleed  = DSD_555_ASTBL_T_RC_BLEED;
		context->exp_bleed   = RC_CHARGE_EXP(context->t_rc_bleed);
		context->t_rc_charge = DSD_555_ASTBL_T_RC_CHARGE;
		context->exp_charge  = RC_CHARGE_EXP(context->t_rc_charge);
		context->t_rc_discharge = DSD_555_ASTBL_T_RC_DISCHARGE;
		context->exp_discharge  = RC_CHARGE_EXP(context->t_rc_discharge);
	}

	context->output_is_ac = info->options & DISC_555_OUT_AC;
	/* Calculate DC shift needed to make squarewave waveform AC */
	context->ac_shift = context->output_is_ac ? -context->v_out_high / 2.0 : 0;

	context->flip_flop = 1;
	context->cap_voltage = 0;

	/* Step to set the output */
	DISCRETE_STEP_CALL(dsd_555_astbl);
}


/************************************************************************
 *
 * DSD_555_MSTBL - 555 Monostable simulation
 *
 * input[0]    - Reset value
 * input[1]    - Trigger input
 * input[2]    - R2 value
 * input[3]    - C value
 *
 * also passed discrete_555_desc structure
 *
 * Oct 2004, D Renaud.
 ************************************************************************/
#define DSD_555_MSTBL__RESET	(! *(node->input[0]))
#define DSD_555_MSTBL__TRIGGER	(*(node->input[1]))
#define DSD_555_MSTBL__R		(*(node->input[2]))
#define DSD_555_MSTBL__C		(*(node->input[3]))

static DISCRETE_STEP(dsd_555_mstbl)
{
	const  discrete_555_desc     *info    = (const  discrete_555_desc *)node->custom;
	struct dsd_555_mstbl_context *context = (struct dsd_555_mstbl_context *)node->context;

	double v_cap;			/* Current voltage on capacitor, before dt */
	double v_cap_next = 0;	/* Voltage on capacitor, after dt */

	if(DSD_555_MSTBL__RESET)
	{
		/* We are in RESET */
		node->output[0]     = 0;
		context->flip_flop  = 0;
		context->cap_voltage = 0;
	}
	else
	{
		int trigger;

		if (context->trig_is_logic)
			trigger = !DSD_555_MSTBL__TRIGGER;
		else
			trigger = DSD_555_MSTBL__TRIGGER < context->trigger;

		if (context->trig_discharges_cap && trigger)
			context->cap_voltage = 0;

		if (!context->flip_flop)
		{
			/* Wait for trigger */
			if (trigger)
				context->flip_flop = 1;
		}
		else
		{
			v_cap = context->cap_voltage;

			/* Sometimes a switching network is used to setup the capacitance.
             * These may select 'no' capacitor, causing oscillation to stop.
             */
			if (DSD_555_MSTBL__C == 0)
			{
				context->flip_flop = 0;
				/* The voltage goes high because the cap circuit is open. */
				v_cap_next = info->v_pos;
				v_cap      = info->v_pos;
				context->cap_voltage = 0;
			}
			else
			{
				/* Charging */
				v_cap_next = v_cap + ((info->v_pos - v_cap) * RC_CHARGE_EXP(DSD_555_MSTBL__R * DSD_555_MSTBL__C));

				/* Has it charged past upper limit? */
				/* If trigger is still enabled, then we keep charging,
                 * regardless of threshold. */
				if ((v_cap_next >= context->threshold) && !trigger)
				{
					v_cap_next = 0;
					v_cap      = context->threshold;
					context->flip_flop = 0;
				}
			}

			context->cap_voltage = v_cap_next;

			switch (info->options & DISC_555_OUT_MASK)
			{
				case DISC_555_OUT_SQW:
					node->output[0] = context->flip_flop * context->v_out_high;
					/* Fake it to AC if needed */
					if (context->output_is_ac)
						node->output[0] -= context->v_out_high / 2.0;
					break;
				case DISC_555_OUT_CAP:
					node->output[0] = v_cap_next;
					/* Fake it to AC if needed */
					if (context->output_is_ac)
						node->output[0] -= context->threshold * 3.0 /4.0;
					break;
			}
		}
	}
}

static DISCRETE_RESET(dsd_555_mstbl)
{
	const  discrete_555_desc     *info    = (const  discrete_555_desc *)node->custom;
	struct dsd_555_mstbl_context *context = (struct dsd_555_mstbl_context *)node->context;

	context->output_type = info->options & DISC_555_OUT_MASK;
	if ((context->output_type == DISC_555_OUT_COUNT_F) || (context->output_type == DISC_555_OUT_COUNT_R))
	{
		discrete_log(disc_info, "Invalid Output type in NODE_%d.\n", node->node - NODE_00);
		context->output_type = DISC_555_OUT_SQW;
	}

	/* Use the defaults or supplied values. */
	context->v_out_high = (info->v_out_high == DEFAULT_555_HIGH) ? info->v_pos - 1.2 : info->v_out_high;
	context->v_charge   = (info->v_charge   == DEFAULT_555_CHARGE) ? info->v_pos : info->v_charge;

	/* Setup based on v_pos power source */
	context->threshold = info->v_pos * 2.0 / 3.0;
	context->trigger   = info->v_pos / 3.0;

	context->output_is_ac = info->options & DISC_555_OUT_AC;
	/* Calculate DC shift needed to make squarewave waveform AC */
	context->ac_shift     = context->output_is_ac ? -context->v_out_high / 2.0 : 0;

	context->trig_is_logic       = (info->options & DISC_555_TRIGGER_IS_VOLTAGE) ? 0: 1;
	context->trig_discharges_cap = (info->options & DISC_555_TRIGGER_DISCHARGES_CAP) ? 1: 0;

	context->flip_flop   = 0;
	context->cap_voltage = 0;

	node->output[0] = 0;
}


/************************************************************************
 *
 * DSD_555_CC - Usage of node_description values
 *
 * input[0]    - Reset input value
 * input[1]    - Voltage input for Constant current source.
 * input[2]    - R value to set CC current.
 * input[3]    - C value
 * input[4]    - rBias value
 * input[5]    - rGnd value
 * input[6]    - rDischarge value
 *
 * also passed discrete_555_cc_desc structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DSD_555_CC__RESET	(! *(node->input[0]))
#define DSD_555_CC__VIN		(*(node->input[1]))
#define DSD_555_CC__R		(*(node->input[2]))
#define DSD_555_CC__C		(*(node->input[3]))
#define DSD_555_CC__RBIAS	(*(node->input[4]))
#define DSD_555_CC__RGND	(*(node->input[5]))
#define DSD_555_CC__RDIS	(*(node->input[6]))

/* bit mask of the above RC inputs not including DSD_555_CC__R */
#define DSD_555_CC_RC_MASK	0x78

/* charge/discharge constants */
#define DSD_555_CC_T_RC_BLEED			(DEFAULT_555_BLEED_R * DSD_555_CC__C)
#define DSD_555_CC_T_RC_DISCHARGE_01	(DSD_555_CC__RDIS * DSD_555_CC__C)
#define DSD_555_CC_T_RC_DISCHARGE_NO_I	(DSD_555_CC__RGND * DSD_555_CC__C)
#define DSD_555_CC_T_RC_CHARGE			(r_charge * DSD_555_CC__C)
#define DSD_555_CC_T_RC_DISCHARGE		(r_discharge * DSD_555_CC__C)


static DISCRETE_STEP(dsd_555_cc)
{
	const  discrete_555_cc_desc *info    = (const  discrete_555_cc_desc *)node->custom;
	struct dsd_555_cc_context   *context = (struct dsd_555_cc_context *)node->context;

	int		count_f  = 0;
	int		count_r  = 0;
	double	i;					/* Charging current created by vIn */
	double	r_charge = 0;		/* Equivalent charging resistor */
	double	r_discharge = 0;	/* Equivalent discharging resistor */
	double	vi     = 0;			/* Equivalent voltage from current source */
	double	v_bias = 0;			/* Equivalent voltage from bias voltage */
	double	v      = 0;			/* Equivalent voltage total from current source and bias circuit if used */
	double	dt;					/* change in time */
	double	x_time = 0;			/* time since change happened */
	double	t_rc ;				/* RC time constant */
	double	v_cap;				/* Current voltage on capacitor, before dt */
	double	v_cap_next = 0;		/* Voltage on capacitor, after dt */
	double	v_vcharge_limit;	/* vIn and the junction voltage limit the max charging voltage from i */
	double	r_temp;				/* play thing */
	double	exponent;
	int		update_exponent, update_t_rc;


	if (DSD_555_CC__RESET)
	{
		/* We are in RESET */
		node->output[0]      = 0;
		context->flip_flop   = 1;
		context->cap_voltage = 0;
		return;
	}

	dt    = disc_info->sample_time;	/* Change in time */
	v_cap = context->cap_voltage;	/* Set to voltage before change */
	v_vcharge_limit = DSD_555_CC__VIN + info->v_cc_junction;	/* the max v_cap can be and still be charged by i */
	/* Calculate charging current */
	i = (context->v_cc_source - v_vcharge_limit) / DSD_555_CC__R;
	if ( i < 0) i = 0;

	if (info->options & DISCRETE_555_CC_TO_CAP)
	{
		vi = i * DSD_555_CC__RDIS;
	}
	else
	{
		switch (context->type)	/* see dsd_555_cc_reset for descriptions */
		{
			case 1:
				r_discharge = DSD_555_CC__RDIS;
			case 0:
				break;
			case 3:
				r_discharge = RES_2_PARALLEL(DSD_555_CC__RDIS, DSD_555_CC__RGND);
			case 2:
				r_charge = DSD_555_CC__RGND;
				vi       = i * r_charge;
				break;
			case 4:
				r_charge = DSD_555_CC__RBIAS;
				vi       = i * r_charge;
				v_bias   = info->v_pos;
				break;
			case 5:
				r_charge = DSD_555_CC__RBIAS + DSD_555_CC__RDIS;
				vi      = i * DSD_555_CC__RBIAS;
				v_bias  = info->v_pos;
				r_discharge = DSD_555_CC__RDIS;
				break;
			case 6:
				r_charge = RES_2_PARALLEL(DSD_555_CC__RBIAS, DSD_555_CC__RGND);
				vi      = i * r_charge;
				v_bias  = info->v_pos * RES_VOLTAGE_DIVIDER(DSD_555_CC__RGND, DSD_555_CC__RBIAS);
				break;
			case 7:
				r_temp   = DSD_555_CC__RBIAS + DSD_555_CC__RDIS;
				r_charge = RES_2_PARALLEL(r_temp, DSD_555_CC__RGND);
				r_temp  += DSD_555_CC__RGND;
				r_temp   = DSD_555_CC__RGND / r_temp;	/* now has voltage divider ratio, not resistance */
				vi      = i * DSD_555_CC__RBIAS * r_temp;
				v_bias  = info->v_pos * r_temp;
				r_discharge = RES_2_PARALLEL(DSD_555_CC__RGND, DSD_555_CC__RDIS);
				break;
		}
	}

	/* Keep looping until all toggling in time sample is used up. */
	update_t_rc = context->has_rc_nodes;
	update_exponent = update_t_rc;
	do
	{
		if (context->type <= 1)
		{
			/* Standard constant current charge */
			if (context->flip_flop)
			{
				if (i == 0)
				{
					/* No charging current, so we have to discharge the cap
                     * due to cap and circuit losses.
                     */
					if (update_exponent)
					{
						t_rc     = DSD_555_CC_T_RC_BLEED;
						exponent = RC_CHARGE_EXP_DT(t_rc, dt);
					}
					else
						exponent = context->exp_bleed;
					v_cap_next = v_cap - (v_cap * exponent);
					dt = 0;
				}
				else
				{
					/* Charging */
					/* iC=C*dv/dt  works out to dv=iC*dt/C */
					v_cap_next = v_cap + (i * dt / DSD_555_CC__C);
					/* Yes, if the cap voltage has reached the max voltage it can,
                     * and the 555 threshold has not been reached, then oscillation stops.
                     * This is the way the actual electronics works.
                     * This is why you never play with the pots after being factory adjusted
                     * to work in the proper range. */
					if (v_cap_next > v_vcharge_limit) v_cap_next = v_vcharge_limit;
					dt = 0;

					/* has it charged past upper limit? */
					if (v_cap_next >= context->threshold)
					{
						/* calculate the overshoot time */
						dt     = DSD_555_CC__C * (v_cap_next - context->threshold) / i;
						x_time = dt;
						v_cap  = context->threshold;
						context->flip_flop = 0;
						count_f++;
						update_exponent = 1;
					}
				}
			}
			else if (DSD_555_CC__RDIS != 0)
			{
				/* Discharging */
				if (update_t_rc)
					t_rc = DSD_555_CC_T_RC_DISCHARGE_01;
				else
					t_rc = context->t_rc_discharge_01;
				if (update_exponent)
					exponent = RC_CHARGE_EXP_DT(t_rc, dt);
				else
					exponent = context->exp_discharge_01;

				if (info->options & DISCRETE_555_CC_TO_CAP)
				{
					/* Asteroids - Special Case */
					/* Charging in discharge mode */
					/* If the cap voltage is past the current source charging limit
                     * then only the bias voltage will charge the cap. */
					v          = (v_cap < v_vcharge_limit) ? vi : v_vcharge_limit;
					v_cap_next = v_cap + ((v - v_cap) * exponent);
				}
				else
				{
					v_cap_next = v_cap - (v_cap * exponent);
				}

				dt = 0;
				/* has it discharged past lower limit? */
				if (v_cap_next <= context->trigger)
				{
					dt     = t_rc  * log(1.0 / (1.0 - ((context->trigger - v_cap_next) / v_cap)));
					x_time = dt;
					v_cap  = context->trigger;
					context->flip_flop = 1;
					count_r++;
					update_exponent = 1;
				}
			}
			else	/* Immediate discharge. No change in dt. */
			{
				x_time = dt;
				v_cap  = context->trigger;
				context->flip_flop = 1;
				count_r++;
			}
		}
		else
		{
			/* The constant current gets changed to a voltage due to a load resistor. */
			if (context->flip_flop)
			{
				if ((i == 0) && (DSD_555_CC__RBIAS == 0))
				{
					/* No charging current, so we have to discharge the cap
                     * due to rGnd.
                     */
					if (update_t_rc)
						t_rc = DSD_555_CC_T_RC_DISCHARGE_NO_I;
					else
						t_rc = context->t_rc_discharge_no_i;
					if (update_exponent)
						exponent = RC_CHARGE_EXP_DT(t_rc, dt);
					else
						exponent = context->exp_discharge_no_i;

					v_cap_next = v_cap - (v_cap * exponent);
					dt = 0;
				}
				else
				{
					/* Charging */
					/* If the cap voltage is past the current source charging limit
                     * then only the bias voltage will charge the cap. */
					v = v_bias;
					if (v_cap < v_vcharge_limit) v += vi;
					else if (context->type <= 3) v = v_vcharge_limit;

					if (update_t_rc)
						t_rc = DSD_555_CC_T_RC_CHARGE;
					else
						t_rc = context->t_rc_charge;
					if (update_exponent)
						exponent = RC_CHARGE_EXP_DT(t_rc, dt);
					else
						exponent = context->exp_charge;

					v_cap_next = v_cap + ((v - v_cap) * exponent);
					dt         = 0;

					/* has it charged past upper limit? */
					if (v_cap_next >= context->threshold)
					{
						/* calculate the overshoot time */
						dt     = t_rc  * log(1.0 / (1.0 - ((v_cap_next - context->threshold) / (v - v_cap))));
						x_time = dt;
						v_cap  = context->threshold;
						context->flip_flop = 0;
						count_f++;
						update_exponent = 1;
					}
				}
			}
			else /* Discharging */
			if (r_discharge)
			{
				if (update_t_rc)
					t_rc = DSD_555_CC_T_RC_DISCHARGE;
				else
					t_rc = context->t_rc_discharge;
				if (update_exponent)
					exponent = RC_CHARGE_EXP_DT(t_rc, dt);
				else
					exponent = context->exp_discharge;

				v_cap_next = v_cap - (v_cap * exponent);
				dt = 0;

				/* has it discharged past lower limit? */
				if (v_cap_next <= context->trigger)
				{
					/* calculate the overshoot time */
					dt     = t_rc  * log(1.0 / (1.0 - ((context->trigger - v_cap_next) / v_cap)));
					x_time = dt;
					v_cap  = context->trigger;
					context->flip_flop = 1;
					count_r++;
					update_exponent = 1;
				}
			}
			else	/* Immediate discharge. No change in dt. */
			{
				x_time = dt;
				v_cap  = context->trigger;
				context->flip_flop = 1;
				count_r++;
			}
		}
	} while(dt);

	context->cap_voltage = v_cap_next;

	/* Convert last switch time to a ratio */
	x_time = x_time / disc_info->sample_time;

	switch (context->output_type)
	{
		case DISC_555_OUT_SQW:
			if (count_r && (~context->type & 0x01))
			{
				/* There has been an immediate discharge, so keep low for 1 sample. */
				node->output[0] = 0;
			}
			else
				node->output[0] = context->flip_flop * context->v_out_high;
			/* Fake it to AC if needed */
			node->output[0] += context->ac_shift;
			break;
		case DISC_555_OUT_CAP:
			node->output[0] = v_cap_next + context->ac_shift;
			break;
		case DISC_555_OUT_ENERGY:
			if (x_time == 0) x_time = 1.0;
			node->output[0]  = context->v_out_high * (context->flip_flop ? x_time : (1.0 - x_time));
			node->output[0] += context->ac_shift;
			break;
		case DISC_555_OUT_LOGIC_X:
			node->output[0] = context->flip_flop + x_time;
			break;
		case DISC_555_OUT_COUNT_F_X:
			node->output[0] = count_f ? count_f + x_time : count_f;
			break;
		case DISC_555_OUT_COUNT_R_X:
			node->output[0] =  count_r ? count_r + x_time : count_r;
			break;
		case DISC_555_OUT_COUNT_F:
			node->output[0] = count_f;
			break;
		case DISC_555_OUT_COUNT_R:
			node->output[0] = count_r;
			break;
	}
}

static DISCRETE_RESET(dsd_555_cc)
{
	const  discrete_555_cc_desc *info    = (const  discrete_555_cc_desc *)node->custom;
	struct dsd_555_cc_context   *context = (struct dsd_555_cc_context *)node->context;

	double	r_temp, r_discharge = 0, r_charge = 0;

	context->flip_flop   = 1;
	context->cap_voltage = 0;

	context->output_type = info->options & DISC_555_OUT_MASK;

	/* Use the defaults or supplied values. */
	context->v_out_high  = (info->v_out_high  == DEFAULT_555_HIGH) ? info->v_pos - 1.2 : info->v_out_high;
	context->v_cc_source = (info->v_cc_source == DEFAULT_555_CC_SOURCE) ? info->v_pos : info->v_cc_source;

	/* Setup based on v_pos power source */
	context->threshold = info->v_pos * 2.0 / 3.0;
	context->trigger   = info->v_pos / 3.0;

	context->output_is_ac = info->options & DISC_555_OUT_AC;
	/* Calculate DC shift needed to make squarewave waveform AC */
	context->ac_shift     = context->output_is_ac ? -context->v_out_high / 2.0 : 0;

	/* There are 8 different types of basic oscillators
     * depending on the resistors used.  We will determine
     * the type of circuit at reset, because the ciruit type
     * is constant.  See Below.
     */
	context->type = (DSD_555_CC__RDIS > 0) | ((DSD_555_CC__RGND  > 0) << 1) | ((DSD_555_CC__RBIAS  > 0) << 2);

	/* optimization if none of the values are nodes */
	context->has_rc_nodes = 0;
	if (node->input_is_node & DSD_555_CC_RC_MASK)
		context->has_rc_nodes = 1;
	else
	{
		switch (context->type)	/* see dsd_555_cc_reset for descriptions */
		{
			case 1:
				r_discharge = DSD_555_CC__RDIS;
			case 0:
				break;
			case 3:
				r_discharge = RES_2_PARALLEL(DSD_555_CC__RDIS, DSD_555_CC__RGND);
			case 2:
				r_charge = DSD_555_CC__RGND;
				break;
			case 4:
				r_charge = DSD_555_CC__RBIAS;
				break;
			case 5:
				r_charge = DSD_555_CC__RBIAS + DSD_555_CC__RDIS;
				r_discharge = DSD_555_CC__RDIS;
				break;
			case 6:
				r_charge = RES_2_PARALLEL(DSD_555_CC__RBIAS, DSD_555_CC__RGND);
				break;
			case 7:
				r_temp   = DSD_555_CC__RBIAS + DSD_555_CC__RDIS;
				r_charge = RES_2_PARALLEL(r_temp, DSD_555_CC__RGND);
				r_discharge = RES_2_PARALLEL(DSD_555_CC__RGND, DSD_555_CC__RDIS);
				break;
		}

		context->exp_bleed  = RC_CHARGE_EXP(DSD_555_CC_T_RC_BLEED);
		context->t_rc_discharge_01 = DSD_555_CC_T_RC_DISCHARGE_01;
		context->exp_discharge_01  = RC_CHARGE_EXP(context->t_rc_discharge_01);
		context->t_rc_discharge_no_i = DSD_555_CC_T_RC_DISCHARGE_NO_I;
		context->exp_discharge_no_i  = RC_CHARGE_EXP(context->t_rc_discharge_no_i);
		context->t_rc_charge = DSD_555_CC_T_RC_CHARGE;
		context->exp_charge  = RC_CHARGE_EXP(context->t_rc_charge);
		context->t_rc_discharge = DSD_555_CC_T_RC_DISCHARGE;
		context->exp_discharge  = RC_CHARGE_EXP(context->t_rc_discharge);
	}

	/* Step to set the output */
	DISCRETE_STEP_CALL(dsd_555_cc);

	/*
     * TYPES:
     * Note: These are equivalent circuits shown without the 555 circuitry.
     *       See the schematic in src\sound\discrete.h for full hookup info.
     *
     * DISCRETE_555_CC_TO_DISCHARGE_PIN
     * When the CC source is connected to the discharge pin, it allows the
     * circuit to charge when the 555 is in charge mode.  But when in discharge
     * mode, the CC source is grounded, disabling it's effect.
     *
     * [0]
     * No resistors.  Straight constant current charge of capacitor.
     * When there is not any charge current, the cap will bleed off.
     * Once the lower threshold(trigger) is reached, the output will
     * go high but the cap will continue to discharge due to losses.
     *   .------+---> cap_voltage      CHARGING:
     *   |      |                 dv (change in voltage) compared to dt (change in time in seconds).
     * .---.   ---                dv = i * dt / C; where i is current in amps and C is capacitance in farads.
     * | i |   --- C              cap_voltage = cap_voltage + dv
     * '---'    |
     *   |      |               DISCHARGING:
     *  gnd    gnd                instantaneous
     *
     * [1]
     * Same as type 1 but with rDischarge.  rDischarge has no effect on the charge rate because
     * of the constant current source i.
     * When there is not any charge current, the cap will bleed off.
     * Once the lower threshold(trigger) is reached, the output will
     * go high but the cap will continue to discharge due to losses.
     *   .----ZZZ-----+---> cap_voltage      CHARGING:
     *   | rDischarge |                 dv (change in voltage) compared to dt (change in time in seconds).
     * .---.         ---                dv = i * dt / C; where i is current in amps and C is capacitance in farads.
     * | i |         --- C              cap_voltage = cap_voltage + dv
     * '---'          |
     *   |            |               DISCHARGING:
     *  gnd          gnd                thru rDischarge
     *
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     * !!!!! IMPORTANT NOTE ABOUT TYPES 3 - 7 !!!!!
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     *
     * From here on in all the circuits have either an rBias or rGnd resistor.
     * This converts the constant current into a voltage source.
     * So all the remaining circuit types will be converted to this circuit.
     * When discharging, rBias is out of the equation because the 555 is grounding the circuit
     * after that point.
     *
     * .------------.     Rc                  Rc is the equivilent circuit resistance.
     * |     v      |----ZZZZ---+---> cap_voltage    v  is the equivilent circuit voltage.
     * |            |           |
     * '------------'          ---            Then the standard RC charging formula applies.
     *       |                 --- C
     *       |                  |             NOTE: All the following types are converted to Rc and v values.
     *      gnd                gnd
     *
     * [2]
     * When there is not any charge current, the cap will bleed off.
     * Once the lower threshold(trigger) is reached, the output will
     * go high but the cap will continue to discharge due to rGnd.
     *   .-------+------+------> cap_voltage         CHARGING:
     *   |       |      |                       v = vi = i * rGnd
     * .---.    ---     Z                       Rc = rGnd
     * | i |    --- C   Z rGnd
     * '---'     |      |                     DISCHARGING:
     *   |       |      |                       instantaneous
     *  gnd     gnd    gnd
     *
     * [3]
     * When there is not any charge current, the cap will bleed off.
     * Once the lower threshold(trigger) is reached, the output will
     * go high but the cap will continue to discharge due to rGnd.
     *   .----ZZZ-----+------+------> cap_voltage    CHARGING:
     *   | rDischarge |      |                  v = vi = i * rGnd
     * .---.         ---     Z                  Rc = rGnd
     * | i |         --- C   Z rGnd
     * '---'          |      |                DISCHARGING:
     *   |            |      |                  thru rDischarge || rGnd  ( || means in parallel)
     *  gnd          gnd    gnd
     *
     * [4]
     *     .---ZZZ---+------------+-------------> cap_voltage      CHARGING:
     *     |  rBias  |            |                           Rc = rBias
     * .-------.   .---.         ---                          vi = i * rBias
     * | vBias |   | i |         --- C                        v = vBias + vi
     * '-------'   '---'          |
     *     |         |            |                         DISCHARGING:
     *    gnd       gnd          gnd                          instantaneous
     *
     * [5]
     *     .---ZZZ---+----ZZZ-----+-------------> cap_voltage      CHARGING:
     *     |  rBias  | rDischarge |                           Rc = rBias + rDischarge
     * .-------.   .---.         ---                          vi = i * rBias
     * | vBias |   | i |         --- C                        v = vBias + vi
     * '-------'   '---'          |
     *     |         |            |                         DISCHARGING:
     *    gnd       gnd          gnd                          thru rDischarge
     *
     * [6]
     *     .---ZZZ---+------------+------+------> cap_voltage      CHARGING:
     *     |  rBias  |            |      |                    Rc = rBias || rGnd
     * .-------.   .---.         ---     Z                    vi = i * Rc
     * | vBias |   | i |         --- C   Z rGnd               v = vBias * (rGnd / (rBias + rGnd)) + vi
     * '-------'   '---'          |      |
     *     |         |            |      |                  DISCHARGING:
     *    gnd       gnd          gnd    gnd                   instantaneous
     *
     * [7]
     *     .---ZZZ---+----ZZZ-----+------+------> cap_voltage      CHARGING:
     *     |  rBias  | rDischarge |      |                    Rc = (rBias + rDischarge) || rGnd
     * .-------.   .---.         ---     Z                    vi = i * rBias * (rGnd / (rBias + rDischarge + rGnd))
     * | vBias |   | i |         --- C   Z rGnd               v = vBias * (rGnd / (rBias + rDischarge + rGnd)) + vi
     * '-------'   '---'          |      |
     *     |         |            |      |                  DISCHARGING:
     *    gnd       gnd          gnd    gnd                   thru rDischarge || rGnd
     */

    /*
     * DISCRETE_555_CC_TO_CAP
     *
     * When the CC source is connected to the capacitor, it allows the
     * current to charge the cap while it is in discharge mode, slowing the
     * discharge.  So in charge mode it charges linearly from the constant
     * current cource.  But when in discharge mode it behaves like circuit
     * type 2 above.
     *   .-------+------+------> cap_voltage         CHARGING:
     *   |       |      |                       dv = i * dt / C
     * .---.    ---     Z                       cap_voltage = cap_voltage + dv
     * | i |    --- C   Z rDischarge
     * '---'     |      |                     DISCHARGING:
     *   |       |      |                       v = vi = i * rGnd
     *  gnd     gnd   discharge                 Rc = rDischarge
     */
}


/************************************************************************
 *
 * DSD_555_VCO1 - Usage of node_description values
 *
 * input[0]    - Reset input value
 * input[1]    - Modulation Voltage (Vin1)
 * input[2]    - Control Voltage (Vin2)
 *
 * also passed discrete_5555_vco1_desc structure
 *
 * Apr 2006, D Renaud.
 ************************************************************************/
#define DSD_555_VCO1__RESET	(*(node->input[0]))	/* reset active low */
#define DSD_555_VCO1__VIN1	(*(node->input[1]))
#define DSD_555_VCO1__VIN2	(*(node->input[2]))

static DISCRETE_STEP(dsd_555_vco1)
{
	const  discrete_555_vco1_desc *info    = (const  discrete_555_vco1_desc *)node->custom;
	struct dsd_555_vco1_context   *context = (struct dsd_555_vco1_context *)node->context;

	int		count_f = 0;
	int		count_r = 0;
	double	dt;				/* change in time */
	double	x_time  = 0;	/* time since change happened */
	double	v_cap;			/* Current voltage on capacitor, before dt */
	double	v_cap_next = 0;	/* Voltage on capacitor, after dt */

	dt    = disc_info->sample_time;	/* Change in time */
	v_cap = context->cap_voltage;

	/* Check: if the Control Voltage node is connected. */
	if (context->ctrlv_is_node && DSD_555_VCO1__RESET)	/* reset active low */
	{
		/* If CV is less then .25V, the circuit will oscillate way out of range.
         * So we will just ignore it when it happens. */
		if (DSD_555_VCO1__VIN2 < .25) return;
		/* If it is a node then calculate thresholds based on Control Voltage */
		context->threshold = DSD_555_VCO1__VIN2;
		context->trigger   = DSD_555_VCO1__VIN2 / 2.0;
		/* Since the thresholds may have changed we need to update the FF */
		if (v_cap >= context->threshold)
		{
			x_time = dt;
			context->flip_flop = 0;
			count_f++;
		}
		else
		if (v_cap <= context->trigger)
		{
			x_time = dt;
			context->flip_flop = 1;
			count_r++;
		}
	}

	/* Keep looping until all toggling in time sample is used up. */
	do
	{
		if (context->flip_flop)
		{
			/* if we are in reset then toggle f/f and discharge */
			if (!DSD_555_VCO1__RESET)	/* reset active low */
			{
				context->flip_flop = 0;
				count_f++;
			}
			else
			{
				/* Charging */
				/* iC=C*dv/dt  works out to dv=iC*dt/C */
				v_cap_next = v_cap + (context->i_charge * dt / info->c);
				dt         = 0;

				/* has it charged past upper limit? */
				if (v_cap_next >= context->threshold)
				{
					/* calculate the overshoot time */
					dt     = info->c * (v_cap_next - context->threshold) / context->i_charge;
					v_cap  = context->threshold;
					x_time = dt;
					context->flip_flop = 0;
					count_f++;
				}
			}
		}
		else
		{
			/* Discharging */
			/* iC=C*dv/dt  works out to dv=iC*dt/C */
			v_cap_next = v_cap - (context->i_discharge * dt / info->c);

			/* if we are in reset, then the cap can discharge to 0 */
			if (!DSD_555_VCO1__RESET)	/* reset active low */
			{
				if (v_cap_next < 0) v_cap_next = 0;
				dt = 0;
			}
			else
			{
				/* if we are out of reset and the cap voltage is less then
                 * the lower threshold, toggle f/f and start charging */
				if (v_cap <= context->trigger)
				{
					if (context->flip_flop == 0)
					{
						/* don't need to track x_time here */
						context->flip_flop = 1;
						count_r++;
					}
				}
				else
				{
					dt = 0;
					/* has it discharged past lower limit? */
					if (v_cap_next <= context->trigger)
					{
						/* calculate the overshoot time */
						dt     = info->c * (v_cap_next - context->trigger) / context->i_discharge;
						v_cap  = context->trigger;
						x_time = dt;
						context->flip_flop = 1;
						count_r++;
					}
				}
			}
		}
	} while(dt);

	context->cap_voltage = v_cap_next;

	/* Convert last switch time to a ratio.  No x_time in reset. */
	x_time = x_time / disc_info->sample_time;
	if (!DSD_555_VCO1__RESET) x_time = 0;

	switch (context->output_type)
	{
		case DISC_555_OUT_SQW:
			node->output[0] = context->flip_flop * context->v_out_high + context->ac_shift;
			break;
		case DISC_555_OUT_CAP:
			node->output[0] = v_cap_next;
			/* Fake it to AC if needed */
			if (context->output_is_ac)
				node->output[0] -= context->threshold * 3.0 /4.0;
			break;
		case DISC_555_OUT_ENERGY:
			if (x_time == 0) x_time = 1.0;
			node->output[0]  = context->v_out_high * (context->flip_flop ? x_time : (1.0 - x_time));
			node->output[0] += context->ac_shift;
			break;
		case DISC_555_OUT_LOGIC_X:
			node->output[0] = context->flip_flop + x_time;
			break;
		case DISC_555_OUT_COUNT_F_X:
			node->output[0] = count_f ? count_f + x_time : count_f;
			break;
		case DISC_555_OUT_COUNT_R_X:
			node->output[0] =  count_r ? count_r + x_time : count_r;
			break;
		case DISC_555_OUT_COUNT_F:
			node->output[0] = count_f;
			break;
		case DISC_555_OUT_COUNT_R:
			node->output[0] = count_r;
			break;
	}
}

static DISCRETE_RESET(dsd_555_vco1)
{
	const  discrete_555_vco1_desc *info    = (const  discrete_555_vco1_desc *)node->custom;
	struct dsd_555_vco1_context   *context = (struct dsd_555_vco1_context *)node->context;

	double v_ratio_r3, v_ratio_r4_1, r_in_1;

	context->output_type  = info->options & DISC_555_OUT_MASK;
	context->output_is_ac = info->options & DISC_555_OUT_AC;

	/* Setup op-amp parameters */

	/* The voltage at op-amp +in is always a fixed ratio of the modulation voltage. */
	v_ratio_r3 = info->r3 / (info->r2 + info->r3);			/* +in voltage */
	/* The voltage at op-amp -in is 1 of 2 fixed ratios of the modulation voltage,
     * based on the 555 Flip-Flop state. */
	/* If the FF is 0, then only R1 is connected allowing the full modulation volatge to pass. */
	/* v_ratio_r4_0 = 1 */
	/* If the FF is 1, then R1 & R4 make a voltage divider similar to R2 & R3 */
	v_ratio_r4_1 = info->r4 / (info->r1 + info->r4);		/* -in voltage */
	/* the input resistance to the op amp depends on the FF state */
	/* r_in_0 = info->r1 when FF = 0 */
	r_in_1 = 1.0 / (1.0 / info->r1 + 1.0 / info->r4);	/* input resistance when r4 switched in */

	/* Now that we know the voltages entering the op amp and the resistance for the
     * FF states, we can predetermine the ratios for the charge/discharge currents. */
	context->i_discharge = (1 - v_ratio_r3) / info->r1;
	context->i_charge    = (v_ratio_r3 - v_ratio_r4_1) / r_in_1;

	/* the cap starts off discharged */
	context->cap_voltage = 0;

	/* Setup 555 parameters */

	/* There is no charge on the cap so the 555 goes high at init. */
	context->flip_flop     = 1;
	context->ctrlv_is_node = (node->input_is_node >> 2) & 1;
	context->v_out_high    = (info->v_out_high == DEFAULT_555_HIGH) ? info->v_pos - 1.2 : info->v_out_high;

	/* Calculate 555 thresholds.
     * If the Control Voltage is a node, then the thresholds will be calculated each step.
     * If the Control Voltage is a fixed voltage, then the thresholds will be calculated
     * from that.  Otherwise we will use thresholds based on v_pos. */
	if (!context->ctrlv_is_node && (DSD_555_VCO1__VIN2 != -1))
	{
		/* Setup based on supplied Control Voltage static value */
		context->threshold = DSD_555_VCO1__VIN2;
		context->trigger   = DSD_555_VCO1__VIN2 / 2.0;
	}
	else
	{
		/* Setup based on v_pos power source */
		context->threshold = info->v_pos * 2.0 / 3.0;
		context->trigger   = info->v_pos / 3.0;
	}

	/* Calculate DC shift needed to make squarewave waveform AC */
	context->ac_shift = context->output_is_ac ? -context->v_out_high / 2.0 : 0;
}

/************************************************************************
 *
 * DSD_566 - Usage of node_description values
 *
 * input[0]    - Enable input value
 * input[1]    - Modulation Voltage
 * input[2]    - R value
 * input[3]    - C value
 *
 * also passed discrete_566_desc structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DSD_566__ENABLE	(*(node->input[0]))
#define DSD_566__VMOD	(*(node->input[1]))
#define DSD_566__R		(*(node->input[2]))
#define DSD_566__C		(*(node->input[3]))

static DISCRETE_STEP(dsd_566)
{
	const  discrete_566_desc *info    = (const  discrete_566_desc *)node->custom;
	struct dsd_566_context   *context = (struct dsd_566_context *)node->context;

	double i;				/* Charging current created by vIn */
	double dt;				/* change in time */
	double v_cap;			/* Current voltage on capacitor, before dt */
	double v_cap_next = 0;	/* Voltage on capacitor, after dt */
	double v_charge;

	if (DSD_566__ENABLE && !context->error)
	{
		dt    = disc_info->sample_time;	/* Change in time */
		v_cap = context->cap_voltage;	/* Set to voltage before change */

		/* get the v_charge and update each step if it is a node */
		if (context->v_charge_node != NULL)
		{
			v_charge  = *context->v_charge_node;
			v_charge -= info->v_neg;
		}
		else
			v_charge = context->v_charge;

		/* Calculate charging current */
		i = (v_charge - DSD_566__VMOD) / DSD_566__R;

		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->flip_flop)
			{
				/* Discharging */
				v_cap_next = v_cap - (i * dt / DSD_566__C);
				dt         = 0;

				/* has it discharged past lower limit? */
				if (v_cap_next <= context->threshold_low)
				{
					if (v_cap_next < context->threshold_low)
					{
						/* calculate the overshoot time */
						dt = DSD_566__C * (context->threshold_low - v_cap_next) / i;
					}
					v_cap = context->threshold_low;
					context->flip_flop = 0;
					/*
                     * If the sampling rate is too low and the desired frequency is too high
                     * then we will start getting too many outputs that can't catch up.  We will
                     * limit this to 3.  The output is already incorrect because of the low sampling,
                     * but at least this way it can recover.
                     */
					context->state[0] = (context->state[0] + 1) & 0x03;
				}
			}
			else
			{
				/* Charging */
				/* iC=C*dv/dt  works out to dv=iC*dt/C */
				v_cap_next = v_cap + (i * dt / DSD_566__C);
				dt         = 0;
				/* Yes, if the cap voltage has reached the max voltage it can,
                 * and the 566 threshold has not been reached, then oscillation stops.
                 * This is the way the actual electronics works.
                 * This is why you never play with the pots after being factory adjusted
                 * to work in the proper range. */
				if (v_cap_next > DSD_566__VMOD) v_cap_next = DSD_566__VMOD;

				/* has it charged past upper limit? */
				if (v_cap_next >= context->threshold_high)
				{
					if (v_cap_next > context->threshold_high)
					{
						/* calculate the overshoot time */
						dt = DSD_566__C * (v_cap_next - context->threshold_high) / i;
					}
					v_cap = context->threshold_high;
					context->flip_flop = 1;
					context->state[1] = (context->state[1] + 1) & 0x03;
				}
			}
		} while(dt);

		context->cap_voltage = v_cap_next;

		switch (info->options & DISC_566_OUT_MASK)
		{
			case DISC_566_OUT_SQUARE:
			case DISC_566_OUT_LOGIC:
				/* use up any output states */
				if (node->output[0] && context->state[0])
				{
					node->output[0] = 0;
					context->state[0]--;
				}
				else if (!node->output[0] && context->state[1])
				{
					node->output[0] = 1;
					context->state[1]--;
				}
				else
				{
					node->output[0] = context->flip_flop;
				}
				if ((info->options & DISC_566_OUT_MASK) != DISC_566_OUT_LOGIC)
					node->output[0] = context->flip_flop ? context->v_sqr_high : context->v_sqr_low;
				break;
			case DISC_566_OUT_TRIANGLE:
				/* we can ignore any unused states when
                 * outputting the cap voltage */
				node->output[0] = v_cap_next;
				if (info->options & DISC_566_OUT_AC)
					node->output[0] -= context->triangle_ac_offset;
				break;
		}
	}
	else
		node->output[0] = 0;
}

static DISCRETE_RESET(dsd_566)
{
	const  discrete_566_desc *info    = (const  discrete_566_desc *)node->custom;
	struct dsd_566_context   *context = (struct dsd_566_context *)node->context;
	node_description *v_charge_node;

	double	v_diff, temp;

	context->error = 0;
	if (info->v_neg >= info->v_pos)
	{
		logerror("[v_neg >= v_pos] - NODE_%d DISABLED!\n", node->node - NODE_00);
		context->error = 1;
		return;
	}

	/* setup v_charge or node */
	v_charge_node = discrete_find_node(disc_info, info->v_charge);
	if (v_charge_node)
		context->v_charge_node = &(v_charge_node->output[NODE_CHILD_NODE_NUM(info->v_charge)]);
	else
	{
		context->v_charge  = (info->v_charge == DEFAULT_566_CHARGE) ? info->v_pos : info->v_charge;
		context->v_charge -= info->v_neg;
		context->v_charge_node = NULL;
	}

	v_diff = info->v_pos - info->v_neg;
	context->flip_flop   = 0;
	context->cap_voltage = 0;
	context->state[0]    = 0;
	context->state[1]    = 0;

	/* The data sheets are useless for this IC.  I will have to get my hands on a chip
     * to make real measurements.  For now this should work fine for 12V. */
	context->threshold_high = v_diff / 2 + info->v_neg;
	context->threshold_low  = context->threshold_high - (0.2 * v_diff);
	context->v_sqr_high     = info->v_pos - 0.6;
	context->v_sqr_low      = context->threshold_high;

	if (info->options & DISC_566_OUT_AC)
	{
		temp = (context->v_sqr_high - context->v_sqr_low) / 2;
		context->v_sqr_high =  temp;
		context->v_sqr_low  = -temp;
		context->triangle_ac_offset = context->threshold_high - (0.1 * v_diff);
	}

	/* Step the output */
	DISCRETE_STEP_CALL(dsd_566);
}

/************************************************************************
 *
 * DSD_LS624 - Usage of node_description values
 *
 * input[0]    - Enable input value
 * input[1]    - Modulation Voltage
 * input[2]    - Range Voltage
 * input[3]    - C value
 * input[4]    - Output type
 *
 * Dec 2007, Couriersud
 ************************************************************************/
#define DSD_LS624__ENABLE	(*(node->input[0]))
#define DSD_LS624__VMOD		(*(node->input[1]))
#define DSD_LS624__VRNG		(*(node->input[2]))
#define DSD_LS624__C		(*(node->input[3]))
#define DSD_LS624__OUTTYPE	(*(node->input[4]))

/*
 * The datasheet mentions a 600 ohm discharge. It also gives
 * equivalent circuits for VI and VR.
 */

#define LS624_F1(x)			(0.19 + 20.0/90.0*(x))
#define LS624_T(_C, _R, _F)		((-600.0 * (_C) * log(1.0-LS624_F1(_R)*0.12/LS624_F1(_F))) * 16.0 )

/* The following formula was derived from figures 2 and 3 in LS624 datasheet. Coefficients
 * where calculated using least square approximation.
 * This approach gives a bit better results compared to the first approach.
 */
#define LS624_F(_C, _VI, _VR)	pow(10, -0.912029404 * log10(_C) + 0.243264328 * (_VI) \
		          - 0.091695877 * (_VR) -0.014110946 * (_VI) * (_VR) - 3.207072925)

static DISCRETE_STEP(dsd_ls624)
{
	struct dsd_ls624_context *context = (struct dsd_ls624_context *)node->context;

	if (DSD_LS624__ENABLE)
	{
		double	dt;	/* change in time */
		double	sample_t;
		double	t;
		double  en = 0.0f;
		int		cntf = 0, cntr = 0;

		sample_t = disc_info->sample_time;	/* Change in time */
		//dt  = LS624_T(DSD_LS624__C, DSD_LS624__VRNG, DSD_LS624__VMOD) / 2.0;
		dt  = 1.0f / (2.0f * LS624_F(DSD_LS624__C, DSD_LS624__VMOD, DSD_LS624__VRNG));
		t   = context->remain;
		en += (double) context->state * t;
		while (t + dt <= sample_t)
		{
			en += (double) context->state * dt;
			context->state = (1 - context->state);
			if (context->state)
				cntr++;
			else
				cntf++;
			t += dt;
		}
		en += (sample_t - t) * (double) context->state;
		context->remain = t - sample_t;

		switch (context->out_type)
		{
			case DISC_LS624_OUT_ENERGY:
				node->output[0] = en / sample_t;
				break;
			case DISC_LS624_OUT_LOGIC:
				/* filter out randomness */
				if (cntf + cntr > 1)
					node->output[0] = 1;
				else
					node->output[0] = context->state;
				break;
			case DISC_LS624_OUT_COUNT_F:
				node->output[0] = cntf;
				break;
			case DISC_LS624_OUT_COUNT_R:
				node->output[0] = cntr;
				break;
		}

	}
	else
		node->output[0] = 0;
}

static DISCRETE_RESET(dsd_ls624)
{
	struct dsd_ls624_context *context = (struct dsd_ls624_context *)node->context;

	context->remain   = 0;
	context->state    = 0;
	context->out_type = DSD_LS624__OUTTYPE;

	/* Step the output */
	DISCRETE_STEP_CALL(dsd_ls624);
}
