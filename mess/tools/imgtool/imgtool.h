#ifndef IMGTOOL_H
#define IMGTOOL_H

#include <stdlib.h>
#include <stdio.h>
#include "osdepend.h"
#include "mess.h"

#if !defined(MAX)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Error codes */
enum {
	IMGTOOLERR_OUTOFMEMORY = 1,
	IMGTOOLERR_UNEXPECTED,
	IMGTOOLERR_BUFFERTOOSMALL,
	IMGTOOLERR_READERROR,
	IMGTOOLERR_WRITEERROR,
	IMGTOOLERR_READONLY,
	IMGTOOLERR_CORRUPTIMAGE,
	IMGTOOLERR_FILENOTFOUND,
	IMGTOOLERR_MODULENOTFOUND,
	IMGTOOLERR_UNIMPLEMENTED,
	IMGTOOLERR_PARAMTOOSMALL,
	IMGTOOLERR_PARAMTOOLARGE,
	IMGTOOLERR_PARAMNEEDED,
	IMGTOOLERR_PARAMNOTNEEDED,
	IMGTOOLERR_PARAMCORRUPT,
	IMGTOOLERR_BADFILENAME,
	IMGTOOLERR_NOSPACE,
	IMGTOOLERR_INPUTPASTEND
};

/* These error codes are actually modifiers that make it easier to distinguish
 * the cause of an error
 *
 * Note - drivers should not use these modifiers
 */
#define IMGTOOLERR_SRC_MODULE			0x1000
#define	IMGTOOLERR_SRC_FUNCTIONALITY	0x2000
#define	IMGTOOLERR_SRC_IMAGEFILE		0x3000
#define	IMGTOOLERR_SRC_FILEONIMAGE		0x4000
#define	IMGTOOLERR_SRC_NATIVEFILE		0x5000
#define	IMGTOOLERR_SRC_PARAM_USER		0x6000
#define	IMGTOOLERR_SRC_PARAM_FILE		0x7000
#define	IMGTOOLERR_SRC_PARAM_CREATE		0x8000

#define ERRORCODE(err)		((err) & 0x0fff)
#define ERRORSOURCE(err)	((err) & 0xf000)
#define ERRORPARAM(err)		(((err) & 0xf0000) / 0x10000)

#define PARAM_TO_ERROR(errcode, param)	((errcode) | ((param) * 0x10000))

typedef struct {
	char *fname;
	size_t fname_len;
	char *attr;
	int attr_len;
	int filesize;
	int eof;
	int corrupt;
} imgtool_dirent;

/* ----------------------------------------------------------------------- */

typedef struct {
	int dummy;
} STREAM;

STREAM *stream_open(const char *fname, int read_or_write);	/* similar params to osd_fopen */
STREAM *stream_open_write_stream(int filesize);
STREAM *stream_open_mem(void *buf, size_t sz);
void stream_close(STREAM *f);
size_t stream_read(STREAM *f, void *buf, size_t sz);
size_t stream_write(STREAM *f, const void *buf, size_t sz);
size_t stream_size(STREAM *f);
int stream_seek(STREAM *f, size_t pos, int where);
/* works currently only for IMG_FILE
   clears FILE on harddisk! */
void stream_clear(STREAM *f);
/* truncate with ansi c library not easy to implement */
//void stream_truncate(STREAM *f, size_t newsize);

/* Transfers sz bytes from source to dest */
size_t stream_transfer(STREAM *dest, STREAM *source, size_t sz);
size_t stream_transfer_all(STREAM *dest, STREAM *source);

/* Fills sz bytes with b */
size_t stream_fill(STREAM *f, unsigned char b, size_t sz);

/* Returns the CRC of a file */
int stream_crc(STREAM *f, unsigned long *result);
int file_crc(const char *fname,  unsigned long *result);

/* Returns whether a stream is read only or not */
int stream_isreadonly(STREAM *f);

/* -----------------------------------------------------------------------
 * Filters
 * ----------------------------------------------------------------------- */

