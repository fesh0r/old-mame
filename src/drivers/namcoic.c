#include "state.h"
#include "driver.h"
#include "vidhrdw/generic.h"
#include "namcos2.h" /* for game-specific hacks */
#include "namcoic.h"

static void zdrawgfxzoom(
		struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
		const struct rectangle *clip,int transparency,int transparent_color,
		int scalex, int scaley, int zpos )
{
	if (!scalex || !scaley) return;
	if (dest_bmp->depth == 15 || dest_bmp->depth == 16)
	{
		if( gfx && gfx->colortable )
		{
			int shadow_offset = (Machine->drv->video_attributes&VIDEO_HAS_SHADOWS)?Machine->drv->total_colors:0;
			const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)];
			UINT8 *source_base = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
			int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
			int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;
			if (sprite_screen_width && sprite_screen_height)
			{
				/* compute sprite increment per screen pixel */
				int dx = (gfx->width<<16)/sprite_screen_width;
				int dy = (gfx->height<<16)/sprite_screen_height;

				int ex = sx+sprite_screen_width;
				int ey = sy+sprite_screen_height;

				int x_index_base;
				int y_index;

				if( flipx )
				{
					x_index_base = (sprite_screen_width-1)*dx;
					dx = -dx;
				}
				else
				{
					x_index_base = 0;
				}

				if( flipy )
				{
					y_index = (sprite_screen_height-1)*dy;
					dy = -dy;
				}
				else
				{
					y_index = 0;
				}

				if( clip )
				{
					if( sx < clip->min_x)
					{ /* clip left */
						int pixels = clip->min_x-sx;
						sx += pixels;
						x_index_base += pixels*dx;
					}
					if( sy < clip->min_y )
					{ /* clip top */
						int pixels = clip->min_y-sy;
						sy += pixels;
						y_index += pixels*dy;
					}
					if( ex > clip->max_x+1 )
					{ /* clip right */
						int pixels = ex-clip->max_x-1;
						ex -= pixels;
					}
					if( ey > clip->max_y+1 )
					{ /* clip bottom */
						int pixels = ey-clip->max_y-1;
						ey -= pixels;
					}
				}

				if( ex>sx )
				{ /* skip if inner loop doesn't draw anything */
					int y;
					if (transparency == TRANSPARENCY_PEN)
					{
						if( priority_bitmap )
						{
							for( y=sy; y<ey; y++ )
							{
								UINT8 *source = source_base + (y_index>>16) * gfx->line_modulo;
								UINT16 *dest = (UINT16 *)dest_bmp->line[y];
								UINT8 *pri = priority_bitmap->line[y];
								int x, x_index = x_index_base;
								for( x=sx; x<ex; x++ )
								{
									int c = source[x_index>>16];
									if( c != transparent_color )
									{
										if( pri[x]<=zpos )
										{
											if( c==0xfe && shadow_offset )
											{
												dest[x] |= shadow_offset;
											}
											else
											{
												dest[x] = pal[c];
											}
											pri[x] = zpos;
										}
									}
									x_index += dx;
								}
								y_index += dy;
							}
						}
					}
				}
			}
		}
	}
}

static data16_t mSpritePos[4];

WRITE16_HANDLER( namco_spritepos16_w )
{
	COMBINE_DATA( &mSpritePos[offset] );
}
READ16_HANDLER( namco_spritepos16_r )
{
	return mSpritePos[offset];
}

WRITE32_HANDLER( namco_spritepos32_w )
{
	data32_t v;
	offset *= 2;
	v = (mSpritePos[offset]<<16)|mSpritePos[offset+1];
	COMBINE_DATA( &v );
	mSpritePos[offset+0] = v>>16;
	mSpritePos[offset+1] = v&0xffff;
}
READ32_HANDLER( namco_spritepos32_r )
{
	offset *= 2;
	return (mSpritePos[offset]<<16)|mSpritePos[offset+1];
}

