/*
  Experimental ti99/2 driver

TODO :
  * find a TI99/2 ROM dump (some TI99/2 ARE in private hands)
  * test the driver !
  * understand the "viden" pin
  * implement cassette
  * implement Hex-Bus

  Raphael Nabet (who really has too much time to waste), december 1999, 2000
*/

/*
  TI99/2 facts :

References :
* TI99/2 main logic board schematics, 83/03/28-30 (on ftp.whtech.com, or just ask me)
  (Thanks to Charles Good for posting this)

general :
* prototypes in 1983
* uses a 10.7MHz TMS9995 CPU, with the following features :
  - 8-bit data bus
  - 256 bytes 16-bit RAM (0xff00-0xff0b & 0xfffc-0xffff)
  - only available int lines are INT4 (used by vdp), INT1*, and NMI* (both free for extension)
  - on-chip decrementer (0xfffa-0xfffb)
  - Unlike tms9900, CRU address range is full 0x0000-0xFFFE (A0 is not used as address).
    This is possible because tms9995 uses d0-d2 instead of the address MSBits to support external
    opcodes.
  - quite more efficient than tms9900, and a few additionnal instructions and features
* 24 or 32kb ROM (16kb plain (1kb of which used by vdp), 16kb split into 2 8kb pages)
* 4kb 8-bit RAM, 256 bytes 16-bit RAM
* custom vdp shares CPU RAM/ROM.  The display is quite alike to tms9928 graphics mode, except
  that colors are a static B&W, and no sprite is displayed. The config (particularily the
  table base addresses) cannot be changed.  Since TI located the pattern generator table in
  ROM, we cannot even redefine the char patterns (unless you insert a custom cartidge which
  overrides the system ROMs).  VBL int triggers int4 on tms9995.
* CRU is handled by one single custom chip, so the schematics don't show many details :-( .
* I/O :
  - 48-key keyboard.  Same as TI99/4a, without alpha lock, and with an additional break key.
    Note that the hardware can make the difference between the two shift keys.
  - cassette I/O (one unit)
  - ALC bus (must be another name for Hex-Bus)
* 60-pin expansion/cartidge port on the back

memory map :
* 0x0000-0x4000 : system ROM (0x1C00-0x2000 (?) : char ROM used by vdp)
* 0x4000-0x6000 : system ROM, mapped to either of two distinct 8kb pages according to the S0
  bit from the keyboard interface (!), which is used to select the first key row.
  [only on second-generation TI99/2 protos, first generation protos only had 24kb of ROM]
* 0x6000-0xE000 : free for expansion
* 0xE000-0xF000 : 8-bit "system" RAM (0xEC00-0xEF00 used by vdp)
* 0xF000-0xF0FB : 16-bit processor RAM (on tms9995)
* 0xF0FC-0xFFF9 : free for expansion
* 0xFFFA-0xFFFB : tms9995 internal decrementer
* 0xFFFC-0xFFFF : 16-bit processor RAM (provides the NMI vector)

CRU map :
* 0x0000-0x1EFC : reserved
* 0x1EE0-0x1EFE : tms9995 flag register
* 0x1F00-0x1FD8 : reserved
* 0x1FDA : tms9995 MID flag
* 0x1FDC-0x1FFF : reserved
* 0x2000-0xE000 : unaffected
* 0xE400-0xE40E : keyboard I/O (8 bits input, either 3 or 6 bit output)
* 0xE80C : cassette I/O
* 0xE80A : ALC BAV
* 0xE808 : ALC HSK
* 0xE800-0xE808 : ALC data (I/O)
* 0xE80E : video enable (probably input - seems to come from the VDP, and is present on the
  expansion connector)
* 0xF000-0xFFFE : reserved
Note that only A15-A11 and A3-A1 (A15 = MSB, A0 = LSB) are decoded in the console, so the keyboard
is actually mapped to 0xE000-0xE7FE, and other I/O bits to 0xE800-0xEFFE.
Also, ti99/2 does not support external instructions better than ti99/4(a).  This is crazy, it
would just have taken three extra tracks on the main board and a OR gate in an ASIC.
*/