struct filter_info {
	int (*sendproc)(struct filter_info *fi, void *buf, int buflen);
	void *filterstate;
	void *filterparam;
	void *internalparam;
};

struct ImageModule;

struct filter_module {
	const char *name;
	const char *humanname;
	void *(*calcreadparam)(const struct ImageModule *imgmod);
	void *(*calcwriteparam)(const struct ImageModule *imgmod);
	int (*filterreadproc)(struct filter_info *fi, void *buf, int buflen);
	int (*filterwriteproc)(struct filter_info *fi, void *buf, int buflen);
	int statesize;
};

typedef struct {
	int dummy;
} FILTER;

typedef const struct filter_module *FILTERMODULE;

enum {
	PURPOSE_READ,
	PURPOSE_WRITE
};

FILTER *filter_init(FILTERMODULE filter, const struct ImageModule *imgmod, int purpose);
void filter_term(FILTER *f);
int filter_writetostream(FILTER *f, STREAM *s, const void *buf, int buflen);
int filter_readfromstream(FILTER *f, STREAM *s, void *buf, int buflen);
int filter_readintobuffer(FILTER *f, STREAM *s);

extern FILTERMODULE filters[];

FILTERMODULE filter_lookup(const char *name);

STREAM *stream_open_filter(STREAM *s, FILTER *f);

/* ----------------------------------------------------------------------- */

typedef struct {
	const struct ImageModule *module;
} IMAGE;

typedef struct {
	const struct ImageModule *module;
} IMAGEENUM;

#define EOLN_CR		"\x0d"
#define EOLN_LF		"\x0a"
#define EOLN_CRLF	"\x0d\x0a"

enum {
	IMGOPTION_FLAG_TYPE_INTEGER	= 0x0000,
	IMGOPTION_FLAG_TYPE_CHAR	= 0x0001,	/* case insensitive; upper case forced */
	IMGOPTION_FLAG_TYPE_STRING	= 0x0002,
	IMGOPTION_FLAG_TYPE_MASK	= 0x000f,

	IMGOPTION_FLAG_HASDEFAULT	= 0x0010
};

struct OptionTemplate {
	const char *name;
	const char *description;
	int flags;
	int min;
	int max;
	const char *defaultvalue;
};

struct NamedOption {
	const char *name;
	const char *value;
};

typedef union {
	int i;			/* integer or char */
	const char *s;	/* string */
} ResolvedOption;

enum {
	IMGMODULE_FLAG_FILENAMES_PREFERUCASE	= 0x0001	/* this means that all filenames are normally upper case */
};

struct ImageModule {
	const char *name;
	const char *humanname;
	const char *fileextension;
	const char *crcfile;
	const char *crcsysname;
	const char *eoln;
	int flags;

	int (*init)(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	void (*info)(IMAGE *img, char *string, const int len);
	int (*beginenum)(IMAGE *img, IMAGEENUM **outenum);
	int (*nextenum)(IMAGEENUM *enumeration, imgtool_dirent *ent);
	void (*closeenum)(IMAGEENUM *enumeration);
	size_t (*freespace)(IMAGE *img);
	int (*readfile)(IMAGE *img, const char *fname, STREAM *destf);
	int (*writefile)(IMAGE *img, const char *fname, STREAM *sourcef, const ResolvedOption *writeoptions);
	int (*deletefile)(IMAGE *img, const char *fname);
	int (*create)(const struct ImageModule *mod, STREAM *f, const ResolvedOption *createoptions);

	/* size must be set with the size of the buffer,
	   if the buffer is too small it will be relocated with realloc */
	int (*read_sector)(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char **buffer, int *size);
	int (*write_sector)(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char *buffer, int size);

	const struct OptionTemplate *fileoptions_template;
	const struct OptionTemplate *createoptions_template;

	void *extra;
};

/* ----------------------------------------------------------------------- */

/* Use IMAGEMODULE for (potentially) full featured images */
#define IMAGEMODULE(name,humanname,ext,crcfile,crcsysname,eoln,flags,\
		init,exit,info,beginenum,nextenum,closeenum,freespace,readfile,writefile, \
		deletefile,create,read_sector,write_sector,fileoptions_template,createoptions_template)	\
