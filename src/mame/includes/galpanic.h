#include "includes/kaneko16.h"

class galpanic_state : public kaneko16_state
{
public:
	galpanic_state(const machine_config &mconfig, device_type type, const char *tag)
		: kaneko16_state(mconfig, type, tag),
		  m_bgvideoram(*this, "bgvideoram"),
		  m_fgvideoram(*this, "fgvideoram"),
		  m_spriteram(*this, "spriteram") { }

	required_shared_ptr<UINT16> m_bgvideoram;
	required_shared_ptr<UINT16> m_fgvideoram;
	bitmap_ind16 m_bitmap;
	bitmap_ind16 m_sprites_bitmap;
	optional_shared_ptr<UINT16> m_spriteram;
	DECLARE_WRITE16_MEMBER(galpanic_6295_bankswitch_w);
	DECLARE_WRITE16_MEMBER(galpanica_6295_bankswitch_w);
	DECLARE_WRITE16_MEMBER(galpanica_misc_w);
	DECLARE_WRITE16_MEMBER(galpanic_coin_w);
	DECLARE_WRITE16_MEMBER(galpanic_bgvideoram_mirror_w);
	DECLARE_READ16_MEMBER(comad_timer_r);
	DECLARE_READ16_MEMBER(zipzap_random_read);
	DECLARE_READ8_MEMBER(comad_okim6295_r);
	DECLARE_VIDEO_START(galpanic);
	DECLARE_PALETTE_INIT(galpanic);
};


/*----------- defined in video/galpanic.c -----------*/


WRITE16_HANDLER( galpanic_bgvideoram_w );
WRITE16_HANDLER( galpanic_paletteram_w );

SCREEN_UPDATE_IND16( galpanic );
SCREEN_UPDATE_IND16( comad );


