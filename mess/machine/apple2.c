/***************************************************************************

  apple2.c

  Machine file to handle emulation of the Apple II series.

  TODO:  Make a standard set of peripherals work.
  TODO:  Allow swappable peripherals in each slot.
  TODO:  Verify correctness of C08X switches.
			- need to do double-read before write-enable RAM

***************************************************************************/

/* common.h included for the RomModule definition */
//#include "common.h"
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"

UINT8 *apple2_slot_rom;
UINT8 *apple2_slot1;
UINT8 *apple2_slot2;
UINT8 *apple2_slot3;
UINT8 *apple2_slot4;
UINT8 *apple2_slot5;
UINT8 *apple2_slot6;
UINT8 *apple2_slot7;

/*UINT8 *apple2_rom;*/

APPLE2_STRUCT a2;

/* local */

static int a2_speaker_state;

static void mockingboard_init (int slot);
static int mockingboard_r (int offset);
static void mockingboard_w (int offset, int data);
static WRITE_HANDLER ( apple2_mainram_w );
static WRITE_HANDLER ( apple2_auxram_w );

/***************************************************************************
  apple2_init_machine
***************************************************************************/
void apple2e_init_machine(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* Init our language card banks to initially point to ROM */
	memory_set_bankhandler_w (1, 0, MWA_ROM);
	cpu_setbank (1, &RAM[0x21000]);
	memory_set_bankhandler_w (2, 0, MWA_ROM);
	cpu_setbank (2, &RAM[0x22000]);
	/* Use built-in slot ROM ($c100-$c7ff) */
	cpu_setbank (3, &RAM[0x20100]);
	/* Use main zp/stack */
	cpu_setbank (4, &RAM[0x0000]);
	/* Use main RAM */
	memory_set_bankhandler_w (5, 0, apple2_mainram_w);
	cpu_setbank (5, &RAM[0x0200]);
	/* Use built-in slot ROM ($c800) */
	cpu_setbank (6, &RAM[0x20800]);

	/* Slot 3 is funky - it isn't mapped like the other slot ROMs */
	memcpy (&RAM[0x24200], &RAM[0x20300], 0x100);

	AY3600_init();

	memset (&a2, 0, sizeof (APPLE2_STRUCT));
	a2_speaker_state = 0;

	/* TODO: add more initializers as we add more slots */
	mockingboard_init (4);
	apple2_slot6_init();
}

/***************************************************************************
  apple2_id_rom
***************************************************************************/
#if 1
/* R Nabet : the proto does not match the function at all !!! */
int  apple2_id_rom(const char *name, const char * gamename)
{
	return 0;
}
#else
int apple2_id_rom (int id)
{
	FILE *romfile;
	UINT8 magic[4];
	int retval;

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0))) return 0;

	retval = 0;
	/* Verify the file is in Apple II format */
	osd_fread (romfile, magic, 4);
	if (memcmp(magic,"2IMG",4)==0)
		retval = 1;

	osd_fclose (romfile);
	return retval;
}
#endif

/***************************************************************************
  apple2e_load_rom
***************************************************************************/
int apple2e_load_rom (int id)
{
	/* Initialize second half of graphics memory to 0xFF for sneaky decoding purposes */
	memset(memory_region(REGION_GFX1) + 0x1000, 0xFF, 0x1000);

	return INIT_PASS;
}

/***************************************************************************
  apple2ee_load_rom
***************************************************************************/
int apple2ee_load_rom (int id)
{
	/* Initialize second half of graphics memory to 0xFF for sneaky decoding purposes */
	memset(memory_region(REGION_GFX1) + 0x1000, 0xFF, 0x1000);

	return INIT_PASS;
}