struct ImageModule imgmod_##name = \
{								\
	#name,						\
	(humanname),				\
	(ext),						\
	(crcfile),					\
	(crcsysname),				\
	(eoln),						\
	(flags),					\
	(init),						\
	(exit),						\
	(info),						\
	(beginenum),				\
	(nextenum),					\
	(closeenum),				\
	(freespace),				\
	(readfile),					\
	(writefile),				\
	(deletefile),				\
	(create),					\
	(read_sector),				\
	(write_sector),				\
	(fileoptions_template),		\
	(createoptions_template),	\
	NULL			\
};

/* Use CARTMODULE for cartriges (where the only relevant option is CRC checking */
#define CARTMODULE(name,humanname,ext)	\
struct ImageModule imgmod_##name = \
{					\
	#name,			\
	(humanname),	\
	(ext),			\
	(#name ".crc"),	\
	#name,			\
	NULL,			\
	0,				\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL,			\
	NULL			\
};

/* ---------------------------------------------------------------------------
 * Image calls
 *
 * These are the calls that front ends should use for manipulating images. You
 * should never call the module functions directly because they may not be
 * implemented (i.e. - the function pointers are NULL). The img_* functions are
 * aware of these issues and will make the appropriate checks as well as
 * marking up return codes with the source.  In addition, some of the img_*
 * calls are high level calls that simply image manipulation
 *
 * Calls that return 'int' that are not explictly noted otherwise return
 * imgtool error codes
 * ---------------------------------------------------------------------------
 */

/* findimagemodule
 *
 * Description:
 *		Looks up the ImageModule with a certain name
 *
 * Parameters:
 *		name:				The name of the module
 *
 * Returns:
 *		The module with that name.  Returns NULL if the name is unknown
 */
const struct ImageModule *findimagemodule(const char *name);

/* getmodules
 *
 * Description:
 *		Retrieves the list of modules
 *
 * Parameters:
 *		len:				Place to receive the length of the list
 *
 * Returns:
 *		An array of pointers to the modules
 */
const struct ImageModule **getmodules(size_t *len);

/* imageerror
 *
 * Description:
 *		Returns a human readable string
 *
 * Parameters:
 *		err:				The error return code
 */
const char *imageerror(int err);

/* img_open
 * img_open_byname
 *
 * Description:
 *		Opens an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		read_or_write:		Open mode (use OSD_FOPEN_* constants)
 *		outimg:				Placeholder for image pointer
 */
int img_open(const struct ImageModule *module, const char *fname, int read_or_write, IMAGE **outimg);
int img_open_byname(const char *modulename, const char *fname, int read_or_write, IMAGE **outimg);

/* img_close
 *
 * Description:
 *		Closes an image
 *
 * Parameters:
 *		img:				The image to close
 */
void img_close(IMAGE *img);

/* img_info
 *
 * Description:
 *		Returns format specific information about an image
 *
 * Parameters:
 *		img:				The image to query the info of
 *		string:				Buffer to place info in
 *		len:				Length of buffer
 */
int img_info(IMAGE *img, char *string, const int len);

/* img_beginenum
 *
 * Description:
 *		Begins enumerating files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		outenum:			The resulting enumeration
 */
int img_beginenum(IMAGE *img, IMAGEENUM **outenum);

/* img_nextenum
 *
 * Description:
 *		Continues enumerating files within an image
 *
 * Parameters:
 *		enumeration:		The enumeration
 *		ent:				Place to receive directory entry
 */
int img_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);

/* img_countfiles
 *
 * Description:
 *		Counts the total amount of files within an image
 *
 * Parameters:
 *		img:				The image to enumerate
 *		totalfiles:			Place to receive the file count
 */
int img_countfiles(IMAGE *img, int *totalfiles);

