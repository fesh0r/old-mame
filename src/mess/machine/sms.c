#include "emu.h"
#include "crsshair.h"
#include "video/315_5124.h"
#include "sound/2413intf.h"
#include "imagedev/cartslot.h"
#include "machine/eeprom.h"
#include "includes/sms.h"

#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

#define CF_CODEMASTERS_MAPPER      0x001
#define CF_KOREAN_MAPPER           0x002
#define CF_KOREAN_ZEMINA_MAPPER    0x004
#define CF_KOREAN_NOBANK_MAPPER    0x008
#define CF_93C46_EEPROM            0x010
#define CF_ONCART_RAM              0x020
#define CF_GG_SMS_MODE             0x040
#define CF_KOREAN_ZEMINA_NEMESIS   0x080
#define CF_4PAK_MAPPER             0x100
#define CF_JANGGUN_MAPPER          0x200
#define CF_TVDRAW                  0x400
#define CF_MAPPER_BITS             ( CF_CODEMASTERS_MAPPER | CF_KOREAN_MAPPER | CF_KOREAN_ZEMINA_MAPPER | \
										CF_KOREAN_NOBANK_MAPPER | CF_4PAK_MAPPER | CF_JANGGUN_MAPPER )

#define LGUN_RADIUS           6
#define LGUN_X_INTERVAL       4


TIMER_CALLBACK_MEMBER(sms_state::rapid_fire_callback)
{
	m_rapid_fire_state_1 ^= 0xff;
	m_rapid_fire_state_2 ^= 0xff;
}


void sms_state::map_cart_16k( UINT16 address, UINT16 bank )
{
	switch ( address )
	{
	case 0x0000:
		map_cart_8k( 0x0000, bank * 2 );
		map_cart_8k( 0x2000, bank * 2 + 1 );
		break;

	case 0x0400:
		map_cart_8k( 0x0400, bank * 2 );
		map_cart_8k( 0x2000, bank * 2 + 1 );
		break;

	case 0x4000:
		map_cart_8k( 0x4000, bank * 2 );
		map_cart_8k( 0x6000, bank * 2 + 1 );
		break;

	case 0x8000:
		map_cart_8k( 0x8000, bank * 2 );
		map_cart_8k( 0xA000, bank * 2 + 1 );
		break;

	default:
		fatalerror("map_cart_16k: Unsupported map address %04x passed\n", address);
		break;
	}
}


void sms_state::map_cart_8k( UINT16 address, UINT16 bank )
{
	UINT8 *bank_start = m_banking_none;

	if ( m_cartridge[m_current_cartridge].ROM )
	{
		UINT16 rom_bank_count = m_cartridge[m_current_cartridge].size / 0x2000;
		bank_start = m_cartridge[m_current_cartridge].ROM + ((rom_bank_count > 0) ? bank % rom_bank_count : 0) * 0x2000;
	}

	switch ( address )
	{
	case 0x0000:
		m_banking_cart[1] = bank_start;
		m_banking_cart[2] = bank_start + 0x400;
		membank("bank1")->set_base(m_banking_cart[1]);
		membank("bank2")->set_base(m_banking_cart[2]);
		break;

	case 0x0400:
		m_banking_cart[2] = bank_start + 0x400;
		membank("bank2")->set_base(m_banking_cart[2]);
		break;

	case 0x2000:
		m_banking_cart[7] = bank_start;
		membank("bank7")->set_base(m_banking_cart[7]);
		break;

	case 0x4000:
		m_banking_cart[3] = bank_start;
		membank("bank3")->set_base(m_banking_cart[3]);
		break;

	case 0x6000:
		m_banking_cart[4] = bank_start;
		membank("bank4")->set_base(m_banking_cart[4]);
		break;

	case 0x8000:
		m_banking_cart[5] = bank_start;
		membank("bank5")->set_base(m_banking_cart[5]);
		break;

	case 0xA000:
		m_banking_cart[6] = bank_start;
		membank("bank6")->set_base(m_banking_cart[6]);
		break;

	default:
		fatalerror("map_cart_8k: Unsuppored map address %04x passed\n", address);
		break;
	}
}


void sms_state::map_bios_16k( UINT16 address, UINT16 bank)
{
	switch ( address )
	{
	case 0x0000:
		map_bios_8k( 0x0000, bank * 2 );
		map_bios_8k( 0x2000, bank * 2 + 1 );
		break;

	case 0x0400:
		map_bios_8k( 0x0400, bank * 2 );
		map_bios_8k( 0x2000, bank * 2 + 1 );
		break;

	case 0x4000:
		map_bios_8k( 0x4000, bank * 2 );
		map_bios_8k( 0x6000, bank * 2 + 1 );
		break;

	case 0x8000:
		map_bios_8k( 0x8000, bank * 2 );
		map_bios_8k( 0xA000, bank * 2 + 1 );
		break;

	default:
		fatalerror("map_bios_16k: Unsupported map address %04x passed\n", address);
		break;
	}
}


void sms_state::map_bios_8k( UINT16 address, UINT16 bank )
{
	UINT8 *bank_start = m_banking_none;

	if ( m_BIOS )
	{
		bank_start = m_BIOS + ((m_bios_page_count > 0) ? bank % ( m_bios_page_count << 1 ) : 0) * 0x2000;
	}

	switch ( address )
	{
	case 0x0000:
		m_banking_bios[1] = bank_start;
		membank("bank1")->set_base(m_banking_bios[1]);
		break;

	case 0x0400:
		if ( m_has_bios_0400 )
		{
			break;
		}
		m_banking_bios[2] = bank_start + 0x400;
		membank("bank2")->set_base(m_banking_bios[2]);
		break;

	case 0x2000:
		if ( m_has_bios_0400 || m_has_bios_2000 )
		{
			break;
		}
		m_banking_bios[7] = bank_start;
		membank("bank7")->set_base(m_banking_bios[7]);
		break;

	case 0x4000:
		if ( m_has_bios_0400 || m_has_bios_2000 )
		{
			break;
		}
		m_banking_bios[3] = bank_start;
		membank("bank3")->set_base(m_banking_bios[3]);
		break;

	case 0x6000:
		if ( m_has_bios_0400 || m_has_bios_2000 )
		{
			break;
		}
		m_banking_bios[4] = bank_start;
		membank("bank4")->set_base(m_banking_bios[4]);
		break;

	case 0x8000:
		if ( m_has_bios_0400 || m_has_bios_2000 )
		{
			break;
		}
		m_banking_bios[5] = bank_start;
		membank("bank5")->set_base(m_banking_bios[5]);
		break;

	case 0xA000:
		if ( m_has_bios_0400 || m_has_bios_2000 )
		{
			break;
		}
		m_banking_bios[6] = bank_start;
		membank("bank6")->set_base(m_banking_bios[6]);
		break;

	default:
		fatalerror("map_cart_8k: Unsuppored map address %04x passed\n", address);
		break;
	}
}


WRITE8_MEMBER(sms_state::sms_input_write)
{
	switch (offset)
	{
	case 0:
		switch (ioport("CTRLSEL")->read_safe(0x00) & 0x0f)
		{
		case 0x04:  /* Sports Pad */
			if (data != m_sports_pad_last_data_1)
			{
				UINT32 cpu_cycles = m_main_cpu->total_cycles();

				m_sports_pad_last_data_1 = data;
				if (cpu_cycles - m_last_sports_pad_time_1 > 512)
				{
					m_sports_pad_state_1 = 3;
					m_sports_pad_1_x = ioport("SPORT0")->read();
					m_sports_pad_1_y = ioport("SPORT1")->read();
				}
				m_last_sports_pad_time_1 = cpu_cycles;
				m_sports_pad_state_1 = (m_sports_pad_state_1 + 1) & 3;
			}
			break;
		}
		break;

	case 1:
		switch (ioport("CTRLSEL")->read_safe(0x00) & 0xf0)
		{
		case 0x40:  /* Sports Pad */
			if (data != m_sports_pad_last_data_2)
			{
				UINT32 cpu_cycles = m_main_cpu->total_cycles();

				m_sports_pad_last_data_2 = data;
				if (cpu_cycles - m_last_sports_pad_time_2 > 2048)
				{
					m_sports_pad_state_2 = 3;
					m_sports_pad_2_x = ioport("SPORT2")->read();
					m_sports_pad_2_y = ioport("SPORT3")->read();
				}
				m_last_sports_pad_time_2 = cpu_cycles;
				m_sports_pad_state_2 = (m_sports_pad_state_2 + 1) & 3;
			}
			break;
		}
		break;
	}
}


