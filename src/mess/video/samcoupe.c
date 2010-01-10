/***************************************************************************

 SAM Coupe Driver - Written By Lee Hammerton

  Functions to emulate the video hardware of the coupe.

 At present these are not done using the mame driver standard, they are
 simply plot pixelled.. I will fix this in a future version.

***************************************************************************/

#include "driver.h"
#include "includes/samcoupe.h"


static void draw_mode4_line(const device_config *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = screen->machine->driver_data;
	int x;

	for (x = 0; x < 256;)
	{
		UINT8 tmp = screen->machine->generic.videoram.u8[(x/2) + (y*128)];

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
		*BITMAP_ADDR16(bitmap, y, x*2+0) = asic->clut[tmp >> 4];
		*BITMAP_ADDR16(bitmap, y, x*2+1) = asic->clut[tmp >> 4];
		x++;
		*BITMAP_ADDR16(bitmap, y, x*2+0) = asic->clut[tmp & 0x0f];
		*BITMAP_ADDR16(bitmap, y, x*2+1) = asic->clut[tmp & 0x0f];
		x++;
#endif
	}
}

static void draw_mode3_line(const device_config *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = screen->machine->driver_data;
	int x;

	for (x = 0; x < 512;)
	{
		UINT8 tmp = screen->machine->generic.videoram.u8[(x/4) + (y*128)];

#ifdef MONO
		if (tmp >> 6)
			plot_pixel(bitmap, x, y, 127);
		else
			plot_pixel(bitmap, x, y, 0);
		x++;
		if ((tmp >> 4) & 0x03)
			plot_pixel(bitmap, x, y, 127);
		else
			plot_pixel(bitmap, x, y, 0);
		x++;
		if ((tmp >> 2) & 0x03)
			plot_pixel(bitmap, x, y, 127);
		else
			plot_pixel(bitmap, x, y, 0);
		x++;
		if (tmp & 0x03)
			plot_pixel(bitmap, x, y, 127);
		else
			plot_pixel(bitmap, x, y, 0);
		x++;
#else
		*BITMAP_ADDR16(bitmap, y, x) = asic->clut[tmp >> 6];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = asic->clut[(tmp >> 4) & 0x03];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = asic->clut[(tmp >> 2) & 0x03];
		x++;
		*BITMAP_ADDR16(bitmap, y, x) = asic->clut[tmp & 0x03];
		x++;
#endif
	}
}

static void draw_mode2_line(const device_config *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = screen->machine->driver_data;
	int b, scrx = 0;
	UINT16 ink = 127, pap = 0;
	UINT8 *attr = screen->machine->generic.videoram.u8 + 32*192 + y*32;
	int x;

	for (x = 0; x < 256/8; x++)
	{
		UINT8 tmp = screen->machine->generic.videoram.u8[x + (y*32)];

#ifndef MONO
		ink = asic->clut[(*attr) & 0x07];
		pap = asic->clut[((*attr) >> 3) & 0x07];
#endif

		attr++;

		for (b = 0x80; b!= 0; b >>= 1)
		{
			*BITMAP_ADDR16(bitmap, y, scrx++) = (tmp & b) ? ink : pap;
			*BITMAP_ADDR16(bitmap, y, scrx++) = (tmp & b) ? ink : pap;
		}
	}
}

static void draw_mode1_line(const device_config *screen, bitmap_t *bitmap, int y)
{
	coupe_asic *asic = screen->machine->driver_data;
	int block, x = 0;

	/* get base address of attribute and data values */
	UINT8 *attr = screen->machine->generic.videoram.u8 + 32*192 + ((y & 0xf8) << 2);
	UINT8 *data = screen->machine->generic.videoram.u8 + (((y & 0xc0) << 5) | ((y & 0x07) << 8) | ((y & 0x38) << 2));

	/* loop over all 32 blocks */
	for (block = 0; block < 32; block++)
	{
        int bit;

        /* get current colors from attribute byte */
        UINT8 ink = asic->clut[(*attr & 0x07) + ((*attr >> 3) & 0x08)];
        UINT8 paper = asic->clut[(*attr >> 3) & 0x0f];

        /* draw a block (8 pixels, doubled) */
		for (bit = 0x80; bit != 0; bit >>= 1)
		{
			*BITMAP_ADDR16(bitmap, y, x++) = (*data & bit) ? ink : paper;
			*BITMAP_ADDR16(bitmap, y, x++) = (*data & bit) ? ink : paper;
		}

		/* save current attribute */
		asic->attribute = *attr;

		/* move to the next block of values */
		attr++;
		data++;
	}
}

VIDEO_UPDATE( samcoupe )
{
	int scanline = video_screen_get_vpos(screen);
	coupe_asic *asic = screen->machine->driver_data;

	/* line interrupt? */
	if (asic->line_int == scanline)
		samcoupe_irq(screen->machine->firstcpu, SAM_LINE_INT);

	/* display disabled? (only in mode 3 or 4) */
	if ((asic->vmpr & 0x40) && (asic->border & 0x80))
	{
		/* display is disabled, draw a black screen */
		bitmap_fill(bitmap, cliprect, 0);
	}
	else
	{
		/* display enabled, draw line in the right screen mode */
		switch ((asic->vmpr & 0x60) >> 5)
		{
		case 0:	draw_mode1_line(screen, bitmap, scanline); break;
		case 1:	draw_mode2_line(screen, bitmap, scanline); break;
		case 2:	draw_mode3_line(screen, bitmap, scanline); break;
		case 3:	draw_mode4_line(screen, bitmap, scanline); break;
		}
	}
//printf("videomode: %u\n", (asic->vmpr & 0x60) >> 5);
	return 0;
}
