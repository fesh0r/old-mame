/***************************************************************************

	commodore vic20 home computer
	Peter Trauner
	(peter.trauner@jk.uni-linz.ac.at)

    documentation
     Marko.Makela@HUT.FI (vic6560)
     www.funet.fi

***************************************************************************/
#include <ctype.h>
#include <stdio.h>

#include "driver.h"
#include "osd_cpu.h"
#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"

#include "includes/vc20.h"
#include "includes/vc1541.h"
#include "machine/6522via.h"
#include "includes/vc20tape.h"
#include "includes/cbmserb.h"
#include "includes/cbmieeeb.h"
#include "includes/vic6560.h"

static UINT8 keyboard[8] =
{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static int via1_portb, via1_porta, via0_ca2;
static int serial_atn = 1, serial_clock = 1, serial_data = 1;

static int ieee=0; /* ieee cartridge (interface and rom)*/

UINT8 *vc20_memory;
UINT8 *vc20_memory_9400;

/** via 0 addr 0x9110
 ca1 restore key (low)
 ca2 cassette motor on
 pa0 serial clock in
 pa1 serial data in
 pa2 also joy 0 in
 pa3 also joy 1 in
 pa4 also joy 2 in
 pa5 also joybutton/lightpen in
 pa6 cassette switch in
 pa7 inverted serial atn out
 pa2 till pa6, port b, cb1, cb2 userport
 irq connected to m6502 nmi
*/
static void vc20_via0_irq (int level)
{
	cpu_set_nmi_line (0, level);
}

static READ_HANDLER( vc20_via0_read_ca1 )
{
	return !KEY_RESTORE;
}

static READ_HANDLER( vc20_via0_read_ca2 )
{
	DBG_LOG (1, "tape", ("motor read %d\n", via0_ca2));
	return via0_ca2;
}

static WRITE_HANDLER( vc20_via0_write_ca2 )
{
	via0_ca2 = data ? 1 : 0;
	vc20_tape_motor (via0_ca2);
}

static READ_HANDLER( vc20_via0_read_porta )
{
	UINT8 value = 0xff;

	if (JOYSTICK) {
		if (JOYSTICK_BUTTON) value&=~0x20;
		if (JOYSTICK_LEFT) value&=~0x10;
		if (JOYSTICK_DOWN) value&=~0x8;
		if (JOYSTICK_UP) value&=~0x4;
	}
	if (PADDLES) {
		if (PADDLE1_BUTTON) value&=~0x10;
	}
	/* to short to be recognized normally */
	/* should be reduced to about 1 or 2 microseconds */
	/*  if(LIGHTPEN_BUTTON) value&=~0x20; */
	if (!serial_clock || !cbm_serial_clock_read ())
		value &= ~1;
	if (!serial_data || !cbm_serial_data_read ())
		value &= ~2;
	if (!vc20_tape_switch ())
		value &= ~0x40;
	return value;
}

static WRITE_HANDLER( vc20_via0_write_porta )
{
	cbm_serial_atn_write (serial_atn = !(data & 0x80));
	DBG_LOG (1, "serial out", ("atn %s\n", serial_atn ? "high" : "low"));
}

/* via 1 addr 0x9120
 * port a input from keyboard (low key pressed in line of matrix
 * port b select lines of keyboard matrix (low)
 * pb7 also joystick right in! (low)
 * pb3 also cassette write
 * ca1 cassette read
 * ca2 inverted serial clk out
 * cb1 serial srq in
 * cb2 inverted serial data out
 * irq connected to m6502 irq
 */
static void vc20_via1_irq (int level)
{
	cpu_set_irq_line (0, M6502_IRQ_LINE, level);
}

static READ_HANDLER( vc20_via1_read_porta )
{
	int value = 0xff;

	if (!(via1_portb & 0x01))
		value &= keyboard[0];

	if (!(via1_portb & 0x02))
		value &= keyboard[1];

	if (!(via1_portb & 0x04))
		value &= keyboard[2];

	if (!(via1_portb & 0x08))
		value &= keyboard[3];

	if (!(via1_portb & 0x10))
		value &= keyboard[4];

	if (!(via1_portb & 0x20))
		value &= keyboard[5];

	if (!(via1_portb & 0x40))
		value &= keyboard[6];

	if (!(via1_portb & 0x80))
		value &= keyboard[7];

	return value;
}

static READ_HANDLER( vc20_via1_read_ca1 )
{
	return vc20_tape_read ();
}

static WRITE_HANDLER( vc20_via1_write_ca2 )
{
	cbm_serial_clock_write (serial_clock = !data);
}

static READ_HANDLER( vc20_via1_read_portb )
{
	UINT8 value = 0xff;

    if (!(via1_porta&0x80)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x80)) t&=~0x80;
	if (!(keyboard[6]&0x80)) t&=~0x40;
	if (!(keyboard[5]&0x80)) t&=~0x20;
	if (!(keyboard[4]&0x80)) t&=~0x10;
	if (!(keyboard[3]&0x80)) t&=~0x08;
	if (!(keyboard[2]&0x80)) t&=~0x04;
	if (!(keyboard[1]&0x80)) t&=~0x02;
	if (!(keyboard[0]&0x80)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x40)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x40)) t&=~0x80;
	if (!(keyboard[6]&0x40)) t&=~0x40;
	if (!(keyboard[5]&0x40)) t&=~0x20;
	if (!(keyboard[4]&0x40)) t&=~0x10;
	if (!(keyboard[3]&0x40)) t&=~0x08;
	if (!(keyboard[2]&0x40)) t&=~0x04;
	if (!(keyboard[1]&0x40)) t&=~0x02;
	if (!(keyboard[0]&0x40)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x20)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x20)) t&=~0x80;
	if (!(keyboard[6]&0x20)) t&=~0x40;
	if (!(keyboard[5]&0x20)) t&=~0x20;
	if (!(keyboard[4]&0x20)) t&=~0x10;
	if (!(keyboard[3]&0x20)) t&=~0x08;
	if (!(keyboard[2]&0x20)) t&=~0x04;
	if (!(keyboard[1]&0x20)) t&=~0x02;
	if (!(keyboard[0]&0x20)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x10)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x10)) t&=~0x80;
	if (!(keyboard[6]&0x10)) t&=~0x40;
	if (!(keyboard[5]&0x10)) t&=~0x20;
	if (!(keyboard[4]&0x10)) t&=~0x10;
	if (!(keyboard[3]&0x10)) t&=~0x08;
	if (!(keyboard[2]&0x10)) t&=~0x04;
	if (!(keyboard[1]&0x10)) t&=~0x02;
	if (!(keyboard[0]&0x10)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x08)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x08)) t&=~0x80;
	if (!(keyboard[6]&0x08)) t&=~0x40;
	if (!(keyboard[5]&0x08)) t&=~0x20;
	if (!(keyboard[4]&0x08)) t&=~0x10;
	if (!(keyboard[3]&0x08)) t&=~0x08;
	if (!(keyboard[2]&0x08)) t&=~0x04;
	if (!(keyboard[1]&0x08)) t&=~0x02;
	if (!(keyboard[0]&0x08)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x04)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x04)) t&=~0x80;
	if (!(keyboard[6]&0x04)) t&=~0x40;
	if (!(keyboard[5]&0x04)) t&=~0x20;
	if (!(keyboard[4]&0x04)) t&=~0x10;
	if (!(keyboard[3]&0x04)) t&=~0x08;
	if (!(keyboard[2]&0x04)) t&=~0x04;
	if (!(keyboard[1]&0x04)) t&=~0x02;
	if (!(keyboard[0]&0x04)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x02)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x02)) t&=~0x80;
	if (!(keyboard[6]&0x02)) t&=~0x40;
	if (!(keyboard[5]&0x02)) t&=~0x20;
	if (!(keyboard[4]&0x02)) t&=~0x10;
	if (!(keyboard[3]&0x02)) t&=~0x08;
	if (!(keyboard[2]&0x02)) t&=~0x04;
	if (!(keyboard[1]&0x02)) t&=~0x02;
	if (!(keyboard[0]&0x02)) t&=~0x01;
	value &=t;
    }
    if (!(via1_porta&0x01)) {
	UINT8 t=0xff;
	if (!(keyboard[7]&0x01)) t&=~0x80;
	if (!(keyboard[6]&0x01)) t&=~0x40;
	if (!(keyboard[5]&0x01)) t&=~0x20;
	if (!(keyboard[4]&0x01)) t&=~0x10;
	if (!(keyboard[3]&0x01)) t&=~0x08;
	if (!(keyboard[2]&0x01)) t&=~0x04;
	if (!(keyboard[1]&0x01)) t&=~0x02;
	if (!(keyboard[0]&0x01)) t&=~0x01;
	value &=t;
    }

	if (JOYSTICK) {
		if (JOYSTICK_RIGHT) value&=~0x80;
	}
	if (PADDLES) {
		if (PADDLE2_BUTTON) value&=~0x80;
	}

	return value;
}

