/*********************************************************/
/*    Konami PCM controller                              */
/*********************************************************/

#pragma once

#ifndef __K007232_H__
#define __K007232_H__

typedef struct _k007232_interface k007232_interface;
struct _k007232_interface
{
	void (*portwritehandler)(int);
};

WRITE8_HANDLER( k007232_write_port_0_w );
WRITE8_HANDLER( k007232_write_port_1_w );
WRITE8_HANDLER( k007232_write_port_2_w );
READ8_HANDLER( k007232_read_port_0_r );
READ8_HANDLER( k007232_read_port_1_r );
READ8_HANDLER( k007232_read_port_2_r );

void k007232_set_bank( int chip, int chABank, int chBBank );

/*
  The 007232 has two channels and produces two outputs. The volume control
  is external, however to make it easier to use we handle that inside the
  emulation. You can control volume and panning: for each of the two channels
  you can set the volume of the two outputs. If panning is not required,
  then volumeB will be 0 for channel 0, and volumeA will be 0 for channel 1.
  Volume is in the range 0-255.
*/
void k007232_set_volume(int chip,int channel,int volumeA,int volumeB);

SND_GET_INFO( k007232 );
#define SOUND_K007232 SND_GET_INFO_NAME( k007232 )

#endif /* __K007232_H__ */