/*
  TI99/8 preliminary infos (all I know :-( ) :

  name : Texas Instruments Computer TI99/8 (no "Home")

References :
* machine room <http://...>
* TI99/8 user manual

general :
* a few dozen units built in 1983, never released
* same CPU as TI99/2
* 220kb of ROM, including monitor, GPL interpreter (personnal guess), TI-extended basic II,
  and a P-code interpreter with a few utilities.  The user could change the CPU speed to
  improve compatibility with TI99/4x modules.
* 64kb 8-bit RAM, 16kb vdp RAM, possibly 256 bytes 16-bit RAM
* tms9928anc vdp : quite like tms9928a, but two additionnal "split-screen" modes
* I/O
  - 54-key keyboard, plus 2 optional joysticks
  - sound and speech (both ti99/4-like)
  - Hex-Bus
  - Cassette
* cartidge port on the top
* 50-pin(?) expansion port on the back (so, it was not even the same as TI99/2 ????)

memory map :
* 0x8000-unknown address (0x801f?) : custom RAM mapper chip.  4kb page size,
  total address space 16Mb(?).
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/tms9901.h"
#include "vidhrdw/tms9928a.h"
#include "cpu/tms9900/tms9900.h"

static int ROM_paged;

static void init_ti99_2_24(void)
{
	/* no ROM paging */
	ROM_paged = 0;
}

static void init_ti99_2_32(void)
{
	/* ROM paging enabled */
	ROM_paged = 1;
}

static int ti99_2_24_load_rom(void)
{
	cpu_setbank(1, memory_region(REGION_CPU1)+0x4000);

	return 0;
}

#define TI99_2_32_ROMPAGE0 memory_region(REGION_CPU1)+0x4000
#define TI99_2_32_ROMPAGE1 memory_region(REGION_CPU1)+0x10000

static int ti99_2_32_load_rom(void)
{
	memcpy(memory_region(REGION_CPU1), memory_region(REGION_CPU1)+0x10000, 0x4000);

	cpu_setbank(1, TI99_2_32_ROMPAGE0);

	return 0;
}

static void ti99_2_init_machine(void)
{
}

static void ti99_2_stop_machine(void)
{
}

static int ti99_2_vblank_interrupt(void)
{
	TMS9928A_interrupt();

	/* We trigger a level-4 interrupt.  The PULSE_LINE is a mere guess. */
	cpu_set_irq_line(0, 1, PULSE_LINE);

	return ignore_interrupt();
}


/*
  TI99/2 vdp emulation.

  Things could not be simpler.
  We display 24 rows and 32 columns of characters.  Each 8*8 pixel character pattern is defined
  in a 128-entry table located in ROM.  Character code for each screen position are stored
  sequentially in RAM.  Colors are a fixed Black on White.
*/

static unsigned char ti99_2_palette[] =
{
	255, 255, 255,
	0, 0, 0
};

static unsigned short ti99_2_colortable[] =
{
	0, 1
};

#define TI99_2_PALETTE_SIZE sizeof(ti99_2_palette)/3
#define TI99_2_COLORTABLE_SIZE sizeof(ti99_2_colortable)/2

static void ti99_2_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	memcpy(palette, & ti99_2_palette, sizeof(ti99_2_palette));
	memcpy(colortable, & ti99_2_colortable, sizeof(ti99_2_colortable));
}

static int ti99_2_vh_start(void)
{
	videoram_size = 768;

	return generic_vh_start();
}

#define ti99_2_vh_stop generic_vh_stop
#define ti99_2_video_w videoram_w

