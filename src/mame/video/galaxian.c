/***************************************************************************

  Galaxian hardware family

***************************************************************************/

#include "driver.h"
#include "includes/galaxian.h"

static const rectangle _spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8,   30*8-1
};
static const rectangle _spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};

static const rectangle* spritevisiblearea;
static const rectangle* spritevisibleareaflipx;


#define STARS_COLOR_BASE 		(memory_region_length(REGION_PROMS))
#define BULLETS_COLOR_BASE		(STARS_COLOR_BASE + 64)
#define BACKGROUND_COLOR_BASE	(BULLETS_COLOR_BASE + 2)


UINT8 *galaxian_videoram;
UINT8 *galaxian_spriteram;
UINT8 *galaxian_spriteram2;
UINT8 *galaxian_attributesram;
UINT8 *galaxian_bulletsram;
UINT8 *rockclim_videoram;
size_t galaxian_spriteram_size;
size_t galaxian_spriteram2_size;
size_t galaxian_bulletsram_size;


static TILE_GET_INFO( get_tile_info );
static TILE_GET_INFO( rockclim_get_tile_info );
static tilemap *bg_tilemap;
static tilemap *rockclim_tilemap;
static int mooncrst_gfxextend;
static int spriteram2_present;
static UINT8 gfxbank[5];
static UINT8 flipscreen_x;
static UINT8 flipscreen_y;
static UINT8 color_mask;
static tilemap *dambustr_tilemap2;
static UINT8 *dambustr_videoram2;
static void (*modify_charcode)(UINT16 *code,UINT8 x);		/* function to call to do character banking */
static void  gmgalax_modify_charcode(UINT16 *code,UINT8 x);
static void mooncrst_modify_charcode(UINT16 *code,UINT8 x);
static void mooncrgx_modify_charcode(UINT16 *code,UINT8 x);
static void  moonqsr_modify_charcode(UINT16 *code,UINT8 x);
static void mshuttle_modify_charcode(UINT16 *code,UINT8 x);
static void   pisces_modify_charcode(UINT16 *code,UINT8 x);
static void mimonkey_modify_charcode(UINT16 *code,UINT8 x);
static void  batman2_modify_charcode(UINT16 *code,UINT8 x);
static void  mariner_modify_charcode(UINT16 *code,UINT8 x);
static void  jumpbug_modify_charcode(UINT16 *code,UINT8 x);
static void dambustr_modify_charcode(UINT16 *code,UINT8 x);

static void (*modify_spritecode)(UINT8 *spriteram,int*,int*,int*,int);	/* function to call to do sprite banking */
static void  gmgalax_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mooncrst_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mooncrgx_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  moonqsr_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mshuttle_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  calipso_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void   pisces_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void mimonkey_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  batman2_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void  jumpbug_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void dkongjrm_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void   ad2083_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);
static void dambustr_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs);

static void (*modify_color)(UINT8 *color);	/* function to call to do modify how the color codes map to the PROM */
static void frogger_modify_color(UINT8 *color);
static void gmgalax_modify_color(UINT8 *color);
static void drivfrcg_modify_color(UINT8 *color);

static void (*modify_ypos)(UINT8*);	/* function to call to do modify how vertical positioning bits are connected */
static void frogger_modify_ypos(UINT8 *sy);

static TIMER_CALLBACK( stars_blink_callback );
static TIMER_CALLBACK( stars_scroll_callback );

static void (*tilemap_set_scroll)( tilemap *, int col, int value );

/* star circuit */
#define STAR_COUNT  252
struct star
{
	int x,y,color;
};
static struct star stars[STAR_COUNT];
static int stars_colors_start;
       UINT8 galaxian_stars_on;
static INT32 stars_scrollpos;
static UINT8 stars_blink_state;
static emu_timer *stars_blink_timer;
static emu_timer *stars_scroll_timer;
static UINT8 timer_adjusted;
       void galaxian_init_stars(running_machine *machine, int colors_offset);
static void (*draw_stars)(running_machine *, bitmap_t *);		/* function to call to draw the star layer */
static void     noop_draw_stars(running_machine *machine, bitmap_t *bitmap);
       void galaxian_draw_stars(running_machine *machine, bitmap_t *bitmap);
static 	   void scramble_draw_stars(running_machine *machine, bitmap_t *bitmap);
static void   rescue_draw_stars(running_machine *machine, bitmap_t *bitmap);
static void  mariner_draw_stars(running_machine *machine, bitmap_t *bitmap);
static void  jumpbug_draw_stars(running_machine *machine, bitmap_t *bitmap);
static void start_stars_blink_timer(double ra, double rb, double c);
static void start_stars_scroll_timer(void);

/* bullets circuit */
static UINT8 darkplnt_bullet_color;
static void (*draw_bullets)(running_machine *, bitmap_t *,int,int,int);	/* function to call to draw a bullet */
static void galaxian_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);
static void gteikob2_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);
static void scramble_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);
static void   theend_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);
static void darkplnt_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);
static void dambustr_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y);

/* background circuit */
static UINT8 background_enable;
static UINT8 background_red, background_green, background_blue;
static void (*draw_background)(running_machine *, bitmap_t *);	/* function to call to draw the background */
static void galaxian_draw_background(running_machine *machine, bitmap_t *bitmap);
static void scramble_draw_background(running_machine *machine, bitmap_t *bitmap);
static void  turtles_draw_background(running_machine *machine, bitmap_t *bitmap);
static void  mariner_draw_background(running_machine *machine, bitmap_t *bitmap);
static void  frogger_draw_background(running_machine *machine, bitmap_t *bitmap);
static void stratgyx_draw_background(running_machine *machine, bitmap_t *bitmap);
static void  minefld_draw_background(running_machine *machine, bitmap_t *bitmap);
static void   rescue_draw_background(running_machine *machine, bitmap_t *bitmap);
static void dambustr_draw_background(running_machine *machine, bitmap_t *bitmap);

