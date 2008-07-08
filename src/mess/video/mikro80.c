/***************************************************************************

		Mikro-80 video driver by Miodrag Milanovic

		10/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "includes/mikro80.h"
  
UINT8 *mikro80_video_ram;
UINT8 *mikro80_cursor_ram;
    
VIDEO_START( mikro80 )
{
}
 
VIDEO_UPDATE( mikro80 )
{
	UINT8 *gfx = memory_region(screen->machine, REGION_GFX1);
	int x,y,b;

	for(y = 0; y < 32*8; y++ )
	{
		for(x = 0; x < 64; x++ )
		{
			int addr = x + (y / 8)*64;
			UINT8 code = gfx[mikro80_video_ram [addr]*8+ (y % 8)];
			UINT8 attr = mikro80_cursor_ram[addr+1] & 0x80 ? 1 : 0;
			for (b = 7; b >= 0; b--)
			{								
				UINT8 col = (code >> b) & 0x01;
				*BITMAP_ADDR16(bitmap, y, x*8+(7-b)) =  attr ? col ^ 1 : col;
			}
		}
	}
	return 0;
}
