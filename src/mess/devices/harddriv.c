/*********************************************************************

	Code to interface the MESS image code with MAME's harddisk core.

	We do not support diff files as it will involve some changes in
	the MESS image code.  Additionally, the need for diff files comes
	from MAME's need for "cannonical" hard drive images.

	Raphael Nabet 2003

	Update: 23-Feb-2004 - Unlike floppy disks, for which we support
	myriad formats on many systems, it is my intention for MESS to
	standardize on the CHD file format for hard drives so I made a few
	changes to support this

*********************************************************************/

#include "mame.h"
#include "harddisk.h"
#include "harddriv.h"


#define MAX_HARDDISKS	8
#define USE_CHD_OPEN	0

static const char *const error_strings[] =
{
	"no error",
	"no drive interface",
	"out of memory",
	"invalid file",
	"invalid parameter",
	"invalid data",
	"file not found",
	"requires parent",
	"file not writeable",
	"read error",
	"write error",
	"codec error",
	"invalid parent",
	"hunk out of range",
	"decompression error",
	"compression error",
	"can't create file",
	"can't verify file"
	"operation not supported",
	"can't find metadata",
	"invalid metadata size",
	"unsupported CHD version"
};

static hard_disk_file *drive_handles[MAX_HARDDISKS];

static const char *chd_get_error_string(int chderr)
{
	if ((chderr < 0 ) || (chderr >= (sizeof(error_strings) / sizeof(error_strings[0]))))
		return NULL;
	return error_strings[chderr];
}



static OPTION_GUIDE_START(mess_hd_option_guide)
	OPTION_INT('C', "cylinders",		"Cylinders")
	OPTION_INT('H', "heads",			"Heads")
	OPTION_INT('S', "sectors",			"Sectors")
	OPTION_INT('L', "sectorlength",		"Sector Bytes")
	OPTION_INT('K', "hunksize",			"Hunk Bytes")
OPTION_GUIDE_END

static const char *mess_hd_option_spec =
	"C1-[512]-1024;H1/2/[4]/8;S1-[16]-64;L128/256/[512]/1024;K512/1024/2048/[4096]";


#define MESSHDTAG "mess_hd"

struct mess_hd
{
	chd_file *chd;
	hard_disk_file *hard_disk_handle;
};

static struct mess_hd *get_drive(const device_config *img)
{
	return image_lookuptag(img, MESSHDTAG);
}



/*************************************
 *
 *	device_init_mess_hd()
 *
 *	Device init
 *
 *************************************/

DEVICE_START( mess_hd )
{
	struct mess_hd *hd;
	hd = image_alloctag(device, MESSHDTAG, sizeof(struct mess_hd));
	hd->hard_disk_handle = NULL;
}



/*************************************
 *
 *	DEVICE_IMAGE_LOAD_NAME(mess_hd)
 *  DEVICE_IMAGE_CREATE_NAME(mess_hd)
 *
 *	Device load and create
 *
 *************************************/

static int internal_load_mess_hd(const device_config *image, const char *metadata)
{
	chd_error err = 0;
	struct mess_hd *hd;
	int is_writeable;
	int id = image_index_in_device(image);

	hd = get_drive(image);

	/* open the CHD file */
	do
	{
		is_writeable = image_is_writable(image);
		hd->chd = NULL;
		err = chd_open_file(image_core_file(image), is_writeable ? CHD_OPEN_READWRITE : CHD_OPEN_READ, NULL, &hd->chd);

		/* special case; if we get CHDERR_FILE_NOT_WRITEABLE, make the
		 * image read only and repeat */
		if (err == CHDERR_FILE_NOT_WRITEABLE)
			image_make_readonly(image);
	}
	while(!hd->chd && is_writeable && (err == CHDERR_FILE_NOT_WRITEABLE));
	if (!hd->chd)
		goto done;

	/* if we created the image and hence, have metadata to set, set the metadata */
	if (metadata)
	{
		err = chd_set_metadata(hd->chd, HARD_DISK_METADATA_TAG, 0, metadata, strlen(metadata) + 1);
		if (err != CHDERR_NONE)
			goto done;
	}

	/* open the hard disk file */
	hd->hard_disk_handle = hard_disk_open(hd->chd);
	if (!hd->hard_disk_handle)
		goto done;

	drive_handles[id] = hd->hard_disk_handle;

done:
	if (err)
	{
		/* if we had an error, close out the CHD */
		if (hd->chd != NULL)
		{
			chd_close(hd->chd);
			hd->chd = NULL;
		}

		image_seterror(image, IMAGE_ERROR_UNSPECIFIED, chd_get_error_string(err));
	}
	return err ? INIT_FAIL : INIT_PASS;
}



