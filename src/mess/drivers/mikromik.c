#include "driver.h"
#include "includes/mikromik.h"
#include "devices/basicdsk.h"
#include "cpu/i8085/i8085.h"
#include "machine/8237dma.h"
#include "machine/nec765.h"
#include "video/i8275.h"
#include "video/upd7220.h"

/*

	TODO:

	- PCB layout
	- character generator
	- memory mapped I/O
	- pixel display
	- system disks
	- hook up DMA channels
	- UPD uPD7220 emulation

*/

#define MM1_DMA_I8275	0
#define MM1_DMA_I8272	0
#define MM1_DMA_UPD7220	0

/* Video */

static I8275_DMA_REQUEST( crtc_dma_request )
{
	mm1_state *driver_state = device->machine->driver_data;
	//logerror("i8275 DMA\n");
	dma8237_drq_write(driver_state->i8237, MM1_DMA_I8275, state);
}

static I8275_INT_REQUEST( crtc_int_request )
{
	//logerror("i8275 INT\n");
}

static I8275_DISPLAY_PIXELS( crtc_display_pixels )
{
}

static const i8275_interface mm1_i8275_intf = 
{
	SCREEN_TAG,
	8,
	0,
	crtc_dma_request,
	crtc_int_request,
	crtc_display_pixels
};

static UPD7220_DISPLAY_PIXELS( hgdc_display_pixels )
{
}

static WRITE_LINE_DEVICE_HANDLER( hgdc_drq_w )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, MM1_DMA_UPD7220, state);
}

static UPD7220_INTERFACE( mm1_upd7220_intf )
{
	SCREEN_TAG,
	8,
	hgdc_display_pixels,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(hgdc_drq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static VIDEO_UPDATE( mm1 )
{
	mm1_state *state = screen->machine->driver_data;

	i8275_update(state->i8275, bitmap, cliprect);

	return 0;
}

static DMA8237_MEM_READ( memory_dma_r )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA read %04x\n", offset);
	return memory_read_byte(program, offset);
}

static DMA8237_MEM_WRITE( memory_dma_w )
{
	const address_space *program = cputag_get_address_space(device->machine, I8085_TAG, ADDRESS_SPACE_PROGRAM);
	//logerror("DMA write %04x:%02x\n", offset, data);
	memory_write_byte(program, offset, data);
}

static DMA8237_CHANNEL_WRITE( crtc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	i8275_dack_set_data(state->i8275, data);
}

#ifdef UNUSED_CODE
static DMA8237_CHANNEL_WRITE( hgdc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	upd7220_dack_w(state->upd7220, 0, data);
}

static DMA8237_CHANNEL_READ( fdc_dack_r )
{
	mm1_state *state = device->machine->driver_data;

	return nec765_dack_r(state->i8272, 0);
}

static DMA8237_CHANNEL_WRITE( fdc_dack_w )
{
	mm1_state *state = device->machine->driver_data;

	nec765_dack_w(state->i8272, 0, data);
}
#endif

static DMA8237_OUT_EOP( dma_eop_w )
{
}

static const struct dma8237_interface mm1_dma8237_intf =
{
	I8085_TAG,
	1.0e-6, // 1us???
	memory_dma_r,
	memory_dma_w,
	{ NULL, NULL, NULL, NULL },
	{ crtc_dack_w, NULL, NULL, NULL },
	dma_eop_w
};

static NEC765_INTERRUPT( fdc_irq )
{
}

static NEC765_DMA_REQUEST( fdc_drq )
{
	mm1_state *driver_state = device->machine->driver_data;

	dma8237_drq_write(driver_state->i8237, MM1_DMA_I8272, state);
}

static const nec765_interface mm1_nec765_intf =
{
	fdc_irq,
	fdc_drq,
	NULL,
	NEC765_RDY_PIN_CONNECTED // ???
};

/* Memory Maps */

static ADDRESS_MAP_START( mm1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xfeff) AM_RAM
	AM_RANGE(0xff00, 0xff0f) AM_DEVREADWRITE(I8237_TAG, dma8237_r, dma8237_w)
    AM_RANGE(0xff20, 0xff21) AM_DEVREADWRITE(I8275_TAG, i8275_r, i8275_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( mm1 )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( mm1 )
{
	mm1_state *state = machine->driver_data;

	/* look up devices */
	state->i8237 = devtag_get_device(machine, I8237_TAG);
	state->i8272 = devtag_get_device(machine, I8272_TAG);
	state->i8275 = devtag_get_device(machine, I8275_TAG);

	/* register for state saving */
	//state_save_register_global(machine, state->);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( mm1 )
	MDRV_DRIVER_DATA(mm1_state)

	/* basic system hardware */
	MDRV_CPU_ADD(I8085_TAG, 8085A, 2000000)
	MDRV_CPU_PROGRAM_MAP(mm1_map, 0)

	MDRV_MACHINE_START(mm1)

	/* video hardware */
	MDRV_SCREEN_ADD( SCREEN_TAG, RASTER )
	MDRV_SCREEN_REFRESH_RATE( 50 )
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 800, 327 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 800-1, 0, 327-1 )

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(mm1)

	/* peripheral hardware */
	MDRV_I8275_ADD(I8275_TAG, mm1_i8275_intf)
	MDRV_DMA8237_ADD(I8237_TAG, mm1_dma8237_intf)
	MDRV_NEC765A_ADD(I8272_TAG, mm1_nec765_intf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( mm1m6 )
	MDRV_IMPORT_FROM(mm1)

	MDRV_UPD7220_ADD(UPD7220_TAG, 0, mm1_upd7220_intf)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( mm1m6 )
	ROM_REGION( 0x1000, I8085_TAG, 0 )
	ROM_LOAD( "mm1.bin", 0x0000, 0x1000, CRC(07400e72) SHA1(354ff97817a607ca38d296af8b2813878d092a08) )

	ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "chargen", 0x0000, 0x1000, NO_DUMP )
ROM_END

#define rom_mm1m7 rom_mm1m6

/* System Configuration */

static DEVICE_IMAGE_LOAD( mm1_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		if (size == 80*2*16*256)  // 640KB
		{
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 80, 2, 16, 256, 1, 0, FALSE);

			return INIT_PASS;
		}
	}

	return INIT_FAIL;
}

