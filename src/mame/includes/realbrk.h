class realbrk_state : public driver_device
{
public:
	realbrk_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_spriteram(*this, "spriteram"),
		m_vram_0(*this, "vram_0"),
		m_vram_1(*this, "vram_1"),
		m_vram_2(*this, "vram_2"),
		m_vregs(*this, "vregs"),
		m_dsw_select(*this, "dsw_select"),
		m_backup_ram(*this, "backup_ram"),
		m_vram_0ras(*this, "vram_0ras"),
		m_vram_1ras(*this, "vram_1ras"){ }

	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_vram_0;
	required_shared_ptr<UINT16> m_vram_1;
	required_shared_ptr<UINT16> m_vram_2;
	required_shared_ptr<UINT16> m_vregs;
	optional_shared_ptr<UINT16> m_dsw_select;
	optional_shared_ptr<UINT16> m_backup_ram;
	optional_shared_ptr<UINT16> m_vram_0ras;
	optional_shared_ptr<UINT16> m_vram_1ras;
	bitmap_ind16 *m_tmpbitmap0;
	bitmap_ind16 *m_tmpbitmap1;
	int m_disable_video;
	tilemap_t *m_tilemap_0;
	tilemap_t *m_tilemap_1;
	tilemap_t *m_tilemap_2;
	DECLARE_READ16_MEMBER(realbrk_dsw_r);
	DECLARE_READ16_MEMBER(pkgnsh_input_r);
	DECLARE_READ16_MEMBER(pkgnshdx_input_r);
	DECLARE_READ16_MEMBER(backup_ram_r);
	DECLARE_READ16_MEMBER(backup_ram_dx_r);
	DECLARE_WRITE16_MEMBER(backup_ram_w);
	DECLARE_WRITE16_MEMBER(realbrk_flipscreen_w);
	DECLARE_WRITE16_MEMBER(dai2kaku_flipscreen_w);
	DECLARE_WRITE16_MEMBER(realbrk_vram_0_w);
	DECLARE_WRITE16_MEMBER(realbrk_vram_1_w);
	DECLARE_WRITE16_MEMBER(realbrk_vram_2_w);
	DECLARE_WRITE16_MEMBER(realbrk_vregs_w);
	TILE_GET_INFO_MEMBER(get_tile_info_0);
	TILE_GET_INFO_MEMBER(get_tile_info_1);
	TILE_GET_INFO_MEMBER(get_tile_info_2);
	virtual void video_start();
	UINT32 screen_update_realbrk(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_dai2kaku(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(realbrk_interrupt);
};
