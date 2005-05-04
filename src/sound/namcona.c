/*
    Namco NA1/2 Sound Hardware

    PCM samples and sound sequencing metadata are written by the main CPU to
    shared RAM.

    The sound CPU's type is unknown, though it appears to be little endian.
    It has an internal BIOS.

    RAM[0x820]:         song select
    RAM[0x822]:         song control (fade?)
    RAM[0x824..0x89e]:
        Even addresses are used to select the sound effect.
        0x40 is written to odd addresses as a signal to play the requested sound.

    Metadata vectors:
    addr  sample
    0000: 0012   // table of addresses for each VOX sequence
    0002: 07e4   // table of wave records (5 words each record)
    0004: 0794   // unknown table (5 words each record)
    0006: 0000   // unknown (always zero?)
    0008: 0102   // unknown address table
    000a: 0384   // unknown address table
    000c: 0690   // unknown chunk
    000e: 07bc   // unknown table (5 words each record)
    0010: 7700   // unknown
    0012: addresses for each song sequence start here

    Known issues:
    - many opcodes are ignored or implemented imperfectly
    - the metadata contains several mystery tables
*/

#include <math.h>
#include "driver.h"
#include "namcona.h"

#define kTwelfthRootTwo 1.059463094
#define FIXED_POINT_SHIFT (10) /* for mixing */
#define MAX_VOICE 16
#define MAX_SEQUENCE  0x40
#define MAX_SEQUENCE_RECURSION 4 /* ? */

struct voice
{
	INT32 bActive;

	INT32 flags;
	INT32 start; /* fixed point */
	INT32 end;   /* fixed point */
	INT32 loop;  /* fixed point */
	INT32 baseFreq;
	INT32 bank;

	INT32 delta; /* fixed point */
	INT32 pos;
	INT32 volume;
	INT32 masterVolume; /* copied from pSequence->volume each time a note is played */
	INT32 pan;
	INT32 dnote;
	INT32 detune;
};

struct sequence
{
	data8_t volume;
	data8_t reg2; /* master freq? */
	data8_t tempo;
	int addr;
	int pause;
	int channel[8];
	int stackData[MAX_SEQUENCE_RECURSION]; /* used for sub-sequences */
	int stackSize;
	int count; /* used for "repeat" opcode */
	int count2;
	int count3;
};

struct namcona
{
	int mSampleRate;
	sound_stream * mStream;
	INT16 *mpMixerBuffer;
	INT32 *mpPitchTable;
	data16_t *mpROM;
	data16_t *mpMetaData;
	struct sequence mSequence[MAX_SEQUENCE];
	struct voice mVoice[MAX_VOICE];
};


/**
 * Given a sequence, return a pointer to the associated 16 bit word in shared RAM.
 * The 16 bits are interpretted as follows:
 * xxxxxxxx-------- index into sequence table (selects song/sample)
 * --------x------- 1 if sequence is active/playing
 * ---------x------ 1 if the main CPU has written a new sound command
 * ----------xxxxxx unknown/unused
 */
data16_t *
GetSequenceStatusAddr( struct namcona *chip, struct sequence *pSeq )
{
	int offs = pSeq - chip->mSequence;
	return &chip->mpROM[0x820/2+offs];
}

static void
Silence( struct namcona *chip )
{
	int i;
	for( i=0; i<MAX_VOICE; i++ )
	{
		chip->mVoice[i].bActive = 0;
	}
	for( i=0; i<MAX_SEQUENCE; i++ )
	{
		data16_t *pStatus = GetSequenceStatusAddr(chip, &chip->mSequence[i]);
		*pStatus &= 0xff7f; /* wipe "sequence-is-playing" flag */
	}
}

static data8_t
ReadMetaDataByte( struct namcona *chip, int addr )
{
	data16_t data = chip->mpMetaData[addr/2];
	return (addr&1)?(data&0xff):(data>>8);
} /* ReadMetaDataByte */

