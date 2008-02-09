#include "video/pc_video.h"

#if 0
	// cutted from some aga char rom
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("mda.chr",     0x00000, 0x01000, CRC(ac1686f3))
#endif

void pc_mda_init_video(void);
void pc_mda_europc_init(void);

void pc_mda_timer(void);

VIDEO_START ( pc_mda );
pc_video_update_proc pc_mda_choosevideomode(int *width, int *height, struct mscrtc6845 *crtc);

WRITE8_HANDLER ( pc_MDA_w );
READ8_HANDLER ( pc_MDA_r );

extern const unsigned char mda_palette[4][3];
extern const gfx_layout pc_mda_charlayout;
GFXDECODE_EXTERN( pc_mda );
extern const unsigned short mda_colortable[256*2+1*2];

PALETTE_INIT( pc_mda );

//internal use
void pc_mda_cursor(struct mscrtc6845_cursor *cursor);