/*
    Light Phaser (light gun) emulation notes:
    - The sensor is activated based on color brightness of some individual
      pixels being drawn by the beam, at circular area where the gun is aiming.
    - Currently, brightness is calculated based only on single pixels.
    - In general, after the trigger is pressed, games draw the next frame using
      a light color pattern, to make sure sensor will be activated. If emulation
      skips that frame, sensor may stay deactivated. Frameskip set to 0 (no skip) is
      recommended to avoid problems.
    - When sensor switches from off to on, a value is latched for HCount register
      and a flag is set. The emulation uses the flag to avoid sequential latches and
      to signal that TH line is activated when the status of the input port is read.
      When the status is read, the flag is cleared, or else it is cleared later when
      the Pause status is read (end of a frame). This is necessary because the
      "Color & Switch Test" ROM only reads the TH bit after the VINT line.
    - The gun test of "Color & Switch Test" is an example that requires checks
      of sensor status independent of other events, like trigger press or TH bit
      reads. Another example is the title screen of "Hang-On & Safari Hunt", where
      the game only reads HCount register in a loop, expecting a latch by the gun.
    - The whole procedure is managed by a timer callback, that always reschedule
      itself to run in some intervals when the beam is at the circular area.
*/
int sms_state::lgun_bright_aim_area( emu_timer *timer, int lgun_x, int lgun_y )
{
	const int r_x_r = LGUN_RADIUS * LGUN_RADIUS;
	const rectangle &visarea = m_main_scr->visible_area();
	int beam_x = m_main_scr->hpos();
	int beam_y = m_main_scr->vpos();
	int dx, dy;
	int result = 0;
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
			bitmap_rgb32 &bitmap = m_vdp->get_bitmap();

			/* brightness of the lightgray color in the frame drawn by Light Phaser games */
			const UINT8 sensor_min_brightness = 0x7f;

			/* TODO: Check how Light Phaser behaves for border areas. For Gangster Town, should */
			/* a shot at right border (HC~=0x90) really appear at active scr, near to left border? */
			if (beam_x < SEGA315_5124_LBORDER_START + SEGA315_5124_LBORDER_WIDTH || beam_x >= SEGA315_5124_LBORDER_START + SEGA315_5124_LBORDER_WIDTH + 256)
				return 0;

			rgb_t color = bitmap.pix32(beam_y, beam_x);

			/* reference: http://www.w3.org/TR/AERT#color-contrast */
			UINT8 brightness = (RGB_RED(color) * 0.299) + (RGB_GREEN(color) * 0.587) + (RGB_BLUE(color) * 0.114);
			//printf ("color brightness: %2X for x %d y %d\n", brightness, beam_x, beam_y);

			result = (brightness >= sensor_min_brightness) ? 1 : 0;

			/* next check at same line */
			beam_x += LGUN_X_INTERVAL;
			pos_changed = 1;
		}
		else
			break;
	}
	timer->adjust(m_main_scr->time_until_pos(beam_y, beam_x));

	return result;
}


void sms_state::lphaser_hcount_latch( int hpos )
{
	/* A delay seems to occur when the Light Phaser latches the
	   VDP hcount, then an offset is added here to the hpos. */
	m_vdp->hcount_latch_at_hpos(hpos + m_lphaser_x_offs);
}


UINT16 sms_state::screen_hpos_nonscaled(int scaled_hpos)
{
	const rectangle &visarea = m_main_scr->visible_area();
	int offset_x = (scaled_hpos * (visarea.max_x - visarea.min_x)) / 255;
	return visarea.min_x + offset_x;
}


UINT16 sms_state::screen_vpos_nonscaled(int scaled_vpos)
{
	const rectangle &visarea = m_main_scr->visible_area();
	int offset_y = (scaled_vpos * (visarea.max_y - visarea.min_y)) / 255;
	return visarea.min_y + offset_y;
}


void sms_state::lphaser1_sensor_check()
{
	const int x = screen_hpos_nonscaled( ioport("LPHASER0")->read() );
	const int y = screen_vpos_nonscaled( ioport("LPHASER1")->read() );

	if (lgun_bright_aim_area(m_lphaser_1_timer, x, y))
	{
		if (m_lphaser_1_latch == 0)
		{
			m_lphaser_1_latch = 1;
			lphaser_hcount_latch(x);
		}
	}
}

void sms_state::lphaser2_sensor_check()
{
	const int x = screen_hpos_nonscaled( ioport("LPHASER2")->read() );
	const int y = screen_vpos_nonscaled( ioport("LPHASER3")->read() );

	if (lgun_bright_aim_area(m_lphaser_2_timer, x, y))
	{
		if (m_lphaser_2_latch == 0)
		{
			m_lphaser_2_latch = 1;
			lphaser_hcount_latch(x);
		}
	}
}


// at each input port read we check if lightguns are enabled in one of the ports:
// if so, we turn on crosshair and the lightgun timer
TIMER_CALLBACK_MEMBER(sms_state::lightgun_tick)
{
	if ((ioport("CTRLSEL")->read_safe(0x00) & 0x0f) == 0x01)
	{
		/* enable crosshair */
		crosshair_set_screen(machine(), 0, CROSSHAIR_SCREEN_ALL);
		if (!m_lphaser_1_timer->enabled())
			lphaser1_sensor_check();
	}
	else
	{
		/* disable crosshair */
		crosshair_set_screen(machine(), 0, CROSSHAIR_SCREEN_NONE);
		m_lphaser_1_timer->enable(0);
	}

	if ((ioport("CTRLSEL")->read_safe(0x00) & 0xf0) == 0x10)
	{
		/* enable crosshair */
		crosshair_set_screen(machine(), 1, CROSSHAIR_SCREEN_ALL);
		if (!m_lphaser_2_timer->enabled())
			lphaser2_sensor_check();
	}
	else
	{
		/* disable crosshair */
		crosshair_set_screen(machine(), 1, CROSSHAIR_SCREEN_NONE);
		m_lphaser_2_timer->enable(0);
	}
}


TIMER_CALLBACK_MEMBER(sms_state::lphaser_1_callback)
{
	lphaser1_sensor_check();
}


TIMER_CALLBACK_MEMBER(sms_state::lphaser_2_callback)
{
	lphaser2_sensor_check();
}


INPUT_CHANGED_MEMBER(sms_state::lgun1_changed)
{
	if (!m_lphaser_1_timer ||
		(ioport("CTRLSEL")->read_safe(0x00) & 0x0f) != 0x01)
		return;

	if (newval != oldval)
		lphaser1_sensor_check();
}

INPUT_CHANGED_MEMBER(sms_state::lgun2_changed)
{
	if (!m_lphaser_2_timer ||
		(ioport("CTRLSEL")->read_safe(0x00) & 0xf0) != 0x10)
		return;

	if (newval != oldval)
		lphaser2_sensor_check();
}


void sms_state::sms_get_inputs( address_space &space )
{
	UINT8 data = 0x00;
	UINT32 cpu_cycles = m_main_cpu->total_cycles();

	m_input_port0 = 0xff;
	m_input_port1 = 0xff;

	if (cpu_cycles - m_last_paddle_read_time > 256)
	{
		m_paddle_read_state ^= 0xff;
		m_last_paddle_read_time = cpu_cycles;
	}

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	machine().scheduler().timer_set(attotime::zero, timer_expired_delegate(FUNC(sms_state::lightgun_tick),this));

	/* Player 1 */
	switch (ioport("CTRLSEL")->read_safe(0x00) & 0x0f)
	{
	case 0x00:  /* Joystick */
		data = ioport("PORT_DC")->read();
		/* Check Rapid Fire setting for Button A */
		if (!(data & 0x10) && (ioport("RFU")->read() & 0x01))
			data |= m_rapid_fire_state_1 & 0x10;

		/* Check Rapid Fire setting for Button B */
		if (!(data & 0x20) && (ioport("RFU")->read() & 0x02))
			data |= m_rapid_fire_state_1 & 0x20;

		m_input_port0 = (m_input_port0 & 0xc0) | (data & 0x3f);
		break;

	case 0x01:  /* Light Phaser */
		data = (ioport("CTRLIPT")->read() & 0x01) << 4;
		/* Check Rapid Fire setting for Trigger */
		if (!(data & 0x10) && (ioport("RFU")->read() & 0x01))
			data |= m_rapid_fire_state_1 & 0x10;

		/* just consider the trigger (button) bit */
		data |= ~0x10;

		m_input_port0 = (m_input_port0 & 0xc0) | (data & 0x3f);
		break;

	case 0x02:  /* Paddle Control */
		/* Get button A state */
		data = ioport("PADDLE0")->read();

		if (m_paddle_read_state)
			data = data >> 4;

		m_input_port0 = (m_input_port0 & 0xc0) | (data & 0x0f) | (m_paddle_read_state & 0x20)
						| ((ioport("CTRLIPT")->read() & 0x02) << 3);
		break;

	case 0x04:  /* Sega Sports Pad */
		switch (m_sports_pad_state_1)
		{
		case 0:
			data = (m_sports_pad_1_x >> 4) & 0x0f;
			break;
		case 1:
			data = m_sports_pad_1_x & 0x0f;
			break;
		case 2:
			data = (m_sports_pad_1_y >> 4) & 0x0f;
			break;
		case 3:
			data = m_sports_pad_1_y & 0x0f;
			break;
		}
		m_input_port0 = (m_input_port0 & 0xc0) | data | ((ioport("CTRLIPT")->read() & 0x0c) << 2);
		break;
	}

	/* Player 2 */
	switch (ioport("CTRLSEL")->read_safe(0x00)  & 0xf0)
	{
	case 0x00:  /* Joystick */
		data = ioport("PORT_DC")->read();
		m_input_port0 = (m_input_port0 & 0x3f) | (data & 0xc0);

		data = ioport("PORT_DD")->read();
		/* Check Rapid Fire setting for Button A */
		if (!(data & 0x04) && (ioport("RFU")->read() & 0x04))
			data |= m_rapid_fire_state_2 & 0x04;

		/* Check Rapid Fire setting for Button B */
		if (!(data & 0x08) && (ioport("RFU")->read() & 0x08))
			data |= m_rapid_fire_state_2 & 0x08;

		m_input_port1 = (m_input_port1 & 0xf0) | (data & 0x0f);
		break;

	case 0x10:  /* Light Phaser */
		data = (ioport("CTRLIPT")->read() & 0x10) >> 2;
		/* Check Rapid Fire setting for Trigger */
		if (!(data & 0x04) && (ioport("RFU")->read() & 0x04))
			data |= m_rapid_fire_state_2 & 0x04;

		/* just consider the trigger (button) bit */
		data |= ~0x04;

		m_input_port1 = (m_input_port1 & 0xf0) | (data & 0x0f);
		break;

	case 0x20:  /* Paddle Control */
		/* Get button A state */
		data = ioport("PADDLE1")->read();
		if (m_paddle_read_state)
			data = data >> 4;

		m_input_port0 = (m_input_port0 & 0x3f) | ((data & 0x03) << 6);
		m_input_port1 = (m_input_port1 & 0xf0) | ((data & 0x0c) >> 2) | (m_paddle_read_state & 0x08)
						| ((ioport("CTRLIPT")->read() & 0x20) >> 3);
		break;

	case 0x40:  /* Sega Sports Pad */
		switch (m_sports_pad_state_2)
		{
		case 0:
			data = m_sports_pad_2_x & 0x0f;
			break;
		case 1:
			data = (m_sports_pad_2_x >> 4) & 0x0f;
			break;
		case 2:
			data = m_sports_pad_2_y & 0x0f;
			break;
		case 3:
			data = (m_sports_pad_2_y >> 4) & 0x0f;
			break;
		}
		m_input_port0 = (m_input_port0 & 0x3f) | ((data & 0x03) << 6);
		m_input_port1 = (m_input_port1 & 0xf0) | (data >> 2) | ((ioport("CTRLIPT")->read() & 0xc0) >> 4);
		break;
	}
}


