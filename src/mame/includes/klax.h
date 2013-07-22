/*************************************************************************

    Atari Klax hardware

*************************************************************************/

#include "machine/atarigen.h"

class klax_state : public atarigen_state
{
public:
	klax_state(const machine_config &mconfig, device_type type, const char *tag)
		: atarigen_state(mconfig, type, tag),
			m_playfield_tilemap(*this, "playfield") { }

	required_device<tilemap_device> m_playfield_tilemap;

	virtual void update_interrupts();
	virtual void scanline_update(screen_device &screen, int scanline);
	DECLARE_WRITE16_MEMBER(interrupt_ack_w);
	TILE_GET_INFO_MEMBER(get_playfield_tile_info);
	DECLARE_MACHINE_START(klax);
	DECLARE_MACHINE_RESET(klax);
	DECLARE_VIDEO_START(klax);
	UINT32 screen_update_klax(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_WRITE16_MEMBER( klax_latch_w );
};
