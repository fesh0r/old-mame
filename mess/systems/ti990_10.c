/*
	TI990/10 driver

	This driver boots the DX10 build tape and build a bootable system disk.
	I have been able to run a few programs (including most games from the
	"fun and games" tape), but I have been unable to perform system generation
	and install BASIC/COBOL/PASCAL.

TODO :
* programmer panel
* emulate TILINE fully: timings, tiline timeout, possibly memory error
* finish tape emulation (write support)
* add additional devices as need appears (931 VDT, FD800, card reader, ASR/KSR, printer)
* emulate 990/10A and 990/12 CPUs?
* find out why the computer locks when executing ALGS and BASIC/COBOL/PASCAL installation
*/

/*
	CRU map:

	990/10 CPU board:
	1fa0-1fbe: map file CRU interface
	1fc0-1fde: error interrupt register
	1fe0-1ffe: control panel

	optional hardware (default configuration):
	0000-001e: 733 ASR
	0020-003e: PROM programmer
	0040-005e: card reader
	0060-007e: line printer
	0080-00be: FD800 floppy disc
	00c0-00ee: 913 VDT, or 911 VDT
	0100-013e: 913 VDT #2, or 911 VDT
	0140-017e: 913 VDT #3, or 911 VDT
	1700-177e (0b00-0b7e, 0f00-0f7e): CI402 serial controller #0 (#1, #2) (for 931/940 VDT)
		(note that CRU base 1700 is used by the integrated serial controller in newer S300,
		S300A, 990/10A (and 990/5?) systems)
	1f00-1f1e: CRU expander #1 interrupt register
	1f20-1f3e: CRU expander #2 interrupt register
	1f40-1f5e: TILINE coupler interrupt control #1-8


	TPCS map:
	1ff800: disk controller #1 (system disk)
	1ff810->1ff870: extra disk controllers #2 through #8
	1ff880 (1ff890): tape controller #1 (#2)
	1ff900->1ff950: communication controllers #1 through #6
	1ff980 (1ff990, 1ff9A0): CI403/404 serial controller #1 (#2, #3) (for 931/940 VDT)
	1ffb00, 1ffb04, etc: ECC memory controller #1, #2, etc, diagnostic
	1ffb10, 1ffb14, etc: cache memory controller #1, #2, etc, diagnostic


	interrupt map (default configuration):
	0,1,2: CPU board
	3: free
	4: card reader
	5: line clock
	6: 733 ASR/KSR
	7: FD800 floppy (or FD1000 floppy)
	8: free
	9: 913 VDT #3
	10: 913 VDT #2
	11: 913 VDT
	12: free
	13: hard disk
	14 line printer
	15: PROM programmer (actually not used)
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "machine/ti990.h"
#include "machine/990_hd.h"
#include "machine/990_tap.h"
#include "vidhrdw/911_vdt.h"

static void machine_init_ti990_10(void)
{
	ti990_hold_load();

	ti990_reset_int();

	ti990_tpc_init(ti990_set_int9);
	ti990_hdc_init(ti990_set_int13);
}

static void ti990_10_line_interrupt(void)
{
	vdt911_keyboard(0);

	ti990_line_interrupt();
}

/*static void idle_callback(int state)
{
}*/

static void rset_callback(void)
{
	ti990_cpuboard_reset();

	vdt911_reset();
	/* ... */

	/* clear controller panel and smi fault LEDs */
}

static void lrex_callback(void)
{
	/* right??? */
	ti990_hold_load();
}

/*
	TI990/10 video emulation.

	We emulate a single VDT911 CRT terminal.
*/


static int video_start_ti990_10(void)
{
	const vdt911_init_params_t params =
	{
		char_1920,
		vdt911_model_US/*vdt911_model_UK*//*vdt911_model_French*//*vdt911_model_French*/
		/*vdt911_model_German*//*vdt911_model_Swedish*//*vdt911_model_Norwegian*/
		/*vdt911_model_Japanese*//*vdt911_model_Arabic*//*vdt911_model_FrenchWP*/,
		ti990_set_int10
	};

	return vdt911_init_term(0, & params);
}

