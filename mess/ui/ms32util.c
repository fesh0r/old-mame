#include "driver.h"
#include "device.h"
#include "ms32util.h"

BOOL DriverIsComputer(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER) != 0;
}

BOOL DriverIsModified(int driver_index)
{
	return (drivers[driver_index]->flags & GAME_COMPUTER_MODIFIED) != 0;
}

BOOL DriverUsesMouse(int driver_index)
{
	const input_port_entry *input_ports;
	BOOL retval = FALSE;

	if (drivers[driver_index]->construct_ipt == NULL)
		return FALSE;
		
	begin_resource_tracking();
	input_ports = input_port_allocate(drivers[driver_index]->construct_ipt);

	while (1)
	{
		UINT32 type;

		type = input_ports->type;

		if (type == IPT_END)
			break;

		if (type == IPT_MOUSE_X || type == IPT_MOUSE_Y)
			{
				retval = TRUE;
				break;
			}

		input_ports++;
	}

	end_resource_tracking();

	return retval;
}

BOOL DriverHasDevice(const game_driver *gamedrv, iodevice_t type)
{
	BOOL b;
	const struct IODevice *devices;

	begin_resource_tracking();
	devices = devices_allocate(gamedrv);
	b = device_find(devices, IO_PRINTER) ? TRUE : FALSE;
	end_resource_tracking();
	return b;
}