/***************************************************************************
  apple2_interrupt
***************************************************************************/
int apple2_interrupt(void)
{
	int irq_freq = 1;

	irq_freq --;
	if (irq_freq < 0) irq_freq = 1;

	/* We poll the keyboard periodically to scan the keys.  This is
	   actually consistent with how the AY-3600 keyboard controller works. */
	AY3600_interrupt();

	/* control-reset mapped to control-delete */
	if (osd_is_key_pressed (KEYCODE_LCONTROL) && osd_is_key_pressed (KEYCODE_BACKSPACE))
	{
		cpu_set_reset_line (0,PULSE_LINE);
		return 0;
	}
	else
	{
		if (irq_freq)
			return interrupt ();
		else
			return 0;
	}
}

/***************************************************************************
  apple2_LC_ram1_w
***************************************************************************/
WRITE_HANDLER ( apple2_LC_ram1_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xc000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram2_w
***************************************************************************/
WRITE_HANDLER ( apple2_LC_ram2_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xd000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_LC_ram_w
***************************************************************************/
WRITE_HANDLER ( apple2_LC_ram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	RAM[0xe000 + offset + aux_offset] = data;
}

/***************************************************************************
  apple2_mainram_w
***************************************************************************/
WRITE_HANDLER ( apple2_mainram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	RAM[0x0200 + offset] = data;
}

/***************************************************************************
  apple2_auxram_w
***************************************************************************/
WRITE_HANDLER ( apple2_auxram_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	RAM[0x10200 + offset] = data;
}

void apple2_slotrom_disable (int offset, int data)
{
//	a2_RDCXROM = 1;
}

/***************************************************************************
  apple2_c00x_r
***************************************************************************/
READ_HANDLER ( apple2_c00x_r )
{
	/* Read the keyboard data and strobe */
	return AY3600_keydata_strobe_r();
}

/***************************************************************************
  apple2_c00x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c00x_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	switch (offset)
	{
		/* 80STOREOFF */
		case 0x00:		a2.STORE80 = 0x00;		break;
		/* 80STOREON - use 80-column memory mapping */
		case 0x01:		a2.STORE80 = 0x80;		break;
		/* RAMRDOFF */
		case 0x02:
			a2.RAMRD = 0x00;
			cpu_setbank (5, &RAM[0x0200]);
			break;
		/* RAMRDON - read from aux 48k */
		case 0x03:
			a2.RAMRD = 0x80;
			cpu_setbank (5, &RAM[0x10200]);
			break;
		/* RAMWRTOFF */
		case 0x04:
			a2.RAMWRT = 0x00;
			memory_set_bankhandler_w (5, 0, apple2_mainram_w);
			break;
		/* RAMWRTON - write to aux 48k */
		case 0x05:
			a2.RAMWRT = 0x80;
			memory_set_bankhandler_w (5, 0, apple2_auxram_w);
			break;
		/* INTCXROMOFF */
		case 0x06:
			a2.INTCXROM = 0x00;
			/* TODO: don't switch slot 3 */
			cpu_setbank (3, &RAM[0x24000]);
			break;
		/* INTCXROMON */
		case 0x07:
			a2.INTCXROM = 0x80;
			/* TODO: don't switch slot 3 */
			cpu_setbank (3, &RAM[0x20100]);
			break;
		/* ALTZPOFF */
		case 0x08:
			a2.ALTZP = 0x00;
			cpu_setbank (4, &RAM[0x00000]);
			if (a2.LC_RAM)
			{
				cpu_setbank (2, &RAM[0xe000]);
				if (a2.LC_RAM2)
				{
					cpu_setbank (1, &RAM[0xd000]);
				}
				else
				{
					cpu_setbank (1, &RAM[0xc000]);
				}
			}
			break;
		/* ALTZPON - use aux ZP, stack and language card area */
		case 0x09:
			a2.ALTZP = 0x80;
			cpu_setbank (4, &RAM[0x10000]);
			if (a2.LC_RAM)
			{
				cpu_setbank (2, &RAM[0x1e000]);
				if (a2.LC_RAM2)
				{
					cpu_setbank (1, &RAM[0x1d000]);
				}
				else
				{
					cpu_setbank (1, &RAM[0x1c000]);
				}
			}
			break;
		/* SLOTC3ROMOFF */
		case 0x0A:		a2.SLOTC3ROM = 0x00;	break;
		/* SLOTC3ROMON - use external slot 3 ROM */
		case 0x0B:		a2.SLOTC3ROM = 0x80;	break;
		/* 80COLOFF */
		case 0x0C:		a2.COL80 = 0x00;		break;
		/* 80COLON - use 80-column display mode */
		case 0x0D:		a2.COL80 = 0x80;		break;
		/* ALTCHARSETOFF */
		case 0x0E:		a2.ALTCHARSET = 0x00;	break;
		/* ALTCHARSETON - use alt character set */
		case 0x0F:		a2.ALTCHARSET = 0x80;	break;
	}

	logerror("a2 softswitch_w: %04x\n", offset + 0xc000);
}

