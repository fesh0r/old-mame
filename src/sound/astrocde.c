/***********************************************************

     Astrocade custom 'IO' chip sound chip driver
     Frank Palazzolo

     Portions copied from the Pokey emulator by Ron Fries

     First Release:
        09/20/98
     Updated 11/2004
        Fixed noise generator bug
        Changed to stream system
        Fixed out of bounds memory access bug

***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "astrocde.h"
//#include "cpu/z80/z80.h"


struct astrocade_info
{
	const struct astrocade_interface *intf;

	int div_by_N_factor;

	int current_count_A;
	int current_count_B;
	int current_count_C;
	int current_count_V;
	int current_count_N;

	int current_state_A;
	int current_state_B;
	int current_state_C;
	int current_state_V;

	int current_size_A;
	int current_size_B;
	int current_size_C;
	int current_size_V;
	int current_size_N;

	sound_stream *stream;

	/* Registers */

	int master_osc;
	int freq_A;
	int freq_B;
	int freq_C;
	int vol_A;
	int vol_B;
	int vol_C;
	int vibrato;
	int vibrato_speed;
	int mux;
	int noise_am;
	int vol_noise4;
	int vol_noise8;

	int randbyte;
	int randbit;
};

static void astrocade_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int num_samples)
{
	struct astrocade_info *chip = param;
	stream_sample_t *bufptr;
	int count;
	int data, data16, noise_plus_osc, vib_plus_osc;

	bufptr = buffer[0];
	count = num_samples;

	/* For each sample */
	while(count>0)
	{
		/* Update current output value */

		if (chip->current_count_N == 0)
		{
			chip->randbyte = rand() & 0xff;
		}

		chip->current_size_V = 32768*chip->vibrato_speed/chip->div_by_N_factor;

		if (!chip->mux)
		{
			if (chip->current_state_V == -1)
				vib_plus_osc = (chip->master_osc-chip->vibrato)&0xff;
			else
				vib_plus_osc = chip->master_osc;
			chip->current_size_A = vib_plus_osc*chip->freq_A/chip->div_by_N_factor;
			chip->current_size_B = vib_plus_osc*chip->freq_B/chip->div_by_N_factor;
			chip->current_size_C = vib_plus_osc*chip->freq_C/chip->div_by_N_factor;
		}
		else
		{
			noise_plus_osc = ((chip->master_osc-(chip->vol_noise8&chip->randbyte)))&0xff;
			chip->current_size_A = noise_plus_osc*chip->freq_A/chip->div_by_N_factor;
			chip->current_size_B = noise_plus_osc*chip->freq_B/chip->div_by_N_factor;
			chip->current_size_C = noise_plus_osc*chip->freq_C/chip->div_by_N_factor;
			chip->current_size_N = 2*noise_plus_osc/chip->div_by_N_factor;
		}

		data = (chip->current_state_A*chip->vol_A +
				chip->current_state_B*chip->vol_B +
				chip->current_state_C*chip->vol_C);

		if (chip->noise_am)
		{
			chip->randbit = rand() & 1;
			data = data + chip->randbit*chip->vol_noise4;
		}

		/* Put it in the buffer */

		data16 = data<<8;
		*bufptr = data16;
		bufptr++;

		/* Update the state of the chip */

		if (chip->current_count_A >= chip->current_size_A)
		{
			chip->current_state_A = -chip->current_state_A;
			chip->current_count_A = 0;
		}
		else
			chip->current_count_A++;

		if (chip->current_count_B >= chip->current_size_B)
		{
			chip->current_state_B = -chip->current_state_B;
			chip->current_count_B = 0;
		}
		else
			chip->current_count_B++;

		if (chip->current_count_C >= chip->current_size_C)
		{
			chip->current_state_C = -chip->current_state_C;
			chip->current_count_C = 0;
		}
		else
			chip->current_count_C++;

		if (chip->current_count_V >= chip->current_size_V)
		{
			chip->current_state_V = -chip->current_state_V;
			chip->current_count_V = 0;
		}
		else
			chip->current_count_V++;

		if (chip->current_count_N >= chip->current_size_N)
		{
			chip->current_count_N = 0;
		}
		else
			chip->current_count_N++;

		count--;
	}
}


