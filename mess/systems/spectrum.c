/***************************************************************************
		Spectrum/Inves/TK90X etc. memory map:

	CPU:
		0000-3fff ROM
		4000-ffff RAM

		Spectrum 128/+2/+2a/+3 memory map:

		CPU:
				0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
				4000-7fff Banked RAM
				8000-bfff Banked RAM
				c000-ffff Banked RAM

		TS2068 memory map: (Can't have both EXROM and DOCK active)
		The 8K EXROM can be loaded into multiple pages.

	CPU:
				0000-1fff	  ROM / EXROM / DOCK (Cartridge)
				2000-3fff	  ROM / EXROM / DOCK
				4000-5fff \
				6000-7fff  \
				8000-9fff  |- RAM / EXROM / DOCK
				a000-bfff  |
				c000-dfff  /
				e000-ffff /


Interrupts:

Changes:

29/1/2000		KT - Implemented initial +3 emulation
30/1/2000		KT - Improved input port decoding for reading
					 and therefore correct keyboard handling for Spectrum and +3
31/1/2000		KT - Implemented buzzer sound for Spectrum and +3.
					 Implementation copied from Paul Daniel's Jupiter driver.
					 Fixed screen display problems with dirty chars.
					 Added support to load .Z80 snapshots. 48k support so far.
13/2/2000		KT - Added Interface II, Kempston, Fuller and Mikrogen joystick support
17/2/2000		DJR - Added full key descriptions and Spectrum+ keys.
				Fixed Spectrum +3 keyboard problems.
17/2/2000		KT - Added tape loading from WAV/Changed from DAC to generic speaker code
18/2/2000		KT - Added tape saving to WAV
27/2/2000		KT - Took DJR's changes and added my changes.
27/2/2000		KT - Added disk image support to Spectrum +3 driver.
27/2/2000		KT - Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000		DJR - Tape handling dipswitch.
26/3/2000		DJR - Snapshot files are now classifed as snapshots not cartridges.
04/4/2000		DJR - Spectrum 128 / +2 Support.
13/4/2000		DJR - +4 Support (unofficial 48K hack).
13/4/2000		DJR - +2a Support (rom also used in +3 models).
13/4/2000		DJR - TK90X, TK95 and Inves support (48K clones).
21/4/2000		DJR - TS2068 and TC2048 support (TC2048 Supports extra video
				modes but doesn't have bank switching or sound chip).
09/5/2000		DJR - Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000		DJR - Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000		DJR - Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000		DJR - Fixed +3 Floppy support
10/2/2001		KT  - re-arranged code and split into each model emulated
					Code is split into 48k, 128k, +3, tc2048 and ts2048 segments.
					128k uses some of the functions in 48k, +3 uses some functions in 128,
					and tc2048/ts2048 use some of the functions in 48k.
					The code has been arranged so these functions come in some kind
					of "override" order, read functions changed to use READ_HANDLER
					and write functions changed to use WRITE_HANDLER
					Added Scorpion256 preliminary.
18/6/2001		DJR - Added support for Interface 2 cartridges.

 Initialisation values used when determining which model is being emulated.
   48K	   Spectrum doesn't use either port.
   128K/+2 Bank switches with port 7ffd only.
   +3/+2a  Bank switches with both ports.

	Notes:

48K machines can't run code in screen memory (128K, TS2068, TC2048 etc. OK).
Port #FF Vertical refresh not emulated (Arkanoid doesn't run).
No contented memory.
No hi-res colour effects (need contended memory first for accurate timing).
Multiface 1 and Interface 1 not supported.
Horace and the Spiders cartridge doesn't run properly.
.TZX files not supported.
128K emulation is not perfect - the 128K machines crash and hang while
running quite a lot of games.
Disk errors occur on some +3 games.

The TK90X and TK95 roms output 0 to port #df on start up.
The purpose of this port is unknown (probably display mode as TS2068) and
thus is not emulated.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/spectrum.h"
#include "eventlst.h"

/* +3 hardware */
#include "includes/nec765.h"
#include "cpuintrf.h"
#include "includes/dsk.h"


extern void spectrum_128_update_memory(void);
extern void spectrum_plus3_update_memory(void);


static struct AY8910interface spectrum_ay_interface =
{
	1,
	1773400,
	{25,25},
	{0},
	{0},
	{0}
};


/****************************************************************************************************/
/* Spectrum 48k functions */

/*
 bit 7-5: not used
 bit 4: Ear output/Speaker
 bit 3: MIC/Tape Output
 bit 2-0: border colour
*/

int PreviousFE = 0;

WRITE_HANDLER(spectrum_port_fe_w)
{
	unsigned char Changed;

	Changed = PreviousFE^data;

	/* border colour changed? */
	if ((Changed & 0x07)!=0)
	{
		/* yes - send event */
		EventList_AddItemOffset(0x0fe, data & 0x07, TIME_TO_CYCLES(0,cpu_getscanline()*cpu_getscanlineperiod()));
	}

	if ((Changed & (1<<4))!=0)
	{
		/* DAC output state */
		speaker_level_w(0,(data>>4) & 0x01);
	}

	if ((Changed & (1<<3))!=0)
	{
		/* write cassette data */
		device_output(IO_CASSETTE, 0, (data & (1<<3)) ? -32768: 32767);
	}

	PreviousFE = data;
}




static MEMORY_READ_START (spectrum_readmem)
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_r },
	{ 0x5800, 0x5aff, spectrum_colorram_r },
	{ 0x5b00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START (spectrum_writemem)
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x57ff, spectrum_characterram_w },
	{ 0x5800, 0x5aff, spectrum_colorram_w },
	{ 0x5b00, 0xffff, MWA_RAM },
MEMORY_END

/* KT: more accurate keyboard reading */
/* DJR: Spectrum+ keys added */
READ_HANDLER(spectrum_port_fe_r)
{
   int lines = offset>>8;
   int data = 0xff;

   int cs_extra1 = readinputport(8)  & 0x1f;
   int cs_extra2 = readinputport(9)  & 0x1f;
   int cs_extra3 = readinputport(10) & 0x1f;
   int ss_extra1 = readinputport(11) & 0x1f;
   int ss_extra2 = readinputport(12) & 0x1f;

   /* Caps - V */
   if ((lines & 1)==0)
   {
		data &= readinputport(0);
		/* CAPS for extra keys */
		if (cs_extra1 != 0x1f || cs_extra2 != 0x1f || cs_extra3 != 0x1f)
			data &= ~0x01;
   }

   /* A - G */
   if ((lines & 2)==0)
		data &= readinputport(1);

   /* Q - T */
   if ((lines & 4)==0)
		data &= readinputport(2);

   /* 1 - 5 */
   if ((lines & 8)==0)
		data &= readinputport(3) & cs_extra1;

   /* 6 - 0 */
   if ((lines & 16)==0)
		data &= readinputport(4) & cs_extra2;

   /* Y - P */
   if ((lines & 32)==0)
		data &= readinputport(5) & ss_extra1;

   /* H - Enter */
   if ((lines & 64)==0)
		data &= readinputport(6);

	/* B - Space */
	if ((lines & 128)==0)
	{
		data &= readinputport(7) & cs_extra3 & ss_extra2;
		/* SYMBOL SHIFT for extra keys */
		if (ss_extra1 != 0x1f || ss_extra2 != 0x1f)
			data &= ~0x02;
	}

	data |= (0xe0); /* Set bits 5-7 - as reset above */

	/* cassette input from wav */
	if (device_input(IO_CASSETTE, 0)>255 )
	{
		data &= ~0x40;
	}

	/* Issue 2 Spectrums default to having bits 5, 6 & 7 set.
	Issue 3 Spectrums default to having bits 5 & 7 set and bit 6 reset. */
	if (readinputport(16) & 0x80)
		data ^= (0x40);

	return data;
}

/* kempston joystick interface */
READ_HANDLER(spectrum_port_1f_r)
{
  return readinputport(13) & 0x1f;
}

/* fuller joystick interface */
READ_HANDLER(spectrum_port_7f_r)
{
  return readinputport(14) | (0xff^0x8f);
}

/* mikrogen joystick interface */
READ_HANDLER(spectrum_port_df_r)
{
  return readinputport(15) | (0xff^0x1f);
}

READ_HANDLER ( spectrum_port_r )
{
		if ((offset & 1)==0)
			return spectrum_port_fe_r(offset);

		if ((offset & 0xff)==0x1f)
			return spectrum_port_1f_r(offset);

		if ((offset & 0xff)==0x7f)
			return spectrum_port_7f_r(offset);

		if ((offset & 0xff)==0xdf)
			return spectrum_port_df_r(offset);

		logerror("Read from port: %04x\n", offset);

		return 0xff;
}

