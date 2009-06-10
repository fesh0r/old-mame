/***************************************************************************

	microbee.c

    machine driver
	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000


****************************************************************************/

#include "driver.h"
#include "includes/mbee.h"

static UINT8 mbee_vsync;
static UINT8 fdc_status = 0;
static const device_config *mbee_fdc;
static const device_config *mbee_z80pio;
static const device_config *mbee_speaker;
static const device_config *mbee_cassette;
static const device_config *mbee_printer;

/***********************************************************

	PIO

************************************************************/

static WRITE8_DEVICE_HANDLER( mbee_pio_interrupt )
{
	cputag_set_input_line(device->machine, "maincpu", 0, data );
}

static WRITE8_DEVICE_HANDLER( pio_ardy )
{
	/* devices need to be redeclared in this callback for some strange reason */
	mbee_printer = devtag_get_device(device->machine, "centronics");
	centronics_strobe_w(mbee_printer, (data) ? 0 : 1);
}

static WRITE8_DEVICE_HANDLER( pio_port_a_w )
{
	/* hardware strobe driven by PIO ARDY, bit 7..0 = data */
	z80pio_astb_w( mbee_z80pio, 1);	/* needed - otherwise nothing prints */
	centronics_data_w(mbee_printer, 0, data);
};

static WRITE8_DEVICE_HANDLER( pio_port_b_w )
{
/*	PIO port B - d5..d2 not emulated
	d7 network interrupt (microbee network for classrooms)
	d6 speaker
	d5 rs232 output (1=mark)
	d4 rs232 input (0=mark)
	d3 rs232 CTS (0=clear to send)
	d2 rs232 clock or DTR
	d1 cass out
	d0 cass in */

	cassette_output(mbee_cassette, (data & 0x02) ? -1.0 : +1.0);

	speaker_level_w(mbee_speaker, (data & 0x40) ? 1 : 0);
};

static READ8_DEVICE_HANDLER( pio_port_b_r )
{
	UINT8 data = 0;

	if (cassette_input(mbee_cassette) > 0.03)
		data |= 0x01;

	data |= mbee_vsync << 7;

	if (mbee_vsync) mbee_vsync = 0;

	return data;
};

const z80pio_interface mbee_z80pio_intf =
{
	DEVCB_HANDLER(mbee_pio_interrupt),	/* callback when change interrupt status */
	DEVCB_NULL,
	DEVCB_HANDLER(pio_port_b_r),
	DEVCB_HANDLER(pio_port_a_w),
	DEVCB_HANDLER(pio_port_b_w),
	DEVCB_HANDLER(pio_ardy),
	DEVCB_NULL
};

READ8_DEVICE_HANDLER( mbee_pio_r )
{
	if (!offset)
		return z80pio_d_r(device, 0);
	else
	if (offset == 1)
		return z80pio_c_r(device, 0);
	else
	if (offset == 2)
		return z80pio_d_r(device, 1);
	else
		return z80pio_c_r(device, 1);
}

WRITE8_DEVICE_HANDLER( mbee_pio_w )
{
	if (!offset)
		z80pio_d_w(device, 0, data);
	else
	if (offset == 1)
		z80pio_c_w(device, 0, data);
	else
	if (offset == 2)
		z80pio_d_w(device, 1, data);
	else
		z80pio_c_w(device, 1, data);
}


/*************************************************************************************

	Floppy DIsk

	The callback is quite simple, no interrupts are used.
	If either IRQ or DRQ activate, they set bit 7 of inport 0x48.

*************************************************************************************/

static WD17XX_CALLBACK( mbee_fdc_callback )
{
	if (WD17XX_IRQ_SET || WD17XX_DRQ_SET)
		fdc_status |= 0x80;
	else
		fdc_status &= 0x7f;
}

const wd17xx_interface mbee_wd17xx_interface = { mbee_fdc_callback, NULL };

READ8_HANDLER ( mbee_fdc_status_r )
{
/*	d7 indicate if IRQ or DRQ is occuring (1=happening)
	d6..d0 not used */

	return fdc_status;
}

WRITE8_HANDLER ( mbee_fdc_motor_w )
{
/*	d7..d4 not used
	d3 density (1=MFM)
	d2 side (1=side 1)
	d1..d0 drive select (0 to 3 - although no microbee ever had more than 2 drives) */

	wd17xx_set_drive(mbee_fdc, data & 3);
	wd17xx_set_side(mbee_fdc, (data & 4) ? 1 : 0);
	wd17xx_set_density(mbee_fdc, (data & 8) ? 1 : 0);
}

