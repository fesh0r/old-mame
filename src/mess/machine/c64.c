/***************************************************************************
	commodore c64 home computer

    peter.trauner@jk.uni-linz.ac.at
    documentation
     www.funet.fi
***************************************************************************/

/*
  unsolved problems:
   execution of code in the io devices
    (program write some short test code into the vic sprite register)
 */

#include "driver.h"

#include "cpu/m6502/m6502.h"
#include "cpu/z80/z80.h"
#include "sound/sid6581.h"
#include "machine/6526cia.h"
#include "deprecat.h"

#define VERBOSE_DBG 1
#include "includes/cbm.h"
#include "includes/cbmserb.h"
#include "includes/vc1541.h"
#include "includes/vc20tape.h"
#include "video/vic6567.h"
#include "video/vdc8563.h"

#include "includes/c128.h"	/* we need c128_bankswitch_64 in c64_m6510_port_write */
#include "includes/c64.h"

static void c64_driver_shutdown (running_machine *machine);

unsigned char c65_keyline = { 0xff };
UINT8 c65_6511_port=0xff;

UINT8 c128_keyline[3] = {0xff, 0xff, 0xff};


/* keyboard lines */
UINT8 c64_keyline[10] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* expansion port lines input */
int c64_pal = 0;
UINT8 c64_game=1, c64_exrom=1;

/* cpu port */
int c128_va1617;
UINT8 *c64_vicaddr, *c128_vicaddr;
UINT8 *c64_memory;
UINT8 *c64_colorram;
UINT8 *c64_basic;
UINT8 *c64_kernal;
UINT8 *c64_chargen;
UINT8 *c64_roml=0;
UINT8 *c64_romh=0;
static UINT8 *c64_io_mirror = NULL;

static UINT8 c64_port_data;

static UINT8 *roml=0, *romh=0;
static int ultimax = 0;
int c64_tape_on = 1;
static int c64_cia1_on = 1;
static UINT8 cartridge = 0;
static int c64_io_enabled = 0;

static enum
{
	CartridgeAuto = 0, 
	CartridgeUltimax, 
	CartridgeC64
}
cartridgetype = CartridgeAuto;

static UINT8 serial_clock, serial_data, serial_atn;
static UINT8 vicirq = 0;

static int is_c65(running_machine *machine)
{
	return !strncmp(machine->gamedrv->name, "c65", 3);
}

static int is_c128(running_machine *machine)
{
	return !strncmp(machine->gamedrv->name, "c128", 4);
}

