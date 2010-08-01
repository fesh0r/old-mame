/******************************************************************************

  mk1.c

Driver file to handle emulation of the Novag/Videomaster Chess Champion MK 1
by PeT mess@utanet.at 2000,2001.

Minor updates by Wilbert Pol - 2007

Hardware descriptions:
- An F8 3850 CPU accompanied by a 3853 memory interface
  Variations seen:
  - MOSTEK MK 3853N 7915 Philippines ( static memory interface for f8)
  - MOSTEK MK 3850N-3 7917 Philipines (fairchild f8 cpu)
  - 3850PK F7901 SINGAPORE (Fairchild F8 CPU)
  - 3853PK F7851 SINGAPORE (static memory interface for F8)
- 2KB 2316 compatible ROM
  Variations seen:
  - Signetics 7916E C48091 82S210-1 COPYRIGHT
  - RO-3-8316A 8316A-4480 7904 TAIWAN
- 2 x 2111 256x4 SRAM to provide 256 bytes of RAM
  Variations seen:
  - AM9111 BPC / P2111A-4 7851
- 16 keys placed in a 4 x 4 matrix
- Power on switch
- L/S switch. This switch is directly tied to the RESET pin of the F8 CPU.
  This allows the user to reset the CPU without destroying the RAM contents.
- A 4 character 11 segment digit display using a 15 pin interface. Of the 15
  pins 3 pins are not connected, so three segments are never used and this
  leaves a standard 7 segments display with a dot in the lower right.
- The digit display is driven by two other components:
  - SN75492N MALAYSIA 7840B
  - ULN2033A 7847
- Hardware addressing is controlled by a NBF4001AE.
- Unknown if there is a speaker.

TODO:
- Figure out exact clock frequency

******************************************************************************/

#include "emu.h"

#include "cpu/f8/f8.h"
#include "machine/f3853.h"
#include "mk1.lh"

#define MAIN_CLOCK	1000000

static UINT8 mk1_f8[2];
static UINT8 mk1_led[4];

static READ8_HANDLER( mk1_f8_r ) {
    UINT8 data = mk1_f8[offset];

    if ( offset == 0 ) {
		if ( data & 1 ) data |= input_port_read(space->machine, "LINE1");
		if ( data & 2 ) data |= input_port_read(space->machine, "LINE2");
		if ( data & 4 ) data |= input_port_read(space->machine, "LINE3");
		if ( data & 8 ) data |= input_port_read(space->machine, "LINE4");
		if ( data & 0x10 ) {
			if ( input_port_read(space->machine, "LINE1") & 0x10 ) data |= 1;
			if ( input_port_read(space->machine, "LINE2") & 0x10 ) data |= 2;
			if ( input_port_read(space->machine, "LINE3") & 0x10 ) data |= 4;
			if ( input_port_read(space->machine, "LINE4") & 0x10 ) data |= 8;
		}
		if ( data & 0x20 ) {
			if ( input_port_read(space->machine, "LINE1") & 0x20 ) data |= 1;
			if ( input_port_read(space->machine, "LINE2") & 0x20 ) data |= 2;
			if ( input_port_read(space->machine, "LINE3") & 0x20 ) data |= 4;
			if ( input_port_read(space->machine, "LINE4") & 0x20 ) data |= 8;
		}
		if ( data & 0x40 ) {
			if ( input_port_read(space->machine, "LINE1") & 0x40 ) data |= 1;
			if ( input_port_read(space->machine, "LINE2") & 0x40 ) data |= 2;
			if ( input_port_read(space->machine, "LINE3") & 0x40 ) data |= 4;
			if ( input_port_read(space->machine, "LINE4") & 0x40 ) data |= 8;
		}
		if ( data & 0x80 ) {
			if ( input_port_read(space->machine, "LINE1") & 0x80 ) data |= 1;
			if ( input_port_read(space->machine, "LINE2") & 0x80 ) data |= 2;
			if ( input_port_read(space->machine, "LINE3") & 0x80 ) data |= 4;
			if ( input_port_read(space->machine, "LINE4") & 0x80 ) data |= 8;
		}
    }
    return data;
}

