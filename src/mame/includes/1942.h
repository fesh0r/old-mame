/***************************************************************************

    1942

***************************************************************************/

class _1942_state : public driver_device
{
public:
	_1942_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_spriteram(*this, "spriteram"),
		m_fg_videoram(*this, "fg_videoram"),
		m_bg_videoram(*this, "bg_videoram"),
		m_audiocpu(*this, "audiocpu"),
		m_maincpu(*this, "maincpu") { }

	/* memory pointers */
	required_shared_ptr<UINT8> m_spriteram;
	required_shared_ptr<UINT8> m_fg_videoram;
	required_shared_ptr<UINT8> m_bg_videoram;

	/* video-related */
	tilemap_t *m_fg_tilemap;
	tilemap_t *m_bg_tilemap;
	int m_palette_bank;
	UINT8 m_scroll[2];

	/* devices */
	required_device<cpu_device> m_audiocpu;
	DECLARE_WRITE8_MEMBER(c1942_bankswitch_w);
	DECLARE_WRITE8_MEMBER(c1942_fgvideoram_w);
	DECLARE_WRITE8_MEMBER(c1942_bgvideoram_w);
	DECLARE_WRITE8_MEMBER(c1942_palette_bank_w);
	DECLARE_WRITE8_MEMBER(c1942_scroll_w);
	DECLARE_WRITE8_MEMBER(c1942_c804_w);
	DECLARE_DRIVER_INIT(1942);
	TILE_GET_INFO_MEMBER(get_fg_tile_info);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_1942(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(c1942_scanline);
	void draw_sprites( bitmap_ind16 &bitmap, const rectangle &cliprect );
	required_device<cpu_device> m_maincpu;
};
