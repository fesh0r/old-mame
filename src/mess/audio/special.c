/***************************************************************************

  audio/special.c

  Functions to emulate sound hardware of Specialist MX
  ( based on code of DAI interface )

****************************************************************************/

#include "includes/special.h"


// device type definition
const device_type SPECIMX = &device_creator<specimx_sound_device>;


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  specimx_sound_device - constructor
//-------------------------------------------------

specimx_sound_device::specimx_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, SPECIMX, "Specialist MX Custom", tag, owner, clock, "specimx_sound", __FILE__),
		device_sound_interface(mconfig, *this),
		m_mixer_channel(NULL)
{
	memset(m_specimx_input, 0, sizeof(int)*3);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void specimx_sound_device::device_start()
{
	m_specimx_input[0] = m_specimx_input[1] = m_specimx_input[2] = 0;
	m_mixer_channel = stream_alloc(0, 1, machine().sample_rate());
}


//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void specimx_sound_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	INT16 channel_0_signal;
	INT16 channel_1_signal;
	INT16 channel_2_signal;

	stream_sample_t *sample_left = outputs[0];

	channel_0_signal = m_specimx_input[0] ? 3000 : -3000;
	channel_1_signal = m_specimx_input[1] ? 3000 : -3000;
	channel_2_signal = m_specimx_input[2] ? 3000 : -3000;

	while (samples--)
	{
		*sample_left = 0;

		/* music channel 0 */
		*sample_left += channel_0_signal;

		/* music channel 1 */
		*sample_left += channel_1_signal;

		/* music channel 2 */
		*sample_left += channel_2_signal;

		sample_left++;
	}
}


void specimx_sound_device::set_input(int index, int state)
{
	if (m_mixer_channel!=NULL)
	{
		m_mixer_channel->update();
	}
	m_specimx_input[index] = state;
}
