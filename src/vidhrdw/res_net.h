#ifndef _res_net_h_
#define _res_net_h_

/*****************************************************************************

 Compute weights for resistors networks.

 Function can evaluate from one to three networks at a time.

 The output weights can be either scaled with automatically calculated scaler
 or scaled with 'scaler' provided on entry.

 On entry
 --------

 'minval','maxval' specify range of output signals (sum of weights).
 'scaler'        if negative, function will calculate proper scaler,
                 otherwise it will use the one provided here.
 'count_x'       is a number of resistors in this network
 'resistances_x' is a pointer to a table containing the resistances
 'weights_x'     is a pointer to a table to be filled with weights
                 (can contain negative values if 'minval' is below zero).
 'pulldown_x'    is a resistance of a pulldown resistor (0 means no pulldown resistor)
 'pullup_x'      is a resistance of a pullup resistor (0 means no pullup resistor)


 Return value
 ------------

 Value of the scaler that was used to fit the output within the expected range.
 Note that if you provide your own scaler on entry, it will be returned here.

 All resistances expected in Ohms.


 Hint
 ----

 If there is no need to calculate three networks at a time, just specify '0'
 for the 'count_x' for unused network.

*****************************************************************************/

static double compute_resistor_weights(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * weights_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * weights_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * weights_3, int pulldown_3, int pullup_3 );

#define combine_8_weights(tab,w0,w1,w2,w3,w4,w5,w6,w7)	((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5 + tab[6]*w6 + tab[7]*w7) + 0.5))
#define combine_7_weights(tab,w0,w1,w2,w3,w4,w5,w6)		((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5 + tab[6]*w6) + 0.5))
#define combine_6_weights(tab,w0,w1,w2,w3,w4,w5)		((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4 + tab[5]*w5) + 0.5))
#define combine_5_weights(tab,w0,w1,w2,w3,w4)			((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3 + tab[4]*w4) + 0.5))
#define combine_4_weights(tab,w0,w1,w2,w3)				((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2 + tab[3]*w3) + 0.5))
#define combine_3_weights(tab,w0,w1,w2)					((int)((tab[0]*w0 + tab[1]*w1 + tab[2]*w2) + 0.5))
#define combine_2_weights(tab,w0,w1)					((int)((tab[0]*w0 + tab[1]*w1) + 0.5))



/* this should be moved to one of the core files */

#define MAX_NETS 3
#define MAX_RES_PER_NET 8

