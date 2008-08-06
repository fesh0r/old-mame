/*
World Cup 90 bootleg driver
---------------------------

Ernesto Corvi
(ernesto@imagina.com)

CPU #1 : Handles background & foreground tiles, controllers, dipswitches.
CPU #2 : Handles sprites and palette
CPU #3 : Audio. The audio chip is a YM2203. I need help with this!.

Memory Layout:

CPU #1
0000-8000 ROM
8000-9000 RAM
a000-a800 Color Ram for background #1 tiles
a800-b000 Video Ram for background #1 tiles
c000-c800 Color Ram for background #2 tiles
c800-c000 Video Ram for background #2 tiles
e000-e800 Color Ram for foreground tiles
e800-f000 Video Ram for foreground tiles
f800-fc00 Common Ram with CPU #2
fd00-fd00 Stick 1, Coin 1 & Start 1 input port
fd02-fd02 Stick 2, Coin 2 & Start 2 input port
fd06-fc06 Dip Switch A
fd08-fc08 Dip Switch B

CPU #2
0000-c000 ROM
c000-d000 RAM
d000-d800 RAM Sprite Ram
e000-e800 RAM Palette Ram
f800-fc00 Common Ram with CPU #1

CPU #3
0000-0xc000 ROM
???????????

Notes:
-----
The bootleg video hardware is quite different from the original machine.
I could not figure out the encoding of the scrolling for the new
video hardware. The memory positions, in case anyone wants to try, are
the following ( CPU #1 memory addresses ):
fd06: scroll bg #1 X coordinate
fd04: scroll bg #1 Y coordinate
fd08: scroll bg #2 X coordinate
fd0a: scroll bg #2 Y coordinate
fd0e: ????

What i used instead, was the local copy kept in RAM. These values
are the ones the original machine uses. This will differ when trying
to use some of this code to write a driver for a similar tecmo bootleg.

Sprites are also very different. Theres a code snippet in the ROM
that converts the original sprites to the new format, which only allows
16x16 sprites. That snippet also does some ( nasty ) clipping.

Colors are accurate. The graphics ROMs have been modified severely
and encoded in a different way from the original machine. Even if
sometimes it seems colors are not entirely correct, this is only due
to the crappy artwork of the person that did the bootleg.

Dip switches are not complete and they dont seem to differ from
the original machine.

Last but not least, the set of ROMs i have for Euro League seem to have
the sprites corrupted. The game seems to be exactly the same as the
World Cup 90 bootleg.

Noted added by ClawGrip 28-Mar-2008:
-----------------------------------
-Dumped and added the all the PCB GALs.
-Removed the second YM2203, Ernesto said it wasn't present on his board,
 and also isn't on mine.
-My PCB has a different ROM (a05.bin), but only two bytes are different.
 Dox suggested that it can be just a year or text mod, so I decided not
 to include my set. If anyone wants it, please mail me:
 clawgrip at hotmail dot com. I can't find any graphical difference
 between my set and the one already on MAME.

*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"


#define TEST_DIPS false /* enable to test unmapped dip switches */

extern UINT8 *wc90b_fgvideoram,*wc90b_bgvideoram,*wc90b_txvideoram;

extern UINT8 *wc90b_scroll1x;
extern UINT8 *wc90b_scroll2x;

extern UINT8 *wc90b_scroll1y;
extern UINT8 *wc90b_scroll2y;

VIDEO_START( wc90b );
WRITE8_HANDLER( wc90b_bgvideoram_w );
WRITE8_HANDLER( wc90b_fgvideoram_w );
WRITE8_HANDLER( wc90b_txvideoram_w );
VIDEO_UPDATE( wc90b );

static int msm5205next;

static UINT8 *wc90b_shared;

static READ8_HANDLER( wc90b_shared_r )
{
	return wc90b_shared[offset];
}

static WRITE8_HANDLER( wc90b_shared_w )
{
	wc90b_shared[offset] = data;
}

static WRITE8_HANDLER( wc90b_bankswitch_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(machine, "main");


	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	memory_set_bankptr(1,&RAM[bankaddress]);
}

static WRITE8_HANDLER( wc90b_bankswitch1_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(machine, "sub");


	bankaddress = 0x10000 + ((data & 0xf8) << 8);
	memory_set_bankptr(2,&RAM[bankaddress]);
}

