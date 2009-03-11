/***************************************************************************

There are three IRQ sources:
- IRQ0
- IRQ1 = IRQA from the video PIA
- IRQ2 = IRQA from the IEEE488 PIA

Interrupt handling on the Osborne-1 is a bit akward. When an interrupt is
taken by the Z80 the ROMMODE is enabled on each fetch of an instruction
byte. During execution of an instruction the previous ROMMODE setting seems
to be used. Side effect of this is that when an interrupt is taken and the
stack pointer is pointing to 0000-3FFF then the return address will still
be written to RAM if RAM was switched in.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "cpu/z80/z80daisy.h"
#include "sound/beep.h"
#include "includes/osborne1.h"

#define RAMMODE		(0x01)


static struct osborne1
{
	UINT8	bank2_enabled;
	UINT8	bank3_enabled;
	UINT8	*bank4_ptr;
	UINT8	*empty_4K;
	/* IRQ states */
	int		pia_0_irq_state;
	int		pia_1_irq_state;
	/* video related */
	UINT8	new_start_x;
	UINT8	new_start_y;
	emu_timer	*video_timer;
	UINT8	*charrom;
	UINT8	charline;
	UINT8	start_y;
	/* bankswitch setting */
	UINT8	bankswitch;
	UINT8	in_irq_handler;
	/* beep state */
	UINT8	beep;
} osborne1;


WRITE8_HANDLER( osborne1_0000_w )
{
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled || ( osborne1.in_irq_handler && osborne1.bankswitch == RAMMODE ) )
	{
		mess_ram[ offset ] = data;
	}
}


WRITE8_HANDLER( osborne1_1000_w )
{
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled || ( osborne1.in_irq_handler && osborne1.bankswitch == RAMMODE ) )
	{
		mess_ram[ 0x1000 + offset ] = data;
	}
}


READ8_HANDLER( osborne1_2000_r )
{
	UINT8	data = 0xFF;
	const device_config *fdc = devtag_get_device(space->machine, "mb8877");
	const device_config *pia_0 = devtag_get_device(space->machine, "pia_0" );
	const device_config *pia_1 = devtag_get_device(space->machine, "pia_1" );

	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled )
	{
		data = mess_ram[ 0x2000 + offset ];
	}
	else
	{
		switch( offset & 0x0F00 )
		{
		case 0x100:	/* Floppy */
			data = wd17xx_r( fdc, offset );
			break;
		case 0x200:	/* Keyboard */
			/* Row 0 */
			if ( offset & 0x01 )	data &= input_port_read(space->machine, "ROW0");
			/* Row 1 */
			if ( offset & 0x02 )	data &= input_port_read(space->machine, "ROW1");
			/* Row 2 */
			if ( offset & 0x04 )	data &= input_port_read(space->machine, "ROW3");
			/* Row 3 */
			if ( offset & 0x08 )	data &= input_port_read(space->machine, "ROW4");
			/* Row 4 */
			if ( offset & 0x10 )	data &= input_port_read(space->machine, "ROW5");
			/* Row 5 */
			if ( offset & 0x20 )	data &= input_port_read(space->machine, "ROW2");
			/* Row 6 */
			if ( offset & 0x40 )	data &= input_port_read(space->machine, "ROW6");
			/* Row 7 */
			if ( offset & 0x80 )	data &= input_port_read(space->machine, "ROW7");
			break;
		case 0x900:	/* IEEE488 PIA */
			data = pia6821_r( pia_0, offset & 0x03 );
			break;
		case 0xA00:	/* Serial */
			break;
		case 0xC00:	/* Video PIA */
			data = pia6821_r( pia_1, offset & 0x03 );
			break;
		}
	}
	return data;
}


