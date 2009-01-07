#include "driver.h"
#include "cpu/sc61860/sc61860.h"

#include "includes/pocketc.h"
#include "includes/pc1251.h"

/* C-CE while reset, program will not be destroyed! */

static UINT8 outa,outb;

static int power=1; /* simulates pressed cce when mess is started */

void pc1251_outa(const device_config *device, int data)
{
	outa=data;
}

void pc1251_outb(const device_config *device, int data)
{
	outb=data;
}

void pc1251_outc(const device_config *device, int data)
{

}

int pc1251_ina(const device_config *device)
{
	int data = outa;
	running_machine *machine = device->machine;

	if (outb & 0x01)
	{
		data |= input_port_read(machine, "KEY0");

		/* At Power Up we fake a 'CL' pressure */
		if (power)
			data |= 0x02;		// problem with the deg lcd
	}

	if (outb & 0x02)
		data |= input_port_read(machine, "KEY1");

	if (outb & 0x04)
		data |= input_port_read(machine, "KEY2");

	if (outa & 0x01)
		data |= input_port_read(machine, "KEY3");

	if (outa & 0x02)
		data |= input_port_read(machine, "KEY4");

	if (outa & 0x04)
		data |= input_port_read(machine, "KEY5");

	if (outa & 0x08)
		data |= input_port_read(machine, "KEY6");

	if (outa & 0x10)
		data |= input_port_read(machine, "KEY7");

	if (outa & 0x20)
		data |= input_port_read(machine, "KEY8");

	if (outa & 0x40)
		data |= input_port_read(machine, "KEY9");

	return data;
}

int pc1251_inb(const device_config *device)
{
	int data = outb;

	if (outb & 0x08) 
		data |= (input_port_read(device->machine, "MODE") & 0x07);

	return data;
}

int pc1251_brk(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x01);
}

int pc1251_reset(const device_config *device)
{
	return (input_port_read(device->machine, "EXTRA") & 0x02);
}

/* currently enough to save the external ram */
NVRAM_HANDLER( pc1251 )
{
	const device_config *main_cpu = cputag_get_cpu(machine, "main");
	UINT8 *ram = memory_region(machine, "main") + 0x8000;
	UINT8 *cpu = sc61860_internal_ram(main_cpu);

	if (read_or_write)
	{
		mame_fwrite(file, cpu, 96);
		mame_fwrite(file, ram, 0x4800);
	}
	else if (file)
	{
		mame_fread(file, cpu, 96);
		mame_fread(file, ram, 0x4800);
	}
	else
	{
		memset(cpu, 0, 96);
		memset(ram, 0, 0x4800);
	}
}

static TIMER_CALLBACK(pc1251_power_up)
{
	power = 0;
}

DRIVER_INIT( pc1251 )
{
	int i;
	const address_space *space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	UINT8 *gfx = memory_region(machine, "gfx1");
	for (i=0; i<128; i++) gfx[i]=i;

	timer_set(machine, ATTOTIME_IN_SEC(1), NULL, 0, pc1251_power_up);

	// c600 b800 b000 a000 8000 tested
	// 4 kb memory feedback 512 bytes too few???
	// 11 kb ram: program stored at 8000
#if 1
	memory_install_write8_handler(space, 0x8000, 0xc7ff, 0, 0, SMH_BANK1);
	memory_set_bankptr(machine, 1, memory_region(machine, "main") + 0x8000);
#else
	if ((input_port_read(machine, "DSW0") & 0xc0) == 0xc0)
	{
		memory_install_write8_handler(space, 0x8000, 0xafff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0xb000, 0xc5ff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xc600, 0xc7ff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0xc800, 0xf7ff, 0, 0, SMH_RAM);
	}
	else if ((input_port_read(machine, "DSW0") & 0xc0) == 0x80)
	{
		memory_install_write8_handler(space, 0xa000, 0xafff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xb000, 0xc7ff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0xc800, 0xcbff, 0, 0, SMH_RAM);
	}
	else if ((input_port_read(machine, "DSW0") & 0xc0) == 0x40)
	{
		memory_install_write8_handler(space, 0xa000, 0xb7ff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xb800, 0xc7ff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0xc800, 0xcbff, 0, 0, SMH_RAM);
	}
	else
	{
		memory_install_write8_handler(space, 0xa000, 0xc5ff, 0, 0, SMH_NOP);
		memory_install_write8_handler(space, 0xc600, 0xc9ff, 0, 0, SMH_RAM);
		memory_install_write8_handler(space, 0xca00, 0xcbff, 0, 0, SMH_RAM);
	}
#endif
}

