/***************************************************************************

  /systems/odyssey2.c

  Driver file to handle emulation of the Odyssey2.

***************************************************************************/

#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "vidhrdw/generic.h"
#include "includes/odyssey2.h"

MEMORY_READ_START( readmem )
	{ 0x0000, 0x03FF, MRA_ROM },
	{ 0x0400, 0x0bFF, MRA_BANK1 },
	{ 0x0c00, 0x0FFF, MRA_BANK2 },
MEMORY_END

MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x03FF, MWA_ROM },
	{ 0x0400, 0x0bFF, MWA_BANK1 },
	{ 0x0c00, 0x0FFF, MWA_BANK2 },
MEMORY_END

PORT_READ_START( readport )
	{ 0x00, 	0xff,	  odyssey2_bus_r},
	{ I8039_p1, I8039_p1, odyssey2_getp1 },
	{ I8039_p2, I8039_p2, odyssey2_getp2 },
	{ I8039_bus, I8039_bus, odyssey2_getbus },
	{ I8039_t1, I8039_t1, odyssey2_t1_r },
PORT_END

PORT_WRITE_START( writeport )
	{ 0x00, 	0xff,	  odyssey2_bus_w },
	{ I8039_p1, I8039_p1, odyssey2_putp1 },
	{ I8039_p2, I8039_p2, odyssey2_putp2 },
	{ I8039_bus, I8039_bus, odyssey2_putbus },
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_LOW, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( odyssey2 )
	PORT_START		/* IN0 */
	DIPS_HELPER( 0x01, "0", KEYCODE_0, CODE_NONE)
	DIPS_HELPER( 0x02, "1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x04, "2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x08, "3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x10, "4", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x20, "5", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x40, "6", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x80, "7", KEYCODE_7, CODE_NONE)
	PORT_START		/* IN1 */
	DIPS_HELPER( 0x01, "8", KEYCODE_8, CODE_NONE)
	DIPS_HELPER( 0x02, "9", KEYCODE_9, CODE_NONE)
	DIPS_HELPER( 0x04, "?? :", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x08, "?? $", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x10, "Space", KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x20, "?", KEYCODE_SLASH, CODE_NONE)
	DIPS_HELPER( 0x40, "L", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x80, "P", KEYCODE_P, CODE_NONE)
	PORT_START		/* IN2 */
	DIPS_HELPER( 0x01, "+", KEYCODE_PLUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "W", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x04, "E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x08, "R", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x10, "T", KEYCODE_T, CODE_NONE)
	DIPS_HELPER( 0x20, "U", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x40, "I", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x80, "O", KEYCODE_O, CODE_NONE)
	PORT_START		/* IN3 */
	DIPS_HELPER( 0x01, "Q", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x02, "S", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x04, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x08, "F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x10, "G", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x20, "H", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x40, "J", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x80, "K", KEYCODE_K, CODE_NONE)
	PORT_START		/* IN4 */
	DIPS_HELPER( 0x01, "A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x02, "Z", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x04, "X", KEYCODE_X, CODE_NONE)
	DIPS_HELPER( 0x08, "C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x10, "V", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x20, "B", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x40, "M", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x80, ".", KEYCODE_STOP, CODE_NONE)
	PORT_START		/* IN5 */
	DIPS_HELPER( 0x01, "-", KEYCODE_MINUS, CODE_NONE)
	DIPS_HELPER( 0x02, "*", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x04, "/", KEYCODE_SLASH_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "=", KEYCODE_EQUALS, CODE_NONE)
	DIPS_HELPER( 0x10, "Yes ", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x20, "No ", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x40, "CLR", KEYCODE_BACKSPACE, CODE_NONE)
	DIPS_HELPER( 0x80, "ENT", KEYCODE_ENTER, CODE_NONE)
	PORT_START		/* IN6 */
	DIPS_HELPER( 0x01, "Player 1/left up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/left right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/left down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/left left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 1/left fire", KEYCODE_LCONTROL, CODE_NONE)
	PORT_BIT ( 0xe0, 0xe0,	 IPT_UNUSED )
	PORT_START		/* IN7 */
	DIPS_HELPER( 0x01, "Player 2/right up", KEYCODE_HOME, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/right right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/right down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 2/right left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/right fire", KEYCODE_LALT, CODE_NONE)
	PORT_BIT ( 0xe0, 0xe0,	 IPT_UNUSED )
INPUT_PORTS_END

static struct GfxLayout odyssey2_graphicslayout =
{
        8,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 
	    0,
	    1,
	    2,
	    3,
	    4,
	    5,
	    6,
	    7,
        },
        /* y offsets */
        { 0 },
        1*8
};


static struct GfxLayout odyssey2_spritelayout =
{
        8,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 
	    7,6,5,4,3,2,1,0
        },
        /* y offsets */
        { 0 },
        1*8
};

static struct GfxDecodeInfo odyssey2_gfxdecodeinfo[] = {
    { REGION_GFX1, 0x0000, &odyssey2_graphicslayout,                     0, 2 },
    { REGION_GFX1, 0x0000, &odyssey2_spritelayout,                     0, 2 },
    { -1 } /* end of array */
};

static struct MachineDriver machine_driver_odyssey2 =
{
	/* basic machine hardware */
	{
		{
			CPU_I8048,
			1790000/5,  /* 1.79 MHz */
			readmem,writemem,readport,writeport,
			ignore_interrupt,1,
			odyssey2_line,60*262
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	odyssey2_init_machine,	/* init_machine */
	0,						/* stop_machine */

	/* video hardware */
//	262,240, {0,262-1,0,240-1},
	320,300, {0,320-1,0,300-1},
	odyssey2_gfxdecodeinfo,			   /* graphics decode info */
	ARRAY_LENGTH(odyssey2_colors),
	2,
	odyssey2_vh_init_palette,

	VIDEO_TYPE_RASTER,
	0,
	odyssey2_vh_start,
	odyssey2_vh_stop,
	odyssey2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{ 
	    { 0 }
	}
};


ROM_START (odyssey2)
    ROM_REGION(0x10000,REGION_CPU1,0)    /* safer for the memory handler/bankswitching??? */
    ROM_LOAD ("o2bios.rom", 0x0000, 0x0400, 0x8016a315)
    ROM_REGION(0x100, REGION_GFX1, 0)
    ROM_REGION(0x2000, REGION_USER1, 0)
ROM_END

void init_odyssey2(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
}

static const struct IODevice io_odyssey2[] = {
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"bin\0",            /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		NULL,				/* id */
		odyssey2_load_rom,	/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMPX( 1982, odyssey2, 0,		odyssey2, odyssey2, odyssey2,		  "Magnavox",  "ODYSSEY 2", GAME_NOT_WORKING|GAME_NO_SOUND )
// philips g7000/videopac


#ifdef RUNTIME_LOADER
extern void odyssey2_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"odyssey2")==0) drivers[i]=&driver_odyssey2;
	}
}
#endif
