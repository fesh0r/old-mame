/***************************************************************************
	commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"

#include "includes/cbm.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/cbmdrive.h"

#include "includes/pet.h"

#include "devices/cartslot.h"
#include "devices/cassette.h"

#define VERBOSE_LEVEL 0
#define DBG_LOG(N,M,A) \
	{ \
		if(VERBOSE_LEVEL >= N) \
		{ \
			if( M ) \
				logerror("%11.6f: %-24s", attotime_to_double(timer_get_time()), (char*) M ); \
			logerror A; \
		} \
	}

/* keyboard lines */
static int pet_basic1 = 0; /* basic version 1 for quickloader */
static int superpet = 0;
static int cbm8096 = 0;
static int pet_keyline_select;

int pet_font = 0;
UINT8 *pet_memory;
UINT8 *superpet_memory;
UINT8 *pet_videoram;
static UINT8 *pet80_bank1_base;

static emu_timer *datasette1_timer;
static emu_timer *datasette2_timer;


static READ8_HANDLER( pet_mc6845_register_r )
{
	device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
	return mc6845_register_r(devconf, offset);
}

static WRITE8_HANDLER( pet_mc6845_register_w )
{
	device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
	mc6845_register_w(devconf, offset, data);
}

static WRITE8_HANDLER( pet_mc6845_address_w )
{
	device_config *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, "crtc");
	mc6845_address_w(devconf, offset, data);
}

