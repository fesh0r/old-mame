/***************************************************************************

    driver.c

    Driver construction helpers.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"



/***************************************************************************

    Miscellaneous bits & pieces

***************************************************************************/

/*-------------------------------------------------
    expand_machine_driver - construct a machine
    driver from the macroized state
-------------------------------------------------*/

void expand_machine_driver(void (*constructor)(machine_config *), machine_config *output)
{
	/* keeping this function allows us to pre-init the driver before constructing it */
	memset(output, 0, sizeof(*output));
	(*constructor)(output);
}



/*-------------------------------------------------
    driver_add_cpu - add a CPU during machine
    driver expansion
-------------------------------------------------*/

cpu_config *driver_add_cpu(machine_config *machine, const char *tag, int type, int cpuclock)
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
    driver_find_cpu - find a tagged CPU during
    machine driver expansion
-------------------------------------------------*/

cpu_config *driver_find_cpu(machine_config *machine, const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (machine->cpu[cpunum].tag && strcmp(machine->cpu[cpunum].tag, tag) == 0)
			return &machine->cpu[cpunum];

	logerror("Can't find CPU '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    driver_remove_cpu - remove a tagged CPU
    during machine driver expansion
-------------------------------------------------*/

void driver_remove_cpu(machine_config *machine, const char *tag)
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
    driver_add_speaker - add a speaker during
    machine driver expansion
-------------------------------------------------*/

speaker_config *driver_add_speaker(machine_config *machine, const char *tag, float x, float y, float z)
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
    driver_find_speaker - find a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

speaker_config *driver_find_speaker(machine_config *machine, const char *tag)
{
	int speakernum;

	for (speakernum = 0; speakernum < MAX_SPEAKER; speakernum++)
		if (machine->speaker[speakernum].tag && strcmp(machine->speaker[speakernum].tag, tag) == 0)
			return &machine->speaker[speakernum];

	logerror("Can't find speaker '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    driver_remove_speaker - remove a tagged speaker
    system during machine driver expansion
-------------------------------------------------*/

void driver_remove_speaker(machine_config *machine, const char *tag)
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
    driver_add_sound - add a sound system during
    machine driver expansion
-------------------------------------------------*/

sound_config *driver_add_sound(machine_config *machine, const char *tag, int type, int clock)
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
    driver_find_sound - find a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

sound_config *driver_find_sound(machine_config *machine, const char *tag)
{
	int soundnum;

	for (soundnum = 0; soundnum < MAX_SOUND; soundnum++)
		if (machine->sound[soundnum].tag && strcmp(machine->sound[soundnum].tag, tag) == 0)
			return &machine->sound[soundnum];

	logerror("Can't find sound '%s'!\n", tag);
	return NULL;
}



/*-------------------------------------------------
    driver_remove_sound - remove a tagged sound
    system during machine driver expansion
-------------------------------------------------*/

void driver_remove_sound(machine_config *machine, const char *tag)
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