static void video_update_ti990_10(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	vdt911_refresh(bitmap, 0, 0, 0);
}

/*
  Memory map - see description above
*/

static MEMORY_READ16_START (ti990_10_readmem)

	{ 0x000000, 0x0fffff, MRA16_RAM },		/* let's say we have 1MB of RAM */
	{ 0x100000, 0x1ff7ff, MRA16_NOP },		/* free TILINE space */
	{ 0x1ff800, 0x1ff81f, ti990_hdc_r },	/* disk controller TPCS */
	{ 0x1ff820, 0x1ff87f, MRA16_NOP },		/* free TPCS */
	{ 0x1ff880, 0x1ff89f, ti990_tpc_r },	/* tape controller TPCS */
	{ 0x1ff8a0, 0x1ffbff, MRA16_NOP },		/* free TPCS */
	{ 0x1ffc00, 0x1fffff, MRA16_ROM },		/* LOAD ROM */

MEMORY_END

static MEMORY_WRITE16_START (ti990_10_writemem)

	{ 0x000000, 0x0fffff, MWA16_RAM },		/* let's say we have 1MB of RAM */
	{ 0x100000, 0x1ff7ff, MWA16_NOP },		/* free TILINE space */
	{ 0x1ff800, 0x1ff81f, ti990_hdc_w },	/* disk controller TPCS */
	{ 0x1ff820, 0x1ff87f, MWA16_NOP },		/* free TPCS */
	{ 0x1ff880, 0x1ff89f, ti990_tpc_w },	/* tape controller TPCS */
	{ 0x1ff8a0, 0x1ffbff, MWA16_NOP },		/* free TPCS */
	{ 0x1ffc00, 0x1fffff, MWA16_ROM },		/* LOAD ROM */

MEMORY_END


/*
  CRU map
*/

static PORT_WRITE16_START ( ti990_10_writeport )

	{ 0x80 << 1, 0x8f << 1, vdt911_0_cru_w },

	{ 0xfd0 << 1, 0xfdf << 1, ti990_10_mapper_cru_w },
	{ 0xfe0 << 1, 0xfef << 1, ti990_10_eir_cru_w },
	{ 0xff0 << 1, 0xfff << 1, ti990_panel_write },

PORT_END

static PORT_READ16_START ( ti990_10_readport )

	{ 0x10 << 1, 0x11 << 1, vdt911_0_cru_r },

	{ 0x1fa << 1, 0x1fb << 1, ti990_10_mapper_cru_r },
	{ 0x1fc << 1, 0x1fd << 1, ti990_10_eir_cru_r },
	{ 0x1fe << 1, 0x1ff << 1, ti990_panel_read },

PORT_END

static ti990_10reset_param reset_params =
{
	/*idle_callback*/NULL,
	rset_callback,
	lrex_callback,
	ti990_ckon_ckof_callback,

	ti990_set_int2
};

static struct beep_interface vdt_911_beep_interface =
{
	1,
	{ 50 }
};


