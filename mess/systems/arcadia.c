/******************************************************************************
 PeT mess@utanet.at May 2001

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include <assert.h>
#include "driver.h"
#include "cpu/s2650/s2650.h"

#include "includes/arcadia.h"

static MEMORY_READ_START( arcadia_readmem )
{ 0x0000, 0x0fff, MRA_ROM },
{ 0x1800, 0x1aff, arcadia_video_r },
{ 0x2000, 0x2fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( arcadia_writemem )
//	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x1800, 0x1aff, arcadia_video_w },
//	{ 0x2000, 0x2fff, MWA_ROM },
MEMORY_END

static PORT_READ_START( arcadia_readport )
//{ S2650_CTRL_PORT,S2650_CTRL_PORT, },
//{ S2650_DATA_PORT,S2650_DATA_PORT, },
{ S2650_SENSE_PORT,S2650_SENSE_PORT, arcadia_vsync_r},
PORT_END

static PORT_WRITE_START( arcadia_writeport )
PORT_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( arcadia )
	PORT_START
	DIPS_HELPER( 0x01, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x02, "Option", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x04, DEF_STR(Difficulty), KEYCODE_F5, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x02, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x01, "Player 1/Left Clear", KEYCODE_BACKSPACE, KEYCODE_Z)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x04, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x02, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x01, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x02, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x01, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_C)
	PORT_START
	PORT_BIT ( 0xff, 0xf0,	 IPT_UNUSED ) // used in palladium
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x04, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xff, 0xf0,	 IPT_UNUSED ) // used in palladium
#if 0
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER,1,2000,0,0x1f,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER,1,2000,0,0x1f,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_DEL,KEYCODE_PGDN,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_HOME,KEYCODE_END,JOYCODE_2_UP,JOYCODE_2_DOWN)
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

INPUT_PORTS_START( vcg )
	PORT_START
	DIPS_HELPER( 0x01, "Start", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x02, "Selector A", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x04, "Selector B", KEYCODE_F5, CODE_NONE)
	PORT_START
PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED ) // some bits must be high
	DIPS_HELPER( 0x08, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x02, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x01, "Player 1/Left Clear", KEYCODE_BACKSPACE, KEYCODE_Z)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 2/Button", KEYCODE_2, KEYCODE_LCONTROL)
	DIPS_HELPER( 0x04, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x02, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x01, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x02, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x01, "Player 1/Left Enter", KEYCODE_ENTER, KEYCODE_C)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x01, "Player 1/Left x1", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/Left x2", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/Left x3", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/Left x4", KEYCODE_G, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 4", KEYCODE_4_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right Clear", KEYCODE_DEL_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 2/Button", KEYCODE_2_PAD, KEYCODE_LALT)
	DIPS_HELPER( 0x04, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 8", KEYCODE_8_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x08, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right 6", KEYCODE_6_PAD, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x01, "Player 2/Right ENTER", KEYCODE_ENTER_PAD, CODE_NONE)
	PORT_START
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	DIPS_HELPER( 0x01, "Player 2/Right x1", KEYCODE_ASTERISK, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 2/Right x2", KEYCODE_SLASH_PAD, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 2/Right x3", KEYCODE_MINUS_PAD, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 2/Right x4", KEYCODE_PLUS_PAD, CODE_NONE)
#if 0
    // shit, auto centering too slow, so only using 5 bits, and scaling at videoside
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER,1,2000,0,0x1f,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER,1,2000,0,0x1f,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_X|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_DEL,KEYCODE_PGDN,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)
    PORT_START
PORT_ANALOGX(0x1ff,0x10,IPT_AD_STICK_Y|IPF_CENTER|IPF_PLAYER2,100,10,0,0x1f,KEYCODE_HOME,KEYCODE_END,JOYCODE_2_UP,JOYCODE_2_DOWN)
#else
	PORT_START
	DIPS_HELPER( 0x01, "Player 1/left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x02, "Player 1/right", KEYCODE_RIGHT, CODE_NONE)
	DIPS_HELPER( 0x04, "Player 1/down", KEYCODE_DOWN, CODE_NONE)
	DIPS_HELPER( 0x08, "Player 1/up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x10, "Player 2/left", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x20, "Player 2/right", KEYCODE_PGDN, CODE_NONE)
	DIPS_HELPER( 0x40, "Player 2/down", KEYCODE_END, CODE_NONE)
	DIPS_HELPER( 0x80, "Player 2/up", KEYCODE_HOME, CODE_NONE)
#endif
INPUT_PORTS_END

static struct GfxLayout arcadia_charlayout =
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

static struct GfxDecodeInfo arcadia_gfxdecodeinfo[] = {
    { REGION_GFX1, 0x0000, &arcadia_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static int arcadia_frame_int(void)
{
	return 0;
}

static unsigned char arcadia_palette[8][3] =
{
    { 255, 255, 255 }, // white
    { 255, 255, 0 }, // yellow
    { 0, 255, 255 }, // cyan
    { 0, 255, 0 }, // green
    { 255, 0, 255 }, // magenta
    { 255, 0, 0 }, // red
    { 0, 0, 255, }, // blue
    { 0, 0, 0 } // black
};

static unsigned short arcadia_colortable[1][2] = {
	{ 0, 1 },
};

static void arcadia_init_colors (unsigned char *sys_palette,
				 unsigned short *sys_colortable,
				 const unsigned char *color_prom)
{
    memcpy (sys_palette, arcadia_palette, sizeof (arcadia_palette));
    memcpy(sys_colortable, arcadia_colortable,sizeof(arcadia_colortable));
}

static void arcadia_machine_init(void)
{
}

static struct MachineDriver machine_driver_arcadia =
{
	/* basic machine hardware */
	{
		{
			CPU_S2650,
			3580000/3,
			arcadia_readmem,arcadia_writemem,
			arcadia_readport,arcadia_writeport,
			arcadia_frame_int, 1,
			arcadia_video_line,262*60,
			NULL
		}
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	arcadia_machine_init,
	0,//pc1401_machine_stop,
#if arcadia_DEBUG
	128+2*XPOS+40, 262, { 0, 2*XPOS+128-1+40, 0, 262-1},
#else
	128+2*XPOS, 262, { 0, 2*XPOS+128-1, 0, 262-1},
#endif
	arcadia_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (arcadia_palette) / sizeof (arcadia_palette[0]) ,
	sizeof (arcadia_colortable) / sizeof(arcadia_colortable[0][0]),
	arcadia_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
	arcadia_vh_start,
	arcadia_vh_stop,
	arcadia_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
	    {SOUND_CUSTOM, &arcadia_sound_interface},
	}
};

