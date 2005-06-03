/*  Konami NWK-TR */

#include "driver.h"
#include "cpu/powerpc/ppc.h"

static data8_t led_reg0;
static data8_t led_reg1;

#define SHOW_LEDS	0
#define LED_ON		0xff00ff00

#if SHOW_LEDS
static void draw_7segment_led(struct mame_bitmap *bitmap, int x, int y, data8_t value)
{
	plot_box(bitmap, x-1, y-1, 7, 11, 0x00000000);

	/* Top */
	if( (value & 0x40) == 0 ) {
		plot_box(bitmap, x+1, y+0, 3, 1, LED_ON);
	}
	/* Middle */
	if( (value & 0x01) == 0 ) {
		plot_box(bitmap, x+1, y+4, 3, 1, LED_ON);
	}
	/* Bottom */
	if( (value & 0x08) == 0 ) {
		plot_box(bitmap, x+1, y+8, 3, 1, LED_ON);
	}
	/* Top Left */
	if( (value & 0x02) == 0 ) {
		plot_box(bitmap, x+0, y+1, 1, 3, LED_ON);
	}
	/* Top Right */
	if( (value & 0x20) == 0 ) {
		plot_box(bitmap, x+4, y+1, 1, 3, LED_ON);
	}
	/* Bottom Left */
	if( (value & 0x04) == 0 ) {
		plot_box(bitmap, x+0, y+5, 1, 3, LED_ON);
	}
	/* Bottom Right */
	if( (value & 0x10) == 0 ) {
		plot_box(bitmap, x+4, y+5, 1, 3, LED_ON);
	}
}
#endif

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}



/* K001604 Tilemap chip (move to konamiic.c ?) */

static UINT32 *K001604_tile_ram;
static UINT32 *K001604_char_ram;
static UINT8 *K001604_dirty_map;
static int K001604_gfx_index, K001604_char_dirty;
static struct tilemap *K001604_layer[1];

#define K001604_NUM_TILES		16384

static struct GfxLayout K001604_char_layout =
{
	8, 8,
	K001604_NUM_TILES,
	8,
	{ 8,9,10,11,12,13,14,15 },
	{ 1*16, 0*16, 3*16, 2*16, 5*16, 4*16, 7*16, 6*16 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128 },
	8*128
};

static void K001604_tile_info(int tile_index)
{
	UINT32 val = K001604_tile_ram[tile_index];
	int color = val >> 16;
	int tile = val & 0xffff;
	SET_TILE_INFO(K001604_gfx_index, tile, color/2, 0);
}

int K001604_vh_start(void)
{
	int width;
	const char *gamename = Machine->gamedrv->name;

	/* HACK !!! To be removed */
	if( stricmp(gamename, "thrilld") == 0 || stricmp(gamename, "gticlub") == 0 )
		width = 256;
	else
		width = 128;

	for(K001604_gfx_index = 0; K001604_gfx_index < MAX_GFX_ELEMENTS; K001604_gfx_index++)
		if (Machine->gfx[K001604_gfx_index] == 0)
			break;
	if(K001604_gfx_index == MAX_GFX_ELEMENTS)
		return 1;

	K001604_char_ram = auto_malloc(0x400000);
	if( !K001604_char_ram )
		return 1;

	K001604_tile_ram = auto_malloc(0x20000);
	if( !K001604_tile_ram )
		return 1;

	K001604_dirty_map = auto_malloc(K001604_NUM_TILES);
	if( !K001604_dirty_map ) {
		free(K001604_char_ram);
		free(K001604_tile_ram);
		return 1;
	}

	K001604_layer[0] = tilemap_create(K001604_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8, 8, width, 64);

	if( !K001604_layer[0] ) {
		free(K001604_dirty_map);
		free(K001604_tile_ram);
		free(K001604_char_ram);
	}

	tilemap_set_transparent_pen(K001604_layer[0], 0);

	memset(K001604_char_ram, 0, 0x400000);
	memset(K001604_tile_ram, 0, 0x10000);
	memset(K001604_dirty_map, 0, K001604_NUM_TILES);

	Machine->gfx[K001604_gfx_index] = decodegfx((UINT8*)K001604_char_ram, &K001604_char_layout);
	if( !Machine->gfx[K001604_gfx_index] ) {
		free(K001604_dirty_map);
		free(K001604_tile_ram);
		free(K001604_char_ram);
	}

	if (Machine->drv->color_table_len)
	{
		Machine->gfx[K001604_gfx_index]->colortable = Machine->remapped_colortable;
		Machine->gfx[K001604_gfx_index]->total_colors = Machine->drv->color_table_len / 16;
	}
	else
	{
		Machine->gfx[K001604_gfx_index]->colortable = Machine->pens;
		Machine->gfx[K001604_gfx_index]->total_colors = Machine->drv->total_colors / 16;
	}

	return 0;
}

