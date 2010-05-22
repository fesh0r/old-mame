/*********************************************************************

    formats/d64_dsk.h

    Floppy format code for Commodore 2040/8050 type disk images

*********************************************************************/

#ifndef __D64_DSK__
#define __D64_DSK__

#include "formats/flopimg.h"

/***************************************************************************
    PROTOTYPES
***************************************************************************/

FLOPPY_IDENTIFY( d64_dsk_identify );
FLOPPY_IDENTIFY( d67_dsk_identify );
FLOPPY_IDENTIFY( d71_dsk_identify );
FLOPPY_IDENTIFY( d80_dsk_identify );
FLOPPY_IDENTIFY( d82_dsk_identify );

FLOPPY_CONSTRUCT( d64_dsk_construct );

#endif