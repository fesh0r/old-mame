// license:MAME
// copyright-holders:smf
/*

scsihle.c

Base class for HLE'd SCSI devices.

*/

#include "machine/scsihle.h"

scsihle_device::scsihle_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source) :
	scsidev_device(mconfig, type, name, tag, owner, clock, shortname, source)
{
}

void scsihle_device::device_start()
{
	scsidev_device::device_start();
	t10_start(*this);

	req_timer = timer_alloc(0);
	sel_timer = timer_alloc(1);
	dataout_timer = timer_alloc(2);
}

void scsihle_device::device_reset()
{
	t10_reset();
}

int scsihle_device::GetDeviceID()
{
	return scsiID;
}

void scsihle_device::static_set_deviceid( device_t &device, int _scsiID )
{
	scsihle_device &scsidev = downcast<scsihle_device &>(device);
	scsidev.scsiID = _scsiID;
}

#define BSY_DELAY_NS    50
#define REQ_DELAY_NS    90

static const char *const phasenames[] =
{
	"data out", "data in", "command", "status", "none", "none", "message out", "message in", "bus free","select"
};

// scsidev
#define SCSI_CMD_BUFFER_WRITE ( 0x3b )
#define SCSI_CMD_BUFFER_READ ( 0x3c )

// scsihd
#define SCSI_CMD_FORMAT_UNIT        0x04
#define SCSI_CMD_SEARCH_DATA_EQUAL  0x31
#define SCSI_CMD_READ_DEFECT        0x37


#define IS_COMMAND(cmd)             (command[0]==cmd)

#define FORMAT_UNIT_TIMEOUT         5

/*
    LOGLEVEL
        0   no logging,
        1   just commands
        2   1 + data
        3   2 + line changes
*/

#define LOGLEVEL            0

#define LOG(level,...)      if(LOGLEVEL>=level) logerror(__VA_ARGS__)

void scsihle_device::scsi_out_req_delay(UINT8 state)
{
	req_timer->adjust(attotime::from_nsec(REQ_DELAY_NS),state);
}

void scsihle_device::dump_bytes(UINT8 *buff, int count)
{
	int byteno;

	for(byteno=0; byteno<count; byteno++)
	{
		logerror("%02X ",buff[byteno]);
	}
}

void scsihle_device::dump_command_bytes()
{
	logerror("sending command 0x%02X to ScsiID %d\n",command[0],scsiID);
	dump_bytes(command,cmd_idx);
	logerror("\n\n");
}

void scsihle_device::dump_data_bytes(int count)
{
	logerror("Data buffer[0..%d]\n",count);
	dump_bytes(buffer,count);
	logerror("\n\n");
}

void scsihle_device::scsibus_read_data()
{
	data_last = (bytes_left >= m_sector_bytes) ? m_sector_bytes : bytes_left;

	LOG(2,"SCSIBUS:scsibus_read_data bytes_left=%04X, data_last=%04X\n",bytes_left,data_last);

	data_idx=0;

	if (data_last > 0)
	{
		ReadData(buffer, data_last);
		bytes_left-=data_last;

		scsi_out( buffer[ data_idx++ ], SCSI_MASK_DATA );
	}
}

void scsihle_device::scsibus_write_data()
{
	if (data_last > 0)
	{
		WriteData(buffer, data_last);
		bytes_left-=data_last;
	}

	data_idx=0;
}

void scsihle_device::device_timer(emu_timer &timer, device_timer_id tid, int param, void *ptr)
{
	switch( tid )
	{
	case 0:
		scsi_out(param ? SCSI_MASK_REQ : 0, SCSI_MASK_REQ);
		break;

	case 1:
		scsi_out(param ? SCSI_MASK_BSY : 0, SCSI_MASK_BSY);
		break;

	case 2:
		// Some drives, notably the ST225N and ST125N, accept fromat unit commands
		// with flags set indicating that bad block data should be transfered but
		// don't then implemnt a data in phase, this timeout it to catch these !
		if(IS_COMMAND(SCSI_CMD_FORMAT_UNIT) && (data_idx==0))
		{
			scsi_change_phase(SCSI_PHASE_STATUS);
	}
		break;
}
}

