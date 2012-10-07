
#include "sound/okim6295.h"

struct f2_tempsprite
{
	int code, color;
	int flipx, flipy;
	int x, y;
	int zoomx, zoomy;
	int primask;
};

class taitof2_state : public driver_device
{
public:
	taitof2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_sprite_extension(*this, "sprite_ext"),
		  m_spriteram(*this, "spriteram"),
		  m_cchip2_ram(*this, "cchip2_ram"),
		  m_oki(*this, "oki") { }

	/* memory pointers */
	optional_shared_ptr<UINT16> m_sprite_extension;
	required_shared_ptr<UINT16> m_spriteram;
	UINT16 *        m_spriteram_buffered;
	UINT16 *        m_spriteram_delayed;
	optional_shared_ptr<UINT16> m_cchip2_ram;       	// for megablst only
//  UINT16 *        m_paletteram;    // currently this uses generic palette handling


	/* video-related */
	struct f2_tempsprite *m_spritelist;
	int             m_sprite_type;

	UINT16          m_spritebank[8];
//  UINT16          m_spritebank_eof[8];
	UINT16          m_spritebank_buffered[8];

	INT32           m_sprites_disabled;
	INT32			m_sprites_active_area;
	INT32			m_sprites_master_scrollx;
	INT32			m_sprites_master_scrolly;
	/* remember flip status over frames because driftout can fail to set it */
	INT32           m_sprites_flipscreen;

	/* On the left hand screen edge (assuming horiz screen, no
       screenflip: in screenflip it is the right hand edge etc.)
       there may be 0-3 unwanted pixels in both tilemaps *and*
       sprites. To erase this we use f2_hide_pixels (0 to +3). */

	INT32           m_hide_pixels;
	INT32           m_flip_hide_pixels;	/* Different in some games */

	INT32           m_pivot_xdisp;	/* Needed in games with a pivot layer */
	INT32           m_pivot_ydisp;

	INT32           m_game;

	UINT8           m_tilepri[6]; // todo - move into taitoic.c
	UINT8           m_spritepri[6]; // todo - move into taitoic.c
	UINT8           m_spriteblendmode; // todo - move into taitoic.c

	int             m_prepare_sprites;

	/* misc */
	INT32           m_mjnquest_input;
	int             m_last[2];
	int             m_nibble;
	INT32           m_driveout_sound_latch;
	INT32           m_oki_bank;

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	optional_device<okim6295_device> m_oki;
	device_t *m_tc0100scn;
	device_t *m_tc0100scn_1;
	device_t *m_tc0100scn_2;
	device_t *m_tc0360pri;
	device_t *m_tc0280grd;
	device_t *m_tc0430grw;
	device_t *m_tc0480scp;
	DECLARE_WRITE16_MEMBER(growl_coin_word_w);
	DECLARE_WRITE16_MEMBER(taitof2_4p_coin_word_w);
	DECLARE_WRITE16_MEMBER(ninjak_coin_word_w);
	DECLARE_READ16_MEMBER(ninjak_input_r);
	DECLARE_READ16_MEMBER(cameltry_paddle_r);
	DECLARE_READ16_MEMBER(mjnquest_dsw_r);
	DECLARE_READ16_MEMBER(mjnquest_input_r);
	DECLARE_WRITE16_MEMBER(mjnquest_inputselect_w);
	DECLARE_WRITE8_MEMBER(sound_bankswitch_w);
	DECLARE_READ8_MEMBER(driveout_sound_command_r);
	DECLARE_WRITE8_MEMBER(oki_bank_w);
	DECLARE_WRITE16_MEMBER(driveout_sound_command_w);
	DECLARE_WRITE16_MEMBER(cchip2_word_w);
	DECLARE_READ16_MEMBER(cchip2_word_r);
	DECLARE_WRITE16_MEMBER(taitof2_sprite_extension_w);
	DECLARE_WRITE16_MEMBER(taitof2_spritebank_w);
	DECLARE_WRITE16_MEMBER(koshien_spritebank_w);
	DECLARE_WRITE8_MEMBER(cameltrya_porta_w);
	DECLARE_DRIVER_INIT(driveout);
	DECLARE_DRIVER_INIT(cameltry);
	DECLARE_DRIVER_INIT(mjnquest);
	DECLARE_DRIVER_INIT(finalb);
	DECLARE_MACHINE_START(f2);
	DECLARE_VIDEO_START(taitof2_default);
	DECLARE_MACHINE_START(common);
	DECLARE_VIDEO_START(taitof2_dondokod);
	DECLARE_VIDEO_START(taitof2_driftout);
	DECLARE_VIDEO_START(taitof2_finalb);
	DECLARE_VIDEO_START(taitof2_megab);
	DECLARE_VIDEO_START(taitof2_thundfox);
	DECLARE_VIDEO_START(taitof2_ssi);
	DECLARE_VIDEO_START(taitof2_gunfront);
	DECLARE_VIDEO_START(taitof2_growl);
	DECLARE_VIDEO_START(taitof2_mjnquest);
	DECLARE_VIDEO_START(taitof2_footchmp);
	DECLARE_VIDEO_START(taitof2_hthero);
	DECLARE_VIDEO_START(taitof2_koshien);
	DECLARE_VIDEO_START(taitof2_yuyugogo);
	DECLARE_VIDEO_START(taitof2_ninjak);
	DECLARE_VIDEO_START(taitof2_solfigtr);
	DECLARE_VIDEO_START(taitof2_pulirula);
	DECLARE_VIDEO_START(taitof2_metalb);
	DECLARE_VIDEO_START(taitof2_qzchikyu);
	DECLARE_VIDEO_START(taitof2_yesnoj);
	DECLARE_VIDEO_START(taitof2_deadconx);
	DECLARE_VIDEO_START(taitof2_deadconxj);
	DECLARE_VIDEO_START(taitof2_dinorex);
	DECLARE_VIDEO_START(taitof2_quiz);
	UINT32 screen_update_taitof2(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_pri_roz(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_pri(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_thundfox(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_ssi(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_deadconx(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_yesnoj(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_taitof2_metalb(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void screen_eof_taitof2_no_buffer(screen_device &screen, bool state);
	void screen_eof_taitof2_partial_buffer_delayed(screen_device &screen, bool state);
	void screen_eof_taitof2_partial_buffer_delayed_thundfox(screen_device &screen, bool state);
	void screen_eof_taitof2_full_buffer_delayed(screen_device &screen, bool state);
	void screen_eof_taitof2_partial_buffer_delayed_qzchikyu(screen_device &screen, bool state);
	INTERRUPT_GEN_MEMBER(taitof2_interrupt);
	TIMER_CALLBACK_MEMBER(taitof2_interrupt6);
};
