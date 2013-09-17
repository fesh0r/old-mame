/**********************************************************************

    Commodore C2N/1530/1531 Datassette emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "c2n.h"




//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type C2N = &device_creator<c2n_device>;
const device_type C1530 = &device_creator<c1530_device>;
const device_type C1531 = &device_creator<c1531_device>;


//-------------------------------------------------
//  cassette_interface cbm_cassette_interface
//-------------------------------------------------

const cassette_interface cbm_cassette_interface =
{
	cbm_cassette_formats,
	NULL,
	(cassette_state) (CASSETTE_STOPPED | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_MUTED),
	"cbm_cass",
	NULL
};


//-------------------------------------------------
//  MACHINE_CONFIG( c2n )
//-------------------------------------------------

static MACHINE_CONFIG_FRAGMENT( c2n )
	MCFG_CASSETTE_ADD("cassette", cbm_cassette_interface )
MACHINE_CONFIG_END


//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor c2n_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( c2n );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  c2n_device - constructor
//-------------------------------------------------

c2n_device::c2n_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source)
	: device_t(mconfig, type, name, tag, owner, clock, shortname, source),
		device_pet_datassette_port_interface(mconfig, *this),
		m_cassette(*this, "cassette")
{
}

c2n_device::c2n_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, C2N, "C2N Datassette", tag, owner, clock, "c2n", __FILE__),
		device_pet_datassette_port_interface(mconfig, *this),
		m_cassette(*this, "cassette")
{
}


//-------------------------------------------------
//  c1530_device - constructor
//-------------------------------------------------

c1530_device::c1530_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: c2n_device(mconfig, C1530, "C1530 Datassette", tag, owner, clock, "c1530", __FILE__) { }


//-------------------------------------------------
//  c1531_device - constructor
//-------------------------------------------------

c1531_device::c1531_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: c2n_device(mconfig, C1531, "C1531 Datassette", tag, owner, clock, "c1531", __FILE__) { }


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void c2n_device::device_start()
{
	// allocate timers
	m_read_timer = timer_alloc();
	m_read_timer->adjust(attotime::from_hz(44100), 0, attotime::from_hz(44100));
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void c2n_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	int input = (m_cassette->input() > +0.0) ? 1 : 0;

	m_slot->read_w(input);
}


//-------------------------------------------------
//  datassette_read - read data
//-------------------------------------------------

int c2n_device::datassette_read()
{
	return (m_cassette->input() > +0.0) ? 1 : 0;
}


//-------------------------------------------------
//  datassette_write - write data
//-------------------------------------------------

void c2n_device::datassette_write(int state)
{
	m_cassette->output(state ? -(0x5a9e >> 1) : +(0x5a9e >> 1));
}


//-------------------------------------------------
//  datassette_sense - switch sense
//-------------------------------------------------

int c2n_device::datassette_sense()
{
	return (m_cassette->get_state() & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED;
}


//-------------------------------------------------
//  datassette_motor - motor
//-------------------------------------------------

void c2n_device::datassette_motor(int state)
{
	if (!state)
	{
		m_cassette->change_state(CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		m_read_timer->enable(true);
	}
	else
	{
		m_cassette->change_state(CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		m_read_timer->enable(false);
	}
}
