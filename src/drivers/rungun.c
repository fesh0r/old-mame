/*
   Run and Gun / Slam Dunk
   (c) 1993 Konami

   Driver by R. Belmont.

   This hardware uses the 55673 sprite chip like PreGX and System GX, but in a 4 bit
   per pixel layout.  There is also an all-TTL front overlay tilemap and a rotating
   scaling background done with the PSAC2 ('936).

   Status: Front tilemap should be complete, sprites are mostly correct, controls
   should be fine.  Protection exists so the game is not playable.

   TODO:
   - protection and other oddities (rungun shows gameplay in attract mode, rungunu doesn't)
   - sprite palettes are not entirely right
   - priorities are wrong
*/

#include "driver.h"
#include "state.h"

#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"

VIDEO_START(rng);
VIDEO_UPDATE(rng);
READ16_HANDLER( ttl_ram_r );
WRITE16_HANDLER( ttl_ram_w );

data16_t* rng_936_videoram;
WRITE16_HANDLER(rng_936_videoram_w);

static int init_eeprom_count;
static int rng_irq_control;

static struct EEPROM_interface eeprom_interface =
{
	7,			/* address bits */
	8,			/* data bits */
	"011000",		/*  read command */
	"011100",		/* write command */
	"0100100000000",/* erase command */
	"0100000000000",/* lock command */
	"0100110000000" /* unlock command */
};

static NVRAM_HANDLER(rungun)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);

		if (file)
		{
			init_eeprom_count = 0;
			EEPROM_load(file);
		}
		else
			init_eeprom_count = 10;
	}
}

static WRITE16_HANDLER( rngctrl_w )
{
	// control register:
	// bit 3: enable IRQ 5
	// bit 2: OBJCHA
	// bit 1: ??? (screen on?)
	// bit 0: ???

	K053246_set_OBJCHA_line((data & 0x4) ? ASSERT_LINE : CLEAR_LINE);

	rng_irq_control = data;
}

static INTERRUPT_GEN(rng_interrupt)
{
	if (rng_irq_control & 0x08)
	{
		cpu_set_irq_line(0, MC68000_IRQ_5, HOLD_LINE);
	}
}

static READ16_HANDLER( rngplayer1_r )
{
	return readinputport(2) | (readinputport(4)<<8);
}

static READ16_HANDLER( rngplayer2_r )
{
	return readinputport(3) | (readinputport(5)<<8);
}

static READ16_HANDLER( rngcoins_r )
{
	return readinputport(0);
}

static WRITE16_HANDLER( sound_cmd1_w )
{
	if(ACCESSING_MSB) {
		soundlatch_w(0, data>>8);
		return;
	}
}

static WRITE16_HANDLER( sound_cmd2_w )
{
	if(ACCESSING_MSB)
	{
		soundlatch2_w(0, data>>8);
		return;
	}
}

static WRITE16_HANDLER( sound_irq_w )
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
}

static READ16_HANDLER( sound_status_r )
{
//	int latch = soundlatch3_r(0);

	return 0x3f3f;	// pass POST for now (hack - need to find NMI disable register in 54539!)
}

static READ16_HANDLER( rngeeprom_r )
{
	if (ACCESSING_LSB)
	{
		int res = input_port_1_word_r(0,0) | EEPROM_read_bit();
		if (init_eeprom_count)
		{
			init_eeprom_count--;
			res &= 0xf7;
		}

		return res;
	}

	return 0;
}

static WRITE16_HANDLER( rngeeprom_w )
{
	if (ACCESSING_LSB)
	{
		EEPROM_write_bit((data&0x01) ? 1 : 0);
		EEPROM_set_cs_line((data&0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data&0x04) ? ASSERT_LINE : CLEAR_LINE);
		return;
	}

//	fprintf(stderr, "unknown MSB write %x to eeprom\n", data);
}

static MEMORY_READ16_START( rngreadmem )
	{ 0x000000, 0x2fffff, MRA16_ROM },	// main program + data
	{ 0x300000, 0x3007ff, MRA16_RAM },
	{ 0x380000, 0x39ffff, MRA16_RAM },
	{ 0x400000, 0x43ffff, MRA16_NOP }, //K053936_0_rom_r }, // '936 ROM readback window
	{ 0x480000, 0x480001, rngplayer1_r },	// player 1/3
	{ 0x480002, 0x480003, rngplayer2_r },	// player 2/4
	{ 0x480004, 0x480005, rngcoins_r },	// coins
	{ 0x480006, 0x480007, rngeeprom_r },
	{ 0x580014, 0x580017, sound_status_r },
	{ 0x5c0000, 0x5c000d, K053246_word_r },	// 246A ROM readback window
	{ 0x600000, 0x600fff, K053247_word_r },
	{ 0x601000, 0x601fff, MRA16_RAM },
	{ 0x6c0000, 0x6cffff, MRA16_RAM },
	{ 0x700000, 0x7007ff, MRA16_RAM },
	{ 0x740000, 0x741fff, ttl_ram_r },	// text plane RAM
