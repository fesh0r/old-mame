/******************************************************************************

	pcw16.c
	system driver

	Kevin Thacker [MESS driver]

  Thankyou to:

	- Cliff Lawson @ Amstrad plc for his documentation (Anne ASIC documentation),
					and extensive help.
			(web.ukonline.co.uk/cliff.lawson/)
			(www.amstrad.com)
    - John Elliot for his help and tips
			(he's written a CP/M implementation for the PCW16)
			(www.seasip.deomon.co.uk)
	- and others who offered their help (Richard Fairhurst, Richard Wildey)

	Hardware:
		- 2mb dram max,
		- 2mb flash-file memory max (in 2 1mb chips),
		- 16Mhz Z80 (core combined in Anne ASIC),
		- Anne ASIC (keyboard interface, video (colours), dram/flash/rom paging,
		real time clock, "glue" logic for Super I/O)
		- Winbond Super I/O chip (PC type hardware - FDC, Serial, LPT, Hard-drive)
		- PC/AT keyboard - some keys are coloured to identify special functions, but
		these are the same as normal PC keys
		- PC Serial Mouse - uses Mouse System Mouse protocol
		- PC 1.44MB Floppy drive

    Primary Purpose:
		- built as a successor to the PCW8526/PCW9512 series
		- wordprocessor system (also contains spreadsheet and other office applications)
		- 16Mhz processor used so proportional fonts and enhanced wordprocessing features
		  are possible, true WYSIWYG wordprocessing.
		- flash-file can store documents.

    To Do:
		- reduce memory usage so it is more MESSD friendly
		- different configurations
		- implement configuration register
		- extract game-port hardware from pc driver - used in any PCW16 progs?
		- extract hard-drive code from PC driver and use in this driver
		- implement printer
		- .. anything else that requires implementing

	 Info:
	   - to use this driver you need a OS rescue disc.
	   (HINT: This also contains the boot-rom)
	  - the OS will be installed from the OS rescue disc into the Flash-ROM

	Uses "MEMCARD" dir to hold flash-file data.
	To use the power button, flick the dip switch off/on

 From comp.sys.amstrad.8bit FAQ:

  "Amstrad made the following PCW systems :

  - 1) PCW8256
  - 2) PCW8512
  - 3) PCW9512
  - 4) PCW9512+
  - 5) PcW10
  - 6) PcW16

  1 had 180K drives, 2 had a 180K A drive and a 720K B drive, 3 had only
  720K drives. All subsequent models had 3.5" disks using CP/M format at
  720K until 6 when it switched to 1.44MB in MS-DOS format. The + of
  model 4 was that it had a "real" parallel interface so could be sold
  with an external printer such as the Canon BJ10. The PcW10 wasn't
  really anything more than 4 in a more modern looking case.

  The PcW16 is a radical digression who's sole "raison d'etre" was to
  make a true WYSIWYG product but this meant a change in the screen and
  processor (to 16MHz) etc. which meant that it could not be kept
  compatible with the previous models (though documents ARE compatible)"




 ******************************************************************************/
/* PeT 19.October 2000
   added/changed printer support
   not working reliable, seams to expect parallelport in epp/ecp mode
   epp/ecp modes in parallel port not supported yet
   so ui disabled */

#include "driver.h"
#include "includes/pcw16.h"

// PC-Parallel Port
#include "includes/pclpt.h"
#include "includes/centroni.h" // centronics printer handshake simulation
#include "printer.h" // printer device
// PC-AT keyboard
#include "includes/pckeybrd.h"
// change to superio later
#include "includes/pc_fdc_h.h"
// for pc disk images
#include "includes/pc_flopp.h"
// for pc com port
#include "includes/uart8250.h"
// for pc serial mouse
#include "includes/pc_mouse.h"
// pcw/pcw16 beeper
//#include "sound/beep.h"

#include "includes/28f008sa.h"

//#define PCW16_DUMP_RAM
//#define PCW16_DUMP_CPU_RAM

/* ram - up to 2mb */
unsigned char *pcw16_ram = NULL;

// general timer
void	 *pcw16_timer;
// timer to update real time clock
void	 *pcw16_rtc_timer;
// timer to refresh keyboard (not a actual interrupt in PCW16 system!)
void	 *pcw16_keyboard_timer;
// interrupt counter
unsigned long pcw16_interrupt_counter;
// video control
extern int pcw16_video_control;
/* controls which bank of 2mb address space is paged into memory */
static int pcw16_banks[4];

// output of 4-bit port from Anne ASIC
static int pcw16_4_bit_port;
// code defining which INT fdc is connected to
static int pcw16_fdc_int_code;
// interrupt bits
// bit 7: ??
// bit 6: fdc
// bit 5: ??
// bit 3,4; Serial
// bit 2: Vsync state
// bit 1: keyboard int
// bit 0: Display ints
static int pcw16_system_status;

// debugging - write whole ram
#ifdef PCW16_DUMP_RAM
/* write ram */
void pcw16_dump_ram(void)
{
	int i;
	void *file;

	if (pcw16_ram!=NULL)
	{
		file = osd_fopen(Machine->gamedrv->name, "pcwram.bin", OSD_FILETYPE_MEMCARD, OSD_FOPEN_WRITE);

		if (file)
		{
			for (i=0; i<2048*1024; i++)
			{
				osd_fwrite(file, &pcw16_ram[i], 1);
			}

//			osd_fwrite(file, pcw16_ram, 2048*1024);
			osd_fclose(file);
		}
	}

}
#endif

