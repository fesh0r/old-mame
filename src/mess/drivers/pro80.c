/***************************************************************************

        Protec Pro-80

        06/12/2009 Skeleton driver.

        TODO:
        - Cassette load/save
        - Use messram for optional ram
        - Fix Step command (don't works due of missing interrupt emulation)

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "pro80.lh"

class pro80_state : public driver_device
{
public:
	pro80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config)
	{ }

	void machine_reset();

	WRITE8_MEMBER( digit_w );
	WRITE8_MEMBER( segment_w );
	READ8_MEMBER( kp_r );

	UINT8 digit_sel;
};


WRITE8_MEMBER( pro80_state::digit_w )
{
	// --xx xxxx digit select
	// -x-- ---- cassette out
	// x--- ---- ???
	digit_sel = data & 0x3f;
}

WRITE8_MEMBER( pro80_state::segment_w )
{
	if (digit_sel)
	{
		if (!BIT(digit_sel, 0)) output_set_digit_value(0, data);
		if (!BIT(digit_sel, 1)) output_set_digit_value(1, data);
		if (!BIT(digit_sel, 2)) output_set_digit_value(2, data);
		if (!BIT(digit_sel, 3)) output_set_digit_value(3, data);
		if (!BIT(digit_sel, 4)) output_set_digit_value(4, data);
		if (!BIT(digit_sel, 5)) output_set_digit_value(5, data);

		digit_sel = 0;
	}
}

READ8_MEMBER( pro80_state::kp_r )
{
	UINT8 data = 0x0f;

	if (!BIT(digit_sel, 0)) data &= input_port_read(space.machine, "LINE0");
	if (!BIT(digit_sel, 1)) data &= input_port_read(space.machine, "LINE1");
	if (!BIT(digit_sel, 2)) data &= input_port_read(space.machine, "LINE2");
	if (!BIT(digit_sel, 3)) data &= input_port_read(space.machine, "LINE3");
	if (!BIT(digit_sel, 4)) data &= input_port_read(space.machine, "LINE4");
	if (!BIT(digit_sel, 5)) data &= input_port_read(space.machine, "LINE5");

	return data;
}

static ADDRESS_MAP_START(pro80_mem, ADDRESS_SPACE_PROGRAM, 8, pro80_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x03ff) AM_ROM
	AM_RANGE(0x1000, 0x13ff) AM_RAM
	AM_RANGE(0x1400, 0x17ff) AM_RAM // 2nd RAM is optional
ADDRESS_MAP_END

static ADDRESS_MAP_START( pro80_io , ADDRESS_SPACE_IO, 8, pro80_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	//AM_RANGE(0x40, 0x43) Z80PIO
	AM_RANGE(0x44, 0x47) AM_READ(kp_r)
	AM_RANGE(0x48, 0x4b) AM_WRITE(digit_w)
	AM_RANGE(0x4c, 0x4f) AM_WRITE(segment_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pro80 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CR") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("CW") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("SSt") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("Res") PORT_CODE(KEYCODE_EQUALS)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("EXE") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("NEx") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("REx") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("MEx") PORT_CODE(KEYCODE_M)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("7 [IY]") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("6 [IX]") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("5 [PC]") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("9 [H]") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("4 [SP]") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("8 [L]") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYPAD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
INPUT_PORTS_END

void pro80_state::machine_reset()
{
	digit_sel = 0;
}

static MACHINE_CONFIG_START( pro80, pro80_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MCFG_CPU_PROGRAM_MAP(pro80_mem)
    MCFG_CPU_IO_MAP(pro80_io)

    /* video hardware */
    MCFG_DEFAULT_LAYOUT(layout_pro80)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pro80 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// This rom dump is taken out of manual for this machine
	ROM_LOAD( "pro80.bin", 0x0000, 0x0400, CRC(1bf6e0a5) SHA1(eb45816337e08ed8c30b589fc24960dc98b94db2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY     FULLNAME       FLAGS */
COMP( 1981, pro80,  0,       0, 	pro80,		pro80,	 0, 	 "Protec",   "Pro-80",		GAME_NOT_WORKING | GAME_NO_SOUND)

