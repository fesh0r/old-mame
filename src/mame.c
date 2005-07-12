/***************************************************************************

    mame.c

    Controls execution of the core MAME system.

****************************************************************************

    Since there has been confusion in the past over the order of
    initialization and other such things, here it is, all spelled out
    as of March, 2005:

    main()
        - does platform-specific init
        - calls run_game() [mame.c]

        run_game() [mame.c]
            - begins resource tracking (level 1)
            - calls mame_validitychecks() [mame.c] to perform validity checks on all compiled drivers
            - calls expand_machine_driver() [mame.c] to construct the machine driver
            - calls cpuintrf_init() [cpuintrf.c] to determine which CPUs are available
            - calls init_game_options() [mame.c] to compute parameters based on global options struct
            - initializes the savegame system
            - calls osd_init() [osdepend.h] to do platform-specific initialization

            - begins resource tracking (level 2)
            - calls init_machine() [mame.c]

            init_machine() [mame.c]
                - calls uistring_init() [ui_text.c] to initialize the localized strings
                - calls code_init() [input.c] to initialize the input system
                - calls input_port_alloc() [inptport.c] to construct the game's input ports
                - calls chd_set_interface() [chd.c] to initialize the hard disk system
                - calls rom_load() [common.c] to load the game's ROMs
                - calls timer_init() [timer.c] to reset the timer system
                - calls cpu_init_refresh_timer() [cpuexec.c] to start the refresh timer
                - calls cpu_init() [cpuexec.c] to initialize the CPUs
                - calls load_input_port_settings() [input.c] to load the configuration file
                - calls memory_init() [memory.c] to process the game's memory maps
                - calls the driver's DRIVER_INIT callback

            - calls run_machine() [mame.c]

            run_machine() [mame.c]
                - calls vh_open() [mame.c]

                vh_open() [mame.c]
                    - calls palette_start() [palette.c] to allocate the palette
                    - calls decode_graphics() [mame.c] to decode the graphics
                    - computes game resolution and aspect ratio
                    - calls artwork_create_display() [artwork.c] to set up the artwork and init the display
                    - allocates the scrbitmap
                    - sets the initial refresh rate and visible area
                    - calls init_buffered_spriteram() [mame.c] to set up buffered spriteram
                    - calls builduifont() [usrintrf.c] to create the user interface font
                    - creates the debugger bitmap and font (old debugger only)
                    - calls palette_init() [palette.c] to finish palette initialization
                    - resets the performance tracking variables

                - calls tilemap_init() [tilemap.c] to initialize the tilemap system
                - calls the driver's VIDEO_START callback
                - calls sound_start() [sndintrf.c] to start the audio system
                - disposes of regions marked as disposable
                - calls run_machine_core() [mame.c]

                run_machine_core() [mame.c]
                    - shows the copyright screen
                    - shows the game warnings
                    - shows the game info screen
                    - calls init_user_interface() [usrintrf.c] to initialize the user interface
                    - calls InitCheat() [cheat.c] to initialize the cheat system
                    - calls the driver's NVRAM_HANDLER to load NVRAM
                    - calls cpu_run() [cpuexec.c]

                    cpu_run() [cpuexec.c]
                        - calls mame_debug_init() [mamedbg.c] to init the debugger (old debugger only)
                        - calls cpu_pre_run() [cpuexec.c]

                        cpu_pre_run() [cpuexec.c]
                            - begins resource tracking (level 3)
                            - calls hs_open() and hs_init() [hiscore.c] to set up high score hacks
                            - calls cpu_inittimers() [cpuexec.c] to set up basic system timers
                            - calls watchdog_setup() [cpuexec.c] to set up watchdog timers
                            - calls sound_reset() [sndintrf.c] to reset all the sound chips
                            - loops over each CPU and initializes its internal state
                            - calls the driver's MACHINE_INIT callback
                            - loops over each CPU and calls cpunum_reset() [cpuintrf.c] to reset it
                            - calls cpu_vblankreset() [cpuexec.c] to set up the first VBLANK callback

                        --------------( at this point, we're up and running )---------------------------

                        - calls cpu_post_run() [cpuexec.c]

                        cpu_post_run() [cpuexec.c]
                            - calls hs_close() [hiscore.c] to flush high score data
                            - calls the driver's MACHINE_STOP callback
                            - ends resource tracking (level 3), freeing all auto_mallocs and timers

                        - if the machine is just being reset, loops back to the cpu_pre_run() step above
                        - calls mame_debug_exit() [mamedbg.c] to shut down the debugger (old debugger only)

                    - calls the driver's NVRAM_HANDLER to save NVRAM
                    - calls StopCheat() [cheat.c] to tear down the cheat system
                    - calls save_input_port_settings() [inptport.c] to save the game's configuration

                - calls sound_stop() [sndintrf.c] to stop the audio system
                - calls the driver's VIDEO_STOP callback
                - calls tilemap_close() [tilemap.c] to tear down the tilemap system
                - calls vh_close() [mame.c]

                vh_close() [mame.c]
                    - frees the decoded graphics
                    - frees the fonts
                    - calls osd_close_display() [osdepend.h] to shut down the display

            - calls shutdown_machine() [mame.c]

            shutdown_machine() [mame.c]
                - calls cpu_exit() [cpuexec.c] to tear down the CPU system
                - calls memory_shutdown() [memory.c] to tear down the memory system
                - frees all the memory regions
                - calls chd_close_all() [chd.c] to tear down the hard disks
                - calls code_close() [input.c] to tear down the input system
                - calls state_save_reset() [state.c] to reset the saved state system
                - calls coin_counter_reset() [common.c] to reset coin counters

            - ends resource tracking (level 2), freeing all auto_mallocs and timers
            - calls osd_exit() [osdepend.h] to do platform-specific cleanup
            - ends resource tracking (level 1), freeing all auto_mallocs and timers

        - exits the program

***************************************************************************/

#include "driver.h"
#include <ctype.h>
#include <stdarg.h>
#include "ui_text.h"
#include "mamedbg.h"
#include "artwork.h"
#include "state.h"
#include "unzip.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/vector.h"
#include "palette.h"
#include "harddisk.h"


/***************************************************************************

    Constants

***************************************************************************/

#define FRAMES_PER_FPS_UPDATE		12



/***************************************************************************

    Global variables

***************************************************************************/

/* handy globals for other parts of the system */
void *record;	/* for -record */
void *playback; /* for -playback */
int mame_debug; /* !0 when -debug option is specified */
int bailing;	/* set to 1 if the startup is aborted to prevent multiple error messages */

/* the active machine */
static struct RunningMachine active_machine;
struct RunningMachine *Machine = &active_machine;

/* the active game driver */
static const struct GameDriver *gamedrv;
static struct InternalMachineDriver internal_drv;

/* various game options filled in by the OSD */
struct GameOptions options;

/* the active video display */
static struct mame_display current_display;
static UINT8 visible_area_changed;
static UINT8 refresh_rate_changed;

/* video updating */
static UINT8 full_refresh_pending;
static int last_partial_scanline;

/* speed computation */
static cycles_t last_fps_time;
static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int vfcount;
static struct performance_info performance;

/* misc other statics */
static int settingsloaded;
static int leds_status;

/* artwork callbacks */
#ifndef MESS
static struct artwork_callbacks mame_artwork_callbacks =
{
	NULL,
	artwork_load_artwork_file
};
#endif

#ifdef MESS
int skip_this_frame;
#endif



/***************************************************************************

    Hard disk interface prototype

***************************************************************************/

static struct chd_interface_file *mame_chd_open(const char *filename, const char *mode);
static void mame_chd_close(struct chd_interface_file *file);
static UINT32 mame_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer);
static UINT32 mame_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer);
static UINT64 mame_chd_length(struct chd_interface_file *file);

static struct chd_interface mame_chd_interface =
{
	mame_chd_open,
	mame_chd_close,
	mame_chd_read,
	mame_chd_write,
	mame_chd_length
};



/***************************************************************************

    Other function prototypes

***************************************************************************/

