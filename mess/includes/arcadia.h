#include "driver.h"

// use this for debugging
#if 0
#define arcadia_DEBUG 1
#else
#define arcadia_DEBUG 0
#endif

extern INTERRUPT_GEN( arcadia_video_line );
 READ8_HANDLER(arcadia_vsync_r);

 READ8_HANDLER(arcadia_video_r);
WRITE8_HANDLER(arcadia_video_w);


// space vultures sprites above
// combat below and invisible
#define YPOS 0
//#define YBOTTOM_SIZE 24
// grand slam sprites left and right
// space vultures left
// space attack left
#if arcadia_DEBUG
#define XPOS 48
#else
#define XPOS 32
#endif

extern VIDEO_START( arcadia );
extern VIDEO_UPDATE( arcadia );

extern struct CustomSound_interface arcadia_sound_interface;
extern int arcadia_custom_start (const struct MachineSound *driver);
extern void arcadia_custom_stop (void);
extern void arcadia_custom_update (void);
extern void arcadia_soundport_w (int mode, int data);
