/***************************************************************************

  Color Graphics Adapter (CGA) section


  Notes on Port 3D8
  (http://www.clipx.net/ng/interrupts_and_ports/ng2d045.php)

	Port 3D8  -  Color/VGA Mode control register

			xx1x xxxx  Attribute bit 7. 0=blink, 1=Intesity
			xxx1 xxxx  640x200 mode
			xxxx 1xxx  Enable video signal
			xxxx x1xx  Select B/W mode
			xxxx xx1x  Select graphics
			xxxx xxx1  80x25 text


	The usage of the above control register for various modes is:
			xx10 1100  40x25 alpha B/W
			xx10 1000  40x25 alpha color
			xx10 1101  80x25 alpha B/W
			xx10 1001  80x25 alpha color
			xxx0 1110  320x200 graph B/W
			xxx0 1010  320x200 graph color
			xxx1 1110  640x200 graph B/W


	PC1512 display notes

	The PC1512 built-in display adaptor is an emulation of IBM's CGA.  Unlike a
	real CGA, it is not built around a real MC6845 controller, and so attempts
	to get custom video modes out of it may not work as expected. Its 640x200
	CGA mode can be set up to be a 16-color mode rather than mono.

	If you program it with BIOS calls, the PC1512 behaves just like a real CGA,
	except:

	- The 'greyscale' text modes (0 and 2) behave just like the 'color'
	  ones (1 and 3). On a color monitor both are in color; on a mono
	  monitor both are in greyscale.
	- Mode 5 (the 'greyscale' graphics mode) displays in color, using
	  an alternative color palette: Cyan, Red and White.
	- The undocumented 160x100x16 "graphics" mode works correctly.

	(source John Elliott http://www.seasip.info/AmstradXT/pc1512disp.html)


  Cursor signal handling:

  The alpha dots signal is set when a character pixel should be set. This signal is
  also set when the cursor should be displayed. The following formula for alpha
  dots is derived from the schematics:
  ALPHA DOTS = ( ( CURSOR DLY ) AND ( CURSOR BLINK ) ) OR ( ( ( NOT AT7 ) OR CURSOR DLY OR -BLINK OR NOT ENABLE BLINK ) AND ( CHG DOTS ) )

  -CURSOR BLINK = VSYNC DLY (LS393) (changes every 8 vsyncs)
  -BLINK = -CURSOR BLINK (LS393) (changes every 16 vsyncs)
  -CURSOR DLY = -CURSOR signal from mc6845 and LS174
  CHG DOTS = character pixel (from character rom)

  For non-blinking modes this formula reduces to:
  ALPHA DOTS = ( ( CURSOR DLY ) AND ( CURSOR BLINK ) ) OR ( CHG DOTS )

  This means the cursor switches on/off state every 8 vsyncs.


  For blinking modes this formula reduces to:
  ALPHA DOTS = ( ( CURSOR DLY ) AND ( CURSOR BLINK ) ) OR ( ( ( NOT AT7 ) OR CURSOR DLY OR -BLINK ) AND ( CHG DOTS ) )

  So, at the cursor location the attribute blinking is ignored and only regular
  cursor blinking takes place (state switches every 8 vsyncs). On non-cursor
  locations with the highest attribute bits set the character will switch
  on/off every 16 vsyncs. In all other cases the character is displayed as
  usual.

***************************************************************************/

#include "driver.h"
//#include "deprecat.h"
#include "video/pc_cga.h"
#include "video/mc6845.h"
#include "video/pc_video.h"
#include "video/cgapal.h"
#include "memconv.h"

#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */
#define	NTSC_FILTER	0

#define CGA_LOG(N,M,A) \
	if(VERBOSE_CGA>=N){ if( M )logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()),(char*)M ); logerror A; }

/***************************************************************************

	Static declarations

***************************************************************************/

static VIDEO_START( pc_cga );
static PALETTE_INIT( pc_cga );


INPUT_PORTS_START( pcvideo_cga )
	PORT_START_TAG( "pcvideo_cga_config" )
	PORT_CONFNAME( 0x03, 0x00, "CGA character set")
	PORT_CONFSETTING(0x00, DEF_STR( Normal ))
	PORT_CONFSETTING(0x01, "Alternative")
	PORT_CONFNAME( 0x1C, 0x00, "CGA monitor type")
	PORT_CONFSETTING(0x00, "Colour RGB")
	PORT_CONFSETTING(0x04, "Mono RGB")
	PORT_CONFSETTING(0x08, "Colour composite")
	PORT_CONFSETTING(0x0C, "Television")
	PORT_CONFSETTING(0x10, "LCD")
	PORT_CONFNAME( 0xE0, 0x00, "CGA chipset")
	PORT_CONFSETTING(0x00, "IBM")
	PORT_CONFSETTING(0x20, "Amstrad PC1512")
	PORT_CONFSETTING(0x40, "Amstrad PPC512")
	PORT_CONFSETTING(0x60, "ATI")
	PORT_CONFSETTING(0x80, "Paradise")
INPUT_PORTS_END


INPUT_PORTS_START( pcvideo_cga_at )
	PORT_START_TAG( "pcvideo_cga_config" )
	PORT_BIT( 0x03, 0x01, IPT_UNUSED )	/* Always use fat characters */
	PORT_CONFNAME( 0x1C, 0x00, "CGA monitor type")
	PORT_CONFSETTING(0x00, "Colour RGB")
	PORT_CONFSETTING(0x04, "Mono RGB")
	PORT_CONFSETTING(0x08, "Colour composite")
	PORT_CONFSETTING(0x0C, "Television")
	PORT_CONFSETTING(0x10, "LCD")
	PORT_CONFNAME( 0xE0, 0x00, "CGA chipset")
	PORT_CONFSETTING(0x00, "IBM")
	PORT_CONFSETTING(0x20, "Amstrad PC1512")
	PORT_CONFSETTING(0x40, "Amstrad PPC512")
	PORT_CONFSETTING(0x60, "ATI")
	PORT_CONFSETTING(0x80, "Paradise")
INPUT_PORTS_END


