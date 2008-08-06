/***************************************************************************

    Gottlieb hardware

***************************************************************************/

#include "driver.h"
#include "gottlieb.h"
#include "machine/laserdsc.h"
#include "video/resnet.h"

UINT8 *gottlieb_charram;

UINT8 gottlieb_gfxcharlo;
UINT8 gottlieb_gfxcharhi;

static UINT8 background_priority = 0;
static UINT8 spritebank;

static tilemap *bg_tilemap;
static double weights[4];

static render_texture *video_texture;
static render_texture *overlay_texture;
static const rectangle overlay_clip = { 0, GOTTLIEB_VIDEO_HBLANK-1, 0, GOTTLIEB_VIDEO_VBLANK-8 };



/*************************************
 *
 *  Palette RAM writes
 *
 *************************************/

WRITE8_HANDLER( gottlieb_paletteram_w )
{
	int r, g, b, val;

	paletteram[offset] = data;

	/* blue & green are encoded in the even bytes */
	val = paletteram[offset & ~1];
	g = combine_4_weights(weights, (val >> 4) & 1, (val >> 5) & 1, (val >> 6) & 1, (val >> 7) & 1);
	b = combine_4_weights(weights, (val >> 0) & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1);

	/* red is encoded in the odd bytes */
	val = paletteram[offset | 1];
	r = combine_4_weights(weights, (val >> 0) & 1, (val >> 1) & 1, (val >> 2) & 1, (val >> 3) & 1);

	palette_set_color(machine, offset / 2, MAKE_ARGB(((offset & ~1) == 0) ? 0x00 : 0xff, r, g, b));
}



/*************************************
 *
 *  Video controls
 *
 *************************************/

