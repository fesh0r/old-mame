/***************************************************************************

        elektor

        08/04/2010 Skeleton driver.

Similar to the VC4000. Created by Philips & 'Elektor' magazine.

Chips
- 2650A
- 2636

****************************************************************************/
#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/s2650/s2650.h"


class elektor_state : public driver_device
{
public:
	elektor_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

};


static ADDRESS_MAP_START(elektor_mem, AS_PROGRAM, 8, elektor_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x0800, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START(elektor_io, AS_IO, 8, elektor_state)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( elektor )
INPUT_PORTS_END


static MACHINE_RESET(elektor)
{
}

static VIDEO_START( elektor )
{
}

static SCREEN_UPDATE_IND16( elektor )
{
	return 0;
}

static MACHINE_CONFIG_START( elektor, elektor_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(elektor_mem)
	MCFG_CPU_IO_MAP(elektor_io)

	MCFG_MACHINE_RESET(elektor)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_VIDEO_START(elektor)
	MCFG_SCREEN_UPDATE_STATIC(elektor)
	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( elektor )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "elektor.rom", 0x0000, 0x0800, CRC(e6ef1ee1) SHA1(6823b5a22582344016415f2a37f9f3a2dc75d2a7))
ROM_END

/* Driver */

/*    YEAR  NAME      PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY    FULLNAME                     FLAGS */
COMP( 1979, elektor,  0,      0,       elektor,   elektor,  0,   "Elektor", "Elektor TV Games Computer", GAME_NOT_WORKING | GAME_NO_SOUND )
