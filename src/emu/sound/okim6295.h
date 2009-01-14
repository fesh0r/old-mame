#pragma once

#ifndef __OKIM6295_H__
#define __OKIM6295_H__

/* an interface for the OKIM6295 and similar chips */

/*
  Note about the playback frequency: the external clock is internally divided,
  depending on pin 7, by 132 (high) or 165 (low).
*/
typedef struct _okim6295_interface okim6295_interface;
struct _okim6295_interface
{
	int pin7;
	const char *rgnoverride;
};

extern const okim6295_interface okim6295_interface_pin7high;
extern const okim6295_interface okim6295_interface_pin7low;



void okim6295_set_bank_base(int which, int base);
void okim6295_set_pin7(int which, int pin7);

READ8_HANDLER( okim6295_status_0_r );
READ8_HANDLER( okim6295_status_1_r );
READ8_HANDLER( okim6295_status_2_r );
READ16_HANDLER( okim6295_status_0_lsb_r );
READ16_HANDLER( okim6295_status_1_lsb_r );
READ16_HANDLER( okim6295_status_2_lsb_r );
READ16_HANDLER( okim6295_status_0_msb_r );
READ16_HANDLER( okim6295_status_1_msb_r );
READ16_HANDLER( okim6295_status_2_msb_r );
WRITE8_HANDLER( okim6295_data_0_w );
WRITE8_HANDLER( okim6295_data_1_w );
WRITE8_HANDLER( okim6295_data_2_w );
WRITE16_HANDLER( okim6295_data_0_lsb_w );
WRITE16_HANDLER( okim6295_data_1_lsb_w );
WRITE16_HANDLER( okim6295_data_2_lsb_w );
WRITE16_HANDLER( okim6295_data_0_msb_w );
WRITE16_HANDLER( okim6295_data_1_msb_w );
WRITE16_HANDLER( okim6295_data_2_msb_w );

/*
    To help the various custom ADPCM generators out there,
    the following routines may be used.
*/
struct adpcm_state
{
	INT32	signal;
	INT32	step;
};
void reset_adpcm(struct adpcm_state *state);
INT16 clock_adpcm(struct adpcm_state *state, UINT8 nibble);

SND_GET_INFO( okim6295 );
#define SOUND_OKIM6295 SND_GET_INFO_NAME( okim6295 )

#endif /* __OKIM6295_H__ */