static data16_t
ReadMetaDataWord( struct namcona *chip, int addr )
{
	return ReadMetaDataByte(chip, addr)+ReadMetaDataByte(chip, addr+1)*256;
} /* ReadMetaDataWord */

static signed char
ReadPCMSample(struct namcona *chip, int addr, int flag )
{
	data16_t data16 = chip->mpROM[addr/2];
	int dat = (addr&1)?(data16&0xff):(data16>>8);

	if( flag&0x100 )
	{
		if( dat&0x80 )
		{
			dat = -(dat&0x7f) - 1;
		}
	}

	return dat;
} /* ReadPCMSample */

static void
RenderSamples(struct namcona *chip, stream_sample_t **buffer, INT16 *pSource, int length )
{
	int i;
	stream_sample_t * pDest1 = buffer[0];
	stream_sample_t * pDest2 = buffer[1];
	for (i = 0; i < length; i++)
	{
		INT32 dataL = /* 100 * */ (*pSource++);
		INT32 dataR = /* 100 * */ (*pSource++);
		if( dataL > 0x7fff )
		{
			dataL =  0x7fff; /* clip */
		}
		else if( dataL < -0x8000 )
		{
			dataL = -0x8000; /* clip */
		}
		if( dataR > 0x7fff )
		{
			dataR =  0x7fff; /* clip */
		}
		else if( dataR < -0x8000 )
		{
			dataR = -0x8000; /* clip */
		}
		*pDest1++ = (INT16)dataL; /* stereo left */
		*pDest2++ = (INT16)dataR; /* stereo right */
	}
} /* RenderSamples */

static void
PushSequenceAddr(struct namcona *chip, struct sequence *pSequence, int addr )
{
	if( pSequence->stackSize<MAX_SEQUENCE_RECURSION )
	{
		pSequence->stackData[pSequence->stackSize++] = addr;
	}
	else
	{
		logerror( "sound/namcona.c stack overflow!\n" );
	}
} /* PushSequenceAddr */

static void
PopSequenceAddr(struct namcona *chip, struct sequence *pSequence )
{
	if( pSequence->stackSize )
	{
		pSequence->addr = pSequence->stackData[--pSequence->stackSize];
	}
	else
	{
		data16_t *pStatus = GetSequenceStatusAddr(chip,pSequence);
		*pStatus &= 0xff7f; /* wipe "sequence-is-playing" flag */
	}
} /* PopSequenceAddr */

static void
HandleSubroutine(struct namcona *chip, struct sequence *pSequence )
{
	int addr = ReadMetaDataWord(chip, pSequence->addr );
	PushSequenceAddr(chip, pSequence, pSequence->addr+2 );
	pSequence->addr = addr;
} /* HandleSubroutine */

static void
HandleRepeat(struct namcona *chip, struct sequence *pSequence )
{
	int count = ReadMetaDataByte( chip, pSequence->addr++ );
	int addr = ReadMetaDataWord(chip, pSequence->addr );
	if( pSequence->count < count )
	{
		pSequence->count++;
		pSequence->addr = addr;
	}
	else
	{
		pSequence->count = 0;
		pSequence->addr += 2;
	}
} /* HandleRepeat */

static void
HandleRepeatOut(struct namcona *chip, struct sequence *pSequence )
{
	int count = ReadMetaDataByte( chip, pSequence->addr++ );
	int addr = ReadMetaDataWord(chip, pSequence->addr );
	if( pSequence->count2 < count )
	{
		pSequence->count2++;
		pSequence->addr += 2;
	}
	else
	{
		pSequence->count2 = 0;
		pSequence->addr = addr;
	}
} /* HandleRepeatOut */

static void
MapArgs(struct namcona *chip, struct sequence *pSequence,
		 int bCommon,
		 void (*callback)( struct namcona *chip, struct sequence *, int chan, data8_t data ) )
{
	data8_t set = ReadMetaDataByte(chip,pSequence->addr++);
	data8_t data = 0;
	int i;
	if( bCommon )
	{
		data = ReadMetaDataByte( chip, pSequence->addr++ );
	}
	for( i=0; i<8; i++ )
	{
		if( set&(1<<(7-i)) )
		{
			if( !bCommon )
			{
				data = ReadMetaDataByte( chip,pSequence->addr++ );
			}
			callback( chip, pSequence, i, data );
		}
	}
} /* MapArgs */

