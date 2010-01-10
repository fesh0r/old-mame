/**********************************************************************

    Commodore 1551 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

	TODO:

	- move M6510T port handling to cpu core
	- TCBM bus
	- connect to C16/Plus4

*/

#include "driver.h"
#include "c1551.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/g64_dsk.h"
#include "machine/6525tpi.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6510T_TAG		"u2"
#define M6525_TAG		"u3"
#define M6523_TAG		"ci_u2"

#define FLOPPY_TAG		"c1551_floppy"

static const double C1551_BITRATE[4] =
{
	XTAL_16MHz/13.0,	/* tracks 1-17 */
	XTAL_16MHz/14.0,	/* tracks 18-24 */
	XTAL_16MHz/15.0, 	/* tracks 25-30 */
	XTAL_16MHz/16.0		/* tracks 31-42 */
};

#define SYNC_MARK			0x3ff		/* 10 consecutive 1-bits */

#define TRACK_BUFFER_SIZE	8194		/* 2 bytes track length + maximum of 8192 bytes of GCR encoded data */
#define TRACK_DATA_START	2

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1551_t c1551_t;
struct _c1551_t
{
	UINT8 track_buffer[TRACK_BUFFER_SIZE];				/* track data buffer */
	int track_len;						/* track length */
	int buffer_pos;						/* current byte position within track buffer */
	int bit_pos;						/* current bit position within track buffer byte */
	int bit_count;						/* current data byte bit counter */
	UINT16 data;						/* data shift register */

	/* signals */
	UINT8 yb;							/* GCR data byte to write */
	int byte;							/* byte ready */
	int atna;							/* attention acknowledge */
	int ds;								/* density select */
	int stp;							/* stepping motor */
	int soe;							/* s? output enable */
	int mode;							/* mode (0 = write, 1 = read) */

	/* devices */
	const device_config *cpu;
	const device_config *tpi0;
	const device_config *tpi1;
	const device_config *image;

	/* timers */
	emu_timer *bit_timer;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1551_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == C1551);
	return (c1551_t *)device->token;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( bit_tick )
-------------------------------------------------*/

static TIMER_CALLBACK( bit_tick )
{
	const device_config *device = (device_config *)ptr;
	c1551_t *c1551 = get_safe_token(device);
	int byte = 0;

	/* shift in data from the read head */
	c1551->data <<= 1;
	c1551->data |= BIT(c1551->track_buffer[c1551->buffer_pos], c1551->bit_pos);
	c1551->bit_pos--;
	c1551->bit_count++;

	if (c1551->bit_pos < 0)
	{
		c1551->bit_pos = 7;
		c1551->buffer_pos++;

		if (c1551->buffer_pos > c1551->track_len + 1)
		{
			/* loop to the start of the track */
			c1551->buffer_pos = TRACK_DATA_START;
		}
	}

	if ((c1551->data & SYNC_MARK) == SYNC_MARK)
	{
		/* SYNC detected */
		c1551->bit_count = 0;
	}

	if (c1551->bit_count > 7)
	{
		/* byte ready */
		c1551->bit_count = 0;
		byte = 1;

		if (!(c1551->data & 0xff))
		{
			/* simulate weak bits with randomness */
			c1551->data = (c1551->data & 0xff00) | (mame_rand(machine) & 0xff);
		}
	}

	c1551->byte = byte;
}

/*-------------------------------------------------
    c1551_port_r - M6510T port read
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c1551_port_r )
{
	/*

        bit     description

        P0      STEP0
        P1      STEP1
        P2      MOTOR ON
        P3      ACT
        P4      WPRT
        P5      DS0
        P6      DS1
        P7      BYTE READY

    */

	c1551_t *c1551 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* write protect sense */
	data |= !floppy_wpt_r(c1551->image) << 4;
	
	/* byte ready */
	data |= !(c1551->soe && c1551->byte) << 7;

	return data;
}

/*-------------------------------------------------
    c1551_port_w - M6510T port write
-------------------------------------------------*/

