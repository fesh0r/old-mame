/***************************************************************************

        SWTPC 6800

        10/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "machine/terminal.h"

static WRITE8_HANDLER(swtpc_terminal_w)
{
	running_device *devconf = devtag_get_device(space->machine, "terminal");
	terminal_write(devconf,0,data);
}


static ADDRESS_MAP_START(swtpc_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x8005, 0x8005 ) AM_WRITE(swtpc_terminal_w)
	AM_RANGE( 0xa000, 0xa07f ) AM_RAM
	AM_RANGE( 0xe000, 0xe3ff ) AM_MIRROR(0x1c00) AM_ROM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( swtpc )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(swtpc)
{
}

static WRITE8_DEVICE_HANDLER( swtpc_kbd_put )
{
}

static GENERIC_TERMINAL_INTERFACE( swtpc_terminal_intf )
{
	DEVCB_HANDLER(swtpc_kbd_put)
};

static MACHINE_DRIVER_START( swtpc )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu", M6800, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(swtpc_mem)

    MDRV_MACHINE_RESET(swtpc)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG, swtpc_terminal_intf)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( swtpc )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "swt", "SWTBUG" )
	ROMX_LOAD( "swtbug.bin", 0xe000, 0x0400, CRC(f9130ef4) SHA1(089b2d2a56ce9526c3e78ce5d49ce368b9eabc0c), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "mik", "MIKBUG" )
	ROMX_LOAD( "mikbug.bin", 0xe000, 0x0400, CRC(e7f4d9d0) SHA1(5ad585218f9c9c70f38b3c74e3ed5dfe0357621c), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( ????, swtpc,  0,       0, 		swtpc,	swtpc,	 0, 	"Southwest Technical Products Corporation",   "SWTPC 6800",		GAME_NOT_WORKING | GAME_NO_SOUND)

