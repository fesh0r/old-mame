/***************************************************************************

Namco 52XX

This instance of the Fujitsu MB8852 MCU is programmed to act as a sample player.
It is used by just two games: Bosconian and Pole Position.

A0-A15 = address to read from sample ROMs
D0-D7 = data freom sample ROMs
CMD = command from CPU (sample to play, 0 = none)
OUT = sound output

      +------+
 EXTAL|1   42|Vcc
  XTAL|2   41|CMD3
/RESET|3   40|CMD2
  /IRQ|4   39|CMD1
  n.c.|5   38|CMD0
  [2] |6   37|A7
  n.c.|7   36|A6
  [1] |8   35|A5
  OUT0|9   34|A4
  OUT1|10  33|A3
  OUT2|11  32|A2
  OUT3|12  31|A1
    A8|13  30|A0
    A9|14  29|D7
   A10|15  28|D6
   A11|16  27|D5
[3]A12|17  26|D4
[3]A13|18  25|D3
[3]A14|19  24|D2
[3]A15|20  23|D1
   GND|21  22|D0
      +------+

[1] in polepos, GND; in bosco, 4kHz output from a 555 timer
[2] in polepos, +5V; in bosco, GND
[3] in polepos, these are true address lines, in bosco they are chip select lines
    (each one select one of the four ROM chips). Behaviour related to [2]?

TODO:
- the purpose of the 555 timer in bosco is unknown; maybe modulate the output?
Jan 12, 2005.  The 555 is probably an external playback frequency.

***************************************************************************/

#include "sndintrf.h"
#include "streams.h"
#include "filter.h"
#include "namco52.h"


typedef struct _namco_52xx namco_52xx;
struct _namco_52xx
{
	const namco_52xx_interface *intf;	/* pointer to our config data */
	UINT8 *rom;			/* pointer to sample ROM */
	UINT32 rom_len;
	sound_stream * stream;			/* the output stream */
	double n52_pb_cycle;	/* playback clock time based on machine sample rate */
	double n52_step;		/* playback clock step based on machine sample rate */
	/* n52_pb_cycle is incremented by n52_step every machine-sample.
     * At every integer value of n52_pb_cycle the next 4bit value is used. */
	INT32 n52_start;		/* current effect start position in the ROM */
	INT32 n52_end;			/* current effect end position in the ROM */
	INT32 n52_length;		/* # of 4bit samples in current effect */
	INT32 n52_pos;			/* current 4bit sample of effect */
	filter2_context n52_hp_filter;
	filter2_context n52_lp_filter;
};


INLINE namco_52xx *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == SOUND);
	assert(sound_get_type(device) == SOUND_NAMCO_52XX);
	return (namco_52xx *)device->token;
}


static void namco_52xx_reset(namco_52xx *chip);


static STREAM_UPDATE( namco_52xx_stream_update_one )
{
	namco_52xx *chip = (namco_52xx *)param;
	int i, rom_pos, whole_pb_cycles, buf;
	stream_sample_t *buffer = outputs[0];

	if (chip->n52_start >= chip->n52_end)
	{
		memset(buffer, 0, samples * sizeof(*buffer));
		return;
	}

	for (i = 0; i < samples; i++)
	{
		chip->n52_pb_cycle += chip->n52_step;
		if (chip->n52_pb_cycle >= 1)
		{
			whole_pb_cycles = (int)chip->n52_pb_cycle;
			chip->n52_pos += whole_pb_cycles;
			chip->n52_pb_cycle -= whole_pb_cycles;
		}

		if (chip->n52_pos > chip->n52_length)
		{
			/* sample done */
			memset(&buffer[i], 0, (samples - i) * sizeof(INT16));
			i = samples;
			namco_52xx_reset(chip);
		}
		else
		{
			/* filter and fill the buffer */
			rom_pos = chip->n52_start + (chip->n52_pos >> 1);
			/* get the 4bit sample from rom and shift to +7/-8 value */
			chip->n52_hp_filter.x0 = (((chip->n52_pos & 1) ? chip->rom[rom_pos] >> 4 : chip->rom[rom_pos]) & 0x0f) - 0x08;
			filter2_step(&chip->n52_hp_filter);
			chip->n52_lp_filter.x0 = chip->n52_hp_filter.y0;
			filter2_step(&chip->n52_lp_filter);
			/* convert 4bit filtered to 16bit allowing room for filter gain */
			buf = (int)(chip->n52_lp_filter.y0 * 0x0fff);
			if (buf > 32767) buf = 32767;
			if (buf < -32768) buf = -32768;
			buffer[i] = buf;
		}
	}
}

static void namco_52xx_reset(namco_52xx *chip)
{
	chip->n52_pb_cycle = chip->n52_start = chip->n52_end = chip->n52_length = chip->n52_pos = 0;

	filter2_reset(&chip->n52_hp_filter);
	filter2_reset(&chip->n52_lp_filter);
}

static DEVICE_RESET( namco_52xx )
{
	namco_52xx_reset(get_safe_token(device));
}

static DEVICE_START( namco_52xx )
{
	namco_52xx *chip = get_safe_token(device);
	int rate = device->clock/32;

	chip->intf = (const namco_52xx_interface *)device->static_config;
	chip->rom     = device->region;
	chip->rom_len = device->regionbytes;

	if (chip->intf->play_rate == 0)
	{
		/* If play clock is 0 (grounded) then default to internal clock */
		chip->n52_step = (double)device->clock / 384 / rate;
	}
	else
	{
		chip->n52_step = chip->intf->play_rate / rate;
	}
	filter2_setup(device, FILTER_HIGHPASS, chip->intf->hp_filt_fc, Q_TO_DAMP(chip->intf->hp_filt_q), 1, &chip->n52_hp_filter);
	filter2_setup(device, FILTER_LOWPASS,  chip->intf->lp_filt_fc, Q_TO_DAMP(chip->intf->lp_filt_q), chip->intf->filt_gain, &chip->n52_lp_filter);


	chip->stream = stream_create(device, 0, 1, rate, chip, namco_52xx_stream_update_one);

	namco_52xx_reset(chip);

	state_save_register_device_item(device, 0, chip->n52_pb_cycle);
	state_save_register_device_item(device, 0, chip->n52_step);
	state_save_register_device_item(device, 0, chip->n52_start);
	state_save_register_device_item(device, 0, chip->n52_end);
	state_save_register_device_item(device, 0, chip->n52_length);
	state_save_register_device_item(device, 0, chip->n52_pos);
}


void namcoio_52xx_write(const device_config *device, int data)
{
	namco_52xx *chip = get_safe_token(device);
	data &= 0x0f;

	if (data != 0)
	{
		stream_update(chip->stream);

		chip->n52_start = chip->rom[data-1] + (chip->rom[data-1+0x10] << 8);
		chip->n52_end = chip->rom[data] + (chip->rom[data+0x10] << 8);

		if (chip->n52_end >= chip->rom_len)
			chip->n52_end = chip->rom_len;

		chip->n52_length = (chip->n52_end - chip->n52_start) * 2;
		chip->n52_pos = 0;
		chip->n52_pb_cycle= 0;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

DEVICE_GET_INFO( namco_52xx )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(namco_52xx);				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( namco_52xx );			break;
		case DEVINFO_FCT_STOP:							/* Nothing */										break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( namco_52xx );			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Namco 52XX");						break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Namco custom");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");								break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);							break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}

