/*
** Spectravideo SVI-318 and SVI-328
**
** Sean Young, Tomas Karlsson
**
** Todo:
** - serial ports
** - svi_cas format
** - disk drives
** - 80 column cartridge
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/wd179x.h"
#include "includes/svi318dk.h"
#include "includes/svi318.h"
#include "formats/svi_cas.h"
#include "machine/8255ppi.h"
#include "vidhrdw/tms9928a.h"
#include "printer.h"
#include "image.h"

static SVI_318 svi;
static UINT8 *pcart;

static void svi318_set_banks (void);

/*
** Cartridge stuff
*/

static int svi318_verify_cart (UINT8 magic[2])
{
    /* read the first two bytes */
	if ( (magic[0] == 0xf3) && (magic[1] == 0xc3) )
		return IMAGE_VERIFY_PASS;
	else
		return IMAGE_VERIFY_FAIL;
}



int svi318_load_rom (int id, mame_file *f, int open_mode)
{
	UINT8 *p;
	int size;

	/* A cartridge isn't strictly mandatory */
	if (f == NULL)
	{
		logerror("SVI318 - warning: no cartridge specified!\n");
		return INIT_PASS;
	}

	if (f)
	{
		p = image_malloc(IO_CARTSLOT, id, 0x8000);
		if (!p)
		{
			logerror ("malloc () failed!\n");
			return INIT_FAIL;
		}

		memset (p, 0xff, 0x8000);
		size = mame_fsize (f);
		if (mame_fread (f, p, size) != size)
		{
			logerror ("can't read file %s\n", image_filename (IO_CASSETTE, id) );
			return INIT_FAIL;
		}

		if(svi318_verify_cart(p)==IMAGE_VERIFY_FAIL)
			return INIT_FAIL;
		pcart = p;
		svi.banks[0][1] = p;

		return INIT_PASS;
	}

	return INIT_FAIL;
}

void svi318_exit_rom (int id)
{
	pcart = svi.banks[0][1] = NULL;
}

/*
** PPI stuff
*/

/*

Port A (Port address: 98H) (I/O Status: Input) (Operating mode: 0)

Bit 7: Cassette: Read data
Bit 6: Cassette: Ready
Bit 5: Joystick 2: Trigger
Bit 4: Joystick 1: Trigger
Bit 3: Joystick 2: EOC
Bit 2: Joystick 2: /SENSE
Bit 1: Joystick 1: EOC
Bit 0: Joystick 1: /SENSE

*/
static READ_HANDLER ( svi318_ppi_port_a_r )
	{
    int data = 0x0f;

	if (device_input (IO_CASSETTE, 0) > 255) data |= 0x80;
	if (!svi318_cassette_present (0) ) data |= 0x40;
	data |= readinputport (12) & 0x30;

	return data;
	}

/*

Port B (Port address: 99H) (I/O Status: Input) (Operating mode: 0)

Bit 7: Keyboard: Column status of selected line
Bit 6: Keyboard: Column status of selected line
Bit 5: Keyboard: Column status of selected line
Bit 4: Keyboard: Column status of selected line
Bit 3: Keyboard: Column status of selected line
Bit 2: Keyboard: Column status of selected line
Bit 1: Keyboard: Column status of selected line
Bit 0: Keyboard: Column status of selected line

*/
static READ_HANDLER ( svi318_ppi_port_b_r )
	{
	int row;

	row = ppi8255_peek (0, 2) & 0x0f;
    if (row <= 10) return readinputport (row);
	else return 0xff;
	}

