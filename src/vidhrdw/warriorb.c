#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;

static int taito_hide_pixels;

/**********************************************************/

static int has_two_TC0100SCN(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the second TC0100SCN is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0100SCN_word_1_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if the TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0110PCR_step1_word_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}

static int has_second_TC0110PCR(void)
{
	const struct Memory_WriteAddress16 *mwa;

	/* scan the memory handlers and see if a second TC0110PCR is used */

	mwa = Machine->drv->cpu[0].memory_write;
	if (mwa)
	{
		while (!IS_MEMPORT_END(mwa))
		{
			if (!IS_MEMPORT_MARKER(mwa))
			{
				if (mwa->handler == TC0110PCR_step1_word_1_w)
					return 1;
			}
			mwa++;
		}
	}

	return 0;
}


int warriorb_core_vh_start (void)
{

	spritelist = malloc(0x800 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	if (TC0100SCN_vh_start(has_two_TC0100SCN() ? 2 : 1,TC0100SCN_GFX_NUM,taito_hide_pixels))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	if (has_second_TC0110PCR())
		if (TC0110PCR_1_vh_start())
			return 1;

	return 0;
}

int warriorb_vh_start (void)
{
	taito_hide_pixels = 0;
	return (warriorb_core_vh_start());
}

void warriorb_vh_stop (void)
{
	free(spritelist);
	spritelist = 0;

	TC0100SCN_vh_stop();

	if (has_TC0110PCR())
		TC0110PCR_vh_stop();

	if (has_second_TC0110PCR())
		TC0110PCR_1_vh_stop();
}


/********************************************************
          SPRITE READ AND WRITE HANDLERS
********************************************************/

// None //


/*********************************************************
				PALETTE
*********************************************************/

void warriorb_update_palette (void)
{
	int i,j;
	int offs,data,tilenum,color;
	unsigned short palette_map[256];
	memset (palette_map, 0, sizeof (palette_map));

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+1];
		tilenum = data & 0x7fff;

		data = spriteram16[offs+2];
		color = (data & 0x7f);

		if (tilenum)
		{
			palette_map[color] |= Machine->gfx[0]->pen_usage[tilenum];
		}
	}

	/* Tell MAME about the color usage */
	for (i = 0;i < 256;i++)
	{
		int usage = palette_map[i];

		if (usage)
		{
			if (palette_map[i] & (1 << 0))
				palette_used_colors[i * 16 + 0] = PALETTE_COLOR_USED;
			for (j = 1; j < 16; j++)
				if (palette_map[i] & (1 << j))
					palette_used_colors[i * 16 + j] = PALETTE_COLOR_USED;
		}
	}
}




/************************************************************
			SPRITE DRAW ROUTINE
************************************************************/

static void warriorb_draw_sprites(struct osd_bitmap *bitmap,int *primasks,int y_offs)
{
	int offs, data, data2, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int code;

#ifdef MAME_DEBUG
	int unknown=0;
#endif

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+0];
		y = (-(data &0x1ff) - 24) & 0x1ff;	// offset changes with vis. area
		flipy = (data & 0x200) >> 9;

		data = spriteram16[offs+1];
		tilenum = data & 0x7fff;

		data2 = spriteram16[offs+2];
		/* 8,4 also seen in msbyte */
		priority = (data2 & 0x0100) >> 8;
		color    = (data2 & 0x7f);

		data = spriteram16[offs+3];
		x = ((data & 0x3ff) + 4) &0x3ff;
		flipx = (data & 0x400) >> 10;

		if (!tilenum) continue;

#ifdef MAME_DEBUG
		if (data2 & 0xf280)   unknown |= (data2 &0xf280);
#endif

		y += y_offs;

		/* sprite wrap: coords become negative at high values */
		if (x>0x3c0) x -= 0x400;
		if (y>0x180) y -= 0x200;

		curx = x;
		cury = y;
		code = tilenum;

		sprite_ptr->code = code;
		sprite_ptr->color = color;
		sprite_ptr->flipx = flipx;
		sprite_ptr->flipy = flipy;
		sprite_ptr->x = curx;
		sprite_ptr->y = cury;

		if (primasks)
		{
			sprite_ptr->primask = primasks[priority];
			sprite_ptr++;
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[0],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx,sprite_ptr->flipy,
					sprite_ptr->x,sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	/* this happens only if primsks != NULL */
	while (sprite_ptr != spritelist)
	{
		sprite_ptr--;

		pdrawgfx(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				&Machine->visible_area,TRANSPARENCY_PEN,0,
				sprite_ptr->primask);
	}

#ifdef MAME_DEBUG
	if (unknown)
	{
		char buf[80];
		sprintf(buf,"unknown sprite bits: %04x",unknown);
		usrintf_showmessage(buf);
	}
#endif
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

void warriorb_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layer[3];

	TC0100SCN_tilemap_update();

	palette_init_used_colors();
	warriorb_update_palette();
	palette_used_colors[0] |= PALETTE_COLOR_VISIBLE;
	palette_recalc();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,NULL);
	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */
	TC0100SCN_tilemap_draw(bitmap,0,layer[0],0,1);
	TC0100SCN_tilemap_draw(bitmap,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,0,layer[2],0,4);

	/* Sprites can be under/over the layer below text layer */
	{
		int primasks[2] = {0xf0,0xfc};
		warriorb_draw_sprites(bitmap,primasks,8);
	}
}