// debugging - write ram as seen by cpu
#ifdef PCW16_DUMP_CPU_RAM
void pcw16_dump_cpu_ram(void)
{
	void *file;

	file = osd_fopen(Machine->gamedrv->name, "pcwcpuram.bin", OSD_FILETYPE_MEMCARD,OSD_FOPEN_WRITE);

	if (file)
	{
		int i;
		for (i=0; i<65536; i++)
		{
			char data;

			data = cpu_readmem16(i);

			osd_fwrite(file, &data, 1);

		}

		/* close file */
		osd_fclose(file);
	}
}
#endif

void pcw16_refresh_ints(void)
{
	/* any bits set excluding vsync */
	if ((pcw16_system_status & (~0x04))!=0)
	{
		cpu_set_irq_line(0,0, HOLD_LINE);
	}
	else
	{
		cpu_set_irq_line(0,0, CLEAR_LINE);
	}
}


void pcw16_timer_callback(int dummy)
{
	/* do not increment past 15 */
	if (pcw16_interrupt_counter!=15)
	{
		pcw16_interrupt_counter++;
		/* display int */
		pcw16_system_status |= (1<<0);
	}

	if (pcw16_interrupt_counter!=0)
	{
		pcw16_refresh_ints();
	}
}

MEMORY_READ_START( readmem_pcw16 )
	{0x0000, 0x03fff, MRA_BANK1},
	{0x4000, 0x07fff, MRA_BANK2},
	{0x8000, 0x0Bfff, MRA_BANK3},
	{0xC000, 0x0ffff, MRA_BANK4},
MEMORY_END

extern int pcw16_colour_palette[16];

WRITE_HANDLER(pcw16_palette_w)
{
	pcw16_colour_palette[offset & 0x0f] = data & 31;
}

static char *pcw16_mem_ptr[4];

const mem_write_handler pcw16_write_handler_dram[4] =
{
	MWA_BANK5,
	MWA_BANK6,
	MWA_BANK7,
	MWA_BANK8
};

const mem_read_handler pcw16_read_handler_dram[4] =
{
	MRA_BANK1,
	MRA_BANK2,
	MRA_BANK3,
	MRA_BANK4
};
/*******************************************/


/* PCW16 Flash interface */
/* PCW16 can have two 1mb flash chips */

/* read flash0 */
static int pcw16_flash0_bank_handler_r(int bank, int offset)
{
	int flash_offset = (pcw16_banks[bank]<<14) | offset;

	return flash_bank_handler_r(0, flash_offset);
}

/* read flash1 */
static int pcw16_flash1_bank_handler_r(int bank, int offset)
{
	int flash_offset = ((pcw16_banks[bank]&0x03f)<<14) | offset;

	return flash_bank_handler_r(1, flash_offset);
}

/* flash 0 */
static READ_HANDLER(pcw16_flash0_bank_handler0_r)
{
	return pcw16_flash0_bank_handler_r(0, offset);
}

static READ_HANDLER(pcw16_flash0_bank_handler1_r)
{
	return pcw16_flash0_bank_handler_r(1, offset);
}

static READ_HANDLER(pcw16_flash0_bank_handler2_r)
{
	return pcw16_flash0_bank_handler_r(2, offset);
}

static READ_HANDLER(pcw16_flash0_bank_handler3_r)
{
	return pcw16_flash0_bank_handler_r(3, offset);
}

/* flash 1 */
static READ_HANDLER(pcw16_flash1_bank_handler0_r)
{
	return pcw16_flash1_bank_handler_r(0, offset);
}

static READ_HANDLER(pcw16_flash1_bank_handler1_r)
{
	return pcw16_flash1_bank_handler_r(1, offset);
}

static READ_HANDLER(pcw16_flash1_bank_handler2_r)
{
	return pcw16_flash1_bank_handler_r(2, offset);
}

static READ_HANDLER(pcw16_flash1_bank_handler3_r)
{
	return pcw16_flash1_bank_handler_r(3, offset);
}

static mem_read_handler pcw16_flash0_bank_handlers_r[4] =
{
	pcw16_flash0_bank_handler0_r,
	pcw16_flash0_bank_handler1_r,
	pcw16_flash0_bank_handler2_r,
	pcw16_flash0_bank_handler3_r,
};

static mem_read_handler pcw16_flash1_bank_handlers_r[4] =
{
	pcw16_flash1_bank_handler0_r,
	pcw16_flash1_bank_handler1_r,
	pcw16_flash1_bank_handler2_r,
	pcw16_flash1_bank_handler3_r,
};

/* write flash0 */
static void pcw16_flash0_bank_handler_w(int bank, int offset, int data)
{
	int flash_offset = (pcw16_banks[bank]<<14) | offset;

	flash_bank_handler_w(0, flash_offset, data);
}

/* read flash1 */
static void pcw16_flash1_bank_handler_w(int bank, int offset, int data)
{
	int flash_offset = ((pcw16_banks[bank]&0x03f)<<14) | offset;

	flash_bank_handler_w(1, flash_offset,data);
}

/* flash 0 */
WRITE_HANDLER(pcw16_flash0_bank_handler0_w)
{
	pcw16_flash0_bank_handler_w(0, offset, data);
}


WRITE_HANDLER(pcw16_flash0_bank_handler1_w)
{
	pcw16_flash0_bank_handler_w(1, offset, data);
}

WRITE_HANDLER(pcw16_flash0_bank_handler2_w)
{
	pcw16_flash0_bank_handler_w(2, offset, data);
}

WRITE_HANDLER(pcw16_flash0_bank_handler3_w)
{
	pcw16_flash0_bank_handler_w(3, offset, data);
}


/* flash 1 */
WRITE_HANDLER(pcw16_flash1_bank_handler0_w)
{
	pcw16_flash1_bank_handler_w(0, offset, data);
}


