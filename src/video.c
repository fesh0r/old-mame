/***************************************************************************

    video.c

    Core MAME video routines.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "profiler.h"
#include "png.h"
#include "debugger.h"
#include "vidhrdw/vector.h"

#ifdef NEW_RENDER
#include "render.h"
#else
#include "artwork.h"
#endif

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
#include "mamedbg.h"
#endif


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define LOG_PARTIAL_UPDATES(x)		/* logerror x */

#define FRAMES_PER_FPS_UPDATE		12

/* VERY IMPORTANT: bitmap_alloc must allocate also a "safety area" 16 pixels wide all
   around the bitmap. This is required because, for performance reasons, some graphic
   routines don't clip at boundaries of the bitmap. */
#define BITMAP_SAFETY				16



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

#ifdef MESS
typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	union
	{
		void		(*full_refresh)(void);
	} func;
};
#endif



/***************************************************************************
    GLOBALS
***************************************************************************/

/* handy globals for other parts of the system */
int vector_updates = 0;

/* main bitmap to render to */
#ifdef NEW_RENDER
static int skipping_this_frame;
static render_texture *scrtexture[MAX_SCREENS];
static int scrformat[MAX_SCREENS];
static int scrchanged[MAX_SCREENS];
static render_bounds scrbounds[MAX_SCREENS];
static
#endif
mame_bitmap *scrbitmap[MAX_SCREENS][2];
static int curbitmap[MAX_SCREENS];
static rectangle eff_visible_area[MAX_SCREENS];

/* the active video display */
#ifndef NEW_RENDER
static mame_display current_display;
static UINT8 visible_area_changed;
static UINT8 refresh_rate_changed;
static UINT8 full_refresh_pending;
#endif

/* video updating */
static int last_partial_scanline[MAX_SCREENS];

/* speed computation */
static cycles_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static performance_info performance;

/* movie file */
static mame_file *movie_file = NULL;
static int movie_frame = 0;

/* misc other statics */
static UINT32 leds_status;
#ifdef MESS
static callback_item *full_refresh_callback_list;
#endif

/* artwork callbacks */
#ifndef NEW_RENDER
#ifndef MESS
static artwork_callbacks mame_artwork_callbacks =
{
	NULL,
	artwork_load_artwork_file
};
#endif
#endif



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void video_pause(int pause);
static void video_exit(void);
static int allocate_graphics(const gfx_decode *gfxdecodeinfo);
static void decode_graphics(const gfx_decode *gfxdecodeinfo);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);
static void recompute_fps(int skipped_it);
static void recompute_visible_areas(void);



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    video_init - start up the video system
-------------------------------------------------*/

int video_init(void)
{
	movie_file = NULL;
#ifdef MESS
	full_refresh_callback_list = NULL;
#endif
	movie_frame = 0;

#ifndef NEW_RENDER
	add_pause_callback(video_pause);
#endif
	add_exit_callback(video_exit);

#ifndef NEW_RENDER
{
	int bmwidth = Machine->drv->screen[0].maxwidth;
	int bmheight = Machine->drv->screen[0].maxheight;
	artwork_callbacks *artcallbacks;
	osd_create_params params;

	/* if we're a vector game, override the screen width and height */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* compute the visible area for raster games */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		params.width = Machine->drv->screen[0].default_visible_area.max_x - Machine->drv->screen[0].default_visible_area.min_x + 1;
		params.height = Machine->drv->screen[0].default_visible_area.max_y - Machine->drv->screen[0].default_visible_area.min_y + 1;
	}
	else
	{
		params.width = bmwidth;
		params.height = bmheight;
	}

	/* fill in the rest of the display parameters */
	params.aspect_x = 1333;
	params.aspect_y = 1000;
	params.depth = Machine->color_depth;
	params.colors = palette_get_total_colors_with_ui();
	params.fps = Machine->drv->screen[0].refresh_rate;
	params.video_attributes = Machine->drv->video_attributes;

#ifdef MESS
	artcallbacks = &mess_artwork_callbacks;
#else
	artcallbacks = &mame_artwork_callbacks;
#endif

	/* initialize the display through the artwork (and eventually the OSD) layer */
	if (artwork_create_display(&params, direct_rgb_components, artcallbacks))
		return 1;

	/* the create display process may update the vector width/height, so recompute */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* now allocate the screen bitmap */
	scrbitmap[0][0] = auto_bitmap_alloc_depth(bmwidth, bmheight, Machine->color_depth);
	if (!scrbitmap[0][0])
		return 1;

	/* set the default refresh rate */
	set_refresh_rate(0, Machine->drv->screen[0].refresh_rate);

	/* set the default visible area */
	set_visible_area(0, 0,1,0,1);	// make sure everything is recalculated on multiple runs
	set_visible_area(0,
			Machine->drv->screen[0].default_visible_area.min_x,
			Machine->drv->screen[0].default_visible_area.max_x,
			Machine->drv->screen[0].default_visible_area.min_y,
			Machine->drv->screen[0].default_visible_area.max_y);
}
#else
{
	int scrnum;

	/* loop over screens and allocate bitmaps */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
			/* allocate bitmaps */
			scrbitmap[scrnum][0] = auto_bitmap_alloc_depth(Machine->drv->screen[scrnum].maxwidth, Machine->drv->screen[scrnum].maxheight, Machine->color_depth);
			scrbitmap[scrnum][1] = auto_bitmap_alloc_depth(Machine->drv->screen[scrnum].maxwidth, Machine->drv->screen[scrnum].maxheight, Machine->color_depth);

			/* choose the texture format */
			if (Machine->color_depth == 16)
				scrformat[scrnum] = TEXFORMAT_PALETTE16;
			else if (Machine->color_depth == 15)
				scrformat[scrnum] = TEXFORMAT_RGB15;
			else
				scrformat[scrnum] = TEXFORMAT_RGB32;

			/* allocate a texture per screen */
			scrtexture[scrnum] = render_texture_alloc(scrbitmap[scrnum][0], NULL, &adjusted_palette[Machine->drv->screen[scrnum].palette_base], scrformat[scrnum], NULL, NULL);

			/* set the default refresh rate */
			set_refresh_rate(scrnum, Machine->drv->screen[scrnum].refresh_rate);

			/* set the default visible area */
			set_visible_area(scrnum, 0,1,0,1);	// make sure everything is recalculated on multiple runs
			set_visible_area(scrnum,
					Machine->drv->screen[scrnum].default_visible_area.min_x,
					Machine->drv->screen[scrnum].default_visible_area.max_x,
					Machine->drv->screen[scrnum].default_visible_area.min_y,
					Machine->drv->screen[scrnum].default_visible_area.max_y);
		}
}
#endif

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			return 1;

