#include "driver.h"
#include "machine/eeprom.h"
#include "vidhrdw/konamiic.h"


static int layer_colorbase[3],sprite_colorbase,bg_colorbase;
static int priorityflag;
static int layerpri[3];
static int prmrsocr_sprite_bank;
static int sorted_layer[3];
static int dim_c,dim_v;	/* ssriders, tmnt2 */


/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

/* Missing in Action */

static void mia_tile_callback(int layer,int bank,int *code,int *color)
{
	tile_info.flags = (*color & 0x04) ? TILE_FLIPX : 0;
	if (layer == 0)
	{
		*code |= ((*color & 0x01) << 8);
		*color = layer_colorbase[layer] + ((*color & 0x80) >> 5) + ((*color & 0x10) >> 1);
	}
	else
	{
		*code |= ((*color & 0x01) << 8) | ((*color & 0x18) << 6) | (bank << 11);
		*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
	}
}

static void tmnt_tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x03) << 8) | ((*color & 0x10) << 6) | ((*color & 0x0c) << 9)
			| (bank << 13);
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}

static int detatwin_rombank;

static void detatwin_tile_callback(int layer,int bank,int *code,int *color)
{
	/* (color & 0x02) is flip y handled internally by the 052109 */
	*code |= ((*color & 0x01) << 8) | ((*color & 0x10) << 5) | ((*color & 0x0c) << 8)
			| (bank << 12) | detatwin_rombank << 14;
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}



/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void mia_sprite_callback(int *code,int *color,int *priority,int *shadow)
{
	*color = sprite_colorbase + (*color & 0x0f);
}

static void tmnt_sprite_callback(int *code,int *color,int *priority,int *shadow)
{
	*code |= (*color & 0x10) << 9;
	*color = sprite_colorbase + (*color & 0x0f);
}

static void punkshot_sprite_callback(int *code,int *color,int *priority_mask,int *shadow)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*code |= (*color & 0x10) << 9;
	*color = sprite_colorbase + (*color & 0x0f);
}

static void thndrx2_sprite_callback(int *code,int *color,int *priority_mask,int *shadow)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

  Callbacks for the K053245

***************************************************************************/

static void lgtnfght_sprite_callback(int *code,int *color,int *priority_mask)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase + (*color & 0x1f);
}

static void detatwin_sprite_callback(int *code,int *color,int *priority_mask)
{
#if 0
if (keyboard_pressed(KEYCODE_Q) && (*color & 0x20)) *color = rand();
if (keyboard_pressed(KEYCODE_W) && (*color & 0x40)) *color = rand();
if (keyboard_pressed(KEYCODE_E) && (*color & 0x80)) *color = rand();
#endif
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase + (*color & 0x1f);
}

static void prmrsocr_sprite_callback(int *code,int *color,int *priority_mask)
{
	int pri = 0x20 | ((*color & 0x60) >> 2);
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*code |= prmrsocr_sprite_bank << 14;

	*color = sprite_colorbase + (*color & 0x1f);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int mia_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 32;
	layer_colorbase[2] = 40;
	sprite_colorbase = 16;
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,mia_tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,REVERSE_PLANE_ORDER,mia_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int tmnt_vh_start(void)
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 32;
	layer_colorbase[2] = 40;
	sprite_colorbase = 16;
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,REVERSE_PLANE_ORDER,tmnt_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int punkshot_vh_start(void)
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,punkshot_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int lgtnfght_vh_start(void)	/* also tmnt2, ssriders */
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,lgtnfght_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int detatwin_vh_start(void)
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,detatwin_tile_callback))
		return 1;
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,detatwin_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int glfgreat_vh_start(void)
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,lgtnfght_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int thndrx2_vh_start(void)
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,thndrx2_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

int prmrsocr_vh_start(void)
{
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tmnt_tile_callback))
		return 1;
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,prmrsocr_sprite_callback))
	{
		K052109_vh_stop();
		return 1;
	}
	return 0;
}

void punkshot_vh_stop(void)
{
	K052109_vh_stop();
	K051960_vh_stop();
}

void lgtnfght_vh_stop(void)
{
	K052109_vh_stop();
	K053245_vh_stop();
}

void detatwin_vh_stop(void)
{
	K052109_vh_stop();
	K053245_vh_stop();
}

void glfgreat_vh_stop(void)
{
	K052109_vh_stop();
	K053245_vh_stop();
}

void thndrx2_vh_stop(void)
{
	K052109_vh_stop();
	K051960_vh_stop();
}

