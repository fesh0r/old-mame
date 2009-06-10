/*****************************************************************************
 *
 * includes/cbmdrive.h
 *
 * commodore cbm floppy drives vc1541 c1551
 * synthetic simulation
 *
 * contains state machines and file system accesses
 *
 ****************************************************************************/

#ifndef CBMDRIVE_H_
#define CBMDRIVE_H_


#define IEC 1
#define SERIAL 2
#define IEEE 3


/*----------- defined in machine/cbmdrive.c -----------*/

/* data for one drive */
typedef struct
{
	int interface;
	unsigned char cmdbuffer[32];
	int cmdpos;
#define OPEN 1
#define READING 2
#define WRITING 3
	int state;						   /*0 nothing */
	unsigned char *buffer;
	int size;
	int pos;
	union
	{
		struct
		{
			int handshakein, handshakeout;
			int datain, dataout;
			int status;
			int state;
		}
		iec;
		struct
		{
			int device;
			int data, clock, atn;
			int state, value;
			int forme;				   /* i am selected */
			int last;				   /* last byte to be sent */
			int broadcast;			   /* sent to all */
			attotime time;
		}
		serial;
		struct
		{
			int device;
			int state;
			UINT8 data;
		} ieee;
	}
	i;
#define D64_IMAGE 1
	int drive;
	unsigned char *image;	   /*d64 image */

	char filename[20];
}
CBM_Drive;

extern CBM_Drive cbm_drive[2];


typedef struct
{
	int count;
	CBM_Drive *drives[4];
	/* whole + computer + drives */
	int /*reset, request[6], */ data[6], clock[6], atn[6];
}
CBM_Serial;

extern CBM_Serial cbm_serial;


void cbm_drive_open_helper(void);
void c1551_state(running_machine *machine, CBM_Drive * drive);
void vc1541_state(running_machine *machine, CBM_Drive * drive);
void c2031_state(running_machine *machine, CBM_Drive *drive);

void cbm_drive_0_config(int interface, int serialnr);
void cbm_drive_1_config(int interface, int serialnr);


/* IEC interface for c16 with c1551 */

/* To be passed directly to the drivers */
WRITE8_DEVICE_HANDLER( c1551_0_write_data );
READ8_DEVICE_HANDLER( c1551_0_read_data );
WRITE8_DEVICE_HANDLER( c1551_0_write_handshake );
READ8_DEVICE_HANDLER( c1551_0_read_handshake );
READ8_DEVICE_HANDLER( c1551_0_read_status );

WRITE8_DEVICE_HANDLER( c1551_1_write_data );
READ8_DEVICE_HANDLER( c1551_1_read_data );
WRITE8_DEVICE_HANDLER( c1551_1_write_handshake );
READ8_DEVICE_HANDLER( c1551_1_read_handshake );
READ8_DEVICE_HANDLER( c1551_1_read_status );


MACHINE_DRIVER_EXTERN( simulated_drive );

void cbmfloppy_device_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info);


#endif /* CBMDRIVE_H_ */