#ifndef NEW_RENDER
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* if the debugger is enabled, initialize its bitmap and font */
	if (Machine->debug_mode)
	{
		int depth = options.debug_depth ? options.debug_depth : Machine->color_depth;

		/* first allocate the debugger bitmap */
		Machine->debug_bitmap = auto_bitmap_alloc_depth(options.debug_width, options.debug_height, depth);
		if (!Machine->debug_bitmap)
			return 1;

		/* then create the debugger font */
		Machine->debugger_font = build_debugger_font();
		if (Machine->debugger_font == NULL)
			return 1;
	}
#endif
#endif

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo)
		if (allocate_graphics(Machine->drv->gfxdecodeinfo))
			return 1;

	/* initialize the palette - must be done after osd_create_display() */
	palette_config();

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* actually decode the graphics */
	if (Machine->drv->gfxdecodeinfo)
		decode_graphics(Machine->drv->gfxdecodeinfo);

	/* reset performance data */
	last_fps_time = osd_cycles();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = Machine->refresh_rate[0];
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	/* initialize tilemaps */
	if (tilemap_init() != 0)
		fatalerror("tilemap_init failed");

	recompute_visible_areas();
	return 0;
}


/*-------------------------------------------------
    video_pause - pause the video system
-------------------------------------------------*/

static void video_pause(int pause)
{
	schedule_full_refresh();
}


/*-------------------------------------------------
    video_exit - close down the video system
-------------------------------------------------*/

static void video_exit(void)
{
	int i;

	/* stop recording any movie */
	record_movie_stop();

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

#ifndef NEW_RENDER
#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* free the font elements */
	if (Machine->debugger_font)
	{
		freegfx(Machine->debugger_font);
		Machine->debugger_font = NULL;
	}
#endif

	/* close down the OSD layer's display */
	osd_close_display();
#else
{
	int scrnum;

	/* free all the render textures */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL && scrtexture[scrnum] != NULL)
			render_texture_free(scrtexture[scrnum]);
}
#endif
}


/*-------------------------------------------------
    allocate_graphics - allocate memory for the
    graphics
-------------------------------------------------*/

static int allocate_graphics(const gfx_decode *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT32 extxoffs[MAX_ABS_GFX_SIZE], extyoffs[MAX_ABS_GFX_SIZE];
		gfx_layout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;
		if (glcopy.extxoffs)
		{
			memcpy(extxoffs, glcopy.extxoffs, glcopy.width * sizeof(extxoffs[0]));
			glcopy.extxoffs = extxoffs;
		}
		if (glcopy.extyoffs)
		{
			memcpy(extyoffs, glcopy.extyoffs, glcopy.height * sizeof(extyoffs[0]));
			glcopy.extyoffs = extyoffs;
		}

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* for non-raw graphics, decode the X and Y offsets */
		if (glcopy.planeoffset[0] != GFX_RAW)
		{
			UINT32 *xoffset = glcopy.extxoffs ? extxoffs : glcopy.xoffset;
			UINT32 *yoffset = glcopy.extyoffs ? extyoffs : glcopy.yoffset;

			/* loop over all the planes, converting fractions */
			for (j = 0; j < glcopy.planes; j++)
			{
				UINT32 value = glcopy.planeoffset[j];
				if (IS_FRAC(value))
					glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			/* loop over all the X/Y offsets, converting fractions */
			for (j = 0; j < glcopy.width; j++)
			{
				UINT32 value = xoffset[j];
				if (IS_FRAC(value))
					xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}

			for (j = 0; j < glcopy.height; j++)
			{
				UINT32 value = yoffset[j];
				if (IS_FRAC(value))
					yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
			}
		}

		/* otherwise, just use yoffset[0] as the line modulo */
		else
		{
			int base = gfxdecodeinfo[i].start;
			int end = region_length/8;
			while (glcopy.total > 0)
			{
				int elementbase = base + (glcopy.total - 1) * glcopy.charincrement / 8;
				int lastpixelbase = elementbase + glcopy.height * glcopy.yoffset[0] / 8 - 1;
				if (lastpixelbase < end)
					break;
				glcopy.total--;
			}
		}

		/* allocate the graphics */
		Machine->gfx[i] = allocgfx(&glcopy);

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}


/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static void decode_graphics(const gfx_decode *gfxdecodeinfo)
{
	int totalgfx = 0, curgfx = 0;
	int i;

	/* count total graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
			totalgfx += Machine->gfx[i]->total_elements;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
		if (Machine->gfx[i])
		{
			/* if we have a valid region, decode it now */
			if (gfxdecodeinfo[i].memory_region > REGION_INVALID)
			{
				UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
				gfx_element *gfx = Machine->gfx[i];
				int j;

				/* now decode the actual graphics */
				for (j = 0; j < gfx->total_elements; j += 1024)
				{
					int num_to_decode = (j + 1024 < gfx->total_elements) ? 1024 : (gfx->total_elements - j);
					decodegfx(gfx, region_base + gfxdecodeinfo[i].start, j, num_to_decode);
					curgfx += num_to_decode;
#ifdef NEW_RENDER
{
	char buffer[200];
	sprintf(buffer, "Decoding (%d%%)", curgfx * 100 / totalgfx);
	ui_set_startup_text(buffer, FALSE);
}
#endif
				}
			}

			/* otherwise, clear the target region */
			else
				memset(Machine->gfx[i]->gfxdata, 0, Machine->gfx[i]->char_modulo * Machine->gfx[i]->total_elements);
		}
}


