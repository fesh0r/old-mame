/***************************************************************************

  cbm vc20/c64
  cbm c16 series (other physical representation)
  tape/cassette/datassette emulation

***************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include "snprintf.h"

#include "driver.h"
#include "unzip.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/vc20tape.h"

struct DACinterface vc20tape_sound_interface =
{
	1,
	{25}
};

#define TONE_ON_VALUE 0xff

/* write line high active, */
/* read line low active!? */

static struct
{
	int on, noise;
	int play, record;

	int data;
	int motor;
	void (*read_callback) (UINT32, UINT8);

#define TAPE_WAV 1
#define TAPE_PRG 2
#define TAPE_ZIP 3
	int type;						   /* 0 nothing */
}
tape;

/* these are the values for wav files */
struct
{
	int state;
	void *timer;
	int image_type;
    int image_id;
	int pos;
	struct GameSample *sample;
} wav;

/* these are the values for prg files */
struct
{
	int state;

	/* these values are shared with the zip driver */
	void *timer;
	int image_type;
    int image_id;
	int pos;

#define VC20_SHORT		(176e-6)
#define VC20_MIDDLE		(256e-6)
#define VC20_LONG		(336e-6)
#define C16_SHORT	(246e-6)		   /* messured */
#define C16_MIDDLE	(483e-6)
#define C16_LONG	(965e-6)
#define PCM_SHORT	(prg.c16?C16_SHORT:VC20_SHORT)
#define PCM_MIDDLE	(prg.c16?C16_MIDDLE:VC20_MIDDLE)
#define PCM_LONG	(prg.c16?C16_LONG:VC20_LONG)
	int c16;
	UINT8 *prg;
	int length;
	int stateblock, stateheader, statebyte, statebit;
	int prgdata;
	char name[16];					   /*name for cbm */
	UINT8 chksum;
	double lasttime;
} prg;

/* these are values for zip files */
struct
{
	int image_type;
    int image_id;
	int state;
	ZIP *zip;
	struct zipent *zipentry;
} zip;

