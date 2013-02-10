/*****************************************************************************
 *
 * includes/nascom1.h
 *
 ****************************************************************************/

#ifndef NASCOM1_H_
#define NASCOM1_H_

#include "imagedev/snapquik.h"
#include "machine/wd17xx.h"

struct nascom1_portstat_t
{
	UINT8   stat_flags;
	UINT8   stat_count;
};

struct nascom2_fdc_t
{
	UINT8 select;
	UINT8 irq;
	UINT8 drq;
};


class nascom1_state : public driver_device
{
public:
	nascom1_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_videoram(*this, "videoram"){ }

	required_shared_ptr<UINT8> m_videoram;
	device_t *m_hd6402;
	int m_tape_size;
	UINT8 *m_tape_image;
	int m_tape_index;
	nascom1_portstat_t m_portstat;
	nascom2_fdc_t m_nascom2_fdc;
	DECLARE_READ8_MEMBER(nascom2_fdc_select_r);
	DECLARE_WRITE8_MEMBER(nascom2_fdc_select_w);
	DECLARE_READ8_MEMBER(nascom2_fdc_status_r);
	DECLARE_READ8_MEMBER(nascom1_port_00_r);
	DECLARE_WRITE8_MEMBER(nascom1_port_00_w);
	DECLARE_READ8_MEMBER(nascom1_port_01_r);
	DECLARE_WRITE8_MEMBER(nascom1_port_01_w);
	DECLARE_READ8_MEMBER(nascom1_port_02_r);
	DECLARE_DRIVER_INIT(nascom1);
	virtual void machine_reset();
	UINT32 screen_update_nascom1(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	UINT32 screen_update_nascom2(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_WRITE_LINE_MEMBER(nascom2_fdc_intrq_w);
	DECLARE_WRITE_LINE_MEMBER(nascom2_fdc_drq_w);
	DECLARE_READ8_MEMBER(nascom1_hd6402_si);
	DECLARE_WRITE8_MEMBER(nascom1_hd6402_so);
	DECLARE_DEVICE_IMAGE_LOAD_MEMBER( nascom1_cassette );
	DECLARE_DEVICE_IMAGE_UNLOAD_MEMBER( nascom1_cassette );
};


/*----------- defined in machine/nascom1.c -----------*/

extern const wd17xx_interface nascom2_wd17xx_interface;

SNAPSHOT_LOAD( nascom1 );
#endif /* NASCOM1_H_ */
