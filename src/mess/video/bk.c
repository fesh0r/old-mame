/***************************************************************************

        BK video driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/bk.h"


VIDEO_START( bk0010 )
{
}

VIDEO_UPDATE( bk0010 )
{
	bk_state *state = screen->machine->driver_data<bk_state>();
	UINT16 code;
	int y, x, b;
	int nOfs;

	nOfs = (state->scrool - 728) % 256;

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 32; x++)
		{
			code = state->bk0010_video_ram[((y+nOfs) %256)*32 + x];
			for (b = 0; b < 16; b++)
			{
				*BITMAP_ADDR16(bitmap, y, x*16 + b) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}