WRITE8_HANDLER( osborne1_2000_w )
{
	const device_config *fdc = devtag_get_device(space->machine, "mb8877");
	const device_config *pia_0 = devtag_get_device(space->machine, "pia_0" );
	const device_config *pia_1 = devtag_get_device(space->machine, "pia_1" );

	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled )
	{
		mess_ram[ 0x2000 + offset ] = data;
	}
	else
	{
		if ( osborne1.in_irq_handler && osborne1.bankswitch == RAMMODE )
		{
			mess_ram[ 0x2000 + offset ] = data;
		}
		/* Handle writes to the I/O area */
		switch( offset & 0x0F00 )
		{
		case 0x100:	/* Floppy */
			wd17xx_w( fdc, offset, data );
			break;
		case 0x900:	/* IEEE488 PIA */
			pia6821_w( pia_0, offset & 0x03, data );
			break;
		case 0xA00:	/* Serial */
			break;
		case 0xC00:	/* Video PIA */
			pia6821_w( pia_1, offset & 0x03, data );
			break;
		}
	}
}


WRITE8_HANDLER( osborne1_3000_w )
{
	/* Check whether regular RAM is enabled */
	if ( ! osborne1.bank2_enabled || ( osborne1.in_irq_handler && osborne1.bankswitch == RAMMODE ) )
	{
		mess_ram[ 0x3000 + offset ] = data;
	}
}


WRITE8_HANDLER( osborne1_videoram_w )
{
	/* Check whether the video attribute section is enabled */
	if ( osborne1.bank3_enabled )
	{
		data |= 0x7F;
	}
	osborne1.bank4_ptr[offset] = data;
}


WRITE8_HANDLER( osborne1_bankswitch_w )
{
	switch( offset )
	{
	case 0x00:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 0;
		break;
	case 0x01:
		osborne1.bank2_enabled = 0;
		osborne1.bank3_enabled = 0;
		break;
	case 0x02:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 1;
		break;
	case 0x03:
		osborne1.bank2_enabled = 1;
		osborne1.bank3_enabled = 0;
		break;
	}
	if ( osborne1.bank2_enabled )
	{
		memory_set_bankptr(space->machine,1, memory_region(space->machine, "maincpu") );
		memory_set_bankptr(space->machine,2, osborne1.empty_4K );
		memory_set_bankptr(space->machine,3, osborne1.empty_4K );
	}
	else
	{
		memory_set_bankptr(space->machine,1, mess_ram );
		memory_set_bankptr(space->machine,2, mess_ram + 0x1000 );
		memory_set_bankptr(space->machine,3, mess_ram + 0x3000 );
	}
	osborne1.bank4_ptr = mess_ram + ( ( osborne1.bank3_enabled ) ? 0x10000 : 0xF000 );
	memory_set_bankptr(space->machine,4, osborne1.bank4_ptr );
	osborne1.bankswitch = offset;
	osborne1.in_irq_handler = 0;
}


static DIRECT_UPDATE_HANDLER( osborne1_opbase )
{
	if ( ( address & 0xF000 ) == 0x2000 )
	{
		if ( ! osborne1.bank2_enabled )
		{
			direct->bytemask = 0x0fff;
			direct->decrypted = mess_ram + 0x2000;
			direct->raw = mess_ram + 0x2000;
			direct->bytestart = 0x2000;
			direct->byteend = 0x2fff;
			return ~0;
		}
	}
	return address;
}


static void osborne1_update_irq_state(running_machine *machine)
{
	//logerror("Changing irq state; pia_0_irq_state = %s, pia_1_irq_state = %s\n", osborne1.pia_0_irq_state ? "SET" : "CLEARED", osborne1.pia_1_irq_state ? "SET" : "CLEARED" );

	if ( osborne1.pia_1_irq_state )
	{
		cpu_set_input_line(machine->cpu[0], 0, ASSERT_LINE );
	}
	else
	{
		cpu_set_input_line(machine->cpu[0], 0, CLEAR_LINE );
	}
}


static WRITE_LINE_DEVICE_HANDLER( ieee_pia_irq_a_func )
{
	osborne1.pia_0_irq_state = state;
	osborne1_update_irq_state(device->machine);
}


