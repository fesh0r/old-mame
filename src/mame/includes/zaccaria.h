class zaccaria_state : public driver_device
{
public:
	zaccaria_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_videoram(*this, "videoram"),
		m_attributesram(*this, "attributesram"),
		m_spriteram(*this, "spriteram"),
		m_spriteram2(*this, "spriteram2"){ }

	int m_dsw;
	int m_active_8910;
	int m_port0a;
	int m_acs;
	int m_last_port0b;
	int m_toggle;
	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_attributesram;
	tilemap_t *m_bg_tilemap;
	required_shared_ptr<UINT8> m_spriteram;
	required_shared_ptr<UINT8> m_spriteram2;
	UINT8 m_nmi_mask;
	DECLARE_READ8_MEMBER(zaccaria_dsw_r);
	DECLARE_WRITE8_MEMBER(sound_command_w);
	DECLARE_WRITE8_MEMBER(sound1_command_w);
	DECLARE_READ8_MEMBER(zaccaria_prot1_r);
	DECLARE_READ8_MEMBER(zaccaria_prot2_r);
	DECLARE_WRITE8_MEMBER(coin_w);
	DECLARE_WRITE8_MEMBER(nmi_mask_w);
	DECLARE_WRITE8_MEMBER(zaccaria_videoram_w);
	DECLARE_WRITE8_MEMBER(zaccaria_attributes_w);
	DECLARE_WRITE8_MEMBER(zaccaria_flip_screen_x_w);
	DECLARE_WRITE8_MEMBER(zaccaria_flip_screen_y_w);
	DECLARE_CUSTOM_INPUT_MEMBER(acs_r);
	DECLARE_WRITE8_MEMBER(zaccaria_dsw_sel_w);
	DECLARE_WRITE8_MEMBER(ay8910_port0a_w);
	DECLARE_WRITE_LINE_MEMBER(zaccaria_irq0a);
	DECLARE_WRITE_LINE_MEMBER(zaccaria_irq0b);
	DECLARE_READ8_MEMBER(zaccaria_port0a_r);
	DECLARE_WRITE8_MEMBER(zaccaria_port0a_w);
	DECLARE_WRITE8_MEMBER(zaccaria_port0b_w);
	DECLARE_WRITE8_MEMBER(zaccaria_port1b_w);
	DECLARE_WRITE8_MEMBER(mc1408_data_w);
	TILE_GET_INFO_MEMBER(get_tile_info);
	virtual void video_start();
	virtual void palette_init();
};


/*----------- defined in video/zaccaria.c -----------*/



SCREEN_UPDATE_IND16( zaccaria );
