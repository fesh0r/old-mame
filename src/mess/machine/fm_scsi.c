/*
 *
 *  Implementation of Fujitsu FMR-50, FMR-60 and FM-Towns SCSI hardware
 *
 *

 info from Toshiya Takeda's e-FMR50 source (the Towns uses the same controller?)
 0xc30 = Data register
 0xc32 = Status register (read)
         bit 7 = REQ
         bit 6 = I/O
         bit 5 = MSG
         bit 4 = C/D
         bit 3 = BUSY
         bit 1 = INT
         bit 0 = PERR
 0xc32 = Control register (write)
         bit 7 = WEN
         bit 6 = IMSK
         bit 4 = ATN
         bit 2 = SEL
         bit 1 = DMAE
         bit 0 = RST
*/
#include "emu.h"
#include "fm_scsi.h"
#include "machine/devhelpr.h"
#include "debugger.h"

/*
 *  Device config
 */

const device_type FMSCSI = fmscsi_device_config::static_alloc_device_config;

GENERIC_DEVICE_CONFIG_SETUP(fmscsi, "FM-SCSI")

void fmscsi_device_config::device_config_complete()
{
    // copy static configuration if present
	const FMSCSIinterface *intf = reinterpret_cast<const FMSCSIinterface *>(static_config());
	if (intf != NULL)
		*static_cast<FMSCSIinterface *>(this) = *intf;

    // otherwise, initialize it to defaults
    else
    {
		scsidevs = NULL;
		memset(&irq_callback,0,sizeof(irq_callback));
		memset(&drq_callback,0,sizeof(drq_callback));
    }
}

/*
 * Device
 */

fmscsi_device::fmscsi_device(running_machine &machine, const fmscsi_device_config &config)
    : device_t(machine, config),
      m_config(config)
{
}

void fmscsi_device::device_start()
{
	int x;

    m_input_lines = 0;
    m_output_lines = 0;
    m_data = 0;
    m_command_index = 0;
    m_last_id = 0;
    m_target = 0;
    m_phase = SCSI_PHASE_BUS_FREE;

    devcb_resolve_write_line(&m_irq_func,&m_config.irq_callback,this);
    devcb_resolve_write_line(&m_drq_func,&m_config.drq_callback,this);

    memset(m_SCSIdevices,0,sizeof(m_SCSIdevices));

    // initialise SCSI devices, if any present
    for(x=0;x<m_config.scsidevs->devs_present;x++)
    {
    	SCSIAllocInstance(machine, m_config.scsidevs->devices[x].scsiClass,
				&m_SCSIdevices[m_config.scsidevs->devices[x].scsiID],
				m_config.scsidevs->devices[x].diskregion );
    }

    // allocate read timer
    m_transfer_timer = device_timer_alloc(*this,TIMER_TRANSFER);
    m_phase_timer = device_timer_alloc(*this,TIMER_PHASE);
}

void fmscsi_device::device_reset()
{
    m_input_lines = 0;
    m_output_lines = 0;
    m_data = 0;
    m_command_index = 0;
    m_last_id = 0;
    m_target = 0;
    m_result_length = 0;
    m_result_index = 0;

    m_phase = SCSI_PHASE_BUS_FREE;
    fmscsi_rescan();
}

void fmscsi_device::device_stop()
{
	int x;

	// clean up the devices
	for (x=0;x<m_config.scsidevs->devs_present;x++)
	{
		SCSIDeleteInstance( m_SCSIdevices[m_config.scsidevs->devices[x].scsiID] );
	}
}

// get the length of a SCSI command based on it's command byte type
int fmscsi_device::get_scsi_cmd_len(UINT8 cbyte)
{
	int group;

	group = (cbyte>>5) & 7;

	if (group == 0) return 6;
	if (group == 1 || group == 2) return 10;
	if (group == 5) return 12;

	fatalerror("fmscsi: Unknown SCSI command group %d", group);

	return 6;
}