/* img_filesize
 *
 * Description:
 *		Retrieves the file length of the specified file
 *
 * Parameters:
 *		img:				The image to enumerate
 *		fname				Filename of file on the image
 *		filesize			Place to receive the file length
 */
int img_filesize(IMAGE *img, const char *fname, int *filesize);

/* img_closeenum
 *
 * Description:
 *		Closes an enumeration
 *
 * Parameters:
 *		enumeration:		The enumeration to close
 */
void img_closeenum(IMAGEENUM *enumeration);

/* img_freespace
 *
 * Description:
 *		Returns free space on an image, in bytes
 *
 * Parameters:
 *		img:				The image to query
 *		sz					Place to receive free space
 */
int img_freespace(IMAGE *img, int *sz);

/* img_readfile
 *
 * Description:
 *		Start reading from a file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 *      filter:             Filter to use, or NULL if none
 */
int img_readfile(IMAGE *img, const char *fname, STREAM *destf,
	FILTERMODULE filter);

/* img_writefile
 *
 * Description:
 *		Start writing to a new file on an image with a stream
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		destf:				Place to receive the stream
 *		options:			Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
int img_writefile(IMAGE *img, const char *fname, STREAM *sourcef,
	const struct NamedOption *_options, FILTERMODULE filter);

/* img_getfile
 *
 * Description:
 *		Read a file from an image, storing it into a native file
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 *		dest:				Filename for native file to write to
 *      filter:             Filter to use, or NULL if none
 */
int img_getfile(IMAGE *img, const char *fname, const char *dest,
	FILTERMODULE filter);

/* img_putfile
 *
 * Description:
 *		Read a native file and store it on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		newfname:			The filename on the image to store (if NULL, then
 *							the file will be named basename(source)
 *		source:				Native filename for source
 *		options:			Options to specify on the new file
 *      filter:             Filter to use, or NULL if none
 */
int img_putfile(IMAGE *img, const char *newfname, const char *source,
	const struct NamedOption *_options, FILTERMODULE filter);

/* img_deletefile
 *
 * Description:
 *		Delete a file on an image
 *
 * Parameters:
 *		img:				The image to read from
 *		fname:				The filename on the image
 */
int img_deletefile(IMAGE *img, const char *fname);

/* img_create
 * img_create_byname
 *
 * Description:
 *		Creates an image
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		options:			Options that control how the image is created
 *							(tracks, sectors, etc)
 */
int img_create(const struct ImageModule *module, const char *fname, const struct NamedOption *_options);
int img_create_byname(const char *modulename, const char *fname, const struct NamedOption *_options);

typedef struct {
	unsigned long crc;
	char *longname;
	char *manufacturer;
	int year;
	char *playable;
	char *extrainfo;
	char buffer[1024];
} imageinfo;

/* img_getinfo
 * img_getinfo_byname
 *
 * Description:
 *		Retrieves information about an image from the CRC databases
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		info				Place to receive information about the image
 */
int img_getinfo(const struct ImageModule *module, const char *fname, imageinfo *info);
int img_getinfo_byname(const char *modulename, const char *fname, imageinfo *info);

/* img_goodname
 * img_goodname_byname
 *
 * Description:
 *		Figures out what the "Good" name of a particular image is
 *
 * Parameters:
 *		module/modulename:	The module for this image format
 *		fname:				The native filename for the image
 *		base:				Arbitrary string to prefix the good name with
 *		result:				Placeholder to receive allocated string (i.e. you
 *							must call free() with the "Good" name)
 */
int img_goodname(const struct ImageModule *module, const char *fname, const char *base, char **result);
int img_goodname_byname(const char *modulename, const char *fname, const char *base, char **result);

/* imgtool_test
 * imgtool_test_byname
 *
 * Description:
 *		(Only present when MAME_DEBUG is on)
 *		These functions run a test suite on the module
 *
 * Parameters:
 *		module/modulename:	The module for this image format.  If NULL, tests are run on all modules
 */