static void
AssignChannel(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	if( data<MAX_VOICE )
	{
		struct voice *pVoice = &chip->mVoice[data];
		pSequence->channel[chan] = data;
		pVoice->bActive = 0;
		pVoice->bank = 0;
		pVoice->volume = 0x80;
		pVoice->pan = 0x80;
		pVoice->dnote = 0;
		pVoice->detune = 0;
	}
} /* AssignChannel */

static void
IgnoreUnknownOp(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
} /* IgnoreUnknownOp */

static void
SelectWave(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	int bank = 0x20000 + pVoice->bank*0x20000;
	int addr  = ReadMetaDataWord(chip,2)+10*data;

	pVoice->flags    = ReadMetaDataWord(chip,addr+0*2);
	pVoice->start    = ReadMetaDataWord(chip,addr+1*2)*2+bank;
	pVoice->end      = ReadMetaDataWord(chip,addr+2*2)*2+bank;
	pVoice->loop     = ReadMetaDataWord(chip,addr+3*2)*2+bank;
	pVoice->baseFreq = ReadMetaDataWord(chip,addr+4*2); /* unsure what this is; not currently used */

	pVoice->start <<= FIXED_POINT_SHIFT;
	pVoice->end   <<= FIXED_POINT_SHIFT;
	pVoice->loop  <<= FIXED_POINT_SHIFT;

	pVoice->bActive = 0;
	pVoice->dnote = 0;
	pVoice->detune = 0;
} /* SelectWave */

static void
PlayNote(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	if( data & 0x80 )
	{
		pVoice->bActive = 0;
	}
	else
	{
		data16_t Note = (data<<8) + (pVoice->dnote<<8) + pVoice->detune + pVoice->baseFreq;
		if (Note < 0x7f00)
		{
			pVoice->delta = chip->mpPitchTable[Note>>8];
			pVoice->delta += (chip->mpPitchTable[(Note>>8)+1]-pVoice->delta) *(Note&0xff)/256;

			pVoice->bActive = 1;
			pVoice->pos = pVoice->start;
			pVoice->masterVolume = pSequence->volume;
		}
		else
		{
			pVoice->bActive = 0;
		}
	}
} /* PlayNote */

static void
Detune(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	pVoice->detune = data;
} /* Detune */


static void
DNote(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	pVoice->dnote = data;
} /* DNote */

static void
Pan(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	pVoice->pan = data;
} /* Pan */

static void
Volume(struct namcona *chip, struct sequence *pSequence, int chan, data8_t data )
{
	struct voice *pVoice = &chip->mVoice[pSequence->channel[chan]];
	pVoice->volume = data;
} /* Volume */