WRITE8_MEMBER(sms_state::sms_fm_detect_w)
{
	if (m_has_fm)
		m_fm_detect = (data & 0x01);
}


READ8_MEMBER(sms_state::sms_fm_detect_r)
{
	if (m_has_fm)
	{
		return m_fm_detect;
	}
	else
	{
		if (m_bios_port & IO_CHIP)
		{
			return 0xff;
		}
		else
		{
			sms_get_inputs(space);
			return m_input_port0;
		}
	}
}

WRITE8_MEMBER(sms_state::sms_io_control_w)
{
	bool latch_hcount = false;

	if (data & 0x08)
	{
		/* check if TH pin level is high (1) and was low last time */
		if (data & 0x80 && !(m_ctrl_reg & 0x80))
		{
			latch_hcount = true;
		}
		sms_input_write(space, 0, (data & 0x20) >> 5);
	}

	if (data & 0x02)
	{
		if (data & 0x20 && !(m_ctrl_reg & 0x20))
		{
			latch_hcount = true;
		}
		sms_input_write(space, 1, (data & 0x80) >> 7);
	}

	if (latch_hcount)
	{
		m_vdp->hcount_latch();
	}

	m_ctrl_reg = data;
}


READ8_MEMBER(sms_state::sms_count_r)
{
	if (offset & 0x01)
		return m_vdp->hcount_read(*m_space, offset);
	else
		return m_vdp->vcount_read(*m_space, offset);
}


/*
 Check if the pause button is pressed.
 If the gamegear is in sms mode, check if the start button is pressed.
 */
WRITE_LINE_MEMBER(sms_state::sms_pause_callback)
{
	if (m_is_gamegear && !(m_cartridge[m_current_cartridge].features & CF_GG_SMS_MODE))
		return;

	if (!(ioport(m_is_gamegear ? "START" : "PAUSE")->read() & 0x80))
	{
		if (!m_paused)
		{
			m_main_cpu->set_input_line(INPUT_LINE_NMI, PULSE_LINE);
		}
		m_paused = 1;
	}
	else
	{
		m_paused = 0;
	}

	/* clear Light Phaser latch flags for next frame */
	m_lphaser_1_latch = 0;
	m_lphaser_2_latch = 0;
}

READ8_MEMBER(sms_state::sms_input_port_0_r)
{
	if (m_bios_port & IO_CHIP)
	{
		return 0xff;
	}
	else
	{
		sms_get_inputs(space);
		return m_input_port0;
	}
}


READ8_MEMBER(sms_state::sms_input_port_1_r)
{
	if (m_bios_port & IO_CHIP)
		return 0xff;

	sms_get_inputs(space);

	/* Reset Button */
	m_input_port1 = (m_input_port1 & 0xef) | (ioport("RESET")->read_safe(0x01) & 0x01) << 4;

	/* Do region detection if TH of ports A and B are set to output (0) */
	if (!(m_ctrl_reg & 0x0a))
	{
		/* Move bits 7,5 of IO control port into bits 7, 6 */
		m_input_port1 = (m_input_port1 & 0x3f) | (m_ctrl_reg & 0x80) | (m_ctrl_reg & 0x20) << 1;

		/* Inverse region detect value for Japanese machines */
		if (m_is_region_japan)
			m_input_port1 ^= 0xc0;
	}
	else
	{
		if (m_ctrl_reg & 0x02 && m_lphaser_1_latch)
		{
			m_input_port1 &= ~0x40;
			m_lphaser_1_latch = 0;
		}

		if (m_ctrl_reg & 0x08 && m_lphaser_2_latch)
		{
			m_input_port1 &= ~0x80;
			m_lphaser_2_latch = 0;
		}
	}

	return m_input_port1;
}



WRITE8_MEMBER(sms_state::sms_ym2413_register_port_0_w)
{
	if (m_has_fm)
		m_ym->write(space, 0, (data & 0x3f));
}


WRITE8_MEMBER(sms_state::sms_ym2413_data_port_0_w)
{
	if (m_has_fm)
	{
		logerror("data_port_0_w %x %x\n", offset, data);
		m_ym->write(space, 1, data);
	}
}


READ8_MEMBER(sms_state::gg_input_port_2_r)
{
	//logerror("joy 2 read, val: %02x, pc: %04x\n", ((m_is_region_japan ? 0x00 : 0x40) | (machine.root_device().ioport("START")->read() & 0x80)), activecpu_get_pc());
	return ((m_is_region_japan ? 0x00 : 0x40) | (ioport("START")->read() & 0x80));
}


READ8_MEMBER(sms_state::sms_sscope_r)
{
	int sscope = ioport("SEGASCOPE")->read_safe(0x00);

	if ( sscope )
	{
		// Scope is attached
		return m_sscope_state;
	}

	return m_mainram[0x1FF8 + offset];
}


WRITE8_MEMBER(sms_state::sms_sscope_w)
{
	m_mainram[0x1FF8 + offset] = data;

	int sscope = ioport("SEGASCOPE")->read_safe(0x00);

	if ( sscope )
	{
		// Scope is attached
		m_sscope_state = data;

		// There are occurrences when Sega Scope's state changes after VBLANK, or at
		// active screen. Most cases are solid-color frames of scene transitions, but
		// one exception is the first frame of Zaxxon 3-D's title screen. In that
		// case, this method is enough for setting the intended state for the frame.
		// No information found about a minimum time need for switch open/closed lens.
		if (m_main_scr->vpos() < (m_main_scr->height() >> 1))
		{
			m_frame_sscope_state = m_sscope_state;
		}
	}
}


READ8_MEMBER(sms_state::sms_mapper_r)
{
	return m_mainram[0x1FFC + offset];
}

/* Terebi Oekaki */
/* The following code comes from sg1000.c. We should eventually merge these TV Draw implementations */
WRITE8_MEMBER(sms_state::sms_tvdraw_axis_w)
{
	UINT8 tvboard_on = ioport("TVDRAW")->read_safe(0x00);

	if (data & 0x01)
	{
		m_cartridge[m_current_cartridge].m_tvdraw_data = tvboard_on ? ioport("TVDRAW_X")->read() : 0x80;

		if (m_cartridge[m_current_cartridge].m_tvdraw_data < 4) m_cartridge[m_current_cartridge].m_tvdraw_data = 4;
		if (m_cartridge[m_current_cartridge].m_tvdraw_data > 251) m_cartridge[m_current_cartridge].m_tvdraw_data = 251;
	}
	else
	{
		m_cartridge[m_current_cartridge].m_tvdraw_data = tvboard_on ? ioport("TVDRAW_Y")->read() + 0x20 : 0x80;
	}
}

READ8_MEMBER(sms_state::sms_tvdraw_status_r)
{
	UINT8 tvboard_on = ioport("TVDRAW")->read_safe(0x00);
	return tvboard_on ? ioport("TVDRAW_PEN")->read() : 0x01;
}

READ8_MEMBER(sms_state::sms_tvdraw_data_r)
{
	return m_cartridge[m_current_cartridge].m_tvdraw_data;
}


WRITE8_MEMBER(sms_state::sms_93c46_w)
{
	if ( m_cartridge[m_current_cartridge].m_93c46_enabled )
	{
		m_cartridge[m_current_cartridge].m_93c46_lines = data;

		logerror( "sms_93c46_w: setting eeprom lines: DI=%s CLK=%s CS=%s\n", data & 0x01 ? "1" : "0", data & 0x02 ? "1" : "0", data & 0x04 ? "1" : "0" );
		m_eeprom->write_bit( ( data & 0x01 ) ? ASSERT_LINE : CLEAR_LINE );
		m_eeprom->set_cs_line( !( data & 0x04 ) ? ASSERT_LINE : CLEAR_LINE );
		m_eeprom->set_clock_line( ( data & 0x02 ) ? ASSERT_LINE : CLEAR_LINE );
	}
}


READ8_MEMBER(sms_state::sms_93c46_r)
{
	UINT8 data = m_banking_cart[5][0];

	if ( m_cartridge[m_current_cartridge].m_93c46_enabled )
	{
		data = ( m_cartridge[m_current_cartridge].m_93c46_lines & 0xFC ) | 0x02;
		data |= m_eeprom->read_bit() ? 1 : 0;
	}

	return data;
}