INPUT_PORTS_START( pcvideo_pc1512 )
	PORT_START_TAG( "pcvideo_cga_config" )
	PORT_CONFNAME( 0x03, 0x03, "CGA character set")
	PORT_CONFSETTING(0x00, "Greek")
	PORT_CONFSETTING(0x01, "Danish 2")
	PORT_CONFSETTING(0x02, "Danish 1")
	PORT_CONFSETTING(0x03, "Default")
	PORT_CONFNAME( 0x1C, 0x00, "CGA monitor type")
	PORT_CONFSETTING(0x00, "Colour RGB")
	PORT_CONFSETTING(0x04, "Mono RGB")
	PORT_BIT ( 0xE0, 0x20, IPT_UNUSED ) /* Chipset is always PC1512 */
INPUT_PORTS_END

/* Dipswitch for font selection */
#define CGA_FONT        (input_port_read_direct(cga.config_input_port)&3)

/* Dipswitch for monitor selection */
#define CGA_MONITOR     (input_port_read_direct(cga.config_input_port)&0x1C)
#define CGA_MONITOR_RGB         0x00    /* Colour RGB */
#define CGA_MONITOR_MONO        0x04    /* Greyscale RGB */
#define CGA_MONITOR_COMPOSITE   0x08    /* Colour composite */
#define CGA_MONITOR_TELEVISION  0x0C    /* Television */
#define CGA_MONITOR_LCD         0x10    /* LCD, eg PPC512 */


/* Dipswitch for chipset selection */
#define CGA_CHIPSET     (input_port_read_direct(cga.config_input_port)&0xE0)
#define CGA_CHIPSET_IBM         0x00    /* Original IBM CGA */
#define CGA_CHIPSET_PC1512      0x20    /* PC1512 CGA subset */
#define CGA_CHIPSET_PC200       0x40    /* PC200 in CGA mode */
#define CGA_CHIPSET_ATI         0x60    /* ATI (supports Plantronics) */
#define CGA_CHIPSET_PARADISE    0x80    /* Paradise (used in PC1640) */


static VIDEO_UPDATE( mc6845_cga );
static READ8_HANDLER( pc_cga8_r );
static WRITE8_HANDLER( pc_cga8_w );
static READ16_HANDLER( pc_cga16le_r );
static WRITE16_HANDLER( pc_cga16le_w );
static READ32_HANDLER( pc_cga32le_r );
static WRITE32_HANDLER( pc_cga32le_w );
static MC6845_UPDATE_ROW( cga_update_row );
static MC6845_ON_HSYNC_CHANGED( cga_hsync_changed );
static MC6845_ON_VSYNC_CHANGED( cga_vsync_changed );
static VIDEO_START( pc1512 );
static VIDEO_UPDATE( mc6845_pc1512 );


static const mc6845_interface mc6845_cga_intf =
{
	CGA_SCREEN_NAME,	/* screen number */
	XTAL_14_31818MHz/8,	/* clock */
	8,					/* numbers of pixels per video memory address */
	NULL,				/* begin_update */
	cga_update_row,		/* update_row */
	NULL,				/* end_update */
	NULL,				/* on_de_chaged */
	cga_hsync_changed,	/* on_hsync_changed */
	cga_vsync_changed	/* on_vsync_changed */
};


MACHINE_DRIVER_START( pcvideo_cga )
	MDRV_SCREEN_ADD(CGA_SCREEN_NAME, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_14_31818MHz,912,0,640,262,0,200)
	MDRV_PALETTE_LENGTH(/* CGA_PALETTE_SETS * 16*/ 65536 )

	MDRV_PALETTE_INIT(pc_cga)

	MDRV_DEVICE_ADD(CGA_MC6845_NAME, MC6845)
	MDRV_DEVICE_CONFIG( mc6845_cga_intf )

	MDRV_VIDEO_START( pc_cga )
	MDRV_VIDEO_UPDATE( mc6845_cga )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pcvideo_pc1512 )
	MDRV_IMPORT_FROM( pcvideo_cga )
	MDRV_VIDEO_START( pc1512 )
	MDRV_VIDEO_UPDATE( mc6845_pc1512 )
MACHINE_DRIVER_END



/***************************************************************************

	Local variables and declarations

***************************************************************************/

static struct
{
	UINT8 mode_control;	/* wo 0x3d8 */
	UINT8 color_select;	/* wo 0x3d9 */
	UINT8 status;		/* ro 0x3da */
	UINT8 plantronics;	/* wo 0x3dd, ATI chipset only */

	UINT8 frame;

	UINT8	*chr_gen;

	const input_port_config *config_input_port;

	mc6845_update_row_func	update_row;
	UINT8	palette_lut_2bpp[4];
	UINT8	vsync;
	UINT8	hsync;
} cga;



/***************************************************************************
 *
 * NTSC filter
 *
 * Code taken from http://www.reenigne.org/crtc.html
 *
 ***************************************************************************/

static struct ntsc_decoder
{
	int period;

	int *I_filter;
	int *Q_filter;

	int *Y_buffer;
	int *I_buffer;
	int *Q_buffer;

	int Y_total;
	int I_total;
	int Q_total;
} ntsc;


/* Clear NTSC buffers and running totals. Call this before the beginning of each new line. */
static void ntsc_clear(struct ntsc_decoder *ntsc)
{
	int j;
	for (j=0;j<ntsc->period;++j)
	{
		ntsc->Y_buffer[j] = 0;
		ntsc->I_buffer[j] = 0;
		ntsc->Q_buffer[j] = 0;
	}
	ntsc->Y_total = 0;
	ntsc->I_total = 0;
	ntsc->Q_total = 0;
}


