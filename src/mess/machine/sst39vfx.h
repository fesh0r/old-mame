/*

	SST Multi-Purpose Flash (MPF)

	(c) 2001-2007 Tim Schuerewegen

	SST39VF020  - 256 KByte
	SST39VF400A - 512 Kbyte

*/

#ifndef _SST39VFX_H_
#define _SST39VFX_H_

#include "driver.h"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _sst39vfx_config sst39vfx_config;
struct _sst39vfx_config
{
	int cpu_datawidth;
	int cpu_endianess;
};

/***************************************************************************
    MACROS
***************************************************************************/

#define SST39VF020		DEVICE_GET_INFO_NAME(sst39vf020)

#define MDRV_SST39VF020_ADD(_tag,_cpu_datawidth,_cpu_endianess) \
	MDRV_DEVICE_ADD(_tag, SST39VF020, 0) \
	MDRV_DEVICE_CONFIG_DATA32(sst39vfx_config, cpu_datawidth, _cpu_datawidth) \
	MDRV_DEVICE_CONFIG_DATA32(sst39vfx_config, cpu_endianess, _cpu_endianess)

#define MDRV_SST39VF020_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)	
	
#define SST39VF400A		DEVICE_GET_INFO_NAME(sst39vf400a)

#define MDRV_SST39VF400A_ADD(_tag,_cpu_datawidth,_cpu_endianess) \
	MDRV_DEVICE_ADD(_tag, SST39VF400A, 0) \
	MDRV_DEVICE_CONFIG_DATA32(sst39vfx_config, cpu_datawidth, _cpu_datawidth) \
	MDRV_DEVICE_CONFIG_DATA32(sst39vfx_config, cpu_endianess, _cpu_endianess)

#define MDRV_SST39VF400A_REMOVE(_tag) \
	MDRV_DEVICE_REMOVE(_tag)

/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO(sst39vf020);
DEVICE_GET_INFO(sst39vf400a);

// get base/size
UINT8* sst39vfx_get_base( const device_config *device );
UINT32 sst39vfx_get_size( const device_config *device );

// read/write handler
/*
READ8_HANDLER( sst39vfx_r );
WRITE8_HANDLER( sst39vfx_w );
*/

// load/save
void sst39vfx_load(const device_config *device, mame_file *file);
void sst39vfx_save(const device_config *device, mame_file *file);

// non-volatile ram handler
/*
NVRAM_HANDLER( sst39vfx );
*/

#endif
