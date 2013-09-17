/***************************************************************************

        AlphaSmart Pro

        driver by Sandro Ronco

    TODO:
    - ADB and PS/2
    - charset ROM is wrong

****************************************************************************/

#include "emu.h"
#include "cpu/mc68hc11/mc68hc11.h"
#include "machine/nvram.h"
#include "video/hd44780.h"
#include "rendlay.h"

class alphasmart_state : public driver_device
{
public:
	alphasmart_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu"),
			m_lcdc0(*this, "ks0066_0"),
			m_lcdc1(*this, "ks0066_1"),
			m_nvram(*this, "nvram"),
			m_rambank(*this, "rambank"),
			m_io_col0(*this, "COL0"),
			m_io_col1(*this, "COL1"),
			m_io_col2(*this, "COL2"),
			m_io_col3(*this, "COL3"),
			m_io_col4(*this, "COL4"),
			m_io_col5(*this, "COL5"),
			m_io_col6(*this, "COL6"),
			m_io_col7(*this, "COL7"),
			m_io_col8(*this, "COL8"),
			m_io_col9(*this, "COL9"),
			m_io_cola(*this, "COLA"),
			m_io_colb(*this, "COLB"),
			m_io_colc(*this, "COLC"),
			m_io_cold(*this, "COLD"),
			m_io_cole(*this, "COLE"),
			m_io_colf(*this, "COLF"),
			m_battery_status(*this, "BATTERY")
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<hd44780_device> m_lcdc0;
	required_device<hd44780_device> m_lcdc1;
	required_device<nvram_device> m_nvram;
	required_memory_bank m_rambank;
	required_ioport m_io_col0;
	required_ioport m_io_col1;
	required_ioport m_io_col2;
	required_ioport m_io_col3;
	required_ioport m_io_col4;
	required_ioport m_io_col5;
	required_ioport m_io_col6;
	required_ioport m_io_col7;
	required_ioport m_io_col8;
	required_ioport m_io_col9;
	required_ioport m_io_cola;
	required_ioport m_io_colb;
	required_ioport m_io_colc;
	required_ioport m_io_cold;
	required_ioport m_io_cole;
	required_ioport m_io_colf;
	required_ioport m_battery_status;

	virtual void machine_start();
	virtual void machine_reset();
	virtual void palette_init();
	virtual UINT32 screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	DECLARE_INPUT_CHANGED_MEMBER(kb_irq);
	DECLARE_READ8_MEMBER(kb_r);
	DECLARE_WRITE8_MEMBER(kb_matrixl_w);
	DECLARE_WRITE8_MEMBER(kb_matrixh_w);
	DECLARE_READ8_MEMBER(port_a_r);
	DECLARE_WRITE8_MEMBER(port_a_w);
	DECLARE_READ8_MEMBER(port_d_r);
	DECLARE_WRITE8_MEMBER(port_d_w);

private:
	UINT8           m_matrix[2];
	UINT8           m_port_a;
	UINT8           m_port_d;
	bitmap_ind16 *  m_tmp_bitmap;
};

INPUT_CHANGED_MEMBER(alphasmart_state::kb_irq)
{
	m_maincpu->set_input_line(MC68HC11_IRQ_LINE, HOLD_LINE);
}

READ8_MEMBER(alphasmart_state::kb_r)
{
	ioport_port* portnames[] =
	{
		m_io_col0, m_io_col1, m_io_col2, m_io_col3, m_io_col4, m_io_col5, m_io_col6, m_io_col7,
		m_io_col8, m_io_col9, m_io_cola, m_io_colb, m_io_colc, m_io_cold, m_io_cole, m_io_colf
	};

	UINT16 matrix = (m_matrix[1]<<8) | m_matrix[0];
	UINT8 data = 0xff;

	for(int i=0; i<16; i++)
		if (!(matrix & (1<<i)))
			data &= portnames[i]->read();

	return data;
}

WRITE8_MEMBER(alphasmart_state::kb_matrixl_w)
{
	m_matrix[0] = data;
}

WRITE8_MEMBER(alphasmart_state::kb_matrixh_w)
{
	m_matrix[1] = data;
}

READ8_MEMBER(alphasmart_state::port_a_r)
{
	return (m_port_a & 0xfd) | (m_battery_status->read() << 1);
}

WRITE8_MEMBER(alphasmart_state::port_a_w)
{
	if ((m_matrix[1] & 0x04))
	{
		UINT8 lcdc_data = 0;

		if ((m_port_a ^ data) & 0x80)
			lcdc_data |= m_lcdc0->read(space, BIT(m_matrix[1], 1));

		if ((m_port_a ^ data) & 0x20)
			lcdc_data |= m_lcdc1->read(space, BIT(m_matrix[1], 1));

		m_port_d = (m_port_d & 0xc3) | (lcdc_data>>2);
	}
	else
	{
		UINT8 lcdc_data = (m_port_d<<2) & 0xf0;

		if ((m_port_a ^ data) & data & 0x80)
			m_lcdc0->write(space, BIT(m_matrix[1], 1), lcdc_data);

		if ((m_port_a ^ data) & data & 0x20)
			m_lcdc1->write(space, BIT(m_matrix[1], 1), lcdc_data);
	}

	m_rambank->set_entry(((data>>3) & 0x01) | ((data>>4) & 0x02));
	m_port_a = data;
}

