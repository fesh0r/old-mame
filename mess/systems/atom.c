/***************************************************************************
Acorn Atom:

Memory map.

CPU: 65C02
		0000-00ff Zero page
		0100-01ff Stack
		0200-1fff RAM (expansion)
		2000-21ff RAM (dos catalogue buffer)
		2200-27ff RAM (dos seq file buffer)
		2800-28ff RAM (float buffer)
		2900-7fff RAM (text RAM)
		8000-97ff VDG 6847
		9800-9fff RAM (expansion)
		a000-afff ROM (extension)
		b000-b003 PPIA 8255
		b003-b7ff NOP
		b800-b80f VIA 6522
		b810-b9ff NOP
		ba00-ba04 FDC 8271
		bc00-bfdf NOP
		bfe0-bfe2 MOUSE
		bfe3-bfff NOP
		c000-cfff ROM (basic)
		d000-dfff ROM (float)
		e000-efff ROM (dos)
		f000-ffff ROM (kernel)

Video:		MC6847

Sound:		Buzzer

Floppy:		FDC8271

Hardware:	PPIA 8255

	output	b000	0 - 3 keyboard row, 4 - 7 graphics mode
			b002	0 cas output, 1 enable 2.4Khz, 2 buzzer, 3 colour set

	input	b001	0 - 5 keyboard column, 6 CTRL key, 7 SHIFT key
			b002	4 2.4kHz input, 5 cas input, 6 REPT key, 7 60 Hz input

			VIA 6522
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/8255ppi.h"
#include "vidhrdw/m6847.h"
#include "includes/atom.h"

/* functions */

/* port i/o functions */

/* memory w/r functions */

static struct MemoryReadAddress atom_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_RAM },
	{ 0x8000, 0x97ff, videoram_r },		// VDG 6847
	{ 0x9800, 0x9fff, MRA_RAM },
	{ 0xa000, 0xafff, MRA_ROM },
	{ 0xb000, 0xb7ff, ppi8255_0_r },	// PPIA 8255
	{ 0xb800, 0xb9ff, MRA_NOP },		// VIA 6522
	{ 0xba00, 0xbfdf, MRA_NOP },		// FDC 8271
	{ 0xbfe0, 0xbfff, MRA_NOP },		// MOUSE
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }
};

static struct MemoryWriteAddress atom_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_RAM },
	{ 0x8000, 0x97ff, videoram_w, &videoram, &videoram_size}, // VDG 6847
	{ 0x9800, 0x9fff, MWA_RAM },
	{ 0xa000, 0xafff, MWA_ROM },
	{ 0xb000, 0xb7ff, ppi8255_0_w },	// PIA 8255

	{ 0xb800, 0xb9ff, MWA_NOP },		// VIA 6522
	{ 0xba00, 0xbfdf, MWA_NOP },		// FDC 8271
	{ 0xbfe0, 0xbfff, MWA_NOP },		// MOUSE
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }
};

/* graphics output */

/* keyboard input */

/* not implemented: BREAK */

INPUT_PORTS_START (atom)
	PORT_START /* 0 VBLANK */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START /* 1 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "escape", KEYCODE_ESC, IP_JOY_NONE)

	PORT_START /* 2 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE)

	PORT_START /* 3 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "up", KEYCODE_UP, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE)

	PORT_START /* 4 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "right", KEYCODE_RIGHT, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_EQUALS, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE)

	PORT_START /* 5 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "caps lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "backspace", KEYCODE_BACKSPACE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE)

	PORT_START /* 6 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "uparrow", KEYCODE_TILDE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "copy", KEYCODE_TAB, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE)

	PORT_START /* 7 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "enter", KEYCODE_ENTER, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE)

	PORT_START /* 8 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_BACKSLASH, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", KEYCODE_QUOTE, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE)

	PORT_START /* 9 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE)

	PORT_START /* 10 */
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "space", KEYCODE_SPACE, IP_JOY_NONE)
	PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "deadkey", KEYCODE_NONE, IP_JOY_NONE)
	PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE)
	PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE)

	PORT_START /* 11 CTRL & SHIFT */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LControl", KEYCODE_LCONTROL, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RControl", KEYCODE_RCONTROL, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "LShift", KEYCODE_LSHIFT, IP_JOY_NONE)
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "RShift", KEYCODE_RSHIFT, IP_JOY_NONE)

	PORT_START /* 12 REPT key */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "LAlt", KEYCODE_LALT, IP_JOY_NONE)
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RAlt", KEYCODE_RALT, IP_JOY_NONE)

INPUT_PORTS_END

/* sound output */

static	struct	Speaker_interface atom_sh_interface =
{
	1,
	{ 100 },
	{ 0 },
	{ NULL }
};

/* machine definition */

static struct MachineDriver machine_driver_atom =
{
	/* basic machine hardware */
	{
		{
			CPU_M65C02,
			1000000,
			atom_readmem, atom_writemem,
			0, 0,
			0, 0,
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames/sec, vblank duration */
	0,
	atom_init_machine,
	atom_stop_machine,

	/* video hardware */
	32*8,										/* screen width */
	16*12,									/* screen height (pixels doubled) */
	{ 0, 32*8-1, 0, 16*12-1},					/* visible_area */
	0,							/* graphics decode info */
	M6847_TOTAL_COLORS,
	0,
	m6847_vh_init_palette,						/* initialise palette */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	atom_vh_start,
	m6847_vh_stop,
	m6847_vh_update,
	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SPEAKER,
			&atom_sh_interface
		}
	}
};

ROM_START (atom)
	ROM_REGION (0x10000, REGION_CPU1)
	ROM_LOAD ("akernel.rom", 0xf000, 0x1000, 0xc604db3d)
	// ROM_LOAD ("ados.rom", 0xe000, 0x1000, 0xe5b1f5f6)
	ROM_LOAD ("afloat.rom", 0xd000, 0x1000, 0x81d86af7)
	ROM_LOAD ("abasic.rom", 0xc000, 0x1000, 0x43798b9b)
	//ROM_REGION (0x300, REGION_GFX1)
	//ROM_LOAD ("atom.chr", 0x0000, 0x0300, 0x0)
ROM_END

static const struct IODevice io_atom[] =
{
	{
		IO_CARTSLOT,
		1,						/* count */
		"atm\0",				/* file extn */
		IO_RESET_ALL,			/* reset if file changed */
        NULL,                   /* id */
		atom_init_atm,			/* init */
		NULL,					/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		NULL,					/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
	{ IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
COMP( 1979, atom,     0,        atom,     atom,     0,        "Acorn",  "Atom" )
