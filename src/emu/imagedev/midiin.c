/*********************************************************************

    midiin.c

    MIDI In image device and serial transmitter

*********************************************************************/

#include "emu.h"
#include "midiin.h"

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

const device_type MIDIIN = &device_creator<midiin_device>;

/*-------------------------------------------------
    ctor
-------------------------------------------------*/

midiin_device::midiin_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, MIDIIN, "MIDI In image device", tag, owner, clock),
		device_image_interface(mconfig, *this),
			device_serial_interface(mconfig, *this)
{
}

/*-------------------------------------------------
    device_start
-------------------------------------------------*/

void midiin_device::device_start()
{
	m_input_func.resolve(m_input_callback, *this);
	m_timer = timer_alloc(0);
	m_midi = NULL;
	m_timer->enable(false);
}

void midiin_device::device_reset()
{
	m_tx_busy = false;
	m_xmit_read = m_xmit_write = 0;

	// we don't Rx, we Tx at 31250 8-N-1
	set_rcv_rate(0);
	set_tra_rate(31250);
	set_data_frame(8, 1, SERIAL_PARITY_NONE);
}

/*-------------------------------------------------
    device_config_complete
-------------------------------------------------*/

void midiin_device::device_config_complete(void)
{
	const midiin_config *intf = reinterpret_cast<const midiin_config *>(static_config());
	if(intf != NULL)
	{
		*static_cast<midiin_config *>(this) = *intf;
	}
	else
	{
		memset(&m_input_callback, 0, sizeof(m_input_callback));
	}
	update_names();
}

/*-------------------------------------------------
    device_timer
-------------------------------------------------*/

void midiin_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	UINT8 buf[8192*4];
	int bytesRead;

	if (m_midi == NULL) {
		return;
	}

	while (osd_poll_midi_channel(m_midi))
	{
		bytesRead = osd_read_midi_channel(m_midi, buf);

		if (bytesRead > 0)
		{
			for (int i = 0; i < bytesRead; i++)
			{
				xmit_char(buf[i]);
			}
		}
	}
}

/*-------------------------------------------------
    call_load
-------------------------------------------------*/

bool midiin_device::call_load(void)
{
	m_midi = osd_open_midi_input(filename());

	if (m_midi == NULL)
	{
		return IMAGE_INIT_FAIL;
	}

	m_timer->adjust(attotime::from_hz(1500), 0, attotime::from_hz(1500));
	m_timer->enable(true);
	return IMAGE_INIT_PASS;
}

/*-------------------------------------------------
    call_unload
-------------------------------------------------*/

void midiin_device::call_unload(void)
{
	if (m_midi)
	{
		osd_close_midi_channel(m_midi);
	}
		m_timer->enable(false);
		m_midi = NULL;
}

void midiin_device::tra_complete()
{
	// is there more waiting to send?
	if (m_xmit_read != m_xmit_write)
	{
//      printf("tx1 %02x\n", m_xmitring[m_xmit_read]);
		transmit_register_setup(m_xmitring[m_xmit_read++]);
		if (m_xmit_read >= XMIT_RING_SIZE)
		{
			m_xmit_read = 0;
		}
	}
	else
	{
		m_tx_busy = false;
	}
}

void midiin_device::tra_callback()
{
	int bit = transmit_register_get_data_bit();
	m_input_func(bit);
}

void midiin_device::xmit_char(UINT8 data)
{
//  printf("MIDI in: xmit %02x\n", data);

	// if tx is busy it'll pick this up automatically when it completes
	if (!m_tx_busy)
	{
		m_tx_busy = true;
//      printf("tx0 %02x\n", data);
		transmit_register_setup(data);
	}
	else
	{
		// tx is busy, it'll pick this up next time
		m_xmitring[m_xmit_write++] = data;
		if (m_xmit_write >= XMIT_RING_SIZE)
		{
			m_xmit_write = 0;
		}
	}
}

void midiin_device::input_callback(UINT8 state)
{
}
