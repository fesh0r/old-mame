/* Bingo Roll */

#include "driver.h"
#include "cpu/i86/i86.h"
#include "cpu/pic16c5x/pic16c5x.h"
#include "sound/saa1099.h"

static UINT16 *blit_ram;

static VIDEO_START(bingor)
{

}

static VIDEO_UPDATE(bingor)
{
	int x,y,count;

	bitmap_fill(bitmap,cliprect,get_black_pen(screen->machine));

	count = (0x2000/2);

	for(y=0;y<256;y++)
	{
		for(x=0;x<286;x+=4)
		{
			UINT32 color;

			color = (blit_ram[count] & 0xf000)>>12;

			if((x+3)<video_screen_get_visible_area(screen)->max_x && ((y)+0)<video_screen_get_visible_area(screen)->max_y)
				*BITMAP_ADDR32(bitmap, y, x+3) = screen->machine->pens[color];

			color = (blit_ram[count] & 0x0f00)>>8;

			if((x+2)<video_screen_get_visible_area(screen)->max_x && ((y)+0)<video_screen_get_visible_area(screen)->max_y)
				*BITMAP_ADDR32(bitmap, y, x+2) = screen->machine->pens[color];

			color = (blit_ram[count] & 0x00f0)>>4;

			if((x+1)<video_screen_get_visible_area(screen)->max_x && ((y)+0)<video_screen_get_visible_area(screen)->max_y)
				*BITMAP_ADDR32(bitmap, y, x+1) = screen->machine->pens[color];

			color = (blit_ram[count] & 0x000f)>>0;

			if((x+0)<video_screen_get_visible_area(screen)->max_x && ((y)+0)<video_screen_get_visible_area(screen)->max_y)
				*BITMAP_ADDR32(bitmap, y, x+0) = screen->machine->pens[color];

			count++;
		}
	}

	return 0;
}

#if 0
static READ16_HANDLER( test_r )
{
	return mame_rand(space->machine);
}
#endif

static ADDRESS_MAP_START( bingor_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x90000, 0x9ffff) AM_ROM AM_REGION("gfx", 0)
	AM_RANGE(0xa0300, 0xa031f) AM_RAM_WRITE(paletteram16_RRRRGGGGBBBBIIII_word_w) AM_BASE(&paletteram16) //wrong
	AM_RANGE(0xa0000, 0xaffff) AM_RAM AM_BASE(&blit_ram)
	AM_RANGE(0xf0000, 0xfffff) AM_ROM AM_REGION("boot_prg",0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bingor_io, ADDRESS_SPACE_IO, 16 )
//  AM_RANGE(0x0000, 0x00ff) AM_READ( test_r )
	AM_RANGE(0x0100, 0x0101) AM_DEVWRITE8("saa", saa1099_data_w, 0x00ff)
	AM_RANGE(0x0102, 0x0103) AM_DEVWRITE8("saa", saa1099_control_w, 0x00ff)
//  AM_RANGE(0x0200, 0x0201) AM_READ( test_r )
ADDRESS_MAP_END

static READ8_HANDLER( test8_r )
{
	return mame_rand(space->machine);
}

