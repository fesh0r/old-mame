/***************************************************************************

    mconfig.h

    Machine configuration macros and functions.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __MCONFIG_H__
#define __MCONFIG_H__

#include "devintrf.h"
#include <stddef.h>


/***************************************************************************
    CONSTANTS
***************************************************************************/

/* token types */
enum
{
	MCONFIG_TOKEN_INVALID,
	MCONFIG_TOKEN_END,
	MCONFIG_TOKEN_INCLUDE,

	MCONFIG_TOKEN_DEVICE_ADD,
	MCONFIG_TOKEN_DEVICE_REMOVE,
	MCONFIG_TOKEN_DEVICE_MODIFY,
	MCONFIG_TOKEN_DEVICE_CONFIG,
	MCONFIG_TOKEN_DEVICE_CONFIG_DATA32,
	MCONFIG_TOKEN_DEVICE_CONFIG_DATA64,
	MCONFIG_TOKEN_DEVICE_CONFIG_DATAFP32,
	MCONFIG_TOKEN_DEVICE_CONFIG_DATAFP64,

	MCONFIG_TOKEN_CPU_ADD,
	MCONFIG_TOKEN_CPU_MODIFY,
	MCONFIG_TOKEN_CPU_REMOVE,
	MCONFIG_TOKEN_CPU_REPLACE,
	MCONFIG_TOKEN_CPU_FLAGS,
	MCONFIG_TOKEN_CPU_CONFIG,
	MCONFIG_TOKEN_CPU_PROGRAM_MAP,
	MCONFIG_TOKEN_CPU_DATA_MAP,
	MCONFIG_TOKEN_CPU_IO_MAP,
	MCONFIG_TOKEN_CPU_VBLANK_INT,
	MCONFIG_TOKEN_CPU_PERIODIC_INT,

	MCONFIG_TOKEN_DRIVER_DATA,
	MCONFIG_TOKEN_INTERLEAVE,
	MCONFIG_TOKEN_WATCHDOG_VBLANK,
	MCONFIG_TOKEN_WATCHDOG_TIME,

	MCONFIG_TOKEN_MACHINE_START,
	MCONFIG_TOKEN_MACHINE_RESET,
	MCONFIG_TOKEN_NVRAM_HANDLER,
	MCONFIG_TOKEN_MEMCARD_HANDLER,

	MCONFIG_TOKEN_VIDEO_ATTRIBUTES,
	MCONFIG_TOKEN_GFXDECODE,
	MCONFIG_TOKEN_PALETTE_LENGTH,
	MCONFIG_TOKEN_DEFAULT_LAYOUT,

	MCONFIG_TOKEN_PALETTE_INIT,
	MCONFIG_TOKEN_VIDEO_START,
	MCONFIG_TOKEN_VIDEO_RESET,
	MCONFIG_TOKEN_VIDEO_EOF,
	MCONFIG_TOKEN_VIDEO_UPDATE,

	MCONFIG_TOKEN_SOUND_START,
	MCONFIG_TOKEN_SOUND_RESET,

	MCONFIG_TOKEN_SOUND_ADD,
	MCONFIG_TOKEN_SOUND_REMOVE,
	MCONFIG_TOKEN_SOUND_MODIFY,
	MCONFIG_TOKEN_SOUND_CONFIG,
	MCONFIG_TOKEN_SOUND_REPLACE,
	MCONFIG_TOKEN_SOUND_ROUTE,
};


/* ----- flags for video_attributes ----- */

/* should VIDEO_UPDATE by called at the start of VBLANK or at the end? */
#define	VIDEO_UPDATE_BEFORE_VBLANK		0x0000
#define	VIDEO_UPDATE_AFTER_VBLANK		0x0004

/* indicates VIDEO_UPDATE will add container bits its */
#define VIDEO_SELF_RENDER				0x0008

/* automatically extend the palette creating a darker copy for shadows */
#define VIDEO_HAS_SHADOWS				0x0010

/* automatically extend the palette creating a brighter copy for highlights */
#define VIDEO_HAS_HIGHLIGHTS			0x0020

/* Mish 181099:  See comments in video/generic.c for details */
#define VIDEO_BUFFERS_SPRITERAM			0x0040

/* force VIDEO_UPDATE to be called even for skipped frames */
#define VIDEO_ALWAYS_UPDATE				0x0080

