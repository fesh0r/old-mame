/*

	telmac.c


	MESS Driver by Curt Coder


	Telmac 1800
	-----------
	(c) 10/1977 Telercas Oy, Finland

	CPU:		CDP1802		1.76 MHz
	RAM:		2 KB (4 KB)
	ROM:		512 B

	Video:		OSM-200 (1 KB videoram, 10.2 MHz xtal)

	Designer:	Osmo Kainulainen
	Keyboard:	OS-70 A (QWERTY, hexadecimal keypad)


	Oscom 1000B
	-----------
	(c) 197? Oscom Oy, Finland

	CPU:		CDP1802A	? MHz

	Enhanced Telmac 1800 with built-in CRT board (OSM-200).


	Telmac 2000
	-----------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		16 KB
	ROM:		512 B / 2 KB (TOOL-2000)

	Video:		CDP1864		1.75 MHz
	Color RAM:	512 B

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII, KB-16, KB-64


	Telmac 2000E
	------------
	(c) 1980 Telercas Oy, Finland

	CPU:		CDP1802A	1.75 MHz
	RAM:		8 KB
	ROM:		8 KB

	Video:		CDP1864		1.75 MHz
	Color RAM:	1 KB

	Colors:		8 fg, 4 bg
	Resolution:	64x192
	Sound:		frequency control, volume on/off
	Keyboard:	ASCII (RCA VP-601/VP-611), KB-16/KB-64

	SBASIC:		24.0


	Oscom Nano
	----------
	(c) 1981 Oscom Oy, Finland

	CPU:		CDP1802A	? MHz
	ROM:		512 B / 1 KB

	Small form factor version of Telmac 1800. Combined text and graphics output.


	Telmac TMC-121/111/112
	----------------------
	(c) 198? Telercas Oy, Finland

	CPU:		CDP1802A	? MHz

	Built from Telmac 2000 series cards. Huge metal box.

*/

#include "driver.h"
#include "inputx.h"
#include "cpu/cdp1802/cdp1802.h"
#include "video/generic.h"
#include "video/cdp1864.h"
#include "devices/cassette.h"
#include "sound/beep.h"
#include "rescap.h"

extern VIDEO_START( osm200 );
extern VIDEO_UPDATE( osm200 );

/* Read/Write Handlers */

static int keylatch;

static WRITE8_HANDLER( keyboard_latch_w )
{
	keylatch = data;
}

static WRITE8_HANDLER( tmc2000_bankswitch_w )
{
	memory_set_bank(1, data & 0x01);

	cdp1864_tone_latch_w(0, data);
}

/* Memory Maps */

// Telmac 1800

