/***************************************************************************

	wave.c

	Functions to handle loading, creation, recording and playback
	of wave samples for IO_CASSETTE

****************************************************************************/
#include "driver.h"

/* Our private wave file structure */
struct wave_file {
	int channel;			/* channel for playback */
	void *file; 			/* osd file handle */
	int mode;				/* write mode? */
	int (*fill_wave)(INT16 *,int,UINT8*);
	void *timer;			/* timer (TIME_NEVER) for reading sample values */
	INT16 play_sample;		/* current sample value for playback */
	INT16 record_sample;	/* current sample value for playback */
	int offset; 			/* offset set by device_seek function */
	int play_pos;			/* sample position for playback */
	int record_pos; 		/* sample position for recording */
	int counter;			/* sample fraction counter for playback */
	int smpfreq;			/* sample frequency from the WAV header */
	int resolution; 		/* sample resolution in bits/sample (8 or 16) */
	int max_samples;		/* number of samples that could be stored in data */
	int samples;
	int length; 			/* length in bytes (max_samples * resolution / 8) */
	void *data; 			/* sample data */
	int status;				/* other status (mute, motor inhibit) */
};

static struct Wave_interface *intf;
static struct wave_file wave[MAX_WAVE] = {{-1,},{-1,}};

static void wave_update_output_buffer(int id);


#define WAVE_OK    0
#define WAVE_ERR   1
#define WAVE_FMT   2

/*****************************************************************************
 * helper functions
 *****************************************************************************/
