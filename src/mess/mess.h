/*********************************************************************

	mess.h

	Core MESS headers

*********************************************************************/

#ifndef MESS_H
#define MESS_H

#include <stdarg.h>

struct SystemConfigurationParamBlock;

#include "image.h"
#include "artworkx.h"
#include "memory.h"
#include "compcfg.h"
#include "configms.h"
#include "messopts.h"



/***************************************************************************

	Constants

***************************************************************************/

#define LCD_FRAMES_PER_SECOND	30



/**************************************************************************/

/* MESS_DEBUG is a debug switch (for developers only) for
   debug code, which should not be found in distributions, like testdrivers,...
   contrary to MAME_DEBUG, NDEBUG it should not be found in the makefiles of distributions
   use it in your private root makefile */
/* #define MESS_DEBUG */

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

/***************************************************************************/

extern const char mess_disclaimer[];

UINT32 hash_data_extract_crc32(const char *d);



/***************************************************************************/

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* runs checks to see if device code is proper */
int mess_validitychecks(void);

/* these are called from mame.c */
void mess_predevice_init(running_machine *machine);
void mess_postdevice_init(running_machine *machine);

enum
{
	OSD_FOPEN_READ,
	OSD_FOPEN_WRITE,
	OSD_FOPEN_RW,
	OSD_FOPEN_RW_CREATE
};

/* --------------------------------------------------------------------------------------------- */

/* This call is used to return the next compatible driver with respect to
 * software images.  It is usable both internally and from front ends
 */
const game_driver *mess_next_compatible_driver(const game_driver *drv);
int mess_count_compatible_drivers(const game_driver *drv);

/* --------------------------------------------------------------------------------------------- */

/* RAM configuration calls */
extern UINT32 mess_ram_size;
extern UINT8 *mess_ram;
extern UINT8 mess_ram_default_value;

#define RAM_STRING_BUFLEN 16
void		ram_dump(const char *filename);

/* --------------------------------------------------------------------------------------------- */

/* dummy read handlers */
READ8_HANDLER(return8_FE);
READ8_HANDLER(return8_FF);
READ16_HANDLER(return16_FFFF);

#endif
