/************************************************************************
 *  Mattel Intellivision + Keyboard Component Drivers
 *
 *  Frank Palazzolo
 *
 *  TBD:
 *		    Map game controllers correctly (right controller + 16 way)
 *		    Add tape support (intvkbd)
 *		    Add runtime tape loading
 *		    Fix memory system workaround
 *            (memory handler stuff in CP1610, debugger, and shared mem)
 *		    STIC
 *            collisions
 *            reenable dirty support
 *		    Cleanup
 *			  Separate stic & vidhrdw better, get rid of *2 for kbd comp
 *		    Add better runtime cart loading
 *		    Switch to tilemap system
 *
 ************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/stic.h"
#include "includes/intv.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

static unsigned char intv_palette[16][3] =
{
	{0x00, 0x00, 0x00}, /* BLACK */
	{0x00, 0x2D, 0xFF}, /* BLUE */
	{0xFF, 0x3D, 0x10}, /* RED */
	{0xC9, 0xCF, 0xAB}, /* TAN */
	{0x38, 0x6B, 0x3F}, /* DARK GREEN */
	{0x00, 0xA7, 0x56}, /* GREEN */
	{0xFA, 0xEA, 0x50}, /* YELLOW */
	{0xFF, 0xFC, 0xFF}, /* WHITE */
	{0xBD, 0xAC, 0xC8}, /* GRAY */
	{0x24, 0xB8, 0xFF}, /* CYAN */
	{0xFF, 0xB4, 0x1F}, /* ORANGE */
	{0x54, 0x6E, 0x00}, /* BROWN */
	{0xFF, 0x4E, 0x57}, /* PINK */
	{0xA4, 0x96, 0xFF}, /* LIGHT BLUE */
	{0x75, 0xCC, 0x80}, /* YELLOW GREEN */
	{0xB5, 0x1A, 0x58}  /* PURPLE */
};

static void intv_init_palette(unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	int i,j;

    memcpy(sys_palette, intv_palette, sizeof (intv_palette));
    for(i=0;i<16;i++)
    {
    	for(j=0;j<16;j++)
    	{
    		*sys_colortable++ = i;
    		*sys_colortable++ = j;
		}
	}
}

static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	3579545/2,	/* Colorburst/2 */
	{ 100 },
	{ intv_right_control_r },
	{ intv_left_control_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

/* graphics output */

struct GfxLayout intv_gromlayout =
{
	16, 16,
	256,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*16, 0*16, 1*16, 1*16, 2*16, 2*16, 3*16, 3*16,
	  4*16, 4*16, 5*16, 5*16, 6*16, 6*16, 7*16, 7*16 },
	8 * 16
};

struct GfxLayout intv_gramlayout =
{
	16, 16,
	64,
	1,
	{ 0 },
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7},
	{ 0*8, 0*8, 1*8, 1*8, 2*8, 2*8, 3*8, 3*8,
	  4*8, 4*8, 5*8, 5*8, 6*8, 6*8, 7*8, 7*8 },
	8 * 8
};

struct GfxLayout intvkbd_charlayout =
{
	8, 8,
	256,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8 * 8
};

static struct	GfxDecodeInfo intv_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ -1 }
};

static struct	GfxDecodeInfo intvkbd_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000<<1, &intv_gromlayout, 0, 256},
    { 0, 0, &intv_gramlayout, 0, 256 },    /* Dynamically decoded from RAM */
	{ REGION_GFX1, 0x0000, &intvkbd_charlayout, 0, 256},
	{ -1 }
};

INPUT_PORTS_START( intv )
	PORT_START /* IN0 */	/* Right Player Controller Starts Here */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )

	PORT_START /* IN1 */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But1", KEYCODE_LCONTROL, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But2", KEYCODE_Z, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "But3", KEYCODE_X, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "NA", KEYCODE_NONE, IP_JOY_NONE )

	PORT_START /* IN2 */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "UP", KEYCODE_UP, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PGUP", KEYCODE_PGUP, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "PGDN", KEYCODE_PGDN, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "END", KEYCODE_END, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "HOME", KEYCODE_HOME, IP_JOY_NONE )

	PORT_START /* IN3 */	/* Left Player Controller Starts Here */

	PORT_START /* IN4 */

	PORT_START /* IN5 */

INPUT_PORTS_END