/*
Port C (Port address: 97H) (I/O Status: Output) (Operating mode: 0)

Bit 7: Keyboard: Click sound bit (pulse)
Bit 6: Cassette: Audio out (pulse)
Bit 5: Cassette: Write data
Bit 4: Cassette: Motor relay control (0=on, 1=off)
Bit 3: Keyboard: Line select 3
Bit 2: Keyboard: Line select 2
Bit 1: Keyboard: Line select 1
Bit 0: Keyboard: Line select 0

*/
static WRITE_HANDLER ( svi318_ppi_port_c_w )
	{
	static int old_val = 0xff;
	int val;

    /* key click */
    if ( (old_val ^ data) & 0xc0)
		{
		val = (data & 0x80) ? 0x3e : 0;
		val += (data & 0x40) ? 0x3e : 0;
        DAC_signed_data_w (0, val);
		}
    /* cassette motor on/off */
    if (svi318_cassette_present (0) )
        device_status (IO_CASSETTE, 0, (data & 0x10) ? 0 : 1);
    /* cassette signal write */
    if ( (old_val ^ data) & 0x20)
        device_output (IO_CASSETTE, 0, (data & 0x20) ? -32767 : 32767);

	old_val = data;
	}

static ppi8255_interface svi318_ppi8255_interface = {
    1,
    {svi318_ppi_port_a_r},
    {svi318_ppi_port_b_r},
    {NULL},
    {NULL},
    {NULL},
    {svi318_ppi_port_c_w}
};

READ_HANDLER (svi318_ppi_r)
	{
	return ppi8255_0_r (offset);
	}

WRITE_HANDLER (svi318_ppi_w)
	{
	ppi8255_0_w (offset + 2, data);
	}

/*
** Printer ports
*/

WRITE_HANDLER (svi318_printer_w)
	{
    if (!offset)
		svi.prn_data = data;
	else
		{
		if ( (svi.prn_strobe & 1) && !(data & 1) )
            device_output (IO_PRINTER, 0, svi.prn_data);

        svi.prn_strobe = data;
		}
	}

READ_HANDLER (svi318_printer_r)
	{
	if (device_status (IO_PRINTER, 0, 0) )
        return 0xfe;

    return 0xff;
	}

/*
** PSG port A and B
*/

/*
Bit Name    Description
1   /CART   Bank 11, ROM Cartridge 0000-7FFF
2   /BK21   Bank 21, RAM 0000-7FFF
3   /BK22   Bank 22, RAM 8000-FFFF
4   /BK31   Bank 31, RAM 0000-7FFF
5   /BK32   Bank 32, RAM 8000-7FFF
6   CAPS    Caps-Lock diod
7   /ROMEN0 Bank 12, ROM Cartridge CCS3 8000-BFFF*
8   /ROMEN1 Bank 12, ROM Cartridge CCS4 C000-FFFF*

* The /CART signals must be active for any effect and all banks of RAM are
disabled.
*/

WRITE_HANDLER (svi318_psg_port_b_w)
	{
	if ( (svi.bank_switch ^ data) & 0x20)
		set_led_status (0, !(data & 0x20) );

    svi.bank_switch = data;
    svi318_set_banks ();
	}

/*

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

READ_HANDLER (svi318_psg_port_a_r)
	{
	return readinputport (11);
	}

/*
** Disk drives
*/

#ifdef SVI_DISK

// SVI-801 Floppy disk controller
typedef struct
{
	UINT8 seldrive;
	UINT8 irq_drq;
} SVI318_FDC_STRUCT;


static SVI318_FDC_STRUCT svi318_fdc_status;
static UINT8 svi318_dsk_heads[2];

/*************************************************************/
/* Function: svi801_FDC_Callback                             */
/* Purpose:  Callback routine for the FDC.                   */
/*************************************************************/
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

/*************************************************************/
/* Function: svi801_Select_DriveMotor                        */
/* Purpose:  Floppy drive and motor select.                  */
/*************************************************************/
WRITE_HANDLER (fdc_disk_motor_w)
{
    UINT8 seldrive = 255;

    if (data == 0)
    {
//        wd179x_stop_drive();
        return;
    }
    if (data & 2) seldrive=1;

    if (data & 1) seldrive=0;

    if (seldrive > 3) return;

    svi318_fdc_status.seldrive = seldrive;
    wd179x_set_drive (seldrive);
}