/* calls VIDEO_UPDATE for every visible scanline, even for skipped frames */
#define VIDEO_UPDATE_SCANLINE			0x0100



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* In mamecore.h: typedef struct _machine_config machine_config; */
struct _machine_config
{
	UINT32				driver_data_size;			/* amount of memory needed for driver_data */

	cpu_config			cpu[MAX_CPU];				/* array of CPUs in the system */
	UINT32				cpu_slices_per_frame;		/* number of times to interleave execution per frame */
	INT32				watchdog_vblank_count;		/* number of VBLANKs until the watchdog kills us */
	attotime			watchdog_time;				/* length of time until the watchdog kills us */

	void 				(*machine_start)(running_machine *machine);		/* one-time machine start callback */
	void 				(*machine_reset)(running_machine *machine);		/* machine reset callback */

	void 				(*nvram_handler)(running_machine *machine, mame_file *file, int read_or_write); /* NVRAM save/load callback  */
	void 				(*memcard_handler)(running_machine *machine, mame_file *file, int action); /* memory card save/load callback  */

	UINT32				video_attributes;			/* flags describing the video system */
	const gfx_decode_entry *gfxdecodeinfo;			/* pointer to array of graphics decoding information */
	UINT32				total_colors;				/* total number of colors in the palette */
	const char *		default_layout;				/* default layout for this machine */

	void 				(*init_palette)(running_machine *machine, const UINT8 *color_prom); /* one-time palette init callback  */
	void				(*video_start)(running_machine *machine);		/* one-time video start callback */
	void				(*video_reset)(running_machine *machine);		/* video reset callback */
	void				(*video_eof)(running_machine *machine);			/* end-of-frame video callback */
	UINT32				(*video_update)(running_machine *machine, int screen, mame_bitmap *bitmap, const rectangle *cliprect); /* video update callback */

	sound_config		sound[MAX_SOUND];			/* array of sound chips in the system */

	void				(*sound_start)(running_machine *machine);		/* one-time sound start callback */
	void				(*sound_reset)(running_machine *machine);		/* sound reset callback */

	device_config *		devicelist;					/* list head for devices */
};



/***************************************************************************
    MACROS FOR BUILDING MACHINE DRIVERS
***************************************************************************/

/* this type is used to encode machine configuration definitions */
typedef union _machine_config_token machine_config_token;
union _machine_config_token
{
	TOKEN_COMMON_FIELDS
	const machine_config_token *tokenptr;
	const gfx_decode_entry *gfxdecode;
	device_type devtype;
	void (*interrupt)(running_machine *machine, int cpunum);
	driver_init_func driver_init;
	nvram_handler_func nvram_handler;
	memcard_handler_func memcard_handler;
	machine_start_func machine_start;
	machine_reset_func machine_reset;
	sound_start_func sound_start;
	sound_reset_func sound_reset;
	video_start_func video_start;
	video_reset_func video_reset;
	palette_init_func palette_init;
	video_eof_func video_eof;
	video_update_func video_update;
};



/* start/end tags for the machine driver */
#define MACHINE_DRIVER_START(_name) \
	const machine_config_token machine_config_##_name[] = {

#define MACHINE_DRIVER_END \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_END, 8) };

/* use this to declare external references to a machine driver */
#define MACHINE_DRIVER_EXTERN(_name) \
	extern const machine_config_token machine_config_##_name[];


/* importing data from other machine drivers */
#define MDRV_IMPORT_FROM(_name) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_INCLUDE, 8), \
	TOKEN_PTR(tokenptr, machine_config_##_name),


/* add/remove/config devices */
#define MDRV_DEVICE_ADD(_tag, _type) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_DEVICE_ADD, 8), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define MDRV_DEVICE_REMOVE(_tag, _type) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_DEVICE_REMOVE, 8), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define MDRV_DEVICE_MODIFY(_tag, _type)	\
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_DEVICE_MODIFY, 8), \
	TOKEN_PTR(devtype, _type), \
	TOKEN_STRING(_tag),

#define MDRV_DEVICE_CONFIG(_config) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_DEVICE_CONFIG, 8), \
	TOKEN_PTR(voidptr, &(_config)),

#define MDRV_DEVICE_CONFIG_DATA32(_struct, _field, _val) \
	TOKEN_UINT32_PACK3(MCONFIG_TOKEN_DEVICE_CONFIG_DATA32, 8, sizeof(((_struct *)NULL)->_field), 6, offsetof(_struct, _field), 12), \
	TOKEN_UINT32((UINT32)(_val)),