/* Create an NTSC decoder object.

   period is the number of times the NTSC signal is sampled per cycle of the color subcarrier clock (3.579545 MHz).
   e.g. CGA is 8 because you need to sample the chroma signal at 28.6MHz to retain all color information.
   For 4-color composite systems such as CoCo, 2 samples may suffice.

   samples is also the number of data points whose values are used to determine the color of a given point.

   Points here do not necessarily correspond to pixels - points are samples of the RGB color decoded output and pixels
   are CPU addressable points in the input composite data.

   The correct value for hue will usually be a multiple of 128.
*/
static void ntsc_decoder_init(struct ntsc_decoder *ntsc, int period, int hue)
{
	int j;

	ntsc->period = period;
	ntsc->I_filter = auto_malloc(period * sizeof(int));
	ntsc->Q_filter = auto_malloc(period * sizeof(int));
	ntsc->Y_buffer = auto_malloc(period * sizeof(int));
	ntsc->I_buffer = auto_malloc(period * sizeof(int));
	ntsc->Q_buffer = auto_malloc(period * sizeof(int));
	ntsc_clear(ntsc);
	for (j=0;j<period;++j)
	{
		double angle = M_PI*((hue+(j<<8))/(period*128.0)-33.0/180);
		ntsc->I_filter[j] = (int)(512.0*cos(angle));
		ntsc->Q_filter[j] = (int)(512.0*sin(angle));
	}
}


/* Decode some NTSC data, working from left to right.

   The composite (luma+chroma) values are in "input", scaled to 0-255 values.
   Red, green and blue bytes (in that order) are placed in "output".

   Note that consecutive calls to ntsc_decode() should be on consecutive input data,
   or the first ntsc->samples points will be inaccurate. To avoid this inaccuracy,
   first call ntsc_decode(ntsc, input, NULL, 1) with "input" containing the
   samples directly before the samples you will subsequently be using.
*/
static void ntsc_decode(struct ntsc_decoder *ntsc, UINT8 *input, UINT8 *output, int periods)
{
	int j;

	int *I_filter;
	int *Q_filter;
	int *Y_buffer;
	int *I_buffer;
	int *Q_buffer;
	int Y_total = ntsc->Y_total;
	int I_total = ntsc->I_total;
	int Q_total = ntsc->Q_total;
	int Y,I,Q;
	int R,G,B;
	int period = ntsc->period;

	while ((--periods)>=0)
	{
		I_filter = ntsc->I_filter;
		Q_filter = ntsc->Q_filter;
		Y_buffer = ntsc->Y_buffer;
		I_buffer = ntsc->I_buffer;
		Q_buffer = ntsc->Q_buffer;

		for (j=0;j<period;++j)
		{

			/* Get next point */
			Y = *(input++);
			I = Y * *(I_filter++);
			Q = Y * *(Q_filter++);

			/* Update running totals */
			Y_total += Y - *Y_buffer;
			I_total += I - *I_buffer;
			Q_total += Q - *Q_buffer;

			/* Save point in buffer */
			*(Y_buffer++) = Y;
			*(I_buffer++) = I;
			*(Q_buffer++) = Q;

			/* Compute averages over entire period */
			Y = Y_total/period;
			I = I_total/period;
			Q = Q_total/period;

			/* Clamp YIQ values to avoid hue drift of oversaturated colors */
			if (Y>256) Y=256; if (Y<0) Y=0;
			if (I>39041) I=39041; if (I<-39041) I=-39041;
			if (Q>34249) Q=34249; if (Q<-34249) Q=-34249;

			/* Convert YIQ to RGB */
			Y<<=16;
			R = (Y + 249*I + 159*Q)>>16;
			G = (Y -  70*I - 166*Q)>>16;
			B = (Y - 283*I + 436*Q)>>16;

			/* Clamp the RGB values */
			if (R>255) R=255; if (R<0) R=0;
			if (G>255) G=255; if (G<0) G=0;
			if (B>255) B=255; if (B<0) B=0;

			/* Emit the RGB values to the output buffer */
			*output = R; output++;
			*output = G; output++;
			*output = B; output++;
		}
	}
	ntsc->Y_total = Y_total;
	ntsc->I_total = I_total;
	ntsc->Q_total = Q_total;
}


/***************************************************************************

	Methods

***************************************************************************/

/* Initialise the cga palette */
static PALETTE_INIT( pc_cga )
{
	int i, r, g, b;

	for ( i = 0; i < CGA_PALETTE_SETS * 16; i++ )
	{
		palette_set_color_rgb( machine, i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2] );
	}

	i = 0x8000;
	for ( r = 0; r < 32; r++ )
	{
		for ( g = 0; g < 32; g++ )
		{
			for ( b = 0; b < 32; b++ )
			{
				palette_set_color_rgb( machine, i, r << 3, g << 3, b << 3 );
				i++;
			}
		}
	}
}


static int internal_pc_cga_video_start(running_machine *machine, int personality)
{
	memset(&cga, 0, sizeof(cga));
	cga.update_row = NULL;

	cga.chr_gen = memory_region( machine, REGION_GFX1 ) + 0x1000;

	state_save_register_item("pccga", 0, cga.mode_control);
	state_save_register_item("pccga", 0, cga.color_select);
	state_save_register_item("pccga", 0, cga.status);
	state_save_register_item("pccga", 0, cga.plantronics);

	cga.config_input_port = input_port_by_tag(machine->portconfig, "pcvideo_cga_config" );

	return 0;
}



static VIDEO_START( pc_cga )
{
	int buswidth;

	/* Changed video RAM size to full 32k, for cards which support the
	 * Plantronics chipset.
	 * TODO: Cards which don't support Plantronics should repeat at
	 * BC000h */
	buswidth = cputype_databus_width(machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, SMH_BANK11 );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, pc_video_videoram_w );
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga8_r );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga8_w );
			break;

		case 16:
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, SMH_BANK11 );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, pc_video_videoram16le_w );
			memory_install_read16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga16le_r );
			memory_install_write16_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga16le_w );
			break;

		case 32:
			memory_install_read32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, SMH_BANK11 );
			memory_install_write32_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x04000, pc_video_videoram32_w );
			memory_install_read32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga32le_r );
			memory_install_write32_handler(machine, 0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga32le_w );
			break;

		default:
			fatalerror("CGA:  Bus width %d not supported\n", buswidth);
			break;
	}

	videoram_size = 0x4000;

	videoram = auto_malloc(videoram_size);

	memory_set_bankptr(11, videoram);

	internal_pc_cga_video_start(machine, M6845_PERSONALITY_GENUINE);

	ntsc_decoder_init( &ntsc, 8, 256 );
}



