/***************************************************************************

		UT88 video driver by Miodrag Milanovic

		06/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "includes/ut88.h"
  
UINT8 *ut88_video_ram;
    
const gfx_layout ut88_charlayout =
{
	8, 8,				/* 8x8 characters */
	256,				/* 256 characters */
	1,				  /* 1 bits per pixel */
	{0},				/* no bitplanes; 1 bit per pixel */	
	{0, 1, 2, 3, 4, 5, 6, 7},
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8*8					/* size of one char */
};

VIDEO_START( ut88 )
{
}
 
VIDEO_UPDATE( ut88 )
{
	int x,y;

	for(y = 0; y < 28; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int code = ut88_video_ram[ x + y*64 ] & 0x7f;		
			int attr = ut88_video_ram[ x+1 + y*64 ] & 0x80;
			drawgfx(bitmap, screen->machine->gfx[0],  code | attr, 0, 0,0, x*8,y*8,
				NULL, TRANSPARENCY_NONE, 0);
		}
	}	
	return 0;
}