WRITE_HANDLER ( spectrum_port_w )
{
		if ((offset & 1)==0)
			spectrum_port_fe_w(offset,data);
		else
		{
			logerror("Write %02x to Port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (spectrum_readport)
		{0x0000, 0xffff, spectrum_port_r},
PORT_END

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_WRITE_START (spectrum_writeport)
		{0x0000, 0xffff, spectrum_port_w},
PORT_END


/****************************************************************************************************/
/* functions and data used by spectrum 128, spectrum +2, spectrum +3 and scorpion */
static unsigned char *spectrum_ram = NULL;

static int spectrum_alloc_ram(int ram_size_in_k)
{
	spectrum_ram = (unsigned char *)malloc(ram_size_in_k*1024);
	if (spectrum_ram)
	{
		memset(spectrum_ram, 0, ram_size_in_k*1024);
		return 1;
	}

	return 0;
}

static void spectrum_free_ram(void)
{
	if (spectrum_ram)
	{
		free(spectrum_ram);
		spectrum_ram = NULL;
	}
}



/****************************************************************************************************/
/* Spectrum 128 specific functions */

int spectrum_128_port_7ffd_data = -1;
unsigned char *spectrum_128_screen_location = NULL;


static WRITE_HANDLER(spectrum_128_port_7ffd_w)
{
	   /* D0-D2: RAM page located at 0x0c000-0x0ffff */
	   /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
	   /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
	   /* D5 - Disable paging */

		/* disable paging? */
		if (spectrum_128_port_7ffd_data & 0x20)
				return;

		/* store new state */
		spectrum_128_port_7ffd_data = data;

		/* update memory */
		spectrum_128_update_memory();
}

extern void spectrum_128_update_memory(void)
{
		unsigned char *ChosenROM;
		int ROMSelection;

		if (spectrum_128_port_7ffd_data & 8)
		{
				logerror("SCREEN 1: BLOCK 7\n");
				spectrum_128_screen_location = spectrum_ram + (7<<14);
		}
		else
		{
				logerror("SCREEN 0: BLOCK 5\n");
				spectrum_128_screen_location = spectrum_ram + (5<<14);
		}

		/* select ram at 0x0c000-0x0ffff */
		{
				int ram_page;
				unsigned char *ram_data;

				ram_page = spectrum_128_port_7ffd_data & 0x07;
				ram_data = spectrum_ram + (ram_page<<14);

				cpu_setbank(4, ram_data);
				cpu_setbank(8, ram_data);

				logerror("RAM at 0xc000: %02x\n",ram_page);
		}

		/* ROM switching */
		ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01);

		/* rom 0 is 128K rom, rom 1 is 48 BASIC */

		ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

		cpu_setbank(1, ChosenROM);

		logerror("rom switch: %02x\n", ROMSelection);
}



static WRITE_HANDLER(spectrum_128_port_bffd_w)
{
		AY8910_write_port_0_w(0, data);
}

static WRITE_HANDLER(spectrum_128_port_fffd_w)
{
		AY8910_control_port_0_w(0, data);
}

/* +3 manual is confused about this */

static READ_HANDLER(spectrum_128_port_fffd_r)
{
		return AY8910_read_port_0_r(0);
}

READ_HANDLER ( spectrum_128_port_r )
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 /* KT: the following is not decoded exactly, need to check what
	 is correct */
	 if ((offset & 2)==0)
	 {
		 switch ((offset>>8) & 0xff)
		 {
				case 0xff:
						return spectrum_128_port_fffd_r(offset);
		 }
	 }

	 if ((offset & 0xff)==0x1f)
		 return spectrum_port_1f_r(offset);

	 if ((offset & 0xff)==0x7f)
		 return spectrum_port_7f_r(offset);

	 if ((offset & 0xff)==0xdf)
		 return spectrum_port_df_r(offset);

	 logerror("Read from 128 port: %04x\n", offset);

	 return 0xff;
}

WRITE_HANDLER ( spectrum_128_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);

		/* Only decodes on A15, A14 & A1 */
		else if ((offset & 2)==0)
		{
				switch ((offset>>8) & 0xc0)
				{
						case 0x40:
								spectrum_128_port_7ffd_w(offset, data);
								break;
						case 0x80:
								spectrum_128_port_bffd_w(offset, data);
								break;
						case 0xc0:
								spectrum_128_port_fffd_w(offset, data);
								break;
						default:
								logerror("Write %02x to 128 port: %04x\n", data, offset);
				}
		}
		else
		{
			logerror("Write %02x to 128 port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (spectrum_128_readport)
	{0x0000, 0xffff, spectrum_128_port_r},
PORT_END

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_WRITE_START (spectrum_128_writeport)
	{0x0000, 0xffff, spectrum_128_port_w},
PORT_END


static MEMORY_READ_START (spectrum_128_readmem)
	{ 0x0000, 0x3fff, MRA_BANK1 },
	{ 0x4000, 0x7fff, MRA_BANK2 },
	{ 0x8000, 0xbfff, MRA_BANK3 },
	{ 0xc000, 0xffff, MRA_BANK4 },
MEMORY_END

static MEMORY_WRITE_START (spectrum_128_writemem)
	{ 0x0000, 0x3fff, MWA_BANK5 },
	{ 0x4000, 0x7fff, MWA_BANK6 },
	{ 0x8000, 0xbfff, MWA_BANK7 },
	{ 0xc000, 0xffff, MWA_BANK8 },
MEMORY_END


void spectrum_128_init_machine(void)
{
	if (spectrum_alloc_ram(128)!=0)
	{
		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);

		/* 0x0000-0x3fff always holds ROM */
		memory_set_bankhandler_w(5, 0, MWA_ROM);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);
		memory_set_bankhandler_w(7, 0, MWA_BANK7);
		memory_set_bankhandler_w(8, 0, MWA_BANK8);


		/* Bank 5 is always in 0x4000 - 0x7fff */
		cpu_setbank(2, spectrum_ram + (5<<14));
		cpu_setbank(6, spectrum_ram + (5<<14));

		/* Bank 2 is always in 0x8000 - 0xbfff */
		cpu_setbank(3, spectrum_ram + (2<<14));
		cpu_setbank(7, spectrum_ram + (2<<14));

		/* set initial ram config */
		spectrum_128_port_7ffd_data = 0;
		spectrum_128_update_memory();

		spectrum_init_machine();
	}
}



void  spectrum_128_exit_machine(void)
{
	spectrum_free_ram();
}



/****************************************************************************************************/
/* Spectrum + 3 specific functions */
/* This driver uses some of the spectrum_128 functions. The +3 is similar to a spectrum 128
but with a disc drive */

int spectrum_plus3_port_1ffd_data = -1;


static nec765_interface spectrum_plus3_nec765_interface =
{
		NULL,
		NULL
};


static int spectrum_plus3_memory_selections[]=
{
		0,1,2,3,
		4,5,6,7,
		4,5,6,3,
		4,7,6,3
};

static WRITE_HANDLER(spectrum_plus3_port_3ffd_w)
{
		if (~readinputport(16) & 0x20)
				nec765_data_w(0,data);
}

static READ_HANDLER(spectrum_plus3_port_3ffd_r)
{
		if (readinputport(16) & 0x20)
				return 0xff;
		else
				return nec765_data_r(0);
}


static READ_HANDLER(spectrum_plus3_port_2ffd_r)
{
		if (readinputport(16) & 0x20)
				return 0xff;
		else
				return nec765_status_r(0);
}


extern void spectrum_plus3_update_memory(void)
{
		if (spectrum_128_port_7ffd_data & 8)
		{
				logerror("+3 SCREEN 1: BLOCK 7\n");
				spectrum_128_screen_location = spectrum_ram + (7<<14);
		}
		else
		{
				logerror("+3 SCREEN 0: BLOCK 5\n");
				spectrum_128_screen_location = spectrum_ram + (5<<14);
		}

		if ((spectrum_plus3_port_1ffd_data & 0x01)==0)
		{
				int ram_page;
				unsigned char *ram_data;

				/* ROM switching */
				unsigned char *ChosenROM;
				int ROMSelection;

				/* select ram at 0x0c000-0x0ffff */
				ram_page = spectrum_128_port_7ffd_data & 0x07;
				ram_data = spectrum_ram + (ram_page<<14);

				cpu_setbank(4, ram_data);
				cpu_setbank(8, ram_data);

				logerror("RAM at 0xc000: %02x\n",ram_page);

				/* Reset memory between 0x4000 - 0xbfff in case extended paging was being used */
				/* Bank 5 in 0x4000 - 0x7fff */
				cpu_setbank(2, spectrum_ram + (5<<14));
				cpu_setbank(6, spectrum_ram + (5<<14));

				/* Bank 2 in 0x8000 - 0xbfff */
				cpu_setbank(3, spectrum_ram + (2<<14));
				cpu_setbank(7, spectrum_ram + (2<<14));


				ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01) |
					((spectrum_plus3_port_1ffd_data>>1) & 0x02);

				/* rom 0 is editor, rom 1 is syntax, rom 2 is DOS, rom 3 is 48 BASIC */

				ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

				cpu_setbank(1, ChosenROM);
				memory_set_bankhandler_w(5, 0, MWA_ROM);

				logerror("rom switch: %02x\n", ROMSelection);
		}
		else
		{
				/* Extended memory paging */

				int *memory_selection;
				int MemorySelection;
				unsigned char *ram_data;

				MemorySelection = (spectrum_plus3_port_1ffd_data>>1) & 0x03;

				memory_selection = &spectrum_plus3_memory_selections[(MemorySelection<<2)];

				ram_data = spectrum_ram + (memory_selection[0]<<14);
				cpu_setbank(1, ram_data);
				cpu_setbank(5, ram_data);
				/* allow writes to 0x0000-0x03fff */
				memory_set_bankhandler_w(5, 0, MWA_BANK5);

				ram_data = spectrum_ram + (memory_selection[1]<<14);
				cpu_setbank(2, ram_data);
				cpu_setbank(6, ram_data);

				ram_data = spectrum_ram + (memory_selection[2]<<14);
				cpu_setbank(3, ram_data);
				cpu_setbank(7, ram_data);

				ram_data = spectrum_ram + (memory_selection[3]<<14);
				cpu_setbank(4, ram_data);
				cpu_setbank(8, ram_data);

				logerror("extended memory paging: %02x\n",MemorySelection);
		 }
}



static WRITE_HANDLER(spectrum_plus3_port_7ffd_w)
{
	   /* D0-D2: RAM page located at 0x0c000-0x0ffff */
	   /* D3 - Screen select (screen 0 in ram page 5, screen 1 in ram page 7 */
	   /* D4 - ROM select - which rom paged into 0x0000-0x03fff */
	   /* D5 - Disable paging */

		/* disable paging? */
		if (spectrum_128_port_7ffd_data & 0x20)
				return;

		/* store new state */
		spectrum_128_port_7ffd_data = data;

		/* update memory */
		spectrum_plus3_update_memory();
}

