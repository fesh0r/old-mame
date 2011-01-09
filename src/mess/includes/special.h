/*****************************************************************************
 *
 * includes/special.h
 *
 ****************************************************************************/

#ifndef SPECIAL_H_
#define SPECIAL_H_

#include "machine/i8255a.h"
#include "machine/pit8253.h"

class special_state : public driver_device
{
public:
	special_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *specimx_colorram;
	UINT8 erik_color_1;
	UINT8 erik_color_2;
	UINT8 erik_background;
	UINT8 *specialist_video_ram;
	UINT8 specimx_color;
	device_t *specimx_audio;
	int specialist_8255_porta;
	int specialist_8255_portb;
	int specialist_8255_portc;
	UINT8 RR_register;
	UINT8 RC_register;
};


/*----------- defined in machine/special.c -----------*/

extern const struct pit8253_config specimx_pit8253_intf;
extern const i8255a_interface specialist_ppi8255_interface;

DRIVER_INIT( special );
MACHINE_RESET( special );

READ8_HANDLER( specialist_keyboard_r );
WRITE8_HANDLER( specialist_keyboard_w );

MACHINE_RESET( specimx );
MACHINE_START ( specimx );

WRITE8_HANDLER( specimx_select_bank );

READ8_HANDLER ( specimx_video_color_r );
WRITE8_HANDLER( specimx_video_color_w );

READ8_HANDLER ( specimx_disk_ctrl_r );
WRITE8_HANDLER( specimx_disk_ctrl_w );

DRIVER_INIT( erik );
MACHINE_RESET( erik );

READ8_HANDLER ( erik_rr_reg_r );
WRITE8_HANDLER( erik_rr_reg_w );
READ8_HANDLER ( erik_rc_reg_r );
WRITE8_HANDLER( erik_rc_reg_w );
READ8_HANDLER ( erik_disk_reg_r );
WRITE8_HANDLER( erik_disk_reg_w );

/*----------- defined in video/special.c -----------*/

VIDEO_START( special );
VIDEO_UPDATE( special );

VIDEO_START( specialp );
VIDEO_UPDATE( specialp );

VIDEO_START( specimx );
VIDEO_UPDATE( specimx );

VIDEO_START( erik );
VIDEO_UPDATE( erik );
PALETTE_INIT( erik );

PALETTE_INIT( specimx );
extern const rgb_t specimx_palette[16];

/*----------- defined in audio/special.c -----------*/

DECLARE_LEGACY_SOUND_DEVICE(SPECIMX, specimx_sound);

void specimx_set_input(device_t *device, int index, int state);

#endif /* SPECIAL_H_ */
