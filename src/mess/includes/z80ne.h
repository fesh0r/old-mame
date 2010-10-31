/*****************************************************************************
 *
 * includes/z80ne.h
 *
 * Nuova Elettronica Z80NE
 *
 * http://www.z80ne.com/
 *
 ****************************************************************************/

#ifndef Z80NE_H_
#define Z80NE_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define Z80NE_CPU_SPEED_HZ		1920000	/* 1.92 MHz */

#define LX383_KEYS			16
#define LX383_DOWNSAMPLING	16

#define LX385_TAPE_SAMPLE_FREQ 38400


class z80ne_state : public driver_device
{
public:
	z80ne_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *videoram;
};


/*----------- defined in machine/z80ne.c -----------*/

READ8_HANDLER(lx383_r);
WRITE8_HANDLER(lx383_w);
READ8_HANDLER(lx385_data_r);
WRITE8_HANDLER(lx385_data_w);
READ8_HANDLER(lx385_ctrl_r);
WRITE8_HANDLER(lx385_ctrl_w);
READ8_DEVICE_HANDLER(lx388_mc6847_videoram_r);
VIDEO_UPDATE(lx388);
READ8_HANDLER(lx388_data_r);
READ8_HANDLER(lx388_read_field_sync);
READ8_DEVICE_HANDLER(lx390_fdc_r);
WRITE8_DEVICE_HANDLER(lx390_fdc_w);
READ8_DEVICE_HANDLER(lx390_reset_bank);
WRITE8_DEVICE_HANDLER(lx390_motor_w);


DRIVER_INIT(z80ne);
DRIVER_INIT(z80net);
DRIVER_INIT(z80netb);
DRIVER_INIT(z80netf);
MACHINE_RESET(z80ne);
MACHINE_RESET(z80net);
MACHINE_RESET(z80netb);
MACHINE_RESET(z80netf);
MACHINE_START(z80ne);
MACHINE_START(z80net);
MACHINE_START(z80netb);
MACHINE_START(z80netf);

INPUT_CHANGED(z80ne_reset);
INPUT_CHANGED(z80ne_nmi);

#endif /* Z80NE_H_ */