void K001604_tile_update(void)
{
	if(K001604_char_dirty) {
		int i;
		for(i=0; i<K001604_NUM_TILES; i++) {
			if(K001604_dirty_map[i]) {
				K001604_dirty_map[i] = 0;
				decodechar(Machine->gfx[K001604_gfx_index], i, (UINT8 *)K001604_char_ram, &K001604_char_layout);
			}
		}
		tilemap_mark_all_tiles_dirty(K001604_layer[0]);
		K001604_char_dirty = 0;
	}
}

void K001604_tile_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	tilemap_draw(bitmap, cliprect, K001604_layer[0], 0,0);
}

static UINT32 K001604_reg[256];

READ32_HANDLER(K001604_tile_r)
{
	return K001604_tile_ram[offset];
}

READ32_HANDLER(K001604_char_r)
{
	int set, bank;
	UINT32 addr;

	set = (K001604_reg[0x60/4] & 0x1000000) ? 0x100000 : 0;

	if( set ) {
		bank = (K001604_reg[0x60/4] >> 8) & 0x3;
	} else {
		bank = (K001604_reg[0x60/4] & 0x3);
	}

	addr = offset + ((set + (bank * 0x40000)) / 4);

	return K001604_char_ram[addr];
}

WRITE32_HANDLER(K001604_tile_w)
{
	COMBINE_DATA(K001604_tile_ram + offset);
	tilemap_mark_tile_dirty(K001604_layer[0], offset);
}

WRITE32_HANDLER(K001604_char_w)
{
	int set, bank;
	UINT32 addr;

	set = (K001604_reg[0x60/4] & 0x1000000) ? 0x100000 : 0;

	if( set ) {
		bank = (K001604_reg[0x60/4] >> 8) & 0x3;
	} else {
		bank = (K001604_reg[0x60/4] & 0x3);
	}

	addr = offset + ((set + (bank * 0x40000)) / 4);

	COMBINE_DATA(K001604_char_ram + addr);
	K001604_dirty_map[addr / 32] = 1;
	K001604_char_dirty = 1;
}



WRITE32_HANDLER(K001604_reg_w)
{
	COMBINE_DATA(K001604_reg + offset);
}

READ32_HANDLER(K001604_reg_r)
{
	return K001604_reg[offset];
}






VIDEO_START( nwktr )
{
	return K001604_vh_start();
}


VIDEO_UPDATE( nwktr )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	K001604_tile_update();
	K001604_tile_draw(bitmap, cliprect);

#if SHOW_LEDS
	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
#endif
}

/******************************************************************/

static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if( offset == 0 ) {
		if( mem_mask == 0x00ffffff )
			r |= 0 << 24;
		if( mem_mask == 0xffff00ff )
			r |= 0 << 8;
		return r;
	}
	if( offset == 1 ) {
		if( mem_mask == 0x00ffffff )	/* Dip switches */
			r |= 0x00 << 24;			/* 0x80 = test mode */
		return r;
	}
	return 0;
}

static WRITE32_HANDLER( sysreg_w )
{
	if( offset == 0 ) {
		if( mem_mask == 0x00ffffff )
			led_reg0 = (data >> 24) & 0xff;
		if( mem_mask == 0xff00ffff )
			led_reg1 = (data >> 16) & 0xff;
		return;
	}
	if( offset == 1 )
		return;
}

