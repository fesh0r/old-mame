/***************************************************************************

    Taito Qix hardware

    driver by John Butler, Ed Mueller, Aaron Giles

***************************************************************************/

#include "video/mc6845.h"


#define MAIN_CLOCK_OSC			20000000	/* 20 MHz */
#define SLITHER_CLOCK_OSC		21300000	/* 21.3 MHz */
#define SOUND_CLOCK_OSC			7372800		/* 7.3728 MHz */
#define COIN_CLOCK_OSC			4000000		/* 4 MHz */
#define QIX_CHARACTER_CLOCK		(20000000/2/16)


typedef struct _qix_state qix_state;
struct _qix_state
{
	/* machine state */
	UINT8 *_68705_port_out;
	UINT8 *_68705_ddr;
	UINT8  _68705_port_in[3];
	UINT8  coinctrl;

	/* video state */
	UINT8 *videoram;
	UINT8 *videoram_address;
	UINT8 *videoram_mask;
	UINT8 *paletteram;
	UINT8  flip;
	UINT8  palette_bank;
	UINT8  leds;
	UINT8 *scanline_latch;
};


/*----------- defined in machine/qix.c -----------*/

MACHINE_START( qix );
MACHINE_START( qixmcu );
MACHINE_START( slither );
MACHINE_RESET( qix );

WRITE8_HANDLER( zookeep_bankswitch_w );

READ8_HANDLER( qix_data_firq_r );
READ8_HANDLER( qix_data_firq_ack_r );
WRITE8_HANDLER( qix_data_firq_w );
WRITE8_HANDLER( qix_data_firq_ack_w );

READ8_HANDLER( qix_video_firq_r );
READ8_HANDLER( qix_video_firq_ack_r );
WRITE8_HANDLER( qix_video_firq_w );
WRITE8_HANDLER( qix_video_firq_ack_w );

READ8_HANDLER( qix_68705_portA_r );
READ8_HANDLER( qix_68705_portB_r );
READ8_HANDLER( qix_68705_portC_r );
WRITE8_HANDLER( qix_68705_portA_w );
WRITE8_HANDLER( qix_68705_portB_w );
WRITE8_HANDLER( qix_68705_portC_w );

WRITE8_HANDLER( qix_pia_0_w );

MC6845_ON_VSYNC_CHANGED( qix_vsync_changed );


/*----------- defined in video/qix.c -----------*/

MACHINE_DRIVER_EXTERN( qix_video );
MACHINE_DRIVER_EXTERN( zookeep_video );
MACHINE_DRIVER_EXTERN( slither_video );

WRITE8_HANDLER( qix_flip_screen_w );
WRITE8_HANDLER( qix_palettebank_w );


/*----------- defined in audio/qix.c -----------*/

MACHINE_DRIVER_EXTERN( qix_audio );
MACHINE_DRIVER_EXTERN( slither_audio );

WRITE8_HANDLER( qix_dac_w );
WRITE8_HANDLER( qix_vol_w );
