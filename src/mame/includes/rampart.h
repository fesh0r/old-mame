/*************************************************************************

    Atari Rampart hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "sound/okim6295.h"

class rampart_state : public atarigen_state
{
public:
	rampart_state(const machine_config &mconfig, device_type type, const char *tag)
		: atarigen_state(mconfig, type, tag),
			m_bitmap(*this, "bitmap"),
		m_oki(*this, "oki") { }

	required_shared_ptr<UINT16> m_bitmap;
	UINT8           m_has_mo;
	virtual void update_interrupts();
	virtual void scanline_update(screen_device &screen, int scanline);
	DECLARE_WRITE16_MEMBER(latch_w);
	DECLARE_DRIVER_INIT(rampart);
	DECLARE_MACHINE_START(rampart);
	DECLARE_MACHINE_RESET(rampart);
	DECLARE_VIDEO_START(rampart);
	UINT32 screen_update_rampart(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void rampart_bitmap_render(bitmap_ind16 &bitmap, const rectangle &cliprect);
	required_device<okim6295_device> m_oki;
};