static VIDEO_UPDATE( mc6845_cga )
{
	UINT8 *gfx = memory_region(screen->machine, REGION_GFX1);
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect);

	/* Check for changes in font dipsetting */
	switch ( CGA_FONT & 0x01 )
	{
	case 0:
		cga.chr_gen = gfx + 0x1800;
		break;
	case 1:
		cga.chr_gen = gfx + 0x1000;
		break;
	}
	return 0;
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_inten_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_text_inten_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;
		UINT16 bg = attr >> 4;

		if ( i == cursor_x && ( cga.frame & 0x08 ) )
		{
			data = 0xFF;
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8. Composite monitor, greyscale.
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_inten_comp_grey_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_text_inten_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = 0x10 + ( attr & 0x0F );
		UINT16 bg = 0x10 + ( ( attr >> 4 ) & 0x07 );

		if ( i == cursor_x && ( cga.frame & 0x08 ) )
		{
			data = 0xFF;
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}

/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_inten_alt_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_text_inten_alt_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;

		if ( i == cursor_x && ( cga.frame & 0x08 ) )
		{
			data = 0xFF;
		}

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x40 ) ? fg : 0; p++;
		*p = ( data & 0x20 ) ? fg : 0; p++;
		*p = ( data & 0x10 ) ? fg : 0; p++;
		*p = ( data & 0x08 ) ? fg : 0; p++;
		*p = ( data & 0x04 ) ? fg : 0; p++;
		*p = ( data & 0x02 ) ? fg : 0; p++;
		*p = ( data & 0x01 ) ? fg : 0; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_blink_update_row )
{
	UINT16	*p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_text_blink_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;
		UINT16 bg = attr >> 4;

		if ( i == cursor_x )
		{
			if ( cga.frame & 0x08 )
			{
				data = 0xFF;
			}
		}
		else
		{
			if ( ( attr & 0x80 ) && ( cga.frame & 0x10 ) )
			{
				data = 0x00;
			}
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_blink_alt_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_text_blink_alt_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x07;
		UINT16 bg = 0;

		if ( i == cursor_x )
		{
			if ( cga.frame & 0x08 )
			{
				data = 0xFF;
			}
		}
		else
		{
			if ( ( attr & 0x80 ) && ( cga.frame & 0x10 ) )
			{
				data = 0x00;
				bg = ( attr >> 4 ) & 0x07;
			}
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/* The lo-res (320x200) graphics mode on a colour composite monitor */

static MC6845_UPDATE_ROW( cga_gfx_4bppl_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_gfx_4bppl_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;

		data = videoram[ offset + 1 ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
	}
}


/* The hi-res graphics mode on a colour composite monitor
 *
 * The different scaling factors mean that the '160x200' versions of screens
 * are the same size as the normal colour ones.
 */

static const UINT8 yc_lut2[4] = { 0, 182, 71, 255 };

static const UINT8 yc_lut[16][8] =
{
	{ 0, 0, 0, 0, 0, 0, 0, 0 },	/* black */
	{ 0, 0, 0, 0, 1, 1, 1, 1 },	/* blue */
	{ 0, 1, 1, 1, 1, 0, 0, 0 },	/* green */
	{ 0, 0, 1, 1, 1, 1, 0, 0 },	/* cyan */
	{ 1, 1, 0, 0, 0, 0, 1, 1 }, /* red */
	{ 1, 0, 0, 0, 0, 1, 1, 1 }, /* magenta */
	{ 1, 1, 1, 1, 0, 0, 0, 0 }, /* yellow */
	{ 1, 1, 1, 1, 1, 1, 1, 1 }, /* white */
	/* Intensity set */
	{ 2, 2, 2, 2, 2, 2, 2, 2 }, /* black */
	{ 2, 2, 2, 2, 3, 3, 3, 3 }, /* blue */
	{ 2, 3, 3, 3, 3, 2, 2, 2 }, /* green */
	{ 2, 2, 3, 3, 3, 3, 2, 2 }, /* cyan */
	{ 3, 3, 2, 2, 2, 2, 3, 3 }, /* red */
	{ 3, 2, 2, 2, 2, 3, 3, 3 }, /* magenta */
	{ 3, 3, 3, 3, 2, 2, 2, 2 }, /* yellow */
	{ 3, 3, 3, 3, 3, 3, 3, 3 }, /* white */
};

static MC6845_UPDATE_ROW( cga_gfx_4bpph_update_row )
{
	UINT8	samples[1280];
	UINT8	ntsc_decoded[3*1280];
	int		samp_index = 0;
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_gfx_4bpph_update_row",("\n"));
if ( NTSC_FILTER )
{
	ntsc_clear( &ntsc );
	memset( ntsc_decoded, 0, sizeof(ntsc_decoded));
}
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

if ( NTSC_FILTER )
{
		samples[samp_index] = yc_lut2[yc_lut[data>>4][0]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][1]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][2]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][3]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][4]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][5]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][6]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][7]]; samp_index++;

		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][0]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][1]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][2]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][3]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][4]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][5]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][6]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][7]]; samp_index++;
}

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;

		data = videoram[ offset + 1 ];

if ( NTSC_FILTER )
{
		samples[samp_index] = yc_lut2[yc_lut[data>>4][0]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][1]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][2]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][3]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][4]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][5]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][6]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data>>4][7]]; samp_index++;

		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][0]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][1]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][2]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][3]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][4]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][5]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][6]]; samp_index++;
		samples[samp_index] = yc_lut2[yc_lut[data & 0x0F][7]]; samp_index++;
}

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
	}
