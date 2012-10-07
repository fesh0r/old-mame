/*****************************************************************************
 *
 * includes/orao.h
 *
 ****************************************************************************/

#ifndef ORAO_H_
#define ORAO_H_


class orao_state : public driver_device
{
public:
	orao_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_memory(*this, "memory"),
		m_video_ram(*this, "video_ram"){ }

	required_shared_ptr<UINT8> m_memory;
	required_shared_ptr<UINT8> m_video_ram;
	DECLARE_READ8_MEMBER(orao_io_r);
	DECLARE_WRITE8_MEMBER(orao_io_w);
	DECLARE_DRIVER_INIT(orao);
	DECLARE_DRIVER_INIT(orao103);
	virtual void machine_reset();
	virtual void video_start();
	UINT32 screen_update_orao(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
};

#endif /* ORAO_H_ */
