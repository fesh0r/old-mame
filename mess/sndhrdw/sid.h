#ifndef __SID_H_
#define __SID_H_

/*
  approximation of the sid6581 chip
  this part is for one chip,
*/

#include "includes/sid6581.h"
#include "sidtypes.h"
#include "sidvoice.h"

/* private area */
typedef struct _SID6581 {
    int on;

    int mixer_channel; // mame stream/ mixer channel

    int (*ad_read) (int which);
    SIDTYPE type;
    udword clock;

    uword PCMfreq; // samplerate of the current systems soundcard/DAC
    udword PCMsid, PCMsidNoise;

#if 0
	/* following depends on type */
	ptr2sidVoidFunc ModeNormalTable[16];
	ptr2sidVoidFunc ModeRingTable[16];
	// for speed reason it could be better to make them global!
	UINT8* waveform30;
	UINT8* waveform50;
	UINT8* waveform60;
	UINT8* waveform70;
#endif
	int reg[0x20];

//	bool sidKeysOn[0x20], sidKeysOff[0x20];

	ubyte masterVolume;
	uword masterVolumeAmplIndex;


	struct {
		bool Enabled;
		UINT8 Type, CurType;
		filterfloat Dy, ResDy;
		UINT16 Value;
	} filter;

	sidOperator optr1, optr2, optr3;
    int optr3_outputmask;
} SID6581;

void sid6581_init (SID6581 *This);

bool sidEmuReset(SID6581 *This);

int sid6581_port_r (SID6581 *This, int offset);
void sid6581_port_w (SID6581 *This, int offset, int data);

void sid_set_type(SID6581 *This, SIDTYPE type);

void initMixerEngine(void);
void filterTableInit(void);
extern void MixerInit(bool threeVoiceAmplify);

void sidEmuFillBuffer(SID6581 *This, void* buffer, udword bufferLen );

#if 0
void sidFilterTableInit(void);
#endif

#endif
