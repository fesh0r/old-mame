/***************************************************************************

  dsk.c

 CPCEMU standard and extended disk image support.
 Used on Amstrad CPC and Spectrum +3 drivers.

 KT - 27/2/00 - Moved Disk Image handling code into this file
							- Fixed a few bugs
							- Cleaned code up a bit
***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "includes/flopdrv.h"
#include "includes/dsk.h"
/* disk image and extended disk image support code */
/* supports up to 84 tracks and 2 sides */

#define dsk_MAX_TRACKS 84
#define dsk_MAX_SIDES	2
#define dsk_NUM_DRIVES 4
#define dsk_SECTORS_PER_TRACK	20

typedef struct
{
	unsigned char *data; /* the whole image data */
	unsigned long track_offsets[dsk_MAX_TRACKS*dsk_MAX_SIDES]; /* offset within data for each track */
	unsigned long sector_offsets[dsk_SECTORS_PER_TRACK]; /* offset within current track for sector data */
	int current_track;		/* current track */
	int disk_image_type;  /* image type: standard or extended */
} dsk_drive;

floppy_interface dsk_floppy_interface=
{
	dsk_seek_callback,
	dsk_get_sectors_per_track,
	dsk_get_id_callback,
	dsk_read_sector_data_into_buffer,
	dsk_write_sector_data_from_buffer,
	NULL,
	NULL
};

static void dsk_disk_image_init(dsk_drive *);

static dsk_drive drives[dsk_NUM_DRIVES]; /* the drives */