static void ti99_2_vh_refresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int i, sx, sy;

	if (full_refresh)
		memset(dirtybuffer, 1, videoram_size);

	sx = sy = 0;

	for (i = 0; i < 768; i++)
	{
		if (dirtybuffer[i])
		{
			dirtybuffer[i] = 0;

			/* Is the char code masked or not ??? */
			drawgfx(bitmap, Machine->gfx[0], videoram[i] & 0x7F, 0,
			          0, 0, sx, sy, &Machine->visible_area, TRANSPARENCY_NONE, 0);
			osd_mark_dirty(sx, sy, sx+7, sy+7);
		}

		sx += 8;
		if (sx == 256)
		{
			sx = 0;
			sy += 8;
		}
	}
}

static struct GfxLayout ti99_2_charlayout =
{
	8,8,        /* 8 x 8 characters */
	128,        /* 128 characters */
	1,          /* 1 bits per pixel */
	{ 0 },      /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, },
	8*8         /* every char takes 8 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x1c00, & ti99_2_charlayout, 0, 0 },
	{ -1 }    /* end of array */
};


/*
  Memory map - see description above
*/

static MEMORY_READ_START (ti99_2_readmem )

	{ 0x0000, 0x3fff, MRA_ROM },            /*system ROM*/
	{ 0x4000, 0x5fff, /*MRA_ROM*/MRA_BANK1 },   /*system ROM, banked on 32kb ROMs protos*/
	{ 0x6000, 0xdfff, MRA_NOP },            /*free for expansion*/
	{ 0xe000, 0xefff, MRA_RAM },            /*system RAM*/
	{ 0xf000, 0xffff, MRA_NOP },            /*processor RAM or free*/

MEMORY_END

static MEMORY_WRITE_START ( ti99_2_writemem )

	{ 0x0000, 0x3fff, MWA_ROM },            /*system ROM*/
	{ 0x4000, 0x5fff, /*MWA_ROM*/MWA_BANK1 },       /*system ROM, banked on 32kb ROMs protos*/
	{ 0x6000, 0xdfff, MWA_NOP },            /*free for expansion*/
	{ 0xe000, 0xebff, MWA_RAM },            /*system RAM*/
	{ 0xec00, 0xeeff, ti99_2_video_w, & videoram }, /*system RAM : used for video*/
	{ 0xef00, 0xefff, MWA_RAM },            /*system RAM*/
	{ 0xf000, 0xffff, MWA_NOP },            /*processor RAM or free*/

MEMORY_END


/*
  CRU map - see description above
*/

/* current keyboard row */
static int KeyRow = 0;

/* write the current keyboard row */
static WRITE_HANDLER ( ti99_2_write_kbd )
{
	offset &= 0x7;  /* other address lines are not decoded */

	if (offset <= 2)
	{
		/* this implementation is just a guess */
		if (data)
			KeyRow |= 1 << offset;
		else
			KeyRow &= ~ (1 << offset);
	}
	/* now, we handle ROM paging */
	if (ROM_paged)
	{	/* if we have paged ROMs, page according to S0 keyboard interface line */
		cpu_setbank(1, (KeyRow == 0) ? TI99_2_32_ROMPAGE1 : TI99_2_32_ROMPAGE0);
	}
}

static WRITE_HANDLER ( ti99_2_write_misc_cru )
{
	offset &= 0x7;  /* other address lines are not decoded */

	switch (offset)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		/* ALC I/O */
		break;
	case 4:
		/* ALC HSK */
		break;
	case 5:
		/* ALC BAV */
		break;
	case 6:
		/* cassette output */
		break;
	case 7:
		/* video enable */
		break;
	}
}

static PORT_WRITE_START ( ti99_2_writeport )

	{0x7000, 0x73ff, ti99_2_write_kbd},
	{0x7400, 0x77ff, ti99_2_write_misc_cru},

PORT_END

/* read keys in the current row */
static READ_HANDLER ( ti99_2_read_kbd )
{
	return readinputport(KeyRow);
}

static READ_HANDLER ( ti99_2_read_misc_cru )
{
	return 0;
}

static PORT_READ_START ( ti99_2_readport )

	{0x0E00, 0x0E7f, ti99_2_read_kbd},
	{0x0E80, 0x0Eff, ti99_2_read_misc_cru},

PORT_END


