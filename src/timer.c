/***************************************************************************

  timer.c

  Functions needed to generate timing and synchronization between several
  CPUs.

  Changes 2/27/99:
  	- added some rounding to the sorting of timers so that two timers
  		allocated to go off at the same time will go off in the order
  		they were allocated, without concern for floating point rounding
  		errors (thanks Juergen!)
  	- fixed a bug where the base_time was not updated when a CPU was
  		suspended, making subsequent calls to getabsolutetime() return an
  		incorrect time (thanks Nicola!)
  	- changed suspended CPUs so that they don't eat their timeslice until
  		all other CPUs have used up theirs; this allows a slave CPU to
  		trigger a higher priority CPU in the middle of the timeslice
  	- added the ability to call timer_reset() on a oneshot or pulse timer
  		from within that timer's callback; in this case, the timer won't
  		get removed (oneshot) or won't get reprimed (pulse)

  Changes 12/17/99 (HJB):
	- added overclocking factor and functions to set/get it at runtime.

  Changes 12/23/99 (HJB):
	- added burn() function pointer to tell CPU cores when we want to
	  burn cycles, because the cores might need to adjust internal
	  counters or timers.

***************************************************************************/

#include "cpuintrf.h"
#include "driver.h"
#include "timer.h"

#include <stdarg.h>


#define VERBOSE 0

#define MAX_TIMERS 256


/*-------------------------------------------------
	internal timer structures
-------------------------------------------------*/

typedef struct timer_entry
{
	struct timer_entry *next;
	struct timer_entry *prev;
	void (*callback)(int);
	int callback_param;
	int tag;
	UINT8 enabled;
	UINT8 temporary;
	double period;
	double start;
	double expire;
} timer_entry;

typedef struct
{
	int *icount;
	void (*burn)(int cycles);
    int index;
	int suspended;
	int trigger;
	int nocount;
	int lost;
	double time;
	double sec_to_cycles;
	double cycles_to_sec;
	double overclock;
} cpu_entry;



/*-------------------------------------------------
	global variables
-------------------------------------------------*/

/* conversion constants */
double cycles_to_sec[MAX_CPU];
double sec_to_cycles[MAX_CPU];

/* list of per-CPU timer data */
static cpu_entry cpudata[MAX_CPU+1];
static cpu_entry *lastcpu;
static cpu_entry *active_cpu;
static cpu_entry *last_active_cpu;

/* list of active timers */
static timer_entry timers[MAX_TIMERS];
static timer_entry *timer_head;
static timer_entry *timer_free_head;
static timer_entry *timer_free_tail;

/* other internal states */
static double base_time;
static double global_offset;
static timer_entry *callback_timer;
static int callback_timer_modified;

/* prototypes */
static int pick_cpu(int *cpu, int *cycles, double expire);

#if VERBOSE
#define verbose_print logerror
#endif



/*-------------------------------------------------
	getabsolutetime - return the current absolute
	time
-------------------------------------------------*/

INLINE double getabsolutetime(void)
{
	if (active_cpu && (*active_cpu->icount + active_cpu->lost) > 0)
		return base_time - ((double)(*active_cpu->icount + active_cpu->lost) * active_cpu->cycles_to_sec);
	else
		return base_time;
}


/*-------------------------------------------------
	timer_adjust_icount - adjust the current CPU's
	instruction counter so that a new event will
	fire at the right time
-------------------------------------------------*/

INLINE void timer_adjust_icount(timer_entry *timer, double time, double period)
{
	int newicount, diff;

	/* compute a new icount for the current CPU */
	if (period == TIME_NOW)
		newicount = 0;
	else
		newicount = (int)((timer->expire - time) * active_cpu->sec_to_cycles) + 1;

	/* determine if we're scheduled to run more cycles */
	diff = *active_cpu->icount - newicount;

	/* if so, set the new icount and compute the amount of "lost" time */
	if (diff > 0)
	{
		active_cpu->lost += diff;
		if (active_cpu->burn)
			(*active_cpu->burn)(diff);  /* let the CPU burn the cycles */
		else
			*active_cpu->icount = newicount;  /* CPU doesn't care */
	}
}


/*-------------------------------------------------
	timer_new - allocate a new timer
-------------------------------------------------*/

