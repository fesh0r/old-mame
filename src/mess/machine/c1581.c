/**********************************************************************

    Commodore 1581/1563 Single Disk Drive emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    TODO:

	- D81 sector translation
	- WD1770 MO/RDY pin confusion
	- ready signal polarity
	- floppy access
    - fast serial
    - power LED
    - activity LED

*/

#include "driver.h"
#include "c1581.h"
#include "cpu/m6502/m6502.h"
#include "devices/flopdrv.h"
#include "formats/basicdsk.h"
#include "machine/6526cia.h"
#include "machine/cbmiec.h"
#include "machine/wd17xx.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 0

#define M6502_TAG		"u1"
#define M8520_TAG		"u5"
#define WD1770_TAG		"u4"

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _c1581_t c1581_t;
struct _c1581_t
{
	int address;

	int data_out;
	int atn_ack;

	/* devices */
	const device_config *cpu;
	const device_config *cia;
	const device_config *wd1770;
	const device_config *serial_bus;
	const device_config *image;
};

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE c1581_t *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert((device->type == C1581) || (device->type == C1563));
	return (c1581_t *)device->token;
}

INLINE c1581_config *get_safe_config(const device_config *device)
{
	assert(device != NULL);
	assert((device->type == C1581) || (device->type == C1563));
	return (c1581_config *)device->inline_config;
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    c1581_iec_atn_w - serial bus attention
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_atn_w )
{
	c1581_t *c1581 = get_safe_token(device);
	int data_out = !c1581->data_out && !(c1581->atn_ack && !state);

	mos6526_flag_w(c1581->cia, state);

	cbm_iec_data_w(c1581->serial_bus, device, data_out);
}

/*-------------------------------------------------
    c1581_iec_reset_w - serial bus reset
-------------------------------------------------*/

WRITE_LINE_DEVICE_HANDLER( c1581_iec_reset_w )
{
	if (!state)
	{
		device_reset(device);
	}
}

/*-------------------------------------------------
    ADDRESS_MAP( c1581_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1581_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE(M8520_TAG, cia_r, cia_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1581", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    ADDRESS_MAP( c1563_map )
-------------------------------------------------*/

static ADDRESS_MAP_START( c1563_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0x2000) AM_RAM
	AM_RANGE(0x4000, 0x400f) AM_MIRROR(0x1ff0) AM_DEVREADWRITE(M8520_TAG, cia_r, cia_w)
	AM_RANGE(0x6000, 0x6003) AM_MIRROR(0x1ffc) AM_DEVREADWRITE(WD1770_TAG, wd17xx_r, wd17xx_w)
	AM_RANGE(0x8000, 0xffff) AM_ROM AM_REGION("c1563", 0)
ADDRESS_MAP_END

/*-------------------------------------------------
    cia6526_interface c1581_cia_intf
-------------------------------------------------*/

static READ8_DEVICE_HANDLER( c1581_cia_pa_r )
{
	/*

        bit     description

        PA0     SIDE0
        PA1     /RDY
        PA2     /MOTOR
        PA3     DEV# SEL
        PA4     DEV# SEL
        PA5     POWER LED
        PA6     ACT LED
        PA7     /DISK CHNG

    */

	c1581_t *c1581 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* ready */
	data |= !(floppy_drive_get_flag_state(c1581->image, FLOPPY_DRIVE_READY) == FLOPPY_DRIVE_READY) << 1;

	/* serial address */
	data |= c1581->address << 3;

	return data;
}

static WRITE8_DEVICE_HANDLER( c1581_cia_pa_w )
{
	/*

        bit     description

        PA0     SIDE0
        PA1     /RDY
        PA2     /MOTOR
        PA3     DEV# SEL
        PA4     DEV# SEL
        PA5     POWER LED
        PA6     ACT LED
        PA7     /DISK CHNG

    */

	c1581_t *c1581 = get_safe_token(device->owner);
	int motor = BIT(data, 2);

	/* side 0 */
	wd17xx_set_side(c1581->wd1770, !BIT(data, 0));

	/* motor */
	floppy_mon_w(c1581->image, motor);
	floppy_drive_set_ready_state(c1581->image, !motor, 1);

	/* power led */

	/* active led */

}

static READ8_DEVICE_HANDLER( c1581_cia_pb_r )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6     /WPRT
        PB7     ATN IN

    */

	c1581_t *c1581 = get_safe_token(device->owner);
	UINT8 data = 0;

	/* data in */
	data = !cbm_iec_data_r(c1581->serial_bus);

	/* clock in */
	data |= !cbm_iec_clk_r(c1581->serial_bus) << 2;

	/* write protect */
	data |= !floppy_wpt_r(c1581->image) << 6;

	/* attention in */
	data |= !cbm_iec_atn_r(c1581->serial_bus) << 7;

	return data;
}

static WRITE8_DEVICE_HANDLER( c1581_cia_pb_w )
{
	/*

        bit     description

        PB0     DATA IN
        PB1     DATA OUT
        PB2     CLK IN
        PB3     CLK OUT
        PB4     ATN ACK
        PB5     FAST SER DIR
        PB6     /WPRT
        PB7     ATN IN

    */

	c1581_t *c1581 = get_safe_token(device->owner);

	int data_out = BIT(data, 1);
	int clk_out = BIT(data, 3);
	int atn_ack = BIT(data, 4);

	/* data out */
	int serial_data = !data_out && !(atn_ack && !cbm_iec_atn_r(c1581->serial_bus));
	cbm_iec_data_w(c1581->serial_bus, device->owner, serial_data);
	c1581->data_out = data_out;

	/* clock out */
	cbm_iec_clk_w(c1581->serial_bus, device->owner, !clk_out);

	/* attention acknowledge */
	c1581->atn_ack = BIT(data, 4);
}