WRITE8_HANDLER( gottlieb_video_control_w )
{
	/* bit 0 controls foreground/background priority */
	if (background_priority != (data & 0x01))
		video_screen_update_partial(machine->primary_screen, video_screen_get_vpos(machine->primary_screen));
	background_priority = data & 0x01;

	/* bit 1 controls horizonal flip screen */
	if (flip_screen_x_get() != (data & 0x02))
	{
		flip_screen_x_set(data & 0x02);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* bit 2 controls horizonal flip screen */
	if (flip_screen_y_get() != (data & 0x04))
	{
		flip_screen_y_set(data & 0x04);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	/* in Q*Bert Qubes only, bit 4 controls the sprite bank */
	spritebank = (data & 0x10) >> 4;
}


WRITE8_HANDLER( gottlieb_laserdisc_video_control_w )
{
	/* bit 0 works like the other games */
	gottlieb_video_control_w(machine, offset, data & 0x01);

	/* bit 1 controls the sprite bank. */
	spritebank = (data & 0x02) >> 1;

	/* bit 2 video enable (0 = black screen) */

	/* bit 3 genlock control (1 = show laserdisc image) */
	render_container_set_palette_alpha(render_container_get_screen(machine->primary_screen), 0, (data & 0x08) ? 0x00 : 0xff);
}



/*************************************
 *
 *  Video RAM and character RAM access
 *
 *************************************/

WRITE8_HANDLER( gottlieb_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}


WRITE8_HANDLER( gottlieb_charram_w )
{
	if (gottlieb_charram[offset] != data)
	{
		gottlieb_charram[offset] = data;
		decodechar(machine->gfx[0], offset / 32, gottlieb_charram);
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}



/*************************************
 *
 *  Palette RAM writes
 *
 *************************************/

static TILE_GET_INFO( get_bg_tile_info )
{
	int code = videoram[tile_index];
	if ((code & 0x80) == 0)
		SET_TILE_INFO(gottlieb_gfxcharlo, code, 0, 0);
	else
		SET_TILE_INFO(gottlieb_gfxcharhi, code, 0, 0);
}


VIDEO_START( gottlieb )
{
	static const int resistances[4] = { 2000, 1000, 470, 240 };

	/* compute palette information */
	/* note that there really are pullup/pulldown resistors, but this situation is complicated */
	/* by the use of transistors, so we ignore that and just use the realtive resistor weights */
	compute_resistor_weights(0,	255, -1.0,
			4, resistances, weights, 180, 0,
			4, resistances, weights, 180, 0,
			4, resistances, weights, 180, 0);

	/* configure the background tilemap */
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 8, 8, 32, 32);
	tilemap_set_transparent_pen(bg_tilemap, 0);
	tilemap_set_scrolldx(bg_tilemap, 0, 318 - 256);

	/* save some state */
	state_save_register_global(background_priority);
	state_save_register_global(spritebank);
}


VIDEO_START( gottlieb_laserdisc )
{
	/* handle normal video */
	VIDEO_START_CALL(gottlieb);

	/* allocate an overlay texture */
	overlay_texture = render_texture_alloc(NULL, NULL);
	if (overlay_texture == NULL)
		fatalerror("Out of memory allocating overlay texture");

	/* allocate a video texture */
	video_texture = render_texture_alloc(NULL, NULL);
	if (video_texture == NULL)
		fatalerror("Out of memory allocating video texture");
}



/*************************************
 *
 *  Sprite rendering
 *
 *************************************/

static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
    int offs;

	for (offs = 0; offs < 256; offs += 4)
	{
		/* coordinates hand tuned to make the position correct in Q*Bert Qubes start */
		/* of level animation. */
		int sx = (spriteram[offs + 1]) - 4;
		int sy = (spriteram[offs]) - 13;
		int code = (255 ^ spriteram[offs + 2]) + 256 * spritebank;

		if (flip_screen_x_get()) sx = 233 - sx;
		if (flip_screen_y_get()) sy = 244 - sy;

		drawgfx(bitmap, machine->gfx[2],
			code, 0,
			flip_screen_x_get(), flip_screen_y_get(),
			sx,sy,
			cliprect,
			TRANSPARENCY_PEN, 0);
	}
}



/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_UPDATE( gottlieb )
{
	/* if the background has lower priority, render it first, else clear the screen */
	if (!background_priority)
		tilemap_draw(bitmap, cliprect, bg_tilemap, TILEMAP_DRAW_OPAQUE, 0);
	else
		fillbitmap(bitmap, 0, cliprect);

	/* draw the sprites */
	draw_sprites(screen->machine, bitmap, cliprect);

	/* if the background has higher priority, render it now */
	if (background_priority)
		tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	return 0;
}


VIDEO_UPDATE( gottlieb_laserdisc )
{
	const device_config *laserdisc = device_list_first(screen->machine->config->devicelist, LASERDISC);
	rectangle clip = *cliprect;
	bitmap_t *video_bitmap;

	/* scale the cliprect to the screen and render it */
	clip.min_x = 0;
	clip.max_x = GOTTLIEB_VIDEO_HBLANK - 1;
	clip.min_y = cliprect->min_y * GOTTLIEB_VIDEO_VCOUNT / bitmap->height;
	clip.max_y = (cliprect->max_y + 1) * GOTTLIEB_VIDEO_VCOUNT / bitmap->height - 1;
	video_update_gottlieb(screen, bitmap, &clip);

	/* if this is the last update, handle it */
	if (cliprect->max_y == video_screen_get_visible_area(screen)->max_y)
	{
		/* update the texture with the overlay contents */
		render_texture_set_bitmap(overlay_texture, bitmap, &overlay_clip, 0, TEXFORMAT_PALETTEA16);

		/* now talk to the laserdisc */
		laserdisc_get_video(laserdisc, &video_bitmap);
		if (video_bitmap != NULL)
			render_texture_set_bitmap(video_texture, video_bitmap, NULL, 0, TEXFORMAT_YUY16);

		/* add both quads to the screen */
		render_container_empty(render_container_get_screen(screen));
		render_screen_add_quad(screen, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), video_texture, PRIMFLAG_BLENDMODE(BLENDMODE_NONE) | PRIMFLAG_SCREENTEX(1));
		render_screen_add_quad(screen, 0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(0xff,0xff,0xff,0xff), overlay_texture, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_SCREENTEX(1));
	}

	return 0;
}
