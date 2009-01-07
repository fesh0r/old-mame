/*
  defines centronics/parallel port printer interface

  provides a centronics printer simulation (sends output to IO_PRINTER)
*/

#include "driver.h"
#include "centroni.h"
#include "devices/printer.h"

typedef struct {
	const CENTRONICS_CONFIG *config;
	UINT8 data;
	UINT8 control;
	double time_;
	emu_timer *timer;

	/* These bytes are used in the timer callback. When the timer callback
	is executed, the control value is updated with the new data. The mask
	defines the bits that are changing, and the data is the new data */
	/* The user defined callback in CENTRONICS_CONFIG is called with the
	change. This allows systems that have interrupts triggered from the centronics
	interface to function correctly. e.g. Amstrad NC100 */
	/* mask of data to set in timer callback */
	UINT8 new_control_mask;
	/* data to set in timer callback */
	UINT8 new_control_data;
} CENTRONICS;

static CENTRONICS cent[3]={
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 },
	{ 0,0,0, 0.0 }
};

static TIMER_CALLBACK(centronics_timer_callback);

void centronics_config(running_machine *machine,int nr, const CENTRONICS_CONFIG *config)
{
	CENTRONICS *This=cent+nr;
	This->config=config;
	This->timer = timer_alloc(machine, centronics_timer_callback, NULL);
}

void centronics_write_data(running_machine *machine,int nr, UINT8 data)
{
	CENTRONICS *This=cent+nr;
	This->data=data;
}

UINT8 centronics_read_data(running_machine *machine,int nr)
{
	CENTRONICS *This=cent+nr;
	return This->data;
}

/* execute user callback in CENTRONICS_CONFIG when state of control from printer
has changed */
static TIMER_CALLBACK(centronics_timer_callback)
{
	int nr = param;
	CENTRONICS *This = cent + nr;

	/* update control state */
	This->control &=~This->new_control_mask;
	This->control |=This->new_control_data;

	/* if callback is specified, call it with the new state of the outputs from the printer */
	if (This->config->handshake_out)
		This->config->handshake_out(machine, nr, This->control, This->new_control_mask);

	/* phase 2: schedule ack end */
	if ( This->control & CENTRONICS_ACKNOWLEDGE )
	{
		This->new_control_mask = CENTRONICS_ACKNOWLEDGE;
		This->new_control_data = 0;
		timer_adjust_oneshot(This->timer, ATTOTIME_IN_USEC(2), nr);
	}
	/* phase 3: end */
	else if ( This->control & CENTRONICS_NOT_BUSY )
	{
		timer_adjust_oneshot(This->timer, attotime_never, nr);
	}
	/* phase 1: schedule not busy & ack */
	else
	{
		This->new_control_mask = CENTRONICS_NOT_BUSY | CENTRONICS_ACKNOWLEDGE;
		This->new_control_data = CENTRONICS_NOT_BUSY | CENTRONICS_ACKNOWLEDGE;
		timer_adjust_oneshot(This->timer, ATTOTIME_IN_USEC(15), nr);
	}
}

static const device_config *printer_device(running_machine *machine, int index)
{
	const char *tag;
	switch(index)
	{
		case 0:	
			tag = "printer";
			break;
		case 1:	
			tag = "printer1";
			break;
		case 2:	
			tag = "printer2";
			break;
		case 3:	
			tag = "printer3";
			break;
		default:
			fatalerror("\nInvalid centronics device: %X",index);
			break;
	}
	return device_list_find_by_tag(machine->config->devicelist, PRINTER, tag);
}

void centronics_write_handshake(running_machine *machine,int nr, int data, int mask)
{
	CENTRONICS *This=cent+nr;
	const device_config *device;

	int neu=(data&mask)|(This->control&(~mask));

	if (neu & CENTRONICS_NO_RESET)
	{
	  /* strobe down */
	  if ( (This->control&CENTRONICS_STROBE) && !(neu&CENTRONICS_STROBE) )
	  {
			/* schedule busy */
			This->new_control_mask = CENTRONICS_NOT_BUSY;
			This->new_control_data = 0;
			timer_adjust_oneshot(This->timer, ATTOTIME_IN_USEC(5), nr);

			/* output */
			device = printer_device(machine, nr);
			if (device != NULL)
				printer_output(device, This->data);
		}

	}
	This->control=neu;
}

int centronics_read_handshake(running_machine *machine,int nr)
{
	CENTRONICS *This=cent+nr;
	UINT8 data=0;
	const device_config *device;

	/* state of busy */
	data |= (This->control & CENTRONICS_NOT_BUSY);

	if (This->config->type == PRINTER_IBM)
		data |= CENTRONICS_ONLINE;
	else
	{
		if (This->control & CENTRONICS_SELECT)
			data |= CENTRONICS_ONLINE;
	}

	data |= CENTRONICS_NO_ERROR;

	device = printer_device(machine, nr);
	if ((device == NULL) || !printer_is_ready(device))
		data |= CENTRONICS_NO_PAPER;

	/* state of acknowledge */
	data |= (This->control & CENTRONICS_ACKNOWLEDGE);

	return data;
}

const CENTRONICS_DEVICE CENTRONICS_PRINTER_DEVICE= {
	NULL,
	centronics_write_data,
	centronics_read_handshake,
	centronics_write_handshake
};

