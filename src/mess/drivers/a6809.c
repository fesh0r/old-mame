/***************************************************************************

    Acorn 6809

    12/05/2009 Skeleton driver.

    Acorn System 3 update?
    http://acorn.chriswhy.co.uk/8bit_Upgrades/Acorn_6809_CPU.html

ToDo:
    - FDC (address A00)
    - Cursor
    - Scrolling
    - Centronics Printer (VIA port A)


****************************************************************************/

#include "emu.h"
#include "cpu/m6809/m6809.h"
#include "machine/6522via.h"
#include "machine/keyboard.h"
#include "video/saa5050.h"
#include "video/mc6845.h"
#include "imagedev/cassette.h"
#include "sound/wave.h"


class a6809_state : public driver_device
{
public:
	a6809_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_via(*this, "via"),
		m_videoram(*this, "videoram"),
		m_cass(*this, "cassette"),
		m_maincpu(*this, "maincpu") { }

	DECLARE_READ8_MEMBER(via_pb_r);
	DECLARE_WRITE8_MEMBER(kb_w);
	DECLARE_READ8_MEMBER(videoram_r);
	DECLARE_WRITE_LINE_MEMBER(cass_w);
	TIMER_DEVICE_CALLBACK_MEMBER(a6809_c);
	TIMER_DEVICE_CALLBACK_MEMBER(a6809_p);
private:
	UINT8 m_cass_data[4];
	bool m_cass_state;
	UINT8 m_keydata;
	virtual void machine_reset();
	required_device<via6522_device> m_via;
	required_shared_ptr<UINT8> m_videoram;
	required_device<cassette_image_device> m_cass;
	required_device<cpu_device> m_maincpu;
};


static ADDRESS_MAP_START(a6809_mem, AS_PROGRAM, 8, a6809_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x03ff) AM_RAM
	AM_RANGE(0x0400,0x07ff) AM_RAM AM_SHARE("videoram")
	AM_RANGE(0x0800,0x0800) AM_DEVWRITE("mc6845", mc6845_device, address_w)
	AM_RANGE(0x0801,0x0801) AM_DEVREADWRITE("mc6845", mc6845_device, register_r, register_w)
	AM_RANGE(0x0900,0x090f) AM_MIRROR(0xf0) AM_DEVREADWRITE("via", via6522_device, read, write)
	AM_RANGE(0xf000,0xf7ff) // optional ROM
	AM_RANGE(0xf800,0xffff) AM_ROM AM_REGION("maincpu", 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( a6809_io, AS_IO, 8, a6809_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( a6809 )
INPUT_PORTS_END


void a6809_state::machine_reset()
{
}

static MC6845_UPDATE_ROW( a6809_update_row )
{
}


static MC6845_INTERFACE( a6809_crtc6845_interface )
{
	false,
	12 /*?*/,
	NULL,
	a6809_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	NULL
};

READ8_MEMBER( a6809_state::videoram_r )
{
	return m_videoram[offset];
}

static SAA5050_INTERFACE( a6809_saa5050_intf )
{
	DEVCB_DRIVER_MEMBER(a6809_state, videoram_r),
	40, 24, 40  /* x, y, size */
};

READ8_MEMBER( a6809_state::via_pb_r )
{
	return m_keydata | m_cass_data[2];
}

WRITE_LINE_MEMBER( a6809_state::cass_w )
{
	m_cass_state = state;
}

static const via6522_interface via_intf =
{
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(a6809_state, via_pb_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(a6809_state, cass_w), // out CB2
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0)
};

TIMER_DEVICE_CALLBACK_MEMBER(a6809_state::a6809_c)
{
	m_cass_data[3]++;

	if (m_cass_state)
		m_cass->output(BIT(m_cass_data[3], 0) ? -1.0 : +1.0); // 2400Hz
	else
		m_cass->output(BIT(m_cass_data[3], 1) ? -1.0 : +1.0); // 1200Hz
}

TIMER_DEVICE_CALLBACK_MEMBER(a6809_state::a6809_p)
{
	/* cassette - turn 1200/2400Hz to a bit */
	m_cass_data[1]++;
	UINT8 cass_ws = (m_cass->input() > +0.03) ? 1 : 0;

	if (cass_ws != m_cass_data[0])
	{
		m_cass_data[0] = cass_ws;
		m_cass_data[2] = ((m_cass_data[1] < 12) ? 128 : 0);
		m_cass_data[1] = 0;
	}
}

WRITE8_MEMBER( a6809_state::kb_w )
{
	m_keydata = data & 0x7f;

	m_via->write_cb1(1);
	m_via->write_cb1(0);
}

static ASCII_KEYBOARD_INTERFACE( kb_intf )
{
	DEVCB_DRIVER_MEMBER(a6809_state, kb_w)
};

static MACHINE_CONFIG_START( a6809, a6809_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",M6809E, XTAL_4MHz)
	MCFG_CPU_PROGRAM_MAP(a6809_mem)
	MCFG_CPU_IO_MAP(a6809_io)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500))
	MCFG_SCREEN_SIZE(40 * 12, 24 * 20)
	MCFG_SCREEN_VISIBLE_AREA(0, 40 * 12 - 1, 0, 24 * 20 - 1)
	MCFG_SCREEN_UPDATE_DEVICE("saa5050", saa5050_device, screen_update)


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_WAVE_ADD(WAVE_TAG, "cassette")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MCFG_SAA5050_ADD("saa5050", 6000000, a6809_saa5050_intf)

	/* Devices */
	MCFG_VIA6522_ADD("via", XTAL_4MHz / 4, via_intf)
	MCFG_MC6845_ADD("mc6845", MC6845, "screen", XTAL_4MHz / 2, a6809_crtc6845_interface)
	MCFG_ASCII_KEYBOARD_ADD("keyboard", kb_intf)
	MCFG_CASSETTE_ADD( "cassette", default_cassette_interface )
	MCFG_TIMER_DRIVER_ADD_PERIODIC("a6809_c", a6809_state, a6809_c, attotime::from_hz(4800))
	MCFG_TIMER_DRIVER_ADD_PERIODIC("a6809_p", a6809_state, a6809_p, attotime::from_hz(40000))
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( a6809 )
	ROM_REGION( 0x800, "maincpu", 0 )
	ROM_LOAD( "acorn6809.bin", 0x000, 0x800, CRC(5fa5b632) SHA1(b14a884bf82a7a8c23bc03c2e112728dd1a74896) )

	ROM_REGION( 0x100, "proms", 0 )
	ROM_LOAD( "acorn6809.ic11", 0x0000, 0x0100, CRC(7908317d) SHA1(e0f1e5bd3a8598d3b62bc432dd1f3892ed7e66d8) ) // address decoder
ROM_END

/* Driver */

/*    YEAR   NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1980, a6809,  0,      0,       a6809,     a6809, driver_device,   0,      "Acorn",  "System 3 (6809 CPU)", GAME_NOT_WORKING )
