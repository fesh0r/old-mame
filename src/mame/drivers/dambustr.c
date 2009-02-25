/***************************************************************************

Dambusters
(c) 1981 South West Research Ltd. (Bristol, UK)

Reverse-engineering and MAME Driver by Norbert Kehrer (August 2006)

2008-08
Dip locations verified with manual

***************************************************************************/


#include "driver.h"

#include "cpu/z80/z80.h"
#include "includes/galaxian.h"
#include "galaxold.h"


static int noise_data = 0;


static WRITE8_HANDLER( dambustr_noise_enable_w )
{
	if (data != noise_data) {
		noise_data = data;
		galaxian_noise_enable_w(space, offset, data);
	}
}


static ADDRESS_MAP_START( dambustr_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(SMH_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(SMH_RAM)
	AM_RANGE(0xd000, 0xd3ff) AM_READ(SMH_RAM)
	AM_RANGE(0xd400, 0xd7ff) AM_READ(galaxold_videoram_r)
	AM_RANGE(0xd800, 0xd8ff) AM_READ(SMH_RAM)
	AM_RANGE(0xe000, 0xe000) AM_READ_PORT("IN0")
	AM_RANGE(0xe800, 0xefff) AM_READ_PORT("IN1")
	AM_RANGE(0xf000, 0xf7ff) AM_READ_PORT("DSW")
	AM_RANGE(0xf800, 0xffff) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( dambustr_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(SMH_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(SMH_RAM)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(dambustr_bg_color_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(dambustr_bg_split_line_w)
	AM_RANGE(0xd000, 0xd3ff) AM_WRITE(galaxold_videoram_w) AM_BASE(&galaxold_videoram)
	AM_RANGE(0xd800, 0xd83f) AM_WRITE(galaxold_attributesram_w) AM_BASE(&galaxold_attributesram)
	AM_RANGE(0xd840, 0xd85f) AM_WRITE(SMH_RAM) AM_BASE(&galaxold_spriteram) AM_SIZE(&galaxold_spriteram_size)
	AM_RANGE(0xd860, 0xd87f) AM_WRITE(SMH_RAM) AM_BASE(&galaxold_bulletsram) AM_SIZE(&galaxold_bulletsram_size)
	AM_RANGE(0xd880, 0xd8ff) AM_WRITE(SMH_RAM)
	AM_RANGE(0xe002, 0xe003) AM_WRITE(galaxold_coin_counter_w)
	AM_RANGE(0xe004, 0xe007) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0xe800, 0xe802) AM_WRITE(galaxian_background_enable_w)
	AM_RANGE(0xe803, 0xe803) AM_WRITE(dambustr_noise_enable_w)
	AM_RANGE(0xe804, 0xe804) AM_WRITE(galaxian_shoot_enable_w)	// probably louder than normal shot
	AM_RANGE(0xe805, 0xe805) AM_WRITE(galaxian_shoot_enable_w)	// normal shot (like Galaxian)
	AM_RANGE(0xe806, 0xe807) AM_WRITE(galaxian_vol_w)
	AM_RANGE(0xf001, 0xf001) AM_WRITE(galaxold_nmi_enable_w)
	AM_RANGE(0xf004, 0xf004) AM_WRITE(galaxold_stars_enable_w)
	AM_RANGE(0xf006, 0xf006) AM_WRITE(galaxold_flip_screen_x_w)
	AM_RANGE(0xf007, 0xf007) AM_WRITE(galaxold_flip_screen_y_w)
	AM_RANGE(0xf800, 0xf800) AM_WRITE(galaxian_pitch_w)
ADDRESS_MAP_END


static INPUT_PORTS_START( dambustr )
	PORT_START("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_DIPNAME( 0x20, 0x00, "Clear Swear Words" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_SERVICE( 0x40, IP_ACTIVE_HIGH )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON3 )

	PORT_START("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_DIPNAME( 0x40, 0x00, "2nd Coin Counter" ) PORT_DIPLOCATION("SW:!1")
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Coinage ) ) PORT_DIPLOCATION("SW:!2")
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_START("DSW")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) ) PORT_DIPLOCATION("SW:!3,!4")
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, "Game Test Mode" ) PORT_DIPLOCATION("SW:!5") /* Manual says must be OFF */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, "Union Jack" ) PORT_DIPLOCATION("SW:!6")
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