INLINE timer_entry *timer_new(void)
{
	timer_entry *timer;

	/* remove an empty entry */
	if (!timer_free_head)
		return NULL;
	timer = timer_free_head;
	timer_free_head = timer->next;
	if (!timer_free_head)
		timer_free_tail = NULL;

	return timer;
}


/*-------------------------------------------------
	timer_list_insert - insert a new timer into
	the list at the appropriate location
-------------------------------------------------*/

INLINE void timer_list_insert(timer_entry *timer)
{
	double expire = timer->enabled ? timer->expire : TIME_NEVER;
	timer_entry *t, *lt = NULL;

#ifdef MAME_DEBUG // LBO - new code in this block
{
	int tnum = 0;
	/* loop over the timer list */
	for (t = timer_head; t; t = t->next)
	{
		tnum ++;
		if (t == timer)
		{
			printf ("This timer is already inserted in the list!\n");
		}
		if (tnum == MAX_TIMERS-1)
		{
			printf ("Timer list is full!\n");
		}
	}
}
#endif

	/* loop over the timer list */
	for (t = timer_head; t; lt = t, t = t->next)
	{
		/* if the current list entry expires after us, we should be inserted before it */
		/* note that due to floating point rounding, we need to allow a bit of slop here */
		/* because two equal entries -- within rounding precision -- need to sort in */
		/* the order they were inserted into the list */
		if ((t->expire - expire) > TIME_IN_NSEC(1))
		{
			/* link the new guy in before the current list entry */
			timer->prev = t->prev;
			timer->next = t;

			if (t->prev)
				t->prev->next = timer;
			else
				timer_head = timer;
			t->prev = timer;
			return;
		}
	}

	/* need to insert after the last one */
	if (lt)
		lt->next = timer;
	else
		timer_head = timer;
	timer->prev = lt;
	timer->next = NULL;
}


/*-------------------------------------------------
	timer_list_remove - remove a timer from the
	linked list
-------------------------------------------------*/

INLINE void timer_list_remove(timer_entry *timer)
{
#ifdef MAME_DEBUG // LBO - new code in this block
{
	int tnum = 0;
	timer_entry *t;
	/* loop over the timer list */
	for (t = timer_head; t; t = t->next)
	{
		tnum ++;
		if (t == timer)
		{
			break;
		}
	}

	if (t == NULL)
		printf ("timer not found in list");
}
#endif

	/* remove it from the list */
	if (timer->prev)
		timer->prev->next = timer->next;
	else
		timer_head = timer->next;
	if (timer->next)
		timer->next->prev = timer->prev;
}


/*-------------------------------------------------
	timer_init - initialize the timer system
-------------------------------------------------*/

void timer_init(void)
{
	cpu_entry *cpu;
	int i;

	/* we need to wait until the first call to timer_cyclestorun before using real CPU times */
	base_time = 0.0;
	global_offset = 0.0;
	callback_timer = NULL;
	callback_timer_modified = 0;

	/* reset the timers */
	memset(timers, 0, sizeof(timers));

	/* initialize the lists */
	timer_head = NULL;
	timer_free_head = &timers[0];
	for (i = 0; i < MAX_TIMERS-1; i++)
	{
		timers[i].tag = -1;
		timers[i].next = &timers[i+1];
	}
	timers[MAX_TIMERS-1].next = NULL;
	timer_free_tail = &timers[MAX_TIMERS-1];

	/* reset the CPU timers */
	memset(cpudata, 0, sizeof(cpudata));

	/* compute the cycle times */
	lastcpu = cpudata;
	for (cpu = cpudata, i = 0; i < MAX_CPU; cpu++, i++)
	{
		int cputype = Machine->drv->cpu[i].cpu_type;

		/* stop when we hit a dummy */
		if (cputype == CPU_DUMMY)
			break;
		lastcpu = cpu;

		/* make a pointer to this CPU's interface functions */
		cpu->icount = cputype_get_interface(cputype)->icount;
		cpu->burn = cputype_get_interface(cputype)->burn;

		/* get the CPU's overclocking factor */
		cpu->overclock = cputype_get_interface(cputype)->overclock;

        /* everyone is active but suspended by the reset line until further notice */
		cpu->suspended = SUSPEND_REASON_RESET;

		/* set the CPU index */
		cpu->index = i;

		/* compute the cycle times */
		cpu->sec_to_cycles = sec_to_cycles[i] = cpu->overclock * Machine->drv->cpu[i].cpu_clock;
		cpu->cycles_to_sec = cycles_to_sec[i] = 1.0 / sec_to_cycles[i];
	}

	/* reset our active CPU tracking */
	active_cpu = NULL;
	last_active_cpu = lastcpu;
}


