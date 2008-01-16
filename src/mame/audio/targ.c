/*************************************************************************

    Targ hardware

*************************************************************************/

/* Sound channel usage
   0 = CPU music,  Shoot
   1 = Crash
   2 = Spectar sound
   3 = Tone generator
*/

#include "driver.h"
#include "sound/samples.h"
#include "sound/dac.h"
#include "targ.h"



#define SPECTAR_MAXFREQ		525000
#define TARG_MAXFREQ		125000


static int max_freq;

static UINT8 port_1_last;
static UINT8 port_2_last;

static UINT8 tone_freq;
static UINT8 tone_active;
static UINT8 tone_pointer;


static const INT16 sine_wave[32] =
{
	 0x0f0f,  0x0f0f,  0x0f0f,  0x0606,  0x0606,  0x0909,  0x0909,  0x0606,  0x0606,  0x0909,  0x0606,  0x0d0d,  0x0f0f,  0x0f0f,  0x0d0d,  0x0000,
	-0x191a, -0x2122, -0x1e1f, -0x191a, -0x1314, -0x191a, -0x1819, -0x1819, -0x1819, -0x1314, -0x1314, -0x1314, -0x1819, -0x1e1f, -0x1e1f, -0x1819
};


/* some macros to make detecting bit changes easier */
#define RISING_EDGE(bit)  ( (data & bit) && !(port_1_last & bit))
#define FALLING_EDGE(bit) (!(data & bit) &&  (port_1_last & bit))



static void adjust_sample(UINT8 freq)
{
	tone_freq = freq;

	if ((tone_freq == 0xff) || (tone_freq == 0x00))
		sample_set_volume(3, 0);
	else
	{
		sample_set_freq(3, 1.0 * max_freq / (0xff - tone_freq));
		sample_set_volume(3, tone_active);
	}
}


WRITE8_HANDLER( targ_audio_1_w )
{
	/* CPU music */
	if ((data & 0x01) != (port_1_last & 0x01))
		DAC_data_w(0,(data & 0x01) * 0xff);

	/* shot */
	if (FALLING_EDGE(0x02) && !sample_playing(0))  sample_start(0,1,0);
	if (RISING_EDGE(0x02)) sample_stop(0);

	/* crash */
	if (RISING_EDGE(0x20))
	{
		if (data & 0x40)
			sample_start(1,2,0);
		else
			sample_start(1,0,0);
	}

	/* Sspec */
	if (data & 0x10)
		sample_stop(2);
	else
	{
		if ((data & 0x08) != (port_1_last & 0x08))
		{
			if (data & 0x08)
				sample_start(2,3,1);
			else
				sample_start(2,4,1);
		}
	}

	/* Game (tone generator enable) */
	if (FALLING_EDGE(0x80))
	{
		tone_pointer = 0;
		tone_active = 0;

		adjust_sample(tone_freq);
	}

	if (RISING_EDGE(0x80))
		tone_active=1;

	port_1_last = data;
}


WRITE8_HANDLER( targ_audio_2_w )
{
	if ((data & 0x01) && !(port_2_last & 0x01))
	{
		UINT8 *prom = memory_region(TARG_TONE_REGION);

		tone_pointer = (tone_pointer + 1) & 0x0f;

		adjust_sample(prom[((data & 0x02) << 3) | tone_pointer]);
	}

	port_2_last = data;
}


WRITE8_HANDLER( spectar_audio_2_w )
{
	adjust_sample(data);
}


static const char *const sample_names[] =
{
	"*targ",
	"expl.wav",
	"shot.wav",
	"sexpl.wav",
	"spslow.wav",
	"spfast.wav",
	0
};


static void common_audio_start(int freq)
{
	max_freq = freq;

	tone_freq = 0;
	tone_active = 0;

	sample_set_volume(3, 0);
	sample_start_raw(3, sine_wave, 32, 1000, 1);

	state_save_register_global(port_1_last);
	state_save_register_global(port_2_last);
	state_save_register_global(tone_freq);
	state_save_register_global(tone_active);
}


static void spectar_audio_start(void)
{
	common_audio_start(SPECTAR_MAXFREQ);
}


static void targ_audio_start(void)
{
	common_audio_start(TARG_MAXFREQ);

	tone_pointer = 0;

	state_save_register_global(tone_pointer);
}


static const struct Samplesinterface spectar_samples_interface =
{
	4,	/* number of channel */
	sample_names,
	spectar_audio_start
};


static const struct Samplesinterface targ_samples_interface =
{
	4,	/* number of channel */
	sample_names,
	targ_audio_start
};


MACHINE_DRIVER_START( spectar_audio )

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(spectar_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( targ_audio )

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(targ_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END
