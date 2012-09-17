/*************************************************************************

    Atari Xybots hardware

*************************************************************************/

#include "machine/atarigen.h"

class xybots_state : public atarigen_state
{
public:
	xybots_state(const machine_config &mconfig, device_type type, const char *tag)
		: atarigen_state(mconfig, type, tag) { }

	UINT16			m_h256;
	DECLARE_READ16_MEMBER(special_port1_r);
	DECLARE_DRIVER_INIT(xybots);
	TILE_GET_INFO_MEMBER(get_alpha_tile_info);
	TILE_GET_INFO_MEMBER(get_playfield_tile_info);
	DECLARE_MACHINE_START(xybots);
	DECLARE_MACHINE_RESET(xybots);
	DECLARE_VIDEO_START(xybots);
};


/*----------- defined in video/xybots.c -----------*/


SCREEN_UPDATE_IND16( xybots );
