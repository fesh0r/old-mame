/*************************************************************************

    Super Burger Time & China Town

*************************************************************************/

class supbtime_state : public driver_device
{
public:
	supbtime_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_spriteram(*this, "spriteram"),
		m_pf1_rowscroll(*this, "pf1_rowscroll"),
		m_pf2_rowscroll(*this, "pf2_rowscroll"){ }

	/* memory pointers */
	required_shared_ptr<UINT16> m_spriteram;
	required_shared_ptr<UINT16> m_pf1_rowscroll;
	required_shared_ptr<UINT16> m_pf2_rowscroll;
//  UINT16 *  m_paletteram;    // currently this uses generic palette handling (in decocomn.c)

	/* video-related */

	/* devices */
	cpu_device *m_maincpu;
	cpu_device *m_audiocpu;
	device_t *m_deco_tilegen1;
	DECLARE_READ16_MEMBER(supbtime_controls_r);
	DECLARE_WRITE16_MEMBER(sound_w);
	virtual void machine_start();
	UINT32 screen_update_supbtime(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};