static WRITE8_HANDLER( mk1_f8_w ) {
	/* 0 is high and allows also input */
	mk1_f8[offset] = data;

	if ( ! ( mk1_f8[1] & 1 ) ) mk1_led[0] = BITSWAP8( mk1_f8[0],2,1,3,4,5,6,7,0 );
	if ( ! ( mk1_f8[1] & 2 ) ) mk1_led[1] = BITSWAP8( mk1_f8[0],2,1,3,4,5,6,7,0 );
	if ( ! ( mk1_f8[1] & 4 ) ) mk1_led[2] = BITSWAP8( mk1_f8[0],2,1,3,4,5,6,7,0 );
	if ( ! ( mk1_f8[1] & 8 ) ) mk1_led[3] = BITSWAP8( mk1_f8[0],2,1,3,4,5,6,7,0 );
}

static ADDRESS_MAP_START( mk1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x07ff ) AM_ROM
	AM_RANGE( 0x1800, 0x18ff ) AM_RAM
ADDRESS_MAP_END


static ADDRESS_MAP_START( mk1_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x0, 0x1 ) AM_READWRITE( mk1_f8_r, mk1_f8_w )
	AM_RANGE( 0xc, 0xf ) AM_DEVREADWRITE("f3853", f3853_r, f3853_w )
ADDRESS_MAP_END


static INPUT_PORTS_START( mk1 )
	PORT_START("RESET")	/* 0 */
	PORT_DIPNAME ( 0x01, 0x01, "Switch")
	PORT_DIPSETTING(  0, "L" )
	PORT_DIPSETTING(  1, "S" )

	PORT_START("LINE1")	/* 1 */
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White A    King") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White B    Queen") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White C    Bishop") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White D    PLAY") PORT_CODE(KEYCODE_D)

	PORT_START("LINE2")	/* 2 */
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White E    Knight") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White F    Castle") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White G    Pawn") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("White H    md") PORT_CODE(KEYCODE_H)

	PORT_START("LINE3")	/* 3 */
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 1    King") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 2    Queen") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 3    Bishop") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 4    fp") PORT_CODE(KEYCODE_4)

	PORT_START("LINE4")	/* 4 */
	PORT_BIT ( 0x0f, 0x0,	 IPT_UNUSED )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 5    Knight") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 6    Castle") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 7    Pawn") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Black 8    ep") PORT_CODE(KEYCODE_8)
INPUT_PORTS_END


static TIMER_CALLBACK( mk1_update_leds )
{
	int i;

	for ( i = 0; i < 4; i++ ) {
		output_set_digit_value( i, mk1_led[i] >> 1 );
		output_set_led_value( i, mk1_led[i] & 0x01 );
		mk1_led[i] = 0;
	}
}


static MACHINE_START( mk1 )
{
	timer_pulse(machine,  ATTOTIME_IN_HZ(30), NULL, 0, mk1_update_leds );
}


static void mk1_interrupt( running_device *device, UINT16 addr, int level )
{
	cpu_set_input_line_vector(device->machine->device("maincpu"), 0, addr );
	cputag_set_input_line(device->machine,"maincpu", 0, level ? F8_INT_INTR : F8_INT_NONE );
}


static const f3853_config mk1_config =
{
	mk1_interrupt
};


static MACHINE_DRIVER_START( mk1 )
	/* basic machine hardware */
	MDRV_CPU_ADD( "maincpu", F8, MAIN_CLOCK )        /* MK3850 */
	MDRV_CPU_PROGRAM_MAP( mk1_mem)
	MDRV_CPU_IO_MAP( mk1_io)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_START( mk1 )

	MDRV_F3853_ADD( "f3853", MAIN_CLOCK, mk1_config )

    /* video hardware */
	MDRV_DEFAULT_LAYOUT( layout_mk1 )
MACHINE_DRIVER_END


ROM_START( mk1 )
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("82c210-1", 0x0000, 0x800, CRC(278f7bf3) SHA1(b384c95ba691d52dfdddd35987a71e9746a46170))
ROM_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

// seams to be developed by mostek (MK)
/*     YEAR   NAME  PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY                 FULLNAME */
CONS( 1979,  mk1,  0,		0,		mk1,	mk1,	0,		"Computer Electronic",  "Chess Champion MK I", GAME_NO_SOUND )