const pia6821_interface osborne1_ieee_pia_config =
{
	DEVCB_NULL,							/* in_a_func */
	DEVCB_NULL,							/* in_b_func */
	DEVCB_NULL,							/* in_ca1_func */
	DEVCB_NULL,							/* in_cb1_func */
	DEVCB_NULL,							/* in_ca2_func */
	DEVCB_NULL,							/* in_cb2_func */
	DEVCB_NULL,							/* out_a_func */
	DEVCB_NULL,							/* out_b_func */
	DEVCB_NULL,							/* out_ca2_func */
	DEVCB_NULL,							/* out_cb2_func */
	DEVCB_LINE(ieee_pia_irq_a_func),	/* irq_a_func */
	DEVCB_NULL							/* irq_b_func */
};


static WRITE8_DEVICE_HANDLER( video_pia_out_cb2_dummy )
{
}


static WRITE8_DEVICE_HANDLER( video_pia_port_a_w )
{
	const device_config *fdc = devtag_get_device(device->machine, "mb8877");

	osborne1.new_start_x = data >> 1;
	wd17xx_set_density(fdc, ( data & 0x01 ) ? DEN_FM_LO : DEN_FM_HI );

	//logerror("Video pia port a write: %02X, density set to %s\n", data, data & 1 ? "DEN_FM_LO" : "DEN_FM_HI" );
}


static WRITE8_DEVICE_HANDLER( video_pia_port_b_w )
{
	const device_config *fdc = devtag_get_device(device->machine, "mb8877");

	osborne1.new_start_y = data & 0x1F;
	osborne1.beep = ( data & 0x20 ) ? 1 : 0;
	if ( data & 0x40 )
	{
		wd17xx_set_drive( fdc, 0 );
	}
	else if ( data & 0x80 )
	{
		wd17xx_set_drive( fdc, 1 );
	}
	//logerror("Video pia port b write: %02X\n", data );
}


static WRITE_LINE_DEVICE_HANDLER( video_pia_irq_a_func )
{
	osborne1.pia_1_irq_state = state;
	osborne1_update_irq_state(device->machine);
}


const pia6821_interface osborne1_video_pia_config =
{
	DEVCB_NULL,								/* in_a_func */
	DEVCB_NULL,								/* in_b_func */
	DEVCB_NULL,								/* in_ca1_func */
	DEVCB_NULL,								/* in_cb1_func */
	DEVCB_NULL,								/* in_ca2_func */
	DEVCB_NULL,								/* in_cb2_func */
	DEVCB_HANDLER(video_pia_port_a_w),		/* out_a_func */
	DEVCB_HANDLER(video_pia_port_b_w),		/* out_b_func */
	DEVCB_NULL,								/* out_ca2_func */
	DEVCB_HANDLER(video_pia_out_cb2_dummy),	/* out_cb2_func */
	DEVCB_LINE(video_pia_irq_a_func),		/* irq_a_func */
	DEVCB_NULL								/* irq_b_func */
};


//static const struct aica6850_interface osborne1_6850_config =
//{
//	10,	/* tx_clock */
//	10,	/* rx_clock */
//	NULL,	/* rx_pin */
//	NULL,	/* tx_pin */
//	NULL,	/* cts_pin */
//	NULL,	/* rts_pin */
//	NULL,	/* dcd_pin */
//	NULL	/* int_callback */
//};


