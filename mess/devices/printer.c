/*
 * printer.c : simple printer port implementation
 * "online/offline" and output only.
 */

#include "driver.h"
#include "devices/printer.h"
#include "image.h"



int printer_status(mess_image *img, int newstatus)
{
	/* if there is a file attached to it, it's online */
	return image_exists(img) != 0;
}



void printer_output(mess_image *img, int data)
{
	UINT8 d = data;
	mame_file *fp;

	fp = image_fp(img);
	if (fp)
		mame_fwrite(fp, &d, 1);
}



void printer_device_getinfo(struct IODevice *iodev)
{
	iodev->type = IO_PRINTER;
	iodev->file_extensions = "prn\0";
	iodev->readable = 0;
	iodev->writeable = 1;
	iodev->creatable = 1;
}


