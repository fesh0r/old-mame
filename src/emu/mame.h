/***************************************************************************

    mame.h

    Controls execution of the core MAME system.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __MAME_H__
#define __MAME_H__

#include "mamecore.h"
#include "video.h"
#include "crsshair.h"
#include "restrack.h"
#include "options.h"
#include "inptport.h"
#include <stdarg.h>

#ifdef MESS
#include "image.h"
#endif /* MESS */



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* return values from run_game */
#define MAMERR_NONE				0		/* no error */
#define MAMERR_FAILED_VALIDITY	1		/* failed validity checks */
#define MAMERR_MISSING_FILES	2		/* missing files */
#define MAMERR_FATALERROR		3		/* some other fatal error */
#define MAMERR_DEVICE			4		/* device initialization error (MESS-specific) */
#define MAMERR_NO_SUCH_GAME		5		/* game was specified but doesn't exist */
#define MAMERR_INVALID_CONFIG	6		/* some sort of error in configuration */
#define MAMERR_IDENT_NONROMS	7		/* identified all non-ROM files */
#define MAMERR_IDENT_PARTIAL	8		/* identified some files but not all */
#define MAMERR_IDENT_NONE		9		/* identified no files */


/* program phases */
#define MAME_PHASE_PREINIT		0
#define MAME_PHASE_INIT			1
#define MAME_PHASE_RESET		2
#define MAME_PHASE_RUNNING		3
#define MAME_PHASE_EXIT			4


/* maxima */
#define MAX_GFX_ELEMENTS		32
#define MAX_MEMORY_REGIONS		32


/* MESS vs. MAME abstractions */
#ifndef MESS
#define APPNAME					"MAME"
#define APPNAME_LOWER			"mame"
#define CONFIGNAME				"mame"
#define APPLONGNAME				"M.A.M.E."
#define CAPGAMENOUN				"GAME"
#define CAPSTARTGAMENOUN		"Game"
#define GAMENOUN				"game"
#define GAMESNOUN				"games"
#define HISTORYNAME				"History"
#define COPYRIGHT				"Copyright Nicola Salmoria\nand the MAME team\nhttp://mamedev.org"
#else
#define APPNAME					"MESS"
#define APPNAME_LOWER			"mess"
#define CONFIGNAME				"mess"
#define APPLONGNAME				"M.E.S.S."
#define CAPGAMENOUN				"SYSTEM"
#define CAPSTARTGAMENOUN		"System"
#define GAMENOUN				"system"
#define GAMESNOUN				"systems"
#define HISTORYNAME				"System Info"
#define COPYRIGHT				"Copyright the MESS team\nhttp://mess.org"
#endif


/* output channels */
enum _output_channel
{
    OUTPUT_CHANNEL_ERROR,
    OUTPUT_CHANNEL_WARNING,
    OUTPUT_CHANNEL_INFO,
    OUTPUT_CHANNEL_DEBUG,
    OUTPUT_CHANNEL_VERBOSE,
    OUTPUT_CHANNEL_LOG,
    OUTPUT_CHANNEL_COUNT
};
typedef enum _output_channel output_channel;


/* memory region types */
enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_USER9,
	REGION_USER10,
	REGION_USER11,
	REGION_USER12,
	REGION_USER13,
	REGION_USER14,
	REGION_USER15,
	REGION_USER16,
	REGION_USER17,
	REGION_USER18,
	REGION_USER19,
	REGION_USER20,
	REGION_DISKS,
	REGION_PLDS,
	REGION_MAX
};

extern const char *const memory_region_names[REGION_MAX];



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* output channel callback */
typedef void (*output_callback_func)(void *param, const char *format, va_list argptr);


/* forward type declarations */
typedef struct _mame_private mame_private;
typedef struct _palette_private palette_private;
typedef struct _streams_private streams_private;
typedef struct _devices_private devices_private;
typedef struct _input_port_private input_port_private;


/* description of the currently-running machine */
/* typedef struct _running_machine running_machine; -- in mamecore.h */
struct _running_machine
{
	/* configuration data */
	const machine_config *	config;				/* points to the constructed machine_config */
	const input_port_config *portconfig;		/* points to a list of input port configurations */

	/* game-related information */
	const game_driver *		gamedrv;			/* points to the definition of the game machine */
	const char *			basename;			/* basename used for game-related paths */

	/* video-related information */
	gfx_element *			gfx[MAX_GFX_ELEMENTS];/* array of pointers to graphic sets (chars, sprites) */
	const device_config *	primary_screen;		/* the primary screen device, or NULL if screenless */
	palette_t *				palette;			/* global palette object */

	/* palette-related information */
	const pen_t *			pens;				/* remapped palette pen numbers */
	struct _colortable_t *	colortable;			/* global colortable for remapping */
	pen_t *					shadow_table;		/* table for looking up a shadowed pen */

	/* audio-related information */
	int						sample_rate;		/* the digital audio sample rate */

	/* debugger-related information */
	int						debug_mode;			/* was debug mode enabled? */

	/* internal core information */
	mame_private *			mame_data;			/* internal data from mame.c */
	palette_private *		palette_data;		/* internal data from palette.c */
	streams_private *		streams_data;		/* internal data from streams.c */
	devices_private *		devices_data;		/* internal data from devices.c */
	input_port_private *	input_port_data;	/* internal data from inptport.c */
#ifdef MESS
	images_private *		images_data;		/* internal data from image.c */
#endif /* MESS */