static void
UpdateSequence(struct namcona *chip, struct sequence *pSequence )
{
	data16_t *pStatus = GetSequenceStatusAddr(chip,pSequence);
	data16_t data = *pStatus;

	if( data&0x0040 )
	{ /* bit 0x0040 indicates that a sound request was written by the main CPU */
		int offs = ReadMetaDataWord(chip,0)+(data>>8)*2;
		memset( pSequence, 0x00, sizeof(struct sequence) );
		if( pSequence == &chip->mSequence[0] ) Silence(chip); /* hack! */
		pSequence->addr = ReadMetaDataWord(chip,offs);
		*pStatus = (data&0xffbf)|0x0080; /* set "sequence-is-playing" flag */
	}

	while( (*pStatus)&0x0080 )
	{
		if( pSequence->pause )
		{
			pSequence->pause--;
			return;
		}
		else
		{
			int code = ReadMetaDataByte(chip,pSequence->addr++);
			if( code&0x80 )
			{
				pSequence->pause = pSequence->tempo*((code&0x3f)+1);
			}
			else
			{
				int bCommon = (code&0x40);
				switch( code&0x3f )
				{
				case 0x01: /* master volume */
					pSequence->volume = ReadMetaDataByte(chip,pSequence->addr++);
					break;

				case 0x02: /* master freq adjust? */
					pSequence->reg2 = ReadMetaDataByte(chip,pSequence->addr++);
					break;

				case 0x03: /* tempo */
					pSequence->tempo = ReadMetaDataByte(chip,pSequence->addr++)/2;
					if (pSequence->tempo == 0) pSequence->tempo = 1;
					break;

				case 0x04:
					HandleSubroutine(chip, pSequence );
					break;

				case 0x05: /* end-of-sequence */
					PopSequenceAddr(chip, pSequence );
					break;

				case 0x06: /* operand is note index */
					MapArgs(chip, pSequence, bCommon, PlayNote );
					pSequence->pause = pSequence->tempo;
					break;

				case 0x07:
					MapArgs(chip, pSequence, bCommon, SelectWave );
					break;

				case 0x08: /* attack? */
					MapArgs(chip, pSequence, bCommon, Volume );
					break;

				case 0x09:
					pSequence->addr = ReadMetaDataWord(chip,pSequence->addr);
					break;

				case 0x0a:
					HandleRepeat(chip, pSequence );
					break;

				case 0x0b:
					HandleRepeatOut(chip, pSequence );
					break;

				case 0x0c:
					MapArgs(chip, pSequence, bCommon, DNote );
					break;

				case 0x0d:
					MapArgs(chip, pSequence, bCommon, Detune );
					break;

				case 0x0e: /* ? */
				case 0x0f: /* ? */
				case 0x10: /* ? */
				case 0x11: /* ? */
				case 0x12: /* ? */
				case 0x13: /* always used? normally 0x00 */
				case 0x14: /* ? */
					MapArgs(chip, pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x16:
					MapArgs(chip, pSequence, bCommon, Pan );
					break;

				case 0x17:
					MapArgs(chip, pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x19: // one loop?
					if (pSequence->count3 == 0)
					{
						pSequence->addr = ReadMetaDataWord(chip,pSequence->addr);
						pSequence->count3++;
					}
					else
					{
						pSequence->addr += 2;
						pSequence->count3 = 0;
					}
					break;

				case 0x1b: // ?
					pSequence->addr += 2;
					break;

				case 0x1c: // ?
					pSequence->addr += 2;
					break;

				case 0x1e:
					{
						int no = ReadMetaDataByte(chip,pSequence->addr++);	/* Sequence No */
						int cod = ReadMetaDataByte(chip,pSequence->addr++);
						if (no < MAX_SEQUENCE)
						{
							struct sequence *pSequence2 = &chip->mSequence[no];
							data16_t *pStatus2 = GetSequenceStatusAddr(chip,pSequence2);
							int offs = 0x12+cod*2;
							*pStatus2 = (cod<<8)|0x0080;
							memset( pSequence2, 0x00, sizeof(struct sequence) );
							pSequence2->addr = ReadMetaDataWord(chip,offs);
						}
					}
					break;

				case 0x20:
					MapArgs(chip, pSequence, bCommon, AssignChannel );
					break;

				case 0x22:
					MapArgs(chip, pSequence, bCommon, IgnoreUnknownOp );
					break;

				case 0x23:
					{
						data8_t reg23_0 = ReadMetaDataByte(chip,pSequence->addr++); /* Channel select */
						data8_t reg23_1 = ReadMetaDataByte(chip,pSequence->addr++); /* PCM bank select */
						/* reg23_0: 0 = Ch. 0- 3 PCM Bank select
                                    1 = Ch. 4- 7 PCM Bank select
                                    2 = Ch. 8-11 PCM Bank select
                                    3 = Ch.12-15 PCM Bank select
                        */
						if( reg23_0 < 4 )
						{
							chip->mVoice[reg23_0*4 + 0].bank = reg23_1;
							chip->mVoice[reg23_0*4 + 1].bank = reg23_1;
							chip->mVoice[reg23_0*4 + 2].bank = reg23_1;
							chip->mVoice[reg23_0*4 + 3].bank = reg23_1;
						}
					}
					break;

				case 0x24:
					MapArgs(chip, pSequence, bCommon, IgnoreUnknownOp );
					break;

				default:
					//printf( "? 0x%x\n", code&0x3f );
					*pStatus &= 0xff7f; /* clear "sequence-is-playing" flag */
					break;
				}
			}
		}
	}
} /* UpdateSequence */

static void
UpdateSound( void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length )
{
	struct namcona *chip = param;
	int i;

	for( i=0; i<MAX_SEQUENCE; i++ )
	{
		UpdateSequence(chip, &chip->mSequence[i] );
	}

	if( length>chip->mSampleRate ) length = chip->mSampleRate;
	memset(chip->mpMixerBuffer, 0, length * sizeof(INT16) * 2);
	for( i=0;i<MAX_VOICE;i++ )
	{
		struct voice *pVoice = &chip->mVoice[i];
		if( pVoice->bActive && pVoice->delta )
		{
			INT32 delta  = pVoice->delta;
			INT32 end    = pVoice->end;
			INT32 pos    = pVoice->pos;
			INT32 vol    = pVoice->volume*pVoice->masterVolume/8;
			INT32 pan    = pVoice->pan;
			INT16 *pDest = chip->mpMixerBuffer;
			INT16 dat;
			int j;
			for( j=0; j<length; j++ )
			{
				if( pos >= end )
				{
					if( pVoice->flags&0x1000 )
					{
						pos = pos - end + pVoice->loop;
					}
					else
					{
						pVoice->bActive = 0;
						break;
					}
				}
				dat = ReadPCMSample(chip,pos>>FIXED_POINT_SHIFT, pVoice->flags)*vol/256;
				*pDest++ += dat*(0x100-pan)/256;
				*pDest++ += dat*pan/256;
				pos += delta;
			}
			pVoice->pos = pos;
		}
	}
	RenderSamples(chip, buffer, chip->mpMixerBuffer, length );
} /* UpdateSound */

static void *namcona_start(int sndindex, int clock, const void *config)
{
	const struct NAMCONAinterface *intf = config;
	struct namcona *chip;
	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));
	chip->mSampleRate = clock;
	chip->mStream = stream_create(0, 2, chip->mSampleRate, chip, UpdateSound);
	chip->mpROM = (data16_t *)memory_region(REGION_CPU1);
	chip->mpMetaData = chip->mpROM+intf->metadata_offset;

	memset( chip->mVoice, 0x00, sizeof(chip->mVoice) );
	memset( chip->mSequence, 0x00, sizeof(chip->mSequence) );

	chip->mpMixerBuffer = auto_malloc( sizeof(INT16)*chip->mSampleRate*2 );
	if( chip->mpMixerBuffer )
	{
		chip->mpPitchTable = auto_malloc( sizeof(INT32)*0xff );
		if( chip->mpPitchTable )
		{
			int i;
			for( i=0; i<0xff; i++ )
			{
				int data = i;
				double freq = freq = (1<<FIXED_POINT_SHIFT)/4;
				while( data>0x3a )
				{
					data--;
					freq *= kTwelfthRootTwo;
				}
				while( data<0x3a )
				{
					data++;
					freq /= kTwelfthRootTwo;
				}
				chip->mpPitchTable[i] = (INT32)freq;
			}
			return chip;
		}
	}
	return NULL;
} /* NAMCONA_sh_start */