/*-------------------------------------------------
	timer_get_overclock - get overclocking factor
	for a CPU
-------------------------------------------------*/

double timer_get_overclock(int cpunum)
{
	cpu_entry *cpu = &cpudata[cpunum];
	return cpu->overclock;
}


/*-------------------------------------------------
	timer_set_overclock - set overclocking factor
	for a CPU
-------------------------------------------------*/

void timer_set_overclock(int cpunum, double overclock)
{
	cpu_entry *cpu = &cpudata[cpunum];
	cpu->overclock = overclock;
	cpu->sec_to_cycles = sec_to_cycles[cpunum] = cpu->overclock * Machine->drv->cpu[cpunum].cpu_clock;
	cpu->cycles_to_sec = cycles_to_sec[cpunum] = 1.0 / sec_to_cycles[cpunum];
}


/*-------------------------------------------------
	timer_alloc - allocate a permament timer that
	isn't primed yet
-------------------------------------------------*/

void *timer_alloc(void (*callback)(int))
{
	timer_entry *timer = timer_new();
	double time = getabsolutetime();

	/* fail if we can't allocate a new entry */
	if (!timer)
		return NULL;

	/* fill in the record */
	timer->callback = callback;
	timer->callback_param = 0;
	timer->enabled = 0;
	timer->temporary = 0;
	timer->tag = get_resource_tag();
	timer->period = 0;

	/* compute the time of the next firing and insert into the list */
	timer->start = time;
	timer->expire = TIME_NEVER;
	timer_list_insert(timer);

	/* return a handle */
	return timer;
}


/*-------------------------------------------------
	timer_adjust - adjust the time when this
	timer will fire, and whether or not it will
	fire periodically
-------------------------------------------------*/

void timer_adjust(void *which, double duration, int param, double period)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	/* if this is the callback timer, mark it modified */
	if (timer == callback_timer)
		callback_timer_modified = 1;

	/* compute the time of the next firing and insert into the list */
	timer->callback_param = param;
	timer->enabled = 1;

	/* set the start and expire times */
	timer->start = time;
	timer->expire = time + duration;
	timer->period = period;

	/* remove and re-insert the timer in its new order */
	timer_list_remove(timer);
	timer_list_insert(timer);

	/* if we're supposed to fire before the end of this cycle, adjust the counter */
	if (active_cpu && timer->expire < base_time)
		timer_adjust_icount(timer, time, period);
}


/*-------------------------------------------------
	timer_pulse - allocate a pulse timer, which
	repeatedly calls the callback using the given
	period
-------------------------------------------------*/

void timer_pulse(double period, int param, void (*callback)(int))
{
	void *timer = timer_alloc(callback);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* adjust to our liking */
	timer_adjust(timer, period, param, period);
}


/*-------------------------------------------------
	timer_set - allocate a one-shot timer, which
	calls the callback after the given duration
-------------------------------------------------*/

void timer_set(double duration, int param, void (*callback)(int))
{
	timer_entry *timer = timer_alloc(callback);

	/* fail if we can't allocate */
	if (!timer)
		return;

	/* mark the timer temporary */
	timer->temporary = 1;

	/* adjust to our liking */
	timer_adjust(timer, duration, param, 0);
}


/*-------------------------------------------------
	timer_reset - reset the timing on a timer
-------------------------------------------------*/

void timer_reset(void *which, double duration)
{
	timer_entry *timer = which;

	/* adjust the timer */
	timer_adjust(timer, duration, timer->callback_param, timer->period);
}


/*-------------------------------------------------
	timer_remove - remove a timer from the system
-------------------------------------------------*/

void timer_remove(void *which)
{
	timer_entry *timer = which;

	/* error if this is an inactive timer */
	if (timer->tag == -1)
	{
		logerror("timer_remove: removed an inactive timer!\n");
		return;
	}

	/* remove it from the list */
	timer_list_remove(timer);

	/* mark it as dead */
	timer->tag = -1;

	/* free it up by adding it back to the free list */
	if (timer_free_tail)
		timer_free_tail->next = timer;
	else
		timer_free_head = timer;
	timer->next = NULL;
	timer_free_tail = timer;

	#if VERBOSE
		verbose_print("T=%.6g: Removed %08X\n", getabsolutetime() + global_offset, (UINT32)timer);
	#endif
}