static const cia6526_interface c1581_cia_intf =
{
	DEVCB_CPU_INPUT_LINE(M6502_TAG, INPUT_LINE_IRQ0),
	DEVCB_NULL,
	XTAL_16MHz/8,
	{
		{ DEVCB_HANDLER(c1581_cia_pa_r), DEVCB_HANDLER(c1581_cia_pa_w) },
		{ DEVCB_HANDLER(c1581_cia_pb_r), DEVCB_HANDLER(c1581_cia_pb_w) }
	}
};

/*-------------------------------------------------
    wd17xx_interface c1581_wd1770_intf
-------------------------------------------------*/

static const wd17xx_interface c1581_wd1770_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	{ FLOPPY_0, NULL, NULL, NULL }
};

/*-------------------------------------------------
    FLOPPY_OPTIONS( c1581 )
-------------------------------------------------*/

static FLOPPY_OPTIONS_START( c1581 )
	FLOPPY_OPTION( c1581, "d81", "Commodore 1581 Disk Image", basicdsk_identify_default, basicdsk_construct_default,
		HEADS([2])
		TRACKS([80])
		SECTORS([10])
		SECTOR_LENGTH([512])
		FIRST_SECTOR_ID([1]))
FLOPPY_OPTIONS_END

/*-------------------------------------------------
    floppy_config c1581_floppy_config
-------------------------------------------------*/

static const floppy_config c1581_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(c1581),
	DO_NOT_KEEP_GEOMETRY
};

/*-------------------------------------------------
    MACHINE_DRIVER( c1581 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1581 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(c1581_map)

	MDRV_CIA8520_ADD(M8520_TAG, XTAL_16MHz/8, c1581_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ c1581_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    MACHINE_DRIVER( c1563 )
-------------------------------------------------*/

static MACHINE_DRIVER_START( c1563 )
	MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_16MHz/8)
	MDRV_CPU_PROGRAM_MAP(c1563_map)

	MDRV_CIA8520_ADD(M8520_TAG, XTAL_16MHz/8, c1581_cia_intf)
	MDRV_WD1770_ADD(WD1770_TAG, /*XTAL_16MHz/2,*/ c1581_wd1770_intf)

	MDRV_FLOPPY_DRIVE_ADD(FLOPPY_0, c1581_floppy_config)
MACHINE_DRIVER_END

/*-------------------------------------------------
    ROM( c1581 )
-------------------------------------------------*/

ROM_START( c1581 )
	ROM_REGION( 0x8000, "c1581", ROMREGION_LOADBYNAME )
	ROM_LOAD( "beta.u2",	  0x0000, 0x8000, CRC(ecc223cd) SHA1(a331d0d46ead1f0275b4ca594f87c6694d9d9594) )
	ROM_LOAD( "318045-01.u2", 0x0000, 0x8000, CRC(113af078) SHA1(3fc088349ab83e8f5948b7670c866a3c954e6164) )
	ROM_LOAD( "318045-02.u2", 0x0000, 0x8000, CRC(a9011b84) SHA1(01228eae6f066bd9b7b2b6a7fa3f667e41dad393) )
ROM_END

/*-------------------------------------------------
    ROM( c1563 )
-------------------------------------------------*/

ROM_START( c1563 )
	ROM_REGION( 0x8000, "c1563", ROMREGION_LOADBYNAME )
	ROM_LOAD( "1563-rom.bin", 0x0000, 0x8000, CRC(1d184687) SHA1(2c5111a9c15be7b7955f6c8775fea25ec10c0ca0) )
ROM_END

/*-------------------------------------------------
    DEVICE_START( c1581 )
-------------------------------------------------*/

static DEVICE_START( c1581 )
{
	c1581_t *c1581 = get_safe_token(device);
	const c1581_config *config = get_safe_config(device);

	/* set serial address */
	assert((config->address > 7) && (config->address < 12));
	c1581->address = config->address - 8;

	/* find our CPU */
	c1581->cpu = device_find_child_by_tag(device, M6502_TAG);

	/* find devices */
	c1581->cia = device_find_child_by_tag(device, M8520_TAG);
	c1581->wd1770 = device_find_child_by_tag(device, WD1770_TAG);
	c1581->serial_bus = devtag_get_device(device->machine, config->serial_bus_tag);
	c1581->image = device_find_child_by_tag(device, FLOPPY_0);

	/* set floppy density */
	wd17xx_set_density(c1581->wd1770, DEN_MFM_LO);

	/* register for state saving */
}

/*-------------------------------------------------
    DEVICE_RESET( c1581 )
-------------------------------------------------*/

static DEVICE_RESET( c1581 )
{
	c1581_t *c1581 = get_safe_token(device);

	device_reset(c1581->cpu);
	device_reset(c1581->cia);
	device_reset(c1581->wd1770);
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1581 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1581 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(c1581_t);									break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = sizeof(c1581_config);								break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;							break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1581);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1581);			break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(c1581);						break;
		case DEVINFO_FCT_STOP:							/* Nothing */												break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(c1581);						break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Commodore 1581");							break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);									break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team"); 				break;
	}
}

/*-------------------------------------------------
    DEVICE_GET_INFO( c1563 )
-------------------------------------------------*/

DEVICE_GET_INFO( c1563 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_ROM_REGION:					info->romregion = ROM_NAME(c1563);							break;
		case DEVINFO_PTR_MACHINE_CONFIG:				info->machine_config = MACHINE_DRIVER_NAME(c1563);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Commodore 1563");							break;

		default:										DEVICE_GET_INFO_CALL(c1581);								break;
	}
}