void fmscsi_device::fmscsi_rescan()
{
	int i;

	// try to open the devices
	for (i = 0; i < m_config.scsidevs->devs_present; i++)
	{
		// if a device wasn't already allocated
//		if (!m_SCSIdevices[m_config.scsidevs->devices[i].scsiID])
		{
			SCSIDeleteInstance( m_SCSIdevices[m_config.scsidevs->devices[i].scsiID] );
			SCSIAllocInstance( machine,
					m_config.scsidevs->devices[i].scsiClass,
					&m_SCSIdevices[m_config.scsidevs->devices[i].scsiID],
					m_config.scsidevs->devices[i].diskregion );
		}
	}
}

void fmscsi_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch(id)
	{
	case TIMER_TRANSFER:
		set_input_line(FMSCSI_LINE_REQ,1);
		//logerror("FMSCSI: timer triggered: %i/%i\n",m_result_index,m_result_length);
		if(m_output_lines & FMSCSI_LINE_DMAE)
		{
			devcb_call_write_line(&m_drq_func,1);
		}
		break;
	case TIMER_PHASE:
		set_phase(param);
		break;
	}
}

UINT8 fmscsi_device::fmscsi_data_r(void)
{
	// read from data bus
	// ACK is automatic on accessing the data bus, so REQ will go low.
	set_input_line(FMSCSI_LINE_REQ,0);

	if(m_phase == SCSI_PHASE_DATAIN)
	{
		m_data = m_buffer[m_result_index % 512];
		//logerror("FMSCSI: DATAIN phase read data %02x\n",m_data);
		m_result_index++;
		if(m_result_index % 512 == 0)
			SCSIReadData(m_SCSIdevices[m_target],m_buffer,512);  // read next sector

		if(m_result_index >= m_result_length)
		{
			// end of data transfer
			timer_adjust_oneshot(m_transfer_timer,attotime_never,0);  // stop timer
			timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_STATUS);
			if(m_output_lines & FMSCSI_LINE_DMAE)
			{
				devcb_call_write_line(&m_drq_func,0);
			}
			logerror("FMSCSI: Stopping transfer : (%i/%i)\n",m_result_index,m_result_length);
		}
		return m_data;
	}

	if(m_phase == SCSI_PHASE_MESSAGE_IN)
	{
		m_data = 0;  // command complete message
		timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_BUS_FREE);
		m_command_index = 0;
		return m_data;
	}

	if(m_phase == SCSI_PHASE_STATUS)
	{
		m_data = 0;  // GOOD status
		// no command complete message?
		timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_MESSAGE_IN);
		m_command_index = 0;
		//set_input_line(FMSCSI_LINE_REQ,1);  // raise REQ yet again
		return m_data;
	}

	return m_data;
}

void fmscsi_device::fmscsi_data_w(UINT8 data)
{
	int phase;

	// write to data bus
	m_data = data;
	//logerror("FMSCSI: data write %02x\n",data);

	// ACK is automatic on accessing the data bus, so REQ will go low.
	set_input_line(FMSCSI_LINE_REQ,0);

	if(m_phase == SCSI_PHASE_BUS_FREE)
	{
		// select target
		switch(data & 0x7f)
		{
		case 0x01: m_target = 0; break;
		case 0x02: m_target = 1; break;
		case 0x04: m_target = 2; break;
		case 0x08: m_target = 3; break;
		case 0x10: m_target = 4; break;
		}
	}
	if(m_phase == SCSI_PHASE_DATAOUT)
	{
		m_buffer[m_result_index % 512] = m_data;
		m_result_index++;
		if(m_result_index % 512 == 0)
			SCSIWriteData(m_SCSIdevices[m_target],m_buffer,512);  // write buffer to disc
		if(m_result_index >= m_result_length)
		{
			// end of data transfer
			timer_adjust_oneshot(m_transfer_timer,attotime_never,0);  // stop timer
			timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_STATUS);
			if(m_output_lines & FMSCSI_LINE_DMAE)
			{
				devcb_call_write_line(&m_drq_func,0);
			}
			logerror("FMSCSI: Stopping transfer : (%i/%i)\n",m_result_index,m_result_length);
		}
	}
	if(m_phase == SCSI_PHASE_COMMAND)
	{
		m_command[m_command_index] = data;
		logerror("FMSCSI: writing command byte %02x [%i]\n",data,m_command_index);
		m_command_index++;
		if(m_command_index >= get_scsi_cmd_len(m_command[0]))
		{
			// command complete
			SCSISetCommand(m_SCSIdevices[m_target],m_command,m_command_index);
			SCSIExecCommand(m_SCSIdevices[m_target],&m_result_length);
			SCSIGetPhase(m_SCSIdevices[m_target],&phase);
			if(m_command[0] == 1)  // rezero unit command - not implemented in SCSI code
				timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_STATUS);
			else
				timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),phase);

			logerror("FMSCSI: Command %02x sent, result length = %i\n",m_command[0],m_result_length);
		}
		else
		{
			timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_COMMAND);
		}
	}
	if(m_phase == SCSI_PHASE_MESSAGE_OUT)
	{
		timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_STATUS);
	}
}

