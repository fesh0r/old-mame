#ifndef __SID_6581_H_
#define __SID_6581_H_

#include "driver.h"

/* 
   c64 / c128 sound interface
   cbmb series interface
   c16 sound card
   c64 hardware modification for second sid
   c65 has 2 inside

   selection between two type 6581 8580 would be nice 

   sid6582 6581 with other input voltages 
*/
#define MAX_SID6581 2

typedef enum { MOS6581, MOS8580 } SIDTYPE;
typedef struct {
	/* this is here, until this sound approximation is added to
	   mame's sound devices */
	struct CustomSound_interface custom;

	int count;
	struct {
		/* bypassed to mixer_allocate_channel, so use
		   the macros defined in src/sound/mixer.h to load values*/
		int default_mixer_level;
		SIDTYPE type;
		int clock;
		int (*ad_read)(int channel);
	} chips[MAX_SID6581];
} SID6581_interface;

extern void sid6581_set_type(int number, SIDTYPE type);

extern void sid6581_reset (int number);

extern void sid6581_update(void);

extern WRITE_HANDLER ( sid6581_0_port_w );
extern WRITE_HANDLER ( sid6581_1_port_w );
extern READ_HANDLER  ( sid6581_0_port_r );
extern READ_HANDLER  ( sid6581_1_port_r );

int sid6581_custom_start (const struct MachineSound *driver);
void sid6581_custom_stop(void);
void sid6581_custom_update(void);

#endif
