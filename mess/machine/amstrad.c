/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

Amstrad hardware consists of:

- General Instruments AY-3-8912 (audio and keyboard scanning)
- Intel 8255PPI (keyboard, access to AY-3-8912, cassette etc)
- Z80A CPU
- 765 FDC (disc drive interface)
- "Gate Array" (custom chip by Amstrad controlling colour, mode,
rom/ram selection


***************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "vidhrdw/m6845.h"
#include "includes/amstrad.h"
//#include "systems/i8255.h"
#include "machine/8255ppi.h"
#include "includes/nec765.h"
#include "includes/dsk.h"
#include "cassette.h"

void AmstradCPC_GA_Write(int);
void AmstradCPC_SetUpperRom(int);
void Amstrad_RethinkMemory(void);
void Amstrad_Init(void);
void amstrad_handle_snapshot(unsigned char *);


static unsigned char *snapshot = NULL;

extern unsigned char *Amstrad_Memory;
static int snapshot_loaded = 0;

int amstrad_floppy_init(int id)
{
	if (device_filename(IO_FLOPPY, id)==NULL)
		return INIT_PASS;

	return dsk_floppy_load(id);
}



/* used to setup computer if a snapshot was specified */
OPBASE_HANDLER( amstrad_opbaseoverride )
{
	/* clear op base override */
	memory_set_opbase_handler(0,0);

	if (snapshot_loaded)
	{
		/* its a snapshot file - setup hardware state */
		amstrad_handle_snapshot(snapshot);

		/* free memory */
		free(snapshot);
		snapshot = 0;

		snapshot_loaded = 0;

	}

	return (cpunum_get_pc(0) & 0x0ffff);
}

void amstrad_setup_machine(void)
{
	/* allocate ram - I control how it is accessed so I must
	allocate it somewhere - here will do */
	Amstrad_Memory = malloc(128*1024);
	if(!Amstrad_Memory) return;

	if (snapshot_loaded)
	{
		/* setup for snapshot */
		memory_set_opbase_handler(0,amstrad_opbaseoverride);
	}


	if (!snapshot_loaded)
	{
		Amstrad_Reset();
	}
}



int amstrad_cassette_init(int id)
{
	struct cassette_args args;
	memset(&args, 0, sizeof(args));
	args.create_smpfreq = 22050;	/* maybe 11025 Hz would be sufficient? */
	return cassette_init(id, &args);
}

void amstrad_cassette_exit(int id)
{
	device_close(IO_CASSETTE, id);
}


/* load CPCEMU style snapshots */
void amstrad_handle_snapshot(unsigned char *pSnapshot)
{
	int RegData;
	int i;


	/* init Z80 */
	RegData = (pSnapshot[0x011] & 0x0ff) | ((pSnapshot[0x012] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_AF, RegData);

	RegData = (pSnapshot[0x013] & 0x0ff) | ((pSnapshot[0x014] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_BC, RegData);

	RegData = (pSnapshot[0x015] & 0x0ff) | ((pSnapshot[0x016] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_DE, RegData);

	RegData = (pSnapshot[0x017] & 0x0ff) | ((pSnapshot[0x018] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_HL, RegData);

	RegData = (pSnapshot[0x019] & 0x0ff) ;
	cpunum_set_reg(0,Z80_R, RegData);

	RegData = (pSnapshot[0x01a] & 0x0ff);
	cpunum_set_reg(0,Z80_I, RegData);

	if ((pSnapshot[0x01b] & 1)==1)
	{
		cpunum_set_reg(0,Z80_IFF1, 1);
	}
	else
	{
		cpunum_set_reg(0,Z80_IFF1, 0);
	}

	if ((pSnapshot[0x01c] & 1)==1)
	{
		cpunum_set_reg(0,Z80_IFF2, 1);
	}
	else
	{
		cpunum_set_reg(0,Z80_IFF2, 0);
	}

	RegData = (pSnapshot[0x01d] & 0x0ff) | ((pSnapshot[0x01e] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_IX, RegData);

	RegData = (pSnapshot[0x01f] & 0x0ff) | ((pSnapshot[0x020] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_IY, RegData);

	RegData = (pSnapshot[0x021] & 0x0ff) | ((pSnapshot[0x022] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_SP, RegData);
	cpunum_set_sp(0,RegData);

	RegData = (pSnapshot[0x023] & 0x0ff) | ((pSnapshot[0x024] & 0x0ff)<<8);

	cpunum_set_reg(0,Z80_PC, RegData);
//	cpu_set_pc(RegData);

	RegData = (pSnapshot[0x025] & 0x0ff);
	cpunum_set_reg(0,Z80_IM, RegData);

	RegData = (pSnapshot[0x026] & 0x0ff) | ((pSnapshot[0x027] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_AF2, RegData);

	RegData = (pSnapshot[0x028] & 0x0ff) | ((pSnapshot[0x029] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_BC2, RegData);

	RegData = (pSnapshot[0x02a] & 0x0ff) | ((pSnapshot[0x02b] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_DE2, RegData);

	RegData = (pSnapshot[0x02c] & 0x0ff) | ((pSnapshot[0x02d] & 0x0ff)<<8);
	cpunum_set_reg(0,Z80_HL2, RegData);

	/* init GA */
	for (i=0; i<17; i++)
	{
		AmstradCPC_GA_Write(i);

		AmstradCPC_GA_Write(((pSnapshot[0x02f + i] & 0x01f) | 0x040));
	}

	AmstradCPC_GA_Write(pSnapshot[0x02e] & 0x01f);

	AmstradCPC_GA_Write(((pSnapshot[0x040] & 0x03f) | 0x080));

	AmstradCPC_GA_Write(((pSnapshot[0x041] & 0x03f) | 0x0c0));

	/* init CRTC */
	for (i=0; i<18; i++)
	{
                crtc6845_address_w(0,i);
                crtc6845_register_w(0, pSnapshot[0x043+i] & 0x0ff);
	}

    crtc6845_address_w(0,i);

	/* upper rom selection */
	AmstradCPC_SetUpperRom(pSnapshot[0x055]);

	/* PPI */
	ppi8255_w(0,3,pSnapshot[0x059] & 0x0ff);

	ppi8255_w(0,0,pSnapshot[0x056] & 0x0ff);
	ppi8255_w(0,1,pSnapshot[0x057] & 0x0ff);
	ppi8255_w(0,2,pSnapshot[0x058] & 0x0ff);

	/* PSG */
	for (i=0; i<16; i++)
	{
		AY8910_control_port_0_w(0,i);

		AY8910_write_port_0_w(0,pSnapshot[0x05b + i] & 0x0ff);
	}

	AY8910_control_port_0_w(0,pSnapshot[0x05a]);

	{
		int MemSize;
		int MemorySize;

		MemSize = (pSnapshot[0x06b] & 0x0ff) | ((pSnapshot[0x06c] & 0x0ff)<<8);

		if (MemSize==128)
		{
			MemorySize = 128*1024;
		}
		else
		{
			MemorySize = 64*1024;
		}

		memcpy(Amstrad_Memory, &pSnapshot[0x0100], MemorySize);

	}

	Amstrad_RethinkMemory();

}

/* load image */
int amstrad_load(int type, int id, unsigned char **ptr)
{
	void *file;

	file = image_fopen(type, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_READ);

	if (file)
	{
		int datasize;
		unsigned char *data;

		/* get file size */
		datasize = osd_fsize(file);

		if (datasize!=0)
		{
			/* malloc memory for this data */
			data = malloc(datasize);

			if (data!=NULL)
			{
				/* read whole file */
				osd_fread(file, data, datasize);

				*ptr = data;

				/* close file */
				osd_fclose(file);

				logerror("File loaded!\r\n");

				/* ok! */
				return 1;
			}
			osd_fclose(file);

		}
	}

	return 0;
}

/* load snapshot */
int amstrad_snapshot_load(int id)
{
	/* machine can be started without a snapshot */
	/* if filename not specified, then init is ok */
	if (device_filename(IO_SNAPSHOT, id)==NULL)
		return INIT_PASS;

	/* filename specified */
	snapshot_loaded = 0;

	/* load and verify image */
	if (amstrad_load(IO_SNAPSHOT,id,&snapshot))
	{
		snapshot_loaded = 1;
		if (memcmp(snapshot, "MV - SNA", 8)==0)
			return INIT_PASS;
		else
			return INIT_FAIL;
	}

	return INIT_FAIL;
}

void amstrad_snapshot_exit(int id)
{
	if (snapshot!=NULL)
		free(snapshot);

	snapshot_loaded = 0;

}

int	amstrad_plus_cartridge_init(int id)
{
	/* cpc+ requires a cartridge to be inserted to run */
	if (device_filename(IO_CARTSLOT, id)==NULL)
		return INIT_FAIL;

	return INIT_PASS;
}

void amstrad_plus_cartridge_exit(int id)
{



}