void scsihle_device::scsibus_exec_command()
{
	int command_local = 0;

	if(LOGLEVEL)
		dump_command_bytes();

	//is_linked=command[cmd_idx-1] & 0x01;
	is_linked=0;

	// Check for locally executed commands, and if found execute them
	switch (command[0])
	{
		// Format unit
		case SCSI_CMD_FORMAT_UNIT:
			LOG(1,"SCSIBUS: format unit command[1]=%02X & 0x10\n",(command[1] & 0x10));
			command_local=1;
			if((command[1] & 0x10)==0x10)
				m_phase = SCSI_PHASE_DATAOUT;
			else
				m_phase = SCSI_PHASE_STATUS;

			bytes_left=4;
			dataout_timer->adjust(attotime::from_seconds(FORMAT_UNIT_TIMEOUT));
			break;

		case SCSI_CMD_SEARCH_DATA_EQUAL:
			LOG(1,"SCSIBUS: Search_data_equaln");
			command_local=1;
			bytes_left=0;
			m_phase = SCSI_PHASE_STATUS;
			break;

		case SCSI_CMD_READ_DEFECT:
			LOG(1,"SCSIBUS: read defect list\n");
			command_local=1;

			buffer[0] = 0x00;
			buffer[1] = command[2];
			buffer[3] = 0x00; // defect list len msb
			buffer[4] = 0x00; // defect list len lsb

			bytes_left=4;
			m_phase = SCSI_PHASE_DATAIN;
			break;

		// write buffer
		case SCSI_CMD_BUFFER_WRITE:
			LOG(1,"SCSIBUS: write_buffer\n");
			command_local=1;
			bytes_left=(command[7]<<8)+command[8];
			m_phase = SCSI_PHASE_DATAOUT;
			break;

		// read buffer
		case SCSI_CMD_BUFFER_READ:
			LOG(1,"SCSIBUS: read_buffer\n");
			command_local=1;
			bytes_left=(command[7]<<8)+command[8];
			m_phase = SCSI_PHASE_DATAIN;
			break;
	}


	// Check for locally executed command, if not then pass it on
	// to the disk driver
	if(!command_local)
	{
		SetCommand(command, cmd_idx);
		ExecCommand();
		GetLength(&bytes_left);
		data_idx=0;
	}

	scsi_change_phase(m_phase);

	LOG(1,"SCSIBUS:bytes_left=%02X data_idx=%02X\n",bytes_left,data_idx);

	// This is correct as we need to read from disk for commands other than just read data
	if ((m_phase == SCSI_PHASE_DATAIN) && (!command_local))
		scsibus_read_data();
}

UINT8 scsihle_device::scsibus_driveno(UINT8 drivesel)
{
	switch (drivesel)
	{
		case 0x01: return 0;
		case 0x02: return 1;
		case 0x04: return 2;
		case 0x08: return 3;
		case 0x10: return 4;
		case 0x20: return 5;
		case 0x40: return 6;
		case 0x80: return 7;
		default: return 0;
	}
}

void scsihle_device::scsi_change_phase(UINT8 newphase)
{
	LOG(1,"scsi_change_phase() from=%s, to=%s\n",phasenames[m_phase],phasenames[newphase]);

	m_phase=newphase;
	cmd_idx=0;
	data_idx=0;

	switch(m_phase)
	{
		case SCSI_PHASE_BUS_FREE:
			scsi_out( 0, SCSI_MASK_ALL );
			LOG(1,"SCSIBUS: done\n\n");
			break;

		case SCSI_PHASE_COMMAND:
			scsi_out( SCSI_MASK_CD, SCSI_MASK_DATA | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );
			LOG(1,"\nSCSIBUS: Command begin\n");
			scsi_out_req_delay( 1 );
			break;

		case SCSI_PHASE_DATAOUT:
			scsi_out( 0, SCSI_MASK_DATA | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );
			scsi_out_req_delay( 1 );
			break;

		case SCSI_PHASE_DATAIN:
			scsi_out( SCSI_MASK_IO, SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );
			scsi_out_req_delay( 1 );
			break;

		case SCSI_PHASE_STATUS:
			scsi_out( SCSI_STATUS_OK | SCSI_MASK_CD | SCSI_MASK_IO, SCSI_MASK_DATA | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );
			scsi_out_req_delay( 1 );
			break;

		case SCSI_PHASE_MESSAGE_OUT:
			scsi_out( SCSI_MASK_CD | SCSI_MASK_MSG, SCSI_MASK_DATA | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );
			scsi_out_req_delay( 1 );
			break;

		case SCSI_PHASE_MESSAGE_IN:
			scsi_out( 0 | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG, SCSI_MASK_DATA | SCSI_MASK_CD | SCSI_MASK_IO | SCSI_MASK_MSG );// no errors for the time being !
			scsi_out_req_delay( 1 );
			break;
	}
}

