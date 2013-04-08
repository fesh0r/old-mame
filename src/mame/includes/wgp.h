/*************************************************************************

    World Grand Prix

*************************************************************************/

#include <audio/taitosnd.h>

class wgp_state : public driver_device
{
public:
	wgp_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_spritemap(*this, "spritemap"),
		m_spriteram(*this, "spriteram"),
		m_pivram(*this, "pivram"),
		m_piv_ctrlram(*this, "piv_ctrlram"),
		m_sharedram(*this, "sharedram"){ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_spritemap;
	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_pivram;
	required_shared_ptr<UINT16> m_piv_ctrlram;
	required_shared_ptr<UINT16> m_sharedram;
//  UINT16 *    m_paletteram;    // currently this uses generic palette handling

	/* video-related */
	tilemap_t   *m_piv_tilemap[3];
	UINT16      m_piv_ctrl_reg;
	UINT16      m_piv_zoom[3];
	UINT16      m_piv_scrollx[3];
	UINT16      m_piv_scrolly[3];
	UINT16      m_rotate_ctrl[8];
	int         m_piv_xoffs;
	int         m_piv_yoffs;
	UINT8       m_dislayer[4];

	/* misc */
	UINT16      m_cpua_ctrl;
	UINT16      m_port_sel;
	INT32       m_banknum;

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	cpu_device *m_subcpu;
	device_t *m_tc0100scn;
	tc0140syt_device *m_tc0140syt;
	DECLARE_READ16_MEMBER(sharedram_r);
	DECLARE_WRITE16_MEMBER(sharedram_w);
	DECLARE_WRITE16_MEMBER(cpua_ctrl_w);
	DECLARE_READ16_MEMBER(lan_status_r);
	DECLARE_WRITE16_MEMBER(rotate_port_w);
	DECLARE_READ16_MEMBER(wgp_adinput_r);
	DECLARE_WRITE16_MEMBER(wgp_adinput_w);
	DECLARE_WRITE8_MEMBER(sound_bankswitch_w);
	DECLARE_WRITE16_MEMBER(wgp_sound_w);
	DECLARE_READ16_MEMBER(wgp_sound_r);
	DECLARE_READ16_MEMBER(wgp_pivram_word_r);
	DECLARE_WRITE16_MEMBER(wgp_pivram_word_w);
	DECLARE_READ16_MEMBER(wgp_piv_ctrl_word_r);
	DECLARE_WRITE16_MEMBER(wgp_piv_ctrl_word_w);
	DECLARE_DRIVER_INIT(wgp);
	DECLARE_DRIVER_INIT(wgp2);
	TILE_GET_INFO_MEMBER(get_piv0_tile_info);
	TILE_GET_INFO_MEMBER(get_piv1_tile_info);
	TILE_GET_INFO_MEMBER(get_piv2_tile_info);
	virtual void machine_start();
	virtual void machine_reset();
	virtual void video_start();
	DECLARE_VIDEO_START(wgp2);
	UINT32 screen_update_wgp(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(wgp_cpub_interrupt);
	TIMER_CALLBACK_MEMBER(wgp_interrupt4);
	TIMER_CALLBACK_MEMBER(wgp_interrupt6);
	TIMER_CALLBACK_MEMBER(wgp_cpub_interrupt6);
	void wgp_postload();
	inline void common_get_piv_tile_info( tile_data &tileinfo, int tile_index, int num );
	void wgp_core_vh_start( int piv_xoffs, int piv_yoffs );
	void draw_sprites( bitmap_ind16 &bitmap, const rectangle &cliprect, int y_offs );
	void wgp_piv_layer_draw( bitmap_ind16 &bitmap, const rectangle &cliprect, int layer, int flags, UINT32 priority );
	void parse_control();
	void reset_sound_region(  )  /* assumes Z80 sandwiched between the 68Ks */;
	DECLARE_WRITE_LINE_MEMBER(irqhandler);
};