WRITE_HANDLER(pcw16_flash1_bank_handler1_w)
{
	pcw16_flash1_bank_handler_w(1, offset, data);
}

WRITE_HANDLER(pcw16_flash1_bank_handler2_w)
{
	pcw16_flash1_bank_handler_w(2, offset, data);
}

WRITE_HANDLER(pcw16_flash1_bank_handler3_w)
{
	pcw16_flash1_bank_handler_w(3, offset, data);
}

static mem_write_handler pcw16_flash0_bank_handlers_w[4] =
{
	pcw16_flash0_bank_handler0_w,
	pcw16_flash0_bank_handler1_w,
	pcw16_flash0_bank_handler2_w,
	pcw16_flash0_bank_handler3_w,
};

static mem_write_handler pcw16_flash1_bank_handlers_w[4] =
{
	pcw16_flash1_bank_handler0_w,
	pcw16_flash1_bank_handler1_w,
	pcw16_flash1_bank_handler2_w,
	pcw16_flash1_bank_handler3_w,
};

typedef enum
{
	/* rom which is really first block of flash0 */
	PCW16_MEM_ROM,
	/* flash 0 */
	PCW16_MEM_FLASH_1,
	/* flash 1 i.e. unexpanded pcw16 */
	PCW16_MEM_FLASH_2,
	/* dram */
	PCW16_MEM_DRAM,
	/* no mem. i.e. unexpanded pcw16 */
	PCW16_MEM_NONE
} PCW16_RAM_TYPE;

READ_HANDLER(pcw16_no_mem_r)
{
	return 0x0ff;
}

static void pcw16_set_bank_handlers(int bank, PCW16_RAM_TYPE type)
{
	switch (type)
	{
		/* rom */
		case PCW16_MEM_ROM:
		{
			memory_set_bankhandler_r(bank+1, 0, pcw16_read_handler_dram[bank]);
			memory_set_bankhandler_w(bank+5, 0, MWA_NOP);
		}
		break;


		/* sram */
		case PCW16_MEM_FLASH_1:
		{
			memory_set_bankhandler_r(bank+1, 0, pcw16_flash0_bank_handlers_r[bank]);
			memory_set_bankhandler_w(bank+5, 0, pcw16_flash0_bank_handlers_w[bank]);
		}
		break;

		case PCW16_MEM_FLASH_2:
		{
			memory_set_bankhandler_r(bank+1, 0, pcw16_flash1_bank_handlers_r[bank]);
			memory_set_bankhandler_w(bank+5, 0, pcw16_flash1_bank_handlers_w[bank]);
		}
		break;

		case PCW16_MEM_NONE:
		{
			memory_set_bankhandler_r(bank+1, 0, pcw16_no_mem_r);
			memory_set_bankhandler_w(bank+5, 0, MWA_NOP);
		}
		break;

		/* dram */
		default:
		case PCW16_MEM_DRAM:
		{
			memory_set_bankhandler_r(bank+1, 0, pcw16_read_handler_dram[bank]);
			memory_set_bankhandler_w(bank+5, 0, pcw16_write_handler_dram[bank]);

		}
		break;
	}
}

static void pcw16_update_bank(int bank)
{
	unsigned char *mem_ptr = pcw16_ram;
	int bank_id = 0;
	int bank_offs = 0;


	/* get memory bank */
	bank_id = pcw16_banks[bank];


#ifdef PCW16_DUMP_CPU_RAM
	if (bank_id==4)
	{
		pcw16_dump_cpu_ram();
	}
#endif

	if ((bank_id & 0x080)==0)
	{
		bank_offs = 0;

		if (bank_id<4)
		{
			/* lower 4 banks are write protected. Use the rom
			loaded */
			mem_ptr = &memory_region(REGION_CPU1)[0x010000];
		}
		else
		{
			/* nvram */
			if ((bank_id & 0x040)==0)
			{
				mem_ptr = (unsigned char *)flash_get_base(0);
			}
			else
			{
				mem_ptr = (unsigned char *)flash_get_base(1);
			}
		}

	}
	else
	{
		bank_offs = 128;
			/* dram */
			mem_ptr = pcw16_ram;
	}

	mem_ptr = mem_ptr + ((bank_id - bank_offs)<<14);
	pcw16_mem_ptr[bank] = (char*)mem_ptr;
	cpu_setbank((bank+1), mem_ptr);
	cpu_setbank((bank+5), mem_ptr);

	if ((bank_id & 0x080)==0)
	{
		/* selections 0-3 within the first 64k are write protected */
		if (bank_id<4)
		{
			/* rom */
			pcw16_set_bank_handlers(bank, PCW16_MEM_ROM);
		}
		else
		{
			/* selections 0-63 are for flash-rom 0, selections
			64-128 are for flash-rom 1 */
			if ((bank_id & 0x040)==0)
			{
				pcw16_set_bank_handlers(bank, PCW16_MEM_FLASH_1);
			}
			else
			{
				pcw16_set_bank_handlers(bank, PCW16_MEM_FLASH_2);
			}
		}
	}
	else
	{
		pcw16_set_bank_handlers(bank, PCW16_MEM_DRAM);
	}
}


/* update memory h/w */
static void pcw16_update_memory(void)
{
	pcw16_update_bank(0);
	pcw16_update_bank(1);
	pcw16_update_bank(2);
	pcw16_update_bank(3);

}

READ_HANDLER(pcw16_bankhw_r)
{
//	logerror("bank r: %d \n", offset);

	return pcw16_banks[offset];
}

WRITE_HANDLER(pcw16_bankhw_w)
{
	//logerror("bank w: %d block: %02x\n", offset, data);

	pcw16_banks[offset] = data;

	pcw16_update_memory();
}

