/*****************************************************************************
 *
 * includes/pokemini.h
 *
 ****************************************************************************/

#ifndef POKEMINI_H_
#define POKEMINI_H_


/*----------- defined in machine/pokemini.c -----------*/

extern UINT8 *pokemini_ram;
MACHINE_RESET( pokemini );
WRITE8_HANDLER( pokemini_hwreg_w );
READ8_HANDLER( pokemini_hwreg_r );

DEVICE_START( pokemini_cart );
DEVICE_IMAGE_LOAD( pokemini_cart );

#endif /* POKEMINI_H */