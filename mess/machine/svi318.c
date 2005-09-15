/*
** Spectravideo SVI-318 and SVI-328
**
** Sean Young, Tomas Karlsson
**
** Todo:
** - serial ports
** - svi_cas format
** - 80 column cartridge
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "includes/svi318.h"
#include "formats/svi_cas.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "devices/basicdsk.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "sound/dac.h"
#include "sound/ay8910.h"
#include "image.h"

static SVI_318 svi;
static UINT8 *pcart;

static void svi318_set_banks (void);

/* Cartridge */

static int svi318_verify_cart (UINT8 magic[2])
{
	/* read the first two bytes */
	if ( (magic[0] == 0xf3) && (magic[1] == 0x31) )
		return IMAGE_VERIFY_PASS;
	else
		return IMAGE_VERIFY_FAIL;
}

DEVICE_LOAD( svi318_cart )
{
	UINT8 *p;
	int size;

	p = image_malloc(image, 0x8000);
	if (!p)
		return INIT_FAIL;

	memset(p, 0xff, 0x8000);
	size = mame_fsize(file);
	if (mame_fread(file, p, size) != size)
	{
		logerror ("can't read file %s\n", image_filename(image) );
		return INIT_FAIL;
	}

	if (svi318_verify_cart(p)==IMAGE_VERIFY_FAIL)
		return INIT_FAIL;
	pcart = p;
	svi.banks[0][1] = p;

	return INIT_PASS;
}

DEVICE_UNLOAD( svi318_cart )
{
	pcart = svi.banks[0][1] = NULL;
}

/* PPI */

/*
 PPI Port A Input (address 98H)
 Bit Name     Description
  1  TA       Joystick 1, /SENSE
  2  TB       Joystick 1, EOC
  3  TC       Joystick 2, /SENSE
  4  TD       Joystick 2, EOC
  5  TRIGGER1 Joystick 1, Trigger
  6  TRIGGER2 Joystick 2, Trigger
  7  /READY   Cassette, Ready
  8  CASR     Cassette, Read data
*/

static READ8_HANDLER ( svi318_ppi_port_a_r )
{
	int data = 0x0f;

	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE, 0)) > 0.0038)
		data |= 0x80;
	if (!svi318_cassette_present(0) )
		data |= 0x40;
	data |= readinputport(12) & 0x30;

	return data;
}

/*
 PPI Port B Input (address 99H)
 Bit Name Description
  1  IN0  Keyboard, Column status of selected line
  2  IN1  Keyboard, Column status of selected line
  3  IN2  Keyboard, Column status of selected line
  4  IN3  Keyboard, Column status of selected line
  5  IN4  Keyboard, Column status of selected line
  6  IN5  Keyboard, Column status of selected line
  7  IN6  Keyboard, Column status of selected line
  8  IN7  Keyboard, Column status of selected line
*/

static  READ8_HANDLER ( svi318_ppi_port_b_r )
{
	int row;

	row = ppi8255_peek (0, 2) & 0x0f;
	if (row <= 10)
	{
		if (row == 6)
			return readinputport(row) & readinputport(14);
		else
			return readinputport(row);
	}
	return 0xff;
}

/*
 PPI Port C Output (address 97H)
 Bit Name   Description
  1  KB0    Keyboard, Line select 0
  2  KB1    Keyboard, Line select 1
  3  KB2    Keyboard, Line select 2
  4  KB3    Keyboard, Line select 3
  5  CASON  Cassette, Motor relay control (0=on, 1=off)
  6  CASW   Cassette, Write data
  7  CASAUD Cassette, Audio out (pulse)
  8  SOUND  Keyboard, Click sound bit (pulse)
*/

static WRITE8_HANDLER ( svi318_ppi_port_c_w )
{
	int val;

	/* key click */
	val = (data & 0x80) ? 0x3e : 0;
	val += (data & 0x40) ? 0x3e : 0;
	DAC_signed_data_w (0, val);

	/* cassette motor on/off */
	if (svi318_cassette_present (0) )
	{
		cassette_change_state(
			image_from_devtype_and_index(IO_CASSETTE, 0),
			(data & 0x10) ? CASSETTE_MOTOR_DISABLED : CASSETTE_MOTOR_ENABLED,
			CASSETTE_MOTOR_DISABLED);
	}

	/* cassette signal write */
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0), (data & 0x20) ? -1.0 : +1.0);
}

