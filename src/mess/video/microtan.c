/***************************************************************************
 *  Microtan 65
 *
 *  video hardware
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 ***************************************************************************/

#include "driver.h"
#include "includes/microtan.h"


UINT8 microtan_chunky_graphics = 0;
UINT8 *microtan_chunky_buffer = NULL;

static tilemap_t *bg_tilemap;

WRITE8_HANDLER( microtan_videoram_w )
{
	if ((space->machine->generic.videoram.u8[offset] != data) || (microtan_chunky_buffer[offset] != microtan_chunky_graphics))
	{
		space->machine->generic.videoram.u8[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
		microtan_chunky_buffer[offset] = microtan_chunky_graphics;
	}
}

static TILE_GET_INFO(get_bg_tile_info)
{
	int gfxn = microtan_chunky_buffer[tile_index];
	int code = machine->generic.videoram.u8[tile_index];

	SET_TILE_INFO(gfxn, code, 0, 0);
}

VIDEO_START( microtan )
{
	bg_tilemap = tilemap_create(machine, get_bg_tile_info, tilemap_scan_rows,
		8, 16, 32, 16);

	microtan_chunky_buffer = auto_alloc_array(machine, UINT8, machine->generic.videoram_size);
	memset(microtan_chunky_buffer, 0, machine->generic.videoram_size);
	microtan_chunky_graphics = 0;
}

VIDEO_UPDATE( microtan )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}
