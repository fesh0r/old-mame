#include "driver.h"
#include "video/konicdev.h"
#include "includes/simpsons.h"

static int bg_colorbase,sprite_colorbase,layer_colorbase[3];
UINT8 *simpsons_xtraram;
static int layerpri[3];


/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

void simpsons_tile_callback(running_machine *machine, int layer,int bank,int *code,int *color,int *flags,int *priority)
{
	*code |= ((*color & 0x3f) << 8) | (bank << 14);
	*color = layer_colorbase[layer] + ((*color & 0xc0) >> 6);
}


/***************************************************************************

  Callbacks for the K053247

***************************************************************************/

void simpsons_sprite_callback(running_machine *machine, int *code,int *color,int *priority_mask)
{
	int pri = (*color & 0x0f80) >> 6;	/* ??????? */
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;

	*color = sprite_colorbase + (*color & 0x001f);
}


/***************************************************************************

  Extra video banking

***************************************************************************/

static READ8_HANDLER( simpsons_K052109_r )
{
	const device_config *k052109 = devtag_get_device(space->machine, "k052109");
	return k052109_r(k052109, offset + 0x2000);
}

static WRITE8_HANDLER( simpsons_K052109_w )
{
	const device_config *k052109 = devtag_get_device(space->machine, "k052109");
	k052109_w(k052109, offset + 0x2000, data);
}

static READ8_HANDLER( simpsons_K053247_r )
{
	int offs;

	if (offset < 0x1000)
	{
		offs = offset >> 1;

		if (offset & 1)
			return(space->machine->generic.spriteram.u16[offs] & 0xff);
		else
			return(space->machine->generic.spriteram.u16[offs] >> 8);
	}
	else return simpsons_xtraram[offset - 0x1000];
}

static WRITE8_HANDLER( simpsons_K053247_w )
{
	int offs;

	if (offset < 0x1000)
	{
		UINT16 *spriteram16 = space->machine->generic.spriteram.u16;
		offs = offset >> 1;

		if (offset & 1)
			spriteram16[offs] = (spriteram16[offs] & 0xff00) | data;
		else
			spriteram16[offs] = (spriteram16[offs] & 0x00ff) | (data<<8);
	}
	else simpsons_xtraram[offset - 0x1000] = data;
}

void simpsons_video_banking(running_machine *machine, int bank)
{
	const device_config *k052109 = devtag_get_device(machine, "k052109");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	if (bank & 1)
	{
		memory_install_read_bank(space, 0x0000, 0x0fff, 0, 0, "bank5");
		memory_install_write8_handler(space, 0x0000, 0x0fff, 0, 0, paletteram_xBBBBBGGGGGRRRRR_be_w);
		memory_set_bankptr(machine, "bank5", machine->generic.paletteram.v);
	}
	else
		memory_install_readwrite8_device_handler(space, k052109, 0x0000, 0x0fff, 0, 0, k052109_r, k052109_w);

	if (bank & 2)
		memory_install_readwrite8_handler(space, 0x2000, 0x3fff, 0, 0, simpsons_K053247_r, simpsons_K053247_w);
	else
		memory_install_readwrite8_handler(space, 0x2000, 0x3fff, 0, 0, simpsons_K052109_r, simpsons_K052109_w);
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
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

VIDEO_UPDATE( simpsons )
{
	const device_config *k053246 = devtag_get_device(screen->machine, "k053246");
	const device_config *k053251 = devtag_get_device(screen->machine, "k053251");
	const device_config *k052109 = devtag_get_device(screen->machine, "k052109");
	int layer[3];

	bg_colorbase       = k053251_get_palette_index(k053251, K053251_CI0);
	sprite_colorbase   = k053251_get_palette_index(k053251, K053251_CI1);
	layer_colorbase[0] = k053251_get_palette_index(k053251, K053251_CI2);
	layer_colorbase[1] = k053251_get_palette_index(k053251, K053251_CI3);
	layer_colorbase[2] = k053251_get_palette_index(k053251, K053251_CI4);

	k052109_tilemap_update(k052109);

	layer[0] = 0;
	layerpri[0] = k053251_get_priority(k053251, K053251_CI2);
	layer[1] = 1;
	layerpri[1] = k053251_get_priority(k053251, K053251_CI3);
	layer[2] = 2;
	layerpri[2] = k053251_get_priority(k053251, K053251_CI4);

	sortlayers(layer,layerpri);

	bitmap_fill(screen->machine->priority_bitmap,cliprect,0);
	bitmap_fill(bitmap,cliprect,16 * bg_colorbase);
	k052109_tilemap_draw(k052109, bitmap, cliprect, layer[0], 0,1);
	k052109_tilemap_draw(k052109, bitmap, cliprect, layer[1], 0,2);
	k052109_tilemap_draw(k052109, bitmap, cliprect, layer[2], 0,4);

	k053247_sprites_draw(k053246, bitmap, cliprect);
	return 0;
}
