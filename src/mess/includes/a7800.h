/*****************************************************************************
 *
 * includes/a7800.h
 *
 ****************************************************************************/

#ifndef A7800_H_
#define A7800_H_


/*----------- defined in video/a7800.c -----------*/

VIDEO_START( a7800 );
VIDEO_UPDATE( a7800 );
INTERRUPT_GEN( a7800_interrupt );
 READ8_HANDLER  ( a7800_MARIA_r);
WRITE8_HANDLER ( a7800_MARIA_w );


/*----------- defined in machine/a7800.c -----------*/

extern int a7800_lines;
extern int a7800_ispal;

MACHINE_RESET( a7800 );

void a7800_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions);

DEVICE_INIT( a7800_cart );
DEVICE_LOAD( a7800_cart );

 READ8_HANDLER  ( a7800_TIA_r );
WRITE8_HANDLER ( a7800_TIA_w );
 READ8_HANDLER  ( a7800_RIOT_r );
WRITE8_HANDLER ( a7800_RAM0_w );
WRITE8_HANDLER ( a7800_cart_w );

DRIVER_INIT( a7800_ntsc );
DRIVER_INIT( a7800_pal );


#endif /* A7800_H_ */
