#include "devices/snapquik.h"
#include "mess.h"

/* ----------------------------------------------------------------------- */

struct snapquick_info
{
	const struct IODevice *dev;
	mess_image *img;
	mame_file *fp;
	int file_size;
	struct snapquick_info *next;
};

static struct snapquick_info *snapquick_infolist;

static void snapquick_processsnapshot(int arg)
{
	struct snapquick_info *si;
	snapquick_loadproc loadproc;
	const char *file_type;
	
	si = (struct snapquick_info *) arg;
	loadproc = (snapquick_loadproc) si->dev->user1;
	file_type = image_filetype(si->img);
	loadproc(si->fp, file_type, si->file_size);
	image_unload(si->img);
}

static int snapquick_load(mess_image *img, mame_file *fp, int open_mode)
{
	const struct IODevice *dev;
	struct snapquick_info *si;
	double delay;
	int file_size;

	if (fp)
	{
		file_size = mame_fsize(fp);
		if (file_size <= 0)
			return INIT_FAIL;

		si = (struct snapquick_info *) image_malloc(img, sizeof(struct snapquick_info));
		if (!si)
			return INIT_FAIL;

		dev = image_device(img);
		assert(dev);

		si->dev = dev;
		si->img = img;
		si->fp = fp;
		si->file_size = file_size;
		si->next = snapquick_infolist;
		snapquick_infolist = si;

		delay = ((double) (UINT32) dev->user2) / 65536.0;

		timer_set(delay, (int) si, snapquick_processsnapshot);
	}
	return INIT_PASS;
}

static void snapquick_unload(mess_image *img)
{
	struct snapquick_info **si = &snapquick_infolist;

	while(*si && ((*si)->img != img))
		si = &(*si)->next;
	if (*si)
		*si = (*si)->next;
}

/* ----------------------------------------------------------------------- */

const struct IODevice *snapquick_specify(struct IODevice *iodev, int type,
	const char *file_extensions, snapquick_loadproc loadproc, double delay)
{
	memset(iodev, 0, sizeof(*iodev));
	iodev->type = type;
	iodev->count = 1;
	iodev->file_extensions = file_extensions;
	iodev->flags = DEVICE_LOAD_RESETS_NONE;
	iodev->open_mode = OSD_FOPEN_READ;
	iodev->load = snapquick_load;
	iodev->unload = snapquick_unload;
	iodev->user1 = (void *) loadproc;
	iodev->user2 = (void *) (UINT32) (delay * 65536);
	return iodev;
}

