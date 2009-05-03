/******************************************************************************
 Synertek Systems Corp. SYM-1

 Early driver by PeT mess@utanet.at May 2000
 Rewritten by Dirk Best October 2007

******************************************************************************/


#include "driver.h"
#include "includes/sym1.h"
#include "includes/cbm.h"

/* M6502 CPU */
#include "cpu/m6502/m6502.h"

/* Peripheral chips */
#include "machine/6522via.h"
#include "machine/6532riot.h"
#include "machine/74145.h"
#include "sound/speaker.h"

#define LED_REFRESH_DELAY  ATTOTIME_IN_USEC(70)


static UINT8 riot_port_a, riot_port_b;
static emu_timer *led_update;



/******************************************************************************
 6532 RIOT
******************************************************************************/


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_0_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 0);
}


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_1_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 1);
}


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_2_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 2);
}


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_3_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 3);
}


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_4_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 4);
}


static WRITE_LINE_DEVICE_HANDLER( sym1_74145_output_5_w )
{
	if (state) timer_adjust_oneshot(led_update, LED_REFRESH_DELAY, 5);
}


static TIMER_CALLBACK( led_refresh )
{
	output_set_digit_value(param, riot_port_a);
}


static UINT8 sym1_riot_a_r(const device_config *device, UINT8 olddata)
{
	int data = 0x7f;

	/* scan keypad rows */
	if (!(riot_port_a & 0x80)) data &= input_port_read(device->machine, "ROW-0");
	if (!(riot_port_b & 0x01)) data &= input_port_read(device->machine, "ROW-1");
	if (!(riot_port_b & 0x02)) data &= input_port_read(device->machine, "ROW-2");
	if (!(riot_port_b & 0x04)) data &= input_port_read(device->machine, "ROW-3");

	/* determine column */
	if ( ((riot_port_a ^ 0xff) & (input_port_read(device->machine, "ROW-0") ^ 0xff)) & 0x7f )
		data &= ~0x80;

	return data;
}


static UINT8 sym1_riot_b_r(const device_config *device, UINT8 olddata)
{
	int data = 0xff;

	/* determine column */
	if ( ((riot_port_a ^ 0xff) & (input_port_read(device->machine, "ROW-1") ^ 0xff)) & 0x7f )
		data &= ~0x01;

	if ( ((riot_port_a ^ 0xff) & (input_port_read(device->machine, "ROW-2") ^ 0xff)) & 0x3f )
		data &= ~0x02;

	if ( ((riot_port_a ^ 0xff) & (input_port_read(device->machine, "ROW-3") ^ 0xff)) & 0x1f )
		data &= ~0x04;

	data &= ~0x80; // else hangs 8b02

	return data;
}


static void sym1_riot_a_w(const device_config *device, UINT8 newdata, UINT8 data)
{
	logerror("%x: riot_a_w 0x%02x\n", cpu_get_pc( cputag_get_cpu(device->machine, "maincpu") ), data);

	/* save for later use */
	riot_port_a = data;
}


static void sym1_riot_b_w(const device_config *device, UINT8 newdata, UINT8 data)
{
	logerror("%x: riot_b_w 0x%02x\n", cpu_get_pc( cputag_get_cpu(device->machine, "maincpu") ), data);

	/* save for later use */
	riot_port_b = data;

	/* first 4 pins are connected to the 74145 */
	ttl74145_w(devtag_get_device(device->machine, "ttl74145"), 0, data & 0x0f);
}


const riot6532_interface sym1_r6532_interface =
{
	sym1_riot_a_r,
	sym1_riot_b_r,
	sym1_riot_a_w,
	sym1_riot_b_w
};


const ttl74145_interface sym1_ttl74145_intf =
{
	DEVCB_LINE(sym1_74145_output_0_w),  /* connected to DS0 */
	DEVCB_LINE(sym1_74145_output_1_w),  /* connected to DS1 */
	DEVCB_LINE(sym1_74145_output_2_w),  /* connected to DS2 */
	DEVCB_LINE(sym1_74145_output_3_w),  /* connected to DS3 */
	DEVCB_LINE(sym1_74145_output_4_w),  /* connected to DS4 */
	DEVCB_LINE(sym1_74145_output_5_w),  /* connected to DS5 */
	DEVCB_DEVICE_LINE("speaker", speaker_level_w),
	DEVCB_NULL,	/* not connected */
	DEVCB_NULL,	/* not connected */
	DEVCB_NULL	/* not connected */
};


/******************************************************************************
 6522 VIA
******************************************************************************/


static void sym1_irq(const device_config *device, int level)
{
	cputag_set_input_line(device->machine, "maincpu", M6502_IRQ_LINE, level);
}


static READ8_DEVICE_HANDLER( sym1_via0_b_r )
{
	return 0xff;
}


static WRITE8_DEVICE_HANDLER( sym1_via0_b_w )
{
	logerror("%s: via0_b_w 0x%02x\n", cpuexec_describe_context(device->machine), data);
}


/* PA0: Write protect R6532 RAM
 * PA1: Write protect RAM 0x400-0x7ff
 * PA2: Write protect RAM 0x800-0xbff
 * PA3: Write protect RAM 0xc00-0xfff
 */
