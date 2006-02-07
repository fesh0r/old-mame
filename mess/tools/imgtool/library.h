/****************************************************************************

	library.h

	Code relevant to the Imgtool library; analgous to the MESS/MAME driver
	list.

	Unlike MESS and MAME which have static driver lists, Imgtool has a
	concept of a library and this library is built at startup time.
	dynamic for which modules are added to.  This makes "dynamic" modules
	much easier

****************************************************************************/

#ifndef LIBRARY_H
#define LIBRARY_H

#include <time.h>

#include "osd_cpu.h"
#include "opresolv.h"
#include "stream.h"
#include "unicode.h"


typedef struct _imgtool_image imgtool_image;
typedef struct _imgtool_imageenum imgtool_imageenum;
typedef struct _imgtool_library imgtool_library;

typedef enum
{
	SUGGESTION_END,
	SUGGESTION_POSSIBLE,
	SUGGESTION_RECOMMENDED
} imgtool_suggestion_viability_t;

union filterinfo
{
	INT64	i;											/* generic integers */
	void *	p;											/* generic pointers */
	genf *  f;											/* generic function pointers */
	const char *s;										/* generic strings */

	imgtoolerr_t (*read_file)(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf);
	imgtoolerr_t (*write_file)(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts);
	imgtoolerr_t (*check_stream)(imgtool_stream *stream, imgtool_suggestion_viability_t *viability);
};

typedef void (*filter_getinfoproc)(UINT32 state, union filterinfo *info);

typedef enum
{
	ITLS_NAME,
	ITLS_DESCRIPTION
} imgtool_libsort_t;

typedef struct
{
	char filename[1024];
	char attr[64];
	UINT64 filesize;

	time_t creation_time;
	time_t lastmodified_time;

	/* flags */
	unsigned int eof : 1;
	unsigned int corrupt : 1;
	unsigned int directory : 1;
} imgtool_dirent;

typedef struct
{
	UINT8 level;
	UINT64 block;
} imgtool_chainent;

typedef enum
{
	FORK_END,
	FORK_DATA,
	FORK_RESOURCE,
	FORK_ALTERNATE
} imgtool_forktype_t;

typedef struct
{
	imgtool_forktype_t type;
	UINT64 size;
	char forkname[64];
} imgtool_forkent;

typedef struct
{
	imgtool_suggestion_viability_t viability;
	filter_getinfoproc filter;
	const char *fork;
	const char *description;
} imgtool_transfer_suggestion;

enum
{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	IMGTOOLATTR_INT_FIRST = 0x00000,
	IMGTOOLATTR_INT_MAC_TYPE,
	IMGTOOLATTR_INT_MAC_CREATOR,
	IMGTOOLATTR_INT_MAC_FINDERFLAGS,
	IMGTOOLATTR_INT_MAC_COORDX,
	IMGTOOLATTR_INT_MAC_COORDY,
	IMGTOOLATTR_INT_MAC_FINDERFOLDER,
	IMGTOOLATTR_INT_MAC_ICONID,
	IMGTOOLATTR_INT_MAC_SCRIPTCODE,
	IMGTOOLATTR_INT_MAC_EXTENDEDFLAGS,
	IMGTOOLATTR_INT_MAC_COMMENTID,
	IMGTOOLATTR_INT_MAC_PUTAWAYDIRECTORY,

	/* --- the following bits of info are returned as pointers to data or functions --- */
	IMGTOOLATTR_PTR_FIRST = 0x10000,

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	IMGTOOLATTR_STR_FIRST = 0x20000,

	/* --- the following bits of info are returned as time_t values --- */
	IMGTOOLATTR_TIME_FIRST = 0x30000,
	IMGTOOLATTR_TIME_CREATED,
	IMGTOOLATTR_TIME_LASTMODIFIED
};

typedef union
{
	INT64	i;
	time_t	t;
} imgtool_attribute;

typedef struct
{
	unsigned icon16x16_specified : 1;
	unsigned icon32x32_specified : 1;
	UINT32 icon16x16[16][16];
	UINT32 icon32x32[32][32];
} imgtool_iconinfo;

struct ImageModule
{
	struct ImageModule *previous;
	struct ImageModule *next;

	const char *name;
	const char *description;
	const char *extensions;
	const char *eoln;

	size_t image_extra_bytes;
	size_t imageenum_extra_bytes;

	char path_separator;
	char alternate_path_separator;
	
