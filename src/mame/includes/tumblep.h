/*************************************************************************

    Tumble Pop

*************************************************************************/

#include "video/decospr.h"

class tumblep_state : public driver_device
{
public:
	tumblep_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_spriteram(*this, "spriteram"),
		m_pf1_rowscroll(*this, "pf1_rowscroll"),
		m_pf2_rowscroll(*this, "pf2_rowscroll"),
		m_sprgen(*this, "spritegen")
	{ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_pf1_rowscroll;
	required_shared_ptr<UINT16> m_pf2_rowscroll;
	optional_device<decospr_device> m_sprgen;
//  UINT16 *  m_paletteram;    // currently this uses generic palette handling (in decocomn.c)

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	device_t *m_deco_tilegen1;
	DECLARE_READ16_MEMBER(tumblep_prot_r);
	DECLARE_WRITE16_MEMBER(tumblep_sound_w);
	DECLARE_WRITE16_MEMBER(jumppop_sound_w);
	DECLARE_READ16_MEMBER(tumblepop_controls_r);
	DECLARE_WRITE16_MEMBER(tumblep_oki_w);
	DECLARE_DRIVER_INIT(tumblep);
	virtual void machine_start();
	UINT32 screen_update_tumblep(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	void tumblep_patch_code(UINT16 offset);
};