WRITE_HANDLER(pcw16_video_control_w)
{
	//logerror("video control w: %02x\n", data);

	pcw16_video_control = data;
}

/* PCW16 KEYBOARD */

unsigned char pcw16_keyboard_status;
unsigned char pcw16_keyboard_data_shift;



#define PCW16_KEYBOARD_PARITY_MASK	(1<<7)
#define PCW16_KEYBOARD_STOP_BIT_MASK (1<<6)
#define PCW16_KEYBOARD_START_BIT_MASK (1<<5)
#define PCW16_KEYBOARD_BUSY_STATUS	(1<<4)
#define PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK (1<<1)
#define PCW16_KEYBOARD_TRANSMIT_MODE (1<<0)

#define PCW16_KEYBOARD_RESET_INTERFACE (1<<2)

#define PCW16_KEYBOARD_DATA	(1<<1)
#define PCW16_KEYBOARD_CLOCK (1<<0)

/* parity table. Used to set parity bit in keyboard status register */
static int pcw16_keyboard_parity_table[256];

static int pcw16_keyboard_bits = 0;
static int pcw16_keyboard_bits_output = 0;

static int pcw16_keyboard_state = 0;
static int pcw16_keyboard_previous_state=0;
static void pcw16_keyboard_reset(void);
static void pcw16_keyboard_int(int);

static void pcw16_keyboard_init(void)
{
	int i;
	int b;

	/* if sum of all bits in the byte is even, then the data
	has even parity, otherwise it has odd parity */
	for (i=0; i<256; i++)
	{
		int data;
		int sum;

		sum = 0;
		data = i;

		for (b=0; b<8; b++)
		{
			sum+=data & 0x01;

			data = data>>1;
		}

		pcw16_keyboard_parity_table[i] = sum & 0x01;
	}


	/* clear int */
	pcw16_keyboard_int(0);
	/* reset state */
	pcw16_keyboard_state = 0;
	/* reset ready for transmit */
	pcw16_keyboard_reset();
}

static void pcw16_keyboard_refresh_outputs(void)
{
	/* generate output bits */
	pcw16_keyboard_bits_output = pcw16_keyboard_bits;

	/* force clock low? */
	if (pcw16_keyboard_state & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)
	{
		pcw16_keyboard_bits_output &= ~PCW16_KEYBOARD_CLOCK;
	}
}

static void pcw16_keyboard_set_clock_state(int state)
{
	pcw16_keyboard_bits &= ~PCW16_KEYBOARD_CLOCK;

	if (state)
	{
		pcw16_keyboard_bits |= PCW16_KEYBOARD_CLOCK;
	}

	pcw16_keyboard_refresh_outputs();
}

static void pcw16_keyboard_int(int state)
{
	pcw16_system_status &= ~(1<<1);

	if (state)
	{
		pcw16_system_status |= (1<<1);
	}

	pcw16_refresh_ints();
}

static void pcw16_keyboard_reset(void)
{
	/* clock set to high */
	pcw16_keyboard_set_clock_state(1);
}

/* interfaces to a pc-at keyboard */
READ_HANDLER(pcw16_keyboard_data_shift_r)
{
	//logerror("keyboard data shift r: %02x\n", pcw16_keyboard_data_shift);
	pcw16_keyboard_state &= ~(PCW16_KEYBOARD_BUSY_STATUS);

	pcw16_keyboard_int(0);
	/* reset for reception */
	pcw16_keyboard_reset();

	/* read byte */
	return pcw16_keyboard_data_shift;
}

/* if force keyboard clock is low it is safe to send */
int		pcw16_keyboard_can_transmit(void)
{
	/* clock is not forced low */
	/* and not busy - i.e. not already sent a char */
	return ((pcw16_keyboard_bits_output & PCW16_KEYBOARD_CLOCK)!=0);
}

/* issue a begin byte transfer */
void	pcw16_begin_byte_transfer(void)
{
}


/* signal a code has been received */
void	pcw16_keyboard_signal_byte_received(int data)
{

	/* clear clock */
	pcw16_keyboard_set_clock_state(0);

	/* set code in shift register */
	pcw16_keyboard_data_shift = data;
	/* busy */
	pcw16_keyboard_state |= PCW16_KEYBOARD_BUSY_STATUS;

	/* initialise start, stop and parity bits */
	pcw16_keyboard_state &= ~PCW16_KEYBOARD_START_BIT_MASK;
	pcw16_keyboard_state |=PCW16_KEYBOARD_STOP_BIT_MASK;

	/* "Keyboard data has odd parity, so the parity bit in the
	status register should only be set when the shift register
	data itself has even parity. */

	pcw16_keyboard_state &= ~PCW16_KEYBOARD_PARITY_MASK;

	/* if data has even parity, set parity bit */
	if ((pcw16_keyboard_parity_table[data])==0)
		pcw16_keyboard_state |= PCW16_KEYBOARD_PARITY_MASK;

	pcw16_keyboard_int(1);
}


WRITE_HANDLER(pcw16_keyboard_data_shift_w)
{
	//logerror("Keyboard Data Shift: %02x\n", data);
	/* writing to shift register clears parity */
	/* writing to shift register clears start bit */
	pcw16_keyboard_state &= ~(
		PCW16_KEYBOARD_PARITY_MASK |
		PCW16_KEYBOARD_START_BIT_MASK);

	/* writing to shift register sets stop bit */
	pcw16_keyboard_state |= PCW16_KEYBOARD_STOP_BIT_MASK;

	pcw16_keyboard_data_shift = data;

}

