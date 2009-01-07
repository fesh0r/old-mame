/***************************************************************************

        PK-8020 driver by Miodrag Milanovic

        18/07/2008 Preliminary driver.

****************************************************************************/

#include "driver.h"
#include "includes/pk8020.h"

VIDEO_START( pk8020 )
{
}

VIDEO_UPDATE( pk8020 )
{
 	UINT8 code1,code2,code3,code4;
 	UINT8 col;
	int y, x, b;

	// draw image
	for (x = 0; x < 32; x++)
	{
		for (y = 0; y < 256; y++)
		{
			code1 = mess_ram[0x10000 + x*256 + y];
			code2 = mess_ram[0x14000 + x*256 + y];
			code3 = mess_ram[0x18000 + x*256 + y];
			code4 = mess_ram[0x1C000 + x*256 + y];
			for (b = 0; b < 8; b++)
			{
				col = ((code1 >> b) & 0x01) * 8 + ((code2 >> b) & 0x01) * 4 + ((code3 >> b) & 0x01)* 2+ ((code4 >> b) & 0x01);
				*BITMAP_ADDR16(bitmap, 255-y, x*8+(7-b)) =  col;
			}
		}
	}


	return 0;
}

PALETTE_INIT( pk8020 )
{
	int i;
	for(i=0;i<16;i++) {
		palette_set_color( machine, i, MAKE_RGB(0,0,0) );
	}
}
