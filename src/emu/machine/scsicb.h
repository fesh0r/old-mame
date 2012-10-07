/*

scsicb.h

Implementation of a raw SCSI/SASI bus for machines that don't use a SCSI
controler chip such as the RM Nimbus, which implements it as a bunch of
74LS series chips.

*/

#pragma once

#ifndef _SCSICB_H_
#define _SCSICB_H_

#include "scsidev.h"

struct SCSICB_interface
{
	devcb_write_line _out_bsy_func;
	devcb_write_line _out_sel_func;
	devcb_write_line _out_cd_func;
	devcb_write_line _out_io_func;
	devcb_write_line _out_msg_func;
	devcb_write_line _out_req_func;
	devcb_write_line _out_ack_func;
	devcb_write_line _out_atn_func;
	devcb_write_line _out_rst_func;
};

class scsicb_device : public scsidev_device,
					  public SCSICB_interface
{
public:
	// construction/destruction
	scsicb_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	virtual void scsi_in( UINT32 data, UINT32 mask );

	UINT8 scsi_data_r();
	void scsi_data_w( UINT8 data );

	DECLARE_READ8_MEMBER( scsi_data_r );
	DECLARE_WRITE8_MEMBER( scsi_data_w );

	DECLARE_READ_LINE_MEMBER( scsi_bsy_r );
	DECLARE_READ_LINE_MEMBER( scsi_sel_r );
	DECLARE_READ_LINE_MEMBER( scsi_cd_r );
	DECLARE_READ_LINE_MEMBER( scsi_io_r );
	DECLARE_READ_LINE_MEMBER( scsi_msg_r );
	DECLARE_READ_LINE_MEMBER( scsi_req_r );
	DECLARE_READ_LINE_MEMBER( scsi_ack_r );
	DECLARE_READ_LINE_MEMBER( scsi_atn_r );
	DECLARE_READ_LINE_MEMBER( scsi_rst_r );

	DECLARE_WRITE_LINE_MEMBER( scsi_bsy_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_sel_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_cd_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_io_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_msg_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_req_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_ack_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_atn_w );
	DECLARE_WRITE_LINE_MEMBER( scsi_rst_w );

protected:
	// device-level overrides
	virtual void device_config_complete();
	virtual void device_start();

private:
	devcb_resolved_write_line out_bsy_func;
	devcb_resolved_write_line out_sel_func;
	devcb_resolved_write_line out_cd_func;
	devcb_resolved_write_line out_io_func;
	devcb_resolved_write_line out_msg_func;
	devcb_resolved_write_line out_req_func;
	devcb_resolved_write_line out_ack_func;
	devcb_resolved_write_line out_atn_func;
	devcb_resolved_write_line out_rst_func;

	UINT8 get_scsi_line(UINT32 mask);
	void set_scsi_line(UINT32 mask, UINT8 state);

	UINT32 linestate;
};

#define MCFG_SCSICB_ADD(_tag, _intf) \
	MCFG_DEVICE_ADD(_tag, SCSICB, 0) \
	MCFG_DEVICE_CONFIG(_intf)

// device type definition
extern const device_type SCSICB;

#endif
