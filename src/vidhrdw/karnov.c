/*******************************************************************************

	Karnov - Bryan McPhail, mish@tendril.co.uk

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static unsigned char *dirty_f;
static struct mame_bitmap *bitmap_f;
data16_t karnov_scroll[2], *karnov_pf_data;
static struct tilemap *fix_tilemap;
static int flipscreen;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Karnov has two 1024x8 palette PROM.
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 7 -- unused
        -- unused
        -- unused
        -- unused
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
void karnov_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}

void karnov_flipscreen_w(int data)
{
	static int last_flip;
	flipscreen=data;
	if (flipscreen!=last_flip)
		memset(dirty_f,1,0x800);
	last_flip=flipscreen;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}

static void draw_background(struct mame_bitmap *bitmap)
{
	int my,mx,offs,color,tile,fx,fy;
	int scrollx=karnov_scroll[0];
	int scrolly=karnov_scroll[1];

	if (flipscreen) fx=fy=1; else fx=fy=0;

	/* 1st area is stored along X-axis... */
	mx=-1; my=0;
	for (offs = 0;offs < 0x400; offs ++) {
		mx++;
		if (mx==32) {mx=0; my++;}

		if (!dirty_f[offs]) continue; else dirty_f[offs]=0;

		tile=karnov_pf_data[offs];
		color = tile >> 12;
		tile = tile&0x7ff;
		if (flipscreen)
			drawgfx(bitmap_f,Machine->gfx[1],tile,
				color, fx, fy, 496-16*mx,496-16*my,
		 		0,TRANSPARENCY_NONE,0);
		else
			drawgfx(bitmap_f,Machine->gfx[1],tile,
				color, fx, fy, 16*mx,16*my,
		 		0,TRANSPARENCY_NONE,0);
	}

	/* 2nd area is stored along Y-axis... */
	mx=0; my=-1;
	for (offs = 0x400 ;offs < 0x800; offs ++) {
		my++;
		if (my==32) {my=0; mx++;}

		if (!dirty_f[offs]) continue; else dirty_f[offs]=0;

		tile=karnov_pf_data[offs];
		color = tile >> 12;
		tile=tile&0x7ff;

		if (flipscreen)
			drawgfx(bitmap_f,Machine->gfx[1],tile,
				color, fx, fy, 496-16*mx,496-16*my,
		 		0,TRANSPARENCY_NONE,0);
		else
			drawgfx(bitmap_f,Machine->gfx[1],tile,
				color, fx, fy, 16*mx,16*my,
		 		0,TRANSPARENCY_NONE,0);
	}

	if (!flipscreen) {
		scrolly=-scrolly;
		scrollx=-scrollx;
	} else {
		scrolly=scrolly+256;
		scrollx=scrollx+256;
	}
	copyscrollbitmap(bitmap,bitmap_f,1,&scrollx,1,&scrolly,0,TRANSPARENCY_NONE,0);
}

static void draw_sprites(struct mame_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs <0x800;offs += 4) {
		int x,y,sprite,sprite2,colour,fx,fy,extra;

	    y=buffered_spriteram16[offs];
	    if (!(y&0x8000)) continue;

	    y=y&0x1ff;
	    sprite=buffered_spriteram16[offs+3];
	    colour=sprite>>12;
	    sprite=sprite&0xfff;
	    x=buffered_spriteram16[offs+2]&0x1ff;

		fx=buffered_spriteram16[offs+1];
	    if ((fx&0x10)) extra=1; else extra=0;
		fy=fx&0x2;
		fx=fx&0x4;

		if (extra) y=y+16;

	    /* Convert the co-ords..*/
		x=(x+16)%0x200;
		y=(y+16)%0x200;
		x=256 - x;
		y=256 - y;
		if (flipscreen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			if (extra) y=y-16;
		}

		/* Y Flip determines order of multi-sprite */
		if (extra && fy) {
			sprite2=sprite;
			sprite++;
		}
		else sprite2=sprite+1;

		drawgfx(bitmap,Machine->gfx[2],
				sprite,
				colour,fx,fy,x,y,
				0,TRANSPARENCY_PEN,0);

    	/* 1 more sprite drawn underneath */
    	if (extra)
    		drawgfx(bitmap,Machine->gfx[2],
				sprite2,
				colour,fx,fy,x,y+16,
				0,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/

void karnov_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	draw_background(bitmap);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,fix_tilemap,0,0);
}

/******************************************************************************/

static void get_fix_tile_info(int tile_index)
{
	int tile=videoram16[tile_index];
	SET_TILE_INFO(
			0,
			tile&0xfff,
			tile>>14,
			0)
}

WRITE16_HANDLER( karnov_videoram_w )
{
	COMBINE_DATA(&videoram16[offset]);
	tilemap_mark_tile_dirty(fix_tilemap,offset);
}

WRITE16_HANDLER( karnov_playfield_w )
{
	COMBINE_DATA(&karnov_pf_data[offset]);
	dirty_f[offset] = 1;
}

/******************************************************************************/

void karnov_vh_stop (void)
{
	if (dirty_f) free(dirty_f);
	if (bitmap_f) bitmap_free (bitmap_f);
}

int karnov_vh_start (void)
{
	/* Allocate bitmaps */
	if ((bitmap_f = bitmap_alloc(512,512)) == 0) {
		karnov_vh_stop ();
		return 1;
	}

	dirty_f=malloc(0x800);
	memset(dirty_f,1,0x800);

	fix_tilemap=tilemap_create(get_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap) return 1;
	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

int wndrplnt_vh_start (void)
{
	/* Allocate bitmaps */
	if ((bitmap_f = bitmap_alloc(512,512)) == 0) {
		karnov_vh_stop ();
		return 1;
	}

	dirty_f=malloc(0x800);
	memset(dirty_f,1,0x800);

	fix_tilemap=tilemap_create(get_fix_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!fix_tilemap) return 1;
	tilemap_set_transparent_pen(fix_tilemap,0);

	return 0;
}

/******************************************************************************/