/* ti99/2 : 54-key keyboard */
INPUT_PORTS_START(ti99_2)

	PORT_START    /* col 0 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 #", KEYCODE_3, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)

	PORT_START    /* col 1 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)

	PORT_START    /* col 2 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A", KEYCODE_A, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)

	PORT_START    /* col 3 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

	PORT_START    /* col 4 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT/*KEYCODE_CAPSLOCK*/, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

	PORT_START    /* col 5 */
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "BREAK", KEYCODE_ESC, IP_JOY_NONE)
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, IP_JOY_NONE)
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)

	PORT_START    /* col 6 */
		PORT_BITX(0xFF, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

	PORT_START    /* col 7 */
		PORT_BITX(0xFF, IP_ACTIVE_LOW, IPT_UNUSED, DEF_STR( Unused ), IP_KEY_NONE, IP_JOY_NONE)

INPUT_PORTS_END


static struct tms9995reset_param ti99_2_processor_config =
{
#if 0
	REGION_CPU1,/* region for processor RAM */
	0xf000,     /* offset : this area is unused in our region, and matches the processor address */
	0xf0fc,		/* offset for the LOAD vector */
	NULL,       /* no IDLE callback */
	1,          /* use fast IDLE */
#endif
	1           /* enable automatic wait state generation */
};

static struct MachineDriver machine_driver_ti99_2 =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9995,
			10700000,     /* 10.7 Mhz*/

			ti99_2_readmem, ti99_2_writemem, ti99_2_readport, ti99_2_writeport,
			ti99_2_vblank_interrupt, 1,
			0, 0,
			& ti99_2_processor_config
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,
	ti99_2_init_machine,
	ti99_2_stop_machine,

	/* video hardware */
	256,                      /* screen width */
	192,                      /* screen height */
	{ 0, 256-1, 0, 192-1},    /* visible_area */
	gfxdecodeinfo,            /* graphics decode info (???)*/
	TI99_2_PALETTE_SIZE,      /* palette is 3*total_colors bytes long */
	TI99_2_COLORTABLE_SIZE,   /* length in shorts of the color lookup table */
	ti99_2_init_palette,      /* palette init */

	VIDEO_TYPE_RASTER,
	0,
	ti99_2_vh_start,
	ti99_2_vh_stop,
	ti99_2_vh_refresh,

	/* sound hardware */
	0,
	0,0,0,
#if 0
	{ /* no sound ! */
	}
#endif
};


/*
  ROM loading
*/
ROM_START(ti99_224)
	/*CPU memory space*/
	ROM_REGION(0x10000,REGION_CPU1,0)
	ROM_LOAD("992rom.bin", 0x0000, 0x6000, 0x00000000)      /* system ROMs */
ROM_END

ROM_START(ti99_232)
	/*64kb CPU memory space + 8kb to read the extra ROM page*/
	ROM_REGION(0x12000,REGION_CPU1,0)
	ROM_LOAD("992rom32.bin", 0x0000, 0x6000, 0x00000000)    /* system ROM - 32kb */
	ROM_CONTINUE(0x10000,0x2000)
ROM_END

static const struct IODevice io_ti99_2[] =
{
	/* one expansion/cartidge port on the back */
	/* one tape drive port */
	/* Hex-bus disk controller supports up to 4 floppy disk drives */
	/* None of these is supported (tape should be easy) */
    { IO_END }
};

#define io_ti99_224 io_ti99_2
#define io_ti99_232 io_ti99_2

/*		YEAR	NAME		PARENT		MACHINE		INPUT	INIT		COMPANY					FULLNAME */
COMP(	1983,	ti99_224,	0,			ti99_2,		ti99_2,	ti99_2_24,	"Texas Instruments",	"TI-99/2 BASIC Computer (24kb ROMs)" )
COMP(	1983,	ti99_232,	ti99_224,	ti99_2,		ti99_2,	ti99_2_32,	"Texas Instruments",	"TI-99/2 BASIC Computer (32kb ROMs)" )