static void c64_nmi(running_machine *machine)
{
	static int nmilevel = 0;
	int cia1irq = cia_get_irq(1);

	if (nmilevel != (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq)	/* KEY_RESTORE */
	{
		if (is_c128(machine))
		{
			if (cpu_getactivecpu()==0)
			{
				/* z80 */
				cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
			}
			else
			{
				cpunum_set_input_line(machine, 1, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
			}
		}
		
		else
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_NMI, (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq);
		}
		
		nmilevel = (input_port_read(machine, "SPECIAL") & 0x80) || cia1irq;
	}
}


/*
 * cia 0
 * port a
 * 7-0 keyboard line select
 * 7,6: paddle select( 01 port a, 10 port b)
 * 4: joystick a fire button
 * 3,2: Paddles port a fire button
 * 3-0: joystick a direction
 * port b
 * 7-0: keyboard raw values
 * 4: joystick b fire button, lightpen select
 * 3,2: paddle b fire buttons (left,right)
 * 3-0: joystick b direction
 * flag cassette read input, serial request in
 * irq to irq connected
 */
static UINT8 c64_cia0_port_a_r (void)
{
	UINT8 value = 0xff;
	UINT8 cia0portb = cia_get_output_b(0);

	if (!(cia0portb&0x80))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x80)) t&=~0x80;
		if (!(c64_keyline[6]&0x80)) t&=~0x40;
		if (!(c64_keyline[5]&0x80)) t&=~0x20;
		if (!(c64_keyline[4]&0x80)) t&=~0x10;
		if (!(c64_keyline[3]&0x80)) t&=~0x08;
		if (!(c64_keyline[2]&0x80)) t&=~0x04;
		if (!(c64_keyline[1]&0x80)) t&=~0x02;
		if (!(c64_keyline[0]&0x80)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x40))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x40)) t&=~0x80;
		if (!(c64_keyline[6]&0x40)) t&=~0x40;
		if (!(c64_keyline[5]&0x40)) t&=~0x20;
		if (!(c64_keyline[4]&0x40)) t&=~0x10;
		if (!(c64_keyline[3]&0x40)) t&=~0x08;
		if (!(c64_keyline[2]&0x40)) t&=~0x04;
		if (!(c64_keyline[1]&0x40)) t&=~0x02;
		if (!(c64_keyline[0]&0x40)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x20))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x20)) t&=~0x80;
		if (!(c64_keyline[6]&0x20)) t&=~0x40;
		if (!(c64_keyline[5]&0x20)) t&=~0x20;
		if (!(c64_keyline[4]&0x20)) t&=~0x10;
		if (!(c64_keyline[3]&0x20)) t&=~0x08;
		if (!(c64_keyline[2]&0x20)) t&=~0x04;
		if (!(c64_keyline[1]&0x20)) t&=~0x02;
		if (!(c64_keyline[0]&0x20)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x10))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x10)) t&=~0x80;
		if (!(c64_keyline[6]&0x10)) t&=~0x40;
		if (!(c64_keyline[5]&0x10)) t&=~0x20;
		if (!(c64_keyline[4]&0x10)) t&=~0x10;
		if (!(c64_keyline[3]&0x10)) t&=~0x08;
		if (!(c64_keyline[2]&0x10)) t&=~0x04;
		if (!(c64_keyline[1]&0x10)) t&=~0x02;
		if (!(c64_keyline[0]&0x10)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x08))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x08)) t&=~0x80;
		if (!(c64_keyline[6]&0x08)) t&=~0x40;
		if (!(c64_keyline[5]&0x08)) t&=~0x20;
		if (!(c64_keyline[4]&0x08)) t&=~0x10;
		if (!(c64_keyline[3]&0x08)) t&=~0x08;
		if (!(c64_keyline[2]&0x08)) t&=~0x04;
		if (!(c64_keyline[1]&0x08)) t&=~0x02;
		if (!(c64_keyline[0]&0x08)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x04))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x04)) t&=~0x80;
		if (!(c64_keyline[6]&0x04)) t&=~0x40;
		if (!(c64_keyline[5]&0x04)) t&=~0x20;
		if (!(c64_keyline[4]&0x04)) t&=~0x10;
		if (!(c64_keyline[3]&0x04)) t&=~0x08;
		if (!(c64_keyline[2]&0x04)) t&=~0x04;
		if (!(c64_keyline[1]&0x04)) t&=~0x02;
		if (!(c64_keyline[0]&0x04)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x02))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x02)) t&=~0x80;
		if (!(c64_keyline[6]&0x02)) t&=~0x40;
		if (!(c64_keyline[5]&0x02)) t&=~0x20;
		if (!(c64_keyline[4]&0x02)) t&=~0x10;
		if (!(c64_keyline[3]&0x02)) t&=~0x08;
		if (!(c64_keyline[2]&0x02)) t&=~0x04;
		if (!(c64_keyline[1]&0x02)) t&=~0x02;
		if (!(c64_keyline[0]&0x02)) t&=~0x01;
		value &=t;
	}
	if (!(cia0portb&0x01))
	{
		UINT8 t=0xff;
		if (!(c64_keyline[7]&0x01)) t&=~0x80;
		if (!(c64_keyline[6]&0x01)) t&=~0x40;
		if (!(c64_keyline[5]&0x01)) t&=~0x20;
		if (!(c64_keyline[4]&0x01)) t&=~0x10;
		if (!(c64_keyline[3]&0x01)) t&=~0x08;
		if (!(c64_keyline[2]&0x01)) t&=~0x04;
		if (!(c64_keyline[1]&0x01)) t&=~0x02;
		if (!(c64_keyline[0]&0x01)) t&=~0x01;
		value &=t;
	}

	if ( input_port_read(Machine, "DSW0") & 0x0100 )
		value &= c64_keyline[8];
	else
		value &= c64_keyline[9];

	return value;
}

static UINT8 c64_cia0_port_b_r (void)
{
    UINT8 value = 0xff;
	UINT8 cia0porta = cia_get_output_a(0);

    if (!(cia0porta & 0x80)) value &= c64_keyline[7];
    if (!(cia0porta & 0x40)) value &= c64_keyline[6];
    if (!(cia0porta & 0x20)) value &= c64_keyline[5];
    if (!(cia0porta & 0x10)) value &= c64_keyline[4];
    if (!(cia0porta & 0x08)) value &= c64_keyline[3];
    if (!(cia0porta & 0x04)) value &= c64_keyline[2];
    if (!(cia0porta & 0x02)) value &= c64_keyline[1];
    if (!(cia0porta & 0x01)) value &= c64_keyline[0];

	if ( input_port_read(Machine, "DSW0") & 0x0100 )
		value &= c64_keyline[9];
    else 
		value &= c64_keyline[8];

    if (is_c128(Machine))
    {
		if (!vic2e_k0_r ())
			value &= c128_keyline[0];
		if (!vic2e_k1_r ())
			value &= c128_keyline[1];
		if (!vic2e_k2_r ())
			value &= c128_keyline[2];
    }
    if (is_c65(Machine))
	{
		if (!(c65_6511_port&2))
			value&=c65_keyline;
    }

    return value;
}

static void c64_cia0_port_b_w (UINT8 data)
{
    vic2_lightpen_write (data & 0x10);
}

static void c64_irq (running_machine *machine, int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6510", ("irq %s\n", level ? "start" : "end"));
		if (is_c128(machine))
		{
			if (0&&(cpu_getactivecpu()==0))
			{
				cpunum_set_input_line(machine, 0, 0, level);
			}
			else
			{
				cpunum_set_input_line(machine, 1, M6510_IRQ_LINE, level);
			}
		}
		else
		{
			cpunum_set_input_line(machine, 0, M6510_IRQ_LINE, level);
		}
		old_level = level;
	}
}

