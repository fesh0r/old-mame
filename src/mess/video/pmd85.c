/***************************************************************************

  pmd85.c

  Functions to emulate the video hardware of PMD-85.

  Krzysztof Strzecha

***************************************************************************/

#include "driver.h"
#include "includes/pmd85.h"

const unsigned char pmd85_palette[3*3] =
{
	0x00, 0x00, 0x00,
	0x7f, 0x7f, 0x7f,
	0xff, 0xff, 0xff
};

PALETTE_INIT( pmd85 )
{
	int i;

	for ( i = 0; i < sizeof(pmd85_palette) / 3; i++ ) {
		palette_set_color_rgb(machine, i, pmd85_palette[i*3], pmd85_palette[i*3+1], pmd85_palette[i*3+2]);
	}
}

VIDEO_START( pmd85 )
{
}

static void pmd85_draw_scanline(running_machine *machine, bitmap_t *bitmap, int pmd85_scanline)
{
	int x, i;
	int pen0, pen1;
	UINT8 data;

	/* set up scanline */
	UINT16 *scanline = BITMAP_ADDR16(bitmap, pmd85_scanline, 0);

	/* address of current line in PMD-85 video memory */
	UINT8* pmd85_video_ram_line = mess_ram + 0xc000 + 0x40*pmd85_scanline;

	for (x=0; x<288; x+=6)
	{
		data = pmd85_video_ram_line[x/6];
		pen0 = 0;
		pen1 = data & 0x80 ? 1 : 2;

		for (i=0; i<6; i++)
			scanline[x+i] = machine->pens[(data & (0x01<<i)) ? pen1 : pen0];

	}
}

VIDEO_UPDATE( pmd85 )
{
	int pmd85_scanline;

	for (pmd85_scanline=0; pmd85_scanline<256; pmd85_scanline++)
		pmd85_draw_scanline (screen->machine, bitmap, pmd85_scanline);
	return 0;
}
