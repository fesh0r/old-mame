/***********************************************************************

	atom.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

	Many thanks to Kees van Oss for:
	1.	Tape input/output curcuit diagram. It describes in great detail how the 2.4khz
		tone, 2.4khz tone enable, tape output and tape input are connected.
	2.	The DOS rom for the Atom so I could complete the floppy disc emulation.
	3.	Details of the eprom expansion board for the Atom.
	4.	His demo programs which I used to test the driver.

***********************************************************************/

#include "driver.h"
#include "machine/8255ppi.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"
#include "includes/i8271.h"
#include "includes/basicdsk.h"
#include "includes/flopdrv.h"
#include "machine/6522via.h"
#include "cassette.h"

static UINT8	atom_8255_porta;
static UINT8	atom_8255_portb;
static UINT8	atom_8255_portc;

/* printer data written */
static char atom_printer_data = 0x07f;
/* 2.4khz timer state */
static void *atom_timer = NULL;

/* I am not sure if this is correct, the atom appears to have a 2.4Khz timer used for reading tapes?? */
static int	timer_state = 0;

static void atom_via_irq_func(int state)
{
	if (state)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}


}

/* printer status */
READ_HANDLER(atom_via_in_a_func)
{
	unsigned char data;

	data = atom_printer_data;

	if (!device_status(IO_PRINTER,0,0))
	{
		/* offline */
		data |=0x080;
	}

	return data;
}

/* printer data */
WRITE_HANDLER(atom_via_out_a_func)
{
	/* data is written to port, this causes a pulse on ca2 (printer /strobe input),
	and data is written */
	/* atom has a 7-bit printer port */
	atom_printer_data = data & 0x07f;
}

static unsigned char previous_ca2_data = 0;

/* one of these is pulsed! */
WRITE_HANDLER(atom_via_out_ca2_func)
{
	/* change in state of ca2 output? */
	if (((previous_ca2_data^data)&0x01)!=0)
	{
		/* only look for one transition state */
		if (data & 0x01)
		{
			/* output data to printer */
			device_output(IO_PRINTER, 0, atom_printer_data);
		}
	}

	previous_ca2_data = data;
}

struct via6522_interface atom_6522_interface=
{
	atom_via_in_a_func,		/* printer status */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	atom_via_out_a_func,	/* printer data */
	NULL,
	atom_via_out_ca2_func,	/* printer strobe */
	NULL,
	atom_via_irq_func,
	NULL,
	NULL,
	NULL,
	NULL
};

READ_HANDLER(atom_via_r)
{
	return via_0_r(offset & 0x0f);
}

WRITE_HANDLER(atom_via_w)
{
	via_0_w(offset & 0x0f, data);

}



static	ppi8255_interface	atom_8255_int =
{
	1,
	{atom_8255_porta_r},
	{atom_8255_portb_r},
	{atom_8255_portc_r},
	{atom_8255_porta_w},
	{atom_8255_portb_w},
	{atom_8255_portc_w},
};

static int previous_i8271_int_state = 0;

static void atom_8271_interrupt_callback(int state)
{
	/* I'm assuming that the nmi is edge triggered */
	/* a interrupt from the fdc will cause a change in line state, and
	the nmi will be triggered, but when the state changes because the int
	is cleared this will not cause another nmi */
	/* I'll emulate it like this to be sure */

	if (state!=previous_i8271_int_state)
	{
		if (state)
		{
			/* I'll pulse it because if I used hold-line I'm not sure
			it would clear - to be checked */
			cpu_set_nmi_line(0, PULSE_LINE);
		}
	}

	previous_i8271_int_state = state;
}

static struct i8271_interface atom_8271_interface=
{
	atom_8271_interrupt_callback,
	NULL
};


static void atom_timer_callback(int dummy)
{
	/* change timer state */
	timer_state^=1;

	/* the 2.4khz signal is notted (A), and nand'ed with the 2.4kz enable, resulting
	in B. The final cassette output is the result of tape output nand'ed with B */


	{
		unsigned char A;
		unsigned char B;
		unsigned char result;

		/* 2.4khz signal - notted */
		A = (~timer_state);
		/* 2.4khz signal notted, and anded with 2.4khz enable */
		B = (~(A & (atom_8255_portc>>1))) & 0x01;

		result = (~(B & atom_8255_portc)) & 0x01;

		/* tape output */
		device_output(IO_CASSETTE, 0, (result & 0x01) ? -32768 : 32767);
	}
}


