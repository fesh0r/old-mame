/*****************************************************************************
 *
 * includes/vc4000.h
 *
 ****************************************************************************/

#ifndef VC4000_H_
#define VC4000_H_



// define this to use digital inputs instead of the slow
// autocentering analog mame joys
#define ANALOG_HACK


/*----------- defined in video/vc4000.c -----------*/

extern INTERRUPT_GEN( vc4000_video_line );
extern  READ8_HANDLER(vc4000_vsync_r);

extern  READ8_HANDLER(vc4000_video_r);
extern WRITE8_HANDLER(vc4000_video_w);

extern VIDEO_START( vc4000 );
extern VIDEO_UPDATE( vc4000 );


/*----------- defined in audio/vc4000.c -----------*/

#define SOUND_VC4000	DEVICE_GET_INFO_NAME( vc4000_sound )

DEVICE_GET_INFO( vc4000_sound );

void vc4000_soundport_w (const device_config *device, int mode, int data);


#endif /* VC4000_H_ */