static WRITE8_DEVICE_HANDLER( c1551_port_w )
{
	/*

        bit     description

        P0      STEP0
        P1      STEP1
        P2      MOTOR ON
        P3      ACT
        P4      WPRT
        P5      DS0
        P6      DS1
        P7      BYTE READY

    */

	c1551_t *c1551 = get_safe_token(device->owner);

	int stp = data & 0x03;
	int ds = (data >> 5) & 0x03;
	int mtr = BIT(data, 2);

	/* stepper motor */
	if (c1551->stp != stp)
	{
		int tracks = 0;

		switch (c1551->stp)
		{
		case 0:	if (stp == 1) tracks++; else if (stp == 3) tracks--; break;
		case 1:	if (stp == 2) tracks++; else if (stp == 0) tracks--; break;
		case 2: if (stp == 3) tracks++; else if (stp == 1) tracks--; break;
		case 3: if (stp == 0) tracks++; else if (stp == 2) tracks--; break;
		}

		if (tracks != 0)
		{
			c1551->track_len = TRACK_BUFFER_SIZE;
			c1551->buffer_pos = TRACK_DATA_START;
			c1551->bit_pos = 7;
			c1551->bit_count = 0;

			/* step read/write head */
			floppy_drive_seek(c1551->image, tracks);

			/* read track data */
			floppy_drive_read_track_data_info_buffer(c1551->image, 0, c1551->track_buffer, &c1551->track_len);

			/* extract track length */
			c1551->track_len = (c1551->track_buffer[1] << 8) | c1551->track_buffer[0];
		}

		c1551->stp = stp;
	}

	/* spindle motor */
	floppy_mon_w(c1551->image, !mtr);
	timer_enable(c1551->bit_timer, mtr);

	/* activity LED */

	/* density select */
	if (c1551->ds != ds)
	{
		timer_adjust_periodic(c1551->bit_timer, attotime_zero, 0, ATTOTIME_IN_HZ(C1551_BITRATE[ds]/4));
		c1551->ds = ds;
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1551_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1551_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0001) AM_DEVREADWRITE(M6510T_TAG, c1551_port_r, c1551_port_w)
	AM_RANGE(0x0002, 0x07ff) AM_RAM
	AM_RANGE(0x4000, 0x4007) AM_DEVREADWRITE(M6525_TAG, tpi6525_r, tpi6525_w)
	AM_RANGE(0xc000, 0xffff) AM_ROM AM_REGION("c1551", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    tpi6525_interface c1551_tpi_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c1551_tpi_pa_r )
{
	/*

        bit     description

        PA0     6523 P0
        PA1     6523 P1
        PA2     6523 P2
        PA3     6523 P3
        PA4     6523 P4
        PA5     6523 P5
        PA6     6523 P6
        PA7     6523 P7

    */

	return 0;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pa_w )
{
	/*

        bit     description

        PA0     6523 P0
        PA1     6523 P1
        PA2     6523 P2
        PA3     6523 P3
        PA4     6523 P4
        PA5     6523 P5
        PA6     6523 P6
        PA7     6523 P7

    */
}

static READ8_DEVICE_HANDLER( c1551_tpi_pb_r )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */

	c1551_t *c1551 = get_safe_token(device->owner);

	return c1551->data & 0xff;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pb_w )
{
	/*

        bit     description

        PB0     YB0
        PB1     YB1
        PB2     YB2
        PB3     YB3
        PB4     YB4
        PB5     YB5
        PB6     YB6
        PB7     YB7

    */

	c1551_t *c1551 = get_safe_token(device->owner);

	c1551->yb = data;
}

static READ8_DEVICE_HANDLER( c1551_tpi_pc_r )
{
	/*

        bit     description

        PC0     6523 _IRQ
        PC1     6523 _RES
        PC2     interface J1
        PC3     6523 pin 20
        PC4     SOE
        PC5     JP1
        PC6     _SYNC
        PC7     6523 phi2

    */

	c1551_t *c1551 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* SYNC detect line */
	data |= !(c1551->mode && ((c1551->data & SYNC_MARK) == SYNC_MARK)) << 6;

	return data;
}

