#include "driver.h"
#include "vidhrdw/stic.h"
#include "includes/intv.h"
#include "cpu/cp1600/cp1600.h"

static unsigned int intvkbd_dualport_ram[0x4000];

READ16_HANDLER ( intvkbd_dualport16_r )
{
	//if (offset == 0x6c)
		//logerror("---read() = %04x\n",intvkbd_dualport_ram[offset]);
	return intvkbd_dualport_ram[offset];
}

WRITE16_HANDLER ( intvkbd_dualport16_w )
{
	unsigned char *RAM;

	//if (offset == 0x6c)
		//logerror("---write() = %04x\n",data);
	intvkbd_dualport_ram[offset] = data;

	/* copy the LSB over to the 6502 OP RAM, in case they are opcodes */
	RAM	 = memory_region(REGION_CPU2);
	RAM[offset] = data&0xff;
}

READ_HANDLER ( intvkbd_dualport8_lsb_r )
{
	//if (offset == 0x6c)
		//logerror("---lsb_r() = %02x\n",intvkbd_dualport_ram[offset]&0xff);
	unsigned char *RAM;

	RAM	 = memory_region(REGION_CPU2);
	return RAM[offset]&0xff;
	//return intvkbd_dualport_ram[offset]&0xff;
}

WRITE_HANDLER ( intvkbd_dualport8_lsb_w )
{
	UINT16 mask;
	unsigned char *RAM;

	//if (offset == 0x6c)
		//logerror("---lsb_w() = %02x\n",data);
	mask = intvkbd_dualport_ram[offset] & 0xff00;
	intvkbd_dualport_ram[offset] = mask | data;

	/* copy over to the 6502 OP RAM, in case they are opcodes */
	RAM	 = memory_region(REGION_CPU2);
	RAM[offset] = data;
}

static int intvkbd_keyboard_col;
static int tape_int_pending;
static int sr1_int_pending;

int intvkbd_text_blanked;

READ_HANDLER ( intvkbd_dualport8_msb_r )
{
	unsigned char rv;

	if (offset < 0x100)
	{
		switch (offset)
		{
			case 0x000:
				rv = input_port_10_r(0) & 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - XOR Data?\n",rv,offset);
				break;
			case 0x001:
				rv = (input_port_10_r(0) & 0x40) << 1;
				logerror("TAPE: Read %02x from 0x40%02x - Sense 1?\n",rv,offset);
				break;
			case 0x002:
				rv = (input_port_10_r(0) & 0x20) << 2;
				logerror("TAPE: Read %02x from 0x40%02x - Sense 2?\n",rv,offset);
				break;
			case 0x003:
				rv = (input_port_10_r(0) & 0x10) << 3;
				logerror("TAPE: Read %02x from 0x40%02x - Tape Present\n",rv,offset);
				break;
			case 0x004:
				rv = (input_port_10_r(0) & 0x08) << 4;
				logerror("TAPE: Read %02x from 0x40%02x - Comp (339/1)\n",rv,offset);
				break;
			case 0x005:
				rv = (input_port_10_r(0) & 0x04) << 5;
				logerror("TAPE: Read %02x from 0x40%02x - Clocked Comp (339/13)\n",rv,offset);
				break;
			case 0x006:
				if (sr1_int_pending)
					rv = 0x00;
				else
					rv = 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - SR1 Int Pending\n",rv,offset);
				break;
			case 0x007:
				if (tape_int_pending)
					rv = 0x00;
				else
					rv = 0x80;
				logerror("TAPE: Read %02x from 0x40%02x - Tape? Int Pending\n",rv,offset);
				break;
			case 0x060:	/* Keyboard Read */
				rv = 0xff;
				if (intvkbd_keyboard_col == 0)
					rv = input_port_0_r(0);
				if (intvkbd_keyboard_col == 1)
					rv = input_port_1_r(0);
				if (intvkbd_keyboard_col == 2)
					rv = input_port_2_r(0);
				if (intvkbd_keyboard_col == 3)
					rv = input_port_3_r(0);
				if (intvkbd_keyboard_col == 4)
					rv = input_port_4_r(0);
				if (intvkbd_keyboard_col == 5)
					rv = input_port_5_r(0);
				if (intvkbd_keyboard_col == 6)
					rv = input_port_6_r(0);
				if (intvkbd_keyboard_col == 7)
					rv = input_port_7_r(0);
				if (intvkbd_keyboard_col == 8)
					rv = input_port_8_r(0);
				if (intvkbd_keyboard_col == 9)
					rv = input_port_9_r(0);
				break;
			case 0x80:
				rv = 0x00;
				logerror("TAPE: Read %02x from 0x40%02x, clear tape int pending\n",rv,offset);
				tape_int_pending = 0;
				break;
			case 0xa0:
				rv = 0x00;
				logerror("TAPE: Read %02x from 0x40%02x, clear SR1 int pending\n",rv,offset);
				sr1_int_pending = 0;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0xce:
			case 0xcf:
				/* TMS9927 regs */
				rv = intvkbd_tms9927_r(offset-0xc0);
				break;
			default:
				rv = (intvkbd_dualport_ram[offset]&0x0300)>>8;
				logerror("Unknown read %02x from 0x40%02x\n",rv,offset);
				break;
		}
		return rv;
	}
	else
		return (intvkbd_dualport_ram[offset]&0x0300)>>8;
}