static WRITE_HANDLER( vc20_via1_write_porta )
{
	via1_porta = data;
}


static WRITE_HANDLER( vc20_via1_write_portb )
{
/*  if( errorlog ) fprintf(errorlog, "via1_write_portb: $%02X\n", data); */
	vc20_tape_write (data & 8 ? 1 : 0);
	via1_portb = data;
}

static READ_HANDLER( vc20_via1_read_cb1 )
{
	DBG_LOG (1, "serial in", ("request read\n"));
	return cbm_serial_request_read ();
}

static WRITE_HANDLER( vc20_via1_write_cb2 )
{
	cbm_serial_data_write (serial_data = !data);
}

/* ieee 6522 number 1 (via4)
 port b
  0 dav out
  1 nrfd out
  2 ndac out (or port a pin 2!?)
  3 eoi in
  4 dav in
  5 nrfd in
  6 ndac in
  7 atn in
 */
static READ_HANDLER( vc20_via4_read_portb )
{
	UINT8 data=0;
	if (cbm_ieee_eoi_r()) data|=8;
	if (cbm_ieee_dav_r()) data|=0x10;
	if (cbm_ieee_nrfd_r()) data|=0x20;
	if (cbm_ieee_ndac_r()) data|=0x40;
	if (cbm_ieee_atn_r()) data|=0x80;
	return data;
}