#if 0
static void
DumpSampleTable( FILE *f, struct namcona *chip, int table, unsigned char *special )
{
	int iStart = ReadMetaDataWord(chip,table);
	int i=iStart;
	fprintf( f, "\nsample table:\n" );
	while( i<0x10000 )
	{
		fprintf( f, "%04x(%02x): %04x %04x %04x %04x %04x (len=%d)\n",
			i, (i-iStart)/10,
			ReadMetaDataWord(chip,i+0*2), // flags
			ReadMetaDataWord(chip,i+1*2), // start
			ReadMetaDataWord(chip,i+2*2), // end
			ReadMetaDataWord(chip,i+3*2), // loop
			ReadMetaDataWord(chip,i+4*2), // freq
			ReadMetaDataWord(chip,i+2*2)-ReadMetaDataWord(chip,i+1*2) );
		i+=5*2;
		if( special[i] || special[i+1] ) break;
	}
	fprintf( f, "\n" );
}

static void
DumpBytes( FILE *f, struct namcona *chip, int addr, unsigned char *special )
{
	fprintf( f, "%04x:", addr );
	while( addr<0x10000 )
	{
		fprintf( f, " %02x", ReadMetaDataByte(chip,addr) );
		addr++;
		if( (addr&0xf)==0 ) fprintf( f, "\n%04x;", addr );
		if( special[addr] ) break;
	}
	fprintf( f, "\n" );
}

