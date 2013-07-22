/***************************************************************************

  video.c

***************************************************************************/

#include "emu.h"
#include "includes/qdrmfgp.h"



void qdrmfgp_tile_callback(running_machine &machine, int layer, int *code, int *color, int *flags)
{
	qdrmfgp_state *state = machine.driver_data<qdrmfgp_state>();
	*color = ((*color>>2) & 0x0f) | state->m_pal;
}

void qdrmfgp2_tile_callback(running_machine &machine, int layer, int *code, int *color, int *flags)
{
	*color = (*color>>1) & 0x7f;
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START_MEMBER(qdrmfgp_state,qdrmfgp)
{
	m_k056832->set_layer_association(0);

	m_k056832->set_layer_offs(0, 2, 0);
	m_k056832->set_layer_offs(1, 4, 0);
	m_k056832->set_layer_offs(2, 6, 0);
	m_k056832->set_layer_offs(3, 8, 0);
}

VIDEO_START_MEMBER(qdrmfgp_state,qdrmfgp2)
{
	m_k056832->set_layer_association(0);

	m_k056832->set_layer_offs(0, 3, 1);
	m_k056832->set_layer_offs(1, 5, 1);
	m_k056832->set_layer_offs(2, 7, 1);
	m_k056832->set_layer_offs(3, 9, 1);
}

/***************************************************************************

  Display refresh

***************************************************************************/

UINT32 qdrmfgp_state::screen_update_qdrmfgp(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	bitmap.fill(get_black_pen(machine()), cliprect);

	m_k056832->tilemap_draw(bitmap, cliprect, 3, 0, 1);
	m_k056832->tilemap_draw(bitmap, cliprect, 2, 0, 2);
	m_k056832->tilemap_draw(bitmap, cliprect, 1, 0, 4);
	m_k056832->tilemap_draw(bitmap, cliprect, 0, 0, 8);
	return 0;
}
