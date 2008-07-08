#include "driver.h"
#include "includes/crtc6845.h"
#include "video/pc_video.h"

#define CGA_PALETTE_SETS 83	/* one for colour, one for mono,
				 * 81 for colour composite */

#define CGA_SCREEN_NAME	"cga_screen"
#define CGA_MC6845_NAME	"mc6845_cga"

MACHINE_DRIVER_EXTERN( pcvideo_cga );
INPUT_PORTS_EXTERN( pcvideo_cga );
INPUT_PORTS_EXTERN( pcvideo_cga_at );

/* has a special 640x200 in 16 color mode, 4 banks at 0xb8000 */
MACHINE_DRIVER_EXTERN( pcvideo_pc1512 );
INPUT_PORTS_EXTERN( pcvideo_pc1512 );


/* Used in machine/pc.c */
READ16_HANDLER( pc1512_16le_r );
WRITE16_HANDLER( pc1512_16le_w );
WRITE16_HANDLER( pc1512_videoram16le_w );