static double compute_resistor_weights(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * weights_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * weights_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * weights_3, int pulldown_3, int pullup_3 )
{

	int networks_no;

	int rescount[MAX_NETS];		/* number of resistors in each of the nets */
	double r[MAX_NETS][MAX_RES_PER_NET];		/* resistances */
	double w[MAX_NETS][MAX_RES_PER_NET];		/* calulated weights */
	double ws[MAX_NETS][MAX_RES_PER_NET];	/* calulated, scaled weights */
	int r_pd[MAX_NETS];			/* pulldown resistances */
	int r_pu[MAX_NETS];			/* pullup resistances */

	double max_out[MAX_NETS];
	double * out[MAX_NETS];

	int i,j,n;
	double scale;
	double max;

	/* parse input parameters */

	networks_no = 0;
	for (n = 0; n < MAX_NETS; n++)
	{
		int count, pd, pu;
		const int * resistances;
		double * weights;

		switch(n){
		case 0:
				count		= count_1;
				resistances	= resistances_1;
				weights		= weights_1;
				pd			= pulldown_1;
				pu			= pullup_1;
				break;
		case 1:
				count		= count_2;
				resistances	= resistances_2;
				weights		= weights_2;
				pd			= pulldown_2;
				pu			= pullup_2;
				break;
		case 2:
		default:
				count		= count_3;
				resistances	= resistances_3;
				weights		= weights_3;
				pd			= pulldown_3;
				pu			= pullup_3;
				break;
		}

		if (count > 0)
		{
			rescount[networks_no] = count;
			for (i=0; i < count; i++)
			{
				r[networks_no][i] = 1.0 * resistances[i];
			}
			out[networks_no] = weights;
			r_pd[networks_no] = pd;
			r_pu[networks_no] = pu;
			networks_no++;
		}
	}
	if (networks_no < 1)
	{
		/* error - no networks to anaylse */
		logerror("compute_resistor_weights: ERROR - no input data\n");
		return (0.0);
	}

	/* calculate outputs for all given networks */
	for( i = 0; i < networks_no; i++ )
	{
		double R0, R1, Vout, dst;

		/* of n resistors */
		for(n = 0; n < rescount[i]; n++)
		{
			R0 = ( r_pd[i] == 0 ) ? 1.0/1e12 : 1.0/r_pd[i];
			R1 = ( r_pu[i] == 0 ) ? 1.0/1e12 : 1.0/r_pu[i];

			for( j = 0; j < rescount[i]; j++ )
			{
				if( j==n )	/* only one resistance in the network connected to Vcc */
				{
					if (r[i][j] != 0.0)
						R1 += 1.0/r[i][j];
				}
				else
					if (r[i][j] != 0.0)
						R0 += 1.0/r[i][j];
			}

			/* now determine the voltage */
			R0 = 1.0/R0;
			R1 = 1.0/R1;
			Vout = (maxval - minval) * R0 / (R1 + R0) + minval;

			/* and convert it to a destination value */
			dst = (Vout < minval) ? minval : (Vout > maxval) ? maxval : Vout;

			w[i][n] = dst;
		}
	}

	/* calculate maximum outputs for all given networks */
	j = 0;
	max = 0.0;
	for( i = 0; i < networks_no; i++ )
	{
		double sum = 0.0;

		/* of n resistors */
		for( n = 0; n < rescount[i]; n++ )
			sum += w[i][n];	/* maximum output, ie when each resistance is connected to Vcc */

		max_out[i] = sum;
		if (max < sum)
		{
			max = sum;
			j = i;
		}
	}


	if (scaler < 0.0)	/* use autoscale ? */
		/* calculate the output scaler according to the network with the greatest output */
		scale = ((double)maxval) / max_out[j];
	else				/* use scaler provided on entry */
		scale = scaler;

	/* calculate scaled output and fill the output table(s)*/
	for(i = 0; i < networks_no;i++)
	{
		for (n = 0; n < rescount[i]; n++)
		{
			ws[i][n] = w[i][n]*scale;	/* scale the result */
			(out[i])[n] = ws[i][n];		/* fill the output table */
		}
	}

/* debug code */
#ifdef MAME_DEBUG
	logerror("compute_resistor_networks:  scaler = %15.10f\n",scale);
	logerror("min val :%i  max val:%i  Total number of networks :%i\n", minval, maxval, networks_no );

	for(i = 0; i < networks_no;i++)
	{
		double sum = 0.0;

		logerror(" Network no.%i=>  resistances: %i", i, rescount[i] );
		if (r_pu[i] != 0)
			logerror(", pullup resistor: %i Ohms",r_pu[i]);
		if (r_pd[i] != 0)
			logerror(", pulldown resistor: %i Ohms",r_pd[i]);
		logerror("\n  maximum output of this network:%10.5f (scaled to %15.10f)\n", max_out[i], max_out[i]*scale );
		for (n = 0; n < rescount[i]; n++)
		{
			logerror("   res %2i:%9.1f Ohms  weight=%10.5f (scaled = %15.10f)\n", n, r[i][n], w[i][n], ws[i][n] );
			sum += ws[i][n];
		}
		logerror("                              sum of scaled weights = %15.10f\n", sum  );
	}
#endif
/* debug end */

	return (scale);

}




/* for open collector outputs PROMs */

static double compute_resistor_net_outputs(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * outputs_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * outputs_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * outputs_3, int pulldown_3, int pullup_3 );