/*-------------------------------------------------
	timer_free - remove all timers on the current
	resource tag
-------------------------------------------------*/

void timer_free(void)
{
	int tag = get_resource_tag();
	timer_entry *timer, *next;

	/* scan the list */
	for (timer = timer_head; timer != NULL; timer = next)
	{
		/* prefetch the next timer in case we remove this one */
		next = timer->next;

		/* if this tag matches, remove it */
		if (timer->tag == tag)
			timer_remove(timer);
	}
}


/*-------------------------------------------------
	timer_enable - enable/disable a timer
-------------------------------------------------*/

int timer_enable(void *which, int enable)
{
	timer_entry *timer = which;
	int old;

	#if VERBOSE
		if (enable) verbose_print("T=%.6g: Enabled %08X\n", getabsolutetime() + global_offset, (UINT32)timer);
		else verbose_print("T=%.6g: Disabled %08X\n", getabsolutetime() + global_offset, (UINT32)timer);
	#endif

	/* set the enable flag */
	old = timer->enabled;
	timer->enabled = enable;

	/* remove the timer and insert back into the list */
	timer_list_remove(timer);
	timer_list_insert(timer);

	return old;
}


/*-------------------------------------------------
	timer_timeelapsed - return the time since the
	last trigger
-------------------------------------------------*/

double timer_timeelapsed(void *which)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	return time - timer->start;
}


/*-------------------------------------------------
	timer_timeleft - return the time until the
	next trigger
-------------------------------------------------*/

double timer_timeleft(void *which)
{
	double time = getabsolutetime();
	timer_entry *timer = which;

	return timer->expire - time;
}


/*-------------------------------------------------
	timer_get_time - return the current time
-------------------------------------------------*/

double timer_get_time(void)
{
	return global_offset + getabsolutetime();
}


/*-------------------------------------------------
	timer_starttime - return the time when this
	timer started counting
-------------------------------------------------*/

double timer_starttime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->start;
}


/*-------------------------------------------------
	timer_firetime - return the time when this
	timer will fire next
-------------------------------------------------*/

double timer_firetime(void *which)
{
	timer_entry *timer = which;
	return global_offset + timer->expire;
}


/*-------------------------------------------------
	timer_schedule_cpu - begin CPU execution by
	determining how many cycles the CPU should run
-------------------------------------------------*/

int timer_schedule_cpu(int *cpu, int *cycles)
{
	double end;

	/* then see if there are any CPUs that aren't suspended and haven't yet been updated */
	if (pick_cpu(cpu, cycles, timer_head->expire))
		return 1;

	/* everyone is up-to-date; expire any timers now */
	end = timer_head->expire;
	while (timer_head->expire <= end)
	{
		timer_entry *timer = timer_head;
		int was_enabled = timer->enabled;

		/* the base time is now the time of the timer */
		base_time = timer->expire;

		#if VERBOSE
			verbose_print("T=%.6g: %08X fired (exp time=%.6g)\n", getabsolutetime() + global_offset, (UINT32)timer, timer->expire + global_offset);
		#endif

		/* if this is a one-shot timer, disable it now */
		if (timer->period == 0)
			timer->enabled = 0;

		/* set the global state of which callback we're in */
		callback_timer_modified = 0;
		callback_timer = timer;

		/* call the callback */
		if (was_enabled && timer->callback)
		{
			profiler_mark(PROFILER_TIMER_CALLBACK);
			(*timer->callback)(timer->callback_param);
			profiler_mark(PROFILER_END);
		}

		/* clear the callback timer global */
		callback_timer = NULL;

		/* reset or remove the timer, but only if it wasn't modified during the callback */
		if (!callback_timer_modified)
		{
			/* if the timer is temporary, remove it now */
			if (timer->temporary)
				timer_remove(timer);

			/* otherwise, reschedule it */
			else
			{
				timer->start = timer->expire;
				timer->expire += timer->period;

				timer_list_remove(timer);
				timer_list_insert(timer);
			}
		}
	}

	/* reset scheduling so it starts with CPU 0 */
	last_active_cpu = lastcpu;

#ifdef MAME_DEBUG
{
	extern int debug_key_delay;
	debug_key_delay = 0x7ffe;
}
#endif

	/* go back to scheduling */
	return pick_cpu(cpu, cycles, timer_head->expire);
}