WRITE8_HANDLER(c64_tape_read)
{
	cia_issue_index(machine, 0);
}

static void c64_cia0_interrupt (running_machine *machine, int level)
{
	c64_irq (machine, level || vicirq);
}

void c64_vic_interrupt (int level)
{
#if 1
	if (level != vicirq)
	{
		c64_irq (Machine, level || cia_get_irq(0));
		vicirq = level;
	}
#endif
}

/*
 * cia 1
 * port a
 * 7 serial bus data input
 * 6 serial bus clock input
 * 5 serial bus data output
 * 4 serial bus clock output
 * 3 serial bus atn output
 * 2 rs232 data output
 * 1-0 vic-chip system memory bank select
 *
 * port b
 * 7 user rs232 data set ready
 * 6 user rs232 clear to send
 * 5 user
 * 4 user rs232 carrier detect
 * 3 user rs232 ring indicator
 * 2 user rs232 data terminal ready
 * 1 user rs232 request to send
 * 0 user rs232 received data
 * flag restore key or rs232 received data input
 * irq to nmi connected ?
 */
static UINT8 c64_cia1_port_a_r (void)
{
	UINT8 value = 0xff;

	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~0x40;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~0x80;
	return value;
}

static void c64_cia1_port_a_w (UINT8 data)
{
	static const int helper[4] = {0xc000, 0x8000, 0x4000, 0x0000};

	cbm_serial_clock_write (serial_clock = !(data & 0x10));
	cbm_serial_data_write (serial_data = !(data & 0x20));
	cbm_serial_atn_write (serial_atn = !(data & 8));
	c64_vicaddr = c64_memory + helper[data & 3];
	if (is_c128(Machine))
	{
		c128_vicaddr = c64_memory + helper[data & 3] + c128_va1617;
	}
}

static void c64_cia1_interrupt (running_machine *machine, int level)
{
	c64_nmi(machine);
}

const cia6526_interface c64_cia0 =
{
	CIA6526,
	c64_cia0_interrupt,
	0.0, 60,

	{
		{ c64_cia0_port_a_r, NULL },
		{ c64_cia0_port_b_r, c64_cia0_port_b_w }
	}
};

const cia6526_interface c64_cia1 =
{
	CIA6526,
	c64_cia1_interrupt,
	0.0, 60,

	{
		{ c64_cia1_port_a_r, c64_cia1_port_a_w },
		{ 0, 0 }
	}
};

