/***************************************************************************

        BOB85 driver by Miodrag Milanovic

        24/05/2008 Preliminary driver.
        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "bob85.lh"

static ADDRESS_MAP_START(bob85_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x02ff ) AM_ROM
	AM_RANGE( 0x0300, 0x1fff ) AM_RAM
ADDRESS_MAP_END

static UINT8 prev_key = 0;
static UINT8 count_key = 0;

static READ8_HANDLER(bob85_keyboard_r)
{
	UINT8 retVal = 0;
	UINT8 line0 = input_port_read(space->machine, "LINE0");
	UINT8 line1 = input_port_read(space->machine, "LINE1");
	UINT8 line2 = input_port_read(space->machine, "LINE2");
	if (line0!=0) {
		switch(line0) {
			case 0x01 : retVal = 0x80; break;
			case 0x02 : retVal = 0x81; break;
			case 0x04 : retVal = 0x82; break;
			case 0x08 : retVal = 0x83; break;
			case 0x10 : retVal = 0x84; break;
			case 0x20 : retVal = 0x85; break;
			case 0x40 : retVal = 0x86; break;
			case 0x80 : retVal = 0x87; break;
			default : retVal = 0; break;
		}
	}
	if (line1!=0) {
		switch(line1) {
			case 0x01 : retVal = 0x88; break;
			case 0x02 : retVal = 0x89; break;
			case 0x04 : retVal = 0x8A; break;
			case 0x08 : retVal = 0x8B; break;
			case 0x10 : retVal = 0x8C; break;
			case 0x20 : retVal = 0x8D; break;
			case 0x40 : retVal = 0x8E; break;
			case 0x80 : retVal = 0x8F; break;
			default : retVal = 0; break;
		}
	}
	if (line2!=0) {
		switch(line2) {
			case 0x01 : retVal |= 0x90; break;
			case 0x02 : retVal |= 0xA0; break;
			case 0x04 : retVal |= 0xB0; break;
			case 0x08 : retVal |= 0xC0; break;
			case 0x10 : retVal |= 0xD0; break;
			case 0x20 : retVal |= 0xF0; break;
			default :  break;
		}
	}
	if (retVal != prev_key) {
		prev_key = retVal;
		count_key = 0;
		return retVal;
	} else {
		if (count_key <1) {
			count_key++;
			return retVal;
		} else {
			return 0;
		}
	}

	if (retVal == 0) {
		prev_key = 0;
		count_key = 0;
	}

	return retVal;
}

static WRITE8_HANDLER(bob85_7seg_w)
{
	output_set_digit_value(offset,BITSWAP8( data,3,2,1,0,7,6,5,4 ));
}

static ADDRESS_MAP_START( bob85_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE ( 0x0A, 0x0A ) AM_READ(bob85_keyboard_r)
	AM_RANGE ( 0x0A, 0x0F ) AM_WRITE(bob85_7seg_w)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( bob85 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_1) PORT_CHAR('1')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_2) PORT_CHAR('2')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_3) PORT_CHAR('3')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_4) PORT_CHAR('4')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_5) PORT_CHAR('5')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_6) PORT_CHAR('6')
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_8) PORT_CHAR('8')
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_9) PORT_CHAR('9')
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("GO") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("SMEM") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("REC") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("VEK1") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("VEK2") PORT_CODE(KEYCODE_F5)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NEXT") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0xC0, IP_ACTIVE_HIGH, IPT_UNUSED)
INPUT_PORTS_END


static MACHINE_RESET(bob85)
{
}

static MACHINE_DRIVER_START( bob85 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",8085A, 2000000)
    MDRV_CPU_PROGRAM_MAP(bob85_mem)
    MDRV_CPU_IO_MAP(bob85_io)

    MDRV_MACHINE_RESET(bob85)

    /* video hardware */
	MDRV_DEFAULT_LAYOUT(layout_bob85)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( bob85 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bob85.rom", 0x0000, 0x0300, CRC(adde33a8) SHA1(00f26dd0c52005e7705e6cc9cb11a20e572682c6))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( ????, bob85,  0,       0, 	bob85,	bob85,	 0, 	  "Unknown",   "BOB85",		GAME_NOT_WORKING | GAME_NO_SOUND)
