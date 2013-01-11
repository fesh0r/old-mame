class konamigx_state : public driver_device
{
public:
	konamigx_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this,"maincpu"),
		m_workram(*this,"workram"),
		m_psacram(*this,"psacram"),
		m_subpaletteram32(*this,"subpaletteram"),
		m_k053936_0_ctrl(*this,"k053936_0_ctrl",32),
		m_k053936_0_linectrl(*this,"k053936_0_line",32),
		m_k053936_0_ctrl_16(*this,"k053936_0_ct16",16),
		m_k053936_0_linectrl_16(*this,"k053936_0_li16",16),
		m_konamigx_type3_psac2_bank(*this,"psac2_bank")
		{ }

	required_device<cpu_device> m_maincpu;
	optional_shared_ptr<UINT32> m_workram;
	optional_shared_ptr<UINT32> m_psacram;
	optional_shared_ptr<UINT32> m_subpaletteram32;
	optional_shared_ptr<UINT16> m_k053936_0_ctrl;
	optional_shared_ptr<UINT16> m_k053936_0_linectrl;
	optional_shared_ptr<UINT16> m_k053936_0_ctrl_16;
	optional_shared_ptr<UINT16> m_k053936_0_linectrl_16;
	optional_shared_ptr<UINT32> m_konamigx_type3_psac2_bank;
	DECLARE_WRITE32_MEMBER(esc_w);
	DECLARE_WRITE32_MEMBER(eeprom_w);
	DECLARE_WRITE32_MEMBER(control_w);
	DECLARE_READ32_MEMBER(waitskip_r);
	DECLARE_READ32_MEMBER(ccu_r);
	DECLARE_WRITE32_MEMBER(ccu_w);
	DECLARE_READ32_MEMBER(sound020_r);
	DECLARE_WRITE32_MEMBER(sound020_w);
	DECLARE_READ32_MEMBER(le2_gun_H_r);
	DECLARE_READ32_MEMBER(le2_gun_V_r);
	DECLARE_READ32_MEMBER(gx5bppspr_r);
	DECLARE_READ32_MEMBER(gx6bppspr_r);
	DECLARE_READ32_MEMBER(type1_roz_r1);
	DECLARE_READ32_MEMBER(type1_roz_r2);
	DECLARE_READ32_MEMBER(type3_sync_r);
	DECLARE_WRITE32_MEMBER(type4_prot_w);
	DECLARE_WRITE32_MEMBER(type1_cablamps_w);
	DECLARE_READ16_MEMBER(sndcomm68k_r);
	DECLARE_WRITE16_MEMBER(sndcomm68k_w);
	DECLARE_READ16_MEMBER(tms57002_data_word_r);
	DECLARE_WRITE16_MEMBER(tms57002_data_word_w);
	DECLARE_READ16_MEMBER(tms57002_status_word_r);
	DECLARE_WRITE16_MEMBER(tms57002_control_word_w);
	DECLARE_READ16_MEMBER(K055550_word_r);
	DECLARE_WRITE16_MEMBER(K055550_word_w);
	DECLARE_WRITE16_MEMBER(K053990_martchmp_word_w);
	DECLARE_WRITE32_MEMBER(fantjour_dma_w);
	DECLARE_WRITE32_MEMBER(konamigx_type3_psac2_bank_w);
	DECLARE_WRITE32_MEMBER(konamigx_palette_w);
	DECLARE_WRITE32_MEMBER(konamigx_palette2_w);
	DECLARE_WRITE32_MEMBER(konamigx_555_palette_w);
	DECLARE_WRITE32_MEMBER(konamigx_555_palette2_w);
	DECLARE_WRITE32_MEMBER(konamigx_tilebank_w);
	DECLARE_WRITE32_MEMBER(konamigx_t1_psacmap_w);
	DECLARE_WRITE32_MEMBER(konamigx_t4_psacmap_w);
	DECLARE_CUSTOM_INPUT_MEMBER(gx_rdport1_3_r);
	DECLARE_DRIVER_INIT(konamigx);
	TILE_GET_INFO_MEMBER(get_gx_psac_tile_info);
	TILE_GET_INFO_MEMBER(get_gx_psac3_tile_info);
	TILE_GET_INFO_MEMBER(get_gx_psac3_alt_tile_info);
	TILE_GET_INFO_MEMBER(get_gx_psac1a_tile_info);
	TILE_GET_INFO_MEMBER(get_gx_psac1b_tile_info);
	DECLARE_MACHINE_START(konamigx);
	DECLARE_MACHINE_RESET(konamigx);
	DECLARE_VIDEO_START(konamigx_5bpp);
	DECLARE_VIDEO_START(dragoonj);
	DECLARE_VIDEO_START(le2);
	DECLARE_VIDEO_START(konamigx_6bpp);
	DECLARE_VIDEO_START(konamigx_6bpp_2);
	DECLARE_VIDEO_START(opengolf);
	DECLARE_VIDEO_START(racinfrc);
	DECLARE_VIDEO_START(konamigx_type3);
	DECLARE_VIDEO_START(konamigx_type4);
	DECLARE_VIDEO_START(konamigx_type4_vsn);
	DECLARE_VIDEO_START(konamigx_type4_sd2);
	DECLARE_VIDEO_START(winspike);
	UINT32 screen_update_konamigx(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_konamigx_left(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_konamigx_right(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(konamigx_vbinterrupt);
	INTERRUPT_GEN_MEMBER(tms_sync);
	TIMER_CALLBACK_MEMBER(dmaend_callback);
	TIMER_DEVICE_CALLBACK_MEMBER(konamigx_hbinterrupt);
};


/*----------- defined in video/konamigx.c -----------*/

// 2nd-Tier GX/MW Hardware Functions

void K053936GP_set_offset(int chip, int xoffs, int yoffs);
void K053936GP_clip_enable(int chip, int status);
void K053936GP_set_cliprect(int chip, int minx, int maxx, int miny, int maxy);



// 1st-Tier GX/MW Variables and Functions
extern UINT8  konamigx_wrport1_0, konamigx_wrport1_1;
extern UINT16 konamigx_wrport2;



// Sprite Callbacks

/* callbacks should return color codes in this format:
    fedcba9876543210fedcba9876543210
    ----------------xxxxxxxxxxxxxxxx (bit 00-15: color)
    --------------xx---------------- (bit 16-17: blend code)
    ------------xx------------------ (bit 18-19: brightness code)
    -x------------------------------ (bit 30   : skip shadow)
    x------------------------------- (bit 31   : full shadow)
*/
#define K055555_COLORMASK   0x0000ffff
#define K055555_MIXSHIFT    16
#define K055555_BRTSHIFT    18
#define K055555_SKIPSHADOW  0x40000000
#define K055555_FULLSHADOW  0x80000000



// Centralized Sprites and Layer Blitter

/* Mixer Flags
    fedcba9876543210fedcba9876543210
    --------------------FFEEDDCCBBAA (layer A-F blend modes)
    ----------------DCBA------------ (layer A-D line/row scroll disables)
    ----FFEEDDCCBBAA---------------- (layer A-F mix codes in forced blending)
    ---x---------------------------- (disable shadows)
    --x----------------------------- (disable z-buffering)
*/
#define GXMIX_BLEND_AUTO    0           // emulate all blend effects
#define GXMIX_BLEND_NONE    1           // disable all blend effects
#define GXMIX_BLEND_FAST    2           // simulate translucency
#define GXMIX_BLEND_FORCE   3           // force mix code on selected layer(s)
#define GXMIX_NOLINESCROLL  0x1000      // disable linescroll on selected layer(s)
#define GXMIX_NOSHADOW      0x10000000  // disable all shadows (shadow pens will be skipped)
#define GXMIX_NOZBUF        0x20000000  // disable z-buffering (shadow pens will be drawn as solid)

// Sub Layer Flags
#define GXSUB_K053250   0x10    // chip type: 0=K053936 ROZ+, 1=K053250 LVC
#define GXSUB_4BPP      0x04    //  16 colors
#define GXSUB_5BPP      0x05    //  32 colors
#define GXSUB_8BPP      0x08    // 256 colors

void konamigx_mixer(running_machine &machine, bitmap_rgb32 &bitmap, const rectangle &cliprect,
					tilemap_t *sub1, int sub1flags,
					tilemap_t *sub2, int sub2flags,
					int mixerflags, bitmap_ind16* extra_bitmap, int rushingheroes_hack);

void konamigx_mixer_init(running_machine &machine, int objdma);
void konamigx_mixer_primode(int mode);


void konamigx_objdma(void);
extern UINT16 *K053247_ram;

extern int konamigx_current_frame;


/*----------- defined in machine/konamigx.c -----------*/

// K055550/K053990/ESC protection devices handlers
void konamigx_esc_alert(UINT32 *srcbase, int srcoffs, int count, int mode);

void fantjour_dma_install(running_machine &machine);
