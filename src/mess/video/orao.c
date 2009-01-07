/***************************************************************************

		Orao video driver by Miodrag Milanovic

		01/03/2008 Updated to work with latest SVN code
		22/02/2008 Preliminary driver.
		     
****************************************************************************/

#include "driver.h"
#include "includes/orao.h"

UINT8 *orao_video_ram;
  
VIDEO_START( orao )
{
}
 
VIDEO_UPDATE( orao )
{
	UINT8 code;
	int y, x, b;
	
	int addr = 0;	
	for (y = 0; y < 256; y++)
	{
		int horpos = 0;
		for (x = 0; x < 32; x++)
		{			
			code = orao_video_ram[addr++];
			for (b = 0; b < 8; b++)
			{				
				*BITMAP_ADDR16(bitmap, y, horpos++) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}


