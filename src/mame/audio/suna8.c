/*
    SunA 8 Bit Games samples

    Format: PCM unsigned 4 bit mono 8kHz
*/

#include "emu.h"
#include "sound/samples.h"
#include "includes/suna8.h"

#define FREQ_HZ 8000
#define NUMSAMPLES 0x1000

WRITE8_MEMBER(suna8_state::suna8_play_samples_w)
{
	if ( data )
	{
		samples_device *samples = downcast<samples_device *>(machine().device("samples"));
		if ( ~data & 0x10 )
		{
			samples->start_raw(0, &m_samplebuf[NUMSAMPLES * m_sample], NUMSAMPLES, FREQ_HZ);
		}
		else if ( ~data & 0x08 )
		{
			m_sample &= 3;
			samples->start_raw(0, &m_samplebuf[NUMSAMPLES * (m_sample+7)], NUMSAMPLES, FREQ_HZ);
		}
	}
}

WRITE8_MEMBER(suna8_state::rranger_play_samples_w)
{
	if (data)
	{
		if (( m_sample != 0 ) && ( ~data & 0x30 ))	// don't play sample zero when those bits are active
		{
			samples_device *samples = downcast<samples_device *>(machine().device("samples"));
			samples->start_raw(0, &m_samplebuf[NUMSAMPLES * m_sample], NUMSAMPLES, FREQ_HZ);
		}
	}
}

WRITE8_MEMBER(suna8_state::suna8_samples_number_w)
{
	m_sample = data & 0xf;
}

SAMPLES_START( suna8_sh_start )
{
	suna8_state *state = device.machine().driver_data<suna8_state>();
	running_machine &machine = device.machine();

	int i, len = state->memregion("samples")->bytes() * 2;	// 2 samples per byte
	UINT8 *ROM = state->memregion("samples")->base();

	state->m_samplebuf = auto_alloc_array(machine, INT16, len);

	// Convert 4 bit to 16 bit samples
	for(i = 0; i < len; i++)
		state->m_samplebuf[i] = (INT8)(((ROM[i/2] << ((i & 1)?0:4)) & 0xf0)  ^ 0x80) * 0x100;
}