/* pia at 0xe810
   port a
    7 sense input (low for diagnostics)
    6 ieee eoi in
    5 cassette 2 switch in
    4 cassette 1 switch in
    3..0 keyboard line select

  ca1 cassette 1 read
  ca2 ieee eoi out

  cb1 video sync in
  cb2 cassette 1 motor out
*/
static  READ8_HANDLER ( pet_pia0_port_a_read )
{
	int data = 0xf0 | pet_keyline_select;

	if ((cassette_get_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette1" )) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x10;

	if ((cassette_get_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette2" )) & CASSETTE_MASK_UISTATE) != CASSETTE_STOPPED)
		data &= ~0x20;

	if (!cbm_ieee_eoi_r()) 
		data &= ~0x40;

	return data;
}

static WRITE8_HANDLER ( pet_pia0_port_a_write )
{
	pet_keyline_select = data & 0x0f;
}

/* Keyboard reading/handling for regular keyboard */
static  READ8_HANDLER ( pet_pia0_port_b_read )
{
	UINT8 data = 0xff;
	static const char *keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", 
										"ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };
	
	if (pet_keyline_select < 10) 
	{
		data = input_port_read(machine, keynames[pet_keyline_select]);
		/* Check for left-shift lock */
		if ((pet_keyline_select == 8) && (input_port_read(machine, "SPECIAL") & 0x80)) 
			data &= 0xfe;
	}
	return data;
}

/* Keyboard handling for business keyboard */
static READ8_HANDLER( petb_pia0_port_b_read )
{
	UINT8 data = 0xff;
	static const char *keynames[] = { "ROW0", "ROW1", "ROW2", "ROW3", "ROW4", 
										"ROW5", "ROW6", "ROW7", "ROW8", "ROW9" };
	
	if (pet_keyline_select < 10) 
	{
		data = input_port_read(machine, keynames[pet_keyline_select]);
		/* Check for left-shift lock */
		/* 2008-05 FP: For some reason, superpet read it in the opposite way!! */
		/* While waiting for confirmation from docs, we add a workaround here. */
		if (superpet)
		{
			if ((pet_keyline_select == 6) && !(input_port_read(machine, "SPECIAL") & 0x80)) 
				data &= 0xfe;
		}		
		else
		{
			if ((pet_keyline_select == 6) && (input_port_read(machine, "SPECIAL") & 0x80)) 
				data &= 0xfe;
		}
	}
	return data;
}

/* NOT WORKING - Just placeholder */
static READ8_HANDLER( pet_pia0_ca1_in )
{
	// cassette 1 read
	return (cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette1" )) > +0.0) ? 1 : 0;
}


static WRITE8_HANDLER( pet_pia0_ca2_out )
{
	cbm_ieee_eoi_w(0, data);
}

static WRITE8_HANDLER( pet_pia0_cb2_out )
{
	if (!data)
	{
		cassette_change_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette1" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(datasette1_timer, attotime_zero, 0, ATTOTIME_IN_HZ(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette1" ),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		timer_reset(datasette1_timer, attotime_never);
	}
}


static void pet_irq (running_machine *machine, int level)
{
	static int old_level = 0;

	if (level != old_level)
	{
		DBG_LOG (3, "mos6502", ("irq %s\n", level ? "start" : "end"));
		if (superpet)
			cpunum_set_input_line(machine, 1, M6809_IRQ_LINE, level);
		cpunum_set_input_line(machine, 0, M6502_IRQ_LINE, level);
		old_level = level;
	}
}

/* pia at 0xe820 (ieee488)
   port a data in
   port b data out
  ca1 atn in
  ca2 ndac out
  cb1 srq in
  cb2 dav out
 */
static  READ8_HANDLER ( pet_pia1_port_a_read )
{
	return cbm_ieee_data_r();
}

static WRITE8_HANDLER ( pet_pia1_port_b_write )
{
	cbm_ieee_data_w(0, data);
}

static  READ8_HANDLER ( pet_pia1_ca1_read )
{
	return cbm_ieee_atn_r();
}

static WRITE8_HANDLER ( pet_pia1_ca2_write )
{
	cbm_ieee_ndac_w(0, data);
}

static WRITE8_HANDLER ( pet_pia1_cb2_write )
{
	cbm_ieee_dav_w(0, data);
}

static  READ8_HANDLER ( pet_pia1_cb1_read )
{
	return cbm_ieee_srq_r();
}

static const pia6821_interface pet_pia0 =
{
	pet_pia0_port_a_read,		/* in_a_func */
	pet_pia0_port_b_read,		/* in_b_func */
	pet_pia0_ca1_in,			/* in_ca1_func */
	NULL,						/* in_cb1_func */
	NULL,						/* in_ca2_func */
	NULL,						/* in_cb2_func */
	pet_pia0_port_a_write,		/* out_a_func */
	NULL,						/* out_b_func */
	pet_pia0_ca2_out,			/* out_ca2_func */
	pet_pia0_cb2_out,			/* out_cb2_func */
	NULL,						/* irq_a_func */
	pet_irq						/* irq_b_func */
};

static const pia6821_interface petb_pia0 =
{
	pet_pia0_port_a_read,		/* in_a_func */
	petb_pia0_port_b_read,		/* in_b_func */
	pet_pia0_ca1_in,			/* in_ca1_func */
	NULL,						/* in_cb1_func */
	NULL,						/* in_ca2_func */
	NULL,						/* in_cb2_func */
	pet_pia0_port_a_write,		/* out_a_func */
	NULL,						/* out_b_func */
	pet_pia0_ca2_out,			/* out_ca2_func */
	pet_pia0_cb2_out,			/* out_cb2_func */
	NULL,						/* irq_a_func */
	pet_irq						/* irq_b_func */
};

static const pia6821_interface pet_pia1 =
{
	pet_pia1_port_a_read,		/* in_a_func */
	NULL,						/* in_b_func */
    pet_pia1_ca1_read,			/* in_ca1_func */
    pet_pia1_cb1_read,			/* in_cb1_func */
	NULL,						/* in_ca2_func */
	NULL,						/* in_cb2_func */
	NULL,						/* out_a_func */
    pet_pia1_port_b_write,		/* out_b_func */
    pet_pia1_ca2_write,			/* out_ca2_func */
    pet_pia1_cb2_write,			/* out_cb2_func */
};

static WRITE8_HANDLER( pet_address_line_11 )
{
	DBG_LOG (1, "address line", ("%d\n", data));
	if (data) pet_font |= 1;
	else pet_font &= ~1;
}

/* userport, cassettes, rest ieee488
   ca1 userport
   ca2 character rom address line 11
   pa user port

   pb0 ieee ndac in
   pb1 ieee nrfd out
   pb2 ieee atn out
   pb3 userport/cassettes
   pb4 cassettes
   pb5 userport/???
   pb6 ieee nrfd in
   pb7 ieee dav in

   cb1 cassettes
   cb2 user port
 */
static  READ8_HANDLER( pet_via_port_b_r )
{
	UINT8 data = 0;

	if (cbm_ieee_ndac_r()) data |= 0x01;

	//	data & 0x08 -> cassette write (it seems to BOTH cassettes from schematics)

	if (!(data & 0x10))
	{
		cassette_change_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette2" ),CASSETTE_MOTOR_ENABLED,CASSETTE_MASK_MOTOR);
		timer_adjust_periodic(datasette2_timer, attotime_zero, 0, ATTOTIME_IN_HZ(48000));	// I put 48000 because I was given some .wav with this freq
	}
	else
	{
		cassette_change_state(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette2" ),CASSETTE_MOTOR_DISABLED ,CASSETTE_MASK_MOTOR);
		timer_reset(datasette2_timer, attotime_never);
	}

	if (cbm_ieee_nrfd_r()) data |= 0x40;

	if (cbm_ieee_dav_r()) data |= 0x80;

	return data;
}

/* NOT WORKING - Just placeholder */
static READ8_HANDLER( pet_via_cb1_r )
{
	// cassette 2 read
	return (cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette2" )) > +0.0) ? 1 : 0;
}


static WRITE8_HANDLER( pet_via_port_b_w )
{
	cbm_ieee_nrfd_w(0, data & 2);
	cbm_ieee_atn_w(0, data & 4);
}


static const struct via6522_interface pet_via =
{
	NULL,					/* in_a_func */
	pet_via_port_b_r,		/* in_b_func */
	NULL,					/* in_ca1_func */
	pet_via_cb1_r,			/* in_cb1_func */
	NULL,					/* in_ca2_func */
	NULL,					/* in_cb2_func */
	NULL,					/* out_a_func */
	pet_via_port_b_w,		/* out_b_func */
	pet_address_line_11		/* out_ca2_func */
};

static struct {
	int bank; /* rambank to be switched in 0x9000 */
	int rom; /* rom socket 6502? at 0x9000 */
} spet= { 0 };

static WRITE8_HANDLER(cbm8096_io_w)
{
	if (offset < 0x10) ;
	else if (offset < 0x14) pia_0_w(machine, offset & 3, data);
	else if (offset < 0x20) ;
	else if (offset < 0x24) pia_1_w(machine, offset & 3, data);
	else if (offset < 0x40) ;
	else if (offset < 0x50) via_0_w(machine, offset & 0xf, data);
	else if (offset < 0x80) ;
	else if (offset == 0x80) pet_mc6845_address_w(machine, offset, data);
	else if (offset == 0x81) pet_mc6845_register_w(machine, offset, data);
}

static READ8_HANDLER(cbm8096_io_r)
{
	int data = 0xff;
	if (offset < 0x10) ;
	else if (offset < 0x14) data = pia_0_r(machine, offset & 3);
	else if (offset < 0x20) ;
	else if (offset < 0x24) data = pia_1_r(machine, offset & 3);
	else if (offset < 0x40) ;
	else if (offset < 0x50) data = via_0_r(machine, offset & 0xf);
	else if (offset < 0x80) ;
	else if (offset == 0x81) data = pet_mc6845_register_r(machine, offset);
	return data;
}

static WRITE8_HANDLER(pet80_bank1_w) {
	pet80_bank1_base[offset] = data;
}

/*
65520        8096 memory control register
        bit 0    1=write protect $8000-BFFF
        bit 1    1=write protect $C000-FFFF
        bit 2    $8000-BFFF bank select
        bit 3    $C000-FFFF bank select
        bit 5    1=screen peek through
        bit 6    1=I/O peek through
        bit 7    1=enable expansion memory

*/
WRITE8_HANDLER(cbm8096_w)
{
	read8_machine_func rh;
	write8_machine_func wh;

	if (data & 0x80)
	{
		if (data & 0x40)
		{
			rh = cbm8096_io_r;
			wh = cbm8096_io_w;
		}
		else
		{
			rh = SMH_BANK7;
			if (!(data & 2))
				wh = SMH_BANK7;
			else
				wh = SMH_NOP;
		}
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, rh);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, wh);

		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xe7ff, 0, 0, (data & 2) == 0 ? SMH_BANK6 : SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffef, 0, 0, (data & 2) == 0 ? SMH_BANK8 : SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfff1, 0xffff, 0, 0, (data & 2) == 0 ? SMH_BANK9 : SMH_NOP);

		if (data & 0x20)
		{
			pet80_bank1_base = pet_memory + 0x8000;
			memory_set_bankptr(1, pet80_bank1_base);
			wh = pet80_bank1_w;
		}
		else
		{
			if (!(data & 1))
				wh = SMH_BANK1;
			else
				wh = SMH_NOP;
		}
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, wh);

		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, 0, (data & 1) == 0 ? SMH_BANK2 : SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, (data & 1) == 0 ? SMH_BANK3 : SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, 0, (data & 1) == 0 ? SMH_BANK4 : SMH_NOP);


		if (data & 4) 
		{
			if (!(data & 0x20)) 
			{
				pet80_bank1_base = pet_memory + 0x14000;
				memory_set_bankptr(1, pet80_bank1_base);
			}
			memory_set_bankptr(2, pet_memory + 0x15000);
			memory_set_bankptr(3, pet_memory + 0x16000);
			memory_set_bankptr(4, pet_memory + 0x17000);
		} 
		else 
		{
			if (!(data & 0x20)) 
			{
				pet80_bank1_base = pet_memory + 0x10000;
				memory_set_bankptr(1, pet80_bank1_base);
			}
			memory_set_bankptr(2, pet_memory + 0x11000);
			memory_set_bankptr(3, pet_memory + 0x12000);
			memory_set_bankptr(4, pet_memory + 0x13000);
		}

		if (data & 8) 
		{
			if (!(data & 0x40)) 
			{
				memory_set_bankptr(7, pet_memory + 0x1e800);
			}
			memory_set_bankptr(6, pet_memory + 0x1c000);
			memory_set_bankptr(8, pet_memory + 0x1f000);
			memory_set_bankptr(9, pet_memory + 0x1fff1);
		} 
		else 
		{
			if (!(data & 0x40)) 
			{
				memory_set_bankptr(7, pet_memory+ 0x1a800);
			}
			memory_set_bankptr(6, pet_memory + 0x18000);
			memory_set_bankptr(8, pet_memory + 0x1b000);
			memory_set_bankptr(9, pet_memory + 0x1bff1);
		}
	}
	else
	{
		pet80_bank1_base = pet_memory + 0x8000;
		memory_set_bankptr(1, pet80_bank1_base );
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8fff, 0, 0, pet80_bank1_w);

		memory_set_bankptr(2, pet_memory + 0x9000);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9fff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(3, pet_memory + 0xa000);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xa000, 0xafff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(4, pet_memory + 0xb000);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xbfff, 0, 0, SMH_UNMAP);

		memory_set_bankptr(6, pet_memory + 0xc000);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xc000, 0xe7ff, 0, 0, SMH_UNMAP);

		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, cbm8096_io_r);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xe800, 0xefff, 0, 0, cbm8096_io_w);

		memory_set_bankptr(8, pet_memory + 0xf000);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xf000, 0xffef, 0, 0, SMH_UNMAP);

		memory_set_bankptr(9, pet_memory + 0xfff1);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfff1, 0xffff, 0, 0, SMH_UNMAP);
	}
}

