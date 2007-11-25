//============================================================
//
//  winsync.c - Win32 OSD core synchronization functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "osdcore.h"


//============================================================
//  DEBUGGING
//============================================================

#define DEBUG_SLOW_LOCKS	0



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef BOOL (WINAPI *try_enter_critical_section_ptr)(LPCRITICAL_SECTION lpCriticalSection);

struct _osd_lock
{
	CRITICAL_SECTION	critsect;
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

static try_enter_critical_section_ptr try_enter_critical_section = NULL;
static int checked_for_try_enter = FALSE;



//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
	osd_lock *lock = malloc(sizeof(*lock));
	if (lock == NULL)
		return NULL;
	InitializeCriticalSection(&lock->critsect);
	return lock;
}


//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *lock)
{
#if DEBUG_SLOW_LOCKS
	osd_ticks_t ticks = osd_ticks();
#endif

	// block until we can acquire the lock
	EnterCriticalSection(&lock->critsect);

#if DEBUG_SLOW_LOCKS
	// log any locks that take more than 1ms
	ticks = osd_ticks() - ticks;
	if (ticks > osd_ticks_per_second() / 1000) mame_printf_debug("Blocked %d ticks on lock acquire\n", (int)ticks);
#endif
}


//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *lock)
{
	int result = TRUE;

	// if we haven't yet checked for the TryEnter API, do it now
	if (!checked_for_try_enter)
	{
		// see if we can use TryEnterCriticalSection
		HMODULE library = LoadLibrary(TEXT("kernel32.dll"));
		if (library != NULL)
			try_enter_critical_section = (try_enter_critical_section_ptr)GetProcAddress(library, "TryEnterCriticalSection");
		checked_for_try_enter = TRUE;
	}

	// if we have it, use it, otherwise just block
	if (try_enter_critical_section != NULL)
		result = (*try_enter_critical_section)(&lock->critsect);
	else
		EnterCriticalSection(&lock->critsect);
	return result;
}


//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *lock)
{
	LeaveCriticalSection(&lock->critsect);
}


//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *lock)
{
	DeleteCriticalSection(&lock->critsect);
	free(lock);
}


//============================================================
//  osd_compare_exchange32
//============================================================

#ifndef osd_compare_exchange32
INT32 osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	return InterlockedCompareExchange((LPLONG)ptr, (LONG)exchange, (LONG)compare);
}
#endif


//============================================================
//  osd_compare_exchange64
//============================================================

#ifdef PTR64
#ifndef osd_compare_exchange64
INT64 osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	return InterlockedCompareExchange64((LONGLONG*)ptr, (LONGLONG)exchange, (LONGLONG)compare);
}
#endif
#endif