static int init_machine(void);
static void shutdown_machine(void);
static int run_machine(void);
static void run_machine_core(void);

static void recompute_fps(int skipped_it);
static int vh_open(void);
static void vh_close(void);
static int init_game_options(void);
static int decode_graphics(const struct GfxDecodeInfo *gfxdecodeinfo);
static void compute_aspect_ratio(const struct InternalMachineDriver *drv, int *aspect_x, int *aspect_y);
static void scale_vectorgames(int gfx_width, int gfx_height, int *width, int *height);
static int init_buffered_spriteram(void);

#ifdef MESS
#include "mesintrf.h"
#define handle_user_interface	handle_mess_user_interface
#endif


/***************************************************************************

    Inline functions

***************************************************************************/

/*-------------------------------------------------
    bail_and_print - set the bailing flag and
    print a message if one hasn't already been
    printed
-------------------------------------------------*/

INLINE void bail_and_print(const char *message)
{
	if (!bailing)
	{
		bailing = 1;
		printf("%s\n", message);
	}
}




/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    run_game - run the given game in a session
-------------------------------------------------*/

int run_game(int game)
{
	int err = 1;

	begin_resource_tracking();

	/* validity checks -- the default is to perform these in all builds now
     * due to the number of incorrect submissions */
	if (!options.skip_validitychecks)
		if (mame_validitychecks())
			return 1;

	/* first give the machine a good cleaning */
	memset(Machine, 0, sizeof(Machine));

	/* initialize the driver-related variables in the Machine */
	Machine->gamedrv = gamedrv = drivers[game];
	expand_machine_driver(gamedrv->drv, &internal_drv);
	Machine->drv = &internal_drv;
	Machine->refresh_rate = Machine->drv->frames_per_second;

	/* initialize the CPU interfaces first */
	if (cpuintrf_init())
		return 1;

	/* initialize the game options */
	if (init_game_options())
		return 1;

	/* if we're coming in with a savegame request, process it now */
	if (options.savegame)
	{
		if (strlen(options.savegame) == 1)
			cpu_loadsave_schedule(LOADSAVE_LOAD, options.savegame[0]);
		else
			cpu_loadsave_schedule_file(LOADSAVE_LOAD, options.savegame);
	}
	else
		cpu_loadsave_reset();

	/* here's the meat of it all */
	bailing = 0;

	/* let the OSD layer start up first */
	if (osd_init())
		bail_and_print("Unable to initialize system");
	else
	{
		begin_resource_tracking();

		/* then finish setting up our local machine */
		if (init_machine())
			bail_and_print("Unable to initialize machine emulation");
		else
		{
			/* then run it */
			if (run_machine())
				bail_and_print("Unable to start machine emulation");
			else
				err = 0;

			/* shutdown the local machine */
			shutdown_machine();
		}

		/* clear the zip cache (I don't know what would be the bestplace to do this) */
		unzip_cache_clear();

		/* stop tracking resources and exit the OSD layer */
		end_resource_tracking();
		osd_exit();
	}

	end_resource_tracking();
	return err;
}



/*-------------------------------------------------
    init_machine - initialize the emulated machine
-------------------------------------------------*/

static int init_machine(void)
{
	/* load the localization file */
	if (uistring_init(options.language_file) != 0)
	{
		logerror("uistring_init failed\n");
		goto cant_load_language_file;
	}

	/* initialize the input system */
	if (code_init() != 0)
	{
		logerror("code_init failed\n");
		goto cant_init_input;
	}

	/* if we have inputs, process them now */
	if (gamedrv->construct_ipt)
	{
		/* allocate input ports */
		Machine->input_ports = input_port_allocate(gamedrv->construct_ipt);
		if (!Machine->input_ports)
		{
			logerror("could not allocate Machine->input_ports\n");
			goto cant_allocate_input_ports;
		}

		/* allocate default input ports */
		Machine->input_ports_default = input_port_allocate(gamedrv->construct_ipt);
		if (!Machine->input_ports_default)
		{
			logerror("could not allocate Machine->input_ports_default\n");
			goto cant_allocate_input_ports_default;
		}
	}

	/* init the hard drive interface now, before attempting to load */
	chd_set_interface(&mame_chd_interface);

	/* load the ROMs if we have some */
	if (gamedrv->rom && rom_load(gamedrv->rom) != 0)
	{
		logerror("readroms failed\n");
		goto cant_load_roms;
	}

	/* first init the timers; some CPUs have built-in timers and will need */
	/* to allocate them up front */
	timer_init();
	cpu_init_refresh_timer();

	/* now set up all the CPUs */
	cpu_init();

#ifdef MESS
	/* initialize the devices */
	if (devices_init(gamedrv) || devices_initialload(gamedrv, TRUE))
	{
		logerror("devices_init failed\n");
		goto cant_load_roms;
	}
#endif

	/* load input ports settings (keys, dip switches, and so on) */
	settingsloaded = load_input_port_settings();

	/* multi-session safety - set spriteram size to zero before memory map is set up */
	spriteram_size = spriteram_2_size = 0;

	/* initialize the memory system for this game */
	if (!memory_init())
	{
		logerror("memory_init failed\n");
		goto cant_init_memory;
	}

	/* clear out the memcard interface */
	init_memcard();

	/* call the game driver's init function */
	if (gamedrv->driver_init)
		(*gamedrv->driver_init)();

#ifdef MESS
	/* initialize the devices */
	if (devices_initialload(gamedrv, FALSE))
	{
		logerror("devices_initialload failed\n");
		goto cant_load_roms;
	}
#endif

	return 0;

cant_init_memory:
cant_load_roms:
	Machine->input_ports_default = 0;
cant_allocate_input_ports_default:
	Machine->input_ports = 0;
cant_allocate_input_ports:
	code_close();
cant_init_input:
cant_load_language_file:
	return 1;
}



/*-------------------------------------------------
    run_machine - start the various subsystems
    and the CPU emulation; returns non zero in
    case of error
-------------------------------------------------*/

static int run_machine(void)
{
	int res = 1;

	/* start the video hardware */
	if (vh_open())
		bail_and_print("Unable to start video emulation");
	else
	{
		/* initialize tilemaps */
		tilemap_init();
		flip_screen_set(0);

		/* start up the driver's video */
		if (Machine->drv->video_start && (*Machine->drv->video_start)())
			bail_and_print("Unable to start video emulation");
		else
		{
			/* start the audio system */
			if (sound_start())
				bail_and_print("Unable to start audio emulation");
			else
			{
				int region;

				/* free memory regions allocated with REGIONFLAG_DISPOSE (typically gfx roms) */
				for (region = 0; region < MAX_MEMORY_REGIONS; region++)
					if (Machine->memory_region[region].flags & ROMREGION_DISPOSE)
					{
						int i;

						/* invalidate contents to avoid subtle bugs */
						for (i = 0; i < memory_region_length(region); i++)
							memory_region(region)[i] = rand();
						free(Machine->memory_region[region].base);
						Machine->memory_region[region].base = 0;
					}

				/* now do the core execution */
				run_machine_core();
				res = 0;

				/* store the sound system */
				sound_stop();
			}

			/* shut down the driver's video and kill and artwork */
			if (Machine->drv->video_stop)
				(*Machine->drv->video_stop)();
		}

		/* close down the tilemap and video systems */
		tilemap_close();
		vh_close();
	}

	return res;
}



/*-------------------------------------------------
    run_machine_core - core execution loop
-------------------------------------------------*/