static WRITE8_DEVICE_HANDLER( sym1_via2_a_w )
{
	const address_space *cpu0space = cputag_get_address_space( device->machine, "maincpu", ADDRESS_SPACE_PROGRAM );

	logerror("SYM1 VIA2 W 0x%02x\n", data);

	memory_install_write8_handler(cpu0space, 0xa600, 0xa67f, 0, 0,
		((input_port_read(device->machine, "WP") & 0x01) && !(data & 0x01)) ? SMH_NOP : SMH_BANK5);

	memory_install_write8_handler(cpu0space, 0x0400, 0x07ff, 0, 0,
		((input_port_read(device->machine, "WP") & 0x02) && !(data & 0x02)) ? SMH_NOP : SMH_BANK2);

	memory_install_write8_handler(cpu0space, 0x0800, 0x0bff, 0, 0,
		((input_port_read(device->machine, "WP") & 0x04) && !(data & 0x04)) ? SMH_NOP : SMH_BANK3);

	memory_install_write8_handler(cpu0space, 0x0c00, 0x0fff, 0, 0,
		((input_port_read(device->machine, "WP") & 0x08) && !(data & 0x08)) ? SMH_NOP : SMH_BANK4);
}


const via6522_interface sym1_via0 =
{
	DEVCB_NULL,           /* VIA Port A Input */
	DEVCB_HANDLER(sym1_via0_b_r),  /* VIA Port B Input */
	DEVCB_NULL,           /* VIA Port CA1 Input */
	DEVCB_NULL,           /* VIA Port CB1 Input */
	DEVCB_NULL,           /* VIA Port CA2 Input */
	DEVCB_NULL,           /* VIA Port CB2 Input */
	DEVCB_NULL,           /* VIA Port A Output */
	DEVCB_HANDLER(sym1_via0_b_w),  /* VIA Port B Output */
	DEVCB_NULL,           /* VIA Port CA1 Output */
	DEVCB_NULL,           /* VIA Port CB1 Output */
	DEVCB_NULL,           /* VIA Port CA2 Output */
	DEVCB_NULL,           /* VIA Port CB2 Output */
	DEVCB_LINE(sym1_irq)        /* VIA IRQ Callback */
};


const via6522_interface sym1_via1 =
{
	DEVCB_NULL,           /* VIA Port A Input */
	DEVCB_NULL,           /* VIA Port B Input */
	DEVCB_NULL,           /* VIA Port CA1 Input */
	DEVCB_NULL,           /* VIA Port CB1 Input */
	DEVCB_NULL,           /* VIA Port CA2 Input */
	DEVCB_NULL,           /* VIA Port CB2 Input */
	DEVCB_NULL,           /* VIA Port A Output */
	DEVCB_NULL,           /* VIA Port B Output */
	DEVCB_NULL,           /* VIA Port CA1 Output */
	DEVCB_NULL,           /* VIA Port CB1 Output */
	DEVCB_NULL,           /* VIA Port CA2 Output */
	DEVCB_NULL,           /* VIA Port CB2 Output */
	DEVCB_LINE(sym1_irq)        /* VIA IRQ Callback */
};


const via6522_interface sym1_via2 =
{
	DEVCB_NULL,           /* VIA Port A Input */
	DEVCB_NULL,           /* VIA Port B Input */
	DEVCB_NULL,           /* VIA Port CA1 Input */
	DEVCB_NULL,           /* VIA Port CB1 Input */
	DEVCB_NULL,           /* VIA Port CA2 Input */
	DEVCB_NULL,           /* VIA Port CB2 Input */
	DEVCB_HANDLER(sym1_via2_a_w),  /* VIA Port A Output */
	DEVCB_NULL,           /* VIA Port B Output */
	DEVCB_NULL,           /* VIA Port CA1 Output */
	DEVCB_NULL,           /* VIA Port CB1 Output */
	DEVCB_NULL,           /* VIA Port CA2 Output */
	DEVCB_NULL,           /* VIA Port CB2 Output */
	DEVCB_LINE(sym1_irq)        /* VIA IRQ Callback */
};



/******************************************************************************
 Driver init and reset
******************************************************************************/


DRIVER_INIT( sym1 )
{
	/* wipe expansion memory banks that are not installed */
	if (mess_ram_size < 4*1024)
	{
		memory_install_readwrite8_handler(cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM ),
			mess_ram_size, 0x0fff, 0, 0, SMH_NOP, SMH_NOP);
	}

	/* allocate a timer to refresh the led display */
	led_update = timer_alloc(machine, led_refresh, NULL);
}


MACHINE_RESET( sym1 )
{
	/* make 0xf800 to 0xffff point to the last half of the monitor ROM
	   so that the CPU can find its reset vectors */
	memory_install_readwrite8_handler(cputag_get_address_space( machine, "maincpu", ADDRESS_SPACE_PROGRAM ),
			0xf800, 0xffff, 0, 0, SMH_BANK1, SMH_NOP);
	memory_set_bankptr(machine, 1, sym1_monitor + 0x800);
	device_reset(cputag_get_cpu(machine, "maincpu"));
}