static OPBASE_HANDLER(atom_opbase_handler)
{
	/* clear op base override */
	memory_set_opbase_handler(0,0);

	/* this is temporary */
	/* Kees van Oss mentions that address 8-b are used for the random number
	generator. I don't know if this is hardware, or random data because the
	ram chips are not cleared at start-up. So at this time, these numbers
	are poked into the memory to simulate it. When I have more details I will fix it */
	memory_region(REGION_CPU1)[0x08] = rand() & 0x0ff;
	memory_region(REGION_CPU1)[0x09] = rand() & 0x0ff;
	memory_region(REGION_CPU1)[0x0a] = rand() & 0x0ff;
	memory_region(REGION_CPU1)[0x0b] = rand() & 0x0ff;

	return cpu_get_pc() & 0x0ffff;
}
void atom_init_machine(void)
{
	ppi8255_init (&atom_8255_int);
	atom_8255_porta = 0xff;
	atom_8255_portb = 0xff;
	atom_8255_portc = 0xff;

	i8271_init(&atom_8271_interface);

	via_config(0, &atom_6522_interface);
	via_set_clock(0,1000000);
	via_reset();

	timer_state = 0;
	atom_timer = timer_pulse(TIME_IN_HZ(2400*2), 0, atom_timer_callback);

	/* cassette motor control */
	device_status(IO_CASSETTE, 0, 1);

	memory_set_opbase_handler(0,atom_opbase_handler);

}

void atom_stop_machine(void)
{
	if (atom_timer)
		timer_remove(atom_timer);

	i8271_stop();

	/* cassette motor control */
	device_status(IO_CASSETTE, 0, 0);
}



/* load image */
int atom_load(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}


struct atm
{
	UINT8	atm_name[16];
	UINT8	atm_start_high;
	UINT8	atm_start_low;
	UINT8	atm_exec_high;
	UINT8	atm_exec_low;
	UINT8	atm_size_high;
	UINT8	atm_size_low;
};

/* this only works if loaded using file-manager. This should work
for binary files, but will not work with basic files. This also does not support
.tap files which contain multiple .atm files joined together! */
int atom_init_atm (int id)
{
	unsigned char *quickload_data;

	if (atom_load(IO_QUICKLOAD, id, &quickload_data))
	{
		if (quickload_data!=NULL)
		{
			int i;
			unsigned char *data;
			struct atm *atm_header = (struct atm *)quickload_data;
			unsigned long addr;
			unsigned long exec;
			unsigned long size;

			/* calculate data address */
			data = (unsigned char *)((unsigned long)quickload_data + sizeof(struct atm));

			/* get start address */
			addr = (
					(atm_header->atm_start_low & 0x0ff) |
					((atm_header->atm_start_high & 0x0ff)<<8)
					);

			/* get size */
			size = (
					(atm_header->atm_size_low & 0x0ff) |
					((atm_header->atm_size_high & 0x0ff)<<8)
					);

			/* get execute address */
			exec = (
				(atm_header->atm_exec_low & 0x0ff)	|
				((atm_header->atm_exec_high & 0x0ff)<<8)
				);

			/* copy data into memory */
			for (i=size-1; i>=0; i--)
			{
				cpu_writemem16(addr, data[0]);
				addr++;
				data++;
			}


			/* set new pc address */
			cpunum_set_pc(0,exec);

			/* free the data */
			free(quickload_data);

			return INIT_PASS;
		}
	}

	return INIT_PASS;
}


/* load floppy */
int atom_floppy_init(int id)
{
	if (basicdsk_floppy_init(id)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(id,80,1,10,256,0,0);

		return INIT_PASS;
	}

	return INIT_PASS;
}


