/*

scsihle.h

Base class for HLE'd SCSI devices.

*/

#ifndef _SCSIHLE_H_
#define _SCSIHLE_H_

#include "machine/scsibus.h"
#include "machine/scsidev.h"

class scsihle_device : public scsidev_device
{
public:
	// construction/destruction
	scsihle_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock);

	virtual void SetDevice( void *device ) = 0;
	virtual void GetDevice( void **device ) = 0;
	virtual void SetCommand( UINT8 *command, int commandLength );
	virtual void GetCommand( UINT8 **command, int *commandLength );
	virtual void ExecCommand( int *transferLength );
	virtual void WriteData( UINT8 *data, int dataLength );
	virtual void ReadData( UINT8 *data, int dataLength );
	virtual void SetPhase( int phase );
	virtual void GetPhase( int *phase );
	virtual int GetDeviceID();
	virtual int GetSectorBytes() = 0;

	virtual void scsi_in( UINT32 data, UINT32 mask );

	// configuration helpers
	static void static_set_deviceid(device_t &device, int _scsiID);

protected:
	// device-level overrides
	virtual void device_start();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	void scsi_change_phase(UINT8 newphase);
	int get_scsi_cmd_len(int cbyte);
	UINT8 scsibus_driveno(UINT8  drivesel);
	void scsibus_read_data();
	void scsibus_write_data();
	void scsibus_exec_command();
	void check_process_dataout();
	void dump_command_bytes();
	void dump_data_bytes(int count);
	void dump_bytes(UINT8 *buff, int count);

	//emu_timer *req_timer;
	//emu_timer *ack_timer;
	emu_timer *sel_timer;
	emu_timer *dataout_timer;

	UINT8 command[ 32 ];
	UINT8 cmd_idx;
	UINT8 is_linked;

	UINT8 buffer[ 1024 ];
	UINT16 data_idx;
	int bytes_left;
	int data_last;
	int sectorbytes;

	int commandLength;
	int phase;
	int scsiID;
};

extern int SCSILengthFromUINT8( UINT8 *length );
extern int SCSILengthFromUINT16( UINT8 *length );

#define SCSI_PHASE_DATAOUT ( 0 )
#define SCSI_PHASE_DATAIN ( 1 )
#define SCSI_PHASE_COMMAND ( 2 )
#define SCSI_PHASE_STATUS ( 3 )
#define SCSI_PHASE_MESSAGE_OUT ( 6 )
#define SCSI_PHASE_MESSAGE_IN ( 7 )
#define SCSI_PHASE_BUS_FREE ( 8 )
#define SCSI_PHASE_SELECT ( 9 )

#define SCSI_CMD_TEST_UNIT_READY ( 0x00 )
#define SCSI_CMD_RECALIBRATE ( 0x01 )
#define SCSI_CMD_REQUEST_SENSE ( 0x03 )
#define SCSI_CMD_MODE_SELECT ( 0x15 )
#define SCSI_CMD_SEND_DIAGNOSTIC ( 0x1d )

//
// Status / Sense data taken from Adaptec ACB40x0 documentation.
//

#define SCSI_STATUS_OK              0x00
#define SCSI_STATUS_CHECK           0x02
#define SCSI_STATUS_EQUAL           0x04
#define SCSI_STATUS_BUSY            0x08

#define SCSI_SENSE_ADDR_VALID       0x80
#define SCSI_SENSE_NO_SENSE         0x00
#define SCSI_SENSE_NO_INDEX         0x01
#define SCSI_SENSE_SEEK_NOT_COMP    0x02
#define SCSI_SENSE_WRITE_FAULT      0x03
#define SCSI_SENSE_DRIVE_NOT_READY  0x04
#define SCSI_SENSE_NO_TRACK0        0x06
#define SCSI_SENSE_ID_CRC_ERROR     0x10
#define SCSI_SENSE_UNCORRECTABLE    0x11
#define SCSI_SENSE_ADDRESS_NF       0x12
#define SCSI_SENSE_RECORD_NOT_FOUND 0x14
#define SCSI_SENSE_SEEK_ERROR       0x15
#define SCSI_SENSE_DATA_CHECK_RETRY 0x18
#define SCSI_SENSE_ECC_VERIFY       0x19
#define SCSI_SENSE_INTERLEAVE_ERROR 0x1A
#define SCSI_SENSE_UNFORMATTED      0x1C
#define SCSI_SENSE_ILLEGAL_COMMAND  0x20
#define SCSI_SENSE_ILLEGAL_ADDRESS  0x21
#define SCSI_SENSE_VOLUME_OVERFLOW  0x23
#define SCSI_SENSE_BAD_ARGUMENT     0x24
#define SCSI_SENSE_INVALID_LUN      0x25
#define SCSI_SENSE_CART_CHANGED     0x28
#define SCSI_SENSE_ERROR_OVERFLOW   0x2C

// SCSI IDs
enum
{
	SCSI_ID_0 = 0,
	SCSI_ID_1,
	SCSI_ID_2,
	SCSI_ID_3,
	SCSI_ID_4,
	SCSI_ID_5,
	SCSI_ID_6,
	SCSI_ID_7
};

#define MCFG_SCSIDEV_ADD(_tag, _type, _id) \
	MCFG_DEVICE_ADD(_tag, _type, 0) \
	scsihle_device::static_set_deviceid(*device, _id);

#endif