READ_HANDLER(pcw16_keyboard_status_r)
{
	/* bit 2,3 are bits 8 and 9 of vdu pointer */
	return (pcw16_keyboard_state &
		(PCW16_KEYBOARD_PARITY_MASK |
		 PCW16_KEYBOARD_STOP_BIT_MASK |
		 PCW16_KEYBOARD_START_BIT_MASK |
		 PCW16_KEYBOARD_BUSY_STATUS |
		 PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK |
		 PCW16_KEYBOARD_TRANSMIT_MODE));
}

WRITE_HANDLER(pcw16_keyboard_control_w)
{
	//logerror("Keyboard control w: %02x\n",data);

	pcw16_keyboard_previous_state = pcw16_keyboard_state;

	/* if set, set parity */
	if (data & 0x080)
	{
		pcw16_keyboard_state |= PCW16_KEYBOARD_PARITY_MASK;
	}

	/* clear read/write bits */
	pcw16_keyboard_state &=
		~(PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK |
			PCW16_KEYBOARD_TRANSMIT_MODE);
	/* set read/write bits from data */
	pcw16_keyboard_state |= (data & 0x03);

	if (data & PCW16_KEYBOARD_RESET_INTERFACE)
	{
		pcw16_keyboard_reset();
	}

	if (data & PCW16_KEYBOARD_TRANSMIT_MODE)
	{
		/* force clock changed */
		if (((pcw16_keyboard_state^pcw16_keyboard_previous_state) & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)!=0)
		{
			/* just cleared? */
			if ((pcw16_keyboard_state & PCW16_KEYBOARD_FORCE_KEYBOARD_CLOCK)==0)
			{

				/* write */
				/* busy */
				pcw16_keyboard_state |= PCW16_KEYBOARD_BUSY_STATUS;
				/* keyboard takes data */
				at_keyboard_write(pcw16_keyboard_data_shift);
				/* set clock low - no furthur transmissions */
				pcw16_keyboard_set_clock_state(0);
				/* set int */
				pcw16_keyboard_int(1);
			}
		}


	}

	if (((pcw16_keyboard_state^pcw16_keyboard_previous_state) & PCW16_KEYBOARD_TRANSMIT_MODE)!=0)
	{
		if ((pcw16_keyboard_state & PCW16_KEYBOARD_TRANSMIT_MODE)==0)
		{
			if ((pcw16_system_status & (1<<1))!=0)
			{
				pcw16_keyboard_int(0);
			}
		}
	}

	pcw16_keyboard_refresh_outputs();
}


static void pcw16_keyboard_timer_callback(int dummy)
{
	at_keyboard_polling();
	if (pcw16_keyboard_can_transmit())
	{
		int data;

		data = at_keyboard_read();

		if (data!=-1)
		{
//			if (data==4)
//			{
//				pcw16_dump_cpu_ram();
//			}

			pcw16_keyboard_signal_byte_received(data);
		}
	}
}

MEMORY_WRITE_START( writemem_pcw16 )
	{0x00000, 0x03fff, MWA_BANK5},
	{0x04000, 0x07fff, MWA_BANK6},
	{0x08000, 0x0bfff, MWA_BANK7},
	{0x0c000, 0x0ffff, MWA_BANK8},
MEMORY_END

static unsigned char rtc_seconds;
static unsigned char rtc_minutes;
static unsigned char rtc_hours;
static unsigned char rtc_days_max;
static unsigned char rtc_days;
static unsigned char rtc_months;
static unsigned char rtc_years;
static unsigned char rtc_control;
static unsigned char rtc_256ths_seconds;

static int rtc_days_in_each_month[]=
{
	31,/* jan */
	28, /* feb */
	31, /* march */
	30, /* april */
	31, /* may */
	30, /* june */
	31, /* july */
	31, /* august */
	30, /* september */
	31, /* october */
	30, /* november */
	31	/* december */
};

static int rtc_days_in_february[] =
{
	29, 28, 28, 28
};

static void rtc_setup_max_days(void)
{
	/* february? */
	if (rtc_months == 2)
	{
		/* low two bits of year select number of days in february */
		rtc_days_max = rtc_days_in_february[rtc_years & 0x03];
	}
	else
	{
		rtc_days_max = (unsigned char)rtc_days_in_each_month;
	}
}

static void rtc_timer_callback(int dummy)
{
	int fraction_of_second;

	/* halt counter? */
	if ((rtc_control & 0x01)!=0)
	{
		/* no */

		/* increment 256th's of a second register */
		fraction_of_second = rtc_256ths_seconds+1;
		/* add bit 8 = overflow */
		rtc_seconds+=(fraction_of_second>>8);
		/* ensure counter is in range 0-255 */
		rtc_256ths_seconds = fraction_of_second & 0x0ff;
	}

	if (rtc_seconds>59)
	{
		rtc_seconds = 0;

		rtc_minutes++;

		if (rtc_minutes>59)
		{
			rtc_minutes = 0;

			rtc_hours++;

			if (rtc_hours>23)
			{
				rtc_hours = 0;

				rtc_days++;

				if (rtc_days>rtc_days_max)
				{
					rtc_days = 1;

					rtc_months++;

					if (rtc_months>12)
					{
						rtc_months = 1;

						/* 7 bit year counter */
						rtc_years = (rtc_years + 1) & 0x07f;

					}

					rtc_setup_max_days();
				}

			}


		}
	}
}

READ_HANDLER(rtc_year_invalid_r)
{
	/* year in lower 7 bits. RTC Invalid status is rtc_control bit 0
	inverted */
	return (rtc_years & 0x07f) | (((rtc_control & 0x01)<<7)^0x080);
}

