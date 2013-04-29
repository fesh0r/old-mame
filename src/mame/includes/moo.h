/*************************************************************************

    Wild West C.O.W.boys of Moo Mesa / Bucky O'Hare

*************************************************************************/
#include "sound/okim6295.h"
#include "sound/k054539.h"

class moo_state : public driver_device
{
public:
	moo_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_workram(*this, "workram"),
		m_spriteram(*this, "spriteram"),
		m_maincpu(*this, "maincpu"),
		m_soundcpu(*this, "soundcpu"),
		m_oki(*this, "oki"),
		m_k054539(*this, "k054539"),
		m_k053246(*this, "k053246"),
		m_k053251(*this, "k053251"),
		m_k056832(*this, "k056832"),
		m_k054338(*this, "k054338") { }

	/* memory pointers */
	optional_shared_ptr<UINT16> m_workram;
	required_shared_ptr<UINT16> m_spriteram;
//  UINT16 *    m_paletteram;    // currently this uses generic palette handling

	/* video-related */
	int         m_sprite_colorbase;
	int         m_layer_colorbase[4];
	int         m_layerpri[3];
	int         m_alpha_enabled;
	UINT16      m_zmask;

	/* misc */
	UINT16      m_protram[16];
	UINT16      m_cur_control2;

	/* devices */
	required_device<cpu_device> m_maincpu;
	optional_device<cpu_device> m_soundcpu;
	optional_device<okim6295_device> m_oki;
	optional_device<k054539_device> m_k054539;
	required_device<k053247_device> m_k053246;
	required_device<k053251_device> m_k053251;
	required_device<k056832_device> m_k056832;
	required_device<k054338_device> m_k054338;

	emu_timer *m_dmaend_timer;
	DECLARE_READ16_MEMBER(control2_r);
	DECLARE_WRITE16_MEMBER(control2_w);
	DECLARE_WRITE16_MEMBER(sound_cmd1_w);
	DECLARE_WRITE16_MEMBER(sound_cmd2_w);
	DECLARE_WRITE16_MEMBER(sound_irq_w);
	DECLARE_READ16_MEMBER(sound_status_r);
	DECLARE_WRITE8_MEMBER(sound_bankswitch_w);
	DECLARE_READ16_MEMBER(K053247_scattered_word_r);
	DECLARE_WRITE16_MEMBER(K053247_scattered_word_w);
	DECLARE_WRITE16_MEMBER(moo_prot_w);
	DECLARE_WRITE16_MEMBER(moobl_oki_bank_w);
	DECLARE_MACHINE_START(moo);
	DECLARE_MACHINE_RESET(moo);
	DECLARE_VIDEO_START(moo);
	DECLARE_VIDEO_START(bucky);
	UINT32 screen_update_moo(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	INTERRUPT_GEN_MEMBER(moo_interrupt);
	INTERRUPT_GEN_MEMBER(moobl_interrupt);
	TIMER_CALLBACK_MEMBER(dmaend_callback);
	void moo_objdma();
};

/*----------- defined in video/moo.c -----------*/
extern void moo_tile_callback(running_machine &machine, int layer, int *code, int *color, int *flags);
extern void moo_sprite_callback(running_machine &machine, int *code, int *color, int *priority_mask);