static WRITE_HANDLER( vc20_via4_write_portb )
{
	cbm_ieee_dav_w(0,data&1);
	cbm_ieee_nrfd_w(0,data&2);
	cbm_ieee_ndac_w(0,data&4);
}

/* ieee 6522 number 2 (via5)
   port a data read
   port b data write
   cb1 srq in ?
   cb2 eoi out
   ca2 atn out
*/
static WRITE_HANDLER( vc20_via5_write_porta )
{
	cbm_ieee_data_w(0,data);
}

static READ_HANDLER( vc20_via5_read_portb )
{
	return cbm_ieee_data_r();
}

static WRITE_HANDLER( vc20_via5_write_ca2 )
{
	cbm_ieee_atn_w(0,data);
}

static READ_HANDLER( vc20_via5_read_cb1 )
{
	return cbm_ieee_srq_r();
}

static WRITE_HANDLER( vc20_via5_write_cb2 )
{
	cbm_ieee_eoi_w(0,data);
}

static struct via6522_interface via0 =
{
	vc20_via0_read_porta,
	0,								   /*via0_read_portb, */
	vc20_via0_read_ca1,
	0,								   /*via0_read_cb1, */
	vc20_via0_read_ca2,
	0,								   /*via0_read_cb2, */
	vc20_via0_write_porta,
	0,								   /*via0_write_portb, */
	vc20_via0_write_ca2,
	0,								   /*via0_write_cb2, */
	vc20_via0_irq
}, via1 =
{
	vc20_via1_read_porta,
	vc20_via1_read_portb,
	vc20_via1_read_ca1,
	vc20_via1_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via1_write_porta,								   /*via1_write_porta, */
	vc20_via1_write_portb,
	vc20_via1_write_ca2,
	vc20_via1_write_cb2,
	vc20_via1_irq
},
/* via2,3 used by vc1541 and 2031 disk drives */
via4 =
{
	0, /*vc20_via4_read_porta, */
	vc20_via4_read_portb,
	0, /*vc20_via4_read_ca1, */
	0, /*vc20_via5_read_cb1, */
	0, /* via1_read_ca2 */
	0,								   /*via1_read_cb2, */
	0,								   /*via1_write_porta, */
	vc20_via4_write_portb,
	0, /*vc20_via5_write_ca2, */
	0, /*vc20_via5_write_cb2, */
	vc20_via1_irq
}, via5 =
{
	0,/*vc20_via5_read_porta, */
	vc20_via5_read_portb,
	0, /*vc20_via5_read_ca1, */
	vc20_via5_read_cb1,
	0,								   /*via1_read_ca2, */
	0,								   /*via1_read_cb2, */
	vc20_via5_write_porta,
	0,/*vc20_via5_write_portb, */
	vc20_via5_write_ca2,
	vc20_via5_write_cb2,
	vc20_via1_irq
};

