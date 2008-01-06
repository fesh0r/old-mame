/***************************************************************************

  video\cvs.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "video/s2636.h"
#include "cpu/s2650/s2650.h"

#define MAX_STARS        250
#define STARS_COLOR_BASE 16


#ifdef LSB_FIRST
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#endif


struct star
{
	int x,y,code;
};


UINT8 *cvs_color_ram;
UINT8 *cvs_video_ram;
UINT8 *cvs_bullet_ram;

static UINT8 *palette_ram;
static UINT8 *character_ram;

static struct star stars[MAX_STARS];
static int    total_stars;
static int    stars_on;
static UINT8  character_mode;
static UINT16 character_ram_page;
static UINT8  scroll_reg;
static int    stars_scroll;

static mame_bitmap *background_bitmap;
mame_bitmap *cvs_collision_bitmap;
mame_bitmap *cvs_collision_background;
static mame_bitmap *scrolled_background;

int cvs_collision_register;


/******************************************************
 * Convert Colour prom to format for Mame Colour Map  *
 *                                                    *
 * There is a prom used for colour mapping and plane  *
 * priority. This is converted to a colour table here *
 *                                                    *
 * colours are taken from SRAM and are programmable   *
 ******************************************************/

PALETTE_INIT( cvs )
{
	int attr,col,map;

	#define COLOR(gfxn,offs) (colortable[machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

    /* Colour Mapping Prom */

    for(attr = 0;attr < 256; attr++)
    {
    	for(col = 0; col < 8; col++)
        {
          	map = color_prom[(col * 256) + attr];

            /* bits 0 and 2 are swapped */

            COLOR(0,attr*8 + col) = ((map & 1) << 2) + (map & 2) + ((map & 4) >> 2);
        }
    }

    /* Background Collision Map */

    for(map=0;map<8;map++)
    {
    	COLOR(0,2048+map) = (map & 4) >> 2;
        COLOR(0,2056+map) = (map & 2) >> 1;
        COLOR(0,2064+map) = ((map & 2) >> 1) || ((map & 4) >> 2);
    }

    /* Sprites */

    for(map=0;map<8;map++)
    {
    	COLOR(0,map*2 + 2072) = 0;
    	COLOR(0,map*2 + 2073) = 8 + map;
    }

    /* set the sprite chip offsets */
	s2636_x_offset = -26;
	s2636_y_offset = 3;
}


WRITE8_HANDLER( cvs_video_fx_w )
{
	if (data & 0xce)
		logerror("%4x : Unimplemented CVS video fx = %2x\n",activecpu_get_pc(),data);

    stars_on = data & 0x01;

    if (data & 0x02)   logerror("       SHADE BRIGHTER TO RIGHT\n");
    if (data & 0x04)   logerror("       SCREEN ROTATE\n");
    if (data & 0x08)   logerror("       SHADE BRIGHTER TO LEFT\n");

    set_led_status(1, data & 0x10);	/* lamp 1 */
    set_led_status(2, data & 0x20);	/* lamp 2 */

    if (data & 0x40)   logerror("       SHADE BRIGHTER TO BOTTOM\n");
    if (data & 0x80)   logerror("       SHADE BRIGHTER TO TOP\n");
}



READ8_HANDLER( cvs_input_r )
{
	UINT8 ret;

	/* the upper 4 bits of the address is used to select the tile banking attributes */
	character_mode = (offset >> 4) & 0x03;
	character_ram_page = (offset << 2) & 0x300;

	/* the lower 4 (or 3?) bits select the port to read */
	switch (offset & 0x0f)	/* might be 0x07 */
	{
	case 0x00:  ret = input_port_0_r(0); break;
	case 0x02:  ret = input_port_1_r(0); break;
	case 0x03:  ret = input_port_2_r(0); break;
	case 0x04:  ret = input_port_3_r(0); break;
	case 0x06:  ret = input_port_4_r(0); break;
	case 0x07:  ret = input_port_5_r(0); break;
	default:    ret = 0x00; break;
	}

	return ret;
}