READ_HANDLER (atom_8255_porta_r )
{
	/* logerror("8255: Read port a\n"); */
	return (atom_8255_porta);
}

READ_HANDLER ( atom_8255_portb_r )
{
	/* ilogerror("8255: Read port b: %02X %02X\n",
			readinputport ((atom_8255.atom_8255_porta & 0x0f) + 1),
			readinputport (11) & 0xc0); */
	return ((readinputport ((atom_8255_porta & 0x0f) + 1) & 0x3f) |
											(readinputport (11) & 0xc0));
}

READ_HANDLER ( atom_8255_portc_r )
{

/* | */
	atom_8255_portc &= 0x0f;

	/* cassette input */
	if (device_input(IO_CASSETTE,0)>255)
	{
		atom_8255_portc |= (1<<5);
	}

	/* 2.4khz input */
	if (timer_state)
	{
		atom_8255_portc |= (1<<4);
	}

	atom_8255_portc |= (readinputport (0) & 0x80);
	atom_8255_portc |= (readinputport (12) & 0x40);
	/* logerror("8255: Read port c (%02X)\n",atom_8255.atom_8255_portc); */
	return (atom_8255_portc);
}

/* Atom 6847 modes:

0000xxxx	Text
0001xxxx	64x64	4
0011xxxx	128x64	2
0101xxxx	128x64	4
0111xxxx	128x96	2
1001xxxx	128x96	4
1011xxxx	128x192	2
1101xxxx	128x192	4
1111xxxx	256x192	2

*/

WRITE_HANDLER ( atom_8255_porta_w )
{
	if ((data & 0xf0) != (atom_8255_porta & 0xf0))
	{
		m6847_gm2_w(0,	data & 0x80);
		m6847_gm1_w(0,	data & 0x40);
		m6847_gm0_w(0,	data & 0x20);
		m6847_ag_w(0,	data & 0x10);
		m6847_set_cannonical_row_height();
		//schedule_full_refresh();
	}
	atom_8255_porta = data;
/*	logerror("8255: Write port a, %02x\n", data); */
}

WRITE_HANDLER ( atom_8255_portb_w )
{
	atom_8255_portb = data;
/*	logerror("8255: Write port b, %02x\n", data); */
}

WRITE_HANDLER (atom_8255_portc_w)
{
	atom_8255_portc = data;
	speaker_level_w(0, (data & 0x04) >> 2);

	m6847_css_w(0,(data & 0x08));
/*	logerror("8255: Write port c, %02x\n", data); */
}


/* KT- I've assumed that the atom 8271 is linked in exactly the same way as on the bbc */
READ_HANDLER(atom_8271_r)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			return i8271_r(offset);
		case 4:
			return i8271_data_r(offset);
		default:
			break;
	}

	return 0x0ff;
}

WRITE_HANDLER(atom_8271_w)
{
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
			/* 8271 registers */
			i8271_w(offset, data);
			return;
		case 4:
			i8271_data_w(offset, data);
			return;
		default:
			break;
	}
}


int atom_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void atom_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}



/*********************************************/
/* emulates a 16-slot eprom box for the Atom */
static unsigned char selected_eprom = 0;

static void atom_eprom_box_refresh(void)
{
    unsigned char *eprom_data;

	/* get address of eprom data */
	eprom_data = memory_region(REGION_CPU1) + 0x010000 + (selected_eprom<<12);
	/* set bank address */
	cpu_setbank(1, eprom_data);
}

void atom_eprom_box_init(void)
{
	/* set initial eprom */
	selected_eprom = 0;
	/* set memory handler */
    memory_set_bankhandler_r(1, 0, MRA_BANK1);
	/* init */
	atom_eprom_box_refresh();
}

/* write to eprom box, changes eprom selected */
WRITE_HANDLER(atom_eprom_box_w)
{
	selected_eprom = data & 0x0f;

	atom_eprom_box_refresh();
}

/* read from eprom box register, can this be done in the real hardware */
READ_HANDLER(atom_eprom_box_r)
{
	return selected_eprom;
}

void	atomeb_init_machine(void)
{
	atom_init_machine();
	atom_eprom_box_init();
}