INLINE data8_t
nth_byte16( const data16_t *pSource, int which )
{
	data16_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xff;
	}
	else
	{
		return data>>8;
	}
} /* nth_byte16 */

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE data16_t
nth_word32( const data32_t *pSource, int which )
{
	data32_t data = pSource[which/2];
	if( which&1 )
	{
		return data&0xffff;
	}
	else
	{
		return data>>16;
	}
} /* nth_word32 */

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
INLINE data8_t
nth_byte32( const data32_t *pSource, int which )
{
		data32_t data = pSource[which/4];
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

/**************************************************************************************************************/

static int (*mpCodeToTile)( int code ); /* sprite banking callback */
static int mGfxC355;	/* gfx bank for sprites */
static int mPalXOR;		/* XOR'd with palette select register; needed for System21 */

/**
 * 0x00000 sprite attr (page0)
 * 0x02000 sprite list (page0)
 *
 * 0x02400 window attributes
 * 0x04000 format
 * 0x08000 tile
 * 0x10000 sprite attr (page1)
 * 0x14000 sprite list (page1)
 */
static void
draw_spriteC355( struct mame_bitmap *bitmap, const struct rectangle *cliprect, const data16_t *pSource, int pri, int zpos )
{
	unsigned screen_height_remaining, screen_width_remaining;
	unsigned source_height_remaining, source_width_remaining;
	int hpos,vpos;
	data16_t hsize,vsize;
	data16_t palette;
	data16_t linkno;
	data16_t offset;
	data16_t format;
	int tile_index;
	int num_cols,num_rows;
	int dx,dy;
	int row,col;
	int sx,sy,tile;
	int flipx,flipy;
	UINT32 zoomx, zoomy;
	int tile_screen_width;
	int tile_screen_height;
	const data16_t *spriteformat16 = &spriteram16[0x4000/2];
	const data16_t *spritetile16   = &spriteram16[0x8000/2];
	int color;
	const data16_t *pWinAttr;
	struct rectangle clip;
	int xscroll, yscroll;

	/**
	 * ----xxxx-------- window select
	 * --------xxxx---- priority
	 * ------------xxxx palette select
	 */
	palette = pSource[6];
	switch( namcos2_gametype )
	{
	case NAMCONB2_OUTFOXIES:
	case NAMCONB2_MACH_BREAKERS:
	case NAMCOS2_SUZUKA_8_HOURS_2:
	case NAMCOS2_SUZUKA_8_HOURS:
	case NAMCOS2_LUCKY_AND_WILD:
		if( pri != ((palette>>5)&7) ) return;
		break;

	default:
		if( pri != ((palette>>4)&0x7) ) return;
		break;
	}

	linkno		= pSource[0]; /* LINKNO */
	offset		= pSource[1]; /* OFFSET */
	hpos		= pSource[2]; /* HPOS		0x000..0x7ff (signed) */
	vpos		= pSource[3]; /* VPOS		0x000..0x7ff (signed) */
	hsize		= pSource[4]; /* HSIZE		max 0x3ff pixels */
	vsize		= pSource[5]; /* VSIZE		max 0x3ff pixels */
	/* pSource[6] contains priority/palette */
	/* pSource[7] is used in Lucky & Wild, possibly for sprite-road priority */

	if( linkno*4>=0x4000/2 ) return; /* avoid garbage memory reads */

	xscroll = (INT16)mSpritePos[1];
	yscroll = (INT16)mSpritePos[0];

	xscroll &= 0x1ff; if( xscroll & 0x100 ) xscroll |= ~0x1ff;
	yscroll &= 0x1ff; if( yscroll & 0x100 ) yscroll |= ~0x1ff;

	if( bitmap->width > 288 )
	{ /* Medium Resolution: System21 adjust */
		if( namcos2_gametype == NAMCOS21_CYBERSLED )
		{
			xscroll -= 0x04;
			yscroll += 0x1d;
		}
		else
		{ /* Starblade */
			yscroll += 0x10;
		}
	}
	else
	{ /* Namco NB1, Namco System 2 */
		xscroll += 0x26;
		yscroll += 0x19;
	}
	
	hpos -= xscroll;
	vpos -= yscroll;
	pWinAttr = &spriteram16[0x2400/2+((palette>>8)&0xf)*4];
	clip.min_x = pWinAttr[0] - xscroll;
	clip.max_x = pWinAttr[1] - xscroll;
	clip.min_y = pWinAttr[2] - yscroll;
	clip.max_y = pWinAttr[3] - yscroll;
	if( clip.min_x < cliprect->min_x ){ clip.min_x = cliprect->min_x; }
	if( clip.min_y < cliprect->min_y ){ clip.min_y = cliprect->min_y; }
	if( clip.max_x > cliprect->max_x ){ clip.max_x = cliprect->max_x; }
	if( clip.max_y > cliprect->max_y ){ clip.max_y = cliprect->max_y; }
	hpos&=0x7ff; if( hpos&0x400 ) hpos |= ~0x7ff; /* sign extend */
	vpos&=0x7ff; if( vpos&0x400 ) vpos |= ~0x7ff; /* sign extend */

	tile_index		= spriteformat16[linkno*4+0];
	format			= spriteformat16[linkno*4+1];
	dx				= spriteformat16[linkno*4+2];
	dy				= spriteformat16[linkno*4+3];
	num_cols		= (format>>4)&0xf;
	num_rows		= (format)&0xf;

	if( num_cols == 0 ) num_cols = 0x10;
	flipx = (hsize&0x8000)?1:0;
	hsize &= 0x3ff;//0x1ff;
	if( hsize == 0 ) return;
	zoomx = (hsize<<16)/(num_cols*16);
	dx = (dx*zoomx+0x8000)>>16;
	if( flipx )
	{
		hpos += dx;
	}
	else
	{
		hpos -= dx;
	}

	if( num_rows == 0 ) num_rows = 0x10;
	flipy = (vsize&0x8000)?1:0;
	vsize &= 0x3ff;
	if( vsize == 0 ) return;
	zoomy = (vsize<<16)/(num_rows*16);
	dy = (dy*zoomy+0x8000)>>16;
	if( flipy )
	{
		vpos += dy;
	}
	else
	{
		vpos -= dy;
	}

	color = (palette&0xf)^mPalXOR;

	source_height_remaining = num_rows*16;
	screen_height_remaining = vsize;
	sy = vpos;
	for( row=0; row<num_rows; row++ )
	{
		tile_screen_height = 16*screen_height_remaining/source_height_remaining;
		zoomy = (screen_height_remaining<<16)/source_height_remaining;
		if( flipy )
		{
			sy -= tile_screen_height;
		}
		source_width_remaining = num_cols*16;
		screen_width_remaining = hsize;
		sx = hpos;
		for( col=0; col<num_cols; col++ )
		{
			tile_screen_width = 16*screen_width_remaining/source_width_remaining;
			zoomx = (screen_width_remaining<<16)/source_width_remaining;
			if( flipx )
			{
				sx -= tile_screen_width;
			}
			tile = spritetile16[tile_index++];
			if( (tile&0x8000)==0 )
			{
				zdrawgfxzoom(
					bitmap,Machine->gfx[mGfxC355],
					mpCodeToTile(tile) + offset,
					color,
					flipx,flipy,
					sx,sy,
					&clip,
					TRANSPARENCY_PEN,0xff,
					zoomx, zoomy, zpos );
			}
			if( !flipx )
			{
				sx += tile_screen_width;
			}
			screen_width_remaining -= tile_screen_width;
			source_width_remaining -= 16;
		} /* next col */
		if( !flipy )
		{
			sy += tile_screen_height;
		}
		screen_height_remaining -= tile_screen_height;
		source_height_remaining -= 16;
	} /* next row */
} /* draw_spriteC355 */


static int DefaultCodeToTile( int code )
{
	return code;
}

void
namco_obj_init( int gfxbank, int palXOR, int (*codeToTile)( int code ) )
{
	mGfxC355 = gfxbank;
	mPalXOR = palXOR;
	if( codeToTile )
	{
		mpCodeToTile = codeToTile;
	}
	else
	{
		mpCodeToTile = DefaultCodeToTile;
	}
	spriteram16 = auto_malloc(0x20000);
	memset( mSpritePos,0x00,sizeof(mSpritePos) );
} /* namcosC355_init */

static void
DrawObjectList(
		struct mame_bitmap *bitmap,
		const struct rectangle *cliprect,
		int pri,
		const data16_t *pSpriteList16,
		const data16_t *pSpriteTable )
{
	data16_t which;
	int i;
	int count = 0;
	/* count the sprites */
	for( i=0; i<256; i++ )
	{
		which = pSpriteList16[i];
		count++;
		if( which&0x100 ) break;
	}
	/* draw the sprites */
	for( i=0; i<count; i++ )
	{
		which = pSpriteList16[i];
		draw_spriteC355( bitmap, cliprect, &pSpriteTable[(which&0xff)*8], pri, i );
	}
} /* DrawObjectList */

void
namco_obj_draw( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	if( pri==0 )
	{
		fillbitmap( priority_bitmap, 0, cliprect );
	}
	DrawObjectList( bitmap,cliprect,pri,&spriteram16[0x02000/2], &spriteram16[0x00000/2] );
	DrawObjectList( bitmap,cliprect,pri,&spriteram16[0x14000/2], &spriteram16[0x10000/2] );
} /* namco_obj_draw */

WRITE16_HANDLER( namco_obj16_w )
{
	COMBINE_DATA( &spriteram16[offset] );
} /* namco_obj16_w */

READ16_HANDLER( namco_obj16_r )
{
	return spriteram16[offset];
} /* namco_obj16_r */

WRITE32_HANDLER( namco_obj32_w )
{
	data32_t v;
	offset *= 2;
	v = (spriteram16[offset]<<16)|spriteram16[offset+1];
	COMBINE_DATA( &v );
	spriteram16[offset] = v>>16;
	spriteram16[offset+1] = v&0xffff;
} /* namco_obj32_w */

READ32_HANDLER( namco_obj32_r )
{
	offset *= 2;
	return (spriteram16[offset]<<16)|spriteram16[offset+1];
} /* namco_obj32_r */

/**************************************************************************************************************/

/* ROZ abstraction (preliminary)
 *
 * Used by:
 *	Namco NB2 - The Outfoxies, Mach Breakers
 *	Namco System 2 - Metal Hawk, Lucky and Wild
 */
static data16_t *rozbank16;
static data16_t *rozvideoram16;
static data16_t *rozcontrol16;
static int mRozGfxBank;
static int mRozMaskRegion;
static struct tilemap *mRozTilemap[2];
static int mRozPage[2]; /* base addr for tilemap */

/* RozMask and RozSize are used to dynamically construct a smaller ROZ tilemap, mirroring
 * the upper left quadrant.
 *
 * The ROZ tilemaps normally contain a full 128x128 tiles, but occasionally need to be
 * rendered as smaller (i.e. 16x16) tilemaps, with wrapping.
 *
 * The exact mapping of control registers for this purpose is not yet understood.
 */
static int mRozMask[2];
static int mRozSize[2]; /* 32, 64, or 128 */

static void
SetRozSize( int which, int size )
{
	if( mRozSize[which]!=size )
	{
		mRozSize[which] = size;
		mRozMask[which] = (size-1);
		mRozMask[which] |= (mRozMask[which]<<7);
		tilemap_mark_all_tiles_dirty( mRozTilemap[which] );
	}
}

static void
roz_get_info( int tile_index,int which )
{
	data16_t tile = rozvideoram16[mRozPage[which]+(tile_index&mRozMask[which])];
	int bank;
	int mangle;

	switch( namcos2_gametype )
	{
	case NAMCONB2_MACH_BREAKERS:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile;
		break;

	case NAMCONB2_OUTFOXIES:
		bank = nth_byte16( &rozbank16[which*8/2], (tile>>11)&0x7 );
		tile = (tile&0x7ff)|(bank*0x800);
		mangle = tile&~(0x50);
		/* the pixmap index is mangled, the transparency bitmask index is not */
		if( tile&0x10 ) mangle |= 0x40;
		if( tile&0x40 ) mangle |= 0x10;
		break;

	case NAMCOS2_LUCKY_AND_WILD:
		mangle	= tile&0x01ff;
		tile &= 0x3fff;
		switch( tile>>9 )
		{
		case 0x00: mangle |= 0x1c00; break;
		case 0x01: mangle |= 0x0800; break;
		case 0x02: mangle |= 0x0000; break;

		case 0x08: mangle |= 0x1e00; break;
		case 0x09: mangle |= 0x0a00; break;
		case 0x0a: mangle |= 0x0200; break;

		case 0x10: mangle |= 0x2000; break;
		case 0x11: mangle |= 0x0c00; break;
		case 0x12: mangle |= 0x0400; break;

		case 0x18: mangle |= 0x2200; break;
		case 0x19: mangle |= 0x0e00; break;
		case 0x1a: mangle |= 0x0600; break;
		}
		break;

	case NAMCOS2_METAL_HAWK:
	default:
		mangle = tile&0x01ff;
		if( tile&0x1000 ) mangle |= 0x0200;
		if( tile&0x0200 ) mangle |= 0x0400;
		if( tile&0x0400 ) mangle |= 0x0800;
		if( tile&0x0800 ) mangle |= 0x1000;
		break;
	}
	SET_TILE_INFO( mRozGfxBank,mangle,0/*color*/,0 );
	tile_info.mask_data = 32*tile + (UINT8 *)memory_region( mRozMaskRegion );
} /* roz_get_info */

static void
roz_get_info0( int tile_index )
{
	roz_get_info( tile_index,0 );
}
static void
roz_get_info1( int tile_index )
{
	roz_get_info( tile_index,1 );
}

int
namco_roz_init( int gfxbank, int maskregion )
{
	/* allocate both ROZ layers */
	int i;
	static void (*roz_info[2])(int tile_index) = { roz_get_info0, roz_get_info1 };

	mRozGfxBank = gfxbank;
	mRozMaskRegion = maskregion;

	rozbank16 = auto_malloc(0x10);
	rozvideoram16 = auto_malloc(0x20000);
	rozcontrol16 = auto_malloc(0x20);
	if( rozbank16 && rozvideoram16 && rozcontrol16 )
	{
		for( i=0; i<2; i++ )
		{
			mRozPage[i] = -1;
			mRozTilemap[i] = tilemap_create( roz_info[i], tilemap_scan_rows,
				TILEMAP_BITMASK,16,16,128,128 );

			if( mRozTilemap[i] == NULL ) return 1; /* error */
	
			SetRozSize( i,128 );
		}
		return 0;
	}
	return -1;
} /* namco_roz_init */

/**
 * ROZ control attributes:
 *
 * unk  attr  xx   xy   yx   yy   x0   y0
 *
 * Lucky & Wild:
 * 0000 080e 2100 0000 0000 0100 38e0 fde0	0:badge
 * 1000 084e 00fe 0000 0000 00fe f9f6 fd96	0:zooming car
 * 0000 080e 1100 0000 0000 0000 26d0 0450	0:"LUCKY & WILD"
 * 0000 03bf 0100 0000 0000 0100 fde0 7fe0	1:talking heads
 * 1000 080a 0100 0000 0000 0100 fde0 ffe0	0:player select
 *
 * Outfoxies:
 * 1000 0211 02ff 02ff 0000 0000 02ff f97b ffa7 (tv)
 * 0000 0211 00ff 0000 0000 0000 00ff fddb 0de7 (char select)
 * 1000 0271 0101 0000 0000 0101 0fc4 56d7 (stage)
 * 0000 0252 0101 4000 0000 0101 0fc4 d6d7 (stage)
 *
 * Mach Breakers:
 * 0000 0000 0100 0000 0000 0100 f4d0 f8e0 // character select
 * 0000 0020 0100 0000 0000 0100 f4d0 f8e0 // map
 * 1000 0871 40b5 8f4a 00b6 00b5 7804 0994 // (pri=4) skater
 * 0000 0210 0100 0000 0000 0100 40d0 38e0 // (pri=3) ramp
 * 0000 0377 0100 0000 0000 0100 fdd0 c0e0 // monsterpull floor
 * 1000 0370 0100 0000 0000 0100 fdd0 26e0 // (pri=1) shooting gallery
 * 8000 0a30 4080 0000 0000 0080 a268 42f1 // steel earth dive
 * 0000 0210 0100 0000 0000 0100 f4d0 f8e0 // dirt earth dive
 *
 * unk  attr  xx   xy   yx   yy   x0   y0
 */
void namco_roz_draw(
	struct mame_bitmap *bitmap,
	const struct rectangle *cliprect,
	int pri )
{
	const int xoffset = 36,yoffset = 3;
	int which;
	for( which=0; which<2; which++ )
	{
		const data16_t *pSource = &rozcontrol16[which*8];
		data16_t attrs = pSource[1];
		if( (attrs&0x8000)==0 )
		{ /* layer is enabled */
			int color = attrs&0xf;
			int page;
			int roz_pri;
			switch( namcos2_gametype )
			{
			case NAMCONB2_OUTFOXIES:
				roz_pri = 4-which; /* ? */
				page = pSource[3]&0x4000;
				if( attrs == 0x0211 ) roz_pri = 1; /* hack */
				break;

			case NAMCONB2_MACH_BREAKERS:
				roz_pri = 4-which; /* ? */
				page = (pSource[2]&0x6000)*2;

				if( (pSource[0]==0x1000 && pSource[1]==0x0871) || /* hyper glide: player */
					(pSource[0]==0x8000 && pSource[1]==0x0887) )  /* monster pull: monster */
				{
					SetRozSize( which, 64 );
				}
				else if( (pSource[0] == 0x8000 && pSource[1] == 0x0a30) || /* dirt dive */
					     (pSource[0] == 0x8000 && pSource[1] == 0x0a90)    /* bomb's away */ )
				{
					roz_pri = 5;
				}
				else if( pSource[0] == 0x1000 && pSource[1] == 0x0370 )
				{ /* shooting gallery */
					roz_pri = 1;
				}
				else if( pSource[0] == 0x0000 &&
				   (pSource[1] == 0x0000 || pSource[1]==0x0020) &&
					pSource[2] == 0x0100 &&
					pSource[3] == 0x0000 &&
					pSource[4] == 0x0000 &&
					pSource[5] == 0x0100 &&
					pSource[6] == 0xf4d0 &&
					pSource[7] == 0xf8e0 )
				{ /* backdrop for character select screen */
					SetRozSize( which, 32 );
					roz_pri = 1; /* or 2? */
				}
				else
				{
					SetRozSize( which,128 );
				}
				//if( code_pressed(KEYCODE_LSHIFT) )
				//	roz_pri = pSource[0]&0xf;
				break;

			case NAMCOS2_LUCKY_AND_WILD:
				roz_pri = 5-which; /* ? */
				page = (attrs&0x0800)?0:0x4000; /* ? */
				break;

			case NAMCOS2_METAL_HAWK:
			default:
				roz_pri = which; /* ? */
				page = pSource[3]&0x4000;
				break;
			}
			if( roz_pri==pri )
			{
				int bDirty;
				UINT32 startx,starty;
				int incxx,incxy,incyx,incyy;
				data16_t temp;

				temp = pSource[2];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incxx = (INT16)temp;

				temp = pSource[3];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incxy =  (INT16)temp;

				temp = pSource[4];
				if( temp&0x8000 ) temp |= 0xf000; else temp&=0x0fff; /* sign extend */
				incyx =  (INT16)temp;

				incyy =  (INT16)pSource[5];
				startx = (INT16)pSource[6];
				starty = (INT16)pSource[7];

				startx <<= 4;
				starty <<= 4;
				startx += xoffset * incxx + yoffset * incyx;
				starty += xoffset * incxy + yoffset * incyy;

				bDirty = 0;
				tilemap_set_palette_offset(mRozTilemap[which],color*256);
				if( mRozPage[which] != page )
				{
					mRozPage[which] = page;
					bDirty = 1;
				}

				if( bDirty )
				{
					tilemap_mark_all_tiles_dirty( mRozTilemap[which] );
				}

				if( mRozSize[which]==32 )
				{ /* hack */
					incxx<<=1;
					incyy<<=1;
					startx>>=1; if( startx&0x4000 ) startx|=0x8000;
					starty>>=1; if( starty&0x4000 ) starty|=0x8000;
				}

				tilemap_draw_roz(bitmap,cliprect,mRozTilemap[which],
					startx << 8,
					starty << 8,
					incxx << 8,
					incxy << 8,
					incyx << 8,
					incyy << 8,
					1,	/* copy with wraparound */
					0,0);
			} /* roz_pri==pri */
		}
	}
} /* namco_roz_draw */

READ16_HANDLER( namco_rozcontrol16_r )
{
	return rozcontrol16[offset];
}

WRITE16_HANDLER( namco_rozcontrol16_w )
{
	COMBINE_DATA( &rozcontrol16[offset] );
}

READ16_HANDLER( namco_rozbank16_r )
{
	return rozbank16[offset];
}

WRITE16_HANDLER( namco_rozbank16_w )
{
	data16_t old_data;

	old_data = rozbank16[offset];
	COMBINE_DATA( &rozbank16[offset] );
	if( rozbank16[offset]!=old_data )
	{
		tilemap_mark_all_tiles_dirty( mRozTilemap[0] );
		tilemap_mark_all_tiles_dirty( mRozTilemap[1] );
	}
}

static void writerozvideo( int offset, data16_t data )
{
	int i;
	if( rozvideoram16[offset]!=data )
	{
		rozvideoram16[offset] = data;
		for( i=0; i<2; i++ )
		{
			if( mRozPage[i]==(offset&0xc000) )
			{
				int col,row;
				col = offset&0x7f;
				row = (offset/0x80)&0x7f;
				if( col<mRozSize[i] && row<mRozSize[i] )
				{
					while( row<128 )
					{
						int c;
						for( c=col; c<128; c+=mRozSize[i] )
						{
							tilemap_mark_tile_dirty( mRozTilemap[i], row*128+c );
						}
						row += mRozSize[i];
					}
				}
			}
		}
	}
}

READ16_HANDLER( namco_rozvideoram16_r )
{
	return rozvideoram16[offset];
}

WRITE16_HANDLER( namco_rozvideoram16_w )
{
	data16_t v;

	v = rozvideoram16[offset];
	COMBINE_DATA( &v );
	writerozvideo( offset, v );
}

READ32_HANDLER( namco_rozcontrol32_r )
{
	offset *= 2;
	return (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
}

WRITE32_HANDLER( namco_rozcontrol32_w )
{
	data32_t v;
	offset *=2;
	v = (rozcontrol16[offset]<<16)|rozcontrol16[offset+1];
	COMBINE_DATA(&v);
	rozcontrol16[offset] = v>>16;
	rozcontrol16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozbank32_r )
{
	offset *= 2;
	return (rozbank16[offset]<<16)|rozbank16[offset+1];
}

WRITE32_HANDLER( namco_rozbank32_w )
{
	data32_t v;
	offset *=2;
	v = (rozbank16[offset]<<16)|rozbank16[offset+1];
	COMBINE_DATA(&v);
	rozbank16[offset] = v>>16;
	rozbank16[offset+1] = v&0xffff;
}

READ32_HANDLER( namco_rozvideoram32_r )
{
	offset *= 2;
	return (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
}

WRITE32_HANDLER( namco_rozvideoram32_w )
{
	data32_t v;
	offset *= 2;
	v = (rozvideoram16[offset]<<16)|rozvideoram16[offset+1];
	COMBINE_DATA( &v );
	writerozvideo(offset,v>>16);
	writerozvideo(offset+1,v&0xffff);
}

/**************************************************************************************************************/
/*
	Land Line Buffer
	Land Generator
		0xf,0x7,0xe,0x6,0xd,0x5,0xc,0x4,
		0xb,0x3,0xa,0x2,0x9,0x1,0x8,0x0

*/

/* Preliminary!  The road circuitry is identical for all the driving games.
 *
 * There are several chunks of RAM
 *
 *	Road Tilemap:
 *		0x00000..0x0ffff	64x512 tilemap
 *
 *	Road Tiles:
 *		0x10000..0x1f9ff	16x16x2bpp tiles
 *
 *
 *	Line Attributes:
 *
 *		0x1fa00..0x1fbdf	xxx- ---- ---- ----		priority
 *							---- xxxx xxxx xxxx		xscroll
 *
 *		0x1fbfe				horizontal adjust?
 *							0x0017
 *							0x0018 (Final Lap3)
 *
 *		0x1fc00..0x1fddf	selects line in source bitmap
 *		0x1fdfe				yscroll
 *
 *		0x1fe00..0x1ffdf	---- --xx xxxx xxxx		zoomx
 *		0x1fffd				always 0xffff 0xffff?
 */
static data16_t *mpRoadRAM; /* at 0x880000 in Final Lap; at 0xa00000 in Lucky&Wild */
static unsigned char *mpRoadDirty;
static int mbRoadSomethingIsDirty;
static int mRoadGfxBank;
static struct tilemap *mpRoadTilemap;
static pen_t mRoadTransparentColor;
static int mbRoadNeedTransparent;

#define ROAD_COLS			64
#define ROAD_ROWS			512
#define ROAD_TILE_SIZE		16
#define ROAD_TILEMAP_WIDTH	(ROAD_TILE_SIZE*ROAD_COLS)
#define ROAD_TILEMAP_HEIGHT (ROAD_TILE_SIZE*ROAD_ROWS)

#define ROAD_TILE_COUNT_MAX	(0xfa00/0x40) /* 0x3e8 */
#define WORDS_PER_ROAD_TILE (0x40/2)

static struct GfxLayout RoadTileLayout =
{
	ROAD_TILE_SIZE,
	ROAD_TILE_SIZE,
	ROAD_TILE_COUNT_MAX,
	2,
	{
#ifndef LSB_FIRST
		0,8
#else
		8,0
#endif
	},
	{/* x offset */
		0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
		0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17
	},
	{/* y offset */
		0x000,0x020,0x040,0x060,0x080,0x0a0,0x0c0,0x0e0,
		0x100,0x120,0x140,0x160,0x180,0x1a0,0x1c0,0x1e0
	},
	0x200, /* offset to next tile */
};

void get_road_info( int tile_index )
{
	data16_t data = mpRoadRAM[tile_index];
	/* ------xx xxxxxxxx tile number
	 * xxxxxx-- -------- palette select
	 */
	int tile = (data&0x3ff);
	int color = (data>>10);

	SET_TILE_INFO( mRoadGfxBank, tile, color , 0 )
} /* get_road_info */

READ16_HANDLER( namco_road16_r )
{
	return mpRoadRAM[offset];
}

WRITE16_HANDLER( namco_road16_w )
{
	COMBINE_DATA( &mpRoadRAM[offset] );
	if( offset<0x10000/2 )
	{
		tilemap_mark_tile_dirty( mpRoadTilemap, offset );
	}
	else
	{
		offset -= 0x10000/2;
		if( offset<ROAD_TILE_COUNT_MAX*WORDS_PER_ROAD_TILE )
		{
			mpRoadDirty[offset/WORDS_PER_ROAD_TILE] = 1;
			mbRoadSomethingIsDirty = 1;
		}
	}
}

static void
UpdateRoad( void )
{
	int i;
	if( mbRoadSomethingIsDirty )
	{
		for( i=0; i<ROAD_TILE_COUNT_MAX; i++ )
		{
			if( mpRoadDirty[i] )
			{
				decodechar(
					Machine->gfx[mRoadGfxBank],
					i,
					0x10000+(UINT8 *)mpRoadRAM,
					&RoadTileLayout );
				mpRoadDirty[i] = 0;
			}
		}
		tilemap_mark_all_tiles_dirty( mpRoadTilemap );
		mbRoadSomethingIsDirty = 0;
	}
}

static void
RoadMarkAllDirty(void)
{
	memset( mpRoadDirty,0x01,ROAD_TILE_COUNT_MAX );
	mbRoadSomethingIsDirty = 1;
}

int
namco_road_init( int gfxbank )
{
	mbRoadNeedTransparent = 0;
	mRoadGfxBank = gfxbank;
	mpRoadDirty = auto_malloc(ROAD_TILE_COUNT_MAX);
	if( mpRoadDirty )
	{
		memset( mpRoadDirty,0x00,ROAD_TILE_COUNT_MAX );
		mbRoadSomethingIsDirty = 0;
		mpRoadRAM = auto_malloc(0x20000);
		if( mpRoadRAM )
		{
			struct GfxElement *pGfx = decodegfx( 0x10000+(UINT8 *)mpRoadRAM, &RoadTileLayout );
			if( pGfx )
			{
				pGfx->colortable = &Machine->remapped_colortable[0xf00];
				pGfx->total_colors = 0x3f;

				Machine->gfx[gfxbank] = pGfx;
				mpRoadTilemap = tilemap_create(
					get_road_info,tilemap_scan_rows,
					TILEMAP_OPAQUE,
					ROAD_TILE_SIZE,ROAD_TILE_SIZE,
					ROAD_COLS,ROAD_ROWS);

				if( mpRoadTilemap )
				{
					state_save_register_UINT8 ("namco_road", 0, "RoadDirty", mpRoadDirty, ROAD_TILE_COUNT_MAX);
					state_save_register_UINT16("namco_road", 0, "RoadRAM",   mpRoadRAM,   0x20000 / 2);
					state_save_register_func_postload(RoadMarkAllDirty);

					return 0;
				}
			}
		}
	}
	return -1;
} /* namco_road_init */

void
namco_road_set_transparent_color(pen_t pen)
{
	mbRoadNeedTransparent = 1;
	mRoadTransparentColor = pen;
}

void
namco_road_draw( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri )
{
	struct mame_bitmap *pSourceBitmap;
	unsigned yscroll;
	int i;

	UpdateRoad();

	pSourceBitmap = tilemap_get_pixmap(mpRoadTilemap);
	yscroll = mpRoadRAM[0x1fdfe/2];

	for( i=cliprect->min_y; i<=cliprect->max_y; i++ )
	{
		UINT16 *pDest = bitmap->line[i];
		int screenx	= mpRoadRAM[0x1fa00/2+i+15];

		if( pri == ((screenx&0xe000)>>13) )
		{
			unsigned zoomx	= mpRoadRAM[0x1fe00/2+i+15]&0x3ff;
			if( zoomx )
			{
				unsigned sourcey = mpRoadRAM[0x1fc00/2+i+15]+yscroll;
				const UINT16 *pSourceGfx = pSourceBitmap->line[sourcey&(ROAD_TILEMAP_HEIGHT-1)];
				unsigned dsourcex = (ROAD_TILEMAP_WIDTH<<16)/zoomx;
				unsigned sourcex = 0;
				int numpixels = (44*ROAD_TILE_SIZE<<16)/dsourcex;

				/* draw this scanline */
				screenx &= 0x0fff; /* mask off priority bits */
				if( screenx&0x0800 )
				{
					/* sign extend */
					screenx -= 0x1000;
				}

				/* adjust the horizontal placement */
				screenx -= 64; /*needs adjustment to left*/

				if( screenx<0 )
				{ /* crop left */
					numpixels += screenx;
					sourcex -= dsourcex*screenx;
					screenx = 0;
				}

				if( screenx + numpixels > bitmap->width )
				{ /* crop right */
					numpixels = bitmap->width - screenx;
				}

				/* BUT: support transparent color for Thunder Ceptor */
				if (mbRoadNeedTransparent)
				{
					while( numpixels-- > 0 )
					{
						int pen = pSourceGfx[sourcex>>16];
						/* TBA: work out palette mapping for Final Lap, Suzuka */
						if (pen != mRoadTransparentColor)
							pDest[screenx] = pen;
						screenx++;
						sourcex += dsourcex;
					}
				}
				else
				{
					while( numpixels-- > 0 )
					{
						int pen = pSourceGfx[sourcex>>16];
						/* TBA: work out palette mapping for Final Lap, Suzuka */
						pDest[screenx++] = pen;
						sourcex += dsourcex;
					}
				}
			}
		}
	}
} /* namco_road_draw */