void scsihle_device::scsi_in( UINT32 data, UINT32 mask )
{
	// Reset aborts and returns to bus free
	if( ( mask & SCSI_MASK_RST ) != 0 && ( data & SCSI_MASK_RST ) != 0 )
	{
		scsi_change_phase(SCSI_PHASE_BUS_FREE);
		cmd_idx=0;
		data_idx=0;
		is_linked=0;

		return;
	}

	switch (m_phase)
	{
		case SCSI_PHASE_BUS_FREE:
			// Note this assumes we only have one initiator and therefore
			// only one line active.
			if( ( mask & SCSI_MASK_SEL ) != 0 && scsibus_driveno(data & SCSI_MASK_DATA) == scsiID)
			{
				void *hdfile = NULL;
				// Check to see if device had image file mounted, if not, do not set busy,
				// and stay busfree.
				GetDevice(&hdfile);
				if(hdfile!=(void *)NULL)
				{
					if( ( data & SCSI_MASK_SEL ) == 0 )
					{
						scsi_change_phase(SCSI_PHASE_COMMAND);
					}
					else
					{
						sel_timer->adjust(attotime::from_nsec(BSY_DELAY_NS),1);
					}
				}
			}
			break;

		case SCSI_PHASE_COMMAND:
			if( ( mask & SCSI_MASK_ACK ) != 0 )
			{
				if( ( data & SCSI_MASK_ACK ) == 0 )
				{
					command[ cmd_idx++ ] = data & SCSI_MASK_DATA;

					// If the command is ready go and execute it
					if(cmd_idx==get_scsi_cmd_len(command[0]))
					{
						scsibus_exec_command();
					}
					else
					{
						scsi_out_req_delay( 1 );
					}
				}
				else
				{
					scsi_out_req_delay( 0 );
				}
			}
			break;

		case SCSI_PHASE_DATAIN:
			if( ( mask & SCSI_MASK_ACK ) != 0 )
			{
				if( ( data & SCSI_MASK_ACK ) == 0 )
				{
					// check to see if we have reached the end of the block buffer
					// and that there is more data to read from the scsi disk
					if(data_idx == m_sector_bytes)
					{
						scsibus_read_data();
					}

					if(data_idx == data_last && bytes_left == 0)
					{
						scsi_change_phase(SCSI_PHASE_STATUS);
					}
					else
					{
						scsi_out( buffer[ data_idx++ ], SCSI_MASK_DATA );
						scsi_out_req_delay( 1 );
					}
				}
				else
				{
					scsi_out_req_delay( 0 );
				}
			}
			break;

		case SCSI_PHASE_DATAOUT:
			if( ( mask & SCSI_MASK_ACK ) != 0 )
			{
				if( ( data & SCSI_MASK_ACK ) == 0 )
				{
					//LOG(1,"SCSIBUS:bytes_left=%02X data_idx=%02X\n",bytes_left,data_idx);
					buffer[data_idx++]=data & SCSI_MASK_DATA;

					if(IS_COMMAND(SCSI_CMD_FORMAT_UNIT))
					{
						// If we have the first byte, then cancel the dataout timout
						if(data_idx==1)
							dataout_timer->adjust(attotime::never);

						// When we have the first 3 bytes, calculate how many more are in the
						// bad block list.
						if(data_idx==3)
						{
							bytes_left+=((buffer[2]<<8)+buffer[3]);
							LOG(1,"format_unit reading an extra %d bytes\n",bytes_left-4);
							dump_data_bytes(4);
						}
					}

					// If the data buffer is full flush it to the SCSI disk

					data_last = (bytes_left >= m_sector_bytes) ? m_sector_bytes : bytes_left;

					if(data_idx == data_last)
						scsibus_write_data();

					if(data_idx == 0 && bytes_left == 0)
					{
						scsi_change_phase(SCSI_PHASE_STATUS);
					}
					else
					{
						scsi_out_req_delay( 1 );
					}
				}
				else
				{
					scsi_out_req_delay( 0 );
				}
			}
			break;

		case SCSI_PHASE_STATUS:
			if( ( mask & SCSI_MASK_ACK ) != 0 )
			{
				if( ( data & SCSI_MASK_ACK ) == 0 )
				{
					if(cmd_idx > 0)
					{
						scsi_change_phase(SCSI_PHASE_MESSAGE_IN);
					}
					else
					{
						scsi_out_req_delay( 1 );
					}
				}
				else
				{
					cmd_idx++;
					scsi_out_req_delay( 0 );
				}
			}
			break;

		case SCSI_PHASE_MESSAGE_IN:
			if( ( mask & SCSI_MASK_ACK ) != 0 )
			{
				if( ( data & SCSI_MASK_ACK ) == 0 )
				{
					if(cmd_idx > 0)
					{
						if(is_linked)
							scsi_change_phase(SCSI_PHASE_COMMAND);
						else
							scsi_change_phase(SCSI_PHASE_BUS_FREE);
					}
					else
					{
						scsi_out_req_delay( 1 );
					}
				}
				else
				{
					cmd_idx++;
					scsi_out_req_delay( 0 );
				}
			}
			break;
	}
}

// get the length of a SCSI command based on it's command byte type
int scsihle_device::get_scsi_cmd_len(int cbyte)
{
	int group;

	group = (cbyte>>5) & 7;

	if (group == 0 || group == 3 || group == 6 || group == 7) return 6;
	if (group == 1 || group == 2) return 10;
	if (group == 5) return 12;

	fatalerror("scsihle: Unknown SCSI command group %d, command byte=%02X\n", group,cbyte);

	return 6;
}