READ8_MEMBER(alphasmart_state::port_d_r)
{
	return m_port_d;
}

WRITE8_MEMBER(alphasmart_state::port_d_w)
{
	m_port_d = data;
}


static ADDRESS_MAP_START(alphasmart_mem, AS_PROGRAM, 8, alphasmart_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x003f ) AM_NOP   // internal registers
	AM_RANGE( 0x0040, 0x00ff ) AM_RAM   // internal RAM
	AM_RANGE( 0x0000, 0x7fff ) AM_RAMBANK("rambank")
	AM_RANGE( 0x8000, 0x8000 ) AM_READWRITE(kb_r, kb_matrixh_w)
	AM_RANGE( 0xc000, 0xc000 ) AM_WRITE(kb_matrixl_w)
	AM_RANGE( 0x8000, 0xffff ) AM_ROM   AM_REGION("maincpu", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START(alphasmart_io, AS_IO, 8, alphasmart_state)
	AM_RANGE( MC68HC11_IO_PORTA, MC68HC11_IO_PORTA ) AM_READWRITE(port_a_r, port_a_w)
	AM_RANGE( MC68HC11_IO_PORTD, MC68HC11_IO_PORTD ) AM_READWRITE(port_d_r, port_d_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( alphasmart )
	PORT_START("COL0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F8)    PORT_CHAR(UCHAR_MAMEKEY(F1))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F7)    PORT_CHAR(UCHAR_MAMEKEY(F7))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_L)     PORT_CHAR('l') PORT_CHAR('L')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_9)     PORT_CHAR('9') PORT_CHAR('(')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_O)     PORT_CHAR('o') PORT_CHAR('O')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_STOP)  PORT_CHAR('.') PORT_CHAR('>')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COL1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_5)     PORT_CHAR('5') PORT_CHAR('%')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_G)     PORT_CHAR('g') PORT_CHAR('G')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_T)     PORT_CHAR('t') PORT_CHAR('T')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_B)     PORT_CHAR('b') PORT_CHAR('B')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F)     PORT_CHAR('f') PORT_CHAR('F')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_4)     PORT_CHAR('4') PORT_CHAR('$')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_R)     PORT_CHAR('r') PORT_CHAR('R')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_V)     PORT_CHAR('v') PORT_CHAR('V')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COL2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_6)     PORT_CHAR('6') PORT_CHAR('^')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_H)     PORT_CHAR('h') PORT_CHAR('H')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Y)     PORT_CHAR('y') PORT_CHAR('Y')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_N)     PORT_CHAR('n') PORT_CHAR('N')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_J)     PORT_CHAR('j') PORT_CHAR('J')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_7)     PORT_CHAR('7') PORT_CHAR('&')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_U)     PORT_CHAR('u') PORT_CHAR('U')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_M)     PORT_CHAR('m') PORT_CHAR('M')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COL3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'')    PORT_CHAR('\"') PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_0)     PORT_CHAR('0') PORT_CHAR(')')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_P)     PORT_CHAR('p') PORT_CHAR('P')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("COL4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER_PAD) PORT_NAME("Enter") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("COL5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_END)  PORT_CHAR(UCHAR_MAMEKEY(END))    PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COL6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LWIN) PORT_CODE(KEYCODE_PGUP)  PORT_NAME("Left Command") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RWIN) PORT_CODE(KEYCODE_PGDN) PORT_NAME("Right Command") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)//
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_DEL)  PORT_NAME("Clear File") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("COL7")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F12) PORT_NAME("Send") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_UP)   PORT_CHAR(UCHAR_MAMEKEY(UP))     PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_INSERT)   PORT_CHAR(UCHAR_MAMEKEY(INSERT)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COL8")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("COL9")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LALT) PORT_NAME("Left Alt/Option") PORT_CHAR(UCHAR_MAMEKEY(LALT))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RALT) PORT_NAME("Right Alt/Option") PORT_CHAR(UCHAR_MAMEKEY(LALT))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLA")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=')    PORT_CHAR('+') PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F6)   PORT_CHAR(UCHAR_MAMEKEY(F6)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']')    PORT_CHAR('}') PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_K)    PORT_CHAR('k')  PORT_CHAR('K')   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_8)    PORT_CHAR('8')  PORT_CHAR('*')   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_I)    PORT_CHAR('i')  PORT_CHAR('I')   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLB")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F9)   PORT_NAME("Pause") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F5)        PORT_CHAR(UCHAR_MAMEKEY(F5))     PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSPACE) PORT_NAME("Backspace") PORT_CHAR(UCHAR_MAMEKEY(BACKSPACE)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_SPACE)     PORT_CHAR(' ') PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|') PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F10)  PORT_NAME("ScrLk") PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ENTER)   PORT_NAME("Return") PORT_CHAR(13) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLC")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F2)   PORT_CHAR(UCHAR_MAMEKEY(F2))    PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F4)   PORT_CHAR(UCHAR_MAMEKEY(F4))    PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F3)   PORT_CHAR(UCHAR_MAMEKEY(F3))    PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_D)    PORT_CHAR('d')  PORT_CHAR('D')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_3)    PORT_CHAR('3')  PORT_CHAR('#')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_E)    PORT_CHAR('e')  PORT_CHAR('E')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_C)    PORT_CHAR('c')  PORT_CHAR('C')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLD")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_F1)   PORT_CHAR(UCHAR_MAMEKEY(F1))    PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_S)    PORT_CHAR('s')  PORT_CHAR('S')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_2)    PORT_CHAR('2')  PORT_CHAR('@')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_W)    PORT_CHAR('w')  PORT_CHAR('W')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_X)    PORT_CHAR('x')  PORT_CHAR('X')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLE")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_ESC)  PORT_CHAR(UCHAR_MAMEKEY(ESC))   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_TAB)  PORT_CHAR('\t')   PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_A)    PORT_CHAR('a')  PORT_CHAR('A')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_1)    PORT_CHAR('1')  PORT_CHAR('!')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Q)    PORT_CHAR('q')  PORT_CHAR('Q')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_Z)    PORT_CHAR('z')  PORT_CHAR('Z')  PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_START("COLF")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_MAMEKEY(RSHIFT)) PORT_CHANGED_MEMBER(DEVICE_SELF, alphasmart_state, kb_irq, NULL)

	PORT_START("BATTERY")
	PORT_CONFNAME(0x01, 0x01, "Battery status")
	PORT_CONFSETTING (0x00, DEF_STR(Low))
	PORT_CONFSETTING (0x01, DEF_STR(Normal))
