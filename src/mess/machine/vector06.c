/***************************************************************************

        Vector06c driver by Miodrag Milanovic

        10/07/2008 Preliminary driver.

****************************************************************************/


#include "includes/vector06.h"


READ8_MEMBER( vector06_state::vector06_8255_portb_r )
{
	UINT8 key = 0xff;
	if (BIT(m_keyboard_mask, 0)) key &= ioport("LINE0")->read();
	if (BIT(m_keyboard_mask, 1)) key &= ioport("LINE1")->read();
	if (BIT(m_keyboard_mask, 2)) key &= ioport("LINE2")->read();
	if (BIT(m_keyboard_mask, 3)) key &= ioport("LINE3")->read();
	if (BIT(m_keyboard_mask, 4)) key &= ioport("LINE4")->read();
	if (BIT(m_keyboard_mask, 5)) key &= ioport("LINE5")->read();
	if (BIT(m_keyboard_mask, 6)) key &= ioport("LINE6")->read();
	if (BIT(m_keyboard_mask, 7)) key &= ioport("LINE7")->read();
	return key;
}

READ8_MEMBER( vector06_state::vector06_8255_portc_r )
{
	UINT8 ret = ioport("LINE8")->read();

	if (m_cassette->input() > 0)
		ret |= 0x10;

	return ret;
}

WRITE8_MEMBER( vector06_state::vector06_8255_porta_w )
{
	m_keyboard_mask = data ^ 0xff;
}

void vector06_state::vector06_set_video_mode(int width)
{
	rectangle visarea(0, width+64-1, 0, 256+64-1);
	machine().primary_screen->configure(width+64, 256+64, visarea, machine().primary_screen->frame_period().attoseconds);
}

WRITE8_MEMBER( vector06_state::vector06_8255_portb_w )
{
	m_color_index = data & 0x0f;
	if ((data & 0x10) != m_video_mode)
	{
		m_video_mode = data & 0x10;
		vector06_set_video_mode((m_video_mode==0x10) ? 512 : 256);
	}
}

WRITE8_MEMBER( vector06_state::vector06_color_set )
{
	UINT8 r = (data & 7) << 5;
	UINT8 g = ((data >> 3) & 7) << 5;
	UINT8 b = ((data >>6) & 3) << 6;
	palette_set_color( machine(), m_color_index, MAKE_RGB(r,g,b) );
}


READ8_MEMBER( vector06_state::vector06_romdisk_portb_r )
{
	UINT8 *romdisk = memregion("maincpu")->base() + 0x18000;
	UINT16 addr = (m_romdisk_msb | m_romdisk_lsb) & 0x7fff;
	return romdisk[addr];
}

WRITE8_MEMBER( vector06_state::vector06_romdisk_porta_w )
{
	m_romdisk_lsb = data;
}

WRITE8_MEMBER( vector06_state::vector06_romdisk_portc_w )
{
	m_romdisk_msb = data << 8;
}

I8255A_INTERFACE( vector06_ppi8255_2_interface )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_romdisk_porta_w),
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_romdisk_portb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_romdisk_portc_w)
};


I8255A_INTERFACE( vector06_ppi8255_interface )
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_8255_porta_w),
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_8255_portb_r),
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_8255_portb_w),
	DEVCB_DRIVER_MEMBER(vector06_state, vector06_8255_portc_r),
	DEVCB_NULL
};

READ8_MEMBER( vector06_state::vector06_8255_1_r )
{
	return m_ppi->read(space, offset^3);
}

WRITE8_MEMBER( vector06_state::vector06_8255_1_w )
{
	m_ppi->write(space, offset^3, data);
}

READ8_MEMBER( vector06_state::vector06_8255_2_r )
{
	return m_ppi2->read(space, offset^3);
}

WRITE8_MEMBER( vector06_state::vector06_8255_2_w )
{
	m_ppi2->write(space, offset^3, data);
}

INTERRUPT_GEN_MEMBER(vector06_state::vector06_interrupt)
{
	m_vblank_state++;
	if (m_vblank_state>1) m_vblank_state=0;
	device.execute().set_input_line(0,m_vblank_state ? HOLD_LINE : CLEAR_LINE);

}

IRQ_CALLBACK_MEMBER(vector06_state::vector06_irq_callback)
{
	// Interupt is RST 7
	return 0xff;
}

TIMER_CALLBACK_MEMBER(vector06_state::reset_check_callback)
{
	UINT8 val = ioport("RESET")->read();

	if (BIT(val, 0))
	{
		membank("bank1")->set_base(memregion("maincpu")->base() + 0x10000);
		m_maincpu->reset();
	}

	if (BIT(val, 1))
	{
		membank("bank1")->set_base(m_ram->pointer() + 0x0000);
		m_maincpu->reset();
	}
}

WRITE8_MEMBER( vector06_state::vector06_disc_w )
{
// something here needs to turn the motor on

	wd17xx_set_side (m_fdc,BIT(data, 2) ^ 1);
	wd17xx_set_drive(m_fdc,BIT(data, 0));
}

void vector06_state::machine_start()
{
	machine().scheduler().timer_pulse(attotime::from_hz(50), timer_expired_delegate(FUNC(vector06_state::reset_check_callback),this));
}

void vector06_state::machine_reset()
{
	address_space &space = m_maincpu->space(AS_PROGRAM);

	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(vector06_state::vector06_irq_callback),this));
	space.install_read_bank (0x0000, 0x7fff, "bank1");
	space.install_write_bank(0x0000, 0x7fff, "bank2");
	space.install_read_bank (0x8000, 0xffff, "bank3");
	space.install_write_bank(0x8000, 0xffff, "bank4");

	membank("bank1")->set_base(memregion("maincpu")->base() + 0x10000);
	membank("bank2")->set_base(m_ram->pointer() + 0x0000);
	membank("bank3")->set_base(m_ram->pointer() + 0x8000);
	membank("bank4")->set_base(m_ram->pointer() + 0x8000);

	m_keyboard_mask = 0;
	m_color_index = 0;
	m_video_mode = 0;
}
