/*****************************************************************************
 *
 * includes/llc.h
 *
 ****************************************************************************/

#ifndef LLC_H_
#define LLC_H_

#include "machine/z80pio.h"
#include "machine/z80ctc.h"

class llc_state : public driver_device
{
public:
	llc_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag) ,
		m_video_ram(*this, "video_ram"){ }

	optional_shared_ptr<UINT8> m_video_ram;
	UINT8 m_s_code;
	UINT8 m_llc1_key_state;
	DECLARE_WRITE8_MEMBER(llc2_rom_disable_w);
	DECLARE_WRITE8_MEMBER(llc2_basic_enable_w);
};


/*----------- defined in machine/llc.c -----------*/
extern DRIVER_INIT( llc1 );
extern MACHINE_START( llc1 );
extern MACHINE_RESET( llc1 );

extern const z80pio_interface llc1_z80pio_intf;
extern const z80pio_interface llc2_z80pio_intf;

extern const z80ctc_interface llc1_ctc_intf;
extern const z80ctc_interface llc2_ctc_intf;

extern DRIVER_INIT( llc2 );
extern MACHINE_RESET( llc2 );

/*----------- defined in video/llc.c -----------*/

extern VIDEO_START( llc1 );
extern SCREEN_UPDATE_IND16( llc1 );
extern VIDEO_START( llc2 );
extern SCREEN_UPDATE_IND16( llc2 );

#endif
