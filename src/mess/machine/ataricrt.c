/***************************************************************************

	MESS specific Atari init and Cartridge code for Atari 8 bit systems

***************************************************************************/

#include "driver.h"
#include "includes/atari.h"
#include "ataridev.h"

static int a800_cart_loaded = 0;
static int a800_cart_is_16k = 0;
static int atari = 0;

/*************************************
 *
 *  Generic code
 *
 *************************************/

/* 2009-04 FP: is this used anywhere? */
DRIVER_INIT( atari )
{
	offs_t ram_top;
	offs_t ram_size;

	if (!strcmp(machine->gamedrv->name, "a5200")
		|| !strcmp(machine->gamedrv->name, "a600xl"))
	{
		ram_size = 0x8000;
	}
	else
	{
		ram_size = 0xa000;
	}

	/* install RAM */
	ram_top = MIN(mess_ram_size, ram_size) - 1;
	memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_set_bankptr(machine, 2, mess_ram);
}


static void a800_setbank(running_machine *machine, int n)
{
	void *read_addr;
	void *write_addr;
	UINT8 *mem = memory_region(machine, "maincpu");

	switch (n)
	{
		case 1:
			read_addr = &mem[0x10000];
			write_addr = NULL;
			break;
		default:
			if( atari <= ATARI_400 )
			{
				/* Atari 400 has no RAM here, so install the NOP handler */
				read_addr = NULL;
				write_addr = NULL;
			}
			else
			{
				read_addr = &mess_ram[0x08000];
				write_addr = &mess_ram[0x08000];
			}
			break;
	}

	memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0,
		read_addr ? SMH_BANK(1) : SMH_NOP);
	memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM), 0x8000, 0xbfff, 0, 0,
		write_addr ? SMH_BANK(1) : SMH_NOP);
	if (read_addr)
		memory_set_bankptr(machine, 1, read_addr);
	if (write_addr)
		memory_set_bankptr(machine, 1, write_addr);
}


static void cart_reset(running_machine *machine)
{
	if (a800_cart_loaded)
		a800_setbank(machine, 1);
}

/* MESS specific parts that have to be started */
static void ms_atari_machine_start(running_machine *machine, int type, int has_cart)
{
	offs_t ram_top;
	offs_t ram_size;
	
	/* set atari type (needed for banks above) */
	atari = type;

	/* determine RAM */
	if (!strcmp(machine->gamedrv->name, "a400")
		|| !strcmp(machine->gamedrv->name, "a400pal")
		|| !strcmp(machine->gamedrv->name, "a800")
		|| !strcmp(machine->gamedrv->name, "a800pal")
		|| !strcmp(machine->gamedrv->name, "a800xl"))
	{
		ram_size = 0xA000;
	}
	else
	{
		ram_size = 0x8000;
	}

	/* install RAM */
	ram_top = MIN(mess_ram_size, ram_size) - 1;
	memory_install_read8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_install_write8_handler(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),
		0x0000, ram_top, 0, 0, SMH_BANK(2));
	memory_set_bankptr(machine, 2, mess_ram);

	/* cartridge */
	if (has_cart)
		add_reset_callback(machine, cart_reset);
}


/*************************************
 *
 *  Atari 400
 *
 *************************************/

MACHINE_START( a400 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_400, TRUE);
}


/*************************************
 *
 *  Atari 800
 *
 *************************************/

MACHINE_START( a800 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_800, TRUE);
}


