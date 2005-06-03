/* Dark Mist
(c) Taito / Seibu

Similar HW to Cross Shooter but without GFX in custom blocks?

preliminary driver by Tomasz Slanina

Main CPU : z80 (with encryption, external to z80)
Sound CPU: custom T5182 cpu (like seibu sound system but with internal code)

ToDo:

(almost everything)
-- sort out and decrypt gfx
-- finish memory map
-- where is the banking register (is this why it resets over and over again, or is it a decrypt/irq error?)
-- emulate video hardware
-- hook up inputs
-- simulate sound? (or find a way of dumping t5182 internal rom)

*/

#include "driver.h"
#include "vidhrdw/generic.h"

READ8_HANDLER( darkmist_random_read )
{
//  return 0xff; // no flashing..
	return 0x00;
//  return rand();
}

VIDEO_UPDATE( darkmist)
{
//tilemap_draw(bitmap,cliprect,tilemap,0,0);

	const data8_t *pSource;
	int sx,sy;
	data8_t tile;

	pSource = videoram;
	for( sy=0; sy<256; sy+=8 )
	{
		for( sx=0; sx<256; sx+=8 )
		{
			tile = pSource[0];
			if(tile<32 || tile >90)tile=32;

			pSource ++;
			drawgfx(
				bitmap,Machine->uirotfont,
				tile,
				0,
				0,0,
				sx,sy,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
		}
	}
}

static ADDRESS_MAP_START( memmap, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1) // where is the bank register..
	AM_RANGE(0xc000, 0xc1ff) AM_READ(MRA8_RAM) AM_WRITE(paletteram_BBGGGRRR_w) AM_BASE(&paletteram)	// guess, maybe not
	AM_RANGE(0xc200, 0xc205) AM_READ(darkmist_random_read)//AM_RAM // inputs ?

	AM_RANGE(0xc500, 0xc500) AM_READ(darkmist_random_read)//AM_RAM//AM_WRITE(cshooter_c500_w)
	AM_RANGE(0xc600, 0xc600) AM_READ(darkmist_random_read)//AM_RAM//AM_WRITE(MWA8_NOP)          // see notes
	AM_RANGE(0xc700, 0xc700) AM_READ(darkmist_random_read)//AM_RAM//AM_WRITE(cshooter_c700_w)
	AM_RANGE(0xc800, 0xc80f) AM_READ(darkmist_random_read)//AM_RAM//AM_WRITE(MWA8_NOP)          // see notes

	AM_RANGE(0xd000, 0xd7ff) AM_RAM //txtram
	AM_RANGE(0xd800, 0xdfff) AM_RAM AM_BASE(&videoram)
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END



INPUT_PORTS_START( darkmist )

INPUT_PORTS_END



static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX3, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX4, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX5, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX6, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX7, 0, &tiles8x8_layout, 0, 16 },

	{ -1 }
};

VIDEO_START(darkmist)
{
	return 0;
}


INTERRUPT_GEN( darkmist_interrupt )
{
	if(cpu_getiloops())
	{
		cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x08);
	}
	else
	{
//      cpunum_set_input_line_and_vector(0, 0, HOLD_LINE, 0x10);
	}
}


