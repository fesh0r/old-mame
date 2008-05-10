#ifndef MFMDISK_H
#define MFMDISK_H

#include "device.h"
#include "image.h"
#include "flopdrv.h"


extern const floppy_interface mfm_disk_floppy_interface;

DEVICE_IMAGE_LOAD( mfm_disk );


#endif /* MFMDISK_H */
