#include "driver.h"
#include "vidhrdw/konamiic.h"


static int zoom_colorbase[2],road_colorbase[2],sprite_colorbase;


/***************************************************************************

  Callbacks for the K053247

***************************************************************************/

static void overdriv_sprite_callback(int *code,int *color,int *priority_mask)
{
	int pri = (*color & 0xffe0) >> 5;	/* ??????? */
	if (pri) *priority_mask = 0x02;
	else     *priority_mask = 0x00;

	*color = sprite_colorbase + (*color & 0x001f);
}


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

static void zoom_callback_0(int *code,int *color)
{
	tile_info.flags = (*color & 0x40) ? TILE_FLIPX : 0;
	*code |= ((*color & 0x03) << 8);
	*color = zoom_colorbase[0] + ((*color & 0x3c) >> 2);
}

static void zoom_callback_1(int *code,int *color)
{
	tile_info.flags = (*color & 0x40) ? TILE_FLIPX : 0;
	*code |= ((*color & 0x03) << 8);
	*color = zoom_colorbase[1] + ((*color & 0x3c) >> 2);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int overdriv_vh_start(void)
{
	K053251_vh_start();

	if (K051316_vh_start_0(REGION_GFX2,4,zoom_callback_0))
	{
		return 1;
	}

	if (K051316_vh_start_1(REGION_GFX3,4,zoom_callback_1))
	{
		K051316_vh_stop_0();
		return 1;
	}
	if (K053247_vh_start(REGION_GFX1,77,22,NORMAL_PLANE_ORDER,overdriv_sprite_callback))
	{
		K051316_vh_stop_0();
		K051316_vh_stop_1();
		return 1;
	}

	K051316_wraparound_enable(0,1);
	K051316_set_offset(0,14,-1);
	K051316_set_offset(1,15,0);

	return 0;
}

void overdriv_vh_stop(void)
{
	K051316_vh_stop_0();
	K051316_vh_stop_1();
	K053247_vh_stop();
}



/***************************************************************************

  Display refresh

***************************************************************************/

void overdriv_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i;


	sprite_colorbase  = K053251_get_palette_index(K053251_CI0);
	road_colorbase[1] = K053251_get_palette_index(K053251_CI1);
	road_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	zoom_colorbase[1] = K053251_get_palette_index(K053251_CI3);
	zoom_colorbase[0] = K053251_get_palette_index(K053251_CI4);

	K051316_tilemap_update_0();
	K051316_tilemap_update_1();

	palette_init_used_colors();
	K053247_mark_sprites_colors();

	/* set transparent pens for the K051316 */
	for (i = 0;i < 64;i++)
		palette_used_colors[(zoom_colorbase[1] + i) * 16] = PALETTE_COLOR_TRANSPARENT;

	palette_recalc();

	fillbitmap(priority_bitmap,0,NULL);

	K051316_zoom_draw_0(bitmap,0);
	K051316_zoom_draw_1(bitmap,1);

	K053247_sprites_draw(bitmap);
}
