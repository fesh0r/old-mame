class tagteam_state : public driver_device
{
public:
	tagteam_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_videoram(*this, "videoram"),
		m_colorram(*this, "colorram"),
		m_maincpu(*this, "maincpu"),
		m_audiocpu(*this, "audiocpu") { }

	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_colorram;
	int m_palettebank;
	tilemap_t *m_bg_tilemap;

	UINT8 m_sound_nmi_mask;
	DECLARE_WRITE8_MEMBER(sound_command_w);
	DECLARE_WRITE8_MEMBER(irq_clear_w);
	DECLARE_WRITE8_MEMBER(sound_nmi_mask_w);
	DECLARE_WRITE8_MEMBER(tagteam_videoram_w);
	DECLARE_WRITE8_MEMBER(tagteam_colorram_w);
	DECLARE_READ8_MEMBER(tagteam_mirrorvideoram_r);
	DECLARE_READ8_MEMBER(tagteam_mirrorcolorram_r);
	DECLARE_WRITE8_MEMBER(tagteam_mirrorvideoram_w);
	DECLARE_WRITE8_MEMBER(tagteam_mirrorcolorram_w);
	DECLARE_WRITE8_MEMBER(tagteam_control_w);
	DECLARE_WRITE8_MEMBER(tagteam_flipscreen_w);
	DECLARE_INPUT_CHANGED_MEMBER(coin_inserted);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_tagteam(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(sound_timer_irq);
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	required_device<cpu_device> m_maincpu;
	required_device<cpu_device> m_audiocpu;
};