static WRITE8_DEVICE_HANDLER( c1551_tpi_pc_w )
{
	/*

        bit     description

        PC0     6523 _IRQ
        PC1     6523 _RES
        PC2     interface J1
        PC3     6523 pin 20
        PC4     SOE
        PC5     JP1
        PC6     _SYNC
        PC7     6523 phi2

    */

	c1551_t *c1551 = get_safe_token(device->owner);

	/* SOE */
	c1551->soe = BIT(data, 4);
}

static const tpi6525_interface c1551_tpi_intf =
{
	c1551_tpi_pa_r,
	c1551_tpi_pb_r,
	c1551_tpi_pc_r,
	c1551_tpi_pa_w,
	c1551_tpi_pb_w,
	c1551_tpi_pc_w,
	NULL,
	NULL,
	NULL
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1551 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1551 )
//  FLOPPY_OPTION( c1551, "d64", "Commodore 1551 Disk Image", d64_dsk_identify, d64_dsk_construct, NULL )
	FLOPPY_OPTION( c1551, "g64", "Commodore 1551 GCR Disk Image", g64_dsk_identify, g64_dsk_construct, NULL )
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1551_floppy_config
-------------------------------------------------*/

static const floppy_config c1551_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_SS_80,
	FLOPPY_OPTIONS_NAME(c1551),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1551 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1551 )
	MDRV_CPU_ADD(M6510T_TAG, M6510T, 2000000)
	MDRV_CPU_PROGRAM_MAP(c1551_map)

	MDRV_TPI6525_ADD(M6525_TAG, c1551_tpi_intf)
//  MDRV_MOS6523_ADD(M6523_TAG, 2000000, c1551_mos6523_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_TAG, c1551_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c1551 )
-------------------------------------------------*/

ROM_START( c1551 )
	ROM_REGION( 0x4000, "c1551", ROMREGION_LOADBYNAME )
	ROM_LOAD( "318001-01.u4", 0x0000, 0x4000, CRC(6d16d024) SHA1(fae3c788ad9a6cc2dbdfbcf6c0264b2ca921d55e) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1551 )
-------------------------------------------------*/

static DEVICE_START( c1551 )
{
	c1551_t *c1551 = get_safe_token(device);

	/* find our CPU */
	c1551->cpu = device_find_child_by_tag(device, M6510T_TAG);

	/* find devices */
	c1551->tpi0 = device_find_child_by_tag(device, M6525_TAG);
	c1551->tpi1 = device_find_child_by_tag(device, M6523_TAG);
	c1551->image = device_find_child_by_tag(device, FLOPPY_0);

	/* allocate track buffer */
//  c1551->track_buffer = auto_alloc_array(device->machine, UINT8, TRACK_BUFFER_SIZE);

	/* allocate data timer */
	c1551->bit_timer = timer_alloc(device->machine, bit_tick, (void *)device);

	/* register for state saving */
//  state_save_register_device_item_pointer(device, 0, c1551->track_buffer, TRACK_BUFFER_SIZE);
	state_save_register_device_item(device, 0, c1551->track_len);
	state_save_register_device_item(device, 0, c1551->buffer_pos);
	state_save_register_device_item(device, 0, c1551->bit_pos);
	state_save_register_device_item(device, 0, c1551->bit_count);
	state_save_register_device_item(device, 0, c1551->data);
	state_save_register_device_item(device, 0, c1551->yb);
	state_save_register_device_item(device, 0, c1551->byte);
	state_save_register_device_item(device, 0, c1551->atna);
	state_save_register_device_item(device, 0, c1551->ds);
	state_save_register_device_item(device, 0, c1551->stp);
	state_save_register_device_item(device, 0, c1551->soe);
	state_save_register_device_item(device, 0, c1551->mode);
}

/*-------------------------------------------------
    DEVICE_RESET( c1551 )
-------------------------------------------------*/

static DEVICE_RESET( c1551 )
{
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1551 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1551 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1551_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;												break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1551);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1551);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1551);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1551);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1551");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore VIC-1540");						break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}
