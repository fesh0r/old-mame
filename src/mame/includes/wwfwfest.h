#include "video/bufsprite.h"
#include "sound/okim6295.h"

class wwfwfest_state : public driver_device
{
public:
	wwfwfest_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_fg0_videoram(*this, "fg0_videoram"),
		m_bg0_videoram(*this, "bg0_videoram"),
		m_bg1_videoram(*this, "bg1_videoram"),
		m_spriteram(*this, "spriteram") ,
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu"),
		m_oki(*this, "oki") { }

	required_shared_ptr<UINT16> m_fg0_videoram;
	required_shared_ptr<UINT16> m_bg0_videoram;
	required_shared_ptr<UINT16> m_bg1_videoram;
	UINT16 m_pri;
	UINT16 m_bg0_scrollx;
	UINT16 m_bg0_scrolly;
	UINT16 m_bg1_scrollx;
	UINT16 m_bg1_scrolly;
	tilemap_t *m_fg0_tilemap;
	tilemap_t *m_bg0_tilemap;
	tilemap_t *m_bg1_tilemap;
	UINT16 m_sprite_xoff;
	UINT16 m_bg0_dx;
	UINT16 m_bg1_dx[2];
	required_device<buffered_spriteram16_device> m_spriteram;
	DECLARE_WRITE16_MEMBER(wwfwfest_1410_write);
	DECLARE_WRITE16_MEMBER(wwfwfest_scroll_write);
	DECLARE_WRITE16_MEMBER(wwfwfest_irq_ack_w);
	DECLARE_WRITE16_MEMBER(wwfwfest_flipscreen_w);
	DECLARE_READ16_MEMBER(wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_r);
	DECLARE_WRITE16_MEMBER(wwfwfest_paletteram16_xxxxBBBBGGGGRRRR_word_w);
	DECLARE_WRITE16_MEMBER(wwfwfest_soundwrite);
	DECLARE_WRITE16_MEMBER(wwfwfest_fg0_videoram_w);
	DECLARE_WRITE16_MEMBER(wwfwfest_bg0_videoram_w);
	DECLARE_WRITE16_MEMBER(wwfwfest_bg1_videoram_w);
	DECLARE_CUSTOM_INPUT_MEMBER(dsw_3f_r);
	DECLARE_CUSTOM_INPUT_MEMBER(dsw_c0_r);
	DECLARE_WRITE8_MEMBER(oki_bankswitch_w);
	TILE_GET_INFO_MEMBER(get_fg0_tile_info);
	TILE_GET_INFO_MEMBER(get_bg0_tile_info);
	TILE_GET_INFO_MEMBER(get_bg1_tile_info);
	virtual void video_start();
	DECLARE_VIDEO_START(wwfwfstb);
	UINT32 screen_update_wwfwfest(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(wwfwfest_scanline);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect );
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
	required_device<okim6295_device> m_oki;
};