static WRITE_HANDLER(spectrum_plus3_port_1ffd_w)
{

		/* D0-D1: ROM/RAM paging */
		/* D2: Affects if d0-d1 work on ram/rom */
		/* D3 - Disk motor on/off */
		/* D4 - parallel port strobe */

		floppy_drive_set_motor_state(0, data & (1<<3));
		floppy_drive_set_motor_state(1, data & (1<<3));
		floppy_drive_set_ready_state(0, 1, 1);
		floppy_drive_set_ready_state(1, 1, 1);

		spectrum_plus3_port_1ffd_data = data;

		/* disable paging? */
		if ((spectrum_128_port_7ffd_data & 0x20)==0)
		{
				/* no */
				spectrum_plus3_update_memory();
		}
}

READ_HANDLER ( spectrum_plus3_port_r )
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 /* KT: the following is not decoded exactly, need to check what
	 is correct */
	 if ((offset & 2)==0)
	 {
		 switch ((offset>>8) & 0xff)
		 {
				case 0xff: return spectrum_128_port_fffd_r(offset);
				case 0x2f: return spectrum_plus3_port_2ffd_r(offset);
				case 0x3f: return spectrum_plus3_port_3ffd_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		 }
	 }

	 logerror("Read from +3 port: %04x\n", offset);

	 return 0xff;
}

WRITE_HANDLER ( spectrum_plus3_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);

		/* the following is not decoded exactly, need to check
		what is correct! */
		else if ((offset & 2)==0)
		{
				switch ((offset>>8) & 0xf0)
				{
						case 0x70:
								spectrum_plus3_port_7ffd_w(offset, data);
								break;
						case 0xb0:
								spectrum_128_port_bffd_w(offset, data);
								break;
						case 0xf0:
								spectrum_128_port_fffd_w(offset, data);
								break;
						case 0x10:
								spectrum_plus3_port_1ffd_w(offset, data);
								break;
						case 0x30:
								spectrum_plus3_port_3ffd_w(offset, data);
						default:
								logerror("Write %02x to +3 port: %04x\n", data, offset);
				}
		}
		else
		{
			logerror("Write %02x to +3 port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (spectrum_plus3_readport)
		{0x0000, 0xffff, spectrum_plus3_port_r},
PORT_END

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_WRITE_START (spectrum_plus3_writeport)
		{0x0000, 0xffff, spectrum_plus3_port_w},
PORT_END


void spectrum_plus3_init_machine(void)
{
	if (spectrum_alloc_ram(128))
	{

		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);

		memory_set_bankhandler_w(5, 0, MWA_BANK5);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);
		memory_set_bankhandler_w(7, 0, MWA_BANK7);
		memory_set_bankhandler_w(8, 0, MWA_BANK8);

		nec765_init(&spectrum_plus3_nec765_interface, NEC765A);

		floppy_drive_set_geometry(0, FLOPPY_DRIVE_SS_40);
		floppy_drive_set_geometry(1, FLOPPY_DRIVE_SS_40);

		/* Initial configuration */
		spectrum_128_port_7ffd_data = 0;
		spectrum_plus3_port_1ffd_data = 0;
		spectrum_plus3_update_memory();

		spectrum_init_machine();
	}
}

void	spectrum_plus3_exit_machine(void)
{
	nec765_stop();
	spectrum_free_ram();
}



/****************************************************************************************************/
/* TS2048 specific functions */


int ts2068_port_ff_data = -1; /* Display enhancement control */
int ts2068_port_f4_data = -1; /* Horizontal Select Register */
unsigned char *ts2068_ram = NULL;

static READ_HANDLER(ts2068_port_f4_r)
{
		return ts2068_port_f4_data;
}

static WRITE_HANDLER(ts2068_port_f4_w)
{
		ts2068_port_f4_data = data;
		ts2068_update_memory();
}

static WRITE_HANDLER(ts2068_port_f5_w)
{
		AY8910_control_port_0_w(0, data);
}

static READ_HANDLER(ts2068_port_f6_r)
{
		/* TODO - Reading from register 14 reads the joystick ports
		   set bit 8 of address to read joystick #1
		   set bit 9 of address to read joystick #2
		   if both bits are set then OR values
		   Bit 0 up, 1 down, 2 left, 3 right, 7 fire active low. Other bits 1
		*/
		return AY8910_read_port_0_r(0);
}

static WRITE_HANDLER(ts2068_port_f6_w)
{
		AY8910_write_port_0_w(0, data);
}

static READ_HANDLER(ts2068_port_ff_r)
{
		return ts2068_port_ff_data;
}

static WRITE_HANDLER(ts2068_port_ff_w)
{
		/* Bits 0-2 Video Mode Select
		   Bits 3-5 64 column mode ink/paper selection
					(See ts2068_vh_screenrefresh for more info)
		   Bit	6	17ms Interrupt Inhibit
		   Bit	7	Cartridge (0) / EXROM (1) select
		*/
		ts2068_port_ff_data = data;
		ts2068_update_memory();
		logerror("Port %04x write %02x\n", offset, data);
}


READ_HANDLER ( ts2068_port_r )
{
		switch (offset & 0xff)
		{
				/* Note: keys only decoded on port #fe not all even ports so
				   ports #f4 & #f6 correctly read */
				case 0xf4: return ts2068_port_f4_r(offset);
				case 0xf6: return ts2068_port_f6_r(offset);
				case 0xff: return ts2068_port_ff_r(offset);

				case 0xfe: return spectrum_port_fe_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		}
		logerror("Read from port: %04x\n", offset);

		return 0xff;
}

WRITE_HANDLER ( ts2068_port_w )
{
/* Ports #fd & #fc were reserved by Timex for bankswitching and are not used
   by either the hardware or system software.
   Port #fb is the Thermal printer port and works exactly as the Sinclair
   Printer - ie not yet emulated.
*/
		switch (offset & 0xff)
		{
				case 0xfe: spectrum_port_fe_w(offset,data); break;
				case 0xf4: ts2068_port_f4_w(offset,data); break;
				case 0xf5: ts2068_port_f5_w(offset,data); break;
				case 0xf6: ts2068_port_f6_w(offset,data); break;
				case 0xff: ts2068_port_ff_w(offset,data); break;
				default:
						logerror("Write %02x to Port: %04x\n", data, offset);
		}
}



/*******************************************************************
 *
 *		Bank switch between the 3 internal memory banks HOME, EXROM
 *		and DOCK (Cartridges). The HOME bank contains 16K ROM in the
 *		0-16K area and 48K RAM fills the rest. The EXROM contains 8K
 *		ROM and can appear in every 8K segment (ie 0-8K, 8-16K etc).
 *		The DOCK is empty and is meant to be occupied by cartridges
 *		you can plug into the cartridge dock of the 2068.
 *
 *		The address space is divided into 8 8K chunks. Bit 0 of port
 *		#f4 corresponds to the 0-8K chunk, bit 1 to the 8-16K chunk
 *		etc. If the bit is 0 then the chunk is controlled by the HOME
 *		bank. If the bit is 1 then the chunk is controlled by either
 *		the DOCK or EXROM depending on bit 7 of port #ff. Note this
 *		means that that the Z80 can't see chunks of the EXROM and DOCK
 *		at the same time.
 *
 *******************************************************************/
