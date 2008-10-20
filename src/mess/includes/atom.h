/*****************************************************************************
 *
 * includes/atom.h
 *
 ****************************************************************************/

#ifndef ATOM_H_
#define ATOM_H_

#include "devices/snapquik.h"
#include "machine/8255ppi.h"


/*----------- defined in machine/atom.c -----------*/

extern UINT8 atom_8255_porta;
extern UINT8 atom_8255_portc;

extern const ppi8255_interface atom_8255_int;

MACHINE_RESET( atom );
QUICKLOAD_LOAD( atom );
READ8_DEVICE_HANDLER (atom_8255_porta_r);
READ8_DEVICE_HANDLER (atom_8255_portb_r);
READ8_DEVICE_HANDLER (atom_8255_portc_r);
WRITE8_DEVICE_HANDLER (atom_8255_porta_w );
WRITE8_DEVICE_HANDLER (atom_8255_portb_w );
WRITE8_DEVICE_HANDLER (atom_8255_portc_w );

/* for floppy disc interface */
READ8_HANDLER (atom_8271_r);
WRITE8_HANDLER (atom_8271_w);

DEVICE_IMAGE_LOAD( atom_floppy );

READ8_HANDLER(atom_eprom_box_r);
WRITE8_HANDLER(atom_eprom_box_w);
void atom_eprom_box_init(running_machine *machine);

MACHINE_RESET( atomeb );


/*----------- defined in video/atom.c -----------*/

VIDEO_START( atom );


#endif /* ATOM_H_ */
