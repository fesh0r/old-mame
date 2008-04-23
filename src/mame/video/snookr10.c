/**********************************************************************************


    SNOOKER 10 / SANDII'

    Driver by Roberto Fresca.


    Games running on this hardware:

    * Snooker 10 (Ver 1.11), Sandii', 1998.
    * Apple 10 (Ver 1.21),   Sandii', 1998.
    * Ten Balls (Ver 1.05),  unknown, 1997.


**********************************************************************************/

#include "driver.h"
#include "video/resnet.h"

static tilemap *bg_tilemap;


PALETTE_INIT( snookr10 )
{
	int i;

	/* RRRBBBGG */
	if (color_prom == 0) return;

	for (i = 0;i < machine->config->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(machine,i,MAKE_RGB(r,g,b));
	}
}

WRITE8_HANDLER( snookr10_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

WRITE8_HANDLER( snookr10_colorram_w )
{
	colorram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

/**** normal hardware limit ****
    - bits -
    7654 3210
    xxxx xx--   tiles color.
    xxx- x-xx   tiles color (title).
    xxxx -xxx   tiles color (background).
*/

static TILE_GET_INFO( get_bg_tile_info )
{
/*  - bits -
    7654 3210
    xxxx ----   tiles color.
    ---- xxxx   unused.
*/
	int offs = tile_index;
	int attr = videoram[offs] + (colorram[offs] << 8);
	int code = attr & 0xfff;
	int color = colorram[offs] >> 4;	/* 4 bits for color */

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( snookr10 )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 4, 8, 128, 32);
}

VIDEO_UPDATE( snookr10 )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}
