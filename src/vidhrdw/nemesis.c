/***************************************************************************

  vidhrdw.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include <math.h>

data16_t *nemesis_videoram1b;
data16_t *nemesis_videoram1f;
data16_t *nemesis_videoram2b;
data16_t *nemesis_videoram2f;

data16_t *nemesis_characterram;
size_t nemesis_characterram_size;
data16_t *nemesis_xscroll1,*nemesis_xscroll2,*nemesis_yscroll;
data16_t *nemesis_yscroll1,*nemesis_yscroll2;

static int spriteram_words;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;
static struct osd_bitmap *tmpbitmap4;

static unsigned char *video1_dirty;	/* 0x800 chars - foreground */
static unsigned char *video2_dirty;	/* 0x800 chars - background */
static unsigned char *char_dirty;	/* 2048 chars */
static unsigned char *sprite_dirty;	/* 512 sprites */
static unsigned char *sprite3216_dirty;	/* 256 sprites */
static unsigned char *sprite816_dirty;	/* 1024 sprites */
static unsigned char *sprite1632_dirty;	/* 256 sprites */
static unsigned char *sprite3232_dirty;	/* 128 sprites */
static unsigned char *sprite168_dirty;	/* 1024 sprites */
static unsigned char *sprite6464_dirty;	/* 32 sprites */

WRITE16_HANDLER( nemesis_palette_word_w )
{
	int r,g,b,bit1,bit2,bit3,bit4,bit5;

	COMBINE_DATA(paletteram16 + offset);
	data = paletteram16[offset];

	/* Mish, 30/11/99 - Schematics show the resistor values are:
		300 Ohms
		620 Ohms
		1200 Ohms
		2400 Ohms
		4700 Ohms

		So the correct weights per bit are 8, 17, 33, 67, 130
	*/

	#define MULTIPLIER 8 * bit1 + 17 * bit2 + 33 * bit3 + 67 * bit4 + 130 * bit5

	bit1=(data >>  0)&1;
	bit2=(data >>  1)&1;
	bit3=(data >>  2)&1;
	bit4=(data >>  3)&1;
	bit5=(data >>  4)&1;
	r = MULTIPLIER;
	r = pow (r/255.0, 2)*255;
	bit1=(data >>  5)&1;
	bit2=(data >>  6)&1;
	bit3=(data >>  7)&1;
	bit4=(data >>  8)&1;
	bit5=(data >>  9)&1;
	g = MULTIPLIER;
	g = pow (g/255.0, 2)*255;
	bit1=(data >>  10)&1;
	bit2=(data >>  11)&1;
	bit3=(data >>  12)&1;
	bit4=(data >>  13)&1;
	bit5=(data >>  14)&1;
	b = MULTIPLIER;
	b = pow (b/255.0, 2)*255;

	palette_change_color(offset,r,g,b);
}

WRITE16_HANDLER( salamander_palette_word_w )
{
	int r,g,b;

	COMBINE_DATA(paletteram16 + offset);
	offset &= ~1;

	data = ((paletteram16[offset] << 8) & 0xff00) | (paletteram16[offset+1] & 0xff);

	r = (data >>  0) & 0x1f;
	g = (data >>  5) & 0x1f;
	b = (data >> 10) & 0x1f;

	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_change_color(offset / 2,r,g,b);
}

READ16_HANDLER( nemesis_videoram1b_word_r )
{
	return nemesis_videoram1b[offset];
}
READ16_HANDLER( nemesis_videoram1f_word_r )
{
	return nemesis_videoram1f[offset];
}

WRITE16_HANDLER( nemesis_videoram1b_word_w )
{
	COMBINE_DATA(nemesis_videoram1b + offset);
	video1_dirty[offset] = 1;
}
WRITE16_HANDLER( nemesis_videoram1f_word_w )
{
	COMBINE_DATA(nemesis_videoram1f + offset);
	video2_dirty[offset] = 1;
}

READ16_HANDLER( nemesis_videoram2b_word_r )
{
	return nemesis_videoram2b[offset];
}
READ16_HANDLER( nemesis_videoram2f_word_r )
{
	return nemesis_videoram2f[offset];
}

WRITE16_HANDLER( nemesis_videoram2b_word_w )
{
	COMBINE_DATA(nemesis_videoram2b + offset);
	video1_dirty[offset] = 1;
}
WRITE16_HANDLER( nemesis_videoram2f_word_w )
{
	COMBINE_DATA(nemesis_videoram2f + offset);
	video2_dirty[offset] = 1;
}