WRITE_HANDLER ( vc20_write_9400 )
{
	vc20_memory_9400[offset] = data | 0xf0;
}


int vic6560_dma_read_color (int offset)
{
	return vc20_memory_9400[offset & 0x3ff];
}

int vic6560_dma_read (int offset)
{
	/* should read real system bus between 0x9000 and 0xa000 */
	return vc20_memory[VIC6560ADDR2VC20ADDR (offset)];
}

static void vc20_memory_init(void)
{
	static int inited=0;
	UINT8 *memory = memory_region (REGION_CPU1);

	if (inited) return;
	/* power up values are random (more likely bit set)
	   measured in cost reduced german vc20
	   6116? 2kbx8 ram in main area
	   2114 1kbx4 ram at non used color ram area 0x9400 */

	memset(memory, 0, 0x400);
	memset(memory+0x1000, 0, 0x1000);

	memory[0x288]=0xff;// makes ae's graphics look correctly
	memory[0xd]=0xff; // for moneywars
	memory[0x1046]=0xff; // for jelly monsters, cosmic cruncher;

#if 0
	/* 2114 poweron ? 64 x 0xff, 64x 0, and so on */
	for (i = 0; i < 0x400; i += 0x40)
	{
		memset (memory + i, i & 0x40 ? 0 : 0xff, 0x40);
		memset (memory+0x9400 + i, 0xf0 | (i & 0x40 ? 0 : 0xf), 0x40);
	}
	// this would be the straight forward memory init for
	// non cost reduced vic20 (2114 rams)
	for (i=0x1000;i<0x2000;i+=0x40) memset(memory+i,i&0x40?0:0xff,0x40);
#endif

	/* i think roms look like 0xff */
	// german cost reduced vc20 reads back highbyte of address
	// when empty!
	memset (memory + 0x400, 0xff, 0x1000 - 0x400);
	memset (memory + 0x2000, 0xff, 0x6000);
	memset (memory + 0xa000, 0xff, 0x1000);

	/* clears ieee cartrige rom */
	/* memset (memory + 0xa000, 0xff, 0x2000); */

	inited=1;
}

