/*****************************************************************************
 *
 * includes/c64.h
 * 
 * Commodore C64 Home Computer
 * 
 * peter.trauner@jk.uni-linz.ac.at
 * 
 * Documentation: www.funet.fi
 *
 ****************************************************************************/

#ifndef C64_H_
#define C64_H_

#include "machine/6526cia.h"


/*----------- defined in machine/c64.c -----------*/

/* private area */
extern UINT8 c65_keyline;
extern UINT8 c65_6511_port;

extern UINT8 c128_keyline[3];

extern UINT8 *c64_colorram;
extern UINT8 *c64_basic;
extern UINT8 *c64_kernal;
extern UINT8 *c64_chargen;
extern UINT8 *c64_memory;

UINT8 c64_m6510_port_read(UINT8 direction);
void c64_m6510_port_write(UINT8 direction, UINT8 data);

READ8_HANDLER ( c64_colorram_read );
WRITE8_HANDLER ( c64_colorram_write );

DRIVER_INIT( c64 );
DRIVER_INIT( c64pal );
DRIVER_INIT( ultimax );
DRIVER_INIT( c64gs );
DRIVER_INIT( sx64 );
void c64_common_init_machine (running_machine *machine);

MACHINE_START( c64 );
INTERRUPT_GEN( c64_frame_interrupt );
TIMER_CALLBACK( c64_tape_timer );

/* private area */
READ8_HANDLER(c64_ioarea_r);
WRITE8_HANDLER(c64_ioarea_w);

WRITE8_HANDLER ( c64_write_io );
READ8_HANDLER ( c64_read_io );
int c64_paddle_read (int which);
void c64_vic_interrupt (int level);

extern int c64_pal;
extern int c64_tape_on;
extern UINT8 *c64_roml;
extern UINT8 *c64_romh;
extern UINT8 c64_keyline[10];
extern int c128_va1617;
extern UINT8 *c64_vicaddr, *c128_vicaddr;
extern UINT8 c64_game, c64_exrom;
extern const cia6526_interface c64_cia0, c64_cia1;

void c64_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);
void ultimax_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);

#endif /* C64_H_ */