READ16_HANDLER( gx400_xscroll1_word_r ) { return nemesis_xscroll1[offset];}
READ16_HANDLER( gx400_xscroll2_word_r ) { return nemesis_xscroll2[offset];}
READ16_HANDLER( gx400_yscroll_word_r ) { return nemesis_yscroll[offset];}
READ16_HANDLER( gx400_yscroll1_word_r ) { return nemesis_yscroll1[offset];}
READ16_HANDLER( gx400_yscroll2_word_r ) { return nemesis_yscroll2[offset];}

WRITE16_HANDLER( gx400_xscroll1_word_w ) { COMBINE_DATA(nemesis_xscroll1 + offset);}
WRITE16_HANDLER( gx400_xscroll2_word_w ) { COMBINE_DATA(nemesis_xscroll2 + offset);}
WRITE16_HANDLER( gx400_yscroll_word_w ) { COMBINE_DATA(nemesis_yscroll + offset);}
WRITE16_HANDLER( gx400_yscroll1_word_w ) { COMBINE_DATA(nemesis_yscroll1 + offset);}
WRITE16_HANDLER( gx400_yscroll2_word_w ) { COMBINE_DATA(nemesis_yscroll2 + offset);}


/* we have to straighten out the 16-bit word into bytes for gfxdecode() to work */
READ16_HANDLER( nemesis_characterram_word_r )
{
	return nemesis_characterram[offset];
}

WRITE16_HANDLER( nemesis_characterram_word_w )
{
	data16_t oldword = nemesis_characterram[offset];
	COMBINE_DATA(nemesis_characterram + offset);
	data = nemesis_characterram[offset];

	if (oldword != data)
	{
		char_dirty[offset / 16] = 1;
		sprite_dirty[offset / 64] = 1;
		sprite3216_dirty[offset / 128] = 1;
		sprite1632_dirty[offset / 128] = 1;
		sprite3232_dirty[offset / 256] = 1;
		sprite168_dirty[offset / 32] = 1;
		sprite816_dirty[offset / 32] = 1;
		sprite6464_dirty[offset / 1024] = 1;
	}
}


/* Fix the gdx decode info for lsb first systems */
static void nemesis_lsbify_gfx(void)
{
	int i, j;
	for(i=0; i<8; i++)
		for(j=0; j<Machine->drv->gfxdecodeinfo[i].gfxlayout->width; j++)
			Machine->drv->gfxdecodeinfo[i].gfxlayout->xoffset[j] ^= 8;
}


/* free the palette dirty array */
void nemesis_vh_stop(void)
{
#ifdef LSB_FIRST
	nemesis_lsbify_gfx();
#endif

	bitmap_free(tmpbitmap);
	bitmap_free(tmpbitmap2);
	bitmap_free(tmpbitmap3);
	bitmap_free(tmpbitmap4);
	tmpbitmap=0;
	free (char_dirty);
	free (sprite_dirty);
	free (sprite3216_dirty);
	free (sprite1632_dirty);
	free (sprite3232_dirty);
	free (sprite168_dirty);
	free (sprite816_dirty);
	free (sprite6464_dirty);
	char_dirty = 0;
	free (video1_dirty);
	free (video2_dirty);
}