extern void ts2068_update_memory(void)
{
		unsigned char *ChosenROM, *ExROM, *DOCK;

		DOCK = timex_cart_data;

		ExROM = memory_region(REGION_CPU1) + 0x014000;

		if (ts2068_port_f4_data & 0x01)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(1, ExROM);
						memory_set_bankhandler_r(1, 0, MRA_BANK1);
						memory_set_bankhandler_w(9, 0, MWA_ROM);
						logerror("0000-1fff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(1, DOCK);
							memory_set_bankhandler_r(1, 0, MRA_BANK1);
							if (timex_cart_chunks&0x01)
								memory_set_bankhandler_w(9, 0, MWA_BANK9);
							else
								memory_set_bankhandler_w(9, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(1, 0, MRA_NOP);
							memory_set_bankhandler_w(9, 0, MWA_ROM);
						}
						logerror("0000-1fff Cartridge\n");
				}
		}
		else
		{
				ChosenROM = memory_region(REGION_CPU1) + 0x010000;
				cpu_setbank(1, ChosenROM);
				memory_set_bankhandler_r(1, 0, MRA_BANK1);
				memory_set_bankhandler_w(9, 0, MWA_ROM);
				logerror("0000-1fff HOME\n");
		}

		if (ts2068_port_f4_data & 0x02)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(2, ExROM);
						memory_set_bankhandler_r(2, 0, MRA_BANK2);
						memory_set_bankhandler_w(10, 0, MWA_ROM);
						logerror("2000-3fff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(2, DOCK+0x2000);
							memory_set_bankhandler_r(2, 0, MRA_BANK2);
							if (timex_cart_chunks&0x02)
								memory_set_bankhandler_w(10, 0, MWA_BANK10);
							else
								memory_set_bankhandler_w(10, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(2, 0, MRA_NOP);
							memory_set_bankhandler_w(10, 0, MWA_ROM);
						}
						logerror("2000-3fff Cartridge\n");
				}
		}
		else
		{
				ChosenROM = memory_region(REGION_CPU1) + 0x012000;
				cpu_setbank(2, ChosenROM);
				memory_set_bankhandler_r(2, 0, MRA_BANK2);
				memory_set_bankhandler_w(10, 0, MWA_ROM);
				logerror("2000-3fff HOME\n");
		}

		if (ts2068_port_f4_data & 0x04)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(3, ExROM);
						memory_set_bankhandler_r(3, 0, MRA_BANK3);
						memory_set_bankhandler_w(11, 0, MWA_ROM);
						logerror("4000-5fff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(3, DOCK+0x4000);
							memory_set_bankhandler_r(3, 0, MRA_BANK3);
							if (timex_cart_chunks&0x04)
								memory_set_bankhandler_w(11, 0, MWA_BANK11);
							else
								memory_set_bankhandler_w(11, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(3, 0, MRA_NOP);
							memory_set_bankhandler_w(11, 0, MWA_ROM);
						}
						logerror("4000-5fff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(3, ts2068_ram);
				cpu_setbank(11, ts2068_ram);
				memory_set_bankhandler_r(3, 0, MRA_BANK3);
				memory_set_bankhandler_w(11, 0, MWA_BANK11);
				logerror("4000-5fff RAM\n");
		}

		if (ts2068_port_f4_data & 0x08)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(4, ExROM);
						memory_set_bankhandler_r(4, 0, MRA_BANK4);
						memory_set_bankhandler_w(12, 0, MWA_ROM);
						logerror("6000-7fff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(4, DOCK+0x6000);
							memory_set_bankhandler_r(4, 0, MRA_BANK4);
							if (timex_cart_chunks&0x08)
								memory_set_bankhandler_w(12, 0, MWA_BANK12);
							else
								memory_set_bankhandler_w(12, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(4, 0, MRA_NOP);
							memory_set_bankhandler_w(12, 0, MWA_ROM);
						}
						logerror("6000-7fff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(4, ts2068_ram + 0x2000);
				cpu_setbank(12, ts2068_ram + 0x2000);
				memory_set_bankhandler_r(4, 0, MRA_BANK4);
				memory_set_bankhandler_w(12, 0, MWA_BANK12);
				logerror("6000-7fff RAM\n");
		}

		if (ts2068_port_f4_data & 0x10)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(5, ExROM);
						memory_set_bankhandler_r(5, 0, MRA_BANK5);
						memory_set_bankhandler_w(13, 0, MWA_ROM);
						logerror("8000-9fff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(5, DOCK+0x8000);
							memory_set_bankhandler_r(5, 0, MRA_BANK5);
							if (timex_cart_chunks&0x10)
								memory_set_bankhandler_w(13, 0, MWA_BANK13);
							else
								memory_set_bankhandler_w(13, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(5, 0, MRA_NOP);
							memory_set_bankhandler_w(13, 0, MWA_ROM);
						}
						logerror("8000-9fff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(5, ts2068_ram + 0x4000);
				cpu_setbank(13, ts2068_ram + 0x4000);
				memory_set_bankhandler_r(5, 0, MRA_BANK5);
				memory_set_bankhandler_w(13, 0, MWA_BANK13);
				logerror("8000-9fff RAM\n");
		}

		if (ts2068_port_f4_data & 0x20)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(6, ExROM);
						memory_set_bankhandler_r(6, 0, MRA_BANK6);
						memory_set_bankhandler_w(14, 0, MWA_ROM);
						logerror("a000-bfff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(6, DOCK+0xa000);
							memory_set_bankhandler_r(6, 0, MRA_BANK6);
							if (timex_cart_chunks&0x20)
								memory_set_bankhandler_w(14, 0, MWA_BANK14);
							else
								memory_set_bankhandler_w(14, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(6, 0, MRA_NOP);
							memory_set_bankhandler_w(14, 0, MWA_ROM);
						}
						logerror("a000-bfff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(6, ts2068_ram + 0x6000);
				cpu_setbank(14, ts2068_ram + 0x6000);
				memory_set_bankhandler_r(6, 0, MRA_BANK6);
				memory_set_bankhandler_w(14, 0, MWA_BANK14);
				logerror("a000-bfff RAM\n");
		}

		if (ts2068_port_f4_data & 0x40)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(7, ExROM);
						memory_set_bankhandler_r(7, 0, MRA_BANK7);
						memory_set_bankhandler_w(15, 0, MWA_ROM);
						logerror("c000-dfff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(7, DOCK+0xc000);
							memory_set_bankhandler_r(7, 0, MRA_BANK7);
							if (timex_cart_chunks&0x40)
								memory_set_bankhandler_w(15, 0, MWA_BANK15);
							else
								memory_set_bankhandler_w(15, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(7, 0, MRA_NOP);
							memory_set_bankhandler_w(15, 0, MWA_ROM);
						}
						logerror("c000-dfff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(7, ts2068_ram + 0x8000);
				cpu_setbank(15, ts2068_ram + 0x8000);
				memory_set_bankhandler_r(7, 0, MRA_BANK7);
				memory_set_bankhandler_w(15, 0, MWA_BANK15);
				logerror("c000-dfff RAM\n");
		}

		if (ts2068_port_f4_data & 0x80)
		{
				if (ts2068_port_ff_data & 0x80)
				{
						cpu_setbank(8, ExROM);
						memory_set_bankhandler_r(8, 0, MRA_BANK8);
						memory_set_bankhandler_w(16, 0, MWA_ROM);
						logerror("e000-ffff EXROM\n");
				}
				else
				{
						if (timex_cart_type == TIMEX_CART_DOCK)
						{
							cpu_setbank(8, DOCK+0xe000);
							memory_set_bankhandler_r(8, 0, MRA_BANK8);
							if (timex_cart_chunks&0x80)
								memory_set_bankhandler_w(16, 0, MWA_BANK16);
							else
								memory_set_bankhandler_w(16, 0, MWA_ROM);
						}
						else
						{
							memory_set_bankhandler_r(8, 0, MRA_NOP);
							memory_set_bankhandler_w(16, 0, MWA_ROM);
						}
						logerror("e000-ffff Cartridge\n");
				}
		}
		else
		{
				cpu_setbank(8, ts2068_ram + 0xa000);
				cpu_setbank(16, ts2068_ram + 0xa000);
				memory_set_bankhandler_r(8, 0, MRA_BANK8);
				memory_set_bankhandler_w(16, 0, MWA_BANK16);
				logerror("e000-ffff RAM\n");
		}
}


static PORT_READ_START (ts2068_readport)
	{0x0000, 0x0ffff, ts2068_port_r},
PORT_END

static PORT_WRITE_START (ts2068_writeport)
	{0x0000, 0x0ffff, ts2068_port_w},
PORT_END

static MEMORY_READ_START (ts2068_readmem)
	{ 0x0000, 0x1fff, MRA_BANK1 },
	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x5fff, MRA_BANK3 },
	{ 0x6000, 0x7fff, MRA_BANK4 },
	{ 0x8000, 0x9fff, MRA_BANK5 },
	{ 0xa000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, MRA_BANK7 },
	{ 0xe000, 0xffff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START (ts2068_writemem)
	{ 0x0000, 0x1fff, MWA_BANK9 },
	{ 0x2000, 0x3fff, MWA_BANK10 },
	{ 0x4000, 0x5fff, MWA_BANK11 },
	{ 0x6000, 0x7fff, MWA_BANK12 },
	{ 0x8000, 0x9fff, MWA_BANK13 },
	{ 0xa000, 0xbfff, MWA_BANK14 },
	{ 0xc000, 0xdfff, MWA_BANK15 },
	{ 0xe000, 0xffff, MWA_BANK16 },
MEMORY_END



void ts2068_init_machine(void)
{
		ts2068_ram = (unsigned char *)malloc(48*1024);
		if(!ts2068_ram) return;
		memset(ts2068_ram, 0, 48*1024);

		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_r(4, 0, MRA_BANK4);
		memory_set_bankhandler_r(5, 0, MRA_BANK5);
		memory_set_bankhandler_r(6, 0, MRA_BANK6);
		memory_set_bankhandler_r(7, 0, MRA_BANK7);
		memory_set_bankhandler_r(8, 0, MRA_BANK8);

		/* 0x0000-0x3fff always holds ROM */
		memory_set_bankhandler_w(9, 0, MWA_BANK9);
		memory_set_bankhandler_w(10, 0, MWA_BANK10);
		memory_set_bankhandler_w(11, 0, MWA_BANK11);
		memory_set_bankhandler_w(12, 0, MWA_BANK12);
		memory_set_bankhandler_w(13, 0, MWA_BANK13);
		memory_set_bankhandler_w(14, 0, MWA_BANK14);
		memory_set_bankhandler_w(15, 0, MWA_BANK15);
		memory_set_bankhandler_w(16, 0, MWA_BANK16);

		ts2068_port_ff_data = 0;
		ts2068_port_f4_data = 0;
		ts2068_update_memory();

		spectrum_init_machine();
}

void ts2068_exit_machine(void)
{
		if (ts2068_ram!=NULL)
				free(ts2068_ram);
}



/****************************************************************************************************/
/* TC2048 specific functions */


static void tc2048_port_ff_w(int offset, int data)
{
		ts2068_port_ff_data = data;
		logerror("Port %04x write %02x\n", offset, data);
}

READ_HANDLER ( tc2048_port_r )
{
		if ((offset & 1)==0)
				return spectrum_port_fe_r(offset);
		switch (offset & 0xff)
		{
				case 0xff: return ts2068_port_ff_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		}

		logerror("Read from port: %04x\n", offset);
		return 0xff;
}

WRITE_HANDLER ( tc2048_port_w )
{
		if ((offset & 1)==0)
				spectrum_port_fe_w(offset,data);
		else if ((offset & 0xff)==0xff)
				tc2048_port_ff_w(offset,data);
		else
		{
				logerror("Write %02x to Port: %04x\n", data, offset);
		}
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (tc2048_readport)
	{0x0000, 0x0ffff, tc2048_port_r},
PORT_END

/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_WRITE_START (tc2048_writeport)
	{0x0000, 0x0ffff, tc2048_port_w},
PORT_END


static MEMORY_READ_START (tc2048_readmem)
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0xffff, MRA_BANK1 },
MEMORY_END

static MEMORY_WRITE_START (tc2048_writemem)
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0xffff, MWA_BANK2 },
MEMORY_END


void tc2048_init_machine(void)
{
		ts2068_ram = (unsigned char *)malloc(48*1024);
		if(!ts2068_ram) return;
		memset(ts2068_ram, 0, 48*1024);

		memory_set_bankhandler_r(1, 0, MRA_BANK1);
		memory_set_bankhandler_w(2, 0, MWA_BANK2);
		cpu_setbank(1, ts2068_ram);
		cpu_setbank(2, ts2068_ram);
		ts2068_port_ff_data = 0;

		spectrum_init_machine();
}


/****************************************************************************************************/
/* BETADISK/TR-DOS disc controller emulation */
/* microcontroller KR1818VG93 is a russian wd179x clone */
#include "includes/wd179x.h"

/*
DRQ (D6) and INTRQ (D7).
DRQ - signal showing request of data by microcontroller
INTRQ - signal of completion of execution of command.
*/

static int betadisk_status;
static int betadisk_active;
static void (*betadisk_memory_update)(void);

static OPBASE_HANDLER(betadisk_opbase_handler)
{

	int pc;

	pc = cpu_get_pc();

	if ((pc & 0xc000)!=0x0000)
	{
		/* outside rom area */
		betadisk_active = 0;

		betadisk_memory_update();
	}
	else
	{
		/* inside rom area, switch on betadisk */
	//	betadisk_active = 1;

	//	betadisk_memory_update();
	}


	return pc & 0x0ffff;
}

static void betadisk_wd179x_callback(int state)
{
	switch (state)
	{
		case WD179X_DRQ_SET:
		{
			betadisk_status |= (1<<6);
		}
		break;

		case WD179X_DRQ_CLR:
		{
			betadisk_status &=~(1<<6);
		}
		break;

		case WD179X_IRQ_SET:
		{
			betadisk_status |= (1<<7);
		}
		break;

		case WD179X_IRQ_CLR:
		{
			betadisk_status &=~(1<<7);
		}
		break;
	}
}

/* these are active only when betadisk is enabled */
WRITE_HANDLER(betadisk_w)
{

	if (betadisk_active)
	{

	}
}


/* these are active only when betadisk is enabled */
READ_HANDLER(betadisk_r)
{
	if (betadisk_active)
	{
		/* decoding of these ports might be wrong - to be checked! */
		if ((offset & 0x01f)==0x01f)
		{
			switch (offset & 0x0ff)
			{

			}
		}

	}

	return 0x0ff;
}

void	 betadisk_init(void)
{
	betadisk_active = 0;
	betadisk_status = 0x03f;
	wd179x_init(WD_TYPE_179X,&betadisk_wd179x_callback);
}

void	betadisk_exit(void)
{
	wd179x_exit();
}

/****************************************************************************************************/
/* Zs Scorpion 256 */

/*
port 7ffd. full compatibility with Zx spectrum 128. digits are:

D0-D2 - number of RAM page to put in C000-FFFF
D3    - switch of address for RAM of screen. 0 - 4000, 1 - c000
D4    - switch of ROM : 0-zx128, 1-zx48
D5    - 1 in this bit will block further output in port 7FFD, until reset.
*/

/*
port 1ffd - additional port for resources of computer.

D0    - block of ROM in 0-3fff. when set to 1 - allows read/write page 0 of RAM
D1    - selects ROM expansion. this rom contains main part of service monitor.
D2    - not used
D3    - used for output in RS-232C
D4    - extended RAM. set to 1 - connects RAM page with number 8-15 in
	C000-FFFF. number of page is given in gidits D0-D2 of port 7FFD
D5    - signal of strobe for interface centronics. to form the strobe has to be
	set to 1.
D6-D7 - not used. ( yet ? )
*/

/* rom 0=zx128, 1=zx48, 2 = service monitor, 3=tr-dos */

static int scorpion_256_port_1ffd_data = 0;

extern void scorpion_update_memory(void)
{
		unsigned char *ChosenROM;
		int ROMSelection;

		if (spectrum_128_port_7ffd_data & 8)
		{
				logerror("SCREEN 1: BLOCK 7\n");
				spectrum_128_screen_location = spectrum_ram + (7<<14);
		}
		else
		{
				logerror("SCREEN 0: BLOCK 5\n");
				spectrum_128_screen_location = spectrum_ram + (5<<14);
		}

		/* select ram at 0x0c000-0x0ffff */
		{
				int ram_page;
				unsigned char *ram_data;

				ram_page = (spectrum_128_port_7ffd_data & 0x07) | ((scorpion_256_port_1ffd_data & (1<<4))>>1);
				ram_data = spectrum_ram + (ram_page<<14);

				cpu_setbank(4, ram_data);
				cpu_setbank(8, ram_data);

				logerror("RAM at 0xc000: %02x\n",ram_page);
		}

		if (scorpion_256_port_1ffd_data & (1<<0))
		{
			/* ram at 0x0000 */
			logerror("RAM at 0x0000\n");

			/* connect page 0 of ram to 0x0000 */
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(5, 0, MWA_BANK5);
			cpu_setbank(1, spectrum_ram+(8<<14));
			cpu_setbank(5, spectrum_ram+(8<<14));
		}
		else
		{
			/* rom at 0x0000 */
			logerror("ROM at 0x0000\n");

			/* connect page 0 of rom to 0x0000 */
			memory_set_bankhandler_r(1, 0, MRA_BANK1);
			memory_set_bankhandler_w(5, 0, MWA_NOP);

			if (scorpion_256_port_1ffd_data & (1<<1))
			{
				ROMSelection = 2;
			}
			else
			{

				/* ROM switching */
				ROMSelection = ((spectrum_128_port_7ffd_data>>4) & 0x01);
			}

			/* rom 0 is 128K rom, rom 1 is 48 BASIC */
			ChosenROM = memory_region(REGION_CPU1) + 0x010000 + (ROMSelection<<14);

			cpu_setbank(1, ChosenROM);

			logerror("rom switch: %02x\n", ROMSelection);
		}
}


static WRITE_HANDLER(scorpion_port_7ffd_w)
{
	logerror("scorpion 7ffd w: %02x\n", data);

	/* disable paging? */
	if (spectrum_128_port_7ffd_data & 0x20)
		return;

	/* store new state */
	spectrum_128_port_7ffd_data = data;

	/* update memory */
	scorpion_update_memory();
}

static WRITE_HANDLER(scorpion_port_1ffd_w)
{
	logerror("scorpion 1ffd w: %02x\n", data);

	scorpion_256_port_1ffd_data = data;

	/* disable paging? */
	if ((spectrum_128_port_7ffd_data & 0x20)==0)
	{
		scorpion_update_memory();
	}
}


/* not sure if decoding is full or partial on scorpion */
/* TO BE CHECKED! */
static READ_HANDLER(scorpion_port_r)
{
	 if ((offset & 1)==0)
	 {
		 return spectrum_port_fe_r(offset);
	 }

	 /* KT: the following is not decoded exactly, need to check what
	 is correct */
	 if ((offset & 2)==0)
	 {
		 switch ((offset>>8) & 0xff)
		 {
				case 0xff: return spectrum_128_port_fffd_r(offset);
				case 0x1f: return spectrum_port_1f_r(offset);
				case 0x7f: return spectrum_port_7f_r(offset);
				case 0xdf: return spectrum_port_df_r(offset);
		 }
	 }
#if 0
	 switch (offset & 0x0ff)
	 {
		case 0x01f:
			return wd179x_status_r(offset);
		case 0x03f:
			return wd179x_track_r(offset);
		case 0x05f:
			return wd179x_sector_r(offset);
		case 0x07f:
			return wd179x_data_r(offset);
		case 0x0ff:
			return betadisk_status;
	 }
#endif
	 logerror("Read from scorpion port: %04x\n", offset);

	 return 0xff;
}


/* not sure if decoding is full or partial on scorpion */
/* TO BE CHECKED! */
static WRITE_HANDLER(scorpion_port_w)
{
		if ((offset & 1)==0)
			spectrum_port_fe_w(offset,data);

		else if ((offset & 2)==0)
		{
				switch ((offset>>8) & 0xf0)
				{
					case 0x70:
							scorpion_port_7ffd_w(offset, data);
							break;
					case 0xb0:
							spectrum_128_port_bffd_w(offset, data);
							break;
					case 0xf0:
							spectrum_128_port_fffd_w(offset, data);
							break;
					case 0x10:
							scorpion_port_1ffd_w(offset, data);
							break;
					default:
							logerror("Write %02x to scorpion port: %04x\n", data, offset);
				}
		}
		else
		{
			logerror("Write %02x to scorpion port: %04x\n", data, offset);
		}
#if 0
		/* decoding of these ports might be wrong - to be checked! */
			switch (offset & 0x0ff)
			{
				case 0x01f:
				{
					wd179x_command_w(offset,data);
				}
				break;

				case 0x03f:
				{
					wd179x_track_w(offset,data);
				}
				break;

				case 0x05f:
				{
					wd179x_sector_w(offset,data);
				}
				break;

				case 0x07f:
				{
					wd179x_data_w(offset,data);
				}
				break;

				case 0x0ff:
				{
					int density;

					/*
						D0, D1 - diskdrive select. 00 for drive A, 01 for drive B
									   10 for drive C, 11 for drive D
						D2     - hardware microcontroller reset. by resetting and then setting this bit
							 again, we can form impulse of microcontroller reset. usually this reset
							 happenes in very begin of TR-DOS session.

						D3     - this digit blocks signal HLT of microcontroller. For normal work must
							 contain '1'.

						D4     - Diskdrive head select. contents of this digit translates directly to
							 diskdrive. 0 means first head or 'bottom' side of disk, 1 - second
							 head/'top' side of disk.

						D5     - Density select. reset of this digit makes microcontroller works in FM
							 mode, seted digit - MFM.
					*/

					wd179x_set_drive(data & 0x03);

			//		if (data & (1<<2))
			//		{
			//			wd179x_reset();
			//		}

					wd179x_set_side((data>>4) & 0x01);

					if (data & (1<<5))
					{
						density = DEN_FM_HI;
					}
					else
					{
						density = DEN_MFM_LO;
					}


					wd179x_set_density(data>>5);
				}
			}
#endif
}



/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (scorpion_readport)
		{0x0000, 0xffff, scorpion_port_r},
PORT_END

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static PORT_WRITE_START (scorpion_writeport)
		{0x0000, 0xffff, scorpion_port_w},
PORT_END


void scorpion_init_machine(void)
{
	if (spectrum_alloc_ram(256))
	{
		memory_set_bankhandler_r(4, 0, MRA_BANK4);
		memory_set_bankhandler_w(8, 0, MWA_BANK8);

		/* Bank 5 is always in 0x4000 - 0x7fff */
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);
		cpu_setbank(2, spectrum_ram + (5<<14));
		cpu_setbank(6, spectrum_ram + (5<<14));

		/* Bank 2 is always in 0x8000 - 0xbfff */
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_w(7, 0, MWA_BANK7);
		cpu_setbank(3, spectrum_ram + (2<<14));
		cpu_setbank(7, spectrum_ram + (2<<14));


		spectrum_128_port_7ffd_data = 0;
		scorpion_256_port_1ffd_data = 0;

		scorpion_update_memory();

		betadisk_init();
	}
}

void	scorpion_exit_machine(void)
{
	betadisk_exit();
	spectrum_free_ram();
}



/****************************************************************************************************/
/* pentagon */

static READ_HANDLER(pentagon_port_r)
{



	return 0x0ff;

}


static WRITE_HANDLER(pentagon_port_w)
{


}



/* ports are not decoded full.
The function decodes the ports appropriately */
static PORT_READ_START (pentagon_readport)
		{0x0000, 0xffff, pentagon_port_r},
PORT_END

/* KT: Changed it to this because the ports are not decoded fully.
The function decodes the ports appropriately */
static PORT_WRITE_START (pentagon_writeport)
		{0x0000, 0xffff, pentagon_port_w},
PORT_END


void pentagon_init_machine(void)
{
	if (spectrum_alloc_ram(128))
	{
		memory_set_bankhandler_r(4, 0, MRA_BANK4);

		memory_set_bankhandler_w(8, 0, MWA_BANK8);

		/* Bank 5 is always in 0x4000 - 0x7fff */
		memory_set_bankhandler_r(2, 0, MRA_BANK2);
		memory_set_bankhandler_w(6, 0, MWA_BANK6);
		cpu_setbank(2, spectrum_ram + (5<<14));
		cpu_setbank(6, spectrum_ram + (5<<14));

		/* Bank 2 is always in 0x8000 - 0xbfff */
		memory_set_bankhandler_r(3, 0, MRA_BANK3);
		memory_set_bankhandler_w(7, 0, MWA_BANK7);
		cpu_setbank(3, spectrum_ram + (2<<14));
		cpu_setbank(7, spectrum_ram + (2<<14));

		betadisk_init();
	}
}

void	pentagon_exit_machine(void)
{
	betadisk_exit();
	spectrum_free_ram();
}


/****************************************************************************************************/

static struct GfxLayout spectrum_charlayout = {
	8,8,
	256,
	1,						/* 1 bits per pixel */

	{ 0 },					/* no bitplanes; 1 bit per pixel */

	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 8*256, 16*256, 24*256, 32*256, 40*256, 48*256, 56*256 },

	8				/* every char takes 1 consecutive byte */
};

static struct GfxDecodeInfo spectrum_gfxdecodeinfo[] = {
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ 0, 0x0, &spectrum_charlayout, 0, 0x80 },
	{ -1 } /* end of array */
};

