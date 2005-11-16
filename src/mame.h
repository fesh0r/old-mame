/***************************************************************************

    mame.h

    Controls execution of the core MAME system.

***************************************************************************/

#ifndef __MAME_H__
#define __MAME_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "fileio.h"
#include "drawgfx.h"
#include "palette.h"

#ifdef MESS
#include "device.h"
#endif /* MESS */

extern char build_version[];



/***************************************************************************

    Parameters

***************************************************************************/

#define MAX_GFX_ELEMENTS 32
#define MAX_MEMORY_REGIONS 32

#ifndef MESS
#define APPNAME					"MAME"
#define APPLONGNAME				"M.A.M.E."
#define CAPGAMENOUN				"GAME"
#define CAPSTARTGAMENOUN		"Game"
#define GAMENOUN				"game"
#define GAMESNOUN				"games"
#define HISTORYNAME				"History"
#else
#define APPNAME					"MESS"
#define APPLONGNAME				"M.E.S.S."
#define CAPGAMENOUN				"SYSTEM"
#define CAPSTARTGAMENOUN		"System"
#define GAMENOUN				"system"
#define GAMESNOUN				"systems"
#define HISTORYNAME				"System Info"
#endif



/***************************************************************************

    Core description of the currently-running machine

***************************************************************************/

struct _region_info
{
	UINT8 *		base;
	size_t		length;
	UINT32		type;
	UINT32		flags;
};
typedef struct _region_info region_info;


struct _running_machine
{
	/* ----- game-related information ----- */

	/* points to the definition of the game machine */
	const game_driver *		gamedrv;

	/* points to the constructed MachineDriver */
	const machine_config *	drv;

	/* array of memory regions */
	region_info				memory_region[MAX_MEMORY_REGIONS];

	/* number of bad ROMs encountered */
	int						rom_load_warnings;


	/* ----- video-related information ----- */

	/* array of pointers to graphic sets (chars, sprites) */
	gfx_element *			gfx[MAX_GFX_ELEMENTS];

	/* main bitmap to render to (but don't do it directly!) */
	mame_bitmap *	scrbitmap;

	/* current visible area, and a prerotated one adjusted for orientation */
	rectangle 		visible_area;
	rectangle		absolute_visible_area;

	/* current video refresh rate */
	float					refresh_rate;

	/* remapped palette pen numbers. When you write directly to a bitmap in a
       non-paletteized mode, use this array to look up the pen number. For example,
       if you want to use color #6 in the palette, use pens[6] instead of just 6. */
	pen_t *					pens;

	/* lookup table used to map gfx pen numbers to color numbers */
	UINT16 *				game_colortable;

	/* the above, already remapped through Machine->pens */
	pen_t *					remapped_colortable;

	/* video color depth: 16, 15 or 32 */
	int						color_depth;


	/* ----- audio-related information ----- */

	/* the digital audio sample rate; 0 if sound is disabled. */
	int						sample_rate;


	/* ----- input-related information ----- */

	/* the input ports definition from the driver is copied here and modified */
	input_port_entry *		input_ports;

	/* original input_ports without modifications */
	input_port_entry *		input_ports_default;


	/* ----- user interface-related information ----- */

	/* user interface orientation */
	int 					ui_orientation;


	/* ----- debugger-related information ----- */

	/* bitmap where the debugger is rendered */
	mame_bitmap *	debug_bitmap;

	/* pen array for the debugger, analagous to the pens above */
	pen_t *					debug_pens;

	/* colortable mapped through the pens, as for the game */
	pen_t *					debug_remapped_colortable;

	/* font used by the debugger */
	gfx_element *			debugger_font;

#ifdef MESS
	struct IODevice *devices;
#endif /* MESS */
};
typedef struct _running_machine running_machine;



/***************************************************************************

    Options passed from the frontend to the main core

***************************************************************************/

#define ARTWORK_USE_ALL			(~0)
#define ARTWORK_USE_NONE		(0)
#define ARTWORK_USE_BACKDROPS	0x01
#define ARTWORK_USE_OVERLAYS	0x02
#define ARTWORK_USE_BEZELS		0x04


#ifdef MESS
/*
 * This is a filename and it's associated peripheral type
 * The types are defined in device.h (IO_...)
 */
struct ImageFile
{
	const char *name;
	iodevice_t type;
};
#endif /* MESS */

/* The host platform should fill these fields with the preferences specified in the GUI */
/* or on the commandline. */
struct _global_options
{
	mame_file *	record;			/* handle to file to record input to */
	mame_file *	playback;		/* handle to file to playback input from */
	mame_file *	language_file;	/* handle to file for localization */

	int		mame_debug;		/* 1 to enable debugging */
	int		cheat;			/* 1 to enable cheating */
	int 	gui_host;		/* 1 to tweak some UI-related things for better GUI integration */
	int 	skip_disclaimer;	/* 1 to skip the disclaimer screen at startup */
	int 	skip_gameinfo;		/* 1 to skip the game info screen at startup */
	int 	skip_warnings;		/* 1 to skip the warnings screen at startup */

	int		samplerate;		/* sound sample playback rate, in Hz */
	int		use_samples;	/* 1 to enable external .wav samples */

	float	brightness;		/* brightness of the display */
	float	pause_bright;		/* additional brightness when in pause */
	float	gamma;			/* gamma correction of the display */
	int		vector_width;	/* requested width for vector games; 0 means default (640) */
	int		vector_height;	/* requested height for vector games; 0 means default (480) */
	int		ui_orientation;	/* orientation of the UI relative to the video */