static int tape_interrupts_enabled;
static int tape_unknown_write[6];
static int tape_motor_mode;
static char *tape_motor_mode_desc[8] =
{
	"IDLE", "IDLE", "IDLE", "IDLE",
	"EJECT", "PLAY/RECORD", "REWIND", "FF"
};

WRITE_HANDLER ( intvkbd_dualport8_msb_w )
{
	unsigned int mask;

	if (offset < 0x100)
	{
		switch (offset)
		{
			case 0x020:
				tape_motor_mode &= 3;
				if (data & 1)
					tape_motor_mode |= 4;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[tape_motor_mode]);
				break;
			case 0x021:
				tape_motor_mode &= 5;
				if (data & 1)
					tape_motor_mode |= 2;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[tape_motor_mode]);
				break;
			case 0x022:
				tape_motor_mode &= 6;
				if (data & 1)
					tape_motor_mode |= 1;
				logerror("TAPE: Motor Mode: %s\n",tape_motor_mode_desc[tape_motor_mode]);
				break;
			case 0x023:
			case 0x024:
			case 0x025:
			case 0x026:
			case 0x027:
				tape_unknown_write[offset - 0x23] = (data & 1);
				break;
			case 0x040:
				tape_unknown_write[5] = (data & 1);
				break;
			case 0x041:
				if (data & 1)
					logerror("TAPE: Tape Interrupts Enabled\n",data,offset);
				else
					logerror("TAPE: Tape Interrupts Disabled\n",data,offset);
				tape_interrupts_enabled = (data & 1);
				break;
			case 0x042:
				if (data & 1)
					logerror("TAPE: Cart Bus Interrupts Disabled\n",data,offset);
				else
					logerror("TAPE: Cart Bus Interrupts Enabled\n",data,offset);
				break;
			case 0x043:
				if (data & 0x01)
					intvkbd_text_blanked = 0;
				else
					intvkbd_text_blanked = 1;
				break;
			case 0x044:
				intvkbd_keyboard_col &= 0x0e;
				intvkbd_keyboard_col |= (data&0x01);
				break;
			case 0x045:
				intvkbd_keyboard_col &= 0x0d;
				intvkbd_keyboard_col |= ((data&0x01)<<1);
				break;
			case 0x046:
				intvkbd_keyboard_col &= 0x0b;
				intvkbd_keyboard_col |= ((data&0x01)<<2);
				break;
			case 0x047:
				intvkbd_keyboard_col &= 0x07;
				intvkbd_keyboard_col |= ((data&0x01)<<3);
				break;
			case 0x80:
				logerror("TAPE: Write to 0x40%02x, clear tape int pending\n",offset);
				tape_int_pending = 0;
				break;
			case 0xa0:
				logerror("TAPE: Write to 0x40%02x, clear SR1 int pending\n",offset);
				sr1_int_pending = 0;
				break;
			case 0xc0:
			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
			case 0xc5:
			case 0xc6:
			case 0xc7:
			case 0xc8:
			case 0xc9:
			case 0xca:
			case 0xcb:
			case 0xcc:
			case 0xcd:
			case 0xce:
			case 0xcf:
				/* TMS9927 regs */
				intvkbd_tms9927_w(offset-0xc0, data);
				break;
			default:
				logerror("%04X: Unknown write %02x to 0x40%02x\n",cpu_get_pc(),data,offset);
				break;
		}
	}
	else
	{
		mask = intvkbd_dualport_ram[offset] & 0x00ff;
		intvkbd_dualport_ram[offset] = mask | ((data<<8)&0x0300);
	}
}