READ8_HANDLER(superpet_r)
{
	return 0xff;
}

WRITE8_HANDLER(superpet_w)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 3: 1 pull down diagnostic pin on the userport
			   1: 1 if jumpered programable ram r/w
			   0: 0 if jumpered programable m6809, 1 m6502 selected */
			break;

		case 4:
		case 5:
			spet.bank = data & 0xf;
			memory_configure_bank(1, 0, 16, superpet_memory, 0x1000);
			memory_set_bank(1, spet.bank);
			/* 7 low writeprotects systemlatch */
			break;

		case 6:
		case 7:
			spet.rom = data & 1;
			break;
	}
}

static TIMER_CALLBACK(pet_interrupt)
{
	static int level = 0;

	pia_0_cb1_w(machine, 0, level);
	level = !level;
}


/* NOT WORKING - Just placeholder */
static TIMER_CALLBACK( pet_tape1_timer )
{
//	cassette 1
	UINT8 data = (cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette1" )) > +0.0) ? 1 : 0;
	pia_0_ca1_w(machine, 0, data);
}

/* NOT WORKING - Just placeholder */
static TIMER_CALLBACK( pet_tape2_timer )
{
//	cassette 2
	UINT8 data = (cassette_input(device_list_find_by_tag( machine->config->devicelist, CASSETTE, "cassette2" )) > +0.0) ? 1 : 0;
	via_0_cb1_w(machine, 0, data);
}


static void pet_common_driver_init(running_machine *machine)
{
	int i;

	/* BIG HACK; need to phase out this retarded memory management */
	if (!pet_memory)
		pet_memory = mess_ram;

	memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK10);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK10);
	memory_set_bankptr(10, pet_memory);

	if (mess_ram_size < 0x8000)
	{
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x7FFF, 0, 0, SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x7FFF, 0, 0, SMH_NOP);
	}

	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < mess_ram_size; i += 0x40)
	{
		memset (pet_memory + i, i & 0x40 ? 0 : 0xff, 0x40);
	}

	/* pet clock */
	timer_pulse(ATTOTIME_IN_MSEC(10), NULL, 0, pet_interrupt);

	/* datasette */
	datasette1_timer = timer_alloc(pet_tape1_timer, NULL);
	datasette2_timer = timer_alloc(pet_tape2_timer, NULL);


	via_config(0, &pet_via);
	pia_config(1, &pet_pia1);

	cbm_ieee_open();
}