#ifndef NEW_RENDER
/*-------------------------------------------------
    scale_vectorgames - scale the vector games
    to a given resolution
-------------------------------------------------*/

static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height)
{
	double x_scale, y_scale, scale;

	/* compute the scale values */
	x_scale = (double)gfx_width  / *width;
	y_scale = (double)gfx_height / *height;

	/* pick the smaller scale factor */
	scale = (x_scale < y_scale) ? x_scale : y_scale;

	/* compute the new size */
	*width  = *width  * scale + 0.5;
	*height = *height * scale + 0.5;

	/* round to the nearest 4 pixel value */
	*width  &= ~3;
	*height &= ~3;
}
#endif


/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		logerror("video_init():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);

	/* register for saving it */
	state_save_register_global_pointer(buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);

		/* register for saving it */
		state_save_register_global_pointer(buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (UINT16 *)buffered_spriteram;
	buffered_spriteram32 = (UINT32 *)buffered_spriteram;
	buffered_spriteram16_2 = (UINT16 *)buffered_spriteram_2;
	buffered_spriteram32_2 = (UINT32 *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************

    Screen rendering and management.

***************************************************************************/

/*-------------------------------------------------
    set_visible_area - adjusts the visible portion
    of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int scrnum, int min_x, int max_x, int min_y, int max_y)
{
#ifndef NEW_RENDER
	if (       Machine->visible_area[0].min_x == min_x
			&& Machine->visible_area[0].max_x == max_x
			&& Machine->visible_area[0].min_y == min_y
			&& Machine->visible_area[0].max_y == max_y)
		return;

	/* "dirty" the area for the next display update */
	visible_area_changed = 1;

	/* bounds check */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) && scrbitmap[0][0])
		if ((min_x < 0) || (min_y < 0) || (max_x >= scrbitmap[0][0]->width) || (max_y >= scrbitmap[0][0]->height))
		{
			fatalerror("set_visible_area(%d,%d,%d,%d) out of bounds; bitmap dimensions are (%d,%d)",
				min_x, min_y, max_x, max_y,
				scrbitmap[0][0]->width, scrbitmap[0][0]->height);
		}

	/* set the new values in the Machine struct */
	Machine->visible_area[0].min_x = min_x;
	Machine->visible_area[0].max_x = max_x;
	Machine->visible_area[0].min_y = min_y;
	Machine->visible_area[0].max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		Machine->absolute_visible_area.min_x = 0;
		Machine->absolute_visible_area.max_x = scrbitmap[0][0]->width - 1;
		Machine->absolute_visible_area.min_y = 0;
		Machine->absolute_visible_area.max_y = scrbitmap[0][0]->height - 1;
	}

	/* raster games need to use the visible area */
	else
		Machine->absolute_visible_area = Machine->visible_area[0];

	/* recompute scanline timing */
	cpu_compute_scanline_timing();

	/* set UI visible area */
	ui_set_visible_area(Machine->absolute_visible_area.min_x,
						Machine->absolute_visible_area.min_y,
						Machine->absolute_visible_area.max_x,
						Machine->absolute_visible_area.max_y);
#else
	if (       Machine->visible_area[scrnum].min_x == min_x
			&& Machine->visible_area[scrnum].max_x == max_x
			&& Machine->visible_area[scrnum].min_y == min_y
			&& Machine->visible_area[scrnum].max_y == max_y)
		return;

	/* bounds check */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR) && scrbitmap[scrnum][0])
		if ((min_x < 0) || (min_y < 0) || (max_x >= scrbitmap[scrnum][0]->width) || (max_y >= scrbitmap[scrnum][0]->height))
		{
			fatalerror("set_visible_area(%d,%d,%d,%d) out of bounds; bitmap dimensions are (%d,%d)",
				min_x, min_y, max_x, max_y,
				scrbitmap[scrnum][0]->width, scrbitmap[scrnum][0]->height);
		}

	/* set the new values in the Machine struct */
	Machine->visible_area[scrnum].min_x = min_x;
	Machine->visible_area[scrnum].max_x = max_x;
	Machine->visible_area[scrnum].min_y = min_y;
	Machine->visible_area[scrnum].max_y = max_y;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
#endif
}


/*-------------------------------------------------
    set_refresh_rate - adjusts the refresh rate
    of the video mode dynamically
-------------------------------------------------*/