static const gfx_layout dambustr_charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};


static const gfx_layout dambustr_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};


static GFXDECODE_START( dambustr )
	GFXDECODE_ENTRY( "gfx1", 0x0000, dambustr_charlayout,   0, 8 )
	GFXDECODE_ENTRY( "gfx1", 0x0000, dambustr_spritelayout, 0, 8 )
GFXDECODE_END


static DRIVER_INIT(dambustr)
{
	int i, j, tmp;
	int tmpram[16];
	UINT8 *rom = memory_region(machine, "maincpu");
	UINT8 *usr = memory_region(machine, "user1");
	UINT8 *gfx = memory_region(machine, "gfx1");

	// Bit swap addresses
	for(i=0; i<4096*4; i++) {
		rom[i] = usr[BITSWAP16(i,15,14,13,12, 4,10,9,8,7,6,5,3,11,2,1,0)];
	};

	// Swap program ROMs
	for(i=0; i<0x1000; i++) {
		tmp = rom[0x5000+i];
		rom[0x5000+i] = rom[0x6000+i];
		rom[0x6000+i] = rom[0x1000+i];
		rom[0x1000+i] = tmp;
	};

	// Bit swap in $1000-$1fff and $4000-$5fff
	for(i=0; i<0x1000; i++) {
		rom[0x1000+i] =	BITSWAP8(rom[0x1000+i],7,6,5,1,3,2,4,0);
		rom[0x4000+i] =	BITSWAP8(rom[0x4000+i],7,6,5,1,3,2,4,0);
		rom[0x5000+i] =	BITSWAP8(rom[0x5000+i],7,6,5,1,3,2,4,0);
	};

	// Swap graphics ROMs
	for(i=0;i<0x4000;i+=16)	{
		for(j=0; j<16; j++)
			tmpram[j] = gfx[i+j];
		for(j=0; j<8; j++) {
			gfx[i+j] = tmpram[j*2];
			gfx[i+j+8] = tmpram[j*2+1];
		};
	};
}



static MACHINE_DRIVER_START( dambustr )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 18432000/6)	/* 3.072 MHz */
	MDRV_CPU_PROGRAM_MAP(dambustr_readmem, dambustr_writemem)

	MDRV_MACHINE_RESET(galaxold)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(16000.0/132/2)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)

	MDRV_GFXDECODE(dambustr)
	MDRV_PALETTE_LENGTH(32+2+64+8)		/* 32 for the characters, 2 for the bullets, 64 for the stars, 8 for the background */

	MDRV_PALETTE_INIT(dambustr)
	MDRV_VIDEO_START(dambustr)
	MDRV_VIDEO_UPDATE(dambustr)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	/* sound hardware */
	MDRV_SOUND_ADD("samples", SAMPLES, 0)
	MDRV_SOUND_CONFIG(galaxian_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


ROM_START( dambustr )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "db8a",   0x4000, 0x1000, CRC(fd041ff4) SHA1(8d27da7bf0c655633711b960cbc23950c8a371ae) )
	ROM_LOAD( "db6a",   0x5000, 0x1000, CRC(448db54b) SHA1(c9afbf02bf4d4ac2972ab7ac6adfa4e951ae79c2) )
	ROM_LOAD( "db7a",   0x6000, 0x1000, CRC(675b1f5e) SHA1(6a386212a640fb467b6956a4dc5a68476af1cf97) )
	ROM_LOAD( "db5a",   0x7000, 0x1000, CRC(75659ecc) SHA1(b61254fb12f3999607abd88d1cc649dcfbf0384c) )

	ROM_REGION( 0x10000,"user1",0)
 	ROM_LOAD( "db11a",   0x0000, 0x1000, CRC(427bd3fb) SHA1(cdbaef4040fa2e0598a086e320d51ecb26a591dd) )
	ROM_LOAD( "db9a",    0x1000, 0x1000, CRC(57164563) SHA1(8471d0660f39511d0afa3cdd63a1e84b0ea80fd0) )
	ROM_LOAD( "db10a",   0x2000, 0x1000, CRC(075b9c5e) SHA1(ff6ce873897004c0e796813725e260df85a520f9) )
	ROM_LOAD( "db12a",   0x3000, 0x1000, CRC(ed01a68b) SHA1(9dd37c2a25865717a7acdd7e2a3bef26a4cef3d9) )

	ROM_REGION( 0x4000, "gfx1", ROMREGION_DISPOSE )
	ROM_LOAD( "db1a",   0x1000, 0x1000, CRC(4cb964cd) SHA1(1c90b14deb201a64b8ed4378b022e9e4574aed94) )
	ROM_LOAD( "db2a",   0x3000, 0x1000, CRC(0a0a6af5) SHA1(ecd2a6696ce9154f030c830ccb45690787881a73) )
	ROM_LOAD( "db3a",   0x0000, 0x1000, CRC(9e9a9710) SHA1(a9f67a05a2882b9f6f3378cc73e90539de4b8ca4) )
	ROM_LOAD( "db4a",   0x2000, 0x1000, CRC(d9d2df33) SHA1(97057fe33c146898755b556558ff707b9f4551ec) )

	// This ROM needs to be dumped, only a guess at the moment:
	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD( "dambustr.clr", 0x0000, 0x0020, BAD_DUMP CRC(cda7b558) SHA1(2977967917970dffa91e69db19c62ba8ff6c3053) )