#define MDRV_DEVICE_CONFIG_DATA64(_struct, _field, _val) \
	TOKEN_UINT32_PACK3(MCONFIG_TOKEN_DEVICE_CONFIG_DATA64, 8, sizeof(((_struct *)NULL)->_field), 6, offsetof(_struct, _field), 12), \
	TOKEN_UINT64((UINT64)(_val)),

#define MDRV_DEVICE_CONFIG_DATAFP32(_struct, _field, _val, _fixbits) \
	TOKEN_UINT32_PACK4(MCONFIG_TOKEN_DEVICE_CONFIG_DATAFP32, 8, sizeof(((_struct *)NULL)->_field), 6, _fixbits, 6, offsetof(_struct, _field), 12), \
	TOKEN_UINT32((UINT32)((float)(_val) * (float)(1 << (_fixbits)))),

#define MDRV_DEVICE_CONFIG_DATAFP64(_struct, _field, _val, _fixbits) \
	TOKEN_UINT32_PACK4(MCONFIG_TOKEN_DEVICE_CONFIG_DATAFP64, 8, sizeof(((_struct *)NULL)->_field), 6, _fixbits, 6, offsetof(_struct, _field), 12), \
	TOKEN_UINT64((UINT64)((float)(_val) * (float)((UINT64)1 << (_fixbits)))),

#ifdef PTR64
#define MDRV_DEVICE_CONFIG_DATAPTR(_struct, _field, _val) MDRV_DEVICE_CONFIG_DATA64(_struct, _field, _val)
#else
#define MDRV_DEVICE_CONFIG_DATAPTR(_struct, _field, _val) MDRV_DEVICE_CONFIG_DATA32(_struct, _field, _val)
#endif