void set_refresh_rate(int scrnum, float fps)
{
	/* bail if already equal */
	if (Machine->refresh_rate[scrnum] == fps)
		return;

#ifndef NEW_RENDER
	/* "dirty" the rate for the next display update */
	refresh_rate_changed = 1;
#endif

	/* set the new values in the Machine struct */
	Machine->refresh_rate[scrnum] = fps;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}


/*-------------------------------------------------
    schedule_full_refresh - force a full erase
    and refresh the next frame
-------------------------------------------------*/

void schedule_full_refresh(void)
{
#ifndef NEW_RENDER
	full_refresh_pending = 1;
#endif
}


/*-------------------------------------------------
    force_partial_update - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scrnum, int scanline)
{
	rectangle clip = eff_visible_area[scrnum];
#if defined(MESS) && !defined(NEW_RENDER)
	callback_item *cb;
#endif

	LOG_PARTIAL_UPDATES(("Partial: force_partial_update(%d,%d): ", scrnum, scanline));

	/* if skipping this frame, bail */
	if (skip_this_frame())
	{
		LOG_PARTIAL_UPDATES(("skipped due to frameskipping\n"));
		return;
	}

	/* skip if less than the lowest so far */
	if (scanline < last_partial_scanline[scrnum])
	{
		LOG_PARTIAL_UPDATES(("skipped because less than previous\n"));
		return;
	}

#ifdef NEW_RENDER
	/* skip if this screen is not visible anywhere */
	if (!(render_get_live_screens_mask() & (1 << scrnum)))
	{
		LOG_PARTIAL_UPDATES(("skipped because screen not live\n"));
		return;
	}
#endif

#ifndef NEW_RENDER
	/* if there's a dirty bitmap and we didn't do any partial updates yet, handle it now */
	if (full_refresh_pending && last_partial_scanline[scrnum] == 0)
	{
		fillbitmap(scrbitmap[0][curbitmap[0]], get_black_pen(), NULL);
#ifdef MESS
		for (cb = full_refresh_callback_list; cb; cb = cb->next)
			(*cb->func.full_refresh)();
#endif
		full_refresh_pending = 0;
	}
#endif

	/* set the start/end scanlines */
	if (last_partial_scanline[scrnum] > clip.min_y)
		clip.min_y = last_partial_scanline[scrnum];
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		UINT32 flags;

		profiler_mark(PROFILER_VIDEO);
		LOG_PARTIAL_UPDATES(("updating %d-%d\n", clip.min_y, clip.max_y));
		flags = (*Machine->drv->video_update)(scrnum, scrbitmap[scrnum][curbitmap[scrnum]], &clip);
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);

#ifdef NEW_RENDER
		/* if we modified the bitmap, we have to commit */
		scrchanged[scrnum] |= (~flags & UPDATE_HAS_NOT_CHANGED);
#endif
	}

	/* remember where we left off */
	last_partial_scanline[scrnum] = scanline + 1;
}


/*-------------------------------------------------
    add_full_refresh_callback - request callback on
	full refesh
-------------------------------------------------*/

#ifdef MESS
void add_full_refresh_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	assert_always(mame_get_phase() == MAME_PHASE_INIT, "Can only call add_full_refresh_callback at init time!");

	cb = auto_malloc(sizeof(*cb));
	cb->func.full_refresh = callback;
	cb->next = NULL;

	for (cur = &full_refresh_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}
#endif



/*-------------------------------------------------
    reset_partial_updates - reset partial updates
    at the start of each frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	/* reset partial updates */
	LOG_PARTIAL_UPDATES(("Partial: reset to 0\n"));
	memset(last_partial_scanline, 0, sizeof(last_partial_scanline));
	performance.partial_updates_this_frame = 0;
}


/*-------------------------------------------------
    update_video_and_audio - actually call the
    OSD layer to perform an update
-------------------------------------------------*/

void update_video_and_audio(void)
{
	int skipped_it = skip_this_frame();

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	debug_trace_delay = 0;
#endif

#ifndef NEW_RENDER
	/* fill in our portion of the display */
	current_display.changed_flags = 0;

	/* set the main game bitmap */
	current_display.game_bitmap = scrbitmap[0][curbitmap[0]];
	current_display.game_bitmap_update = Machine->absolute_visible_area;
	if (!skipped_it)
		current_display.changed_flags |= GAME_BITMAP_CHANGED;

	/* set the visible area */
	current_display.game_visible_area = Machine->absolute_visible_area;
	if (visible_area_changed)
		current_display.changed_flags |= GAME_VISIBLE_AREA_CHANGED;

	/* set the refresh rate */
	current_display.game_refresh_rate = Machine->refresh_rate[0];
	if (refresh_rate_changed)
		current_display.changed_flags |= GAME_REFRESH_RATE_CHANGED;

	/* set the vector dirty list */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (!full_refresh_pending && !ui_is_dirty() && !skipped_it)
		{
			current_display.vector_dirty_pixels = vector_dirty_list;
			current_display.changed_flags |= VECTOR_PIXELS_CHANGED;
		}

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* set the debugger bitmap */
	current_display.debug_bitmap = Machine->debug_bitmap;
	if (debugger_bitmap_changed)
		current_display.changed_flags |= DEBUG_BITMAP_CHANGED;
	debugger_bitmap_changed = 0;

	/* adjust the debugger focus */
	if (debugger_focus != current_display.debug_focus)
	{
		current_display.debug_focus = debugger_focus;
		current_display.changed_flags |= DEBUG_FOCUS_CHANGED;
	}
#endif

	/* set the LED status */
	if (leds_status != current_display.led_state)
	{
		current_display.led_state = leds_status;
		current_display.changed_flags |= LED_STATE_CHANGED;
	}

	/* update with data from other parts of the system */
	palette_update_display(&current_display);

	/* render */
	artwork_update_video_and_audio(&current_display);

	/* reset dirty flags */
	visible_area_changed = 0;
	refresh_rate_changed = 0;
#else
{
	int scrnum;

	/* call the OSD to update */
	skipping_this_frame = osd_update(mame_timer_get_time());

	/* empty the containers */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
			render_container_empty(render_container_get_screen(scrnum));
}
#endif

	/* update FPS */
	recompute_fps(skipped_it);
}


