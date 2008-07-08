/*

	Telmac 2000E
    ------------
    (c) 1980 Telercas Oy, Finland

    CPU:        CDP1802A    1.75 MHz
    RAM:        8 KB
    ROM:        8 KB

    Video:      CDP1864     1.75 MHz
    Color RAM:  1 KB

    Colors:     8 fg, 4 bg
    Resolution: 64x192
    Sound:      frequency control, volume on/off
    Keyboard:   ASCII (RCA VP-601/VP-611), KB-16/KB-64

    SBASIC:     24.0


    Telmac TMC-121/111/112
    ----------------------
    (c) 198? Telercas Oy, Finland

    CPU:        CDP1802A    ? MHz

    Built from Telmac 2000 series cards. Huge metal box.

*/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/printer.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "video/cdp1864.h"
#include "sound/beep.h"
#include "rescap.h"

#define SCREEN_TAG "main"
#define CDP1864_TAG "cdp1864"

static const device_config *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* Read/Write Handlers */

static READ8_HANDLER( vismac_r )
{
	return 0;
}

static WRITE8_HANDLER( vismac_w )
{
}

static READ8_HANDLER( floppy_r )
{
	return 0;
}

static WRITE8_HANDLER( floppy_w )
{
}

static READ8_HANDLER( ascii_keyboard_r )
{
	return 0;
}

static READ8_HANDLER( io_r )
{
	return 0;
}

static WRITE8_HANDLER( io_w )
{
}

static WRITE8_HANDLER( io_select_w )
{
}

static int keylatch;

static WRITE8_HANDLER( keyboard_latch_w )
{
	keylatch = data;
}

/* Memory Maps */

static ADDRESS_MAP_START( tmc2000e_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0xc000, 0xdfff) AM_ROM
	AM_RANGE(0xfc00, 0xffff) AM_WRITEONLY AM_BASE(&colorram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000e_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVWRITE(CDP1864, CDP1864_TAG, cdp1864_tone_latch_w)
	AM_RANGE(0x02, 0x02) AM_DEVWRITE(CDP1864, CDP1864_TAG, cdp1864_step_bgcolor_w)
	AM_RANGE(0x03, 0x03) AM_READWRITE(ascii_keyboard_r, keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x05, 0x05) AM_READWRITE(vismac_r, vismac_w)
	AM_RANGE(0x06, 0x06) AM_READWRITE(floppy_r, floppy_w)
	AM_RANGE(0x07, 0x07) AM_READ_PORT("DSW0") AM_WRITE(io_select_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc2000e )
	PORT_START_TAG("DSW0")	// System Configuration DIPs
	PORT_DIPNAME( 0x80, 0x00, "Keyboard Type" )
	PORT_DIPSETTING(    0x00, "ASCII" )
	PORT_DIPSETTING(    0x80, "Matrix" )
	PORT_DIPNAME( 0x40, 0x00, "Operating System" )
	PORT_DIPSETTING(    0x00, "TOOL-2000-E" )
	PORT_DIPSETTING(    0x40, "Load from disk" )
	PORT_DIPNAME( 0x30, 0x00, "Display Interface" )
	PORT_DIPSETTING(    0x00, "PAL" )
	PORT_DIPSETTING(    0x10, "CDG-80" )
	PORT_DIPSETTING(    0x20, "VISMAC" )
	PORT_DIPSETTING(    0x30, "UART" )
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

/* Video */

static int cdp1864_efx;

static CDP1864_ON_INT_CHANGED( tmc2000e_int_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, level);
}

static CDP1864_ON_DMAO_CHANGED( tmc2000e_dmao_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1864_ON_EFX_CHANGED( tmc2000e_efx_w )
{
	cdp1864_efx = level;
}

static CDP1864_INTERFACE( tmc2000e_cdp1864_intf )
{
	SCREEN_TAG,
	CDP1864_CLK_FREQ,
	CDP1864_INTERLACED,
	tmc2000e_int_w,
	tmc2000e_dmao_w,
	tmc2000e_efx_w,
	RES_K(2.2),	// unverified
	RES_K(1),	// unverified
	RES_K(5.1),	// unverified
	RES_K(4.7)	// unverified
};

static VIDEO_UPDATE( tmc2000e )
{
	const device_config *cdp1864 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1864, CDP1864_TAG);

	cdp1864_update(cdp1864, bitmap, cliprect);

	return 0;
}