WRITE8_MEMBER(sms_state::sms_mapper_w)
{
	bool bios_selected = false;
	bool cartridge_selected = false;

	offset &= 3;

	m_mapper[offset] = data;
	m_mainram[0x1FFC + offset] = data;

	if (m_bios_port & IO_BIOS_ROM || (m_is_gamegear && m_BIOS == NULL))
	{
		if (!(m_bios_port & IO_CARTRIDGE) || (m_is_gamegear && m_BIOS == NULL))
		{
			if (!m_cartridge[m_current_cartridge].ROM)
				return;
			cartridge_selected = true;
		}
		else
		{
			/* nothing to page in */
			return;
		}
	}
	else
	{
		if (!m_BIOS)
			return;
		bios_selected = true;
	}

	switch (offset)
	{
	case 0: /* Control */
		/* Is it ram or rom? */
		if (data & 0x08) /* it's ram */
		{
			if ( m_cartridge[m_current_cartridge].features & CF_93C46_EEPROM )
			{
				if ( data & 0x80 )
				{
					m_eeprom->reset();
					logerror("sms_mapper_w: eeprom CS=1\n");
					m_eeprom->set_cs_line( ASSERT_LINE );
				}
				logerror("sms_mapper_w: eeprom enabled\n");
				m_cartridge[m_current_cartridge].m_93c46_enabled = true;
			}
			else
			{
				UINT8 *sram = NULL;
				m_cartridge[m_current_cartridge].sram_save = 1;         /* SRAM should be saved on exit. */
				if (data & 0x04)
				{
					sram = m_cartridge[m_current_cartridge].cartSRAM + 0x4000;
				}
				else
				{
					sram = m_cartridge[m_current_cartridge].cartSRAM;
				}
				membank("bank5")->set_base(sram);
				membank("bank6")->set_base(sram + 0x2000);
			}
		}
		else /* it's rom */
		{
			if (m_bios_port & IO_BIOS_ROM || ! m_has_bios)
			{
				if ( ! ( m_cartridge[m_current_cartridge].features & ( CF_KOREAN_NOBANK_MAPPER | CF_KOREAN_ZEMINA_MAPPER ) ) )
				{
					if ( m_cartridge[m_current_cartridge].features & CF_93C46_EEPROM )
					{
						if ( data & 0x80 )
						{
							m_eeprom->reset();
							logerror("sms_mapper_w: eeprom CS=1\n");
							m_eeprom->set_cs_line( ASSERT_LINE );
						}
						logerror("sms_mapper_w: eeprom disabled\n");
						m_cartridge[m_current_cartridge].m_93c46_enabled = false;
					}
					else
					{
						map_cart_16k( 0x8000, m_mapper[3] );
					}
				}
			}
			else
			{
				map_bios_16k( 0x8000, m_mapper[3] );
			}
		}
		break;

	case 1: /* Select 16k ROM bank for 0400-3FFF */
		if ( cartridge_selected || m_is_gamegear )
		{
			if ( ! ( m_cartridge[m_current_cartridge].features & ( CF_KOREAN_NOBANK_MAPPER | CF_KOREAN_ZEMINA_MAPPER ) ) )
			{
				map_cart_16k( 0x400, data );
			}
		}
		if ( bios_selected )
		{
			map_bios_16k( 0x400, data );
		}
		break;

	case 2: /* Select 16k ROM bank for 4000-7FFF */
		if ( cartridge_selected || m_is_gamegear )
		{
			if ( ! ( m_cartridge[m_current_cartridge].features & ( CF_KOREAN_NOBANK_MAPPER | CF_KOREAN_ZEMINA_MAPPER ) ) )
			{
				map_cart_16k( 0x4000, data );
			}
		}
		if ( bios_selected )
		{
			map_bios_16k( 0x4000, data );
		}
		break;

	case 3: /* Select 16k ROM bank for 8000-BFFF */
		if ( cartridge_selected || m_is_gamegear )
		{
			if ( m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER)
			{
				return;
			}

			if ( ! ( m_mapper[0] & 0x08 ) )     // Is RAM disabled
			{
				if ( ! ( m_cartridge[m_current_cartridge].features & ( CF_KOREAN_NOBANK_MAPPER | CF_KOREAN_ZEMINA_MAPPER ) ) )
				{
					map_cart_16k( 0x8000, data );
				}
			}
		}

		if ( bios_selected )
		{
			if ( ! ( m_mapper[0] & 0x08 ) )     // Is RAM disabled
			{
				map_bios_16k( 0x8000, data );
			}
		}
		break;
	}
}

WRITE8_MEMBER(sms_state::sms_korean_zemina_banksw_w)
{
	if (m_cartridge[m_current_cartridge].features & CF_KOREAN_ZEMINA_MAPPER)
	{
		if (!m_cartridge[m_current_cartridge].ROM)
			return;

		switch (offset & 3)
		{
			case 0:
				map_cart_8k( 0x8000, data );
				break;
			case 1:
				map_cart_8k( 0xA000, data );
				break;
			case 2:
				map_cart_8k( 0x4000, data );
				break;
			case 3:
				map_cart_8k( 0x6000, data );
				break;
		}
		LOG(("Zemina mapper write: offset %x data %x.\n", offset, data));
	}
}

WRITE8_MEMBER(sms_state::sms_codemasters_page0_w)
{
	if (m_cartridge[m_current_cartridge].ROM && m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		map_cart_16k( 0x0000, data );
	}
}


WRITE8_MEMBER(sms_state::sms_codemasters_page1_w)
{
	if (m_cartridge[m_current_cartridge].ROM && m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		/* Check if we need to switch in some RAM */
		if (data & 0x80)
		{
			m_cartridge[m_current_cartridge].ram_page = data & 0x07;
			membank("bank6")->set_base(m_cartridge[m_current_cartridge].cartRAM + m_cartridge[m_current_cartridge].ram_page * 0x2000);
		}
		else
		{
			map_cart_16k( 0x4000, data );
			membank("bank6")->set_base(m_banking_cart[5] + 0x2000);
		}
	}
}


WRITE8_MEMBER(sms_state::sms_4pak_page0_w)
{
	m_cartridge[m_current_cartridge].m_4pak_page0 = data;

	map_cart_16k( 0x0000, data );
	map_cart_16k( 0x8000, ( m_cartridge[m_current_cartridge].m_4pak_page0 & 0x30 ) + m_cartridge[m_current_cartridge].m_4pak_page2 );
}


WRITE8_MEMBER(sms_state::sms_4pak_page1_w)
{
	m_cartridge[m_current_cartridge].m_4pak_page1 = data;

	map_cart_16k( 0x4000, data );
}


WRITE8_MEMBER(sms_state::sms_4pak_page2_w)
{
	m_cartridge[m_current_cartridge].m_4pak_page2 = data;

	map_cart_16k( 0x8000, ( m_cartridge[m_current_cartridge].m_4pak_page0 & 0x30 ) + m_cartridge[m_current_cartridge].m_4pak_page2 );
}


WRITE8_MEMBER(sms_state::sms_janggun_bank0_w)
{
	map_cart_8k( 0x4000, data );
}


WRITE8_MEMBER(sms_state::sms_janggun_bank1_w)
{
	map_cart_8k( 0x6000, data );
}


WRITE8_MEMBER(sms_state::sms_janggun_bank2_w)
{
	map_cart_8k( 0x8000, data );
}


WRITE8_MEMBER(sms_state::sms_janggun_bank3_w)
{
	map_cart_8k( 0xA000, data );
}


WRITE8_MEMBER(sms_state::sms_bios_w)
{
	m_bios_port = data;

	logerror("bios write %02x, pc: %04x\n", data, space.device().safe_pc());

	setup_rom();
}


WRITE8_MEMBER(sms_state::sms_cartram2_w)
{
	if (m_mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset + 0x2000);
		if (m_mapper[0] & 0x04)
		{
			m_cartridge[m_current_cartridge].cartSRAM[offset + 0x6000] = data;
		}
		else
		{
			m_cartridge[m_current_cartridge].cartSRAM[offset + 0x2000] = data;
		}
	}

	if (m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER)
	{
		m_cartridge[m_current_cartridge].cartRAM[m_cartridge[m_current_cartridge].ram_page * 0x2000 + offset] = data;
	}

	if (m_cartridge[m_current_cartridge].features & CF_KOREAN_MAPPER && offset == 0) /* Dodgeball King mapper */
	{
		map_cart_16k( 0x8000, data );
	}
}


WRITE8_MEMBER(sms_state::sms_cartram_w)
{
	int page;

	if (m_mapper[0] & 0x08)
	{
		logerror("write %02X to cartram at offset #%04X\n", data, offset);
		if (m_mapper[0] & 0x04)
		{
			m_cartridge[m_current_cartridge].cartSRAM[offset + 0x4000] = data;
		}
		else
		{
			m_cartridge[m_current_cartridge].cartSRAM[offset] = data;
		}
	}
	else
	{
		if (m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER && offset == 0) /* Codemasters mapper */
		{
			UINT8 rom_page_count = m_cartridge[m_current_cartridge].size / 0x4000;
			page = (rom_page_count > 0) ? data % rom_page_count : 0;
			if (!m_cartridge[m_current_cartridge].ROM)
				return;
			m_banking_cart[5] = m_cartridge[m_current_cartridge].ROM + page * 0x4000;
			membank("bank5")->set_base(m_banking_cart[5]);
			membank("bank6")->set_base(m_banking_cart[5] + 0x2000);
			LOG(("rom 2 paged in %x (Codemasters mapper).\n", page));
		}
		else if (m_cartridge[m_current_cartridge].features & CF_ONCART_RAM)
		{
			m_cartridge[m_current_cartridge].cartRAM[offset & (m_cartridge[m_current_cartridge].ram_size - 1)] = data;
		}
		else
		{
			logerror("INVALID write %02X to cartram at offset #%04X\n", data, offset);
		}
	}
}


