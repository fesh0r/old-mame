/***************************************************************************

  galaxy.c

  Functions to emulate the video hardware of the Galaksija.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/galaxy.h"
#include "cpu/z80/z80.h"

unsigned char *galaxy_charram;
size_t galaxy_charram_size;


static int horizontal_pos = 0x0b;

int galaxy_vh_start (void)
{
	if( generic_vh_start() )
		return 1;
    return 0;
}

void galaxy_vh_stop (void)
{
	generic_vh_stop();
}

WRITE_HANDLER( galaxy_vh_charram_w )
{
    galaxy_charram[offset] = data;
    dirtybuffer[offset]	= 1;
}

void galaxy_vh_screenrefresh (struct mame_bitmap *bitmap, int full_refresh)
{

	int offs;
	struct rectangle black_area = {0,0,0,16*13};
	static int fast_mode = FALSE;
	

	if (!galaxy_interrupts_enabled)
	{
		black_area.min_x = 0;
		black_area.max_x = 32*8-1;
		black_area.min_y = 0;
		black_area.max_y = 16*13-1;
		fillbitmap(bitmap, Machine->pens[0], &black_area);
		fast_mode = TRUE;
		return;
	}


	if (horizontal_pos!=cpu_readmem16(0x2ba8))
	{
		full_refresh=1;
		horizontal_pos = cpu_readmem16(0x2ba8);
		if (horizontal_pos > 0x0b)
		{
			black_area.min_x =  0; 
			black_area.max_x =  8*(horizontal_pos-0x0b)-1;
		}
		if (horizontal_pos < 0x0b)
		{
			black_area.min_x = 8*(21+horizontal_pos); 
			black_area.max_x = 32*8-1;
		}
		if (horizontal_pos == 0x0b)
			black_area.min_x =  black_area.max_x = 0; 
		
		fillbitmap(bitmap, Machine->pens[0], &black_area);
	}	


	if( full_refresh )
		memset(dirtybuffer, 1, videoram_size);

	if( fast_mode )
	{
		memset(dirtybuffer, 1, videoram_size);
		fast_mode = FALSE;
	}

	for( offs = 0; offs < videoram_size; offs++ )
    	{
        	if( dirtybuffer[offs]  )
		{
			int sx, sy;
		        int code = videoram[offs];

			sx = (offs % 32) * 8 + horizontal_pos*8-88;
			

			if (sx>=0 && sx<32*8)
			{
           		        if ((code>63 && code<96) || code>127) code-=64;
				sy = (offs / 32) * 13;
				drawgfx(bitmap, Machine->gfx[0], code & 0x7f, 1, 0,0, sx,sy,
					&Machine->visible_area, TRANSPARENCY_NONE, 0);
			}
		        dirtybuffer[offs] = 0;
		}
    	}

	galaxy_interrupts_enabled = FALSE;
}