MEMORY_END

static MEMORY_WRITE16_START( rngwritemem )
	{ 0x000000, 0x2fffff, MWA16_ROM },
	{ 0x300000, 0x3007ff, paletteram16_xBBBBBGGGGGRRRRR_word_w, &paletteram16 },
	{ 0x380000, 0x39ffff, MWA16_RAM },	// work RAM
	{ 0x480008, 0x480009, rngeeprom_w },	// eeprom control
	{ 0x48000c, 0x48000d, rngctrl_w },
	{ 0x540000, 0x540001, sound_irq_w },
	{ 0x58000c, 0x58000d, sound_cmd1_w },
	{ 0x58000e, 0x58000f, sound_cmd2_w },
	{ 0x600000, 0x600fff, K053247_word_w },	// OBJ RAM
	{ 0x601000, 0x601fff, MWA16_RAM },
	{ 0x640000, 0x640007, K053246_word_w },	// '246A registers
	{ 0x680000, 0x68001f, MWA16_RAM, &K053936_0_ctrl },	// '936 registers
	{ 0x6c0000, 0x6cffff, rng_936_videoram_w, &rng_936_videoram }, 	// PSAC2 ('936) RAM (34v + 35v)
	{ 0x700000, 0x7007ff, MWA16_RAM, &K053936_0_linectrl },	// "Line RAM"
	{ 0x740000, 0x741fff, ttl_ram_w },	// text plane RAM
	{ 0x7c0000, 0x7c0001, MWA16_NOP },	// watchdog
MEMORY_END

/**********************************************************************************/

static int cur_sound_region;

static void reset_sound_region(void)
{
	cpu_setbank(2, memory_region(REGION_CPU2) + 0x10000 + cur_sound_region*0x4000);
}

static WRITE_HANDLER( sound_bankswitch_w )
{
	cur_sound_region = (data & 0xf);
	reset_sound_region();
}

static INTERRUPT_GEN(audio_interrupt)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

/* sound (this should be split into sndhrdw/xexex.c or pregx.c or so someday) */

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK2 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe22f, K054539_0_r },
	{ 0xe230, 0xe3ff, MRA_RAM },
	{ 0xe400, 0xe62f, K054539_1_r },
	{ 0xe630, 0xe7ff, MRA_RAM },
	{ 0xf002, 0xf002, soundlatch_r },
	{ 0xf003, 0xf003, soundlatch2_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xdfff, MWA_RAM },
	{ 0xe000, 0xe22f, K054539_0_w },
	{ 0xe230, 0xe3ff, MWA_RAM },
	{ 0xe400, 0xe62f, K054539_1_w },
	{ 0xe630, 0xe7ff, MWA_RAM },
	{ 0xf000, 0xf000, soundlatch3_w },
	{ 0xf800, 0xf800, sound_bankswitch_w },
	{ 0xfff1, 0xfff3, MWA_NOP },
MEMORY_END

static struct K054539interface k054539_interface =
{
	2,			/* 2 chips */
	48000,
	{ REGION_SOUND1, REGION_SOUND1 },
	{ { 100, 100 }, { 100, 100 } },
	{ NULL }
};

/**********************************************************************************/

static struct GfxLayout bglayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4,
	  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bglayout,     0x0000, 64 },
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( rng )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 16000000)
	MDRV_CPU_MEMORY(rngreadmem,rngwritemem)
	MDRV_CPU_VBLANK_INT(rng_interrupt,1)

	MDRV_CPU_ADD_TAG("sound", Z80, 8000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(audio_interrupt, 480)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_NVRAM_HANDLER(rungun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_HAS_SHADOWS)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(11*8, 59*8-1, 3*8, 31*8-1 )
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(rng)
	MDRV_VIDEO_UPDATE(rng)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K054539, k054539_interface)
MACHINE_DRIVER_END

