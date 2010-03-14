/***************************************************************************

        Microsystems International Limited MOD-8

        02/12/2009 Working driver [Miodrag Milanovic]
        18/11/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8008/i8008.h"
#include "machine/teleprinter.h"

class mod8_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, mod8_state(machine)); }

	mod8_state(running_machine &machine) { }

	UINT16 tty_data;
	UINT8 tty_key_data;
	int tty_cnt;
};

static WRITE8_HANDLER(out_w)
{
	mod8_state *state = (mod8_state *)space->machine->driver_data;
	running_device *devconf = devtag_get_device(space->machine, TELEPRINTER_TAG);

	state->tty_data >>= 1;
	state->tty_data |= (data & 0x01) ? 0x8000 : 0;
	state->tty_cnt++;
	if (state->tty_cnt==10) {
		teleprinter_write(devconf,0,(state->tty_data >> 7) & 0x7f);
		state->tty_cnt = 0;
	}
}

static WRITE8_HANDLER(tty_w)
{
	mod8_state *state = (mod8_state *)space->machine->driver_data;

	state->tty_data = 0;
	state->tty_cnt = 0;
}

static READ8_HANDLER(tty_r)
{
	mod8_state *state = (mod8_state *)space->machine->driver_data;
	UINT8 d = state->tty_key_data & 0x01;

	state->tty_key_data >>= 1;
	return d;
}

static ADDRESS_MAP_START(mod8_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000,0x6ff) AM_ROM
	AM_RANGE(0x700,0xfff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( mod8_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00,0x00) AM_READ(tty_r)
	AM_RANGE(0x0a,0x0a) AM_WRITE(out_w)
	AM_RANGE(0x0b,0x0b) AM_WRITE(tty_w)
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mod8 )
	PORT_INCLUDE(generic_teleprinter)
INPUT_PORTS_END

static IRQ_CALLBACK ( mod8_irq_callback )
{
	return 0xC0; // LAA - NOP equivalent
}

static MACHINE_RESET(mod8)
{
	cpu_set_irq_callback(devtag_get_device(machine, "maincpu"), mod8_irq_callback);
}

static WRITE8_DEVICE_HANDLER( mod8_kbd_put )
{
	mod8_state *state = (mod8_state *)device->machine->driver_data;

	state->tty_key_data = data ^ 0xff;
	cputag_set_input_line(device->machine, "maincpu", 0, HOLD_LINE);
}

static GENERIC_TELEPRINTER_INTERFACE( mod8_teleprinter_intf )
{
	DEVCB_HANDLER(mod8_kbd_put)
};

static MACHINE_DRIVER_START( mod8 )

    MDRV_DRIVER_DATA( mod8_state )

    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8008, 800000)
    MDRV_CPU_PROGRAM_MAP(mod8_mem)
    MDRV_CPU_IO_MAP(mod8_io)

    MDRV_MACHINE_RESET(mod8)

    /* video hardware */
    MDRV_IMPORT_FROM( generic_teleprinter )
	MDRV_GENERIC_TELEPRINTER_ADD(TELEPRINTER_TAG,mod8_teleprinter_intf)

MACHINE_DRIVER_END


/* ROM definition */
ROM_START( mod8 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon8.001", 0x0000, 0x0100, CRC(b82ac6b8) SHA1(fbea5a6dd4c779ca1671d84089f857a3f548ffcb))
	ROM_LOAD( "mon8.002", 0x0100, 0x0100, CRC(8b82bc3c) SHA1(66222511527b27e56a5a1f9656d424d407eac7d3))
	ROM_LOAD( "mon8.003", 0x0200, 0x0100, CRC(679ae913) SHA1(22423efcb9051c9812fcbac9a27af70415d0dd81))
	ROM_LOAD( "mon8.004", 0x0300, 0x0100, CRC(2a4e580f) SHA1(8b0cb9660fde3cacd299faaa31724e4f3262d77f))
	ROM_LOAD( "mon8.005", 0x0400, 0x0100, CRC(e281bb1a) SHA1(cc7c2746e075512dbf5eed88ae3aea009558dbd0))
	ROM_LOAD( "mon8.006", 0x0500, 0x0100, CRC(b7e2f585) SHA1(5408adabc3df6e6ea8dcfb2327b2883b435ab85e))
	ROM_LOAD( "mon8.007", 0x0600, 0x0100, CRC(49a5c626) SHA1(66c1865db9151818d3b20ec3c68dd793cb98a221))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( 1974, mod8,   0,       0, 		mod8,	mod8,	 0, 	"Microsystems International Ltd.",   "MOD-8",		GAME_NO_SOUND)