/* load image */
int dsk_load(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

static int dsk_floppy_verify(UINT8 *diskimage_data)
{
	if ( (memcmp(diskimage_data, "MV - CPC", 8)==0) || 	/* standard disk image? */
		 (memcmp(diskimage_data, "EXTENDED", 8)==0))	/* extended disk image? */
	{
		return IMAGE_VERIFY_PASS;
	}
	return IMAGE_VERIFY_FAIL;
}


/* load floppy */
int dsk_floppy_load(int id)
{
	dsk_drive *thedrive = &drives[id];

	/* load disk image */
	if (dsk_load(IO_FLOPPY,id,&thedrive->data))
	{
		if (thedrive->data)
		{
			dsk_disk_image_init(thedrive); /* initialise dsk */
            floppy_drive_set_disk_image_interface(id,&dsk_floppy_interface);
            if(dsk_floppy_verify(thedrive->data) == IMAGE_VERIFY_PASS)
            	return INIT_PASS;
            else
            	return INIT_PASS;
		}
	}

	return INIT_PASS;
}

int dsk_save(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			data = *ptr;
			if (data!=NULL)
			{
				osd_fwrite(file, data, datasize);

				/* close file */
				osd_fclose(file);

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	};

	return 0;
}


void dsk_floppy_exit(int id)
{
	dsk_drive *thedrive = &drives[id];

	if (thedrive->data!=NULL)
	{
		dsk_save(IO_FLOPPY,id,&thedrive->data);
		free(thedrive->data);
	}
	thedrive->data = NULL;
}



void dsk_dsk_init_track_offsets(dsk_drive *thedrive)
{
	int track_offset;
	int i;
	int track_size;
	int tracks, sides;
	int skip, length,offs;
	unsigned char *file_loaded = thedrive->data;


	/* get size of each track from main header. Size of each
	track includes a 0x0100 byte header, and the actual sector data for
	all sectors on the track */
	track_size = file_loaded[0x032] | (file_loaded[0x033]<<8);

	/* main header is 0x0100 in size */
	track_offset = 0x0100;

	sides = file_loaded[0x031];
	tracks = file_loaded[0x030];


	/* single sided? */
	if (sides==1)
	{
		skip = 2;
		length = tracks;
	}
	else
	{
		skip = 1;
		length = tracks*sides;
	}

	offs = 0;
	for (i=0; i<length; i++)
	{
		thedrive->track_offsets[offs] = track_offset;
		track_offset+=track_size;
		offs+=skip;
	}

}

void dsk_dsk_init_sector_offsets(dsk_drive *thedrive,int track,int side)
{
	int track_offset;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = thedrive->track_offsets[(track<<1) + side];

	if (track_offset!=0)
	{
		int spt;
		int sector_offset;
		int sector_size;
		int i;

		unsigned char *track_header;

		track_header= &thedrive->data[track_offset];

		/* sectors per track as specified in nec765 format command */
		/* sectors on this track */
		spt = track_header[0x015];

		sector_size = (1<<(track_header[0x014]+7));

		/* track header is 0x0100 bytes in size */
		sector_offset = 0x0100;

		for (i=0; i<spt; i++)
		{
			thedrive->sector_offsets[i] = sector_offset;
			sector_offset+=sector_size;
		}
	}
}

void	 dsk_extended_dsk_init_track_offsets(dsk_drive *thedrive)
{
	int track_offset;
	int i;
	int track_size;
	int tracks, sides;
	int offs, skip, length;
	unsigned char *file_loaded = thedrive->data;

	sides = file_loaded[0x031];
	tracks = file_loaded[0x030];

	if (sides==1)
	{
		skip = 2;
		length = tracks;
	}
	else
	{
		skip = 1;
		length = tracks*sides;
	}

	/* main header is 0x0100 in size */
	track_offset = 0x0100;
	offs = 0;
	for (i=0; i<length; i++)
	{
		int track_size_high_byte;

		/* track size is specified as a byte, and is multiplied
		by 256 to get size in bytes. If 0, track doesn't exist and
		is unformatted, otherwise it exists. Track size includes 0x0100
		header */
		track_size_high_byte = file_loaded[0x034 + i];

		if (track_size_high_byte != 0)
		{
			/* formatted track */
			track_size = track_size_high_byte<<8;

			thedrive->track_offsets[offs] = track_offset;
			track_offset+=track_size;
		}

		offs+=skip;
	}
}


void dsk_extended_dsk_init_sector_offsets(dsk_drive *thedrive,int track,int side)
{
	int track_offset;

	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = thedrive->track_offsets[(track<<1) + side];

	if (track_offset!=0)
	{
		int spt;
		int sector_offset;
		int sector_size;
		int i;
		unsigned char *id_info;
		unsigned char *track_header;

		track_header= &thedrive->data[track_offset];

		/* sectors per track as specified in nec765 format command */
		/* sectors on this track */
		spt = track_header[0x015];

		id_info = track_header + 0x018;

		/* track header is 0x0100 bytes in size */
		sector_offset = 0x0100;

		for (i=0; i<spt; i++)
		{
                        sector_size = id_info[(i<<3) + 6] + (id_info[(i<<3) + 7]<<8);

			thedrive->sector_offsets[i] = sector_offset;
			sector_offset+=sector_size;
		}
	}
}



void dsk_disk_image_init(dsk_drive *thedrive)
{
	/*-----------------27/02/00 11:26-------------------
	 clear offsets
	--------------------------------------------------*/
	memset(&thedrive->track_offsets[0], 0, dsk_MAX_TRACKS*dsk_MAX_SIDES*sizeof(unsigned long));
	memset(&thedrive->sector_offsets[0], 0, 20*sizeof(unsigned long));

	if (memcmp(thedrive->data,"MV - CPC",8)==0)
	{
		thedrive->disk_image_type = 0;

		/* standard disk image */
		dsk_dsk_init_track_offsets(thedrive);

	}
	else
	if (memcmp(thedrive->data,"EXTENDED",8)==0)
	{
		thedrive->disk_image_type = 1;

		/* extended disk image */
		dsk_extended_dsk_init_track_offsets(thedrive);
	}
}


void dsk_seek_callback(int drive, int physical_track)
{
	drive = drive & 0x03;
	drives[drive].current_track = physical_track;
}

static int get_track_offset(int drive, int side)
{
	dsk_drive *thedrive;

	drive = drive & 0x03;
	side = side & 0x01;

	thedrive = &drives[drive];

	return thedrive->track_offsets[(thedrive->current_track<<1) + side];
}

static unsigned char *get_floppy_data(int drive)
{
	drive = drive & 0x03;
	return drives[drive].data;
}

void dsk_get_id_callback(int drive, chrn_id *id, int id_index, int side)
{
	int id_offset;
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	drive = drive & 0x03;
	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(drive, side);

	/* track exists? */
	if (track_offset==0)
		return;

	/* yes */
	data = get_floppy_data(drive);

	if (data==0)
		return;

	track_header = data + track_offset;

	id_offset = 0x018 + (id_index<<3);

	id->C = track_header[id_offset + 0];
	id->H = track_header[id_offset + 1];
	id->R = track_header[id_offset + 2];
	id->N = track_header[id_offset + 3];
	id->flags = 0;
	id->data_id = id_index;

	if (track_header[id_offset + 5] & 0x040)
	{
		id->flags |= ID_FLAG_DELETED_DATA;
	}




//	id->ST0 = track_header[id_offset + 4];
//	id->ST1 = track_header[id_offset + 5];

}


static void dsk_set_ddam(int drive, int id_index, int side, int ddam)
{
	int id_offset;
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	drive = drive & 0x03;
	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(drive, side);

	/* track exists? */
	if (track_offset==0)
		return;

	/* yes */
	data = get_floppy_data(drive);

	if (data==0)
		return;

	track_header = data + track_offset;

	id_offset = 0x018 + (id_index<<3);

	track_header[id_offset + 5] &= ~0x040;

	if (ddam)
	{
		track_header[id_offset + 5] |= 0x040;
	}
}


char * dsk_get_sector_ptr_callback(int drive, int sector_index, int side)
{
	int track_offset;
	int sector_offset;
	int track;
	dsk_drive *thedrive;
	unsigned char *data;

	drive = drive & 0x03;
	side = side & 0x01;

	thedrive = &drives[drive];

	track = thedrive->current_track;

	/* offset to track header in image */
	track_offset = get_track_offset(drive, side);

	/* track exists? */
	if (track_offset==0)
		return NULL;


	/* setup sector offsets */
	switch (thedrive->disk_image_type)
	{
	case 0:
		dsk_dsk_init_sector_offsets(thedrive,track, side);
		break;


	case 1:
		dsk_extended_dsk_init_sector_offsets(thedrive, track, side);
		break;

	default:
		break;
	}

	sector_offset = thedrive->sector_offsets[sector_index];

	data = get_floppy_data(drive);

	if (data==0)
		return NULL;

	return (char *)(data + track_offset + sector_offset);
}

void dsk_write_sector_data_from_buffer(int drive, int side, int index1, char *ptr, int length, int ddam)
{
	char * pSectorData;

	pSectorData = dsk_get_sector_ptr_callback(drive, index1, side);

	if (pSectorData!=NULL)
	{
		memcpy(pSectorData, ptr, length);
	}

	/* set ddam */
	dsk_set_ddam(drive, index1, side,ddam);
}

void dsk_read_sector_data_into_buffer(int drive, int side, int index1, char *ptr, int length)
{
	char *pSectorData;

	pSectorData = dsk_get_sector_ptr_callback(drive, index1, side);

	if (pSectorData!=NULL)
	{
		memcpy(ptr, pSectorData, length);

	}
}

int    dsk_get_sectors_per_track(int drive, int side)
{
	int track_offset;
	unsigned char *track_header;
	unsigned char *data;

	drive = drive & 0x03;
	side = side & 0x01;

	/* get offset to track header in image */
	track_offset = get_track_offset(drive, side);

	/* track exists? */
	if (track_offset==0)
		return 0;

	data = get_floppy_data(drive);

	if (data==0)
		return 0;

	/* yes, get sectors per track */
	track_header = data  + track_offset;

	return track_header[0x015];
}

