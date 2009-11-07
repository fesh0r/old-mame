/*********************************************************************

    m6847.h

    Implementation of Motorola 6847 video hardware chip

**********************************************************************/

#ifndef __M6847_H__
#define __M6847_H__

#include "devcb.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

typedef enum
{
	M6847_VERSION_ORIGINAL_NTSC,
	M6847_VERSION_ORIGINAL_PAL,
	M6847_VERSION_M6847Y_NTSC,
	M6847_VERSION_M6847Y_PAL,
	M6847_VERSION_M6847T1_NTSC,
	M6847_VERSION_M6847T1_PAL,
	M6847_VERSION_GIME_NTSC,
	M6847_VERSION_GIME_PAL
} m6847_type;

/* for now, the MAME core forces us to use these */
#define M6847_NTSC_FRAMES_PER_SECOND	60
#define M6847_PAL_FRAMES_PER_SECOND		50

typedef enum
{
	M6847_CLOCK,
	M6847_HSYNC
} m6847_timing_type;


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _mc6847_interface mc6847_interface;
struct _mc6847_interface
{
	/* data fetch */
	devcb_read8 in_dd_func;

	/* mode control lines input */
	devcb_read_line in_gm2_func;
	devcb_read_line in_gm1_func;
	devcb_read_line in_gm0_func;
	devcb_read_line in_intext_func;
	devcb_read_line in_inv_func;
	devcb_read_line in_as_func;
	devcb_read_line in_ag_func;
	devcb_read_line in_css_func;

	/* synchronizing outputs */
	devcb_write_line out_fs_func;
	devcb_write_line out_hs_func;
	devcb_write_line out_rs_func;
};

typedef struct _mc6847_config mc6847_config;
struct _mc6847_config
{
	m6847_type type;
	int cpu0_timing_factor;

	UINT8 (*get_char_rom)(running_machine *machine, UINT8 ch, int line);

	/* needed for the CoCo 3 */
	int (*new_frame_callback)(running_machine *machine);	/* returns whether the M6847 is in charge of this frame */
	void (*custom_prepare_scanline)(int scanline);

	const UINT32 *custom_palette;
};


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

DEVICE_GET_INFO( mc6847 );


/***************************************************************************
    DEVICE CONFIGURATION MACROS
***************************************************************************/

#define MC6847 DEVICE_GET_INFO_NAME(mc6847)

#define MDRV_MC6847_ADD(_tag, _interface) \
	MDRV_DEVICE_ADD(_tag, MC6847, 0) \
	MDRV_DEVICE_CONFIG(_interface)

#define MDRV_MC6847_TYPE(_type) \
	MDRV_DEVICE_CONFIG_DATA32(mc6847_config, type, _type)

#define MDRV_MC6847_TIMING_FACTOR(_factor) \
	MDRV_DEVICE_CONFIG_DATA32(mc6847_config, cpu0_timing_factor, _factor)

#define MDRV_MC6847_CHAR_ROM(_get_char_rom) \
	MDRV_DEVICE_CONFIG_DATAPTR(mc6847_config, get_char_rom, _get_char_rom)

#define MDRV_MC6847_FRAME_CALLBACK(_new_frame_callback) \
	MDRV_DEVICE_CONFIG_DATAPTR(mc6847_config, new_frame_callback, _new_frame_callback)

#define MDRV_MC6847_PREPARE_SCANLINE(_prepare_scanline) \
	MDRV_DEVICE_CONFIG_DATAPTR(mc6847_config, custom_prepare_scanline, _prepare_scanline)

#define MDRV_MC6847_PALETTE(_palette) \
	MDRV_DEVICE_CONFIG_DATAPTR(mc6847_config, custom_palette, _palette)


/***************************************************************************
    DEVICE I/O FUNCTIONS
***************************************************************************/

/* mode control input lines */
WRITE_LINE_DEVICE_HANDLER( mc6847_gm2_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_gm1_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_gm0_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_intext_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_inv_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_as_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_ag_w );
WRITE_LINE_DEVICE_HANDLER( mc6847_css_w );

/* synchronizing outputs */
READ_LINE_DEVICE_HANDLER( mc6847_fs_r );
READ_LINE_DEVICE_HANDLER( mc6847_hs_r );



/* video update proc */
UINT32 mc6847_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

void mc6847_set_palette(const device_config *device, UINT32 *palette);

/* hack for the coco until the 6883sam can be updated */
int mc6847_get_scanline(const device_config *device);


void m6847_video_changed(void);

/* timing functions */
UINT64 m6847_time(const device_config *device, m6847_timing_type timing);
attotime m6847_time_until(const device_config *device, m6847_timing_type timing, UINT64 target_time);

/* CoCo 3 hooks */
attotime m6847_scanline_time(const device_config *device, int scanline);

INPUT_PORTS_EXTERN( m6847_artifacting );


#endif /* __M6847_H__ */