#ifdef UNUSED_CODE
static DEVICE_IMAGE_LOAD( mm2_floppy )
{
	if (image_has_been_created(image))
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image) == INIT_PASS)
	{
		int size = image_length(image);

		switch (size)
		{
		case 40*2*9*512: // 360KB
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 9, 512, 1, 0, FALSE);
			break;

		case 40*2*18*512: // 720KB (number of sectors has been doubled, instead of tracks as in 720KB DOS floppies)
			/* image, tracks, heads, sectors per track, sector length, first sector id, offset track zero, track skipping */
			basicdsk_set_geometry(image, 40, 2, 18, 512, 1, 0, FALSE);
			break;

		default:
			return INIT_FAIL;
		}

		return INIT_PASS;
	}

	return INIT_FAIL;
}
#endif

static void dual_640kb_floppy(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:						info->load = DEVICE_IMAGE_LOAD_NAME(mm1_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:			strcpy(info->s = device_temp_str(), "img"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( mm1m6 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( mm1m7 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
	CONFIG_DEVICE(dual_640kb_floppy)
	// 5MB Winchester
SYSTEM_CONFIG_END

/* System Drivers */

//    YEAR  NAME		PARENT  COMPAT	MACHINE		INPUT		INIT	CONFIG    COMPANY			FULLNAME				FLAGS
COMP( 1981, mm1m6,		0,		0,		mm1m6,		mm1,		0, 		mm1m6,	  "Nokia Data",		"MikroMikko 1 M6",		GAME_NOT_WORKING )
COMP( 1981, mm1m7,		mm1m6,	0,		mm1m6,		mm1,		0, 		mm1m7,	  "Nokia Data",		"MikroMikko 1 M7",		GAME_NOT_WORKING )