static UINT16 rockclim_v;
static UINT16 rockclim_h;
static int dambustr_bg_split_line;
static int dambustr_bg_color_1;
static int dambustr_bg_color_2;
static int dambustr_bg_priority;
static int dambustr_char_bank;
static bitmap_t *dambustr_tmpbitmap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Galaxian has one 32 bytes palette PROM, connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

  The blue background in Scramble and other games goes through a 390 ohm
  resistor.

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
PALETTE_INIT( galaxian )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < memory_region_length(REGION_PROMS);i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = BIT(*color_prom,6);
		bit1 = BIT(*color_prom,7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}


	galaxian_init_stars(machine, STARS_COLOR_BASE);


	/* bullets - yellow and white */
	palette_set_color(machine,BULLETS_COLOR_BASE+0,MAKE_RGB(0xef,0xef,0x00));
	palette_set_color(machine,BULLETS_COLOR_BASE+1,MAKE_RGB(0xef,0xef,0xef));
}

PALETTE_INIT( scramble )
{
	PALETTE_INIT_CALL(galaxian);


	/* blue background - 390 ohm resistor */
	palette_set_color(machine,BACKGROUND_COLOR_BASE,MAKE_RGB(0,0,0x56));
}

PALETTE_INIT( moonwar )
{
	PALETTE_INIT_CALL(scramble);


	/* wire mod to connect the bullet blue output to the 220 ohm resistor */
	palette_set_color(machine,BULLETS_COLOR_BASE+0,MAKE_RGB(0xef,0xef,0x97));
}

PALETTE_INIT( turtles )
{
	int i;


	PALETTE_INIT_CALL(galaxian);


	/*  The background color generator is connected this way:

        RED   - 390 ohm resistor
        GREEN - 470 ohm resistor
        BLUE  - 390 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		int r = BIT(i,0) * 0x55;
		int g = BIT(i,1) * 0x47;
		int b = BIT(i,2) * 0x55;

		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( stratgyx )
{
	int i;


	PALETTE_INIT_CALL(galaxian);


	/*  The background color generator is connected this way:

        RED   - 270 ohm resistor
        GREEN - 560 ohm resistor
        BLUE  - 470 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		int r = BIT(i,0) * 0x7c;
		int g = BIT(i,1) * 0x3c;
		int b = BIT(i,2) * 0x47;

		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( frogger )
{
	PALETTE_INIT_CALL(galaxian);


	/* blue background - 470 ohm resistor */
	palette_set_color(machine,BACKGROUND_COLOR_BASE,MAKE_RGB(0,0,0x47));
}

PALETTE_INIT( rockclim )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < memory_region_length(REGION_PROMS);i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = BIT(*color_prom,6);
		bit1 = BIT(*color_prom,7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}
}
/***************************************************************************

  Convert the color PROMs into a more useable format.

  Dark Planet has one 32 bytes palette PROM, connected to the RGB output this way:

  bit 5 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
PALETTE_INIT( darkplnt )
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(*color_prom,0);
		bit1 = BIT(*color_prom,1);
		bit2 = BIT(*color_prom,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		g = 0x00;
		/* blue component */
		bit0 = BIT(*color_prom,3);
		bit1 = BIT(*color_prom,4);
		bit2 = BIT(*color_prom,5);
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color_rgb(machine,i,r,g,b);
		color_prom++;
	}


	/* bullets - red and blue */
	palette_set_color(machine,BULLETS_COLOR_BASE+0,MAKE_RGB(0xef,0x00,0x00));
	palette_set_color(machine,BULLETS_COLOR_BASE+1,MAKE_RGB(0x00,0x00,0xef));
}

PALETTE_INIT( minefld )
{
	int i;


	PALETTE_INIT_CALL(galaxian);


	/* set up background colors */

	/* graduated blue */

	for (i = 0; i < 128; i++)
	{
		int r = 0;
		int g = i;
		int b = i * 2;
		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}

	/* graduated brown */

	for (i = 0; i < 128; i++)
	{
		int r = i * 1.5;
		int g = i * 0.75;
		int b = i / 2;
		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+128+i,r,g,b);
	}
}

PALETTE_INIT( rescue )
{
	int i;


	PALETTE_INIT_CALL(galaxian);


	/* set up background colors */

	/* graduated blue */

	for (i = 0; i < 128; i++)
	{
		int r = 0;
		int g = i;
		int b = i * 2;
		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}

PALETTE_INIT( mariner )
{
	int i;


	PALETTE_INIT_CALL(galaxian);


	/* set up background colors */

	/* 16 shades of blue - the 4 bits are connected to the following resistors:

        bit 0 -- 4.7 kohm resistor
              -- 2.2 kohm resistor
              -- 1   kohm resistor
        bit 0 -- .47 kohm resistor */

	for (i = 0; i < 16; i++)
	{
		int r,g,b;

		r = 0;
		g = 0;
		b = 0x0e * BIT(i,0) + 0x1f * BIT(i,1) + 0x43 * BIT(i,2) + 0x8f * BIT(i,3);

		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}


PALETTE_INIT( dambustr )
{
	int i;

	PALETTE_INIT_CALL(galaxian);


	/*
    Assumption (not clear from the schematics):
    The background color generator is connected this way:

        RED   - 470 ohm resistor
        GREEN - 470 ohm resistor
        BLUE  - 470 ohm resistor */


	for (i = 0; i < 8; i++)
	{
		int r = BIT(i,0) * 0x47;
		int g = BIT(i,1) * 0x47;
		int b = BIT(i,2) * 0x4f;
		palette_set_color_rgb(machine,BACKGROUND_COLOR_BASE+i,r,g,b);
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

static void state_save_register(void)
{
	state_save_register_global_array(gfxbank);
	state_save_register_global(flipscreen_x);
	state_save_register_global(flipscreen_y);

	state_save_register_global(galaxian_stars_on);
	state_save_register_global(stars_scrollpos);
	state_save_register_global(stars_blink_state);

	state_save_register_global(darkplnt_bullet_color);

	state_save_register_global(background_enable);
	state_save_register_global(background_red);
	state_save_register_global(background_green);
	state_save_register_global(background_blue);
}

static void video_start_common(running_machine *machine, tilemap_mapper_func get_memory_offset)
{
	bg_tilemap = tilemap_create(get_tile_info,get_memory_offset,8,8,32,32);

	tilemap_set_transparent_pen(bg_tilemap,0);


	modify_charcode = 0;
	modify_spritecode = 0;
	modify_color = 0;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = galaxian_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flipscreen_x = 0;
	flipscreen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = (machine->gfx[0]->color_granularity == 4) ? 7 : 3;

	state_save_register();
}

VIDEO_START( galaxian_plain )
{
	video_start_common(machine,tilemap_scan_rows);

	tilemap_set_scroll_cols(bg_tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;
}

VIDEO_START( galaxian )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_stars = galaxian_draw_stars;

	draw_bullets = galaxian_draw_bullets;
}

VIDEO_START( gmgalax )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = gmgalax_modify_charcode;
	modify_spritecode = gmgalax_modify_spritecode;
	modify_color      = gmgalax_modify_color;
}

VIDEO_START( mooncrst )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = mooncrst_modify_charcode;
	modify_spritecode = mooncrst_modify_spritecode;
}

VIDEO_START( mooncrgx )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = mooncrgx_modify_charcode;
	modify_spritecode = mooncrgx_modify_spritecode;
}

VIDEO_START( moonqsr )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = moonqsr_modify_charcode;
	modify_spritecode = moonqsr_modify_spritecode;
}

