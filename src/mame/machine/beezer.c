#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6522via.h"
#include "includes/beezer.h"

static int pbus;

static READ8_DEVICE_HANDLER( b_via_0_pb_r );
static WRITE8_DEVICE_HANDLER( b_via_0_pa_w );
static WRITE8_DEVICE_HANDLER( b_via_0_pb_w );
static READ8_DEVICE_HANDLER( b_via_0_ca2_r );
static WRITE8_DEVICE_HANDLER( b_via_0_ca2_w );
static void b_via_0_irq (const device_config *device, int level);

static READ8_DEVICE_HANDLER( b_via_1_pa_r );
static READ8_DEVICE_HANDLER( b_via_1_pb_r );
static WRITE8_DEVICE_HANDLER( b_via_1_pa_w );
static WRITE8_DEVICE_HANDLER( b_via_1_pb_w );
static void b_via_1_irq (const device_config *device, int level);

static READ8_DEVICE_HANDLER( via_0_cb1_r )
{
	const device_config *via_0 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_0");
	return via_cb1_r(via_0, offset);
}

static WRITE8_DEVICE_HANDLER( via_0_cb1_w )
{
	const device_config *via_0 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_0");
	via_cb1_w(via_0, offset, data);
}

static READ8_DEVICE_HANDLER( via_0_cb2_r )
{
	const device_config *via_0 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_0");
	return via_cb2_r(via_0, offset);
}

static READ8_DEVICE_HANDLER( via_1_ca1_r )
{
	const device_config *via_1 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_1");
	return via_ca1_r(via_1, offset);
}

static WRITE8_DEVICE_HANDLER( via_1_ca1_w )
{
	const device_config *via_1 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_1");
	via_ca1_w(via_1, offset, data);
}

static READ8_DEVICE_HANDLER( via_1_ca2_r )
{
	const device_config *via_1 = device_list_find_by_tag(device->machine->config->devicelist, VIA6522, "via6522_1");
	return via_ca2_r(via_1, offset);
}

const via6522_interface b_via_0_interface =
{
	/*inputs : A/B         */ 0, b_via_0_pb_r,
	/*inputs : CA/B1,CA/B2 */ 0, via_1_ca2_r, b_via_0_ca2_r, via_1_ca1_r,
	/*outputs: A/B         */ b_via_0_pa_w, b_via_0_pb_w,
	/*outputs: CA/B1,CA/B2 */ 0, 0, b_via_0_ca2_w, via_1_ca1_w,
	/*irq                  */ b_via_0_irq
};

const via6522_interface b_via_1_interface =
{
	/*inputs : A/B         */ b_via_1_pa_r, b_via_1_pb_r,
	/*inputs : CA/B1,CA/B2 */ via_0_cb2_r, 0, via_0_cb1_r, 0,
	/*outputs: A/B         */ b_via_1_pa_w, b_via_1_pb_w,
	/*outputs: CA/B1,CA/B2 */ 0, 0, via_0_cb1_w, 0,
	/*irq                  */ b_via_1_irq
};

static READ8_DEVICE_HANDLER( b_via_0_ca2_r )
{
	return 0;
}

static WRITE8_DEVICE_HANDLER( b_via_0_ca2_w )
{
}

static void b_via_0_irq (const device_config *device, int level)
{
	cpu_set_input_line(device->machine->cpu[0], M6809_IRQ_LINE, level);
}

static READ8_DEVICE_HANDLER( b_via_0_pb_r )
{
	return pbus;
}

static WRITE8_DEVICE_HANDLER( b_via_0_pa_w )
{
	if ((data & 0x08) == 0)
		cpu_set_input_line(device->machine->cpu[1], INPUT_LINE_RESET, ASSERT_LINE);
	else
		cpu_set_input_line(device->machine->cpu[1], INPUT_LINE_RESET, CLEAR_LINE);

	if ((data & 0x04) == 0)
	{
		switch (data & 0x03)
		{
		case 0:
			pbus = input_port_read(device->machine, "IN0");
			break;
		case 1:
			pbus = input_port_read(device->machine, "IN1") | (input_port_read(device->machine, "IN2") << 4);
			break;
		case 2:
			pbus = input_port_read(device->machine, "DSWB");
			break;
		case 3:
			pbus = 0xff;
			break;
		}
	}
}

static WRITE8_DEVICE_HANDLER( b_via_0_pb_w )
{
	pbus = data;
}

static void b_via_1_irq (const device_config *device, int level)
{
	cpu_set_input_line(device->machine->cpu[1], M6809_IRQ_LINE, level);
}

static READ8_DEVICE_HANDLER( b_via_1_pa_r )
{
	return pbus;
}

static READ8_DEVICE_HANDLER( b_via_1_pb_r )
{
	return 0xff;
}

static WRITE8_DEVICE_HANDLER( b_via_1_pa_w )
{
	pbus = data;
}

static WRITE8_DEVICE_HANDLER( b_via_1_pb_w )
{
}

DRIVER_INIT( beezer )
{
	pbus = 0;
}

WRITE8_HANDLER( beezer_bankswitch_w )
{
	if ((data & 0x07) == 0)
	{
		const device_config *via_0 = device_list_find_by_tag(space->machine->config->devicelist, VIA6522, "via6522_0");
		memory_install_write8_handler(space, 0xc600, 0xc7ff, 0, 0, watchdog_reset_w);
		memory_install_write8_handler(space, 0xc800, 0xc9ff, 0, 0, beezer_map_w);
		memory_install_read8_handler(space, 0xca00, 0xcbff, 0, 0, beezer_line_r);
		memory_install_readwrite8_device_handler(space, via_0, 0xce00, 0xcfff, 0, 0, via_r, via_w);
	}
	else
	{
		UINT8 *rom = memory_region(space->machine, "main") + 0x10000;
		memory_install_readwrite8_handler(space, 0xc000, 0xcfff, 0, 0, SMH_BANK1, SMH_BANK1);
		memory_set_bankptr(space->machine, 1, rom + (data & 0x07) * 0x2000 + ((data & 0x08) ? 0x1000: 0));
	}
}