static READ32_HANDLER( rtc_r )
{
	return 0;
}

static WRITE32_HANDLER( rtc_w )
{

}

static READ32_HANDLER( sound_r )
{
	return 0x005f0000;
}

static data32_t video_reg = 0;

static READ32_HANDLER( video_r )
{
	video_reg ^= 0x80;
	return video_reg;
}

/******************************************************************/

static ADDRESS_MAP_START( nwktr_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(3)
	AM_RANGE(0x74000000, 0x740000ff) AM_READWRITE(K001604_reg_r, K001604_reg_w)
	AM_RANGE(0x74010000, 0x74017fff) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_READWRITE(K001604_tile_r, K001604_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_READWRITE(K001604_char_r, K001604_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_RAM
	AM_RANGE(0x780c0000, 0x780c0003) AM_READ(video_r)
	AM_RANGE(0x7d000000, 0x7d00ffff) AM_READ(sysreg_r)
	AM_RANGE(0x7d010000, 0x7d01ffff) AM_WRITE(sysreg_w)
	AM_RANGE(0x7d020000, 0x7d021fff) AM_READWRITE( rtc_r, rtc_w )
	AM_RANGE(0x7d030000, 0x7d030003) AM_READ(sound_r)
	AM_RANGE(0x7d050000, 0x7d05ffff) AM_RAM				/* LAN BOARD ? */
	AM_RANGE(0x7e000000, 0x7e3fffff) AM_ROM AM_REGION(REGION_USER2, 0) AM_SHARE(4)
	AM_RANGE(0x7f000000, 0x7f1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM	AM_SHARE(3)	/* Work RAM */
	AM_RANGE(0xfe000000, 0xfe3fffff) AM_ROM AM_SHARE(4)
	AM_RANGE(0xff000000, 0xff1fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END



/**********************************************************************/

/********************************************************************/



INPUT_PORTS_START( nwktr )
INPUT_PORTS_END

static ppc_config nwktr_ppc_cfg =
{
	PPC_MODEL_403GA
};

static MACHINE_DRIVER_START( nwktr )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(nwktr_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(nwktr_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(nwktr)
	MDRV_VIDEO_UPDATE(nwktr)

MACHINE_DRIVER_END

/*************************************************************************/

ROM_START(racingj)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD16_WORD_SWAP("676nc01.bin", 0x000000, 0x200000, CRC(690346b5) SHA1(157ab6788382ef4f5a8772f08819f54d0856fcc8))

	ROM_REGION(0x400000, REGION_USER2, 0)		/* Data roms */
	ROM_LOAD32_WORD("676a05.bin", 0x000000, 0x200000, CRC(fb4de1ad) SHA1(f6aa4eb1b5d22901a2aaf899ed3237a9dfdc55b5))
	ROM_LOAD32_WORD("676a04.bin", 0x000002, 0x200000, CRC(d7808cb6) SHA1(0668fae5bb94cc120fe196d4b18200f7b512317f))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program roms */
        ROM_LOAD( "676gna08.7s", 0x000000, 0x080000, CRC(8973f6f2) SHA1(f5648a7e0205f7e979ccacbb52936809ce14a184) )

	ROM_REGION(0x1000000, REGION_USER3, 0) 		/* other roms (textures?) */
        ROM_LOAD( "676a09.16p",   0x000000, 0x400000, CRC(f85c8dc6) SHA1(8b302c80be309b5cc68b75945fcd7b87a56a4c9b) )
        ROM_LOAD( "676a10.14p",   0x400000, 0x400000, CRC(7b5b7828) SHA1(aec224d62e4b1e8fdb929d7947ce70d84ba676cf) )
        ROM_LOAD( "676a13.8x",    0x800000, 0x400000, CRC(29077763) SHA1(ee087ca0d41966ca0fd10727055bb1dcd05a0873) )
        ROM_LOAD( "676a14.16x",   0xc00000, 0x400000, CRC(50a7e3c0) SHA1(7468a66111a3ddf7c043cd400fa175cae5f65632) )

ROM_END

ROM_START(racingj2)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD16_WORD_SWAP("888a01.27p", 0x000000, 0x200000, CRC(d077890a) SHA1(08b252324cf46fbcdb95e8f9312287920cd87c5d))

	ROM_REGION(0x400000, REGION_USER2, 0)		/* Data roms */
	ROM_LOAD32_WORD("676a05.bin", 0x000000, 0x200000, CRC(fb4de1ad) SHA1(f6aa4eb1b5d22901a2aaf899ed3237a9dfdc55b5))
	ROM_LOAD32_WORD("676a04.bin", 0x000002, 0x200000, CRC(d7808cb6) SHA1(0668fae5bb94cc120fe196d4b18200f7b512317f))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program roms */
        ROM_LOAD( "888a08.7s",    0x000000, 0x080000, CRC(55fbea65) SHA1(ad953f758181731efccadcabc4326e6634c359e8) )

	ROM_REGION(0x1200000, REGION_USER3, 0) 		/* other roms (textures?) */
        ROM_LOAD( "888a06.12t",   0x000000, 0x200000, CRC(00cbec4d) SHA1(1ce7807d86e90edbf4eecba462a27c725f5ad862) )
        ROM_LOAD( "888a09.16p",   0x200000, 0x400000, CRC(11e2fed2) SHA1(24b8a367b59fedb62c56f066342f2fa87b135fc5) )
        ROM_LOAD( "888a10.14p",   0x600000, 0x400000, CRC(328ce610) SHA1(dbbc779a1890c53298c0db129d496df048929496) )
        ROM_LOAD( "888a13.8x",    0xa00000, 0x400000, CRC(2292f530) SHA1(0f4d1332708fd5366a065e0a928cc9610558b42d) )
        ROM_LOAD( "888a14.16x",   0xe00000, 0x400000, CRC(6a834a26) SHA1(d1fbd7ae6afd05f0edac4efde12a5a45aa2bc7df) )
ROM_END

ROM_START(thrilld)
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD16_WORD_SWAP("713be01.27p", 0x000000, 0x200000, CRC(d84a7723) SHA1(f4e9e08591b7e5e8419266dbe744d56a185384ed))

	ROM_REGION(0x400000, REGION_USER2, 0)		/* Data roms */
	ROM_LOAD32_WORD("713a05.14t", 0x000000, 0x200000, CRC(6f1e6802) SHA1(91f8a170327e9b4ee6a64aee0c106b981a317e69))
	ROM_LOAD32_WORD("713a04.16t", 0x000002, 0x200000, CRC(c994aaa8) SHA1(d82b9930a11e5384ad583684a27c95beec03cd5a))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program roms */
        ROM_LOAD( "713a08.7s",    0x000000, 0x080000, CRC(6a72a825) SHA1(abeac99c5343efacabcb0cdff6d34f9f967024db) )

	ROM_REGION(0x1000000, REGION_USER3, 0) 		/* other roms (textures?) */
        ROM_LOAD( "713a09.16p",   0x000000, 0x400000, CRC(058f250a) SHA1(63b8e60004ec49009633e86b4992c00083def9a8) )
        ROM_LOAD( "713a10.14p",   0x400000, 0x400000, CRC(27f9833e) SHA1(1540f00d2571ecb81b914c553682b67fca94bbbd) )
        ROM_LOAD( "713a13.8x",    0x800000, 0x400000, CRC(b795c66b) SHA1(6e50de0d5cc444ffaa0fec7ada8c07f643374bb2) )
        ROM_LOAD( "713a14.16x",   0xc00000, 0x400000, CRC(5275a629) SHA1(16fadef06975f0f3625cac8f84e2e77ed7d75e15) )
ROM_END

/*************************************************************************/

GAMEX( 1998, racingj,	0,		nwktr,	nwktr,	0,		ROT0,	"Konami",	"Racing Jam", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1999, racingj2,	racingj,nwktr,	nwktr,	0,		ROT0,	"Konami",	"Racing Jam: Chapter 2", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1998, thrilld,	0,		nwktr,	nwktr,	0,		ROT0,	"Konami",	"Thrill Drive", GAME_NOT_WORKING|GAME_NO_SOUND )