/*************************************************************/
/* Function: svi801_Select_SideDensity                       */
/* Purpose:  Floppy density and head select.                 */
/*************************************************************/
WRITE_HANDLER (fdc_density_side_w)
{
    UINT8 sec_per_track;
    UINT16 sector_size;
		
    if (data & 2)
        wd179x_set_side (1);
    else
        wd179x_set_side (0);

    if (data & 1)
    {
	wd179x_set_density (DEN_FM_LO);
	sec_per_track =  18;
	sector_size = 128;
    }
    else
    {
	wd179x_set_density (DEN_MFM_LO);
	sec_per_track =  17;
	sector_size = 256;
    }
    
//  svi318dsk_set_geometry(UINT8 drive, UINT16 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT8 first_sector_id, UINT16 offset_track_zero)
    svi318dsk_set_geometry(svi318_fdc_status.seldrive, 40, svi318_dsk_heads[svi318_fdc_status.seldrive], sec_per_track, sector_size, 1, 0);

//wd179x_set_geometry(UINT8 density, UINT8 drive, UINT8 tracks, UINT8 heads, UINT8 sec_per_track, UINT16 sector_length, UINT16 dir_sector, UINT16 dir_length, UINT8 first_sector_id);
//	wd179x_set_geometry(svi801_FDC.Density, svi801_FDC.DriveSelect, TRACKS_DISK, svi801_DiskImage[svi801_FDC.DriveSelect].Heads, bytSectorsTrack, sector_size, 0, 0, 1);
}

READ_HANDLER (svi318_fdc_status_r)
{
	return svi318_fdc_status.irq_drq;
}


int svi318_floppy_init(int id, mame_file *fp, int open_mode)
{
	int size;

	if (fp == NULL)
		return INIT_PASS;

	if (fp && ! is_effective_mode_create(open_mode))
		{
		size = mame_fsize (fp);

		switch (size)
			{
			case 172032:	// Single sided
				svi318_dsk_heads[id] = 1;
				break;
			case 346112:	// Double sided
				svi318_dsk_heads[id] = 2;
				break;
			default:
				return INIT_FAIL;
			}
		}
	else
		return INIT_FAIL;

	if (svi318dsk_floppy_init (id, fp, open_mode) != INIT_PASS)
		return INIT_FAIL;

	svi318dsk_set_geometry (id, 40, svi318_dsk_heads[id], 17, 256, 1, 0);

	return INIT_PASS;
}
#endif

/*
** The init functions & stuff
*/

/* z80 stuff */
static int z80_table_num[5] = { Z80_TABLE_op, Z80_TABLE_xy,
    Z80_TABLE_ed, Z80_TABLE_cb, Z80_TABLE_xycb };
static UINT8 *old_z80_tables[5], *z80_table;

void svi318_vdp_interrupt (int i)
	{
    cpu_set_irq_line (0, 0, (i ? HOLD_LINE : CLEAR_LINE));
	}

DRIVER_INIT( svi318 )
	{
	int i,n;

    memset (&svi, 0, sizeof (svi) );

    svi.svi318 = !strcmp (Machine->gamedrv->name, "svi318");

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
    z80_table = malloc (0x500);
    if (!z80_table)
        logerror ("Cannot malloc z80 cycle table, using default values\n");
    else
        {
        for (i=0;i<5;i++)
            {
            old_z80_tables[i] = (void *) z80_get_cycle_table (z80_table_num[i]);
            for (n=0;n<256;n++)
                {
                z80_table[i*0x100+n] = old_z80_tables[i][n] + (i > 1 ? 2 : 1);
                }
            z80_set_cycle_table (i, z80_table + i*0x100);
            }
        }

#ifdef SVI_DISK
	/* floppy */
	wd179x_init (WD_TYPE_179X,svi_fdc_callback);
#endif
	}

MACHINE_INIT( svi318 )
	{
    /* video stuff */
    TMS9928A_reset ();
    cpu_irq_line_vector_w(0,0,0xff);

    /* PPI */
	ppi8255_0_w (3, 0x92);

    svi.bank_switch = 0xff;
    svi318_set_banks ();

#ifdef SVI_DISK
	wd179x_reset ();
#endif
	}

