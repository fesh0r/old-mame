/************************************************************************
Aquarius Memory map

	CPU: z80

	Memory map
		0000 1fff	BASIC
		2000 2fff	expansion?
		3000 33ff	screen ram
		3400 37ff	colour ram
		3800 3fff	RAM (standard)
		4000 7fff	RAM (expansion)
		8000 ffff	RAM (emulator only)

	Ports: Out
		fc			Buzzer, bit 0.
		fe			Printer.

	Ports: In
		fc			Tape in, bit 1.
		fe			Printer.
		ff			Keyboard, Bit set in .B selects keyboard matrix
					line. Return bit 0 - 5 low for pressed key.

************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/generic.h"
#include "includes/aquarius.h"

/* structures */

/* port i/o functions */

PORT_READ_START( aquarius_readport )
	{0xfe, 0xfe, aquarius_port_fe_r},
	{0xff, 0xff, aquarius_port_ff_r},
PORT_END

PORT_WRITE_START( aquarius_writeport )
	{0xfc, 0xfc, aquarius_port_fc_w},
	{0xfe, 0xfe, aquarius_port_fe_w},
	{0xff, 0xff, aquarius_port_ff_w},
PORT_END

/* Memory w/r functions */

MEMORY_READ_START( aquarius_readmem )
	{0x0000, 0x1fff, MRA_ROM},
	{0x2000, 0x2fff, MRA_NOP},
	{0x3000, 0x37ff, videoram_r},
	{0x3800, 0x3fff, MRA_RAM},
	{0x4000, 0x7fff, MRA_NOP},
	{0x8000, 0xffff, MRA_NOP},

MEMORY_END

MEMORY_WRITE_START( aquarius_writemem )
	{0x0000, 0x1fff, MWA_ROM},
	{0x2000, 0x2fff, MWA_NOP},
	{0x3000, 0x37ff, videoram_w, &videoram, &videoram_size},
	{0x3800, 0x3fff, MWA_RAM},
	{0x4000, 0x7fff, MWA_NOP},
	{0x8000, 0xffff, MWA_NOP},
MEMORY_END

/* graphics output */

struct	GfxLayout	aquarius_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8,
	  4*8, 5*8, 6*8, 7*8, },
	8 * 8
};

static	struct	GfxDecodeInfo	aquarius_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &aquarius_charlayout, 0, 256},
MEMORY_END

static	unsigned	char	aquarius_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xff, 0x7f, 0x7f,	/* Red */
	0x7f, 0xff, 0x7f,	/* Green */
	0xff, 0xff, 0x7f,	/* Yellow */
	0x7f, 0x7f, 0xff,	/* Blue */
	0xff, 0x7f, 0xff,	/* Magenta */
	0x7f, 0xff, 0xff,	/* Cyan */
	0xff, 0xff, 0xff,	/* White */
	0x00, 0x00, 0x00,	/* Black */
	0x7f, 0x00, 0x00,	/* Dark Red */
	0x00, 0x7f, 0x00,	/* Dark Green */
	0x7f, 0x7f, 0x00,	/* Dark Yellow */
	0x00, 0x00, 0x7f,	/* Dark Blue */
	0x7f, 0x00, 0x7f,	/* Dark Magenta */
	0x00, 0x7f, 0x7f,	/* Dark Cyan */
	0x7f, 0x7f, 0x7f,	/* Grey */
};

static	unsigned	short	aquarius_colortable[] =
{
    0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15, 0,
    0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15, 1,
    0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15, 2,
    0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15, 3,
    0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15, 4,
    0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15, 5,
    0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15, 6,
    0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15, 7,
    0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15, 8,
    0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15, 9,
    0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,10,
    0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,11,
    0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,12,
    0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,13,
    0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,14,
    0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,15,
};

static	void	aquarius_init_palette (unsigned char *sys_palette,
			unsigned short *sys_colortable, const unsigned char *color_prom)
{
	memcpy (sys_palette, aquarius_palette, sizeof (aquarius_palette));
	memcpy (sys_colortable, aquarius_colortable, sizeof (aquarius_colortable));
}

/* Keyboard input */

INPUT_PORTS_START (aquarius)
	PORT_START	/* 0: count = 0 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "return", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 1: count = 1 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "p", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BIT (0xff, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 2: count = 2 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "o", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "k", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "m", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "n", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "j", KEYCODE_J, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 3: count = 3 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "i", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "u", KEYCODE_U, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "h", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "b", KEYCODE_B, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 4: count = 4 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "y", KEYCODE_Y, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "g", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "v", KEYCODE_V, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "c", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f", KEYCODE_F, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 5: count = 5 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "t", KEYCODE_T, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "r", KEYCODE_R, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "d", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "x", KEYCODE_X, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 6: count = 6 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "e", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "s", KEYCODE_S, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "z", KEYCODE_Z, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, " ", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a", KEYCODE_A, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 7: count = 7 */
	PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "w", KEYCODE_W, IP_JOY_NONE)
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "shift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "shift", KEYCODE_RSHIFT, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "ctl", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "ctl", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BIT (0xc0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START	/* 8: Machine config */
	PORT_DIPNAME(0x03, 3, "RAM Size")
	PORT_DIPSETTING(0, "4Kb")
	PORT_DIPSETTING(1, "20Kb")
	PORT_DIPSETTING(2, "56Kb")
INPUT_PORTS_END

/* Sound output */

static struct Speaker_interface aquarius_speaker =
{
	1,			/* one speaker */
	{ 100 },	/* mixing levels */
	{ 0 },		/* optional: number of different levels */
	{ NULL }	/* optional: level lookup table */
};

/* Machine definition */

static	struct	MachineDriver	machine_driver_aquarius =
{
	{
		{
			CPU_Z80,
			3500000,
			aquarius_readmem, aquarius_writemem,
			aquarius_readport, aquarius_writeport,
			interrupt, 1,
		},
	},
	60,  DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	aquarius_init_machine,
	aquarius_stop_machine,
	40 * 8,
	24 * 8,
	{ 0, 40 * 8 - 1, 0, 24 * 8 - 1},
	aquarius_gfxdecodeinfo,
	sizeof (aquarius_palette) / 3,
	sizeof (aquarius_colortable),
	aquarius_init_palette,
	VIDEO_TYPE_RASTER,
	0,
	aquarius_vh_start,
	aquarius_vh_stop,
	aquarius_vh_screenrefresh,
	0, 0, 0, 0,
	{
		{
			SOUND_SPEAKER,
			&aquarius_speaker
		}
	}
};

ROM_START(aquarius)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("aq2.rom", 0x0000, 0x2000, 0xa2d15bcf)
	ROM_REGION(0x0800, REGION_GFX1,0)
	ROM_LOAD("aq2.chr", 0x0000, 0x0800, BADCRC(0x0b3edeed))
ROM_END

static	const	struct	IODevice	io_aquarius[] =
{
	{ IO_END }
};

/*		YEAR	NAME		PARENT		MACHINE		INPUT		INIT	COMPANY		FULLNAME */
COMP(	1983,	aquarius,	0,			aquarius,	aquarius,	0,		"Mattel",	"Aquarius" )