/*-------------------------------------------------
	timer_update_cpu - end CPU execution by
	updating the number of cycles the CPU
	actually ran
-------------------------------------------------*/

void timer_update_cpu(int cpunum, int ran)
{
	cpu_entry *cpu = cpudata + cpunum;

	/* update the time if we haven't been suspended */
	if (!cpu->suspended)
	{
		cpu->time += (double)(ran - cpu->lost) * cpu->cycles_to_sec;
		cpu->lost = 0;
	}

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d finished (net=%d)\n", cpu->time + global_offset, cpunum, ran - cpu->lost);
	#endif

	/* time to renormalize? */
	if (cpu->time >= 1.0)
	{
		static const double one = 1.0;
		timer_entry *timer;
		cpu_entry *c;

		#if VERBOSE
			verbose_print("T=%.6g: Renormalizing\n", cpu->time + global_offset);
		#endif

		/* renormalize all the CPU timers */
		for (c = cpudata; c <= lastcpu; c++)
			c->time -= one;

		/* renormalize all the timers' times */
		for (timer = timer_head; timer; timer = timer->next)
		{
			timer->start -= one;
			timer->expire -= one;
		}

		/* renormalize the global timers */
		global_offset += one;
	}

	/* now stop counting cycles */
	base_time = cpu->time;
	active_cpu = NULL;
}


/*-------------------------------------------------
	timer_suspendcpu - suspend a CPU but continue
	to count time for it
-------------------------------------------------*/

void timer_suspendcpu(int cpunum, int suspend, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	int nocount = cpu->nocount;
	int old = cpu->suspended;

	#if VERBOSE
		if (suspend) verbose_print("T=%.6g: Suspending CPU %d\n", getabsolutetime() + global_offset, cpunum);
		else verbose_print("T=%.6g: Resuming CPU %d\n", getabsolutetime() + global_offset, cpunum);
	#endif

	/* mark the CPU */
	if (suspend)
		cpu->suspended |= reason;
	else
		cpu->suspended &= ~reason;
	cpu->nocount = 0;

	/* if this is the active CPU and we're halting, stop immediately */
	if (active_cpu && cpu == active_cpu && !old && cpu->suspended)
	{
		#if VERBOSE
			verbose_print("T=%.6g: Reset ICount\n", getabsolutetime() + global_offset);
		#endif

		/* set the CPU's time to the current time */
		cpu->time = base_time = getabsolutetime();	/* ASG 990225 - also set base_time */
		cpu->lost = 0;

		/* no more instructions */
		if (cpu->burn)
			(*cpu->burn)(*cpu->icount); /* let the CPU burn the cycles */
		else
			*cpu->icount = 0;	/* CPU doesn't care */
    }

	/* else if we're unsuspending a CPU, reset its time */
	else if (old && !cpu->suspended && !nocount)
	{
		double time = getabsolutetime();

		/* only update the time if it's later than the CPU's time */
		if (time > cpu->time)
			cpu->time = time;
		cpu->lost = 0;

		#if VERBOSE
			verbose_print("T=%.6g: Resume time\n", cpu->time + global_offset);
		#endif
	}
}



/*-------------------------------------------------
	timer_holdcpu - hold a CPU and don't count
	time for it
-------------------------------------------------*/

void timer_holdcpu(int cpunum, int hold, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;

	/* same as suspend */
	timer_suspendcpu(cpunum, hold, reason);

	/* except that we don't count time */
	if (hold)
		cpu->nocount = 1;
}



/*-------------------------------------------------
	timer_iscpususpended - query if a CPU is
	suspended or not
-------------------------------------------------*/

int timer_iscpususpended(int cpunum, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	return (cpu->suspended & reason) && !cpu->nocount;
}



/*-------------------------------------------------
	timer_iscpuheld - query if a CPU is held or not
-------------------------------------------------*/

int timer_iscpuheld(int cpunum, int reason)
{
	cpu_entry *cpu = cpudata + cpunum;
	return (cpu->suspended & reason) && cpu->nocount;
}



