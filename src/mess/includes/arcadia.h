/*****************************************************************************
 *
 * includes/arcadia.h
 *
 ****************************************************************************/

#ifndef ARCADIA_H_
#define ARCADIA_H_


// space vultures sprites above
// combat below and invisible
#define YPOS 0
//#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#define XPOS 32


/*----------- defined in video/arcadia.c -----------*/

extern INTERRUPT_GEN( arcadia_video_line );
READ8_HANDLER(arcadia_vsync_r);

READ8_HANDLER(arcadia_video_r);
WRITE8_HANDLER(arcadia_video_w);

extern VIDEO_START( arcadia );
extern VIDEO_UPDATE( arcadia );


/*----------- defined in audio/arcadia.c -----------*/

#define SOUND_ARCADIA	DEVICE_GET_INFO_NAME(arcadia_sound)

DEVICE_GET_INFO(arcadia_sound);
void arcadia_soundport_w (const device_config *device, int mode, int data);


#endif /* ARCADIA_H_ */