#ifdef MAME_DEBUG
int imgtool_test(const struct ImageModule *module);
int imgtool_test_byname(const char *modulename);
#endif /* MAME_DEBUG */

/* ---------------------------------------------------------------------------
 * Wave/Cassette calls
 * ---------------------------------------------------------------------------
 */

enum {
	WAVEIMAGE_LSB_FIRST = 0,
	WAVEIMAGE_MSB_FIRST = 1
};

struct WaveExtra
{
	int (*initalt)(STREAM *instream, STREAM **outstream, int *basepos, int *length, int *channels, int *frequency, int *resolution);
	int (*nextfile)(IMAGE *img, imgtool_dirent *ent);
	int (*readfile)(IMAGE *img, STREAM *destf);
	int zeropulse;
	int threshpulse;
	int onepulse;
	int waveflags;
	const UINT8 *blockheader;
	int blockheadersize;

};

#define WAVEMODULE(name,humanname,ext,eoln,flags,zeropulse,onepulse,threshpulse,waveflags,blockheader,blockheadersize,\
		initalt,nextfile,readfilechunk)	\
static struct WaveExtra waveextra_##name = \
{						\
	(initalt),			\
	(nextfile),			\
	(readfilechunk),	\
	(zeropulse),		\
	(onepulse),			\
	(threshpulse),		\
	(waveflags),		\
	(blockheader),		\
	(blockheadersize),	\
}; \
struct ImageModule imgmod_##name = \
{						\
	#name,				\
	(humanname),		\
	(ext),				\
	NULL,				\
	NULL,				\
	(eoln),				\
	(flags),			\
	imgwave_init,		\
	imgwave_exit,		\
	NULL,				\
	imgwave_beginenum,	\
	imgwave_nextenum,	\
	imgwave_closeenum,	\
	NULL,				\
	imgwave_readfile,	\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	NULL,				\
	(void *) &waveextra_##name \
};

/* These are called internally */
int imgwave_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
void imgwave_exit(IMAGE *img);
int imgwave_beginenum(IMAGE *img, IMAGEENUM **outenum);
int imgwave_nextenum(IMAGEENUM *enumeration, imgtool_dirent *ent);
void imgwave_closeenum(IMAGEENUM *enumeration);
int imgwave_readfile(IMAGE *img, const char *fname, STREAM *destf);

/* These are callable from wave modules */
int imgwave_seek(IMAGE *img, int pos);
int imgwave_forward(IMAGE *img);
int imgwave_read(IMAGE *img, UINT8 *buf, int bufsize);

/* ---------------------------------------------------------------------------
 * Floppy calls
 * ---------------------------------------------------------------------------
 */

struct FloppyFormat
{
	int (*init)(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, IMAGE **outimg);
	void (*exit)(IMAGE *img);
	int (*create)(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, UINT8 sides, UINT8 tracks, UINT8 filler);
	int (*readdata)(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf);
	int (*writedata)(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf);
	void (*get_geometry)(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors);
	int (*isreadonly)(IMAGE *img);
	void *extra;
};

#define FLOPPYFORMAT(name, init,exit,create,readdata,writedata) \
static const struct FloppyFormat imgfloppyfmt_##name = \
{\
	(init), \
	(exit), \
	(create), \
	(readdata), \
	(writedata), \
	NULL \
};

struct BasicDskExtra
{
	int (*resolvegeometry)(STREAM *f, UINT8 *tracks, UINT8 *sides, UINT8 *sec_per_track, UINT16 *sector_length, UINT8 *first_sector_id, int *offset);
	int (*createheader)(STREAM *f, UINT8 sides, UINT8 tracks, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id);
	UINT8 sec_per_track;
	UINT8 first_sector_id;
	UINT16 sector_length;
};