READ_HANDLER(rtc_month_r)
{
	return rtc_months;
}

READ_HANDLER(rtc_days_r)
{
	return rtc_days;
}

READ_HANDLER(rtc_hours_r)
{
	return rtc_hours;
}

READ_HANDLER(rtc_minutes_r)
{
	return rtc_minutes;
}

READ_HANDLER(rtc_seconds_r)
{
	return rtc_seconds;
}

READ_HANDLER(rtc_256ths_seconds_r)
{
	return rtc_256ths_seconds;
}

WRITE_HANDLER(rtc_control_w)
{
	/* write control */
	rtc_control = data;
}

WRITE_HANDLER(rtc_seconds_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_seconds = data;
}

WRITE_HANDLER(rtc_minutes_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_minutes = data;
}

WRITE_HANDLER(rtc_hours_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;
}

WRITE_HANDLER(rtc_days_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_days = data;
}

WRITE_HANDLER(rtc_month_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_months = data;

	rtc_setup_max_days();
}


WRITE_HANDLER(rtc_year_w)
{
	/* TODO: Writing register could cause next to increment! */
	rtc_hours = data;

	rtc_setup_max_days();
}

static int previous_fdc_int_state;

static void pcw16_trigger_fdc_int(void)
{
	int state;

	state = pcw16_system_status & (1<<6);

	switch (pcw16_fdc_int_code)
	{
		/* nmi */
		case 0:
		{
			/* I'm assuming that the nmi is edge triggered */
			/* a interrupt from the fdc will cause a change in line state, and
			the nmi will be triggered, but when the state changes because the int
			is cleared this will not cause another nmi */
			/* I'll emulate it like this to be sure */

			if (state!=previous_fdc_int_state)
			{
				if (state)
				{
					/* I'll pulse it because if I used hold-line I'm not sure
					it would clear - to be checked */
					cpu_set_nmi_line(0, PULSE_LINE);
				}
			}
		}
		break;

		/* attach fdc to int */
		case 1:
		{
			pcw16_refresh_ints();
		}
		break;

		/* do not interrupt */
		default:
			break;
	}

	previous_fdc_int_state = state;
}

READ_HANDLER(pcw16_system_status_r)
{
//	logerror("system status r: \n");

	return pcw16_system_status | (readinputport(0) & 0x04);
}

READ_HANDLER(pcw16_timer_interrupt_counter_r)
{
	int data;

	data = pcw16_interrupt_counter;

	pcw16_interrupt_counter = 0;
	/* clear display int */
	pcw16_system_status &= ~(1<<0);

	pcw16_refresh_ints();

	return data;
}


WRITE_HANDLER(pcw16_system_control_w)
{
	//logerror("0x0f8: function: %d\n",data);

	/* lower 4 bits define function code */
	switch (data & 0x0f)
	{
		/* no effect */
		case 0x00:
		case 0x09:
		case 0x0a:
		case 0x0d:
		case 0x0e:
			break;

		/* system reset */
		case 0x01:
			break;

		/* connect IRQ6 input to /NMI */
		case 0x02:
		{
			pcw16_fdc_int_code = 0;
		}
		break;

		/* connect IRQ6 input to /INT */
		case 0x03:
		{
			pcw16_fdc_int_code = 1;
		}
		break;

		/* dis-connect IRQ6 input from /NMI and /INT */
		case 0x04:
		{
			pcw16_fdc_int_code = 2;
		}
		break;

		/* set terminal count */
		case 0x05:
		{
			pc_fdc_set_tc_state(1);
		}
		break;

		/* clear terminal count */
		case 0x06:
		{
			pc_fdc_set_tc_state(0);
		}
		break;

		/* bleeper on */
		case 0x0b:
		{
                        beep_set_state(0,1);
		}
		break;

		/* bleeper off */
		case 0x0c:
		{
                        beep_set_state(0,0);
		}
		break;

		/* drive video outputs */
		case 0x07:
		{
		}
		break;

		/* float video outputs */
		case 0x08:
		{
		}
		break;

		/* set 4-bit output port to value X */
		case 0x0f:
		{
			/* bit 7 - ?? */
			/* bit 6 - ?? */
			/* bit 5 - green/red led (1==green)*/
			/* bit 4 - monitor on/off (1==on) */

			pcw16_4_bit_port = data>>4;


		}
		break;
	}
}

/**** SUPER I/O connections */

/* write to Super I/O chip. FDC Data Rate. */
WRITE_HANDLER(pcw16_superio_fdc_datarate_w)
{
	pc_fdc_w(PC_FDC_DATA_RATE_REGISTER,data);
}

/* write to Super I/O chip. FDC Digital output register */
WRITE_HANDLER(pcw16_superio_fdc_digital_output_register_w)
{
	pc_fdc_w(PC_FDC_DIGITAL_OUTPUT_REGISTER, data);
}

/* write to Super I/O chip. FDC Data Register */
WRITE_HANDLER(pcw16_superio_fdc_data_w)
{
	pc_fdc_w(PC_FDC_DATA_REGISTER, data);
}

/* write to Super I/O chip. FDC Data Register */
READ_HANDLER(pcw16_superio_fdc_data_r)
{
	return pc_fdc_r(PC_FDC_DATA_REGISTER);
}

/* write to Super I/O chip. FDC Main Status Register */
READ_HANDLER(pcw16_superio_fdc_main_status_register_r)
{
	return pc_fdc_r(PC_FDC_MAIN_STATUS_REGISTER);
}

READ_HANDLER(pcw16_superio_fdc_digital_input_register_r)
{
	return pc_fdc_r(PC_FDC_DIGITIAL_INPUT_REGISTER);
}