VIDEO_START( mshuttle )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = mshuttle_modify_charcode;
	modify_spritecode = mshuttle_modify_spritecode;
}

VIDEO_START( pisces )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = pisces_modify_spritecode;
}

VIDEO_START( gteikob2 )
{
	VIDEO_START_CALL(pisces);

	draw_bullets = gteikob2_draw_bullets;
}

VIDEO_START( batman2 )
{
	VIDEO_START_CALL(galaxian);

	modify_charcode   = batman2_modify_charcode;
	modify_spritecode = batman2_modify_spritecode;

}

VIDEO_START( scramble )
{
	VIDEO_START_CALL(galaxian_plain);

	/* FIXME: This most probably needs to be adjusted
     * again when RAW video params are added to scramble
     */
	tilemap_set_scrolldx(bg_tilemap, 0, 0);

	draw_stars = scramble_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;
}

VIDEO_START( sfx )
{
	video_start_common(machine,tilemap_scan_cols);

	tilemap_set_scroll_rows(bg_tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrollx;

	draw_stars = scramble_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = turtles_draw_background;
}

VIDEO_START( turtles )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_background = turtles_draw_background;
}

VIDEO_START( theend )
{
	VIDEO_START_CALL(galaxian);

	draw_bullets = theend_draw_bullets;
}

VIDEO_START( darkplnt )
{
	VIDEO_START_CALL(galaxian_plain);

	tilemap_set_scrolldx(bg_tilemap, 0, 0);
	draw_bullets = darkplnt_draw_bullets;
}

VIDEO_START( rescue )
{
	VIDEO_START_CALL(scramble);

	draw_stars = rescue_draw_stars;

	draw_background = rescue_draw_background;
}

VIDEO_START( minefld )
{
	VIDEO_START_CALL(scramble);

	draw_stars = rescue_draw_stars;

	draw_background = minefld_draw_background;
}

VIDEO_START( stratgyx )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_background = stratgyx_draw_background;
}

VIDEO_START( ckongs )
{
	VIDEO_START_CALL(scramble);

	modify_spritecode = mshuttle_modify_spritecode;
}

VIDEO_START( calipso )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;

	modify_spritecode = calipso_modify_spritecode;
}

VIDEO_START( mariner )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_stars = mariner_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = mariner_draw_background;

	modify_charcode = mariner_modify_charcode;
}

VIDEO_START( froggers )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_background = frogger_draw_background;
	modify_color = frogger_modify_color;
}

VIDEO_START( frogger )
{
	VIDEO_START_CALL(froggers);

	modify_ypos = frogger_modify_ypos;
}

VIDEO_START( jumpbug )
{
	VIDEO_START_CALL(scramble);

	draw_stars = jumpbug_draw_stars;

	modify_charcode   = jumpbug_modify_charcode;
	modify_spritecode = jumpbug_modify_spritecode;
}

VIDEO_START( azurian )
{
	VIDEO_START_CALL(galaxian_plain);

	draw_stars = galaxian_draw_stars;
	draw_bullets = scramble_draw_bullets; /* Shots are yellow like in Scramble */
}

VIDEO_START( mimonkey )
{
	VIDEO_START_CALL(scramble);

	modify_charcode   = mimonkey_modify_charcode;
	modify_spritecode = mimonkey_modify_spritecode;
}

VIDEO_START( dkongjrm )
{
	VIDEO_START_CALL(galaxian_plain);

	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = dkongjrm_modify_spritecode;

	spriteram2_present= 1;
}

VIDEO_START( newsin7 )
{
	VIDEO_START_CALL(scramble);

	spritevisiblearea      = &_spritevisibleareaflipx;
	spritevisibleareaflipx = &_spritevisiblearea;
}

VIDEO_START( scorpion )
{
	VIDEO_START_CALL(scramble);

	modify_spritecode = batman2_modify_spritecode;
}

static void rockclim_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	tilemap_draw(bitmap,0,rockclim_tilemap, 0,0);
}

static void rockclim_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2])	*code|=0x40;
}

VIDEO_START( rockclim )
{
	VIDEO_START_CALL(galaxian);
	rockclim_tilemap = tilemap_create(rockclim_get_tile_info,tilemap_scan_rows,8,8,64,32);
	draw_background = rockclim_draw_background;
	modify_charcode = mooncrst_modify_charcode;
	modify_spritecode = rockclim_modify_spritecode;
	rockclim_v = rockclim_h = 0;
	state_save_register_global(rockclim_v);
	state_save_register_global(rockclim_h);
}