void run_machine_core(void)
{
	/* disable artwork for the start */
	artwork_enable(0);

	/* if we didn't find a settings file, show the disclaimer */
	if (settingsloaded || options.skip_disclaimer || showcopyright(artwork_get_ui_bitmap()) == 0)
	{
		/* show info about incorrect behaviour (wrong colors etc.) */
		if (options.skip_warnings || showgamewarnings(artwork_get_ui_bitmap()) == 0)
		{
			/* show info about the game */
			if (options.skip_gameinfo || showgameinfo(artwork_get_ui_bitmap()) == 0)
			{
				init_user_interface();

				/* enable artwork now */
				artwork_enable(1);

				/* disable cheat if no roms */
				if (!gamedrv->rom)
					options.cheat = 0;

				/* start the cheat engine */
				if (options.cheat)
					InitCheat();

				/* load the NVRAM now */
				if (Machine->drv->nvram_handler)
				{
					mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0);
					(*Machine->drv->nvram_handler)(nvram_file, 0);
					if (nvram_file)
						mame_fclose(nvram_file);
				}

				/* run the emulation! */
				cpu_run();

				/* save the NVRAM */
				if (Machine->drv->nvram_handler)
				{
					mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
					if (nvram_file != NULL)
					{
						(*Machine->drv->nvram_handler)(nvram_file, 1);
						mame_fclose(nvram_file);
					}
				}

				/* stop the cheat engine */
				if (options.cheat)
					StopCheat();

				/* save input ports settings */
				save_input_port_settings();
			}
		}
	}
}



/*-------------------------------------------------
    shutdown_machine - tear down the emulated
    machine
-------------------------------------------------*/

static void shutdown_machine(void)
{
	int i;

#ifdef MESS
	/* close down any devices */
	devices_exit();
#endif

	/* reset the CPU system -- must be done before memory goes away */
	cpu_exit();

	/* release any allocated memory */
	memory_shutdown();

	/* free the memory allocated for various regions */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		free_memory_region(i);

	/* close all hard drives */
	chd_close_all();

	/* close down the input system */
	code_close();

	/* reset the saved states */
	state_save_reset();

	/* reset coin counters */
	coin_counter_reset();
}



/*-------------------------------------------------
    mame_pause - pause or resume the system
-------------------------------------------------*/

void mame_pause(int pause)
{
	osd_pause(pause);
	osd_sound_enable(!pause);
	palette_set_global_brightness_adjust(pause ? options.pause_bright : 1.00);
	schedule_full_refresh();
}



/*-------------------------------------------------
    expand_machine_driver - construct a machine
    driver from the macroized state
-------------------------------------------------*/

void expand_machine_driver(void (*constructor)(struct InternalMachineDriver *), struct InternalMachineDriver *output)
{
	/* keeping this function allows us to pre-init the driver before constructing it */
	memset(output, 0, sizeof(*output));
	(*constructor)(output);
}



/*-------------------------------------------------
    vh_open - start up the video system
-------------------------------------------------*/

static int vh_open(void)
{
	struct osd_create_params params;
	struct artwork_callbacks *artcallbacks;
	int bmwidth = Machine->drv->screen_width;
	int bmheight = Machine->drv->screen_height;

	/* first allocate the necessary palette structures */
	if (palette_start())
		goto cant_start_palette;

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* palette_init() routine because it might need to check the Machine->gfx[] data */
	if (Machine->drv->gfxdecodeinfo)
		if (decode_graphics(Machine->drv->gfxdecodeinfo))
			goto cant_decode_graphics;

	/* if we're a vector game, override the screen width and height */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* compute the visible area for raster games */
	if (!(Machine->drv->video_attributes & VIDEO_TYPE_VECTOR))
	{
		params.width = Machine->drv->default_visible_area.max_x - Machine->drv->default_visible_area.min_x + 1;
		params.height = Machine->drv->default_visible_area.max_y - Machine->drv->default_visible_area.min_y + 1;
	}
	else
	{
		params.width = bmwidth;
		params.height = bmheight;
	}

	/* fill in the rest of the display parameters */
	compute_aspect_ratio(Machine->drv, &params.aspect_x, &params.aspect_y);
	params.depth = Machine->color_depth;
	params.colors = palette_get_total_colors_with_ui();
	params.fps = Machine->drv->frames_per_second;
	params.video_attributes = Machine->drv->video_attributes;

#ifdef MESS
	artcallbacks = &mess_artwork_callbacks;
#else
	artcallbacks = &mame_artwork_callbacks;
#endif

	/* initialize the display through the artwork (and eventually the OSD) layer */
	if (artwork_create_display(&params, direct_rgb_components, artcallbacks))
		goto cant_create_display;

	/* the create display process may update the vector width/height, so recompute */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		scale_vectorgames(options.vector_width, options.vector_height, &bmwidth, &bmheight);

	/* now allocate the screen bitmap */
	Machine->scrbitmap = auto_bitmap_alloc_depth(bmwidth, bmheight, Machine->color_depth);
	if (!Machine->scrbitmap)
		goto cant_create_scrbitmap;

	/* set the default refresh rate */
	set_refresh_rate(Machine->drv->frames_per_second);

	/* set the default visible area */
	set_visible_area(0,1,0,1);	// make sure everything is recalculated on multiple runs
	set_visible_area(
			Machine->drv->default_visible_area.min_x,
			Machine->drv->default_visible_area.max_x,
			Machine->drv->default_visible_area.min_y,
			Machine->drv->default_visible_area.max_y);

	/* create spriteram buffers if necessary */
	if (Machine->drv->video_attributes & VIDEO_BUFFERS_SPRITERAM)
		if (init_buffered_spriteram())
			goto cant_init_buffered_spriteram;

	/* build our private user interface font */
	/* This must be done AFTER osd_create_display() so the function knows the */
	/* resolution we are running at and can pick a different font depending on it. */
	/* It must be done BEFORE palette_init() because that will also initialize */
	/* (through osd_allocate_colors()) the uifont colortable. */
	Machine->uifont = builduifont();
	if (Machine->uifont == NULL)
		goto cant_build_uifont;

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	/* if the debugger is enabled, initialize its bitmap and font */
	if (mame_debug)
	{
		int depth = options.debug_depth ? options.debug_depth : Machine->color_depth;

		/* first allocate the debugger bitmap */
		Machine->debug_bitmap = auto_bitmap_alloc_depth(options.debug_width, options.debug_height, depth);
		if (!Machine->debug_bitmap)
			goto cant_create_debug_bitmap;

		/* then create the debugger font */
		Machine->debugger_font = build_debugger_font();
		if (Machine->debugger_font == NULL)
			goto cant_build_debugger_font;
	}
#endif

	/* initialize the palette - must be done after osd_create_display() */
	if (palette_init())
		goto cant_init_palette;

	/* force the first update to be full */
	set_vh_global_attribute(NULL, 0);

	/* reset performance data */
	last_fps_time = osd_cycles();
	rendered_frames_since_last_fps = frames_since_last_fps = 0;
	performance.game_speed_percent = 100;
	performance.frames_per_second = Machine->refresh_rate;
	performance.vector_updates_last_second = 0;

	/* reset video statics and get out of here */
	pdrawgfx_shadow_lowpri = 0;
	leds_status = 0;

	return 0;

cant_init_palette:

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
cant_build_debugger_font:
cant_create_debug_bitmap:
#endif

cant_build_uifont:
cant_init_buffered_spriteram:
cant_create_scrbitmap:
cant_create_display:
cant_decode_graphics:
cant_start_palette:
	vh_close();
	return 1;
}



/*-------------------------------------------------
    vh_close - close down the video system
-------------------------------------------------*/

static void vh_close(void)
{
	int i;

	/* free all the graphics elements */
	for (i = 0; i < MAX_GFX_ELEMENTS; i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}

	/* free the font elements */
	if (Machine->uifont)
	{
		freegfx(Machine->uifont);
		Machine->uifont = NULL;
	}
	if (Machine->uirotfont)
	{
		freegfx(Machine->uirotfont);
		Machine->uirotfont = NULL;
	}
	if (Machine->debugger_font)
	{
		freegfx(Machine->debugger_font);
		Machine->debugger_font = NULL;
	}

	/* close down the OSD layer's display */
	osd_close_display();
}



/*-------------------------------------------------
    compute_aspect_ratio - determine the aspect
    ratio encoded in the video attributes
-------------------------------------------------*/