/***************************************************************************
  apple2_c01x_r
***************************************************************************/
READ_HANDLER ( apple2_c01x_r )
{
//	logerror("a2 softswitch_r: %04x\n", offset + 0xc010);
	switch (offset)
	{
		case 0x00:			return AY3600_anykey_clearstrobe_r();
		case 0x01:			return a2.LC_RAM2;
		case 0x02:			return a2.LC_RAM;
		case 0x03:			return a2.RAMRD;
		case 0x04:			return a2.RAMWRT;
		case 0x05:			return a2.INTCXROM;
		case 0x06:			return a2.ALTZP;
		case 0x07:			return a2.SLOTC3ROM;
		case 0x08:			return a2.STORE80;
		case 0x09:			return input_port_0_r(0);	/* RDVBLBAR */
		case 0x0A:			return a2.TEXT;
		case 0x0B:			return a2.MIXED;
		case 0x0C:			return a2.PAGE2;
		case 0x0D:			return a2.HIRES;
		case 0x0E:			return a2.ALTCHARSET;
		case 0x0F:			return a2.COL80;
	}

	return 0;
}

/***************************************************************************
  apple2_c01x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c01x_w )
{
	/* Clear the keyboard strobe - ignore the returned results */
	AY3600_anykey_clearstrobe_r();
}

/***************************************************************************
  apple2_c02x_r
***************************************************************************/
READ_HANDLER ( apple2_c02x_r )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c02x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c02x_w )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c03x_r
***************************************************************************/
READ_HANDLER ( apple2_c03x_r )
{
	switch (offset)
	{
		case 0x00:
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	if (a2_speaker_state==0xFF)
		a2_speaker_state=0;
	else
		a2_speaker_state=0xFF;
	DAC_data_w(0,a2_speaker_state);

	return 0;
}

/***************************************************************************
  apple2_c03x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c03x_w )
{
	switch (offset)
	{
		case 0x00:
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			if (a2_speaker_state==0xFF)
				a2_speaker_state=0;
			else
				a2_speaker_state=0xFF;
			DAC_data_w(0,a2_speaker_state);
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
	if (a2_speaker_state==0xFF)
		a2_speaker_state=0;
	else
		a2_speaker_state=0xFF;
	DAC_data_w(0,a2_speaker_state);
	if (a2_speaker_state==0xFF)
		a2_speaker_state=0;
	else
		a2_speaker_state=0xFF;
	DAC_data_w(0,a2_speaker_state);
}

/***************************************************************************
  apple2_c04x_r
***************************************************************************/
READ_HANDLER ( apple2_c04x_r )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c04x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c04x_w )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c05x_r
***************************************************************************/
READ_HANDLER ( apple2_c05x_r )
{
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
		/* Joystick/paddle pots */
		case 0x08:		a2.AN0   = 0x80; break;	/* AN0 has reverse SET logic */
		case 0x09:		a2.AN0   = 0x00; break;
		case 0x0A:		a2.AN1   = 0x80; break; /* AN1 has reverse SET logic */
		case 0x0B:		a2.AN1   = 0x00; break;
		case 0x0C:		a2.AN2   = 0x80; break; /* AN2 has reverse SET logic */
		case 0x0D:		a2.AN2   = 0x00; break;
		case 0x0E:		a2.AN3   = 0x80; break; /* AN3 has reverse SET logic */
		case 0x0F:		a2.AN3   = 0x00; break;
	}

	return 0;
}