static void
DumpUnkTable( FILE *f, struct namcona *chip, int table, unsigned char *special )
{
	int iStart = ReadMetaDataWord(chip,table);
	int iFinish = ReadMetaDataWord(chip,iStart);
	int i,addr;

	if( iFinish<iStart )
	{ /* hack for CosmoGang */
		iFinish = ReadMetaDataWord(chip,iStart+2);
	}

	fprintf( f, "\nunk table; %d entries:\n", (iFinish-iStart)/2 );
	fprintf( f, "%04x:", iStart );


	for( i=iStart; i<iFinish; i+=2 )
	{
		addr = ReadMetaDataWord(chip,i);
		fprintf( f, " %04x", addr );
		special[addr] = 1;
	}
	fprintf( f,"\n" );
	for( i=iStart; i<iFinish; i+=2 )
	{
		addr = ReadMetaDataWord(chip,i);
		fprintf( f, "%04x:", addr );
		for(;;)
		{
			fprintf( f, " %02x", ReadMetaDataByte(chip,addr) );
			addr++;
			if( addr>=0x10000 || special[addr] ) break;
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n" );
}
#endif

static void
namcona_stop( void *chip )
{
	#if 0
	FILE *f = fopen("snd.txt","w");
	if( f )
	{
		unsigned char *special = malloc(0x10000);
		if( special )
		{
			int i,addr, table;
			memset( special, 0x00, 0x10000 );
			for( i=0; i<0x12; i+=2 )
			{
				addr = ReadMetaDataWord(chip,i);
				if( i ) special[addr] = 1;
				fprintf( f, "%04x: %04x\n", i, addr );
			}
			table = 0x12;
			i = table;
			while( i<0x10000 )
			{
				addr = ReadMetaDataWord(chip,i);
				special[addr] = 1;
				i+=2;
				if( special[i] ) break;
			}
			DumpSampleTable( f, chip, 0x0002, special );
			DumpSampleTable( f, chip, 0x0004, special );
			/* 0x0006 is unused */
			DumpUnkTable( f, chip, 0x0008, special );
			DumpUnkTable( f, chip, 0x000a, special );
			fprintf( f, "\nunknown chunk:\n" );
			DumpBytes(f, chip, ReadMetaDataWord(chip,0x0c), special );
			DumpSampleTable( f, chip, 0x000e, special );
			fprintf( f, "\nSong Table:\n" );
			i = 0x12;
			while( i<0x10000 )
			{
				addr = ReadMetaDataWord(chip,i);
				fprintf( f, "[%02x] ", (i-0x12)/2 );
				DumpBytes( f,chip,addr,special );
				i+=2;
				if( special[i] ) break;
			}
			free( special );
		}
		fclose( f );
	}
	#endif
} /* NAMCONA_sh_stop */




/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void namcona_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void namcona_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = namcona_set_info;		break;
		case SNDINFO_PTR_START:							info->start = namcona_start;			break;
		case SNDINFO_PTR_STOP:							info->stop = namcona_stop;				break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Namco NA";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Namco custom";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}

