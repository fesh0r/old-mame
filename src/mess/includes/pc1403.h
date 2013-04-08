/*****************************************************************************
 *
 * includes/pc1403.h
 *
 * Pocket Computer 1403
 *
 ****************************************************************************/

#ifndef PC1403_H_
#define PC1403_H_

#include "machine/nvram.h"

#define CONTRAST (ioport("DSW0")->read() & 0x07)


class pc1403_state : public driver_device
{
public:
	pc1403_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 m_portc;
	UINT8 m_outa;
	int m_power;
	UINT8 m_asic[4];
	int m_DOWN;
	int m_RIGHT;
	UINT8 m_reg[0x100];

	DECLARE_DRIVER_INIT(pc1403);
	UINT32 screen_update_pc1403(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_CALLBACK_MEMBER(pc1403_power_up);
	DECLARE_READ_LINE_MEMBER(pc1403_reset);
	DECLARE_READ_LINE_MEMBER(pc1403_brk);
	DECLARE_WRITE8_MEMBER(pc1403_outa);
	DECLARE_WRITE8_MEMBER(pc1403_outc);
	DECLARE_READ8_MEMBER(pc1403_ina);

	DECLARE_READ8_MEMBER(pc1403_asic_read);
	DECLARE_WRITE8_MEMBER(pc1403_asic_write);
	DECLARE_READ8_MEMBER(pc1403_lcd_read);
	DECLARE_WRITE8_MEMBER(pc1403_lcd_write);
	virtual void video_start();
	virtual void machine_start();
};

#endif /* PC1403_H_ */
