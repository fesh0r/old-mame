/*************************************************************************

	Atari Dominos hardware

*************************************************************************/

/*----------- defined in machine/dominos.c -----------*/

READ_HANDLER( dominos_port_r );
READ_HANDLER( dominos_sync_r );
WRITE_HANDLER( dominos_attract_w );
WRITE_HANDLER( dominos_tumble_w );
WRITE_HANDLER( dominos_lamp2_w );
WRITE_HANDLER( dominos_lamp1_w );


/*----------- defined in vidhrdw/dominos.c -----------*/

VIDEO_UPDATE( dominos );