READ8_HANDLER( cvs_collision_r )
{
	return cvs_collision_register;
}

READ8_HANDLER( cvs_collision_clear )
{
	cvs_collision_register = 0;

	return 0;
}


WRITE8_HANDLER( cvs_scroll_w )
{
	scroll_reg = 255 - data;
}


READ8_HANDLER( cvs_video_or_color_ram_r )
{
	if (!activecpu_get_reg(S2650_FO))
		return cvs_color_ram[offset];
	else
		return cvs_video_ram[offset];
}

WRITE8_HANDLER( cvs_video_or_color_ram_w )
{
	if (!activecpu_get_reg(S2650_FO))
		cvs_color_ram[offset] = data;
	else
		cvs_video_ram[offset] = data;
}


READ8_HANDLER( cvs_bullet_ram_or_palette_r )
{
	if (!activecpu_get_reg(S2650_FO))
		return cvs_bullet_ram[offset];
	else
		return palette_ram[offset & 0x0f];
}

WRITE8_HANDLER( cvs_bullet_ram_or_palette_w )
{
	if (!activecpu_get_reg(S2650_FO))
		cvs_bullet_ram[offset] = data;
	else
		palette_ram[offset & 0x0f] = data;
}


READ8_HANDLER( cvs_s2636_1_or_character_ram_r )
{
	if (!activecpu_get_reg(S2650_FO))
		return s2636_1_ram[offset];
	else
		return character_ram[(0 * 0x800) | 0x400 | character_ram_page | offset];
}

WRITE8_HANDLER( cvs_s2636_1_or_character_ram_w )
{
	if (!activecpu_get_reg(S2650_FO))
		s2636_1_ram[offset] = data;
	else
		character_ram[(0 * 0x800) | 0x400 | character_ram_page | offset] = data;
}


READ8_HANDLER( cvs_s2636_2_or_character_ram_r )
{
	if (!activecpu_get_reg(S2650_FO))
		return s2636_2_ram[offset];
	else
		return character_ram[(1 * 0x800) | 0x400 | character_ram_page | offset];
}

WRITE8_HANDLER( cvs_s2636_2_or_character_ram_w )
{
	if (!activecpu_get_reg(S2650_FO))
		s2636_2_ram[offset] = data;
	else
		character_ram[(1 * 0x800) | 0x400 | character_ram_page | offset] = data;
}


READ8_HANDLER( cvs_s2636_3_or_character_ram_r )
{
	if (!activecpu_get_reg(S2650_FO))
		return s2636_3_ram[offset];
	else
		return character_ram[(2 * 0x800) | 0x400 | character_ram_page | offset];
}

WRITE8_HANDLER( cvs_s2636_3_or_character_ram_w )
{
	if (!activecpu_get_reg(S2650_FO))
		s2636_3_ram[offset] = data;
	else
		character_ram[(2 * 0x800) | 0x400 | character_ram_page | offset] = data;
}


VIDEO_START( cvs )
{
	int generator = 0;
	int y;


	/* allocate memory */
	cvs_color_ram = auto_malloc(0x400);
	palette_ram = auto_malloc(0x10);
	character_ram = auto_malloc(3 * 0x800);  /* only half is used, but
                                                by allocating twice the amount,
                                                we can use the same gfx_layout */

	/* precalculate the star background */

	total_stars = 0;

	for (y = 255;y >= 0;y--)
	{
		int x;

		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;

			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (((~generator >> 16) & 1) && (generator & 0xfe) == 0xfe)
			{
            	if(((~(generator >> 12)) & 0x01) && ((~(generator >> 13)) & 0x01))
                {
				    if (total_stars < MAX_STARS)
				    {
					    stars[total_stars].x = x;
					    stars[total_stars].y = y;
					    stars[total_stars].code = 1;

					    total_stars++;
				    }
                }
			}
		}
	}


	/* create the bitmaps for the S2636 chips */
	s2636_1_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);
	s2636_2_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);
	s2636_3_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);


	/* create helper bitmaps */
	background_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, machine->screen[0].format);
	cvs_collision_bitmap = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);
	cvs_collision_background = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);
	scrolled_background = auto_bitmap_alloc(machine->screen[0].width, machine->screen[0].height, BITMAP_FORMAT_INDEXED8);
}



