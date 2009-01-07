/*********************************************************************

	cassette.h

	MESS interface to the cassette image abstraction code

*********************************************************************/

#ifndef CASSETTE_H
#define CASSETTE_H

#include "device.h"
#include "image.h"
#include "formats/cassimg.h"


typedef enum
{
	/* this part of the state is controlled by the UI */
	CASSETTE_STOPPED			= 0,
	CASSETTE_PLAY				= 1,
	CASSETTE_RECORD				= 2,

	/* this part of the state is controlled by drivers */
	CASSETTE_MOTOR_ENABLED		= 0,
	CASSETTE_MOTOR_DISABLED		= 4,
	CASSETTE_SPEAKER_ENABLED	= 0,
	CASSETTE_SPEAKER_MUTED		= 8,

	/* masks */
	CASSETTE_MASK_UISTATE		= 3,
	CASSETTE_MASK_MOTOR			= 4,
	CASSETTE_MASK_SPEAKER		= 8,
	CASSETTE_MASK_DRVSTATE		= 12
} cassette_state;


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct cassette_config_t	cassette_config;
struct cassette_config_t
{
	const struct CassetteFormat* 	const *formats;
	const struct CassetteOptions	*create_opts;
	const cassette_state			default_state;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

cassette_state cassette_get_state(const device_config *cassette);
void cassette_set_state(const device_config *cassette, cassette_state state);
void cassette_change_state(const device_config *cassette, cassette_state state, cassette_state mask);

double cassette_input(const device_config *cassette);
void cassette_output(const device_config *cassette, double value);

cassette_image *cassette_get_image(const device_config *cassette);
double cassette_get_position(const device_config *cassette);
double cassette_get_length(const device_config *cassette);
void cassette_seek(const device_config *cassette, double time, int origin);

#define CASSETTE	DEVICE_GET_INFO_NAME(cassette)
DEVICE_GET_INFO(cassette);

/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MDRV_CASSETTE_ADD(_tag, _config) 	\
	MDRV_DEVICE_ADD(_tag, CASSETTE, 0)			\
	MDRV_DEVICE_CONFIG(_config)

#define MDRV_CASSETTE_REMOVE(_tag)			\
	MDRV_DEVICE_REMOVE(_tag, CASSETTE)

#define MDRV_CASSETTE_MODIFY(_tag, _config)	\
	MDRV_DEVICE_MODIFY(_tag, CASSETTE)		\
	MDRV_DEVICE_CONFIG(_config)

extern const cassette_config default_cassette_config;

#endif /* CASSETTE_H */
