/***************************************************************************

  snes.c

  Driver file to handle emulation of the Nintendo Super NES.

  Anthony Kruize
  Based on the original MESS driver by Lee Hammerton (aka Savoury Snax)

  Driver is preliminary right now.
  Sound emulation currently consists of the SPC700 and that's about it. Without
  the DSP being emulated, there's no sound even if the code is being executed.
  I need to figure out how to get the 65816 and the SPC700 to stay in sync.

  The memory map included below is setup in a way to make it easier to handle
  Mode 20 and Mode 21 ROMs.

  Todo (in no particular order):
    - Emulate extra chips - superfx, dsp2, sa-1 etc.
    - Add sound emulation. Currently the SPC700 is emulated, but that's it.
    - Add horizontal mosaic, hi-res. interlaced etc to video emulation.
    - Add support for fullgraphic mode(partially done).
    - Fix support for Mode 7. (In Progress)
    - Handle interleaved roms (maybe even multi-part roms, but how?)
    - Add support for running at 3.58Mhz at the appropriate time.
    - I'm sure there's lots more ...

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/snes.h"
#include "devices/cartslot.h"
#include "inputx.h"

static ADDRESS_MAP_START( snes_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x000000, 0x2fffff) AM_READWRITE(snes_r_bank1, snes_w_bank1)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x300000, 0x3fffff) AM_READWRITE(snes_r_bank2, snes_w_bank2)	/* I/O and ROM (repeats for each bank) */
	AM_RANGE(0x400000, 0x5fffff) AM_READWRITE(snes_r_bank3, MWA8_ROM)		/* ROM (and reserved in Mode 20) */
	AM_RANGE(0x600000, 0x6fffff) AM_NOP										/* Reserved */
	AM_RANGE(0x700000, 0x77ffff) AM_READWRITE(snes_r_sram, MWA8_RAM)			/* 256KB Mode 20 save ram + reserved from 0x8000 - 0xffff */
	AM_RANGE(0x780000, 0x7dffff) AM_NOP										/* Reserved */
	AM_RANGE(0x7e0000, 0x7fffff) AM_RAM										/* 8KB Low RAM, 24KB High RAM, 96KB Expanded RAM */
	AM_RANGE(0x800000, 0xffffff) AM_READWRITE(snes_r_bank4, snes_w_bank4)	/* Mirror and ROM */
ADDRESS_MAP_END



static ADDRESS_MAP_START( spc_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ef) AM_RAM									/* lower 32k ram */
	AM_RANGE(0x00f0, 0x00ff) AM_READWRITE(spc_io_r, spc_io_w)		/* spc io */
	AM_RANGE(0x0100, 0x7fff) AM_RAM									/* lower 32k ram continued */
	AM_RANGE(0x8000, 0xffbf) AM_RAM									/* upper 32k ram */
	AM_RANGE(0xffc0, 0xffff) AM_READWRITE(spc_bank_r, spc_bank_w)	/* upper 32k ram continued or Initial Program Loader ROM */
ADDRESS_MAP_END



INPUT_PORTS_START( snes )
	PORT_START  /* IN 0 : Joypad 1 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("P1 Button A")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("P1 Button X")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("P1 Button L")  PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("P1 Button R")  PORT_PLAYER(1)
	PORT_START  /* IN 1 : Joypad 1 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P1 Button B")  PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P1 Button Y")  PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1)

	PORT_START  /* IN 2 : Joypad 2 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("P2 Button A")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("P2 Button X")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("P2 Button L")  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("P2 Button R")  PORT_PLAYER(2)
	PORT_START  /* IN 3 : Joypad 2 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P2 Button B")  PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P2 Button Y")  PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2)

	PORT_START  /* IN 4 : Joypad 3 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("P3 Button A")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("P3 Button X")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("P3 Button L")  PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("P3 Button R")  PORT_PLAYER(3)
	PORT_START  /* IN 5 : Joypad 3 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P3 Button B")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P3 Button Y")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT) PORT_PLAYER(3)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START) PORT_PLAYER(3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(3)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(3)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(3)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(3)

	PORT_START  /* IN 6 : Joypad 4 - L */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("P4 Button A")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON4) PORT_NAME("P4 Button X")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON5) PORT_NAME("P4 Button L")  PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON6) PORT_NAME("P4 Button R")  PORT_PLAYER(4)
	PORT_START  /* IN 7 : Joypad 4 - H */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("P4 Button B")  PORT_PLAYER(4)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("P4 Button Y")  PORT_PLAYER(4)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_SELECT) PORT_PLAYER(4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START) PORT_PLAYER(4)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP) PORT_PLAYER(4)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN) PORT_PLAYER(4)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT) PORT_PLAYER(4)
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT) PORT_PLAYER(4)

	PORT_START	/* IN 8 : Configuration */
	PORT_CONFNAME( 0x1, 0x1, "Enforce 32 sprites/line" )
	PORT_CONFSETTING(   0x0, DEF_STR( No )  )
	PORT_CONFSETTING(   0x1, DEF_STR( Yes ) )