#define BASICDSKFORMAT(name, resolvegeometry,createheader,sec_per_track,sector_length,first_sector_id) \
static const struct BasicDskExtra imgfloppyfmt_basicdsk_##name = \
{\
	(resolvegeometry),\
	(createheader),\
	(sec_per_track),\
	(first_sector_id),\
	(sector_length)\
};\
static const struct FloppyFormat imgfloppyfmt_##name = \
{\
	imgfloppy_basicdsk_init, \
	imgfloppy_basicdsk_exit, \
	imgfloppy_basicdsk_create, \
	imgfloppy_basicdsk_readdata, \
	imgfloppy_basicdsk_writedata, \
	imgfloppy_basicdsk_get_geometry, \
	imgfloppy_basicdsk_isreadonly, \
	(void *) &imgfloppyfmt_basicdsk_##name \
};

struct FloppyExtra
{
	const struct FloppyFormat *flopformat;
	UINT8 filler;
};

#define FLOPPYMODULE(name,humanname,ext,crcfile,crcsysname,eoln,flags,\
		info,beginenum,nextenum,closeenum,freespace,readfile,writefile, \
		deletefile,fileoptions_template, \
		floppyformat,maxsides,mintracks,maxtracks,deftracks,filler) \
\
static const struct OptionTemplate imgfloppy_##name##_copts[] = \
{\
	{ "tracks",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	mintracks,	maxtracks,	NULL},	\
	{ "sides",	NULL, IMGOPTION_FLAG_TYPE_INTEGER,	1,			maxsides,	NULL},	\
	{ NULL, NULL, 0, 0, 0, 0 } \
};\
static const struct FloppyExtra imgfloppy_##name##_extra = \
{\
	&imgfloppyfmt_##floppyformat, \
	filler \
};\
struct ImageModule imgmod_##name = \
{								\
	#name,						\
	(humanname),				\
	(ext),						\
	(crcfile),					\
	(crcsysname),				\
	(eoln),						\
	(flags),					\
	imgfloppy_init,				\
	imgfloppy_exit,				\
	(info),						\
	(beginenum),				\
	(nextenum),					\
	(closeenum),				\
	(freespace),				\
	(readfile),					\
	(writefile),				\
	(deletefile),				\
	imgfloppy_create,			\
	imgfloppy_readsector,		\
	imgfloppy_writesector,		\
	(fileoptions_template),		\
	imgfloppy_##name##_copts,	\
	(void *) &imgfloppy_##name##_extra	\
};

/* These are called internally */
int imgfloppy_init(const struct ImageModule *mod, STREAM *f, IMAGE **outimg);
void imgfloppy_exit(IMAGE *img);
int imgfloppy_readsector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char **buffer, int *size);
int imgfloppy_writesector(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, char *buffer, int size);
int imgfloppy_create(const struct ImageModule *mod, STREAM *f, const ResolvedOption *createoptions);
int imgfloppy_basicdsk_init(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, IMAGE **outimg);
void imgfloppy_basicdsk_exit(IMAGE *img);
int imgfloppy_basicdsk_create(const struct ImageModule *mod, const struct FloppyFormat *flopformat, STREAM *f, UINT8 sides, UINT8 tracks, UINT8 filler);
int imgfloppy_basicdsk_readdata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf);
int imgfloppy_basicdsk_writedata(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf);
void imgfloppy_basicdsk_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors);
int imgfloppy_basicdsk_isreadonly(IMAGE *img);

#define IMGFLOP_CREATEOPTION_TRACKS			0
#define IMGFLOP_CREATEOPTION_SIDES			1

/* These are callable from floppy modules */
int imgfloppy_read(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, UINT8 *buf);
int imgfloppy_write(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, const UINT8 *buf);
int imgfloppy_transfer_from(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, STREAM *destf);
int imgfloppy_transfer_to(IMAGE *img, UINT8 head, UINT8 track, UINT8 sector, int offset, int length, STREAM *destf);
void imgfloppy_get_geometry(IMAGE *img, UINT8 *sides, UINT8 *tracks, UINT8 *sectors);
int imgfloppy_isreadonly(IMAGE *img);

#endif /* IMGTOOL_H */
