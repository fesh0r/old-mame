#include <stdarg.h>

#ifndef MESS_H
#define MESS_H

#include "osdepend.h"
#include "device.h"
#include "driver.h"
#include "image.h"

#define MAX_DEV_INSTANCES 5

#define LCD_FRAMES_PER_SECOND	30

/* MESS_DEBUG is a debug switch (for developers only) for
   debug code, which should not be found in distributions, like testdrivers,...
   contrary to MAME_DEBUG, NDEBUG it should not be found in the makefiles of distributions
   use it in your private root makefile */
/* #define MESS_DEBUG */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
	#include <stdbool.h>
#elif (! defined(__bool_true_false_are_defined)) && (! defined(__cplusplus))
	#ifndef bool
		#define bool int
	#endif
	#ifndef true
		#define true 1
	#endif
	#ifndef false
		#define false 0
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Win32 defines this for vararg functions */
#ifndef DECL_SPEC
#define DECL_SPEC
#endif

int mess_printf(const char *fmt, ...);

void showmessinfo(void);
int filemanager(struct mame_bitmap *bitmap, int selected);

#if HAS_WAVE
int tapecontrol(struct mame_bitmap *bitmap, int selected);
void tapecontrol_gettime(char *timepos, size_t timepos_size, int id, int *curpos, int *endpos);
#endif

/* driver.h - begin */
#define IPT_SELECT1		IPT_COIN1
#define IPT_SELECT2		IPT_COIN2
#define IPT_SELECT3		IPT_COIN3
#define IPT_SELECT4		IPT_COIN4
#define IPT_KEYBOARD	IPT_TILT
/* driver.h - end */

/* IODevice Initialisation return values.  Use these to determine if */
/* the emulation can continue if IODevice initialisation fails */
#define INIT_PASS 0
#define INIT_FAIL 1
#define IMAGE_VERIFY_PASS 0
#define IMAGE_VERIFY_FAIL 1

/* possible values for mame_fopen() last argument:
 * OSD_FOPEN_READ
 *	open existing file in read only mode.
 *	ZIP images can be opened only in this mode, unless
 *	we add support for writing into ZIP files.
 * OSD_FOPEN_WRITE
 *	open new file in write only mode (truncate existing file).
 *	used for output images (eg. a cassette being written).
 * OSD_FOPEN_RW
 *	open existing(!) file in read/write mode.
 *	used for floppy/harddisk images. if it fails, a driver
 *	might try to open the image with OSD_FOPEN_READ and set
 *	an internal 'write protect' flag for the FDC/HDC emulation.
 * OSD_FOPEN_RW_CREATE
 *	open existing file or create new file in read/write mode.
 *	used for floppy/harddisk images. if a file doesn't exist,
 *	it shall be created. Used to 'format' new floppy or harddisk
 *	images from within the emulation. A driver might use this
 *	if both, OSD_FOPEN_RW and OSD_FOPEN_READ modes, failed.
 *
 * extra values for IODevice openmode field (modes are not supported by
 * mame_fopen() yet, image_fopen_new emulates them):
 * OSD_FOPEN_RW_OR_READ
 *  open existing file in read/write mode.  If it fails, try to open it as
 *  read-only.
 * OSD_FOPEN_RW_CREATE_OR_READ
 *  open existing file in read/write mode.  If it fails, try to open it as
 *  read-only.  If it fails, try to create a new R/W image
 * OSD_FOPEN_READ_OR_WRITE
 *  open existing file in read-only mode if it exists.  If it does not, open
 *  the file as write-only.  (used by wave.c)
 * OSD_FOPEN_NONE
 *  Leaves the open mode undefined: implies that image_fopen_custom() will be
 *  called with the proper open mode (this style is deprecated and not
 *  recommended, though it may still prove useful in some rare cases).
 */
enum
{
	OSD_FOPEN_READ, OSD_FOPEN_WRITE, OSD_FOPEN_RW, OSD_FOPEN_RW_CREATE,
	OSD_FOPEN_RW_OR_READ, OSD_FOPEN_RW_CREATE_OR_READ, OSD_FOPEN_READ_OR_WRITE,

	OSD_FOPEN_NONE = -1
};

#define is_effective_mode_writable(mode) ((mode) != OSD_FOPEN_READ)
#define is_effective_mode_create(mode) (((mode) == OSD_FOPEN_RW_CREATE) || ((mode) == OSD_FOPEN_WRITE))

enum
{
	DEVICE_LOAD_RESETS_NONE	= 0,	/* changing the device file doesn't reset anything */
	DEVICE_LOAD_RESETS_CPU	= 1,	/* changing the device file resets the CPU */
	DEVICE_MUST_BE_LOADED	= 2,
	DEVICE_LOAD_AT_INIT		= 4
};

#ifdef MAME_DEBUG
/* runs checks to see if device code is proper */
int messvaliditychecks(void);

/* runs a set of test cases on the driver; can pass in an optional callback
 * to provide a way to identify images to test with
 */
void messtestdriver(const struct GameDriver *gamedrv, const char *(*getfodderimage)(unsigned int index, int *foddertype));
#endif

/* these are called from mame.c*/
int devices_init(const struct GameDriver *gamedrv);
int devices_initialload(const struct GameDriver *gamedrv, int ispreload);
void devices_exit(void);

/* access mess.c internal fields for a device type (instance id) */
extern int			device_count(int type);
extern const char  *device_typename(int type);
extern const char  *device_brieftypename(int type);
extern int          device_typeid(const char *name);
extern const char  *device_typename_id(int type, int id);
extern const char  *device_file_extension(int type, int extnum);

/* access functions from the struct IODevice arrays of a driver */
extern const void *device_info(int type, int id);

const struct IODevice *device_first(const struct GameDriver *gamedrv);
const struct IODevice *device_next(const struct GameDriver *gamedrv, const struct IODevice *dev);
const struct IODevice *device_find(const struct GameDriver *gamedrv, int type);

/* functions to load and save battery backed NVRAM */
int battery_load(const char *filename, void *buffer, int length);
int battery_save(const char *filename, void *buffer, int length);

/* handy wrapper for palette_set_color */
void palette_set_colors(pen_t color_base, const UINT8 *colors, int color_count);

/* RAM configuration calls */
#define RAM_STRING_BUFLEN 16
extern UINT32 mess_ram_size;
extern UINT8 *mess_ram;
extern UINT32 ram_option(const struct GameDriver *gamedrv, unsigned int i);
extern int ram_option_count(const struct GameDriver *gamedrv);
extern int ram_is_valid_option(const struct GameDriver *gamedrv, UINT32 ram);
extern UINT32 ram_default(const struct GameDriver *gamedrv);
extern UINT32 ram_parse_string(const char *s);
extern const char *ram_string(char *buffer, UINT32 ram);
extern int ram_validate_option(void);
extern void cpu_setbank_fromram(int bank, UINT32 ramposition, mem_read_handler rhandler, mem_write_handler whandler);

extern void ram_dump(const char *filename);

/* gets the path to the MESS executable */
extern const char *mess_path;

#ifdef __cplusplus
}
#endif

#endif