WRITE8_MEMBER(sms_state::gg_sio_w)
{
	logerror("*** write %02X to SIO register #%d\n", data, offset);

	m_gg_sio[offset & 0x07] = data;
	switch (offset & 7)
	{
		case 0x00: /* Parallel Data */
			break;

		case 0x01: /* Data Direction/ NMI Enable */
			break;

		case 0x02: /* Serial Output */
			break;

		case 0x03: /* Serial Input */
			break;

		case 0x04: /* Serial Control / Status */
			break;
	}
}


READ8_MEMBER(sms_state::gg_sio_r)
{
	logerror("*** read SIO register #%d\n", offset);

	switch (offset & 7)
	{
		case 0x00: /* Parallel Data */
			break;

		case 0x01: /* Data Direction/ NMI Enable */
			break;

		case 0x02: /* Serial Output */
			break;

		case 0x03: /* Serial Input */
			break;

		case 0x04: /* Serial Control / Status */
			break;
	}

	return m_gg_sio[offset];
}

void sms_state::sms_machine_stop()
{
	/* Does the cartridge have SRAM that should be saved? */
	if (m_cartridge[m_current_cartridge].sram_save) {
		device_image_interface *image = dynamic_cast<device_image_interface *>(machine().device("cart1"));
		image->battery_save(m_cartridge[m_current_cartridge].cartSRAM, sizeof(UINT8) * NVRAM_SIZE );
	}
}


void sms_state::setup_rom()
{
	/* 1. set up bank pointers to point to nothing */
	membank("bank1")->set_base(m_banking_none);
	membank("bank2")->set_base(m_banking_none);
	membank("bank7")->set_base(m_banking_none);
	membank("bank3")->set_base(m_banking_none);
	membank("bank4")->set_base(m_banking_none);
	membank("bank5")->set_base(m_banking_none);
	membank("bank6")->set_base(m_banking_none);

	/* 2. check and set up expansion port */
	if (!(m_bios_port & IO_EXPANSION) && (m_bios_port & IO_CARTRIDGE) && (m_bios_port & IO_CARD))
	{
		/* TODO: Implement me */
		logerror("Switching to unsupported expansion port.\n");
	}

	/* 3. check and set up card rom */
	if (!(m_bios_port & IO_CARD) && (m_bios_port & IO_CARTRIDGE) && (m_bios_port & IO_EXPANSION))
	{
		/* TODO: Implement me */
		logerror("Switching to unsupported card rom port.\n");
	}

	/* 4. check and set up cartridge rom */
	/* if ((!(bios_port & IO_CARTRIDGE) && (bios_port & IO_EXPANSION) && (bios_port & IO_CARD)) || state->m_is_gamegear) { */
	/* Out Run Europa initially writes a value to port 3E where IO_CARTRIDGE, IO_EXPANSION and IO_CARD are reset */
	if ((!(m_bios_port & IO_CARTRIDGE)) || m_is_gamegear)
	{
		membank("bank1")->set_base(m_banking_cart[1]);
		membank("bank2")->set_base(m_banking_cart[2]);
		membank("bank7")->set_base(m_banking_cart[7]);
		membank("bank3")->set_base(m_banking_cart[3]);
		membank("bank4")->set_base(m_banking_cart[3] + 0x2000);
		membank("bank5")->set_base(m_banking_cart[5]);
		membank("bank6")->set_base(m_banking_cart[5] + 0x2000);
		logerror("Switched in cartridge rom.\n");
	}

	/* 5. check and set up bios rom */
	if (!(m_bios_port & IO_BIOS_ROM))
	{
		/* 0x0400 bioses */
		if (m_has_bios_0400)
		{
			membank("bank1")->set_base(m_banking_bios[1]);
			logerror("Switched in 0x0400 bios.\n");
		}
		/* 0x2000 bioses */
		if (m_has_bios_2000)
		{
			membank("bank1")->set_base(m_banking_bios[1]);
			membank("bank2")->set_base(m_banking_bios[2]);
			logerror("Switched in 0x2000 bios.\n");
		}
		if (m_has_bios_full)
		{
			membank("bank1")->set_base(m_banking_bios[1]);
			membank("bank2")->set_base(m_banking_bios[2]);
			membank("bank7")->set_base(m_banking_bios[7]);
			membank("bank3")->set_base(m_banking_bios[3]);
			membank("bank4")->set_base(m_banking_bios[3] + 0x2000);
			membank("bank5")->set_base(m_banking_bios[5]);
			membank("bank6")->set_base(m_banking_bios[5] + 0x2000);
			logerror("Switched in full bios.\n");
		}
	}

	if (m_cartridge[m_current_cartridge].features & CF_ONCART_RAM)
	{
		membank("bank5")->set_base(m_cartridge[m_current_cartridge].cartRAM);
		membank("bank6")->set_base(m_cartridge[m_current_cartridge].cartRAM);
	}
}


int sms_state::sms_verify_cart( UINT8 *magic, int size )
{
	int retval;

	retval = IMAGE_VERIFY_FAIL;

	/* Verify the file is a valid image - check $7ff0 for "TMR SEGA" */
	if (size >= 0x8000)
	{
		if (!strncmp((char*)&magic[0x7ff0], "TMR SEGA", 8))
		{
			retval = IMAGE_VERIFY_PASS;
		}

	}

	return retval;
}

#ifdef UNUSED_FUNCTION
// For the moment we switch to a different detection routine which allows to detect
// in a single run Codemasters mapper, Korean mapper (including Jang Pung 3 which
// uses a diff signature then the one below here) and Zemina mapper (used by Wonsiin, etc.).
// I leave these here to document alt. detection routines and in the case these functions
// can be updated

/* Check for Codemasters mapper
  0x7FE3 - 93 - sms Cosmis Spacehead
              - sms Dinobasher
              - sms The Excellent Dizzy Collection
              - sms Fantastic Dizzy
              - sms Micro Machines
              - gamegear Cosmic Spacehead
              - gamegear Micro Machines
         - 94 - gamegear Dropzone
              - gamegear Ernie Els Golf (also has 64KB additional RAM on the cartridge)
              - gamegear Pete Sampras Tennis
              - gamegear S.S. Lucifer
         - 95 - gamegear Micro Machines 2 - Turbo Tournament

The Korean game Jang Pung II also seems to use a codemasters style mapper.
 */
int sms_state::detect_codemasters_mapper( UINT8 *rom )
{
	static const UINT8 jang_pung2[16] = { 0x00, 0xba, 0x38, 0x0d, 0x00, 0xb8, 0x38, 0x0c, 0x00, 0xb6, 0x38, 0x0b, 0x00, 0xb4, 0x38, 0x0a };

	if (((rom[0x7fe0] & 0x0f ) <= 9) && (rom[0x7fe3] == 0x93 || rom[0x7fe3] == 0x94 || rom[0x7fe3] == 0x95) &&  rom[0x7fef] == 0x00)
		return 1;

	if (!memcmp(&rom[0x7ff0], jang_pung2, 16))
		return 1;

	return 0;
}


int sms_state::detect_korean_mapper( UINT8 *rom )
{
	static const UINT8 signatures[2][16] =
	{
		{ 0x3e, 0x11, 0x32, 0x00, 0xa0, 0x78, 0xcd, 0x84, 0x85, 0x3e, 0x02, 0x32, 0x00, 0xa0, 0xc9, 0xff }, /* Dodgeball King */
		{ 0x41, 0x48, 0x37, 0x37, 0x44, 0x37, 0x4e, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20 }, /* Sangokushi 3 */
	};
	int i;

	for (i = 0; i < 2; i++)
	{
		if (!memcmp(&rom[0x7ff0], signatures[i], 16))
		{
			return 1;
		}
	}
	return 0;
}
#endif


int sms_state::detect_tvdraw( UINT8 *rom )
{
	static const UINT8 terebi_oekaki[7] = { 0x61, 0x6e, 0x6e, 0x61, 0x6b, 0x6d, 0x6e }; // "annakmn"

	if (!memcmp(&rom[0x13b3], terebi_oekaki, 7))
		return 1;

	return 0;
}


int sms_state::detect_lphaser_xoffset( UINT8 *rom )
{
	static const UINT8 signatures[6][16] =
	{
		/* Spacegun */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0xff, 0xff, 0x9d, 0x99, 0x10, 0x90, 0x00, 0x40 },
		/* Gangster Town */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x19, 0x87, 0x1b, 0xc9, 0x74, 0x50, 0x00, 0x4f },
		/* Shooting Gallery */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x20, 0x20, 0x8a, 0x3a, 0x72, 0x50, 0x00, 0x4f },
		/* Rescue Mission */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x20, 0x20, 0xfb, 0xd3, 0x06, 0x51, 0x00, 0x4f },
		/* Laser Ghost */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0x00, 0x00, 0xb7, 0x55, 0x74, 0x70, 0x00, 0x40 },
		/* Assault City */
		{ 0x54, 0x4d, 0x52, 0x20, 0x53, 0x45, 0x47, 0x41, 0xff, 0xff, 0x9f, 0x74, 0x34, 0x70, 0x00, 0x40 },
	};

	if (!(m_bios_port & IO_CARTRIDGE) && m_cartridge[m_current_cartridge].size >= 0x8000)
	{
		if (!memcmp(&rom[0x7ff0], signatures[0], 16) || !memcmp(&rom[0x7ff0], signatures[1], 16))
			return 41;

		if (!memcmp(&rom[0x7ff0], signatures[2], 16))
			return 50;

		if (!memcmp(&rom[0x7ff0], signatures[3], 16))
			return 48;

		if (!memcmp(&rom[0x7ff0], signatures[4], 16))
			return 45;

		if (!memcmp(&rom[0x7ff0], signatures[5], 16))
			return 54;

	}
	return 51;
}