INPUT_PORTS_START( spectrum )
	PORT_START /* 0xFEFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS SHIFT",                       KEYCODE_LSHIFT,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z  COPY    :      LN       BEEP",  KEYCODE_Z,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X  CLEAR   Pound  EXP      INK",   KEYCODE_X,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C  CONT    ?      LPRINT   PAPER", KEYCODE_C,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V  CLS     /      LLIST    FLASH", KEYCODE_V,  IP_JOY_NONE )

	PORT_START /* 0xFDFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A  NEW     STOP   READ     ~",  KEYCODE_A,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S  SAVE    NOT    RESTORE  |",  KEYCODE_S,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D  DIM     STEP   DATA     \\", KEYCODE_D,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F  FOR     TO     SGN      {",  KEYCODE_F,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G  GOTO    THEN   ABS      }",  KEYCODE_G,  IP_JOY_NONE )

	PORT_START /* 0xFBFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q  PLOT    <=     SIN      ASN",    KEYCODE_Q,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W  DRAW    <>     COS      ACS",    KEYCODE_W,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E  REM     >=     TAN      ATN",    KEYCODE_E,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R  RUN     <      INT      VERIFY", KEYCODE_R,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T  RAND    >      RND      MERGE",  KEYCODE_T,  IP_JOY_NONE )

		/* interface II uses this port for joystick */
	PORT_START /* 0xF7FE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1          !      BLUE     DEF FN", KEYCODE_1,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2          @      RED      FN",     KEYCODE_2,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3          #      MAGENTA  LINE",   KEYCODE_3,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4          $      GREEN    OPEN#",  KEYCODE_4,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5          %      CYAN     CLOSE#", KEYCODE_5,  IP_JOY_NONE )

		/* protek clashes with interface II! uses 5 = left, 6 = down, 7 = up, 8 = right, 0 = fire */
	PORT_START /* 0xEFFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0          _      BLACK    FORMAT", KEYCODE_0,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9          )               POINT",  KEYCODE_9,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8          (               CAT",    KEYCODE_8,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7          '      WHITE    ERASE",  KEYCODE_7,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6          &      YELLOW   MOVE",   KEYCODE_6,  IP_JOY_NONE )

	PORT_START /* 0xDFFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P  PRINT   \"      TAB      (c)", KEYCODE_P,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O  POKE    ;      PEEK     OUT", KEYCODE_O,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I  INPUT   AT     CODE     IN",  KEYCODE_I,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U  IF      OR     CHR$     ]",   KEYCODE_U,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y  RETURN  AND    STR$     [",   KEYCODE_Y,  IP_JOY_NONE )

	PORT_START /* 0xBFFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER",                              KEYCODE_ENTER,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L  LET     =      USR      ATTR",    KEYCODE_L,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K  LIST    +      LEN      SCREEN$", KEYCODE_K,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J  LOAD    -      VAL      VAL$",    KEYCODE_J,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H  GOSUB   ^      SQR      CIRCLE",  KEYCODE_H,  IP_JOY_NONE )

	PORT_START /* 0x7FFE */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE",                              KEYCODE_SPACE,   IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SYMBOL SHIFT",                       KEYCODE_RSHIFT,  IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M  PAUSE   .      PI       INVERSE", KEYCODE_M,  IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N  NEXT    ,      INKEY$   OVER",    KEYCODE_N,  IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B  BORDER  *      BIN      BRIGHT",  KEYCODE_B,  IP_JOY_NONE )

		PORT_START /* Spectrum+ Keys (set CAPS + 1-5) */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "EDIT          (CAPS + 1)",  KEYCODE_F1,         IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CAPS LOCK     (CAPS + 2)",  KEYCODE_CAPSLOCK,   IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "TRUE VID      (CAPS + 3)",  KEYCODE_F2,         IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "INV VID       (CAPS + 4)",  KEYCODE_F3,         IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor left   (CAPS + 5)",  KEYCODE_LEFT,       IP_JOY_NONE )
		PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

		PORT_START /* Spectrum+ Keys (set CAPS + 6-0) */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL           (CAPS + 0)",  KEYCODE_BACKSPACE,  IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "GRAPH         (CAPS + 9)",  KEYCODE_LALT,       IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor right  (CAPS + 8)",  KEYCODE_RIGHT,      IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor up     (CAPS + 7)",  KEYCODE_UP,         IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cursor down   (CAPS + 6)",  KEYCODE_DOWN,       IP_JOY_NONE )
		PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

		PORT_START /* Spectrum+ Keys (set CAPS + SPACE and CAPS + SYMBOL */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK",                     KEYCODE_PAUSE,      IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXT MODE",                  KEYCODE_LCONTROL,   IP_JOY_NONE )
		PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

		PORT_START /* Spectrum+ Keys (set SYMBOL SHIFT + O/P */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_F4,  IP_JOY_NONE )
