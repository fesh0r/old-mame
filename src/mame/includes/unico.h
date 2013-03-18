class unico_state : public driver_device
{
public:
	unico_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_vram(*this, "vram"),
		m_scroll(*this, "scroll"),
		m_vram32(*this, "vram32"),
		m_scroll32(*this, "scroll32"),
		m_spriteram(*this, "spriteram", 0){ }

	optional_shared_ptr<UINT16> m_vram;
	optional_shared_ptr<UINT16> m_scroll;
	optional_shared_ptr<UINT32> m_vram32;
	optional_shared_ptr<UINT32> m_scroll32;
	tilemap_t *m_tilemap[3];
	int m_sprites_scrolldx;
	int m_sprites_scrolldy;
	optional_shared_ptr<UINT16> m_spriteram;
	DECLARE_WRITE16_MEMBER(zeropnt_sound_bank_w);
	DECLARE_READ16_MEMBER(unico_gunx_0_msb_r);
	DECLARE_READ16_MEMBER(unico_guny_0_msb_r);
	DECLARE_READ16_MEMBER(unico_gunx_1_msb_r);
	DECLARE_READ16_MEMBER(unico_guny_1_msb_r);
	DECLARE_READ32_MEMBER(zeropnt2_gunx_0_msb_r);
	DECLARE_READ32_MEMBER(zeropnt2_guny_0_msb_r);
	DECLARE_READ32_MEMBER(zeropnt2_gunx_1_msb_r);
	DECLARE_READ32_MEMBER(zeropnt2_guny_1_msb_r);
	DECLARE_WRITE32_MEMBER(zeropnt2_sound_bank_w);
	DECLARE_WRITE32_MEMBER(zeropnt2_leds_w);
	DECLARE_WRITE16_MEMBER(unico_palette_w);
	DECLARE_WRITE32_MEMBER(unico_palette32_w);
	DECLARE_WRITE16_MEMBER(unico_vram_w);
	DECLARE_WRITE32_MEMBER(unico_vram32_w);
	DECLARE_WRITE16_MEMBER(burglarx_sound_bank_w);
	DECLARE_WRITE32_MEMBER(zeropnt2_eeprom_w);
	TILE_GET_INFO_MEMBER(get_tile_info);
	TILE_GET_INFO_MEMBER(get_tile_info32);
	DECLARE_MACHINE_RESET(unico);
	DECLARE_VIDEO_START(unico);
	DECLARE_MACHINE_RESET(zeropt);
	DECLARE_VIDEO_START(zeropnt2);
	UINT32 screen_update_unico(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_zeropnt2(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void unico_draw_sprites(bitmap_ind16 &bitmap,const rectangle &cliprect);
	void zeropnt2_draw_sprites(bitmap_ind16 &bitmap,const rectangle &cliprect);
};