/* claim a palette dirty array */
int nemesis_vh_start(void)
{
#ifdef LSB_FIRST
	nemesis_lsbify_gfx();
#endif

	spriteram_words = spriteram_size / 2;

	if ((tmpbitmap = bitmap_alloc(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	if ((tmpbitmap2 = bitmap_alloc(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	if ((tmpbitmap3 = bitmap_alloc(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	if ((tmpbitmap4 = bitmap_alloc(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		nemesis_vh_stop();
		return 1;
	}

	char_dirty = malloc(2048);
	if (!char_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(char_dirty,1,2048);

	sprite_dirty = malloc(512);
	if (!sprite_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite_dirty,1,512);

	sprite3216_dirty = malloc(256);
	if (!sprite3216_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite3216_dirty,1,256);

	sprite1632_dirty = malloc(256);
	if (!sprite1632_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite1632_dirty,1,256);

	sprite3232_dirty = malloc(128);
	if (!sprite3232_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite3232_dirty,1,128);

	sprite168_dirty = malloc(1024);
	if (!sprite168_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite168_dirty,1,1024);

	sprite816_dirty = malloc(1024);
	if (!sprite816_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite816_dirty,1,32);

	sprite6464_dirty = malloc(32);
	if (!sprite6464_dirty) {
		nemesis_vh_stop();
		return 1;
	}
	memset(sprite6464_dirty,1,32);

	video1_dirty = malloc (0x800);
	video2_dirty = malloc (0x800);
	if (!video1_dirty || !video2_dirty)
	{
		nemesis_vh_stop();
		return 1;
	}
	memset(video1_dirty,1,0x800);
	memset(video2_dirty,1,0x800);

	memset(nemesis_characterram,0,nemesis_characterram_size);

	return 0;
}


/* This is a bit slow, but it works. I'll speed it up later */
static void nemesis_drawgfx_zoomup(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scale)
{
	int ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm;
	int col;
	struct rectangle myclip;

	int dda_x=0;
	int dda_y=0;
	int ex_count;
	int ey_count;
	int real_x;
	int ysize;
	int xsize;
	const UINT32 *paldata;	/* ASG 980209 */
	int transmask;

	if (!gfx) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	transmask = 1 << transparent_color;

	if ((gfx->pen_usage[code] & ~transmask) == 0)
		/* character is totally transparent, no need to draw */
		return;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - gfx->width - sx;

		if (clip)
		{
			int temp;

			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - gfx->height - sy;

		if (clip)
		{
			int temp;

			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	xsize=gfx->width;
	ysize=gfx->height;
	/* Clipping currently done in code loop */
	ex = sx + xsize -1;
	ey = sy + ysize -1;
/*	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	if (ey >= dest->height) tey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;
*/
	/* start = code * gfx->height; */
	if (flipy)	/* Y flip */
	{
		start = code * gfx->height + gfx->height-1;
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height;
		dy = 1;
	}



	paldata = &gfx->colortable[gfx->color_granularity * color];

	if (flipx)	/* X flip */
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
			y=sy + ysize -1;
		else
			y=sy;
		dda_y=0x80;
		ey_count=sy;
		do
		{
			if(y>=clip->min_y && y<=clip->max_y)
			{
				if (Machine->orientation & ORIENTATION_FLIP_X)
				{
					bm  = dest->line[y]+sx + xsize -1;
					real_x=sx + xsize -1;
				} else {
					bm  = dest->line[y]+sx;
					real_x=sx;
				}
				sd = (gfx->gfxdata + start * gfx->line_modulo + xsize -1);
				dda_x=0x80;
				ex_count=sx;
				col = *(sd);
				do
				{
					if ((real_x<=clip->max_x) && (real_x>=clip->min_x))
						if (col != transparent_color) *bm = paldata[col];
					if (Machine->orientation & ORIENTATION_FLIP_X)
					{
						bm--;
						real_x--;
					} else {
						bm++;
						real_x++;
					}
					dda_x-=scale;
					if(dda_x<=0)
					{
						dda_x+=0x80;
						sd--;
						ex_count++;
						col = *(sd);
					}
				} while(ex_count <= ex);
			}
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				y--;
			else
				y++;
			dda_y-=scale;
			if(dda_y<=0)
			{
				dda_y+=0x80;
				start+=dy;
				ey_count++;
			}

		} while(ey_count <= ey);
	}
	else		/* normal */
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
			y=sy + ysize -1;
		else
			y=sy;
		dda_y=0x80;
		ey_count=sy;
		do
		{
			if(y>=clip->min_y && y<=clip->max_y)
			{
				if (Machine->orientation & ORIENTATION_FLIP_X)
				{
					bm  = dest->line[y]+sx + xsize -1;
					real_x=sx + xsize -1;
				} else {
					bm  = dest->line[y]+sx;
					real_x=sx;
				}
				sd = (gfx->gfxdata + start * gfx->line_modulo);
				dda_x=0x80;
				ex_count=sx;
				col = *(sd);
				do
				{
					if ((real_x<=clip->max_x) && (real_x>=clip->min_x))
						if (col != transparent_color) *bm = paldata[col];
					if (Machine->orientation & ORIENTATION_FLIP_X)
					{
						bm--;
						real_x--;
					} else {
						bm++;
						real_x++;
					}
					dda_x-=scale;
					if(dda_x<=0)
					{
						dda_x+=0x80;
						sd++;
						ex_count++;
						col = *(sd);
					}
				} while(ex_count <= ex);
			}
			if (Machine->orientation & ORIENTATION_FLIP_Y)
				y--;
			else
				y++;
			dda_y-=scale;
			if(dda_y<=0)
			{
				dda_y+=0x80;
				start+=dy;
				ey_count++;
			}

		} while(ey_count <= ey);
	}
}

/* This is a bit slow, but it works. I'll speed it up later */
static void nemesis_drawgfx_zoomdown(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,int scale)
{
	int ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm;
	int col;
	struct rectangle myclip;

	int dda_x=0;
	int dda_y=0;
	int ex_count;
	int ey_count;
	int real_x;
	int ysize;
	int xsize;
	int transmask;
	const UINT32 *paldata;	/* ASG 980209 */

	if (!gfx) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	transmask = 1 << transparent_color;
	if ((gfx->pen_usage[code] & ~transmask) == 0)
		/* character is totally transparent, no need to draw */
		return;


	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = sx;
		sx = sy;
		sy = temp;

		temp = flipx;
		flipx = flipy;
		flipy = temp;

		if (clip)
		{
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = clip->min_y;
			myclip.min_y = temp;
			temp = clip->max_x;
			myclip.max_x = clip->max_y;
			myclip.max_y = temp;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		sx = dest->width - gfx->width - sx;

		if (clip)
		{
			int temp;


			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_x;
			myclip.min_x = dest->width-1 - clip->max_x;
			myclip.max_x = dest->width-1 - temp;
			myclip.min_y = clip->min_y;
			myclip.max_y = clip->max_y;
			clip = &myclip;
		}
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		sy = dest->height - gfx->height - sy;

		if (clip)
		{
			int temp;

			myclip.min_x = clip->min_x;
			myclip.max_x = clip->max_x;
			/* clip and myclip might be the same, so we need a temporary storage */
			temp = clip->min_y;
			myclip.min_y = dest->height-1 - clip->max_y;
			myclip.max_y = dest->height-1 - temp;
			clip = &myclip;
		}
	}


	/* check bounds */
	xsize=gfx->width;
	ysize=gfx->height;
	ex = sx + xsize -1;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;
	ey = sy + ysize -1;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	/* start = code * gfx->height; */
	if (flipy)	/* Y flip */
	{
		start = code * gfx->height + gfx->height-1;
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height;
		dy = 1;
	}



	paldata = &gfx->colortable[gfx->color_granularity * color];

	if (flipx)	/* X flip */
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
			y=sy + ysize -1;
		else
			y=sy;
		dda_y=0-scale/2;
		for(ey_count=0;ey_count<ysize;ey_count++)
		{
			if(dda_y<0) dda_y+=0x80;
			if(dda_y>=0)
			{
				dda_y-=scale;
				if(y>=clip->min_y && y<=clip->max_y)
				{
					if (Machine->orientation & ORIENTATION_FLIP_X)
					{
						bm  = dest->line[y]+sx + xsize -1;
						real_x=sx + xsize -1;
					} else {
						bm  = dest->line[y]+sx;
						real_x=sx;
					}
					sd = (gfx->gfxdata + start * gfx->line_modulo + xsize -1);
					dda_x=0-scale/2;
					for(ex_count=0;ex_count<xsize;ex_count++)
					{
						if(dda_x<0) dda_x+=0x80;
						if(dda_x>=0)
						{
							dda_x-=scale;
							if ((real_x<=clip->max_x) && (real_x>=clip->min_x))
							{
								col = *(sd);
								if (col != transparent_color) *bm = paldata[col];
							}
							if (Machine->orientation & ORIENTATION_FLIP_X)
							{
								bm--;
								real_x--;
							} else {
								bm++;
								real_x++;
							}
						}
						sd--;
					}
				}
				if (Machine->orientation & ORIENTATION_FLIP_Y)
					y--;
				else
					y++;
			}
			start+=dy;
		}

	}
	else		/* normal */
	{
		if (Machine->orientation & ORIENTATION_FLIP_Y)
			y=sy + ysize -1;
		else
			y=sy;
		dda_y=0-scale/2;
		for(ey_count=0;ey_count<ysize;ey_count++)
		{
			if(dda_y<0) dda_y+=0x80;
			if(dda_y>=0)
			{
				dda_y-=scale;
				if(y>=clip->min_y && y<=clip->max_y)
				{
					if (Machine->orientation & ORIENTATION_FLIP_X)
					{
						bm  = dest->line[y]+sx + xsize -1;
						real_x=sx + xsize -1;
					} else {
						bm  = dest->line[y]+sx;
						real_x=sx;
					}
					sd = (gfx->gfxdata + start * gfx->line_modulo);
					dda_x=0-scale/2;
					for(ex_count=0;ex_count<xsize;ex_count++)
					{
						if(dda_x<0) dda_x+=0x80;
						if(dda_x>=0)
						{
							dda_x-=scale;
							if ((real_x<=clip->max_x) && (real_x>=clip->min_x))
							{
								col = *(sd);
								if (col != transparent_color) *bm = paldata[col];
							}
							if (Machine->orientation & ORIENTATION_FLIP_X)
							{
								bm--;
								real_x--;
							} else {
								bm++;
								real_x++;
							}
						}
						sd++;
					}
				}
				if (Machine->orientation & ORIENTATION_FLIP_Y)
					y--;
				else
					y++;
			}
			start+=dy;
		}

	}
}


static void draw_sprites(struct osd_bitmap *bitmap)
{
	/*
	 *	16 bytes per sprite, in memory from 56000-56fff
	 *
	 *	byte	0 :	relative priority.
	 *	byte	2 :	size (?) value #E0 means not used., bit 0x01 is flipx
	                0xc0 is upper 2 bits of zoom.
					0x38 is size.
	 * 	byte	4 :	zoom = 0xff
	 *	byte	6 :	low bits sprite code.
	 *	byte	8 :	color + hi bits sprite code., bit 0x20 is flipy bit. bit 0x01 is high bit of X pos.
	 *	byte	A :	X position.
	 *	byte	C :	Y position.
	 * 	byte	E :	not used.
	 */

	int adress;	/* start of sprite in spriteram */
	int sx;	/* sprite X-pos */
	int sy;	/* sprite Y-pos */
	int code;	/* start of sprite in obj RAM */
	int color;	/* color of the sprite */
	int flipx,flipy;
	int zoom;
	int char_type;
	int priority;
	int size;

	for (priority=0;priority<256;priority++)
	{
		for (adress = 0;adress < spriteram_words;adress += 8)
		{
			if((spriteram16[adress] & 0xff)!=priority) continue;

			zoom = spriteram16[adress+2] & 0xff;
			if (!(spriteram16[adress+2] & 0xff00) && ((spriteram16[adress+3] & 0xff00) != 0xff00))
				code = spriteram16[adress+3] + ((spriteram16[adress+4] & 0xc0) << 2);
			else
				code = (spriteram16[adress+3] & 0xff) + ((spriteram16[adress+4] & 0xc0) << 2);

			if (zoom != 0xFF || code!=0)
			{
				size=spriteram16[adress+1];
				zoom+=(size&0xc0)<<2;

				sx = spriteram16[adress+5]&0xff;
				sy = spriteram16[adress+6]&0xff;
				if(spriteram16[adress+4]&1)
					sx-=0x100;	/* fixes left side clip */
				color = (spriteram16[adress+4] & 0x1e) >> 1;
				flipx = spriteram16[adress+1] & 0x01;
				flipy = spriteram16[adress+4] & 0x20;

				switch(size&0x38)
				{
					case 0x00:	/* sprite 32x32*/
						char_type=4;
						code/=8;
						break;
					case 0x08:	/* sprite 16x32 */
						char_type=5;
						code/=4;
						break;
					case 0x10:	/* sprite 32x16 */
						char_type=2;
						code/=4;
						break;
					case 0x18:		/* sprite 64x64 */
						char_type=7;
						code/=32;
						break;
					case 0x20:	/* char 8x8 */
						char_type=0;
						code*=2;
						break;
					case 0x28:		/* sprite 16x8 */
						char_type=6;
						break;
					case 0x30:	/* sprite 8x16 */
						char_type=3;
						break;
					case 0x38:
					default:	/* sprite 16x16 */
						char_type=1;
						code/=2;
						break;
				}

				/*  0x80 == no zoom */
				if(zoom==0x80)
				{
					drawgfx(bitmap,Machine->gfx[char_type],
							code,
							color,
							flipx,flipy,
							sx,sy,
							&Machine->visible_area,TRANSPARENCY_PEN,0);
				}
				else if(zoom>=0x80)
				{
					nemesis_drawgfx_zoomdown(bitmap,Machine->gfx[char_type],
							code,
							color,
							flipx,flipy,
							sx,sy,
							&Machine->visible_area,TRANSPARENCY_PEN,0,zoom);
				}
				else if(zoom>=0x10)
				{
					nemesis_drawgfx_zoomup(bitmap,Machine->gfx[char_type],
							code,
							color,
							flipx,flipy,
							sx,sy,
							&Machine->visible_area,TRANSPARENCY_PEN,0,zoom);
				}
			} /* if sprite */
		} /* for loop */
	} /* priority */
}

/******************************************************************************/

static void setup_palette(void)
{
	int color,code,i;
	int colmask[0x80];
	int pal_base,offs;

	palette_init_used_colors();

	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 0x80;color++) colmask[color] = 0;
	for (offs = 0x800 - 1;offs >= 0;offs --)
	{
		code = nemesis_videoram1f[offs] & 0x7ff;
		if (char_dirty[code] == 1)
		{
			decodechar(Machine->gfx[0],code,(unsigned char *)nemesis_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);
			char_dirty[code] = 2;
		}
		color = nemesis_videoram2f[offs] & 0x7f;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0;color < 0x80;color++) colmask[color] = 0;
	for (offs = 0;offs < spriteram_words;offs += 8)
	{
		int char_type;
		int zoom;

		zoom = spriteram16[offs+2] & 0xff;
		if (!(spriteram16[offs+2] & 0xff00) && ((spriteram16[offs+3] & 0xff00) != 0xff00))
			code = spriteram16[offs+3] + ((spriteram16[offs+4] & 0xc0) << 2);
		else
			code = (spriteram16[offs+3] & 0xff) + ((spriteram16[offs+4] & 0xc0) << 2);

		if (zoom != 0xFF || code!=0)
		{
			int size = spriteram16[offs+1];
			switch(size&0x38)
			{
				case 0x00:
					/* sprite 32x32*/
					char_type=4;
					code/=8;
					if (sprite3232_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite3232_dirty[code] = 0;
					}
					break;
				case 0x08:
					/* sprite 16x32 */
					char_type=5;
					code/=4;
					if (sprite1632_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite1632_dirty[code] = 0;

					}
					break;
				case 0x10:
					/* sprite 32x16 */
					char_type=2;
					code/=4;
					if (sprite3216_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite3216_dirty[code] = 0;
					}
					break;
				case 0x18:
					/* sprite 64x64 */
					char_type=7;
					code/=32;
					if (sprite6464_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite6464_dirty[code] = 0;
					}
					break;
				case 0x20:
					/* char 8x8 */
					char_type=0;
					code*=2;
					if (char_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
						Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						char_dirty[code] = 0;
					}
					break;
				case 0x28:
					/* sprite 16x8 */
					char_type=6;
					if (sprite168_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite168_dirty[code] = 0;
					}
					break;
				case 0x30:
					/* sprite 8x16 */
					char_type=3;
					if (sprite816_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite816_dirty[code] = 0;
					}
					break;
				default:
					logerror("UN-SUPPORTED SPRITE SIZE %-4x\n",size&0x38);
				case 0x38:
					/* sprite 16x16 */
					char_type=1;
					code/=2;
					if (sprite_dirty[code] == 1)
					{
						decodechar(Machine->gfx[char_type],code,(unsigned char *)nemesis_characterram,
								Machine->drv->gfxdecodeinfo[char_type].gfxlayout);
						sprite_dirty[code] = 2;

					}
					break;
			}

			color = (spriteram16[offs+4] & 0x1e) >> 1;
			colmask[color] |= Machine->gfx[char_type]->pen_usage[code];
		}
	}


	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 0x80;color++) colmask[color] = 0;
	for (offs = 0x800 - 1;offs >= 0;offs --)
	{
		code = nemesis_videoram1b[offs] & 0x7ff;
		if (char_dirty[code] == 1)
		{
			decodechar(Machine->gfx[0],code,(unsigned char *)nemesis_characterram,
					Machine->drv->gfxdecodeinfo[0].gfxlayout);
			char_dirty[code] = 2;
		}
		color = nemesis_videoram2b[offs] & 0x7f;
		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (color = 0;color < 0x80;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
		memset(video1_dirty,1,0x800);
		memset(video2_dirty,1,0x800);
	}
}

static void setup_backgrounds(void)
{
	int offs;

	/* Do the foreground first */
	for (offs = 0x800 - 1;offs >= 0;offs --)
	{
		int code,color;


		code = nemesis_videoram1f[offs] & 0x7ff;

		if (video2_dirty[offs] || char_dirty[code])
		{
			int sx,sy,flipx,flipy;

			color = nemesis_videoram2f[offs] & 0x7f;

			video2_dirty[offs] = 0;

			sx = offs % 64;
			sy = offs / 64;
			flipx = nemesis_videoram2f[offs] & 0x80;
			flipy = nemesis_videoram1f[offs] & 0x800;

			if(nemesis_videoram1f[offs]!=0 || nemesis_videoram2f[offs]!=0)
			{
				if (nemesis_videoram1f[offs] & 0x1000)		//screen priority
				{
					struct rectangle clip;

					drawgfx(tmpbitmap3,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						8*sx,8*sy,
						0,TRANSPARENCY_NONE,0);

					clip.min_x=8*sx;
					clip.max_x=8*sx+7;
					clip.min_y=8*sy;
					clip.max_y=8*sy+7;
					fillbitmap(tmpbitmap,palette_transparent_pen,&clip);
				} else {
					struct rectangle clip;

					drawgfx(tmpbitmap,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						8*sx,8*sy,
						0,TRANSPARENCY_NONE,0);

					clip.min_x=8*sx;
					clip.max_x=8*sx+7;
					clip.min_y=8*sy;
					clip.max_y=8*sy+7;
					fillbitmap(tmpbitmap3,palette_transparent_pen,&clip);
				}
			} else {
				struct rectangle clip;
				clip.min_x=8*sx;
				clip.max_x=8*sx+7;
				clip.min_y=8*sy;
				clip.max_y=8*sy+7;
				fillbitmap(tmpbitmap,palette_transparent_pen,&clip);
				fillbitmap(tmpbitmap3,palette_transparent_pen,&clip);
			}
		}
	}

	/* Background */
	for (offs = 0x800 - 1;offs >= 0;offs --)
	{
		int code,color;


		code = nemesis_videoram1b[offs] & 0x7ff;

		if (video1_dirty[offs] || char_dirty[code])
		{
			int sx,sy,flipx,flipy;

			video1_dirty[offs] = 0;

			color = nemesis_videoram2b[offs] & 0x7f;

			sx = offs % 64;
			sy = offs / 64;
			flipx = nemesis_videoram2b[offs] & 0x80;
			flipy = nemesis_videoram1b[offs] & 0x800;

			if(nemesis_videoram1b[offs]!=0 || nemesis_videoram2b[offs]!=0)
			{
				if (nemesis_videoram1b[offs] & 0x1000)		//screen priority
				{
					struct rectangle clip;

					drawgfx(tmpbitmap4,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						8*sx,8*sy,
						0,TRANSPARENCY_NONE,0);

					clip.min_x=8*sx;
					clip.max_x=8*sx+7;
					clip.min_y=8*sy;
					clip.max_y=8*sy+7;
					fillbitmap(tmpbitmap2,palette_transparent_pen,&clip);
				} else {
					struct rectangle clip;

					drawgfx(tmpbitmap2,Machine->gfx[0],
						code,
						color,
						flipx,flipy,
						8*sx,8*sy,
						0,TRANSPARENCY_NONE,0);

					clip.min_x=8*sx;
					clip.max_x=8*sx+7;
					clip.min_y=8*sy;
					clip.max_y=8*sy+7;
					fillbitmap(tmpbitmap4,palette_transparent_pen,&clip);
				}
			} else {
				struct rectangle clip;
				clip.min_x=8*sx;
				clip.max_x=8*sx+7;
				clip.min_y=8*sy;
				clip.max_y=8*sy+7;
				fillbitmap(tmpbitmap2,palette_transparent_pen,&clip);
				fillbitmap(tmpbitmap4,palette_transparent_pen,&clip);
			}
		}
	}
}

/******************************************************************************/

void nemesis_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int xscroll[256],xscroll2[256],yscroll;

	setup_palette();

	/* Render backgrounds */
	setup_backgrounds();

	/* screen flash */
	fillbitmap(bitmap,Machine->pens[paletteram16[0x00] & 0x7ff],&Machine->visible_area);

	/* Copy the background bitmap */
	yscroll = -(nemesis_yscroll[0x180] & 0xff);	/* used on nemesis level 2 */
	for (offs = 0;offs < 256;offs++)
	{
		xscroll2[offs] = -((nemesis_xscroll2[offs] & 0xff) +
				((nemesis_xscroll2[0x100 + offs] & 1) << 8));
	}
	copyscrollbitmap(bitmap,tmpbitmap,256,xscroll2,1,&yscroll,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	/* Do the foreground */
	for (offs = 0;offs < 256;offs++)
	{
		xscroll[offs] = -((nemesis_xscroll1[offs] & 0xff) +
				((nemesis_xscroll1[0x100 + offs] & 1) << 8));
	}
	copyscrollbitmap(bitmap,tmpbitmap2,256,xscroll,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	draw_sprites(bitmap);

	copyscrollbitmap(bitmap,tmpbitmap3,256,xscroll2,1,&yscroll,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	for (offs = 0; offs < 2048; offs++)
	{
		if (char_dirty[offs] == 2)
			char_dirty[offs] = 0;
	}
}

void twinbee_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int xscroll[256],xscroll2[256],yscroll;

	setup_palette();

	/* Render backgrounds */
	setup_backgrounds();

	/* Copy the background bitmap */
	yscroll = -(nemesis_yscroll[0x180] & 0xff);	/* used on nemesis level 2 */
	for (offs = 0;offs < 256;offs++)
	{
		xscroll2[offs] = -((nemesis_xscroll2[offs] & 0xff) +
				((nemesis_xscroll2[0x100 + offs] & 1) << 8));
	}
	copyscrollbitmap(bitmap,tmpbitmap,256,xscroll2,1,&yscroll,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Do the foreground */
	for (offs = 0;offs < 256;offs++)
	{
		xscroll[offs] = -((nemesis_xscroll1[offs] & 0xff) +
				((nemesis_xscroll1[0x100 + offs] & 1) << 8));
	}
	copyscrollbitmap(bitmap,tmpbitmap2,256,xscroll,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	if (Machine->orientation & ORIENTATION_SWAP_XY)
		Machine->orientation ^= ORIENTATION_FLIP_X;
	else
		Machine->orientation ^= ORIENTATION_FLIP_Y;

	draw_sprites(bitmap);

	if (Machine->orientation & ORIENTATION_SWAP_XY)
		Machine->orientation ^= ORIENTATION_FLIP_X;
	else
		Machine->orientation ^= ORIENTATION_FLIP_Y;

	copyscrollbitmap(bitmap,tmpbitmap3,256,xscroll2,1,&yscroll,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

	for (offs = 0; offs < 2048; offs++)
	{
		if (char_dirty[offs] == 2)
			char_dirty[offs] = 0;
	}
}

void salamand_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int xscroll1[256],xscroll2[256];
	int yscroll1[64],yscroll2[64];
	int row_scroll=0,culumn_scroll=0;

	setup_palette();

	/* Render backgrounds */
	setup_backgrounds();

	/* screen flash */
	fillbitmap(bitmap,Machine->pens[paletteram16[0x00] & 0x7ff],&Machine->visible_area);

	for (offs = 0; offs < 64; offs++)
	{
		yscroll1[offs] = -nemesis_yscroll2[offs];
		yscroll2[offs] = -nemesis_yscroll1[offs];
		if (yscroll1[offs]) culumn_scroll |= 0x01;
		if (yscroll2[offs]) culumn_scroll |= 0x02;
	}
	for (offs = 0; offs < 256; offs++)
	{
		xscroll1[offs] = -((nemesis_xscroll1[offs] & 0xff) + ((nemesis_xscroll1[0x100 + offs] & 1) << 8));
		xscroll2[offs] = -((nemesis_xscroll2[offs] & 0xff) + ((nemesis_xscroll2[0x100 + offs] & 1) << 8));
		if (xscroll1[offs]) row_scroll |= 0x01;
		if (xscroll2[offs]) row_scroll |= 0x02;
	}

	/* Copy the background bitmap */
	if (culumn_scroll & 0x01)
	{
		if (row_scroll & 0x01)
			copyscrollbitmap(bitmap,tmpbitmap2,256,xscroll1,1,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		else
			copyscrollbitmap(bitmap,tmpbitmap2,0,0,64,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	} else {
		copyscrollbitmap(bitmap,tmpbitmap2,256,xscroll1,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	/* Copy the foreground bitmap */
	if (culumn_scroll & 0x02)
	{
		if (row_scroll & 0x02)
			copyscrollbitmap(bitmap,tmpbitmap,256,xscroll2,1,yscroll2,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		else
			copyscrollbitmap(bitmap,tmpbitmap,0,0,64,yscroll2,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

		draw_sprites(bitmap);

		if (culumn_scroll & 0x01)
		{
			if (row_scroll & 0x01)
				copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll1,1,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
			else
				copyscrollbitmap(bitmap,tmpbitmap4,0,0,64,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		} else {
			copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll1,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		}

		if (row_scroll & 0x02)
			copyscrollbitmap(bitmap,tmpbitmap3,256,xscroll2,1,yscroll2,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		else
			copyscrollbitmap(bitmap,tmpbitmap3,0,0,64,yscroll2,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	} else {
		copyscrollbitmap(bitmap,tmpbitmap,256,xscroll2,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);

		draw_sprites(bitmap);

		if (culumn_scroll & 0x01)
		{
			if (row_scroll & 0x01)
				copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll1,1,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
			else
				copyscrollbitmap(bitmap,tmpbitmap4,0,0,64,yscroll1,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		} else {
			copyscrollbitmap(bitmap,tmpbitmap4,256,xscroll1,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
		}

		copyscrollbitmap(bitmap,tmpbitmap3,256,xscroll2,0,0,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
	}

	for (offs = 0; offs < 2048; offs++)
	{
		if (char_dirty[offs] == 2)
			char_dirty[offs] = 0;
	}
}

