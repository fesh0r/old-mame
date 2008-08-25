/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

#include "driver.h"
#include "pc_mda.h"
#include "pc_video.h"
#include "video/mc6845.h"

#define VERBOSE_MDA	0		/* MDA (Monochrome Display Adapter) */

#define MDA_CLOCK	16257000

#define MDA_LOG(N,M,A) \
	if(VERBOSE_MDA>=N){ if( M )logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()),(char*)M ); logerror A; }

static const unsigned char mda_palette[4][3] =
{
	{ 0x00,0x00,0x00 },
	{ 0x00,0x55,0x00 },
	{ 0x00,0xaa,0x00 },
	{ 0x00,0xff,0x00 }
};

static struct
{
	UINT8 status;

	int pc_framecnt;

	UINT8 mode_control, configuration_switch; //hercules

	gfx_element *gfx_char[4];

	mc6845_update_row_func  update_row;
	UINT8   *chr_gen;
	UINT8   vsync;
	UINT8   hsync;
} mda;

/* Initialise the mda palette */
static PALETTE_INIT( pc_mda )
{
	int i;
	for(i = 0; i < (sizeof(mda_palette) / 3); i++)
		palette_set_color_rgb(machine, i, mda_palette[i][0], mda_palette[i][1], mda_palette[i][2]);
}

static VIDEO_START( pc_mda );
static VIDEO_UPDATE( mc6845_mda );
static READ8_HANDLER( pc_MDA_r );
static WRITE8_HANDLER( pc_MDA_w );
static MC6845_UPDATE_ROW( mda_update_row );
static MC6845_ON_HSYNC_CHANGED( mda_hsync_changed );
static MC6845_ON_VSYNC_CHANGED( mda_vsync_changed );

static const mc6845_interface mc6845_mda_intf =
{
	MDA_SCREEN_NAME,	/* screen number */
	MDA_CLOCK/9 /*?*/,	/* clock */
	9,					/* number of pixels per video memory address */
	NULL,				/* begin_update */
	mda_update_row,		/* update_row */
	NULL,				/* end_update */
	NULL,				/* on_de_changed */
	mda_hsync_changed,	/* on_hsync_changed */
	mda_vsync_changed	/* on_vsync_changed */
};

MACHINE_DRIVER_START( pcvideo_mda )
	MDRV_SCREEN_ADD( MDA_SCREEN_NAME, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(MDA_CLOCK, 882, 0, 720, 370, 0, 350 )
	MDRV_PALETTE_LENGTH( sizeof(mda_palette) / 3 )

	MDRV_PALETTE_INIT(pc_mda)

	MDRV_DEVICE_ADD( MDA_MC6845_NAME, MC6845)
	MDRV_DEVICE_CONFIG( mc6845_mda_intf )

	MDRV_VIDEO_START( pc_mda )
	MDRV_VIDEO_UPDATE( mc6845_mda)
MACHINE_DRIVER_END


/***************************************************************************

  Monochrome Display Adapter (MDA) section

***************************************************************************/

VIDEO_START( pc_mda )
{
	int buswidth;

	buswidth = cputype_databus_width(machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb0fff, 0, 0x07000, SMH_BANK11 );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xb0fff, 0, 0x07000, pc_video_videoram_w );
			memory_install_read8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_r );
			memory_install_write8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_MDA_w );
			break;

		default:
			fatalerror("MDA:  Bus width %d not supported\n", buswidth);
			break;
	}

	memset( &mda, 0, sizeof(mda));
	mda.update_row = NULL;
	mda.chr_gen = memory_region( machine, "gfx1" );

	videoram_size = 0x1000;	/* This is actually 0x1000 in reality */
	videoram = auto_malloc(videoram_size);
	memory_set_bankptr(11, videoram);
}


static VIDEO_UPDATE( mc6845_mda )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, MDA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect );
	return 0;
}


