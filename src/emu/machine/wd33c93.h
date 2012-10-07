/*
 * wd33c93.h
 *
 */

#ifndef _WD33C93_H_
#define _WD33C93_H_

#include "machine/scsihle.h"

struct WD33C93interface
{
	void (*irq_callback)(running_machine &machine, int state); /* irq callback */
};

/* wd register names */

enum
{
	WD_OWN_ID				= 0x00,
	WD_CONTROL				= 0x01,
	WD_TIMEOUT_PERIOD		= 0x02,
	WD_CDB_1				= 0x03,
	WD_CDB_2				= 0x04,
	WD_CDB_3				= 0x05,
	WD_CDB_4				= 0x06,
	WD_CDB_5				= 0x07,
	WD_CDB_6				= 0x08,
	WD_CDB_7				= 0x09,
	WD_CDB_8				= 0x0a,
	WD_CDB_9				= 0x0b,
	WD_CDB_10				= 0x0c,
	WD_CDB_11				= 0x0d,
	WD_CDB_12				= 0x0e,
	WD_TARGET_LUN			= 0x0f,
	WD_COMMAND_PHASE		= 0x10,
	WD_SYNCHRONOUS_TRANSFER	= 0x11,
	WD_TRANSFER_COUNT_MSB	= 0x12,
	WD_TRANSFER_COUNT		= 0x13,
	WD_TRANSFER_COUNT_LSB	= 0x14,
	WD_DESTINATION_ID		= 0x15,
	WD_SOURCE_ID			= 0x16,
	WD_SCSI_STATUS			= 0x17,
	WD_COMMAND				= 0x18,
	WD_DATA					= 0x19,
	WD_QUEUE_TAG			= 0x1a,
	WD_AUXILIARY_STATUS		= 0x1f
};

#define TEMP_INPUT_LEN	262144
#define FIFO_SIZE		12

#define MCFG_WD33C93_ADD( _tag, _config ) \
	MCFG_DEVICE_ADD( _tag, WD33C93, 0 ) \
	MCFG_DEVICE_CONFIG(_config)

class wd33c93_device : public device_t,
					   public WD33C93interface
{
public:
	// construction/destruction
	wd33c93_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	DECLARE_READ8_MEMBER(read);
	DECLARE_WRITE8_MEMBER(write);

	void get_dma_data( int bytes, UINT8 *pData );
	void write_data(int bytes, UINT8 *pData);
	void clear_dma();
	int get_dma_count();

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr);

private:
	UINT8 getunit( void );
	void set_xfer_count( int count );
	int get_xfer_count( void );
	void read_data(int bytes, UINT8 *pData);
	void complete_immediate( int status );
	void complete_cmd( UINT8 status );
	void unimplemented_cmd();
	void invalid_cmd();
	void reset_cmd();
	void abort_cmd();
	void disconnect_cmd();
	void select_cmd();
	void selectxfer_cmd();
	void negate_ack();
	void xferinfo_cmd();
	void dispatch_command();

	scsihle_device *devices[8];	// SCSI IDs 0-7

	UINT8		sasr;
	UINT8		regs[WD_AUXILIARY_STATUS+1];
	UINT8		fifo[FIFO_SIZE];
	int			fifo_pos;
	UINT8		temp_input[TEMP_INPUT_LEN];
	int			temp_input_pos;
	UINT8		busphase;
	UINT8		identify;
	int			read_pending;
	emu_timer	*cmd_timer;
	emu_timer	*service_req_timer;
	emu_timer	*deassert_cip_timer;
};

// device type definition
extern const device_type WD33C93;

#endif