static ppi8255_interface svi318_ppi8255_interface =
{
	1,
	{svi318_ppi_port_a_r},
	{svi318_ppi_port_b_r},
	{NULL},
	{NULL},
	{NULL},
	{svi318_ppi_port_c_w}
};

READ8_HANDLER (svi318_ppi_r)
{
	return ppi8255_0_r (offset);
}

WRITE8_HANDLER (svi318_ppi_w)
{
	ppi8255_0_w (offset + 2, data);
}

/* Printer port */

WRITE8_HANDLER (svi318_printer_w)
{
	if (!offset)
		svi.prn_data = data;
	else
	{
		if ( (svi.prn_strobe & 1) && !(data & 1) )
			printer_output(image_from_devtype_and_index(IO_PRINTER, 0), svi.prn_data);

		svi.prn_strobe = data;
	}
}

READ8_HANDLER (svi318_printer_r)
{
	if (printer_status(image_from_devtype_and_index(IO_PRINTER, 0), 0) )
		return 0xfe;

	return 0xff;
}

/* PSG */

/*
 PSG Port A Input
 Bit Name   Description
  1  FWD1   Joystick 1, Forward
  2  BACK1  Joystick 1, Back
  3  LEFT1  Joystick 1, Left
  4  RIGHT1 Joystick 1, Right
  5  FWD2   Joystick 2, Forward
  6  BACK2  Joystick 2, Back
  7  LEFT2  Joystick 2, Left
  8  RIGHT2 Joystick 2, Right
*/

READ8_HANDLER (svi318_psg_port_a_r)
{
	return readinputport (11);
}

/*
 PSG Port B Output
 Bit Name    Description
  1  /CART   Memory bank 11, ROM 0000-7FFF (Cartridge /CCS1, /CCS2)
  2  /BK21   Memory bank 21, RAM 0000-7FFF
  3  /BK22   Memory bank 22, RAM 8000-FFFF
  4  /BK31   Memory bank 31, RAM 0000-7FFF
  5  /BK32   Memory bank 32, RAM 8000-7FFF
  6  CAPS    Caps-Lock diod
  7  /ROMEN0 Memory bank 12, ROM 8000-BFFF* (Cartridge /CCS3)
  8  /ROMEN1 Memory bank 12, ROM C000-FFFF* (Cartridge /CCS4)

 * The /CART signal must be active for any effect and all banks
 with RAM are disabled.
*/

WRITE8_HANDLER (svi318_psg_port_b_w)
{
	if ( (svi.bank_switch ^ data) & 0x20)
		set_led_status (0, !(data & 0x20) );

	svi.bank_switch = data;
	svi318_set_banks ();
}

/* Disk drives  */

typedef struct
{
	UINT8 seldrive;
	UINT8 irq_drq;
} SVI318_FDC_STRUCT;

static SVI318_FDC_STRUCT svi318_fdc_status;
static UINT8 svi318_dsk_heads[2];

static void svi_fdc_callback(int param)
{
	switch( param )
	{
	case WD179X_IRQ_CLR:
		svi318_fdc_status.irq_drq &= ~0x80;
		break;
	case WD179X_IRQ_SET:
		svi318_fdc_status.irq_drq |= 0x80;
		break;
	case WD179X_DRQ_CLR:
		svi318_fdc_status.irq_drq &= ~0x40;
		break;
	case WD179X_DRQ_SET:
		svi318_fdc_status.irq_drq |= 0x40;
		break;
	}
}

WRITE8_HANDLER (fdc_disk_motor_w)
{
	switch (data & 3)
	{
	case 1:
		wd179x_set_drive(0);
		svi318_fdc_status.seldrive = 0;
		break;
	case 2:
		wd179x_set_drive(1);
		svi318_fdc_status.seldrive = 1;
		break;
	}
}

