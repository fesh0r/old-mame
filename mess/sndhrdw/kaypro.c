/******************************************************************************
 *	kaypro.c
 *
 *	KAYPRO terminal beeper emulation
 *
 ******************************************************************************/

#include "driver.h"

static	int channel;
#define BELL_FREQ	1000
static	INT32 bell_signal;
static	INT32 bell_counter;

#define CLICK_FREQ	2000
static	INT32 click_signal;
static	INT32 click_counter;

void kaypro_sound_update(int param, INT16 *buffer, int length)
{
	while (length-- > 0)
	{
		if ((bell_counter -= BELL_FREQ) < 0)
		{
			bell_counter += Machine->sample_rate;
			bell_signal = -(bell_signal * 127) / 128;
		}
		if ((click_counter -= CLICK_FREQ) < 0)
		{
			click_counter += Machine->sample_rate;
			click_signal = -(click_signal * 3) / 4;
		}
		*buffer++ = bell_signal + click_signal;
	}
}

void kaypro_sh_start(const struct MachineSound *msound)
{
	channel = stream_init("Beeper", 100, Machine->sample_rate, 0, kaypro_sound_update);
}

void kaypro_sh_stop(void)
{
}

void kaypro_sh_update(void)
{
	stream_update(channel,0);
}

/******************************************************
 *	Ring my bell ;)
 ******************************************************/
void kaypro_bell(void)
{
	bell_signal = 0x3000;
}

/******************************************************
 *	Clicking keys (for the Kaypro 2x)
 ******************************************************/
void kaypro_click(void)
{
	click_signal = 0x3000;
}