/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

static void recompute_fps(int skipped_it)
{
	/* increment the frame counters */
	frames_since_last_fps++;
	if (!skipped_it)
		rendered_frames_since_last_fps++;

	/* if we didn't skip this frame, we may be able to compute a new FPS */
	if (!skipped_it && frames_since_last_fps >= FRAMES_PER_FPS_UPDATE)
	{
		cycles_t cps = osd_cycles_per_second();
		cycles_t curr = osd_cycles();
		double seconds_elapsed = (double)(curr - last_fps_time) * (1.0 / (double)cps);
		double frames_per_sec = (double)frames_since_last_fps / seconds_elapsed;

		/* compute the performance data */
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->refresh_rate[0];
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->refresh_rate[0])
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->refresh_rate[0];
	}
}


/*-------------------------------------------------
    video_frame_update - handle frameskipping and UI,
    plus updating the screen during normal
    operations
-------------------------------------------------*/

void video_frame_update(void)
{
	int paused = mame_is_paused();
	int phase = mame_get_phase();
	int scrnum;

	/* only render sound and video if we're in the running phase */
	if (phase == MAME_PHASE_RUNNING)
	{
		/* update sound */
		sound_frame_update();

		/* finish updating the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (Machine->drv->screen[scrnum].tag != NULL)
				force_partial_update(scrnum, eff_visible_area[scrnum].max_y);

		/* update our movie recording state */
		if (!paused)
			record_movie_frame(0);

#ifdef NEW_RENDER
{
		int livemask = render_get_live_screens_mask();

		/* now add the quads for all the screens */
		for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
			if (livemask & (1 << scrnum))
			{
				/* only update if empty and not a vector game; otherwise assume the driver did it directly */
				if (render_container_is_empty(render_container_get_screen(scrnum)) && !(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
				{
					mame_bitmap *bitmap = scrbitmap[scrnum][curbitmap[scrnum]];
					if (!skipping_this_frame && scrchanged[scrnum])
					{
						render_texture_set_bitmap(scrtexture[scrnum], bitmap, NULL, &adjusted_palette[Machine->drv->screen[scrnum].palette_base], scrformat[scrnum]);
						curbitmap[scrnum] = 1 - curbitmap[scrnum];
					}
					render_screen_add_quad(scrnum, scrbounds[scrnum].x0, scrbounds[scrnum].y0, scrbounds[scrnum].x1, scrbounds[scrnum].y1, MAKE_ARGB(0xff,0xff,0xff,0xff), scrtexture[scrnum], PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_SCREENTEX(1));
				}
			}

		/* reset the screen changed flags */
		memset(scrchanged, 0, sizeof(scrchanged));
}
#endif
	}

	/* the user interface must be called between vh_update() and osd_update_video_and_audio(), */
	/* to allow it to overlay things on the game display. We must call it even */
	/* if the frame is skipped, to keep a consistent timing. */
#ifndef NEW_RENDER
	ui_update_and_render(artwork_get_ui_bitmap());
#else
	ui_update_and_render();
#endif

	/* blit to the screen */
	update_video_and_audio();

	/* call the end-of-frame callback */
	if (phase == MAME_PHASE_RUNNING)
	{
		if (Machine->drv->video_eof && !paused)
		{
			profiler_mark(PROFILER_VIDEO);
			(*Machine->drv->video_eof)();
			profiler_mark(PROFILER_END);
		}

		/* reset partial updates if we're paused or if the debugger is active */
		if (paused || mame_debug_is_active())
			reset_partial_updates();

		/* recompute visible areas */
		recompute_visible_areas();
	}
}


/*-------------------------------------------------
    recompute_visible_areas - determine the
    effective visible areas and screen bounds
-------------------------------------------------*/

static void recompute_visible_areas(void)
{
	int scrnum;

	/* iterate over live screens */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (Machine->drv->screen[scrnum].tag != NULL)
		{
#ifdef NEW_RENDER
			render_container *scrcontainer = render_container_get_screen(scrnum);
			float xoffs = render_container_get_xoffset(scrcontainer);
			float yoffs = render_container_get_yoffset(scrcontainer);
			float xscale = render_container_get_xscale(scrcontainer);
			float yscale = render_container_get_yscale(scrcontainer);
			rectangle visarea = Machine->visible_area[scrnum];
			mame_bitmap *bitmap = scrbitmap[scrnum][curbitmap[scrnum]];
			float viswidth, visheight;
			float x0, y0, x1, y1;
			float xrecip, yrecip;

			/* adjust the max values so they are exclusive rather than inclusive */
			visarea.max_x++;
			visarea.max_y++;

			/* based on the game-configured visible area, compute the bounds we will draw
                the screen at so that a clipping at (0,0)-(1,1) will exactly result in
                the requested visible area */
			viswidth = (float)(visarea.max_x - visarea.min_x);
			visheight = (float)(visarea.max_y - visarea.min_y);
			xrecip = 1.0f / viswidth;
			yrecip = 1.0f / visheight;
			scrbounds[scrnum].x0 = 0.0f - (float)visarea.min_x * xrecip;
			scrbounds[scrnum].x1 = 1.0f + (float)(bitmap->width - visarea.max_x) * xrecip;
			scrbounds[scrnum].y0 = 0.0f - (float)visarea.min_y * yrecip;
			scrbounds[scrnum].y1 = 1.0f + (float)(bitmap->height - visarea.max_y) * yrecip;

			/* now apply the scaling/offset to the scrbounds */
			x0 = (0.5f - 0.5f * xscale + xoffs) + xscale * scrbounds[scrnum].x0;
			x1 = (0.5f - 0.5f * xscale + xoffs) + xscale * scrbounds[scrnum].x1;
			y0 = (0.5f - 0.5f * yscale + yoffs) + yscale * scrbounds[scrnum].y0;
			y1 = (0.5f - 0.5f * yscale + yoffs) + yscale * scrbounds[scrnum].y1;

			/* scale these values by the texture size */
			eff_visible_area[scrnum].min_x = floor((0.0f - x0) * viswidth);
			eff_visible_area[scrnum].max_x = bitmap->width - floor((x1 - 1.0f) * viswidth);
			eff_visible_area[scrnum].min_y = floor((0.0f - y0) * visheight);
			eff_visible_area[scrnum].max_y = bitmap->height - floor((y1 - 1.0f) * visheight);

			/* clamp against the width/height of the bitmaps */
			if (eff_visible_area[scrnum].min_x < 0) eff_visible_area[scrnum].min_x = 0;
			if (eff_visible_area[scrnum].max_x >= bitmap->width) eff_visible_area[scrnum].max_x = bitmap->width - 1;
			if (eff_visible_area[scrnum].min_y < 0) eff_visible_area[scrnum].min_y = 0;
			if (eff_visible_area[scrnum].max_y >= bitmap->height) eff_visible_area[scrnum].max_y = bitmap->height - 1;

			/* union this with the actual visible_area in case any game drivers rely
                on it */
			union_rect(&eff_visible_area[scrnum], &Machine->visible_area[scrnum]);
#else
			eff_visible_area[scrnum] = Machine->visible_area[scrnum];
#endif
		}
}


/*-------------------------------------------------
    skip_this_frame - accessor to determine if this
    frame is being skipped
-------------------------------------------------*/

int skip_this_frame(void)
{
#ifndef NEW_RENDER
	return osd_skip_this_frame();
#else
	return skipping_this_frame;
#endif
}



/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const performance_info *mame_get_performance_info(void)
{
	return &performance;
}




/***************************************************************************

    Screen snapshot and movie recording code

***************************************************************************/

/*-------------------------------------------------
    rotate_snapshot - rotate the snapshot in
    accordance with the orientation
-------------------------------------------------*/

static mame_bitmap *rotate_snapshot(mame_bitmap *bitmap, int orientation, rectangle *bounds)
{
	rectangle newbounds;
	mame_bitmap *copy;
	int x, y, w, h, t;

	/* if we can send it in raw, no need to override anything */
	if (orientation == 0)
		return bitmap;

	/* allocate a copy */
	w = (orientation & ORIENTATION_SWAP_XY) ? bitmap->height : bitmap->width;
	h = (orientation & ORIENTATION_SWAP_XY) ? bitmap->width : bitmap->height;
	copy = auto_bitmap_alloc_depth(w, h, bitmap->depth);

	/* populate the copy */
	for (y = bounds->min_y; y <= bounds->max_y; y++)
		for (x = bounds->min_x; x <= bounds->max_x; x++)
		{
			int tx = x, ty = y;

			/* apply the rotation/flipping */
			if ((orientation & ORIENTATION_SWAP_XY))
			{
				t = tx; tx = ty; ty = t;
			}
			if (orientation & ORIENTATION_FLIP_X)
				tx = copy->width - tx - 1;
			if (orientation & ORIENTATION_FLIP_Y)
				ty = copy->height - ty - 1;

			/* read the old pixel and copy to the new location */
			switch (copy->depth)
			{
				case 15:
				case 16:
					*((UINT16 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT16 *)bitmap->base + y * bitmap->rowpixels + x);
					break;

				case 32:
					*((UINT32 *)copy->base + ty * copy->rowpixels + tx) =
							*((UINT32 *)bitmap->base + y * bitmap->rowpixels + x);
					break;
			}
		}

	/* compute the oriented bounds */
	newbounds = *bounds;

	/* apply X/Y swap first */
	if (orientation & ORIENTATION_SWAP_XY)
	{
		t = newbounds.min_x; newbounds.min_x = newbounds.min_y; newbounds.min_y = t;
		t = newbounds.max_x; newbounds.max_x = newbounds.max_y; newbounds.max_y = t;
	}

	/* apply X flip */
	if (orientation & ORIENTATION_FLIP_X)
	{
		t = copy->width - newbounds.min_x - 1;
		newbounds.min_x = copy->width - newbounds.max_x - 1;
		newbounds.max_x = t;
	}

	/* apply Y flip */
	if (orientation & ORIENTATION_FLIP_Y)
	{
		t = copy->height - newbounds.min_y - 1;
		newbounds.min_y = copy->height - newbounds.max_y - 1;
		newbounds.max_y = t;
	}

	*bounds = newbounds;
	return copy;
}



/*-------------------------------------------------
    save_frame_with - save a frame with a
    given handler for screenshots and movies
-------------------------------------------------*/

static void save_frame_with(mame_file *fp, int scrnum, int (*write_handler)(mame_file *, mame_bitmap *))
{
	mame_bitmap *bitmap;
	int orientation;
	rectangle bounds;
#ifndef NEW_RENDER
	UINT32 saved_rgb_components[3];
#endif

	assert((scrnum >= 0) && (scrnum < MAX_SCREENS));

	bitmap = scrbitmap[scrnum][curbitmap[scrnum]];
	assert(bitmap != NULL);

#ifdef NEW_RENDER
	orientation = render_container_get_orientation(render_container_get_screen(scrnum));
#else
	orientation = Machine->gamedrv->flags & ORIENTATION_MASK;
#endif

	begin_resource_tracking();

	/* allow the artwork system to override certain parameters */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		bounds.min_x = 0;
		bounds.max_x = bitmap->width - 1;
		bounds.min_y = 0;
		bounds.max_y = bitmap->height - 1;
	}
	else
	{
		bounds = Machine->visible_area[0];
	}
#ifndef NEW_RENDER
	memcpy(saved_rgb_components, direct_rgb_components, sizeof(direct_rgb_components));
	artwork_override_screenshot_params(&bitmap, &bounds, direct_rgb_components);
#endif

	/* rotate the snapshot, if necessary */
	bitmap = rotate_snapshot(bitmap, orientation, &bounds);

	/* now do the actual work */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		write_handler(fp, bitmap);
	}
	else
	{
		mame_bitmap *copy;
		int sizex, sizey;

		sizex = bounds.max_x - bounds.min_x + 1;
		sizey = bounds.max_y - bounds.min_y + 1;

		copy = bitmap_alloc_depth(sizex,sizey,bitmap->depth);
		if (copy)
		{
			int x,y,sx,sy;

			sx = bounds.min_x;
			sy = bounds.min_y;

			switch (bitmap->depth)
			{
			case 8:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT8 *)copy->line[y])[x] = ((UINT8 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			case 15:
			case 16:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT16 *)copy->line[y])[x] = ((UINT16 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			case 32:
				for (y = 0;y < copy->height;y++)
				{
					for (x = 0;x < copy->width;x++)
					{
						((UINT32 *)copy->line[y])[x] = ((UINT32 *)bitmap->line[sy+y])[sx+x];
					}
				}
				break;
			default:
				logerror("Unknown color depth\n");
				break;
			}
			write_handler(fp, copy);
			bitmap_free(copy);
		}
	}
#ifndef NEW_RENDER
	memcpy(direct_rgb_components, saved_rgb_components, sizeof(saved_rgb_components));
#endif

	end_resource_tracking();
}


