/***********************************************************************

	apf.c

	Functions to emulate general aspects of the machine (RAM, ROM,
	interrupts, I/O ports)

***********************************************************************/

#include "driver.h"
#include "includes/apf.h"
#include "cassette.h"
#include "formats/apfapt.h"
#include "includes/basicdsk.h"

#if 0
int apf_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}
#endif

int apf_cassette_init(int id)
{
	void *file;
	struct wave_args wa;

	if (device_filename(IO_CASSETTE, id)==NULL)
		return INIT_PASS;


	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);
	if( file )
	{
		int apf_apt_size;

		/* get size of .tap file */
		apf_apt_size = osd_fsize(file);

		logerror("apf .apt size: %04x\n",apf_apt_size);

		if (apf_apt_size!=0)
		{
			UINT8 *apf_apt_data;

			/* allocate a temporary buffer to hold .apt image */
			/* this is used to calculate the number of samples that would be filled when this
			file is converted */
			apf_apt_data = (UINT8 *)malloc(apf_apt_size);

			if (apf_apt_data!=NULL)
			{
				/* number of samples to generate */
				int size_in_samples;

				/* read data into temporary buffer */
				osd_fread(file, apf_apt_data, apf_apt_size);

				/* calculate size in samples */
				size_in_samples = apf_cassette_calculate_size_in_samples(apf_apt_size, apf_apt_data);

				/* seek back to start */
				osd_fseek(file, 0, SEEK_SET);

				/* free temporary buffer */
				free(apf_apt_data);

				/* size of data in samples */
				logerror("size in samples: %d\n",size_in_samples);

				/* internal calculation used in wave.c:

				length =
					wa->header_samples +
					((osd_fsize(w->file) + wa->chunk_size - 1) / wa->chunk_size) * wa->chunk_samples +
					wa->trailer_samples;
				*/


				memset(&wa, 0, sizeof(&wa));
				wa.file = file;
				wa.chunk_size = apf_apt_size;
				wa.chunk_samples = size_in_samples;
                wa.smpfreq = APF_WAV_FREQUENCY;
				wa.fill_wave = apf_cassette_fill_wave;
				wa.header_samples = 0;
				wa.trailer_samples = 0;
				wa.display = 1;
				if( device_open(IO_CASSETTE,id,0,&wa) )
					return INIT_FAIL;

				return INIT_PASS;
			}

			return INIT_FAIL;
		}
	}

	file = image_fopen(IO_CASSETTE, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW_CREATE);
	if( file )
    {
		memset(&wa, 0, sizeof(&wa));
		wa.file = file;
		wa.display = 1;
		wa.smpfreq = 22050;
		if( device_open(IO_CASSETTE,id,1,&wa) )
            return INIT_FAIL;

		return INIT_PASS;
    }

	return INIT_FAIL;
}

void apf_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


/* 256 bytes per sector, single sided, single density, 40 track  */
int apfimag_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY, id)==NULL)
		return INIT_PASS;

	if (strlen(device_filename(IO_FLOPPY, id))==0)
		return INIT_PASS;

	if (basicdsk_floppy_init(id)==INIT_PASS)
	{
		basicdsk_set_geometry(id, 40, 1, 8, 256, 1, 0);
		return INIT_PASS;
	}

	return INIT_FAIL;
}