/***************************************************************************
  apple2_c05x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c05x_w )
{
	switch (offset)
	{
		case 0x00:		a2.TEXT  = 0x00; break;
		case 0x01:		a2.TEXT  = 0x80; break;
		case 0x02:		a2.MIXED = 0x00; break;
		case 0x03:		a2.MIXED = 0x80; break;
		case 0x04:		a2.PAGE2 = 0x00; break;
		case 0x05:		a2.PAGE2 = 0x80; break;
		case 0x06:		a2.HIRES = 0x00; break;
		case 0x07:		a2.HIRES = 0x80; break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c06x_r
***************************************************************************/
READ_HANDLER ( apple2_c06x_r )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			/* Open-Apple/Joystick button 0 */
			if (input_port_1_r(0) & 0x02)
				return 0x80;
			else return 0x00;
			break;
		case 0x02:
			/* Closed-Apple/Joystick button 1 */
			if (input_port_1_r(0) & 0x04)
				return 0x80;
			else return 0x00;
			break;
		case 0x03:
			/* Joystick button 2. Later revision motherboards connected this to SHIFT also */
			if (input_port_1_r(0) & 0x08)
				return 0x80;
			else return 0x00;
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c06x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c06x_w )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c07x_r
***************************************************************************/
READ_HANDLER ( apple2_c07x_r )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c07x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c07x_w )
{
	switch (offset)
	{
		case 0x00:
			break;
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		case 0x05:
			break;
		case 0x06:
			break;
		case 0x07:
			break;
		case 0x08:
			break;
		case 0x09:
			break;
		case 0x0A:
			break;
		case 0x0B:
			break;
		case 0x0C:
			break;
		case 0x0D:
			break;
		case 0x0E:
			break;
		case 0x0F:
			break;
	}
}

/***************************************************************************
  apple2_c08x_r
***************************************************************************/
READ_HANDLER ( apple2_c08x_r )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	/* If the aux switch is set, use the aux language card bank as well */
	int aux_offset = a2.ALTZP ? 0x10000 : 0x0000;

	logerror("language card bankswitch read, offset: $c08%0x\n", offset);

	if ((offset & 0x01)==0x00)
	{
		memory_set_bankhandler_w (1, 0, MWA_ROM);
		memory_set_bankhandler_w (2, 0, MWA_ROM);
		a2.LC_WRITE = 0x00;
	}
	else
	{
		memory_set_bankhandler_w (2, 0, apple2_LC_ram_w);

		if ((offset & 0x08)==0x00)
			memory_set_bankhandler_w (1, 0, apple2_LC_ram2_w);
		else
			memory_set_bankhandler_w (1, 0, apple2_LC_ram1_w);
		a2.LC_WRITE = 0x80;
	}

	switch (offset & 0x03)
	{
		case 0x00:
		case 0x03:
			cpu_setbank (2, &RAM[0xe000 + aux_offset]);
			a2.LC_RAM = 0x80;
			if ((offset & 0x08)==0x00)
			{
				cpu_setbank (1, &RAM[0xd000 + aux_offset]);
				a2.LC_RAM2 = 0x80;
			}
			else
			{
				cpu_setbank (1, &RAM[0xc000 + aux_offset]);
				a2.LC_RAM2 = 0x00;
			}
			break;
		case 0x01:
		case 0x02:
			cpu_setbank (1, &RAM[0x21000]);
			cpu_setbank (2, &RAM[0x22000]);
			a2.LC_RAM = a2.LC_RAM2 = 0;
			break;
	}

	return 0;
}

/***************************************************************************
  apple2_c08x_w
***************************************************************************/
WRITE_HANDLER ( apple2_c08x_w )
{
	/* same as reading */
	logerror("write -- ");
	apple2_c08x_r (offset);
}

