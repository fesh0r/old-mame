/*
This file is a set of function calls and defs required for MESS.
*/

#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include "driver.h"
#include "config.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "ui_text.h"
#include "state.h"
#include "image.h"
#include "inputx.h"

extern struct GameOptions options;

/* Globals */
const char *mess_path;
UINT32 mess_ram_size;
UINT8 *mess_ram;

int DECL_SPEC mess_printf(const char *fmt, ...)
{
	va_list arg;
	int length = 0;

	va_start(arg,fmt);

	if (options.mess_printf_output)
		length = options.mess_printf_output(fmt, arg);
	else if (!options.gui_host)
		length = vprintf(fmt, arg);

	va_end(arg);

	return length;
}

struct distributed_images
{
	const char *names[IO_COUNT][MAX_DEV_INSTANCES];
	int count[IO_COUNT];
};

/*****************************************************************************
 *  --Distribute images to their respective Devices--
 *  Copy the Images specified at the CLI from options.image_files[] to the
 *  array of filenames we keep here, depending on the Device type identifier
 *  of each image.  Multiple instances of the same device are allowed
 *  RETURNS 0 on success, 1 if failed
 ****************************************************************************/
static int distribute_images(struct distributed_images *images)
{
	int i;

	logerror("Distributing Images to Devices...\n");
	/* Set names to NULL */

	for( i = 0; i < options.image_count; i++ )
	{
		int type = options.image_files[i].type;

		assert(type < IO_COUNT);

		/* Do we have too many devices? */
		if (images->count[type] >= MAX_DEV_INSTANCES)
		{
			mess_printf(" Too many devices of type %d\n", type);
			return 1;
		}

		/* Add a filename to the arrays of names */
		images->names[type][images->count[type]] = options.image_files[i].name;
		images->count[type]++;
	}

	/* everything was fine */
	return 0;
}



/* Small check to see if system supports device */
static int supported_device(const struct GameDriver *gamedrv, int type)
{
	const struct IODevice *dev;
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		if(dev->type==type)
			return TRUE;	/* Return OK */
	}
	return FALSE;
}

#if 0
extern void cpu_setbank_fromram(int bank, UINT32 ramposition, mem_read_handler rhandler, mem_write_handler whandler)
{
	assert(mess_ram_size > 0);
	assert(mess_ram);
	assert((rhandler && whandler) || (!rhandler && !whandler));

	if (ramposition >= mess_ram_size && rhandler)
	{
		memory_set_bankhandler_r(bank, MRA_NOP);
		memory_set_bankhandler_w(bank, MWA_NOP);
	}
	else
	{
		if (rhandler)
		{
			/* this is only necessary if not mirroring */
			memory_set_bankhandler_r(bank, rhandler);
			memory_set_bankhandler_w(bank, whandler);
		}
		else
		{
			/* NULL handlers imply mirroring */
			ramposition %= mess_ram_size;
		}
		cpu_setbank(bank, &mess_ram[ramposition]);
	}
}
#endif

static int ram_init(const struct GameDriver *gamedrv)
{
	int i;

	/* validate RAM option */
	if (options.ram != 0)
	{
		if (!ram_is_valid_option(gamedrv, options.ram))
		{
			char buffer[RAM_STRING_BUFLEN];
			int opt_count;

			opt_count = ram_option_count(gamedrv);
			if (opt_count == 0)
			{
				/* this driver doesn't support RAM configurations */
				mess_printf("Driver '%s' does not support RAM configurations\n", gamedrv->name);
			}
			else
			{
				mess_printf("%s is not a valid RAM option for driver '%s' (valid choices are ",
					ram_string(buffer, options.ram), gamedrv->name);
				for (i = 0; i < opt_count; i++)
					mess_printf("%s%s",  i ? " or " : "", ram_string(buffer, ram_option(gamedrv, i)));
				mess_printf(")\n");
			}
			return 1;
		}
		mess_ram_size = options.ram;
	}
	else
	{
		/* none specified; chose default */
		mess_ram_size = ram_default(gamedrv);
	}
	/* if we have RAM, allocate it */
	if (mess_ram_size > 0)
	{
		mess_ram = (UINT8 *) auto_malloc(mess_ram_size);
		if (!mess_ram)
			return 1;
		memset(mess_ram, 0xcd, mess_ram_size);

		state_save_register_UINT32("mess", 0, "ramsize", &mess_ram_size, 1);
		state_save_register_UINT8("mess", 0, "ram", mess_ram, mess_ram_size);
	}
	else
	{
		mess_ram = NULL;
	}
	return 0;
}

/*****************************************************************************
 *  --Initialise Devices--
 *  Call the init() functions for all devices of a driver
 *  ith all user specified image names.
 ****************************************************************************/
