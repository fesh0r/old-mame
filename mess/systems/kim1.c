/******************************************************************************
	KIM-1

	system driver

	Juergen Buchmueller, Oct 1999

    KIM-1 memory map

    range     short     description
    0000-03FF RAM
    1400-16FF ???
    1700-173F 6530-003  see 6530
    1740-177F 6530-002  see 6530
    1780-17BF RAM       internal 6530-003
    17C0-17FF RAM       internal 6530-002
    1800-1BFF ROM       internal 6530-003
    1C00-1FFF ROM       internal 6530-002

    6530
    offset  R/W short   purpose
    0       X   DRA     Data register A
    1       X   DDRA    Data direction register A
    2       X   DRB     Data register B
    3       X   DDRB    Data direction register B
    4       W   CNT1T   Count down from value, divide by 1, disable IRQ
    5       W   CNT8T   Count down from value, divide by 8, disable IRQ
    6       W   CNT64T  Count down from value, divide by 64, disable IRQ
            R   LATCH   Read current counter value, disable IRQ
    7       W   CNT1KT  Count down from value, divide by 1024, disable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow
    8       X   DRA     Data register A
    9       X   DDRA    Data direction register A
    A       X   DRB     Data register B
    B       X   DDRB    Data direction register B
    C       W   CNT1I   Count down from value, divide by 1, enable IRQ
    D       W   CNT8I   Count down from value, divide by 8, enable IRQ
    E       W   CNT64I  Count down from value, divide by 64, enable IRQ
            R   LATCH   Read current counter value, enable IRQ
    F       W   CNT1KI  Count down from value, divide by 1024, enable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow

    6530-002 (U2)
        DRA bit write               read
        ---------------------------------------------
            0-6 segments A-G        key columns 1-7
            7   PTR                 KBD

        DRB bit write               read
        ---------------------------------------------
            0   PTR                 -/-
            1-4 dec 1-3 key row 1-3
                    4   RW3
                    5-9 7-seg   1-6
    6530-003 (U3)
        DRA bit write               read
        ---------------------------------------------
            0-7 bus PA0-7           bus PA0-7

        DRB bit write               read
        ---------------------------------------------
            0-7 bus PB0-7           bus PB0-7

******************************************************************************/

#include "driver.h"
#include "includes/kim1.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

static MEMORY_READ_START ( readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x1700, 0x173f, m6530_003_r },
	{ 0x1740, 0x177f, m6530_002_r },
	{ 0x1780, 0x17bf, MRA_RAM },
	{ 0x17c0, 0x17ff, MRA_RAM },
	{ 0x1800, 0x1bff, MRA_ROM },
	{ 0x1c00, 0x1fff, MRA_ROM },
	{ 0x2000, 0xffff, kim1_mirror_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x1700, 0x173f, m6530_003_w },
	{ 0x1740, 0x177f, m6530_002_w },
	{ 0x1780, 0x17bf, MWA_RAM },
	{ 0x17c0, 0x17ff, MWA_RAM },
	{ 0x1800, 0x1bff, MWA_ROM },
	{ 0x1c00, 0x1fff, MWA_ROM },
	{ 0x2000, 0xffff, kim1_mirror_w },
MEMORY_END