void sms_state::setup_sms_cart()
{
	int i;

	for (i = 0; i < MAX_CARTRIDGES; i++)
	{
		m_cartridge[i].ROM = NULL;
		m_cartridge[i].size = 0;
		m_cartridge[i].features = 0;
		m_cartridge[i].cartSRAM = NULL;
		m_cartridge[i].sram_save = 0;
		m_cartridge[i].cartRAM = NULL;
		m_cartridge[i].ram_size = 0;
		m_cartridge[i].ram_page = 0;
	}
	m_current_cartridge = 0;

	m_bios_port = (IO_EXPANSION | IO_CARTRIDGE | IO_CARD);
	if (!m_is_gamegear && !m_has_bios)
	{
		m_bios_port &= ~(IO_CARTRIDGE);
		m_bios_port |= IO_BIOS_ROM;
	}
}


DEVICE_IMAGE_LOAD_MEMBER( sms_state, sms_cart )
{
	int size, index = 0, offset = 0;

	if (strcmp(image.device().tag(), ":cart1") == 0)
		index = 0;
	if (strcmp(image.device().tag(), ":cart2") == 0)
		index = 1;
	if (strcmp(image.device().tag(), ":cart3") == 0)
		index = 2;
	if (strcmp(image.device().tag(), ":cart4") == 0)
		index = 3;
	if (strcmp(image.device().tag(), ":cart5") == 0)
		index = 4;
	if (strcmp(image.device().tag(), ":cart6") == 0)
		index = 5;
	if (strcmp(image.device().tag(), ":cart7") == 0)
		index = 6;
	if (strcmp(image.device().tag(), ":cart8") == 0)
		index = 7;
	if (strcmp(image.device().tag(), ":cart9") == 0)
		index = 8;
	if (strcmp(image.device().tag(), ":cart10") == 0)
		index = 9;
	if (strcmp(image.device().tag(), ":cart11") == 0)
		index = 10;
	if (strcmp(image.device().tag(), ":cart12") == 0)
		index = 11;
	if (strcmp(image.device().tag(), ":cart13") == 0)
		index = 12;
	if (strcmp(image.device().tag(), ":cart14") == 0)
		index = 13;
	if (strcmp(image.device().tag(), ":cart15") == 0)
		index = 14;
	if (strcmp(image.device().tag(), ":cart16") == 0)
		index = 15;

	m_cartridge[index].features = 0;

	if (image.software_entry() == NULL)
	{
		size = image.length();
	}
	else
		size = image.get_software_region_length("rom");

	/* Check for 512-byte header */
	if ((size / 512) & 1)
	{
		offset = 512;
		size -= 512;
	}

	if (!size)
	{
		image.seterror(IMAGE_ERROR_UNSPECIFIED, "Invalid ROM image: ROM image is too small");
		return IMAGE_INIT_FAIL;
	}

	/* Create a new memory region to hold the ROM. */
	/* Make sure the region holds only complete (0x4000) rom banks */
	m_cartridge[index].size = (size & 0x3fff) ? (((size >> 14) + 1) << 14) : size;
	m_cartridge[index].ROM = auto_alloc_array(machine(), UINT8, m_cartridge[index].size);
	m_cartridge[index].cartSRAM = auto_alloc_array(machine(), UINT8, NVRAM_SIZE);

	/* Load ROM banks */
	if (image.software_entry() == NULL)
	{
		image.fseek(offset, SEEK_SET);

		if (image.fread( m_cartridge[index].ROM, size) != size)
			return IMAGE_INIT_FAIL;
	}
	else
	{
		memcpy(m_cartridge[index].ROM, image.get_software_region("rom") + offset, size);

		const char *pcb = image.get_feature("pcb");
		const char *mapper = image.get_feature("mapper");
		const char *pin_42 = image.get_feature("pin_42");
		const char *eeprom = image.get_feature("eeprom");

		// Check for special mappers (or lack of mappers)
		if ( pcb && !strcmp(pcb, "korean_nobank"))
		{
			m_cartridge[index].features |= CF_KOREAN_NOBANK_MAPPER;
		}

		if ( mapper )
		{
			if ( ! strcmp( mapper, "codemasters" ) )
			{
				m_cartridge[index].features |= CF_CODEMASTERS_MAPPER;
			}
			else if ( ! strcmp( mapper, "korean" ) )
			{
				m_cartridge[index].features |= CF_KOREAN_MAPPER;
			}
			else if ( ! strcmp( mapper, "zemina" ) )
			{
				m_cartridge[index].features |= CF_KOREAN_ZEMINA_MAPPER;
			}
			else if ( ! strcmp( mapper, "nemesis" ) )
			{
				m_cartridge[index].features |= CF_KOREAN_ZEMINA_MAPPER;
				m_cartridge[index].features |= CF_KOREAN_ZEMINA_NEMESIS;
			}
			else if ( ! strcmp( mapper, "4pak" ) )
			{
				m_cartridge[index].features |= CF_4PAK_MAPPER;
			}
			else if ( ! strcmp( mapper, "janggun" ) )
			{
				m_cartridge[index].features |= CF_JANGGUN_MAPPER;
			}
		}

		// Check for gamegear cartridges with PIN 42 set to SMS mode
		if ( pin_42 && ! strcmp(pin_42, "sms_mode"))
		{
			m_cartridge[index].features |= CF_GG_SMS_MODE;
		}

		// Check for presence of 93c46 eeprom
		if ( eeprom && ! strcmp( eeprom, "93c46" ) )
		{
			m_cartridge[index].features |= CF_93C46_EEPROM;
		}
	}

	/* check the image */
	if (!m_has_bios)
	{
		if (sms_verify_cart(m_cartridge[index].ROM, size) == IMAGE_VERIFY_FAIL)
		{
			logerror("Warning loading image: sms_verify_cart failed\n");
		}
	}

	// If no mapper bits are set attempt to autodetect the mapper
	if ( ! ( m_cartridge[index].features & CF_MAPPER_BITS ) )
	{
		/* If no extrainfo information is available try to find special information out on our own */
		/* Check for special cartridge features (new routine, courtesy of Omar Cornut, from MEKA)  */
		if (size >= 0x8000)
		{
			int c0002 = 0, c8000 = 0, cA000 = 0, cFFFF = 0, c3FFE = 0, c4000 = 0, c6000 = 0, i;
			for (i = 0; i < 0x8000; i++)
			{
				if (m_cartridge[index].ROM[i] == 0x32) // Z80 opcode for: LD (xxxx), A
				{
					UINT16 addr = (m_cartridge[index].ROM[i + 2] << 8) | m_cartridge[index].ROM[i + 1];
					if (addr == 0xFFFF)
					{ i += 2; cFFFF++; continue; }
					if (addr == 0x0002 || addr == 0x0003 || addr == 0x0004)
					{ i += 2; c0002++; continue; }
					if (addr == 0x8000)
					{ i += 2; c8000++; continue; }
					if (addr == 0xA000)
					{ i += 2; cA000++; continue; }
					if ( addr == 0x3FFE)
					{ i += 2; c3FFE++; continue; }
					if ( addr == 0x4000 )
					{ i += 2; c4000++; continue; }
					if ( addr == 0x6000 )
					{ i += 2; c6000++; continue; }
				}
			}

			LOG(("Mapper test: c002 = %d, c8000 = %d, cA000 = %d, cFFFF = %d\n", c0002, c8000, cA000, cFFFF));

			// 2 is a security measure, although tests on existing ROM showed it was not needed
			if (c0002 > cFFFF + 2 || (c0002 > 0 && cFFFF == 0))
			{
				UINT8 *rom = m_cartridge[index].ROM;

				m_cartridge[index].features |= CF_KOREAN_ZEMINA_MAPPER;
				// Check for special bank 0 signature
				if ( size == 0x20000 && rom[0] == 0x00 && rom[1] == 0x00 && rom[2] == 0x00 &&
						rom[0x1e000] == 0xF3 && rom[0x1e001] == 0xED && rom[0x1e002] == 0x56 )
				{
					m_cartridge[index].features |= CF_KOREAN_ZEMINA_NEMESIS;
				}
			}
			else if (c8000 > cFFFF + 2 || (c8000 > 0 && cFFFF == 0))
			{
				m_cartridge[index].features |= CF_CODEMASTERS_MAPPER;
			}
			else if (cA000 > cFFFF + 2 || (cA000 > 0 && cFFFF == 0))
			{
				m_cartridge[index].features |= CF_KOREAN_MAPPER;
			}
			else if ( c3FFE > cFFFF + 2 || (c3FFE > 0) )
			{
				m_cartridge[index].features |= CF_4PAK_MAPPER;
			}
			else if ( c4000 > 0 && c6000 > 0 && c8000 > 0 && cA000 > 0 )
			{
				m_cartridge[index].features |= CF_JANGGUN_MAPPER;
			}
		}
	}

	if (m_cartridge[index].features & CF_CODEMASTERS_MAPPER)
	{
		m_cartridge[index].ram_size = 0x10000;
		m_cartridge[index].cartRAM = auto_alloc_array(machine(), UINT8, m_cartridge[index].ram_size);
		m_cartridge[index].ram_page = 0;
	}

	/* For Light Phaser games, we have to detect the x offset */
	m_lphaser_x_offs = detect_lphaser_xoffset(m_cartridge[index].ROM);

	/* Terebi Oekaki (TV Draw) is a SG1000 game with special input device which is compatible with SG1000 Mark III */
	if ((detect_tvdraw(m_cartridge[index].ROM)) && m_is_region_japan)
	{
		m_cartridge[index].features |= CF_TVDRAW;
	}

	if (m_cartridge[index].features & CF_JANGGUN_MAPPER)
	{
		// Reverse bytes when bit 6 in the mapper is set

		if ( m_cartridge[index].size <= 0x40 * 0x4000 )
		{
			UINT8 *new_rom = auto_alloc_array(machine(), UINT8, 0x80 * 0x4000);
			UINT32 dest = 0;

			while ( dest < 0x40 * 0x4000 )
			{
				memcpy( new_rom + dest, m_cartridge[index].ROM, m_cartridge[index].size );
				dest += m_cartridge[index].size;
			}

			for ( dest = 0; dest < 0x40 * 0x4000; dest++ )
			{
				new_rom[ 0x40 * 0x4000 + dest ] = BITSWAP8( new_rom[ dest ], 0, 1, 2, 3, 4, 5, 6, 7);
			}

			m_cartridge[index].ROM = new_rom;
			m_cartridge[index].size = 0x80 * 0x4000;
		}
	}

	LOG(("Cart Features: %x\n", m_cartridge[index].features));

	/* Load battery backed RAM, if available */
	image.battery_load(m_cartridge[index].cartSRAM, sizeof(UINT8) * NVRAM_SIZE, 0x00);

	return IMAGE_INIT_PASS;
}


