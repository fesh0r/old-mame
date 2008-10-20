/***************************************************************************

    sound.h

    Core sound interface functions and definitions.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __SOUND_H__
#define __SOUND_H__

#include "mamecore.h"

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_ROUTES		(16)			/* maximum number of streams of any chip */
#define ALL_OUTPUTS 	(-1)			/* special value indicating all outputs for the current chip */



/***************************************************************************
    MACROS
***************************************************************************/

/* these functions are macros primarily due to include file ordering */
/* plus, they are very simple */
#define speaker_output_count(config)		device_list_items((config)->devicelist, SPEAKER_OUTPUT)
#define speaker_output_first(config)		device_list_first((config)->devicelist, SPEAKER_OUTPUT)
#define speaker_output_next(previous)		device_list_next((previous), SPEAKER_OUTPUT)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* Sound route for the machine driver */
typedef struct _sound_route sound_route;
struct _sound_route
{
	int			output;					/* output ID */
	const char *target;					/* tag of the target */
	float		gain;					/* gain */
	int			input;					/* input ID, -1 is default behavior */
};


/* Sound configuration for the machine driver */
typedef struct _sound_config sound_config;
struct _sound_config
{
	sound_type		type;			/* what type of sound chip? */
	int			clock;					/* clock speed */
	const void *config;					/* configuration for this chip */
	const char *tag;					/* tag for this chip */
	int			routes;					/* number of routes we have */
	sound_route route[MAX_ROUTES];		/* routes for the various streams */
};


/* Speaker configuration for the machine driver */
typedef struct _speaker_config speaker_config;
struct _speaker_config
{
	float		x, y, z;				/* positioning vector */
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* core interfaces */
void sound_init(running_machine *machine);

/* global sound controls */
void sound_mute(int mute);
void sound_set_attenuation(int attenuation);
int sound_get_attenuation(void);
void sound_global_enable(int enable);

/* user gain controls on speaker inputs for mixing */
int sound_get_user_gain_count(running_machine *machine);
void sound_set_user_gain(running_machine *machine, int index, float gain);
float sound_get_user_gain(running_machine *machine, int index);
float sound_get_default_gain(running_machine *machine, int index);
const char *sound_get_user_gain_name(running_machine *machine, int index);

/* misc helpers */
int sound_find_sndnum_by_tag(const char *tag);


/* ----- sound speaker device interface ----- */

/* device get info callback */
#define SPEAKER_OUTPUT DEVICE_GET_INFO_NAME(speaker_output)
DEVICE_GET_INFO( speaker_output );


#endif	/* __SOUND_H__ */