static void vc20_common_driver_init (void)
{
#ifdef VC1541
	VC1541_CONFIG vc1541= { 1, 8 };
#endif
	vc20_memory_init();

	vc20_tape_open (via_1_ca1_w);

	cbm_drive_open ();

	cbm_drive_attach_fs (0);
	cbm_drive_attach_fs (1);

#ifdef VC1541
	vc1541_config (0, 0, &vc1541);
#endif
	via_config (0, &via0);
	via_config (1, &via1);
}

/* currently not used, but when time comes */
void vc20_driver_shutdown (void)
{
	cbm_drive_close ();
	vc20_tape_close ();
}

void vc20_driver_init (void)
{
	vc20_common_driver_init ();
	vic6561_init (vic6560_dma_read, vic6560_dma_read_color);
}

void vic20_driver_init (void)
{
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
}

extern void vic20ieee_driver_init (void)
{
	ieee=1;
	vc20_common_driver_init ();
	vic6560_init (vic6560_dma_read, vic6560_dma_read_color);
	via_config (4, &via4);
	via_config (5, &via5);
	cbm_ieee_open();
}

void vc20_init_machine (void)
{
	if (RAMIN0X0400)
	{
		install_mem_write_handler (0, 0x400, 0xfff, MWA_RAM);
		install_mem_read_handler (0, 0x400, 0xfff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x400, 0xfff, MWA_NOP);
		install_mem_read_handler (0, 0x400, 0xfff, MRA_ROM);
	}
	if (RAMIN0X2000)
	{
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_RAM);
		install_mem_read_handler (0, 0x2000, 0x3fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x2000, 0x3fff, MWA_NOP);
		install_mem_read_handler (0, 0x2000, 0x3fff, MRA_ROM);
	}
	if (RAMIN0X4000)
	{
		install_mem_write_handler (0, 0x4000, 0x5fff, MWA_RAM);
		install_mem_read_handler (0, 0x4000, 0x5fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x4000, 0x5fff, MWA_NOP);
		install_mem_read_handler (0, 0x4000, 0x5fff, MRA_ROM);
	}
	if (RAMIN0X6000)
	{
		install_mem_write_handler (0, 0x6000, 0x7fff, MWA_RAM);
		install_mem_read_handler (0, 0x6000, 0x7fff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0x6000, 0x7fff, MWA_NOP);
		install_mem_read_handler (0, 0x6000, 0x7fff, MRA_ROM);
	}
	if (ieee)
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_ROM);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_ROM);
	}
	else if	(RAMIN0XA000)
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_RAM);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_RAM);
	}
	else
	{
		install_mem_write_handler (0, 0xa000, 0xbfff, MWA_NOP);
		install_mem_read_handler (0, 0xa000, 0xbfff, MRA_ROM);
	}

	cbm_serial_reset_write (0);
#ifdef VC1541
	vc1541_reset ();
#endif
	if (ieee) {
		cbm_drive_0_config (SERIAL8ON ? IEEE : 0, 8);
		cbm_drive_1_config (SERIAL9ON ? IEEE : 0, 9);
	} else {
		cbm_drive_0_config (SERIAL8ON ? SERIAL : 0, 8);
		cbm_drive_1_config (SERIAL9ON ? SERIAL : 0, 9);
	}
	via_reset ();
	via_0_ca1_w (0, vc20_via0_read_ca1(0) );
}

void vc20_shutdown_machine (void)
{
}