/***************************************************************************
  apple2_c0xx_slot1_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot1_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot2_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot2_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot3_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot3_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot4_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot4_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot5_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot5_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot7_r
***************************************************************************/
READ_HANDLER ( apple2_c0xx_slot7_r )
{
	return 0;
}

/***************************************************************************
  apple2_c0xx_slot1_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot1_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot2_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot2_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot3_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot3_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot4_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot4_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot5_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot5_w )
{
	return;
}

/***************************************************************************
  apple2_c0xx_slot7_w
***************************************************************************/
WRITE_HANDLER ( apple2_c0xx_slot7_w )
{
	return;
}

/***************************************************************************
  apple2_slot1_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot1_w )
{
	return;
}

/***************************************************************************
  apple2_slot2_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot2_w )
{
	return;
}

/***************************************************************************
  apple2_slot3_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot3_w )
{
	return;
}

/***************************************************************************
  apple2_slot4_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot4_w )
{
	mockingboard_w (offset, data);
}

/***************************************************************************
  apple2_slot5_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot5_w )
{
	return;
}

/***************************************************************************
  apple2_slot7_w
***************************************************************************/
WRITE_HANDLER ( apple2_slot7_w )
{
	return;
}

READ_HANDLER ( apple2_slot4_r )
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	if (a2.INTCXROM)
		/* Read the built-in ROM */
		return RAM[0x20400 + offset];
	else
		/* Read the slot ROM */
		return mockingboard_r (offset);
}

static void mockingboard_init (int slot)
{
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* TODO: fix this */
	/* What follows is pure filth. It abuses the core like an angry pimp on a bad hair day. */

	/* Since we know that the Mockingboard has no code ROM, we'll copy into the slot ROM space
	   an image of the onboard ROM so that when an IRQ bankswitches to the onboard ROM, we read
	   the proper stuff. Without this, it will choke and try to use the memory handler above, and
	   fail miserably. That should really be fixed. I beg you -- if you are reading this comment,
	   fix this :) */
	memcpy (&RAM[0x24000 + (slot-1) * 0x100], &RAM[0x20000 + (slot * 0x100)], 0x100);
}

static int mockingboard_r (int offset)
{
	static int flip1 = 0, flip2 = 0;

	switch (offset)
	{
		/* This is used to ID the board */
		case 0x04:
			flip1 ^= 0x08;
			return flip1;
			break;
		case 0x84:
			flip2 ^= 0x08;
			return flip2;
			break;
		default:
//			logerror("mockingboard_r unmapped, offset: %02x, pc: %04x\n", offset, cpu_getpc());
			break;
	}
	return 0x00;
}

static void mockingboard_w (int offset, int data)
{
	static int latch0, latch1;

	logerror("mockingboard_w, $%02x:%02x\n", offset, data);

	/* There is a 6522 in here which interfaces to the 8910s */
	switch (offset)
	{
		case 0x00: /* ORB1 */
			switch (data)
			{
				case 0x00: /* reset */
					AY8910_reset (0);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_0_w (0, latch0);
					break;
				case 0x07: /* set register */
					AY8910_control_port_0_w (0, latch0);
					break;
			}
			break;

		case 0x01: /* ORA1 */
			latch0 = data;
			break;

		case 0x02: /* DDRB1 */
		case 0x03: /* DDRA1 */
			break;

		case 0x80: /* ORB2 */
			switch (data)
			{
				case 0x00: /* reset */
					AY8910_reset (1);
					break;
				case 0x04: /* make inactive */
					break;
				case 0x06: /* write data */
					AY8910_write_port_1_w (0, latch1);
					break;
				case 0x07: /* set register */
					AY8910_control_port_1_w (0, latch1);
					break;
			}
			break;

		case 0x81: /* ORA2 */
			latch1 = data;
			break;

		case 0x82: /* DDRB2 */
		case 0x83: /* DDRA2 */
			break;
	}
}