void prmrsocr_vh_stop(void)
{
	K052109_vh_stop();
	K053245_vh_stop();
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( tmnt_paletteram_word_w )
{
	int r,g,b;

	COMBINE_DATA(paletteram16 + offset);
	offset &= ~1;

	data = (paletteram16[offset] << 8) | paletteram16[offset+1];

	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset / 2,r,g,b);
}



WRITE16_HANDLER( tmnt_0a0000_w )
{
	if (ACCESSING_LSB)
	{
		static int last;

		/* bit 0/1 = coin counters */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);	/* 2 players version */

		/* bit 3 high then low triggers irq on sound CPU */
		if (last == 0x08 && (data & 0x08) == 0)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x08;

		/* bit 5 = irq enable */
		interrupt_enable_w(0,data & 0x20);

		/* bit 7 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x80) ? ASSERT_LINE : CLEAR_LINE);

		/* other bits unused */
	}
}

WRITE16_HANDLER( punkshot_0a0020_w )
{
	if (ACCESSING_LSB)
	{
		static int last;


		/* bit 0 = coin counter */
		coin_counter_w(0,data & 0x01);

		/* bit 2 = trigger irq on sound CPU */
		if (last == 0x04 && (data & 0x04) == 0)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x04;

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
}

WRITE16_HANDLER( lgtnfght_0a0018_w )
{
	if (ACCESSING_LSB)
	{
		static int last;


		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 2 = trigger irq on sound CPU */
		if (last == 0x00 && (data & 0x04) == 0x04)
			cpu_cause_interrupt(1,0xff);

		last = data & 0x04;

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
	}
}

WRITE16_HANDLER( detatwin_700300_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

		/* bit 7 = select char ROM bank */
		if (detatwin_rombank != ((data & 0x80) >> 7))
		{
			detatwin_rombank = (data & 0x80) >> 7;
			tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
		}

		/* other bits unknown */
	}
}

WRITE16_HANDLER( glfgreat_122000_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 4 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

		/* other bits unknown */
	}
}


WRITE16_HANDLER( ssriders_eeprom_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0 is data */
		/* bit 1 is cs (active low) */
		/* bit 2 is clock (active high) */
		EEPROM_write_bit(data & 0x01);
		EEPROM_set_cs_line((data & 0x02) ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_clock_line((data & 0x04) ? ASSERT_LINE : CLEAR_LINE);

		/* bits 3-4 control palette dimming */
		/* 4 = DIMPOL = when set, negate SHAD */
		/* 3 = DIMMOD = when set, or BRIT with [negated] SHAD */
		dim_c = data & 0x18;

		/* bit 5 selects sprite ROM for testing in TMNT2 (bits 5-7, actually, according to the schematics) */
		K053244_bankselect(((data & 0x20) >> 5) << 2);
	}
}

WRITE16_HANDLER( ssriders_1c0300_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 3 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x08) ? ASSERT_LINE : CLEAR_LINE);

		/* bits 4-6 control palette dimming (DIM0-DIM2) */
		dim_v = (data & 0x70) >> 4;
	}
}

WRITE16_HANDLER( prmrsocr_122000_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 0,1 = coin counter */
		coin_counter_w(0,data & 0x01);
		coin_counter_w(1,data & 0x02);

		/* bit 4 = enable char ROM reading through the video RAM */
		K052109_set_RMRD_line((data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

		/* bit 6 = sprite ROM bank */
		prmrsocr_sprite_bank = (data & 0x40) >> 6;
		K053244_bankselect(prmrsocr_sprite_bank << 2);

		/* other bits unknown (unused?) */
	}
}



WRITE16_HANDLER( tmnt_priority_w )
{
	if (ACCESSING_LSB)
	{
		/* bit 2/3 = priority; other bits unused */
		/* bit2 = PRI bit3 = PRI2
			  sprite/playfield priority is controlled by these two bits, by bit 3
			  of the background tile color code, and by the SHADOW sprite
			  attribute bit.
			  Priorities are encoded in a PROM (G19 for TMNT). However, in TMNT,
			  the PROM only takes into account the PRI and SHADOW bits.
			  PRI  Priority
			   0   bg fg spr text
			   1   bg spr fg text
			  The SHADOW bit, when set, torns a sprite into a shadow which makes
			  color below it darker (this is done by turning off three resistors
			  in parallel with the RGB output).

			  Note: the background color (color used when all of the four layers
			  are 0) is taken from the *foreground* palette, not the background
			  one as would be more intuitive.
		*/
		priorityflag = (data & 0x0c) >> 2;
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = sorted_layer[a]; sorted_layer[a] = sorted_layer[b]; sorted_layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

void mia_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY,0);
	if ((priorityflag & 1) == 1) K051960_sprites_draw(bitmap,0,0);
	K052109_tilemap_draw(bitmap,1,0,0);
	if ((priorityflag & 1) == 0) K051960_sprites_draw(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0,0);
}

void tmnt_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	K052109_tilemap_update();

	K052109_tilemap_draw(bitmap,2,TILEMAP_IGNORE_TRANSPARENCY,0);
	if ((priorityflag & 1) == 1) K051960_sprites_draw(bitmap,0,0);
	K052109_tilemap_draw(bitmap,1,0,0);
	if ((priorityflag & 1) == 0) K051960_sprites_draw(bitmap,0,0);
	K052109_tilemap_draw(bitmap,0,0,0);
}