/*
        Bit 7   Bit 6   Bit 5   Bit 4   Bit 3   Bit 2   Bit 1   Bit 0

 Row 0  NC      NC      NC      NC      NC      NC      CTRL    SHIFT
 Row 1  NC      NC      NC      NC      NC      NC      RPT     LOCK
 Row 2  NC      /       ,       N       V       X       NC      SPC
 Row 3  (right) .       M       B       C       Z       NC      CLS
 Row 4  (down)  ;       K       H       F       S       NC      TAB
 Row 5  ]       P       I       Y       R       W       NC      Q
 Row 6  (up)    -       9       7       5       3       NC      1
 Row 7  =       0       8       6       4       2       NC      [
 Row 8  (return)(left)  O       U       T       E       NC      ESC
 Row 9  DEL     '       L       J       G       D       NC      A
*/

INPUT_PORTS_START( intvkbd )
	PORT_START /* IN0 */	/* Keyboard Row 0 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE )
    PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE )

	PORT_START /* IN1 */	/* Keyboard Row 1 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "REPEAT", KEYCODE_RCONTROL, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "LOCK", KEYCODE_RSHIFT, IP_JOY_NONE )

	PORT_START /* IN2 */	/* Keyboard Row 2 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "NC", KEYCODE_NONE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, " ", KEYCODE_SPACE, IP_JOY_NONE )

	PORT_START /* IN3 */	/* Keyboard Row 3 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right", KEYCODE_RIGHT, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cls", KEYCODE_LALT, IP_JOY_NONE )

	PORT_START /* IN4 */	/* Keyboard Row 4 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Down", KEYCODE_DOWN, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab", KEYCODE_TAB, IP_JOY_NONE )

	PORT_START /* IN5 */	/* Keyboard Row 5 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE )

	PORT_START /* IN6 */	/* Keyboard Row 6 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Up", KEYCODE_UP, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "_", KEYCODE_MINUS, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE )

	PORT_START /* IN7 */	/* Keyboard Row 7 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "=", KEYCODE_EQUALS, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE )

	PORT_START /* IN8 */	/* Keyboard Row 8 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Return", KEYCODE_ENTER, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left", KEYCODE_LEFT, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Esc", KEYCODE_ESC, IP_JOY_NONE )

	PORT_START /* IN9 */	/* Keyboard Row 9 */
    PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BS)", KEYCODE_BACKSPACE, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "'", KEYCODE_QUOTE, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(?)", KEYCODE_NONE, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE )

	PORT_START /* IN10 */	/* For tape drive testing... */
    PORT_BITX( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_0_PAD, IP_JOY_NONE )
    PORT_BITX( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_1_PAD, IP_JOY_NONE )
    PORT_BITX( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_2_PAD, IP_JOY_NONE )
	PORT_BITX( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_3_PAD, IP_JOY_NONE )
    PORT_BITX( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_4_PAD, IP_JOY_NONE )
    PORT_BITX( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_5_PAD, IP_JOY_NONE )
    PORT_BITX( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_6_PAD, IP_JOY_NONE )
	PORT_BITX( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "", KEYCODE_7_PAD, IP_JOY_NONE )
INPUT_PORTS_END

#define MEM16(A,B,C) { A<<1, (B<<1)+1, C }
//#define MEM16M(A,B,C,D,E) { A<<1, (B<<1)+1, C, D, E }

static MEMORY_READ16_START( readmem )
	MEM16( 0x0000, 0x003f, stic_r ),
    MEM16( 0x0100, 0x01ef, intv_ram8_r ),
    MEM16( 0x01f0, 0x01ff, AY8914_directread_port_0_lsb_r ),
 	MEM16( 0x0200, 0x035f, intv_ram16_r ),
	MEM16( 0x1000, 0x1fff, MRA16_ROM ),		/* Exec ROM, 10-bits wide */
	MEM16( 0x3000, 0x37ff, MRA16_ROM ), 	/* GROM,     8-bits wide */
	MEM16( 0x3800, 0x39ff, intv_gram_r ),	/* GRAM,     8-bits wide */
	MEM16( 0x4800, 0x7fff, MRA16_ROM ),		/* Cartridges? */
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	MEM16( 0x0000, 0x003f, stic_w ),
    MEM16( 0x0100, 0x01ef, intv_ram8_w ),
    MEM16( 0x01f0, 0x01ff, AY8914_directwrite_port_0_lsb_w ),
	MEM16( 0x0200, 0x035f, intv_ram16_w ),
	MEM16( 0x1000, 0x1fff, MWA16_ROM ), 	/* Exec ROM, 10-bits wide */
	MEM16( 0x3000, 0x37ff, MWA16_ROM ),		/* GROM,     8-bits wide */
	MEM16( 0x3800, 0x39ff, intv_gram_w ),	/* GRAM,     8-bits wide */
	MEM16( 0x4800, 0x7fff, MWA16_ROM ),		/* Cartridges? */