#ifdef MAME_DEBUG
	PORT_START	/* IN 9 : debug switches */
	PORT_DIPNAME( 0x3, 0x0, "Browse tiles" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x1, "2bpl"  )
	PORT_DIPSETTING(   0x2, "4bpl"  )
	PORT_DIPSETTING(   0x3, "8bpl"  )
	PORT_DIPNAME( 0xc, 0x0, "Browse maps" )
	PORT_DIPSETTING(   0x0, DEF_STR( Off ) )
	PORT_DIPSETTING(   0x4, "2bpl"  )
	PORT_DIPSETTING(   0x8, "4bpl"  )
	PORT_DIPSETTING(   0xc, "8bpl"  )

	PORT_START	/* IN 10 : debug switches */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Toggle BG 1")  PORT_PLAYER(2)
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("Toggle BG 2")  PORT_PLAYER(2)
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("Toggle BG 3")  PORT_PLAYER(2)
	PORT_BIT( 0x8, IP_ACTIVE_HIGH, IPT_BUTTON10) PORT_NAME("Toggle BG 4")  PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Toggle Objects")  PORT_PLAYER(3)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON8) PORT_NAME("Toggle Main/Sub")  PORT_PLAYER(3)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("Toggle Back col")  PORT_PLAYER(3)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON10) PORT_NAME("Toggle Windows")  PORT_PLAYER(3)

	PORT_START	/* IN 11 : debug input */
	PORT_BIT( 0x1, IP_ACTIVE_HIGH, IPT_BUTTON9) PORT_NAME("Pal prev") 
	PORT_BIT( 0x2, IP_ACTIVE_HIGH, IPT_BUTTON10) PORT_NAME("Pal next") 
	PORT_BIT( 0x4, IP_ACTIVE_HIGH, IPT_BUTTON7) PORT_NAME("Toggle Transparency")  PORT_PLAYER(4)
#endif
INPUT_PORTS_END

static struct CustomSound_interface snes_sound_interface =
{ snes_sh_start, 0, 0 };

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

static PALETTE_INIT( snes )
{
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1F) << 3;
		g = ((i >> 5) & 0x1F) << 3;
		b = ((i >> 10) & 0x1F) << 3;
		palette_set_color( i, r, g, b );
	}

	/* The colortable can be black */
	for( i = 0; i < 256; i++ )
		colortable[i] = 0;
}



/* Loads the battery backed RAM into the appropriate memory area */
static void snes_load_sram(void)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = (UINT8 *)malloc( snes_cart.sram_max );
	ptr = battery_ram;
	image_battery_load( image_from_devtype_and_index(IO_CARTSLOT,0), battery_ram, snes_cart.sram_max );

	if( snes_cart.mode == SNES_MODE_20 )
	{
		for( ii = 0; ii < 8; ii++ )
		{
			memcpy( &snes_ram[0x700000 + (ii * 0x010000)], ptr, 0x7fff );
			ptr += 0x7fff;
		}
	}
	else
	{
		for( ii = 0; ii < 16; ii++ )
		{
			memcpy( &snes_ram[0x306000 + (ii * 0x010000)], ptr, 0x1fff );
			ptr += 0x1fff;
		}
	}

	free( battery_ram );
}

/* Saves the battery backed RAM from the appropriate memory area */
static void snes_save_sram(void)
{
	UINT8 ii;
	UINT8 *battery_ram, *ptr;

	battery_ram = (UINT8 *)malloc( snes_cart.sram_max );
	ptr = battery_ram;

	if( snes_cart.mode == SNES_MODE_20 )
	{
		for( ii = 0; ii < 8; ii++ )
		{
			memcpy( ptr, &snes_ram[0x700000 + (ii * 0x010000)], 0x7fff );
			ptr += 0x7fff;
		}
	}
	else
	{
		for( ii = 0; ii < 16; ii++ )
		{
			memcpy( ptr, &snes_ram[0x306000 + (ii * 0x010000)], 0x1fff );
			ptr += 0x1fff;
		}
	}

	image_battery_save( image_from_devtype_and_index(IO_CARTSLOT,0), battery_ram, snes_cart.sram_max );

	free( battery_ram );
}



