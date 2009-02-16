#pragma once

#ifndef __5220INTF_H__
#define __5220INTF_H__

/* clock rate = 80 * output sample rate,     */
/* usually 640000 for 8000 Hz sample rate or */
/* usually 800000 for 10000 Hz sample rate.  */

typedef struct _tms5220_interface tms5220_interface;
struct _tms5220_interface
{
	void (*irq)(const device_config *device, int state);		/* IRQ callback function */

	int (*read)(const device_config *device, int count);			/* speech ROM read callback */
	void (*load_address)(const device_config *device, int data);	/* speech ROM load address callback */
	void (*read_and_branch)(const device_config *device);		/* speech ROM read and branch callback */
};

WRITE8_DEVICE_HANDLER( tms5220_data_w );
READ8_DEVICE_HANDLER( tms5220_status_r );
int tms5220_ready_r(const device_config *device);
double tms5220_time_to_ready(const device_config *device);
int tms5220_int_r(const device_config *device);

void tms5220_set_frequency(const device_config *device, int frequency);

DEVICE_GET_INFO( tms5220 );
DEVICE_GET_INFO( tmc0285 );
DEVICE_GET_INFO( tms5200 );

#define SOUND_TMS5220 DEVICE_GET_INFO_NAME( tms5220 )
#define SOUND_TMC0285 DEVICE_GET_INFO_NAME( tmc0285 )
#define SOUND_TMS5200 DEVICE_GET_INFO_NAME( tms5200 )

#endif /* __5220INTF_H__ */
