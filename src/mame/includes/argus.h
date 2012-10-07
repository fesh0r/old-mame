class argus_state : public driver_device
{
public:
	argus_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_bg0_scrollx(*this, "bg0_scrollx"),
		m_bg0_scrolly(*this, "bg0_scrolly"),
		m_bg1_scrollx(*this, "bg1_scrollx"),
		m_bg1_scrolly(*this, "bg1_scrolly"),
		m_paletteram(*this, "paletteram"),
		m_txram(*this, "txram"),
		m_bg1ram(*this, "bg1ram"),
		m_spriteram(*this, "spriteram"),
		m_butasan_bg1ram(*this, "butasan_bg1ram"){ }

	optional_shared_ptr<UINT8> m_bg0_scrollx;
	optional_shared_ptr<UINT8> m_bg0_scrolly;
	required_shared_ptr<UINT8> m_bg1_scrollx;
	required_shared_ptr<UINT8> m_bg1_scrolly;
	required_shared_ptr<UINT8> m_paletteram;
	optional_shared_ptr<UINT8> m_txram;
	optional_shared_ptr<UINT8> m_bg1ram;
	required_shared_ptr<UINT8> m_spriteram;
	optional_shared_ptr<UINT8> m_butasan_bg1ram;

	UINT8 *m_dummy_bg0ram;
	UINT8 *m_butasan_txram;
	UINT8 *m_butasan_bg0ram;
	UINT8 *m_butasan_bg0backram;
	UINT8 *m_butasan_txbackram;
	UINT8 *m_butasan_pagedram[2];
	UINT8 m_butasan_page_latch;
	tilemap_t *m_tx_tilemap;
	tilemap_t *m_bg0_tilemap;
	tilemap_t *m_bg1_tilemap;
	UINT8 m_bg_status;
	UINT8 m_butasan_bg1_status;
	UINT8 m_flipscreen;
	UINT16 m_palette_intensity;
	int m_lowbitscroll;
	int m_prvscrollx;
	UINT8 m_valtric_mosaic;
	bitmap_rgb32 m_mosaicbitmap;
	UINT8 m_valtric_unknown;
	UINT8 m_butasan_unknown;
	int m_mosaic;
	DECLARE_WRITE8_MEMBER(argus_bankselect_w);
	DECLARE_WRITE8_MEMBER(valtric_mosaic_w);
	DECLARE_READ8_MEMBER(argus_txram_r);
	DECLARE_WRITE8_MEMBER(argus_txram_w);
	DECLARE_READ8_MEMBER(argus_bg1ram_r);
	DECLARE_WRITE8_MEMBER(argus_bg1ram_w);
	DECLARE_WRITE8_MEMBER(argus_bg_status_w);
	DECLARE_WRITE8_MEMBER(valtric_bg_status_w);
	DECLARE_WRITE8_MEMBER(butasan_bg0_status_w);
	DECLARE_WRITE8_MEMBER(butasan_bg1_status_w);
	DECLARE_WRITE8_MEMBER(argus_flipscreen_w);
	DECLARE_READ8_MEMBER(argus_paletteram_r);
	DECLARE_WRITE8_MEMBER(argus_paletteram_w);
	DECLARE_WRITE8_MEMBER(valtric_paletteram_w);
	DECLARE_WRITE8_MEMBER(butasan_paletteram_w);
	DECLARE_READ8_MEMBER(butasan_bg1ram_r);
	DECLARE_WRITE8_MEMBER(butasan_bg1ram_w);
	DECLARE_WRITE8_MEMBER(butasan_pageselect_w);
	DECLARE_READ8_MEMBER(butasan_pagedram_r);
	DECLARE_WRITE8_MEMBER(butasan_pagedram_w);
	DECLARE_WRITE8_MEMBER(valtric_unknown_w);
	DECLARE_WRITE8_MEMBER(butasan_unknown_w);
	TILE_GET_INFO_MEMBER(argus_get_tx_tile_info);
	TILE_GET_INFO_MEMBER(argus_get_bg0_tile_info);
	TILE_GET_INFO_MEMBER(argus_get_bg1_tile_info);
	TILE_GET_INFO_MEMBER(valtric_get_tx_tile_info);
	TILE_GET_INFO_MEMBER(valtric_get_bg_tile_info);
	TILE_GET_INFO_MEMBER(butasan_get_tx_tile_info);
	TILE_GET_INFO_MEMBER(butasan_get_bg0_tile_info);
	TILE_GET_INFO_MEMBER(butasan_get_bg1_tile_info);
	DECLARE_VIDEO_START(argus);
	DECLARE_VIDEO_RESET(argus);
	DECLARE_VIDEO_START(valtric);
	DECLARE_VIDEO_RESET(valtric);
	DECLARE_VIDEO_START(butasan);
	DECLARE_VIDEO_RESET(butasan);
	UINT32 screen_update_argus(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_valtric(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_butasan(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	TIMER_DEVICE_CALLBACK_MEMBER(argus_scanline);
	TIMER_DEVICE_CALLBACK_MEMBER(butasan_scanline);
};