static ADDRESS_MAP_START( pic_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_READ(test8_r)
	AM_RANGE(0x10, 0x10) AM_READNOP
ADDRESS_MAP_END

static INPUT_PORTS_START( bingor )
	PORT_START("IN0")
	PORT_DIPNAME( 0x0001, 0x0001, "IN0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static INTERRUPT_GEN( vblank_irq )
{
//  cpu_set_input_line_and_vector(device,0,HOLD_LINE,0x08/4); // reads i/o 0x200 and puts the result in ram, pic irq?
	cpu_set_input_line_and_vector(device,0,HOLD_LINE,0x4c/4); // ?
}

static INTERRUPT_GEN( unk_irq )
{
	cpu_set_input_line_and_vector(device,0,HOLD_LINE,0x48/4); // ?
}


static const gfx_layout bingor_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0,4,8,12,16,20,24,28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};

static GFXDECODE_START( bingor )
	GFXDECODE_ENTRY( "gfx", 0, bingor_layout,   0x0, 2  )
GFXDECODE_END


static MACHINE_DRIVER_START( bingor )
	MDRV_CPU_ADD("maincpu", I80186, 14000000 ) //?? Mhz
	MDRV_CPU_PROGRAM_MAP(bingor_map)
	MDRV_CPU_IO_MAP(bingor_io)
	MDRV_CPU_VBLANK_INT("screen", vblank_irq)
	MDRV_CPU_PERIODIC_INT(nmi_line_pulse, 30)
	MDRV_CPU_PERIODIC_INT(unk_irq, 30)

	MDRV_CPU_ADD("pic", PIC16C57, 12000000) //?? Mhz
	MDRV_CPU_IO_MAP(pic_io_map)


	MDRV_GFXDECODE(bingor)
	//MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_SCREEN_VISIBLE_AREA(0, 400-1, 0, 300-1)

	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(bingor)
	MDRV_VIDEO_UPDATE(bingor)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("saa", SAA1099, 6000000 )
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

// I doubt we need to load the eeproms

ROM_START( bingor1 )
	ROM_REGION( 0x10000, "boot_prg", 0 ) /* i186 code */
	ROM_LOAD16_BYTE( "bingo v 29.4.99 l.bin", 0x000000, 0x08000, CRC(b6773bff) SHA1(74e375662730e002e05186bd77098fa0d8e43ade) )
	ROM_LOAD16_BYTE( "bingo v 29.4.99 h.bin", 0x000001, 0x08000, CRC(0e18f90a) SHA1(0743302e675f01f8ad42ac2e67ecb1c1bf870ae7) )

	// gfx roms on this one are twice the size of the others
	ROM_REGION( 0x20000, "gfx", 0 ) /* blitter data? */
	ROM_LOAD16_BYTE( "bingo turbo l.bin", 0x000000, 0x10000, CRC(86b10566) SHA1(5f74b250ced3574feffdc40b6ed013ec5a0c2c97) )
	ROM_LOAD16_BYTE( "bingo turbo h.bin", 0x000001, 0x10000, CRC(7e18f9d7) SHA1(519b65d6812a3762e3215f4918c834d5a444b28a) )

	ROM_REGION( 0x20000, "pic", 0 ) /* protection? */
	ROM_LOAD( "pic16c54b.bin", 0x000, 0x200, CRC(21e8a699) SHA1(8a22292fa3669105d52a9d681d5be345fcfe6607) )

	ROM_REGION( 0x20000, "eeprom", 0 ) /* eeprom */
	ROM_LOAD( "bingor1_24c04a.bin", 0x000000, 0x200, CRC(b169df46) SHA1(ebafc81c6918aae9daa6b90df16161751cfd2590) )
ROM_END

ROM_START( bingor2 )
	ROM_REGION( 0x10000, "boot_prg", 0 ) /* i186 code */
	ROM_LOAD16_BYTE( "bingo roll vip2 v26.02.02_l.bin", 0x000000, 0x08000, CRC(aa464ef9) SHA1(e74e60396478d7a6556b0d16c4d8c0acefa8faad) )
	ROM_LOAD16_BYTE( "bingo roll vip2 v26.02.02_h.bin", 0x000001, 0x08000, CRC(02816885) SHA1(b98b527a72847412479b6b8e153f236757d9eb4e) )

	ROM_REGION( 0x20000, "gfx", 0 ) /* blitter data? */
	ROM_LOAD16_BYTE( "bingo roll grafik l.bin", 0x000000, 0x10000, CRC(3e753e13) SHA1(011b5f530e54332be194830c0a1d2ec31425017a) )
	ROM_LOAD16_BYTE( "bingo roll grafik h.bin", 0x000001, 0x10000, CRC(4eec39ad) SHA1(4201d5ec207d30dcac9813dd6866d2b61c168e75) )

	ROM_REGION( 0x20000, "pic", 0 ) /* protection? */
	ROM_LOAD( "pic16c54c.bin", 0x000, 0x200, CRC(21e8a699) SHA1(8a22292fa3669105d52a9d681d5be345fcfe6607) )

	ROM_REGION( 0x20000, "eeprom", 0 ) /* eeprom */
	ROM_LOAD( "bingor2_24c04a.bin", 0x000000, 0x200, CRC(a7c87036) SHA1(f7d6161bbfdcdc50212f6b71eb2cbbbb18548cc6) )
ROM_END

ROM_START( bingor3 )
	ROM_REGION( 0x10000, "boot_prg", 0 ) /* i186 code */
	ROM_LOAD16_BYTE( "bellstar vip2l 27.07_1.bin", 0x000000, 0x08000, CRC(0115bca7) SHA1(0b692b46bc6641296861666f00ec0475dc7296a1) )
	ROM_LOAD16_BYTE( "bellstar vip2l 27.07_2.bin", 0x000001, 0x08000, CRC(c689aa69) SHA1(fb1f477654909f156c30a6be29f84962f4edb1c3) )

	ROM_REGION( 0x10000, "gfx", 0 ) /* blitter data? */
	ROM_LOAD16_BYTE( "bsg-11.10.02_l.bin", 0x000000, 0x08000, CRC(a8b22477) SHA1(92d638f0f188a43f14487989cf42195311fb2c35) ) //half size?
	ROM_LOAD16_BYTE( "bsg-11.10.02_h.bin", 0x000001, 0x08000, CRC(969d201c) SHA1(7705ceb383ef122538ebf8046041d1c24ec9b9a4) )

	ROM_REGION( 0x20000, "pic", 0 ) /* protection? */
	ROM_LOAD( "pic16c54c.bin", 0x000, 0x400, CRC(5a507be6) SHA1(f4fbfb7e7516eecab32d96b3a34ad88395edac9e) )

	ROM_REGION( 0x20000, "eeprom", 0 ) /* eeprom */
	ROM_LOAD( "bingor3_24c04a.bin", 0x000000, 0x200,  CRC(7a5eb172) SHA1(12d2fc96049427ef1a8acf47242b41b2095d28b6) )
	ROM_LOAD( "bingor3_24c04a_alt.bin", 0x000000, 0x200,  CRC(fcff2d26) SHA1(aec1ddd38149404741a057c74bf84bfb4a8e4aa1) )
ROM_END

// this is a mix of 2 of the other sets.. I don't know if it's correct
ROM_START( bingor4 )
	ROM_REGION( 0x10000, "boot_prg", 0 ) /* i186 code */
	ROM_LOAD16_BYTE( "01.bin", 0x000000, 0x08000, CRC(0115bca7) SHA1(0b692b46bc6641296861666f00ec0475dc7296a1) )
	ROM_LOAD16_BYTE( "02.bin", 0x000001, 0x08000, CRC(c689aa69) SHA1(fb1f477654909f156c30a6be29f84962f4edb1c3) )

	ROM_REGION( 0x20000, "gfx", 0 ) /* blitter data? */
	ROM_LOAD16_BYTE( "bingo roll grafik l.bin", 0x000000, 0x10000, CRC(3e753e13) SHA1(011b5f530e54332be194830c0a1d2ec31425017a) )
	ROM_LOAD16_BYTE( "bingo roll grafik h.bin", 0x000001, 0x10000, CRC(4eec39ad) SHA1(4201d5ec207d30dcac9813dd6866d2b61c168e75) )

	ROM_REGION( 0x20000, "pic", 0 ) /* protection? */
	ROM_LOAD( "pic16c54c.bin", 0x000, 0x200, CRC(21e8a699) SHA1(8a22292fa3669105d52a9d681d5be345fcfe6607) )

	ROM_REGION( 0x20000, "eeprom", 0 ) /* eeprom */
	ROM_LOAD( "bingor4_24c04a.bin", 0x000000, 0x200,  CRC(38cf70a9) SHA1(ba9a1640200963e2d58d761edc13a24fa5ef44c2) )
ROM_END

GAME( 199?, bingor1,    0,      bingor,   bingor,   0,       ROT0,  "<unknown>", "Bingo Roll / Bellstar? (set 1)",     GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 199?, bingor2,    0,      bingor,   bingor,   0,       ROT0,  "<unknown>", "Bingo Roll / Bellstar? (set 2)",     GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 199?, bingor3,    0,      bingor,   bingor,   0,       ROT0,  "<unknown>", "Bingo Roll / Bellstar? (set 3)",     GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 199?, bingor4,    0,      bingor,   bingor,   0,       ROT0,  "<unknown>", "Bingo Roll / Bellstar? (set 4)",     GAME_NOT_WORKING | GAME_NO_SOUND )
