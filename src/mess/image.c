/****************************************************************************

	image.c

	Code for handling devices/software images

	The UI can call image_load and image_unload to associate and disassociate
	with disk images on disk.  In fact, devices that unmount on their own (like
	Mac floppy drives) may call this from within a driver.

****************************************************************************/

#include <ctype.h>

#include "image.h"
#include "mess.h"
#include "unzip.h"
#include "devices/flopdrv.h"
#include "utils.h"
#include "tagpool.h"
#include "hashfile.h"
#include "mamecore.h"
#include "messopts.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _mess_image
{
	/* variables that persist across image mounts */
	tag_pool tagpool;
	memory_pool *mempool;
	const struct IODevice *dev;

	/* error related info */
	image_error_t err;
	char *err_message;

	/* variables that are only non-zero when an image is mounted */
	osd_file *file;
	char *name;
	char *dir;
	char *hash;
	UINT64 length;
	UINT64 pos;
	char *basename_noext;
	
	/* flags */
	unsigned int writeable : 1;
	unsigned int created : 1;
	unsigned int is_loading : 1;

	/* info read from the hash file */
	char *longname;
	char *manufacturer;
	char *year;
	char *playable;
	char *extrainfo;

	/* working directory; persists across mounts */
	char *working_directory;

	/* pointer */
	void *ptr;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static mess_image *images;
static UINT32 multiple_dev_mask;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void image_exit(running_machine *machine);
static void image_clear(mess_image *image);
static void image_clear_error(mess_image *image);



/***************************************************************************
    IMAGE SYSTEM INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    memory_error - report a memory error
-------------------------------------------------*/

static void memory_error(const char *message)
{
	fatalerror("%s", message);
}



/*-------------------------------------------------
    image_init - initialize the core image system
-------------------------------------------------*/

int image_init(void)
{
	int err;
	int count, indx, i, j;
	UINT32 mask, dev_mask = 0;

	/* setup the globals */
	images = NULL;
	multiple_dev_mask = 0;

	/* first count all images, and identify multiply defined devices */
	count = 0;
	for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
	{
		/* check to see if this device type is used multiple times */
		mask = 1 << Machine->devices[i].type;
		if (dev_mask & mask)
			multiple_dev_mask |= mask;
		else
			dev_mask |= mask;

		/* increment the count */
		count += Machine->devices[i].count;
	}

	/* allocate the array */
	if (count > 0)
	{
		images = auto_malloc(count * sizeof(*images));
		memset(images, 0, count * sizeof(*images));
	}

	/* initialize the devices */
	indx = 0;
	for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
	{
		for (j = 0; j < Machine->devices[i].count; j++)
		{
			images[indx + j].mempool = pool_create(memory_error);

			/* setup the device */
			tagpool_init(&images[indx + j].tagpool);
			images[indx + j].dev = &Machine->devices[i];

			if (Machine->devices[i].init)
			{
				err = Machine->devices[i].init(&images[indx + j]);
				if (err != INIT_PASS)
					return err;
			}

		}
		indx += Machine->devices[i].count;
	}

	add_exit_callback(Machine, image_exit);
	return INIT_PASS;
}



/*-------------------------------------------------
    image_exit - shut down the core image system
-------------------------------------------------*/

static void image_exit(running_machine *machine)
{
	int i, j, indx;
	mess_image *image;

	/* unload all devices */
	image_unload_all(FALSE);
	image_unload_all(TRUE);

	indx = 0;

	if (Machine->devices)
	{
		for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
		{
			for (j = 0; j < Machine->devices[i].count; j++)
			{
				/* identify the image */
				image = &images[indx + j];

				/* call the exit handler if appropriate */
				if (Machine->devices[i].exit)
					Machine->devices[i].exit(image);

				/* free the tagpool */
				tagpool_exit(&images[indx + j].tagpool);

				/* free the working directory */
				if (image->working_directory)
				{
					free(image->working_directory);
					image->working_directory = NULL;
				}

				/* free the memory pool */
				pool_free(image->mempool);
				image->mempool = NULL;
			}
			indx += Machine->devices[i].count;
		}
	}
}



/****************************************************************************
    IMAGE LOADING
****************************************************************************/

/*-------------------------------------------------
    set_image_filename - specifies the filename of
	an image
-------------------------------------------------*/

static image_error_t set_image_filename(mess_image *image, const char *filename, const char *zippath)
{
	image_error_t err = IMAGE_ERROR_SUCCESS;
	char *alloc_filename = NULL;
	char *new_name;
	char *new_dir;
	char *new_working_directory;
	int pos;

	/* create the directory string */
	new_dir = filename ? image_strdup(image, filename) : NULL;
	for (pos = strlen(new_dir); (pos > 0); pos--)
	{
		if (strchr(":\\/", new_dir[pos - 1]))
		{
			new_dir[pos] = '\0';
			break;
		}
	}

	/* do we have to concatenate the names? */
	if (zippath)
	{
		alloc_filename = assemble_3_strings(filename, PATH_SEPARATOR, zippath);
		filename = alloc_filename;
	}

	/* copy the string */
	new_name = image_strdup(image, filename);
	if (!new_name)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* copy the working directory */
	new_working_directory = mame_strdup(new_dir);
	if (!new_working_directory)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* set the new name and dir */
	if (image->name)
		image_freeptr(image, image->name);
	if (image->dir)
		image_freeptr(image, image->dir);
	if (image->working_directory)
		free(image->working_directory);
	image->name = new_name;
	image->dir = new_dir;
	image->working_directory = new_working_directory;

done:
	if (alloc_filename)
		free(alloc_filename);
	return err;
}



/*-------------------------------------------------
    is_loaded - quick check to determine whether an
	image is loaded
-------------------------------------------------*/

static int is_loaded(mess_image *image)
{
	return image->file || image->ptr;
}



/*-------------------------------------------------
    load_zip_path - loads a ZIP file with a
	specific path
-------------------------------------------------*/

static image_error_t load_zip_path(mess_image *image, const char *path)
{
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	zip_file *zip = NULL;
	zip_error ziperr;
	const zip_file_header *header;
	const char *zip_extension = ".zip";
	char *path_copy;
	const char *zip_entry;
	void *ptr;
	int pos;

	/* create our own copy of the path */
	path_copy = mame_strdup(path);
	if (!path_copy)
	{
		err = IMAGE_ERROR_OUTOFMEMORY;
		goto done;
	}

	/* loop through the path and try opening zip files */
	ziperr = ZIPERR_FILE_ERROR;
	zip_entry = NULL;
	pos = strlen(path_copy);
	while(pos > strlen(zip_extension))
	{
		/* is this a potential zip path? */
		if ((path_copy[pos] == '\0') || !strncmp(&path_copy[pos], PATH_SEPARATOR, strlen(PATH_SEPARATOR)))
		{
			/* parse out the zip path */
			if (path_copy[pos] == '\0')
			{
				/* no zip path */
				zip_entry = NULL;
			}
			else
			{
				/* we are at a zip path */
				path_copy[pos] = '\0';
				zip_entry = &path_copy[pos + strlen(PATH_SEPARATOR)];
			}

			/* try to open the zip file */
			ziperr = zip_file_open(path_copy, &zip);
			if (ziperr != ZIPERR_FILE_ERROR)
				break;

			/* restore the path if we changed */
			if (zip_entry)
				path_copy[pos] = PATH_SEPARATOR[0];
		}
		pos--;
	}

	/* did we succeed in opening up a zip file? */
	if (ziperr == ZIPERR_NONE)
	{
		/* iterate through the zip file */
		header = zip_file_first_file(zip);

		/* if we specified a zip partial path, find it */
		if (zip_entry)
		{
			while(header)
			{
				if (!mame_stricmp(header->filename, zip_entry))
					break;
				header = zip_file_next_file(zip);
			}
		}

		/* were we successful? */
		if (header)
		{
			/* if no zip path was specified, we have to change the name */
			if (!zip_entry)
			{
				/* use the first entry; tough part is we have to change the name */
				err = set_image_filename(image, image->name, header->filename);
				if (err)
					goto done;
			}

			/* allocate space for this zip file */
			ptr = image_malloc(image, header->uncompressed_length);
			if (!ptr)
			{
				err = IMAGE_ERROR_OUTOFMEMORY;
				goto done;
			}

			ziperr = zip_file_decompress(zip, ptr, header->uncompressed_length);
			if (ziperr == ZIPERR_NONE)
			{
				/* success! */
				err = IMAGE_ERROR_SUCCESS;
				image->ptr = ptr;
				image->length = header->uncompressed_length;
			}
		}
	}

done:
	if (path_copy)
		free(path_copy);
	if (zip)
		zip_file_close(zip);
	return err;
}



/*-------------------------------------------------
    load_image_by_path - loads an image with a
	specific path
-------------------------------------------------*/

static image_error_t load_image_by_path(mess_image *image, const char *software_path,
	const game_driver *gamedrv, UINT32 open_flags, const char *path)
{
	file_error filerr = FILERR_NOT_FOUND;
	image_error_t err = IMAGE_ERROR_FILENOTFOUND;
	char *full_path = NULL;
	const char *file_extension;

	/* assemble the path */
	if (software_path)
	{
		full_path = assemble_5_strings(software_path, PATH_SEPARATOR, gamedrv->name, PATH_SEPARATOR, path);
		path = full_path;
	}

	/* quick check to see if the file is a ZIP file */
	file_extension = strrchr(path, '.');
	if (!file_extension || mame_stricmp(file_extension, ".zip"))
	{
		filerr = osd_open(path, open_flags, &image->file, &image->length);

		/* did the open succeed? */
		switch(filerr)
		{
			case FILERR_NONE:
				/* success! */
				image->writeable = (open_flags & OPEN_FLAG_WRITE) ? 1 : 0;
				image->created = (open_flags & OPEN_FLAG_CREATE) ? 1 : 0;
				err = IMAGE_ERROR_SUCCESS;
				break;

			case FILERR_NOT_FOUND:
			case FILERR_ACCESS_DENIED:
				/* file not found (or otherwise cannot open); continue */
				err = IMAGE_ERROR_FILENOTFOUND;
				break;

			case FILERR_OUT_OF_MEMORY:
				/* out of memory */
				err = IMAGE_ERROR_OUTOFMEMORY;
				break;

			case FILERR_ALREADY_OPEN:
				/* this shouldn't happen */
				err = IMAGE_ERROR_ALREADYOPEN;
				break;

			case FILERR_FAILURE:
			case FILERR_TOO_MANY_FILES:
			case FILERR_INVALID_DATA:
			default:
				/* other errors */
				err = IMAGE_ERROR_INTERNAL;
				break;
		}
	}

	/* special case for ZIP files */
	if ((err == IMAGE_ERROR_FILENOTFOUND) && (open_flags == OPEN_FLAG_READ))
		err = load_zip_path(image, path);

	/* check to make sure that our reported error is reflective of the actual status */
	if (err)
		assert(!is_loaded(image));
	else
		assert(is_loaded(image));

	/* free up memory, and exit */
	if (full_path)
		free(full_path);
	return err;
}



/*-------------------------------------------------
    determine_open_plan - determines which open
	flags to use, and in what order
-------------------------------------------------*/

static void determine_open_plan(mess_image *image, int is_create, UINT32 *open_plan)
{
	unsigned int readable, writeable, creatable;
	int i = 0;

	if (image->dev->getdispositions)
	{
		image->dev->getdispositions(image->dev, image_index_in_device(image),
			&readable, &writeable, &creatable);
	}
	else
	{
		readable = image->dev->readable;
		writeable = image->dev->writeable;
		creatable = image->dev->creatable;
	}

	/* emit flags */
	if (!is_create && readable && writeable)
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE;
	if (!is_create && !readable && writeable)
		open_plan[i++] = OPEN_FLAG_WRITE;
	if (!is_create && readable)
		open_plan[i++] = OPEN_FLAG_READ;
	if (writeable && creatable)
		open_plan[i++] = OPEN_FLAG_READ | OPEN_FLAG_WRITE | OPEN_FLAG_CREATE;
	open_plan[i] = 0;
}



/*-------------------------------------------------
    image_load_internal - core image loading
-------------------------------------------------*/

static int image_load_internal(mess_image *image, const char *path,
	int is_create, int create_format, option_resolution *create_args)
{
	image_error_t err;
	const char *software_path;
	char *software_path_list = NULL;
	const void *buffer;
	const game_driver *gamedrv;
	UINT32 open_plan[4];
	int i;

	/* sanity checks */
	assert_always(image, "image_load(): image is NULL");
	assert_always(path, "image_load(): path is NULL");

	/* we are now loading */
	image->is_loading = 1;

	/* first unload the image */
	image_unload(image);

	/* record the filename */
	image->err = set_image_filename(image, path, NULL);
	if (image->err)
		goto done;

	/* do we need to reset the CPU? */
	if ((timer_get_time() > 0) && image->dev->reset_on_load)
		mame_schedule_soft_reset(Machine);

	/* determine open plan */
	determine_open_plan(image, is_create, open_plan);

	/* attempt to open the file in various ways */
	for (i = 0; !image->file && open_plan[i]; i++)
	{
		software_path = software_path_list;
		do
		{
			gamedrv = Machine->gamedrv;
			while(!is_loaded(image) && gamedrv)
			{
				/* open the file */
				image->err = load_image_by_path(image, software_path, gamedrv, open_plan[i], path);
				if (image->err && (image->err != IMAGE_ERROR_FILENOTFOUND))
					goto done;

				/* move on to the next driver */
				gamedrv = mess_next_compatible_driver(gamedrv);
			}

			/* move on to the next entry in the software path; if we can */
			if (software_path)
				software_path += strlen(software_path) + 1;
		}
		while(!is_loaded(image) && software_path && *software_path);
	}

	/* did we fail to find the file? */
	if (!is_loaded(image))
	{
		image->err = IMAGE_ERROR_FILENOTFOUND;
		goto done;
	}

	/* if applicable, call device verify */
	if (image->dev->imgverify && !image_has_been_created(image))
	{
		/* access the memory */
		buffer = image_ptr(image);
		if (!buffer)
		{
			image->err = IMAGE_ERROR_OUTOFMEMORY;
			goto done;
		}

		/* verify the file */
		err = image->dev->imgverify(buffer, (size_t) image->length);
		if (err)
		{
			image->err = IMAGE_ERROR_INVALIDIMAGE;
			goto done;
		}
	}

	/* call device load or create */
	if (image_has_been_created(image) && image->dev->create)
	{
		err = image->dev->create(image, create_format, create_args);
		if (err)
		{
			if (!image->err)
				image->err = IMAGE_ERROR_UNSPECIFIED;
			goto done;
		}
	}
	else if (image->dev->load)
	{
		/* using device load */
		err = image->dev->load(image);
		if (err)
		{
			if (!image->err)
				image->err = IMAGE_ERROR_UNSPECIFIED;
			goto done;
		}
	}

	/* success! */

done:
	if (software_path_list)
		free(software_path_list);
	if (image->err)
		image_clear(image);
	image->is_loading = 1;
	return image->err ? INIT_FAIL : INIT_PASS;
}



/*-------------------------------------------------
    image_load - load an image into MESS
-------------------------------------------------*/

int image_load(mess_image *image, const char *path)
{
	return image_load_internal(image, path, FALSE, 0, NULL);
}



/*-------------------------------------------------
    image_create - create a MESS image
-------------------------------------------------*/

int image_create(mess_image *image, const char *path, int create_format, option_resolution *create_args)
{
	return image_load_internal(image, path, TRUE, create_format, create_args);
}



/*-------------------------------------------------
    image_clear - clear all internal data pertaining
	to an image
-------------------------------------------------*/

static void image_clear(mess_image *image)
{
	if (image->file)
	{
		osd_close(image->file);
		image->file = NULL;
	}

	pool_clear(image->mempool);
	image->writeable = 0;
	image->created = 0;
	image->name = NULL;
	image->dir = NULL;
	image->hash = NULL;
	image->length = 0;
	image->pos = 0;
	image->longname = NULL;
	image->manufacturer = NULL;
	image->year = NULL;
	image->playable = NULL;
	image->extrainfo = NULL;
	image->basename_noext = NULL;
	image->err_message = NULL;
	image->ptr = NULL;
}



/*-------------------------------------------------
    image_unload_internal - internal call to unload
	images
-------------------------------------------------*/

static void image_unload_internal(mess_image *image, int is_final_unload)
{
	/* is there an actual image loaded? */
	if (!is_loaded(image))
		return;

	/* call the unload function */
	if (image->dev->unload)
		image->dev->unload(image);

	image_clear(image);
	image_clear_error(image);
}



/*-------------------------------------------------
    image_unload - main call to unload an image
-------------------------------------------------*/

void image_unload(mess_image *image)
{
	image_unload_internal(image, FALSE);
}



/*-------------------------------------------------
    image_unload_all - unload all images
-------------------------------------------------*/

void image_unload_all(int ispreload)
{
	int id;
	const struct IODevice *dev;
	mess_image *image;

	if (!ispreload)
		mess_options_extract();

	/* normalize ispreload */
	ispreload = ispreload ? 1 : 0;

	/* unload all devices with matching preload */
	if (Machine->devices)
	{
		for (dev = Machine->devices; dev->type < IO_COUNT; dev++)
		{
			if (dev->load_at_init == ispreload)
			{
				/* all instances */
				for (id = 0; id < dev->count; id++)
				{
					image = image_from_device_and_index(dev, id);

					/* unload this image */
					image_unload_internal(image, TRUE);
				}
			}
		}
	}
}



/****************************************************************************
    ERROR HANDLING
****************************************************************************/

/*-------------------------------------------------
    image_clear_error - clear out any specified
	error
-------------------------------------------------*/

static void image_clear_error(mess_image *image)
{
	image->err = IMAGE_ERROR_SUCCESS;
	if (image->err_message)
	{
		image_freeptr(image, image->err_message);
		image->err_message = NULL;
	}
}



/*-------------------------------------------------
    image_error - returns the error text for an image
	error
-------------------------------------------------*/

const char *image_error(mess_image *image)
{
	static const char *messages[] =
	{
		NULL,
		"Internal error",
		"Unsupported operation",
		"Out of memory",
		"File not found",
		"Invalid image",
		"File already open",
		"Unspecified error"
	};

	return image->err_message ? image->err_message : messages[image->err];
}



/*-------------------------------------------------
    image_seterror - specifies an error on an image
-------------------------------------------------*/

void image_seterror(mess_image *image, image_error_t err, const char *message)
{
	image_clear_error(image);
	image->err = err;
	if (message)
	{
		image->err_message = image_strdup(image, message);
	}
}



/****************************************************************************
  Tag management functions.
  
  When devices have private data structures that need to be associated with a
  device, it is recommended that image_alloctag() be called in the device
  init function.  If the allocation succeeds, then a pointer will be returned
  to a block of memory of the specified size that will last for the lifetime
  of the emulation.  This pointer can be retrieved with image_lookuptag().

  Note that since image_lookuptag() is used to index preallocated blocks of
  memory, image_lookuptag() cannot fail legally.  In fact, an assert will be
  raised if this happens
****************************************************************************/

void *image_alloctag(mess_image *img, const char *tag, size_t size)
{
	return tagpool_alloc(&img->tagpool, tag, size);
}



void *image_lookuptag(mess_image *img, const char *tag)
{
	return tagpool_lookup(&img->tagpool, tag);
}



/****************************************************************************
  Hash info loading

  If the hash is not checked and the relevant info not loaded, force that info
  to be loaded
****************************************************************************/

static int read_hash_config(const char *sysname, mess_image *image)
{
	hash_file *hashfile = NULL;
	const struct hash_info *info = NULL;

	/* open the hash file */
	hashfile = hashfile_open(sysname, FALSE, NULL);
	if (!hashfile)
		goto done;

	/* look up this entry in the hash file */
	info = hashfile_lookup(hashfile, image->hash);
	if (!info)
		goto done;

	/* copy the relevant entries */
	image->longname		= info->longname		? image_strdup(image, info->longname)		: NULL;
	image->manufacturer	= info->manufacturer	? image_strdup(image, info->manufacturer)	: NULL;
	image->year			= info->year			? image_strdup(image, info->year)			: NULL;
	image->playable		= info->playable		? image_strdup(image, info->playable)		: NULL;
	image->extrainfo	= info->extrainfo		? image_strdup(image, info->extrainfo)		: NULL;

done:
	if (hashfile)
		hashfile_close(hashfile);
	return !hashfile || !info;
}



static void run_hash(mess_image *image,
	void (*partialhash)(char *, const unsigned char *, unsigned long, unsigned int),
	char *dest, unsigned int hash_functions)
{
	UINT32 size;
	UINT8 *buf = NULL;

	*dest = '\0';
	size = (UINT32) image_length(image);

	buf = (UINT8 *) malloc_or_die(size);

	/* read the file */
	image_fseek(image, 0, SEEK_SET);
	image_fread(image, buf, size);

	if (partialhash)
		partialhash(dest, buf, size, hash_functions);
	else
		hash_compute(dest, buf, size, hash_functions);

	/* cleanup */
	free(buf);
	image_fseek(image, 0, SEEK_SET);
}



static int image_checkhash(mess_image *image)
{
	const game_driver *drv;
	const struct IODevice *dev;
	char hash_string[HASH_BUF_SIZE];
	int rc;

	/* this call should not be made when the image is not loaded */
	assert(is_loaded(image));

	/* only calculate CRC if it hasn't been calculated, and the open_mode is read only */
	if (!image->hash && !image->writeable && !image->created)
	{
		/* initialize key variables */
		dev = image_device(image);

		/* do not cause a linear read of 600 megs please */
		/* TODO: use SHA/MD5 in the CHD header as the hash */
		if (dev->type == IO_CDROM)
			return FALSE;

		run_hash(image, dev->partialhash, hash_string, HASH_CRC | HASH_MD5 | HASH_SHA1);

		image->hash = image_strdup(image, hash_string);

		/* now read the hash file */
		drv = Machine->gamedrv;
		do
		{
			rc = read_hash_config(drv->name, image);
			drv = mess_next_compatible_driver(drv);
		}
		while(rc && drv);
	}
	return TRUE;
}



/****************************************************************************
  Accessor functions

  These provide information about the device; and about the mounted image
****************************************************************************/

/*-------------------------------------------------
    image_device
-------------------------------------------------*/

const struct IODevice *image_device(mess_image *img)
{
	return img->dev;
}



/*-------------------------------------------------
    image_exists
-------------------------------------------------*/

int image_exists(mess_image *img)
{
	return image_filename(img) != NULL;
}



/*-------------------------------------------------
    image_slotexists
-------------------------------------------------*/

int image_slotexists(mess_image *img)
{
	return image_index_in_device(img) < image_device(img)->count;
}



/*-------------------------------------------------
    image_filename
-------------------------------------------------*/

const char *image_filename(mess_image *img)
{
	return img->name;
}



/*-------------------------------------------------
    image_basename
-------------------------------------------------*/

const char *image_basename(mess_image *img)
{
	return osd_basename((char *) image_filename(img));
}



/*-------------------------------------------------
    image_basename_noext
-------------------------------------------------*/

const char *image_basename_noext(mess_image *img)
{
	const char *s;
	char *ext;

	if (!img->basename_noext)
	{
		s = image_basename(img);
		if (s)
		{
			img->basename_noext = image_strdup(img, s);
			ext = strrchr(img->basename_noext, '.');
			if (ext)
				*ext = '\0';
		}
	}
	return img->basename_noext;
}



/*-------------------------------------------------
    image_filetype
-------------------------------------------------*/

const char *image_filetype(mess_image *img)
{
	const char *s;
	s = image_filename(img);
	if (s)
		s = strrchr(s, '.');
	return s ? s+1 : NULL;
}



/*-------------------------------------------------
    image_filedir
-------------------------------------------------*/

const char *image_filedir(mess_image *image)
{
	return image->dir;
}



/*-------------------------------------------------
    image_typename_id
-------------------------------------------------*/

const char *image_typename_id(mess_image *image)
{
	const struct IODevice *dev;
	int id;
	static char buf[64];

	dev = image_device(image);
	id = image_index_in_device(image);
	return dev->name(dev, id, buf, sizeof(buf) / sizeof(buf[0]));
}



/*-------------------------------------------------
    image_length
-------------------------------------------------*/

UINT64 image_length(mess_image *img)
{
	return img->length;
}



/*-------------------------------------------------
    image_hash
-------------------------------------------------*/

const char *image_hash(mess_image *img)
{
	image_checkhash(img);
	return img->hash;
}



/*-------------------------------------------------
    image_crc
-------------------------------------------------*/

UINT32 image_crc(mess_image *img)
{
	const char *hash_string;
	UINT32 crc = 0;
	
	hash_string = image_hash(img);
	if (hash_string)
		crc = hash_data_extract_crc32(hash_string);

	return crc;
}



/*-------------------------------------------------
    image_is_writable
-------------------------------------------------*/

int image_is_writable(mess_image *img)
{
	return img->writeable;
}



/*-------------------------------------------------
    image_has_been_created
-------------------------------------------------*/

int image_has_been_created(mess_image *img)
{
	return img->created;
}



/*-------------------------------------------------
    image_make_readonly
-------------------------------------------------*/

void image_make_readonly(mess_image *img)
{
	img->writeable = 0;
}



/*-------------------------------------------------
    image_fread
-------------------------------------------------*/

UINT32 image_fread(mess_image *image, void *buffer, UINT32 length)
{
	length = MIN(length, image->length - image->pos);

	if (image->file)
		osd_read(image->file, buffer, image->pos, length, &length);
	else if (image->ptr)
		memcpy(buffer, ((UINT8 *) image->ptr) + image->pos, length);
	else
		length = 0;

	image->pos += length;
	return length;
}



/*-------------------------------------------------
    image_fwrite
-------------------------------------------------*/

UINT32 image_fwrite(mess_image *image, const void *buffer, UINT32 length)
{
	/* if we are not associated with a file, clip the length */
	if (!image->file)
		length = MIN(length, image->length - image->pos);

	if (image->file)
	{
		osd_write(image->file, buffer, image->pos, length, &length);

		/* since we've written to the file, we may need to invalidate the pointer */
		if (image->ptr)
		{
			image_freeptr(image, image->ptr);
			image->ptr = NULL;
		}
	}
	else if (image->ptr)
	{
		memcpy(((UINT8 *) image->ptr) + image->pos, buffer, length);
	}
	else
		length = 0;

	image->pos += length;

	/* did we grow the file? */
	if (image->length < image->pos)
		image->length = image->pos;

	/* return */
	return length;
}



/*-------------------------------------------------
    image_fseek
-------------------------------------------------*/

int image_fseek(mess_image *image, INT64 offset, int whence)
{
	switch(whence)
	{
		case SEEK_SET:
			image->pos = offset;
			break;
		case SEEK_CUR:
			image->pos += offset;
			break;
		case SEEK_END:
			image->pos = image->length + offset;
			break;
	}
	if (image->pos > image->length)
		image->pos = image->length;
	return 0;
}



/*-------------------------------------------------
    image_ftell
-------------------------------------------------*/

UINT64 image_ftell(mess_image *image)
{
	return image->pos;
}



/*-------------------------------------------------
    image_fgetc
-------------------------------------------------*/

int image_fgetc(mess_image *image)
{
	char ch;
	if (image_fread(image, &ch, 1) != 1)
		ch = '\0';
	return ch;
}



/*-------------------------------------------------
    image_feof
-------------------------------------------------*/

int image_feof(mess_image *image)
{
	return image->pos >= image->length;
}



/*-------------------------------------------------
    image_ptr
-------------------------------------------------*/

void *image_ptr(mess_image *image)
{
	UINT64 size;
	UINT64 pos;
	void *ptr;

	if (!image->ptr)
	{
		/* get the image size; bomb out if too big */
		size = image_length(image);
		if (size != (UINT32) size)
			return NULL;

		/* allocate the memory */
		ptr = image_malloc(image, (UINT32) size);

		/* save current position */
		pos = image_ftell(image);

		/* read all data */
		image_fseek(image, 0, SEEK_SET);
		if (image_fread(image, ptr, (UINT32) size) != size)
			return NULL;

		/* reset position */
		image_fseek(image, pos, SEEK_SET);

		/* success */
		image->ptr = ptr;
	}
	return image->ptr;
}



/***************************************************************************
    WORKING DIRECTORIES
***************************************************************************/

/*-------------------------------------------------
    try_change_working_directory - tries to change
	the working directory, but only if the directory
	actually exists
-------------------------------------------------*/

static int try_change_working_directory(mess_image *image, const char *subdir)
{
	osd_directory *directory;
	const osd_directory_entry *entry;
	int success = FALSE;
	int done = FALSE;
	char *new_working_directory;

	directory = osd_opendir(image->working_directory);
	if (directory != NULL)
	{
		while(!done && (entry = osd_readdir(directory)) != NULL)
		{
			if (!mame_stricmp(subdir, entry->name))
			{
				done = TRUE;
				success = entry->type == ENTTYPE_DIR;
			}
		}

		osd_closedir(directory);
	}

	/* did we successfully identify the directory? */
	if (success)
	{
		new_working_directory = malloc_or_die(strlen(image->working_directory)
			+ strlen(PATH_SEPARATOR) + strlen(subdir) + strlen(PATH_SEPARATOR) + 1);
		strcpy(new_working_directory, image->working_directory);

		/* remove final path separator, if present */
		if ((strlen(new_working_directory) >= strlen(PATH_SEPARATOR))
			&& !strcmp(new_working_directory + strlen(new_working_directory) - strlen(PATH_SEPARATOR), PATH_SEPARATOR))
		{
			new_working_directory[strlen(new_working_directory) - strlen(PATH_SEPARATOR)] = '\0';
		}

		strcat(new_working_directory, PATH_SEPARATOR);
		strcat(new_working_directory, subdir);
		strcat(new_working_directory, PATH_SEPARATOR);

		free(image->working_directory);
		image->working_directory = new_working_directory;
	}
	return success;
}



/*-------------------------------------------------
    setup_working_directory - sets up the working
	directory according to a few defaults
-------------------------------------------------*/

static void setup_working_directory(mess_image *image)
{
	char mess_directory[1024];
	const game_driver *gamedrv;

	/* first set up the working directory to be the MESS directory */
	osd_get_emulator_directory(mess_directory, ARRAY_LENGTH(mess_directory));
	image->working_directory = mame_strdup(mess_directory);
	
	/* now try browsing down to "software" */
	if (try_change_working_directory(image, "software"))
	{
		/* now down to a directory for this computer */
		gamedrv = Machine->gamedrv;
		while(gamedrv && !try_change_working_directory(image, gamedrv->name))
		{
			gamedrv = mess_next_compatible_driver(gamedrv);
		}
	}
}



/*-------------------------------------------------
    image_working_directory - returns the working
	directory to use for this image; this is
	valid even if not mounted
-------------------------------------------------*/

const char *image_working_directory(mess_image *image)
{
	/* check to see if we've never initialized the working directory */
	if (image->working_directory == NULL)
		setup_working_directory(image);

	return image->working_directory;
}



/*-------------------------------------------------
    image_set_working_directory - sets the working
	directory to use for this image
-------------------------------------------------*/

void image_set_working_directory(mess_image *image, const char *working_directory)
{
	char *new_working_directory = mame_strdup(working_directory);
	if (image->working_directory)
		free(image->working_directory);
	image->working_directory = new_working_directory;
}



/****************************************************************************
  Memory allocators

  These allow memory to be allocated for the lifetime of a mounted image.
  If these (and the above accessors) are used well enough, they should be
  able to eliminate the need for a unload function.
****************************************************************************/

void *image_malloc(mess_image *image, size_t size)
{
	assert(is_loaded(image) || image->is_loading);
	return pool_malloc(image->mempool, size);
}



void *image_realloc(mess_image *image, void *ptr, size_t size)
{
	assert(is_loaded(image) || image->is_loading);
	return pool_realloc(image->mempool, ptr, size);
}



char *image_strdup(mess_image *image, const char *src)
{
	assert(is_loaded(image) || image->is_loading);
	return pool_strdup(image->mempool, src);
}



void image_freeptr(mess_image *image, void *ptr)
{
	/* should really do something better here */
	pool_realloc(image->mempool, ptr, 0);
}



/****************************************************************************
  CRC Accessor functions

  When an image is mounted; these functions provide access to the information
  pertaining to that image in the CRC database
****************************************************************************/

const char *image_longname(mess_image *img)
{
	image_checkhash(img);
	return img->longname;
}



const char *image_manufacturer(mess_image *img)
{
	image_checkhash(img);
	return img->manufacturer;
}



const char *image_year(mess_image *img)
{
	image_checkhash(img);
	return img->year;
}



const char *image_playable(mess_image *img)
{
	image_checkhash(img);
	return img->playable;
}



const char *image_extrainfo(mess_image *img)
{
	image_checkhash(img);
	return img->extrainfo;
}



/****************************************************************************
  Battery functions

  These functions provide transparent access to battery-backed RAM on an
  image; typically for cartridges.
****************************************************************************/

/*-------------------------------------------------
    open_battery_file - opens the battery backed
	NVRAM file for an image
-------------------------------------------------*/

static file_error open_battery_file(mess_image *image, UINT32 openflags, mame_file **file)
{
	file_error filerr;
	char *basename_noext;
	char *fname;

	basename_noext = strip_extension(image_basename(image));
	if (!basename_noext)
		return FILERR_OUT_OF_MEMORY;
	fname = assemble_4_strings(Machine->gamedrv->name, PATH_SEPARATOR, basename_noext, ".nv");
	filerr = mame_fopen(SEARCHPATH_NVRAM, fname, openflags, file);
	free(fname);
	free(basename_noext);
	return filerr;
}



/*-------------------------------------------------
    image_battery_load - retrieves the battery
	backed RAM for an image
-------------------------------------------------*/

void image_battery_load(mess_image *image, void *buffer, int length)
{
	file_error filerr;
	mame_file *file;
	int bytes_read = 0;

	assert_always(buffer && (length > 0), "Must specify sensical buffer/length");

	/* try to open the battery file and read it in, if possible */
	filerr = open_battery_file(image, OPEN_FLAG_READ, &file);
	if (filerr == FILERR_NONE)
	{
		bytes_read = mame_fread(file, buffer, length);
		mame_fclose(file);
	}

	/* fill remaining bytes (if necessary) */
	memset(((char *) buffer) + bytes_read, '\0', length - bytes_read);
}



/*-------------------------------------------------
    image_battery_save - stores the battery
	backed RAM for an image
-------------------------------------------------*/

void image_battery_save(mess_image *image, const void *buffer, int length)
{
	file_error filerr;
	mame_file *file;

	assert_always(buffer && (length > 0), "Must specify sensical buffer/length");

	/* try to open the battery file and write it out, if possible */
	filerr = open_battery_file(image, OPEN_FLAG_WRITE | OPEN_FLAG_CREATE | OPEN_FLAG_CREATE_PATHS, &file);
	if (filerr == FILERR_NONE)
	{
		mame_fwrite(file, buffer, length);
		mame_fclose(file);
	}
}



/****************************************************************************
  Indexing functions

  These provide various ways of indexing images
****************************************************************************/

int image_absolute_index(mess_image *image)
{
	return image - images;
}



mess_image *image_from_absolute_index(int absolute_index)
{
	return &images[absolute_index];
}



/****************************************************************************
  Deprecated functions

  The usage of these functions is to be phased out.  The first group because
  they reflect the outdated fixed relationship between devices and their
  type/id.
****************************************************************************/

mess_image *image_from_device_and_index(const struct IODevice *dev, int id)
{
	int indx, i;
	mess_image *image = NULL;

	assert(id < dev->count);
	assert(images);

	indx = 0;
	for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
	{
		if (dev == &Machine->devices[i])
		{
			image = &images[indx + id];
			break;
		}
		indx += Machine->devices[i].count;
	}

	assert(image);
	return image;
}



mess_image *image_from_devtag_and_index(const char *devtag, int id)
{
	int indx, i;
	mess_image *image = NULL;

	indx = 0;
	for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
	{
		if (Machine->devices[i].tag && !strcmp(Machine->devices[i].tag, devtag))
		{
			image = &images[indx + id];
			break;
		}
		indx += Machine->devices[i].count;
	}

	assert(image);
	return image;
}



mess_image *image_from_devtype_and_index(iodevice_t type, int id)
{
	int indx, i;
	mess_image *image = NULL;

	assert((multiple_dev_mask & (1 << type)) == 0);
	assert(id < device_count(type));

	indx = 0;
	for (i = 0; Machine->devices[i].type < IO_COUNT; i++)
	{
		if (type == Machine->devices[i].type)
		{
			image = &images[indx + id];
			break;
		}
		indx += Machine->devices[i].count;
	}

	assert(image);
	return image;
}



iodevice_t image_devtype(mess_image *img)
{
	return img->dev->type;
}



int image_index_in_device(mess_image *img)
{
	int indx;

	assert(img);
	indx = img - image_from_device_and_index(img->dev, 0);

	assert(indx >= 0);
	assert(indx < img->dev->count);
	return indx;
}