void fmscsi_device::set_phase(int phase)
{
	m_phase = phase;
	logerror("FMSCSI: phase set to %i\n",m_phase);
	// set input lines accordingly
	switch(phase)
	{
	case SCSI_PHASE_BUS_FREE:
		set_input_line(FMSCSI_LINE_BSY,0);
		set_input_line(FMSCSI_LINE_CD,0);
		set_input_line(FMSCSI_LINE_MSG,0);
		set_input_line(FMSCSI_LINE_IO,0);
		//set_input_line(FMSCSI_LINE_REQ,0);
		break;
	case SCSI_PHASE_COMMAND:
		set_input_line(FMSCSI_LINE_BSY,1);
		set_input_line(FMSCSI_LINE_CD,1);
		set_input_line(FMSCSI_LINE_MSG,0);
		set_input_line(FMSCSI_LINE_IO,0);
		set_input_line(FMSCSI_LINE_REQ,1);
		break;
	case SCSI_PHASE_STATUS:
		set_input_line(FMSCSI_LINE_CD,1);
		set_input_line(FMSCSI_LINE_MSG,0);
		set_input_line(FMSCSI_LINE_IO,1);
		set_input_line(FMSCSI_LINE_REQ,1);
		break;
	case SCSI_PHASE_DATAIN:
		set_input_line(FMSCSI_LINE_CD,0);
		set_input_line(FMSCSI_LINE_MSG,0);
		set_input_line(FMSCSI_LINE_IO,1);
		set_input_line(FMSCSI_LINE_REQ,1);
		// start transfer timer
		timer_adjust_periodic(m_transfer_timer,attotime_zero,0,ATTOTIME_IN_HZ(3000000));  // arbitrary value for now
		SCSIReadData(m_SCSIdevices[m_target],m_buffer,512);
		m_result_index = 0;
		logerror("FMSCSI: Starting transfer (%i)\n",m_result_length);
		break;
	case SCSI_PHASE_DATAOUT:
		set_input_line(FMSCSI_LINE_CD,0);
		set_input_line(FMSCSI_LINE_MSG,0);
		set_input_line(FMSCSI_LINE_IO,0);
		set_input_line(FMSCSI_LINE_REQ,1);
		// start transfer timer
		timer_adjust_periodic(m_transfer_timer,attotime_zero,0,ATTOTIME_IN_HZ(3000000));  // arbitrary value for now
		m_result_index = 0;
		logerror("FMSCSI: Starting transfer (%i)\n",m_result_length);
		break;
	case SCSI_PHASE_MESSAGE_IN:
		set_input_line(FMSCSI_LINE_CD,1);
		set_input_line(FMSCSI_LINE_MSG,1);
		set_input_line(FMSCSI_LINE_IO,1);
		set_input_line(FMSCSI_LINE_REQ,1);
		break;
	case SCSI_PHASE_MESSAGE_OUT:
		set_input_line(FMSCSI_LINE_CD,1);
		set_input_line(FMSCSI_LINE_MSG,1);
		set_input_line(FMSCSI_LINE_IO,0);
		set_input_line(FMSCSI_LINE_REQ,1);
		break;
	}
}