static void compute_aspect_ratio(const struct InternalMachineDriver *drv, int *aspect_x, int *aspect_y)
{
	/* if it's explicitly specified, use it */
	if (drv->aspect_x && drv->aspect_y)
	{
		*aspect_x = drv->aspect_x;
		*aspect_y = drv->aspect_y;
	}

	/* otherwise, attempt to deduce the result */
	else if (!(drv->video_attributes & VIDEO_DUAL_MONITOR))
	{
		*aspect_x = 4;
		*aspect_y = (drv->video_attributes & VIDEO_DUAL_MONITOR) ? 6 : 3;
	}
}



/*-------------------------------------------------
    init_game_options - initialize the various
    game options
-------------------------------------------------*/

static int init_game_options(void)
{
	/* copy some settings into easier-to-handle variables */
	record	   = options.record;
	playback   = options.playback;
	mame_debug = options.mame_debug;

	/* determine the color depth */
	Machine->color_depth = 16;
	alpha_active = 0;
	if (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
	{
		/* pick a default */
		if (Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN)
			Machine->color_depth = 32;
		else
			Machine->color_depth = 15;

		/* enable alpha for direct video modes */
		alpha_active = 1;
		alpha_init();
	}

	/* update the vector width/height with defaults */
	if (options.vector_width == 0) options.vector_width = 640;
	if (options.vector_height == 0) options.vector_height = 480;

	/* initialize the samplerate */
	Machine->sample_rate = options.samplerate;

	/* get orientation right */
	Machine->ui_orientation = options.ui_orientation;

	return 0;
}



/*-------------------------------------------------
    decode_graphics - decode the graphics
-------------------------------------------------*/

static int decode_graphics(const struct GfxDecodeInfo *gfxdecodeinfo)
{
	int i;

	/* loop over all elements */
	for (i = 0; i < MAX_GFX_ELEMENTS && gfxdecodeinfo[i].memory_region != -1; i++)
	{
		int region_length = 8 * memory_region_length(gfxdecodeinfo[i].memory_region);
		UINT8 *region_base = memory_region(gfxdecodeinfo[i].memory_region);
		struct GfxLayout glcopy;
		int j;

		/* make a copy of the layout */
		glcopy = *gfxdecodeinfo[i].gfxlayout;

		/* if the character count is a region fraction, compute the effective total */
		if (IS_FRAC(glcopy.total))
			glcopy.total = region_length / glcopy.charincrement * FRAC_NUM(glcopy.total) / FRAC_DEN(glcopy.total);

		/* loop over all the planes, converting fractions */
		for (j = 0; j < MAX_GFX_PLANES; j++)
		{
			int value = glcopy.planeoffset[j];
			if (IS_FRAC(value))
				glcopy.planeoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* loop over all the X/Y offsets, converting fractions */
		for (j = 0; j < glcopy.width; j++)
		{
			int value = glcopy.xoffset[j];
			if (IS_FRAC(value))
				glcopy.xoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}
		for (j = 0; j < glcopy.height; j++)
		{
			int value = glcopy.yoffset[j];
			if (IS_FRAC(value))
				glcopy.yoffset[j] = FRAC_OFFSET(value) + region_length * FRAC_NUM(value) / FRAC_DEN(value);
		}

		/* some games increment on partial tile boundaries; to handle this without reading */
		/* past the end of the region, we may need to truncate the count */
		/* an example is the games in metro.c */
		if (glcopy.planeoffset[0] == GFX_RAW)
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

		/* now decode the actual graphics */
		if ((Machine->gfx[i] = decodegfx(region_base + gfxdecodeinfo[i].start, &glcopy)) == 0)
		{
			bailing = 1;
			printf("Out of memory decoding gfx\n");
			return 1;
		}

		/* if we have a remapped colortable, point our local colortable to it */
		if (Machine->remapped_colortable)
			Machine->gfx[i]->colortable = &Machine->remapped_colortable[gfxdecodeinfo[i].color_codes_start];
		Machine->gfx[i]->total_colors = gfxdecodeinfo[i].total_color_codes;
	}
	return 0;
}



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



/*-------------------------------------------------
    init_buffered_spriteram - initialize the
    double-buffered spriteram
-------------------------------------------------*/

static int init_buffered_spriteram(void)
{
	/* make sure we have a valid size */
	if (spriteram_size == 0)
	{
		logerror("vh_open():  Video buffers spriteram but spriteram_size is 0\n");
		return 0;
	}

	/* allocate memory for the back buffer */
	buffered_spriteram = auto_malloc(spriteram_size);
	if (!buffered_spriteram)
		return 1;

	/* register for saving it */
	state_save_register_UINT8("generic_video", 0, "buffered_spriteram", buffered_spriteram, spriteram_size);

	/* do the same for the secon back buffer, if present */
	if (spriteram_2_size)
	{
		/* allocate memory */
		buffered_spriteram_2 = auto_malloc(spriteram_2_size);
		if (!buffered_spriteram_2)
			return 1;

		/* register for saving it */
		state_save_register_UINT8("generic_video", 0, "buffered_spriteram_2", buffered_spriteram_2, spriteram_2_size);
	}

	/* make 16-bit and 32-bit pointer variants */
	buffered_spriteram16 = (data16_t *)buffered_spriteram;
	buffered_spriteram32 = (data32_t *)buffered_spriteram;
	buffered_spriteram16_2 = (data16_t *)buffered_spriteram_2;
	buffered_spriteram32_2 = (data32_t *)buffered_spriteram_2;
	return 0;
}



/***************************************************************************

    Screen rendering and management.

***************************************************************************/

/*-------------------------------------------------
    set_visible_area - adjusts the visible portion
    of the bitmap area dynamically
-------------------------------------------------*/

void set_visible_area(int min_x, int max_x, int min_y, int max_y)
{
	if (       Machine->visible_area.min_x == min_x
			&& Machine->visible_area.max_x == max_x
			&& Machine->visible_area.min_y == min_y
			&& Machine->visible_area.max_y == max_y)
		return;

	/* "dirty" the area for the next display update */
	visible_area_changed = 1;

	/* set the new values in the Machine struct */
	Machine->visible_area.min_x = min_x;
	Machine->visible_area.max_x = max_x;
	Machine->visible_area.min_y = min_y;
	Machine->visible_area.max_y = max_y;

	/* vector games always use the whole bitmap */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		Machine->absolute_visible_area.min_x = 0;
		Machine->absolute_visible_area.max_x = Machine->scrbitmap->width - 1;
		Machine->absolute_visible_area.min_y = 0;
		Machine->absolute_visible_area.max_y = Machine->scrbitmap->height - 1;
	}

	/* raster games need to use the visible area */
	else
		Machine->absolute_visible_area = Machine->visible_area;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}



/*-------------------------------------------------
    set_refresh_rate - adjusts the refresh rate
    of the video mode dynamically
-------------------------------------------------*/

void set_refresh_rate(float fps)
{
	/* bail if already equal */
	if (Machine->refresh_rate == fps)
		return;

	/* "dirty" the rate for the next display update */
	refresh_rate_changed = 1;

	/* set the new values in the Machine struct */
	Machine->refresh_rate = fps;

	/* recompute scanline timing */
	cpu_compute_scanline_timing();
}



/*-------------------------------------------------
    schedule_full_refresh - force a full erase
    and refresh the next frame
-------------------------------------------------*/

void schedule_full_refresh(void)
{
	full_refresh_pending = 1;
}



/*-------------------------------------------------
    reset_partial_updates - reset the partial
    updating mechanism for a new frame
-------------------------------------------------*/

void reset_partial_updates(void)
{
	last_partial_scanline = 0;
	performance.partial_updates_this_frame = 0;
}



/*-------------------------------------------------
    force_partial_update - perform a partial
    update from the last scanline up to and
    including the specified scanline
-------------------------------------------------*/

void force_partial_update(int scanline)
{
	struct rectangle clip = Machine->visible_area;

	/* if skipping this frame, bail */
	if (osd_skip_this_frame())
		return;

	/* skip if less than the lowest so far */
	if (scanline < last_partial_scanline)
		return;

	/* if there's a dirty bitmap and we didn't do any partial updates yet, handle it now */
	if (full_refresh_pending && last_partial_scanline == 0)
	{
		fillbitmap(Machine->scrbitmap, get_black_pen(), NULL);
		full_refresh_pending = 0;
	}

	/* set the start/end scanlines */
	if (last_partial_scanline > clip.min_y)
		clip.min_y = last_partial_scanline;
	if (scanline < clip.max_y)
		clip.max_y = scanline;

	/* render if necessary */
	if (clip.min_y <= clip.max_y)
	{
		profiler_mark(PROFILER_VIDEO);
#ifdef MESS
		{
			int update_says_skip = 0;
			(*Machine->drv->video_update)(Machine->scrbitmap, &clip, &update_says_skip);
			if (!update_says_skip)
				skip_this_frame = 0;
			else if (skip_this_frame == -1)
				skip_this_frame = 1;
		}
#else
		(*Machine->drv->video_update)(Machine->scrbitmap, &clip);
#endif
		performance.partial_updates_this_frame++;
		profiler_mark(PROFILER_END);
	}

	/* remember where we left off */
	last_partial_scanline = scanline + 1;
}



/*-------------------------------------------------
    draw_screen - render the final screen bitmap
    and update any artwork
-------------------------------------------------*/
int gbPriorityBitmapIsDirty;

void draw_screen(void)
{
	/* finish updating the screen */
	force_partial_update(Machine->visible_area.max_y);
	if( gbPriorityBitmapIsDirty )
	{
		fillbitmap( priority_bitmap, 0x00, NULL );
		gbPriorityBitmapIsDirty = 0;
	}
}


/*-------------------------------------------------
    update_video_and_audio - actually call the
    OSD layer to perform an update
-------------------------------------------------*/

void update_video_and_audio(void)
{
	int skipped_it = osd_skip_this_frame();

#if defined(MAME_DEBUG) && !defined(NEW_DEBUGGER)
	debug_trace_delay = 0;
#endif

	/* fill in our portion of the display */
	current_display.changed_flags = 0;

#ifdef MESS
	if (skip_this_frame == 1)
		current_display.changed_flags |= GAME_OPTIONAL_FRAMESKIP;
	skip_this_frame = -1;
#endif
	
	/* set the main game bitmap */
	current_display.game_bitmap = Machine->scrbitmap;
	current_display.game_bitmap_update = Machine->absolute_visible_area;
	if (!skipped_it)
		current_display.changed_flags |= GAME_BITMAP_CHANGED;

	/* set the visible area */
	current_display.game_visible_area = Machine->absolute_visible_area;
	if (visible_area_changed)
		current_display.changed_flags |= GAME_VISIBLE_AREA_CHANGED;

	/* set the refresh rate */
	current_display.game_refresh_rate = Machine->refresh_rate;
	if (refresh_rate_changed)
		current_display.changed_flags |= GAME_REFRESH_RATE_CHANGED;

	/* set the vector dirty list */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (!full_refresh_pending && !ui_dirty && !skipped_it)
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

	/* update FPS */
	recompute_fps(skipped_it);

	/* reset dirty flags */
	visible_area_changed = 0;
	refresh_rate_changed = 0;
	if (ui_dirty) ui_dirty--;
}



/*-------------------------------------------------
    recompute_fps - recompute the frame rate
-------------------------------------------------*/

int vector_updates = 0;

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
		performance.game_speed_percent = 100.0 * frames_per_sec / Machine->refresh_rate;
		performance.frames_per_second = (double)rendered_frames_since_last_fps / seconds_elapsed;

		/* reset the info */
		last_fps_time = curr;
		frames_since_last_fps = 0;
		rendered_frames_since_last_fps = 0;
	}

	/* for vector games, compute the vector update count once/second */
	vfcount++;
	if (vfcount >= (int)Machine->refresh_rate)
	{
		performance.vector_updates_last_second = vector_updates;
		vector_updates = 0;

		vfcount -= (int)Machine->refresh_rate;
	}
}