static int vc20_rom_id (int id)
{
	FILE *romfile;
	unsigned char magic[] =
	{0x41, 0x30, 0x20, 0xc3, 0xc2, 0xcd};	/* A0 CBM at 0xa004 (module offset 4) */
	unsigned char buffer[sizeof (magic)];
	char *cp;
	int retval;

	logerror("vc20_rom_id %s\n", device_filename(IO_CARTSLOT,id));
	if (!(romfile = (FILE*)image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0)))
	{
		logerror("rom %s not found\n", device_filename(IO_CARTSLOT,id));
		return 0;
	}

	retval = 0;

	osd_fseek (romfile, 4, SEEK_SET);
	osd_fread (romfile, buffer, sizeof (magic));
	osd_fclose (romfile);

	if (!memcmp (buffer, magic, sizeof (magic)))
		retval = 1;

	if ((cp = strrchr (device_filename(IO_CARTSLOT,id), '.')) != NULL)
	{
		if ((stricmp (cp + 1, "a0") == 0)
			|| (stricmp (cp + 1, "20") == 0)
			|| (stricmp (cp + 1, "40") == 0)
			|| (stricmp (cp + 1, "60") == 0)
			|| (stricmp (cp + 1, "bin") == 0)
			|| (stricmp (cp + 1, "rom") == 0)
			|| (stricmp (cp + 1, "prg") == 0))
			retval = 1;
	}

		if (retval)
			logerror("rom %s recognized\n", device_filename(IO_CARTSLOT,id));
		else
			logerror("rom %s not recognized\n", device_filename(IO_CARTSLOT,id));

	return retval;
}

int vc20_rom_load (int id)
{
	UINT8 *mem = memory_region (REGION_CPU1);
	FILE *fp;
	int size, read;
	char *cp;
	int addr = 0;

	vc20_memory_init();

	if (device_filename(IO_CARTSLOT,id)==NULL) return 0;

	if (!vc20_rom_id (id))
		return 1;
	fp = (FILE*)image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0);
	if (!fp)
	{
		logerror("%s file not found\n", device_filename(IO_CARTSLOT,id));
		return 1;
	}

	size = osd_fsize (fp);

	if ((cp = strrchr (device_filename(IO_CARTSLOT,id), '.')) != NULL)
	{
		if ((cp[1] != 0) && (cp[2] == '0') && (cp[3] == 0))
		{
			switch (toupper (cp[1]))
			{
			case 'A':
				addr = 0xa000;
				break;
			case '2':
				addr = 0x2000;
				break;
			case '4':
				addr = 0x4000;
				break;
			case '6':
				addr = 0x6000;
				break;
			}
		}
		else
		{
			if (stricmp (cp, ".prg") == 0)
			{
				unsigned short in;

				osd_fread_lsbfirst (fp, &in, 2);
				logerror("rom prg %.4x\n", in);
				addr = in;
				size -= 2;
			}
		}
	}
	if (addr == 0)
	{
		if (size == 0x4000)
		{							   /* I think rom at 0x4000 */
			addr = 0x4000;
		}
		else
		{
			addr = 0xa000;
		}
	}

	logerror("loading rom %s at %.4x size:%.4x\n",device_filename(IO_CARTSLOT,id), addr, size);
	read = osd_fread (fp, mem + addr, size);
	osd_fclose (fp);
	if (read != size)
		return 1;
	return 0;
}