MACHINE_STOP( svi318 )
	{
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

    if (z80_table)
        {
        for (i=0;i<5;i++)
            z80_set_cycle_table (i, old_z80_tables[i]);

        free (z80_table);
        }
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

/*
** Memory stuff
*/

WRITE_HANDLER (svi318_writemem0)
	{
	if (svi.bank1 < 2) return;

	if (svi.banks[0][svi.bank1])
		svi.banks[0][svi.bank1][offset] = data;
	}

WRITE_HANDLER (svi318_writemem1)
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
		cpu_setbank (1, svi.banks[0][svi.bank1]);
	else
		cpu_setbank (1, svi.empty_bank);

    if (svi.banks[1][svi.bank2])
		{
		cpu_setbank (2, svi.banks[1][svi.bank2]);
		cpu_setbank (3, svi.banks[1][svi.bank2] + 0x4000);

		/* SVI-318 has only 16kB RAM -- not 32kb! */
		if (!svi.bank2 && svi.svi318)
			cpu_setbank (2, svi.empty_bank);

    	if ((svi.bank1 == 1) && ( (v & 0xc0) != 0xc0))
			{
			cpu_setbank (2, (v&80)?svi.empty_bank:svi.banks[1][1] + 0x4000);
			cpu_setbank (3, (v&40)?svi.empty_bank:svi.banks[1][1]);
			}
		}
	else
		{
		cpu_setbank (2, svi.empty_bank);
		cpu_setbank (3, svi.empty_bank);
		}
	}

/*
** Cassette
*/


static INT16* cas_samples;
static int cas_len;

static int svi318_cassette_fill_wave (INT16* samples, int wavlen, UINT8* casdata)
	{
	if (casdata == CODE_HEADER || casdata == CODE_TRAILER)
		return 0;

	if (wavlen < cas_len)
		{
		logerror ("Not enough space to store converted cas file!\n");
		return 0;
		}

	memcpy (samples, cas_samples, cas_len * 2);

	return cas_len;
	}

static int check_svi_cas (mame_file *f)
	{
	UINT8* casdata;
	int caslen, ret;

    caslen = mame_fsize (f);
	if (caslen < 9) return -1;

    casdata = (UINT8*)malloc (caslen);
    if (!casdata)
		{
       	logerror ("cas2wav: out of memory!\n");
       	return -1;
   		}

    mame_fseek (f, 0, SEEK_SET);
 	if (caslen != mame_fread (f, casdata, caslen) ) return -1;
   	mame_fseek (f, 0, SEEK_SET);

    ret = svi_cas_to_wav (casdata, caslen, &cas_samples, &cas_len);
    if (ret == 2)
	logerror ("cas2wav: out of memory\n");
    else if (ret)
	logerror ("cas2wav: conversion error\n");

    free (casdata);

    return ret;
	}

int svi318_cassette_init(int id, mame_file *file, int open_mode)
{
	int ret;

   	/* A cassette isn't mandatory */
	if (file == NULL)
	{
		logerror("SVI318 - warning: no cassette specified!\n");
		return INIT_PASS;
	}

	if( file )
	{
		if (! is_effective_mode_create(open_mode))
		{
			struct wave_args_legacy wa = {0,};
			wa.file = file;
			/* for cas files */
			cas_samples = NULL;
			cas_len = -1;
			if (!check_svi_cas (file) )
			{
				wa.smpfreq = 22050;
				wa.fill_wave = svi318_cassette_fill_wave;
				wa.header_samples = cas_len;
				wa.trailer_samples = 0;
				wa.chunk_size = cas_len;
				wa.chunk_samples = 0;
			}
			ret = device_open(IO_CASSETTE,id,0,&wa);
			free (cas_samples);
			cas_samples = NULL;
			cas_len = -1;

			return (ret ? INIT_FAIL : INIT_PASS);
		}
		else
		{
			struct wave_args_legacy wa = {0,};
			wa.file = file;
			wa.smpfreq = 44100;
			if( device_open(IO_CASSETTE,id,1,&wa) )
				return INIT_FAIL;
			return INIT_PASS;
		}
	}
    return INIT_FAIL;
}

int svi318_cassette_present (int id)
{
	return image_exists(IO_CASSETTE, id);
}