static TILE_GET_INFO( drivfrcg_get_tile_info )
{
	int code = galaxian_videoram[tile_index];
	UINT8 x = tile_index & 0x1f;
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & 7;
	UINT8 bank = galaxian_attributesram[(x << 1) | 1] & 0x30;

	code |= (bank << 4);
	color |= ((galaxian_attributesram[(x << 1) | 1] & 0x40) >> 3);

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( drivfrcg )
{
	bg_tilemap = tilemap_create(drivfrcg_get_tile_info,tilemap_scan_rows,8,8,32,32);

	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_scroll_cols(bg_tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;

	modify_charcode = 0;
	modify_spritecode = mshuttle_modify_spritecode;
	modify_color = drivfrcg_modify_color;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = galaxian_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flipscreen_x = 0;
	flipscreen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = 0xff;

	state_save_register();
}

VIDEO_START( ad2083 )
{
	bg_tilemap = tilemap_create(drivfrcg_get_tile_info,tilemap_scan_rows,8,8,32,32);

	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_scroll_cols(bg_tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;

	modify_charcode = 0;
	modify_spritecode = ad2083_modify_spritecode;
	modify_color = 0;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = scramble_draw_bullets;

	draw_background = turtles_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flipscreen_x = 0;
	flipscreen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = 7;

	state_save_register();
}

UINT8 *racknrol_tiles_bank;

WRITE8_HANDLER( racknrol_tiles_bank_w )
{
	racknrol_tiles_bank[offset] = data;
	tilemap_mark_all_tiles_dirty(bg_tilemap);
}

static TILE_GET_INFO( racknrol_get_tile_info )
{
	int code = galaxian_videoram[tile_index];
	UINT8 x = tile_index & 0x1f;
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & 7;
	UINT8 bank = racknrol_tiles_bank[x] & 7;

	code |= (bank << 8);

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( racknrol )
{
	bg_tilemap = tilemap_create(racknrol_get_tile_info,tilemap_scan_rows,8,8,32,32);

	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_scroll_cols(bg_tilemap, 32);
	tilemap_set_scroll = tilemap_set_scrolly;

	modify_charcode = 0;
	modify_spritecode = 0;
	modify_color = 0;
	modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = galaxian_draw_background;
	background_enable = 0;
	background_blue = 0;
	background_red = 0;
	background_green = 0;

	draw_stars = noop_draw_stars;

	flipscreen_x = 0;
	flipscreen_y = 0;

	spriteram2_present = 0;

	spritevisiblearea      = &_spritevisiblearea;
	spritevisibleareaflipx = &_spritevisibleareaflipx;

	color_mask = 0xff;

	state_save_register();
}

VIDEO_START( bongo )
{
	VIDEO_START_CALL(galaxian_plain);

	modify_spritecode = batman2_modify_spritecode;
}

static TILE_GET_INFO( dambustr_get_tile_info2 )
{
	UINT8 x = tile_index & 0x1f;

	UINT16 code = dambustr_videoram2[tile_index];
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & color_mask;

	if (modify_charcode)
	{
		modify_charcode(&code, x);
	}

	if (modify_color)
	{
		modify_color(&color);
	}

	SET_TILE_INFO(0, code, color, 0);
}

VIDEO_START( dambustr )
{
	VIDEO_START_CALL(galaxian);

	dambustr_bg_split_line = 0;
	dambustr_bg_color_1 = 0;
	dambustr_bg_color_2 = 0;
	dambustr_bg_priority = 0;
	dambustr_char_bank = 0;

	draw_background = dambustr_draw_background;

	modify_charcode   = dambustr_modify_charcode;
	modify_spritecode = dambustr_modify_spritecode;

	draw_bullets = dambustr_draw_bullets;

	/* allocate the temporary bitmap for the background priority */
	dambustr_tmpbitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, machine->screen[0].format);

	/* make a copy of the tilemap to emulate background priority */
	dambustr_videoram2 = auto_malloc(0x0400);
	dambustr_tilemap2 = tilemap_create(dambustr_get_tile_info2,tilemap_scan_rows,8,8,32,32);

	tilemap_set_transparent_pen(dambustr_tilemap2,0);
}


WRITE8_HANDLER( galaxian_videoram_w )
{
	galaxian_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

READ8_HANDLER( galaxian_videoram_r )
{
	return galaxian_videoram[offset];
}


WRITE8_HANDLER( galaxian_attributesram_w )
{
	if (galaxian_attributesram[offset] != data)
	{
		if (offset & 0x01)
		{
			/* color change */
			int i;

			for (i = offset >> 1; i < 0x0400; i += 32)
				tilemap_mark_tile_dirty(bg_tilemap, i);
		}
		else
		{
			if (modify_ypos)
			{
				modify_ypos(&data);
			}

			tilemap_set_scroll(bg_tilemap, offset >> 1, data);
		}

		galaxian_attributesram[offset] = data;
	}
}


WRITE8_HANDLER( galaxian_flip_screen_x_w )
{
	if (flipscreen_x != (data & 0x01))
	{
		flipscreen_x = data & 0x01;

		tilemap_set_flip(bg_tilemap, (flipscreen_x ? TILEMAP_FLIPX : 0) | (flipscreen_y ? TILEMAP_FLIPY : 0));
	}
}

WRITE8_HANDLER( galaxian_flip_screen_y_w )
{
	if (flipscreen_y != (data & 0x01))
	{
		flipscreen_y = data & 0x01;

		tilemap_set_flip(bg_tilemap, (flipscreen_x ? TILEMAP_FLIPX : 0) | (flipscreen_y ? TILEMAP_FLIPY : 0));
	}
}


WRITE8_HANDLER( gteikob2_flip_screen_x_w )
{
	galaxian_flip_screen_x_w(machine, offset, ~data);
}

WRITE8_HANDLER( gteikob2_flip_screen_y_w )
{
	galaxian_flip_screen_y_w(machine, offset, ~data);
}


WRITE8_HANDLER( hotshock_flip_screen_w )
{
	galaxian_flip_screen_x_w(machine, offset, data);
	galaxian_flip_screen_y_w(machine, offset, data);
}


WRITE8_HANDLER( scramble_background_enable_w )
{
	background_enable = data & 0x01;
}

WRITE8_HANDLER( scramble_background_red_w )
{
	background_red = data & 0x01;
}

WRITE8_HANDLER( scramble_background_green_w )
{
	background_green = data & 0x01;
}

WRITE8_HANDLER( scramble_background_blue_w )
{
	background_blue = data & 0x01;
}


WRITE8_HANDLER( galaxian_stars_enable_w )
{
	galaxian_stars_on = data & 0x01;

	if (!galaxian_stars_on)
	{
		stars_scrollpos = 0;
	}
}


WRITE8_HANDLER( darkplnt_bullet_color_w )
{
	darkplnt_bullet_color = data & 0x01;
}



WRITE8_HANDLER( galaxian_gfxbank_w )
{
	if (gfxbank[offset] != data)
	{
		gfxbank[offset] = data;

		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE8_HANDLER( rockclim_videoram_w )
{
	rockclim_videoram[offset] = data;
	tilemap_mark_tile_dirty(rockclim_tilemap, offset);
}

WRITE8_HANDLER( rockclim_scroll_w )
{
	switch(offset&3)
	{
		case 0: rockclim_h=(rockclim_h&0xff00)|data;tilemap_set_scrollx(rockclim_tilemap , 0, rockclim_h );break;
		case 1:	rockclim_h=(rockclim_h&0xff)|(data<<8);tilemap_set_scrollx(rockclim_tilemap , 0, rockclim_h );break;
		case 2:	rockclim_v=(rockclim_v&0xff00)|data;tilemap_set_scrolly(rockclim_tilemap , 0, rockclim_v );break;
		case 3:	rockclim_v=(rockclim_v&0xff)|(data<<8);tilemap_set_scrolly(rockclim_tilemap , 0, rockclim_v );break;
	}

}


READ8_HANDLER( rockclim_videoram_r )
{
	return rockclim_videoram[offset];
}


WRITE8_HANDLER( dambustr_bg_split_line_w )
{
	dambustr_bg_split_line = data;
}


WRITE8_HANDLER( dambustr_bg_color_w )
{
	dambustr_bg_color_1 = (BIT(data,2)<<2) | (BIT(data,1)<<1) | BIT(data,0);
	dambustr_bg_color_2 = (BIT(data,6)<<2) | (BIT(data,5)<<1) | BIT(data,4);
	dambustr_bg_priority = BIT(data,3);
	dambustr_char_bank = BIT(data,7);
	tilemap_mark_all_tiles_dirty(bg_tilemap);
}



/* character banking functions */

static void gmgalax_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 9);
}

static void mooncrst_modify_charcode(UINT16 *code,UINT8 x)
{
	if (gfxbank[2] && ((*code & 0xc0) == 0x80))
	{
		*code = (*code & 0x3f) | (gfxbank[0] << 6) | (gfxbank[1] << 7) | 0x0100;
	}
}

static void mooncrgx_modify_charcode(UINT16 *code,UINT8 x)
{
	if (gfxbank[2] && ((*code & 0xc0) == 0x80))
	{
		*code = (*code & 0x3f) | (gfxbank[1] << 6) | (gfxbank[0] << 7) | 0x0100;
	}
}

static void moonqsr_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= ((galaxian_attributesram[(x << 1) | 1] & 0x20) << 3);
}

static void mshuttle_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= ((galaxian_attributesram[(x << 1) | 1] & 0x30) << 4);
}

static void pisces_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 8);
}