MEMORY_END

static MEMORY_READ16_START( readmem_kbd )
	MEM16( 0x0000, 0x003f, stic_r ),
    MEM16( 0x0100, 0x01ef, intv_ram8_r ),
    MEM16( 0x01f0, 0x01ff, AY8914_directread_port_0_lsb_r ),
 	MEM16( 0x0200, 0x035f, intv_ram16_r ),
	MEM16( 0x1000, 0x1fff, MRA16_ROM ),		/* Exec ROM, 10-bits wide */
	MEM16( 0x3000, 0x37ff, MRA16_ROM ), 	/* GROM,     8-bits wide */
	MEM16( 0x3800, 0x39ff, intv_gram_r ),	/* GRAM,     8-bits wide */
	MEM16( 0x4800, 0x6fff, MRA16_ROM ),		/* Cartridges? */
	MEM16( 0x7000, 0x7fff, MRA16_ROM ),		/* Keyboard ROM */
	MEM16( 0x8000, 0xbfff, intvkbd_dualport16_r ),	/* Dual-port RAM */
MEMORY_END

static MEMORY_WRITE16_START( writemem_kbd )
	MEM16( 0x0000, 0x003f, stic_w ),
    MEM16( 0x0100, 0x01ef, intv_ram8_w ),
    MEM16( 0x01f0, 0x01ff, AY8914_directwrite_port_0_lsb_w ),
	MEM16( 0x0200, 0x035f, intv_ram16_w ),
	MEM16( 0x1000, 0x1fff, MWA16_ROM ), 	/* Exec ROM, 10-bits wide */
	MEM16( 0x3000, 0x37ff, MWA16_ROM ),		/* GROM,     8-bits wide */
	MEM16( 0x3800, 0x39ff, intv_gram_w ),	/* GRAM,     8-bits wide */
	MEM16( 0x4800, 0x6fff, MWA16_ROM ),		/* Cartridges? */
	MEM16( 0x7000, 0x7fff, MWA16_ROM ),		/* Keyboard ROM */
	MEM16( 0x8000, 0xbfff, intvkbd_dualport16_w ),	/* Dual-port RAM */
MEMORY_END

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x3fff, intvkbd_dualport8_lsb_r }, /* Dual-port RAM */
	{ 0x4000, 0x7fff, intvkbd_dualport8_msb_r }, /* Dual-port RAM */
	{ 0xb7f8, 0xb7ff, MRA_RAM }, /* ??? */
	{ 0xb800, 0xbfff, &videoram_r }, /* Text Display */
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x3fff, intvkbd_dualport8_lsb_w }, /* Dual-port RAM */
	{ 0x4000, 0x7fff, intvkbd_dualport8_msb_w }, /* Dual-port RAM */
	{ 0xb7f8, 0xb7ff, MWA_RAM }, /* ??? */
	{ 0xb800, 0xbfff, &videoram_w }, /* Text Display */
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END

static struct MachineDriver machine_driver_intv =
{
	/* basic machine hardware */
	{
		/* Main CPU (in Master System) */
		{
			CPU_CP1600,
			3579545/4,  /* Colorburst/4 */
			readmem,writemem,0,0,
			intv_interrupt,1
		}
	},
	/* frames per second, VBL duration */
	59.92, DEFAULT_60HZ_VBLANK_DURATION,
	1,						/* slices per frame */
	intv_machine_init,		/* init machine */
	NULL,					/* stop machine */

	/* video hardware */
	40*8, 24*8, { 0, 40*8-1, 0, 24*8-1},
	intv_gfxdecodeinfo,
	sizeof (intv_palette) / sizeof (intv_palette[0]) ,
	2 * 16 * 16,
	intv_init_palette,					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */

