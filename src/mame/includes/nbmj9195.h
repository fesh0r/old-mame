#define VRAM_MAX    2

#define SCANLINE_MIN    0
#define SCANLINE_MAX    512


class nbmj9195_state : public driver_device
{
public:
	nbmj9195_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	int m_inputport;
	int m_dipswbitsel;
	int m_outcoin_flag;
	int m_mscoutm_inputport;
	UINT8 *m_nvram;
	size_t m_nvram_size;
	UINT8 m_pio_dir[5 * 2];
	UINT8 m_pio_latch[5 * 2];
	int m_scrollx[VRAM_MAX];
	int m_scrolly[VRAM_MAX];
	int m_scrollx_raster[VRAM_MAX][SCANLINE_MAX];
	int m_scanline[VRAM_MAX];
	int m_blitter_destx[VRAM_MAX];
	int m_blitter_desty[VRAM_MAX];
	int m_blitter_sizex[VRAM_MAX];
	int m_blitter_sizey[VRAM_MAX];
	int m_blitter_src_addr[VRAM_MAX];
	int m_blitter_direction_x[VRAM_MAX];
	int m_blitter_direction_y[VRAM_MAX];
	int m_dispflag[VRAM_MAX];
	int m_flipscreen[VRAM_MAX];
	int m_clutmode[VRAM_MAX];
	int m_transparency[VRAM_MAX];
	int m_clutsel;
	int m_screen_refresh;
	int m_gfxflag2;
	int m_gfxdraw_mode;
	int m_nb19010_busyctr;
	int m_nb19010_busyflag;
	bitmap_ind16 m_tmpbitmap[VRAM_MAX];
	UINT16 *m_videoram[VRAM_MAX];
	UINT16 *m_videoworkram[VRAM_MAX];
	UINT8 *m_palette;
	UINT8 *m_nb22090_palette;
	UINT8 *m_clut[VRAM_MAX];
	int m_flipscreen_old[VRAM_MAX];
	DECLARE_WRITE8_MEMBER(nbmj9195_soundbank_w);
	DECLARE_READ8_MEMBER(nbmj9195_sound_r);
	DECLARE_WRITE8_MEMBER(nbmj9195_sound_w);
	DECLARE_WRITE8_MEMBER(nbmj9195_soundclr_w);
	DECLARE_WRITE8_MEMBER(nbmj9195_inputportsel_w);
	DECLARE_READ8_MEMBER(mscoutm_dipsw_0_r);
	DECLARE_READ8_MEMBER(mscoutm_dipsw_1_r);
	DECLARE_READ8_MEMBER(tmpz84c011_pio_r);
	DECLARE_WRITE8_MEMBER(tmpz84c011_pio_w);
	DECLARE_READ8_MEMBER(tmpz84c011_0_pa_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_pb_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_pc_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_pd_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_pe_r);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_pa_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_pb_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_pc_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_pd_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_pe_w);
	DECLARE_READ8_MEMBER(tmpz84c011_0_dir_pa_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_dir_pb_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_dir_pc_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_dir_pd_r);
	DECLARE_READ8_MEMBER(tmpz84c011_0_dir_pe_r);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_dir_pa_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_dir_pb_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_dir_pc_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_dir_pd_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_0_dir_pe_w);
	DECLARE_READ8_MEMBER(tmpz84c011_1_pa_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_pb_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_pc_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_pd_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_pe_r);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_pa_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_pb_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_pc_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_pd_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_pe_w);
	DECLARE_READ8_MEMBER(tmpz84c011_1_dir_pa_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_dir_pb_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_dir_pc_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_dir_pd_r);
	DECLARE_READ8_MEMBER(tmpz84c011_1_dir_pe_r);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_dir_pa_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_dir_pb_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_dir_pc_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_dir_pd_w);
	DECLARE_WRITE8_MEMBER(tmpz84c011_1_dir_pe_w);
	DECLARE_READ8_MEMBER(nbmj9195_palette_r);
	DECLARE_WRITE8_MEMBER(nbmj9195_palette_w);
	DECLARE_READ8_MEMBER(nbmj9195_nb22090_palette_r);
	DECLARE_WRITE8_MEMBER(nbmj9195_nb22090_palette_w);
	DECLARE_WRITE8_MEMBER(nbmj9195_blitter_0_w);
	DECLARE_WRITE8_MEMBER(nbmj9195_blitter_1_w);
	DECLARE_READ8_MEMBER(nbmj9195_blitter_0_r);
	DECLARE_READ8_MEMBER(nbmj9195_blitter_1_r);
	DECLARE_WRITE8_MEMBER(nbmj9195_clut_0_w);
	DECLARE_WRITE8_MEMBER(nbmj9195_clut_1_w);
	DECLARE_DRIVER_INIT(nbmj9195);
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_VIDEO_START(nbmj9195_1layer);
	DECLARE_VIDEO_START(nbmj9195_nb22090);
	UINT32 screen_update_nbmj9195(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(ctc0_trg1);
	TIMER_CALLBACK_MEMBER(blitter_timer_callback);
	int nbmj9195_blitter_r(int offset, int vram);
	void nbmj9195_blitter_w(int offset, int data, int vram);
	void nbmj9195_clutsel_w(int data);
	void nbmj9195_clut_w(int offset, int data, int vram);
	void nbmj9195_gfxflag2_w(int data);
	void nbmj9195_vramflip(int vram);
	void update_pixel(int vram, int x, int y);
	void nbmj9195_gfxdraw(int vram);
	void nbmj9195_outcoin_flag_w(int data);
	int nbmj9195_dipsw_r();
	void nbmj9195_dipswbitsel_w(int data);
	void mscoutm_inputportsel_w(int data);
};
