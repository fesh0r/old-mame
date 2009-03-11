/***************************************************************************

    IBM-PC printer interface

***************************************************************************/

#ifndef __PC_LPT_H__
#define __PC_LPT_H__

#include "devcb.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pc_lpt_interface pc_lpt_interface;
struct _pc_lpt_interface
{
	devcb_write_line out_irq_func;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( pc_lpt );

READ8_DEVICE_HANDLER( pc_lpt_r );
WRITE8_DEVICE_HANDLER( pc_lpt_w );

READ8_DEVICE_HANDLER( pc_lpt_data_r );
WRITE8_DEVICE_HANDLER( pc_lpt_data_w );
READ8_DEVICE_HANDLER( pc_lpt_status_r );
READ8_DEVICE_HANDLER( pc_lpt_control_r );
WRITE8_DEVICE_HANDLER( pc_lpt_control_w );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define PC_LPT DEVICE_GET_INFO_NAME(pc_lpt)

#define MDRV_PC_LPT_ADD(_tag, _intf) \
	MDRV_DEVICE_ADD(_tag, PC_LPT, 0) \
	MDRV_DEVICE_CONFIG(_intf)

#define MDRV_PC_LPT_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)


#endif /* __PC_LPT__ */