static void mimonkey_modify_charcode(UINT16 *code,UINT8 x)
{
	*code |= (gfxbank[0] << 8) | (gfxbank[2] << 9);
}

static void batman2_modify_charcode(UINT16 *code,UINT8 x)
{
	if (*code & 0x80)
	{
		*code |= (gfxbank[0] << 8);
	}
}

static void mariner_modify_charcode(UINT16 *code,UINT8 x)
{
	UINT8 *prom;


	/* bit 0 of the PROM controls character banking */

	prom = memory_region(REGION_USER2);

	*code |= ((prom[x] & 0x01) << 8);
}

static void jumpbug_modify_charcode(UINT16 *code,UINT8 x)
{
	if (((*code & 0xc0) == 0x80) &&
		 (gfxbank[2] & 0x01))
	{
		*code += 128 + (( gfxbank[0] & 0x01) << 6) +
					   (( gfxbank[1] & 0x01) << 7) +
					   ((~gfxbank[4] & 0x01) << 8);
	}
}


static void dambustr_modify_charcode(UINT16 *code,UINT8 x)
{
	if (dambustr_char_bank == 0) {	// text mode
		*code |= 0x0300;
	}
	else {				// graphics mode
		if (x == 28)		// only line #28 stays in text mode
			*code |= 0x0300;
		else
			*code &= 0x00ff;
	};
}



/* sprite banking functions */

static void gmgalax_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 7) | 0x40;
}

static void mooncrst_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2] && ((*code & 0x30) == 0x20))
	{
		*code = (*code & 0x0f) | (gfxbank[0] << 4) | (gfxbank[1] << 5) | 0x40;
	}
}

static void mooncrgx_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (gfxbank[2] && ((*code & 0x30) == 0x20))
	{
		*code = (*code & 0x0f) | (gfxbank[1] << 4) | (gfxbank[0] << 5) | 0x40;
	}
}

static void moonqsr_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= ((spriteram[offs + 2] & 0x20) << 1);
}

static void mshuttle_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= ((spriteram[offs + 2] & 0x30) << 2);
}

static void calipso_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* No flips */
	*code = spriteram[offs + 1];
	*flipx = 0;
	*flipy = 0;
}

static void pisces_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 6);
}

static void mimonkey_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code |= (gfxbank[0] << 6) | (gfxbank[2] << 7);
}

static void batman2_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* only the upper 64 sprites are used */
	*code |= 0x40;
}

static void jumpbug_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	if (((*code & 0x30) == 0x20) &&
		 (gfxbank[2] & 0x01) != 0)
	{
		*code += 32 + (( gfxbank[0] & 0x01) << 4) +
					  (( gfxbank[1] & 0x01) << 5) +
					  ((~gfxbank[4] & 0x01) << 6);
	}
}