/*		  PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_QUOTE,  IP_JOY_NONE ) */
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON,  IP_JOY_NONE )
		PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

		PORT_START /* Spectrum+ Keys (set SYMBOL SHIFT + N/M */
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP,   IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA,  IP_JOY_NONE )
		PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)

		PORT_START /* Kempston joystick interface */
		PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK RIGHT",     IP_KEY_NONE,    JOYCODE_1_RIGHT )
		PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK LEFT",      IP_KEY_NONE,   JOYCODE_1_LEFT )
		PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK DOWN",         IP_KEY_NONE,        JOYCODE_1_DOWN )
		PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK UP",         IP_KEY_NONE,        JOYCODE_1_UP)
		PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "KEMPSTON JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1 )

		PORT_START /* Fuller joystick interface */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK UP",     IP_KEY_NONE,    JOYCODE_1_UP )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK DOWN",      IP_KEY_NONE,   JOYCODE_1_DOWN )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK LEFT",         IP_KEY_NONE,        JOYCODE_1_LEFT )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK RIGHT",         IP_KEY_NONE,        JOYCODE_1_RIGHT)
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "FULLER JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1)

		PORT_START /* Mikrogen joystick interface */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK UP",     IP_KEY_NONE,    JOYCODE_1_UP )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK DOWN",      IP_KEY_NONE,   JOYCODE_1_DOWN )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK RIGHT",         IP_KEY_NONE,        JOYCODE_1_RIGHT )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK LEFT",         IP_KEY_NONE,        JOYCODE_1_LEFT)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "MIKROGEN JOYSTICK FIRE",         IP_KEY_NONE,        JOYCODE_1_BUTTON1)


		PORT_START
		PORT_BITX(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Quickload", KEYCODE_F8, IP_JOY_NONE)
		PORT_DIPNAME(0x80, 0x00, "Hardware Version")
		PORT_DIPSETTING(0x00, "Issue 2" )
		PORT_DIPSETTING(0x80, "Issue 3" )
		PORT_DIPNAME(0x40, 0x00, "End of .TAP action")
		PORT_DIPSETTING(0x00, "Disable .TAP support" )
		PORT_DIPSETTING(0x40, "Rewind tape to start (to reload earlier levels)" )
		PORT_DIPNAME(0x20, 0x00, "+3/+2a etc. Disk Drive")
		PORT_DIPSETTING(0x00, "Enabled" )
		PORT_DIPSETTING(0x20, "Disabled" )
		PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static unsigned char spectrum_palette[16*3] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xbf,
	0xbf, 0x00, 0x00, 0xbf, 0x00, 0xbf,
	0x00, 0xbf, 0x00, 0x00, 0xbf, 0xbf,
	0xbf, 0xbf, 0x00, 0xbf, 0xbf, 0xbf,

	0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
	0xff, 0x00, 0x00, 0xff, 0x00, 0xff,
	0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
};

