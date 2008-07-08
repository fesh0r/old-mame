#include "driver.h"
#include "deprecat.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1350.h"

static UINT8 outa,outb;

static int power=1; /* simulates pressed cce when mess is started */

void pc1350_outa(int data)
{
	outa=data;
}

void pc1350_outb(int data)
{
	outb=data;
}

void pc1350_outc(int data)
{

}

int pc1350_ina(void)
{
	running_machine *machine = Machine;
	int data = outa;
	int t = pc1350_keyboard_line_r();

	if (t & 0x01)
		data |= input_port_read(machine, "KEY0");

	if (t & 0x02)
		data |= input_port_read(machine, "KEY1");

	if (t & 0x04)
		data |= input_port_read(machine, "KEY2");

	if (t & 0x08)
		data |= input_port_read(machine, "KEY3");

	if (t & 0x10)
		data |= input_port_read(machine, "KEY4");

	if (t & 0x20)
		data |= input_port_read(machine, "KEY5");

	if (outa & 0x01)
		data |= input_port_read(machine, "KEY6");

	if (outa & 0x02)
		data |= input_port_read(machine, "KEY7");

	if (outa & 0x04)
	{
		data |= input_port_read(machine, "KEY8");
	
		/* At Power Up we fake a 'CLS' pressure */
		if (power)
			data |= 0x08;
	}

	if (outa & 0x08)
		data |= input_port_read(machine, "KEY9");

	if (outa & 0x10)
		data |= input_port_read(machine, "KEY10");

	if (outa & 0xc0) 
		data |= input_port_read(machine, "KEY11");

	// missing lshift

	return data;
}

int pc1350_inb(void)
{
	int data=outb;
	return data;
}

int pc1350_brk(void)
{
	running_machine *machine = Machine;
	return (input_port_read(machine, "EXTRA") & 0x01);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1350 )
{
	UINT8 *ram=memory_region(machine, REGION_CPU1)+0x2000,
		*cpu=sc61860_internal_ram();

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x5000);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x5000);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x5000);
	}
}

static TIMER_CALLBACK(pc1350_power_up)
{
	power=0;
}

MACHINE_START( pc1350 )
{
	timer_set(ATTOTIME_IN_SEC(1), NULL, 0, pc1350_power_up);

	memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6fff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, &mess_ram[0x0000]);

	if (mess_ram_size >= 0x3000)
	{
		memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_BANK2);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_BANK2);
		memory_set_bankptr(2, &mess_ram[0x1000]);
	}
	else
	{
		memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, SMH_NOP);
	}

	if (mess_ram_size >= 0x5000)
	{
		memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, SMH_BANK3);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, SMH_BANK3);
		memory_set_bankptr(3, &mess_ram[0x3000]);
	}
	else
	{
		memory_install_read8_handler(machine, 0,  ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, SMH_NOP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, SMH_NOP);
	}
}