static void dkongjrm_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* No x flip */
	*code = (spriteram[offs + 1] & 0x7f) | 0x80;
	*flipx = 0;
}

static void ad2083_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	/* No x flip */
	*code = (spriteram[offs + 1] & 0x7f) | ((spriteram[offs + 2] & 0x30) << 2);
	*flipx = 0;
}

static void dambustr_modify_spritecode(UINT8 *spriteram,int *code,int *flipx,int *flipy,int offs)
{
	*code += 0x40;
}


/* color PROM mapping functions */

static void frogger_modify_color(UINT8 *color)
{
	*color = ((*color >> 1) & 0x03) | ((*color << 2) & 0x04);
}

static void gmgalax_modify_color(UINT8 *color)
{
	*color |= (gfxbank[0] << 3);
}

static void drivfrcg_modify_color(UINT8 *color)
{
	*color = ((*color & 0x40) >> 3) | (*color & 7);
}

/* y position mapping functions */

static void frogger_modify_ypos(UINT8 *sy)
{
	*sy = (*sy << 4) | (*sy >> 4);
}


/* bullet drawing functions */

static void galaxian_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	int i;


	for (i = 0; i < 4; i++)
	{
		x--;

		if (x >= machine->screen[0].visarea.min_x &&
			x <= machine->screen[0].visarea.max_x)
		{
			int color;


			/* yellow missile, white shells (this is the terminology on the schematics) */
			color = ((offs == 7*4) ? BULLETS_COLOR_BASE : BULLETS_COLOR_BASE + 1);

			*BITMAP_ADDR16(bitmap, y, x) = color;
		}
	}
}

static void gteikob2_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	galaxian_draw_bullets(machine, bitmap, offs, 260 - x, y);
}

static void scramble_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	if (flipscreen_x)  x++;

	x = x - 6;

	if (x >= machine->screen[0].visarea.min_x &&
		x <= machine->screen[0].visarea.max_x)
	{
		/* yellow bullets */
		*BITMAP_ADDR16(bitmap, y, x) = BULLETS_COLOR_BASE;
	}
}

static void darkplnt_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	if (flipscreen_x)  x++;

	x = x - 6;

	if (x >= machine->screen[0].visarea.min_x &&
		x <= machine->screen[0].visarea.max_x)
	{
		*BITMAP_ADDR16(bitmap, y, x) = 32 + darkplnt_bullet_color;
	}
}

static void theend_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	int i;


	/* same as Galaxian, but all bullets are yellow */
	for (i = 0; i < 4; i++)
	{
		x--;

		if (x >= machine->screen[0].visarea.min_x &&
			x <= machine->screen[0].visarea.max_x)
		{
			*BITMAP_ADDR16(bitmap, y, x) = BULLETS_COLOR_BASE;
		}
	}
}

static void dambustr_draw_bullets(running_machine *machine, bitmap_t *bitmap, int offs, int x, int y)
{
	int i, color;

	if (flip_screen_x_get())  x++;

	x = x - 6;

	/* bullets are 2 pixels wide */
	for (i = 0; i < 2; i++)
	{
		if (offs < 4*4)
		{
			color = BULLETS_COLOR_BASE;
			y--;
		}
		else {
			color = BULLETS_COLOR_BASE + 1;
			x--;
		}

		if (x >= machine->screen[0].visarea.min_x &&
			x <= machine->screen[0].visarea.max_x)
		{
			*BITMAP_ADDR16(bitmap, y, x) = color;
		}
	}
}



/* background drawing functions */

static void galaxian_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	/* plain black background */
	fillbitmap(bitmap,0,&machine->screen[0].visarea);
}

static void scramble_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	if (background_enable)
		fillbitmap(bitmap,BACKGROUND_COLOR_BASE,&machine->screen[0].visarea);
	else
		fillbitmap(bitmap,0,&machine->screen[0].visarea);
}

static void turtles_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	int color = (background_blue << 2) | (background_green << 1) | background_red;

	fillbitmap(bitmap,BACKGROUND_COLOR_BASE + color,&machine->screen[0].visarea);
}

static void frogger_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	/* color split point verified on real machine */
	if (flipscreen_x)
	{
		plot_box(bitmap,   0, 0, 128, 256, 0);
		plot_box(bitmap, 128, 0, 128, 256, BACKGROUND_COLOR_BASE);
	}
	else
	{
		plot_box(bitmap,   0, 0, 128, 256, BACKGROUND_COLOR_BASE);
		plot_box(bitmap, 128, 0, 128, 256, 0);
	}
}

static void stratgyx_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	UINT8 x;
	UINT8 *prom;


	/* the background PROM is connected the following way:

       bit 0 = 0 enables the blue gun if BCB is asserted
       bit 1 = 0 enables the red gun if BCR is asserted and
                 the green gun if BCG is asserted
       bits 2-7 are unconnected */

	prom = memory_region(REGION_USER1);

	for (x = 0; x < 32; x++)
	{
		int sx,color;


		color = 0;

		if ((~prom[x] & 0x02) && background_red)   color |= 0x01;
		if ((~prom[x] & 0x02) && background_green) color |= 0x02;
		if ((~prom[x] & 0x01) && background_blue)  color |= 0x04;

		if (flipscreen_x)
			sx = 8 * (31 - x);
		else
			sx = 8 * x;

		plot_box(bitmap, sx, 0, 8, 256, BACKGROUND_COLOR_BASE + color);
	}
}

static void minefld_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	if (background_enable)
	{
		int x;


		for (x = 0; x < 128; x++)
			plot_box(bitmap, x,       0, 1, 256, BACKGROUND_COLOR_BASE + x);

		for (x = 0; x < 120; x++)
			plot_box(bitmap, x + 128, 0, 1, 256, BACKGROUND_COLOR_BASE + x + 128);

		plot_box(bitmap, 248, 0, 16, 256, BACKGROUND_COLOR_BASE);
	}
	else
		fillbitmap(bitmap,0,&machine->screen[0].visarea);
}

