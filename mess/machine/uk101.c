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

static	int	uk101_ramsize = 2;	/* 40Kb */

MACHINE_INIT( uk101 )
{
	logerror("uk101_init\r\n");

	acia6850_config (0, &uk101_acia0);

	if (readinputport(8) != uk101_ramsize)
	{
		uk101_ramsize = readinputport(8);
		switch (uk101_ramsize)
		{
			case 2:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_RAM);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_RAM);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_RAM);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_RAM);
				break;
			case 1:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_NOP);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_NOP);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_RAM);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_RAM);
				break;
			case 0:
				install_mem_write_handler(0, 0x2000, 0x9fff, MWA_NOP);
				install_mem_read_handler(0, 0x2000, 0x9fff, MRA_NOP);
				install_mem_write_handler(0, 0x1000, 0x1fff, MWA_NOP);
				install_mem_read_handler(0, 0x1000, 0x1fff, MRA_NOP);
				break;
		}
	}
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

int	uk101_init_cassette(int id, void *file, int open_mode)
{
	/* a cassette for the uk101 isnt needed */
	if (file == NULL)
	{
		logerror("UK101/Superboard - warning: no cassette specified!\n");
		return INIT_PASS;
	}                                                                                                                                     

	if (file)
	{
		uk101_tape_size = osd_fsize(file);
		uk101_tape_image = (UINT8 *) image_malloc(IO_CASSETTE, id, uk101_tape_size);
		if (!uk101_tape_image || (osd_fread(file, uk101_tape_image, uk101_tape_size) != uk101_tape_size))
		{
			return (1);
		}
		else
		{
			uk101_tape_index = 0;
			return (0);
		}
	}
	return (1);
}

void uk101_exit_cassette(int id)
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

