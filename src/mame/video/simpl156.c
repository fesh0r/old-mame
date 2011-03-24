/* Simple 156 based board

*/

#include "emu.h"
#include "includes/simpl156.h"
#include "video/deco16ic.h"
#include "video/decospr.h"


VIDEO_START( simpl156 )
{
	simpl156_state *state = machine->driver_data<simpl156_state>();

	/* allocate the ram as 16-bit (we do it here because the CPU is 32-bit) */
	state->pf1_rowscroll = auto_alloc_array(machine, UINT16, 0x800/2);
	state->pf2_rowscroll = auto_alloc_array(machine, UINT16, 0x800/2);
	state->spriteram = auto_alloc_array(machine, UINT16, 0x2000/2);
	machine->generic.paletteram.u16 =  auto_alloc_array(machine, UINT16, 0x1000/2);

	/* and register the allocated ram so that save states still work */
	state->save_pointer(NAME(state->pf1_rowscroll), 0x800/2);
	state->save_pointer(NAME(state->pf2_rowscroll), 0x800/2);
	state->save_pointer(NAME(state->spriteram), 0x2000/2);
	state_save_register_global_pointer(machine, machine->generic.paletteram.u16, 0x1000/2);
}

SCREEN_UPDATE( simpl156 )
{
	simpl156_state *state = screen->machine->driver_data<simpl156_state>();

	bitmap_fill(screen->machine->priority_bitmap, NULL, 0);

	deco16ic_pf12_update(state->deco16ic, state->pf1_rowscroll, state->pf2_rowscroll);

	bitmap_fill(bitmap, cliprect, 256);

	deco16ic_tilemap_2_draw(state->deco16ic, bitmap, cliprect, 0, 2);
	deco16ic_tilemap_1_draw(state->deco16ic, bitmap, cliprect, 0, 4);

	//FIXME: flip_screen_x should not be written!
	flip_screen_set_no_update(screen->machine, 1);

	screen->machine->device<decospr_device>("spritegen")->draw_sprites(screen->machine, bitmap, cliprect, state->spriteram, 0x800); // 0x800 needed to charlien title
	return 0;
}