INPUT_PORTS_START( kim1 )
	PORT_START			/* IN0 keys row 0 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "0.6: 0",        KEYCODE_0,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "0.5: 1",        KEYCODE_1,      IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "0.4: 2",        KEYCODE_2,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "0.3: 3",        KEYCODE_3,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "0.2: 4",        KEYCODE_4,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "0.1: 5",        KEYCODE_5,      IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "0.0: 6",        KEYCODE_6,      IP_JOY_NONE )
	PORT_START			/* IN1 keys row 1 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "1.6: 7",        KEYCODE_7,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "1.5: 8",        KEYCODE_8,      IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "1.4: 9",        KEYCODE_9,      IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "1.3: A",        KEYCODE_A,      IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "1.2: B",        KEYCODE_B,      IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "1.1: C",        KEYCODE_C,      IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "1.0: D",        KEYCODE_D,      IP_JOY_NONE )
	PORT_START			/* IN2 keys row 2 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "2.6: E",        KEYCODE_E,      IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "2.5: F",        KEYCODE_F,      IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_KEYBOARD, "2.4: AD (F1)",  KEYCODE_F1,     IP_JOY_NONE )
	PORT_BITX(0x08, 0x08, IPT_KEYBOARD, "2.3: DA (F2)",  KEYCODE_F2,     IP_JOY_NONE )
	PORT_BITX(0x04, 0x04, IPT_KEYBOARD, "2.2: +  (CR)",  KEYCODE_ENTER,  IP_JOY_NONE )
	PORT_BITX(0x02, 0x02, IPT_KEYBOARD, "2.1: GO (F5)",  KEYCODE_F5,     IP_JOY_NONE )
	PORT_BITX(0x01, 0x01, IPT_KEYBOARD, "2.0: PC (F6)",  KEYCODE_F6,     IP_JOY_NONE )
	PORT_START			/* IN3 STEP and RESET keys, MODE switch */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BITX(0x40, 0x40, IPT_KEYBOARD, "sw1: ST (F7)",  KEYCODE_F7,     IP_JOY_NONE )
	PORT_BITX(0x20, 0x20, IPT_KEYBOARD, "sw2: RS (F3)",  KEYCODE_F3,     IP_JOY_NONE )
	PORT_BITX(0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "sw3: SS (NumLock)", KEYCODE_NUMLOCK, IP_JOY_NONE )
	PORT_DIPSETTING( 0x00, "single step")
	PORT_DIPSETTING( 0x10, "run")
	PORT_BIT (0x08, 0x00, IPT_UNUSED )
	PORT_BIT (0x04, 0x00, IPT_UNUSED )
	PORT_BIT (0x02, 0x00, IPT_UNUSED )
	PORT_BIT (0x01, 0x00, IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout led_layout =
{
	18, 24, 	/* 16 x 24 LED 7segment displays */
	128,		/* 128 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15,
	 16,17 },
	{ 0*24, 1*24, 2*24, 3*24,
	  4*24, 5*24, 6*24, 7*24,
	  8*24, 9*24,10*24,11*24,
	 12*24,13*24,14*24,15*24,
	 16*24,17*24,18*24,19*24,
	 20*24,21*24,22*24,23*24,
	 24*24,25*24,26*24,27*24,
	 28*24,29*24,30*24,31*24 },
	24 * 24,	/* every LED code takes 32 times 18 (aligned 24) bit words */
};

static struct GfxLayout key_layout =
{
	24, 18, 	/* 24 * 18 keyboard icons */
	24, 		/* 24  codes */
	2,			/* 2 bit per pixel */
	{ 0, 1 },	/* two bitplanes */
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2,
	  8*2, 9*2,10*2,11*2,12*2,13*2,14*2,15*2,
	 16*2,17*2,18*2,19*2,20*2,21*2,22*2,23*2 },
	{ 0*24*2, 1*24*2, 2*24*2, 3*24*2, 4*24*2, 5*24*2, 6*24*2, 7*24*2,
	  8*24*2, 9*24*2,10*24*2,11*24*2,12*24*2,13*24*2,14*24*2,15*24*2,
	 16*24*2,17*24*2 },
	18 * 24 * 2,	/* every icon takes 18 rows of 24 * 2 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0, &led_layout, 0, 16 },
	{ 2, 0, &key_layout, 16*2, 2 },
MEMORY_END	 /* end of array */

static struct DACinterface dac_interface =
{
	1,			/* number of DACs */
	{ 100 } 	/* volume */
};

static struct MachineDriver machine_driver_kim1 =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 MHz */
			readmem,writemem,0,0,
			kim1_interrupt, 1
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	kim1_init_machine,
	NULL,			/* stop machine */

	/* video hardware (well, actually there was no video ;) */
	600, 768, { 0, 600 - 1, 0, 768 - 1},
	gfxdecodeinfo,
	32768+21,				/* leave extra colors for the overlay */
	256,
	kim1_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */
	kim1_vh_start,
	kim1_vh_stop,
	kim1_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
        {
			SOUND_DAC,
			&dac_interface
		}
    }
};

ROM_START(kim1)
	ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("6530-003.bin",    0x1800, 0x0400, 0xa2a56502)
		ROM_LOAD("6530-002.bin",    0x1c00, 0x0400, 0x2b08e923)
	ROM_REGION(128 * 24 * 3,REGION_GFX1,0)
		/* space filled with 7segement graphics by kim1_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,REGION_GFX2,0)
		/* space filled with key icons by kim1_init_driver */
ROM_END



static const struct IODevice io_kim1[] = {
    {
		IO_CASSETTE,		/* type */
        1,                  /* count */
		"kim\0",            /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
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
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1975, kim1,	  0, 		kim1,	  kim1, 	kim1,	  "MOS Technologies",  "KIM-1" )

