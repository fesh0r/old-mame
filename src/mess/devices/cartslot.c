/**********************************************************************

    cartslot.c

	Cartridge device

**********************************************************************/

#include <ctype.h>
#include "driver.h"
#include "cartslot.h"
#include "multcart.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

enum _process_mode
{
	PROCESS_CLEAR,
	PROCESS_LOAD
};
typedef enum _process_mode process_mode;


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE cartslot_t *get_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == CARTSLOT);
	return (cartslot_t *) device->token;
}


INLINE const cartslot_config *get_config(const device_config *device)
{
	assert(device != NULL);
	assert(device->type == CARTSLOT);
	return (const cartslot_config *) device->inline_config;
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    load_cartridge
-------------------------------------------------*/

static int load_cartridge(const device_config *device, const rom_entry *romrgn, const rom_entry *roment, process_mode mode)
{
	const char *region;
	const char *type;
	UINT32 flags;
	offs_t offset, length, read_length, pos = 0, len;
	UINT8 *ptr;
	UINT8 clear_val;
	int datawidth, littleendian, i, j;
	const device_config *cpu;

	region = ROMREGION_GETTAG(romrgn);
	offset = ROM_GETOFFSET(roment);
	length = ROM_GETLENGTH(roment);
	flags = ROM_GETFLAGS(roment);
	ptr = ((UINT8 *) memory_region(device->machine, region)) + offset;

	if (mode == PROCESS_LOAD)
	{
		/* must this be full size */
		if (flags & ROM_FULLSIZE)
		{
			if (image_length(device) != length)
				return INIT_FAIL;
		}

		/* read the ROM */
		pos = read_length = image_fread(device, ptr, length);

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
		type = ROMREGION_GETTAG(romrgn);
		littleendian = ROMREGION_ISLITTLEENDIAN(romrgn);
		datawidth = ROMREGION_GETWIDTH(romrgn) / 8;

		/* if the region is inverted, do that now */
		cpu = cputag_get_cpu(device->machine, type);
		if (cpu != NULL)
		{
			datawidth = cpu_get_databus_width(cpu, ADDRESS_SPACE_PROGRAM) / 8;
			littleendian = (cpu_get_endianness(cpu) == ENDIANNESS_LITTLE);
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



/*-------------------------------------------------
    process_cartridge
-------------------------------------------------*/

static int process_cartridge(const device_config *image, process_mode mode)
{
	const rom_source *source;
	const rom_entry *romrgn, *roment;
	int result;

	for (source = rom_first_source(image->machine->gamedrv, image->machine->config); source != NULL; source = rom_next_source(image->machine->gamedrv, image->machine->config, source))
	{
		for (romrgn = rom_first_region(image->machine->gamedrv, source); romrgn != NULL; romrgn = rom_next_region(romrgn))
		{
			roment = romrgn + 1;
			while(!ROMENTRY_ISREGIONEND(roment))
			{
				if (ROMENTRY_GETTYPE(roment) == ROMENTRYTYPE_CARTRIDGE)
				{					
					if (strcmp(roment->_hashdata,image->tag)==0)
					{						
						result = load_cartridge(image, romrgn, roment, mode);
						if (!result)
							return result;
					}
				}
				roment++;
			}
		}
	}
	return INIT_PASS;
}


/*-------------------------------------------------
    cartslot_get_pcb
-------------------------------------------------*/

const device_config *cartslot_get_pcb(const device_config *device)
{
	cartslot_t *cart = get_token(device);
	return cart->pcb_device;
}


/*-------------------------------------------------
    cartslot_get_socket
-------------------------------------------------*/

void *cartslot_get_socket(const device_config *device, const char *socket_name)
{
	cartslot_t *cart = get_token(device);
	void *result = NULL;

	if (cart->mc != NULL)
	{
		const multicart_socket *socket;
		for (socket = cart->mc->sockets; socket != NULL; socket = socket->next)
		{
			if (!strcmp(socket->id, socket_name))
				break;
		}
		result = socket ? socket->ptr : NULL;
	}
	else if (socket_name[0] == '\0')
	{
		result = image_ptr(device);
	}
	return result;
}

/*-------------------------------------------------
    cartslot_get_resource_length
-------------------------------------------------*/

int cartslot_get_resource_length(const device_config *device, const char *socket_name)
{
	cartslot_t *cart = get_token(device);
	int result = 0;

	if (cart->mc != NULL)
	{
		const multicart_socket *socket;

		for (socket = cart->mc->sockets; socket != NULL; socket = socket->next)
		{
			if (!strcmp(socket->id, socket_name)) {
				break;
			}
		}
		if (socket != NULL) 
			result = socket->resource->length;
	}
	else 
		result = 0;

	return result;
}


/*-------------------------------------------------
    DEVICE_START( cartslot )
-------------------------------------------------*/

static DEVICE_START( cartslot )
{
	cartslot_t *cart = get_token(device);
	const cartslot_config *config = get_config(device);
	astring *tempstring = astring_alloc();

	/* if this cartridge has a custom DEVICE_START, use it */
	if (config->device_start != NULL)
	{
		(*config->device_start)(device);
		goto done;
	}
	
	/* find the PCB (if there is one) */
	cart->pcb_device = devtag_get_device(
		device->machine,
		device_build_tag(tempstring, device, TAG_PCB));

done:
	astring_free(tempstring);
}


/*-------------------------------------------------
    DEVICE_IMAGE_LOAD( cartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD( cartslot )
{	
	int result;
	cartslot_t *cart = get_token(image);
	const cartslot_config *config = get_config(image);

	/* if this cartridge has a custom DEVICE_IMAGE_LOAD, use it */
	if (config->device_load != NULL)
		return (*config->device_load)(image);

	/* try opening this as if it were a multicart */
	multicart_open(image_filename(image), image->machine->gamedrv->name, MULTICART_FLAGS_LOAD_RESOURCES, &cart->mc);
	if (cart->mc == NULL)
	{
		/* otherwise try the normal route */
		result = process_cartridge(image, PROCESS_LOAD);
		if (result != INIT_PASS)
			return result;
	}

	return INIT_PASS;
}


/*-------------------------------------------------
    DEVICE_IMAGE_UNLOAD( cartslot )
-------------------------------------------------*/

static DEVICE_IMAGE_UNLOAD( cartslot )
{
	cartslot_t *cart = get_token(image);
	const cartslot_config *config = get_config(image);

	/* if this cartridge has a custom DEVICE_IMAGE_UNLOAD, use it */
	if (config->device_unload != NULL)
	{
		(*config->device_unload)(image);
		return;
	}

	if (cart->mc != NULL)
	{
		multicart_close(cart->mc);
		cart->mc = NULL;
	}

	process_cartridge(image, PROCESS_CLEAR);
}


/*-------------------------------------------------
    identify_pcb
-------------------------------------------------*/

static const cartslot_pcb_type *identify_pcb(const device_config *device)
{
	const cartslot_config *config = get_config(device);
	astring *pcb_name = astring_alloc();
	const cartslot_pcb_type *pcb_type = NULL;
	multicart *mc;
	int i;

	if (image_exists(device))
	{
		/* try opening this as if it were a multicart */
		multicart_open_error me = multicart_open(image_filename(device), device->machine->gamedrv->name, MULTICART_FLAGS_DONT_LOAD_RESOURCES, &mc);
		if (me == MCERR_NONE)
		{
			/* this was a multicart - read from it */
			astring_cpyc(pcb_name, mc->pcb_type);
			multicart_close(mc);
		}
		else
		{
			if (me != MCERR_NOT_MULTICART)
				fatalerror("multicart error: %s\n", mc_error_text(me));
			if (image_pcb(device) != NULL)
			{
				/* read from hash file */
				astring_cpyc(pcb_name, image_pcb(device));
			}
		}

		/* look for PCB type with matching name */
		for (i = 0; (i < ARRAY_LENGTH(config->pcb_types)) && (config->pcb_types[i].name != NULL); i++)
		{
			if ((config->pcb_types[i].name[0] == '\0') || !strcmp(astring_c(pcb_name), config->pcb_types[i].name))
			{
				pcb_type = &config->pcb_types[i];
				break;
			}
		}
		
		/* check for unknown PCB type */
		if ((mc != NULL) && (pcb_type == NULL))
			fatalerror("Unknown PCB type \"%s\"\n", astring_c(pcb_name));
	}
	else
	{
		/* no device loaded; use the default */
		pcb_type = (config->pcb_types[0].name != NULL) ? &config->pcb_types[0] : NULL;
	}

	astring_free(pcb_name);
	return pcb_type;
}


/*-------------------------------------------------
    DEVICE_GET_IMAGE_DEVICES(cartslot)
-------------------------------------------------*/

static DEVICE_GET_IMAGE_DEVICES(cartslot)
{
	const cartslot_pcb_type *pcb_type;
	astring *tempstring = astring_alloc();

	pcb_type = identify_pcb(device);
	if (pcb_type != NULL)
	{
		device_list_add(
			listheadptr,
			device,
			pcb_type->devtype,
			device_build_tag(tempstring, device, TAG_PCB),
			0);
	}

	astring_free(tempstring);
}


/*-------------------------------------------------
    DEVICE_GET_INFO( cartslot )
-------------------------------------------------*/

DEVICE_GET_INFO( cartslot )
{	
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(cartslot_t); break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = sizeof(cartslot_config); break;
		case DEVINFO_INT_CLASS:						info->i = DEVICE_CLASS_PERIPHERAL; break;
		case DEVINFO_INT_IMAGE_TYPE:				info->i = IO_CARTSLOT; break;
		case DEVINFO_INT_IMAGE_READABLE:			info->i = 1; break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_CREATABLE:			info->i = 0; break;
		case DEVINFO_INT_IMAGE_RESET_ON_LOAD:		info->i = 1; break;
		case DEVINFO_INT_IMAGE_MUST_BE_LOADED:		if ( device && device->inline_config) {
														info->i = get_config(device)->must_be_loaded; 
													} else {
														info->i = 0; 
													}
													break;

		/* --- the following bits of info are returned as pointers to functions --- */
		case DEVINFO_FCT_START:						info->start = DEVICE_START_NAME(cartslot);					break;
		case DEVINFO_FCT_IMAGE_LOAD:				info->f = (genf *) DEVICE_IMAGE_LOAD_NAME(cartslot);		break; 
		case DEVINFO_FCT_IMAGE_UNLOAD:				info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(cartslot);		break; 
		case DEVINFO_FCT_GET_IMAGE_DEVICES:			info->f = (genf *) DEVICE_GET_IMAGE_DEVICES_NAME(cartslot);	break;
		case DEVINFO_FCT_IMAGE_PARTIAL_HASH:		if ( device && device->inline_config && get_config(device)->device_partialhash) {
														info->f = (genf *) get_config(device)->device_partialhash; 
													} else {
														info->f = NULL; 
													}
													break;			

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:						strcpy(info->s, "Cartslot"); break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Cartslot"); break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__); break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:
			if ( device && device->inline_config && get_config(device)->extensions )
			{
				strcpy(info->s, get_config(device)->extensions);
			}
			else
			{
				strcpy(info->s, "bin");
			}
			break;
	}
}
