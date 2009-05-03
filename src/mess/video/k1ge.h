
#ifndef __K2GE_H_
#define __K2GE_H_

#include "devcb.h"


#define K1GE_SCREEN_HEIGHT	199


#define K1GE		DEVICE_GET_INFO_NAME(k1ge)
#define K2GE		DEVICE_GET_INFO_NAME(k2ge)


#define MDRV_K1GE_ADD(_tag, _clock, _config ) \
	MDRV_DEVICE_ADD( _tag, K1GE, _clock ) \
	MDRV_DEVICE_CONFIG( _config )


#define MDRV_K2GE_ADD(_tag, _clock, _config ) \
	MDRV_DEVICE_ADD( _tag, K2GE, _clock ) \
	MDRV_DEVICE_CONFIG( _config )


typedef struct _k1ge_interface k1ge_interface;
struct _k1ge_interface
{
	const char		*screen_tag;		/* screen we are drawing on */
	const char		*vram_tag;			/* memory region we will use for video ram */
	devcb_write8	vblank_pin_w;		/* called back when VBlank pin may have changed */
	devcb_write8	hblank_pin_w;		/* called back when HBlank pin may have changed */
};


PALETTE_INIT( k1ge );
PALETTE_INIT( k2ge );


DEVICE_GET_INFO( k1ge );
DEVICE_GET_INFO( k2ge );


WRITE8_DEVICE_HANDLER( k1ge_w );
READ8_DEVICE_HANDLER( k1ge_r );

void k1ge_update( const device_config *device, bitmap_t *bitmap, const rectangle *cliprect );

#endif

