/***************************************************************************

	formats.h

	Application independent (i.e. - both MESS/imgtool) interfaces for
	representing image formats

***************************************************************************/

#ifndef FORMATS_H
#define FORMATS_H

#include <assert.h>
#include <stdlib.h>
#include "osd_cpu.h"

struct disk_geometry
{
	UINT8 tracks;
	UINT8 heads;
	UINT8 sectors;
	UINT8 first_sector_id;
	UINT16 sector_size;
};

/***************************************************************************

	The format driver itself

***************************************************************************/

enum
{
	/* If this flag is enabled, then images that have the wrong amount of tracks
	 * will be assumed to have simply been truncated.  Otherwise, image files
	 * without enough tracks will be rejected as invalid
	 */
	BDFD_ROUNDUP_TRACKS	= 1
};

#define HEADERSIZE_FLAG_MODULO	0x80000000	/* for when headers are the file size modulo something */
#define HEADERSIZE_FLAGS		0x80000000

struct InternalBdFormatDriver
{
	const char *name;
	const char *humanname;
	const char *extension;
	UINT8 tracks_options[8];
	UINT8 heads_options[8];
	UINT8 sectors_options[2];
	UINT16 bytes_per_sector;
	UINT32 header_size;
	UINT8 filler_byte;
	int (*header_decode)(const void *header, UINT32 file_size, UINT32 header_size, struct disk_geometry *geometry, int *offset);
	int (*header_encode)(void *buffer, UINT32 *header_size, const struct disk_geometry *geometry);
	int (*read_sector)(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
	int (*write_sector)(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
	int (*format_track)(struct InternalBdFormatDriver *drv, void *bdf, const struct disk_geometry *geometry, UINT8 track, UINT8 head);
	void (*get_sector_info)(void *bdf, const void *header, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
	UINT8 (*get_sector_count)(void *bdf, const void *header, UINT8 track, UINT8 head);
	int flags;
};

/***************************************************************************

	Macros for building block device format drivers

***************************************************************************/

typedef void (*formatdriver_ctor)(struct InternalBdFormatDriver *drv);

/* no need to directly call this */
#ifdef MAME_DEBUG
void validate_construct_formatdriver(struct InternalBdFormatDriver *drv, int tracks_optnum, int heads_optnum, int sectors_optnum);
#else
#define validate_construct_formatdriver(drv, tracks_optnum, heads_optnum, sectors_optnum)
#endif

#define BLOCKDEVICE_FORMATDRIVER_START( format_name )												\
	void construct_formatdriver_##format_name(struct InternalBdFormatDriver *drv)					\
	{																								\
		int tracks_optnum = 0, heads_optnum = 0, sectors_optnum = 0;								\
		memset(drv, 0, sizeof(*drv));																\

#define BLOCKDEVICE_FORMATDRIVER_END																\
		if (heads_optnum == 0)	drv->heads_options[heads_optnum++] = 1;								\
		validate_construct_formatdriver(drv, tracks_optnum, heads_optnum, sectors_optnum);			\
	}																								\

#define BLOCKDEVICE_FORMATDRIVER_EXTERN( format_name )												\
	extern void construct_formatdriver_##format_name(struct InternalBdFormatDriver *drv)

#define	BDFD_NAME(name_)							drv->name = name_;
#define	BDFD_HUMANNAME(humanname_)					drv->humanname = humanname_;
#define	BDFD_EXTENSION(ext)							drv->extension = ext;
#define BDFD_TRACKS_OPTION(tracks_option)			drv->tracks_options[tracks_optnum++] = tracks_option;
#define BDFD_HEADS_OPTION(heads_option)				drv->heads_options[heads_optnum++] = heads_option;
#define BDFD_SECTORS_OPTION(sectors_option)			drv->sectors_options[sectors_optnum++] = sectors_option;
#define BDFD_BYTES_PER_SECTOR(bytes_per_sector_)	drv->bytes_per_sector = bytes_per_sector_;
#define BDFD_FLAGS(flags_)							drv->flags = flags_;
#define BDFD_HEADER_SIZE(header_size_)				drv->header_size = (header_size_);
#define BDFD_HEADER_SIZE_MODULO(header_size_)		drv->header_size = (header_size_) | HEADERSIZE_FLAG_MODULO;
#define BDFD_HEADER_ENCODE(header_encode_)			drv->header_encode = header_encode_;
#define BDFD_HEADER_DECODE(header_decode_)			drv->header_decode = header_decode_;
#define BDFD_FILLER_BYTE(filler_byte_)				drv->filler_byte = filler_byte_;
#define BDFD_READ_SECTOR(read_sector_)				drv->read_sector = read_sector_;
#define BDFD_WRITE_SECTOR(write_sector_)			drv->write_sector = write_sector_;
#define BDFD_FORMAT_TRACK(format_track_)			drv->format_track = format_track_;
#define BDFD_GET_SECTOR_INFO(get_sector_info_)		drv->get_sector_info = get_sector_info_;
#define BDFD_GET_SECTOR_COUNT(get_sector_count_)	drv->get_sector_count = get_sector_count_;

/***************************************************************************

	Macros for building block device format choices

***************************************************************************/

#define BLOCKDEVICE_FORMATCHOICES_START( choices_name )			\
	formatdriver_ctor formatchoices_##choices_name[] = {


#define BLOCKDEVICE_FORMATCHOICES_END							\
		NULL };

#define BLOCKDEVICE_FORMATCHOICES_EXTERN( choices_name )		\
	extern formatdriver_ctor formatchoices_##choices_name[]

#define BDFC_CHOICE( format_name)	construct_formatdriver_##format_name,

/**************************************************************************/

struct bdf_procs
{
	void (*closeproc)(void *file);
	int (*seekproc)(void *file, int offset, int whence);
	int (*readproc)(void *file, void *buffer, int length);
	int (*writeproc)(void *file, const void *buffer, int length);
	int (*filesizeproc)(void *file);
};

enum
{
	BLOCKDEVICE_ERROR_SUCCESS,
	BLOCKDEVICE_ERROR_OUTOFMEMORY,
	BLOCKDEVICE_ERROR_CANTDECODEFORMAT,
	BLOCKDEVICE_ERROR_CANTENCODEFORMAT,
	BLOCKDEVICE_ERROR_SECORNOTFOUND,
	BLOCKDEVICE_ERROR_CORRUPTDATA,
	BLOCKDEVICE_ERROR_UNEXPECTEDLENGTH,
	BLOCKDEVICE_ERROR_UNDEFINEERROR
};

int bdf_create(const struct bdf_procs *procs, formatdriver_ctor format,
	void *file, const struct disk_geometry *geometry, void **outbdf);
int bdf_open(const struct bdf_procs *procs, const formatdriver_ctor *formats,
	void *file, int is_readonly, const char *extension, void **outbdf);
void bdf_close(void *bdf);
int bdf_read(void *bdf, void *buffer, int length);
int bdf_write(void *bdf, const void *buffer, int length);
int bdf_seek(void *bdf, int offset);
const struct disk_geometry *bdf_get_geometry(void *bdf);
int bdf_read_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, void *buffer, int length);
int bdf_write_sector(void *bdf, UINT8 track, UINT8 head, UINT8 sector, int offset, const void *buffer, int length);
void bdf_get_sector_info(void *bdf, UINT8 track, UINT8 head, UINT8 sector_index, UINT8 *sector, UINT16 *sector_size);
UINT8 bdf_get_sector_count(void *bdf, UINT8 track, UINT8 head);
int bdf_is_readonly(void *bdf);

#endif /* FORMATS_H */
