/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "image.h"
#include "cpu/m6502/m6502.h"
#include "machine/mc6850.h"
#include "includes/uk101.h"

static	int		uk101_tape_size = 0;
static	UINT8	*uk101_tape_image = 0;
static	int		uk101_tape_index = 0;

struct acia6850_interface uk101_acia0 =
{
	uk101_acia0_statin,
	uk101_acia0_casin,
	0,
	0
};

MACHINE_INIT( uk101 )
{
	logerror("uk101_init\r\n");

	acia6850_config (0, &uk101_acia0);

	cpu_setbank(1, &mess_ram[0x0000]);
	cpu_setbank(2, &mess_ram[0x1000]);
	cpu_setbank(3, &mess_ram[0x2000]);

	memory_set_bankhandler_r(1, 0, (mess_ram_size > 0x0000) ? MRA_BANK1 : MRA_ROM);
	memory_set_bankhandler_w(1, 0, (mess_ram_size > 0x0000) ? MWA_BANK1 : MWA_ROM);
	memory_set_bankhandler_r(2, 0, (mess_ram_size > 0x1000) ? MRA_BANK2 : MRA_ROM);
	memory_set_bankhandler_w(2, 0, (mess_ram_size > 0x1000) ? MWA_BANK2 : MWA_ROM);
	memory_set_bankhandler_r(3, 0, (mess_ram_size > 0x2000) ? MRA_BANK3 : MRA_ROM);
	memory_set_bankhandler_w(3, 0, (mess_ram_size > 0x2000) ? MWA_BANK3 : MWA_ROM);
}

READ_HANDLER( uk101_acia0_casin )
{
	if (uk101_tape_image && (uk101_tape_index < uk101_tape_size))
							return (uk101_tape_image[uk101_tape_index++]);
	return (0);
}

READ_HANDLER (uk101_acia0_statin )
{
	if (uk101_tape_image && (uk101_tape_index < uk101_tape_size))
							return (ACIA_6850_RDRF);
	return (0);
}

/* || */

DEVICE_LOAD( uk101_cassette )
{
	uk101_tape_size = mame_fsize(file);
	uk101_tape_image = (UINT8 *) image_malloc(image, uk101_tape_size);
	if (!uk101_tape_image || (mame_fread(file, uk101_tape_image, uk101_tape_size) != uk101_tape_size))
		return INIT_FAIL;

	uk101_tape_index = 0;
	return INIT_PASS;
}

DEVICE_UNLOAD( uk101_cassette )
{
	uk101_tape_image = NULL;
	uk101_tape_size = uk101_tape_index = 0;
}

static	INT8	uk101_keyb;

READ_HANDLER ( uk101_keyb_r )
{
	int	rpl = 0xff;
	if (!(uk101_keyb & 0x80)) rpl &= readinputport (0);
	if (!(uk101_keyb & 0x40)) rpl &= readinputport (1);
	if (!(uk101_keyb & 0x20)) rpl &= readinputport (2);
	if (!(uk101_keyb & 0x10)) rpl &= readinputport (3);
	if (!(uk101_keyb & 0x08)) rpl &= readinputport (4);
	if (!(uk101_keyb & 0x04)) rpl &= readinputport (5);
	if (!(uk101_keyb & 0x02)) rpl &= readinputport (6);
	if (!(uk101_keyb & 0x01)) rpl &= readinputport (7);

	if (rpl != 0xff) {
		logerror("keybrd: keyb: %02x. rpl: %02x.\n", uk101_keyb, rpl);
	}
	return (rpl);
}

WRITE_HANDLER ( uk101_keyb_w )
{
	uk101_keyb = data;
}