static void astrocade_reset(void *_chip)
{
	struct astrocade_info *chip = _chip;
	chip->current_count_A = 0;
	chip->current_count_B = 0;
	chip->current_count_C = 0;
	chip->current_count_V = 0;
	chip->current_count_N = 0;
	chip->current_state_A = 1;
	chip->current_state_B = 1;
	chip->current_state_C = 1;
	chip->current_state_V = 1;
	chip->randbyte = 0;
	chip->randbit = 1;
}


static void *astrocade_start(int sndindex, int clock, const void *config)
{
	struct astrocade_info *chip;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));

	chip->intf = config;

	if (Machine->sample_rate == 0)
	{
		return chip;
	}

	chip->div_by_N_factor = clock/Machine->sample_rate;

	chip->stream = stream_create(0,1,Machine->sample_rate,chip,astrocade_update);

	/* reset state */
	astrocade_reset(chip);

	return chip;
}



void astrocade_sound_w(int num, int offset, int data)
{
	struct astrocade_info *chip = sndti_token(SOUND_ASTROCADE, num);
	int i, temp_vib;

	/* update */
	stream_update(chip->stream,0);

	switch(offset)
	{
		case 0:  /* Master Oscillator */
#ifdef VERBOSE
			logerror("Master Osc Write: %02x\n",data);
#endif
			chip->master_osc = data+1;
		break;

		case 1:  /* Tone A Frequency */
#ifdef VERBOSE
			logerror("Tone A Write:        %02x\n",data);
#endif
			chip->freq_A = data+1;
		break;

		case 2:  /* Tone B Frequency */
#ifdef VERBOSE
			logerror("Tone B Write:           %02x\n",data);
#endif
			chip->freq_B = data+1;
		break;

		case 3:  /* Tone C Frequency */
#ifdef VERBOSE
			logerror("Tone C Write:              %02x\n",data);
#endif
			chip->freq_C = data+1;
		break;

		case 4:  /* Vibrato Register */
#ifdef VERBOSE
			logerror("Vibrato Depth:                %02x\n",data&0x3f);
			logerror("Vibrato Speed:                %02x\n",data>>6);
#endif
			chip->vibrato = data & 0x3f;

			temp_vib = (data>>6) & 0x03;
			chip->vibrato_speed = 1;
			for(i=0;i<temp_vib;i++)
				chip->vibrato_speed <<= 1;

		break;

		case 5:  /* Tone C Volume, Noise Modulation Control */
			chip->vol_C = data & 0x0f;
			chip->mux = (data>>4) & 0x01;
			chip->noise_am = (data>>5) & 0x01;
#ifdef VERBOSE
			logerror("Tone C Vol:                      %02x\n",chip->vol_C);
			logerror("Mux Source:                      %02x\n",chip->mux);
			logerror("Noise Am:                        %02x\n",chip->noise_am);
#endif
		break;

		case 6:  /* Tone A & B Volume */
			chip->vol_B = (data>>4) & 0x0f;
			chip->vol_A = data & 0x0f;
#ifdef VERBOSE
			logerror("Tone A Vol:                         %02x\n",chip->vol_A);
			logerror("Tone B Vol:                         %02x\n",chip->vol_B);
#endif
		break;

		case 7:  /* Noise Volume Register */
			chip->vol_noise8 = data;
			chip->vol_noise4 = (data>>4) & 0x0f;
#ifdef VERBOSE
			logerror("Noise Vol:                             %02x\n",chip->vol_noise8);
			logerror("Noise Vol (4):                         %02x\n",chip->vol_noise4);
#endif
		break;
	}
}

WRITE8_HANDLER( astrocade_soundblock1_w )
{
	astrocade_sound_w(0, (offset>>8)&7, data);
}

WRITE8_HANDLER( astrocade_soundblock2_w )
{
	astrocade_sound_w(1, (offset>>8)&7, data);
}

WRITE8_HANDLER( astrocade_sound1_w )
{
	astrocade_sound_w(0, offset, data);
}

WRITE8_HANDLER( astrocade_sound2_w )
{
	astrocade_sound_w(1, offset, data);
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void astrocade_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void astrocade_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = astrocade_set_info;	break;
		case SNDINFO_PTR_START:							info->start = astrocade_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = astrocade_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Astrocade";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Bally";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