INPUT_PORTS_END

void alphasmart_state::palette_init()
{
	palette_set_color(machine(), 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine(), 1, MAKE_RGB(92, 83, 88));
}

UINT32 alphasmart_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_lcdc0->screen_update(screen, *m_tmp_bitmap, cliprect);
	copybitmap(bitmap, *m_tmp_bitmap, 0, 0, 0, 0, cliprect);
	m_lcdc1->screen_update(screen, *m_tmp_bitmap, cliprect);
	copybitmap(bitmap, *m_tmp_bitmap, 0, 0, 0, 18,cliprect);
	return 0;
}

void alphasmart_state::machine_start()
{
	UINT8* ram = (UINT8*)(*memregion("mainram"));

	m_rambank->configure_entries(0, 4, ram, 0x8000);
	m_nvram->set_base(ram, 0x8000*4);

	m_tmp_bitmap = auto_bitmap_ind16_alloc(machine(), 6*40, 9*4);
}

void alphasmart_state::machine_reset()
{
	m_rambank->set_entry(0);
	m_matrix[0] = m_matrix[1] = 0;
	m_port_a = 0;
	m_port_d = 0;
}

static MACHINE_CONFIG_START( alphasmart, alphasmart_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", MC68HC11, XTAL_8MHz/2)  // MC68HC11D0, XTAL is 8 Mhz, unknown divider
	MCFG_CPU_PROGRAM_MAP(alphasmart_mem)
	MCFG_CPU_IO_MAP(alphasmart_io)
	MCFG_MC68HC11_CONFIG(0, 0, 0x00) // FIXME: internal ram should be 192, but the 68hc11 core doesn't handle internal RAM correctly

	MCFG_KS0066_F05_ADD("ks0066_0")
	MCFG_HD44780_LCD_SIZE(2, 40)
	MCFG_KS0066_F05_ADD("ks0066_1")
	MCFG_HD44780_LCD_SIZE(2, 40)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", LCD)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_UPDATE_DRIVER(alphasmart_state, screen_update)
	MCFG_SCREEN_SIZE(6*40, 9*4)
	MCFG_SCREEN_VISIBLE_AREA(0, (6*40)-1, 0, (9*4)-1)
	MCFG_PALETTE_LENGTH(2)
	MCFG_DEFAULT_LAYOUT(layout_lcd)

	MCFG_NVRAM_ADD_0FILL("nvram")
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( alphasma )
	ROM_REGION( 0x8000, "maincpu", 0 )
	ROM_LOAD( "alphasmartpro212.rom",  0x0000, 0x8000, CRC(896ddf1c) SHA1(c3c6a421c9ced92db97431d04b4a3f09a39de716) )   // Checksum 8D24 on label

	ROM_REGION( 0x20000, "mainram", ROMREGION_ERASE )
ROM_END


/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1995, alphasma,  0,       0,  alphasmart, alphasmart, driver_device,   0,   "Intelligent Peripheral Devices",   "AlphaSmart Pro", GAME_NOT_WORKING | GAME_NO_SOUND )