DRIVER_INIT( pet2001 )
{
	pet_basic1 = 1;
	pet_common_driver_init(machine);
	pia_config(0, &pet_pia0);
	pet_vh_init(machine);
}

DRIVER_INIT( pet )
{
	pet_common_driver_init(machine);
	pia_config(0, &pet_pia0);
	pet_vh_init(machine);
}

DRIVER_INIT( petb )
{
	pet_common_driver_init(machine);
	pia_config(0, &petb_pia0);
	pet_vh_init(machine);
}

DRIVER_INIT( pet40 )
{
	pet_common_driver_init(machine);
	pia_config(0, &pet_pia0);
	pet_vh_init(machine);
}

DRIVER_INIT( pet80 )
{
	cbm8096 = 1;
	pet_memory = memory_region(machine, "main");

	pet_common_driver_init(machine);
	pia_config(0, &petb_pia0);
	videoram = &pet_memory[0x8000];
	videoram_size = 0x800;
	pet80_vh_init(machine);
}

DRIVER_INIT( superpet )
{
	superpet = 1;
	pet_common_driver_init(machine);
	pia_config(0, &petb_pia0);

	superpet_memory = auto_malloc(0x10000);

	memory_configure_bank(1, 0, 16, superpet_memory, 0x1000);
	memory_set_bank(1, 0);

	superpet_vh_init(machine);
}

