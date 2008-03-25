/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

	"busnop" emulation by R. Belmont, May 2006.

	"busnop" is a trick by which all reads from 0x0000-0x7FFF are held at 0x00
	until A15 is asserted.  The Z80 NOP instruction is 0x00, so the effect is
	to cause reset to skip all memory up to 0x8000, where the actual handling
	code resides.

****************************************************************************/

#include "driver.h"
#include "deprecat.h"
#include "machine/z80ctc.h"
#include "machine/z80pio.h"
#include "machine/z80sio.h"
#include "machine/wd17xx.h"
#include "includes/mbee.h"
#include "devices/cassette.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"


static UINT8 fdc_drv = 0;
static UINT8 fdc_head = 0;
static UINT8 fdc_den = 0;
static UINT8 fdc_status = 0;
static void pio_interrupt(int state);
static void mbee_fdc_callback(running_machine *machine, wd17xx_state_t event, void *param);

UINT8 *mbee_workram;

static const z80pio_interface pio_intf =
{
	pio_interrupt,	/* callback when change interrupt status */
	NULL,
	NULL,
	NULL,
	NULL,
	0,				/* portA ready active callback (do not support yet)*/
	0				/* portB ready active callback (do not support yet)*/
};

static void pio_interrupt(int state)
{
	cpunum_set_input_line(Machine, 0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
  On reset or power on, a circuit forces rom 8000-8FFF to appear at 0000-0FFF, while ram is disabled.
  It gets set back to normal on the first attempt to write to memory. (/WR line goes active).
*/


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( mbee_reset )
{
	memory_set_bank(1, 0);
}

MACHINE_RESET( mbee )
{
	z80pio_init(0, &pio_intf);
	timer_set(ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
	memory_set_bank(1, 1);
}

MACHINE_START( mbee )
{
	wd17xx_init(machine, WD_TYPE_179X,mbee_fdc_callback, NULL);
}

static mess_image *cassette_device_image(void)
{
	return image_from_devtype_and_index(IO_CASSETTE, 0);
}

/* PIO B data bits
 * 0	cassette data (input)
 * 1	cassette data (output)
 * 2	rs232 clock or DTR line
 * 3	rs232 CTS line (0: clear to send)
 * 4	rs232 input (0: mark)
 * 5	rs232 output (1: mark)
 * 6	speaker
 * 7	network interrupt
 */
READ8_HANDLER ( mbee_pio_r )
{
	UINT8 data=0;
	if (offset == 0) return z80pio_d_r(0,0);
	if (offset == 1) return z80pio_c_r(0,0);
	if (offset == 3) return z80pio_c_r(0,1);
	data = z80pio_d_r(0,1) | 1;
	if (cassette_input(cassette_device_image()) > 0.03)
		data &= ~1;
	return data;
}

WRITE8_HANDLER ( mbee_pio_w )
{
	if (offset == 0) z80pio_d_w(0,0,data);
	if (offset == 1) z80pio_c_w(0,0,data);
	if (offset == 3) z80pio_c_w(0,1,data);

	if( offset == 2 )
	{
		z80pio_d_w(0,1,data);
		data = z80pio_p_r(0,1);
		cassette_output(cassette_device_image(), (data & 0x02) ? -1.0 : +1.0);
		speaker_level_w(0, (data & 0x40) ? 1 : 0);
	}
}

static void mbee_fdc_callback(running_machine *machine, wd17xx_state_t state, void *param)
{
	switch( state )
	{
	case WD17XX_IRQ_CLR:
//		cpunum_set_input_line(machine, 0,0,CLEAR_LINE);
		fdc_status &= ~0x40;
        break;
	case WD17XX_IRQ_SET:
//		cpunum_set_input_line(machine, 0,0,HOLD_LINE);
		fdc_status |= 0x40;
        break;
	case WD17XX_DRQ_CLR:
		fdc_status &= ~0x80;
		break;
	case WD17XX_DRQ_SET:
		fdc_status |= 0x80;
        break;
    }
}

 READ8_HANDLER ( mbee_fdc_status_r )
{
	logerror("mbee fdc_motor_r $%02X\n", fdc_status);
	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
	logerror("mbee fdc_motor_w $%02X\n", data);
	/* Controller latch bits
	 * 0-1	driver select
	 * 2	head select
	 * 3	density (0: FM, 1:MFM)
	 * 4-7	unused
	 */
	fdc_drv = data & 3;
	fdc_head = (data >> 2) & 1;
	fdc_den = (data >> 3) & 1;
	wd17xx_set_drive(fdc_drv);
	wd17xx_set_side(fdc_head);
	if (data & (1<<3))
	{
	   wd17xx_set_density(DEN_FM_HI);
	}
	else
	{
	   wd17xx_set_density(DEN_MFM_LO);
	}

}

INTERRUPT_GEN( mbee_interrupt )
{
	/* once per frame, pulse the PIO B bit 7 */
	z80pio_p_w(0, 1, 0x80);
	z80pio_p_w(0, 1, 0x00);
}

DEVICE_LOAD( mbee_cart )
{
	int size = image_length(image);
	UINT8 *mem = malloc(size);
	if (!mem)
		return INIT_FAIL;

	{
		if( image_fread(image, mem, size) == size )
		{
			memcpy(memory_region(REGION_CPU1)+0x8000, mem, size);
		}
		free(mem);
	}
	return INIT_PASS;
}
