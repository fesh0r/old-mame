/*****************************************************************************
 *
 * includes/mbee.h
 *
 ****************************************************************************/

#ifndef MBEE_H_
#define MBEE_H_

#include "machine/wd17xx.h"
#include "devices/snapquik.h"
#include "devices/z80bin.h"
#include "machine/z80pio.h"
#include "devices/cassette.h"
#include "machine/ctronics.h"
#include "video/mc6845.h"
#include "sound/speaker.h"


class mbee_state : public driver_device
{
public:
	mbee_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 dummy;
};


/*----------- defined in drivers/mbee.c -----------*/



/*----------- defined in machine/mbee.c -----------*/

extern const wd17xx_interface mbee_wd17xx_interface;
extern const z80pio_interface mbee_z80pio_intf;

DRIVER_INIT( mbee );
DRIVER_INIT( mbeeic );
DRIVER_INIT( mbeepc );
DRIVER_INIT( mbeepc85 );
DRIVER_INIT( mbeeppc );
DRIVER_INIT( mbee56 );
DRIVER_INIT( mbee64 );
DRIVER_INIT( mbee128 );
DRIVER_INIT( mbee256 );
MACHINE_RESET( mbee );
MACHINE_RESET( mbee56 );
MACHINE_RESET( mbee64 );
MACHINE_RESET( mbee128 );
MACHINE_RESET( mbee256 );
WRITE8_HANDLER( mbee_04_w );
WRITE8_HANDLER( mbee_06_w );
READ8_HANDLER( mbee_07_r );
READ8_HANDLER( mbeeic_0a_r );
WRITE8_HANDLER( mbeeic_0a_w );
READ8_HANDLER( mbeepc_telcom_low_r );
READ8_HANDLER( mbeepc_telcom_high_r );
READ8_HANDLER( mbee256_speed_low_r );
READ8_HANDLER( mbee256_speed_high_r );
READ8_HANDLER( mbee256_18_r );
WRITE8_HANDLER( mbee64_50_w );
WRITE8_HANDLER( mbee128_50_w );
WRITE8_HANDLER( mbee256_50_w );

READ8_HANDLER ( mbee_fdc_status_r );
WRITE8_HANDLER ( mbee_fdc_motor_w );
INTERRUPT_GEN( mbee_interrupt );
Z80BIN_EXECUTE( mbee );
QUICKLOAD_LOAD( mbee );


/*----------- defined in video/mbee.c -----------*/


READ8_HANDLER( m6545_status_r );
WRITE8_HANDLER( m6545_index_w );
READ8_HANDLER( m6545_data_r );
WRITE8_HANDLER( m6545_data_w );
READ8_HANDLER( mbee_low_r );
READ8_HANDLER( mbee_high_r );
READ8_HANDLER( mbeeic_high_r );
WRITE8_HANDLER( mbeeic_high_w );
WRITE8_HANDLER( mbee_low_w );
WRITE8_HANDLER( mbee_high_w );
READ8_HANDLER( mbeeic_08_r );
WRITE8_HANDLER( mbeeic_08_w );
READ8_HANDLER( mbee_0b_r );
WRITE8_HANDLER( mbee_0b_w );
READ8_HANDLER( mbeeppc_1c_r );
WRITE8_HANDLER( mbeeppc_1c_w );
WRITE8_HANDLER( mbee256_1c_w );
READ8_HANDLER( mbeeppc_low_r );
READ8_HANDLER( mbeeppc_high_r );
WRITE8_HANDLER( mbeeppc_high_w );
WRITE8_HANDLER( mbeeppc_low_w );
MC6845_UPDATE_ROW( mbee_update_row );
MC6845_UPDATE_ROW( mbeeic_update_row );
MC6845_UPDATE_ROW( mbeeppc_update_row );
MC6845_ON_UPDATE_ADDR_CHANGED( mbee_update_addr );
MC6845_ON_UPDATE_ADDR_CHANGED( mbee256_update_addr );

VIDEO_START( mbee );
VIDEO_UPDATE( mbee );
VIDEO_START( mbeeic );
VIDEO_UPDATE( mbeeic );
VIDEO_START( mbeeppc );
VIDEO_UPDATE( mbeeppc );
PALETTE_INIT( mbeeic );
PALETTE_INIT( mbeepc85b );
PALETTE_INIT( mbeeppc );

#endif /* MBEE_H_ */