if (NTSC_FILTER)
{
	ntsc_decode( &ntsc, samples, ntsc_decoded, 160 );
	p = BITMAP_ADDR16(bitmap, y, 0);
	samp_index = 0;
	for ( i = 0; i < ( 8 * x_count ); i++ )
	{
		int r = ( ntsc_decoded[samp_index] + ntsc_decoded[samp_index+3] ) / 2;
		int g = ( ntsc_decoded[samp_index+1] + ntsc_decoded[samp_index+4] ) / 2;
		int b = ( ntsc_decoded[samp_index+2] + ntsc_decoded[samp_index+5] ) / 2;
		*p = 0x8000 + ( ( ( r & 0xF8 ) << 7 ) | ( ( g & 0xF8 ) << 2 ) | ( ( b & 0xF8 ) >> 3 ) );
		p++;
		samp_index +=6; 
	}
}
}


/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
  cga fetches 2 byte per mc6845 access.
***************************************************************************/

static MC6845_UPDATE_ROW( cga_gfx_2bpp_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_gfx_2bpp_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = cga.palette_lut_2bpp[ ( data >> 6 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 4 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 2 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[   data        & 0x03 ]; p++;

		data = videoram[ offset+1 ];

		*p = cga.palette_lut_2bpp[ ( data >> 6 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 4 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 2 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[   data        & 0x03 ]; p++;
	}
}



/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/

static MC6845_UPDATE_ROW( cga_gfx_1bpp_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT8	fg = cga.color_select & 0x0F;
	int i;

	if ( y == 0 ) CGA_LOG(1,"cga_gfx_1bpp_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( ra & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x40 ) ? fg : 0; p++;
		*p = ( data & 0x20 ) ? fg : 0; p++;
		*p = ( data & 0x10 ) ? fg : 0; p++;
		*p = ( data & 0x08 ) ? fg : 0; p++;
		*p = ( data & 0x04 ) ? fg : 0; p++;
		*p = ( data & 0x02 ) ? fg : 0; p++;
		*p = ( data & 0x01 ) ? fg : 0; p++;

		data = videoram[ offset + 1 ];

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x40 ) ? fg : 0; p++;
		*p = ( data & 0x20 ) ? fg : 0; p++;
		*p = ( data & 0x10 ) ? fg : 0; p++;
		*p = ( data & 0x08 ) ? fg : 0; p++;
		*p = ( data & 0x04 ) ? fg : 0; p++;
		*p = ( data & 0x02 ) ? fg : 0; p++;
		*p = ( data & 0x01 ) ? fg : 0; p++;
	}
}


static MC6845_UPDATE_ROW( cga_update_row )
{
	if ( cga.update_row )
	{
		cga.update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static MC6845_ON_HSYNC_CHANGED( cga_hsync_changed )
{
	cga.hsync = hsync ? 1 : 0;
}


static MC6845_ON_VSYNC_CHANGED( cga_vsync_changed )
{
	cga.vsync = vsync ? 8 : 0;
	if ( vsync )
	{
		cga.frame++;
	}
}


static void pc_cga_set_palette_luts(void)
{
	/* Setup 2bpp palette lookup table */
	if ( cga.mode_control & 0x10 )
	{
		cga.palette_lut_2bpp[0] = 0;
	}
	else
	{
		cga.palette_lut_2bpp[0] = cga.color_select & 0x0F;
	}
	if ( cga.mode_control & 0x04 )
	{
		cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 3;
		cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 4;
		cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 7;
	}
	else
	{
		if ( cga.color_select & 0x20 )
		{
			cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 3;
			cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 5;
			cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 7;
		}
		else
		{
			cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 2;
			cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 4;
			cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 6;
		}
	}
	//logerror("2bpp lut set to %d,%d,%d,%d\n", cga.palette_lut_2bpp[0], cga.palette_lut_2bpp[1], cga.palette_lut_2bpp[2], cga.palette_lut_2bpp[3]);
}

/*
 *	rW	CGA mode control register (see #P138)
 *
 *  x x x 0 1 0 0 0 - 320x200, 40x25 text. Colour on RGB and composite monitors.
 *  x x x 0 1 0 0 1 - 640x200, 80x25 text. Colour on RGB and composite monitors.
 *  x x x 0 1 0 1 0 - 320x200 graphics. Colour on RGB and composite monitors.
 *  x x x 0 1 0 1 1 - unknown/invalid.
 *  x x x 0 1 1 0 0 - 320x200, 40x25 text. Colour on RGB, greyscale on composite monitors.
 *  x x x 0 1 1 0 1 - 640x200, 80x25 text. Colour on RGB, greyscale on composite monitors.
 *  x x x 0 1 1 1 0 - 320x200 graphics. Alternative palette on RGB, greyscale on composite monitors.
 *  x x x 0 1 1 1 1 - unknown/invalid.
 *  x x x 1 1 0 0 0 - unknown/invalid.
 *  x x x 1 1 0 0 1 - unknown/invalid.
 *  x x x 1 1 0 1 0 - 160x200/640x200 graphics. 640x200 ?? on RGB monitor, 160x200 on composite monitor.
 *  x x x 1 1 0 1 1 - unknown/invalid.
 *  x x x 1 1 1 0 0 - unknown/invalid.
 *  x x x 1 1 1 0 1 - unknown/invalid.
 *  x x x 1 1 1 1 0 - 640x200 graphics. Colour on black on RGB monitor, monochrome on composite monitor.
 *  x x x 1 1 1 1 1 - unknown/invalid.
 */
static void pc_cga_mode_control_w(running_machine *machine, int data)
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);

	CGA_LOG(1,"CGA_mode_control_w",("$%02x: columns %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));

	cga.mode_control = data;

	//logerror("mode set to %02X\n", cga.mode_control & 0x3F );
	switch ( cga.mode_control & 0x3F )
	{
	case 0x08: case 0x09: case 0x0C: case 0x0D:
		mc6845_set_hpixels_per_column( devconf, 8 );
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE )
		{
			if ( cga.mode_control & 0x04 )
			{
				/* Composite greyscale */
				cga.update_row = cga_text_inten_comp_grey_update_row;
			}
			else
			{
				/* Composite colour */
				cga.update_row = cga_text_inten_update_row;
			}
		}
		else
		{
			/* RGB colour */
			cga.update_row = cga_text_inten_update_row;
		}
		break;
	case 0x0A: case 0x0B: case 0x2A: case 0x2B:
		mc6845_set_hpixels_per_column( devconf, 8 );
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE )
		{
			cga.update_row = cga_gfx_4bppl_update_row;
		}
		else
		{
			cga.update_row = cga_gfx_2bpp_update_row;
		}
		break;
	case 0x0E: case 0x0F: case 0x2E: case 0x2F:
		mc6845_set_hpixels_per_column( devconf, 8 );
		cga.update_row = cga_gfx_2bpp_update_row;
		break;
	case 0x18: case 0x19: case 0x1C: case 0x1D:
		mc6845_set_hpixels_per_column( devconf, 8 );
		cga.update_row = cga_text_inten_alt_update_row;
		break;
	case 0x1A: case 0x1B: case 0x3A: case 0x3B:
		mc6845_set_hpixels_per_column( devconf, 16 );
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE )
		{
			cga.update_row = cga_gfx_4bpph_update_row;
		}
		else
		{
			cga.update_row = cga_gfx_1bpp_update_row;
		}
		break;
	case 0x1E: case 0x1F: case 0x3E: case 0x3F:
		mc6845_set_hpixels_per_column( devconf, 16 );
		cga.update_row = cga_gfx_1bpp_update_row;
		break;
	case 0x28: case 0x29: case 0x2C: case 0x2D:
		mc6845_set_hpixels_per_column( devconf, 8 );
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE )
		{
			if ( cga.mode_control & 0x04 )
			{
				/* Composite greyscale */
				cga.update_row = cga_text_blink_update_row;
			}
			else
			{
				/* Composite colour */
				cga.update_row = cga_text_blink_update_row;
			}
		}
		else
		{
			/* RGB colour */
			cga.update_row = cga_text_blink_update_row;
		}
		break;
	case 0x38: case 0x39: case 0x3C: case 0x3D:
		mc6845_set_hpixels_per_column( devconf, 8 );
		cga.update_row = cga_text_blink_alt_update_row;
		break;
	default:
		cga.update_row = NULL;
		break;
	}

	pc_cga_set_palette_luts();
}