ROM_END


ROM_START( dambust )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "db08.bin",   0x4000, 0x1000, CRC(fd041ff4) SHA1(8d27da7bf0c655633711b960cbc23950c8a371ae) )
	ROM_LOAD( "db06p.bin",  0x5000, 0x1000, CRC(35dcee01) SHA1(2c23c727d9b38322a6d0548dfe6a2a254f3530af) )
	ROM_LOAD( "db07.bin",   0x6000, 0x1000, CRC(675b1f5e) SHA1(6a386212a640fb467b6956a4dc5a68476af1cf97) )
	ROM_LOAD( "db05.bin",   0x7000, 0x1000, CRC(75659ecc) SHA1(b61254fb12f3999607abd88d1cc649dcfbf0384c) )

	ROM_REGION( 0x10000,"user1",0)
 	ROM_LOAD( "db11.bin",   0x0000, 0x1000, CRC(9e6b34fe) SHA1(5cf47f5a5280ac53490240df220edf6178e87f4f) )
	ROM_LOAD( "db09.bin",   0x1000, 0x1000, CRC(57164563) SHA1(8471d0660f39511d0afa3cdd63a1e84b0ea80fd0) )
	ROM_LOAD( "db10p.bin",  0x2000, 0x1000, CRC(c129c57b) SHA1(c25abd7ee97b71941d9fa6acd0d92c116f1ff408) )
	ROM_LOAD( "db12.bin",   0x3000, 0x1000, CRC(ea4c65f5) SHA1(cb761e0543cacd6b437c6e88615f97df83245a34) )

	ROM_REGION( 0x4000, "gfx1", ROMREGION_DISPOSE )
	ROM_LOAD( "db1ap.bin",  0x1000, 0x1000, CRC(4cb964cd) SHA1(1c90b14deb201a64b8ed4378b022e9e4574aed94) )
	ROM_LOAD( "db02.bin",   0x3000, 0x1000, CRC(0a0a6af5) SHA1(ecd2a6696ce9154f030c830ccb45690787881a73) )
	ROM_LOAD( "db03.bin",   0x0000, 0x1000, CRC(9e9a9710) SHA1(a9f67a05a2882b9f6f3378cc73e90539de4b8ca4) )
	ROM_LOAD( "db04.bin",   0x2000, 0x1000, CRC(d9d2df33) SHA1(97057fe33c146898755b556558ff707b9f4551ec) )

	// This ROM needs to be dumped, only a guess at the moment:
	ROM_REGION( 0x0020, "proms", 0 )
	ROM_LOAD( "dambustr.clr", 0x0000, 0x0020, BAD_DUMP CRC(cda7b558) SHA1(2977967917970dffa91e69db19c62ba8ff6c3053) )
ROM_END


GAME( 1981, dambustr, 0,        dambustr, dambustr, dambustr, ROT90, "South West Research", "Dambusters (US)", 0 )
GAME( 1981, dambust,  dambustr, dambustr, dambustr, dambustr, ROT90, "South West Research", "Dambusters (UK)", 0 )