/***************************************************************************
  Draw text mode with 80x25 characters (default) and intense background.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/

static MC6845_UPDATE_ROW( mda_text_inten_update_row )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	chr_base = ( ra & 0x08 ) ? 0x800 | ( ra & 0x07 ) : ra;
	int i;

	if ( y == 0 ) MDA_LOG(1,"mda_text_inten_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x0FFF;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset + 1 ];
		UINT8 data = mda.chr_gen[ chr_base + chr * 8 ];
		UINT8 fg = ( attr & 0x08 ) ? 3 : 2;
		UINT8 bg = 0;

		if ( ( attr & ~0x88 ) == 0 )
		{
			data = 0x00;
		}

		switch( attr )
		{
		case 0x70:
			bg = 2;
			fg = 0;
			break;
		case 0x78:
			bg = 2;
			fg = 1;
			break;
		case 0xF0:
			bg = 3;
			fg = 0;
			break;
		case 0xF8:
			bg = 3;
			fg = 1;
			break;
		}

		if ( ( i == cursor_x && ( mda.pc_framecnt & 0x08 ) ) || ( attr & 0x07 ) == 0x01 )
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
		if ( ( chr & 0xE0 ) == 0xC0 )
		{
			*p = ( data & 0x01 ) ? fg : bg; p++;
		}
		else
		{
			*p = bg; p++;
		}
	}
}


/***************************************************************************
  Draw text mode with 80x25 characters (default) and blinking characters.
  The character cell size is 9x15. Column 9 is column 8 repeated for
  character codes 176 to 223.
***************************************************************************/

static MC6845_UPDATE_ROW( mda_text_blink_update_row )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	chr_base = ( ra & 0x08 ) ? 0x800 | ( ra & 0x07 ) : ra;
	int i;

	if ( y == 0 ) MDA_LOG(1,"mda_text_blink_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x0FFF;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset + 1 ];
		UINT8 data = mda.chr_gen[ chr_base + chr * 8 ];
		UINT8 fg = ( attr & 0x08 ) ? 3 : 2;
		UINT8 bg = 0;

		if ( ( attr & ~0x88 ) == 0 )
		{
			data = 0x00;
		}

		switch( attr )
		{
		case 0x70:
		case 0xF0:
			bg = 2;
			fg = 0;
			break;
		case 0x78:
		case 0xF8:
			bg = 2;
			fg = 1;
			break;
		}

		if ( ( attr & 0x07 ) == 0x01 )
		{
			data = 0xFF;
		}

		if ( i == cursor_x )
		{
			if ( mda.pc_framecnt & 0x08 )
			{
				data = 0xFF;
			}
		}
		else
		{
			if ( ( attr & 0x80 ) && ( mda.pc_framecnt & 0x10 ) )
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
		if ( ( chr & 0xE0 ) == 0xC0 )
		{
			*p = ( data & 0x01 ) ? fg : bg; p++;
		}
		else
		{
			*p = bg; p++;
		}
	}
}


static MC6845_UPDATE_ROW( mda_update_row )
{
	if ( mda.update_row )
	{
		mda.update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static MC6845_ON_HSYNC_CHANGED( mda_hsync_changed )
{
	mda.hsync = hsync ? 1 : 0;
}


static MC6845_ON_VSYNC_CHANGED( mda_vsync_changed )
{
	mda.vsync = vsync ? 0x80 : 0;
	if ( vsync )
	{
		mda.pc_framecnt++;
	}
}


/*
 *	rW	MDA mode control register (see #P138)
 */
static void mda_mode_control_w(int data)
{
	MDA_LOG(1,"MDA_mode_control_w",("$%02x: colums %d, gfx %d, enable %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>3)&1, (data>>5)&1));
	mda.mode_control = data;

	switch( mda.mode_control & 0x2a )
	{
	case 0x08:
		mda.update_row = mda_text_inten_update_row;
		break;
	case 0x28:
		mda.update_row = mda_text_blink_update_row;
		break;
	default:
		mda.update_row = NULL;
	}
}


/*	R-	CRT status register (see #P139)
 *		(EGA/VGA) input status 1 register
 *		7	 HGC vertical sync in progress
 *		6-4  adapter 000  hercules
 *					 001  hercules+
 *					 101  hercules InColor
 *					 else unknown
 *		3	 pixel stream (0 black, 1 white)
 *		2-1  reserved
 *		0	 horizontal drive enable
 */
static int pc_mda_status_r(void)
{
    int data = 0x08 | mda.hsync;
	return data;
}


/*************************************************************************
 *
 *		MDA
 *		monochrome display adapter
 *
 *************************************************************************/
WRITE8_HANDLER ( pc_MDA_w )
{
	device_config   *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, MDA_MC6845_NAME);
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			mc6845_address_w( devconf, offset, data );
			break;
		case 1: case 3: case 5: case 7:
			mc6845_register_w( devconf, offset, data );
			break;
		case 8:
			mda_mode_control_w(data);
			break;
	}
}

 READ8_HANDLER ( pc_MDA_r )
{
	device_config   *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, MDA_MC6845_NAME);
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
			data = pc_mda_status_r();
			break;
		/* 12, 13, 14  are the LPT1 ports */
    }
	return data;
}



