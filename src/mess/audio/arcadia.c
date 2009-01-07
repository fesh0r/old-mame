/***************************************************************************

  PeT mess@utanet.at
  main part in video/

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "includes/arcadia.h"


#define VOLUME (arcadia_sound.reg[2]&0xf)
#define ON (!(arcadia_sound.reg[2]&0x10))

static struct
{
    sound_stream *channel;
    UINT8 reg[3];
    int size, pos;
    unsigned level;
} arcadia_sound;



void arcadia_soundport_w (running_machine *machine, int offset, int data)
{
	stream_update(arcadia_sound.channel);
	arcadia_sound.reg[offset] = data;
	switch (offset)
	{
	case 1:
	    arcadia_sound.pos = 0;
	    arcadia_sound.level = TRUE;
	    // frequency 7874/(data+1)
            arcadia_sound.size=machine->sample_rate*((data&0x7f)+1)/7874;
	    break;
	}
}



/************************************/
/* Sound handler update             */
/************************************/

static STREAM_UPDATE( arcadia_update )
{
	int i;
	stream_sample_t *buffer = outputs[0];

	for (i = 0; i < samples; i++, buffer++)
	{
		*buffer = 0;
		if (arcadia_sound.reg[1] && arcadia_sound.pos <= arcadia_sound.size/2)
		{
			*buffer = 0x2ff * VOLUME; // depends on the volume between sound and noise
		}
		if (arcadia_sound.pos <= arcadia_sound.size)
			arcadia_sound.pos++;
		if (arcadia_sound.pos > arcadia_sound.size)
			arcadia_sound.pos = 0;
	}
}



/************************************/
/* Sound handler start              */
/************************************/

static CUSTOM_START( arcadia_custom_start )
{
    arcadia_sound.channel = stream_create(device, 0, 1, device->machine->sample_rate, 0, arcadia_update);
    return (void *) ~0;
}



const custom_sound_interface arcadia_sound_interface =
{
	arcadia_custom_start
};