/*-------------------------------------------------
    snapshot_save_screen_indexed - save a snapshot to
    the given file handle
-------------------------------------------------*/

void snapshot_save_screen_indexed(mame_file *fp, int scrnum)
{
	save_frame_with(fp, scrnum, png_write_bitmap);
}


/*-------------------------------------------------
    open the next non-existing file of type
    filetype according to our numbering scheme
-------------------------------------------------*/

static mame_file *mame_fopen_next(int filetype)
{
	char name[FILENAME_MAX];
	int seq;

	/* avoid overwriting existing files */
	/* first of all try with "gamename.xxx" */
	sprintf(name,"%.8s", Machine->gamedrv->name);
	if (mame_faccess(name, filetype))
	{
		seq = 0;
		do
		{
			/* otherwise use "nameNNNN.xxx" */
			sprintf(name,"%.4s%04d",Machine->gamedrv->name, seq++);
		} while (mame_faccess(name, filetype));
	}

    return (mame_fopen(Machine->gamedrv->name, name, filetype, 1));
}


/*-------------------------------------------------
    snapshot_save_all_screens - save a snapshot.
-------------------------------------------------*/

void snapshot_save_all_screens(void)
{
#ifdef NEW_RENDER
	UINT32 screenmask = render_get_live_screens_mask();
#else
	UINT32 screenmask = 1;
#endif
	mame_file *fp;
	int scrnum;

	/* write one snapshot per visible screen */
	for (scrnum = 0; scrnum < MAX_SCREENS; scrnum++)
		if (screenmask & (1 << scrnum))
			if ((fp = mame_fopen_next(FILETYPE_SCREENSHOT)) != NULL)
			{
				snapshot_save_screen_indexed(fp, scrnum);
				mame_fclose(fp);
			}
}


