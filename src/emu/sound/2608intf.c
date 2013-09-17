/***************************************************************************

  2608intf.c

  The YM2608 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#include "2608intf.h"
#include "fm.h"

static void psg_set_clock(void *param, int clock)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ay8910_set_clock_ym(ym2608->_psg(), clock);
}

static void psg_write(void *param, int address, int data)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ay8910_write_ym(ym2608->_psg(), address, data);
}

static int psg_read(void *param)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	return ay8910_read_ym(ym2608->_psg());
}

static void psg_reset(void *param)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ay8910_reset_ym(ym2608->_psg());
}

static const ssg_callbacks psgintf =
{
	psg_set_clock,
	psg_write,
	psg_read,
	psg_reset
};

void *ym2608_device::_psg()
{
	return m_psg;
}

/* IRQ Handler */
static void IRQHandler(void *param,int irq)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ym2608->_IRQHandler(irq);
}

void ym2608_device::_IRQHandler(int irq)
{
	if (!m_irq_handler.isnull())
		m_irq_handler(irq);
}

/* Timer overflow callback from timer.c */
void ym2608_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
	case 0:
		ym2608_timer_over(m_chip,0);
		break;

	case 1:
		ym2608_timer_over(m_chip,1);
		break;
	}
}

static void timer_handler(void *param,int c,int count,int clock)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ym2608->_timer_handler(c, count, clock);
}

void ym2608_device::_timer_handler(int c,int count,int clock)
{
	if( count == 0 )
	{   /* Reset FM Timer */
		m_timer[c]->enable(false);
	}
	else
	{   /* Start FM Timer */
		attotime period = attotime::from_hz(clock) * count;

		if (!m_timer[c]->enable(true))
			m_timer[c]->adjust(period);
	}
}

/* update request from fm.c */
void ym2608_update_request(void *param)
{
	ym2608_device *ym2608 = (ym2608_device *) param;
	ym2608->_ym2608_update_request();
}

void ym2608_device::_ym2608_update_request()
{
	m_stream->update();
}

//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void ym2608_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
	ym2608_update_one(m_chip, outputs, samples);
}


void ym2608_device::device_post_load()
{
	ym2608_postload(m_chip);
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void ym2608_device::device_start()
{
	static const ay8910_interface default_ay8910_config =
	{
		AY8910_LEGACY_OUTPUT | AY8910_SINGLE_OUTPUT,
		AY8910_DEFAULT_LOADS,
		DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL
	};

	int rate = clock()/72;
	void *pcmbufa;
	int  pcmsizea;

	const ay8910_interface *ay8910_config = m_ay8910_config != NULL ? m_ay8910_config : &default_ay8910_config;

	m_irq_handler.resolve();
	/* FIXME: Force to use single output */
	m_psg = ay8910_start_ym(this, ay8910_config);
	assert_always(m_psg != NULL, "Error creating YM2608/AY8910 chip");

	/* Timer Handler set */
	m_timer[0] = timer_alloc(0);
	m_timer[1] = timer_alloc(1);

	/* stream system initialize */
	m_stream = machine().sound().stream_alloc(*this,0,2,rate);
	/* setup adpcm buffers */
	pcmbufa  = *region();
	pcmsizea = region()->bytes();

	/* initialize YM2608 */
	m_chip = ym2608_init(this,this,clock(),rate,
					pcmbufa,pcmsizea,
					timer_handler,IRQHandler,&psgintf);
	assert_always(m_chip != NULL, "Error creating YM2608 chip");
}

//-------------------------------------------------
//  device_stop - device-specific stop
//-------------------------------------------------

void ym2608_device::device_stop()
{
	ym2608_shutdown(m_chip);
	ay8910_stop_ym(m_psg);
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void ym2608_device::device_reset()
{
	ym2608_reset_chip(m_chip);
}


READ8_MEMBER( ym2608_device::read )
{
	return ym2608_read(m_chip, offset & 3);
}

WRITE8_MEMBER( ym2608_device::write )
{
	ym2608_write(m_chip, offset & 3, data);
}

const device_type YM2608 = &device_creator<ym2608_device>;

ym2608_device::ym2608_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, YM2608, "YM2608", tag, owner, clock, "ym2608", __FILE__),
		device_sound_interface(mconfig, *this),
		m_irq_handler(*this),
		m_ay8910_config(NULL)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void ym2608_device::device_config_complete()
{
}
