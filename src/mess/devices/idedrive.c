/*********************************************************************

	idedrive.c

	Code to interface the MESS image code with MAME's IDE core

	Raphael Nabet 2003

 *********************************************************************/

#include "driver.h"
#include "idedrive.h"
#include "harddriv.h"
#include "machine/idectrl.h"

/* FIXME - this is so completely broken until we device-ize IDE drives */
struct ide_interface;

static void ide_get_params(const device_config *image, int *which_bus, int *which_address,
	struct ide_interface **intf,
	device_start_func *parent_init,
	device_image_load_func *parent_load,
	device_image_unload_func *parent_unload)
{
	const mess_device_class *devclass = &mess_device_from_core_device(image)->devclass;
	mess_device_class parent_devclass;

	*which_bus = image_index_in_device(image);
	*which_address = (int) mess_device_get_info_int(devclass, DEVINFO_INT_IDEDRIVE_ADDRESS);
	*intf = (struct ide_interface *) mess_device_get_info_ptr(devclass, DEVINFO_PTR_IDEDRIVE_INTERFACE);

	parent_devclass = *devclass;
	parent_devclass.get_info = harddisk_device_getinfo;

	if (parent_init)
		*parent_init = (device_start_func) mess_device_get_info_fct(&parent_devclass, MESS_DEVINFO_PTR_START);
	if (parent_load)
		*parent_load = (device_image_load_func) mess_device_get_info_fct(&parent_devclass, MESS_DEVINFO_PTR_LOAD);
	if (parent_unload)
		*parent_unload = (device_image_unload_func) mess_device_get_info_fct(&parent_devclass, MESS_DEVINFO_PTR_UNLOAD);

	assert(*which_address == 0);
	assert(*intf);
}



/*-------------------------------------------------
	DEVICE_START(ide_hd) - Init an IDE hard disk device
-------------------------------------------------*/

static DEVICE_START(ide_hd)
{
	int which_bus, which_address;
	struct ide_interface *intf;
	device_start_func parent_init;

	/* get the basics */
	ide_get_params(device, &which_bus, &which_address, &intf, &parent_init, NULL, NULL);

	/* call the parent init function */
	parent_init(device);

	/* configure IDE */
	/* FIXME IDE */
	/* ide_controller_init_custom(which_bus, intf, NULL); */
}



/*-------------------------------------------------
	DEVICE_IMAGE_LOAD(ide_hd) - Load an IDE hard disk image
-------------------------------------------------*/

static DEVICE_IMAGE_LOAD(ide_hd)
{
	int result, which_bus, which_address;
	struct ide_interface *intf;
	device_image_load_func parent_load;

	/* get the basics */
	ide_get_params(image, &which_bus, &which_address, &intf, NULL, &parent_load, NULL);

	/* call the parent load function */
	result = parent_load(image);
	if (result != INIT_PASS)
		return result;

	/* configure IDE */
	/* FIXME IDE */
	/* ide_controller_init_custom(which_bus, intf, mess_hd_get_chd_file(image)); */
	/* ide_controller_reset(which_bus); */
	return INIT_PASS;
}



/*-------------------------------------------------
	DEVICE_IMAGE_UNLOAD(ide_hd) - Unload an IDE hard disk image
-------------------------------------------------*/

static DEVICE_IMAGE_UNLOAD(ide_hd)
{
	int which_bus, which_address;
	struct ide_interface *intf;
	device_image_unload_func parent_unload;

	/* get the basics */
	ide_get_params(image, &which_bus, &which_address, &intf, NULL, NULL, &parent_unload);

	/* call the parent unload function */
	parent_unload(image);

	/* configure IDE */
	/* FIXME IDE */
	/* ide_controller_init_custom(which_bus, intf, NULL); */
	/* ide_controller_reset(which_bus); */
}



/*-------------------------------------------------
	ide_hd_validity_check - check this device's validity
-------------------------------------------------*/

static int ide_hd_validity_check(const mess_device_class *devclass)
{
	int error = 0;
	int which_address;
	INT64 count;
	struct ide_interface *intf;

	which_address = (int) mess_device_get_info_int(devclass, DEVINFO_INT_IDEDRIVE_ADDRESS);
	intf = (struct ide_interface *) mess_device_get_info_ptr(devclass, DEVINFO_PTR_IDEDRIVE_INTERFACE);
	count = mess_device_get_info_int(devclass, MESS_DEVINFO_INT_COUNT);

	if (which_address != 0)
	{
		mame_printf_error("%s: IDE device has non-zero address\n", devclass->gamedrv->name);
		error = 1;
	}

	if (!intf)
	{
		mame_printf_error("%s: IDE device does not specify an interface\n", devclass->gamedrv->name);
		error = 1;
	}

	return error;
}



/*-------------------------------------------------
	ide_harddisk_device_getinfo - Get info proc
-------------------------------------------------*/

void ide_harddisk_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:				info->start = DEVICE_START_NAME(ide_hd); break;
		case MESS_DEVINFO_PTR_LOAD:					info->load = DEVICE_IMAGE_LOAD_NAME(ide_hd); break;
		case MESS_DEVINFO_PTR_UNLOAD:				info->unload = DEVICE_IMAGE_UNLOAD_NAME(ide_hd); break;
		case MESS_DEVINFO_PTR_VALIDITY_CHECK:		info->validity_check = ide_hd_validity_check; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_NAME:					strcpy(info->s = device_temp_str(), "ideharddrive"); break;
		case MESS_DEVINFO_STR_SHORT_NAME:			strcpy(info->s = device_temp_str(), "idehd"); break;
		case MESS_DEVINFO_STR_DESCRIPTION:			strcpy(info->s = device_temp_str(), "IDE Hard Disk"); break;

		default: harddisk_device_getinfo(devclass, state, info); break;
	}
}