/* CDP1802 Interface */

static CDP1802_MODE_READ( tmc2000e_mode_r )
{
	if (input_port_read(machine, "RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( tmc2000e_ef_r )
{
	UINT8 flags = 0x0f;
	char port[4];

	/*
        EF1     CDP1864
        EF2     tape in/floppy
        EF3     keyboard
        EF4     I/O port
    */

	// CDP1864

	if (!cdp1864_efx) flags -= EF1;

	// tape in

	if (cassette_input(cassette_device_image()) > +1.0) flags -= EF2;
	
	// keyboard

	sprintf(port, "IN%d", keylatch / 8);
	if (~input_port_read(machine, port) & (1 << (keylatch % 8))) flags -= EF3;

	return flags;
}

static CDP1802_Q_WRITE( tmc2000e_q_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	// turn CDP1864 sound generator on/off

	cdp1864_aoe_w(cdp1864, level);

	// set Q led status

	set_led_status(1, level);

	// tape out

	cassette_output(cassette_device_image(), level ? -1.0 : +1.0);

	// floppy control (FDC-6)
}

static CDP1802_DMA_WRITE( tmc2000e_dma_w )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);

	UINT8 color = (colorram[ma & 0x3ff]) & 0x07; // 0x04 = R, 0x02 = B, 0x01 = G
	
	int rdata = BIT(color, 2);
	int gdata = BIT(color, 0);
	int bdata = BIT(color, 1);

	cdp1864_dma_w(cdp1864, data, rdata, gdata, bdata);
}

static CDP1802_INTERFACE( tmc2000e_config )
{
	tmc2000e_mode_r,
	tmc2000e_ef_r,
	NULL,
	tmc2000e_q_w,
	NULL,
	tmc2000e_dma_w
};

/* Machine Initialization */

static MACHINE_START( tmc2000e )
{
	state_save_register_global(keylatch);
}

static MACHINE_RESET( tmc2000e )
{
	const device_config *cdp1864 = device_list_find_by_tag(machine->config->devicelist, CDP1864, CDP1864_TAG);
	cdp1864->reset(cdp1864);

	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);

	// reset program counter to 0xc000
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc2000e )
	// basic system hardware

	MDRV_CPU_ADD(CDP1802, XTAL_1_75MHz)
	MDRV_CPU_PROGRAM_MAP(tmc2000e_map, 0)
	MDRV_CPU_IO_MAP(tmc2000e_io_map, 0)
	MDRV_CPU_CONFIG(tmc2000e_config)

	MDRV_MACHINE_START(tmc2000e)
	MDRV_MACHINE_RESET(tmc2000e)

	// video hardware

	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16) 
	MDRV_SCREEN_RAW_PARAMS(CDP1864_CLK_FREQ, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_UPDATE(tmc2000e)

	MDRV_DEVICE_ADD(CDP1864_TAG, CDP1864)
	MDRV_DEVICE_CONFIG(tmc2000e_cdp1864_intf)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc2000e )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1", 0xc000, 0x0800, NO_DUMP )
	ROM_LOAD( "2", 0xc800, 0x0800, NO_DUMP )
	ROM_LOAD( "3", 0xd000, 0x0800, NO_DUMP )
	ROM_LOAD( "4", 0xd800, 0x0800, NO_DUMP )
ROM_END

/* System Configuration */

static void tmc2000e_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:					info->i = 1; break;
		case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:	info->i = CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_MUTED; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void tmc2000e_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(basicdsk_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( tmc2000e )
	CONFIG_RAM_DEFAULT	( 8 * 1024)
	CONFIG_RAM			(40 * 1024)
	CONFIG_DEVICE(tmc2000e_cassette_getinfo)
	CONFIG_DEVICE(tmc2000e_floppy_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static TIMER_CALLBACK(setup_beep)
{
	beep_set_state(0, 0);
	beep_set_frequency( 0, 0 );
}

static DRIVER_INIT( tmc2000e )
{
	// enable power led
	set_led_status(2, 1);

	timer_set(attotime_zero, NULL, 0, setup_beep);
}

//    YEAR  NAME      PARENT   COMPAT   MACHINE   INPUT     INIT        CONFIG    COMPANY        FULLNAME
COMP( 1980, tmc2000e, 0,       0,	    tmc2000e, tmc2000e, tmc2000e,	tmc2000e, "Telercas Oy", "Telmac 2000E", GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
