/*****************************************************************************
 *
 * includes/svi318.h
 *
 * Spectravideo SVI-318 and SVI-328
 *
 ****************************************************************************/

#ifndef SVI318_H_
#define SVI318_H_

/*----------- defined in machine/svi318.c -----------*/

DRIVER_INIT( svi318 );
MACHINE_START( svi318_pal );
MACHINE_START( svi318_ntsc );
MACHINE_RESET( svi318 );

DEVICE_INIT( svi318_cart );
DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt(int i);

WRITE8_HANDLER( svi318_writemem1 );
WRITE8_HANDLER( svi318_writemem2 );
WRITE8_HANDLER( svi318_writemem3 );
WRITE8_HANDLER( svi318_writemem4 );

READ8_HANDLER( svi318_io_ext_r );
WRITE8_HANDLER( svi318_io_ext_w );

READ8_HANDLER( svi318_ppi_r );
WRITE8_HANDLER( svi318_ppi_w );

WRITE8_HANDLER( svi318_psg_port_b_w );
READ8_HANDLER( svi318_psg_port_a_r );

int svi318_cassette_present(int id);

DEVICE_LOAD( svi318_floppy );

MC6845_UPDATE_ROW( svi806_crtc6845_update_row );
VIDEO_START( svi328_806 );
VIDEO_UPDATE( svi328_806 );
MACHINE_RESET( svi328_806 );

#endif /* SVI318_H_ */