	int		beam;			/* vector beam width */
	float	vector_flicker;	/* vector beam flicker effect control */
	float	vector_intensity;/* vector beam intensity */
	int		translucency;	/* 1 to enable translucency on vectors */
	int 	antialias;		/* 1 to enable antialiasing on vectors */

	int		use_artwork;	/* bitfield indicating which artwork pieces to use */
	int		artwork_res;	/* 1 for 1x game scaling, 2 for 2x */
	int		artwork_crop;	/* 1 to crop artwork to the game screen */

	const char * savegame;	/* string representing a savegame to load; if one length then interpreted as a character */
	int		auto_save;		/* 1 to automatically save/restore at startup/quitting time */
	char *	bios;			/* specify system bios (if used), 0 is default */

	int		debug_width;	/* requested width of debugger bitmap */
	int		debug_height;	/* requested height of debugger bitmap */
	int		debug_depth;	/* requested depth of debugger bitmap */

	const char *controller;	/* controller-specific cfg to load */

#ifdef MESS
	UINT32 ram;
	struct ImageFile image_files[32];
	int		image_count;
	int disable_normal_ui;

	int		min_width;		/* minimum width for the display */
	int		min_height;		/* minimum height for the display */
#endif /* MESS */
};
typedef struct _global_options global_options;



/***************************************************************************

    Display state passed to the OSD layer for rendering

***************************************************************************/

/* these flags are set in the mame_display struct to indicate that */
/* a particular piece of state has changed since the last call to */
/* osd_update_video_and_audio() */
#define GAME_BITMAP_CHANGED			0x00000001
#define GAME_PALETTE_CHANGED		0x00000002
#define GAME_VISIBLE_AREA_CHANGED	0x00000004
#define VECTOR_PIXELS_CHANGED		0x00000008
#define DEBUG_BITMAP_CHANGED		0x00000010
#define DEBUG_PALETTE_CHANGED		0x00000020
#define DEBUG_FOCUS_CHANGED			0x00000040
#define LED_STATE_CHANGED			0x00000080
#define GAME_REFRESH_RATE_CHANGED	0x00000100
#ifdef MESS
#define GAME_OPTIONAL_FRAMESKIP     0x00000200
#endif


/* the main mame_display structure, containing the current state of the */
/* video display */
struct _mame_display
{
	/* bitfield indicating which states have changed */
	UINT32					changed_flags;

	/* game bitmap and display information */
	mame_bitmap *	game_bitmap;			/* points to game's bitmap */
	rectangle		game_bitmap_update;		/* bounds that need to be updated */
	const rgb_t *			game_palette;			/* points to game's adjusted palette */
	UINT32					game_palette_entries;	/* number of palette entries in game's palette */
	UINT32 *				game_palette_dirty;		/* points to game's dirty palette bitfield */
	rectangle 		game_visible_area;		/* the game's visible area */
	float					game_refresh_rate;		/* refresh rate */
	void *					vector_dirty_pixels;	/* points to X,Y pairs of dirty vector pixels */

	/* debugger bitmap and display information */
	mame_bitmap *	debug_bitmap;			/* points to debugger's bitmap */
	const rgb_t *			debug_palette;			/* points to debugger's palette */
	UINT32					debug_palette_entries;	/* number of palette entries in debugger's palette */
	UINT8					debug_focus;			/* set to 1 if debugger has focus */

	/* other misc information */
	UINT8					led_state;				/* bitfield of current LED states */
};
/* in mamecore.h: typedef struct _mame_display mame_display; */



/***************************************************************************

    Performance data

***************************************************************************/

struct _performance_info
{
	double					game_speed_percent;		/* % of full speed */
	double					frames_per_second;		/* actual rendered fps */
	int						vector_updates_last_second; /* # of vector updates last second */
	int						partial_updates_this_frame; /* # of partial updates last frame */
};
/* In mamecore.h: typedef struct _performance_info performance_info; */



/***************************************************************************

    Globals referencing the current machine and the global options

***************************************************************************/

extern global_options options;
extern running_machine *Machine;



/***************************************************************************

    Function prototypes

***************************************************************************/

/* ----- core system management ----- */

/* execute a given game by index in the drivers[] array */
int run_game(int game);

/* construct a machine driver */
void expand_machine_driver(void (*constructor)(machine_config *), machine_config *output);

/* pause the system */
void mame_pause(int pause);

/* get the current pause state */
int mame_is_paused(void);



/* ----- screen rendering and management ----- */

/* set the current visible area of the screen bitmap */
void set_visible_area(int min_x, int max_x, int min_y, int max_y);

/* set the current refresh rate of the video mode */
void set_refresh_rate(float fps);

/* force an erase and a complete redraw of the video next frame */
void schedule_full_refresh(void);

/* called by cpuexec.c to reset updates at the end of VBLANK */
void reset_partial_updates(void);

/* force a partial update of the screen up to and including the requested scanline */
void force_partial_update(int scanline);

/* finish updating the screen for this frame */
void draw_screen(void);

/* update the video by calling down to the OSD layer */
void update_video_and_audio(void);

/* update the screen, handling frame skipping and rendering */
/* (this calls draw_screen and update_video_and_audio) */
int updatescreen(void);



/* ----- miscellaneous bits & pieces ----- */

/* mame_fopen() must use this to know if high score files can be used */
int mame_highscore_enabled(void);

/* set the state of a given LED */
void set_led_status(int num, int on);

/* return current performance data */
const performance_info *mame_get_performance_info(void);

/* return the index of the given CPU, or -1 if not found */
int mame_find_cpu_index(const char *tag);

/* runs validity checks, -1 for all */
int mame_validitychecks(void);

#ifdef MESS
#include "mess.h"
#endif /* MESS */

#endif	/* __MAME_H__ */
