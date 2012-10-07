/*************************************************************************

    Venture Line Super Rider driver

**************************************************************************/

class suprridr_state : public driver_device
{
public:
	suprridr_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_fgram(*this, "fgram"),
		m_bgram(*this, "bgram"),
		m_spriteram(*this, "spriteram"){ }

	UINT8 m_nmi_enable;
	UINT8 m_sound_data;
	required_shared_ptr<UINT8> m_fgram;
	required_shared_ptr<UINT8> m_bgram;
	tilemap_t *m_fg_tilemap;
	tilemap_t *m_bg_tilemap;
	tilemap_t *m_bg_tilemap_noscroll;
	UINT8 m_flipx;
	UINT8 m_flipy;
	required_shared_ptr<UINT8> m_spriteram;
	DECLARE_WRITE8_MEMBER(nmi_enable_w);
	DECLARE_WRITE8_MEMBER(sound_data_w);
	DECLARE_WRITE8_MEMBER(sound_irq_ack_w);
	DECLARE_WRITE8_MEMBER(coin_lock_w);
	DECLARE_WRITE8_MEMBER(suprridr_flipx_w);
	DECLARE_WRITE8_MEMBER(suprridr_flipy_w);
	DECLARE_WRITE8_MEMBER(suprridr_fgdisable_w);
	DECLARE_WRITE8_MEMBER(suprridr_fgscrolly_w);
	DECLARE_WRITE8_MEMBER(suprridr_bgscrolly_w);
	DECLARE_WRITE8_MEMBER(suprridr_bgram_w);
	DECLARE_WRITE8_MEMBER(suprridr_fgram_w);
	DECLARE_CUSTOM_INPUT_MEMBER(suprridr_control_r);
	DECLARE_READ8_MEMBER(sound_data_r);
	TILE_GET_INFO_MEMBER(get_tile_info);
	TILE_GET_INFO_MEMBER(get_tile_info2);
	virtual void video_start();
	virtual void palette_init();
	UINT32 screen_update_suprridr(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(main_nmi_gen);
	TIMER_CALLBACK_MEMBER(delayed_sound_w);
};

/*----------- defined in video/suprridr.c -----------*/
int suprridr_is_screen_flipped(running_machine &machine);
