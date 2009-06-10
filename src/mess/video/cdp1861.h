/*

					  RCA CDP1861 Video Display Controller

								_______  _______
				  CLK_	 1	---|	   \/		|---  24  Vdd
			     DMAO_	 2	---|				|---  23  CLEAR_
			      INT_	 3	---|				|---  22  SC1
				   TPA	 4	---|				|---  21  SC0
				   TPB	 5	---|				|---  20  DI7
			COMP SYNC_	 6	---|	CDP1861C	|---  19  DI6
				 VIDEO	 7	---|	top view	|---  18  DI5
				RESET_	 8	---|				|---  17  DI4
				  EFX_	 9	---|				|---  16  DI3
			   DISP ON	10	---|			    |---  15  DI2
			  DISP OFF	11	---|				|---  14  DI1
				   Vss	12	---|________________|---  13  DI0


			   http://homepage.mac.com/ruske/cosmacelf/cdp1861.pdf

*/

#ifndef __CDP1861__
#define __CDP1861__

#include "devcb.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define CDP1861_VISIBLE_COLUMNS	64
#define CDP1861_VISIBLE_LINES	128

#define CDP1861_HBLANK_START	14 * 8
#define CDP1861_HBLANK_END		12
#define CDP1861_HSYNC_START		0
#define CDP1861_HSYNC_END		12
#define CDP1861_SCREEN_WIDTH	14 * 8

#define CDP1861_TOTAL_SCANLINES				262

#define CDP1861_SCANLINE_DISPLAY_START		80
#define CDP1861_SCANLINE_DISPLAY_END		208
#define CDP1861_SCANLINE_VBLANK_START		262
#define CDP1861_SCANLINE_VBLANK_END			16
#define CDP1861_SCANLINE_VSYNC_START		16
#define CDP1861_SCANLINE_VSYNC_END			0
#define CDP1861_SCANLINE_INT_START			CDP1861_SCANLINE_DISPLAY_START - 2
#define CDP1861_SCANLINE_INT_END			CDP1861_SCANLINE_DISPLAY_START
#define CDP1861_SCANLINE_EFX_TOP_START		CDP1861_SCANLINE_DISPLAY_START - 4
#define CDP1861_SCANLINE_EFX_TOP_END		CDP1861_SCANLINE_DISPLAY_START
#define CDP1861_SCANLINE_EFX_BOTTOM_START	CDP1861_SCANLINE_DISPLAY_END - 4
#define CDP1861_SCANLINE_EFX_BOTTOM_END		CDP1861_SCANLINE_DISPLAY_END

/***************************************************************************
    MACROS / CONSTANTS
***************************************************************************/

#define CDP1861		DEVICE_GET_INFO_NAME(cdp1861)

#define MDRV_CDP1861_ADD(_tag, _clock, _config) \
	MDRV_DEVICE_ADD(_tag, CDP1861, _clock) \
	MDRV_DEVICE_CONFIG(_config)

#define CDP1861_INTERFACE(name) const cdp1861_interface (name)=

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _cdp1861_interface cdp1861_interface;
struct _cdp1861_interface
{
	const char *cpu_tag;		/* cpu we are working with */
	const char *screen_tag;		/* screen we are acting on */

	/* this gets called for every change of the INT pin (pin 3) */
	devcb_write_line		out_int_func;

	/* this gets called for every change of the DMAO pin (pin 2) */
	devcb_write_line		out_dmao_func;

	/* this gets called for every change of the EFX pin (pin 9) */
	devcb_write_line		out_efx_func;
};

/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* device interface */
DEVICE_GET_INFO( cdp1861 );

/* display on */
WRITE_LINE_DEVICE_HANDLER( cdp1861_dispon_w );

/* display off */
WRITE_LINE_DEVICE_HANDLER( cdp1861_dispoff_w );

/* DMA write */
void cdp1861_dma_w(const device_config *device, UINT8 data);

/* screen update */
void cdp1861_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);

#endif
