/***************************************************************************

    Capcom CPS-3 Sound Hardware

***************************************************************************/
#include "emu.h"
#include "includes/cps3.h"


// device type definition
const device_type CPS3 = &device_creator<cps3_sound_device>;


//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  cps3_sound_device - constructor
//-------------------------------------------------

cps3_sound_device::cps3_sound_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, CPS3, "CPS3 Custom", tag, owner, clock, "cps3_custom", __FILE__),
		device_sound_interface(mconfig, *this),
		m_stream(NULL),
		m_key(0),
		m_base(NULL)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void cps3_sound_device::device_start()
{
	/* Allocate the stream */
	m_stream = stream_alloc(0, 2, clock() / 384);
}


//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void cps3_sound_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	int i;

	// the actual 'user5' region only exists on the nocd sets, on the others it's allocated in the initialization.
	// it's a shared gfx/sound region, so can't be allocated as part of the sound device.
	m_base = (INT8*)machine().driver_data<cps3_state>()->m_user5region;

	/* Clear the buffers */
	memset(outputs[0], 0, samples*sizeof(*outputs[0]));
	memset(outputs[1], 0, samples*sizeof(*outputs[1]));

	for (i = 0; i < CPS3_VOICES; i ++)
	{
		if (m_key & (1 << i))
		{
			int j;

			/* TODO */
			#define SWAP(a) ((a >> 16) | ((a & 0xffff) << 16))

			cps3_voice *vptr = &m_voice[i];

			UINT32 start = vptr->regs[1];
			UINT32 end   = vptr->regs[5];
			UINT32 loop  = (vptr->regs[3] & 0xffff) + ((vptr->regs[4] & 0xffff) << 16);
			UINT32 step  = (vptr->regs[3] >> 16);

			INT16 vol_l = (vptr->regs[7] & 0xffff);
			INT16 vol_r = ((vptr->regs[7] >> 16) & 0xffff);

			UINT32 pos = vptr->pos;
			UINT16 frac = vptr->frac;

			/* TODO */
			start = SWAP(start) - 0x400000;
			end = SWAP(end) - 0x400000;
			loop -= 0x400000;

			/* Go through the buffer and add voice contributions */
			for (j = 0; j < samples; j ++)
			{
				INT32 sample;

				pos += (frac >> 12);
				frac &= 0xfff;


				if (start + pos >= end)
				{
					if (vptr->regs[2])
					{
						pos = loop - start;
					}
					else
					{
						m_key &= ~(1 << i);
						break;
					}
				}

				sample = m_base[BYTE4_XOR_LE(start + pos)];
				frac += step;

				outputs[0][j] += (sample * (vol_l >> 8));
				outputs[1][j] += (sample * (vol_r >> 8));
			}

			vptr->pos = pos;
			vptr->frac = frac;
		}
	}
}


WRITE32_MEMBER( cps3_sound_device::cps3_sound_w )
{
	m_stream->update();

	if (offset < 0x80)
	{
		COMBINE_DATA(&m_voice[offset / 8].regs[offset & 7]);
	}
	else if (offset == 0x80)
	{
		int i;
		UINT16 key = data >> 16;

		for (i = 0; i < CPS3_VOICES; i++)
		{
			// Key off -> Key on
			if ((key & (1 << i)) && !(m_key & (1 << i)))
			{
				m_voice[i].frac = 0;
				m_voice[i].pos = 0;
			}
		}
		m_key = key;
	}
	else
	{
		// during boot: Sound [84] 230000
		logerror("Sound [%x] %x\n", offset, data);
	}
}


READ32_MEMBER( cps3_sound_device::cps3_sound_r )
{
	m_stream->update();

	if (offset < 0x80)
	{
		return m_voice[offset / 8].regs[offset & 7] & mem_mask;
	}
	else if (offset == 0x80)
	{
		return m_key << 16;
	}
	else
	{
		logerror("Unk sound read : %x\n", offset);
		return 0;
	}
}