int vc20_frame_interrupt (void)
{
	static int quickload = 0;

	if (!quickload && QUICKLOAD)
		cbm_quick_open (0, 0, vc20_memory);
	quickload = QUICKLOAD;

	via_0_ca1_w (0, vc20_via0_read_ca1 (0));
	keyboard[0] = 0xff;
	if (KEY_DEL) keyboard[0]&=~0x80;
	if (KEY_POUND) keyboard[0]&=~0x40;
	if (KEY_PLUS) keyboard[0]&=~0x20;
	if (KEY_9) keyboard[0]&=~0x10;
	if (KEY_7) keyboard[0]&=~0x08;
	if (KEY_5) keyboard[0]&=~0x04;
	if (KEY_3) keyboard[0]&=~0x02;
	if (KEY_1) keyboard[0]&=~0x01;

	keyboard[1] = 0xff;
	if (KEY_RETURN) keyboard[1]&=~0x80;
	if (KEY_ASTERIX) keyboard[1]&=~0x40;
	if (KEY_P) keyboard[1]&=~0x20;
	if (KEY_I) keyboard[1]&=~0x10;
	if (KEY_Y) keyboard[1]&=~0x08;
	if (KEY_R) keyboard[1]&=~0x04;
	if (KEY_W) keyboard[1]&=~0x02;
	if (KEY_ARROW_LEFT) keyboard[1]&=~0x01;

	keyboard[2] = 0xff;
	if (KEY_RIGHT) keyboard[2]&=~0x80;
	if (KEY_SEMICOLON) keyboard[2]&=~0x40;
	if (KEY_L) keyboard[2]&=~0x20;
	if (KEY_J) keyboard[2]&=~0x10;
	if (KEY_G) keyboard[2]&=~0x08;
	if (KEY_D) keyboard[2]&=~0x04;
	if (KEY_A) keyboard[2]&=~0x02;
	if (KEY_CTRL) keyboard[2]&=~0x01;

	keyboard[3] = 0xff;
	if (KEY_DOWN) keyboard[3]&=~0x80;
	if (KEY_SLASH) keyboard[3]&=~0x40;
	if (KEY_COMMA) keyboard[3]&=~0x20;
	if (KEY_N) keyboard[3]&=~0x10;
	if (KEY_V) keyboard[3]&=~0x08;
	if (KEY_X) keyboard[3]&=~0x04;
	if (KEY_LEFT_SHIFT) keyboard[3]&=~0x02;
	if (KEY_STOP) keyboard[3]&=~0x01;

	keyboard[4] = 0xff;
	if (KEY_F1) keyboard[4]&=~0x80;
	if (KEY_RIGHT_SHIFT) keyboard[4]&=~0x40;
	if (KEY_POINT) keyboard[4]&=~0x20;
	if (KEY_M) keyboard[4]&=~0x10;
	if (KEY_B) keyboard[4]&=~0x08;
	if (KEY_C) keyboard[4]&=~0x04;
	if (KEY_Z) keyboard[4]&=~0x02;
	if (KEY_SPACE) keyboard[4]&=~0x01;

	keyboard[5] = 0xff;
	if (KEY_F3) keyboard[5]&=~0x80;
	if (KEY_EQUALS) keyboard[5]&=~0x40;
	if (KEY_COLON) keyboard[5]&=~0x20;
	if (KEY_K) keyboard[5]&=~0x10;
	if (KEY_H) keyboard[5]&=~0x08;
	if (KEY_F) keyboard[5]&=~0x04;
	if (KEY_S) keyboard[5]&=~0x02;
	if (KEY_CBM) keyboard[5]&=~0x01;

	keyboard[6] = 0xff;
	if (KEY_F5) keyboard[6]&=~0x80;
	if (KEY_ARROW_UP) keyboard[6]&=~0x40;
	if (KEY_AT) keyboard[6]&=~0x20;
	if (KEY_O) keyboard[6]&=~0x10;
	if (KEY_U) keyboard[6]&=~0x08;
	if (KEY_T) keyboard[6]&=~0x04;
	if (KEY_E) keyboard[6]&=~0x02;
	if (KEY_Q) keyboard[6]&=~0x01;

	keyboard[7] = 0xff;
	if (KEY_F7) keyboard[7]&=~0x80;
	if (KEY_HOME) keyboard[7]&=~0x40;
	if (KEY_MINUS) keyboard[7]&=~0x20;
	if (KEY_0) keyboard[7]&=~0x10;
	if (KEY_8) keyboard[7]&=~0x08;
	if (KEY_6) keyboard[7]&=~0x04;
	if (KEY_4) keyboard[7]&=~0x02;
	if (KEY_2) keyboard[7]&=~0x01;

	vc20_tape_config (DATASSETTE, DATASSETTE_TONE);
	vc20_tape_buttons (DATASSETTE_PLAY, DATASSETTE_RECORD, DATASSETTE_STOP);
	set_led_status (1 /*KB_CAPSLOCK_FLAG */ , KEY_SHIFT_LOCK ? 1 : 0);

	return ignore_interrupt ();
}