int devices_init(const struct GameDriver *gamedrv)
{
	const struct IODevice *dev;
	int i, id;

	/* convienient place to call this */
	{
		const char *cwd;
		char *s;

		cwd = osd_get_cwd();
		s = auto_malloc(strlen(cwd) + 1);
		if (!s)
			return 1;
		strcpy(s, cwd);
		mess_path = s;
	}

	inputx_init();

	/* Check that the driver supports all devices requested (options struct)*/
	for( i = 0; i < options.image_count; i++ )
	{
		if (supported_device(Machine->gamedrv, options.image_files[i].type)==FALSE)
		{
			mess_printf(" ERROR: Device [%s] is not supported by this system\n",device_typename(options.image_files[i].type));
			return 1;
		}
	}

	/* initialize RAM code */
	if (ram_init(gamedrv))
		return 1;

	/* init all devices */
	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
			image_init(dev->type, id);
	}

	return 0;
}

int devices_initialload(const struct GameDriver *gamedrv, int ispreload)
{
	int id;
	int result;
	int count;
	struct distributed_images images;
	const struct IODevice *dev;
	const char *imagename;

	/* normalize ispreload */
	ispreload = ispreload ? DEVICE_LOAD_AT_INIT : 0;

	/* distribute images to appropriate devices */
	memset(&images, 0, sizeof(images));
	if (distribute_images(&images))
		return 1;

	/* load all devices with matching preload */
	for(dev = device_first(gamedrv); dev; dev = device_next(gamedrv, dev))
	{
		count = images.count[dev->type];

		if ((dev->flags & DEVICE_MUST_BE_LOADED) && (count != dev->count))
		{
			mess_printf("Driver requires that device %s must have an image to load\n", device_typename(dev->type));
			return 1;
		}

		/* all instances */
		for( id = 0; id < count; id++ )
		{
			if ((dev->flags & DEVICE_LOAD_AT_INIT) == ispreload)
			{
				imagename = images.names[dev->type][id];
				if (imagename)
				{
					/* load this image */
					result = image_load(dev->type, id, images.names[dev->type][id]);

					if (result != INIT_PASS)
					{
						mess_printf("Driver reports load for %s device failed\n", device_typename(dev->type));
						mess_printf("Ensure image is valid and exists and (if needed) can be created\n");
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

/*
 * Call the exit() functions for all devices of a
 * driver for all images.
 */
void devices_exit(void)
{
	const struct IODevice *dev;
	int id;

	/* unload all devices */
	image_unload_all(TRUE);

	/* exit all devices */
	for(dev = device_first(Machine->gamedrv); dev; dev = device_next(Machine->gamedrv, dev))
	{
		/* all instances */
		for( id = 0; id < dev->count; id++ )
			image_exit(dev->type, id);
	}
}

void showmessdisclaimer(void)
{
	mess_printf(
		"MESS is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
		"several computer and console systems. But hardware is useless without software\n"
		"so a file dump of the BIOS, cartridges, discs, and cassettes which run on that\n"
		"hardware is required. Such files, like any other commercial software, are\n"
		"copyrighted material and it is therefore illegal to use them if you don't own\n"
		"the original media from which the files are derived. Needless to say, these\n"
		"files are not distributed together with MESS. Distribution of MESS together\n"
		"with these files is a violation of copyright law and should be promptly\n"
		"reported to the authors so that appropriate legal action can be taken.\n\n");
}

void showmessinfo(void)
{
	mess_printf(
		"M.E.S.S. v%s\n"
		"Multiple Emulation Super System - Copyright (C) 1997-2002 by the MESS Team\n"
		"M.E.S.S. is based on the ever excellent M.A.M.E. Source code\n"
		"Copyright (C) 1997-2002 by Nicola Salmoria and the MAME Team\n\n",
		build_version);
	showmessdisclaimer();
	mess_printf(
		"Usage:  MESS <system> <device> <software> <options>\n\n"
		"        MESS -list        for a brief list of supported systems\n"
		"        MESS -listdevices for a full list of supported devices\n"
		"        MESS -showusage   to see usage instructions\n"
		"See mess.txt for help, readme.txt for options.\n");

}

static char *battery_nvramfilename(const char *filename)
{
	return strip_extension(osd_basename((char *) filename));
}

/* load battery backed nvram from a driver subdir. in the nvram dir. */
int battery_load(const char *filename, void *buffer, int length)
{
	mame_file *f;
	int bytes_read = 0;
	int result = FALSE;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(filename);
		if (nvram_filename)
		{
			f = mame_fopen(Machine->gamedrv->name, nvram_filename, FILETYPE_NVRAM, 0);
			if (f)
			{
				bytes_read = mame_fread(f, buffer, length);
				mame_fclose(f);
				result = TRUE;
			}
			free(nvram_filename);
		}

		/* fill remaining bytes (if necessary) */
		memset(((char *) buffer) + bytes_read, '\0', length - bytes_read);
	}
	return result;
}

/* save battery backed nvram to a driver subdir. in the nvram dir. */
int battery_save(const char *filename, void *buffer, int length)
{
	mame_file *f;
	char *nvram_filename;

	/* some sanity checking */
	if( buffer != NULL && length > 0 )
	{
		nvram_filename = battery_nvramfilename(filename);
		if (nvram_filename)
		{
			f = mame_fopen(Machine->gamedrv->name, nvram_filename, FILETYPE_NVRAM, 1);
			if (f)
			{
				mame_fwrite(f, buffer, length);
				mame_fclose(f);
				return TRUE;
			}
			free(nvram_filename);
		}
	}
	return FALSE;
}

void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count)
{
	while(color_count--)
	{
		palette_set_color(color_base++, colors[0], colors[1], colors[2]);
		colors += 3;
	}
}

void ram_dump(const char *filename)
{
	mame_file *file;

	if (!filename)
		filename = "ram.bin";

	file = mame_fopen(Machine->gamedrv->name, filename, FILETYPE_NVRAM, OSD_FOPEN_WRITE);
	if (file)
	{
		mame_fwrite(file, mess_ram, mess_ram_size);

		/* close file */
		mame_fclose(file);
	}
}

#ifdef MAME_DEBUG
int messvaliditychecks(void)
{
	int i;
	int error = 0;
	const struct RomModule *region, *rom;
	const struct IODevice *dev;
	long used_devices;
	const char *s;
	extern int device_valididtychecks(void);

	/* MESS specific driver validity checks */
	for(i = 0; drivers[i]; i++)
	{
		/* check device array */
		used_devices = 0;
		for(dev = device_first(drivers[i]); dev; dev = device_next(drivers[i], dev))
		{
			if (dev->type >= IO_COUNT)
			{
				printf("%s: invalid device type %i\n", drivers[i]->name, dev->type);
				error = 1;
			}

			/* make sure that we can't duplicate devices */
			if (used_devices & (1 << dev->type))
			{
				printf("%s: device type '%s' is specified multiple times\n", drivers[i]->name, device_typename(dev->type));
				error = 1;
			}
			used_devices |= (1 << dev->type);

			/* make sure that the file extensions array is valid */
			s = dev->file_extensions;
			while(*s)
				s += strlen(s) + 1;

			/* enforce certain rules for certain device types */
			switch(dev->type) {
			case IO_QUICKLOAD:
			case IO_SNAPSHOT:
				if (dev->count != 1)
				{
					printf("%s: there can only be one instance of devices of type '%s'\n", drivers[i]->name, device_typename(dev->type));
					error = 1;
				}
				/* fallthrough */

			case IO_CARTSLOT:
				if (dev->open_mode != OSD_FOPEN_READ)
				{
					printf("%s: devices of type '%s' must have open mode OSD_FOPEN_READ\n", drivers[i]->name, device_typename(dev->type));
					error = 1;
				}
				break;
			}
		}

		/* this detects some inconsistencies in the ROM structures */
		for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
		{
			for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			{
				char name[100];
				snprintf(name, sizeof(name) / sizeof(name[0]), "%s", ROM_GETNAME(rom));
			}
		}

		/* check system config */
		ram_option_count(drivers[i]);

		/* make sure that our input system likes this driver */
		if (inputx_validitycheck(drivers[i]))
			error = 1;
	}

	if (inputx_validitycheck(NULL))
		error = 1;
	if (device_valididtychecks())
		error = 1;
	return error;
}

enum
{
	TESTERROR_SUCCESS			= 0,

	/* benign errors */
	TESTERROR_NOROMS			= 1,

	/* failures */
	TESTERROR_INITDEVICESFAILED	= -1
};

#if 0
static int try_driver(const struct GameDriver *gamedrv)
{
	int i;
	int error;
	void *mem[128];

	Machine->gamedrv = gamedrv;
	Machine->drv = gamedrv->drv;
	memset(&Machine->memory_region, 0, sizeof(Machine->memory_region));
	if (readroms() != 0)
	{
		error = TESTERROR_NOROMS;
	}
	else
	{
		if (device_init(gamedrv) != 0)
			error = TESTERROR_INITDEVICESFAILED;
		else
			devices_exit();
		for (i = 0;i < MAX_MEMORY_REGIONS;i++)
			free_memory_region(i);
	}

	/* hammer away on malloc, to detect any memory errors */
	for (i = 0; i < sizeof(mem) / sizeof(mem[0]); i++)
		mem[i] = malloc((i * 131 % 51) + 64);
	for (i = 0; i < sizeof(mem) / sizeof(mem[0]); i++)
		free(mem[i]);

	return 0;
}
#endif

void messtestdriver(const struct GameDriver *gamedrv, const char *(*getfodderimage)(unsigned int index, int *foddertype))
{
	struct GameOptions saved_options;

	/* preserve old options; the MESS GUI needs this */
	memcpy(&saved_options, &options, sizeof(saved_options));

	/* clear out images in options */
	memset(&options.image_files, 0, sizeof(options.image_files));
	options.image_count = 0;

	/* try running with no attached devices */
#if 0
	error = try_driver(gamedrv);
	if (gamedrv->flags & GAME_COMPUTER)
		assert(error >= 0);	/* computers should succeed when ran with no attached devices */
	else
		assert(error <= 0);	/* consoles can fail; but should never fail due to a no roms error */
#endif

	/* restore old options */
	memcpy(&options, &saved_options, sizeof(options));
}

#endif /* MAME_DEBUG */
