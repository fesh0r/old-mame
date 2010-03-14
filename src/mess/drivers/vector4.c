/***************************************************************************

        Vector 4

        08/12/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/terminal.h"

static UINT8 received_char = 0;

static WRITE8_HANDLER(vector_terminal_w)
{
	running_device *devconf = devtag_get_device(space->machine, "terminal");
	terminal_write(devconf,0,data);
}

static READ8_HANDLER(vector_terminal_status_r)
{
	if (received_char!=0) return 3; // char received
	return 1; // ready
}

static READ8_HANDLER(vector_terminal_r)
{
	UINT8 retVal = received_char;
	received_char = 0;
	return retVal;
}

static ADDRESS_MAP_START(vector4_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf8ff) AM_ROM
	AM_RANGE(0xfc00, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vector4_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x02, 0x02) AM_READWRITE(vector_terminal_r,vector_terminal_w)
	AM_RANGE(0x03, 0x03) AM_READ(vector_terminal_status_r)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( vector4 )
	PORT_INCLUDE(generic_terminal)
INPUT_PORTS_END


static MACHINE_RESET(vector4)
{
	received_char = 0;
	cpu_set_reg(devtag_get_device(machine, "maincpu"), Z80_PC, 0xe000);
}

static WRITE8_DEVICE_HANDLER( vector4_kbd_put )
{
	received_char = data;
}

static GENERIC_TERMINAL_INTERFACE( vector4_terminal_intf )
{
	DEVCB_HANDLER(vector4_kbd_put)
};


static MACHINE_DRIVER_START( vector4 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(vector4_mem)
    MDRV_CPU_IO_MAP(vector4_io)

    MDRV_MACHINE_RESET(vector4)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_terminal )
	MDRV_GENERIC_TERMINAL_ADD(TERMINAL_TAG,vector4_terminal_intf)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( vector4 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v4c", "ver 4.0c" )
    ROMX_LOAD( "vg40cl_ihl.bin", 0xe000, 0x0400, CRC(dcaf79e6) SHA1(63619ddb12ff51e5862902fb1b33a6630f555ad7), ROM_BIOS(1))
	ROMX_LOAD( "vg40ch_ihl.bin", 0xe400, 0x0400, CRC(3ff97d70) SHA1(b401e49aa97ac106c2fd5ee72d89e683ebe34e34), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v43", "ver 4.3" )
    ROMX_LOAD( "vg-em-43.bin",   0xe000, 0x1000, CRC(29a0fcee) SHA1(ca44de527f525b72f78b1c084c39aa6ce21731b5), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v5", "ver 5.0" )
	ROMX_LOAD( "vg-zcb50.bin",   0xe000, 0x1000, CRC(22d692ce) SHA1(cbb21b0acc98983bf5febd59ff67615d71596e36), ROM_BIOS(3))
    ROM_LOAD( "mfdc.bin", 0xf800, 0x0100, CRC(d82a40d6) SHA1(cd1ef5fb0312cd1640e0853d2442d7d858bc3e3b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT        COMPANY                 FULLNAME       FLAGS */
COMP( ????, vector4,  0,       0,	vector4,	vector4,	0,  	 "Vector Graphics",   "Vector 4",		GAME_NOT_WORKING | GAME_NO_SOUND)

