#ifndef COLECO_H
#define COLECO_H

#include "driver.h"

/* machine/coleco.c */
int coleco_cart_verify(const UINT8 *buf, size_t size);
DEVICE_LOAD( coleco_cart );

READ_HANDLER  ( coleco_paddle_r );
WRITE_HANDLER ( coleco_paddle_toggle_off );
WRITE_HANDLER ( coleco_paddle_toggle_on );

#endif /* COLECO_H */