WRITE8_HANDLER (fdc_density_side_w)
{
	mess_image *image;

	wd179x_set_density(data & 0x01 ? DEN_FM_LO:DEN_MFM_LO);

	wd179x_set_side(data & 0x02 ? 1:0);
            
	image = image_from_devtype_and_index(IO_FLOPPY, svi318_fdc_status.seldrive);
	if (image_exists(image))
	{
		UINT8 sectors;
		UINT16 sectorSize;
		sectors =  data & 0x01 ? 18:17;
		sectorSize = data & 0x01 ? 128:256;
		basicdsk_set_geometry(image, 40, svi318_dsk_heads[svi318_fdc_status.seldrive], sectors, sectorSize, 1, 0, FALSE);
	}
}

READ8_HANDLER (svi318_fdc_status_r)
{
	return svi318_fdc_status.irq_drq;
}

static unsigned long svi318_calcoffset(UINT8 t, UINT8 h, UINT8 s,
	UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero)
{
	unsigned long o;

	if ((t==0) && (h==0)) 
		o = (s-first_sector_id)*128; 
	else
		o = ((t*heads+h)*17+s-first_sector_id)*256-2048; /* (17*256)-(18*128)=2048 */

	return o;
}

DEVICE_LOAD( svi318_floppy )
{
	int size;
	int id = image_index_in_device(image);

	if (!image_has_been_created(image))
	{
		size = mame_fsize (file);

		switch (size)
		{
		case 172032: /* Single sided */
			svi318_dsk_heads[id] = 1;
			break;
		case 346112: /* Double sided */
			svi318_dsk_heads[id] = 2;
			break;
		default:
			return INIT_FAIL;
		}
		}
	else
		return INIT_FAIL;

	if (device_load_basicdsk_floppy(image, file) != INIT_PASS)
		return INIT_FAIL;

	basicdsk_set_geometry(image, 40, svi318_dsk_heads[id], 17, 256, 1, 0, FALSE);
	basicdsk_set_calcoffset(image, svi318_calcoffset);

	return INIT_PASS;
}

/* The init functions */

void svi318_vdp_interrupt (int i)
{
	cpunum_set_input_line(0, 0, (i ? HOLD_LINE : CLEAR_LINE));
}

DRIVER_INIT( svi318 )
{
	int i, n;

	/* z80 stuff */
	static int z80_cycle_table[] =
	{
		Z80_TABLE_op, Z80_TABLE_cb, Z80_TABLE_xy,
        	Z80_TABLE_ed, Z80_TABLE_xycb, Z80_TABLE_ex
	};

	memset(&svi, 0, sizeof (svi) );

	svi.svi318 = !strcmp (Machine->gamedrv->name, "svi318");

	cpunum_set_input_line_vector (0, 0, 0xff);
	ppi8255_init (&svi318_ppi8255_interface);

	/* memory */
	svi.empty_bank = malloc (0x8000);
	if (!svi.empty_bank) { logerror ("Cannot malloc!\n"); return; }
	memset (svi.empty_bank, 0xff, 0x8000);
	svi.banks[0][0] = memory_region(REGION_CPU1);
	svi.banks[1][0] = malloc (0x8000);
	if (!svi.banks[1][0]) { logerror ("Cannot malloc!\n"); return; }
	memset (svi.banks[1][0], 0, 0x8000);

	/* should also be allocated via dip-switches ... redundant? */
	if (!svi.svi318)
	{
		svi.banks[1][2] = malloc (0x8000);
		if (!svi.banks[1][2]) { logerror ("Cannot malloc!\n"); return; }
		memset (svi.banks[1][2], 0, 0x8000);
	}

	svi.banks[0][1] = pcart;

	/* adjust z80 cycles for the M1 wait state */
	for (i = 0; i < sizeof(z80_cycle_table) / sizeof(z80_cycle_table[0]); i++)
	{
		UINT8 *table = auto_malloc (0x100);
		const UINT8 *old_table;

		old_table = cpunum_get_info_ptr (0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i]);
		memcpy (table, old_table, 0x100);

		if (z80_cycle_table[i] == Z80_TABLE_ex)
		{
			table[0x66]++; /* NMI overhead (not used) */
			table[0xff]++; /* INT overhead */
		}
		else
		{
			for (n=0; n<256; n++)
			{
				if (z80_cycle_table[i] == Z80_TABLE_op)
				{
					table[n]++;
				}
				else {
					table[n] += 2;
				}
			}
		}
		cpunum_set_info_ptr(0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i], (void*)table);
	}

	/* floppy */
	wd179x_init(WD_TYPE_179X, svi_fdc_callback);
}

