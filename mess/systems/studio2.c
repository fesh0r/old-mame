/******************************************************************************
 PeT mess@utanet.at May 2000

 Paul Robson's Emulator at www.classicgaming.com/studio2 made it possible
******************************************************************************/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"

#include "includes/studio2.h"

static MEMORY_READ_START( studio2_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
	{ 0x0800, 0x09ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( studio2_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
	{ 0x0800, 0x09ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START( vip_readmem )
    { 0x0000, 0x03ff, MRA_BANK1 }, // rom mapped in at reset, switched to ram with out 4
	{ 0x0400, 0x0fff, MRA_RAM },
	{ 0x8000, 0x83ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( vip_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x0400, 0x0fff, MWA_RAM },
	{ 0x8000, 0x83ff, MWA_ROM },
MEMORY_END

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( studio2 )
	PORT_START
	DIPS_HELPER( 0x002, "Player 1/Left 1", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 1/Left 2", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x008, "Player 1/Left 3", KEYCODE_3, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 1/Left 4", KEYCODE_4, KEYCODE_Q)
	DIPS_HELPER( 0x020, "Player 1/Left 5", KEYCODE_5, KEYCODE_W)
	DIPS_HELPER( 0x040, "Player 1/Left 6", KEYCODE_6, KEYCODE_E)
	DIPS_HELPER( 0x080, "Player 1/Left 7", KEYCODE_7, KEYCODE_A)
	DIPS_HELPER( 0x100, "Player 1/Left 8", KEYCODE_8, KEYCODE_S)
	DIPS_HELPER( 0x200, "Player 1/Left 9", KEYCODE_9, KEYCODE_D)
	DIPS_HELPER( 0x001, "Player 1/Left 0", KEYCODE_0, KEYCODE_X)
	PORT_START
	DIPS_HELPER( 0x002, "Player 2/Right 1", KEYCODE_1_PAD, CODE_NONE)
	DIPS_HELPER( 0x004, "Player 2/Right 2", KEYCODE_2_PAD, KEYCODE_UP)
	DIPS_HELPER( 0x008, "Player 2/Right 3", KEYCODE_3_PAD, CODE_NONE)
	DIPS_HELPER( 0x010, "Player 2/Right 4", KEYCODE_4_PAD, KEYCODE_LEFT)
	DIPS_HELPER( 0x020, "Player 2/Right 5", KEYCODE_5_PAD, CODE_NONE)
	DIPS_HELPER( 0x040, "Player 2/Right 6", KEYCODE_6_PAD, KEYCODE_RIGHT)
	DIPS_HELPER( 0x080, "Player 2/Right 7", KEYCODE_7_PAD, CODE_NONE)
	DIPS_HELPER( 0x100, "Player 2/Right 8", KEYCODE_8_PAD, KEYCODE_DOWN)
	DIPS_HELPER( 0x200, "Player 2/Right 9", KEYCODE_9_PAD, CODE_NONE)
	DIPS_HELPER( 0x001, "Player 2/Right 0", KEYCODE_0_PAD, CODE_NONE)
INPUT_PORTS_END

INPUT_PORTS_START( vip )
	PORT_START
	DIPS_HELPER( 0x0002, "1", KEYCODE_1, KEYCODE_1_PAD)
	DIPS_HELPER( 0x0004, "2", KEYCODE_2, KEYCODE_2_PAD)
	DIPS_HELPER( 0x0008, "3", KEYCODE_3, KEYCODE_3_PAD)
	DIPS_HELPER( 0x1000, "C", KEYCODE_C, CODE_NONE)
	DIPS_HELPER( 0x0010, "4", KEYCODE_4, KEYCODE_4_PAD)
	DIPS_HELPER( 0x0020, "5", KEYCODE_5, KEYCODE_5_PAD)
	DIPS_HELPER( 0x0040, "6", KEYCODE_6, KEYCODE_6_PAD)
	DIPS_HELPER( 0x2000, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x0080, "7", KEYCODE_7, KEYCODE_7_PAD)
	DIPS_HELPER( 0x0100, "8", KEYCODE_8, KEYCODE_8_PAD)
	DIPS_HELPER( 0x0200, "9", KEYCODE_9, KEYCODE_9_PAD)
	DIPS_HELPER( 0x4000, "E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x0400, "A    MR", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x0001, "0    MW", KEYCODE_0, KEYCODE_0_PAD)
	DIPS_HELPER( 0x0800, "B    TR", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x8000, "F    TW", KEYCODE_F, CODE_NONE)
	PORT_START
INPUT_PORTS_END

static struct GfxLayout studio2_charlayout =
{
        32,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        {
			0, 0, 0, 0,
			1, 1, 1, 1,
			2, 2, 2, 2,
			3, 3, 3, 3,
			4, 4, 4, 4,
			5, 5, 5, 5,
			6, 6, 6, 6,
			7, 7, 7, 7
        },
        /* y offsets */
        { 0 },
        1*8
};

static struct GfxDecodeInfo studio2_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &studio2_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static int studio2_frame_int(void)
{
	return ignore_interrupt();
}

/* studio 2
   output q speaker (300 hz tone on/off)
   f1 dma_activ
   f3 on player 2 key pressed
   f4 on player 1 key pressed
   inp 1 video on
   out 2 read key value selects keys to put at f3/f4 */

/* vip
   out 1 turns video off
   out 2 set keyboard multiplexer (bit 0..3 selects key)
   out 4 switch ram at 0000
   inp 1 turn on video
   q sound
   f1 vertical blank line (low when displaying picture
   f2 tape input (high when tone read)
   f3 keyboard in
 */

static int studio2_in_n(int n)
{
	if (n==1) studio2_video_start();
	return 0; //?
}

static UINT8 studio2_keyboard_select;

static void studio2_out_n(int data, int n)
{
	if (n==2) studio2_keyboard_select=data;
}

static void vip_out_n(int data, int n)
{
	if (n==2) studio2_keyboard_select=data;
	if (n==4) {
		cpu_setbank(1,memory_region(REGION_CPU1)+0);
	}
}

static int studio2_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<studio2_keyboard_select)) a|=4;
	if (readinputport(1)&(1<<studio2_keyboard_select)) a|=8;

	return a;
}

static int vip_in_ef(void)
{
	int a=0;
	if (studio2_get_vsync()) a|=1;

	if (readinputport(0)&(1<<studio2_keyboard_select)) a|=4;

	return a;
}

static void studio2_out_q(int level)
{
	beep_set_state(0, level);
}

static CDP1802_CONFIG studio2_config={
	studio2_video_dma,
	studio2_out_n,
	studio2_in_n,
	studio2_out_q,
	studio2_in_ef
};

static CDP1802_CONFIG vip_config={
	studio2_video_dma,
	vip_out_n,
	studio2_in_n,
	studio2_out_q,
	vip_in_ef
};

static unsigned char studio2_palette[2][3] =
{
	{ 0, 0, 0 },
	{ 255,255,255 }
};

static unsigned short studio2_colortable[1][2] = {
	{ 0, 1 },
};

static void studio2_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	memcpy (sys_palette, studio2_palette, sizeof (studio2_palette));
	memcpy(sys_colortable,studio2_colortable,sizeof(studio2_colortable));
}

static void studio2_machine_init(void)
{
	studio2_video_stop();
}

static void vip_machine_init(void)
{
	studio2_video_stop();

	cpu_setbank(1,memory_region(REGION_CPU1)+0x8000);
}

static struct beep_interface studio2_sound=
{
	1,
	{100}
};

static struct MachineDriver machine_driver_studio2 =
{
	/* basic machine hardware */
	{
		{
			CPU_CDP1802,
			1780000/8,
			studio2_readmem,studio2_writemem,0,0,
			studio2_frame_int, 1,
			0,0,
			&studio2_config
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	studio2_machine_init,
	0,//pc1401_machine_stop,

	// ntsc tv display with 15720 line frequency!
	// good width, but only 128 lines height
	64*4, 128, { 0, 64*4 - 1, 0, 128 - 1},
	studio2_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (studio2_palette) / sizeof (studio2_palette[0]) ,
	sizeof (studio2_colortable) / sizeof(studio2_colortable[0][0]),
	studio2_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    studio2_vh_start,
	studio2_vh_stop,
	studio2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		// 300 hertz beeper
        { SOUND_BEEP, &studio2_sound }
    }
};

static struct MachineDriver machine_driver_vip =
{
	/* basic machine hardware */
	{
		{
			CPU_CDP1802,
			1780000/8,
			vip_readmem,vip_writemem,0,0,
			studio2_frame_int, 1,
			0,0,
			&vip_config
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,				/* single CPU */
	vip_machine_init,
	0,//pc1401_machine_stop,

	// ntsc tv display with 15720 line frequency!
	// good width, but only 128 lines height
	64*4, 128, { 0, 64*4 - 1, 0, 128 - 1},
	studio2_gfxdecodeinfo,			   /* graphics decode info */
	sizeof (studio2_palette) / sizeof (studio2_palette[0]) ,
	sizeof (studio2_colortable) / sizeof(studio2_colortable[0][0]),
	studio2_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    studio2_vh_start,
	studio2_vh_stop,
	studio2_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		// 300 hertz beeper
        { SOUND_BEEP, &studio2_sound }
    }
};

ROM_START(studio2)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("studio2.rom", 0x0000, 0x800, 0xa494b339)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

ROM_START(vip)
	ROM_REGION(0x10000,REGION_CPU1, 0)
	ROM_LOAD("monitor.rom", 0x8000, 0x200, 0x5be0a51f)
	ROM_LOAD("chip8.rom", 0x8200, 0x200, 0x3e0f50f0)
	ROM_REGION(0x100,REGION_GFX1, 0)
ROM_END

static int studio2_load_rom(int id)
{
	FILE *cartfile;
	UINT8 *rom = memory_region(REGION_CPU1);
	int size;

	if (device_filename(IO_CARTSLOT, id) == NULL)
	{
/* A cartridge isn't strictly mandatory, but it's recommended */
		return 0;
	}

	if (!(cartfile = (FILE*)image_fopen(IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
	{
		logerror("%s not found\n",device_filename(IO_CARTSLOT,id));
		return 1;
	}
	size=osd_fsize(cartfile);

	if (osd_fread(cartfile, rom+0x400, size)!=size) {
		logerror("%s load error\n",device_filename(IO_CARTSLOT,id));
		osd_fclose(cartfile);
		return 1;
	}
	osd_fclose(cartfile);
	return 0;
}

static const struct IODevice io_studio2[] = {
	// cartridges at 0x400-0x7ff ?
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"bin\0",                        /* file extensions */
		IO_RESET_ALL,					/* reset if file changed */
		0,
		studio2_load_rom, 				/* init */
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


static const struct IODevice io_vip[] = {
	// maybe quickloader
	// tape
    { IO_END }
};

void init_studio2(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;

	beep_set_frequency(0, 300);
}

void init_vip(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;

	beep_set_frequency(0, 300);

	memory_region(REGION_CPU1)[0x8022]=0x3e; //bn3, default monitor
}


/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY   FULLNAME */
// rca cosmac elf development board (2 7segment leds, some switches/keys)
// rca cosmac elf2 16 key keyblock
CONSX( 1977, vip,		0,		vip,		vip,		vip,		"RCA",		"COSMAC VIP", GAME_NOT_WORKING )
CONS( 1976, studio2,	0,		studio2,	studio2, 	studio2,	"RCA",		"Studio II")
// hanimex mpt-02
// colour studio 2 (m1200) with little color capability

#ifdef RUNTIME_LOADER
extern void studio2_runtime_loader_init(void)
{
	int i;
	for (i=0; drivers[i]; i++) {
		if ( strcmp(drivers[i]->name,"studio2")==0) drivers[i]=&driver_studio2;
		if ( strcmp(drivers[i]->name,"vip")==0) drivers[i]=&driver_vip;
	}
}
#endif