static void rescue_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	if (background_enable)
	{
		int x;

		for (x = 0; x < 128; x++)
			plot_box(bitmap, x,       0, 1, 256, BACKGROUND_COLOR_BASE + x);

		for (x = 0; x < 120; x++)
			plot_box(bitmap, x + 128, 0, 1, 256, BACKGROUND_COLOR_BASE + x + 8);

		plot_box(bitmap, 248, 0, 16, 256, BACKGROUND_COLOR_BASE);
	}
	else
		fillbitmap(bitmap,0,&machine->screen[0].visarea);
}

static void mariner_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	UINT8 x;
	UINT8 *prom;


	/* the background PROM contains the color codes for each 8 pixel
       line (column) of the screen.  The first 0x20 bytes for unflipped,
       and the 2nd 0x20 bytes for flipped screen. */

	prom = memory_region(REGION_USER1);

	if (flipscreen_x)
	{
		for (x = 0; x < 32; x++)
		{
			int color;

			if (x == 0)
				color = 0;
			else
				color = prom[0x20 + x - 1];

			plot_box(bitmap, 8 * (31 - x), 0, 8, 256, BACKGROUND_COLOR_BASE + color);
		}
	}
	else
	{
		for (x = 0; x < 32; x++)
		{
			int color;

			if (x == 31)
				color = 0;
			else
				color = prom[x + 1];

			plot_box(bitmap, 8 * x, 0, 8, 256, BACKGROUND_COLOR_BASE + color);
		}
	}
}

static void dambustr_draw_background(running_machine *machine, bitmap_t *bitmap)
{
	int col1 = BACKGROUND_COLOR_BASE + dambustr_bg_color_1;
	int col2 = BACKGROUND_COLOR_BASE + dambustr_bg_color_2;

	if (flip_screen_x_get())
	{
		plot_box(bitmap,   0, 0, 256-dambustr_bg_split_line, 256, col2);
		plot_box(bitmap, 256-dambustr_bg_split_line, 0, dambustr_bg_split_line, 256, col1);
	}
	else
	{
		plot_box(bitmap,   0, 0, 256-dambustr_bg_split_line, 256, col1);
		plot_box(bitmap, 256-dambustr_bg_split_line, 0, dambustr_bg_split_line, 256, col2);
	}

}

static void dambustr_draw_upper_background(bitmap_t *bitmap)
{
	static rectangle clip = { 0, 0, 0, 0 };

	if (flip_screen_x_get())
	{
		clip.min_x = 254 - dambustr_bg_split_line;
		clip.max_x = dambustr_bg_split_line;
		clip.min_y = 0;
		clip.max_y = 255;
		copybitmap(bitmap, dambustr_tmpbitmap, 0, 0, 0, 0, &clip);
	}
	else
	{
		clip.min_x = 0;
		clip.max_x = 254 - dambustr_bg_split_line;
		clip.min_y = 0;
		clip.max_y = 255;
		copybitmap(bitmap, dambustr_tmpbitmap, 0, 0, 0, 0, &clip);
	}
}



/* star drawing functions */

void galaxian_init_stars(running_machine *machine, int colors_offset)
{
	int i;
	int total_stars;
	UINT32 generator;
	int x,y;


	galaxian_stars_on = 0;
	stars_blink_state = 0;
	stars_blink_timer = timer_alloc(stars_blink_callback, NULL);
	stars_scroll_timer = timer_alloc(stars_scroll_callback, NULL);
	timer_adjusted = 0;
	stars_colors_start = colors_offset;

	for (i = 0;i < 64;i++)
	{
		int bits,r,g,b;
		static const int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		r = map[bits];
		bits = (i >> 2) & 0x03;
		g = map[bits];
		bits = (i >> 4) & 0x03;
		b = map[bits];
		palette_set_color_rgb(machine,colors_offset+i,r,g,b);
	}


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 0;y < 256;y++)
	{
		for (x = 0;x < 512;x++)
		{
			UINT32 bit0;


			bit0 = ((~generator >> 16) & 0x01) ^ ((generator >> 4) & 0x01);

			generator = (generator << 1) | bit0;

			if (((~generator >> 16) & 0x01) && (generator & 0xff) == 0xff)
			{
				int color;


				color = (~(generator >> 8)) & 0x3f;
				if (color)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].color = color;

					total_stars++;
				}
			}
		}
	}

	if (total_stars != STAR_COUNT)
	{
		fatalerror("total_stars = %d, STAR_COUNT = %d",total_stars,STAR_COUNT);
	}
}

static void plot_star(running_machine *machine, bitmap_t *bitmap, int x, int y, int color)
{
	if (y < machine->screen[0].visarea.min_y ||
		y > machine->screen[0].visarea.max_y ||
		x < machine->screen[0].visarea.min_x ||
		x > machine->screen[0].visarea.max_x)
		return;


	if (flipscreen_x)
		x = 255 - x;

	if (flipscreen_y)
		y = 255 - y;

	*BITMAP_ADDR16(bitmap, y, x) = stars_colors_start + color;
}

static void noop_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
}

void galaxian_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   stars_scrollpos) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			plot_star(machine, bitmap, x, y, stars[offs].color);
		}
	}
}

static void scramble_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(machine, bitmap, x, y, stars[offs].color);
		}
	}
}

static void rescue_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
	int offs;


	/* same as Scramble, but only top (left) half of screen */

	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((x < 128) && ((y & 0x01) ^ ((x >> 3) & 0x01)))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(machine, bitmap, x, y, stars[offs].color);
		}
	}
}

static void mariner_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
	int offs;
	UINT8 *prom;


	if (!timer_adjusted)
	{
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	/* bit 2 of the PROM controls star visibility */

	prom = memory_region(REGION_USER2);

	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   -stars_scrollpos) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((-stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			if (prom[(x/8 + 1) & 0x1f] & 0x04)
			{
				plot_star(machine, bitmap, x, y, stars[offs].color);
			}
		}
	}
}