static double compute_resistor_net_outputs(
	int minval, int maxval, double scaler,
	int count_1, const int * resistances_1, double * outputs_1, int pulldown_1, int pullup_1,
	int count_2, const int * resistances_2, double * outputs_2, int pulldown_2, int pullup_2,
	int count_3, const int * resistances_3, double * outputs_3, int pulldown_3, int pullup_3 )
{

	int networks_no;

	int rescount[MAX_NETS];		/* number of resistors in each of the nets */
	double r[MAX_NETS][MAX_RES_PER_NET];		/* resistances */
	double o[MAX_NETS][1<<MAX_RES_PER_NET];		/* calulated outputs */
	double os[MAX_NETS][1<<MAX_RES_PER_NET];	/* calulated, scaled outputss */
	int r_pd[MAX_NETS];			/* pulldown resistances */
	int r_pu[MAX_NETS];			/* pullup resistances */

	double max_out[MAX_NETS];
	double min_out[MAX_NETS];
	double * out[MAX_NETS];

	int i,j,n;
	double scale;
	double min;
	double max;

	/* parse input parameters */

	networks_no = 0;
	for (n = 0; n < MAX_NETS; n++)
	{
		int count, pd, pu;
		const int * resistances;
		double * weights;

		switch(n){
		case 0:
				count		= count_1;
				resistances	= resistances_1;
				weights		= outputs_1;
				pd			= pulldown_1;
				pu			= pullup_1;
				break;
		case 1:
				count		= count_2;
				resistances	= resistances_2;
				weights		= outputs_2;
				pd			= pulldown_2;
				pu			= pullup_2;
				break;
		case 2:
		default:
				count		= count_3;
				resistances	= resistances_3;
				weights		= outputs_3;
				pd			= pulldown_3;
				pu			= pullup_3;
				break;
		}

		if (count > 0)
		{
			rescount[networks_no] = count;
			for (i=0; i < count; i++)
			{
				r[networks_no][i] = 1.0 * resistances[i];
			}
			out[networks_no] = weights;
			r_pd[networks_no] = pd;
			r_pu[networks_no] = pu;
			networks_no++;
		}
	}
	if (networks_no<1)
	{
		/* error - no networks to anaylse */
		logerror("compute_resistor_weights: ERROR - no input data\n");
		return (0.0);
	}

	/* calculate outputs for all given networks */
	for( i = 0; i < networks_no; i++ )
	{
		double R0, R1, Vout, dst;

		/* of n resistors, generating 1<<n possible outputs */
		for(n = 0; n < (1<<rescount[i]); n++)
		{
			R0 = ( r_pd[i] == 0 ) ? 1.0/1e12 : 1.0/r_pd[i];
			R1 = ( r_pu[i] == 0 ) ? 1.0/1e12 : 1.0/r_pu[i];

			for( j = 0; j < rescount[i]; j++ )
			{
				if( (n & (1<<j)) == 0 )/* only when this resistance in the network connected to GND */
					if (r[i][j] != 0.0)
						R0 += 1.0/r[i][j];
			}

			/* now determine the voltage */
			R0 = 1.0/R0;
			R1 = 1.0/R1;
			Vout = (maxval - minval) * R0 / (R1 + R0) + minval;

			/* and convert it to a destination value */
			dst = (Vout < minval) ? minval : (Vout > maxval) ? maxval : Vout;

			o[i][n] = dst;
		}
	}

	/* calculate minimum outputs for all given networks */
	j = 0;
	min = maxval;
	max = minval;
	for( i = 0; i < networks_no; i++ )
	{
		double val = 0.0;
		double max_tmp = minval;
		double min_tmp = maxval;

		for (n = 0; n < (1<<rescount[i]); n++)
		{
			if (min_tmp > o[i][n])
				min_tmp = o[i][n];
			if (max_tmp < o[i][n])
				max_tmp = o[i][n];
		}

		max_out[i] = max_tmp;	/* maximum output */
		min_out[i] = min_tmp;	/* minimum output */

		val = min_out[i];	/* minimum output of this network */
		if (min > val)
		{
			min = val;
		}
		val = max_out[i];	/* maximum output of this network */
		if (max < val)
		{
			max = val;
		}
	}


	if (scaler < 0.0)	/* use autoscale ? */
		/* calculate the output scaler according to the network with the smallest output */
		scale = ((double)maxval) / (max-min);
	else				/* use scaler provided on entry */
		scale = scaler;

	/* calculate scaled output and fill the output table(s)*/
	for(i = 0; i < networks_no; i++)
	{
		for (n = 0; n < (1<<rescount[i]); n++)
		{
			os[i][n] = (o[i][n] - min) * scale;	/* scale the result */
			(out[i])[n] = os[i][n];		/* fill the output table */
		}
	}

/* debug code */
#ifdef MAME_DEBUG
	logerror("compute_resistor_networks:  scaler = %15.10f\n",scale);
	logerror("min val :%i  max val:%i  Total number of networks :%i\n", minval, maxval, networks_no );

	for(i = 0; i < networks_no;i++)
	{
		logerror(" Network no.%i=>  resistances: %i", i, rescount[i] );
		if (r_pu[i] != 0)
			logerror(", pullup resistor: %i Ohms",r_pu[i]);
		if (r_pd[i] != 0)
			logerror(", pulldown resistor: %i Ohms",r_pd[i]);
		logerror("\n  maximum output of this network:%10.5f", max_out[i] );
		logerror("\n  minimum output of this network:%10.5f\n", min_out[i] );
		for (n = 0; n < rescount[i]; n++)
		{
			logerror("   res %2i:%9.1f Ohms\n", n, r[i][n]);
		}
		for (n = 0; n < (1<<rescount[i]); n++)
		{
			logerror("   combination %2i  out=%10.5f (scaled = %15.10f)\n", n, o[i][n], os[i][n] );
		}
	}
#endif
/* debug end */

	return (scale);

}

#endif /*_res_net_h_*/
