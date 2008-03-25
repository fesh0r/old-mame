/***************************************************************************

  pcw.c

  Functions to emulate the video hardware of the Amstrad PCW.

***************************************************************************/

#include "driver.h"
#include "includes/pcw.h"

INLINE void pcw_plot_pixel(bitmap_t *bitmap, int x, int y, UINT32 color)
{
	*BITMAP_ADDR16(bitmap, y, x) = (UINT16)color;
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/

VIDEO_START( pcw )
{
}

extern unsigned int roller_ram_addr;
extern unsigned short roller_ram_offset;
extern unsigned char pcw_vdu_video_control_register;

/* two colours */
static const unsigned short pcw_colour_table[PCW_NUM_COLOURS] =
{
	0, 1
};

/* black/white */
static const rgb_t pcw_palette[PCW_NUM_COLOURS] =
{
	MAKE_RGB(0x000, 0x000, 0x000),
	MAKE_RGB(0x0ff, 0x0ff, 0x0ff)
};


/* Initialise the palette */
PALETTE_INIT( pcw )
{
	palette_set_colors(machine, 0, pcw_palette, ARRAY_LENGTH(pcw_palette));
}

/***************************************************************************
  Draw the game screen in the given bitmap_t.
  Do NOT call osd_update_display() from this function,
  it will be called by the main emulation engine.
***************************************************************************/
VIDEO_UPDATE( pcw )
{
	int x,y,b;
	unsigned short roller_ram_offs;
	unsigned char *roller_ram_ptr;
	int pen0,pen1;

	pen0 = 0;
	pen1 = 1;

	/* invert? */
	if (pcw_vdu_video_control_register & (1<<7))
	{
		/* yes */
		pen1^=1;
		pen0^=1;
	}

	pen0 = screen->machine->pens[pen0];
	pen1 = screen->machine->pens[pen1];

	/* video enable? */
	if ((pcw_vdu_video_control_register & (1<<6))!=0)
	{
		rectangle rect;

		/* render top border */
		rect.min_x = 0;
		rect.min_y = 0;
		rect.max_x = PCW_SCREEN_WIDTH;
		rect.max_y = PCW_BORDER_HEIGHT;
		fillbitmap(bitmap, pen0, &rect);

		/* render bottom border */
		rect.min_x = 0;
		rect.min_y = PCW_BORDER_HEIGHT + PCW_DISPLAY_HEIGHT;
		rect.max_x = PCW_SCREEN_WIDTH;
		rect.max_y = rect.min_y + PCW_BORDER_HEIGHT;
		fillbitmap(bitmap, pen0, &rect);

		/* yes */

		/* offset to start in table */
		roller_ram_offs = (roller_ram_offset<<1);

		for (y=0; y<256; y++)
		{
			int by;
			unsigned short line_data;
			unsigned char *line_ptr;

			x = PCW_BORDER_WIDTH;

			roller_ram_ptr = mess_ram + roller_ram_addr + roller_ram_offs;

			/* get line address */
			/* b16-14 control which bank the line is to be found in, b13-3 the address in the bank (in 16-byte units), and b2-0 the offset. Thus a roller RAM address bbbxxxxxxxxxxxyyy indicates bank bbb, address 00xxxxxxxxxxx0yyy. */
			line_data = ((unsigned char *)roller_ram_ptr)[0] | (((unsigned char *)roller_ram_ptr)[1]<<8);

			/* calculate address of pixel data */
			line_ptr = mess_ram + ((line_data & 0x0e000)<<1) + ((line_data & 0x01ff8)<<1) + (line_data & 0x07);

			for (by=0; by<90; by++)
			{
				unsigned char byte;

				byte = line_ptr[0];

				for (b=0; b<8; b++)
				{
					if (byte & 0x080)
					{
						pcw_plot_pixel(bitmap,x+b, y+PCW_BORDER_HEIGHT, pen1);
					}
					else
					{
						pcw_plot_pixel(bitmap,x+b, y+PCW_BORDER_HEIGHT, pen0);

					}
					byte = byte<<1;
				}

				x = x + 8;


				line_ptr = line_ptr+8;
			}

			/* update offset, wrap within 512 byte range */
			roller_ram_offs+=2;
			roller_ram_offs&=511;

		}

		/* render border */
		/* 8 pixels either side of display */
		for (y=0; y<256; y++)
		{
			pcw_plot_pixel(bitmap, 0, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 1, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 2, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 3, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 4, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 5, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 6, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, 7, y+PCW_BORDER_HEIGHT, pen0);

			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+0, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+1, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+2, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+3, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+4, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+5, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+6, y+PCW_BORDER_HEIGHT, pen0);
			pcw_plot_pixel(bitmap, PCW_BORDER_WIDTH+PCW_DISPLAY_WIDTH+7, y+PCW_BORDER_HEIGHT, pen0);
		}
	}
	else
	{
		/* not video - render whole lot in pen 0 */
		rectangle rect;

		/* render top border */
		rect.min_x = 0;
		rect.min_y = 0;
		rect.max_x = PCW_SCREEN_WIDTH;
		rect.max_y = PCW_SCREEN_HEIGHT;

		fillbitmap(bitmap, pen0, &rect);
	}
	return 0;
}