/***************************************************************************

  Hercules Display Adapter section (re-uses parts from the MDA section)

***************************************************************************/

static VIDEO_START( pc_hercules );
static VIDEO_UPDATE( mc6845_hercules );
static READ8_HANDLER( pc_hercules_r );
static WRITE8_HANDLER( pc_hercules_w );

/*
When the Hercules changes to graphics mode, the number of pixels per access and
clock divider should be changed. The currect mc6845 implementation does not
allow this.

The divder/pixels per 6845 clock is 9 for text mode and 16 for graphics mode.
*/

static const mc6845_interface mc6845_hercules_intf =
{
	HERCULES_SCREEN_NAME,	/* screen number */
	MDA_CLOCK/9 /*?*/,		/* clock */
	9,						/* number of pixels per video memory address */
	NULL,					/* begin_update */
	mda_update_row,			/* update_row */
	NULL,					/* end_update */
	NULL,					/* on_de_changed */
	mda_hsync_changed,		/* on_hsync_changed */
	mda_vsync_changed		/* on_vsync_changed */
};

MACHINE_DRIVER_START( pcvideo_hercules )
	MDRV_SCREEN_ADD( HERCULES_SCREEN_NAME, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(MDA_CLOCK, 882, 0, 720, 370, 0, 350 )
	MDRV_PALETTE_LENGTH( sizeof(mda_palette) / 3 )

	MDRV_PALETTE_INIT(pc_mda)

	MDRV_DEVICE_ADD( HERCULES_MC6845_NAME, MC6845)
	MDRV_DEVICE_CONFIG( mc6845_hercules_intf )

	MDRV_VIDEO_START( pc_hercules )
	MDRV_VIDEO_UPDATE( mc6845_hercules )
MACHINE_DRIVER_END


static VIDEO_START( pc_hercules )
{
	int buswidth;

	buswidth = cputype_databus_width(machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
	case 8:
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, SMH_BANK11 );
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0xb0000, 0xbffff, 0, 0, pc_video_videoram_w );
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_hercules_r );
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_hercules_w );
		break;

	default:
		fatalerror("Hercules:  Bus width %d not supported\n", buswidth);
		break;
	}

	memset( &mda, 0, sizeof(mda));
	mda.update_row = NULL;
	mda.chr_gen = memory_region( machine, "gfx1" );

	videoram_size = 0x10000;
	videoram = auto_malloc(videoram_size);
	memory_set_bankptr(11, videoram);
}


/***************************************************************************
  Draw graphics with 720x348 pixels (default); so called Hercules gfx.
  The memory layout is divided into 4 banks where of size 0x2000.
  Every bank holds data for every n'th scanline, 8 pixels per byte,
  bit 7 being the leftmost.
***************************************************************************/