/*-------------------------------------------------
    updatescreen - handle frameskipping and UI,
    plus updating the screen during normal
    operations
-------------------------------------------------*/

int updatescreen(void)
{
	/* update sound */
	sound_frame_update();

	/* if we're not skipping this frame, draw the screen */
	if (osd_skip_this_frame() == 0)
	{
		profiler_mark(PROFILER_VIDEO);
		draw_screen();
		profiler_mark(PROFILER_END);
	}

	/* the user interface must be called between vh_update() and osd_update_video_and_audio(), */
	/* to allow it to overlay things on the game display. We must call it even */
	/* if the frame is skipped, to keep a consistent timing. */
	if (handle_user_interface(artwork_get_ui_bitmap()))
		/* quit if the user asked to */
		return 1;

	/* blit to the screen */
	update_video_and_audio();

	/* call the end-of-frame callback */
	if (Machine->drv->video_eof)
	{
		profiler_mark(PROFILER_VIDEO);
		(*Machine->drv->video_eof)();
		profiler_mark(PROFILER_END);
	}

	return 0;
}



/***************************************************************************

    Miscellaneous bits & pieces

***************************************************************************/

/*-------------------------------------------------
    mame_highscore_enabled - return 1 if high
    scores are enabled
-------------------------------------------------*/

int mame_highscore_enabled(void)
{
	/* disable high score when record/playback is on */
	if (record != 0 || playback != 0)
		return 0;

	/* disable high score when cheats are used */
	if (he_did_cheat != 0)
		return 0;

	return 1;
}



/*-------------------------------------------------
    set_led_status - set the state of a given LED
-------------------------------------------------*/

void set_led_status(int num, int on)
{
	if (on)
		leds_status |=	(1 << num);
	else
		leds_status &= ~(1 << num);
}



/*-------------------------------------------------
    mame_get_performance_info - return performance
    info
-------------------------------------------------*/

const struct performance_info *mame_get_performance_info(void)
{
	return &performance;
}



/*-------------------------------------------------
    mame_find_cpu_index - return the index of the
    given CPU, or -1 if not found
-------------------------------------------------*/

int mame_find_cpu_index(const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (Machine->drv->cpu[cpunum].tag && strcmp(Machine->drv->cpu[cpunum].tag, tag) == 0)
			return cpunum;

	return -1;
}



/*-------------------------------------------------
    machine_add_cpu - add a CPU during machine
    driver expansion
-------------------------------------------------*/

struct MachineCPU *machine_add_cpu(struct InternalMachineDriver *machine, const char *tag, int type, int cpuclock)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].cpu_type == 0)
		{
			machine->cpu[cpunum].tag = tag;
			machine->cpu[cpunum].cpu_type = type;
			machine->cpu[cpunum].cpu_clock = cpuclock;
			return &machine->cpu[cpunum];
		}

	logerror("Out of CPU's!\n");
	return NULL;
}



/*-------------------------------------------------
    machine_find_cpu - find a tagged CPU during
    machine driver expansion
-------------------------------------------------*/

struct MachineCPU *machine_find_cpu(struct InternalMachineDriver *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
			return &machine->cpu[cpunum];

	logerror("Can't find CPU '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    machine_remove_cpu - remove a tagged CPU
    during machine driver expansion
-------------------------------------------------*/

void machine_remove_cpu(struct InternalMachineDriver *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
		{
			memmove(&machine->cpu[cpunum], &machine->cpu[cpunum + 1], sizeof(machine->cpu[0]) * (MAX_CPU - cpunum - 1));
			memset(&machine->cpu[MAX_CPU - 1], 0, sizeof(machine->cpu[0]));
			return;
		}

	logerror("Can't find CPU '%s'!\n", tag);
}



/*-------------------------------------------------
    machine_add_speaker - add a speaker during
    machine driver expansion
-------------------------------------------------*/

struct MachineSpeaker *machine_add_speaker(struct InternalMachineDriver *machine, const char *tag, float x, float y, float z)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag == NULL)
		{
			machine->speaker[speakernum].tag = tag;
			machine->speaker[speakernum].x = x;
			machine->speaker[speakernum].y = y;
			machine->speaker[speakernum].z = z;
			return &machine->speaker[speakernum];
		}

	logerror("Out of speakers!\n");
	return NULL;
}



