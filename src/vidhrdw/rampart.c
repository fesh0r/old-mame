/***************************************************************************

	Atari Rampart hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"



/*************************************
 *
 *	Globals we own
 *
 *************************************/

data16_t *rampart_bitmap;



/*************************************
 *
 *	Statics
 *
 *************************************/

static int *pfusage;
static UINT8 *pfdirty;
static struct osd_bitmap *pfbitmap;
static int xdim, ydim;

int rampart_bitmap_init(int _xdim, int _ydim);
void rampart_bitmap_free(void);
void rampart_bitmap_invalidate(void);
void rampart_bitmap_mark_palette(int base);
void rampart_bitmap_render(struct osd_bitmap *bitmap);



/*************************************
 *
 *	Video system start
 *
 *************************************/

int rampart_vh_start(void)
{
	static const struct atarimo_desc modesc =
	{
		0,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x100,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x00ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x7fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xff80,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x0007 }},	/* mask for the height, in tiles */
		{{ 0,0x8000,0,0 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */
		
		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0,					/* callback routine for ignored entries */
	};

	/* initialize the playfield */
	if (!rampart_bitmap_init(43*8, 30*8))
		goto cant_create_pf;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		goto cant_create_mo;

	/* set the intial scroll offset */
	atarimo_set_xscroll(0, -4, 0);
	return 0;

	/* error cases */
cant_create_mo:
	rampart_bitmap_free();
cant_create_pf:
	return 1;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void rampart_vh_stop(void)
{
	atarimo_free();
	rampart_bitmap_free();
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

void rampart_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	/* mark the used colors */
	palette_init_used_colors();
	rampart_bitmap_mark_palette(0);
	atarimo_mark_palette(0);

	/* update the palette, and mark things dirty if we need to */
	if (palette_recalc())
		rampart_bitmap_invalidate();

	/* draw the layers */
	rampart_bitmap_render(bitmap);
	atarimo_render(0, bitmap, NULL, NULL);
}



/*************************************
 *
 *	Bitmap initialization
 *
 *************************************/

int rampart_bitmap_init(int _xdim, int _ydim)
{
	/* set the dimensions */
	xdim = _xdim;
	ydim = _ydim;

	/* allocate color usage */
	pfusage = malloc(sizeof(pfusage[0]) * 256);
	if (!pfusage)
		goto cant_alloc_usage;
	pfusage[0] = xdim * ydim;
	
	/* allocate dirty map */
	pfdirty = malloc(sizeof(pfdirty[0]) * ydim);
	if (!pfdirty)
		goto cant_alloc_dirty;
	memset(pfdirty, 1, sizeof(pfdirty[0]) * ydim);
	
	/* allocate playfield bitmap */
	pfbitmap = bitmap_alloc(xdim, ydim);
	if (!pfbitmap)
		goto cant_alloc_pf;
	return 1;

	/* error cases */
cant_alloc_pf:
	free(pfdirty);
cant_alloc_dirty:
	free(pfusage);
cant_alloc_usage:
	return 0;
}



/*************************************
 *
 *	Bitmap freeing
 *
 *************************************/

void rampart_bitmap_free(void)
{
	bitmap_free(pfbitmap);
	free(pfdirty);
	free(pfusage);
}



/*************************************
 *
 *	Bitmap invalidating
 *
 *************************************/

void rampart_bitmap_invalidate(void)
{
	memset(pfdirty, 1, ydim * sizeof(pfdirty[0]));
}



/*************************************
 *
 *	Bitmap RAM write handler
 *
 *************************************/

WRITE16_HANDLER( rampart_bitmap_w )
{
	int oldword = rampart_bitmap[offset];
	int newword = oldword;
	int x, y;

	COMBINE_DATA(&newword);
	if (oldword != newword)
	{
		rampart_bitmap[offset] = newword;

		/* track color usage */
		x = offset % 256;
		y = offset / 256;
		if (x < xdim && y < ydim)
		{
			pfusage[(oldword >> 8) & 0xff]--;
			pfusage[oldword & 0xff]--;
			pfusage[(newword >> 8) & 0xff]++;
			pfusage[newword & 0xff]++;
			pfdirty[y] = 1;
		}
	}
}



/*************************************
 *
 *	Bitmap palette marking
 *
 *************************************/

void rampart_bitmap_mark_palette(int base)
{
	int i;
	
	for (i = 0; i < 256; i++)
		if (pfusage[i])
			palette_used_colors[base + i] = PALETTE_COLOR_USED;
}



/*************************************
 *
 *	Bitmap rendering
 *
 *************************************/

void rampart_bitmap_render(struct osd_bitmap *bitmap)
{
	int x, y;

	/* update any dirty scanlines */
	for (y = 0; y < ydim; y++)
		if (pfdirty[y])
		{
			const data16_t *src = &rampart_bitmap[256 * y];

			/* regenerate the line */
			for (x = 0; x < xdim / 2; x++)
			{
				int bits = *src++;
				plot_pixel(pfbitmap, x*2+0, y, Machine->pens[bits >> 8]);
				plot_pixel(pfbitmap, x*2+1, y, Machine->pens[bits & 0xff]);
			}
			pfdirty[y] = 0;
		}

	/* copy the cached bitmap */
	copybitmap(bitmap, pfbitmap, 0, 0, 0, 0, NULL, TRANSPARENCY_NONE, 0);
}