static TIMER_CALLBACK(osborne1_video_callback)
{
	const address_space* space = cpu_get_address_space(machine->cpu[0], ADDRESS_SPACE_PROGRAM);
	const device_config *speaker = devtag_get_device(space->machine, "beep");
	const device_config *pia_1 = devtag_get_device(space->machine, "pia_1");
	int y = video_screen_get_vpos(machine->primary_screen);

	/* Check for start of frame */
	if ( y == 0 )
	{
		/* Clear CA1 on video PIA */
		osborne1.start_y = ( osborne1.new_start_y - 1 ) & 0x1F;
		osborne1.charline = 0;
		pia6821_ca1_w( pia_1, 0, 0 );
	}
	if ( y == 240 )
	{
		/* Set CA1 on video PIA */
		pia6821_ca1_w( pia_1, 0, 0xFF );
	}
	if ( y < 240 )
	{
		/* Draw a line of the display */
		UINT16 address = osborne1.start_y * 128 + osborne1.new_start_x + 11;
		UINT16 *p = BITMAP_ADDR16( tmpbitmap, y, 0 );
		int x;

		for ( x = 0; x < 52; x++ )
		{
			UINT8	character = mess_ram[ 0xF000 + ( ( address + x ) & 0xFFF ) ];
			UINT8	cursor = character & 0x80;
			UINT8	dim = mess_ram[ 0x10000 + ( ( address + x ) & 0xFFF ) ] & 0x80;
			UINT8	bits = osborne1.charrom[ osborne1.charline * 128 + ( character & 0x7F ) ];
			int		bit;

			if ( cursor && osborne1.charline == 9 )
			{
				bits = 0xFF;
			}
			for ( bit = 0; bit < 8; bit++ )
			{
				p[x * 8 + bit] = ( bits & 0x80 ) ? ( dim ? 1 : 2 ) : 0;
				bits = bits << 1;
			}
		}

		osborne1.charline += 1;
		if ( osborne1.charline == 10 )
		{
			osborne1.start_y += 1;
			osborne1.charline = 0;
		}
	}

	if ( ( y % 10 ) == 2 || ( y % 10 ) == 6 )
	{
		beep_set_state( speaker, osborne1.beep );
	}
	else
	{
		beep_set_state( speaker, 0 );
	}

	timer_adjust_oneshot(osborne1.video_timer, video_screen_get_time_until_pos(machine->primary_screen, y + 1, 0 ), 0);
}

/*
 * The Osborne-1 supports the following disc formats:
 * - Osborne single density: 40 tracks, 10 sectors per track, 256-byte sectors (100 KByte)
 * - Osborne double density: 40 tracks, 5 sectors per track, 1024-byte sectors (200 KByte)
 * - IBM Personal Computer: 40 tracks, 8 sectors per track, 512-byte sectors (160 KByte)
 * - Xerox 820 Computer: 40 tracks, 18 sectors per track, 128-byte sectors (90 KByte)
 * - DEC 1820 double density: 40 tracks, 9 sectors per track, 512-byte sectors (180 KByte)
 *
 */
DEVICE_IMAGE_LOAD( osborne1_floppy )
{
	int size, sectors, sectorsize;
	const device_config *fdc = devtag_get_device(image->machine, "mb8877");

	if ( ! image_has_been_created( image ) )
	{
		size = image_length( image );

		switch( size )
		{
		case 40 * 10 * 256:
			sectors = 10;
			sectorsize = 256;
			wd17xx_set_density( fdc, DEN_FM_LO );
			break;
		case 40 * 5 * 1024:
			sectors = 5;
			sectorsize = 1024;
			wd17xx_set_density( fdc, DEN_FM_HI );
			break;
		case 40 * 8 * 512:
			sectors = 8;
			sectorsize = 512;
			wd17xx_set_density( fdc, DEN_FM_LO );
			return INIT_FAIL;
		case 40 * 18 * 128:
			sectors = 18;
			sectorsize = 128;
			wd17xx_set_density( fdc, DEN_FM_LO );
			return INIT_FAIL;
		case 40 * 9 * 512:
			sectors = 9;
			sectorsize = 512;
			wd17xx_set_density( fdc, DEN_FM_HI );
			return INIT_FAIL;
		default:
			return INIT_FAIL;
		}
	}
	else
	{
		return INIT_FAIL;
	}

	if ( device_load_basicdsk_floppy( image ) != INIT_PASS )
	{
		return INIT_FAIL;
	}

	basicdsk_set_geometry( image, 40, 1, sectors, sectorsize, 1, 0, FALSE );

	return INIT_PASS;
}