UINT8 intv_gram[512];
UINT8 intv_gramdirty[64];

READ16_HANDLER( intv_gram_r )
{
	//logerror("read: %d = GRAM(%d)\n",intv_gram[offset],offset);
	return (int)intv_gram[offset];
}

WRITE16_HANDLER( intv_gram_w )
{
	intv_gram[offset]=data&0xff;
	intv_gramdirty[offset>>3] = 1;
	//logerror("write: GRAM(%d) = %d\n",offset,data);
}

static unsigned char intv_ram8[256];

READ16_HANDLER( intv_ram8_r )
{
	//logerror("%x = ram8_r(%x)\n",intv_ram8[offset],offset);
	return (int)intv_ram8[offset];
}

WRITE16_HANDLER( intv_ram8_w )
{
	//logerror("ram8_w(%x) = %x\n",offset,data);
	intv_ram8[offset] = data&0xff;
}

UINT16 intv_ram16[0x160];

READ16_HANDLER( intv_ram16_r )
{
	//logerror("%x = ram16_r(%x)\n",intv_ram16[offset],offset);
	return (int)intv_ram16[offset];
}

WRITE16_HANDLER( intv_ram16_w )
{
	//logerror("%g: WRITING TO GRAM offset = %d\n",timer_get_time(),offset);
	//logerror("ram16_w(%x) = %x\n",offset,data);
	intv_ram16[offset] = data&0xffff;
}

int intv_load_rom_file(int id, int required)
{
	const char *rom_name = device_filename(IO_CARTSLOT,id);
    FILE *romfile;
    int i;

	UINT8 temp;
	UINT8 num_segments;
	UINT8 start_seg;
	UINT8 end_seg;

	UINT16 current_address;
	UINT16 end_address;

	UINT8 high_byte;
	UINT8 low_byte;

	UINT8 *memory = memory_region(REGION_CPU1);

	if(!rom_name)
	{
		if (required)
		{
			printf("intv requires cartridge!\n");
			return INIT_FAIL;
		}
		else
			printf("intvkbd legacy cartridge slot empty - ok\n");
	}

	if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
	{
		return INIT_FAIL;
	}

	osd_fread(romfile,&temp,1);			/* header */
	if (temp != 0xa8)
	{
		return INIT_FAIL;
	}

	osd_fread(romfile,&num_segments,1);

	osd_fread(romfile,&temp,1);
	if (temp != (num_segments ^ 0xff))
	{
		return INIT_FAIL;
	}

	for(i=0;i<num_segments;i++)
	{
		osd_fread(romfile,&start_seg,1);
		current_address = start_seg*0x100;

		osd_fread(romfile,&end_seg,1);
		end_address = end_seg*0x100 + 0xff;

		while(current_address <= end_address)
		{
			osd_fread(romfile,&low_byte,1);
			memory[(current_address<<1)+1] = low_byte;
			osd_fread(romfile,&high_byte,1);
			memory[current_address<<1] = high_byte;
			current_address++;
		}

		osd_fread(romfile,&temp,1);
		osd_fread(romfile,&temp,1);
	}

	for(i=0;i<(16+32+2);i++)
	{
		osd_fread(romfile,&temp,1);
	}

	osd_fclose(romfile);

	return INIT_PASS;
}

