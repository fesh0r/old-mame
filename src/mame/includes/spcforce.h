#include "sound/sn76496.h"

class spcforce_state : public driver_device
{
public:
	spcforce_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_scrollram(*this, "scrollram"),
		m_videoram(*this, "videoram"),
		m_colorram(*this, "colorram"),
        m_sn1(*this, "sn1"),
        m_sn2(*this, "sn2"),
        m_sn3(*this, "sn3"){ }


	required_shared_ptr<UINT8> m_scrollram;
	required_shared_ptr<UINT8> m_videoram;
	required_shared_ptr<UINT8> m_colorram;

	required_device<sn76496_new_device> m_sn1;
    required_device<sn76496_new_device> m_sn2;
    required_device<sn76496_new_device> m_sn3;

	int m_sn76496_latch;
	int m_sn76496_select;

	UINT8 m_irq_mask;
	DECLARE_WRITE8_MEMBER(spcforce_SN76496_latch_w);
	DECLARE_READ8_MEMBER(spcforce_SN76496_select_r);
	DECLARE_WRITE8_MEMBER(spcforce_SN76496_select_w);
	DECLARE_READ8_MEMBER(spcforce_t0_r);
	DECLARE_WRITE8_MEMBER(spcforce_soundtrigger_w);
	DECLARE_WRITE8_MEMBER(irq_mask_w);
	DECLARE_WRITE8_MEMBER(spcforce_flip_screen_w);
	virtual void palette_init();
};


/*----------- defined in video/spcforce.c -----------*/

SCREEN_UPDATE_IND16( spcforce );