/*-------------------------------------------------
    machine_find_speaker - find a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

struct MachineSpeaker *machine_find_speaker(struct InternalMachineDriver *machine, const char *tag)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag && strcmp(machine->speaker[speakernum].tag, tag) == 0)
			return &machine->speaker[speakernum];

	logerror("Can't find speaker '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    machine_remove_speaker - remove a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

void machine_remove_speaker(struct InternalMachineDriver *machine, const char *tag)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag && strcmp(machine->speaker[speakernum].tag, tag) == 0)
		{
			memmove(&machine->speaker[speakernum], &machine->speaker[speakernum + 1], sizeof(machine->speaker[0]) * (MAX_SPEAKER - speakernum - 1));
			memset(&machine->speaker[MAX_SPEAKER - 1], 0, sizeof(machine->speaker[0]));
			return;
		}

	logerror("Can't find speaker '%s'!\n", tag);
}



/*-------------------------------------------------
    machine_add_sound - add a sound system during
    machine driver expansion
-------------------------------------------------*/

struct MachineSound *machine_add_sound(struct InternalMachineDriver *machine, const char *tag, int type, int clock)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].sound_type == 0)
		{
			machine->sound[soundnum].tag = tag;
			machine->sound[soundnum].sound_type = type;
			machine->sound[soundnum].clock = clock;
			machine->sound[soundnum].config = NULL;
			machine->sound[soundnum].routes = 0;
			return &machine->sound[soundnum];
		}

	logerror("Out of sounds!\n");
	return NULL;

}



/*-------------------------------------------------
    machine_find_sound - find a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

struct MachineSound *machine_find_sound(struct InternalMachineDriver *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
			return &machine->sound[soundnum];

	logerror("Can't find sound '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    machine_remove_sound - remove a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

void machine_remove_sound(struct InternalMachineDriver *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
		{
			memmove(&machine->sound[soundnum], &machine->sound[soundnum + 1], sizeof(machine->sound[0]) * (MAX_SOUND - soundnum - 1));
			memset(&machine->sound[MAX_SOUND - 1], 0, sizeof(machine->sound[0]));
			return;
		}

	logerror("Can't find sound '%s'!\n", tag);
}



/*-------------------------------------------------
    mame_chd_open - interface for opening
    a hard disk image
-------------------------------------------------*/

struct chd_interface_file *mame_chd_open(const char *filename, const char *mode)
{
	/* look for read-only drives first in the ROM path */
	if (mode[0] == 'r' && !strchr(mode, '+'))
	{
		const struct GameDriver *drv;

		/* attempt reading up the chain through the parents */
		for (drv = Machine->gamedrv; drv != NULL; drv = drv->clone_of)
		{
			void* file = mame_fopen(drv->name, filename, FILETYPE_IMAGE, 0);

			if (file != NULL)
				return file;
		}

		return NULL;
	}

	/* look for read/write drives in the diff area */
	return (struct chd_interface_file *)mame_fopen(NULL, filename, FILETYPE_IMAGE_DIFF, 1);
}



/*-------------------------------------------------
    mame_chd_close - interface for closing
    a hard disk image
-------------------------------------------------*/

void mame_chd_close(struct chd_interface_file *file)
{
	mame_fclose((mame_file *)file);
}



/*-------------------------------------------------
    mame_chd_read - interface for reading
    from a hard disk image
-------------------------------------------------*/

UINT32 mame_chd_read(struct chd_interface_file *file, UINT64 offset, UINT32 count, void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fread((mame_file *)file, buffer, count);
}



/*-------------------------------------------------
    mame_chd_write - interface for writing
    to a hard disk image
-------------------------------------------------*/

UINT32 mame_chd_write(struct chd_interface_file *file, UINT64 offset, UINT32 count, const void *buffer)
{
	mame_fseek((mame_file *)file, offset, SEEK_SET);
	return mame_fwrite((mame_file *)file, buffer, count);
}


/*-------------------------------------------------
    mame_chd_length - interface for getting
    the length a hard disk image
-------------------------------------------------*/

UINT64 mame_chd_length(struct chd_interface_file *file)
{
	return mame_fsize((mame_file *)file);
}



/***************************************************************************

    Huge bunch of validity checks for the debug build

***************************************************************************/

