/**********************************************************************

    Intel 8275 Programmable CRT Controller emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************
                            _____   _____
                   LC3   1 |*    \_/     | 40  Vcc
                   LC2   2 |             | 39  LA0
                   LC1   3 |             | 38  LA1
                   LC0   4 |             | 37  LTEN
                   DRQ   5 |             | 36  RVV
                 _DACK   6 |             | 35  VSP
                  HRTC   7 |             | 34  GPA1
                  VRTC   8 |             | 33  GPA0
                   _RD   9 |             | 32  HLGT
                   _WR  10 |     8275    | 31  IRQ
                  LPEN  11 |             | 30  CCLK
                   DB0  12 |             | 29  CC6
                   DB1  13 |             | 28  CC5
                   DB2  14 |             | 27  CC4
                   DB3  15 |             | 26  CC3
                   DB4  16 |             | 25  CC2
                   DB5  17 |             | 24  CC1
                   DB6  18 |             | 23  CC0
                   DB7  19 |             | 22  _CS
                   GND  20 |_____________| 21  A0

**********************************************************************/

#pragma once

#ifndef __I8275x__
#define __I8275x__

#include "emu.h"



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_I8275_ADD(_tag, _clock, _hpixels_per_column, _display_func, _drq) \
	MCFG_DEVICE_ADD(_tag, I8275x, _clock) \
	downcast<i8275x_device *>(device)->set_hpixels_per_column(_hpixels_per_column); \
	downcast<i8275x_device *>(device)->set_display_func(_display_func); \
	downcast<i8275x_device *>(device)->set_drq_callback(DEVCB2_##_drq);

#define MCFG_I8275_IRQ_CALLBACK(_irq) \
	downcast<i8275x_device *>(device)->set_irq_callback(DEVCB2_##_irq);
#define MCFG_I8275_HRTC_CALLBACK(_hrtc) \
	downcast<i8275x_device *>(device)->set_hrtc_callback(DEVCB2_##_hrtc);
#define MCFG_I8275_VRTC_CALLBACK(_vrtc) \
	downcast<i8275x_device *>(device)->set_vrtc_callback(DEVCB2_##_vrtc);



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class i8275x_device;


// ======================> i8275_display_pixels_func

typedef void (*i8275_display_pixels_func)(i8275x_device *device, bitmap_rgb32 &bitmap, int x, int y, UINT8 linecount, UINT8 charcode, UINT8 lineattr, UINT8 lten, UINT8 rvv, UINT8 vsp, UINT8 gpa, UINT8 hlgt);
#define I8275_DISPLAY_PIXELS(name)  void name(i8275x_device *device, bitmap_rgb32 &bitmap, int x, int y, UINT8 linecount, UINT8 charcode, UINT8 lineattr, UINT8 lten, UINT8 rvv, UINT8 vsp, UINT8 gpa, UINT8 hlgt)


// ======================> i8275x_device

class i8275x_device :   public device_t,
						public device_video_interface
{
public:
	// construction/destruction
	i8275x_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	template<class _irq> void set_irq_callback(_irq irq) { m_write_irq.set_callback(irq); }
	template<class _drq> void set_drq_callback(_drq drq) { m_write_drq.set_callback(drq); }
	template<class _hrtc> void set_hrtc_callback(_hrtc hrtc) { m_write_hrtc.set_callback(hrtc); }
	template<class _vrtc> void set_vrtc_callback(_vrtc vrtc) { m_write_vrtc.set_callback(vrtc); }
	void set_hpixels_per_column(int hpixels_per_column) { m_hpixels_per_column = hpixels_per_column; }
	void set_display_func(i8275_display_pixels_func func) { m_display_pixels = func; }

	DECLARE_READ8_MEMBER( read );
	DECLARE_WRITE8_MEMBER( write );

	DECLARE_WRITE8_MEMBER( dack_w );

	DECLARE_WRITE_LINE_MEMBER( lpen_w );

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_reset();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

	void recompute_parameters();

	enum
	{
		TIMER_HRTC_ON,
		TIMER_DRQ_ON,
		TIMER_DRQ_OFF,
		TIMER_SCANLINE
	};

	enum
	{
		ST_IE = 0x40,
		ST_IR = 0x20,
		ST_LP = 0x10,
		ST_IC = 0x08,
		ST_VE = 0x04,
		ST_DU = 0x02,
		ST_FO = 0x01
	};

	enum
	{
		CMD_RESET = 0,
		CMD_START_DISPLAY,
		CMD_STOP_DISPLAY,
		CMD_READ_LIGHT_PEN,
		CMD_LOAD_CURSOR,
		CMD_ENABLE_INTERRUPT,
		CMD_DISABLE_INTERRUPT,
		CMD_PRESET_COUNTERS
	};

	enum
	{
		REG_SCN1 = 0,
		REG_SCN2,
		REG_SCN3,
		REG_SCN4,
		REG_CUR_COL,
		REG_CUR_ROW,
		REG_LPEN_COL,
		REG_LPEN_ROW,
		REG_DMA
	};

	enum
	{
		CA_H = 0x01,
		CA_B = 0x02,
		CA_CCCC = 0x3c
	};

	enum
	{
		FAC_H = 0x01,
		FAC_B = 0x02,
		FAC_GG = 0x0c,
		FAC_R = 0x10,
		FAC_U = 0x20
	};

	devcb2_write_line   m_write_irq;
	devcb2_write_line   m_write_drq;
	devcb2_write_line   m_write_hrtc;
	devcb2_write_line   m_write_vrtc;

	i8275_display_pixels_func m_display_pixels;
	int m_hpixels_per_column;

	bitmap_rgb32 m_bitmap;

	UINT8 m_status;
	UINT8 m_param[REG_DMA + 1];
	int m_param_idx;
	int m_param_end;

	UINT8 m_buffer[2][80];
	UINT8 m_fifo[2][16];
	int m_buffer_idx;
	int m_fifo_idx;
	bool m_fifo_next;
	int m_buffer_dma;

	int m_lpen;

	int m_hlgt;
	int m_vsp;
	int m_gpa;
	int m_rvv;
	int m_lten;

	int m_scanline;
	int m_irq_scanline;
	int m_vrtc_scanline;
	int m_vrtc_drq_scanline;
	bool m_du;

	int m_cursor_blink;
	int m_char_blink;

	// timers
	emu_timer *m_hrtc_on_timer;
	emu_timer *m_drq_on_timer;
	emu_timer *m_drq_off_timer;
	emu_timer *m_scanline_timer;
};


// device type definition
extern const device_type I8275x;



#endif