/* add/modify/remove/replace CPUs */
#define MDRV_CPU_ADD_TAG(_tag, _type, _clock) \
	TOKEN_UINT64_PACK3(MCONFIG_TOKEN_CPU_ADD, 8, CPU_##_type, 24, _clock, 32), \
	TOKEN_STRING(_tag),

#define MDRV_CPU_ADD(_type, _clock) \
	MDRV_CPU_ADD_TAG(NULL, _type, _clock)

#define MDRV_CPU_MODIFY(_tag) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_MODIFY, 8), \
	TOKEN_STRING(_tag),

#define MDRV_CPU_REMOVE(_tag) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_REMOVE, 8), \
	TOKEN_STRING(_tag),

#define MDRV_CPU_REPLACE(_tag, _type, _clock) \
	TOKEN_UINT64_PACK3(MCONFIG_TOKEN_CPU_REPLACE, 8, CPU_##_type, 24, _clock, 32), \
	TOKEN_STRING(_tag),


/* CPU parameters */
#define MDRV_CPU_FLAGS(_flags) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_CPU_FLAGS, 8, _flags, 24),

#define MDRV_CPU_CONFIG(_config) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_CONFIG, 8), \
	TOKEN_PTR(voidptr, &(_config)),

#define MDRV_CPU_PROGRAM_MAP(_map1, _map2) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_PROGRAM_MAP, 8), \
	TOKEN_PTR(voidptr, construct_map_##_map1), \
	TOKEN_PTR(voidptr, construct_map_##_map2), \

#define MDRV_CPU_DATA_MAP(_map1, _map2)	\
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_DATA_MAP, 8), \
	TOKEN_PTR(voidptr, construct_map_##_map1), \
	TOKEN_PTR(voidptr, construct_map_##_map2), \

#define MDRV_CPU_IO_MAP(_map1, _map2) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_CPU_IO_MAP, 8), \
	TOKEN_PTR(voidptr, construct_map_##_map1), \
	TOKEN_PTR(voidptr, construct_map_##_map2), \

#define MDRV_CPU_VBLANK_INT(_func, _rate) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_CPU_VBLANK_INT, 8, _rate, 24), \
	TOKEN_PTR(interrupt, _func),

#define MDRV_CPU_PERIODIC_INT(_func, _rate)	\
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_CPU_PERIODIC_INT, 8, _rate, 24), \
	TOKEN_PTR(interrupt, _func),


/* core parameters */
#define MDRV_DRIVER_DATA(_struct) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_DRIVER_DATA, 8, sizeof(_struct), 24),

#define MDRV_INTERLEAVE(_interleave) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_INTERLEAVE, 8, _interleave, 24),

#define MDRV_WATCHDOG_VBLANK_INIT(_count) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_WATCHDOG_VBLANK, 8, _count, 24),

#define MDRV_WATCHDOG_TIME_INIT(_time) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_WATCHDOG_TIME, 8), \
	TOKEN_UINT64(_time),


/* core functions */
#define MDRV_MACHINE_START(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_MACHINE_START, 8), \
	TOKEN_PTR(machine_start, machine_start_##_func),

#define MDRV_MACHINE_RESET(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_MACHINE_RESET, 8), \
	TOKEN_PTR(machine_reset, machine_reset_##_func),

#define MDRV_NVRAM_HANDLER(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_NVRAM_HANDLER, 8), \
	TOKEN_PTR(nvram_handler, nvram_handler_##_func),

#define MDRV_MEMCARD_HANDLER(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_MEMCARD_HANDLER, 8), \
	TOKEN_PTR(memcard_handler, memcard_handler_##_func),


/* core video parameters */
#define MDRV_VIDEO_ATTRIBUTES(_flags) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_VIDEO_ATTRIBUTES, 8, _flags, 24),

#define MDRV_GFXDECODE(_gfx) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_GFXDECODE, 8), \
	TOKEN_PTR(gfxdecode, gfxdecodeinfo_##_gfx),

#define MDRV_PALETTE_LENGTH(_length) \
	TOKEN_UINT32_PACK2(MCONFIG_TOKEN_PALETTE_LENGTH, 8, _length, 24),

#define MDRV_DEFAULT_LAYOUT(_layout) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_DEFAULT_LAYOUT, 8), \
	TOKEN_STRING(&(_layout)[0]),


/* core video functions */
#define MDRV_PALETTE_INIT(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_PALETTE_INIT, 8), \
	TOKEN_PTR(palette_init, palette_init_##_func),

#define MDRV_VIDEO_START(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_VIDEO_START, 8), \
	TOKEN_PTR(video_start, video_start_##_func),

#define MDRV_VIDEO_RESET(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_VIDEO_RESET, 8), \
	TOKEN_PTR(video_reset, video_reset_##_func),

#define MDRV_VIDEO_EOF(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_VIDEO_EOF, 8), \
	TOKEN_PTR(video_eof, video_eof_##_func),

#define MDRV_VIDEO_UPDATE(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_VIDEO_UPDATE, 8), \
	TOKEN_PTR(video_update, video_update_##_func),


/* add/remove screens */
#define MDRV_SCREEN_ADD(_tag, _type) \
	MDRV_DEVICE_ADD(_tag, VIDEO_SCREEN) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, type, SCREEN_TYPE_##_type)

#define MDRV_SCREEN_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, VIDEO_SCREEN)

#define MDRV_SCREEN_MODIFY(_tag) \
	MDRV_DEVICE_MODIFY(_tag, VIDEO_SCREEN)

#define MDRV_SCREEN_FORMAT(_format) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.format, _format)

#define MDRV_SCREEN_TYPE(_type) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, type, SCREEN_TYPE_##_type)

#define MDRV_SCREEN_RAW_PARAMS(_pixclock, _htotal, _hbend, _hbstart, _vtotal, _vbend, _vbstart) \
	MDRV_DEVICE_CONFIG_DATA64(screen_config, defstate.refresh, HZ_TO_ATTOSECONDS(_pixclock) * (_htotal) * (_vtotal)) \
	MDRV_DEVICE_CONFIG_DATA64(screen_config, defstate.vblank, ((HZ_TO_ATTOSECONDS(_pixclock) * (_htotal) * (_vtotal)) / (_vtotal)) * ((_vtotal) - ((_vbstart) - (_vbend)))) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.width, _htotal)	\
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.height, _vtotal)	\
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.min_x, _hbend) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.max_x, (_hbstart) - 1) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.min_y, _vbend) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.max_y, (_vbstart) - 1)

#define MDRV_SCREEN_REFRESH_RATE(_rate) \
	MDRV_DEVICE_CONFIG_DATA64(screen_config, defstate.refresh, HZ_TO_ATTOSECONDS(_rate))

#define MDRV_SCREEN_VBLANK_TIME(_time) \
	MDRV_DEVICE_CONFIG_DATA64(screen_config, defstate.vblank, _time) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.oldstyle_vblank_supplied, TRUE)

#define MDRV_SCREEN_SIZE(_width, _height) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.width, _width) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.height, _height)

#define MDRV_SCREEN_VISIBLE_AREA(_minx, _maxx, _miny, _maxy) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.min_x, _minx) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.max_x, _maxx) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.min_y, _miny) \
	MDRV_DEVICE_CONFIG_DATA32(screen_config, defstate.visarea.max_y, _maxy)

#define MDRV_SCREEN_DEFAULT_POSITION(_xscale, _xoffs, _yscale, _yoffs)	\
	MDRV_DEVICE_CONFIG_DATAFP32(screen_config, xoffset, _xoffs, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(screen_config, xscale, _xscale, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(screen_config, yoffset, _yoffs, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(screen_config, yscale, _yscale, 24)


/* add/remove speakers */
#define MDRV_SPEAKER_ADD(_tag, _x, _y, _z) \
	MDRV_DEVICE_ADD(_tag, SPEAKER_OUTPUT) \
	MDRV_DEVICE_CONFIG_DATAFP32(speaker_config, x, _x, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(speaker_config, y, _y, 24) \
	MDRV_DEVICE_CONFIG_DATAFP32(speaker_config, z, _z, 24)

#define MDRV_SPEAKER_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag, SPEAKER_OUTPUT)

#define MDRV_SPEAKER_STANDARD_MONO(_tag) \
	MDRV_SPEAKER_ADD(_tag, 0.0, 0.0, 1.0)

#define MDRV_SPEAKER_STANDARD_STEREO(_tagl, _tagr) \
	MDRV_SPEAKER_ADD(_tagl, -0.2, 0.0, 1.0) \
	MDRV_SPEAKER_ADD(_tagr, 0.2, 0.0, 1.0)


/* core sound functions */
#define MDRV_SOUND_START(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_SOUND_START, 8), \
	TOKEN_PTR(sound_start, sound_start_##_func),

#define MDRV_SOUND_RESET(_func) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_SOUND_RESET, 8), \
	TOKEN_PTR(sound_start, sound_reset_##_func),


/* add/remove/replace sounds */
#define MDRV_SOUND_ADD_TAG(_tag, _type, _clock) \
	TOKEN_UINT64_PACK3(MCONFIG_TOKEN_SOUND_ADD, 8, SOUND_##_type, 24, _clock, 32), \
	TOKEN_STRING(_tag),

#define MDRV_SOUND_ADD(_type, _clock) \
	MDRV_SOUND_ADD_TAG(NULL, _type, _clock)

#define MDRV_SOUND_REMOVE(_tag) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_SOUND_REMOVE, 8), \
	TOKEN_STRING(_tag),

#define MDRV_SOUND_MODIFY(_tag) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_SOUND_MODIFY, 8), \
	TOKEN_STRING(_tag),

#define MDRV_SOUND_CONFIG(_config) \
	TOKEN_UINT32_PACK1(MCONFIG_TOKEN_SOUND_CONFIG, 8), \
	TOKEN_PTR(voidptr, &(_config)),

#define MDRV_SOUND_REPLACE(_tag, _type, _clock) \
	TOKEN_UINT64_PACK3(MCONFIG_TOKEN_SOUND_REPLACE, 8, SOUND_##_type, 24, _clock, 32), \
	TOKEN_STRING(_tag),

#define MDRV_SOUND_ROUTE_EX(_output, _target, _gain, _input)			\
	TOKEN_UINT64_PACK4(MCONFIG_TOKEN_SOUND_ROUTE, 8, _output, 12, _input, 12, (UINT32)((float)(_gain) * 16777216.0f), 32), \
	TOKEN_STRING(_target),

#define MDRV_SOUND_ROUTE(_output, _target, _gain) \
	MDRV_SOUND_ROUTE_EX(_output, _target, _gain, -1)



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- machine configurations ----- */

/* allocate a new machine configuration and populate it using the supplied constructor */
machine_config *machine_config_alloc(const machine_config_token *tokens);

/* release memory allocated for a machine configuration */
void machine_config_free(machine_config *config);





cpu_config *machine_config_add_cpu(machine_config *machine, const char *tag, cpu_type type, int cpuclock);
cpu_config *machine_config_find_cpu(machine_config *machine, const char *tag);
void machine_config_remove_cpu(machine_config *machine, const char *tag);

sound_config *machine_config_add_sound(machine_config *machine, const char *tag, sound_type type, int clock);
sound_config *machine_config_find_sound(machine_config *machine, const char *tag);
void machine_config_remove_sound(machine_config *machine, const char *tag);


#endif	/* __MCONFIG_H__ */