static TIMER_CALLBACK( setup_osborne1 )
{
	const device_config *speaker = devtag_get_device(machine, "beep");
	const device_config *pia_1 = devtag_get_device(machine, "pia_1");

	beep_set_state( speaker, 0 );
	beep_set_frequency( speaker, 300 /* 60 * 240 / 2 */ );
	pia6821_ca1_w( pia_1, 0, 0 );
}


MACHINE_RESET( osborne1 )
{
	const address_space* space = cpu_get_address_space(machine->cpu[0],ADDRESS_SPACE_PROGRAM);
	/* Initialize memory configuration */
	osborne1_bankswitch_w( space, 0x00, 0 );

	osborne1.pia_0_irq_state = FALSE;
	osborne1.pia_1_irq_state = FALSE;

	osborne1.pia_1_irq_state = 0;
	osborne1.in_irq_handler = 0;

	osborne1.charrom = memory_region( machine, "gfx1" );

	memset( mess_ram + 0x10000, 0xFF, 0x1000 );

	osborne1.video_timer = timer_alloc(machine,  osborne1_video_callback , NULL);
	timer_adjust_oneshot(osborne1.video_timer, video_screen_get_time_until_pos(machine->primary_screen, 1, 0 ), 0);

	timer_set(machine,  attotime_zero, NULL, 0, setup_osborne1 );

	memory_set_direct_update_handler( space, osborne1_opbase );
}


DRIVER_INIT( osborne1 )
{
	memset( &osborne1, 0, sizeof( osborne1 ) );

	osborne1.empty_4K = auto_malloc( 0x1000 );
	memset( osborne1.empty_4K, 0xFF, 0x1000 );

	/* Configure the 6850 ACIA */
//	acia6850_config( 0, &osborne1_6850_config );
}


/****************************************************************
	Osborne1 specific daisy chain code
****************************************************************/


static int osborne1_daisy_irq_state(const device_config *device)
{
    return ( osborne1.pia_1_irq_state ? Z80_DAISY_INT : 0 );
}


static int osborne1_daisy_irq_ack(const device_config *device)
{
    /* Enable ROM and I/O when IRQ is acknowledged */
    UINT8	old_bankswitch = osborne1.bankswitch;
    const address_space* space = cpu_get_address_space(device->machine->cpu[0],ADDRESS_SPACE_PROGRAM);
    
    osborne1_bankswitch_w( space, 0, 0 );
    osborne1.bankswitch = old_bankswitch;
    osborne1.in_irq_handler = 1;
    return 0xF8;
}

static void osborne1_daisy_irq_reti(const device_config *device)
{
}


static DEVICE_START( osborne1_daisy )
{
}


static DEVICE_RESET( osborne1_daisy )
{
}


static DEVICE_SET_INFO( osborne1_daisy )
{
}


DEVICE_GET_INFO( osborne1_daisy )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:					info->i = 4;											break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:			info->i = 0;											break;
		case DEVINFO_INT_CLASS:							info->i = DEVICE_CLASS_PERIPHERAL;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_SET_INFO:						info->set_info = DEVICE_SET_INFO_NAME(osborne1_daisy);	break;
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME(osborne1_daisy);		break;
		case DEVINFO_FCT_STOP:							/* Nothing */											break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME(osborne1_daisy);		break;
		case DEVINFO_FCT_IRQ_STATE:						info->f = (genf *)osborne1_daisy_irq_state;				break;
		case DEVINFO_FCT_IRQ_ACK:						info->f = (genf *)osborne1_daisy_irq_ack;				break;
		case DEVINFO_FCT_IRQ_RETI:						info->f = (genf *)osborne1_daisy_irq_reti;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:							strcpy(info->s, "Osborne1 daisy");								break;
		case DEVINFO_STR_FAMILY:						strcpy(info->s, "Z80");										break;
		case DEVINFO_STR_VERSION:						strcpy(info->s, "1.0");										break;
		case DEVINFO_STR_SOURCE_FILE:					strcpy(info->s, __FILE__);										break;
		case DEVINFO_STR_CREDITS:						strcpy(info->s, "Copyright the MESS Team");					break;
	}
}

