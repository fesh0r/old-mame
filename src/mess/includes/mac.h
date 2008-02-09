/*****************************************************************************
 *
 * includes/mac.h
 * 
 * Macintosh driver declarations
 *
 ****************************************************************************/

#ifndef MAC_H_
#define MAC_H_

#include "sound/custom.h"


// video parameters
#define MAC_H_VIS	(512)
#define MAC_V_VIS	(342)
#define MAC_H_TOTAL	(704)		// (512+192)
#define MAC_V_TOTAL	(370)		// (342+28)


/*----------- defined in machine/mac.c -----------*/

MACHINE_RESET( mac );

DRIVER_INIT(mac128k512k);
DRIVER_INIT(mac512ke);
DRIVER_INIT(macplus);
DRIVER_INIT(macse);
DRIVER_INIT(macclassic);

READ16_HANDLER ( mac_via_r );
WRITE16_HANDLER ( mac_via_w );
READ16_HANDLER ( mac_autovector_r );
WRITE16_HANDLER ( mac_autovector_w );
READ16_HANDLER ( mac_iwm_r );
WRITE16_HANDLER ( mac_iwm_w );
READ16_HANDLER ( mac_scc_r );
WRITE16_HANDLER ( mac_scc_w );
READ16_HANDLER ( macplus_scsi_r );
WRITE16_HANDLER ( macplus_scsi_w );
NVRAM_HANDLER( mac );
void mac_scc_mouse_irq( int x, int y );


/*----------- defined in video/mac.c -----------*/

VIDEO_START( mac );
VIDEO_UPDATE( mac );
PALETTE_INIT( mac );

void mac_set_screen_buffer( int buffer );


/*----------- defined in audio/mac.c -----------*/

void *mac_sh_start(int clock, const struct CustomSound_interface *config);

void mac_enable_sound( int on );
void mac_set_sound_buffer( int buffer );
void mac_set_volume( int volume );

void mac_sh_updatebuffer(void);


#endif /* MAC_H_ */