DEVICE_IMAGE_LOAD( mess_hd )
{
	return internal_load_mess_hd(image, NULL);
}


static DEVICE_IMAGE_CREATE( mess_hd )
{
	int err;
	char metadata[256];
	UINT32 sectorsize, hunksize;
	UINT32 cylinders, heads, sectors, totalsectors;

	cylinders	= option_resolution_lookup_int(create_args, 'C');
	heads		= option_resolution_lookup_int(create_args, 'H');
	sectors		= option_resolution_lookup_int(create_args, 'S');
	sectorsize	= option_resolution_lookup_int(create_args, 'L');
	hunksize	= option_resolution_lookup_int(create_args, 'K');

	totalsectors = cylinders * heads * sectors;

	/* create the CHD file */
	err = chd_create_file(image_core_file(image), (UINT64)totalsectors * (UINT64)sectorsize, hunksize, CHDCOMPRESSION_NONE, NULL);
	if (err != CHDERR_NONE)
		goto error;

	sprintf(metadata, HARD_DISK_METADATA_FORMAT, cylinders, heads, sectors, sectorsize);
	return internal_load_mess_hd(image, metadata);

error:
	return INIT_FAIL;
}



/*************************************
 *
 *	DEVICE_IMAGE_UNLOAD_NAME(mess_hd)
 *
 *	Device unload
 *
 *************************************/

DEVICE_IMAGE_UNLOAD( mess_hd )
{
	struct mess_hd *hd = get_drive(image);

	if (hd->hard_disk_handle != NULL)
	{
		hard_disk_close(hd->hard_disk_handle);
		hd->hard_disk_handle = NULL;
	}

	if (hd->chd != NULL)
	{
		chd_close(hd->chd);
		hd->chd = NULL;
	}

	drive_handles[image_index_in_device(image)] = NULL;
}



/*************************************
 *
 *	Get the MESS/MAME hard disk handle (from the src/harddisk.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

hard_disk_file *mess_hd_get_hard_disk_file(const device_config *image)
{
	struct mess_hd *hd = get_drive(image);
	return hd->hard_disk_handle;
}



/*************************************
 *
 *	Get the MESS/MAME CHD file (from the src/chd.c core)
 *  after an image has been opened with the mess_hd core
 *
 *************************************/

chd_file *mess_hd_get_chd_file(const device_config *image)
{
	chd_file *result = NULL;
	hard_disk_file *hd_file;

	if (image)
	{
		hd_file = mess_hd_get_hard_disk_file(image);
		if (hd_file)
			result = hard_disk_get_chd(hd_file);
	}
	return result;
}



/*************************************
 *
 *	Device specification function
 *
 *************************************/

void harddisk_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_TYPE:						info->i = IO_HARDDISK; break;
		case MESS_DEVINFO_INT_READABLE:					info->i = 1; break;
		case MESS_DEVINFO_INT_WRITEABLE:				info->i = 1; break;
		case MESS_DEVINFO_INT_CREATABLE:				info->i = 0; break;
		case MESS_DEVINFO_INT_CREATE_OPTCOUNT:			info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_START:						info->start = DEVICE_START_NAME(mess_hd); break;
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(mess_hd); break;
		case MESS_DEVINFO_PTR_UNLOAD:					info->unload = DEVICE_IMAGE_UNLOAD_NAME(mess_hd); break;
		case MESS_DEVINFO_PTR_CREATE:					info->create = DEVICE_IMAGE_CREATE_NAME(mess_hd); break;
		case MESS_DEVINFO_PTR_CREATE_OPTGUIDE:			info->p = (void *) mess_hd_option_guide; break;
		case MESS_DEVINFO_PTR_CREATE_OPTSPEC+0:			info->p = (void *) mess_hd_option_spec;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_DEV_FILE:					strcpy(info->s = device_temp_str(), __FILE__); break;
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "chd,hd"); break;
		case MESS_DEVINFO_STR_CREATE_OPTNAME+0:			strcpy(info->s = device_temp_str(), "chd"); break;
		case MESS_DEVINFO_STR_CREATE_OPTDESC+0:			strcpy(info->s = device_temp_str(), "MAME/MESS CHD Hard drive"); break;
		case MESS_DEVINFO_STR_CREATE_OPTEXTS+0:			strcpy(info->s = device_temp_str(), "chd,hd"); break;
	}
}

hard_disk_file *mess_hd_get_hard_disk_file_by_number(int drivenum)
{
	if ((drivenum < 0) || (drivenum > MAX_HARDDISKS))
	{
		return NULL;
	}

	return drive_handles[drivenum];
}