void	pcw16_fdc_interrupt(int state)
{
	/* IRQ6 */
	/* bit 6 of PCW16 system status indicates floppy ints */
	pcw16_system_status &= ~(1<<6);

	if (state)
	{
		pcw16_system_status |= (1<<6);
	}

	pcw16_trigger_fdc_int();
}

pc_fdc_hw_interface pcw16_fdc_interface=
{
	pcw16_fdc_interrupt,
	NULL
};

static void pcw16_com_interrupt(int nr, int state)
{
	static const int irq[2]={4,3};
	pcw16_system_status &= ~(1<<irq[nr]);

	if (state)
	{
		pcw16_system_status |= (1<<irq[nr]);
	}

	pcw16_refresh_ints();
}

static void pcw16_com_refresh_connected(int serial_port_id)
{
	switch (serial_port_id)
	{
#if 0
		case 0:
		{
			/* PC mouse on this port */
			pc_mouse_poll(0);
		}
		break;
#endif
		case 1:
		{
			int new_inputs;

			new_inputs = 0;

			/* Power switch is connected to Ring indicator */
			if (readinputport(0) & 0x040)
			{
				new_inputs = UART8250_INPUTS_RING_INDICATOR;
			}

			uart8250_handshake_in(1, new_inputs);
		}
		break;
	}
}

static uart8250_interface pcw16_com_interface[2]=
{
	{
		TYPE16550,
		1843200,
		pcw16_com_interrupt,
		NULL,
		pc_mouse_handshake_in
//		pcw16_com_refresh_connected
	},
	{
		TYPE16550,
		1843200,
		pcw16_com_interrupt,
		NULL,
		NULL,
		pcw16_com_refresh_connected
	}
};



PORT_READ_START( readport_pcw16 )
	/* super i/o chip */
	{0x01c, 0x01c, pcw16_superio_fdc_main_status_register_r},
	{0x01d, 0x01d, pcw16_superio_fdc_data_r},
	{0x01f, 0x01f, pcw16_superio_fdc_digital_input_register_r},
	{0x020, 0x027, uart8250_0_r},
	{0x028, 0x02f, uart8250_1_r},
	{0x038, 0x03a, pc_parallelport0_r},
	/* anne asic */
	{0x0f0, 0x0f3, pcw16_bankhw_r},
	{0x0f4, 0x0f4, pcw16_keyboard_data_shift_r},
	{0x0f5, 0x0f5, pcw16_keyboard_status_r},
	{0x0f7, 0x0f7, pcw16_timer_interrupt_counter_r},
	{0x0f8, 0x0f8, pcw16_system_status_r},
	{0x0f9, 0x0f9, rtc_256ths_seconds_r},
	{0x0fa, 0x0fa, rtc_seconds_r},
	{0x0fb, 0x0fb, rtc_minutes_r},
	{0x0fc, 0x0fc, rtc_hours_r},
	{0x0fd, 0x0fd, rtc_days_r},
	{0x0fe, 0x0fe, rtc_month_r},
	{0x0ff, 0x0ff, rtc_year_invalid_r},
PORT_END

PORT_WRITE_START( writeport_pcw16 )
	/* super i/o */
	{0x01a, 0x01a, pcw16_superio_fdc_digital_output_register_w},
	{0x01d, 0x01d, pcw16_superio_fdc_data_w},
	{0x01f, 0x01f, pcw16_superio_fdc_datarate_w},
	{0x020, 0x027, uart8250_0_w},
	{0x028, 0x02f, uart8250_1_w},
	{0x038, 0x03a, pc_parallelport0_w},
	/* anne asic */
	{0x0e0, 0x0ef, pcw16_palette_w},
	{0x0f0, 0x0f3, pcw16_bankhw_w},
	{0x0f7, 0x0f7, pcw16_video_control_w},
	{0x0f4, 0x0f4, pcw16_keyboard_data_shift_w},
	{0x0f5, 0x0f5, pcw16_keyboard_control_w},
	{0x0f8, 0x0f8, pcw16_system_control_w},
	{0x0f9, 0x0f9, rtc_control_w},
	{0x0fa, 0x0fa, rtc_seconds_w},
	{0x0fb, 0x0fb, rtc_minutes_w},
	{0x0fc, 0x0fc, rtc_hours_w},
	{0x0fd, 0x0fd, rtc_days_w},
	{0x0fe, 0x0fe, rtc_month_w},
	{0x0ff, 0x0ff, rtc_year_w},
PORT_END

void pcw16_reset(void)
{
	/* initialise defaults */
	pcw16_fdc_int_code = 2;
	/* clear terminal count */
	pc_fdc_set_tc_state(0);
	/* select first rom page */
	pcw16_banks[0] = 0;
	pcw16_update_memory();

	/* temp rtc setup */
	rtc_seconds = 0;
	rtc_minutes = 0;
	rtc_hours = 0;
	rtc_days_max = 0;
	rtc_days = 1;
	rtc_months = 1;
	rtc_years = 0;
	rtc_control = 1;
	rtc_256ths_seconds = 0;

	pcw16_keyboard_init();
	uart8250_reset(0);
	uart8250_reset(1);
	flash_reset(0);
	flash_reset(1);
}


static PC_LPT_CONFIG lpt_config={
	1,
	LPT_UNIDIRECTIONAL, // more one of these epp/ecp aware ports
	NULL
};
static CENTRONICS_CONFIG cent_config={
	PRINTER_CENTRONICS,
	pc_lpt_handshake_in
};