WRITE8_HANDLER( c64_write_io )
{
	c64_io_mirror[ offset ] = data;
	if (offset < 0x400) {
		vic2_port_w (machine, offset & 0x3ff, data);
	} else if (offset < 0x800) {
		sid6581_0_port_w (machine, offset & 0x3ff, data);
	} else if (offset < 0xc00)
		c64_colorram[offset & 0x3ff] = data | 0xf0;
	else if (offset < 0xd00)
		cia_0_w(machine, offset, data);
	else if (offset < 0xe00)
	{
		if (c64_cia1_on)
			cia_1_w(machine, offset, data);
		else
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
	else if (offset < 0xf00)
	{
		/* i/o 1 */
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
	else
	{
		/* i/o 2 */
			DBG_LOG (1, "io write", ("%.3x %.2x\n", offset, data));
	}
}

static UINT8 *c64_io_ram_w_ptr;
static UINT8 *c64_io_ram_r_ptr;

WRITE8_HANDLER(c64_ioarea_w)
{
	if ( c64_io_enabled ) {
		c64_write_io( machine, offset, data );
	} else {
		c64_io_ram_w_ptr[ offset ] = data;
	}
}

READ8_HANDLER( c64_read_io )
{
	if (offset < 0x400)
		return vic2_port_r (machine, offset & 0x3ff);
	else if (offset < 0x800)
		return sid6581_0_port_r (machine, offset & 0x3ff);
	else if (offset < 0xc00)
		return c64_colorram[offset & 0x3ff];
	else if (offset == 0xc00)
		{
			cia_set_port_mask_value(0, 0, input_port_read(machine, "DSW0") & 0x0100 ? c64_keyline[8] : c64_keyline[9] );
			return cia_0_r(machine, offset);
		}
	else if (offset == 0xc01)
		{
			cia_set_port_mask_value(0, 1, input_port_read(machine, "DSW0") & 0x0100 ? c64_keyline[9] : c64_keyline[8] );
			return cia_0_r(machine, offset);
		}
	else if (offset < 0xd00)
		return cia_0_r(machine, offset);
	else if (c64_cia1_on && (offset < 0xe00))
		return cia_1_r(machine, offset);
	DBG_LOG (1, "io read", ("%.3x\n", offset));
	return 0xff;
}

READ8_HANDLER(c64_ioarea_r)
{
	if ( c64_io_enabled ) {
		return c64_read_io( machine, offset );
	}
	return c64_io_ram_r_ptr[ offset ];
}

/*
 * two devices access bus, cpu and vic
 *
 * romh, roml chip select lines on expansion bus
 * loram, hiram, charen bankswitching select by cpu
 * exrom, game bankswitching select by cartridge
 * va15, va14 bank select by cpu for vic
 *
 * exrom, game: normal c64 mode
 * exrom, !game: ultimax mode
 *
 * romh: 8k rom at 0xa000 (hiram && !game && exrom)
 * or 8k ram at 0xe000 (!game exrom)
 * roml: 8k rom at 0x8000 (loram hiram !exrom)
 * or 8k ram at 0x8000 (!game exrom)
 * roml vic: upper 4k rom at 0x3000, 0x7000, 0xb000, 0xd000 (!game exrom)
 *
 * basic rom: 8k rom at 0xa000 (loram hiram game)
 * kernal rom: 8k rom at 0xe000 (hiram !exrom, hiram game)
 * char rom: 4k rom at 0xd000 (!exrom charen hiram
 * game charen !hiram loram
 * game charen hiram)
 * cpu
 *
 * (write colorram)
 * gr_w = !read&&!cas&&((address&0xf000)==0xd000)
 *
 * i_o = !game exrom !read ((address&0xf000)==0xd000)
 * !game exrom ((address&0xf000)==0xd000)
 * charen !hiram loram !read ((address&0xf000)==0xd000)
 * charen !hiram loram ((address&0xf000)==0xd000)
 * charen hiram !read ((address&0xf000)==0xd000)
 * charen hiram ((address&0xf000)==0xd000)
 *
 * vic
 * char rom: x101 (game, !exrom)
 * romh: 0011 (!game, exrom)
 *
 * exrom !game (ultimax mode)
 * addr    CPU     VIC-II
 * ----    ---     ------
 * 0000    RAM     RAM
 * 1000    -       RAM
 * 2000    -       RAM
 * 3000    -       ROMH (upper half)
 * 4000    -       RAM
 * 5000    -       RAM
 * 6000    -       RAM
 * 7000    -       ROMH
 * 8000    ROML    RAM
 * 9000    ROML    RAM
 * A000    -       RAM
 * B000    -       ROMH
 * C000    -       RAM
 * D000    I/O     RAM
 * E000    ROMH    RAM
 * F000    ROMH    ROMH
 */
static void c64_bankswitch(running_machine *machine, int reset)
{
	static int old = -1, exrom, game;
	int data, loram, hiram, charen;

	data = (UINT8) cpunum_get_info_int(0, CPUINFO_INT_M6510_PORT) & 7;
	if ((data == old)&&(exrom==c64_exrom)&&(game==c64_game)&&!reset) return;

	DBG_LOG (1, "bankswitch", ("%d\n", data & 7));
	loram = (data & 1) ? 1 : 0;
	hiram = (data & 2) ? 1 : 0;
	charen = (data & 4) ? 1 : 0;

	if ((!c64_game && c64_exrom)
	    || (loram && !c64_exrom)) // for omega race cartridge
//	    || (loram && hiram && !c64_exrom))
	{
		memory_set_bankptr (1, roml);
		memory_set_bankptr (2, c64_memory + 0x8000);
	}
	else
	{
		memory_set_bankptr (1, c64_memory + 0x8000);
		memory_set_bankptr (2, c64_memory + 0x8000);
	}

#if 1
	if ((!c64_game && !c64_exrom && hiram)
	    /*|| (!c64_exrom)*/) // must be disabled for 8kb c64 cartridges! like space action, super expander, ...
#else
	if ((!c64_game && c64_exrom && hiram)
	    || (!c64_exrom) )
#endif
	{
		memory_set_bankptr (3, romh);
	}
	else if (loram && hiram &&c64_game)
	{
		memory_set_bankptr (3, c64_basic);
	}
	else
	{
		memory_set_bankptr (3, c64_memory + 0xa000);
	}

	if ((!c64_game && c64_exrom)
		|| (charen && (loram || hiram)))
	{
		c64_io_enabled = 1;
//		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, c64_read_io);
//		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, c64_write_io);
//		if ( cpu_getactivecpu() >= 0 )
//			memory_set_opbase(activecpu_get_physical_pc_byte());
	}
	else
	{
		c64_io_enabled = 0;
		c64_io_ram_w_ptr = c64_memory + 0xd000;
		if ( !charen && (loram || hiram)) {
			c64_io_ram_r_ptr = c64_chargen;
		} else {
			c64_io_ram_r_ptr = c64_memory + 0xd000;
		}
//		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, SMH_BANK5);
//		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xd000, 0xdfff, 0, 0, SMH_BANK6);
//		memory_set_bankptr (6, c64_memory + 0xd000);
//		if (!charen && (loram || hiram))
//		{
//			memory_set_bankptr (5, c64_chargen);
//		}
//		else
//		{
//			memory_set_bankptr (5, c64_memory + 0xd000);
//		}
	}

	if (!c64_game && c64_exrom)
	{
		memory_set_bankptr (7, romh);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe000, 0xffff, 0, 0, SMH_NOP);
	}
	else
	{
		if (hiram)
			memory_set_bankptr (7, c64_kernal);
		else
			memory_set_bankptr (7, c64_memory + 0xe000);
		memory_set_bankptr (8, c64_memory + 0xe000);
	}
	/* make sure the opbase function gets called each time */
	/* NPW 15-May-2008 - Another hack in the C64 drivers broken! */
	/* opbase->mem_max = 0xcfff; */
	game = c64_game;
	exrom = c64_exrom;
	old = data;
}

/**
  ddr bit 1 port line is output
  port bit 1 port line is high

  p0 output loram
  p1 output hiram
  p2 output charen
  p3 output cassette data
  p4 input cassette switch
  p5 output cassette motor
  p6,7 not available on M6510
 */
void c64_m6510_port_write(UINT8 direction, UINT8 data)
{
	/* if line is marked as input then keep current value */
	data = ( c64_port_data & ~direction ) | ( data & direction );

	/* resistor makes cassette sense go high when P4 changes to input */
	if ( ! ( direction & 0x10 ) ) {
		data |= 0x10;
	}
	/* resistors make P0,P1,P2 go high when respective line is changed to input */
	if ( ! ( direction & 0x04 ) ) {
		data |= 0x04;
	}
	if ( ! ( direction & 0x02 ) ) {
		data |= 0x02;
	}
	if ( ! ( direction & 0x01 ) ) {
		data |= 0x01;
	}
	c64_port_data = data;

	if (c64_tape_on)
	{
		if ( direction & 0x08 ) {
			vc20_tape_write (!(data & 8));
		}
		if ( direction & 0x20 ) {
			vc20_tape_motor (data & 0x20);
		}
	}
	if (is_c128(Machine))
		c128_bankswitch_64 (Machine, 0);
	else if (is_c65(Machine))
	{
		// NPW 8-Feb-2004 - Don't know why I have to do this
		//c65_bankswitch(Machine);
	}
	else if (!ultimax)
		c64_bankswitch(Machine, 0);
	c64_memory[0x000] = program_read_byte( 0 );
	c64_memory[0x001] = program_read_byte( 1 );
}

UINT8 c64_m6510_port_read(UINT8 direction)
{
	running_machine *machine = Machine;
	UINT8 data = c64_port_data;

	if (c64_tape_on && !vc20_tape_switch())
		data &= ~0x10;
	/* WP: motor is always marked as on??? */
	data &= ~0x20;
	if (is_c128(machine)) {
		if (input_port_read(machine, "SPECIAL") & 0x20)		/* Check Caps Lock */
		{
			data &= ~0x40;
		} else {
			data |=  0x40;
		}
	}
	if (is_c65(machine)) {
		if (input_port_read(machine, "SPECIAL") & 0x20)		/* Check Caps Lock */
		{
			data &= ~0x40;
		} else {
			data |=  0x40;
		}
	}
	return data;
}

int c64_paddle_read (int which)
{
	int pot1=0xff, pot2=0xff, pot3=0xff, pot4=0xff, temp;
	UINT8 cia0porta = cia_get_output_a(0);

	if ((input_port_read(Machine, "DSW0") & 0x0e00 ) == 0x0400)					/* Paddle_2 and Paddle_3 */
	{
		if (which) 
			pot4 = (input_port_read(Machine, "PADDLE3") & 0xff);
		else 
			pot3 = (input_port_read(Machine, "PADDLE2") & 0xff);
	}
	if (( (input_port_read(Machine, "DSW0") & 0x0e00) == 0x0600 ) && which)		/* 2 Buttons Joystick_2 */
	{
		if (input_port_read(Machine, "JOY1") & 0x20 )
			pot4 = 0x00;
	}
	if ( (input_port_read(Machine, "DSW0") & 0x0e00) == 0x0800 )				/* Mouse_2 */
	{
		if (which) 
			pot4 = input_port_read(Machine, "TRACKY");
		else 
			pot3 = input_port_read(Machine, "TRACKX");
	}
	if ((input_port_read(Machine, "DSW0") & 0xe000 ) == 0x4000)					/* Paddle_0 & Paddle_1 */
	{
		if (which) 
			pot2 = (input_port_read(Machine, "PADDLE1") & 0xff);
		else 
			pot1 = (input_port_read(Machine, "PADDLE0") & 0xff);
	}
	if (( (input_port_read(Machine, "DSW0") & 0xe000) == 0x6000 ) && which)		/* 2 Buttons Joystick_1 */
	{
		if (input_port_read(Machine, "JOY0") & 0x20 )
			pot1 = 0x00;
	}
	if (( input_port_read(Machine, "DSW0") & 0xe000 ) == 0x8000 )				/* Mouse_1 */
	{	
		if (which) 
			pot2 = input_port_read(Machine, "TRACKY");
		else 
			pot1 = input_port_read(Machine, "TRACKX");
	}
	if (input_port_read(Machine, "DSW0") & 0x0100)								/* Swap */
	{
		temp = pot1; pot1 = pot2; pot2 = pot1;
		temp = pot3; pot3 = pot4; pot4 = pot3;
	}
	switch (cia0porta & 0xc0) {
	case 0x40:
		if (which) return pot2;
		return pot1;
	case 0x80:
		if (which) return pot4;
			return pot3;
	default:
		return 0;
	}
}

 READ8_HANDLER(c64_colorram_read)
{
	return c64_colorram[offset & 0x3ff];
}

WRITE8_HANDLER( c64_colorram_write )
{
	c64_colorram[offset & 0x3ff] = data | 0xf0;
}

/*
 * only 14 address lines
 * a15 and a14 portlines
 * 0x1000-0x1fff, 0x9000-0x9fff char rom
 */
static int c64_dma_read (int offset)
{
	if (!c64_game && c64_exrom)
	{
		if (offset < 0x3000)
			return c64_memory[offset];
		return c64_romh[offset & 0x1fff];
	}
	if (((c64_vicaddr - c64_memory + offset) & 0x7000) == 0x1000)
		return c64_chargen[offset & 0xfff];
	return c64_vicaddr[offset];
}

static int c64_dma_read_ultimax (int offset)
{
	if (offset < 0x3000)
		return c64_memory[offset];
	return c64_romh[offset & 0x1fff];
}

static int c64_dma_read_color (int offset)
{
	return c64_colorram[offset & 0x3ff] & 0xf;
}

static void c64_common_driver_init (running_machine *machine)
{
	/* configure the M6510 port */
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTREAD, (genf *) c64_m6510_port_read);
	cpunum_set_info_fct(0, CPUINFO_PTR_M6510_PORTWRITE, (genf *) c64_m6510_port_write);

	/*    memset(c64_memory, 0, 0xfd00); */
	if (!ultimax) {
		UINT8 *mem = memory_region(machine, REGION_CPU1);
		c64_basic=mem+0x10000;
		c64_kernal=mem+0x12000;
		c64_chargen=mem+0x14000;
		c64_colorram=mem+0x15000;
		c64_roml=mem+0x15400;
		c64_romh=mem+0x17400;
#if 0
	{0x10000, 0x11fff, SMH_ROM, &c64_basic},	/* basic at 0xa000 */
	{0x12000, 0x13fff, SMH_ROM, &c64_kernal},	/* kernal at 0xe000 */
	{0x14000, 0x14fff, SMH_ROM, &c64_chargen},	/* charrom at 0xd000 */
	{0x15000, 0x153ff, SMH_RAM, &c64_colorram},		/* colorram at 0xd800 */
	{0x15400, 0x173ff, SMH_ROM, &c64_roml},	/* basic at 0xa000 */
	{0x17400, 0x193ff, SMH_ROM, &c64_romh},	/* kernal at 0xe000 */
#endif
	}
	if (c64_tape_on)
		vc20_tape_open (c64_tape_read);

	{
		cia6526_interface cia_intf[2];

		cia_intf[0] = c64_cia0;
		cia_intf[0].tod_clock = c64_pal ? 50 : 60;
		cia_config(machine, 0, &cia_intf[0]);

		if (c64_cia1_on)
		{
			cia_intf[1] = c64_cia1;
			cia_intf[1].tod_clock = c64_pal ? 50 : 60;
			cia_config(machine, 1, &cia_intf[1]);
		}
	}

	if (ultimax)
	{
		vic6567_init (0, c64_pal, c64_dma_read_ultimax, c64_dma_read_color,
					  c64_vic_interrupt);
	}
	else
	{
		vic6567_init (0, c64_pal, c64_dma_read, c64_dma_read_color,
					  c64_vic_interrupt);
	}
	cia_reset();
	add_exit_callback(machine, c64_driver_shutdown);
}

