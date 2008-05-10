/*****************************************************************************
 *
 * includes/oric.h
 *
 ****************************************************************************/

#ifndef ORIC_H_
#define ORIC_H_


/*----------- defined in machine/oric.c -----------*/

MACHINE_START( oric );
MACHINE_RESET( oric );
READ8_HANDLER( oric_IO_r );
WRITE8_HANDLER( oric_IO_w );
READ8_HANDLER( oric_microdisc_r );
WRITE8_HANDLER( oric_microdisc_w );
extern UINT8 *oric_ram;

WRITE8_HANDLER(oric_psg_porta_write);

DEVICE_START( oric_floppy );
DEVICE_IMAGE_LOAD( oric_floppy );

/* Telestrat specific */
MACHINE_START( telestrat );


/*----------- defined in video/oric.c -----------*/

VIDEO_START( oric );
VIDEO_UPDATE( oric );


#endif /* ORIC_H_ */
