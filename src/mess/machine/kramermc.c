/***************************************************************************

        Kramer MC machine driver by Miodrag Milanovic

        13/09/2008 Preliminary driver.

****************************************************************************/


#include "driver.h"
#include "machine/z80pio.h"
#include "includes/kramermc.h"

static UINT8 kramermc_key_row;

static READ8_DEVICE_HANDLER (kramermc_port_a_r)
{
	return 0xff;
}

static READ8_DEVICE_HANDLER (kramermc_port_b_r)
{
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7" };

	return input_port_read(device->machine, keynames[kramermc_key_row]);
}

static WRITE8_DEVICE_HANDLER (kramermc_port_a_w)
{
	kramermc_key_row = ((data >> 1) & 0x07);
}

const z80pio_interface kramermc_z80pio_intf =
{
	NULL,	/* callback when change interrupt status */
	kramermc_port_a_r,
	kramermc_port_b_r,
	kramermc_port_a_w,
	NULL,
	NULL,
	NULL
};

/* Driver initialization */
DRIVER_INIT(kramermc)
{
}

MACHINE_RESET( kramermc )
{
	kramermc_key_row = 0;
}