DRIVER_INIT( c64 )
{
	c64_common_driver_init (machine);
}

DRIVER_INIT( c64pal )
{
	c64_pal = 1;
	c64_common_driver_init (machine);
}

DRIVER_INIT( ultimax )
{
	ultimax = 1;
    c64_cia1_on = 0;
	c64_common_driver_init (machine);
}

DRIVER_INIT( c64gs )
{
	c64_pal = 1;
	c64_tape_on = 0;
    c64_cia1_on = 1;
	c64_common_driver_init (machine);
}

DRIVER_INIT( sx64 )
{
	VC1541_CONFIG vc1541 = { 1, 8 };
	c64_tape_on = 0;
	c64_pal = 1;
	c64_common_driver_init (machine);
	vc1541_config (0, 0, &vc1541);
}

static void c64_driver_shutdown (running_machine *machine)
{
	if (c64_tape_on)
		vc20_tape_close ();
}

void c64_common_init_machine (running_machine *machine)
{
#ifdef VC1541
	vc1541_reset ();
#endif

	if (c64_cia1_on)
	{
		cbm_serial_reset_write (0);
		cbm_drive_0_config (SERIAL, is_c65(machine) ? 10 : 8);
		cbm_drive_1_config (SERIAL, is_c65(machine) ? 11 : 9);
		serial_clock = serial_data = serial_atn = 1;
	}
	c64_vicaddr = c64_memory;
	vicirq = 0;
}

