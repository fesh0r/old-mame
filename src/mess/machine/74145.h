/***************************************************************************

    TTL74145

    BCD-to-Decimal decoder

***************************************************************************/

#ifndef __TTL74145_H__
#define __TTL74145_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _ttl74145_interface ttl74145_interface;
struct _ttl74145_interface
{
	devcb_write_line output_line_0;
	devcb_write_line output_line_1;
	devcb_write_line output_line_2;
	devcb_write_line output_line_3;
	devcb_write_line output_line_4;
	devcb_write_line output_line_5;
	devcb_write_line output_line_6;
	devcb_write_line output_line_7;
	devcb_write_line output_line_8;
	devcb_write_line output_line_9;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( ttl74145 );

/* standard handlers */
WRITE8_DEVICE_HANDLER( ttl74145_w );
READ16_DEVICE_HANDLER( ttl74145_r );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define TTL74145	DEVICE_GET_INFO_NAME(ttl74145)

#define MDRV_TTL74145_ADD(_tag, _intf) \
	MDRV_DEVICE_ADD(_tag, TTL74145, 0) \
	MDRV_DEVICE_CONFIG(_intf)


/***************************************************************************
    DEFAULT INTERFACES
***************************************************************************/

extern const ttl74145_interface default_ttl74145;


#endif /* __TTL74145_H__ */
