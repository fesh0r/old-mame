/*********************************************************************

    mess.h

    Core MESS headers

*********************************************************************/

#ifndef __MESS_H__
#define __MESS_H__

#include "configms.h"
#include "messopts.h"



/***************************************************************************

    Constants

***************************************************************************/

#define LCD_FRAMES_PER_SECOND	30

/**************************************************************************/

/* mess specific layout files */
extern const char layout_lcd[];	/* generic 1:1 lcd screen layout */
extern const char layout_lcd_rot[];	/* same, for use with ROT90 or ROT270 */


/***************************************************************************/

extern const char mess_disclaimer[];

/***************************************************************************/

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* these are called from mame.c */
void mess_predevice_init(running_machine *machine);
void mess_postdevice_init(running_machine *machine);

#endif /* __MESS_H__ */