MACHINE_RESET( pet )
{
	via_reset();
	pia_reset();

	if (superpet)
	{
		spet.rom = 0;
		if (input_port_read(machine, "CFG") & 0x04)
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 1);
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 0);
			pet_font = 2;
		}
		else
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 0);
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 1);
			pet_font = 0;
		}
	}

	if (cbm8096)
	{
		if (input_port_read(machine, "CFG") & 0x08)
		{
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfff0, 0xfff0, 0, 0, cbm8096_w);
		}
		else
		{
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xfff0, 0xfff0, 0, 0, SMH_NOP);
		}
		cbm8096_w(machine, 0, 0);
	}

	cbm_drive_0_config (input_port_read(machine, "CFG") & 2 ? IEEE : 0, 8);
	cbm_drive_1_config (input_port_read(machine, "CFG") & 1 ? IEEE : 0, 9);
}


INTERRUPT_GEN( pet_frame_interrupt )
{
	if (superpet)
	{
		if (input_port_read(machine, "CFG") & 0x04) 
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 1);
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 0);
			pet_font |= 2;
		} 
		else 
		{
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 0);
			cpunum_set_input_line(machine, 0, INPUT_LINE_HALT, 1);
			pet_font &= ~2;
		}
	}

	set_led_status (1, input_port_read(machine, "SPECIAL") & 0x80 ? 1 : 0);		/* Shift Lock */
}