static MACHINE_DRIVER_START( darkmist )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,4000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(memmap, 0)
	MDRV_CPU_VBLANK_INT(darkmist_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 16, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(darkmist)
	MDRV_VIDEO_UPDATE(darkmist)

MACHINE_DRIVER_END

ROM_START( darkmist )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )
	ROM_LOAD( "dm_15.rom", 0x00000, 0x08000, CRC(21e6503c) SHA1(09174fb424b76f7f2a381297e3420ddd2e76b008)  )
	/* banked data (at least 16 is..)  */
	ROM_LOAD( "dm_16.rom", 0x10000, 0x08000, CRC(094579d9) SHA1(2449bc9ba38396912ee9b72dd870ea9fcff95776)  )
	ROM_LOAD( "dm_17.rom", 0x18000, 0x08000, CRC(7723dcae) SHA1(a0c69e7a7b6fd74f7ed6b9c6419aed94aabcd4b0)  )


	/* I guess these aren't all gfx, but they seem to be encrypted ... */

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "dm_01.rom", 0x00000, 0x10000, CRC(652aee6b) SHA1(f4150784f7bd7be83a0041e4c52540aa564062ba) )
	ROM_LOAD16_BYTE( "dm_02.rom", 0x00001, 0x10000, CRC(e2dd15aa) SHA1(1f3a6a1e1afabfe9dc47549ef13ae7696302ae88)  )

	ROM_REGION( 0x10000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "dm_03.rom", 0x00000, 0x08000, CRC(60b40c2a) SHA1(c046273b15dab95ea4851c26ce941e580fa1b6ec)  )
	ROM_LOAD16_BYTE( "dm_04.rom", 0x00001, 0x08000, CRC(d47b8cd9) SHA1(86eb7a5d8ea63c0c91f455b1b8322cc7b9c4a968)  )

	ROM_REGION( 0x20000, REGION_GFX3, 0 )
	ROM_LOAD16_BYTE( "dm_05.rom", 0x00000, 0x10000, CRC(ca79a738) SHA1(66a76ea0d8ecc44f6cc77102303df74f40bf6118)  )
	ROM_LOAD16_BYTE( "dm_06.rom", 0x00001, 0x10000, CRC(9629ed2c) SHA1(453f6a0b12efdadd7fcbe03ad37afb0afa6be051)  )

	ROM_REGION( 0x08000, REGION_GFX4, 0 )
	ROM_LOAD16_BYTE( "dm_07.rom", 0x00000, 0x04000, CRC(889b1277) SHA1(78405110b9cf1ab988c0cbfdb668498dadb41229)  )
	ROM_LOAD16_BYTE( "dm_08.rom", 0x00001, 0x04000, CRC(f76f6f46) SHA1(ce1c67dc8976106b24fee8d3a0b9e5deb016a327)  )

	ROM_REGION( 0x20000, REGION_GFX5, 0 )
	ROM_LOAD16_BYTE( "dm_09.rom", 0x00000, 0x10000, CRC(52154b50) SHA1(5ee1a4bcf0752a057b9993b0069d744c35cf55f4)  )
	ROM_LOAD16_BYTE( "dm_10.rom", 0x00001, 0x10000, CRC(34fd52b5) SHA1(c4ee464ed79ec91f993b0f894572c0288f0ad1d4)  )

	ROM_REGION( 0x10000, REGION_GFX6, 0 )
	ROM_LOAD16_BYTE( "dm_11.rom", 0x00000, 0x08000, CRC(3118e2f9) SHA1(dfd946ea1310851f97d31ce58d8280f2d92b0f59)  )
	ROM_LOAD16_BYTE( "dm_12.rom", 0x00001, 0x08000, CRC(cc4b9839) SHA1(b7e95513d2e06929fed5005caf3bf8c3fba0b597) )

	ROM_REGION( 0x4000, REGION_GFX7, 0 )
	ROM_LOAD16_BYTE( "dm_13.rom", 0x00000, 0x02000, CRC(38bb38d9) SHA1(d751990166dd3d503c5de7667679b96210061cd1)  )
	ROM_LOAD16_BYTE( "dm_14.rom", 0x00001, 0x02000, CRC(ac5a31f3) SHA1(79083390671062be2eab93cc875a0f86d709a963)  )
ROM_END

static DRIVER_INIT(darkmist)
{
	int i;
	unsigned char *ROM = memory_region(REGION_CPU1);

	/* is this complete? */

	for(i=0;i<0x8000;i++)
	{
		UINT8 p, d;
		p = d = memory_region(REGION_CPU1)[i];

		if(((i & 0x20) == 0x00) && ((i & 0x8) != 0))
			p ^= 0x20;

		if(((i & 0x20) == 0x00) && ((i & 0xa) != 0))
			d ^= 0x20;

		if(((i & 0x200) == 0x200) && ((i & 0x408) != 0))
			p ^= 0x10;

		if((i & 0x220) != 0x200)
		{
			p = BITSWAP8(p, 7,6,5,2,3,4,1,0);
			d = BITSWAP8(d, 7,6,5,2,3,4,1,0);
		}

		memory_region(REGION_CPU1)[i] = d;
		memory_region(REGION_CPU1)[i|0x20000] = p;
	}

	memory_set_opcode_base(0,memory_region(REGION_CPU1)+0x20000);


	cpu_setbank(1,&ROM[0x010000]);

}

GAMEX( 1986, darkmist, 0, darkmist, darkmist, darkmist, ROT270, "Taito", "The Lost Castle In Darkmist", GAME_NOT_WORKING|GAME_NO_SOUND )
