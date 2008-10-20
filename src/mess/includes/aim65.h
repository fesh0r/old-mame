/*****************************************************************************
 *
 * includes/aim65.h
 *
 * Rockwell AIM-65
 *
 ****************************************************************************/

#ifndef AIM65_H_
#define AIM65_H_


/** R6502 Clock.
 *
 * The R6502 on AIM65 operates at 1 MHz. The frequency reference is a 4 Mhz
 * crystal controlled oscillator. Dual D-type flip-flop Z10 divides the 4 MHz
 * signal by four to drive the R6502 phase 0 (O0) input with a 1 MHz clock.
 */
#define AIM65_CLOCK  XTAL_4MHz/4


/*----------- defined in machine/aim65.c -----------*/

void aim65_update_ds1(int digit, int data);
void aim65_update_ds2(int digit, int data);
void aim65_update_ds3(int digit, int data);
void aim65_update_ds4(int digit, int data);
void aim65_update_ds5(int digit, int data);

UINT8 aim65_riot_b_r(const device_config *device, UINT8 olddata);
void aim65_riot_a_w(const device_config *device, UINT8 data, UINT8 olddata);
void aim65_riot_irq(const device_config *device, int state);

DRIVER_INIT( aim65 );


/*----------- defined in video/aim65.c -----------*/

VIDEO_START( aim65 );

/* Printer */
void aim65_printer_data_a(UINT8 data);
void aim65_printer_data_b(UINT8 data);

TIMER_CALLBACK(aim65_printer_timer);
WRITE8_HANDLER( aim65_printer_on );


#endif /* AIM65_H_ */
