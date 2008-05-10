/*****************************************************************************
 *
 * includes/pokemini.h
 *
 ****************************************************************************/

#ifndef POKEMINI_H_
#define POKEMINI_H_


/*----------- defined in machine/pokemini.c -----------*/

extern UINT8 *pokemini_ram;
extern MACHINE_RESET( pokemini );
extern WRITE8_HANDLER( pokemini_hwreg_w );
extern READ8_HANDLER( pokemini_hwreg_r );
extern INTERRUPT_GEN( pokemini_int );

DEVICE_START( pokemini_cart );
DEVICE_IMAGE_LOAD( pokemini_cart );


#endif /* POKEMINI_H */