ROM_START(arcadia)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vcg)
	ROM_REGION(0x8000,REGION_CPU1, 0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

static int arcadia_init_cart(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
		printf("%s requires Cartridge!\n", Machine->gamedrv->name);
		return INIT_FAIL;
	}

	memset(rom, 0, 0x8000);
	if (!(cartfile = (FILE*)image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
	{
		logerror("%s not found\n",device_filename(IO_CARTSLOT,id));
		return INIT_FAIL;
	}
	size=osd_fsize(cartfile);

	if (osd_fread(cartfile, rom, size)!=size) {
		logerror("%s load error\n",device_filename(IO_CARTSLOT,id));
		osd_fclose(cartfile);
		return INIT_FAIL;
	}
	osd_fclose(cartfile);
	if (size>0x1000) memmove(rom+0x2000, rom+0x1000, size-0x1000);
#if 1
	// golf cartridge support
	// 4kbyte at 0x0000
	// 2kbyte at 0x4000
	if (size<=0x2000) memcpy (rom+0x3000, rom+0x2000, 0x1000);
	if (size<=0x3000) memcpy (rom+0x4000, rom+0x2000, 0x2000);
#else
	/* this is a testpatch for the golf cartridge
	   so it could be burned in a arcadia 2001 cartridge
	   activate it and use debugger to save patched version */
	// not enough yet (some pointers stored as data?)
	struct { UINT16 address; UINT8 old; UINT8 neu; }
	patch[]= {
	    { 0x0077,0x40,0x20 },
	    { 0x011e,0x40,0x20 },
	    { 0x0348,0x40,0x20 },
	    { 0x03be,0x40,0x20 },
	    { 0x04ce,0x40,0x20 },
	    { 0x04da,0x40,0x20 },
	    { 0x0617,0x40,0x20 },
	    { 0x0efb,0x40,0x20 },
	    { 0x0f00,0x40,0x20 },
	    { 0x0f12,0x40,0x20 }
	};
	for (int i=0; i<ARRAY_LENGTH(patch); i++) {
	    assert(rom[patch[i].address]==patch[i].old);
	    rom[patch[i].address]=patch[i].neu;
	}
#endif
	return INIT_PASS;
}

static const struct IODevice io_arcadia[] = {
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"bin\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		0,
		arcadia_init_cart, 				/* init */
		NULL,							/* exit */
		NULL,							/* info */
		NULL,							/* open */
		NULL,							/* close */
		NULL,							/* status */
		NULL,							/* seek */
		NULL,							/* tell */
		NULL,							/* input */
		NULL,							/* output */
		NULL,							/* input_chunk */
		NULL							/* output_chunk */
	},
    { IO_END }
};

#define io_vcg io_arcadia

void init_arcadia(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
#if 0
	// this is here to allow developement of some simple testroutines
	// for a real console
	{
	    UINT8 *rom=memory_region(REGION_CPU1);
	    /* this is a simple routine to display all rom characters
	       on the display for a snapshot */
	    UINT8 prog[]={ // address 0 of course
		0x20, // eorz, 0
		0x1b, 0x01, // bctr,a $0004
		0x17, // retc a
		0x76, 0x20, // ppsu ii

		// fill screen
		0x04, 0x00, // lodi,0 0
		0x04|1, 0x00, // lodi,1 0
		0xcc|1, 0x78, 0x10, //a: stra,0 $1800,r1
		0x75,9, //cpsl wc|c
		0x84,0x41, // addi,0 0x41
		0x75,9, //cpsl wc|c
		0x84|1, 0x01, // addi,1 1
		0xe4|1, 0x40, // comi,1 40
		0x98, 0x80-15, // bcfr,0 a

		0x04, 0xff, // lodi,0 7
		0xcc, 0x18, 0xfc, // stra,0 $19f8
		0x04, 0x00, // lodi,0 7
		0xcc, 0x18, 0xfd, // stra,0 $18fd
		0x04, 0x07, // lodi,0 7
		0xcc, 0x19, 0xf9, // stra,0 $19f9

		0x04, 0x00, // lodi,0 7
		0xcc, 0x19, 0xbe, // stra,0 $19bf
		0x04, 0x00, // lodi,0 7
		0xcc, 0x19, 0xbf, // stra,0 $19bf

		//loop: 0x0021
		// print keyboards
		0x04|1, 0x00, //y:lodi,1 0
		0x0c|1, 0x79, 0x00, //x: ldra,0 1900,r1
		0x44|0, 0x0f, //andi,0 0f
		0x64|0, 0x10, //ori,0  10
		0xcc|1, 0x78, 0x01, //stra,0 1840,r1
		0x75,9, //cpsl wc|c
		0x84|1, 0x01, //addi,1 1
		0xe4|1, 0x09, //comi,1 9
		0x98, 0x80-18, //bcfr,0 x

		// cycle colors
		0x0c|1, 0x19, 0x00, //ldra,1 1900
		0x44|1, 0xf, //andi,0 0f
		0xe4|1, 1, //comi,1 1
		0x98, +10, //bcfr,0 c
		0x0c, 0x19, 0xbf,//ldra,0 19f9,0
		0x84, 1, //addi,0 1
		0xcc, 0x19, 0xbf, //stra,0 19f9,0
		0x18|3, 12, // bctr,a
		0xe4|1, 2, //c:comi,1 2
		0x98, +10, //bcfr,0 d
		0x0c, 0x19, 0xbf, //ldra,0 19f9,0
		0x84, 8, //addi,0 8
		0xcc, 0x19, 0xbf, //stra,0 19f9,0
		0x18|3, 12, // bctr,a

		// cycle colors
		0xe4|1, 4, //comi,1 4
		0x98, +10, //bcfr,0 c
		0x0c, 0x19, 0xbe,//ldra,0 19f9,0
		0x84, 1, //addi,0 1
		0xcc, 0x19, 0xbe, //stra,0 19f9,0
		0x18|3, 12, // bctr,a
		0xe4|1, 8, //c:comi,1 2
		0x98, +8+9, //bcfr,0 d
		0x0c, 0x19, 0xbe, //ldra,0 19f9,0
		0x84, 8, //addi,0 8
		0xcc, 0x19, 0xbe, //stra,0 19f9,0

		0x0c, 0x19, 0x00, //b: ldra,0 1900
		0x44|0, 0xf, //andi,0 0f
		0xe4, 0, //comi,0 0
		0x98, 0x80-9, //bcfr,0 b

		0x0c, 0x19, 0xbe, //ldra,0 19bf
		0xcc, 0x19, 0xf8, //stra,0 19f8
		0x0c, 0x19, 0xbf, //ldra,0 19bf
		0xcc, 0x19, 0xf9, //stra,0 19f8

		0x0c, 0x19, 0xbe, //ldra,0 17ff
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0d, //stra,0 180f
		0x0c, 0x19, 0xbe, //x: ldra,0 19bf
		0x50, 0x50, 0x50, 0x50, //shr,0 4
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0c, //stra,0 180e

		0x0c, 0x19, 0xbf, //ldra,0 17ff
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0f, //stra,0 180f
		0x0c, 0x19, 0xbf, //x: ldra,0 19bf
		0x50, 0x50, 0x50, 0x50, //shr,0 4
		0x44|0, 0xf, //andi,0 7
		0x64|0, 0x10, //ori,0  10
		0xcc, 0x18, 0x0e, //stra,0 180e

		0x0c, 0x18, 0x00, //ldra,0 1800
		0x84, 1, //addi,0 1
		0xcc, 0x18, 0x00, //stra,0 1800

//		0x1b, 0x80-20-29-26-9-8-2 // bctr,a y
		0x1c|3, 0, 0x32, // bcta,3 loop

		// calling too many subdirectories causes cpu to reset!
		// bxa causes trap
	    };
#if 1
	    FILE *f;
	    f=fopen("chartest.bin","wb");
	    fwrite(prog, ARRAY_LENGTH(prog), sizeof(prog[0]), f);
	    fclose(f);
#endif
	    for (i=0; i<ARRAY_LENGTH(prog); i++) rom[i]=prog[i];

	}
#endif
}

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
// marketed from several firms/names

CONSX( 1982, arcadia,	0,	arcadia,  arcadia,  arcadia,		"Emerson",		"Arcadia 2001", GAME_IMPERFECT_SOUND )
// schmid tvg 2000 (developer? PAL)

// different cartridge connector
// hanimex mpt 03 model

// different cartridge connector (same size as mpt03, but different pinout!)
// 16 keys instead of 12
CONSX( 198?, vcg,	arcadia,arcadia,  vcg,  arcadia,		"Palladium",		"VIDEO - COMPUTER - GAME", GAME_IMPERFECT_SOUND )


#ifdef RUNTIME_LOADER
extern void arcadia_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"arcadia")==0) drivers[i]=&driver_arcadia;
		if ( strcmp(drivers[i]->name,"vcg")==0) drivers[i]=&driver_vcg;
	}
}
#endif