/*-------------------------------------------------
    record_movie - start, stop and update the
    recording of a MNG movie
-------------------------------------------------*/

void record_movie_start(const char *name)
{
	if (movie_file != NULL)
		mame_fclose(movie_file);

	if (name)
		movie_file = mame_fopen(Machine->gamedrv->name, name, FILETYPE_MOVIE, 1);
	else
		movie_file = mame_fopen_next(FILETYPE_MOVIE);

	movie_frame = 0;
}


void record_movie_stop(void)
{
	if (movie_file)
	{
		mng_capture_stop(movie_file);
		mame_fclose(movie_file);
		movie_file = NULL;
	}
}


void record_movie_toggle(void)
{
	if (movie_file == NULL)
	{
		record_movie_start(NULL);
		if (movie_file)
			ui_popup("REC START");
	}
	else
	{
		record_movie_stop();
		ui_popup("REC STOP (%d frames)", movie_frame);
	}
}


void record_movie_frame(int scrnum)
{
	if (movie_file != NULL)
	{
		profiler_mark(PROFILER_MOVIE_REC);

		if (movie_frame++ == 0)
			save_frame_with(movie_file, scrnum, mng_capture_start);
		save_frame_with(movie_file, scrnum, mng_capture_frame);

		profiler_mark(PROFILER_END);
	}
}