static OPBASE_HANDLER( c64_opbase ) {
	if ( ( address & 0xf000 ) == 0xd000 ) {
		if ( c64_io_enabled ) {
			opbase->mask = 0x0fff;
			opbase->ram = c64_io_mirror;
			opbase->rom = c64_io_mirror;
			opbase->mem_min = 0x0000;
			opbase->mem_max = 0xcfff;
			c64_io_mirror[address & 0x0fff] = c64_read_io( machine, address & 0x0fff );
		} else {
			opbase->mask = 0x0fff;
			opbase->ram = c64_io_ram_r_ptr;
			opbase->rom = c64_io_ram_r_ptr;
			opbase->mem_min = 0x0000;
			opbase->mem_max = 0xcfff;
		}
		return ~0;
	}
	return address;
}

MACHINE_START( c64 )
{
	c64_port_data = 0x17;

	c64_io_mirror = auto_malloc( 0x1000 );
	c64_common_init_machine (machine);

	c64_rom_recognition ();
	c64_rom_load(machine);

	if (is_c128(machine))
		c128_bankswitch_64(machine, 1);
	if (!ultimax)
		c64_bankswitch(machine, 1);
	memory_set_opbase_handler( 0, c64_opbase );
}



#define BETWEEN(value1,value2,bottom,top) \
    ( ((value2)>=(bottom))&&((value1)<(top)) )

