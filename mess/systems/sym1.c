/******************************************************************************
 Synertek SYM1
 (kim1 variant)
 PeT mess@utanet.at May 2000
******************************************************************************/

#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/riot6532.h"
#include "machine/6522via.h"

#include "includes/sym1.h"

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x8000, 0x8fff, MRA_ROM },
	{ 0xa000, 0xa00f, via_0_r },
	{ 0xa400, 0xa40f, riot_0_r },
	{ 0xa600, 0xa67f, MRA_RAM },
//	{ 0xab00, 0xab0f, via_1_r },
//	{ 0xac00, 0xac0f, via_2_r },
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x8000, 0x8fff, MWA_ROM },
	{ 0xa000, 0xa00f, via_0_w },
	{ 0xa400, 0xa40f, riot_0_w },
	{ 0xa600, 0xa67f, MWA_RAM },
//	{ 0xab00, 0xab0f, via_1_w },
//	{ 0xac00, 0xac0f, via_2_w },
	{ 0xf000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( sym1 )
	PORT_START			/* IN0 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "0     USR0",	KEYCODE_0,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "4     USR4",	KEYCODE_4,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "8     JUMP",	KEYCODE_8,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "C     CALC",	KEYCODE_C,      IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "CR    S DBL",	KEYCODE_ENTER,  IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "GO    LD P",	KEYCODE_F3,     IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "LD 2  LD 1",	KEYCODE_F1,     IP_JOY_NONE )
	PORT_BIT (0x80, 0x80, IPT_UNUSED )

	PORT_START			/* IN1 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "1     USR1",	KEYCODE_1,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "5     USR5",	KEYCODE_5,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "9     VER",	KEYCODE_9,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "D     DEP",	KEYCODE_D,      IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "-     +",		KEYCODE_MINUS,  IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "REG   SAV P",	KEYCODE_F4,     IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "SAV 2 SAV 1",	KEYCODE_F2,     IP_JOY_NONE )
	PORT_BIT (0x80, 0x80, IPT_UNUSED )

	PORT_START			/* IN2 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "2     USR2",	KEYCODE_2,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "6     USR6",	KEYCODE_6,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "A     ASCII",	KEYCODE_A,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "E     EXEC",	KEYCODE_E,		IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "Right Left",	KEYCODE_RIGHT,  KEYCODE_LEFT )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "MEM   WP",		KEYCODE_F5,     IP_JOY_NONE )
	PORT_BIT (0xc0, 0xc0, IPT_UNUSED )

	PORT_START			/* IN3 */
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "3     USR3",	KEYCODE_3,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "7     USR7",	KEYCODE_7,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "B     B MOV",	KEYCODE_B,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "F     FILL",	KEYCODE_F,		IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "SHIFT",		KEYCODE_LSHIFT,	KEYCODE_RSHIFT )
	PORT_BIT (0xe0, 0xe0, IPT_UNUSED )

	PORT_START			/* IN4 */
	PORT_BITX(0x80, 0x80, IPT_KEYBOARD, "DEBUG OFF",	KEYCODE_F6,     IP_JOY_NONE )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "DEBUG ON",		KEYCODE_F7,     IP_JOY_NONE )

	PORT_BITX(0x03, 0x03, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "RAM", CODE_NONE, CODE_NONE )
	PORT_DIPSETTING( 0x00, "1 KBYTE")
	PORT_DIPSETTING( 0x01, "2 KBYTE")
	PORT_DIPSETTING( 0x02, "3 KBYTE")
	PORT_DIPSETTING( 0x03, "4 KBYTE")
INPUT_PORTS_END

static struct DACinterface dac_interface =
{
	1,			/* number of DACs */
	{ 100 } 	/* volume */
};

static struct MachineDriver machine_driver_sym1 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 MHz */
			readmem,writemem,0,0,
			sym1_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	sym1_init_machine,
	NULL,			/* stop machine */

	/* video hardware (well, actually there was no video ;) */
	800, 600, { 0, 800-1, 0, 600 - 1},
//	640, 480, { 0, 640-1, 0, 480 - 1},
	0,
	242*3 + 32768,
	0,
	sym1_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	sym1_vh_start,
	sym1_vh_stop,
	sym1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
        {
			SOUND_DAC,
			&dac_interface
		}
    }
};

ROM_START(sym1)
	ROM_REGION(0x10000,REGION_CPU1, 0)
//	ROM_LOAD("basicv11", 0xc000, 0x2000, 0x075b0bbd)
	ROM_LOAD("sym1", 0x8000, 0x1000, 0x7a4b1e12)
	ROM_RELOAD(0xf000, 0x1000)
ROM_END

static const struct IODevice io_sym1[] = {
	IODEVICE_CBM_ROM("60\00080\0c0\0"),
#if 0
    {
		IO_CASSETTE,		/* type */
        1,                  /* count */
		"kim\0",            /* file extensions */
        NULL,               /* private */
		0,
		kim1_cassette_init, /* init */
		kim1_cassette_exit, /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
#endif
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMPX( 1978, sym1,	  0, 		sym1,	  sym1, 	sym1,	  "Synertek Systems Corp",  "SYM-1/SY-VIM-1", GAME_NOT_WORKING)


#ifdef RUNTIME_LOADER
extern void sym1_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"sym1")==0) drivers[i]=&driver_sym1;
	}
}
#endif