int mame_validitychecks(void)
{
	int i,j,cpu;
	UINT8 a,b;
	int error = 0;
	const struct InputPort *inp;
	const char *s;


	a = 0xff;
	b = a + 1;
	if (b > a)	{ printf("UINT8 must be 8 bits\n"); error = 1; }

	if (sizeof(INT8)   != 1)	{ printf("INT8 must be 8 bits\n"); error = 1; }
	if (sizeof(UINT8)  != 1)	{ printf("UINT8 must be 8 bits\n"); error = 1; }
	if (sizeof(INT16)  != 2)	{ printf("INT16 must be 16 bits\n"); error = 1; }
	if (sizeof(UINT16) != 2)	{ printf("UINT16 must be 16 bits\n"); error = 1; }
	if (sizeof(INT32)  != 4)	{ printf("INT32 must be 32 bits\n"); error = 1; }
	if (sizeof(UINT32) != 4)	{ printf("UINT32 must be 32 bits\n"); error = 1; }
	if (sizeof(INT64)  != 8)	{ printf("INT64 must be 64 bits\n"); error = 1; }
	if (sizeof(UINT64) != 8)	{ printf("UINT64 must be 64 bits\n"); error = 1; }

	for (i = 0;drivers[i];i++)
	{
		struct InternalMachineDriver drv;
		const struct RomModule *romp;

		expand_machine_driver(drivers[i]->drv, &drv);

		if (drivers[i]->clone_of == drivers[i])
		{
			printf("%s: %s is set as a clone of itself\n",drivers[i]->source_file,drivers[i]->name);
			error = 1;
		}

		if (drivers[i]->clone_of && drivers[i]->clone_of->clone_of)
		{
			if ((drivers[i]->clone_of->clone_of->flags & NOT_A_DRIVER) == 0)
			{
				printf("%s: %s is a clone of a clone\n",drivers[i]->source_file,drivers[i]->name);
				error = 1;
			}
		}

#if 0
//      if (drivers[i]->drv->color_table_len == drivers[i]->drv->total_colors &&
		if (drivers[i]->drv->color_table_len && drivers[i]->drv->total_colors &&
				drivers[i]->drv->vh_init_palette == 0)
		{
			printf("%s: %s could use color_table_len = 0\n",drivers[i]->source_file,drivers[i]->name);
			error = 1;
		}
#endif

		s = drivers[i]->year;
		for (j = 0; s[j]; j++)
		{
			if (!isdigit(s[j]) && s[j] != '?' && s[j] != '+')
			{
				printf("%s: %s has an invalid year '%s'\n", drivers[i]->source_file,drivers[i]->name,s);
				error = 1;
				break;
			}
		}

		for (j = i+1;drivers[j];j++)
		{
			if (!strcmp(drivers[i]->name,drivers[j]->name))
			{
				printf("%s: %s is a duplicate name (%s, %s)\n",drivers[i]->source_file,drivers[i]->name,drivers[i]->source_file,drivers[j]->source_file);
				error = 1;
			}
			if (!strcmp(drivers[i]->description,drivers[j]->description))
			{
				printf("%s: %s is a duplicate description (%s, %s)\n",drivers[i]->description,drivers[i]->source_file,drivers[i]->name,drivers[j]->name);
				error = 1;
			}
#ifndef MESS
			if (drivers[i]->rom && drivers[i]->rom == drivers[j]->rom
					&& (drivers[i]->flags & NOT_A_DRIVER) == 0
					&& (drivers[j]->flags & NOT_A_DRIVER) == 0)
			{
				printf("%s: %s and %s use the same ROM set\n",drivers[i]->source_file,drivers[i]->name,drivers[j]->name);
				error = 1;
			}
#endif
		}

#ifndef MESS
		if ((drivers[i]->flags & NOT_A_DRIVER) == 0)
		{
			if (drv.sound[0].sound_type == 0 && (drivers[i]->flags & GAME_NO_SOUND) == 0 &&
					strcmp(drivers[i]->name,"minivadr"))
			{
				printf("%s: %s missing GAME_NO_SOUND flag\n",drivers[i]->source_file,drivers[i]->name);
				error = 1;
			}
		}
#endif

		romp = drivers[i]->rom;

		if (romp)
		{
			int region_type_used[REGION_MAX];
			int region_length[REGION_MAX];
			const char *last_name = 0;
			int count = -1;

			for (j = 0;j < REGION_MAX;j++)
			{
				region_type_used[j] = 0;
				region_length[j] = 0;
			}

			/* consistency checks on ROMs */
			while (!ROMENTRY_ISEND(romp))
			{
				const char *c;

				if (ROMENTRY_ISREGION(romp))
				{
					int type = ROMREGION_GETTYPE(romp);

					count++;
					if (type && (type >= REGION_MAX || type <= REGION_INVALID))
					{
						printf("%s: %s has invalid ROM_REGION type %x\n",drivers[i]->source_file,drivers[i]->name,type);
						error = 1;
					}

					region_type_used[type]++;
					region_length[type] = region_length[count] = ROMREGION_GETLENGTH(romp);
				}
				if (ROMENTRY_ISFILE(romp))
				{
					const char *hash;

					last_name = c = ROM_GETNAME(romp);
					while (*c)
					{
						if (tolower(*c) != *c)
						{
							printf("%s: %s has upper case ROM name %s\n",drivers[i]->source_file,drivers[i]->name,ROM_GETNAME(romp));
							error = 1;
						}
						c++;
					}

					hash = ROM_GETHASHDATA(romp);
					if (!hash_verify_string(hash))
					{
						printf("%s: rom '%s' has an invalid hash string '%s'\n", drivers[i]->name, ROM_GETNAME(romp), hash);
						error = 1;
					}
				}
				if (!ROMENTRY_ISREGIONEND(romp))						/* ROM_LOAD_XXX() */
				{
					if (ROM_GETOFFSET(romp) + ROM_GETLENGTH(romp) > region_length[count])
					{
						printf("%s: %s has ROM %s extending past the defined memory region\n",drivers[i]->source_file,drivers[i]->name,last_name);
						error = 1;
					}
				}
				romp++;
			}

			for (j = 1;j < REGION_MAX;j++)
			{
				if (region_type_used[j] > 1)
				{
					printf("%s: %s has duplicated ROM_REGION type %x\n",drivers[i]->source_file,drivers[i]->name,j);
					error = 1;
				}
			}


			/* consistency checks on CPUs */
			for (cpu = 0;cpu < MAX_CPU;cpu++)
			{
				if (drv.cpu[cpu].cpu_type)
				{
					int space,mapnum;
					extern void dummy_get_info(UINT32 state, union cpuinfo *info);

					/* checks to see if this driver is using a dummy CPU */
					if (cputype_get_interface(drv.cpu[cpu].cpu_type)->get_info == dummy_get_info)
					{
						printf("%s: %s uses non-present CPU\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}
					else
					{
#ifdef MESS
						/* check to make sure that this CPU core has the necessities filled out */
						const struct cpu_interface *cpuintrf;
						union cpuinfo info;
						const UINT8 *reg;
						int incomplete_cpu_core = 0;
						static const int required_info[] =
						{
							CPUINFO_STR_NAME, CPUINFO_STR_CORE_FAMILY, CPUINFO_STR_CORE_FILE,
							CPUINFO_PTR_REGISTER_LAYOUT
						};

						cpuintrf = cputype_get_interface(drv.cpu[cpu].cpu_type);
						for (j = 0; j < sizeof(required_info) / sizeof(required_info[0]); j++)
						{
							memset(&info, 0, sizeof(info));
							cpuintrf->get_info(required_info[j], &info);
							if (!info.s)
								incomplete_cpu_core = 1;
						}

						memset(&info, 0, sizeof(info));
						cpuintrf->get_info(CPUINFO_PTR_REGISTER_LAYOUT, &info);
						reg = (const UINT8 *) info.p;
						if (reg)
						{
							for (j = 0; reg[j]; j++)
							{
								if (reg[j] != (UINT8)-1)
								{
									memset(&info, 0, sizeof(info));
									cpuintrf->get_info(CPUINFO_STR_REGISTER + reg[j], &info);
									if (!info.s)
										incomplete_cpu_core = 1;
								}
							}
						}

						if (incomplete_cpu_core)
						{
							memset(&info, 0, sizeof(info));
							cpuintrf->get_info(CPUINFO_STR_NAME, &info);
							printf("%s: %s uses incomplete CPU core %s\n",drivers[i]->source_file, drivers[i]->name,
								info.s);
							error = 1;
						}
#endif /* MESS */
					}

					for (space = 0;space < ADDRESS_SPACES;space++)
						for (mapnum = 0;mapnum < 2;mapnum++)
						{
							int alignunit,databus_width,addr_shift;

							databus_width = cputype_databus_width(drv.cpu[cpu].cpu_type, space);
							addr_shift = cputype_addrbus_shift(drv.cpu[cpu].cpu_type, space);
							alignunit = databus_width/8;

#define SPACE_SHIFT(a)		((addr_shift < 0) ? ((a) << -addr_shift) : ((a) >> addr_shift))
#define SPACE_SHIFT_END(a)	((addr_shift < 0) ? (((a) << -addr_shift) | ((1 << -addr_shift) - 1)) : ((a) >> addr_shift))

							if (drv.cpu[cpu].construct_map[space][mapnum])
							{
								struct address_map_t address_map[MAX_ADDRESS_MAP_SIZE];
								const struct address_map_t *map = address_map;
								UINT32 flags, val;

								memset(address_map, 0, sizeof(address_map));
								(*drv.cpu[cpu].construct_map[space][mapnum])(address_map);

								if (IS_AMENTRY_END(map))
									continue;
								if (!IS_AMENTRY_EXTENDED(map))
								{
									printf("%s: %s wrong MEMORY_READ_START\n",drivers[i]->source_file,drivers[i]->name);
									error = 1;
								}

								flags = AM_EXTENDED_FLAGS(map);
								if (flags & AMEF_SPECIFIES_DBITS)
								{
									val = (flags & AMEF_DBITS_MASK) >> AMEF_DBITS_SHIFT;
									val = (val + 1) * 8;
									if (val != databus_width)
									{
										printf("%s: %s cpu #%d uses wrong data width memory handlers! (width = %d, memory = %08x)\n",drivers[i]->source_file,drivers[i]->name,cpu,databus_width,AM_EXTENDED_FLAGS(map));
										error = 1;
									}
								}

								while (!IS_AMENTRY_END(map))
								{
									if (!IS_AMENTRY_EXTENDED(map))
									{
										if (!IS_AMENTRY_MATCH_MASK(map))
										{
											if (map->end < map->start)
											{
												printf("%s: %s wrong memory read handler start = %08x > end = %08x\n",drivers[i]->source_file,drivers[i]->name,map->start,map->end);
												error = 1;
											}
											if ((SPACE_SHIFT(map->start) & (alignunit-1)) != 0 || (SPACE_SHIFT_END(map->end) & (alignunit-1)) != (alignunit-1))
											{
												printf("%s: %s wrong memory read handler start = %08x, end = %08x ALIGN = %d\n",drivers[i]->source_file,drivers[i]->name,map->start,map->end,alignunit);
												error = 1;
											}
										}
									}
									map++;
								}
							}
						}
				}
			}


			/* consistency chekcs on GfxDecodeInfo */
			if (drv.gfxdecodeinfo)
			{
				for (j = 0;j < MAX_GFX_ELEMENTS && drv.gfxdecodeinfo[j].memory_region != -1;j++)
				{
					int len,avail,k,start;
					int type = drv.gfxdecodeinfo[j].memory_region;


/*
                    if (type && (type >= REGION_MAX || type <= REGION_INVALID))
                    {
                        printf("%s: %s has invalid memory region for gfx[%d]\n",drivers[i]->source_file,drivers[i]->name,j);
                        error = 1;
                    }
*/

					if (!IS_FRAC(drv.gfxdecodeinfo[j].gfxlayout->total))
					{
						start = 0;
						for (k = 0;k < MAX_GFX_PLANES;k++)
						{
							if (drv.gfxdecodeinfo[j].gfxlayout->planeoffset[k] > start)
								start = drv.gfxdecodeinfo[j].gfxlayout->planeoffset[k];
						}
						start &= ~(drv.gfxdecodeinfo[j].gfxlayout->charincrement-1);
						len = drv.gfxdecodeinfo[j].gfxlayout->total *
								drv.gfxdecodeinfo[j].gfxlayout->charincrement;
						avail = region_length[type]
								- (drv.gfxdecodeinfo[j].start & ~(drv.gfxdecodeinfo[j].gfxlayout->charincrement/8-1));
						if ((start + len) / 8 > avail)
						{
							printf("%s: %s has gfx[%d] extending past allocated memory\n",drivers[i]->source_file,drivers[i]->name,j);
							error = 1;
						}
					}
				}
			}
		}

		/* consistency checks on input ports */
		if (drivers[i]->construct_ipt)
		{
			int empty_string_found = 0;

			begin_resource_tracking();

			inp = input_port_allocate(drivers[i]->construct_ipt);

			while (inp->type != IPT_END)
			{
				if (inp->type == IPT_INVALID)
				{
					printf("%s: %s has an input port with an invalid type (0); use IPT_OTHER instead\n",drivers[i]->source_file,drivers[i]->name);
					error = 1;
				}
				else if (inp->name && inp->name != IP_NAME_DEFAULT)
				{
					/* check for strings that should be DEF_STR */
					for (j = 0;j < STR_TOTAL;j++)
					{
						if (inp->name == inptport_default_strings[j]) break;
						else if (!my_stricmp(inp->name,inptport_default_strings[j]))
						{
							printf("%s: %s must use DEF_STR( %s )\n",drivers[i]->source_file,drivers[i]->name,inp->name);
							error = 1;
						}
					}

					/* check for inverted off/on dispswitch order */
					if (inp->type == IPT_DIPSWITCH_SETTING && (inp+1)->type == IPT_DIPSWITCH_SETTING)
					{
						if (inp->name == DEF_STR( On ) && (inp+1)->name == DEF_STR( Off ))
						{
							printf("%s: %s has inverted Off/On dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}

						if (inp->name == DEF_STR( Yes ) && (inp+1)->name == DEF_STR( No ))
						{
							printf("%s: %s has inverted No/Yes dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}
					}

					if (port_type_is_analog(inp->type))
					{
						if (inp->analog.sensitivity == 0)
						{
							printf("%s: %s has an analog port with zero sensitivity\n",drivers[i]->source_file,drivers[i]->name);
							error = 1;
						}
					}

					if (!my_stricmp(inp->name,"table"))
					{
						printf("%s: %s must use DEF_STR( Cocktail ), not %s\n",drivers[i]->source_file,drivers[i]->name,inp->name);
						error = 1;
					}

					if (inp->name == DEF_STR( Cabinet ) && (inp+1)->name == DEF_STR( Upright )
							&& inp->default_value != (inp+1)->default_value)
					{
						printf("%s: %s Cabinet must default to Upright\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					/* check for inverted upright/concktail order */
					if (inp->name == DEF_STR( Cocktail ) && (inp+1)->name == DEF_STR( Upright ))
					{
						printf("%s: %s has inverted Upright/Cocktail dipswitch order\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					/* check for unsorted coinage */
					if (inp->name >= DEF_STR( 9C_1C ) && inp->name <= DEF_STR( Free_Play )
							&& (inp+1)->name >= DEF_STR( 9C_1C ) && (inp+1)->name <= DEF_STR( Free_Play )
							&& inp->name >= (inp+1)->name && !memcmp(&inp->dipsetting, &(inp+1)->dipsetting, sizeof(inp->dipsetting)))
					{
						printf("%s: %s has unsorted coinage %s > %s\n",drivers[i]->source_file,drivers[i]->name,inp->name,(inp+1)->name);
						error = 1;
					}

					/* check for bad flip screen options */
					if (inp->name == DEF_STR( Flip_Screen ) && (inp+1)->name != DEF_STR( Off ))
					{
						printf("%s: %s has wrong Flip Screen option %s\n",drivers[i]->source_file,drivers[i]->name,(inp+1)->name);
						error = 1;
					}

					/* check for bad demo sounds defaults */
					if (inp->name == DEF_STR( Demo_Sounds ) && (inp+2)->name == DEF_STR( On )
							&& inp->default_value != (inp+2)->default_value)
					{
						printf("%s: %s Demo Sounds must default to On\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					/* check for demo sounds using the text 'No' */
					if (inp->name == DEF_STR( Demo_Sounds ) && (inp+1)->name == DEF_STR( No ))
					{
						printf("%s: %s has wrong Demo Sounds option No instead of Off\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
					}

					/* check for trailing spaces */
					if (inp->name[0] && inp->name[strlen(inp->name)-1] == ' ')
					{
						printf("%s: %s input '%s' has trailing spaces\n",drivers[i]->source_file,drivers[i]->name,inp->name);
						error = 1;
					}

					/* check for empty string */
					if (!inp->name[0] && !empty_string_found)
					{
						printf("%s: %s has an input with an empty string\n",drivers[i]->source_file,drivers[i]->name);
						error = 1;
						empty_string_found = 1;
					}
				}

				inp++;
			}


			/* consistency checks on sound and speakers */
			for (j = 0; j < MAX_SPEAKER && drv.speaker[j].tag; j++)
			{
				int k;
				for (k = 0; k < MAX_SPEAKER && drv.speaker[k].tag; k++)
					if (j != k && drv.speaker[k].tag && !strcmp(drv.speaker[j].tag, drv.speaker[k].tag))
					{
						printf("%s: %s has multiple speakers tagged as '%s'\n",drivers[i]->source_file,drivers[i]->name,drv.speaker[j].tag);
						error = 1;
					}
				for (k = 0; k < MAX_SOUND && drv.sound[k].sound_type != SOUND_DUMMY; k++)
					if (drv.sound[k].tag && !strcmp(drv.speaker[j].tag, drv.sound[k].tag))
					{
						printf("%s: %s has both a speaker and a sound chip tagged as '%s'\n",drivers[i]->source_file,drivers[i]->name,drv.speaker[j].tag);
						error = 1;
					}
			}

			for (j = 0; j < MAX_SOUND && drv.sound[j].sound_type != SOUND_DUMMY; j++)
			{
				int k, l;
				for (k = 0; k < drv.sound[j].routes; k++)
				{
					for (l = 0; l < MAX_SPEAKER && drv.speaker[l].tag; l++)
						if (!strcmp(drv.sound[j].route[k].target, drv.speaker[l].tag))
							break;
					if (l >= MAX_SPEAKER || !drv.speaker[l].tag)
					{
						for (l = 0; l < MAX_SOUND && drv.sound[l].sound_type != SOUND_DUMMY; l++)
							if (l != j && drv.sound[l].tag && !strcmp(drv.sound[l].tag, drv.sound[j].route[k].target))
								break;
						if (l >= MAX_SOUND || drv.sound[l].sound_type == SOUND_DUMMY)
						{
							printf("%s: %s attempting to route sound to non-existant speaker '%s'\n",drivers[i]->source_file,drivers[i]->name,drv.sound[j].route[k].target);
							error = 1;
						}
					}
				}
			}

			end_resource_tracking();
		}
	}

#ifdef MESS
	if (mess_validitychecks())
		error = 1;
#endif /* MESS */

	return error;
}