MACHINE_INIT( svi318 )
{
	/* video stuff */
	TMS9928A_reset();

	/* PPI */
	ppi8255_0_w(3, 0x92);

	svi.bank_switch = 0xff;
	svi318_set_banks();

	wd179x_reset();
}

MACHINE_STOP( svi318 )
{
/*
	int i,j;

	if (svi.empty_bank) free (svi.empty_bank);
	if (svi.banks[1][0]) free (svi.banks[1][0]);
	for (i=2;i<4;i++)
		for (j=0;j<2;j++)
			{
			if (svi.banks[j][i])
				{
				free (svi.banks[j][i]);
				svi.banks[j][i] = NULL;
				}
			}

	for (i=0; z80_cycle_table[i].table != -1; i++) {
		cpunum_set_info_ptr(0, CPUINFO_PTR_Z80_CYCLE_TABLE + z80_cycle_table[i].table,
				(void*)z80_cycle_table[i].old_table);
	}
*/
}

INTERRUPT_GEN( svi318_interrupt )
{
	int set, i, p, b, bit;

	set = readinputport (13);
	TMS9928A_set_spriteslimit (set & 0x20);
	TMS9928A_interrupt();

	/* memory banks */
	for (i=0;i<4;i++)
	{
		bit = set & (1 << i);
		p = (i & 1);
		b = i / 2 + 2;

		if (bit && !svi.banks[p][b])
		{
			svi.banks[p][b] = malloc (0x8000);
			if (!svi.banks[p][b])
				logerror ("Cannot malloc bank%d%d!\n", b, p + 1);
			else
			{
				memset (svi.banks[p][b], 0, 0x8000);
				logerror ("bank%d%d allocated.\n", b, p + 1);
			}
		}
		else if (!bit && svi.banks[p][b])
		{
			free (svi.banks[p][b]);
			svi.banks[p][b] = NULL;
			logerror ("bank%d%d freed.\n", b, p + 1);
		}
	}
}

/* Memory */

WRITE8_HANDLER (svi318_writemem0)
{
	if (svi.bank1 < 2) return;

	if (svi.banks[0][svi.bank1])
		svi.banks[0][svi.bank1][offset] = data;
}

WRITE8_HANDLER (svi318_writemem1)
{
	switch (svi.bank2)
	{
	case 0:
		if (!svi.svi318 || offset >= 0x4000)
			svi.banks[1][0][offset] = data;

		break;
	case 2:
	case 3:
		if (svi.banks[1][svi.bank2])
			svi.banks[1][svi.bank2][offset] = data;
		break;
	}
}

static void svi318_set_banks ()
{
	const UINT8 v = svi.bank_switch;

	svi.bank1 = (v&1)?(v&2)?(v&8)?0:3:2:1;
	svi.bank2 = (v&4)?(v&16)?0:3:2;

	if (svi.banks[0][svi.bank1])
		memory_set_bankptr (1, svi.banks[0][svi.bank1]);
	else
		memory_set_bankptr (1, svi.empty_bank);

	if (svi.banks[1][svi.bank2])
	{
		memory_set_bankptr (2, svi.banks[1][svi.bank2]);
		memory_set_bankptr (3, svi.banks[1][svi.bank2] + 0x4000);

		/* SVI-318 has only 16kB RAM -- not 32kb! */
		if (!svi.bank2 && svi.svi318)
			memory_set_bankptr (2, svi.empty_bank);

		if ((svi.bank1 == 1) && ( (v & 0xc0) != 0xc0))
		{
			memory_set_bankptr (2, (v&80)?svi.empty_bank:svi.banks[1][1] + 0x4000);
			memory_set_bankptr (3, (v&40)?svi.empty_bank:svi.banks[1][1]);
		}
	}
	else
	{
		memory_set_bankptr (2, svi.empty_bank);
		memory_set_bankptr (3, svi.empty_bank);
	}
}

/* Cassette */

int svi318_cassette_present (int id)
{
	return image_exists(image_from_devtype_and_index(IO_CASSETTE, id));
}