static unsigned short spectrum_colortable[128*2] = {
	0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	1,0, 1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7,
	2,0, 2,1, 2,2, 2,3, 2,4, 2,5, 2,6, 2,7,
	3,0, 3,1, 3,2, 3,3, 3,4, 3,5, 3,6, 3,7,
	4,0, 4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7,
	5,0, 5,1, 5,2, 5,3, 5,4, 5,5, 5,6, 5,7,
	6,0, 6,1, 6,2, 6,3, 6,4, 6,5, 6,6, 6,7,
	7,0, 7,1, 7,2, 7,3, 7,4, 7,5, 7,6, 7,7,

	 8,8,  8,9,  8,10,	8,11,  8,12,  8,13,  8,14,	8,15,
	 9,8,  9,9,  9,10,	9,11,  9,12,  9,13,  9,14,	9,15,
	10,8, 10,9, 10,10, 10,11, 10,12, 10,13, 10,14, 10,15,
	11,8, 11,9, 11,10, 11,11, 11,12, 11,13, 11,14, 11,15,
	12,8, 12,9, 12,10, 12,11, 12,12, 12,13, 12,14, 12,15,
	13,8, 13,9, 13,10, 13,11, 13,12, 13,13, 13,14, 13,15,
	14,8, 14,9, 14,10, 14,11, 14,12, 14,13, 14,14, 14,15,
	15,8, 15,9, 15,10, 15,11, 15,12, 15,13, 15,14, 15,15
};
/* Initialise the palette */
static void spectrum_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom)
{
	memcpy(sys_palette,spectrum_palette,sizeof(spectrum_palette));
	memcpy(sys_colortable,spectrum_colortable,sizeof(spectrum_colortable));
}

int spec_interrupt (void)
{
	static int quickload = 0;

		if (!quickload && (readinputport(16) & 0x8000))
		{
				spec_quick_open (0, 0, NULL);
				quickload = 1;
		}
		else
				quickload = 0;

		return interrupt ();
}

static struct Speaker_interface spectrum_speaker_interface=
{
 1,
 {50},
};

static struct Wave_interface spectrum_wave_interface=
{
	1,	  /* number of cassette drives = number of waves to mix */
	{25},	/* default mixing level */

};

static struct MachineDriver machine_driver_spectrum =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,		/* 3.5 Mhz */
			spectrum_readmem,spectrum_writemem,
			spectrum_readport,spectrum_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	spectrum_init_machine,
	spectrum_shutdown_machine,

	/* video hardware */
	SPEC_SCREEN_WIDTH,				/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER,
	spectrum_eof_callback,
	spectrum_vh_start,
	spectrum_vh_stop,
	spectrum_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* standard spectrum sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};

static struct MachineDriver machine_driver_spectrum_128 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3546900,		/* 3.54690 Mhz */
			spectrum_128_readmem,spectrum_128_writemem,
			spectrum_128_readport,spectrum_128_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	spectrum_128_init_machine,
	spectrum_128_exit_machine,

	/* video hardware */
	SPEC_SCREEN_WIDTH,				/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER,
	spectrum_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum buzzer sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface,
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};

static struct MachineDriver machine_driver_spectrum_plus3 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3546900,		/* 3.54690 Mhz */
			spectrum_128_readmem,spectrum_128_writemem,
			spectrum_plus3_readport,spectrum_plus3_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	spectrum_plus3_init_machine,
	spectrum_plus3_exit_machine,

	/* video hardware */
	SPEC_SCREEN_WIDTH,				/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER,
	spectrum_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/*  Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum buzzer sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface,
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};


static struct MachineDriver machine_driver_ts2068 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3580000,		/* 3.58 Mhz */
			ts2068_readmem,ts2068_writemem,
			ts2068_readport,ts2068_writeport,
			spec_interrupt,1,
		},
	},
		60, 2500,		/* frames per second, vblank duration */
	1,
	ts2068_init_machine,
	ts2068_exit_machine,

	/* video hardware */
	TS2068_SCREEN_WIDTH,			/* screen width */
	TS2068_SCREEN_HEIGHT,			/* screen height */
	{ 0, TS2068_SCREEN_WIDTH-1, 0, TS2068_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	ts2068_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	ts2068_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/*  Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};

static struct MachineDriver machine_driver_uk2086 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3580000,		/* 3.58 Mhz */
			ts2068_readmem,ts2068_writemem,
			ts2068_readport,ts2068_writeport,
			spec_interrupt,1,
		},
	},
		50, 2500,		/* frames per second, vblank duration */
	1,
	ts2068_init_machine,
	ts2068_exit_machine,

	/* video hardware */
	TS2068_SCREEN_WIDTH,			/* screen width */
	TS2068_SCREEN_HEIGHT,			/* screen height */
	{ 0, TS2068_SCREEN_WIDTH-1, 0, TS2068_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	ts2068_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	ts2068_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/*  Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};

static struct MachineDriver machine_driver_tc2048 =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3500000,		/* 3.5 Mhz */
			tc2048_readmem,tc2048_writemem,
			tc2048_readport,tc2048_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	tc2048_init_machine,
	ts2068_exit_machine,

	/* video hardware */
	TS2068_SCREEN_WIDTH,			/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, TS2068_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,
	spectrum_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	tc2048_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* standard spectrum sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};


static struct MachineDriver machine_driver_scorpion =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3546900,		/* 3.54690 Mhz */
			spectrum_128_readmem,spectrum_128_writemem,
			scorpion_readport,scorpion_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	scorpion_init_machine,
	scorpion_exit_machine,

	/* video hardware */
	SPEC_SCREEN_WIDTH,				/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER,
	spectrum_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum buzzer sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface,
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};