void sms_state::setup_cart_banks()
{
	if (m_cartridge[m_current_cartridge].ROM)
	{
		UINT8 rom_page_count = m_cartridge[m_current_cartridge].size / 0x4000;
		m_banking_cart[1] = m_cartridge[m_current_cartridge].ROM;
		m_banking_cart[2] = m_cartridge[m_current_cartridge].ROM + 0x0400;
		m_banking_cart[3] = m_cartridge[m_current_cartridge].ROM + ((1 < rom_page_count) ? 0x4000 : 0);
		m_banking_cart[5] = m_cartridge[m_current_cartridge].ROM + ((2 < rom_page_count) ? 0x8000 : 0);
		m_banking_cart[7] = m_cartridge[m_current_cartridge].ROM + 0x2000;
		/* Codemasters mapper points to bank 0 for page 2 */
		if ( m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER )
		{
			m_banking_cart[5] = m_cartridge[m_current_cartridge].ROM;
		}
		/* Nemesis starts with last 8kb bank in page 0 */
		if (m_cartridge[m_current_cartridge].features & CF_KOREAN_ZEMINA_NEMESIS )
		{
			m_banking_cart[1] = m_cartridge[m_current_cartridge].ROM + ( rom_page_count - 1 ) * 0x4000 + 0x2000;
			m_banking_cart[2] = m_cartridge[m_current_cartridge].ROM + ( rom_page_count - 1 ) * 0x4000 + 0x2000 + 0x400;
		}
	}
	else
	{
		m_banking_cart[1] = m_banking_none;
		m_banking_cart[2] = m_banking_none;
		m_banking_cart[3] = m_banking_none;
		m_banking_cart[5] = m_banking_none;
	}
}

void sms_state::setup_banks()
{
	UINT8 *mem = memregion("maincpu")->base();
	m_banking_none = mem;
	m_banking_bios[1] = m_banking_cart[1] = mem;
	m_banking_bios[2] = m_banking_cart[2] = mem;
	m_banking_bios[3] = m_banking_cart[3] = mem;
	m_banking_bios[4] = m_banking_cart[4] = mem;
	m_banking_bios[5] = m_banking_cart[5] = mem;
	m_banking_bios[6] = m_banking_cart[6] = mem;
	m_banking_bios[7] = m_banking_cart[7] = mem;

	m_BIOS = memregion("user1")->base();

	m_bios_page_count = (m_BIOS ? memregion("user1")->bytes() / 0x4000 : 0);

	setup_cart_banks();

	if (m_BIOS == NULL || m_BIOS[0] == 0x00)
	{
		m_BIOS = NULL;
		m_bios_port |= IO_BIOS_ROM;
		m_has_bios_0400 = 0;
		m_has_bios_2000 = 0;
		m_has_bios_full = 0;
		m_has_bios = 0;
	}

	if (m_BIOS)
	{
		m_banking_bios[1] = m_BIOS;
		m_banking_bios[2] = m_BIOS + 0x0400;
		m_banking_bios[3] = m_BIOS + ((1 < m_bios_page_count) ? 0x4000 : 0);
		m_banking_bios[5] = m_BIOS + ((2 < m_bios_page_count) ? 0x8000 : 0);
	}
}

MACHINE_START_MEMBER(sms_state,sms)
{
	machine().add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(sms_state::sms_machine_stop),this));
	m_rapid_fire_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(sms_state::rapid_fire_callback),this));
	m_rapid_fire_timer->adjust(attotime::from_hz(10), 0, attotime::from_hz(10));

	m_lphaser_1_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(sms_state::lphaser_1_callback),this));
	m_lphaser_2_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(sms_state::lphaser_2_callback),this));

	m_left_lcd = machine().device("left_lcd");
	m_right_lcd = machine().device("right_lcd");
	m_space = &m_main_cpu->space(AS_PROGRAM);

	/* Check if lightgun has been chosen as input: if so, enable crosshair */
	machine().scheduler().timer_set(attotime::zero, timer_expired_delegate(FUNC(sms_state::lightgun_tick),this));

	// alibaba and blockhol are ports of games for the MSX system. The
	// MSX bios usually initializes callback "vectors" at the top of RAM.
	// The code in alibaba does not do this so the IRQ vector only contains
	// the "call $4010" without a following RET statement. That is basically
	// a bug in the program code. The only way this cartridge could have run
	// successfully on a real unit is if the RAM would be initialized with
	// a F0 pattern on power up; F0 = RET P. Do that only for consoles in
	// Japan region (including KR), until confirmed on other consoles.
	if (m_is_region_japan)
	{
		memset((UINT8*)m_space->get_write_ptr(0xc000), 0xf0, 0x1FFF);
	}

	save_item(NAME(m_fm_detect));
	save_item(NAME(m_ctrl_reg));
	save_item(NAME(m_paused));
	save_item(NAME(m_bios_port));
	save_item(NAME(m_mapper));
	save_item(NAME(m_input_port0));
	save_item(NAME(m_input_port1));

	save_item(NAME(m_gg_sio));
	save_item(NAME(m_store_control));
	save_item(NAME(m_rapid_fire_state_1));
	save_item(NAME(m_rapid_fire_state_2));
	save_item(NAME(m_last_paddle_read_time));
	save_item(NAME(m_paddle_read_state));
	save_item(NAME(m_last_sports_pad_time_1));
	save_item(NAME(m_last_sports_pad_time_2));
	save_item(NAME(m_sports_pad_state_1));
	save_item(NAME(m_sports_pad_state_2));
	save_item(NAME(m_sports_pad_last_data_1));
	save_item(NAME(m_sports_pad_last_data_2));
	save_item(NAME(m_sports_pad_1_x));
	save_item(NAME(m_sports_pad_1_y));
	save_item(NAME(m_sports_pad_2_x));
	save_item(NAME(m_sports_pad_2_y));
	save_item(NAME(m_lphaser_1_latch));
	save_item(NAME(m_lphaser_2_latch));
	save_item(NAME(m_sscope_state));
	save_item(NAME(m_frame_sscope_state));
	save_item(NAME(m_current_cartridge));
}

MACHINE_RESET_MEMBER(sms_state,sms)
{
	address_space &space = m_main_cpu->space(AS_PROGRAM);

	m_ctrl_reg = 0xff;
	if (m_has_fm)
		m_fm_detect = 0x01;

	m_bios_port = 0;

	if ( m_cartridge[m_current_cartridge].features & CF_CODEMASTERS_MAPPER )
	{
		/* Install special memory handlers */
		space.install_write_handler(0x0000, 0x0000, write8_delegate(FUNC(sms_state::sms_codemasters_page0_w),this));
		space.install_write_handler(0x4000, 0x4000, write8_delegate(FUNC(sms_state::sms_codemasters_page1_w),this));
	}

	if ( m_cartridge[m_current_cartridge].features & CF_KOREAN_ZEMINA_MAPPER )
	{
		space.install_write_handler(0x0000, 0x0003, write8_delegate(FUNC(sms_state::sms_korean_zemina_banksw_w),this));
	}

	if ( m_cartridge[m_current_cartridge].features & CF_JANGGUN_MAPPER )
	{
		space.install_write_handler(0x4000, 0x4000, write8_delegate(FUNC(sms_state::sms_janggun_bank0_w),this));
		space.install_write_handler(0x6000, 0x6000, write8_delegate(FUNC(sms_state::sms_janggun_bank1_w),this));
		space.install_write_handler(0x8000, 0x8000, write8_delegate(FUNC(sms_state::sms_janggun_bank2_w),this));
		space.install_write_handler(0xA000, 0xA000,write8_delegate(FUNC(sms_state::sms_janggun_bank3_w),this));
	}

	if ( m_cartridge[m_current_cartridge].features & CF_4PAK_MAPPER )
	{
		space.install_write_handler(0x3ffe, 0x3ffe, write8_delegate(FUNC(sms_state::sms_4pak_page0_w),this));
		space.install_write_handler(0x7fff, 0x7fff, write8_delegate(FUNC(sms_state::sms_4pak_page1_w),this));
		space.install_write_handler(0xbfff, 0xbfff, write8_delegate(FUNC(sms_state::sms_4pak_page2_w),this));
	}

	if ( m_cartridge[m_current_cartridge].features & CF_TVDRAW )
	{
		space.install_write_handler(0x6000, 0x6000, write8_delegate(FUNC(sms_state::sms_tvdraw_axis_w),this));
		space.install_read_handler(0x8000, 0x8000, read8_delegate(FUNC(sms_state::sms_tvdraw_status_r),this));
		space.install_read_handler(0xa000, 0xa000, read8_delegate(FUNC(sms_state::sms_tvdraw_data_r),this));
		space.nop_write(0xa000, 0xa000);
		m_cartridge[m_current_cartridge].m_tvdraw_data = 0;
	}

	if ( m_cartridge[m_current_cartridge].features & CF_93C46_EEPROM )
	{
		space.install_write_handler(0x8000,0x8000, write8_delegate(FUNC(sms_state::sms_93c46_w),this));
		space.install_read_handler(0x8000,0x8000, read8_delegate(FUNC(sms_state::sms_93c46_r),this));
	}

	if (m_cartridge[m_current_cartridge].features & CF_GG_SMS_MODE)
	{
		m_vdp->set_sega315_5124_compatibility_mode(true);
	}

	/* Initialize SIO stuff for GG */
	m_gg_sio[0] = 0x7f;
	m_gg_sio[1] = 0xff;
	m_gg_sio[2] = 0x00;
	m_gg_sio[3] = 0xff;
	m_gg_sio[4] = 0x00;

	m_store_control = 0;

	setup_banks();

	setup_rom();

	m_rapid_fire_state_1 = 0;
	m_rapid_fire_state_2 = 0;

	m_last_paddle_read_time = 0;
	m_paddle_read_state = 0;

	m_last_sports_pad_time_1 = 0;
	m_last_sports_pad_time_2 = 0;
	m_sports_pad_state_1 = 0;
	m_sports_pad_state_2 = 0;
	m_sports_pad_last_data_1 = 0;
	m_sports_pad_last_data_2 = 0;
	m_sports_pad_1_x = 0;
	m_sports_pad_1_y = 0;
	m_sports_pad_2_x = 0;
	m_sports_pad_2_y = 0;

	m_lphaser_1_latch = 0;
	m_lphaser_2_latch = 0;

	m_sscope_state = 0;
	m_frame_sscope_state = 0;
}

