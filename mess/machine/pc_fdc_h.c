/* PC floppy interface has been extracted so it can be used in the super i/o chip */

/* TODO:
	- check how the drive select from DOR register, and the drive select
	from the fdc are related !!!! 
	- if all drives do not have a disk in them, and the fdc is reset, is a int generated?
	(if yes, indicates drives are ready without discs, if no indicates no drives are ready)
	- status register a, status register b
*/

#include "includes/pc_fdc_h.h"
#include "includes/nec765.h"

static pc_fdc fdc;

static void pc_fdc_hw_interrupt(int state);
static void pc_fdc_hw_dma_drq(int,int);

static nec765_interface pc_fdc_nec765_interface = 
{
	pc_fdc_hw_interrupt,
	pc_fdc_hw_dma_drq
};

void pc_fdc_reset(void)
{
	/* setup reset condition */
	fdc.data_rate_register = 2;
	fdc.digital_output_register = 0;

	/* bit 7 is disk change */
	fdc.digital_input_register = 0x07f;

	nec765_reset(0);

	/* set FDC at reset */
	nec765_set_reset_state(1);
}

void	pc_fdc_init(pc_fdc_hw_interface *iface)
{
	/* copy specified interface */
	memcpy(&fdc.fdc_interface, iface, sizeof(pc_fdc_hw_interface));

	/* setup nec765 interface */
	nec765_init(&pc_fdc_nec765_interface, NEC765A);

	pc_fdc_reset();

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);
	floppy_drive_set_geometry(1, FLOPPY_DRIVE_DS_80);
	floppy_drive_set_geometry(2, FLOPPY_DRIVE_DS_80);
	floppy_drive_set_geometry(3, FLOPPY_DRIVE_DS_80);
}

void	pc_fdc_exit(void)
{
	nec765_stop();
}


void	pc_fdc_set_tc_state(int state)
{
	/* store state */
	fdc.tc_state = state;

	/* if dma is not enabled, tc's are not acknowledged */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		nec765_set_tc_state(state);
	}
}


void	pc_fdc_hw_interrupt(int state)
{
	fdc.int_state = state;

	/* if dma is not enabled, irq's are masked */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)==0)
		return;

	/* send irq */
	if (fdc.fdc_interface.pc_fdc_interrupt!=NULL)
		fdc.fdc_interface.pc_fdc_interrupt(state);
}

int	pc_fdc_dack_r(void)
{
	int data;

	/* what is output if dack is not acknowledged? */
	data = 0x0ff;

	/* if dma is not enabled, dacks are not acknowledged */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		data = nec765_dack_r(0);
	}

	return data;
}

void	pc_fdc_dack_w(int data)
{
	/* if dma is not enabled, dacks are not issued */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)!=0)
	{
		/* dma acknowledge - and send byte to fdc */
		nec765_dack_w(0,data);
	}
}

void	pc_fdc_hw_dma_drq(int state, int read)
{
	fdc.dma_state = state;
	fdc.dma_read = read;

	/* if dma is not enabled, drqs are masked */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_DMA_ENABLED)==0)
		return;

	if (fdc.fdc_interface.pc_fdc_dma_drq!=NULL)
		fdc.fdc_interface.pc_fdc_dma_drq(state, read);
}

/* read status register */
static READ_HANDLER(pc_fdc_status_r)
{
	return nec765_status_r(0);
}

static WRITE_HANDLER(pc_fdc_data_rate_w)
{
	if ((data & 0x080)!=0)
	{
		/* set ready state */
		nec765_set_ready_state(1);

		/* toggle reset state */
		nec765_set_reset_state(1);
 		nec765_set_reset_state(0);
	
		/* bit is self-clearing */
		data &= ~0x080;
	}

	fdc.data_rate_register = data;
}

static READ_HANDLER(pc_fdc_data_r)
{
	return nec765_data_r(offset);
}


static WRITE_HANDLER(pc_fdc_data_w)
{
	nec765_data_w(0, data);
}

READ_HANDLER(pc_fdc_dir_r)
{
	return fdc.digital_input_register;
}

WRITE_HANDLER(pc_fdc_dor_w)
{

        int selected_drive;

	logerror("FDC DOR: %02x\r\n",data);

        floppy_drive_set_ready_state(fdc.digital_output_register & 0x03, 1, 0);

        fdc.digital_output_register = data;

        selected_drive = data & 0x03;

	/* set floppy drive motor state */
        floppy_drive_set_motor_state(0,(data>>4) & 0x0f);
        floppy_drive_set_motor_state(1,(data>>5) & 0x01);
        floppy_drive_set_motor_state(2,(data>>6) & 0x01);
        floppy_drive_set_motor_state(3,(data>>7) & 0x01);

        if ((data>>4) & (1<<selected_drive))
        {
           floppy_drive_set_ready_state(selected_drive, 1, 0);
        }

	/* changing the DMA enable bit, will affect the terminal count state
	from reaching the fdc - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_set_tc_state(fdc.tc_state);

	/* changing the DMA enable bit, will affect the dma drq state
	from reaching us - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_hw_dma_drq(fdc.dma_state, fdc.dma_read);

	/* changing the DMA enable bit, will affect the irq state
	from reaching us - if dma is enabled this will send it through
	otherwise it will be ignored */
	pc_fdc_hw_interrupt(fdc.int_state);

	/* reset? */
	if ((fdc.digital_output_register & PC_FDC_FLAGS_DOR_FDC_ENABLED)==0)
	{
		/* yes */

			/* pc-xt expects a interrupt to be generated
			when the fdc is reset.
			In the FDC docs, it states that a INT will
			be generated if READY input is true when the
			fdc is reset. 
			
			 It also states, that outputs to drive are set to 0.
			 Maybe this causes the drive motor to go on, and therefore
			 the ready line is set. 

			This in return causes a int?? ---
		
		
		what is not yet clear is if this is a result of the drives ready state
		changing...
		*/
			nec765_set_ready_state(1);

		/* set FDC at reset */
		nec765_set_reset_state(1);
    }
	else
	{
		pc_fdc_set_tc_state(0);

		/* release reset on fdc */
		nec765_set_reset_state(0);
	}
}

READ_HANDLER(pc_fdc_dor_r)
{
	return fdc.digital_output_register;
}

WRITE_HANDLER ( pc_fdc_w )
{
//	logerror("fdc write %.2x %.2x\n",offset,data);
	switch( offset )
	{
		case 0: /* n/a */				   break;
		case 1: /* n/a */				   break;
		case 2: pc_fdc_dor_w(offset,data); 	   break;
		case 3: /* tape drive select? */   break;
		case 4: pc_fdc_data_rate_w(offset,data);  break;
		case 5: pc_fdc_data_w(offset,data);    break;
		case 6: /* fdc reserved */		   break;
		case 7: /* n/a */ break;
	}
}

READ_HANDLER ( pc_fdc_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: data = 0; /* n/a */				   break;	/* status register a */
		case 1: data = 0;/* n/a */				   break;	/* status register b */
		case 2: data = pc_fdc_dor_r(offset);				   break;
		case 3: /* tape drive select? */   break;
		case 4: data = pc_fdc_status_r(offset);  break;
		case 5: data = pc_fdc_data_r(offset);    break;
		case 6: /* FDC reserved */		   break;
		case 7: data = pc_fdc_dir_r(offset);	   break;
    }
//	logerror("fdc read %.2x %.2x\n",offset,data);
	return data;
}