static struct MachineDriver machine_driver_pentagon =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80|CPU_16BIT_PORT,
			3546900,		/* 3.54690 Mhz */
			spectrum_128_readmem,spectrum_128_writemem,
			pentagon_readport,pentagon_writeport,
			spec_interrupt,1,
		},
	},
	50, 2500,		/* frames per second, vblank duration */
	1,
	pentagon_init_machine,
	pentagon_exit_machine,

	/* video hardware */
	SPEC_SCREEN_WIDTH,				/* screen width */
	SPEC_SCREEN_HEIGHT, 			/* screen height */
	{ 0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1},  /* visible_area */
	spectrum_gfxdecodeinfo, 			 /* graphics decode info */
	16, 256,							 /* colors used for the characters */
	spectrum_init_palette,				 /* initialise palette */

	VIDEO_TYPE_RASTER,
	spectrum_eof_callback,
	spectrum_128_vh_start,
	spectrum_128_vh_stop,
	spectrum_128_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* Ay-3-8912 sound */
		{
				SOUND_AY8910,
				&spectrum_ay_interface,
		},
		/* standard spectrum buzzer sound */
		{
				SOUND_SPEAKER,
				&spectrum_speaker_interface,
		},
		/* cassette wave sound */
		{
				SOUND_WAVE,
				&spectrum_wave_interface,
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("spectrum.rom", 0x0000, 0x4000, 0xddee531f)
ROM_END

ROM_START(specbusy)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-busy.rom", 0x0000, 0x4000, 0x1511cddb)
ROM_END

ROM_START(specpsch)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-psych.rom", 0x0000, 0x4000, 0xcd60b589)
ROM_END

ROM_START(specgrot)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-groot.rom", 0x0000, 0x4000, 0xabf18c45)
ROM_END

ROM_START(specimc)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("48-imc.rom", 0x0000, 0x4000, 0xd1be99ee)
ROM_END

ROM_START(speclec)
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("80-lec.rom", 0x0000, 0x4000, 0x5b5c92b1)
ROM_END

ROM_START(spec128)
		ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zx128_0.rom",0x10000,0x4000, 0xe76799d2)
	ROM_LOAD("zx128_1.rom",0x14000,0x4000, 0xb96a36be)
ROM_END

ROM_START(spec128s)
		ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zx128s0.rom",0x10000,0x4000, 0x453d86b2)
	ROM_LOAD("zx128s1.rom",0x14000,0x4000, 0x6010e796)
ROM_END

ROM_START(specpls2)
		ROM_REGION(0x18000,REGION_CPU1,0)
	ROM_LOAD("zxp2_0.rom",0x10000,0x4000, 0x5d2e8c66)
	ROM_LOAD("zxp2_1.rom",0x14000,0x4000, 0x98b1320b)
ROM_END

ROM_START(specpl2a)
		ROM_REGION(0x20000,REGION_CPU1,0)
		ROM_LOAD("p2a41_0.rom",0x10000,0x4000, 0x30c9f490)
		ROM_LOAD("p2a41_1.rom",0x14000,0x4000, 0xa7916b3f)
		ROM_LOAD("p2a41_2.rom",0x18000,0x4000, 0xc9a0b748)
		ROM_LOAD("p2a41_3.rom",0x1c000,0x4000, 0xb88fd6e3)
ROM_END

ROM_START(specpls3)
		ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("pl3-0.rom",0x10000,0x4000, 0x17373da2)
	ROM_LOAD("pl3-1.rom",0x14000,0x4000, 0xf1d1d99e)
	ROM_LOAD("pl3-2.rom",0x18000,0x4000, 0x3dbf351d)
	ROM_LOAD("pl3-3.rom",0x1c000,0x4000, 0x04448eaa)
ROM_END

ROM_START(specpls4)
		ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("plus4.rom",0x0000,0x4000, 0x7e0f47cb)
ROM_END

ROM_START(tk90x)
		ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("tk90x.rom",0x0000,0x4000, 0x3e785f6f)
ROM_END

ROM_START(tk95)
		ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("tk95.rom",0x0000,0x4000, 0x17368e07)
ROM_END

ROM_START(inves)
		ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("inves.rom",0x0000,0x4000, 0x8ff7a4d1)
ROM_END

ROM_START(tc2048)
		ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("tc2048.rom",0x0000,0x4000, 0xf1b5fa67)
ROM_END

ROM_START(ts2068)
		ROM_REGION(0x16000,REGION_CPU1,0)
		ROM_LOAD("ts2068_h.rom",0x10000,0x4000, 0xbf44ec3f)
		ROM_LOAD("ts2068_x.rom",0x14000,0x2000, 0xae16233a)
ROM_END

ROM_START(uk2086)
		ROM_REGION(0x16000,REGION_CPU1,0)
		ROM_LOAD("uk2086_h.rom",0x10000,0x4000, 0x5ddc0ca2)
		ROM_LOAD("ts2068_x.rom",0x14000,0x2000, 0xae16233a)
ROM_END

ROM_START(specp2fr)
		ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("plus2fr0.rom",0x10000,0x4000, 0xc684c535)
		ROM_LOAD("plus2fr1.rom",0x14000,0x4000, 0xf5e509c5)
ROM_END

ROM_START(specp2sp)
		ROM_REGION(0x18000,REGION_CPU1,0)
		ROM_LOAD("plus2sp0.rom",0x10000,0x4000, 0xe807d06e)
		ROM_LOAD("plus2sp1.rom",0x14000,0x4000, 0x41981d4b)
ROM_END

ROM_START(specp3sp)
		ROM_REGION(0x20000,REGION_CPU1,0)
		ROM_LOAD("plus3sp0.rom",0x10000,0x4000, 0x1f86147a)
		ROM_LOAD("plus3sp1.rom",0x14000,0x4000, 0xa8ac4966)
		ROM_LOAD("plus3sp2.rom",0x18000,0x4000, 0xf6bb0296)
		ROM_LOAD("plus3sp3.rom",0x1c000,0x4000, 0xf6d25389)
ROM_END

ROM_START(specpl3e)
		ROM_REGION(0x20000,REGION_CPU1,0)
		ROM_LOAD("roma.bin",0x10000,0x8000, 0x7c20e2c9)
		ROM_LOAD("romb.bin",0x18000,0x8000, 0x4a700c7e)
ROM_END

ROM_START(scorpion)
		ROM_REGION(0x020000, REGION_CPU1, 0)
		ROM_LOAD("scorp0.rom",0x010000, 0x4000, 0x0eb40a09)
		ROM_LOAD("scorp1.rom",0x014000, 0x4000, 0x9d513013)
		ROM_LOAD("scorp2.rom",0x018000, 0x4000, 0xfd0d3ce1)
		ROM_LOAD("scorp3.rom",0x01c000, 0x4000, 0x1fe1d003)
ROM_END

ROM_START(pentagon)
		ROM_REGION(0x020000, REGION_CPU1, 0)
ROM_END

#define IODEVICE_SPEC_QUICK \
{\
   IO_QUICKLOAD,	   /* type */\
   1,				   /* count */\
   "scr\0",            /* file extensions */\
   IO_RESET_ALL,	   /* reset if file changed */\
   NULL,			   /* id */\
   spec_quick_init,    /* init */\
   spec_quick_exit,    /* exit */\
   NULL,			   /* info */\
   spec_quick_open,    /* open */\
   NULL,			   /* close */\
   NULL,			   /* status */\
   NULL,			   /* seek */\
   NULL,			   /* input */\
   NULL,			   /* output */\
   NULL,			   /* input_chunk */\
   NULL 			   /* output_chunk */\
}

static const struct IODevice io_spectrum[] = {
	{
		IO_SNAPSHOT,		/* type */
		1,					/* count */
		"sna\0z80\0",       /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		0,
		spectrum_snap_load,	/* init */
		spectrum_snap_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
		IODEVICE_SPEC_QUICK,
		IO_CASSETTE_WAVE(1,"wav\0tap\0", NULL,spectrum_cassette_init, spectrum_cassette_exit),
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"rom\0",			/* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		0,
		spectrum_cart_load,	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

static const struct IODevice io_specpls3[] = {
	{
		IO_SNAPSHOT,		/* type */
		1,					/* count */
		"sna\0z80\0",       /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		0,
		spectrum_snap_load,	/* init */
		spectrum_snap_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
		IODEVICE_SPEC_QUICK,
		IO_CASSETTE_WAVE(1,"wav\0tap\0", NULL,spectrum_cassette_init, spectrum_cassette_exit),
	{
		IO_FLOPPY,			/* type */
		2,					/* count */
		"dsk\0",            /* file extensions */
		IO_RESET_NONE,		/* reset if file changed */
		0,
		dsk_floppy_load,	/* init */
		dsk_floppy_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
                floppy_status,                           /* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL,				/* output chunk */
	},
	{ IO_END }
};

static const struct IODevice io_ts2068[] = {
	{
		IO_SNAPSHOT,		/* type */
		1,					/* count */
		"sna\0z80\0",       /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		0,
		spectrum_snap_load,	/* init */
		spectrum_snap_exit,	/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
		IODEVICE_SPEC_QUICK,
		IO_CASSETTE_WAVE(1,"wav\0tap\0", NULL,spectrum_cassette_init, spectrum_cassette_exit),
	{
		IO_CARTSLOT,			/* type */
		1,				/* count */
		"dck\0",			/* file extensions */
		IO_RESET_ALL,			/* reset if file changed */
		0,
		timex_cart_load,		/* init */
		timex_cart_exit,		/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
		{ IO_END }
};

#define io_spec128	io_spectrum
#define io_spec128s	io_spectrum
#define io_specpls2	io_spectrum
#define io_specbusy	io_spectrum
#define io_specpsch	io_spectrum
#define io_specgrot	io_spectrum
#define io_specimc	io_spectrum
#define io_speclec	io_spectrum
#define io_specpls4	io_spectrum
#define io_inves	io_spectrum
#define io_tk90x	io_spectrum
#define io_tk95 	io_spectrum
#define io_tc2048	io_spectrum
#define io_uk2086	io_ts2068
#define io_specpl2a	io_specpls3
#define io_specp2fr	io_spectrum
#define io_specp2sp	io_spectrum
#define io_specp3sp	io_specpls3
#define io_specpl3e	io_specpls3
#define io_scorpion	io_specpls3
#define io_pentagon	io_specpls3

/*     YEAR  NAME      PARENT    MACHINE   INPUT     INIT  COMPANY		FULLNAME */
COMP ( 1982, spectrum, 0,	 spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum" )
COMPX( 2000, specpls4, spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum +4", GAME_COMPUTER_MODIFIED )
COMPX( 1994, specbusy, spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum (BusySoft Upgrade v1.18)", GAME_COMPUTER_MODIFIED )
COMPX( ????, specpsch, spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum (Maly's Psycho Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( ????, specgrot, spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum (De Groot's Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( 1985, specimc,  spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum (Collier's Upgrade)", GAME_COMPUTER_MODIFIED )
COMPX( 1987, speclec,  spectrum, spectrum, spectrum,	0, "Sinclair Research",	"ZX Spectrum (LEC Upgrade)", GAME_COMPUTER_MODIFIED )
COMP ( 1986, inves,    spectrum, spectrum, spectrum,	0, "Investronica",	"Inves Spectrum 48K+" )
COMP ( 1985, tk90x,    spectrum, spectrum, spectrum,	0, "Micro Digital",	"TK-90x Color Computer" )
COMP ( 1986, tk95,     spectrum, spectrum, spectrum,	0, "Micro Digital",	"TK-95 Color Computer" )
COMP ( 198?, tc2048,   spectrum, tc2048,   spectrum,	0, "Timex of Portugal",	"TC-2048" )
COMP ( 1983, ts2068,   spectrum, ts2068,   spectrum,	0, "Timex Sinclair",	"TS-2068" )
COMP ( 1986, uk2086,   spectrum, uk2086,   spectrum,	0, "Unipolbrit",	"UK-2086 ver. 1.2" )

COMPX( 1986, spec128,  0,		 spectrum_128,	 spectrum, 0,			 "Sinclair Research",    "ZX Spectrum 128" ,GAME_NOT_WORKING)
COMPX( 1985, spec128s, spec128,  spectrum_128,	 spectrum, 0,			 "Sinclair Research",    "ZX Spectrum 128 (Spain)" ,GAME_NOT_WORKING)
COMPX( 1986, specpls2, spec128,  spectrum_128,	 spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +2" ,GAME_NOT_WORKING)
COMPX( 1987, specpl2a, spec128,  spectrum_plus3, spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +2a" ,GAME_NOT_WORKING)
COMPX( 1987, specpls3, spec128,  spectrum_plus3, spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +3" ,GAME_NOT_WORKING)

COMPX( 1986, specp2fr, spec128,  spectrum_128,	 spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +2 (France)" ,GAME_NOT_WORKING)
COMPX( 1986, specp2sp, spec128,  spectrum_128,	 spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +2 (Spain)" ,GAME_NOT_WORKING)
COMPX( 1987, specp3sp, spec128,  spectrum_plus3, spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +3 (Spain)" ,GAME_NOT_WORKING)
COMPX( 2000, specpl3e, spec128,  spectrum_plus3, spectrum, 0,			 "Amstrad plc",          "ZX Spectrum +3e" , GAME_NOT_WORKING|GAME_COMPUTER_MODIFIED )

COMPX( ????, scorpion, 0, scorpion,	spectrum, 0,			"Zonov and Co.",		"Zs Scorpion 256", GAME_NOT_WORKING)
COMPX( ????, pentagon, spectrum, pentagon,	spectrum, 0,			"???",		"Pentagon", GAME_NOT_WORKING)