void c64_rom_recognition (void)
{
    int i;
	
    cartridgetype=CartridgeAuto;
    
	for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0])) && (cbm_rom[i].size!=0); i++) 
	{
		cartridge=1;
		if ( BETWEEN(0xa000, 0xbfff, cbm_rom[i].addr, cbm_rom[i].addr+cbm_rom[i].size) ) 
		{
			cartridgetype=CartridgeC64;
		} 
		else if ( BETWEEN(0xe000, 0xffff, cbm_rom[i].addr, cbm_rom[i].addr+cbm_rom[i].size) ) 
		{
			cartridgetype=CartridgeUltimax;
		}
    }
}

void c64_rom_load(running_machine *machine)
{
    int i;

    c64_exrom = 1;
    c64_game = 1;
    if (cartridge)
    {
		if (((input_port_read(machine, "CFG") & 0x1c ) == 0) && (cartridgetype == CartridgeAuto))	// AUTO_MODULE
		{
			logerror("Cartridge type not recognized using machine type\n");
		}
	
		if (((input_port_read(machine, "CFG") & 0x1c ) == 8) && (cartridgetype == CartridgeUltimax))// C64_MODULE
		{
			logerror("Cartridge could be ultimax type!?\n");
		}
	
		if (((input_port_read(machine, "CFG") & 0x1c ) == 4) && (cartridgetype == CartridgeC64))	// ULTIMAX_MODULE
		{
			logerror("Cartridge could be c64 type!?\n");
		}

		if ((input_port_read(machine, "CFG") & 0x1c ) == 8)			// C64_MODULE
			cartridgetype = CartridgeC64;

		else if ((input_port_read(machine, "CFG") & 0x1c ) == 4)	// ULTIMAX_MODULE
			cartridgetype = CartridgeUltimax;

		if ((cbm_c64_exrom!=-1)&&(cbm_c64_game!=-1)) 
		{
			c64_exrom=cbm_c64_exrom;
			c64_game=cbm_c64_game;
		} 
	
		else if (ultimax || (cartridgetype == CartridgeUltimax)) 
		{
			c64_game = 0;
		} 
	
		else 
		{
			c64_exrom = 0;
		}
	
		if (ultimax) 
		{
			for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0]))
				&&(cbm_rom[i].size!=0); i++) 
			{
				if (cbm_rom[i].addr==CBM_ROM_ADDR_LO) 
				{
					memcpy(c64_memory+0x8000+0x2000-cbm_rom[i].size,
					cbm_rom[i].chip, cbm_rom[i].size);
				} 
				else if ((cbm_rom[i].addr==CBM_ROM_ADDR_HI)
				       ||(cbm_rom[i].addr==CBM_ROM_ADDR_UNKNOWN)) 
				{
					memcpy(c64_memory+0xe000+0x2000-cbm_rom[i].size,
					cbm_rom[i].chip, cbm_rom[i].size);
				} 
				else 
				{
					memcpy(c64_memory+cbm_rom[i].addr, cbm_rom[i].chip,
					cbm_rom[i].size);
				}
			}
        } 
		
		else /*if ((cartridgetype == CartridgeC64) || (cartridgetype == CartridgeUltimax) )*/
		{
			roml=c64_roml;
			romh=c64_romh;
			memset(roml, 0, 0x2000);
			memset(romh, 0, 0x2000);
			for (i=0; (i<sizeof(cbm_rom)/sizeof(cbm_rom[0])) && (cbm_rom[i].size!=0); i++) 
			{
				if ((cbm_rom[i].addr==CBM_ROM_ADDR_UNKNOWN) ||(cbm_rom[i].addr==CBM_ROM_ADDR_LO) ) 
				{
					memcpy(roml+0x2000-cbm_rom[i].size, cbm_rom[i].chip, cbm_rom[i].size);
				} 
				else if ( ((cartridgetype == CartridgeC64) && (cbm_rom[i].addr == CBM_ROM_ADDR_HI))
			       ||((cartridgetype == CartridgeUltimax) && (cbm_rom[i].addr == CBM_ROM_ADDR_HI)) ) 
				{
					memcpy(romh+0x2000-cbm_rom[i].size, cbm_rom[i].chip, cbm_rom[i].size);
				} 
				else if (cbm_rom[i].addr<0xc000) 
				{
					memcpy(roml+cbm_rom[i].addr-0x8000, cbm_rom[i].chip, cbm_rom[i].size);
				} 
				else 
				{
					memcpy(romh+cbm_rom[i].addr-0xe000, cbm_rom[i].chip, cbm_rom[i].size);
				}
			}
		}
    }
}