/* from sound/samples.c no changes (static declared) */
/* readsamples not useable (loads files only from sample or game directory) */
/* and doesn't search the rompath */
#ifdef LSB_FIRST
#define intelLong(x) (x)
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | \
                       (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#endif
static struct GameSample *vc20_read_wav_sample (void *f)
{
	unsigned long offset = 0;
	UINT32 length, rate, filesize, temp32;
	UINT16 bits, temp16;
	char buf[32];
	struct GameSample *result;

	/* read the core header and make sure it's a WAVE file */
	offset += osd_fread (f, buf, 4);
	if (offset < 4)
		return NULL;
	if (memcmp (&buf[0], "RIFF", 4) != 0)
		return NULL;

	/* get the total size */
	offset += osd_fread (f, &filesize, 4);
	if (offset < 8)
		return NULL;
	filesize = intelLong (filesize);

	/* read the RIFF file type and make sure it's a WAVE file */
	offset += osd_fread (f, buf, 4);
	if (offset < 12)
		return NULL;
	if (memcmp (&buf[0], "WAVE", 4) != 0)
		return NULL;

	/* seek until we find a format tag */
	while (1)
	{
		offset += osd_fread (f, buf, 4);
		offset += osd_fread (f, &length, 4);
		length = intelLong (length);
		if (memcmp (&buf[0], "fmt ", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek (f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* read the format -- make sure it is PCM */
	offset += osd_fread_lsbfirst (f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* number of channels -- only mono is supported */
	offset += osd_fread_lsbfirst (f, &temp16, 2);
	if (temp16 != 1)
		return NULL;

	/* sample rate */
	offset += osd_fread (f, &rate, 4);
	rate = intelLong (rate);

	/* bytes/second and block alignment are ignored */
	offset += osd_fread (f, buf, 6);

	/* bits/sample */
	offset += osd_fread_lsbfirst (f, &bits, 2);
	if (bits != 8 && bits != 16)
		return NULL;

	/* seek past any extra data */
	osd_fseek (f, length - 16, SEEK_CUR);
	offset += length - 16;

	/* seek until we find a data tag */
	while (1)
	{
		offset += osd_fread (f, buf, 4);
		offset += osd_fread (f, &length, 4);
		length = intelLong (length);
		if (memcmp (&buf[0], "data", 4) == 0)
			break;

		/* seek to the next block */
		osd_fseek (f, length, SEEK_CUR);
		offset += length;
		if (offset >= filesize)
			return NULL;
	}

	/* allocate the game sample */
	result = (struct GameSample *)malloc (sizeof (struct GameSample) + length);

	if (result == NULL)
		return NULL;

	/* fill in the sample data */
	result->length = length;
	result->smpfreq = rate;
	result->resolution = bits;

	/* read the data in */
	if (bits == 8)
	{
		osd_fread (f, result->data, length);

		/* convert 8-bit data to signed samples */
		for (temp32 = 0; temp32 < length; temp32++)
			result->data[temp32] -= 0x80;
	}
	else
	{
		/* 16-bit data is fine as-is */
		osd_fread_lsbfirst (f, result->data, length);
	}

	return result;
}

static void vc20_wav_timer (int data);
static void vc20_wav_state (void)
{
	switch (wav.state)
	{
	case 0:
		break;						   /* not inited */
	case 1:						   /* off */
		if (tape.on)
		{
			wav.state = 2;
			break;
		}
		break;
	case 2:						   /* on */
		if (!tape.on)
		{
			wav.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			break;
		}
		if (tape.motor && tape.play)
		{
			wav.state = 3;
			wav.timer = timer_pulse (1.0 / wav.sample->smpfreq, 0, vc20_wav_timer);
			break;
		}
		if (tape.motor && tape.record)
		{
			wav.state = 4;
			break;
		}
		break;
	case 3:						   /* reading */
		if (!tape.on)
		{
			wav.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			if (wav.timer)
				timer_remove (wav.timer);
			break;
		}
		if (!tape.motor || !tape.play)
		{
			wav.state = 2;
			if (wav.timer)
				timer_remove (wav.timer);
			DAC_data_w (0, 0);
			break;
		}
		break;
	case 4:						   /* saving */
		if (!tape.on)
		{
			wav.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			break;
		}
		if (!tape.motor || !tape.record)
		{
			wav.state = 2;
			DAC_data_w (0, 0);
			break;
		}
		break;
	}
}

static void vc20_wav_open (int image_type, int image_id)
{
	FILE *fp;

	fp = (FILE*)osd_fopen (Machine->gamedrv->name, device_filename(image_type,image_id), OSD_FILETYPE_IMAGE, 0);
	if (!fp)
	{
		logerror("tape %s file not found\n", device_filename(image_type,image_id));
		return;
	}
	if ((wav.sample = vc20_read_wav_sample (fp)) == NULL)
	{
		logerror("tape %s could not be loaded\n", device_filename(image_type,image_id));
		osd_fclose (fp);
		return;
	}
	logerror("tape %s loaded\n", device_filename(image_type,image_id));
	osd_fclose (fp);

	wav.image_type = image_type;
    wav.image_id = image_id;
	tape.type = TAPE_WAV;
	wav.pos = 0;
	tape.on = 1;
	wav.state = 2;
}

static void vc20_wav_write (int data)
{
	if (tape.noise)
		DAC_data_w (0, data);
}

static void vc20_wav_timer (int data)
{
	if (wav.sample->resolution == 8)
	{
		tape.data = wav.sample->data[wav.pos] > 0x0;
		wav.pos++;
		if (wav.pos >= wav.sample->length)
		{
			wav.pos = 0;
			tape.play = 0;
		}
	}
	else
	{
		tape.data = ((short *) (wav.sample->data))[wav.pos] > 0x0;
		wav.pos++;
		if (wav.pos * 2 >= wav.sample->length)
		{
			wav.pos = 0;
			tape.play = 0;
		}
	}
	if (tape.noise)
		DAC_data_w (0, tape.data ? TONE_ON_VALUE : 0);
	if (tape.read_callback)
		tape.read_callback (0, tape.data);
	/*    vc20_wav_state(); // removing timer in timer puls itself hangs */
}

static void vc20_prg_timer (int data);
static void vc20_prg_state (void)
{
	switch (prg.state)
	{
	case 0:
		break;						   /* not inited */
	case 1:						   /* off */
		if (tape.on)
		{
			prg.state = 2;
			break;
		}
		break;
	case 2:						   /* on */
		if (!tape.on)
		{
			prg.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			break;
		}
		if (tape.motor && tape.play)
		{
			prg.state = 3;
			prg.timer = timer_set (0.0, 0, vc20_prg_timer);
			break;
		}
		if (tape.motor && tape.record)
		{
			prg.state = 4;
			break;
		}
		break;
	case 3:						   /* reading */
		if (!tape.on)
		{
			prg.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			if (prg.timer)
				timer_remove (prg.timer);
			break;
		}
		if (!tape.motor || !tape.play)
		{
			prg.state = 2;
			if (prg.timer)
				timer_remove (prg.timer);
			DAC_data_w (0, 0);
			break;
		}
		break;
	case 4:						   /* saving */
		if (!tape.on)
		{
			prg.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			break;
		}
		if (!tape.motor || !tape.record)
		{
			prg.state = 2;
			DAC_data_w (0, 0);
			break;
		}
		break;
	}
}

static void vc20_prg_open (int image_type, int image_id)
{
	const char *name;
    FILE *fp;
	int i;

	fp = (FILE*)osd_fopen (Machine->gamedrv->name, device_filename(image_type,image_id), OSD_FILETYPE_IMAGE, 0);
	if (!fp)
	{
		logerror("tape %s file not found\n", device_filename(image_type,image_id));
		return;
	}
	prg.length = osd_fsize (fp);
	if ((prg.prg = (UINT8 *) malloc (prg.length)) == NULL)
	{
		logerror("tape %s could not be loaded\n", device_filename(image_type,image_id));
		osd_fclose (fp);
		return;
	}
	osd_fread (fp, prg.prg, prg.length);
	logerror("tape %s loaded\n", device_filename(image_type,image_id));
	osd_fclose (fp);

	name = device_filename(image_type,image_id);
    for (i = 0; name[i] != 0; i++)
		prg.name[i] = toupper (name[i]);
	for (; i < 16; i++)
		prg.name[i] = ' ';

	prg.image_type = image_type;
    prg.image_id = image_id;
	prg.stateblock = 0;
	prg.stateheader = 0;
	prg.statebyte = 0;
	prg.statebit = 0;
	tape.type = TAPE_PRG;
	tape.on = 1;
	prg.state = 2;
	prg.pos = 0;
}

static void vc20_prg_write (int data)
{
#if 0
	/* this was used to decode cbms tape format, but could */
	/* be converted to a real program writer */
	/* c16: be sure the cpu clock is about 1.8 MHz (when screen is off) */
	static int count = 0;
	static int old = 0;
	static double time = 0;
	static int bytecount = 0, byte;

	if (old != data)
	{
		double neu = timer_get_time ();
		int diff = (neu - time) * 1000000;

		count++;
		logerror("%f %d %s %d\n", (PCM_LONG + PCM_MIDDLE) / 2,
					 bytecount, old ? "high" : "low",
					 diff);
		if (old)
		{
			if (count > 0 /*27000 */ )
			{
				switch (bytecount)
				{
				case 0:
					if (diff > (PCM_LONG + PCM_MIDDLE) * 1e6 / 2)
					{
						bytecount++;
						byte = 0;
					}
					break;
				case 1:
				case 3:
				case 5:
				case 7:
				case 9:
				case 11:
				case 13:
				case 15:
				case 17:
					bytecount++;
					break;
				case 2:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 1;
					bytecount++;
					break;
				case 4:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 2;
					bytecount++;
					break;
				case 6:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 4;
					bytecount++;
					break;
				case 8:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 8;
					bytecount++;
					break;
				case 10:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 0x10;
					bytecount++;
					break;
				case 12:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 0x20;
					bytecount++;
					break;
				case 14:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 0x40;
					bytecount++;
					break;
				case 16:
					if (diff > (PCM_MIDDLE + PCM_SHORT) * 1e6 / 2)
						byte |= 0x80;
					logerror("byte %.2x\n", byte);
					bytecount = 0;
					break;
				}
			}
		}
		old = data;
		time = timer_get_time ();
	}
#endif
	if (tape.noise)
		DAC_data_w (0, data ? TONE_ON_VALUE : 0);
}

static void vc20_tape_bit (int bit)
{
	switch (prg.statebit)
	{
	case 0:
		if (bit)
		{
			timer_reset (prg.timer, prg.lasttime = PCM_MIDDLE);
			prg.statebit = 2;
		}
		else
		{
			prg.statebit++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
		}
		break;
	case 1:
		timer_reset (prg.timer, prg.lasttime = PCM_MIDDLE);
		prg.statebit = 0;
		break;
	case 2:
		timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
		prg.statebit = 0;
		break;
	}
}

static void vc20_tape_byte (void)
{
	static int bit = 0, parity = 0;

	/* convert one byte to vc20 tape data
	 * puls wide modulation
	 * 3 type of pulses (quadratic on/off pulse)
	 * K (short) 176 microseconds
	 * M 256
	 * L 336
	 * LM bit0 bit1 bit2 bit3 bit4 bit5 bit6 bit7 oddparity
	 * 0 coded as KM, 1 as MK
	 * gives 8.96 milliseconds for 1 byte
	 */
	switch (prg.statebyte)
	{
	case 0:
		timer_reset (prg.timer, prg.lasttime = PCM_LONG);
		prg.statebyte++;
		break;
	case 1:
		timer_reset (prg.timer, prg.lasttime = PCM_MIDDLE);
		prg.statebyte++;
		bit = 1;
		parity = 0;
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		vc20_tape_bit (prg.prgdata & bit);
		if (prg.prgdata & bit)
			parity = !parity;
		bit <<= 1;
		prg.statebyte++;
		break;
	case 10:
		vc20_tape_bit (!parity);
		prg.chksum ^= prg.prgdata;
		prg.statebyte = 0;
		prg.pos--;
		break;
	}
}

/* 01 prg id
 * lo hi load address
 * lo hi end address
 * 192-5-1 bytes: filename (filled with 0x20)
 * xor chksum */
static void vc20_tape_prgheader (void)
{
	static int i = 0;

	switch (prg.stateheader)
	{
	case 0:
		prg.chksum = 0;
		prg.prgdata = 1;
		prg.stateheader++;
		vc20_tape_byte ();
		break;
	case 1:
		prg.prgdata = prg.prg[0];
		prg.stateheader++;
		vc20_tape_byte ();
		break;
	case 2:
		prg.prgdata = prg.prg[1];
		prg.stateheader++;
		vc20_tape_byte ();
		break;
	case 3:
		prg.prgdata = (prg.prg[0] + prg.length - 2) & 0xff;
		prg.stateheader++;
		vc20_tape_byte ();
		break;
	case 4:
		prg.prgdata = ((prg.prg[0] + (prg.prg[1] << 8)) + prg.length - 2) >> 8;
		prg.stateheader++;
		i = 0;
		vc20_tape_byte ();
		break;
	case 5:
		if ((i != 16) && (prg.name[i] != 0))
		{
			prg.prgdata = prg.name[i];
			i++;
			vc20_tape_byte ();
			break;
		}
		prg.prgdata = 0x20;
		prg.stateheader++;
		vc20_tape_byte ();
		break;
	case 6:
		if (i != 192 - 5 - 1)
		{
			vc20_tape_byte ();
			i++;
			break;
		}
		prg.prgdata = prg.chksum;
		vc20_tape_byte ();
		prg.stateheader = 0;
		break;
	}
}

static void vc20_tape_program (void)
{
	static int i = 0;

	switch (prg.stateblock)
	{
	case 0:
		prg.pos = (9 + 192 + 1) * 2 + (9 + prg.length - 2 + 1) * 2;
		i = 0;
		prg.stateblock++;
		timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
		break;
	case 1:
		i++;
		if (i < 12000 /*27136 */ )
		{							   /* this time is not so important */
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		/* writing countdown $89 ... $80 */
		prg.stateblock++;
		prg.prgdata = 0x89;
		vc20_tape_byte ();
		break;
	case 2:
		if (prg.prgdata != 0x81)
		{
			prg.prgdata--;
			vc20_tape_byte ();
			break;
		}
		prg.stateblock++;
		vc20_tape_prgheader ();
		break;
	case 3:
		timer_reset (prg.timer, prg.lasttime = PCM_LONG);
		prg.stateblock++;
		i = 0;
		break;
	case 4:
		if (i < 80)
		{
			i++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		/* writing countdown $09 ... $00 */
		prg.prgdata = 9;
		prg.stateblock++;
		vc20_tape_byte ();
		break;
	case 5:
		if (prg.prgdata != 1)
		{
			prg.prgdata--;
			vc20_tape_byte ();
			break;
		}
		prg.stateblock++;
		vc20_tape_prgheader ();
		break;
	case 6:
		timer_reset (prg.timer, prg.lasttime = PCM_LONG);
		prg.stateblock++;
		i = 0;
		break;
	case 7:
		if (i < 80)
		{
			i++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		i = 0;
		prg.stateblock++;
		timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
		break;
	case 8:
		if (i < 3000 /*5376 */ )
		{
			i++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		prg.prgdata = 0x89;
		prg.stateblock++;
		vc20_tape_byte ();
		break;
	case 9:
		if (prg.prgdata != 0x81)
		{
			prg.prgdata--;
			vc20_tape_byte ();
			break;
		}
		i = 2;
		prg.chksum = 0;
		prg.prgdata = prg.prg[i];
		i++;
		vc20_tape_byte ();
		prg.stateblock++;
		break;
	case 10:
		if (i < prg.length)
		{
			prg.prgdata = prg.prg[i];
			i++;
			vc20_tape_byte ();
			break;
		}
		prg.prgdata = prg.chksum;
		vc20_tape_byte ();
		prg.stateblock++;
		break;
	case 11:
		timer_reset (prg.timer, prg.lasttime = PCM_LONG);
		prg.stateblock++;
		i = 0;
		break;
	case 12:
		if (i < 80)
		{
			i++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		/* writing countdown $09 ... $00 */
		prg.prgdata = 9;
		prg.stateblock++;
		vc20_tape_byte ();
		break;
	case 13:
		if (prg.prgdata != 1)
		{
			prg.prgdata--;
			vc20_tape_byte ();
			break;
		}
		prg.chksum = 0;
		i = 2;
		prg.prgdata = prg.prg[i];
		i++;
		vc20_tape_byte ();
		prg.stateblock++;
		break;
	case 14:
		if (i < prg.length)
		{
			prg.prgdata = prg.prg[i];
			i++;
			vc20_tape_byte ();
			break;
		}
		prg.prgdata = prg.chksum;
		vc20_tape_byte ();
		prg.stateblock++;
		break;
	case 15:
		timer_reset (prg.timer, prg.lasttime = PCM_LONG);
		prg.stateblock++;
		i = 0;
		break;
	case 16:
		if (i < 80)
		{
			i++;
			timer_reset (prg.timer, prg.lasttime = PCM_SHORT);
			break;
		}
		prg.stateblock = 0;
		break;
	}

}

static void vc20_prg_timer (int data)
{
	if (!tape.data)
	{								   /* send the same low phase */
		if (tape.noise)
			DAC_data_w (0, 0);
		tape.data = 1;
		timer_reset (prg.timer, prg.lasttime);
	}
	else
	{
		if (tape.noise)
			DAC_data_w (0, TONE_ON_VALUE);
		tape.data = 0;
		if (prg.statebit)
		{
			vc20_tape_bit (0);
		}
		else if (prg.statebyte)
		{							   /* send the rest of the byte */
			vc20_tape_byte ();
		}
		else if (prg.stateheader)
		{
			vc20_tape_prgheader ();
		}
		else
		{
			vc20_tape_program ();
			if (!prg.stateblock)
			{
				prg.timer = 0;
				tape.play = 0;
			}
		}
	}
	if (tape.read_callback)
		tape.read_callback (0, tape.data);
	vc20_prg_state ();
}

static void vc20_zip_timer (int data);
static void vc20_zip_state (void)
{
	switch (zip.state)
	{
	case 0:
		break;						   /* not inited */
	case 1:						   /* off */
		if (tape.on)
		{
			zip.state = 2;
			break;
		}
		break;
	case 2:						   /* on */
		if (!tape.on)
		{
			zip.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			break;
		}
		if (tape.motor && tape.play)
		{
			zip.state = 3;
			prg.timer = timer_set (0.0, 0, vc20_zip_timer);
			break;
		}
		if (tape.motor && tape.record)
		{
			zip.state = 4;
			break;
		}
		break;
	case 3:						   /* reading */
		if (!tape.on)
		{
			zip.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			if (prg.timer)
				timer_remove (prg.timer);
			break;
		}
		if (!tape.motor || !tape.play)
		{
			zip.state = 2;
			if (prg.timer)
				timer_remove (prg.timer);
			DAC_data_w (0, 0);
			break;
		}
		break;
	case 4:						   /* saving */
		if (!tape.on)
		{
			zip.state = 1;
			tape.play = 0;
			tape.record = 0;
			DAC_data_w (0, 0);
			timer_remove (prg.timer);
			break;
		}
		if (!tape.motor || !tape.record)
		{
			zip.state = 2;
			timer_remove (prg.timer);
			DAC_data_w (0, 0);
			break;
		}
		break;
	}
}

static void vc20_zip_readfile (void)
{
	int i;
	char *cp;

	for (i = 0; i < 2; i++)
	{
		zip.zipentry = readzip (zip.zip);
		if (zip.zipentry == NULL)
		{
			i++;
			rewindzip (zip.zip);
			continue;
		}
		if ((cp = strrchr (zip.zipentry->name, '.')) == NULL)
			continue;
		if (stricmp (cp, ".prg") == 0)
			break;
	}

	if (i == 2)
	{
		zip.state = 0;
		return;
	}
	for (i = 0; zip.zipentry->name[i] != 0; i++)
		prg.name[i] = toupper (zip.zipentry->name[i]);
	for (; i < 16; i++)
		prg.name[i] = ' ';

	prg.length = zip.zipentry->uncompressed_size;
	if ((prg.prg = (UINT8 *) malloc (prg.length)) == NULL)
	{
		logerror("out of memory\n");
		zip.state = 0;
	}
	readuncompresszip (zip.zip, zip.zipentry, (char *) prg.prg);
}

static void vc20_zip_open (int image_type, int image_id)
{
	if (!(zip.zip = openzip (device_filename(image_type,image_id))))
	{
		logerror("tape %s not found\n", device_filename(image_type,image_id));
		return;
	}

	logerror("tape %s linked\n", device_filename(image_type,image_id));

	tape.type = TAPE_ZIP;
	tape.on = 1;
	zip.image_type = image_type;
    zip.image_id = image_id;
	zip.state = 2;
	prg.stateblock = 0;
	prg.stateheader = 0;
	prg.statebyte = 0;
	prg.statebit = 0;
	prg.pos = 0;
	vc20_zip_readfile ();
}

static void vc20_zip_timer (int data)
{
	if (!tape.data)
	{								   /* send the same low phase */
		if (tape.noise)
			DAC_data_w (0, 0);
		tape.data = 1;
		timer_reset (prg.timer, prg.lasttime);
	}
	else
	{
		if (tape.noise)
			DAC_data_w (0, TONE_ON_VALUE);
		tape.data = 0;
		if (prg.statebit)
		{
			vc20_tape_bit (0);
		}
		else if (prg.statebyte)
		{							   /* send the rest of the byte */
			vc20_tape_byte ();
		}
		else if (prg.stateheader)
		{
			vc20_tape_prgheader ();
		}
		else
		{
			vc20_tape_program ();
			if (!prg.stateblock)
			{
				/* loading next file of zip */
				timer_reset (prg.timer, 0.0);
				free (prg.prg);
				vc20_zip_readfile ();
			}
		}
	}
	if (tape.read_callback)
		tape.read_callback (0, tape.data);
	vc20_prg_state ();
}

void vc20_tape_open (void (*read_callback) (UINT32, UINT8))
{
	tape.read_callback = read_callback;
#ifndef NEW_GAMEDRIVER
	tape.type = 0;
	tape.on = 0;
	tape.noise = 0;
	tape.play = 0;
	tape.record = 0;
	tape.motor = 0;
	tape.data = 0;
#endif
	prg.c16 = 0;
}

void c16_tape_open (void)
{
	vc20_tape_open (NULL);
	prg.c16 = 1;
}

int vc20_tape_attach_image (int id)
{
    char *cp;

	tape.type = 0;
	tape.on = 0;
	tape.noise = 0;
	tape.play = 0;
	tape.record = 0;
	tape.motor = 0;
	tape.data = 0;

	if (device_filename(IO_CASSETTE,id) == NULL)
		return INIT_PASS;

	if ((cp = strrchr (device_filename(IO_CASSETTE,id), '.')) == NULL)
		return INIT_FAIL;
	if (stricmp (cp, ".wav") == 0)
	{
		vc20_wav_open (IO_CASSETTE,id);
	}
	else if (stricmp (cp, ".prg") == 0)
	{
		vc20_prg_open (IO_CASSETTE,id);
	}
	else if (stricmp (cp, ".zip") == 0)
	{
		vc20_zip_open (IO_CASSETTE,id);
	}
	else
		return INIT_FAIL;
	return INIT_PASS;
}

void vc20_tape_detach_image (int id)
{
	vc20_tape_close();
}

void vc20_tape_close (void)
{
	switch (tape.type)
	{
	case TAPE_WAV:
		free (wav.sample);
		break;
	case TAPE_PRG:
		free (prg.prg);
		break;
	case TAPE_ZIP:
		free (prg.prg);
		closezip (zip.zip);
		break;
	}
	/* HJB reset so vc20_tape_close() can be called multiple times!? */
    tape.type = 0;
}

static void vc20_state (void)
{
	switch (tape.type)
	{
	case TAPE_WAV:
		vc20_wav_state ();
		break;
	case TAPE_PRG:
		vc20_prg_state ();
		break;
	case TAPE_ZIP:
		vc20_zip_state ();
		break;
	}
}

int vc20_tape_switch (void)
{
	int data = 1;

	switch (tape.type)
	{
	case TAPE_WAV:
		data = !((wav.state > 1) && (tape.play || tape.record));
		break;
	case TAPE_PRG:
		data = !((prg.state > 1) && (tape.play || tape.record));
		break;
	case TAPE_ZIP:
		data = !((zip.state > 1) && (tape.play || tape.record));
		break;
	}
	return data;
}

int vc20_tape_read (void)
{
	switch (tape.type)
	{
	case TAPE_WAV:
		if (wav.state == 3)
			return tape.data;
		break;
	case TAPE_PRG:
		if (prg.state == 3)
			return tape.data;
		break;
	case TAPE_ZIP:
		if (zip.state == 3)
			return tape.data;
		break;
	}
	return 0;
}

/* here for decoding tape formats */
void vc20_tape_write (int data)
{
	switch (tape.type)
	{
	case TAPE_WAV:
		if (wav.state == 4)
			vc20_wav_write (data);
		break;
	case TAPE_PRG:
		if (prg.state == 4)
			vc20_prg_write (data);
		break;
	case TAPE_ZIP:
		if (zip.state == 4)
			vc20_prg_write (data);
		break;
	}
}

void vc20_tape_config (int on, int noise)
{
	switch (tape.type)
	{
	case TAPE_WAV:
		tape.on = (wav.state != 0) && on;
		break;
	case TAPE_PRG:
		tape.on = (prg.state != 0) && on;
		break;
	case TAPE_ZIP:
		tape.on = (zip.state != 0) && on;
		break;
	}
	tape.noise = tape.on && noise;
	vc20_state ();
}

void vc20_tape_buttons (int play, int record, int stop)
{
	if (stop)
	{
		tape.play = 0, tape.record = 0;
	}
	else if (play && !tape.record)
	{
		tape.play = tape.on;
	}
	else if (record && !tape.play)
	{
		tape.record = tape.on;
	}
	vc20_state ();
}

void vc20_tape_motor (int data)
{
	tape.motor = !data;
	vc20_state ();
}

void vc20_tape_status (char *text, int size)
{
	text[0] = 0;
	switch (tape.type)
	{
	case TAPE_WAV:
		switch (wav.state)
		{
		case 4:
			snprintf (text, size, "Tape saving");
			break;
		case 3:
			snprintf (text, size, "Tape (%s) loading %d/%dsec",
					  device_filename(wav.image_type, wav.image_id),
					  wav.pos / wav.sample->smpfreq,
					  wav.sample->length / wav.sample->smpfreq);
			break;
		}
		break;
	case TAPE_PRG:
		switch (prg.state)
		{
		case 4:
			snprintf (text, size, "Tape saving");
			break;
		case 3:
			snprintf (text, size, "Tape (%s) loading %d",
				device_filename(prg.image_type, prg.image_id), prg.pos);
			break;
		}
		break;
	case TAPE_ZIP:
		switch (zip.state)
		{
		case 4:
			snprintf (text, size, "Tape saving");
			break;
		case 3:
			snprintf (text, size, "Tape (%s) File %s loading %d",
				device_filename(zip.image_type,zip.image_id), zip.zipentry->name, prg.pos);
			break;
		}
		break;
	}
}