void pcw16_init_machine(void)
{
	pcw16_ram = NULL;

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);

	memory_set_bankhandler_w(5, 0, MWA_BANK5);
	memory_set_bankhandler_w(6, 0, MWA_BANK6);
	memory_set_bankhandler_w(7, 0, MWA_BANK7);
	memory_set_bankhandler_w(8, 0, MWA_BANK8);



	/* dram */
	pcw16_ram = malloc(2048*1024);

	/* flash 0 */
	flash_init(0);
	flash_restore(0, "pcw16f1.nv");

	/* flash 1 */
	flash_init(1);
	flash_restore(1, "pcw16f2.nv");

	pcw16_system_status = 0;
	pcw16_interrupt_counter = 0;

	/* video ints */
	pcw16_timer = timer_pulse(TIME_IN_MSEC(5.83), 0,pcw16_timer_callback);
	/* rtc timer */
	pcw16_rtc_timer = timer_pulse(TIME_IN_SEC(1.0f/256.0f), 0, rtc_timer_callback);

	pcw16_keyboard_timer = timer_pulse(TIME_IN_HZ(50), 0, pcw16_keyboard_timer_callback);


	pc_fdc_init(&pcw16_fdc_interface);
	uart8250_init(0, pcw16_com_interface);
	uart8250_init(1, pcw16_com_interface+1);

	pc_lpt_config(0, &lpt_config);
	centronics_config(0, &cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);

	/* initialise mouse */
	pc_mouse_set_protocol(TYPE_MOUSE_SYSTEMS);
	pc_mouse_set_input_base(1);
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* initialise keyboard */
	at_keyboard_init();
	at_keyboard_set_scan_code_set(3);
	at_keyboard_set_input_port_base(4);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_AT);

	pcw16_reset();

	beep_set_state(0,0);
	beep_set_frequency(0,3750);
}


void pcw16_shutdown_machine(void)
{
	pc_fdc_exit();

	if (pcw16_ram!=NULL)
	{
		free(pcw16_ram);
		pcw16_ram = NULL;
	}

	/* flash 0 */
	flash_store(0,"pcw16f1.nv");
	flash_finish(0);

	flash_store(1,"pcw16f2.nv");
	flash_finish(1);

	if (pcw16_timer)
	{
		timer_remove(pcw16_timer);
		pcw16_timer = NULL;
	}

	if (pcw16_rtc_timer)
	{
		timer_remove(pcw16_rtc_timer);
		pcw16_rtc_timer = NULL;
	}

	if (pcw16_keyboard_timer)
	{
		timer_remove(pcw16_keyboard_timer);
		pcw16_keyboard_timer = NULL;
	}

}

INPUT_PORTS_START(pcw16)
	PORT_START
	/* vblank */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_VBLANK)
	/* power switch - default is on */
	PORT_BITX(0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Power Switch/Suspend", IP_KEY_NONE, IP_JOY_NONE)
	PORT_DIPSETTING(0x0, DEF_STR( Off) )
	PORT_DIPSETTING(0x40, DEF_STR( On) )

	INPUT_MOUSE_SYSTEMS

	AT_KEYBOARD
INPUT_PORTS_END

static struct beep_interface pcw16_beep_interface =
{
	1,
	{100}
};

static struct MachineDriver machine_driver_pcw16 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,  /* type */
			16000000,
			readmem_pcw16,		   /* MemoryReadAddress */
			writemem_pcw16,		   /* MemoryWriteAddress */
			readport_pcw16,		   /* IOReadPort */
			writeport_pcw16,		   /* IOWritePort */
			0,						   /*amstrad_frame_interrupt, *//* VBlank
										* Interrupt */
			0 /*1 */ ,				   /* vblanks per frame */
			0, 0,	/* every scanline */
		},
	},
	50, 							   /* frames per second */
	DEFAULT_REAL_60HZ_VBLANK_DURATION,	   /* vblank duration */
	1,								   /* cpu slices per frame */
	pcw16_init_machine,			   /* init machine */
	pcw16_shutdown_machine,
	/* video hardware */
	PCW16_SCREEN_WIDTH,			   /* screen width */
	PCW16_SCREEN_HEIGHT,			   /* screen height */
	{0, (PCW16_SCREEN_WIDTH - 1), 0, (PCW16_SCREEN_HEIGHT - 1)},	/* rectangle: visible_area */
	0,								   /*amstrad_gfxdecodeinfo, 			 *//* graphics
										* decode info */
	PCW16_NUM_COLOURS, 							   /* total colours */
	PCW16_NUM_COLOURS, 							   /* color table len */
	pcw16_init_palette,			   /* init palette */

	VIDEO_TYPE_RASTER,				   /* video attributes */
	0,								   /* MachineLayer */
	pcw16_vh_start,
	pcw16_vh_stop,
	pcw16_vh_screenrefresh,

		/* sound hardware */
	0,0,0,0,
	{
		{
                        SOUND_BEEP,
                        &pcw16_beep_interface
		}
	},
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

/* the lower 64k of the flash-file memory is write protected. This contains the boot
	rom. The boot rom is also on the OS rescue disc. Handy! */
ROM_START(pcw16)
	ROM_REGION((0x010000+524288), REGION_CPU1,0)
	ROM_LOAD("pcw045.sys",0x10000, 524288, 0xc642f498)
ROM_END

static const struct IODevice io_pcw16[] =
{
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"dsk\0",            /* file extensions */
		IO_RESET_NONE,		/* reset if file changed */
        NULL,               /* id */
		pc_floppy_init, 	/* init */
		pc_floppy_exit, 	/* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        floppy_status,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
//	IO_PRINTER_PORT(1,"\0"),
	{IO_END}
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT COMPANY		FULLNAME */
COMP( 1995, pcw16,	  0,		pcw16,	  pcw16,	0,	 "Amstrad plc", "PCW16")
