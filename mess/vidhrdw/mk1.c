/******************************************************************************
 PeT mess@utanet.at 2000,2001
******************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "vidhrdw/generic.h"

#include "includes/mk1.h"

static struct artwork_info *backdrop;

unsigned char mk1_palette[242][3] =
{
	{ 0x20,0x02,0x05 },
	{ 0xc0, 0, 0 },
};

unsigned short mk1_colortable[1][2] = {
	{ 0, 1 },
};

void mk1_init_colors (unsigned char *sys_palette,
						  unsigned short *sys_colortable,
						  const unsigned char *color_prom)
{
	char backdrop_name[200];
	int used=2;

	memcpy (sys_palette, mk1_palette, sizeof (mk1_palette));
	memcpy(sys_colortable,mk1_colortable,sizeof(mk1_colortable));

    /* try to load a backdrop for the machine */
    sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);

    artwork_load (&backdrop, backdrop_name, used, Machine->drv->total_colors - used);

	if (backdrop)
    {
        logerror("backdrop %s successfully loaded\n", backdrop_name);
        memcpy (&sys_palette[used * 3], backdrop->orig_palette, 
				backdrop->num_pens_used * 3 * sizeof (unsigned char));
    }
    else
    {
        logerror("no backdrop loaded\n");
    }
}


int mk1_vh_start(void)
{
#if 1
	// artwork seams to need this
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*)malloc (videoram_size);
	if (!videoram)
        return 1;

    if (backdrop)
        backdrop_refresh (backdrop);

	return generic_vh_start();
#else
	return 0;
#endif
}

void mk1_vh_stop(void)
{
    if (backdrop)
        artwork_free (&backdrop);
#if 1
	generic_vh_stop();
#endif
}

UINT8 mk1_led[4]= {0};

static char led[]={
	" ii          aaaaaaaaaaaa\r"
	"iiii    fff aaaaaaaaaaaaa bbb\r"
	" ii     fff aaaaaaaaaaaaa bbb\r"
	"        fff               bbb\r"
	"        fff      jjj      bbb\r"
	"       fff       jjj     bbb\r"
	"       fff       jjj     bbb\r"
	"       fff      jjj      bbb\r"
	"       fff      jjj      bbb\r"
	"      fff       jjj     bbb\r"
	"      fff       jjj     bbb\r"
	"      fff      jjj      bbb\r"
	"      fff      jjj      bbb\r"
	"     fff       jjj     bbb\r"
	"     fff       jjj     bbb\r"
	"     fff               bbb\r"
    "     fff ggggggggggggg bbb\r"
    "        gggggggggggggg\r"
    "    eee ggggggggggggg ccc\r"
	"    eee               ccc\r"
	"    eee      kkk      ccc\r"
	"    eee      kkk      ccc\r"
	"   eee       kkk     ccc\r"
	"   eee       kkk     ccc\r"
	"   eee      kkk      ccc\r"
	"   eee      kkk      ccc\r"
	"  eee       kkk     ccc\r"
	"  eee       kkk     ccc\r"
	"  eee      kkk      ccc\r"
	"  eee      kkk      ccc\r"
	" eee       kkk     ccc\r"
	" eee               ccc\r"
    " eee ddddddddddddd ccc   hh\r"
    " eee ddddddddddddd ccc  hhhh\r"
    "     dddddddddddd        hh"
};

static void mk1_draw_9segment(struct osd_bitmap *bitmap,int value, int x, int y)
{
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++) {
		mask=0;
		switch (led[i]) {
		case 'a': mask=0x80; break;
		case 'b': mask=0x40; break;
		case 'c': mask=0x20; break;
		case 'd': mask=0x10; break;
		case 'e': mask=0x08; break;
		case 'f': mask=0x02; break;
		case 'g': mask=0x04; break;
		case 'h': 
			mask=0x01; 
			break;
		case 'i': 
			mask=0x100; 
			break;
		case 'j': 
			mask=0x200; 
			break;
		case 'k': 
			mask=0x400; 
			break;
		}
		
		if (mask!=0) {
			color=Machine->pens[(value&mask)?1:0];
			plot_pixel(bitmap, x+xi, y+yi, color);
			osd_mark_dirty(x+xi,y,x+yi,y);
		}
		if (led[i]!='\r') xi++;
		else { yi++, xi=0; }
	}
}

static struct {
	int x,y;
} mk1_led_pos[4]={
	{102,79},
	{140,79},
	{178,79},
	{216,79}
};

void mk1_vh_screenrefresh (struct osd_bitmap *bitmap, int full_refresh)
{
	int i;

    if (backdrop)
        copybitmap (bitmap, backdrop->artwork, 0, 0, 0, 0, NULL, 
		    TRANSPARENCY_NONE, 0);
    else
	fillbitmap (bitmap, Machine->pens[0], &Machine->visible_area);
    
    for (i=0; i<4; i++) {
	mk1_draw_9segment(bitmap, mk1_led[i], mk1_led_pos[i].x, mk1_led_pos[i].y);
	mk1_led[i]=0;
    }
}
