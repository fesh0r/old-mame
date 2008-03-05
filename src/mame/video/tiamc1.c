/***************************************************************************

  TIA-MC1 video hardware

  driver by Eugene Sandulenko
  special thanks to Shiru for his standalone emulator and documentation

***************************************************************************/

#include "driver.h"
#include "deprecat.h"


UINT8 *tiamc1_tileram;
UINT8 *tiamc1_charram;
UINT8 *tiamc1_spriteram_x;
UINT8 *tiamc1_spriteram_y;
UINT8 *tiamc1_spriteram_a;
UINT8 *tiamc1_spriteram_n;
static UINT8 tiamc1_layers_ctrl;
static UINT8 tiamc1_bg_vshift;
static UINT8 tiamc1_bg_hshift;

static tilemap *bg_tilemap1, *bg_tilemap2;
static rgb_t *palette;

WRITE8_HANDLER( tiamc1_videoram_w )
{
	if(!(tiamc1_layers_ctrl & 2))
		tiamc1_charram[offset + 0x0000] = data;
	if(!(tiamc1_layers_ctrl & 4))
		tiamc1_charram[offset + 0x0800] = data;
	if(!(tiamc1_layers_ctrl & 8))
		tiamc1_charram[offset + 0x1000] = data;
	if(!(tiamc1_layers_ctrl & 16))
		tiamc1_charram[offset + 0x1800] = data;

	if((tiamc1_layers_ctrl & 30) ^ 0xff)
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	if(!(tiamc1_layers_ctrl & 1)) {
		tiamc1_tileram[offset] = data;
		if (offset >= 1024)
			tilemap_mark_tile_dirty(bg_tilemap1, offset);
		else
			tilemap_mark_tile_dirty(bg_tilemap2, offset);
	}
}

WRITE8_HANDLER( tiamc1_bankswitch_w )
{
	if ((data & 128) != (tiamc1_layers_ctrl & 128))
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	tiamc1_layers_ctrl = data;
}

WRITE8_HANDLER( tiamc1_sprite_x_w )
{
	tiamc1_spriteram_x[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_y_w )
{
	tiamc1_spriteram_y[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_a_w )
{
	tiamc1_spriteram_a[offset] = data;
}

WRITE8_HANDLER( tiamc1_sprite_n_w )
{
	tiamc1_spriteram_n[offset] = data;
}

WRITE8_HANDLER( tiamc1_bg_vshift_w ) {
	tiamc1_bg_vshift = data;
}
WRITE8_HANDLER( tiamc1_bg_hshift_w ) {
	tiamc1_bg_hshift = data;
}

WRITE8_HANDLER( tiamc1_palette_w )
{
	palette_set_color(Machine, offset, palette[data]);
}

PALETTE_INIT( tiamc1 )
{
	// Voltage computed by Proteus
	//static const float g_v[8]={1.05f,0.87f,0.81f,0.62f,0.44f,0.25f,0.19f,0.00f};
	//static const float r_v[8]={1.37f,1.13f,1.00f,0.75f,0.63f,0.38f,0.25f,0.00f};
	//static const float b_v[4]={1.16f,0.75f,0.42f,0.00f};

	// Voltage adjusted by Shiru
	static const float g_v[8] = { 1.2071f,0.9971f,0.9259f,0.7159f,0.4912f,0.2812f,0.2100f,0.0000f};
	static const float r_v[8] = { 1.5937f,1.3125f,1.1562f,0.8750f,0.7187f,0.4375f,0.2812f,0.0000f};
	static const float b_v[4] = { 1.3523f,0.8750f,0.4773f,0.0000f};

	int col;
	int r, g, b, ir, ig, ib;
	float tcol;

	palette = auto_malloc(256 * sizeof(*palette));

	for (col = 0; col < 256; col++) {
		ir = (col >> 3) & 7;
		ig = col & 7;
		ib = (col >> 6) & 3;
		tcol = 255.0f * r_v[ir] / r_v[0];
		r = 255 - (((int)tcol) & 255);
		tcol = 255.0f * g_v[ig] / g_v[0];
		g = 255 - (((int)tcol) & 255);
		tcol = 255.0f * b_v[ib] / b_v[0];
		b = 255 - (((int)tcol) & 255);

		palette[col] = MAKE_RGB(r,g,b);
	}
}

static TILE_GET_INFO( get_bg1_tile_info )
{
	int code = tiamc1_tileram[tile_index];

	decodechar(machine->gfx[0], code, tiamc1_charram);

	SET_TILE_INFO(0, code, 0, 0);
}

static TILE_GET_INFO( get_bg2_tile_info )
{
	int code = tiamc1_tileram[tile_index + 1024];

	decodechar(machine->gfx[0], code, tiamc1_charram);

	SET_TILE_INFO(0, code, 0, 0);
}

VIDEO_START( tiamc1 )
{
	bg_tilemap1 = tilemap_create(get_bg1_tile_info, tilemap_scan_rows,
		 8, 8, 32, 32);

	bg_tilemap2 = tilemap_create(get_bg2_tile_info, tilemap_scan_rows,
		 8, 8, 32, 32);

	tiamc1_bg_vshift = 0;
	tiamc1_bg_hshift = 0;

	state_save_register_global(tiamc1_layers_ctrl);
	state_save_register_global(tiamc1_bg_vshift);
	state_save_register_global(tiamc1_bg_hshift);
}

static void draw_sprites(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect)
{
	int offs;

	for (offs = 0; offs < 16; offs++)
	{
		int flipx, flipy, sx, sy, spritecode;

		sx = tiamc1_spriteram_x[offs] ^ 0xff;
		sy = tiamc1_spriteram_y[offs] ^ 0xff;
		flipx = !(tiamc1_spriteram_a[offs] & 0x08);
		flipy = !(tiamc1_spriteram_a[offs] & 0x02);
		spritecode = tiamc1_spriteram_n[offs] ^ 0xff;

		if (!(tiamc1_spriteram_a[offs] & 0x01))
			drawgfx(bitmap, machine->gfx[1],
				spritecode,
				0,
				flipx, flipy,
				sx, sy,
				cliprect, TRANSPARENCY_PEN, 15);
	}
}

VIDEO_UPDATE( tiamc1 )
{
#if 0
	int i;

	for (i = 0; i < 32; i++)
	{
		tilemap_set_scrolly(bg_tilemap1, i, tiamc1_bg_vshift ^ 0xff);
		tilemap_set_scrolly(bg_tilemap2, i, tiamc1_bg_vshift ^ 0xff);
	}

	for (i = 0; i < 32; i++)
	{
		tilemap_set_scrollx(bg_tilemap1, i, tiamc1_bg_hshift ^ 0xff);
		tilemap_set_scrollx(bg_tilemap2, i, tiamc1_bg_hshift ^ 0xff);
	}
#endif

	if (tiamc1_layers_ctrl & 0x80)
		tilemap_draw(bitmap, cliprect, bg_tilemap2, 0, 0);
	else
		tilemap_draw(bitmap, cliprect, bg_tilemap1, 0, 0);


	draw_sprites(machine, bitmap, cliprect);

	return 0;
}

