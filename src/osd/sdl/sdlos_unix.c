//============================================================
//
//  sdlos_*.c - OS specific low level code
//
//  Copyright (c) 1996-2010, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

// MAME headers
#include "osdcore.h"



//============================================================
//   osd_cycles
//============================================================

osd_ticks_t osd_ticks(void)
{
		struct timeval    tp;
		static osd_ticks_t start_sec = 0;

		gettimeofday(&tp, NULL);
		if (start_sec==0)
			start_sec = tp.tv_sec;
		return (tp.tv_sec - start_sec) * (osd_ticks_t) 1000000 + tp.tv_usec;
}

osd_ticks_t osd_ticks_per_second(void)
{
	return (osd_ticks_t) 1000000;
}

//============================================================
//  osd_sleep
//============================================================

void osd_sleep(osd_ticks_t duration)
{
	UINT32 msec;

	// convert to milliseconds, rounding down
	msec = (UINT32)(duration * 1000 / osd_ticks_per_second());

	// only sleep if at least 2 full milliseconds
	if (msec >= 2)
	{
		// take a couple of msecs off the top for good measure
		msec -= 2;
		usleep(msec*1000);
	}
}

//============================================================
//  osd_num_processors
//============================================================

int osd_num_processors(void)
{
	int processors = 1;

#if defined(_SC_NPROCESSORS_ONLN)
	processors = sysconf(_SC_NPROCESSORS_ONLN);
#endif
	return processors;
}

//============================================================
//  osd_malloc
//============================================================

void *osd_malloc(size_t size)
{
#ifndef MALLOC_DEBUG
	return malloc(size);
#else
#error "MALLOC_DEBUG not yet supported"
#endif
}


//============================================================
//  osd_free
//============================================================

void osd_free(void *ptr)
{
#ifndef MALLOC_DEBUG
	free(ptr);
#else
#error "MALLOC_DEBUG not yet supported"
#endif
}

//============================================================
//  osd_alloc_executable
//
//  allocates "size" bytes of executable memory.  this must take
//  things like NX support into account.
//============================================================

void *osd_alloc_executable(size_t size)
{
#if defined(SDLMAME_BSD)
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
#elif defined(SDLMAME_UNIX)
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0);
#endif
}

//============================================================
//  osd_free_executable
//
//  frees memory allocated with osd_alloc_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
	munmap(ptr, size);
}

//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
	#ifdef MAME_DEBUG
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	kill(getpid(), SIGTRAP);
	#else
	printf("Ignoring MAME exception: %s\n", message);
	#endif
}