int intv_load_rom(int id)
{
	/* First, initialize these as empty so that the intellivision
	 * will think that the playcable and keyboard are not attached */
	UINT8 *memory = memory_region(REGION_CPU1);

	/* assume playcable is absent */
	memory[0x4800<<1] = 0xff;
	memory[(0x4800<<1)+1] = 0xff;

	/* assume keyboard is absent */
	memory[0x7000<<1] = 0xff;
	memory[(0x7000<<1)+1] = 0xff;

	return intv_load_rom_file(id, 1);
}

/* Set Reset and INTR/INTRM Vector */
void init_intv(void)
{
}

void intv_machine_init(void)
{
	cpu_irq_line_vector_w(0, CP1600_RESET, 0x1000);

	/* These are actually the same vector, and INTR is unused */
	cpu_irq_line_vector_w(0, CP1600_INT_INTRM, 0x1004);
	cpu_irq_line_vector_w(0, CP1600_INT_INTR,  0x1004);

	return;
}


void intv_interrupt_complete(int x)
{
	cpu_set_irq_line(0, CP1600_INT_INTRM, CLEAR_LINE);
}

int intv_interrupt(void)
{
	cpu_set_irq_line(0, CP1600_INT_INTRM, ASSERT_LINE);
	sr1_int_pending = 1;
	timer_set(TIME_NOW+TIME_IN_CYCLES(3791, 0), 0, intv_interrupt_complete);
	stic_screenrefresh();
	return 0;
}

static UINT8 controller_table[] =
{
	0x81, 0x41, 0x21, 0x82, 0x42, 0x22, 0x84, 0x44,
	0x24, 0x88, 0x48, 0x28, 0xa0, 0x60, 0xc0, 0x00,
	0x04, 0x16, 0x02, 0x13, 0x01, 0x19, 0x08, 0x1c
};

READ_HANDLER( intv_right_control_r )
{
	UINT8 rv = 0x00;

	int bit, byte;
	int value=0;

	for(byte=0; byte<3; byte++)
	{
		switch(byte)
		{
			case 0:
				value = input_port_0_r(0);
				break;
			case 1:
				value = input_port_1_r(0);
				break;
			case 2:
				value = input_port_2_r(0);
				break;
		}
		for(bit=7; bit>=0; bit--)
		{
			if (value & (1<<bit))
			{
				rv |= controller_table[byte*8+(7-bit)];
			}
		}
	}
	return rv ^ 0xff;
}

READ_HANDLER( intv_left_control_r )
{
	return 0xff;
}

/* Intellivision console + keyboard component */

void init_intvkbd(void)
{
}

int intvkbd_load_rom (int id)
{
	if (id == 0) /* Legacy cartridge slot */
	{
		/* First, initialize these as empty so that the intellivision
		 * will think that the playcable is not attached */
		UINT8 *memory = memory_region(REGION_CPU1);

		/* assume playcable is absent */
		memory[0x4800<<1] = 0xff;
		memory[(0x4800<<1)+1] = 0xff;

		intv_load_rom_file(id, 0);
	}

	if (id == 1) /* Keyboard component cartridge slot */
	{
		const char *rom_name = device_filename(IO_CARTSLOT,id);
    	FILE *romfile;

		UINT8 *memory = memory_region(REGION_CPU2);

		if(!rom_name)
		{
			printf("intvkbd cartridge slot empty - ok\n");
			return INIT_PASS;
		}

		if (!(romfile = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
		{
			return INIT_FAIL;
		}

		/* Assume an 8K cart, like BASIC */
		osd_fread(romfile,&memory[0xe000],0x2000);

		osd_fclose(romfile);
	}

	return INIT_PASS;

}