static MACHINE_STOP( snes )
{
	/* Save SRAM */
	if( snes_cart.sram > 0 )
		snes_save_sram();
}



static DEVICE_LOAD(snes_cart)
{
	int i;
	UINT16 totalblocks, readblocks;
	UINT32 offset;
	UINT8 header[512], sample[0xffff];
	UINT8 valid_mode20, valid_mode21;

	/* Cart types */
	static struct
	{
		INT16 Code;
		const char *Name;
	} CartTypes[] =
	{
		{  0, "ROM"             },
		{  1, "ROM,RAM"         },
		{  2, "ROM,SRAM"        },
		{  3, "ROM,DSP1"        },
		{  4, "ROM,RAM,DSP1"    },
		{  5, "ROM,SRAM,DSP1"   },
		{ 19, "ROM,SuperFX"     },
		{ 21, "ROM,SRAM,SuperFX"},
		{ 69, "ROM,SRAM,S-DD1"  },
		{227, "ROM,Z80GB"       },
		{243, "ROM,?(1)"        },
		{246, "ROM,DSP2"        },
		{ -1, "UNKNOWN"         }
	};

	/* Some known countries */
	const char *countries[] =
	{
		"Japan (NTSC)",
		"USA (NTSC)",
		"Australia, Europe, Oceania & Asia (PAL)",
		"Sweden (PAL)",
		"Finland (PAL)",
		"Denmark (PAL)",
		"France (PAL)",
		"Holland (PAL)",
		"Spain (PAL)",
		"Germany, Austria & Switzerland (PAL)",
		"Italy (PAL)",
		"Hong Kong & China (PAL)",
		"Indonesia (PAL)",
		"Korea (PAL)",
		"UNKNOWN"
	};

	if( new_memory_region(REGION_CPU1, 0x1000000,0) )
	{
		logerror("Memory allocation failed reading rom!\n");
		return INIT_FAIL;
	}

	snes_ram = memory_region( REGION_CPU1 );
	memset( snes_ram, 0, 0x1000000 );

	/* Check for a header (512 bytes) */
	offset = 512;
	mame_fread( file, header, 512 );
	if( (header[8] == 0xaa) && (header[9] == 0xbb) && (header[10] == 0x04) )
	{
		/* Found an SWC identifier */
		logerror( "Found header(SWC) - Skipped\n" );
	}
	else if( (header[0] | (header[1] << 8)) == (((mame_fsize(file) - 512) / 1024) / 8) )
	{
		/* Some headers have the rom size at the start, if this matches with the
		 * actual rom size, we probably have a header */
		logerror( "Found header(size) - Skipped\n" );
	}
	else if( (image_length(image) % 0x8000) == 512 )
	{
		/* As a last check we'll see if there's exactly 512 bytes extra to this
		 * image. */
		logerror( "Found header(extra) - Skipped\n" );
	}
	else
	{
		/* No header found so go back to the start of the file */
		logerror( "No header found.\n" );
		offset = 0;
		mame_fseek( file, offset, SEEK_SET );
	}

	/* We need to take a sample of 128kb to test what mode we need to be in */
	mame_fread( file, sample, 0xffff );
	mame_fseek( file, offset, SEEK_SET );	/* Rewind */
	/* Now to determine if this is a lo-ROM or a hi-ROM */
	valid_mode20 = snes_validate_infoblock( sample, 0x7fc0 );
	valid_mode21 = snes_validate_infoblock( sample, 0xffc0 );
	if( valid_mode20 >= valid_mode21 )
	{
		snes_cart.mode = SNES_MODE_20;
		snes_cart.sram_max = 0x40000;
	}
	else if( valid_mode21 > valid_mode20 )
	{
		snes_cart.mode = SNES_MODE_21;
		snes_cart.sram_max = 0x20000;
	}

	/* Find the number of blocks in this ROM */
	totalblocks = ((mame_fsize(file) - offset) >> (snes_cart.mode == SNES_MODE_20 ? 15 : 16));

	/* FIXME: Insert crc check here */

	readblocks = 0;
	if( snes_cart.mode == SNES_MODE_20 )
	{
		/* In mode 20, all blocks are 32kb. There are upto 96 blocks, giving a
		 * total of 24mbit(3mb) of ROM.
		 * The first 48 blocks are located in banks 0x00 to 0x2f at address
		 * 0x8000.  They are mirrored in banks 0x80 to 0xaf.
		 * The next 16 blocks are located in banks 0x30 to 0x3f at address
		 * 0x8000.  They are mirrored in banks 0xb0 to 0xbf.
		 * The final 32 blocks are located in banks 0x40 - 0x5f at address
		 * 0x8000.  They are mirrored in banks 0xc0 to 0xdf.
		 */
		i = 0;
		while( i < 96 && readblocks <= totalblocks )
		{
			mame_fread( file, &snes_ram[(i++ * 0x10000) + 0x8000], 0x8000);
			readblocks++;
		}
	}
	else	/* Mode 21 */
	{
		/* In mode 21, all blocks are 64kb. There are upto 96 blocks, giving a
		 * total of 48mbit(6mb) of ROM.
		 * The first 64 blocks are located in banks 0xc0 to 0xff. The MSB of
		 * each bank is mirrored in banks 0x00 to 0x3f.
		 * The final 32 blocks are located in banks 0x40 to 0x5f.
		 */

		/* read first 64 blocks */
		i = 0;
		while( i < 64 && readblocks <= totalblocks )
		{
			mame_fread( file, &snes_ram[0xc00000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
		/* read the next 32 blocks */
		i = 0;
		while( i < 32 && readblocks <= totalblocks )
		{
			mame_fread( file, &snes_ram[0x400000 + (i++ * 0x10000)], 0x10000);
			readblocks++;
		}
	}

	/* Find the amount of sram */
	snes_cart.sram = snes_r_bank1(0x00ffd8);
	if( snes_cart.sram > 0 )
	{
		snes_cart.sram = ((1 << (snes_cart.sram + 3)) / 8);
		if( snes_cart.sram > snes_cart.sram_max )
			snes_cart.sram = snes_cart.sram_max;
	}

	/* Log snes_cart information */
	{
		char title[21], romid[4], companyid[2];
		UINT8 country;
		logerror( "ROM DETAILS\n" );
		logerror( "\tTotal blocks:  %d (%dmb)\n", totalblocks, totalblocks / (snes_cart.mode == SNES_MODE_20 ? 32 : 16) );
		logerror( "\tROM bank size: %s (LoROM: %d , HiROM: %d)\n", (snes_cart.mode == SNES_MODE_20) ? "LoROM" : "HiROM", valid_mode20, valid_mode21 );
		for( i = 0; i < 2; i++ )
			companyid[i] = snes_r_bank1(0x00ffb0 + i);
		logerror( "\tCompany ID:    %s\n", companyid );
		for( i = 0; i < 4; i++ )
			romid[i] = snes_r_bank1(0x00ffb2 + i);
		logerror( "\tROM ID:        %s\n", romid );
		logerror( "HEADER DETAILS\n" );
		for( i = 0; i < 21; i++ )
			title[i] = snes_r_bank1(0x00ffc0 + i);
		logerror( "\tName:          %s\n", title );
		logerror( "\tSpeed:         %s [%d]\n", ((snes_r_bank1(0x00ffd5) & 0xf0)) ? "FastROM" : "SlowROM", (snes_r_bank1(0x00ffd5) & 0xf0) >> 4 );
		logerror( "\tBank size:     %s [%d]\n", (snes_r_bank1(0x00ffd5) & 0xf) ? "HiROM" : "LoROM", snes_r_bank1(0x00ffd5) & 0xf );
		for( i = 0; i < 12; i++ )
		{
			if( CartTypes[i].Code == snes_r_bank1(0x00ffd6) )
				break;
		}
		logerror( "\tType:          %s [%d]\n", CartTypes[i].Name, snes_r_bank1(0x00ffd6) );
		logerror( "\tSize:          %d megabits [%d]\n", 1 << (snes_r_bank1(0x00ffd7) - 7), snes_r_bank1(0x00ffd7) );
		logerror( "\tSRAM:          %d kilobits [%d]\n", snes_cart.sram * 8, snes_ram[0xffd8] );
		country = snes_r_bank1(0x00ffd9);
		if( country > 14 )
			country = 14;
		logerror( "\tCountry:       %s [%d]\n", countries[country], snes_r_bank1(0x00ffd9) );
		logerror( "\tLicense:       %s [%X]\n", "", snes_r_bank1(0x00ffda) );
		logerror( "\tVersion:       1.%d\n", snes_r_bank1(0x00ffdb) );
		logerror( "\tInv Checksum:  %X %X\n", snes_r_bank1(0x00ffdd), snes_r_bank1(0x00ffdc) );
		logerror( "\tChecksum:      %X %X\n", snes_r_bank1(0x00ffdf), snes_r_bank1(0x00ffde) );
		logerror( "\tNMI Address:   %2X%2Xh\n", snes_r_bank1(0x00fffb), snes_r_bank1(0x00fffa) );
		logerror( "\tStart Address: %2X%2Xh\n", snes_r_bank1(0x00fffd), snes_r_bank1(0x00fffc) );
	}

	/* Load SRAM */
	snes_load_sram();

	/* All done */
	return INIT_PASS;
}


static MACHINE_DRIVER_START( snes )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", G65816, 2680000)	/* 2.68Mhz, also 3.58Mhz */
	MDRV_CPU_PROGRAM_MAP(snes_map, 0)
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, SNES_MAX_LINES_NTSC)

	MDRV_CPU_ADD_TAG("sound", SPC700, 2048000)	/* 2.048 Mhz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(spc_map, 0)
	MDRV_CPU_VBLANK_INT(NULL, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( snes )
	MDRV_MACHINE_STOP( snes )

	/* video hardware */
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( snes )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(SNES_SCR_WIDTH * 2, SNES_SCR_HEIGHT * 2)
	MDRV_VISIBLE_AREA(0, SNES_SCR_WIDTH-1, 0, SNES_SCR_HEIGHT-1 )
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(257)
	MDRV_PALETTE_INIT( snes )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(snes_sound_interface)
	MDRV_SOUND_ROUTE(0, "left", 0.50)
	MDRV_SOUND_ROUTE(0, "right", 0.50)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( snespal )
	MDRV_IMPORT_FROM(snes)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_VBLANK_INT(snes_scanline_interrupt, SNES_MAX_LINES_PAL)
	MDRV_FRAMES_PER_SECOND(50)
MACHINE_DRIVER_END

static void snes_cartslot_getinfo(struct IODevice *dev)
{
	/* cartslot */
	cartslot_device_getinfo(dev);
	dev->count = 1;
	dev->file_extensions = "smc\0sfc\0fig\0swc\0";
	dev->must_be_loaded = 1;
	dev->load = device_load_snes_cart;
}

SYSTEM_CONFIG_START(snes)
	CONFIG_DEVICE(snes_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(snes)
	ROM_REGION(0x1000000,       REGION_CPU1,  0)		/* 65C816 */
	ROM_REGION(SNES_VRAM_SIZE,  REGION_GFX1,  0)		/* VRAM */
	ROM_REGION(SNES_CGRAM_SIZE, REGION_USER1, 0)		/* CGRAM */
	ROM_REGION(SNES_OAM_SIZE,   REGION_USER2, 0)		/* OAM */
	ROM_REGION(0x10000,         REGION_CPU2,  0)		/* SPC700 */
	ROM_LOAD("spc700.rom", 0xFFC0, 0x40, CRC(38000B6B))	/* boot rom */
ROM_END

ROM_START(snespal)
	ROM_REGION(0x1000000,       REGION_CPU1,  0)		/* 65C816 */
	ROM_REGION(SNES_VRAM_SIZE,  REGION_GFX1,  0)		/* VRAM */
	ROM_REGION(SNES_CGRAM_SIZE, REGION_USER1, 0)		/* CGRAM */
	ROM_REGION(SNES_OAM_SIZE,   REGION_USER2, 0)		/* OAM */
	ROM_REGION(0x10000,         REGION_CPU2,  0)		/* SPC700 */
	ROM_LOAD("spc700.rom", 0xFFC0, 0x40, CRC(38000B6B))	/* boot rom */
ROM_END

/*     YEAR  NAME     PARENT  COMPAT  MACHINE  INPUT  INIT  CONFIG  COMPANY     FULLNAME                                      FLAGS */
CONSX( 1989, snes,    0,      0,      snes,    snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (NTSC)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
CONSX( 1991, snespal, snes,   0,      snespal, snes,  0,    snes,   "Nintendo", "Super Nintendo Entertainment System (PAL)",  GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
