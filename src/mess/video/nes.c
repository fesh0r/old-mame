/***************************************************************************

  video/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include <math.h>

#include "driver.h"
#include "deprecat.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"

bitmap_t *nes_zapper_hack;
int nes_vram_sprite[8]; /* Used only by mmc5 for now */
static int last_frame_flip = 0;

static void ppu_nmi(int num, int *ppu_regs)
{
	cpunum_set_input_line(Machine, 0, INPUT_LINE_NMI, PULSE_LINE);
}

static void nes_vh_reset(running_machine *machine)
{
	ppu2c0x_reset( machine, 0, 1 );
}



static void nes_vh_start(running_machine *machine, ppu_t ppu_type, double scanlines_per_frame)
{
	ppu2c0x_interface ppu_interface;

	nes_zapper_hack = NULL;

	last_frame_flip =  0;

	memset(&ppu_interface, 0, sizeof(ppu_interface));
	ppu_interface.type				= ppu_type;
	ppu_interface.num				= 1;
	ppu_interface.vrom_region[0]	= nes.chr_chunks ? "gfx1" : NULL;
	ppu_interface.mirroring[0]		= PPU_MIRROR_NONE;
	ppu_interface.nmi_handler[0]	= ppu_nmi;

	ppu2c0x_init(machine, &ppu_interface);
	ppu2c0x_set_vidaccess_callback(0, nes_ppu_vidaccess);
	ppu2c0x_set_scanlines_per_frame(0, ceil(scanlines_per_frame));

	if (nes.four_screen_vram)
	{
		ppu2c0x_set_mirroring(0, PPU_MIRROR_4SCREEN);
	}
	else
	{
		switch(nes.hard_mirroring)
		{
			case 0:
				ppu2c0x_set_mirroring(0, PPU_MIRROR_HORZ);
				break;
			case 1:
				ppu2c0x_set_mirroring(0, PPU_MIRROR_VERT);
				break;
		}
	}

	add_reset_callback(machine, nes_vh_reset);

	/* Reset the mapper variables. Will also mark the char-gen ram as dirty */
	mapper_reset(nes.mapper);
}



VIDEO_START( nes_ntsc )
{
	nes_vh_start(machine, PPU_2C02, PPU_NTSC_SCANLINES_PER_FRAME);
}

VIDEO_START( nes_pal )
{
	nes_vh_start(machine, PPU_2C07, PPU_PAL_SCANLINES_PER_FRAME);
}

PALETTE_INIT( nes )
{
	ppu2c0x_init_palette(machine, 0);
}

static void draw_sight(bitmap_t *bitmap, UINT16 color, int x_center, int y_center)
{
	int x,y;

	if (x_center<2)   x_center=2;
	if (x_center>253) x_center=253;

	if (y_center<2)   y_center=2;
	if (y_center>253) y_center=253;

	for(y = y_center-5; y < y_center+6; y++)
		if((y >= 0) && (y < 256))
			*BITMAP_ADDR16(bitmap, y, x_center) = color;

	for(x = x_center-5; x < x_center+6; x++)
		if((x >= 0) && (x < 256))
			*BITMAP_ADDR16(bitmap, y_center, x) = color;
}

/***************************************************************************

  Display refresh

***************************************************************************/
VIDEO_UPDATE( nes )
{
	int sights = 0;

	nes_zapper_hack = bitmap;

	/* render the ppu */
	ppu2c0x_render( 0, bitmap, 0, 0, 0, 0 );

	/* figure out what sights to draw, and draw them */
	if ((input_port_read(screen->machine, "CONTROLLERS") & 0x000f) == 0x0002)
		sights |= 0x0001;
	if ((input_port_read(screen->machine, "CONTROLLERS") & 0x000f) == 0x0003)
		sights |= 0x0002;
	if ((input_port_read(screen->machine, "CONTROLLERS") & 0x00f0) == 0x0020)
		sights |= 0x0001;
	if ((input_port_read(screen->machine, "CONTROLLERS") & 0x00f0) == 0x0030)
		sights |= 0x0002;
	if (sights & 0x0001)
		draw_sight(bitmap, 0x30, input_port_read(screen->machine, "ZAPPER1_X"), input_port_read(screen->machine, "ZAPPER1_Y"));
	if (sights & 0x0002)
		draw_sight(bitmap, 0x00, input_port_read(screen->machine, "ZAPPER2_X"), input_port_read(screen->machine, "ZAPPER2_Y"));

	/* if this is a disk system game, check for the flip-disk key */
	if (nes.mapper == 20)
	{
		// latch this input so it doesn't go at warp speed
		if ((input_port_read(screen->machine, "FLIPDISK") & 0x01) && (!last_frame_flip))
		{
			last_frame_flip = 1;
			nes_fds.current_side++;
			if (nes_fds.current_side > nes_fds.sides)
				nes_fds.current_side = 0;

			if (nes_fds.current_side == 0)
			{
				popmessage("No disk inserted.");
			}
			else
			{
				popmessage("Disk set to side %d", nes_fds.current_side);
			}
		}

		if (!(input_port_read(screen->machine, "FLIPDISK") & 0x01))
		{
			last_frame_flip = 0;
		}
	}
	return 0;
}