static MACHINE_DRIVER_START(ti990_10)

	/* basic machine hardware */
	/* TI990/10 CPU @ 4.0(???) MHz */
	MDRV_CPU_ADD(TI990_10, 4000000)
	/*MDRV_CPU_FLAGS(0)*/
	MDRV_CPU_CONFIG(reset_params)
	MDRV_CPU_MEMORY(ti990_10_readmem, ti990_10_writemem)
	MDRV_CPU_PORTS(ti990_10_readport, ti990_10_writeport)
	/*MDRV_CPU_VBLANK_INT(NULL, 0)*/
	MDRV_CPU_PERIODIC_INT(ti990_10_line_interrupt, 120/*or 100 in Europe*/)

	/* video hardware - we emulate a single 911 vdt display */
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	/*MDRV_INTERLEAVE(interleave)*/

	MDRV_MACHINE_INIT( ti990_10 )
	/*MDRV_MACHINE_STOP( ti990_10 )*/
	/*MDRV_NVRAM_HANDLER( NULL )*/

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	/*MDRV_ASPECT_RATIO(num, den)*/
	MDRV_SCREEN_SIZE(560, 280)
	MDRV_VISIBLE_AREA(0, 560-1, 0, /*250*/280-1)

	MDRV_GFXDECODE(vdt911_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(vdt911_palette_size)
	MDRV_COLORTABLE_LENGTH(vdt911_colortable_size)

	MDRV_PALETTE_INIT(vdt911)
	MDRV_VIDEO_START(ti990_10)
	/*MDRV_VIDEO_STOP(ti990_10)*/
	/*MDRV_VIDEO_EOF(name)*/
	MDRV_VIDEO_UPDATE(ti990_10)

	MDRV_SOUND_ATTRIBUTES(0)
	/* 911 VDT has a beep tone generator */
	MDRV_SOUND_ADD(BEEP, vdt_911_beep_interface)

MACHINE_DRIVER_END


/*
  ROM loading
*/
ROM_START(ti990_10)

	/*CPU memory space*/
#if 0

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : older boot ROMs for floppy-disk */
	ROM_LOAD16_BYTE("975383.31", 0x1FFC00, 0x100, 0x64fcd040)
	ROM_LOAD16_BYTE("975383.32", 0x1FFC01, 0x100, 0x64277276)
	ROM_LOAD16_BYTE("975383.29", 0x1FFE00, 0x100, 0xaf92e7bf)
	ROM_LOAD16_BYTE("975383.30", 0x1FFE01, 0x100, 0xb7b40cdc)

#elif 1

	ROM_REGION16_BE(0x200000, REGION_CPU1,0)

	/* TI990/10 : newer "universal" boot ROMs  */
	ROM_LOAD16_BYTE("975383.45", 0x1FFC00, 0x100, 0x391943c7)
	ROM_LOAD16_BYTE("975383.46", 0x1FFC01, 0x100, 0xf40f7c18)
	ROM_LOAD16_BYTE("975383.47", 0x1FFE00, 0x100, 0x1ba571d8)
	ROM_LOAD16_BYTE("975383.48", 0x1FFE01, 0x100, 0x8852b09e)

#else

	ROM_REGION16_BE(0x202000, REGION_CPU1,0)

	/* TI990/12 ROMs - actually incompatible with TI990/10, but I just wanted to disassemble them. */
	ROM_LOAD16_BYTE("ti2025-7", 0x1FFC00, 0x1000, 0x4824f89c)
	ROM_LOAD16_BYTE("ti2025-8", 0x1FFC01, 0x1000, 0x51fef543)
	/* the other half of this ROM is not loaded - it makes no sense as TI990/12 machine code, as
	it is microcode... */

#endif


	/* VDT911 character definitions */
	ROM_REGION(vdt911_chr_region_len, vdt911_chr_region, 0)

ROM_END

static void init_ti990_10(void)
{
#if 0
	/* load specific ti990/12 rom page */
	const int page = 3;

	memmove(memory_region(REGION_CPU1)+0x1FFC00, memory_region(REGION_CPU1)+0x1FFC00+(page*0x400), 0x400);
#endif
	vdt911_init();
}

INPUT_PORTS_START(ti990_10)
	VDT911_KEY_PORTS
INPUT_PORTS_END

SYSTEM_CONFIG_START(ti990_10)
	CONFIG_DEVICE_LEGACY(IO_HARDDISK, 4, "hd\0", IO_RESET_NONE, OSD_FOPEN_RW_OR_READ, ti990_hd_init, ti990_hd_exit, NULL)
	CONFIG_DEVICE_LEGACY(IO_CASSETTE, 4, "tap\0", IO_RESET_NONE, OSD_FOPEN_RW_OR_READ, ti990_tape_init, ti990_tape_exit, NULL)
SYSTEM_CONFIG_END

/*	  YEAR	NAME		PARENT	MACHINE		INPUT		INIT		CONFIG		COMPANY					FULLNAME */
COMP( 1975,	ti990_10,	0,		ti990_10,	ti990_10,	ti990_10,	ti990_10,	"Texas Instruments",	"TI Model 990/10 Minicomputer System" )
