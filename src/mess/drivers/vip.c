/*

    TODO:

    - pcb layout guru-style readme
    - tape interface
    - discrete sound from NE555
    - artwork
    - VP-550/551 Super Sound Board
    - VP-590 Color Board
    - VP-595 Simple Sound Board
    - VP-601/611 ASCII Keyboard
    - VP-700 Expanded Tiny Basic Board

*/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"
#include "sound/beep.h"
#include "video/cdp1861.h"

static QUICKLOAD_LOAD(vip);

#define XTAL XTAL_3_52128MHz

#define SCREEN_TAG "main"
#define CDP1861_TAG "cdp1861"

/* Read/Write Handlers */

static UINT8 keylatch;

static WRITE8_HANDLER( keylatch_w )
{
	keylatch = data & 0x0f;
}

static WRITE8_HANDLER( bankswitch_w )
{
	memory_set_bank(1, 0);
}

/* Memory Maps */

static ADDRESS_MAP_START( vip_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( vip_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_DEVREADWRITE(CDP1861, CDP1861_TAG, cdp1861_dispon_r, cdp1861_dispoff_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keylatch_w)
//  AM_RANGE(0x03, 0x03) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( vip )
	PORT_START("KEYPAD")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 MW") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A MR") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B TR") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F TW") PORT_CODE(KEYCODE_F)

	PORT_START("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

/* Video */

static int cdp1861_efx;

static CDP1861_ON_INT_CHANGED( vip_int_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_INT, level);
}

static CDP1861_ON_DMAO_CHANGED( vip_dmao_w )
{
	cpunum_set_input_line(device->machine, 0, CDP1802_INPUT_LINE_DMAOUT, level);
}

static CDP1861_ON_EFX_CHANGED( vip_efx_w )
{
	cdp1861_efx = level;
}

static CDP1861_INTERFACE( vip_cdp1861_intf )
{
	SCREEN_TAG,
	XTAL_3_52128MHz,
	vip_int_w,
	vip_dmao_w,
	vip_efx_w
};

static VIDEO_UPDATE( vip )
{
	const device_config *cdp1861 = device_list_find_by_tag(screen->machine->config->devicelist, CDP1861, CDP1861_TAG);

	cdp1861_update(cdp1861, bitmap, cliprect);

	return 0;
}

/* CDP1802 Configuration */

static int vip_run;
static int vip_reset;

static CDP1802_MODE_READ( vip_mode_r )
{
	if (input_port_read(machine, "RUN") & 0x01)
	{
		if (!vip_run)
		{
			memory_set_bank(1, 1);
			vip_reset = 0;
			vip_run = 1;
		}

		return CDP1802_MODE_RUN;
	}
	else
	{
		if (!vip_reset)
		{
			const device_config *cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);
			cdp1861->reset(cdp1861);
			vip_reset = 1;
			vip_run = 0;
		}

		return CDP1802_MODE_RESET;
	}
}

static CDP1802_EF_READ( vip_ef_r )
{
	int ef = 0x0f;

	if (cdp1861_efx) ef -= EF1;
	// EF2 = tape (high when tone read)
	if (input_port_read(machine, "KEYPAD") & (1 << keylatch)) ef -= EF3;

	return ef;
}

static CDP1802_Q_WRITE( vip_q_w )
{
	beep_set_state(0, level);
	// PWR Q TAPE
	set_led_status(1, level);
}

static CDP1802_DMA_WRITE( vip_dma_w )
{
	const device_config *cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);

	cdp1861_dma_w(cdp1861, data);
}

static CDP1802_INTERFACE( vip_config )
{
	vip_mode_r,
	vip_ef_r,
	NULL,
	vip_q_w,
	NULL,
	vip_dma_w
};

/* Machine Initialization */

static MACHINE_START( vip )
{
	UINT8 *ram = memory_region(machine, "main");
	UINT16 addr;

	state_save_register_global(keylatch);
	state_save_register_global(vip_reset);
	state_save_register_global(vip_run);

	memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK1);
	memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, 0x0000, mess_ram_size - 1, 0, 0, SMH_BANK1);

	if (mess_ram_size < 0x8000)
	{
		memory_install_read8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x7fff, 0, 0, SMH_UNMAP);
		memory_install_write8_handler(machine, 0, ADDRESS_SPACE_PROGRAM, mess_ram_size, 0x7fff, 0, 0, SMH_UNMAP);
	}

	for (addr = 0; addr < mess_ram_size; addr++)
	{
		ram[addr] = mame_rand(machine) & 0xff;
	}

	memory_configure_bank(1, 0, 2, memory_region(machine, "main"), 0x8000);
}

static MACHINE_RESET( vip )
{
	const device_config *cdp1861 = device_list_find_by_tag(machine->config->devicelist, CDP1861, CDP1861_TAG);
	cdp1861->reset(cdp1861);
	memory_set_bank(1, 1);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( vip )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", CDP1802, XTAL/2)
	MDRV_CPU_PROGRAM_MAP(vip_map, 0)
	MDRV_CPU_IO_MAP(vip_io_map, 0)
	MDRV_CPU_CONFIG(vip_config)

	MDRV_MACHINE_START(vip)
	MDRV_MACHINE_RESET(vip)

    /* video hardware */
	MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL/2, CDP1861_SCREEN_WIDTH, CDP1861_HBLANK_END, CDP1861_HBLANK_START, CDP1861_TOTAL_SCANLINES, CDP1861_SCANLINE_VBLANK_END, CDP1861_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_UPDATE(vip)

	MDRV_DEVICE_ADD(CDP1861_TAG, CDP1861)
	MDRV_DEVICE_CONFIG(vip_cdp1861_intf)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("beep", BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_QUICKLOAD_ADD(vip, "cos", 0)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( vip )
	ROM_REGION( 0x10000, "main", 0 )
	ROM_LOAD( "cdpr566.u10", 0x8000, 0x0200, CRC(5be0a51f) SHA1(40266e6d13e3340607f8b3dcc4e91d7584287c06) )
ROM_END

/* System Configuration */

static QUICKLOAD_LOAD( vip )
{
	int size = image_length(image);

	if (size < 0x8000)
	{
		if (image_fread(image, memory_region(image->machine, "main"), size) != size)
		{
			return INIT_FAIL;
		}
	}

	return INIT_PASS;
}

static void vip_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START( vip )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(vip_cassette_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static TIMER_CALLBACK(setup_beep)
{
	beep_set_state( 0, 0 );
	beep_set_frequency( 0, 1400 );
}

static DRIVER_INIT( vip )
{
	// turn on power led
	set_led_status(0, 1);

	timer_set(attotime_zero, NULL, 0, setup_beep);
}

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY FULLNAME
COMP(1977,	vip,		0,		0,		vip,		vip,		vip,		vip,		"RCA",	"Cosmac VIP (VP-711)", GAME_IMPERFECT_SOUND | GAME_SUPPORTS_SAVE )