INTERRUPT_GEN( cvs_interrupt )
{
	stars_scroll++;

	cpunum_set_input_line_vector(0,0,0x03);
	cpunum_set_input_line(0,0,PULSE_LINE);
}



VIDEO_UPDATE( cvs )
{
	const int ram_based_char_start_indecies[] = { 0xe0, 0xc0, 0x100, 0x80 };
	int code;
	offs_t offs;
	int scroll[8];


	/* set the palette */
	for (offs = 0; offs < 0x10; offs++)
	{
		UINT8 data = palette_ram[offs];

		palette_set_color_rgb(machine, offs, pal2bit(~data >> 0), pal3bit(~data >> 2), pal3bit(~data >> 5));
	}


	/* create our background character set, which is a software
       selectable mixture of RAM and ROM based tiles */

	/* ROM based tiles first */
	for (code = 0; code < ram_based_char_start_indecies[character_mode]; code++)
		decodechar(machine->gfx[0], code, memory_region(REGION_GFX1),
		           machine->drv->gfxdecodeinfo[0].gfxlayout);

	/* now the RAM based ones */
	for (; code < 0x100; code++)
		decodechar(machine->gfx[0], code, character_ram,
		           machine->drv->gfxdecodeinfo[0].gfxlayout);


	/* draw the background */
	for (offs = 0; offs < 0x0400; offs++)
	{
		int collision_color = 0;

		UINT8 code = cvs_video_ram[offs];
		UINT8 color = cvs_color_ram[offs];

		UINT8 x = offs << 3;
		UINT8 y = offs >> 5 << 3;

		drawgfx(background_bitmap, machine->gfx[0],
				code, color,
				0, 0,
				x, y,
				0, TRANSPARENCY_NONE, 0);

		/* foreground for collision detection */
		if (color & 0x80)
			collision_color = 258;
		else
		{
			if ((color & 0x03) == 3)
				collision_color = 256;
			else if((color & 0x01) == 0)
				collision_color = 257;
		}

		if (collision_color)
			drawgfx(cvs_collision_background, machine->gfx[0],
					code, collision_color,
					0, 0,
					x, y,
					0, TRANSPARENCY_NONE, 0);
	}


    /* Update screen - 8 regions, fixed scrolling area */

    scroll[0] = 0;
    scroll[1] = scroll_reg;
    scroll[2] = scroll_reg;
    scroll[3] = scroll_reg;
    scroll[4] = scroll_reg;
    scroll[5] = scroll_reg;
    scroll[6] = 0;
    scroll[7] = 0;

	copyscrollbitmap(bitmap,background_bitmap,0,0,8,scroll,&machine->screen[0].visarea,TRANSPARENCY_NONE,0);
	copyscrollbitmap(scrolled_background,cvs_collision_background,0,0,8,scroll,&machine->screen[0].visarea,TRANSPARENCY_NONE,0);

    /* 2636's */

	fillbitmap(s2636_1_bitmap,0,0);
	s2636_update_bitmap(machine,s2636_1_bitmap,s2636_1_ram,1,cvs_collision_bitmap);

	fillbitmap(s2636_2_bitmap,0,0);
	s2636_update_bitmap(machine,s2636_2_bitmap,s2636_2_ram,2,cvs_collision_bitmap);

	fillbitmap(s2636_3_bitmap,0,0);
	s2636_update_bitmap(machine,s2636_3_bitmap,s2636_3_ram,3,cvs_collision_bitmap);

    /* Bullet Hardware */

    for (offs = 8; offs < 256; offs++ )
    {
        if (cvs_bullet_ram[offs] != 0)
        {
        	int ct;
            for(ct=0;ct<4;ct++)
            {
            	int bx=255-7-cvs_bullet_ram[offs]-ct;

            	/* Bullet/Object Collision */

                if((cvs_collision_register & 8) == 0)
                {
                    if ((*BITMAP_ADDR8(s2636_1_bitmap, offs, bx) != 0) ||
					    (*BITMAP_ADDR8(s2636_2_bitmap, offs, bx) != 0) ||
					    (*BITMAP_ADDR8(s2636_3_bitmap, offs, bx) != 0))
                        cvs_collision_register |= 8;
                }

            	/* Bullet/Background Collision */

                if((cvs_collision_register & 0x80) == 0)
                {
					if (*BITMAP_ADDR8(scrolled_background, offs, bx) != machine->pens[0])
                    	cvs_collision_register |= 0x80;
                }

				*BITMAP_ADDR16(bitmap, offs, bx) = machine->pens[7];
            }
        }
    }

    /* Update 2636 images */

    {
		int x;
        UINT32 S1,S2,S3,SB,pen;

        for(x=255;x>7;x--)
        {
        	UINT32 *sp1 = (UINT32 *)BITMAP_ADDR8(s2636_1_bitmap, x, 0);
	    	UINT32 *sp2 = (UINT32 *)BITMAP_ADDR8(s2636_2_bitmap, x, 0);
		    UINT32 *sp3 = (UINT32 *)BITMAP_ADDR8(s2636_3_bitmap, x, 0);
	        UINT64 *dst = (UINT64 *)BITMAP_ADDR16(bitmap, x, 0);
		    UINT8  *spb = (UINT8  *)BITMAP_ADDR8(scrolled_background, x, 0);

            for(offs=0;offs<62;offs++)
            {
        	     S1 = (*sp1++);
                 S2 = (*sp2++);
                 S3 = (*sp3++);

        	     pen = S1 | S2 | S3;

                 if(pen)
                 {
             	    UINT16 *address = (UINT16 *)dst;
				    if (pen & 0xff000000) address[BL3] = machine->pens[(pen >> 24) & 15];
				    if (pen & 0x00ff0000) address[BL2] = machine->pens[(pen >> 16) & 15];
				    if (pen & 0x0000ff00) address[BL1] = machine->pens[(pen >>  8) & 15];
				    if (pen & 0x000000ff) address[BL0] = machine->pens[(pen & 15)];

                    /* Collision Detection */

                    SB = 0;
				    if (spb[BL3] != machine->pens[0]) SB =  0x08000000;
				    if (spb[BL2] != machine->pens[0]) SB |= 0x00080000;
				    if (spb[BL1] != machine->pens[0]) SB |= 0x00000800;
				    if (spb[BL0] != machine->pens[0]) SB |= 0x00000008;

       	            if (S1 & S2) cvs_collision_register |= 1;
       	            if (S2 & S3) cvs_collision_register |= 2;
    			    if (S1 & S3) cvs_collision_register |= 4;

                    if (SB)
                    {
    			        if (S1 & SB) cvs_collision_register |= 16;
   			            if (S2 & SB) cvs_collision_register |= 32;
       	                if (S3 & SB) cvs_collision_register |= 64;
                    }
                 }

           	     dst++;
                 spb+=4;
            }
        }
    }


    /* stars circuit */

	if (stars_on)
	{
		for (offs = 0;offs < total_stars;offs++)
		{
			UINT8 x = (stars[offs].x + stars_scroll) >> 1;
			UINT8 y = stars[offs].y + ((stars_scroll + stars[offs].x) >> 9);

			if (y >= machine->screen[0].visarea.min_y &&
				y <= machine->screen[0].visarea.max_y)
			{
				if ((y & 1) ^ ((x >> 4) & 1))
				{
					if (flip_screen_x)
						x = ~x;

					if (flip_screen_y)
						y = ~y;

					if (*BITMAP_ADDR16(bitmap, y, x) == machine->pens[0])
					{
						*BITMAP_ADDR16(bitmap, y, x) = machine->pens[7];
					}
				}
			}
		}
	}


	return 0;
}
