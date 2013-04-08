/*****************************************************************************
 *
 * includes/pc1401.h
 *
 * Pocket Computer 1401
 *
 ****************************************************************************/

#ifndef PC1401_H_
#define PC1401_H_

#include "machine/nvram.h"

#define CONTRAST (ioport("DSW0")->read() & 0x07)


class pc1401_state : public driver_device
{
public:
	pc1401_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) { }

	UINT8 m_portc;
	UINT8 m_outa;
	UINT8 m_outb;
	int m_power;
	UINT8 m_reg[0x100];
	DECLARE_DRIVER_INIT(pc1401);
	UINT32 screen_update_pc1401(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_CALLBACK_MEMBER(pc1401_power_up);
	DECLARE_READ_LINE_MEMBER(pc1401_reset);
	DECLARE_READ_LINE_MEMBER(pc1401_brk);
	DECLARE_WRITE8_MEMBER(pc1401_outa);
	DECLARE_WRITE8_MEMBER(pc1401_outb);
	DECLARE_WRITE8_MEMBER(pc1401_outc);
	DECLARE_READ8_MEMBER(pc1401_ina);
	DECLARE_READ8_MEMBER(pc1401_inb);
	DECLARE_READ8_MEMBER(pc1401_lcd_read);
	DECLARE_WRITE8_MEMBER(pc1401_lcd_write);

	virtual void machine_start();
};

#endif /* PC1401_H_ */
