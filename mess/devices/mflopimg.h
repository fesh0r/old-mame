/*********************************************************************

	mflopimg.h

	MESS interface to the floppy disk image abstraction code

*********************************************************************/

#ifndef MFLOPIMG_H
#define MFLOPIMG_H

#include "devices/flopdrv.h"
#include "formats/flopimg.h"

const struct IODevice *floppy_device_specify(struct IODevice *iodev, char *extbuf, size_t extbuflen,
	int count, const struct FloppyFormat *floppy_options);

/* hack for apple II; replace this when we think of something better */
void floppy_install_unload_proc(mess_image *image, void (*proc)(mess_image *image));

/* hack for TI99; replace this when we think of something better */
void floppy_install_tracktranslate_proc(mess_image *image, int (*proc)(mess_image *image, floppy_image *floppy, int physical_track));

#define CONFIG_DEVICE_FLOPPY(count, floppy_options)							\
	if (cfg->device_num-- == 0)												\
	{																		\
		static struct IODevice iodev;										\
		static char extbuf[33];												\
		cfg->dev = floppy_device_specify(&iodev, extbuf, sizeof(extbuf) /	\
			sizeof(extbuf[0]),	count, floppyoptions_##floppy_options);		\
	}																		\

#endif /* MFLOPIMG_H */



