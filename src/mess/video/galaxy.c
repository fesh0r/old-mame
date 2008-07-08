/***************************************************************************

  galaxy.c

  Functions to emulate the video hardware of the Galaksija.

  20/05/2008 - Real video implementation by Miodrag Milanovic
  01/03/2008 - Update by Miodrag Milanovic to make Galaksija video work with new SVN code
***************************************************************************/

#include "driver.h"
#include "includes/galaxy.h"
#include "cpu/z80/z80.h"

UINT32 gal_cnt = 0;
static UINT8 code = 0;
static UINT8 first = 0;
static UINT32 start_addr = 0;
emu_timer *gal_video_timer = NULL;

TIMER_CALLBACK( gal_video )
{	
	int y,x;
	if (galaxy_interrupts_enabled==TRUE) {
		UINT8 *gfx = memory_region(machine, REGION_GFX1);
		UINT8 dat = (gal_latch_value & 0x3c) >> 2;
		if ((gal_cnt >= 48 * 2) && (gal_cnt < 48 * 210)) { // display on screen just first 208 lines
			UINT8 mode = (gal_latch_value >> 1) & 1; // bit 2 latch represents mode
			UINT16 addr = (cpunum_get_reg(0, Z80_I) << 8) | cpunum_get_reg(0, Z80_R) | ((gal_latch_value & 0x80) ^ 0x80);
  			if (mode == 0){
  				// Text mode
	  			if (first==0 && (cpunum_get_reg(0, Z80_R) & 0x1f) ==0) {
		  			// Due to a fact that on real processor latch value is set at
		  			// the end of last cycle we need to skip dusplay of double
		  			// first char in each row
		  			code = 0x00;
		  			first = 1;
				} else {
					code = program_read_byte(addr) & 0xbf;
					code += (code & 0x80) >> 1;
					code = gfx[(code & 0x7f) +(dat << 7 )] ^ 0xff;
					first = 0;
				}
				y = gal_cnt / 48 - 2;
				x = (gal_cnt % 48) * 8;
				
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 0) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 1) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 2) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 3) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 4) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 5) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 6) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 7) & 1;
			}
			else 
			{ // Graphics mode
	  			if (first<4 && (cpunum_get_reg(0, Z80_R) & 0x1f) ==0) {
		  			// Due to a fact that on real processor latch value is set at
		  			// the end of last cycle we need to skip dusplay of 4 times
		  			// first char in each row
		  			code = 0x00;
		  			first++;
				} else {
					code = program_read_byte(addr) ^ 0xff;
					first = 0;
				}
				y = gal_cnt / 48 - 2;
				x = (gal_cnt % 48) * 8;
				
				/* hack - until calc of R is fixed in Z80 */
				if (x==11*8 && y==0) {
					start_addr = addr;
				}
				if ((x/8 >=11) && (x/8<44)) {
					code = program_read_byte(start_addr + y * 32 + (gal_cnt % 48)-11) ^ 0xff;
				} else {
					code = 0x00;
				}
				/* end of hack */
				
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 0) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 1) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 2) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 3) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 4) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 5) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 6) & 1; x++;
				*BITMAP_ADDR16(tmpbitmap, y, x ) = (code >> 7) & 1;
			}					
		}
		gal_cnt++;
	}	
}

VIDEO_UPDATE( galaxy )
{
	timer_adjust_periodic(gal_video_timer, attotime_zero, 0, attotime_never);
	if (galaxy_interrupts_enabled == FALSE) {
		rectangle black_area = {0,384-1,0,208-1};
		fillbitmap(tmpbitmap, 0, &black_area);
	}
	galaxy_interrupts_enabled = FALSE;
	return VIDEO_UPDATE_CALL ( generic_bitmapped );
}