/***************************************************************************

    Bitmap allocation/freeing code

***************************************************************************/

/*-------------------------------------------------
    pp_* -- pixel plotting callbacks
-------------------------------------------------*/

static void pp_8 (mame_bitmap *b, int x, int y, pen_t p)  { ((UINT8 *)b->line[y])[x] = p; }
static void pp_16(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT16 *)b->line[y])[x] = p; }
static void pp_32(mame_bitmap *b, int x, int y, pen_t p)  { ((UINT32 *)b->line[y])[x] = p; }


/*-------------------------------------------------
    rp_* -- pixel reading callbacks
-------------------------------------------------*/

static pen_t rp_8 (mame_bitmap *b, int x, int y)  { return ((UINT8 *)b->line[y])[x]; }
static pen_t rp_16(mame_bitmap *b, int x, int y)  { return ((UINT16 *)b->line[y])[x]; }
static pen_t rp_32(mame_bitmap *b, int x, int y)  { return ((UINT32 *)b->line[y])[x]; }


/*-------------------------------------------------
    pb_* -- box plotting callbacks
-------------------------------------------------*/

static void pb_8 (mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT8 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_16(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT16 *)b->line[y])[x] = p; x++; } y++; } }
static void pb_32(mame_bitmap *b, int x, int y, int w, int h, pen_t p)  { int t=x; while(h-->0){ int c=w; x=t; while(c-->0){ ((UINT32 *)b->line[y])[x] = p; x++; } y++; } }


/*-------------------------------------------------
    bitmap_alloc_core
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_core(int width,int height,int depth,int use_auto)
{
	mame_bitmap *bitmap;

	/* obsolete kludge: pass in negative depth to prevent orientation swapping */
	if (depth < 0)
		depth = -depth;

	/* verify it's a depth we can handle */
	if (depth != 8 && depth != 15 && depth != 16 && depth != 32)
	{
		logerror("osd_alloc_bitmap() unknown depth %d\n",depth);
		return NULL;
	}

	/* allocate memory for the bitmap struct */
	bitmap = use_auto ? auto_malloc(sizeof(mame_bitmap)) : malloc(sizeof(mame_bitmap));
	if (bitmap != NULL)
	{
		int i, rowlen, rdwidth, bitmapsize, linearraysize, pixelsize;
		UINT8 *bm;

		/* initialize the basic parameters */
		bitmap->depth = depth;
		bitmap->width = width;
		bitmap->height = height;

		/* determine pixel size in bytes */
		pixelsize = 1;
		if (depth == 15 || depth == 16)
			pixelsize = 2;
		else if (depth == 32)
			pixelsize = 4;

		/* round the width to a multiple of 8 */
		rdwidth = (width + 7) & ~7;
		rowlen = rdwidth + 2 * BITMAP_SAFETY;
		bitmap->rowpixels = rowlen;

		/* now convert from pixels to bytes */
		rowlen *= pixelsize;
		bitmap->rowbytes = rowlen;

		/* determine total memory for bitmap and line arrays */
		bitmapsize = (height + 2 * BITMAP_SAFETY) * rowlen;
		linearraysize = (height + 2 * BITMAP_SAFETY) * sizeof(UINT8 *);

		/* align to 16 bytes */
		linearraysize = (linearraysize + 15) & ~15;

		/* allocate the bitmap data plus an array of line pointers */
		bitmap->line = use_auto ? auto_malloc(linearraysize + bitmapsize) : malloc(linearraysize + bitmapsize);
		if (bitmap->line == NULL)
		{
			if (!use_auto) free(bitmap);
			return NULL;
		}

		/* clear ALL bitmap, including safety area, to avoid garbage on right */
		bm = (UINT8 *)bitmap->line + linearraysize;
		memset(bm, 0, (height + 2 * BITMAP_SAFETY) * rowlen);

		/* initialize the line pointers */
		for (i = 0; i < height + 2 * BITMAP_SAFETY; i++)
			bitmap->line[i] = &bm[i * rowlen + BITMAP_SAFETY * pixelsize];

		/* adjust for the safety rows */
		bitmap->line += BITMAP_SAFETY;
		bitmap->base = bitmap->line[0];

		/* set the pixel functions */
		if (pixelsize == 1)
		{
			bitmap->read = rp_8;
			bitmap->plot = pp_8;
			bitmap->plot_box = pb_8;
		}
		else if (pixelsize == 2)
		{
			bitmap->read = rp_16;
			bitmap->plot = pp_16;
			bitmap->plot_box = pb_16;
		}
		else
		{
			bitmap->read = rp_32;
			bitmap->plot = pp_32;
			bitmap->plot_box = pb_32;
		}
	}

	/* return the result */
	return bitmap;
}


/*-------------------------------------------------
    bitmap_alloc_depth - allocate a bitmap for a
    specific depth
-------------------------------------------------*/

mame_bitmap *bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, FALSE);
}


/*-------------------------------------------------
    auto_bitmap_alloc_depth - allocate a bitmap
    for a specific depth
-------------------------------------------------*/

mame_bitmap *auto_bitmap_alloc_depth(int width, int height, int depth)
{
	return bitmap_alloc_core(width, height, depth, TRUE);
}


/*-------------------------------------------------
    bitmap_free - free a bitmap
-------------------------------------------------*/

void bitmap_free(mame_bitmap *bitmap)
{
	/* skip if NULL */
	if (!bitmap)
		return;

	/* unadjust for the safety rows */
	bitmap->line -= BITMAP_SAFETY;

	/* free the memory */
	free(bitmap->line);
	free(bitmap);
}