DEVICE_IMAGE_LOAD( a800_cart )
{
	UINT8 *mem = memory_region(image->machine, "maincpu");
	int size;

	/* load an optional (dual) cartridge (e.g. basic.rom) */
	if( strcmp(image->tag,"cart2") == 0 )
	{
		size = image_fread(image, &mem[0x12000], 0x2000);
		a800_cart_is_16k = (size == 0x2000);
		logerror("%s loaded right cartridge '%s' size 16K\n", image->machine->gamedrv->name, image_filename(image) );
	}
	else
	{
		size = image_fread(image, &mem[0x10000], 0x2000);
		a800_cart_loaded = size > 0x0000;
		size = image_fread(image, &mem[0x12000], 0x2000);
		a800_cart_is_16k = size > 0x2000;
		logerror("%s loaded left cartridge '%s' size %s\n", image->machine->gamedrv->name, image_filename(image) , (a800_cart_is_16k) ? "16K":"8K");
	}
	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( a800_cart )
{
	if( strcmp(image->tag,"cart2") == 0 )
	{
		a800_cart_is_16k = 0;
		a800_setbank(image->machine, 1);
    }
	else
	{
		a800_cart_loaded = 0;
		a800_setbank(image->machine, 0);
    }
}


/*************************************
 *
 *  Atari 800XL
 *
 *************************************/

MACHINE_START( a800xl )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_800XL, TRUE);
}

DEVICE_IMAGE_LOAD( a800xl_cart )
{
	UINT8 *mem = memory_region(image->machine, "maincpu");
	astring *fname;
	mame_file *basic_fp;
	file_error filerr;
	unsigned size;

	fname = astring_assemble_3(astring_alloc(), image->machine->gamedrv->name, PATH_SEPARATOR, "basic.rom");
	filerr = mame_fopen(SEARCHPATH_ROM, astring_c(fname), OPEN_FLAG_READ, &basic_fp);
	astring_free(fname);

	if (filerr != FILERR_NONE)
	{
		size = mame_fread(basic_fp, &mem[0x14000], 0x2000);
		if( size < 0x2000 )
		{
			logerror("%s image '%s' load failed (less than 8K)\n", image->machine->gamedrv->name, astring_c(fname));
			mame_fclose(basic_fp);
			return 2;
		}
	}

	/* load an optional (dual) cartidge (e.g. basic.rom) */
	if (filerr != FILERR_NONE)
	{
		{
			size = image_fread(image, &mem[0x14000], 0x2000);
			a800_cart_loaded = size / 0x2000;
			size = image_fread(image, &mem[0x16000], 0x2000);
			a800_cart_is_16k = size / 0x2000;
			logerror("%s loaded cartridge '%s' size %s\n",
					image->machine->gamedrv->name, image_filename(image), (a800_cart_is_16k) ? "16K":"8K");
		}
		mame_fclose(basic_fp);
	}

	return INIT_PASS;
}


/*************************************
 *
 *  Atari 5200 console
 *
 *************************************/

MACHINE_START( a5200 )
{
	atari_machine_start(machine);
	ms_atari_machine_start(machine, ATARI_800XL, TRUE);
}


DEVICE_IMAGE_LOAD( a5200_cart )
{
	UINT8 *mem = memory_region(image->machine, "maincpu");
	int size;

	/* load an optional (dual) cartidge */
	size = image_fread(image, &mem[0x4000], 0x8000);
	if (size<0x8000) memmove(mem+0x4000+0x8000-size, mem+0x4000, size);
	// mirroring of smaller cartridges
	if (size <= 0x1000) memcpy(mem+0xa000, mem+0xb000, 0x1000);
	if (size <= 0x2000) memcpy(mem+0x8000, mem+0xa000, 0x2000);
	if (size <= 0x4000)
	{
		const char *info;
		memcpy(&mem[0x4000], &mem[0x8000], 0x4000);
		info = image_extrainfo(image);
		if (info!=NULL && strcmp(info, "A13MIRRORING")==0)
		{
			memcpy(&mem[0x8000], &mem[0xa000], 0x2000);
			memcpy(&mem[0x6000], &mem[0x4000], 0x2000);
		}
	}
	logerror("%s loaded cartridge '%s' size %dK\n",
		image->machine->gamedrv->name, image_filename(image) , size/1024);
	return INIT_PASS;
}

DEVICE_IMAGE_UNLOAD( a5200_cart )
{
	UINT8 *mem = memory_region(image->machine, "maincpu");
	/* zap the cartridge memory (again) */
	memset(&mem[0x4000], 0x00, 0x8000);
}
