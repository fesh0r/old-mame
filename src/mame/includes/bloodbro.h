class bloodbro_state : public driver_device
{
public:
	bloodbro_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_spriteram(*this, "spriteram"),
		m_bgvideoram(*this, "bgvideoram"),
		m_fgvideoram(*this, "fgvideoram"),
		m_txvideoram(*this, "txvideoram"),
		m_scroll(*this, "scroll"){ }

	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_bgvideoram;
	required_shared_ptr<UINT16> m_fgvideoram;
	required_shared_ptr<UINT16> m_txvideoram;
	optional_shared_ptr<UINT16> m_scroll;

	tilemap_t *m_bg_tilemap;
	tilemap_t *m_fg_tilemap;
	tilemap_t *m_tx_tilemap;

	DECLARE_WRITE16_MEMBER(bloodbro_bgvideoram_w);
	DECLARE_WRITE16_MEMBER(bloodbro_fgvideoram_w);
	DECLARE_WRITE16_MEMBER(bloodbro_txvideoram_w);
	TILE_GET_INFO_MEMBER(get_bg_tile_info);
	TILE_GET_INFO_MEMBER(get_fg_tile_info);
	TILE_GET_INFO_MEMBER(get_tx_tile_info);
	virtual void video_start();
	UINT32 screen_update_bloodbro(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_weststry(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_skysmash(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void bloodbro_draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	void weststry_draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
};