int fmscsi_device::get_phase(void)
{
	return m_phase;
}

void fmscsi_device::set_input_line(UINT8 line, UINT8 state)
{
	if(line == FMSCSI_LINE_REQ)
	{
		if(state != 0 && !(m_input_lines & FMSCSI_LINE_REQ))  // low to high
		{
			if(m_output_lines & FMSCSI_LINE_IMSK && m_phase != SCSI_PHASE_DATAIN && m_phase != SCSI_PHASE_DATAOUT)
			{
				set_input_line(FMSCSI_LINE_INT,1);
				devcb_call_write_line(&m_irq_func,1);
				logerror("FMSCSI: IRQ high\n");
			}
		}
		if(state == 0 && (m_input_lines & FMSCSI_LINE_REQ))  // high to low
		{
			if(m_output_lines & FMSCSI_LINE_IMSK && m_phase != SCSI_PHASE_DATAIN && m_phase != SCSI_PHASE_DATAOUT)
			{
				set_input_line(FMSCSI_LINE_INT,0);
				devcb_call_write_line(&m_irq_func,0);
				logerror("FMSCSI: IRQ low\n");
			}
		}
	}
	if(state != 0)
		m_input_lines |= line;
	else
		m_input_lines &= ~line;
//	logerror("FMSCSI: input line %02x set to %i\n",line,state);
}

UINT8 fmscsi_device::get_input_line(UINT8 line)
{
	return m_input_lines & line;
}

void fmscsi_device::set_output_line(UINT8 line, UINT8 state)
{
	device_image_interface* image;

	if(line == FMSCSI_LINE_RST && state != 0)
	{
		device_reset();
		logerror("FMSCSI: reset\n");
	}

	if(line == FMSCSI_LINE_SEL)
	{
		image = dynamic_cast<device_image_interface*>(machine->device(m_config.scsidevs->devices[m_target].diskregion));
		if(state != 0 && !(m_output_lines & FMSCSI_LINE_SEL)) // low to high transition
		{
			if (image->exists())  // if device is mounted
			{
				timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_COMMAND);
				m_data = 0x08;
			}
		}
	}

	if(line == FMSCSI_LINE_ATN)
	{
		if(state != 0)
			timer_adjust_oneshot(m_phase_timer,ATTOTIME_IN_USEC(800),SCSI_PHASE_MESSAGE_OUT);
	}

	if(state != 0)
		m_output_lines |= line;
	else
		m_output_lines &= ~line;
//	logerror("FMSCSI: output line %02x set to %i\n",line,state);
}

UINT8 fmscsi_device::get_output_line(UINT8 line)
{
	return m_output_lines & line;
}

UINT8 fmscsi_device::fmscsi_status_r(void)
{
	// status inputs
	return m_input_lines;
}

void fmscsi_device::fmscsi_control_w(UINT8 data)
{
	// control outputs
	set_output_line(FMSCSI_LINE_RST,data & FMSCSI_LINE_RST);
	set_output_line(FMSCSI_LINE_DMAE,data & FMSCSI_LINE_DMAE);
	set_output_line(FMSCSI_LINE_IMSK,data & FMSCSI_LINE_IMSK);
	set_output_line(FMSCSI_LINE_ATN,data & FMSCSI_LINE_ATN);
	set_output_line(FMSCSI_LINE_WEN,data & FMSCSI_LINE_WEN);
	set_output_line(FMSCSI_LINE_SEL,data & FMSCSI_LINE_SEL);
	logerror("FMSCSI: control write %02x\n",data);
}

READ8_MEMBER( fmscsi_device::fmscsi_r )
{
	switch(offset & 0x01)
	{
	case 0x00:
		return fmscsi_data_r();
	case 0x01:
		return fmscsi_status_r();
	}
	return 0;
}

WRITE8_MEMBER( fmscsi_device::fmscsi_w )
{
	switch(offset & 0x01)
	{
	case 0x00:
		fmscsi_data_w(data);
		break;
	case 0x01:
		fmscsi_control_w(data);
		break;
	}
}