INTERRUPT_GEN( c64_frame_interrupt )
{
	static int monitor=-1;
	int value, i;
	char port[6], port2[5];

	c64_nmi(machine);

	 if (is_c128(machine))
	 {
	 	if ((input_port_read(machine, "CFG") & 0x20) != monitor)
		{
			if (input_port_read(machine, "CFG") & 0x20)
			{
				vic2_set_rastering(0);
				vdc8563_set_rastering(1);
				video_screen_set_visarea(machine->primary_screen, 0, 655, 0, 215);
			}
			else
			{
				vic2_set_rastering(1);
				vdc8563_set_rastering(0);
				video_screen_set_visarea(machine->primary_screen, 0, 335, 0, 215);
			}
			monitor = input_port_read(machine, "CFG") & 0x20;
		}
	}

	/* Lines 0-7 : common keyboard */
	for (i=0; i<8; i++)
	{
		value = 0xff;
		sprintf(port, "ROW%d", i);
		value &= ~input_port_read(machine, port);
		c64_keyline[i] = value;
	}

	value = 0xff;
	if ( ((input_port_read(machine, "DSW0") & 0xe000 ) == 0x2000) || ((input_port_read(machine, "DSW0") & 0xe000 ) == 0x6000) ) 
	{
		value &= ~(input_port_read(machine, "JOY0") & 0x1f);
	} 
	else if ((input_port_read(machine, "DSW0") & 0xe000 ) == 0x4000)
	{
		if (input_port_read(machine, "PADDLE1") & 0x100)
			value &= ~8;
		if (input_port_read(machine, "PADDLE0") & 0x100)
			value &= ~4;
	} 
	else if (( input_port_read(machine, "DSW0") & 0xe000 ) == 0x8000 )
	{
			if (input_port_read(machine, "TRACKIPT") & 0x02)
				value &= ~0x10;
			if (input_port_read(machine, "TRACKIPT") & 0x01)
				value &= ~1;
	}
	c64_keyline[8] = value;

	value = 0xff;
	if ( ((input_port_read(machine, "DSW0") & 0x0e00 ) == 0x0200) || ((input_port_read(machine, "DSW0") & 0x0e00 ) == 0x0600) ) 
	{
		value &= ~(input_port_read(machine, "JOY1") & 0x1f);
	} 
	else if ((input_port_read(machine, "DSW0") & 0x0e00 ) == 0x0400)
	{
		if (input_port_read(machine, "PADDLE3") & 0x100)
			value &= ~8;
		if (input_port_read(machine, "PADDLE2") & 0x100)
			value &= ~4;
	} 
	else if (( input_port_read(machine, "DSW0") & 0x0e00 ) == 0x0800 )
	{
			if (input_port_read(machine, "TRACKIPT") & 0x02)
				value &= ~0x10;
			if (input_port_read(machine, "TRACKIPT") & 0x01)
				value &= ~1;
	}
	c64_keyline[9] = value;

	/* C128 only : keypad input ports */
	if (is_c128(machine)) 
	{
		for (i=0; i<3; i++)
		{
			value = 0xff;
			sprintf(port2, "KP%d", i);
			value &= ~input_port_read(machine, port2);
			c128_keyline[i] = value;
		}
	}

	/* C65 only : function keys input ports */
	if (is_c65(machine)) 
	{
		value = 0xff;

		value &= ~input_port_read(machine, "FUNCT");
		c65_keyline = value;
	}


	vic2_frame_interrupt (machine, cpunum);

	if (c64_tape_on) 
	{
		vc20_tape_config (input_port_read(machine, "CFG") & 0x4000, input_port_read(machine, "DSW0") & 0x2000);		/* DATASSETTE, DATASSETTE_TONE */
		vc20_tape_buttons (input_port_read(machine, "CFG") & 0x1000, input_port_read(machine, "DSW0") & 0x0800, input_port_read(machine, "DSW0") & 0x0400);	/* DATASETTE_PLAY, DATASETTE_RECORD, DATASETTE_STOP */
	}

	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x40 ? 1 : 0);		/*KB_CAPSLOCK_FLAG */
	set_led_status (0, input_port_read(machine, "DSW0") & 0x0100 ? 1 : 0);		/*KB_NUMLOCK_FLAG */ 
}