READ8_MEMBER(sms_state::sms_store_cart_select_r)
{
	return 0xff;
}


WRITE8_MEMBER(sms_state::sms_store_cart_select_w)
{
	UINT8 slot = data >> 4;
	UINT8 slottype = data & 0x08;

	logerror("switching in part of %s slot #%d\n", slottype ? "card" : "cartridge", slot );
	/* cartridge? slot #0 */
	if (slottype == 0)
		m_current_cartridge = slot;

	setup_cart_banks();
	membank("bank10")->set_base(m_banking_cart[3] + 0x2000);
	setup_rom();
}


READ8_MEMBER(sms_state::sms_store_select1)
{
	return 0xff;
}


READ8_MEMBER(sms_state::sms_store_select2)
{
	return 0xff;
}


READ8_MEMBER(sms_state::sms_store_control_r)
{
	return m_store_control;
}


WRITE8_MEMBER(sms_state::sms_store_control_w)
{
	logerror("0x%04X: sms_store_control write 0x%02X\n", space.device().safe_pc(), data);
	if (data & 0x02)
	{
		m_main_cpu->resume(SUSPEND_REASON_HALT);
	}
	else
	{
		/* Pull reset line of CPU #0 low */
		m_main_cpu->reset();
		m_main_cpu->suspend(SUSPEND_REASON_HALT, 1);
	}
	m_store_control = data;
}

WRITE_LINE_MEMBER(sms_state::sms_store_int_callback)
{
	if ( m_store_control & 0x01 )
	{
		m_control_cpu->set_input_line( 0, state );
	}
	else
	{
		m_main_cpu->set_input_line( 0, state );
	}
}


DRIVER_INIT_MEMBER(sms_state,sg1000m3)
{
	m_is_region_japan = 1;
	m_has_fm = 1;
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,sms1)
{
	m_has_bios_full = 1;
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,smsj)
{
	m_is_region_japan = 1;
	m_has_bios_2000 = 1;
	m_has_fm = 1;
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,sms2kr)
{
	m_is_region_japan = 1;
	m_has_bios_full = 1;
	m_has_fm = 1;
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,smssdisp)
{
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,gamegear)
{
	m_is_gamegear = 1;
	m_has_bios_0400 = 1;
	setup_sms_cart();
}


DRIVER_INIT_MEMBER(sms_state,gamegeaj)
{
	m_is_region_japan = 1;
	m_is_gamegear = 1;
	m_has_bios_0400 = 1;
	setup_sms_cart();
}


VIDEO_START_MEMBER(sms_state,sms1)
{
	m_main_scr->register_screen_bitmap(m_prevleft_bitmap);
	m_main_scr->register_screen_bitmap(m_prevright_bitmap);
	save_item(NAME(m_prevleft_bitmap));
	save_item(NAME(m_prevright_bitmap));
}


void sms_state::screen_vblank_sms1(screen_device &screen, bool state)
{
	// on falling edge
	if (!state)
	{
		// Most of the 3-D games usually set Sega Scope's state for next frame
		// soon after the active area of current frame was drawn, but before
		// it is displayed by the screen update function. That function needs to
		// check the state used at the time of frame drawing, to display it on
		// the correct side. So here, when the frame is about to be drawn, the
		// Sega Scope's state is stored, to be checked by that function.
		m_frame_sscope_state = m_sscope_state;
	}
}


UINT32 sms_state::screen_update_sms1(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	UINT8 sscope = 0;
	UINT8 sscope_binocular_hack;
	UINT8 occluded_view = 0;

	if (&screen != m_main_scr)
	{
		sscope = ioport("SEGASCOPE")->read_safe(0x00);
		if (!sscope)
		{
			// without SegaScope, both LCDs for glasses go black
			occluded_view = 1;
		}
		else if (&screen == m_left_lcd)
		{
			// with SegaScope, state 0 = left screen OFF, right screen ON
			if (!(m_frame_sscope_state & 0x01))
				occluded_view = 1;
		}
		else // it's right LCD
		{
			// with SegaScope, state 1 = left screen ON, right screen OFF
			if (m_frame_sscope_state & 0x01)
				occluded_view = 1;
		}
	}

	if (!occluded_view)
	{
		m_vdp->screen_update(screen, bitmap, cliprect);

		// HACK: fake 3D->2D handling (if enabled, it repeats each frame twice on the selected lens)
		// save a copy of current bitmap for the binocular hack
		if (sscope)
		{
			sscope_binocular_hack = ioport("SSCOPE_BINOCULAR")->read_safe(0x00);

			if (&screen == m_left_lcd)
			{
				if (sscope_binocular_hack & 0x01)
					copybitmap(m_prevleft_bitmap, bitmap, 0, 0, 0, 0, cliprect);
			}
			else // it's right LCD
			{
				if (sscope_binocular_hack & 0x02)
					copybitmap(m_prevright_bitmap, bitmap, 0, 0, 0, 0, cliprect);
			}
		}
	}
	else
	{
		// HACK: fake 3D->2D handling (if enabled, it repeats each frame twice on the selected lens)
		// use the copied bitmap for the binocular hack
		if (sscope)
		{
			sscope_binocular_hack = ioport("SSCOPE_BINOCULAR")->read_safe(0x00);

			if (&screen == m_left_lcd)
			{
				if (sscope_binocular_hack & 0x01)
				{
					copybitmap(bitmap, m_prevleft_bitmap, 0, 0, 0, 0, cliprect);
					return 0;
				}
			}
			else // it's right LCD
			{
				if (sscope_binocular_hack & 0x02)
				{
					copybitmap(bitmap, m_prevright_bitmap, 0, 0, 0, 0, cliprect);
					return 0;
				}
			}
		}
		bitmap.fill(RGB_BLACK);
	}

	return 0;
}

UINT32 sms_state::screen_update_sms(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	m_vdp->screen_update(screen, bitmap, cliprect);
	return 0;
}

VIDEO_START_MEMBER(sms_state,gamegear)
{
	m_main_scr->register_screen_bitmap(m_prev_bitmap);
	save_item(NAME(m_prev_bitmap));
}

UINT32 sms_state::screen_update_gamegear(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	int x, y;
	bitmap_rgb32 &vdp_bitmap = m_vdp->get_bitmap();

	// HACK: fake LCD persistence effect
	// (it would be better to generalize this in the core, to be used for all LCD systems)
	for (y = cliprect.min_y; y <= cliprect.max_y; y++)
	{
		UINT32 *linedst = &bitmap.pix32(y);
		UINT32 *line0 = &vdp_bitmap.pix32(y);
		UINT32 *line1 = &m_prev_bitmap.pix32(y);
		for (x = cliprect.min_x; x <= cliprect.max_x; x++)
		{
			UINT32 color0 = line0[x];
			UINT32 color1 = line1[x];
			UINT16 r0 = (color0 >> 16) & 0x000000ff;
			UINT16 g0 = (color0 >>  8) & 0x000000ff;
			UINT16 b0 = (color0 >>  0) & 0x000000ff;
			UINT16 r1 = (color1 >> 16) & 0x000000ff;
			UINT16 g1 = (color1 >>  8) & 0x000000ff;
			UINT16 b1 = (color1 >>  0) & 0x000000ff;
			UINT8 r = (UINT8)((r0 + r1) >> 1);
			UINT8 g = (UINT8)((g0 + g1) >> 1);
			UINT8 b = (UINT8)((b0 + b1) >> 1);
			linedst[x] = (r << 16) | (g << 8) | b;
		}
	}
	copybitmap(m_prev_bitmap, vdp_bitmap, 0, 0, 0, 0, cliprect);

	return 0;
}