static MC6845_UPDATE_ROW( hercules_gfx_update_row )
{
	UINT16	*p = BITMAP_ADDR16( bitmap, y, 0 );
	UINT16	gfx_base = ( ( mda.mode_control & 0x80 ) ? 0x8000 : 0x0000 ) | ( ( ra & 0x03 ) << 13 );
	int i;

	if ( y == 0 ) MDA_LOG(1,"hercules_gfx_update_row",("\n"));
	for ( i = 0; i < x_count; i++ )
	{
		UINT8	data = videoram[ gfx_base + ( ( ma + i ) << 1 ) ];

		*p = ( data & 0x80 ) ? 2 : 0; p++;
		*p = ( data & 0x40 ) ? 2 : 0; p++;
		*p = ( data & 0x20 ) ? 2 : 0; p++;
		*p = ( data & 0x10 ) ? 2 : 0; p++;
		*p = ( data & 0x08 ) ? 2 : 0; p++;
		*p = ( data & 0x04 ) ? 2 : 0; p++;
		*p = ( data & 0x02 ) ? 2 : 0; p++;
		*p = ( data & 0x01 ) ? 2 : 0; p++;

		data = videoram[ gfx_base + ( ( ma + i ) << 1 ) + 1 ];

		*p = ( data & 0x80 ) ? 2 : 0; p++;
		*p = ( data & 0x40 ) ? 2 : 0; p++;
		*p = ( data & 0x20 ) ? 2 : 0; p++;
		*p = ( data & 0x10 ) ? 2 : 0; p++;
		*p = ( data & 0x08 ) ? 2 : 0; p++;
		*p = ( data & 0x04 ) ? 2 : 0; p++;
		*p = ( data & 0x02 ) ? 2 : 0; p++;
		*p = ( data & 0x01 ) ? 2 : 0; p++;
	}
}


static VIDEO_UPDATE( mc6845_hercules )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, HERCULES_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect );
	return 0;
}


static void hercules_mode_control_w(running_machine *machine, int data)
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, HERCULES_MC6845_NAME);

	MDA_LOG(1,"hercules_mode_control_w",("$%02x: colums %d, gfx %d, enable %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>3)&1, (data>>5)&1));
	mda.mode_control = data;

	switch( mda.mode_control & 0x2a )
	{
	case 0x08:
		mda.update_row = mda_text_inten_update_row;
		break;
	case 0x28:
		mda.update_row = mda_text_blink_update_row;
		break;
	case 0x0A:          /* Hercules modes */
	case 0x2A:
		mda.update_row = hercules_gfx_update_row;
		break;
	default:
		mda.update_row = NULL;
	}

	mc6845_set_clock( devconf, mda.mode_control & 0x02 ? MDA_CLOCK / 16 : MDA_CLOCK / 9 );
	mc6845_set_hpixels_per_column( devconf, mda.mode_control & 0x02 ? 16 : 9 );
}


static void hercules_config_w(int data)
{
	MDA_LOG(1,"HGC_config_w",("$%02x\n", data));
	mda.configuration_switch = data;
}


static WRITE8_HANDLER ( pc_hercules_w )
{
	device_config   *devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, HERCULES_MC6845_NAME);

	switch( offset )
	{
	case 0: case 2: case 4: case 6:
		mc6845_address_w( devconf, offset, data );
		break;
	case 1: case 3: case 5: case 7:
		mc6845_register_w( devconf, offset, data );
		break;
	case 8:
		hercules_mode_control_w(machine, data);
		break;
	case 15:
		hercules_config_w(data);
		break;
	}
}


/*  R-  CRT status register (see #P139)
 *      (EGA/VGA) input status 1 register
 *      7    HGC vertical sync in progress
 *      6-4  adapter 000  hercules
 *                   001  hercules+
 *                   101  hercules InColor
 *                   else unknown
 *      3    pixel stream (0 black, 1 white)
 *      2-1  reserved
 *      0    horizontal drive enable
 */
static int pc_hercules_status_r(void)
{
	int data = mda.vsync | 0x08 | mda.hsync;
	return data;
}


static READ8_HANDLER ( pc_hercules_r )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, HERCULES_MC6845_NAME);
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
		data = pc_hercules_status_r();
		break;
		/* 12, 13, 14  are the LPT1 ports */
	}
	return data;
}