/*
 *	?W	reserved for color select register on color adapter
 */
static void pc_cga_color_select_w(int data)
{
	CGA_LOG(1,"CGA_color_select_w",("$%02x\n", data));
	cga.color_select = data;
	//logerror("color_select_w: %02X\n", data);
	pc_cga_set_palette_luts();
}



/*
 * Select Plantronics modes
 */
static void pc_cga_plantronics_w(int data)
{
	CGA_LOG(1,"CGA_plantronics_w",("$%02x\n", data));

	if (CGA_CHIPSET != CGA_CHIPSET_ATI) return;

	data &= 0x70;	/* Only bits 6-4 are used */
	if (cga.plantronics == data) return;
	cga.plantronics = data;
}



/*************************************************************************
 *
 *		CGA
 *		color graphics adapter
 *
 *************************************************************************/

static READ8_HANDLER( pc_cga8_r )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			/* return last written mc6845 address value here? */
			break;
		case 1: case 3: case 5: case 7:
			data = mc6845_register_r( devconf, offset );
			break;
		case 10:
			data = cga.vsync | ( ( data & 0x40 ) >> 4 ) | cga.hsync;
			break;
    }
	return data;
}



static WRITE8_HANDLER( pc_cga8_w )
{
	device_config	*devconf;

	switch(offset) {
	case 0: case 2: case 4: case 6:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		mc6845_address_w( devconf, offset, data );
		break;
	case 1: case 3: case 5: case 7:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		mc6845_register_w( devconf, offset, data );
		break;
	case 8:
		pc_cga_mode_control_w(machine, data);
		break;
	case 9:
		pc_cga_color_select_w(data);
		break;
	case 0x0d:
		pc_cga_plantronics_w(data);
		break;
	}
}



static READ16_HANDLER( pc_cga16le_r ) { return read16le_with_read8_handler(pc_cga8_r,machine,  offset, mem_mask); }
static WRITE16_HANDLER( pc_cga16le_w ) { write16le_with_write8_handler(pc_cga8_w, machine, offset, data, mem_mask); }
static READ32_HANDLER( pc_cga32le_r ) { return read32le_with_read8_handler(pc_cga8_r, machine, offset, mem_mask); }
static WRITE32_HANDLER( pc_cga32le_w ) { write32le_with_write8_handler(pc_cga8_w, machine, offset, data, mem_mask); }


/* Old plantronics rendering code, leaving it uncommented until we have re-implemented it */

//
// From choosevideomode:
//
//      /* Plantronics high-res */
//      if ((cga.mode_control & 2) && (cga.plantronics & 0x20))
//          proc = cga_pgfx_2bpp;
//      /* Plantronics low-res */
//      if ((cga.mode_control & 2) && (cga.plantronics & 0x10))
//          proc = cga_pgfx_4bpp;
//