/*
	load an existing wave file from disk to memory
*/
static int wave_read(int id)
{
	struct wave_file *w = &wave[id];
    UINT32 offset = 0;
	UINT32 filesize, temp32;
	UINT16 channels, blockAlign, bitsPerSample, temp16;
	unsigned sample_padding;
	char buf[32];

	if( !w->file )
		return WAVE_ERR;

    /* read the core header and make sure it's a WAVE file */
	offset += osd_fread(w->file, buf, 4);
	if( offset < 4 )
	{
		logerror("WAVE read error at offs %d\n", offset);
		return WAVE_ERR;
	}
	if( memcmp (&buf[0], "RIFF", 4) != 0 )
	{
		logerror("WAVE header not 'RIFF'\n");
		return WAVE_FMT;
    }

	/* get the total size */
	offset += osd_fread(w->file, &temp32, 4);
	if( offset < 8 )
	{
		logerror("WAVE read error at offs %d\n", offset);
		return WAVE_ERR;
	}
	filesize = LITTLE_ENDIANIZE_INT32(temp32);
	logerror("WAVE filesize %u bytes\n", filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fread(w->file, buf, 4);
	if( offset < 12 )
	{
		logerror("WAVE read error at offs %d\n", offset);
		return WAVE_ERR;
	}
	if( memcmp (&buf[0], "WAVE", 4) != 0 )
	{
		logerror("WAVE RIFF type not 'WAVE'\n");
		return WAVE_FMT;
	}

	/* seek until we find a format tag */
	while( 1 )
	{
		offset += osd_fread(w->file, buf, 4);
		offset += osd_fread(w->file, &temp32, 4);
		w->length = LITTLE_ENDIANIZE_INT32(temp32);
		if( memcmp(&buf[0], "fmt ", 4) == 0 )
			break;

		/* seek to the next block */
		osd_fseek(w->file, w->length, SEEK_CUR);
		offset += w->length;
		if( offset >= filesize )
		{
			logerror("WAVE no 'fmt ' tag found\n");
			return WAVE_ERR;
        }
	}

	/* read the format -- make sure it is PCM */
	offset += osd_fread_lsbfirst(w->file, &temp16, 2);
	if( temp16 != 1 )
	{
		logerror("WAVE format %d not supported (not = 1 PCM)\n", temp16);
			return WAVE_ERR;
    }
	logerror("WAVE format %d (PCM)\n", temp16);

	/* number of channels -- only mono is supported, but we can mix multi-channel to mono */
	offset += osd_fread_lsbfirst(w->file, &channels, 2);
	logerror("WAVE channels %d\n", channels);

	/* sample rate */
	offset += osd_fread(w->file, &temp32, 4);
	w->smpfreq = LITTLE_ENDIANIZE_INT32(temp32);
	logerror("WAVE sample rate %d Hz\n", w->smpfreq);

	/* bytes/second and block alignment are ignored */
	offset += osd_fread(w->file, buf, 4);
	/* read block alignment */
	offset += osd_fread_lsbfirst(w->file, &blockAlign, 2);

	/***Field specific to PCM***/
	/* read bits/sample */
	offset += osd_fread_lsbfirst(w->file, &bitsPerSample, 2);
	logerror("WAVE bits/sample %d\n", bitsPerSample);
	w->resolution = bitsPerSample;

	/* seek past any extra data */
	osd_fseek(w->file, w->length - 16, SEEK_CUR);
	offset += w->length - 16;

	/* Compute a few constants */
	if (blockAlign % channels)
	{
		logerror("WAVE format is incorrect\n");		/* This is what Microsoft says */
		return WAVE_ERR;
	}

	sample_padding = (blockAlign/channels) - ((bitsPerSample+7) >> 3);


	/* seek until we find a data tag */
	while( 1 )
	{
		offset += osd_fread(w->file, buf, 4);
		offset += osd_fread(w->file, &temp32, 4);
		w->length = LITTLE_ENDIANIZE_INT32(temp32);
		if( memcmp(&buf[0], "data", 4) == 0 )
			break;

		/* seek to the next block */
		osd_fseek(w->file, w->length, SEEK_CUR);
		offset += w->length;
		if( offset >= filesize )
		{
			logerror("WAVE not 'data' tag found\n");
			return WAVE_ERR;
        }
	}

	/* allocate the game sample */
	w->max_samples = w->samples = w->length / blockAlign;
	logerror("WAVE %d samples - %d:%02d\n", w->samples, (w->samples/w->smpfreq)/60, (w->samples/w->smpfreq)%60);

	w->resolution = 16;
	w->length = w->max_samples * w->resolution / 8;

	w->data = malloc(w->length);

	if( w->data == NULL )
	{
		logerror("WAVE failed to malloc %d bytes\n", w->length);
		return WAVE_ERR;
    }

	{
		int bit;
		int channel;
		unsigned i;

		int ch;	/* NPW 07-Sep-2002 - This must be int so that it can have EOF */
		long sample_buf;
		long this_buf;

		UINT16 *dst = w->data;

		for (i=0; i<w->samples; i++)
		{

			this_buf = 0;
			for (channel=0; channel<channels; channel++)
			{
				sample_buf = 0;
				/* skip pad bytes */
				if (sample_padding)
					osd_fseek(w->file, sample_padding, SEEK_CUR);
				/* read ceil(wave_file->bitsPerSample/8) bits */
				for (bit=0; bit<bitsPerSample; bit+=8)
				{
					ch = osd_fgetc(w->file);
					if (ch == EOF)
						return WAVE_ERR;

					sample_buf |= ((/*uintmax_t*/unsigned long) (ch /*& 0xff*/)) << bit;
				}
				/* shift out undefined bits */
				if (bit > bitsPerSample)
					sample_buf >>= bit-bitsPerSample;
				if (bitsPerSample <= 8)
					/* convert to signed if unsigned */
					sample_buf -= 1 << (bitsPerSample-1);
				else
				{
					/* extend sign bit */
					if (sample_buf & ((/*intmax_t*/long)1 << (bitsPerSample-1)))
						sample_buf |= ~ (((/*intmax_t*/long)1 << bitsPerSample)-1);
				}

				/* mix with previous channels */
				this_buf += sample_buf;
			}
			/* normalize and save reply */
			if (bitsPerSample < 16)
				this_buf <<= 16-bitsPerSample;
			else if (bitsPerSample >16)
				this_buf >>= bitsPerSample-16;
			this_buf /= channels;
			*dst++ = this_buf;
		}
	}

	return WAVE_OK;
}

/*
	write the complete sampled tape image from memory to disk
*/
static int wave_write(int id)
{
	struct wave_file *w = &wave[id];
	UINT32 filesize, offset = 0, temp32;
	UINT16 temp16;
	UINT32 data_length;

	if( !w->file )
        return WAVE_ERR;

	data_length = w->samples * w->resolution / 8;

    filesize =
		4 + 	/* 'RIFF' */
		4 + 	/* size of entire file */
		8 + 	/* 'WAVEfmt ' */
		20 +	/* WAVE tag  (including size -- 0x10 in dword) */
		4 + 	/* 'data' */
		4 + 	/* size of data */
		data_length;

    /* write the core header for a WAVE file */
	offset += osd_fwrite(w->file, "RIFF", 4);
    if( offset < 4 )
    {
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	temp32 = LITTLE_ENDIANIZE_INT32(filesize) - 8;
	offset += osd_fwrite(w->file, &temp32, 4);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fwrite(w->file, "WAVE", 4);
	if( offset < 12 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
	}

	/* write a format tag */
	offset += osd_fwrite(w->file, "fmt ", 4);
    if( offset < 12 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }
    /* size of the following 'fmt ' fields */
    offset += osd_fwrite(w->file, "\x10\x00\x00\x00", 4);
	if( offset < 16 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* format: PCM */
	temp16 = 1;
	offset += osd_fwrite_lsbfirst(w->file, &temp16, 2);
	if( offset < 18 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* channels: 1 (mono) */
	temp16 = 1;
    offset += osd_fwrite_lsbfirst(w->file, &temp16, 2);
	if( offset < 20 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* sample rate */
	temp32 = LITTLE_ENDIANIZE_INT32(w->smpfreq);
	offset += osd_fwrite(w->file, &temp32, 4);
	if( offset < 24 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* byte rate */
	temp32 = LITTLE_ENDIANIZE_INT32(w->smpfreq * w->resolution / 8);
	offset += osd_fwrite(w->file, &temp32, 4);
	if( offset < 28 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* block align (size of one `sample') */
	temp16 = w->resolution / 8;
	offset += osd_fwrite_lsbfirst(w->file, &temp16, 2);
	if( offset < 30 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* block align */
	temp16 = w->resolution;
	offset += osd_fwrite_lsbfirst(w->file, &temp16, 2);
	if( offset < 32 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* 'data' tag */
	offset += osd_fwrite(w->file, "data", 4);
	if( offset < 36 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	/* data size */
	temp32 = LITTLE_ENDIANIZE_INT32(data_length);
	offset += osd_fwrite(w->file, &temp32, 4);
	if( offset < 40 )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	if( osd_fwrite_lsbfirst(w->file, w->data, data_length) != data_length )
	{
		logerror("WAVE write error at offs %d\n", offset);
		return WAVE_ERR;
    }

	return WAVE_OK;
}

/*
	display a small tape icon, with the current position in the tape image
*/
static void wave_display(struct mame_bitmap *bitmap, int id)
{
    struct wave_file *w = &wave[id];

	if (image_exists(IO_CASSETTE, id))
	{
		if ((wave_status(id, -1) & (WAVE_STATUS_MOTOR_ENABLE|WAVE_STATUS_MOTOR_INHIBIT)) == WAVE_STATUS_MOTOR_ENABLE)
		{
			if (w->smpfreq)
			{
				char buf[32];
				int x, y, n, t0, t1;

				x = id * Machine->uifontwidth * 16 + 1;
				y = Machine->uiheight - 9;
				n = (w->play_pos * 4 / w->smpfreq) & 3;
				t0 = w->play_pos / w->smpfreq;
				t1 = w->samples / w->smpfreq;
				sprintf(buf, "%c%c %02d:%02d (%04d) [%02d:%02d (%04d)]", n*2+2,n*2+3, t0/60,t0%60, t0, t1/60, t1%60, t1);
				ui_text(bitmap, buf, x, y);
			}
		}
	}
}

/*****************************************************************************
	WaveSound interface
	our tape can be heard on the machine video.

TODO:
	tape output cannot be heard if the recorder is not recording
 *****************************************************************************/

/*
	this is the related stream update callback
*/
static void wave_sound_update(int id, INT16 *buffer, int length)
{
	struct wave_file *w = &wave[id];
	int pos = w->play_pos;
	int count = w->counter;
	INT16 sample = w->play_sample;

	if( !w->timer )
	{
		while( length-- > 0 )
			*buffer++ = /*sample*/0;
	}
	else
	{
		/*if (pos >= w->samples)
		{
			pos = w->samples - 1;
			if (pos < 0)
				pos = 0;
		}*/
		if (w->mode)
			wave_update_output_buffer(id);

		while (length--)
		{
			count -= w->smpfreq;
			while (count <= 0)
			{
				count += Machine->sample_rate;

				/* get the sample (unless we are muted) */
				if (!(w->status & WAVE_STATUS_MUTED))
				{
					if (w->resolution == 16)
						sample = *((INT16 *)w->data + pos);
					else
						sample = *((INT8 *)w->data + pos)*256;
				}

				if (++pos >= w->samples)
				{
					pos = w->samples - 1;
					if (pos < 0)
						pos = 0;
				}
			}
			*buffer++ = sample;
		}

		w->counter = count;
		w->play_pos = pos;
		w->play_sample = sample;
	}
}

int wave_sh_start(const struct MachineSound *msound)
{
	int i;

    intf = msound->sound_interface;

    for( i = 0; i < intf->num; i++ )
	{
		struct wave_file *w = &wave[i];
		char buf[32];

        if( intf->num > 1 )
			sprintf(buf, "Cassette #%d", i+1);
		else
			strcpy(buf, "Cassette");

        w->channel = stream_init(buf, intf->mixing_level[i], Machine->sample_rate, i, wave_sound_update);

        if( w->channel == -1 )
			return 1;
	}

    return 0;
}

void wave_sh_stop(void)
{
	int i;

    for( i = 0; i < intf->num; i++ )
		wave[i].channel = -1;
}

void wave_sh_update(void)
{
	int i;

	for( i = 0; i < intf->num; i++ )
	{
		if( wave[i].channel != -1 )
			stream_update(wave[i].channel, 0);
	}
}

/*****************************************************************************
 * IODevice interface functions
 *****************************************************************************/

/*
 * return info about a wave device
 */
const void *wave_info(int id, int whatinfo)
{
	return NULL;
}

int wave_status(int id, int newstatus)
{
	/* wave status has the following bitfields:
	 *
	 *  Bit 3:  Write-only tape (1=write-only, 0=read-only) (read-only bit)
	 *  Bit 2:  Inhibit Motor (1=inhibit 0=noinhibit)
	 *	Bit 1:	Mute (1=mute 0=nomute)
	 *	Bit 0:	Motor (1=on 0=off)
	 *
	 *  Bit 0 is usually set by the tape control, and bit 2 is usually set by
	 *  the driver
	 *
	 *	Also, you can pass -1 to have it simply return the status
	 */
	struct wave_file *w = &wave[id];
	int reply;

	if( !w->file )
		return 0;

    if( newstatus != -1 )
	{
		w->status = newstatus & (WAVE_STATUS_MOTOR_INHIBIT | WAVE_STATUS_MUTED | WAVE_STATUS_MOTOR_ENABLE);

		if (newstatus & WAVE_STATUS_MOTOR_INHIBIT)
			newstatus = 0;
		else
			newstatus &= WAVE_STATUS_MOTOR_ENABLE;

		if( newstatus && !w->timer )
		{
			w->timer = timer_alloc(NULL);
			timer_adjust(w->timer, TIME_NEVER, 0, 0);
		}
		else
		if( !newstatus && w->timer )
		{
			if (w->mode)
				wave_update_output_buffer(id);
			w->offset = wave_tell(id);
			timer_remove(w->timer);
			w->timer = NULL;
			schedule_full_refresh();
		}
	}
	reply = w->status;
	/*if (w->timer)
		reply |= WAVE_STATUS_MOTOR_ENABLE;*/
	/*if (w->status & WAVE_STATUS_MOTOR_INHIBIT)
		w->status & ~WAVE_STATUS_MOTOR_ENABLE;*/
	/*if ((! (w->status & WAVE_STATUS_MOTOR_INHIBIT)) && ! w->timer)
		reply &= ~WAVE_STATUS_MOTOR_ENABLE;*/
	if (w->mode)
		reply |= WAVE_STATUS_WRITE_ONLY;
	return reply;
}

int wave_open(int id, int mode, void *args)
{
	struct wave_file *w = &wave[id];
    struct wave_args_legacy *wa = args;
	int result;

    /* wave already opened? */
	if( w->file )
		wave_close(id);

	if (! wa->file)
	{
		w->file = NULL;
		return INIT_PASS;
	}

    w->file = wa->file;
	w->mode = mode;
	w->fill_wave = wa->fill_wave;
	w->smpfreq = wa->smpfreq;

	if( w->mode )
	{	/* write-only image */
		w->resolution = 16;
		w->max_samples = w->smpfreq;
		w->samples = 0;
		w->length = w->max_samples * w->resolution / 8;
		w->data = malloc(w->length);
		if( !w->data )
		{
			logerror("WAVE malloc(%d) failed\n", w->length);
			memset(w, 0, sizeof(struct wave_file));
			return WAVE_ERR;
		}
		memset (w->data, 0, w->length);
		return INIT_PASS;
    }
	else
	{
		result = wave_read(id);
		if( result == WAVE_OK )
		{
			/* return sample frequency in the user supplied structure */
			wa->smpfreq = w->smpfreq;
			w->offset = 0;
			return INIT_PASS;
		}

		if( result == WAVE_FMT )
		{
			UINT8 *data;
			int bytes, pos, length;

			/* User supplied fill_wave function? */
			if( w->fill_wave == NULL )
			{
				logerror("WAVE no fill_wave callback, failing now\n");
				return WAVE_ERR;
			}

			logerror("WAVE creating wave using fill_wave() callback\n");

			/* sanity check: default chunk size is one byte */
			if( wa->chunk_size == 0 )
			{
				wa->chunk_size = 1;
				logerror("WAVE chunk_size defaults to %d\n", wa->chunk_size);
			}
			if( wa->smpfreq == 0 )
			{
				wa->smpfreq = 11025;
				logerror("WAVE smpfreq defaults to %d\n", w->smpfreq);
			}

			/* allocate a buffer for the binary data */
			data = malloc(wa->chunk_size);
			if( !data )
			{
				free(w->data);
				/* zap the wave structure */
				memset(&wave[id], 0, sizeof(struct wave_file));
				return WAVE_ERR;
			}

			/* determine number of samples */
			length =
				wa->header_samples +
				((osd_fsize(w->file) + wa->chunk_size - 1) / wa->chunk_size) * wa->chunk_samples +
				wa->trailer_samples;

			w->smpfreq = wa->smpfreq;
			w->resolution = 16;
			w->max_samples = length;
			w->samples = 0;
			w->length = length * 2;   /* 16 bits per sample */

			w->data = malloc(w->length);
			if( !w->data )
			{
				logerror("WAVE failed to malloc %d bytes\n", w->length);
				/* zap the wave structure */
				memset(&wave[id], 0, sizeof(struct wave_file));
				return WAVE_ERR;
			}
			logerror("WAVE creating max %d:%02d samples (%d) at %d Hz\n", (w->max_samples/w->smpfreq)/60, (w->max_samples/w->smpfreq)%60, w->max_samples, w->smpfreq);

			pos = 0;
			/* if there has to be a header */
			if( wa->header_samples > 0 )
			{
				length = (*w->fill_wave)((INT16 *)w->data + pos, w->max_samples - pos, CODE_HEADER);
				if( length < 0 )
				{
					logerror("WAVE conversion aborted at header\n");
					free(w->data);
					/* zap the wave structure */
					memset(&wave[id], 0, sizeof(struct wave_file));
					return WAVE_ERR;
				}
				logerror("WAVE header %d samples\n", length);
				pos += length;
			}

			/* convert the file data to samples */
			bytes = 0;
			osd_fseek(w->file, 0, SEEK_SET);
			while( pos < w->max_samples )
			{
				length = osd_fread(w->file, data, wa->chunk_size);
				if( length == 0 )
					break;
				bytes += length;
				length = (*w->fill_wave)((INT16 *)w->data + pos, w->max_samples - pos, data);
				if( length < 0 )
				{
					logerror("WAVE conversion aborted at %d bytes (%d samples)\n", bytes, pos);
					free(w->data);
					/* zap the wave structure */
					memset(&wave[id], 0, sizeof(struct wave_file));
					return WAVE_ERR;
				}
				pos += length;
			}
			logerror("WAVE converted %d data bytes to %d samples\n", bytes, pos);

			/* if there has to be a trailer */
			if( wa->trailer_samples )
			{
				if( pos < w->max_samples )
				{
					length = (*w->fill_wave)((INT16 *)w->data + pos, w->max_samples - pos, CODE_TRAILER);
					if( length < 0 )
					{
						logerror("WAVE conversion aborted at trailer\n");
						free(w->data);
						/* zap the wave structure */
						memset(&wave[id], 0, sizeof(struct wave_file));
						return WAVE_ERR;
					}
					logerror("WAVE trailer %d samples\n", length);
					pos += length;
				}
			}


			w->samples = pos;

			if( pos < w->max_samples )
			{
				/* what did the fill_wave() calls really fill into the buffer? */
				w->max_samples = pos;
				w->length = pos * 2;   /* 16 bits per sample */
				w->data = realloc(w->data, w->length);
				/* failure in the last step? how sad... */
				if( !w->data )
				{
					logerror("WAVE realloc(%d) failed\n", w->length);
					/* zap the wave structure */
					memset(&wave[id], 0, sizeof(struct wave_file));
					return WAVE_ERR;
				}
			}
			logerror("WAVE %d samples - %d:%02d\n", w->samples, (w->samples/w->smpfreq)/60, (w->samples/w->smpfreq)%60);
			/* hooray! :-) */
			return INIT_PASS;
		}
	}
	return WAVE_ERR;
}

void wave_close(int id)
{
	struct wave_file *w = &wave[id];

    if( !w->file )
		return;

#if 0
    if( w->timer )
	{	/* if we are currently playing/recording the file, we chop off the end (why ???) */
		//if( w->channel != -1 )
		//	stream_update(w->channel, 0);	/* R. Nabet : aaargh ! (to be confirmed) */
		/*w->samples = w->play_pos;
		w->length = w->samples * w->resolution / 8;*/
		timer_remove(w->timer);
		w->timer = NULL;
	}
#else
	/* turn motor off */
	wave_status(id, wave_status(id, -1) & ~ WAVE_STATUS_MOTOR_ENABLE);
#endif

    if( w->mode )
	{	/* if image is writable, we do write it to disk */
		wave_output(id,0);
		wave_write(id);
		w->mode = 0;
	}

    if( w->data )
		free(w->data);

    w->data = NULL;
	w->file = NULL;
	w->offset = 0;
	w->play_pos = 0;
	w->record_pos = 0;
	w->counter = 0;
	w->smpfreq = 0;
	w->resolution = 0;
	w->samples = 0;
	w->length = 0;
}

int wave_seek(int id, int offset, int whence)
{
	struct wave_file *w = &wave[id];
    UINT32 pos = 0;

	if( !w->file )
		return pos;

    switch( whence )
	{
	case SEEK_SET:
		pos = 0;
		break;
	case SEEK_END:
		pos = w->samples - 1;
		break;
	case SEEK_CUR:
		pos = wave_tell(id);
		break;
	}
	w->offset = pos + offset;
	if( w->offset < 0 )
		w->offset = 0;
	if( w->offset >= w->length )
		w->offset = w->length - 1;
	w->play_pos = w->record_pos = w->offset;

	if( w->timer )
		timer_adjust(w->timer, TIME_NEVER, 0, 0);
	return w->offset;
}

static int internal_wave_tell(int id, int allow_overflow)
{
	struct wave_file *w = &wave[id];
    UINT32 pos = w->offset;
	if (w->timer)
		pos += (timer_timeelapsed(w->timer) * w->smpfreq + 0.5);
	if (!allow_overflow && (pos >= w->samples))
		pos = w->samples - 1;
    return pos;
}

int wave_tell(int id)
{
	return internal_wave_tell(id, 0);
}

int wave_input(int id)
{
	struct wave_file *w = &wave[id];
	UINT32 pos = 0;
    int level = 0;

	if( !w->file )
		return level;

    if( w->channel != -1 )
		stream_update(w->channel, 0);

    if( w->timer )
	{
		pos = wave_tell(id);
		if( pos >= 0 )
		{
			if( w->resolution == 16 )
				level = *((INT16 *)w->data + pos);
			else
				level = 256 * *((INT8 *)w->data + pos);
		}
    }
    return level;
}

static void wave_update_output_buffer(int id)
{
	struct wave_file *w = &wave[id];
	UINT32 pos = 0;
	void *new_data;

    if( w->timer )
    {
		pos = internal_wave_tell(id, 1);
		if( pos >= w->max_samples )
        {
			/* add at least one second of data */
			if( pos - w->max_samples < w->smpfreq )
				w->max_samples += w->smpfreq;
			else
				w->max_samples = pos;	/* more than one second */
			w->length = w->max_samples * w->resolution / 8;
			new_data = realloc(w->data, w->length);
			if (! new_data)
            {
                logerror("WAVE realloc(%d) failed\n", w->length);
				/* turn motor off (as this is the kind of the thing we can expect from
				a tape recorder which has reached the physical end of the tape) */
				wave_status(id, wave_status(id, -1) & ~ WAVE_STATUS_MOTOR_ENABLE);
                return;
            }
			w->data = new_data;
        }
		if (pos >= w->samples)
			w->samples = pos;
		while( w->record_pos < pos )
        {
			if( w->resolution == 16 )
				*((INT16 *)w->data + w->record_pos) = w->record_sample;
			else
				*((INT8 *)w->data + w->record_pos) = w->record_sample / 256;
			w->record_pos++;
        }
    }
}

void wave_output(int id, int data)
{
	struct wave_file *w = &wave[id];

	if( !w->file )
	{
	    w->record_sample = data;
		return;
	}

    if( !w->mode )
    {
	    w->record_sample = data;
		return;
	}

	if( data == w->record_sample )
		return;

	if( w->channel != -1 )
		stream_update(w->channel, 0);

	wave_update_output_buffer(id);

    w->record_sample = data;
}

int wave_input_chunk(int id, void *dst, int count)
{
	struct wave_file *w = &wave[id];
	UINT32 pos = 0;

	if( !w->file )
		return 0;

    if( w->timer )
	{
		pos = wave_tell(id);
	}

    if( pos + count >= w->samples )
		count = w->samples - pos - 1;

    if( count > 0 )
	{
		if( w->resolution == 16 )
			memcpy(dst, (INT16 *)w->data + pos, count * sizeof(INT16));
		else
			memcpy(dst, (INT8 *)w->data + pos, count * sizeof(INT8));
	}

    return count;
}

int wave_output_chunk(int id, void *src, int count)
{
	struct wave_file *w = &wave[id];
	UINT32 pos = 0;

	if( !w->file )
		return 0;

    if( w->timer )
	{
		pos = wave_tell(id);
	}

	if( pos + count >= w->max_samples )
	{
		/* add space for new data */
		w->max_samples += count - pos;
		w->length = w->samples * w->resolution / 8;
		w->data = realloc(w->data, w->length);
		if( !w->data )
		{
			logerror("WAVE realloc(%d) failed\n", w->length);
			memset(w, 0, sizeof(struct wave_file));
			return 0;
		}
	}

	if( pos + count >= w->samples )
		w->samples = pos + count;

    if( count > 0 )
	{
		if( w->resolution == 16 )
			memcpy((INT16 *)w->data + pos, src, count * sizeof(INT16));
		else
			memcpy((INT8 *)w->data + pos, src, count * sizeof(INT8));
	}

    return count;
}

/* ----------------------------------------------------------------------- */

void wave_specify(struct IODevice *iodev, int count, char *actualext, const char *fileext,
	int (*init)(int id, void *fp, int open_mode), void (*exit_)(int id))
{
	strcpy(actualext, "wav");
	strcpy(actualext + 4, fileext);
	memset(iodev, 0, sizeof(*iodev));
	iodev->type = IO_CASSETTE;
	iodev->count = count;
	iodev->file_extensions = actualext;
	iodev->reset_depth = IO_RESET_NONE;
	iodev->open_mode = OSD_FOPEN_READ_OR_WRITE;
	iodev->init = init;
	iodev->exit = exit_;
	iodev->info = wave_info;
	iodev->open = wave_open;
	iodev->close = wave_close;
	iodev->status = wave_status;
	iodev->seek = wave_seek;
	iodev->tell = wave_tell;
	iodev->input = wave_input;
	iodev->output = wave_output;
	iodev->display = wave_display;
}