static DEVICE_IMAGE_LOAD( mbee_floppy )
{
	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		if (!mame_stricmp(image_filetype(image), "ss80"))
		{
			basicdsk_set_geometry(image, 80, 1, 10, 512, 1, 0, FALSE);
			return INIT_PASS;
		}
		else
		if (!mame_stricmp(image_filetype(image), "ds40"))
		{
			basicdsk_set_geometry(image, 80, 2, 10, 512, 1, 0, FALSE);
			return INIT_PASS;
		}
		else
		if (!mame_stricmp(image_filetype(image), "ds80"))
		{
			basicdsk_set_geometry(image, 160, 2, 10, 512, 1, 0, FALSE);
			return INIT_PASS;
		}
		else
		if (!mame_stricmp(image_filetype(image), "ds84"))
		{
			basicdsk_set_geometry(image, 168, 2, 10, 512, 1, 0, FALSE);
			return INIT_PASS;
		}
		else
		if (!mame_stricmp(image_filetype(image), "dsk"))
		{
			return INIT_FAIL;	// not handled yet - CPC-EMU formatted image
		}
		else
		if (!mame_stricmp(image_filetype(image), "img"))
		{
			return INIT_FAIL;	// not handled - not investigated yet
		}
	}

	return INIT_FAIL;
}

void mbee_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:		info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:		info->load = DEVICE_IMAGE_LOAD_NAME(mbee_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy(info->s = device_temp_str(), "ss80,ds40,ds80,ds84,dsk,img"); break;

		default:				legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}


/***********************************************************

	Machine

************************************************************/

/*
  On reset or power on, a circuit forces rom 8000-8FFF to appear at 0000-0FFF, while ram is disabled.
  It gets set back to normal on the first attempt to write to memory. (/WR line goes active).
*/


/* after the first 4 bytes have been read from ROM, switch the ram back in */
static TIMER_CALLBACK( mbee_reset )
{
	memory_set_bank(machine, 1, 0);
}

MACHINE_RESET( mbee )
{
	timer_set(machine, ATTOTIME_IN_USEC(4), NULL, 0, mbee_reset);
	memory_set_bank(machine, 1, 1);
	mbee_z80pio = devtag_get_device(machine, "z80pio");
	mbee_speaker = devtag_get_device(machine, "speaker");
	mbee_cassette = devtag_get_device(machine, "cassette");
	mbee_printer = devtag_get_device(machine, "centronics");
	mbee_fdc = devtag_get_device(machine, "wd179x");
}


INTERRUPT_GEN( mbee_interrupt )
{
	const address_space *space = cputag_get_address_space(device->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	/* once per frame, pulse the PIO B bit 7 */
	mbee_vsync = 1;

	/* The printer status connects to the pio ASTB pin, and the printer changing to not
		busy should signal an interrupt routine at B61C, (next line) but this doesn't work.
		The line below does what the interrupt should be doing. */

	z80pio_astb_w( mbee_z80pio, centronics_busy_r(mbee_printer));	/* signal int when not busy (L->H) */

	memory_write_byte(space, 0x109, centronics_busy_r(mbee_printer));
}

/***********************************************************

	Quickload

	These load the standard BIN format, as well
	as COM and MWB files.

************************************************************/

Z80BIN_EXECUTE( mbee )
{
	const device_config *cpu = cputag_get_cpu(machine, "maincpu");
	const address_space *space = cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM);

	memory_write_word_16le(space, 0xa6, execute_address);			/* fix the EXEC command */

	if (autorun)
	{
		memory_write_word_16le(space, 0xa2, execute_address);		/* fix warm-start vector to get around some copy-protections */
		cpu_set_reg(cpu, REG_GENPC, execute_address);
	}
	else
	{
		memory_write_word_16le(space, 0xa2, 0x8517);
	}
}

QUICKLOAD_LOAD( mbee )
{
	const device_config *cpu = cputag_get_cpu(image->machine, "maincpu");
	const address_space *space = cputag_get_address_space(image->machine, "maincpu", ADDRESS_SPACE_PROGRAM);
	UINT16 i, j;
	UINT8 data, sw = input_port_read(image->machine, "CONFIG") & 1;	/* reading the dipswitch: 1 = autorun */

	if (!mame_stricmp(image_filetype(image), "mwb"))
	{
		/* mwb files - standard basic files */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x8c0 + i;

			if (image_fread(image, &data, 1) != 1) return INIT_FAIL;

			if ((j < mbee_size) || (j > 0xefff))
				memory_write_byte(space, j, data);
			else
				return INIT_FAIL;
		}

		if (sw)
		{
			memory_write_word_16le(space, 0xa2,0x801e);	/* fix warm-start vector to get around some copy-protections */
			cpu_set_reg(cpu, REG_GENPC, 0x801e);
		}
		else
			memory_write_word_16le(space, 0xa2,0x8517);
	}
	else if (!mame_stricmp(image_filetype(image), "com"))
	{
		/* com files - most com files are just machine-language games with a wrapper and don't need cp/m to be present */
		for (i = 0; i < quickload_size; i++)
		{
			j = 0x100 + i;

			if (image_fread(image, &data, 1) != 1) return INIT_FAIL;

			if ((j < mbee_size) || (j > 0xefff))
				memory_write_byte(space, j, data);
			else
				return INIT_FAIL;
		}

		if (sw) cpu_set_reg(cpu, REG_GENPC, 0x100);
	}

	return INIT_PASS;
}
