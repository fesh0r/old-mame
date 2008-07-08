#include <ctype.h>
#include "driver.h"
#include "cartslot.h"


static int is_cart_roment(const rom_entry *roment)
{
	return ROMENTRY_GETTYPE(roment) == ROMENTRYTYPE_CARTRIDGE;
}



static int parse_rom_name(const rom_entry *roment, int *position, const char **extensions)
{
	const char *data = roment->_hashdata;

	if (data[0] != '#')
		return -1;
	if (!isdigit(data[1]))
		return -1;
	if (data[2] != '\0')
		return -1;

	if (position)
		*position = data[1] - '0';
	if (extensions)
		*extensions = &data[3];
	return 0;
}



static int load_cartridge(running_machine *machine, const rom_entry *romrgn, const rom_entry *roment, const device_config *image)
{
	UINT32 region, flags;
	offs_t offset, length, read_length, pos = 0, len;
	UINT8 *ptr;
	UINT8 clear_val;
	int type, datawidth, littleendian, i, j;

	region = ROMREGION_GETTYPE(romrgn);
	offset = ROM_GETOFFSET(roment);
	length = ROM_GETLENGTH(roment);
	flags = ROM_GETFLAGS(roment);
	ptr = ((UINT8 *) memory_region(machine, region)) + offset;

	if (image)
	{
		/* must this be full size */
		if (flags & ROM_FULLSIZE)
		{
			if (image_length(image) != length)
				return INIT_FAIL;
		}

		/* read the ROM */
		pos = read_length = image_fread(image, ptr, length);

		/* do we need to mirror the ROM? */
		if (flags & ROM_MIRROR)
		{
			while(pos < length)
			{
				len = MIN(read_length, length - pos);
				memcpy(ptr + pos, ptr, len);
				pos += len;
			}
		}

		/* postprocess this region */
		type = ROMREGION_GETTYPE(romrgn);
		littleendian = ROMREGION_ISLITTLEENDIAN(romrgn);
		datawidth = ROMREGION_GETWIDTH(romrgn) / 8;

		/* if the region is inverted, do that now */
		if (type >= REGION_CPU1 && type < REGION_CPU1 + MAX_CPU)
		{
			int cputype = image->machine->config->cpu[type - REGION_CPU1].type;
			if (cputype != 0)
			{
				datawidth = cputype_databus_width(cputype, ADDRESS_SPACE_PROGRAM) / 8;
				littleendian = (cputype_endianness(cputype) == CPU_IS_LE);
			}
		}

		/* swap the endianness if we need to */
#ifdef LSB_FIRST
		if (datawidth > 1 && !littleendian)
#else
		if (datawidth > 1 && littleendian)
#endif
		{
			for (i = 0; i < length; i += datawidth)
			{
				UINT8 temp[8];
				memcpy(temp, &ptr[i], datawidth);
				for (j = datawidth - 1; j >= 0; j--)
					ptr[i + j] = temp[datawidth - 1 - j];
			}
		}
	}

	/* clear out anything that remains */
	if (!(flags & ROM_NOCLEAR))
	{
		clear_val = (flags & ROM_FILL_FF) ? 0xFF : 0x00;
		memset(ptr + pos, clear_val, length - pos);
	}
	return INIT_PASS;
}



static int process_cartridge(const device_config *image, const device_config *file)
{
	const rom_entry *romrgn, *roment;
	int position = 0, result;

	romrgn = rom_first_region(image->machine->gamedrv);
	while(romrgn)
	{
		roment = romrgn + 1;
		while(!ROMENTRY_ISREGIONEND(roment))
		{
			if (is_cart_roment(roment))
			{
				parse_rom_name(roment, &position, NULL);
				if (position == image_index_in_device(image))
				{
					result = load_cartridge(image->machine, romrgn, roment, file);
					if (!result)
						return result;
				}
			}
			roment++;
		}
		romrgn = rom_next_region(romrgn);
	}
	return INIT_PASS;
}



static DEVICE_START( cartslot_specified )
{
	process_cartridge(device, NULL);
}

static DEVICE_IMAGE_LOAD( cartslot_specified )
{
	return process_cartridge(image, image);
}

static DEVICE_IMAGE_UNLOAD( cartslot_specified )
{
	process_cartridge(image, NULL);
}



void cartslot_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	const rom_entry *romrgn, *roment;
	int position, count = 0, must_be_loaded = 0;
	const char *file_extensions = "bin";
	UINT32 flags;

	/* try to find ROM_CART_LOADs in the ROM declaration */
	romrgn = rom_first_region(devclass->gamedrv);
	while(romrgn)
	{
		roment = romrgn + 1;
		while(!ROMENTRY_ISREGIONEND(roment))
		{
			if (is_cart_roment(roment) && !parse_rom_name(roment, &position, &file_extensions))
			{
				flags = ROM_GETFLAGS(roment);

				/* reject any unsupported flags */
				if (flags & (ROM_GROUPMASK | ROM_SKIPMASK | ROM_REVERSEMASK
					| ROM_BITWIDTHMASK | ROM_BITSHIFTMASK
					| ROM_INHERITFLAGSMASK | ROM_BIOSFLAGSMASK))
				{
					fatalerror("Unsupported ROM cart flags 0x%08X", flags);
				}

				count = MAX(position + 1, count);
				must_be_loaded = (flags & ROM_OPTIONAL) ? 0 : 1;
			}
			roment++;
		}
		romrgn = rom_next_region(romrgn);
	}

	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:						info->i = count; break;
		case MESS_DEVINFO_INT_TYPE:						info->i = IO_CARTSLOT; break;
		case MESS_DEVINFO_INT_READABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_CREATABLE:					info->i = 0; break;
		case MESS_DEVINFO_INT_RESET_ON_LOAD:				info->i = 1; break;
		case MESS_DEVINFO_INT_LOAD_AT_INIT:				info->i = 1; break;
		case MESS_DEVINFO_INT_MUST_BE_LOADED:			info->i = must_be_loaded; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = (count > 0) ? DEVICE_START_NAME(cartslot_specified) : NULL; break;
		case MESS_DEVINFO_PTR_LOAD:						info->load = (count > 0) ? DEVICE_IMAGE_LOAD_NAME(cartslot_specified) : NULL; break;
		case MESS_DEVINFO_PTR_UNLOAD:					info->unload = (count > 0) ? DEVICE_IMAGE_UNLOAD_NAME(cartslot_specified) : NULL; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:
			info->s = device_temp_str();
			strcpy(info->s, file_extensions);
			break;
	}
}
