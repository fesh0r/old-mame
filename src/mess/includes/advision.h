/*****************************************************************************
 *
 * includes/advision.h
 *
 ****************************************************************************/

#ifndef ADVISION_H_
#define ADVISION_H_


/*----------- defined in machine/advision.c -----------*/

extern int advision_framestart;
/*extern int advision_videoenable;*/
extern int advision_videobank;

DRIVER_INIT( advision );
MACHINE_RESET( advision );
READ8_HANDLER ( advision_MAINRAM_r);
WRITE8_HANDLER( advision_MAINRAM_w );

/* Port P1 */
READ8_HANDLER( advision_controller_r );
WRITE8_HANDLER( advision_bankswitch_w );

/* Port P2 */
WRITE8_HANDLER( advision_av_control_w );

READ8_HANDLER ( advision_gett1 );
READ8_HANDLER ( advision_getL );
WRITE8_HANDLER ( advision_putG );
WRITE8_HANDLER ( advision_putD );


/*----------- defined in video/advision.c -----------*/

extern int advision_vh_hpos;

VIDEO_START( advision );
VIDEO_UPDATE( advision );
PALETTE_INIT( advision );
void advision_vh_write(int data);
void advision_vh_update(int data);


#endif /* ADVISION_H_ */