	/* flags */
	unsigned int prefer_ucase : 1;
	unsigned int initial_path_separator : 1;
	unsigned int open_is_strict : 1;
	unsigned int supports_creation_time : 1;
	unsigned int supports_lastmodified_time : 1;
	unsigned int tracks_are_called_cylinders : 1;	/* used for hard drivers */
	unsigned int writing_untested : 1;				/* used when we support writing, but not in main build */
	unsigned int creation_untested : 1;				/* used when we support creation, but not in main build */
	unsigned int supports_bootblock : 1;			/* this module supports loading/storing the boot block */

	imgtoolerr_t	(*open)			(imgtool_image *image, imgtool_stream *f);
	void			(*close)		(imgtool_image *image);
	void			(*info)			(imgtool_image *image, char *string, size_t len);
	imgtoolerr_t	(*begin_enum)	(imgtool_imageenum *enumeration, const char *path);
	imgtoolerr_t	(*next_enum)	(imgtool_imageenum *enumeration, imgtool_dirent *ent);
	void			(*close_enum)	(imgtool_imageenum *enumeration);
	imgtoolerr_t	(*free_space)	(imgtool_image *image, UINT64 *size);
	imgtoolerr_t	(*read_file)	(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *destf);
	imgtoolerr_t	(*write_file)	(imgtool_image *image, const char *filename, const char *fork, imgtool_stream *sourcef, option_resolution *opts);
	imgtoolerr_t	(*delete_file)	(imgtool_image *image, const char *filename);
	imgtoolerr_t	(*list_forks)	(imgtool_image *image, const char *path, imgtool_forkent *ents, size_t len);
	imgtoolerr_t	(*create_dir)	(imgtool_image *image, const char *path);
	imgtoolerr_t	(*delete_dir)	(imgtool_image *image, const char *path);
	imgtoolerr_t	(*list_attrs)	(imgtool_image *image, const char *path, UINT32 *attrs, size_t len);
	imgtoolerr_t	(*get_attrs)	(imgtool_image *image, const char *path, const UINT32 *attrs, imgtool_attribute *values);
	imgtoolerr_t	(*set_attrs)	(imgtool_image *image, const char *path, const UINT32 *attrs, const imgtool_attribute *values);
	imgtoolerr_t	(*attr_name)	(UINT32 attribute, const imgtool_attribute *attr, char *buffer, size_t buffer_len);
	imgtoolerr_t	(*get_iconinfo)	(imgtool_image *image, const char *path, imgtool_iconinfo *iconinfo);
	imgtoolerr_t	(*suggest_transfer)(imgtool_image *image, const char *path, imgtool_transfer_suggestion *suggestions, size_t suggestions_length);
	imgtoolerr_t	(*get_chain)	(imgtool_image *image, const char *path, imgtool_chainent *chain, size_t chain_size);
	imgtoolerr_t	(*create)		(imgtool_image *image, imgtool_stream *f, option_resolution *opts);
	imgtoolerr_t	(*get_sector_size)(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, UINT32 *sector_size);
	imgtoolerr_t	(*read_sector)	(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, void *buffer, size_t len);
	imgtoolerr_t	(*write_sector)	(imgtool_image *image, UINT32 track, UINT32 head, UINT32 sector, const void *buffer, size_t len);
	int				(*approve_filename_char)(unicode_char_t ch);

	const struct OptionGuide *createimage_optguide;
	const char *createimage_optspec;

	const struct OptionGuide *writefile_optguide;
	const char *writefile_optspec;

	const void *extra;
};

/* creates an imgtool library */
imgtool_library *imgtool_library_create(void);

/* closes an imgtool library */
void imgtool_library_close(imgtool_library *library);

/* seeks out and removes a module from an imgtool library */
const struct ImageModule *imgtool_library_unlink(imgtool_library *library,
	const char *module);

/* sorts an imgtool library */
void imgtool_library_sort(imgtool_library *library, imgtool_libsort_t sort);

/* creates an imgtool module; called within module constructors */
imgtoolerr_t imgtool_library_createmodule(imgtool_library *library,
	const char *module_name, struct ImageModule **module);

/* finds a module */
const struct ImageModule *imgtool_library_findmodule(
	imgtool_library *library, const char *module_name);

/* memory allocators for pooled library memory */
void *imgtool_library_alloc(imgtool_library *library, size_t mem);
char *imgtool_library_strdup(imgtool_library *library, const char *s);

struct ImageModule *imgtool_library_iterate(
	imgtool_library *library, const struct ImageModule *module);
struct ImageModule *imgtool_library_index(
	imgtool_library *library, int i);

#endif /* LIBRARY_H */