static ADDRESS_MAP_START( tmc1800_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x8000, 0x81ff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc1800_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
ADDRESS_MAP_END

// Telmac 2000

static ADDRESS_MAP_START( tmc2000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_RAM
	AM_RANGE(0x8000, 0x87ff) AM_RAMBANK(1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tmc2000_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READWRITE(cdp1864_dispon_r, cdp1864_step_bgcolor_w)
	AM_RANGE(0x02, 0x02) AM_WRITE(keyboard_latch_w)
	AM_RANGE(0x04, 0x04) AM_READWRITE(cdp1864_dispoff_r, tmc2000_bankswitch_w)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( tmc1800 )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_CHAR('1')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_CHAR('2')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_CHAR('3')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_CHAR('4')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_CHAR('5')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_CHAR('6')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_CHAR('7')

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_CHAR('8')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')

	PORT_START_TAG("RUN")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Run/Reset") PORT_CODE(KEYCODE_R) PORT_TOGGLE
INPUT_PORTS_END

/* CDP1802 Interfaces */

// Telmac 1800

static UINT8 tmc1800_mode_r(void)
{
	if (readinputportbytag("RUN") & 0x01)
	{
		return CDP1802_MODE_RUN;
	}
	else
	{
		return CDP1802_MODE_RESET;
	}
}

static UINT8 tmc1800_ef_r(void)
{
	UINT8 flags = 0x0f;

	/*
		EF1		?
		EF2		keyboard
		EF3		?
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags -= EF2;

	return flags;
}

static void tmc1800_q_w(int level)
{
}

static const CDP1802_CONFIG tmc1800_config =
{
	tmc1800_mode_r,
	tmc1800_ef_r,
	NULL,
	tmc1800_q_w,
	NULL,
	NULL
};

// Telmac 2000

static UINT8 tmc2000_ef_r(void)
{
	int flags = 0x0f;

	/*
		EF1		?
		EF2		keyboard
		EF3		?
		EF4		?
	*/

	// keyboard
	if (~readinputport(keylatch / 8) & (1 << (keylatch % 8))) flags -= EF2;

	return flags;
}

static void tmc2000_q_w(int level)
{
	// turn CDP1864 sound generator on/off
	cdp1864_audio_output_enable(level);

	// set Q led status
	set_led_status(1, level);
}

static const CDP1802_CONFIG tmc2000_config =
{
	tmc1800_mode_r,
	tmc2000_ef_r,
	NULL,
	tmc2000_q_w,
	NULL,
	cdp1864_dma_w
};

/* Machine Initialization */

static MACHINE_START( tmc1800 )
{
	state_save_register_global(keylatch);
}

static MACHINE_RESET( tmc1800 )
{
	cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
}

static MACHINE_START( tmc2000 )
{
	state_save_register_global(keylatch);

	memory_configure_bank(1, 0, 1, memory_region(REGION_CPU1) + 0x8000, 0);
	memory_configure_bank(1, 1, 1, &colorram, 0);
}

static MACHINE_RESET( tmc2000 )
{
	machine_reset_cdp1864(machine);

	cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);

	// set program counter to 0x8000
	memory_set_bank(1, 0);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( tmc1800 )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, 1750000)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc1800_map, 0)
	MDRV_CPU_IO_MAP(tmc1800_io_map, 0)
	MDRV_CPU_CONFIG(tmc1800_config)

	MDRV_MACHINE_START(tmc1800)
	MDRV_MACHINE_RESET(tmc1800)

	// video hardware

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 0, 199)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(osm200)
	MDRV_VIDEO_UPDATE(osm200)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tmc2000 )

	// basic system hardware

	MDRV_CPU_ADD(CDP1802, CDP1864_CLK_FREQ)	// 1.75 MHz
	MDRV_CPU_PROGRAM_MAP(tmc2000_map, 0)
	MDRV_CPU_IO_MAP(tmc2000_io_map, 0)
	MDRV_CPU_CONFIG(tmc2000_config)

	MDRV_MACHINE_START(tmc2000)
	MDRV_MACHINE_RESET(tmc2000)

	// video hardware

	MDRV_SCREEN_ADD("main", 0)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(CDP1864_CLK_FREQ, CDP1864_SCREEN_WIDTH, CDP1864_HBLANK_END, CDP1864_HBLANK_START, CDP1864_TOTAL_SCANLINES, CDP1864_SCANLINE_VBLANK_END, CDP1864_SCANLINE_VBLANK_START)

	MDRV_PALETTE_LENGTH(8)
	MDRV_VIDEO_START(cdp1864)
	MDRV_VIDEO_UPDATE(cdp1864)

	// sound hardware

	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(BEEP, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( tmc1800 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "82s141.ic2", 0x8000, 0x0200, NO_DUMP ) // MMI 6341-1

	ROM_REGION( 0x400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "82s147.5d",	0x0000, 0x0200, NO_DUMP ) // MMI 6349
	ROM_LOAD( "82s147.5c",	0x0200, 0x0200, NO_DUMP ) // MMI 6349
ROM_END

ROM_START( tmc2000 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_SYSTEM_BIOS( 0, "monitor",  "Monitor" )
	ROMX_LOAD( "200.m5",    0x8000, 0x0200, BAD_DUMP CRC(53BDDF1A) SHA1(3691741A6921A2E2333B697E80D4936BE41DCDA8), ROM_BIOS(1) ) // typed in from the manual
	ROM_SYSTEM_BIOS( 1, "tool2000", "TOOL-2000" )
	ROMX_LOAD( "tool2000",	0x8000, 0x0800, NO_DUMP, ROM_BIOS(2) )
ROM_END

/* System Configuration */

static void tmc1800_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc1800 )
	CONFIG_RAM_DEFAULT	( 2 * 1024)
	CONFIG_RAM			( 4 * 1024)
	CONFIG_DEVICE(tmc1800_cassette_getinfo)
SYSTEM_CONFIG_END

static void tmc2000_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( tmc2000 )
	CONFIG_RAM_DEFAULT	(16 * 1024)
	CONFIG_RAM			(32 * 1024)
	CONFIG_DEVICE(tmc2000_cassette_getinfo)
SYSTEM_CONFIG_END

/* Driver Initialization */

static int tmc2000_colorram_r(UINT16 addr)
{
	UINT8 data = colorram[addr]; // 0x04 = ~B, 0x02 = ~R, 0x01 = ~G

	return ~(((data & 0x04) >> 1) + ((data & 0x02) << 1) + (data & 0x01)) & 0x07;
}

static const CDP1864_interface tmc2000_CDP1864_interface =
{
	RES_K(2.2),	// unverified
	RES_K(1),	// unverified
	RES_K(5.1),	// unverified
	RES_K(4.7),	// unverified
	tmc2000_colorram_r
};

static TIMER_CALLBACK(setup_beep)
{
	beep_set_state(0, 0);
	beep_set_frequency( 0, 0 );
}

static DRIVER_INIT( tmc1800 )
{
	timer_set(attotime_zero, NULL, 0, setup_beep);
}

static DRIVER_INIT( tmc2000 )
{
	timer_set(attotime_zero, NULL, 0, setup_beep);
	cdp1864_configure(&tmc2000_CDP1864_interface);
}

/* System Drivers */

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY         FULLNAME        FLAGS */
COMP( 1977, tmc1800,    0,      0,      tmc1800,    tmc1800,    tmc1800,    tmc1800,    "Telercas Oy",  "Telmac 1800",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
COMP( 1980, tmc2000,    0,      0,      tmc2000,    tmc1800,    tmc2000,    tmc2000,    "Telercas Oy",  "Telmac 2000",  GAME_NOT_WORKING | GAME_SUPPORTS_SAVE )