void punkshot_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);

	K052109_tilemap_update();

	sorted_layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	sorted_layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	sorted_layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);

	sortlayers(sorted_layer,layerpri);

	fillbitmap(priority_bitmap,0,NULL);
	K052109_tilemap_draw(bitmap,sorted_layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	K052109_tilemap_draw(bitmap,sorted_layer[1],0,2);
	K052109_tilemap_draw(bitmap,sorted_layer[2],0,4);

	K051960_sprites_draw(bitmap,-1,-1);
}


void lgtnfght_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);

	K052109_tilemap_update();

	sorted_layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	sorted_layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	sorted_layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);

	sortlayers(sorted_layer,layerpri);

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],&Machine->visible_area);
	K052109_tilemap_draw(bitmap,sorted_layer[0],0,1);
	K052109_tilemap_draw(bitmap,sorted_layer[1],0,2);
	K052109_tilemap_draw(bitmap,sorted_layer[2],0,4);

	K053245_sprites_draw(bitmap);
}

void glfgreat_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI3) + 8;	/* weird... */
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI4);

	K052109_tilemap_update();

	sorted_layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	sorted_layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI3);
	sorted_layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI4);

	sortlayers(sorted_layer,layerpri);

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],&Machine->visible_area);
	K052109_tilemap_draw(bitmap,sorted_layer[0],0,1);
	K052109_tilemap_draw(bitmap,sorted_layer[1],0,2);
	K052109_tilemap_draw(bitmap,sorted_layer[2],0,4);

	K053245_sprites_draw(bitmap);
}

void tmnt2_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	static int lastdim;
	int i,newdim;

	lgtnfght_vh_screenrefresh(bitmap,full_refresh);

	newdim = dim_v | ((~dim_c & 0x10) >> 1);
	if (newdim != lastdim)
	{
		double brt = 1.0 - (1.0-PALETTE_DEFAULT_SHADOW_FACTOR)*newdim/8;
		int cb;

		lastdim = newdim;

		/* only affect the background and sprites, not text layer */
		cb = layer_colorbase[sorted_layer[0]] * 16;
		for (i = cb;i < cb + 128;i++)
			palette_set_brightness(i,brt);
		cb = layer_colorbase[sorted_layer[1]] * 16;
		for (i = cb;i < cb + 128;i++)
			palette_set_brightness(i,brt);
		cb = sprite_colorbase * 16;
		for (i = cb;i < cb + 512;i++)
			palette_set_brightness(i,brt);

		if (~dim_c & 0x10)
			palette_set_shadow_factor(1/PALETTE_DEFAULT_SHADOW_FACTOR);
		else
			palette_set_shadow_factor(PALETTE_DEFAULT_SHADOW_FACTOR);
	}
}


void thndrx2_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	bg_colorbase       = K053251_get_palette_index(K053251_CI0);
	sprite_colorbase   = K053251_get_palette_index(K053251_CI1);
	layer_colorbase[0] = K053251_get_palette_index(K053251_CI2);
	layer_colorbase[1] = K053251_get_palette_index(K053251_CI4);
	layer_colorbase[2] = K053251_get_palette_index(K053251_CI3);

	K052109_tilemap_update();

	sorted_layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI2);
	sorted_layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI4);
	sorted_layer[2] = 2;
	layerpri[2] = K053251_get_priority(K053251_CI3);

	sortlayers(sorted_layer,layerpri);

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[16 * bg_colorbase],&Machine->visible_area);
	K052109_tilemap_draw(bitmap,sorted_layer[0],0,1);
	K052109_tilemap_draw(bitmap,sorted_layer[1],0,2);
	K052109_tilemap_draw(bitmap,sorted_layer[2],0,4);

	K051960_sprites_draw(bitmap,-1,-1);
}
