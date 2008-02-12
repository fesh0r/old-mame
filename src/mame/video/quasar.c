/***************************************************************************

  video\quasar.c

  Functions to emulate the video hardware of the machine.

  Zaccaria S2650 games share various levels of design with the Century Video
  System (CVS) games, and hence some routines are shared from there.

  Shooting seems to mix custom boards from Zaccaria and sound boards from CVS
  hinting at a strong link between the two companies.

  Zaccaria are an italian company, Century were based in Manchester UK

***************************************************************************/

#include "driver.h"
#include "cvs.h"
#include "video/s2636.h"
#include "cpu/s2650/s2650.h"


UINT8 *quasar_effectram;
UINT8 quasar_effectcontrol;

static s2636_t *s2636_0, *s2636_1, *s2636_2;

PALETTE_INIT( quasar )
{
	int i;

	/* allocate the colortable */
	machine->colortable = colortable_alloc(machine, 0x500);

	/* standard 1 bit per color palette (background and sprites) */
	for (i = 0; i < 8; i++)
	{
		rgb_t color = MAKE_RGB(pal1bit(i >> 0), pal1bit(i >> 1), pal1bit(i >> 2));
		colortable_palette_set_color(machine->colortable, i, color);
	}

	/* effects color map */
	for (i = 0; i < 0x100; i++)
	{
		rgb_t color;
		int bit0, bit1, bit2;
		int r, g, b;

		/* red component */
		bit0 = BIT(i, 0);
		bit1 = BIT(i, 1);
		bit2 = BIT(i, 2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = BIT(i, 3);
		bit1 = BIT(i, 4);
		bit2 = BIT(i, 5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = BIT(i, 6);
		bit1 = BIT(i, 7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		/* intensity 0 */
  	    colortable_palette_set_color(machine->colortable, 0x100 + i, RGB_BLACK);

		/* intensity 1 */
		color = MAKE_RGB(r >> 2, g >> 2, b >> 2);
  	    colortable_palette_set_color(machine->colortable, 0x200 + i, color);

		/* intensity 2 */
		color = MAKE_RGB((r >> 2) + (r >> 3), (g >> 2) + (g >> 3), (b >> 2) + (b >> 2));
 	    colortable_palette_set_color(machine->colortable, 0x300 + i, color);

		/* intensity 3 */
		color = MAKE_RGB(r >> 1, g >> 1, b >> 1);
  	    colortable_palette_set_color(machine->colortable, 0x400 + i, color);
	}

	// Address 0-2 from graphic rom
	//         3-5 from color ram
	//         6-8 from sprite chips (Used for priority)
	for (i = 0; i < 0x200; i++)
		colortable_entry_set_value(machine->colortable, i, color_prom[i] & 0x07);

	/* background for collision */
	for (i = 1; i < 8; i++)
		colortable_entry_set_value(machine->colortable, 0x200 + i, 7);
	colortable_entry_set_value(machine->colortable, 0x200, 0);

	/* effects */
	for (i = 0; i < 0x400; i++)
		colortable_entry_set_value(machine->colortable, 0x208 + i, 0x100 + i);
}


VIDEO_START( quasar )
{
	quasar_effectram = auto_malloc(0x400);

	/* configure the S2636 chips */
	s2636_0 = s2636_config(cvs_s2636_0_ram, machine->screen[0].height, machine->screen[0].width, CVS_S2636_Y_OFFSET, CVS_S2636_X_OFFSET);
	s2636_1 = s2636_config(cvs_s2636_1_ram, machine->screen[0].height, machine->screen[0].width, CVS_S2636_Y_OFFSET, CVS_S2636_X_OFFSET);
	s2636_2 = s2636_config(cvs_s2636_2_ram, machine->screen[0].height, machine->screen[0].width, CVS_S2636_Y_OFFSET, CVS_S2636_X_OFFSET);

	/* create helper bitmaps */
	cvs_collision_background = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, machine->screen[0].format);
}

VIDEO_UPDATE( quasar )
{
	int offs;
	mame_bitmap *s2636_0_bitmap;
	mame_bitmap *s2636_1_bitmap;
	mame_bitmap *s2636_2_bitmap;

	/* for every character in the video RAM */
	for (offs = 0; offs < 0x0400; offs++)
	{
		int ox, oy;
        UINT8 code = cvs_video_ram[offs];

		UINT8 x = (offs & 0x1f) << 3;
		UINT8 y = (offs >> 5) << 3;

		// While we have the current character code, draw the effects layer
		// intensity / on and off controlled by latch

		int forecolor = 0x208 + quasar_effectram[offs] + (256 * (((quasar_effectcontrol >> 4) ^ 3) & 3));

		for(ox=0;ox<8;ox++)
			for(oy=0;oy<8;oy++)
				*BITMAP_ADDR16(bitmap, y+oy, x+ox) = forecolor;

		/* Main Screen */

		drawgfx(bitmap,machine->gfx[0],
				code,
				cvs_color_ram[offs] & 0x3f,
				0,0,
				x,y,
				cliprect,TRANSPARENCY_PEN,0);


		/* background for Collision Detection (it can only hit certain items) */

		if((cvs_color_ram[offs] & 7) == 0)
		{
			drawgfx(cvs_collision_background,machine->gfx[0],
					code,
					64,
					0,0,
					x,y,
					cliprect,TRANSPARENCY_NONE,0);
		}
	}

    /* update the S2636 chips */
	s2636_0_bitmap = s2636_update(s2636_0, cliprect);
	s2636_1_bitmap = s2636_update(s2636_1, cliprect);
	s2636_2_bitmap = s2636_update(s2636_2, cliprect);

    /* Bullet Hardware */
    for (offs = 8; offs < 256; offs++ )
    {
        if(cvs_bullet_ram[offs] != 0)
        {
        	int ct;
            for(ct=0;ct<1;ct++)
            {
            	int bx=255-9-cvs_bullet_ram[offs]-ct;

            	/* bullet/object Collision */
				if (*BITMAP_ADDR8(s2636_0_bitmap, offs, bx) != 0) cvs_collision_register |= 0x04;
				if (*BITMAP_ADDR8(s2636_2_bitmap, offs, bx) != 0) cvs_collision_register |= 0x08;

				*BITMAP_ADDR16(bitmap, offs, bx) = machine->pens[7];
            }
        }
    }


	/* mix and copy the S2636 images into the main bitmap, also check for collision */
	{
		int y;

		for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		{
			int x;

			for (x = cliprect->min_x; x <= cliprect->max_x; x++)
			{
				int pixel0 = *BITMAP_ADDR8(s2636_0_bitmap, y, x);
				int pixel1 = *BITMAP_ADDR8(s2636_1_bitmap, y, x);
				int pixel2 = *BITMAP_ADDR8(s2636_2_bitmap, y, x);

				int pixel = pixel0 | pixel1 | pixel2;

				if (S2636_IS_PIXEL_DRAWN(pixel))
				{
					*BITMAP_ADDR16(bitmap, y, x) = machine->pens[S2636_PIXEL_COLOR(pixel)];

					/* S2636 vs. background collision detection */
					if (colortable_entry_get_value(machine->colortable, *BITMAP_ADDR16(cvs_collision_background, y, x)))
					{
						if (S2636_IS_PIXEL_DRAWN(pixel0)) cvs_collision_register |= 0x01;
						if (S2636_IS_PIXEL_DRAWN(pixel2)) cvs_collision_register |= 0x02;
					}
				}
			}
		}
	}

	return 0;
}
