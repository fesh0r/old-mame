/***************************************************************************

  pce.h

  Headers for the NEC PC Engine/TurboGrafx16.

***************************************************************************/

#ifndef PCE_H
#define PCE_H

#define PCE_HEADER_SIZE		512

/* from machine\pce.c */
extern unsigned char *pce_user_ram; /* scratch RAM at F8 */
extern UINT8 *pce_nvram;
DEVICE_LOAD(pce_cart);
NVRAM_HANDLER( pce );
WRITE8_HANDLER ( pce_joystick_w );
 READ8_HANDLER ( pce_joystick_r );

WRITE8_HANDLER( pce_cd_intf_w );
READ8_HANDLER( pce_cd_intf_r );

#define TG_16_JOY_SIG		0x00
#define PCE_JOY_SIG			0x40
#define NO_CD_SIG			0x80
#define CD_SIG				0x00
/* these might be used to indicate something, but they always seem to return 1 */
#define CONST_SIG			0x30

struct pce_struct
{
	UINT8 io_port_options; /*driver-specific options for the PCE*/
};
extern struct pce_struct pce;
DRIVER_INIT( pce );
DRIVER_INIT( tg16 );
DRIVER_INIT( sgx );
#endif