	intv_vh_start,
	intv_vh_stop,
	intv_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static struct MachineDriver machine_driver_intvkbd =
{
	/* basic machine hardware */
	{
		/* Main CPU (in Master System) */
		{
			CPU_CP1600,
			3579545/4,  /* Colorburst/4 */
			readmem_kbd,writemem_kbd,0,0,
			intv_interrupt,1
		},
		/* Slave CPU - runs tape drive, text display */
		{
			CPU_M6502,
			3579545/2,	/* Colorburst/2 */
			readmem2,writemem2,0,0,
			interrupt,1
        }
	},
	/* frames per second, VBL duration */
	59.92, DEFAULT_60HZ_VBLANK_DURATION,
	100,						/* slices per frame */
	intv_machine_init,		/* init machine */
	NULL,					/* stop machine */

	/* video hardware */
	40*8, 24*8, { 0, 40*8-1, 0, 24*8-1},
	intvkbd_gfxdecodeinfo,
	sizeof (intv_palette) / sizeof (intv_palette[0]) ,
	2 * 16 * 16,
	intv_init_palette,					/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,	/* video flags */
	0,						/* obsolete */

	intvkbd_vh_start,
	intvkbd_vh_stop,
	intvkbd_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

ROM_START(intv)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, 0xcbce86f7 )
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, 0x683a4158 )
ROM_END

ROM_START(intvsrs)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "searsexc.bin", 0x1000<<1, 0x2000, 0xea552a22 )
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, 0x683a4158 )
ROM_END

ROM_START(intvkbd)
	ROM_REGION(0x10000<<1,REGION_CPU1,0)
		ROM_LOAD16_WORD( "exec.bin", 0x1000<<1, 0x2000, 0xcbce86f7 )
		ROM_LOAD16_BYTE( "grom.bin", (0x3000<<1)+1, 0x0800, 0x683a4158 )
		ROM_LOAD16_WORD( "024.u60", 0x7000<<1, 0x1000, 0x4f7998ec )
		ROM_LOAD16_BYTE( "4d72.u62", 0x7800<<1, 0x0800, 0xaa57c594 )
		ROM_LOAD16_BYTE( "4d71.u63", (0x7800<<1)+1, 0x0800, 0x069b2f0b )

	ROM_REGION(0x10000,REGION_CPU2,0)
		ROM_LOAD( "0104.u20",  0xc000, 0x1000, 0x5c6f1256)
		ROM_RELOAD( 0xe000, 0x1000 )
		ROM_LOAD("cpu2d.u21",  0xd000, 0x1000, 0x2c2dba33)
		ROM_RELOAD( 0xf000, 0x1000 )

	ROM_REGION(0x00800,REGION_GFX1,0)
		ROM_LOAD( "4c52.u34",  0x0000, 0x0800, 0xcbeb2e96)

ROM_END

static const struct IODevice io_intv[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"rom\0",            /* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		0,
		intv_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
    { IO_END }
};

static const struct IODevice io_intvsrs[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"rom\0",            /* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		0,
		intv_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
    { IO_END }
};

static const struct IODevice io_intvkbd[] = {
	{
		IO_CARTSLOT,		/* type */
		2,					/* count */
		"rom\0bin\0",       /* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		0,
		intvkbd_load_rom,	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
	{
		IO_CASSETTE,		/* type */	/* Actually a tape drive! */
		1,					/* count */
		"tap\0",       		/* file extensions */
		IO_RESET_CPU,		/* reset if file changed */
		0,
		NULL,				/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,               /* open */
		NULL,               /* close */
		NULL,               /* status */
		NULL,               /* seek */
		NULL,				/* tell */
        NULL,               /* input */
		NULL,               /* output */
		NULL,               /* input_chunk */
		NULL,				/* output_chunk */
		NULL				/* correct CRC */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY      FULLNAME */
CONSX( 1979, intv,     0,		intv,     intv, 	intv,	  "Mattel",    "Intellivision", GAME_NOT_WORKING )
CONSX( 19??, intvsrs,  0,		intv,     intv, 	intv,	  "Mattel",    "Intellivision (Sears)", GAME_NOT_WORKING )
COMPX( 1981, intvkbd,  0,		intvkbd,  intvkbd, 	intvkbd,  "Mattel",    "Intellivision Keyboard Component (Unreleased)", GAME_NOT_WORKING)
