/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate the video hardware of the coupe.

 At present these are not done using the mame driver standard, they are
 simply plot pixelled.. I will fix this in a future version.

***************************************************************************/

#include "driver.h"
#include "includes/coupe.h"


static void drawMode4_line(bitmap_t *bitmap,int y)
{
	int x;
	unsigned char tmp=0;

	for (x=0;x<256;)
	{
		tmp=*(videoram + (x/2) + (y*128));
#ifdef MONO
		if (tmp>>4)
		{
			plot_pixel(bitmap, x*2, y, 127);
			plot_pixel(bitmap, x*2+1, y, 127);
		}
		else
		{
			plot_pixel(bitmap, x*2, y, 0);
			plot_pixel(bitmap, x*2+1, y, 0);
		}
		x++;
		if (tmp&0x0F)
		{
			plot_pixel(bitmap, x*2, y, 127);
			plot_pixel(bitmap, x*2+1, y, 127);
		}
		else
		{
			plot_pixel(bitmap, x*2, y, 0);
			plot_pixel(bitmap, x*2+1, y, 0);
		}
		x++;
#else
		*BITMAP_ADDR16(bitmap, y, x*2+0) = coupe_regs.clut[tmp>>4];
		*BITMAP_ADDR16(bitmap, y, x*2+1) = coupe_regs.clut[tmp>>4];
		x++;
		*BITMAP_ADDR16(bitmap, y, x*2+0) = coupe_regs.clut[tmp&0x0F];
		*BITMAP_ADDR16(bitmap, y, x*2+1) = coupe_regs.clut[tmp&0x0F];
		x++;
#endif
	}
}

static void drawMode3_line(bitmap_t *bitmap,int y)
{
	int x;
	unsigned char tmp=0;

	for (x=0;x<512;)
	{
		tmp=*(videoram + (x/4) + (y*128));
#ifdef MONO
		if (tmp>>6)
			plot_pixel(bitmap,x,y,127);
		else
			plot_pixel(bitmap,x,y,0);
		x++;
		if ((tmp>>4)&0x03)
			plot_pixel(bitmap,x,y,127);
		else
			plot_pixel(bitmap,x,y,0);
		x++;
		if ((tmp>>2)&0x03)
			plot_pixel(bitmap,x,y,127);
		else
			plot_pixel(bitmap,x,y,0);
		x++;
		if (tmp&0x03)
			plot_pixel(bitmap,x,y,127);
		else
			plot_pixel(bitmap,x,y,0);
		x++;
#else
		*BITMAP_ADDR16(bitmap, y, x) = coupe_regs.clut[tmp>>6];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = coupe_regs.clut[(tmp>>4)&0x03];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = coupe_regs.clut[(tmp>>2)&0x03];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = coupe_regs.clut[tmp&0x03];
		x++;
#endif
	}
}

static void drawMode2_line(bitmap_t *bitmap,int y)
{
	int x,b,scrx;
	unsigned char tmp=0;
	unsigned short ink,pap;
	unsigned char *attr;

	attr=videoram + 32*192 + y*32;

	scrx=0;
	for (x=0;x<256/8;x++)
	{
		tmp=*(videoram + x + (y*32));
#ifdef MONO
		ink=127;
		pap=0;
#else
		ink=coupe_regs.clut[(*attr) & 0x07];
		pap=coupe_regs.clut[((*attr)>>3) & 0x07];
#endif
		attr++;

		for (b=0x80;b!=0;b>>=1)
		{
			*BITMAP_ADDR16(bitmap, y, scrx++) = (tmp&b) ? ink : pap;
			*BITMAP_ADDR16(bitmap, y, scrx++) = (tmp&b) ? ink : pap;
		}
	}
}

static void drawMode1_line(bitmap_t *bitmap,int y)
{
	int x,b,scrx,scry;
	unsigned char tmp=0;
	unsigned short ink,pap;
	unsigned char *attr;

	attr=videoram + 32*192 + (y/8)*32;

	scrx=0;
	scry=((y&7) * 8) + ((y&0x38)>>3) + (y&0xC0);
	for (x=0;x<256/8;x++)
	{
		tmp=*(videoram + x + (y*32));
#ifdef MONO
		ink=127;
		pap=0;
#else
		ink=coupe_regs.clut[(*attr) & 0x07];
		pap=coupe_regs.clut[((*attr)>>3) & 0x07];
#endif
		attr++;
		for (b=0x80;b!=0;b>>=1)
		{
			*BITMAP_ADDR16(bitmap, scry, scrx++) = (tmp&b) ? ink : pap;
			*BITMAP_ADDR16(bitmap, scry, scrx++) = (tmp&b) ? ink : pap;
		}
	}
}

VIDEO_UPDATE( coupe )
{
	int scanline = video_screen_get_vpos(screen);

	/* line interrupt? */
	if (coupe_regs.line_int == scanline) coupe_irq(screen->machine, 0x01);

	/* display disabled? (only in mode 3 or 4) */
	if ((coupe_regs.vmpr & 0x40) && (coupe_regs.border & 0x80))
	{
		fillbitmap(bitmap, 0, cliprect);
	}
	else
	{
		switch ((coupe_regs.vmpr & 0x60) >> 5)
		{
		case 0: /* mode 1 */
			drawMode1_line(bitmap, scanline);
			break;
		case 1: /* mode 2 */
			drawMode2_line(bitmap, scanline);
			break;
		case 2: /* mode 3 */
			drawMode3_line(bitmap, scanline);
			break;
		case 3: /* mode 4 */
			drawMode4_line(bitmap, scanline);
			break;
		}
	}

	return 0;
}