/***********************************************

	PET Cartridges

***********************************************/


static CBM_ROM pet_cbm_cart[0x20] = { {0} };


static DEVICE_IMAGE_LOAD(pet_cart)
{
	int size = image_length(image), test;
	const char *filetype;
	int address = 0;

	filetype = image_filetype(image);

 	if (!mame_stricmp (filetype, "crt"))
	{
	/* We temporarily remove .crt loading. Previous versions directly used 
	the same routines used to load C64 .crt file, but I seriously doubt the
	formats are compatible. While waiting for confirmation about .crt dumps
	for PET machines, we simply do not load .crt files */
	}
	else 
	{
		/* Assign loading address according to extension */
		if (!mame_stricmp (filetype, "a0"))
			address = 0xa000;

		else if (!mame_stricmp (filetype, "b0"))
			address = 0xb000;

		logerror("Loading cart %s at %.4x size:%.4x\n", image_filename(image), address, size);

		/* Does cart contain any data? */
		pet_cbm_cart[0].chip = (UINT8*) image_malloc(image, size);
		if (!pet_cbm_cart[0].chip)
			return INIT_FAIL;

		/* Store data, address & size */
		pet_cbm_cart[0].addr = address;
		pet_cbm_cart[0].size = size;
		test = image_fread(image, pet_cbm_cart[0].chip, pet_cbm_cart[0].size);

		if (test != pet_cbm_cart[0].size)
			return INIT_FAIL;
	}

	/* Finally load the cart */
//	This could be needed with .crt support
//	for (i = 0; (i < sizeof(pet_cbm_cart) / sizeof(pet_cbm_cart[0])) && (pet_cbm_cart[i].size != 0); i++) 
//		memcpy(pet_memory + pet_cbm_cart[i].addr, pet_cbm_cart[i].chip, pet_cbm_cart[i].size);
	memcpy(pet_memory + pet_cbm_cart[0].addr, pet_cbm_cart[0].chip, pet_cbm_cart[0].size);

	return INIT_PASS;
}


void pet_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt,a0,b0"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(pet_cart); break;

		default:									cartslot_device_getinfo(devclass, state, info); break;
	}
}

void pet4_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:				info->i = 2; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:		strcpy(info->s = device_temp_str(), "crt,a0"); break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(pet_cart); break;

		default:									cartslot_device_getinfo(devclass, state, info); break;
	}
}
