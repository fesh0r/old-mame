/**********************************************************************

    Sega Master System "Light Phaser" (light gun) emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#include "sms_lphaser.h"



//**************************************************************************
//  DEVICE DEFINITIONS
//**************************************************************************

const device_type SMS_LIGHT_PHASER = &device_creator<sms_light_phaser_device>;


#define LGUN_RADIUS           6
#define LGUN_X_INTERVAL       4


INPUT_CHANGED_MEMBER( sms_light_phaser_device::th_pin_w )
{
	if (newval != oldval)
		m_port->th_pin_w(newval);
}


INPUT_CHANGED_MEMBER( sms_light_phaser_device::position_changed )
{
	if (newval != oldval)
		sensor_check();
}


static INPUT_PORTS_START( sms_light_phaser )
	PORT_START("CTRL_PORT")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) // TL (trigger)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_CHANGED_MEMBER(DEVICE_SELF, sms_light_phaser_device, th_pin_w, 0)
	PORT_BIT( 0x9f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("LPHASER_X")
	PORT_BIT( 0xff, 0x00, IPT_LIGHTGUN_X) PORT_CROSSHAIR(X, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(15)  PORT_CHANGED_MEMBER(DEVICE_SELF, sms_light_phaser_device, position_changed, 0)

	PORT_START("LPHASER_Y")
	PORT_BIT( 0xff, 0x00, IPT_LIGHTGUN_Y) PORT_CROSSHAIR(Y, 1.0, 0.0, 0) PORT_SENSITIVITY(50) PORT_KEYDELTA(15) PORT_CHANGED_MEMBER(DEVICE_SELF, sms_light_phaser_device, position_changed, 0)
INPUT_PORTS_END


//-------------------------------------------------
//  input_ports - device-specific input ports
//-------------------------------------------------

ioport_constructor sms_light_phaser_device::device_input_ports() const
{
	return INPUT_PORTS_NAME( sms_light_phaser );
}



//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  sms_light_phaser_device - constructor
//-------------------------------------------------

sms_light_phaser_device::sms_light_phaser_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock) :
	device_t(mconfig, SMS_LIGHT_PHASER, "Light Phaser", tag, owner, clock, "sms_light_phaser", __FILE__),
	device_sms_control_port_interface(mconfig, *this),
	m_lphaser_pins(*this, "CTRL_PORT"),
	m_lphaser_x(*this, "LPHASER_X"),
	m_lphaser_y(*this, "LPHASER_Y")
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void sms_light_phaser_device::device_start()
{
	m_screen = machine().first_screen();
	m_lphaser_timer = timer_alloc(TIMER_LPHASER);
	m_last_state = 1;
}


//-------------------------------------------------
//  sms_peripheral_r - light phaser read
//-------------------------------------------------

UINT8 sms_light_phaser_device::peripheral_r()
{
	return m_lphaser_pins->read();
}


/*
    Light Phaser (light gun) emulation notes:
    - The sensor is activated based on color brightness of some individual
      pixels being drawn by the beam, at circular area where the gun is aiming.
    - Currently, brightness is calculated based only on single pixels.
    - In general, after the trigger is pressed, games draw the next frame using
      a light color pattern, to make sure sensor will be activated. If emulation
      skips that frame, sensor may stay deactivated. Frameskip set to 0 (no skip)
      is recommended to avoid problems.
    - When sensor switches from on (0) to off (1), a value is latched for the
      HCount register.
    - When sensor switches from off to on, a flag is set. The emulation uses the
      flag to signal that TH line is activated when the status of the input port
      is read. After read, the flag is cleared, or else it is cleared later when
      the Pause status is read (end of a frame). This is necessary because the
      "Color & Switch Test" ROM only reads the TH state after VINT occurs.
    - The gun test of "Color & Switch Test" is an example that requires checks
      of sensor status independent of other events, like trigger press or TH bit
      reads. Another example is the title screen of "Hang-On & Safari Hunt", where
      the game only reads HCount register in a loop, expecting a latch by the gun.
    - The whole procedure is managed by a timer callback, that always reschedule
      itself to run in some intervals when the beam is at the circular area.
*/
int sms_light_phaser_device::bright_aim_area( emu_timer *timer, int lgun_x, int lgun_y )
{
	const int r_x_r = LGUN_RADIUS * LGUN_RADIUS;
	const rectangle &visarea = m_screen->visible_area();
	int beam_x = m_screen->hpos();
	int beam_y = m_screen->vpos();
	int dx, dy;
	int result = 1;
	int pos_changed = 0;
	double dx_radius;

	while (1)
	{
		/* If beam's y isn't at a line where the aim area is, change it
		   the next line it enters that area. */
		dy = abs(beam_y - lgun_y);
		if (dy > LGUN_RADIUS || beam_y < visarea.min_y || beam_y > visarea.max_y)
		{
			beam_y = lgun_y - LGUN_RADIUS;
			if (beam_y < visarea.min_y)
				beam_y = visarea.min_y;
			dy = abs(beam_y - lgun_y);
			pos_changed = 1;
		}

		/* Caculate distance in x of the radius, relative to beam's y distance.
		   First try some shortcuts. */
		switch (dy)
		{
		case LGUN_RADIUS:
			dx_radius = 0;
			break;
		case 0:
			dx_radius = LGUN_RADIUS;
			break;
		default:
			/* step 1: r^2 = dx^2 + dy^2 */
			/* step 2: dx^2 = r^2 - dy^2 */
			/* step 3: dx = sqrt(r^2 - dy^2) */
			dx_radius = ceil((float) sqrt((float) (r_x_r - (dy * dy))));
		}

		/* If beam's x isn't in the circular aim area, change it
		   to the next point it enters that area. */
		dx = abs(beam_x - lgun_x);
		if (dx > dx_radius || beam_x < visarea.min_x || beam_x > visarea.max_x)
		{
			/* If beam's x has passed the aim area, advance to
			   next line and recheck y/x coordinates. */
			if (beam_x > lgun_x)
			{
				beam_x = 0;
				beam_y++;
				continue;
			}
			beam_x = lgun_x - dx_radius;
			if (beam_x < visarea.min_x)
				beam_x = visarea.min_x;
			pos_changed = 1;
		}

		if (!pos_changed)
		{
			rgb_t color;
			UINT8 brightness;
			/* brightness of the lightgray color in the frame drawn by Light Phaser games */
			const UINT8 sensor_min_brightness = 0x7f;

			if (m_last_state == 0) /* sensor is on */
			{
				/* keep sensor on until out of the aim area */
				result = 0;
				/* Set next check for when sensor will be off */
				beam_x = lgun_x + dx_radius + 1;
				if (beam_x > visarea.max_x)
					beam_x = visarea.max_x + 1;
				break;
			}

			color = m_port->pixel_r();

			/* reference: http://www.w3.org/TR/AERT#color-contrast */
			brightness = (RGB_RED(color) * 0.299) + (RGB_GREEN(color) * 0.587) + (RGB_BLUE(color) * 0.114);
			//printf ("color brightness: %2X for x %d y %d\n", brightness, beam_x, beam_y);

			result = (brightness >= sensor_min_brightness) ? 0 : 1;

			/* next check at same line */
			beam_x += LGUN_X_INTERVAL;
			pos_changed = 1;
		}
		else
			break;
	}
	timer->adjust(m_screen->time_until_pos(beam_y, beam_x));

	return result;
}


UINT16 sms_light_phaser_device::screen_hpos_nonscaled(int scaled_hpos)
{
	const rectangle &visarea = m_screen->visible_area();
	int offset_x = (scaled_hpos * (visarea.max_x - visarea.min_x)) / 255;
	return visarea.min_x + offset_x;
}


UINT16 sms_light_phaser_device::screen_vpos_nonscaled(int scaled_vpos)
{
	const rectangle &visarea = m_screen->visible_area();
	int offset_y = (scaled_vpos * (visarea.max_y - visarea.min_y)) / 255;
	return visarea.min_y + offset_y;
}


void sms_light_phaser_device::sensor_check()
{
	int new_state;

	const int x = screen_hpos_nonscaled(m_lphaser_x->read());
	const int y = screen_vpos_nonscaled(m_lphaser_y->read());

	new_state = bright_aim_area(m_lphaser_timer, x, y);
	if (new_state != m_last_state)
	{
		m_port->th_pin_w(new_state);
		m_last_state = new_state;
	}
}


void sms_light_phaser_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case TIMER_LPHASER:
		sensor_check();
		break;
	default:
		assert_always(FALSE, "Unknown id in sms_light_phaser_device::device_timer");
	}
}
