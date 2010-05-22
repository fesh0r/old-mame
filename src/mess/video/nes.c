/***************************************************************************

  video/nes.c

  Routines to control the unique NES video hardware/PPU.

***************************************************************************/

#include "emu.h"
#include "video/ppu2c0x.h"
#include "includes/nes.h"
#include "machine/nes_mmc.h"

static void nes_vh_reset( running_machine *machine )
{
	nes_state *state = (nes_state *)machine->driver_data;
	ppu2c0x_set_vidaccess_callback(devtag_get_device(machine,"ppu"), nes_ppu_vidaccess);
	ppu2c0x_set_scanlines_per_frame(devtag_get_device(machine,"ppu"), ceil(state->scanlines_per_frame));

	if (state->four_screen_vram)
		set_nt_mirroring(machine, PPU_MIRROR_4SCREEN);
	else
	{
		switch(state->hard_mirroring)
		{
			case 0:
				set_nt_mirroring(machine, PPU_MIRROR_HORZ);
				break;
			case 1:
				set_nt_mirroring(machine, PPU_MIRROR_VERT);
				break;
			default:
				set_nt_mirroring(machine, PPU_MIRROR_NONE);
				break;
		}
	}
}

static void nes_vh_start(running_machine *machine, double scanlines_per_frame)
{
	nes_state *state = (nes_state *)machine->driver_data;

	state->last_frame_flip =  0;
	state->scanlines_per_frame = scanlines_per_frame;

	add_reset_callback(machine, nes_vh_reset);
}

VIDEO_START( nes_ntsc )
{
	nes_vh_start(machine, PPU_NTSC_SCANLINES_PER_FRAME);
}

VIDEO_START( nes_pal )
{
	nes_vh_start(machine, PPU_PAL_SCANLINES_PER_FRAME);
}

PALETTE_INIT( nes )
{
	ppu2c0x_init_palette(machine, 0);
}


/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( nes )
{
	nes_state *state = (nes_state *)screen->machine->driver_data;

	/* render the ppu */
	ppu2c0x_render(state->ppu, bitmap, 0, 0, 0, 0);

	/* if this is a disk system game, check for the flip-disk key */
	if (state->mapper == 20)
	{
		// latch this input so it doesn't go at warp speed
		if ((input_port_read(screen->machine, "FLIPDISK") & 0x01) && (!state->last_frame_flip))
		{
			state->last_frame_flip = 1;
			state->fds_current_side++;
			if (state->fds_current_side > state->fds_sides)
				state->fds_current_side = 0;

			if (state->fds_current_side == 0)
				popmessage("No disk inserted.");
			else
				popmessage("Disk set to side %d", state->fds_current_side);
		}

		if (!(input_port_read(screen->machine, "FLIPDISK") & 0x01))
			state->last_frame_flip = 0;
	}
	return 0;
}