static void jumpbug_draw_stars(running_machine *machine, bitmap_t *bitmap)
{
	int offs;


	if (!timer_adjusted)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
		start_stars_scroll_timer();
		timer_adjusted = 1;
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		/* determine when to skip plotting */
		if ((y & 0x01) ^ ((x >> 3) & 0x01))
		{
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 0x01))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 0x04))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 0x02))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			x = ((stars[offs].x +   stars_scrollpos) & 0x01ff) >> 1;
			y = ( stars[offs].y + ((stars_scrollpos + stars[offs].x) >> 9)) & 0xff;

			/* no stars in the status area */
			if (x >= 240)  continue;

			plot_star(machine, bitmap, x, y, stars[offs].color);
		}
	}
}


static TIMER_CALLBACK( stars_blink_callback )
{
	stars_blink_state++;
}

static void start_stars_blink_timer(double ra, double rb, double c)
{
	/* calculate the period using the formula given in the 555 datasheet */

	int period_in_ms = 693 * (ra + 2.0 * rb) * c;

	timer_adjust_periodic(stars_blink_timer, ATTOTIME_IN_MSEC(period_in_ms), 0, ATTOTIME_IN_MSEC(period_in_ms));
}


static TIMER_CALLBACK( stars_scroll_callback )
{
	if (galaxian_stars_on)
	{
		stars_scrollpos++;
	}
}

static void start_stars_scroll_timer()
{
	timer_adjust_periodic(stars_scroll_timer, video_screen_get_frame_period(0), 0, video_screen_get_frame_period(0));
}



static TILE_GET_INFO( get_tile_info )
{
	UINT8 x = tile_index & 0x1f;

	UINT16 code = galaxian_videoram[tile_index];
	UINT8 color = galaxian_attributesram[(x << 1) | 1] & color_mask;

	if (modify_charcode)
	{
		modify_charcode(&code, x);
	}

	if (modify_color)
	{
		modify_color(&color);
	}

	SET_TILE_INFO(0, code, color, 0);
}

static TILE_GET_INFO( rockclim_get_tile_info )
{
	UINT16 code = rockclim_videoram[tile_index];
	SET_TILE_INFO(2, code, 0, 0);
}

static void draw_bullets_common(running_machine *machine, bitmap_t *bitmap)
{
	int offs;


	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		UINT8 sx,sy;

		sy = 255 - galaxian_bulletsram[offs + 1];
		sx = 255 - galaxian_bulletsram[offs + 3];

		if (sy < machine->screen[0].visarea.min_y ||
			sy > machine->screen[0].visarea.max_y)
			continue;

		if (flipscreen_y)  sy = 255 - sy;

		draw_bullets(machine, bitmap, offs, sx, sy);
	}
}


static void draw_sprites(running_machine *machine, bitmap_t *bitmap, UINT8 *spriteram, size_t spriteram_size)
{
	int offs;


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		UINT8 sx,sy,color;
		int flipx,flipy,code;


		sx = spriteram[offs + 3] + 1;	/* the existence of +1 is supported by a LOT of games */
		sy = spriteram[offs];			/* Anteater, Mariner, for example */
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;
		code = spriteram[offs + 1] & 0x3f;
		color = spriteram[offs + 2] & color_mask;

		if (modify_spritecode)
		{
			modify_spritecode(spriteram, &code, &flipx, &flipy, offs);
		}

		if (modify_color)
		{
			modify_color(&color);
		}

		if (modify_ypos)
		{
			modify_ypos(&sy);
		}

		if (flipscreen_x)
		{
			sx = 240 - sx;
			flipx = !flipx;
		}

		if (flipscreen_y)
		{
			flipy = !flipy;
		}
		else
		{
			sy = 240 - sy;
		}


		/* In at least Amidar Turtles, sprites #0, #1 and #2 need to be moved */
		/* down (left) one pixel to be positioned correctly. */
		/* Note that the adjustment must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */

		if (offs < 3*4)  sy++;


		drawgfx(bitmap,machine->gfx[1],
				code,color,
				flipx,flipy,
				sx,sy,
				flipscreen_x ? spritevisibleareaflipx : spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}


VIDEO_UPDATE( galaxian )
{
	draw_background(machine, bitmap);


	if (galaxian_stars_on)
	{
		draw_stars(machine, bitmap);
	}


	tilemap_draw(bitmap, 0, bg_tilemap, 0, 0);


	if (draw_bullets)
	{
		draw_bullets_common(machine, bitmap);
	}


	draw_sprites(machine, bitmap, galaxian_spriteram, galaxian_spriteram_size);

	if (spriteram2_present)
	{
		draw_sprites(machine, bitmap, galaxian_spriteram2, galaxian_spriteram2_size);
	}
	return 0;
}


VIDEO_UPDATE( dambustr )
{
	int i, j;
	UINT8 color;

	draw_background(machine, bitmap);

	if (galaxian_stars_on)
	{
		draw_stars(machine, bitmap);
	}

	/* save the background for drawing it again later, if background has priority over characters */
	copybitmap(dambustr_tmpbitmap, bitmap, 0, 0, 0, 0, &machine->screen[0].visarea);

	tilemap_draw(bitmap, 0, bg_tilemap, 0, 0);

	if (draw_bullets)
	{
		draw_bullets_common(machine, bitmap);
	}

	draw_sprites(machine, bitmap, galaxian_spriteram, galaxian_spriteram_size);

	if (dambustr_bg_priority)
	{
		/* draw the upper part of the background, as it has priority */
		dambustr_draw_upper_background(bitmap);

		/* only rows with color code > 3 are stronger than the background */
		memset(dambustr_videoram2, 0x20, 0x0400);
		for (i=0; i<32; i++) {
			color = galaxian_attributesram[(i << 1) | 1] & color_mask;
			if (color > 3) {
				for (j=0; j<32; j++)
					dambustr_videoram2[32*j+i] = galaxian_videoram[32*j+i];
			};
		};
		tilemap_mark_all_tiles_dirty(dambustr_tilemap2);
		tilemap_draw(bitmap, 0, dambustr_tilemap2, 0, 0);
	};

	return 0;
}