//INLINE void pgfx_plot_unit_4bpp(bitmap_t *bitmap,
//							 int x, int y, int offs)
//{
//	int color, values[2];
//	int i;
//
//	if (cga.plantronics & 0x40)
//	{
//		values[0] = videoram[offs | 0x4000];
//		values[1] = videoram[offs];
//	}
//	else
//	{
//		values[0] = videoram[offs];
//		values[1] = videoram[offs | 0x4000];
//	}
//	for (i=3; i>=0; i--)
//	{
//		color = ((values[0] & 0x3) << 1) |
//			((values[1] & 2)   >> 1) |
//			((values[1] & 1)   << 3);
//		*BITMAP_ADDR16(bitmap, y, x+i) = Machine->pens[color];
//		values[0]>>=2;
//		values[1]>>=2;
//	}
//}
//
//
//
///***************************************************************************
//  Draw graphics mode with 640x200 pixels (default) with 2 bits/pixel.
//  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
//  Second plane at CGA_base + 0x4000 / 0x6000
//***************************************************************************/
//
//static void cga_pgfx_4bpp(bitmap_t *bitmap, struct mscrtc6845 *crtc)
//{
//	int i, sx, sy, sh;
//	int	offs = mscrtc6845_get_start(crtc)*2;
//	int lines = mscrtc6845_get_char_lines(crtc);
//	int height = mscrtc6845_get_char_height(crtc);
//	int columns = mscrtc6845_get_char_columns(crtc)*2;
//
//	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff)
//	{
//		for (sh=0; sh<height; sh++, offs|=0x2000)
//		{
//			// char line 0 used as a12 line in graphic mode
//			if (!(sh & 1))
//			{
//				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
//				{
//					pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
//				}
//			}
//			else
//			{
//				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
//				{
//					pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
//				}
//			}
//		}
//	}
//}
//
//
//
//INLINE void pgfx_plot_unit_2bpp(bitmap_t *bitmap,
//					 int x, int y, const UINT16 *palette, int offs)
//{
//	int i;
//	UINT8 bmap[2], values[2];
//	UINT16 *dest;
//
//	if (cga.plantronics & 0x40)
//	{
//		values[0] = videoram[offs];
//		values[1] = videoram[offs | 0x4000];
//	}
//	else
//	{
//		values[0] = videoram[offs | 0x4000];
//		values[1] = videoram[offs];
//	}
//	bmap[0] = bmap[1] = 0;
//	for (i=3; i>=0; i--)
//	{
//		bmap[0] = bmap[0] << 1; if (values[0] & 0x80) bmap[0] |= 1;
//		bmap[0] = bmap[0] << 1; if (values[1] & 0x80) bmap[0] |= 1;
//		bmap[1] = bmap[1] << 1; if (values[0] & 0x08) bmap[1] |= 1;
//		bmap[1] = bmap[1] << 1; if (values[1] & 0x08) bmap[1] |= 1;
//		values[0] = values[0] << 1;
//		values[1] = values[1] << 1;
//	}
//
//	dest = BITMAP_ADDR16(bitmap, y, x);
//	*(dest++) = palette[(bmap[0] >> 6) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 4) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 2) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 0) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 6) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 4) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 2) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 0) & 0x03];
//}
//
//
//
///***************************************************************************
//  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
//  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
//  cga fetches 2 byte per mscrtc6845 access (not modeled here)!
//***************************************************************************/
//
//static void cga_pgfx_2bpp(bitmap_t *bitmap, struct mscrtc6845 *crtc)
//{
//	int i, sx, sy, sh;
//	int	offs = mscrtc6845_get_start(crtc)*2;
//	int lines = mscrtc6845_get_char_lines(crtc);
//	int height = mscrtc6845_get_char_height(crtc);
//	int columns = mscrtc6845_get_char_columns(crtc)*2;
//	int colorset = cga.color_select & 0x3F;
//	const UINT16 *palette;
//
//	/* Most chipsets use bit 2 of the mode control register to
//	 * access a third palette. But not consistently. */
//	pc_cga_check_palette();
//	switch(CGA_CHIPSET)
//	{
//		/* The IBM Professional Graphics Controller behaves like
//		 * the PC1512, btw. */
//		case CGA_CHIPSET_PC1512:
//		if ((colorset < 32) && (cga.mode_control & 4)) colorset += 64;
//		break;
//
//		case CGA_CHIPSET_IBM:
//		case CGA_CHIPSET_PC200:
//		case CGA_CHIPSET_ATI:
//		case CGA_CHIPSET_PARADISE:
//		if (cga.mode_control & 4) colorset = (colorset & 0x1F) + 64;
//		break;
//	}
//
//
//	/* The fact that our palette is located in cga_colortable is a vestigial
//	 * aspect from when we were doing that ugly trick where drawgfx() would
//	 * handle graphics drawing.  Truthfully, we should probably be using
//	 * palette_set_color_rgb() here and not doing the palette lookup in the loop
//	 */
//	palette = &cga_colortable[256*2 + 16*2] + colorset * 4;
//
//	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {
//
//		for (sh=0; sh<height; sh++)
//		{
//			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
//				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
//				{
//					pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, palette, i);
//				}
//			}
//			else
//			{
//				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
//				{
//					pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, palette, i);
//				}
//			}
//		}
//	}
//}



// amstrad pc1512 video hardware
// mapping of the 4 planes into videoram
// (text data should be readable at videoram+0)
static const int videoram_offset[4]= { 0x0000, 0x4000, 0x8000, 0xC000 };


