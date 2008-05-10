/******************************************************************************
	KIM-1

	video driver

	Juergen Buchmueller, Jan 2000

******************************************************************************/

#include "driver.h"
#include "includes/kim1.h"


PALETTE_INIT( kim1 )
{
	int i;

	machine->colortable = colortable_alloc(machine, 21);

	for (i = 0; i < 16; i++)
		colortable_palette_set_color(machine->colortable, i, MAKE_RGB(24 + (i + 1) * (i + 1) - 1, (i + 1) * (i + 1) / 4, 0));
	
	colortable_palette_set_color(machine->colortable, 16, MAKE_RGB(0, 0, 0));
	colortable_palette_set_color(machine->colortable, 17, MAKE_RGB(30, 30, 30));
	colortable_palette_set_color(machine->colortable, 18, MAKE_RGB(90, 90, 90));
	colortable_palette_set_color(machine->colortable, 19, MAKE_RGB(50, 50, 50));
	colortable_palette_set_color(machine->colortable, 20, MAKE_RGB(255, 255, 255));

	for (i = 0; i < 16; i++)
	{
		colortable_entry_set_value(machine->colortable, i*2, 1);
		colortable_entry_set_value(machine->colortable, i*2+1, i);
	}

	colortable_entry_set_value(machine->colortable, 32, 17);
	colortable_entry_set_value(machine->colortable, 33, 18);
	colortable_entry_set_value(machine->colortable, 34, 19);
	colortable_entry_set_value(machine->colortable, 35, 20);
	colortable_entry_set_value(machine->colortable, 36, 17);
	colortable_entry_set_value(machine->colortable, 37, 17);
	colortable_entry_set_value(machine->colortable, 38, 19);
	colortable_entry_set_value(machine->colortable, 39, 15);
}


VIDEO_START( kim1 )
{
    videoram_size = 6 * 2 + 24;
    videoram = auto_malloc (videoram_size);
}


VIDEO_UPDATE( kim1 )
{
	int width = video_screen_get_width(screen);
	int x, y;
	static const int layout[6][4] = { {19, 21, 22, 23}, {16, 17, 20, 18}, {12, 13, 14, 15},
					{ 8,  9, 10, 11}, { 4,  5,  6,  7}, { 0,  1,  2,  3} };

	fillbitmap(bitmap, get_black_pen(screen->machine), NULL);

	/* show leds */
	for (x = 0; x < 6; x++)
	{
		int sy = 408;
		int sx = width - 212 + x * 30 + ((x >= 4) ? 6 : 0);

		drawgfx (bitmap, screen->machine->gfx[0], videoram[x<<1], videoram[(x<<1) + 1],	0, 0, sx, sy, NULL, TRANSPARENCY_PEN, 0);
	}

	/* show buttons */
	for (y = 0; y < 6; y++)
	{
		int sy = 516 + y * 36;

		for (x = 0; x < 4; x++)
		{
			int sx = width - 182 + x * 37;
			int code = layout[y][x];
			int color = 0;
			char port[6];

			sprintf(port, "LINE%d", code / 7);
			color = (input_port_read(screen->machine, port) & (0x40 >> (code % 7))) ? 0 : 1;
			videoram[6 * 2 + code] = color;
			drawgfx (bitmap, screen->machine->gfx[1], layout[y][x], color, 0, 0, sx, sy, NULL, TRANSPARENCY_NONE, 0);
		}
	}
	return 0;
}


