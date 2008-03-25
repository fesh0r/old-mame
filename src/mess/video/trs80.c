/***************************************************************************

  trs80.c

  Functions to emulate the video hardware of the TRS80.

***************************************************************************/
#include "driver.h"
#include "includes/trs80.h"

#define FW  TRS80_FONT_W
#define FH  TRS80_FONT_H

/***************************************************************************
  Statics for this module
***************************************************************************/
static int width_store = 10;  // bodgy value to force an initial resize

VIDEO_UPDATE( trs80 )
{
	int width = 64 - ((trs80_port_ff & 8) << 2);
	int skip = 3 - (width >> 5);
	int i=0,x,y,chr;
	int adj=readinputport(0)&0x40;

	if (width != width_store)
	{
		width_store = width;
		video_screen_set_visarea(screen, 0, width*FW-1, 0, 16*FH-1);
	}

	for (y = 0; y < 16; y++)
	{
		for (x = 0; x < 64; x+=skip)
		{
			i = (y << 6) + x;
			chr=videoram[i];
			if ((adj) && (chr < 32)) chr+=64;			// 7-bit video handler
			drawgfx( bitmap,screen->machine->gfx[0],chr,0,0,0,x/skip*FW,y*FH,
				NULL,TRANSPARENCY_NONE,0);
		}
	}
	return 0;
}

/***************************************************************************
  Write to video ram
***************************************************************************/

WRITE8_HANDLER( trs80_videoram_w )
{
	videoram[offset] = data;
}