/*-------------------------------------------------
	timer_suspendcpu_trigger - suspend a CPU until
	a specified trigger condition is met
-------------------------------------------------*/

void timer_suspendcpu_trigger(int cpunum, int trigger)
{
	cpu_entry *cpu = cpudata + cpunum;

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d suspended until %d\n", getabsolutetime() + global_offset, cpunum, trigger);
	#endif

	/* suspend the CPU immediately if it's not already */
	timer_suspendcpu(cpunum, 1, SUSPEND_REASON_TRIGGER);

	/* set the trigger */
	cpu->trigger = trigger;
}



/*-------------------------------------------------
	timer_holdcpu_trigger - hold a CPU and don't
	count time for it
-------------------------------------------------*/

void timer_holdcpu_trigger(int cpunum, int trigger)
{
	cpu_entry *cpu = cpudata + cpunum;

	#if VERBOSE
		verbose_print("T=%.6g: CPU %d held until %d\n", getabsolutetime() + global_offset, cpunum, trigger);
	#endif

	/* suspend the CPU immediately if it's not already */
	timer_holdcpu(cpunum, 1, SUSPEND_REASON_TRIGGER);

	/* set the trigger */
	cpu->trigger = trigger;
}



/*-------------------------------------------------
	timer_trigger - generates a trigger to
	unsuspend any CPUs waiting for it
-------------------------------------------------*/

void timer_trigger(int trigger)
{
	cpu_entry *cpu;

	/* cause an immediate resynchronization */
	if (active_cpu)
	{
		int left = *active_cpu->icount;
		if (left > 0)
		{
			active_cpu->lost += left;
			if (active_cpu->burn)
				(*active_cpu->burn)(left); /* let the CPU burn the cycles */
			else
				*active_cpu->icount = 0; /* CPU doesn't care */
		}
	}

	/* look for suspended CPUs waiting for this trigger and unsuspend them */
	for (cpu = cpudata; cpu <= lastcpu; cpu++)
	{
		if (cpu->suspended && cpu->trigger == trigger)
		{
			#if VERBOSE
				verbose_print("T=%.6g: CPU %d triggered\n", getabsolutetime() + global_offset, cpu->index);
			#endif

			timer_suspendcpu(cpu->index, 0, SUSPEND_REASON_TRIGGER);
			cpu->trigger = 0;
		}
	}
}


/*-------------------------------------------------
	pick_cpu - pick the next CPU to run
-------------------------------------------------*/

static int pick_cpu(int *cpunum, int *cycles, double end)
{
	cpu_entry *cpu = last_active_cpu;

	/* look for a CPU that isn't suspended and hasn't run its full timeslice yet */
	do
	{
		/* wrap around */
		cpu += 1;
		if (cpu > lastcpu)
			cpu = cpudata;

		/* if this CPU is suspended, just skip it */
		if (cpu->suspended)
			;

		/* if this CPU isn't suspended and has time left.... */
		else if (cpu->time < end)
		{
			/* mark the CPU active, and remember the CPU number locally */
			active_cpu = last_active_cpu = cpu;

			/* return the number of cycles to execute and the CPU number */
			*cpunum = cpu->index;
			*cycles = (int)((double)(end - cpu->time) * cpu->sec_to_cycles);

			if (*cycles > 0)
			{
				#if VERBOSE
					verbose_print("T=%.6g: CPU %d runs %d cycles\n", cpu->time + global_offset, *cpunum, *cycles);
				#endif

				/* remember the base time for this CPU */
				base_time = cpu->time + ((double)*cycles * cpu->cycles_to_sec);

				/* success */
				return 1;
			}
		}
	} while (cpu != last_active_cpu);

	/* ASG 990225 - bump all suspended CPU times after the slice has finished */
	for (cpu = cpudata; cpu <= lastcpu; cpu++)
		if (cpu->suspended && !cpu->nocount)
		{
			/* account for the cycles eaten */
			int cycles_to_eat = (int)((double)(end - cpu->time) * cpu->sec_to_cycles);
			cpu_add_to_totalcycles(cpu->index, cycles_to_eat);

			/* bump forward */
			cpu->time = end;
			cpu->lost = 0;
		}

	/* failure */
	return 0;
}
