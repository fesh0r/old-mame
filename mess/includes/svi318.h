/*
** Spectravideo SVI-318 and SVI-328
*/

/* disk emulation doesn't work yet! */
#define SVI_DISK */

typedef struct {
	/* general */
	int svi318;
	/* memory */
	UINT8 *banks[2][4], *empty_bank, bank_switch, bank1, bank2;
	/* printer */
	UINT8 prn_data, prn_strobe;
} SVI_318;

DRIVER_INIT( svi318 );
MACHINE_INIT( svi318 );
MACHINE_STOP( svi318 );

DEVICE_LOAD( svi318_cart );
DEVICE_UNLOAD( svi318_cart );

INTERRUPT_GEN( svi318_interrupt );
void svi318_vdp_interrupt (int i);

WRITE_HANDLER (svi318_writemem0);
WRITE_HANDLER (svi318_writemem1);

READ_HANDLER (svi318_printer_r);
WRITE_HANDLER (svi318_printer_w);

READ_HANDLER (svi318_ppi_r);
WRITE_HANDLER (svi318_ppi_w);

WRITE_HANDLER (svi318_psg_port_b_w);
READ_HANDLER (svi318_psg_port_a_r);

/* cassette functions */
DEVICE_LOAD( svi318_cassette );
int svi318_cassette_present (int id);

/* floppy functions */
#ifdef SVI_DISK
WRITE_HANDLER (fdc_disk_motor_w);
WRITE_HANDLER (fdc_density_side_w);
READ_HANDLER (svi318_fdc_status_r);
#endif

DEVICE_LOAD( svi318_floppy );

