/***************************************************************************

  bbcbc.c

  BBC Bridge Companion

  TODO:
  Inputs
  Find correct clock frequencies

***************************************************************************/
#include "driver.h"
#include "cpu/z80/z80.h"
#include "video/tms9928a.h"
#include "machine/z80pio.h"
#include "cpu/z80/z80daisy.h"
#include "devices/cartslot.h"

static ADDRESS_MAP_START( bbcbc_prg, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_ROM
	AM_RANGE(0x04000, 0x0bfff) AM_ROM
	AM_RANGE(0x0e000, 0x0e7ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( bbcbc_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x40, 0x43) AM_DEVREADWRITE("z80pio", z80pio_r, z80pio_w)
	AM_RANGE(0x80, 0x80) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0x81, 0x81) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
ADDRESS_MAP_END

static void tms_interrupt(running_machine *machine, int dummy)
{
	cpu_set_input_line(machine->cpu[0], 0, HOLD_LINE);
}

static INTERRUPT_GEN( bbcbc_interrupt )
{
	TMS9928A_interrupt(device->machine);
}

static const TMS9928a_interface tms9129_interface =
{
	TMS9929A,
	0x4000,
	0, 0,
	tms_interrupt
};

/* TODO */
static const z80pio_interface bbcbc_z80pio_intf =
{
	NULL,	/* int callback */
	NULL,	/* port a read */
	NULL,	/* port b read */
	NULL,	/* port a write */
	NULL,	/* port b write */
	NULL,	/* ready a */
	NULL	/* ready b */
};

static const z80_daisy_chain bbcbc_daisy_chain[] =
{
	{ "z80pio" },
	{ NULL }
};

static MACHINE_START( bbcbc )
{
	TMS9928A_configure(&tms9129_interface);
}

static MACHINE_RESET( bbcbc )
{
}


static MACHINE_DRIVER_START( bbcbc )
	MDRV_CPU_ADD( "maincpu", Z80, 4000000 )
	MDRV_CPU_PROGRAM_MAP( bbcbc_prg, 0 )
	MDRV_CPU_IO_MAP( bbcbc_io, 0 )
	MDRV_CPU_CONFIG(bbcbc_daisy_chain)
	MDRV_CPU_VBLANK_INT("screen", bbcbc_interrupt)

	MDRV_MACHINE_START( bbcbc )
	MDRV_MACHINE_RESET( bbcbc )

	MDRV_Z80PIO_ADD( "z80pio", bbcbc_z80pio_intf )

	MDRV_IMPORT_FROM( tms9928a )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE( 50 )
	
	MDRV_CARTSLOT_ADD("cart")
MACHINE_DRIVER_END


ROM_START( bbcbc )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD("br_4_1.ic3", 0x0000, 0x2000, CRC(7c880d75) SHA1(954db096bd9e8edfef72946637a12f1083841fb0))
	ROM_LOAD("br_4_2.ic4", 0x2000, 0x2000, CRC(16a33aef) SHA1(9529f9f792718a3715af2063b91a5fb18f741226))
	ROM_CART_LOAD("cart", 0x4000, 0xBFFF, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*   YEAR  NAME  PARENT  COMPAT  MACHINE INPUT  INIT  CONFIG  COMPANY  FULLNAME  FLAGS */
CONS(1985, bbcbc,     0, 0,      bbcbc,  0, 	0,    0,  "BBC",   "Bridge Companion", GAME_NOT_WORKING )

