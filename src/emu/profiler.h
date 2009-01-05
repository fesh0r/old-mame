/***************************************************************************

    profiler.h

    Functions to manage profiling of MAME execution.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    To start profiling a certain section, e.g. video:
    profiler_mark(PROFILER_VIDEO);

    to end profiling the current section:
    profiler_mark(PROFILER_END);

    the profiler handles a FILO list so calls may be nested.

***************************************************************************/

#pragma once

#ifndef __PROFILER_H__
#define __PROFILER_H__

#include "astring.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* profiling */
enum
{
	PROFILER_END = -1,
	PROFILER_CPU_FIRST = 0,
	PROFILER_CPU_MAX = PROFILER_CPU_FIRST + 32,
	PROFILER_MEMREAD,
	PROFILER_MEMWRITE,
	PROFILER_VIDEO,
	PROFILER_DRAWGFX,
	PROFILER_COPYBITMAP,
	PROFILER_TILEMAP_DRAW,
	PROFILER_TILEMAP_DRAW_ROZ,
	PROFILER_TILEMAP_UPDATE,
	PROFILER_BLIT,
	PROFILER_SOUND,
	PROFILER_TIMER_CALLBACK,
	PROFILER_INPUT,		/* input.c and inptport.c */
	PROFILER_MOVIE_REC,	/* movie recording */
	PROFILER_LOGERROR,	/* logerror */
	PROFILER_EXTRA,		/* everything else */

	/* the USER types are available to driver writers to profile */
	/* custom sections of the code */
	PROFILER_USER1,
	PROFILER_USER2,
	PROFILER_USER3,
	PROFILER_USER4,

	PROFILER_PROFILER,
	PROFILER_IDLE,
	PROFILER_TOTAL
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _profiler_filo_entry profiler_filo_entry;
struct _profiler_filo_entry
{
	int				type;				/* type of entry */
	osd_ticks_t		start;				/* start time */
};


typedef struct _profiler_data profiler_data;
struct _profiler_data
{
	UINT32			context_switches;	/* number of context switches seen */
	osd_ticks_t		duration[PROFILER_TOTAL]; /* duration spent in each entry */
};


typedef struct _profiler_state profiler_state;
struct _profiler_state
{
	UINT8			enabled;			/* are we enabled? */
	UINT8			filoindex;			/* current FILO index */
	UINT8			dataindex;			/* current data index */
	UINT8			dataready;			/* are we to display the data yet? */
	profiler_filo_entry filo[16];		/* array of FILO entries */
	profiler_data	data[16];			/* array of data */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern profiler_state global_profiler;



/***************************************************************************
    MACROS
***************************************************************************/

#ifdef MAME_PROFILER

#define profiler_mark(x)		do { if (global_profiler.enabled) _profiler_mark(x); } while (0)
#define profiler_start()		do { global_profiler.enabled = TRUE; global_profiler.filoindex = global_profiler.dataindex = global_profiler.dataready = 0; } while (0)
#define profiler_stop()			do { global_profiler.enabled = FALSE; } while (0)
#define profiler_get_text(x,s)	_profiler_get_text(x, s)

#else

#define profiler_mark(x)		do { } while (0)
#define profiler_start()		do { } while (0)
#define profiler_stop()			do { } while (0)
#define profiler_get_text(x,s) 	astring_reset(s)

#endif



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- core functions (do not call directly; use macros) ----- */

/* mark the beginning/end of a profiler entry */
void _profiler_mark(int type);

/* return the current text in an astring */
astring *_profiler_get_text(running_machine *machine, astring *string);


#endif	/* __PROFILER_H__ */