	/* driver-specific information */
	void *					driver_data;		/* drivers can hang data off of here instead of using globals */
};


typedef struct _mame_system_tm mame_system_tm;
struct _mame_system_tm
{
	UINT8 second;	/* seconds (0-59) */
	UINT8 minute;	/* minutes (0-59) */
	UINT8 hour;		/* hours (0-23) */
	UINT8 mday;		/* day of month (1-31) */
	UINT8 month;	/* month (0-11) */
	INT32 year;		/* year (1=1 AD) */
	UINT8 weekday;	/* day of week (0-6) */
	UINT16 day;		/* day of year (0-365) */
	UINT8 is_dst;	/* is this daylight savings? */
};


typedef struct _mame_system_time mame_system_time;
struct _mame_system_time
{
	INT64 time;					/* number of seconds elapsed since midnight, January 1 1970 UTC */
	mame_system_tm local_time;	/* local time */
	mame_system_tm utc_time;	/* UTC coordinated time */
};



/***************************************************************************
    GLOBAL VARAIBLES
***************************************************************************/

extern const char mame_disclaimer[];
extern char giant_string_buffer[];

extern const char build_version[];



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- core system management ----- */

/* execute as configured by the OPTION_GAMENAME option on the specified options */
int mame_execute(core_options *options);

/* accesses the core_options for the currently running emulation */
core_options *mame_options(void);

/* return the current phase */
int mame_get_phase(running_machine *machine);

/* request callback on frame update */
void add_frame_callback(running_machine *machine, void (*callback)(running_machine *));

/* request callback on reset */
void add_reset_callback(running_machine *machine, void (*callback)(running_machine *));

/* request callback on pause */
void add_pause_callback(running_machine *machine, void (*callback)(running_machine *, int));

/* request callback on termination */
void add_exit_callback(running_machine *machine, void (*callback)(running_machine *));

/* handle update tasks for a frame boundary */
void mame_frame_update(running_machine *machine);



/* ----- global system states ----- */

/* schedule an exit */
void mame_schedule_exit(running_machine *machine);

/* schedule a hard reset */
void mame_schedule_hard_reset(running_machine *machine);

/* schedule a soft reset */
void mame_schedule_soft_reset(running_machine *machine);

/* schedule a new driver */
void mame_schedule_new_driver(running_machine *machine, const game_driver *driver);

/* schedule a save */
void mame_schedule_save(running_machine *machine, const char *filename);

/* schedule a load */
void mame_schedule_load(running_machine *machine, const char *filename);

/* is a scheduled event pending? */
int mame_is_scheduled_event_pending(running_machine *machine);

/* pause the system */
void mame_pause(running_machine *machine, int pause);

/* get the current pause state */
int mame_is_paused(running_machine *machine);



/* ----- memory region management ----- */

/* allocate a new memory region */
UINT8 *new_memory_region(running_machine *machine, int type, UINT32 length, UINT32 flags);

/* free an allocated memory region */
void free_memory_region(running_machine *machine, int num);

/* return a pointer to a specified memory region */
UINT8 *memory_region(int num);

/* return the size (in bytes) of a specified memory region */
UINT32 memory_region_length(int num);

/* return the type of a specified memory region */
UINT32 memory_region_type(running_machine *machine, int num);

/* return the flags (defined in romload.h) for a specified memory region */
UINT32 memory_region_flags(running_machine *machine, int num);



/* ----- output management ----- */

/* set the output handler for a channel, returns the current one */
void mame_set_output_channel(output_channel channel, output_callback_func callback, void *param, output_callback_func *prevcb, void **prevparam);

/* built-in default callbacks */
void mame_file_output_callback(void *param, const char *format, va_list argptr);
void mame_null_output_callback(void *param, const char *format, va_list argptr);

/* calls to be used by the code */
void mame_printf_error(const char *format, ...) ATTR_PRINTF(1,2);
void mame_printf_warning(const char *format, ...) ATTR_PRINTF(1,2);
void mame_printf_info(const char *format, ...) ATTR_PRINTF(1,2);
void mame_printf_verbose(const char *format, ...) ATTR_PRINTF(1,2);
void mame_printf_debug(const char *format, ...) ATTR_PRINTF(1,2);

/* discourage the use of printf directly */
/* sadly, can't do this because of the ATTR_PRINTF under GCC */
/*
#undef printf
#define printf !MUST_USE_MAME_PRINTF_*_CALLS_WITHIN_THE_CORE!
*/


/* ----- miscellaneous bits & pieces ----- */

/* pop-up a user visible message */
void CLIB_DECL popmessage(const char *format,...) ATTR_PRINTF(1,2);

/* log to the standard error.log file */
void CLIB_DECL logerror(const char *format,...) ATTR_PRINTF(1,2);

/* adds a callback to be called on logerror() */
void add_logerror_callback(running_machine *machine, void (*callback)(running_machine *, const char *));

/* parse the configured INI files */
void mame_parse_ini_files(core_options *options, const game_driver *driver);

/* standardized random number generator */
UINT32 mame_rand(running_machine *machine);

/* return the index of the given CPU, or -1 if not found */
int mame_find_cpu_index(running_machine *machine, const char *tag);

/* retrieve the base system time */
void mame_get_base_datetime(running_machine *machine, mame_system_time *systime);

/* retrieve the current system time */
void mame_get_current_datetime(running_machine *machine, mame_system_time *systime);



#ifdef MESS
#include "mess.h"
#endif /* MESS */

#endif	/* __MAME_H__ */