static const UINT8 mc6845_writeonce_register[31] =
{
	1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static struct
{
	UINT8	write;
	UINT8	read;
	UINT8	mc6845_address;
	UINT8	mc6845_locked_register[31];
} pc1512;


static MC6845_UPDATE_ROW( pc1512_gfx_4bpp_update_row )
{
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT16	offset_base = ra << 13;
	int i;

	if ( y == 0 ) CGA_LOG(1,"pc1512_gfx_4bpp_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = offset_base | ( ( ma + i ) & 0x1FFF );
		UINT16 i = ( cga.color_select & 8 ) ? videoram[ videoram_offset[3] | offset ] << 3 : 0;
		UINT16 r = ( cga.color_select & 4 ) ? videoram[ videoram_offset[2] | offset ] << 2 : 0;
		UINT16 g = ( cga.color_select & 2 ) ? videoram[ videoram_offset[1] | offset ] << 1 : 0;
		UINT16 b = ( cga.color_select & 1 ) ? videoram[ videoram_offset[0] | offset ]      : 0;

		*p = ( ( i & 0x400 ) | ( r & 0x200 ) | ( g & 0x100 ) | ( b & 0x80 ) ) >> 7; p++;
		*p = ( ( i & 0x200 ) | ( r & 0x100 ) | ( g & 0x080 ) | ( b & 0x40 ) ) >> 6; p++;
		*p = ( ( i & 0x100 ) | ( r & 0x080 ) | ( g & 0x040 ) | ( b & 0x20 ) ) >> 5; p++;
		*p = ( ( i & 0x080 ) | ( r & 0x040 ) | ( g & 0x020 ) | ( b & 0x10 ) ) >> 4; p++;
		*p = ( ( i & 0x040 ) | ( r & 0x020 ) | ( g & 0x010 ) | ( b & 0x08 ) ) >> 3; p++;
		*p = ( ( i & 0x020 ) | ( r & 0x010 ) | ( g & 0x008 ) | ( b & 0x04 ) ) >> 2; p++;
		*p = ( ( i & 0x010 ) | ( r & 0x008 ) | ( g & 0x004 ) | ( b & 0x02 ) ) >> 1; p++;
		*p =   ( i & 0x008 ) | ( r & 0x004 ) | ( g & 0x002 ) | ( b & 0x01 )       ; p++;
	}
}


static WRITE8_HANDLER ( pc1512_w )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);

	switch (offset)
	{
	case 0: case 2: case 4: case 6:
		data &= 0x1F;
		mc6845_address_w( devconf, offset, data );
		pc1512.mc6845_address = data;
		break;

	case 1: case 3: case 5: case 7:
		if ( ! pc1512.mc6845_locked_register[pc1512.mc6845_address] )
		{
			mc6845_register_w( devconf, offset, data );
			if ( mc6845_writeonce_register[pc1512.mc6845_address] )
			{
				pc1512.mc6845_locked_register[pc1512.mc6845_address] = 1;
			}
		}
		break;

	case 0x8:
		/* Check if we're changing to graphics mode 2 */
		if ( ( cga.mode_control & 0x12 ) != 0x12 && ( data & 0x12 ) == 0x12 )
		{
			pc1512.write = 0x0F;
		}
		else
		{
			memory_set_bankptr(1, videoram + videoram_offset[0]);
		}
		cga.mode_control = data;
		switch( cga.mode_control & 0x3F )
		{
		case 0x08: case 0x09: case 0x0C: case 0x0D:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = cga_text_inten_update_row;
			break;
		case 0x0A: case 0x0B: case 0x2A: case 0x2B:
			mc6845_set_hpixels_per_column( devconf, 8 );
			if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE )
			{
				cga.update_row = cga_gfx_4bppl_update_row;
			}
			else
			{
				cga.update_row = cga_gfx_2bpp_update_row;
			}
			break;
		case 0x0E: case 0x0F: case 0x2E: case 0x2F:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = cga_gfx_2bpp_update_row;
			break;
		case 0x18: case 0x19: case 0x1C: case 0x1D:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = cga_text_inten_alt_update_row;
			break;
		case 0x1A: case 0x1B: case 0x3A: case 0x3B:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = pc1512_gfx_4bpp_update_row;
			break;
		case 0x1E: case 0x1F: case 0x3E: case 0x3F:
			mc6845_set_hpixels_per_column( devconf, 16 );
			cga.update_row = cga_gfx_1bpp_update_row;
			break;
		case 0x28: case 0x29: case 0x2C: case 0x2D:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = cga_text_blink_update_row;
			break;
		case 0x38: case 0x39: case 0x3C: case 0x3D:
			mc6845_set_hpixels_per_column( devconf, 8 );
			cga.update_row = cga_text_blink_alt_update_row;
			break;
		default:
			cga.update_row = NULL;
			break;
		}
		break;

	case 0xd:
		pc1512.write = data;
		break;

	case 0xe:
		pc1512.read = data;
		if ( ( cga.mode_control & 0x12 ) == 0x12 )
		{
			memory_set_bankptr(1, videoram + videoram_offset[data & 3]);
		}
		break;

	default:
		pc_cga8_w(machine, offset,data);
		break;
	}
}

static READ8_HANDLER ( pc1512_r )
{
	UINT8 data;

	switch (offset)
	{
	case 0xd:
		data = pc1512.write;
		break;

	case 0xe:
		data = pc1512.read;
		break;

	default:
		data = pc_cga8_r(machine, offset);
		break;
	}
	return data;
}


static WRITE8_HANDLER ( pc1512_videoram_w )
{
	if ( ( cga.mode_control & 0x12 ) == 0x12 )
	{
		if (pc1512.write & 1)
			videoram[offset+videoram_offset[0]] = data; /* blue plane */
		if (pc1512.write & 2)
			videoram[offset+videoram_offset[1]] = data; /* green */
		if (pc1512.write & 4)
			videoram[offset+videoram_offset[2]] = data; /* red */
		if (pc1512.write & 8)
			videoram[offset+videoram_offset[3]] = data; /* intensity (text, 4color) */
	}
	else
	{
		videoram[offset + videoram_offset[0]] = data;
	}
}



READ16_HANDLER ( pc1512_16le_r ) { return read16le_with_read8_handler(pc1512_r, machine, offset, mem_mask); }
WRITE16_HANDLER ( pc1512_16le_w ) { write16le_with_write8_handler(pc1512_w, machine, offset, data, mem_mask); }
WRITE16_HANDLER ( pc1512_videoram16le_w ) { write16le_with_write8_handler(pc1512_videoram_w, machine, offset, data, mem_mask); }



static VIDEO_START( pc1512 )
{
	videoram_size = 0x10000;
	videoram = auto_malloc( videoram_size );
	memory_set_bankptr(1,videoram + videoram_offset[0]);

	memset( &pc1512, 0, sizeof ( pc1512 ) );
	pc1512.write = 0xf;
	pc1512.read = 0;

	/* PC1512 cut-down 6845 */
	internal_pc_cga_video_start(machine, M6845_PERSONALITY_PC1512);
}


static VIDEO_UPDATE( mc6845_pc1512 )
{
	UINT8 *gfx = memory_region(screen->machine, REGION_GFX1);
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect);

	/* Check for changes in font dipsetting */
	switch ( CGA_FONT & 0x03 )
	{
	case 0:
		cga.chr_gen = gfx + 0x0000;
		break;
	case 1:
		cga.chr_gen = gfx + 0x0800;
		break;
	case 2:
		cga.chr_gen = gfx + 0x1000;
		break;
	case 3:
		cga.chr_gen = gfx + 0x1800;
		break;
	}

	return 0;
}


