/**************************************************************************

	coco_cas.c

	Format code for Sord M5 Cassettes

**************************************************************************/

#include <string.h>
#include "sord_cas.h"

#define CAS_SAMPLERATE     22050   // output samplerate
#define CAS_SAMPLELEN      14      // length in Hz of one short sample
#define CAS_SILENCE        1       // silence in seconds
#define CAS_TONE_HEADER    2.4     // duration of fixed tone in secs (header)
#define CAS_TONE_DATA      0.15    // duration of fixed tone in secs (data)

static const UINT8 CasHeader[6] = { 'S', 'O', 'R', 'D', 'M', '5'};
static int saved_caslen;

static casserr_t sord_cas_to_wav (const UINT8 *casdata, int caslen, INT16 **wavdata, int *wavlen)
{
	int cas_pos, samples_size, samples_pos, size;
	INT16 *samples, *nsamples;
	UINT8 iByte, iBlockType;
	UINT16 iSampleArray[2] = { 0xC000, 0x4000};
	UINT32 i, j, k, len, iBlockSize, bit, crc;
	float iSampSize, iSampLeft;

	// check header
	if (caslen < 16) return 1;
	if (memcmp( casdata, CasHeader, sizeof(CasHeader)) != 0) return CASSETTE_ERROR_INVALIDIMAGE;
	cas_pos = 16;
	// alloc mem (size not really important)
	samples_size = 666;
	samples = (INT16*) malloc( samples_size * 2);
	if (!samples) return CASSETTE_ERROR_OUTOFMEMORY;
	//
	iSampSize = (CAS_SAMPLELEN * ((float)CAS_SAMPLERATE / 44100)) / 2;
	iSampLeft = 0;
	//
	samples_pos = 0;
	while (cas_pos < caslen)
	{
		// get block type
		iBlockType = casdata[cas_pos+0];
		if ((iBlockType != 'H') && (iBlockType != 'D'))
		{
			free( samples);
			return CASSETTE_ERROR_INVALIDIMAGE;
		}
		// get block size
		iBlockSize = casdata[cas_pos+1];
		if (iBlockSize == 0) iBlockSize = 0x100;
		iBlockSize += 3;
		
		//printf( "block type=[%c] size=[%d]\r\n", iBlockType, iBlockSize);
		
		// re-allocate memory (for worst case scenario)
		size = (CAS_SAMPLERATE * CAS_SILENCE) + (CAS_SAMPLERATE * CAS_TONE_HEADER) + (0x103 * (8+2) * (CAS_SAMPLELEN*2));
		if ((samples_pos + size) >= samples_size)
		{
			samples_size += size;
			nsamples = (INT16*) realloc( samples, samples_size * 2);
			if (!nsamples)
			{
				free( samples);
				return CASSETTE_ERROR_OUTOFMEMORY;
			}
			samples = nsamples;
		}
		//
		bit = 0;
		iSampLeft = 0;
		// add silence
		if (iBlockType == 'H')
		{
			iSampLeft = CAS_SAMPLERATE * CAS_SILENCE;
			while (iSampLeft >= 1) { *(samples + samples_pos++) = 0; iSampLeft--; }
		}
		// add fixed tone
		len = ((float)CAS_SAMPLERATE / (iSampSize * 2));
		if (iBlockType == 'H') len *= CAS_TONE_HEADER; else len *= CAS_TONE_DATA;
		for (i=0;i<len;i++)
		{
			for (j=0;j<2;j++)
			{
				iSampLeft += (iSampSize*1);
				while (iSampLeft >= 1) { *(samples + samples_pos++) = iSampleArray[j]; iSampLeft--; }
			}
		}
		// process block
		crc = 0;
		for (i=0;i<iBlockSize;i++)
		{
			//
			iByte = casdata[cas_pos+i];
			// calc/check checksum
			if (0)
			{
				if (i == iBlockSize)
				{
					if (iByte != (crc & 0xFF))
					{
						free( samples);
						return CASSETTE_ERROR_INVALIDIMAGE;
					};
				}
				if (i > 2) crc += iByte;
			}
			// process byte
			for (j=0;j<10;j++)
			{
				if (j < 2)
				{
					bit = ((j==0)?1:0);
				}
				else
				{
					bit = (iByte >> (j-2)) & 1;
				}
				//
				for (k=0;k<2;k++)
				{
					if (bit == 1) iSampLeft += (iSampSize*1); else iSampLeft += (iSampSize*2);
					while (iSampLeft >= 1) { *(samples + samples_pos++) = iSampleArray[k]; iSampLeft--; }
				}
			}
		}
		// mark end of block
		for (k=0;k<2;k++)
		{
			iSampLeft += (iSampSize*1);
			while (iSampLeft >= 1) { *(samples + samples_pos++) = iSampleArray[k]; iSampLeft--; }
		}
		// next block
		cas_pos += iBlockSize;
	}
	// add silence
	iSampLeft = CAS_SAMPLERATE * CAS_SILENCE;
	while (iSampLeft >= 1) { *(samples + samples_pos++) = (INT16)0xFFFF; iSampLeft--; }
	// return data
	if (wavdata != NULL) *wavdata = samples; else free( samples);
	if (wavlen != NULL) *wavlen = samples_pos;
	//
	return 0;
}

static int sord_cas_to_wav_size(const UINT8 *casdata, int caslen)
{
	int wavlen, err;
	
	saved_caslen = caslen;
	err = sord_cas_to_wav( casdata, caslen, NULL, &wavlen);
	return ((err==CASSETTE_ERROR_SUCCESS)?wavlen:-1);
}



static int sord_cas_fill_wave(INT16 *buffer, int length, UINT8 *bytes)
{
	INT16 *wavdata;
	int wavlen;

	if (sord_cas_to_wav(bytes, saved_caslen, &wavdata, &wavlen))
		return -1;

	memcpy(buffer, wavdata, wavlen * 2);
	free(wavdata);
	return saved_caslen;
}


static struct CassetteLegacyWaveFiller sordm5_legacy_fill_wave =
{
	sord_cas_fill_wave,						/* fill_wave */
	-1,										/* chunk_size */
	0,										/* chunk_samples */
	sord_cas_to_wav_size,					/* chunk_sample_calc */
	22050,									/* sample_frequency */
	0,										/* header_samples */
	0										/* trailer_samples */
};



static casserr_t sordm5_tap_identify(cassette_image *cassette, struct CassetteOptions *opts)
{
	return cassette_legacy_identify(cassette, opts, &sordm5_legacy_fill_wave);
}



static casserr_t sordm5_tap_load(cassette_image *cassette)
{
	return cassette_legacy_construct(cassette, &sordm5_legacy_fill_wave);
}



struct CassetteFormat sordm5_cas_format =
{
	"cas\0",
	sordm5_tap_identify,
	sordm5_tap_load,
	NULL
};



CASSETTE_FORMATLIST_START(sordm5_cassette_formats)
	CASSETTE_FORMAT(sordm5_cas_format)
CASSETTE_FORMATLIST_END
