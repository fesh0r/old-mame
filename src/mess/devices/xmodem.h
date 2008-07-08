/* XMODEM protocol implementation.

   Transfer between an emulated machine and an image using the XMODEM protocol.

   Used in the HP48 G/GX emulation.

   Author: Antoine Mine'
   Date: 29/03/2008
 */

#include "timer.h"
#include "image.h"


#define XMODEM DEVICE_GET_INFO_NAME( xmodem )


typedef struct {

  /* called by XMODEM when it wants to send a byte to the emulated machine */
  void (*send)( running_machine *machine, UINT8 data );
  
} xmodem_config;



extern DEVICE_GET_INFO( xmodem );


/* call when the emulated machine has read the last byte sent by
   XMODEM through the send call-back */
extern void xmodem_byte_transmitted( const device_config *device );

/* call when the emulated machine sends a byte to XMODEM */
extern void xmodem_receive_byte( const device_config *device, UINT8 data );