static WRITE8_HANDLER( wc90b_sound_command_w )
{
	soundlatch_w(machine,offset,data);
	cpunum_set_input_line(machine, 2,0,HOLD_LINE);
}

static WRITE8_HANDLER( adpcm_control_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(machine, "sub");

	/* the code writes either 2 or 3 in the bottom two bits */
	bankaddress = 0x10000 + (data & 0x01) * 0x4000;
	memory_set_bankptr(3,&RAM[bankaddress]);

	MSM5205_reset_w(0,data & 0x08);
}

static WRITE8_HANDLER( adpcm_data_w )
{
	msm5205next = data;
}


static ADDRESS_MAP_START( wc90b_map1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_RAM /* Main RAM */
	AM_RANGE(0xa000, 0xafff) AM_RAM_WRITE(wc90b_fgvideoram_w) AM_BASE(&wc90b_fgvideoram)
	AM_RANGE(0xc000, 0xcfff) AM_RAM_WRITE(wc90b_bgvideoram_w) AM_BASE(&wc90b_bgvideoram)
	AM_RANGE(0xe000, 0xefff) AM_RAM_WRITE(wc90b_txvideoram_w) AM_BASE(&wc90b_txvideoram)
	AM_RANGE(0xf000, 0xf7ff) AM_ROMBANK(1)
	AM_RANGE(0xf800, 0xfbff) AM_READWRITE(wc90b_shared_r, wc90b_shared_w) AM_BASE(&wc90b_shared)
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(wc90b_bankswitch_w)
	AM_RANGE(0xfd00, 0xfd00) AM_WRITE(wc90b_sound_command_w)
	AM_RANGE(0xfd04, 0xfd04) AM_WRITEONLY AM_BASE(&wc90b_scroll1y)
	AM_RANGE(0xfd06, 0xfd06) AM_WRITEONLY AM_BASE(&wc90b_scroll1x)
	AM_RANGE(0xfd08, 0xfd08) AM_WRITEONLY AM_BASE(&wc90b_scroll2y)
	AM_RANGE(0xfd0a, 0xfd0a) AM_WRITEONLY AM_BASE(&wc90b_scroll2x)
	AM_RANGE(0xfd00, 0xfd00) AM_READ(input_port_0_r) /* Stick 1, Coin 1 & Start 1 */
	AM_RANGE(0xfd02, 0xfd02) AM_READ(input_port_1_r) /* Stick 2, Coin 2 & Start 2 */
	AM_RANGE(0xfd06, 0xfd06) AM_READ(input_port_2_r) /* DIP Switch A */
	AM_RANGE(0xfd08, 0xfd08) AM_READ(input_port_3_r) /* DIP Switch B */
ADDRESS_MAP_END

static ADDRESS_MAP_START( wc90b_map2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_ROM
	AM_RANGE(0xc000, 0xcfff) AM_RAM
	AM_RANGE(0xd000, 0xd7ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xd800, 0xdfff) AM_RAM
	AM_RANGE(0xe000, 0xe7ff) AM_RAM_WRITE(paletteram_xxxxBBBBGGGGRRRR_be_w) AM_BASE(&paletteram)
	AM_RANGE(0xe800, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_ROMBANK(2)
	AM_RANGE(0xf800, 0xfbff) AM_READWRITE(wc90b_shared_r, wc90b_shared_w)
	AM_RANGE(0xfc00, 0xfc00) AM_WRITE(wc90b_bankswitch1_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK(3)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(adpcm_control_w)
	AM_RANGE(0xe400, 0xe400) AM_WRITE(adpcm_data_w)
	AM_RANGE(0xe800, 0xe800) AM_READWRITE(YM2203_status_port_0_r, YM2203_control_port_0_w)
	AM_RANGE(0xe801, 0xe801) AM_READWRITE(YM2203_read_port_0_r, YM2203_write_port_0_w)
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xf800) AM_READ(soundlatch_r)
ADDRESS_MAP_END



static INPUT_PORTS_START( wc90b )
	PORT_START("P1")	/* IN0 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START("P2")	/* IN1 bit 0-5 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START("DSW1")	/* DSWA */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, "10 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, DEF_STR( 9C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 8C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 7C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 6C_1C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x40, 0x40, "Countdown Speed" )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )					// 60/60
	PORT_DIPSETTING(    0x00, "Fast" )						// 56/60
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START("DSW2")	/* DSWB */
	PORT_DIPNAME( 0x03, 0x03, "1 Player Game Time" )
	PORT_DIPSETTING(    0x01, "1:00" )
	PORT_DIPSETTING(    0x02, "1:30" )
	PORT_DIPSETTING(    0x03, "2:00" )
	PORT_DIPSETTING(    0x00, "2:30" )
	PORT_DIPNAME( 0x1c, 0x1c, "2 Player Game Time" )
	PORT_DIPSETTING(    0x0c, "1:00" )
	PORT_DIPSETTING(    0x14, "1:30" )
	PORT_DIPSETTING(    0x04, "2:00" )
	PORT_DIPSETTING(    0x18, "2:30" )
	PORT_DIPSETTING(    0x1c, "3:00" )
	PORT_DIPSETTING(    0x08, "3:30" )
	PORT_DIPSETTING(    0x10, "4:00" )
	PORT_DIPSETTING(    0x00, "5:00" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Japanese ) )
INPUT_PORTS_END



static const gfx_layout charlayout =
{
	8,8,	/* 8*8 characters */
	2048,	/* 2048 characters */
	4,	/* 4 bits per pixel */
	{ 0, 0x4000*8, 0x8000*8, 0xc000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_layout tilelayout =
{
	16,16,	/* 16*16 characters */
	256,	/* 256 characters */
	4,	/* 4 bits per pixel */
	{ 0*0x20000*8, 1*0x20000*8, 2*0x20000*8, 3*0x20000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		(0x1000*8)+0, (0x1000*8)+1, (0x1000*8)+2, (0x1000*8)+3, (0x1000*8)+4, (0x1000*8)+5, (0x1000*8)+6, (0x1000*8)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		0x800*8, 0x800*8+1*8, 0x800*8+2*8, 0x800*8+3*8, 0x800*8+4*8, 0x800*8+5*8, 0x800*8+6*8, 0x800*8+7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	16,16,	/* 32*32 characters */
	4096,	/* 1024 characters */
	4,	/* 4 bits per pixel */
	{ 3*0x20000*8, 2*0x20000*8, 1*0x20000*8, 0*0x20000*8 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		(16*8)+0, (16*8)+1, (16*8)+2, (16*8)+3, (16*8)+4, (16*8)+5, (16*8)+6, (16*8)+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 8*8+1*8, 8*8+2*8, 8*8+3*8, 8*8+4*8, 8*8+5*8, 8*8+6*8, 8*8+7*8 },
	32*8	/* every char takes 128 consecutive bytes */
};

static GFXDECODE_START( wc90b )
	GFXDECODE_ENTRY( "gfx1", 0x00000, charlayout,      	1*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x00000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x02000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x04000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x06000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x08000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x0a000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x0c000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x0e000, tilelayout,			2*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x10000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x12000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x14000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x16000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x18000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x1a000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x1c000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx2", 0x1e000, tilelayout,			3*16*16, 16*16 )
	GFXDECODE_ENTRY( "gfx3", 0x00000, spritelayout,		0*16*16, 16*16 ) // sprites
GFXDECODE_END



/* handler called by the 2203 emulator when the internal timers cause an IRQ */
static void irqhandler(running_machine *machine, int irq)
{
	cpunum_set_input_line(machine, 2, INPUT_LINE_NMI, irq ? ASSERT_LINE : CLEAR_LINE);
}

static const struct YM2203interface ym2203_interface =
{
	{
		AY8910_LEGACY_OUTPUT,
		AY8910_DEFAULT_LOADS,
		NULL, NULL, NULL, NULL
	},
	irqhandler
};

static void adpcm_int(running_machine *machine, int data)
{
	static int toggle = 0;

	MSM5205_data_w (0,msm5205next);
	msm5205next>>=4;

	toggle ^= 1;
	if(toggle)
		cpunum_set_input_line(machine, 2, INPUT_LINE_NMI, PULSE_LINE);

}

static const struct MSM5205interface msm5205_interface =
{
	adpcm_int,	            /* interrupt function */
	MSM5205_S96_4B		/* 4KHz 4-bit */
};

static MACHINE_DRIVER_START( wc90b )

	/* basic machine hardware */
	MDRV_CPU_ADD("main", Z80, XTAL_14MHz/2)
	MDRV_CPU_PROGRAM_MAP(wc90b_map1,0)
	MDRV_CPU_VBLANK_INT("main", irq0_line_hold)

	MDRV_CPU_ADD("sub", Z80, XTAL_14MHz/2)
	MDRV_CPU_PROGRAM_MAP(wc90b_map2,0)
	MDRV_CPU_VBLANK_INT("main", irq0_line_hold)

	MDRV_CPU_ADD("audio", Z80, XTAL_19_6608MHz/8)
	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)
	/* IRQs are triggered by the main CPU */

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(wc90b)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(wc90b)
	MDRV_VIDEO_UPDATE(wc90b)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("ym", YM2203, 2510000/2)
	MDRV_SOUND_CONFIG(ym2203_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.20)

	MDRV_SOUND_ADD("msm", MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.30)
MACHINE_DRIVER_END

ROM_START( wc90b )
	ROM_REGION( 0x20000, "main", 0 )
	ROM_LOAD( "a02.bin",      0x00000, 0x10000, CRC(192a03dd) SHA1(ab98d370bba5437f956631b0199b173be55f1c27) )	/* c000-ffff is not used */
	ROM_LOAD( "a03.bin",      0x10000, 0x10000, CRC(f54ff17a) SHA1(a19850fc28a5a0da20795a5cc6b56d9c16554bce) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, "sub", 0 )  /* Second CPU */
	ROM_LOAD( "a04.bin",      0x00000, 0x10000, CRC(3d535e2f) SHA1(f1e1878b5a8316e770c74a1e1f29a7a81a4e5dfe) )	/* c000-ffff is not used */
	ROM_LOAD( "a05.bin",      0x10000, 0x10000, CRC(9e421c4b) SHA1(e23a1f1d5d1e960696f45df653869712eb889839) )	/* banked at f000-f7ff */

	ROM_REGION( 0x18000, "audio", 0 )
	ROM_LOAD( "a01.bin",      0x00000, 0x8000, CRC(3d317622) SHA1(ae4e8c5247bc215a2769786cb8639bce2f80db22) )
	ROM_CONTINUE(             0x10000, 0x8000 ) /* banked at 8000-bfff */

	ROM_REGION( 0x010000, "gfx1", ROMREGION_DISPOSE )
	ROM_LOAD( "a06.bin",      0x000000, 0x04000, CRC(3b5387b7) SHA1(b839b4eafe8bf6f9e841e19fee1bdb64a66f3448) )
	ROM_LOAD( "a08.bin",      0x004000, 0x04000, CRC(c622a5a3) SHA1(468c8c24af1f6f244228b66df04cb0ea81c1875e) )
	ROM_LOAD( "a10.bin",      0x008000, 0x04000, CRC(0923d9f6) SHA1(4b10ee3fc17bb63cda51b2a978d066b6a140a551) )
	ROM_LOAD( "a20.bin",      0x00c000, 0x04000, CRC(b8dec83e) SHA1(fe617ddccdd0dbd05ca09a1507074aa14b529322) )

	ROM_REGION( 0x080000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "a07.bin",      0x000000, 0x20000, CRC(38c31817) SHA1(cb24ed8702d62066366924c033c07ffc78bd1fad) )
	ROM_LOAD( "a09.bin",      0x020000, 0x20000, CRC(32e39e29) SHA1(44f22ed6c983541c7fea5857ba0456aaa87b36d1) )
	ROM_LOAD( "a11.bin",      0x040000, 0x20000, CRC(5ccec796) SHA1(2cc191a4267819eb31962726e2ed4567c825c39e) )
	ROM_LOAD( "a21.bin",      0x060000, 0x20000, CRC(0c54a091) SHA1(3eecb285b5a7bbc310c87492516d7ffb2841aa3b) )

	ROM_REGION( 0x080000, "gfx3", ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "146_a12.bin",  0x000000, 0x10000, CRC(d5a60096) SHA1(a8e351a4b020b4fc2b2cb7d3f0fdfb43fc44d7d9) )
	ROM_LOAD( "147_a13.bin",  0x010000, 0x10000, CRC(36bbf467) SHA1(627b5847ffb098c92edfd58c25391799f3b209e0) )
	ROM_LOAD( "148_a14.bin",  0x020000, 0x10000, CRC(26371c18) SHA1(0887041d86dc9f19dad264ae27dc56fb89ac3265) )
	ROM_LOAD( "149_a15.bin",  0x030000, 0x10000, CRC(75aa9b86) SHA1(0c221bd2e8a5472bb0e515f27fb72b0c8e8c0ca4) )
	ROM_LOAD( "150_a16.bin",  0x040000, 0x10000, CRC(0da825f9) SHA1(cfba0c85fc767726c1d63f87468335d1c2f1eed8) )
	ROM_LOAD( "151_a17.bin",  0x050000, 0x10000, CRC(228429d8) SHA1(3b2dbea53807929c24d593c469a83172f7747f66) )
	ROM_LOAD( "152_a18.bin",  0x060000, 0x10000, CRC(516b6c09) SHA1(9d02514dece864b087f67886009ce54bd51b5575) )
	ROM_LOAD( "153_a19.bin",  0x070000, 0x10000, CRC(f36390a9) SHA1(e5ea36e91b3ced068281524ee79d0432f489715c) )

	ROM_REGION( 0x1000, "plds", ROMREGION_DISPOSE )
	ROM_LOAD( "el_ic39_gal16v8_0.bin", 0x0000, 0x0117, NO_DUMP SHA1(894b345b395097acf6cf52ab8bc922099f97a85f) )
	ROM_LOAD( "el_ic44_gal16v8_1.bin", 0x0200, 0x0117, NO_DUMP SHA1(fd41f55d857995fe87217dd9679c42760c241dc4) )
	ROM_LOAD( "el_ic54_gal16v8_2.bin", 0x0400, 0x0117, NO_DUMP SHA1(f6d138fe42549219e11ee8524b05fe3c2b43f5d3) )
	ROM_LOAD( "el_ic100_gal16v8_3.bin", 0x0600, 0x0117, NO_DUMP SHA1(515fcdf378e75ed078f54439fefce8807403bdd5) )
	ROM_LOAD( "el_ic143_gal16v8_4.bin", 0x0800, 0x0117, NO_DUMP SHA1(fbe632437eac2418da7a3c3e947cfd36f6211407) )
ROM_END


#if 0
/* Different bootleg set with only one new ROM, a05 (added as "el_ic98_27c512_05.bin"), not included because it's
probably just a minor text mod from the supported set (only two bytes differs), although I cannot find the difference:
   Comparing files a05.bin and el_ic98_27c512_05.bin
    00000590: 0F 0B
    00000591: FF FA
*/
ROM_START( wc90ba )
	ROM_REGION( 0x20000, "main", 0 )
	ROM_LOAD( "a02.bin",      0x00000, 0x10000, CRC(192a03dd) SHA1(ab98d370bba5437f956631b0199b173be55f1c27) )	/* c000-ffff is not used */
	ROM_LOAD( "a03.bin",      0x10000, 0x10000, CRC(f54ff17a) SHA1(a19850fc28a5a0da20795a5cc6b56d9c16554bce) )	/* banked at f000-f7ff */

	ROM_REGION( 0x20000, "sub", 0 )  /* Second CPU */
	ROM_LOAD( "a04.bin",              0x00000, 0x10000, CRC(3d535e2f) SHA1(f1e1878b5a8316e770c74a1e1f29a7a81a4e5dfe) )	/* c000-ffff is not used */
	ROM_LOAD( "el_ic98_27c512_05.bin",0x10000, 0x10000, CRC(c70d8c13) SHA1(365718725ea7d0355c68ba703b7f9624cb1134bc) )

	ROM_REGION( 0x18000, "audio", 0 )
	ROM_LOAD( "a01.bin",      0x00000, 0x8000, CRC(3d317622) SHA1(ae4e8c5247bc215a2769786cb8639bce2f80db22) )
	ROM_CONTINUE(             0x10000, 0x8000 ) /* banked at 8000-bfff */

	ROM_REGION( 0x010000, "gfx1", ROMREGION_DISPOSE )
	ROM_LOAD( "a06.bin",      0x000000, 0x04000, CRC(3b5387b7) SHA1(b839b4eafe8bf6f9e841e19fee1bdb64a66f3448) )
	ROM_LOAD( "a08.bin",      0x004000, 0x04000, CRC(c622a5a3) SHA1(468c8c24af1f6f244228b66df04cb0ea81c1875e) )
	ROM_LOAD( "a10.bin",      0x008000, 0x04000, CRC(0923d9f6) SHA1(4b10ee3fc17bb63cda51b2a978d066b6a140a551) )
	ROM_LOAD( "a20.bin",      0x00c000, 0x04000, CRC(b8dec83e) SHA1(fe617ddccdd0dbd05ca09a1507074aa14b529322) )

	ROM_REGION( 0x080000, "gfx2", ROMREGION_DISPOSE )
	ROM_LOAD( "a07.bin",      0x000000, 0x20000, CRC(38c31817) SHA1(cb24ed8702d62066366924c033c07ffc78bd1fad) )
	ROM_LOAD( "a09.bin",      0x020000, 0x20000, CRC(32e39e29) SHA1(44f22ed6c983541c7fea5857ba0456aaa87b36d1) )
	ROM_LOAD( "a11.bin",      0x040000, 0x20000, CRC(5ccec796) SHA1(2cc191a4267819eb31962726e2ed4567c825c39e) )
	ROM_LOAD( "a21.bin",      0x060000, 0x20000, CRC(0c54a091) SHA1(3eecb285b5a7bbc310c87492516d7ffb2841aa3b) )

	ROM_REGION( 0x080000, "gfx3", ROMREGION_DISPOSE | ROMREGION_INVERT )
	ROM_LOAD( "146_a12.bin",  0x000000, 0x10000, CRC(d5a60096) SHA1(a8e351a4b020b4fc2b2cb7d3f0fdfb43fc44d7d9) )
	ROM_LOAD( "147_a13.bin",  0x010000, 0x10000, CRC(36bbf467) SHA1(627b5847ffb098c92edfd58c25391799f3b209e0) )
	ROM_LOAD( "148_a14.bin",  0x020000, 0x10000, CRC(26371c18) SHA1(0887041d86dc9f19dad264ae27dc56fb89ac3265) )
	ROM_LOAD( "149_a15.bin",  0x030000, 0x10000, CRC(75aa9b86) SHA1(0c221bd2e8a5472bb0e515f27fb72b0c8e8c0ca4) )
	ROM_LOAD( "150_a16.bin",  0x040000, 0x10000, CRC(0da825f9) SHA1(cfba0c85fc767726c1d63f87468335d1c2f1eed8) )
	ROM_LOAD( "151_a17.bin",  0x050000, 0x10000, CRC(228429d8) SHA1(3b2dbea53807929c24d593c469a83172f7747f66) )
	ROM_LOAD( "152_a18.bin",  0x060000, 0x10000, CRC(516b6c09) SHA1(9d02514dece864b087f67886009ce54bd51b5575) )
	ROM_LOAD( "153_a19.bin",  0x070000, 0x10000, CRC(f36390a9) SHA1(e5ea36e91b3ced068281524ee79d0432f489715c) )

	ROM_REGION( 0x1000, "plds", ROMREGION_DISPOSE )
	ROM_LOAD( "el_ic39_gal16v8_0.bin", 0x0000, 0x0117, NO_DUMP SHA1(894b345b395097acf6cf52ab8bc922099f97a85f) )
	ROM_LOAD( "el_ic44_gal16v8_1.bin", 0x0200, 0x0117, NO_DUMP SHA1(fd41f55d857995fe87217dd9679c42760c241dc4) )
	ROM_LOAD( "el_ic54_gal16v8_2.bin", 0x0400, 0x0117, NO_DUMP SHA1(f6d138fe42549219e11ee8524b05fe3c2b43f5d3) )
	ROM_LOAD( "el_ic100_gal16v8_3.bin", 0x0600, 0x0117, NO_DUMP SHA1(515fcdf378e75ed078f54439fefce8807403bdd5) )
	ROM_LOAD( "el_ic143_gal16v8_4.bin", 0x0800, 0x0117, NO_DUMP SHA1(fbe632437eac2418da7a3c3e947cfd36f6211407) )
ROM_END
#endif


GAME( 1989, wc90b, wc90, wc90b, wc90b, 0, ROT0, "bootleg", "Euro League", GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )
//GAME( 1989, wc90ba, wc90, wc90b, wc90b, 0, ROT0, "bootleg", "Euro League (alt version)", GAME_NO_COCKTAIL | GAME_IMPERFECT_SOUND )