INPUT_PORTS_START( rng )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* EEPROM data */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* EEPROM ready (always 1) */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_DIPNAME( 0x10, 0x00, "Monitors" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPNAME( 0x20, 0x00, "Number of players" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x20, "4" )
	PORT_DIPNAME( 0x40, 0x00, "Sound Output" )
	PORT_DIPSETTING(    0x40, "Mono" )
	PORT_DIPSETTING(    0x00, "Stereo" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START3 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )
INPUT_PORTS_END

#define ROM_LOAD64_WORD(name,offset,length,crc)		ROMX_LOAD(name, offset, length, crc, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START( rungunu )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247b03", 0x000000, 0x80000, 0xf259fd11 )
	ROM_LOAD16_BYTE( "247b04", 0x000001, 0x80000, 0xb918cf5a )

	/* data */
	ROM_LOAD16_BYTE( "247a01", 0x100000, 0x80000, 0x8341cf7d )
	ROM_LOAD16_BYTE( "247a02", 0x100001, 0x80000, 0xf5ef3f45 )

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("247a05", 0x000000, 0x20000, 0x64e85430 )
	ROM_RELOAD(           0x010000, 0x020000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247a13", 0x000000, 0x200000, 0xc5a8ef29 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, 0xc3f60854 )	// 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, 0x3e315eef )	// 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, 0x5ca7bc06 )	// 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, 0xa5ccd243 )	// 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, 0x57a8d26e )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, 0xb8b2a67e )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, 0x0108142d )
ROM_END

ROM_START( rungun )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247-c03", 0x000000, 0x80000, 0xfec3e1d6 )
	ROM_LOAD16_BYTE( "247-c04", 0x000001, 0x80000, 0x1b556af9 )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, 0x2d774f27 )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, 0xd088c9de )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("247-a05", 0x000000, 0x20000, 0x412fa1e0 )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, 0xcc194089 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, 0xc3f60854 )	// 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, 0x3e315eef )	// 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, 0x5ca7bc06 )	// 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, 0xa5ccd243 )	// 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, 0x57a8d26e )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, 0xb8b2a67e )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, 0x0108142d )
ROM_END

ROM_START( slmdunkj )
	/* main program */
	ROM_REGION( 0x300000, REGION_CPU1, 0)
	ROM_LOAD16_BYTE( "247jaa03.bin", 0x000000, 0x20000, 0x87572078 )
	ROM_LOAD16_BYTE( "247jaa04.bin", 0x000001, 0x20000, 0xaa105e00 )

	/* data (Guru 1 megabyte redump) */
	ROM_LOAD16_BYTE( "247b01.23n", 0x200000, 0x80000, 0x2d774f27 )
	ROM_CONTINUE(                  0x100000, 0x80000)
	ROM_LOAD16_BYTE( "247b02.21n", 0x200001, 0x80000, 0xd088c9de )
	ROM_CONTINUE(                  0x100001, 0x80000)

	/* sound program */
	ROM_REGION( 0x040000, REGION_CPU2, 0 )
	ROM_LOAD("247-a05", 0x000000, 0x20000, 0x412fa1e0 )
	ROM_RELOAD(         0x010000, 0x020000 )

	/* '936 tiles */
	ROM_REGION( 0x400000, REGION_GFX1, 0)
	ROM_LOAD( "247-a13", 0x000000, 0x200000, 0xcc194089 )

	/* sprites */
	ROM_REGION( 0x800000, REGION_GFX2, 0)
	ROM_LOAD64_WORD( "247-a11", 0x000000, 0x200000, 0xc3f60854 )	// 5y
	ROM_LOAD64_WORD( "247-a08", 0x000002, 0x200000, 0x3e315eef )	// 2u
	ROM_LOAD64_WORD( "247-a09", 0x000004, 0x200000, 0x5ca7bc06 )	// 2y
	ROM_LOAD64_WORD( "247-a10", 0x000006, 0x200000, 0xa5ccd243 )	// 5u

	/* TTL text plane ("fix layer") */
	ROM_REGION( 0x20000, REGION_GFX3, 0)
	ROM_LOAD( "247-a12", 0x000000, 0x20000, 0x57a8d26e )

	/* sound data */
	ROM_REGION( 0x400000, REGION_SOUND1, 0)
	ROM_LOAD( "247-a06", 0x000000, 0x200000, 0xb8b2a67e )
	ROM_LOAD( "247-a07", 0x200000, 0x200000, 0x0108142d )
ROM_END



GAMEX( 1993, rungun,   0,      rng, rng, 0, ROT0, "Konami", "Run and Gun (World ver. EAA)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
GAMEX( 1993, rungunu,  rungun, rng, rng, 0, ROT0, "Konami", "Run and Gun (US ver. UAB)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
GAMEX( 1993, slmdunkj, rungun, rng, rng, 0, ROT0, "Konami", "Slam Dunk (Japan ver. JAA))", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS )
