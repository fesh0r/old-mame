/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

  References:
		There are two main references for the info for this driver
		- Tandy Color Computer Unravelled Series
					(http://www.giftmarket.org/unravelled/unravelled.shtml)
		- Assembly Language Programming For the CoCo 3 by Laurence A. Tepolt
		- Kevin K. Darlings GIME reference
					(http://www.cris.com/~Alxevans/gime.txt)
		- Sock Masters's GIME register reference
					(http://www.axess.com/twilight/sock/gime.html)
		- Robert Gault's FAQ
					(http://home.att.net/~robert.gault/Coco/FAQ/FAQ_main.htm)
		- Discussions with L. Curtis Boyle (LCB) and John Kowalski (JK)

  TODO:
		- Implement unimplemented SAM registers
		- Implement unimplemented interrupts (serial)
		- Choose and implement more appropriate ratios for the speed up poke
		- Handle resets correctly

  In the CoCo, all timings should be exactly relative to each other.  This
  table shows how all clocks are relative to each other (info: JK):
		- Main CPU Clock				0.89 MHz
		- Horizontal Sync Interrupt		15.7 kHz/63.5us	(57 clock cycles)
		- Vertical Sync Interrupt		60 Hz			(14934 clock cycles)
		- Composite Video Color Carrier	3.58 MHz/279ns	(1/4 clock cycles)

  It is also noting that the CoCo 3 had two sets of VSync interrupts.  To quote
  John Kowalski:

	One other thing to mention is that the old vertical interrupt and the new
	vertical interrupt are not the same..  The old one is triggered by the
	video's vertical sync pulse, but the new one is triggered on the next scan
	line *after* the last scan line of the active video display.  That is : new
	vertical interrupt triggers somewheres around scan line 230 of the 262 line
	screen (if a 200 line graphics mode is used, a bit earlier if a 192 line
	mode is used and a bit later if a 225 line mode is used).  The old vsync
	interrupt triggers on scanline zero.

	230 is just an estimate [(262-200)/2+200].  I don't think the active part
	of the screen is exactly centered within the 262 line total.  I can
	research that for you if you want an exact number for scanlines before the
	screen starts and the scanline that the v-interrupt triggers..etc.
***************************************************************************/

#include <math.h>
#include <assert.h>

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "includes/rstrtrck.h"
#include "vidhrdw/m6847.h"
#include "includes/dragon.h"
#include "formats/cocopak.h"
#include "formats/cococas.h"
#include "includes/6883sam.h"
#include "includes/cococart.h"
#include "cassette.h"

static UINT8 *coco_rom;
static int coco3_enable_64k;
static int coco3_mmu[16];
static int coco3_gimereg[8];
static int coco3_interupt_line;
static int pia0_irq_a, pia0_irq_b;
static int pia1_firq_a, pia1_firq_b;
static int gime_firq, gime_irq;
static int cart_line, cart_inserted;
static UINT8 pia0_pb, pia1_pb1, soundmux_status, tape_motor;
static UINT8 joystick_axis, joystick;
static int d_dac;

static WRITE_HANDLER ( d_pia1_pb_w );
static WRITE_HANDLER ( coco3_pia1_pb_w );
static WRITE_HANDLER ( d_pia1_pa_w );
static READ_HANDLER (  d_pia1_cb1_r );
static READ_HANDLER (  d_pia0_ca1_r );
static READ_HANDLER (  d_pia0_cb1_r );
static READ_HANDLER (  d_pia0_pa_r );
static READ_HANDLER (  d_pia1_pa_r );
static READ_HANDLER (  d_pia1_pb_r_dragon );
static READ_HANDLER (  d_pia1_pb_r_coco2 );
static WRITE_HANDLER ( d_pia0_pa_w );
static WRITE_HANDLER ( d_pia0_pb_w );
static WRITE_HANDLER ( d_pia1_cb2_w);
static WRITE_HANDLER ( d_pia0_cb2_w);
static WRITE_HANDLER ( d_pia1_ca2_w);
static WRITE_HANDLER ( d_pia0_ca2_w);
static void d_pia0_irq_a(int state);
static void d_pia0_irq_b(int state);
static void d_pia1_firq_a(int state);
static void d_pia1_firq_b(int state);
static void coco3_pia0_irq_a(int state);
static void coco3_pia0_irq_b(int state);
static void coco3_pia1_firq_a(int state);
static void coco3_pia1_firq_b(int state);
static void coco_cartridge_enablesound(int enable);
static void d_sam_set_pageonemode(int val);
static void d_sam_set_mpurate(int val);
static void d_sam_set_memorysize(int val);
static void d_sam_set_maptype(int val);
static void coco3_sam_set_maptype(int val);
static void coco_setcartline(int data);
static void coco3_setcartline(int data);

/* These sets of defines control logging.  When MAME_DEBUG is off, all logging
 * is off.  There is a different set of defines for when MAME_DEBUG is on so I
 * don't have to worry abount accidently committing a version of the driver
 * with logging enabled after doing some debugging work
 *
 * Logging options marked as "Sparse" are logging options that happen rarely,
 * and should not get in the way.  As such, these options are always on
 * (assuming MAME_DEBUG is on).  "Frequent" options are options that happen
 * enough that they might get in the way.
 */
#ifdef MAME_DEBUG
#define LOG_PAK			1	/* [Sparse]   Logging on PAK trailers */
#define LOG_INT_MASKING	1	/* [Sparse]   Logging on changing GIME interrupt masks */
#define LOG_CASSETTE	1	/* [Sparse]   Logging when cassette motor changes state */
#define LOG_TIMER_SET	1	/* [Sparse]   Logging when setting the timer */
#define LOG_INT_TMR		0	/* [Frequent] Logging when timer interrupt is invoked */
#define LOG_FLOPPY		0	/* [Frequent] Set when floppy interrupts occur */
#define LOG_INT_COCO3	0
#define LOG_GIME		0
#define LOG_MMU			0
#define LOG_VBORD		0   /* [Frequent] Occurs when VBORD is changed */
#define LOG_OS9         0
#define LOG_TIMER       0
#define LOG_DEC_TIMER	0
#define LOG_IRQ_RECALC	0
#else /* !MAME_DEBUG */
#define LOG_PAK			0
#define LOG_CASSETTE	0
#define LOG_INT_MASKING	0
#define LOG_TIMER_SET   0
#define LOG_INT_TMR		0
#define LOG_INT_COCO3	0
#define LOG_GIME		0
#define LOG_MMU			0
#define LOG_VBORD		0
#define LOG_OS9         0
#define LOG_FLOPPY		0
#define LOG_TIMER       0
#define LOG_DEC_TIMER	0
#define LOG_IRQ_RECALC	0
#endif /* MAME_DEBUG */

static void coco3_timer_hblank(void);
static int count_bank(void);
static int is_Orch90(void);

static struct pia6821_interface dragon_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia0_pa_r, 0, d_pia0_ca1_r, d_pia0_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_dragon, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	}
};

static struct pia6821_interface coco2_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia0_pa_r, 0, d_pia0_ca1_r, d_pia0_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ d_pia0_irq_a, d_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, d_pia1_pb_r_coco2, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, d_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ d_pia1_firq_a, d_pia1_firq_b
	}
};

static struct pia6821_interface coco3_pia_intf[] =
{
	/* PIA 0 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia0_pa_r, d_pia1_pb_r_coco2, d_pia0_ca1_r, d_pia0_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia0_pa_w, d_pia0_pb_w, d_pia0_ca2_w, d_pia0_cb2_w,
		/*irqs	 : A/B			   */ coco3_pia0_irq_a, coco3_pia0_irq_b
	},

	/* PIA 1 */
	{
		/*inputs : A/B,CA/B1,CA/B2 */ d_pia1_pa_r, 0, 0, d_pia1_cb1_r, 0, 0,
		/*outputs: A/B,CA/B2	   */ d_pia1_pa_w, coco3_pia1_pb_w, d_pia1_ca2_w, d_pia1_cb2_w,
		/*irqs	 : A/B			   */ coco3_pia1_firq_a, coco3_pia1_firq_b
	}
};

static struct sam6883_interface dragon_sam_intf =
{
	m6847_set_row_height,
	m6847_set_video_offset,
	d_sam_set_pageonemode,
	d_sam_set_mpurate,
	d_sam_set_memorysize,
	NULL
};

static struct sam6883_interface dragon64_sam_intf =
{
	m6847_set_row_height,
	m6847_set_video_offset,
	d_sam_set_pageonemode,
	d_sam_set_mpurate,
	d_sam_set_memorysize,
	d_sam_set_maptype
};

static struct sam6883_interface coco3_sam_intf =
{
	NULL,
	m6847_set_video_offset,
	NULL,
	d_sam_set_mpurate,
	NULL,
	coco3_sam_set_maptype
};

static const struct cartridge_slot *coco_cart_interface;

/***************************************************************************
  PAK files

  PAK files were originally for storing Program PAKs, but the file format has
  evolved into a snapshot file format, with a file format so convoluted and
  changing to make it worthy of Microsoft.
***************************************************************************/
static int load_pak_into_region(void *fp, int *pakbase, int *paklen, UINT8 *mem, int segaddr, int seglen)
{
	if (*paklen) {
		if (*pakbase < segaddr) {
			/* We have to skip part of the PAK file */
			int skiplen;

			skiplen = segaddr - *pakbase;
			if (osd_fseek(fp, skiplen, SEEK_CUR)) {
#if LOG_PAK
				logerror("Could not fully read PAK.\n");
#endif
				return 1;
			}

			*pakbase += skiplen;
			*paklen -= skiplen;
		}

		if (*pakbase < segaddr + seglen) {
			mem += *pakbase - segaddr;
			seglen -= *pakbase - segaddr;

			if (seglen > *paklen)
				seglen = *paklen;

			if (osd_fread(fp, mem, seglen) < seglen) {
#if LOG_PAK
				logerror("Could not fully read PAK.\n");
#endif
				return 1;
			}

			*pakbase += seglen;
			*paklen -= seglen;
		}
	}
	return 0;
}

static void pak_load_trailer(const pak_decodedtrailer *trailer)
{
	cpu_set_reg(M6809_PC, trailer->reg_pc);
	cpu_set_reg(M6809_X, trailer->reg_x);
	cpu_set_reg(M6809_Y, trailer->reg_y);
	cpu_set_reg(M6809_U, trailer->reg_u);
	cpu_set_reg(M6809_S, trailer->reg_s);
	cpu_set_reg(M6809_DP, trailer->reg_dp);
	cpu_set_reg(M6809_B, trailer->reg_b);
	cpu_set_reg(M6809_A, trailer->reg_a);
	cpu_set_reg(M6809_CC, trailer->reg_cc);

	/* I seem to only be able to get a small amount of the PIA state from the
	 * snapshot trailers. Thus I am going to configure the PIA myself. The
	 * following PIA writes are the same thing that the CoCo ROM does on
	 * startup. I wish I had a better solution
	 */

	pia_write(0, 1, 0x00);
	pia_write(0, 3, 0x00);
	pia_write(0, 0, 0x00);
	pia_write(0, 2, 0xff);
	pia_write(1, 1, 0x00);
	pia_write(1, 3, 0x00);
	pia_write(1, 0, 0xfe);
	pia_write(1, 2, 0xf8);

	pia_write(0, 1, trailer->pia[1]);
	pia_write(0, 0, trailer->pia[0]);
	pia_write(0, 3, trailer->pia[3]);
	pia_write(0, 2, trailer->pia[2]);

	pia_write(1, 1, trailer->pia[5]);
	pia_write(1, 0, trailer->pia[4]);
	pia_write(1, 3, trailer->pia[7]);
	pia_write(1, 2, trailer->pia[6]);

	/* For some reason, specifying use of high ram seems to screw things up;
	 * I'm not sure whether it is because I'm using the wrong method to get
	 * access that bit or or whether it is something else.  So that is why
	 * I am specifying 0x7fff instead of 0xffff here
	 */
	sam_setstate(trailer->sam, 0x7fff);
}

static int trailer_load = 0;
static pak_decodedtrailer trailer;

static void pak_load_trailer_callback(int param)
{
	pak_load_trailer(&trailer);
}

static int generic_pak_load(int id, UINT8 *rambase, UINT8 *rombase, UINT8 *pakbase)
{
	void *fp;

	fp = image_fopen (IO_SNAPSHOT, id, OSD_FILETYPE_IMAGE, 0);
	if (fp)
	{
		int paklength;
		int pakstart;

		pak_header header;
		int trailerlen;
		UINT8 trailerraw[500];

		if (osd_fread(fp, &header, sizeof(header)) < sizeof(header)) {
#if LOG_PAK
			logerror("Could not fully read PAK.\n");
#endif
			osd_fclose(fp);
			return 1;
		}

		paklength = header.length ? LITTLE_ENDIANIZE_INT16(header.length) : 0x10000;
		pakstart = LITTLE_ENDIANIZE_INT16(header.start);
		if (pakstart == 0xc000)
			cart_inserted = 1;

		if (osd_fseek(fp, paklength, SEEK_CUR)) {
#if LOG_PAK
			logerror("Could not fully read PAK.\n");
#endif
			osd_fclose(fp);
			return 1;
		}

		trailerlen = osd_fread(fp, trailerraw, sizeof(trailerraw));
		if (trailerlen) {
			if (pak_decode_trailer(trailerraw, trailerlen, &trailer)) {
#if LOG_PAK
				logerror("Invalid or unknown PAK trailer.\n");
#endif
				osd_fclose(fp);
				return 1;
			}

			trailer_load = 1;
		}

		if (osd_fseek(fp, sizeof(pak_header), SEEK_SET)) {
#if LOG_PAK
			logerror("Unexpected error while reading PAK.\n");
#endif
			osd_fclose(fp);
			return 1;
		}

		/* Now that we are done reading the trailer; we can cap the length */
		if (paklength > 0xff00)
			paklength = 0xff00;

		/* Since PAK files allow the flexibility of loading anywhere in
		 * the base RAM or ROM, we have to do tricks because in MESS's
		 * memory, RAM and ROM may be separated, hense this function's
		 * two parameters.
		 *
		 * Similarly, some PAKs appear to be loading into high RAM.  I
		 * am not completely sure how to distinguish this, but I can
		 * guess
		 */

		/* Get the RAM portion */
		if (load_pak_into_region(fp, &pakstart, &paklength, rambase, 0x0000, 0x8000)) {
			osd_fclose(fp);
			return 1;
		}

		if (pakstart == 0x8000) {
			/* We are probably loading into high RAM */
			if ((rombase - rambase) < 0x10000) {
				if (load_pak_into_region(fp, &pakstart, &paklength, rambase, 0x8000, 0x7F00)) {
					osd_fclose(fp);
					return 1;
				}
			}
		}
		else {
			/* Get the ROM portion */
			if (load_pak_into_region(fp, &pakstart, &paklength, rombase, 0x8000, 0x4000)) {
				osd_fclose(fp);
				return 1;
			}
			/* Get the PAK portion */
			if (load_pak_into_region(fp, &pakstart, &paklength, pakbase, 0xC000, 0x3F00)) {
				osd_fclose(fp);
				return 1;
			}
		}
		osd_fclose(fp);
	}
	return INIT_PASS;
}

int dragon32_pak_load(int id)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_pak_load(id, &ROM[0], &ROM[0x8000], &ROM[0xc000]);
}

int dragon64_pak_load(int id)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_pak_load(id, &ROM[0], &ROM[0x10000], &ROM[0x14000]);
}

int coco3_pak_load(int id)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_pak_load(id, &ROM[0x70000], &ROM[0x80000], &ROM[0x8c000]);
}

/***************************************************************************
  ROM files

  ROM files are simply raw dumps of cartridges.  I believe that they should
  be used in place of PAK files, when possible
***************************************************************************/

static int generic_rom_load(int id, UINT8 *dest, UINT16 destlength)
{
	UINT8 *rombase;
	int   romsize;
	void *fp;

	cart_inserted = 0;

	fp = image_fopen (IO_CARTSLOT, id, OSD_FILETYPE_IMAGE, 0);
	if (fp) {

		romsize = osd_fsize(fp);

		/* The following hack is for Arkanoid running on the CoCo2.
		   The issuse is the CoCo2 hardware only allows the cartridge
		   interface to access 0xC000-0xFEFF (16K). The Arkanoid ROM is
		   32K starting at 0x8000. The first 16K is totally inaccessable
		   from a CoCo2. Thus we need to skip ahead in the ROM file. On
		   the CoCo3 the entire 32K ROM is accessable. */

		if ( device_crc(IO_CARTSLOT, 0) == 0x25C3AA70 )     /* Test for Arkanoid  */
		{
			if ( destlength == 0x4000 )						/* Test if CoCo2      */
			{
				osd_fseek( fp, 0x4000, SEEK_SET );			/* Move ahead in file */
				romsize -= 0x4000;							/* Adjust ROM size    */
			}
		}

		if (romsize > destlength)
			romsize = destlength;

		osd_fread(fp, dest, romsize);

		cart_inserted = 1;
		osd_fclose(fp);

		/* Now we need to repeat the mirror the ROM throughout the ROM memory */
		rombase = dest;
		dest += romsize;
		destlength -= romsize;
		while(destlength > 0) {
			if (romsize > destlength)
				romsize = destlength;
			memcpy(dest, rombase, romsize);
			dest += romsize;
			destlength -= romsize;
		}
	}
	return INIT_PASS;
}

int dragon32_rom_load(int id)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_rom_load(id, &ROM[0xc000], 0x4000);
}

int dragon64_rom_load(int id)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	return generic_rom_load(id, &ROM[0x14000], 0x4000);
}

int coco3_rom_load(int id)
{
	UINT8 	*ROM = memory_region(REGION_CPU1);
	int		count;
	void	*fp;

	fp = image_fopen(IO_CARTSLOT, 0, OSD_FILETYPE_IMAGE, 0);
	count = count_bank();
	if (fp)
		osd_fclose(fp);

	if( count == 0 )
		/* Load roms starting at 0x8000 and mirror upwards. */
		/* ROM size is 32K max */
		return generic_rom_load(id, &ROM[0x88000], 0x8000);
	else
		/* Load roms starting at 0x8000 and mirror upwards. */
		/* ROM bank is 16K max */
		return generic_rom_load(id, &ROM[0x88000], 0x4000);
}

/***************************************************************************
  Interrupts

  The Dragon/CoCo2 have two PIAs.  These PIAs can trigger interrupts.  PIA0
  is set up to trigger IRQ on the CPU, and PIA1 can trigger FIRQ.  Each PIA
  has two output lines, and an interrupt will be triggered if either of these
  lines are asserted.

  -----  IRQ
  6809 |-<----------- PIA0
       |
       |
       |
       |
       |
       |-<----------- PIA1
  -----

  The CoCo 3 still supports these interrupts, but the GIME can chose whether
  "old school" interrupts are generated, or the new ones generated by the GIME

  -----  IRQ
  6809 |-<----------- PIA0
       |       |                ------
       |       -<-------<-------|    |
       |                        |GIME|
       |       -<-------<-------|    |
       | FIRQ  |                ------
       |-<----------- PIA1
  -----

  In an email discussion with JK, he informs me that when GIME interrupts are
  enabled, this actually does not prevent PIA interrupts.  Apparently JeffV's
  CoCo 3 emulator did not handle this properly.
***************************************************************************/

enum {
	COCO3_INT_TMR	= 0x20,		/* Timer */
	COCO3_INT_HBORD	= 0x10,		/* Horizontal border sync */
	COCO3_INT_VBORD	= 0x08,		/* Vertical border sync */
	COCO3_INT_EI2	= 0x04,		/* Serial data */
	COCO3_INT_EI1	= 0x02,		/* Keyboard */
	COCO3_INT_EI0	= 0x01,		/* Cartridge */

	COCO3_INT_ALL	= 0x3f
};

static void d_recalc_irq(void)
{
	if ((pia0_irq_a == ASSERT_LINE) || (pia0_irq_b == ASSERT_LINE))
		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6809_IRQ_LINE, CLEAR_LINE);
}

static void d_recalc_firq(void)
{
	if ((pia1_firq_a == ASSERT_LINE) || (pia1_firq_b == ASSERT_LINE))
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

static void coco3_recalc_irq(void)
{
#if LOG_IRQ_RECALC
	logerror("coco3_recalc_irq(): gime_irq=%i pia0_irq_a=%i pia0_irq_b=%i (GIME IRQ %s)\n",
		gime_irq, pia0_irq_a, pia0_irq_b, coco3_gimereg[0] & 0x20 ? "enabled" : "disabled");
#endif

	if ((coco3_gimereg[0] & 0x20) && gime_irq)
		cpu_set_irq_line(0, M6809_IRQ_LINE, ASSERT_LINE);
	else
		d_recalc_irq();
}

static void coco3_recalc_firq(void)
{
	if ((coco3_gimereg[0] & 0x10) && gime_firq)
		cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	else
		d_recalc_firq();
}

static void d_pia0_irq_a(int state)
{
	pia0_irq_a = state;
	d_recalc_irq();
}

static void d_pia0_irq_b(int state)
{
	pia0_irq_b = state;
	d_recalc_irq();
}

static void d_pia1_firq_a(int state)
{
	pia1_firq_a = state;
	d_recalc_firq();
}

static void d_pia1_firq_b(int state)
{
	pia1_firq_b = state;
	d_recalc_firq();
}

static void coco3_pia0_irq_a(int state)
{
	pia0_irq_a = state;
	coco3_recalc_irq();
}

static void coco3_pia0_irq_b(int state)
{
	pia0_irq_b = state;
	coco3_recalc_irq();
}

static void coco3_pia1_firq_a(int state)
{
	pia1_firq_a = state;
	coco3_recalc_firq();
}

static void coco3_pia1_firq_b(int state)
{
	pia1_firq_b = state;
	coco3_recalc_firq();
}

static void coco3_raise_interrupt(int mask, int state)
{
	int lowtohigh;

	lowtohigh = state && ((coco3_interupt_line & mask) == 0);

	if (state)
		coco3_interupt_line |= mask;
	else
		coco3_interupt_line &= ~mask;

	if (lowtohigh) {
		if ((coco3_gimereg[0] & 0x20) && (coco3_gimereg[2] & mask)) {
			gime_irq |= (coco3_gimereg[2] & mask);
			coco3_recalc_irq();

#if LOG_INT_COCO3
			logerror("CoCo3 Interrupt: Raising IRQ; scanline=%i\n", rastertrack_scanline());
#endif
		}
		if ((coco3_gimereg[0] & 0x10) && (coco3_gimereg[3] & mask)) {
			gime_firq |= (coco3_gimereg[3] & mask);
			coco3_recalc_firq();

#if LOG_INT_COCO3
			logerror("CoCo3 Interrupt: Raising FIRQ; scanline=%i\n", rastertrack_scanline());
#endif
		}
	}
}

WRITE_HANDLER( coco_m6847_hs_w )
{
	pia_0_ca1_w(0, data);
}

WRITE_HANDLER( coco_m6847_fs_w )
{
	pia_0_cb1_w(0, !data);
}

WRITE_HANDLER( coco3_m6847_hs_w )
{
	if (data)
		coco3_timer_hblank();
	pia_0_ca1_w(0, data);
	coco3_raise_interrupt(COCO3_INT_HBORD, !data);
}

WRITE_HANDLER( coco3_m6847_fs_w )
{
#if LOG_VBORD
	logerror("coco3_m6847_fs_w(): data=%i scanline=%i\n", data, rastertrack_scanline());
#endif
	pia_0_cb1_w(0, !data);
	coco3_raise_interrupt(COCO3_INT_VBORD, !data);
}

/***************************************************************************
  Joystick Abstractions
***************************************************************************/

#define JOYSTICK_RIGHT_X	7
#define JOYSTICK_RIGHT_Y	8
#define JOYSTICK_LEFT_X		9
#define JOYSTICK_LEFT_Y		10

double read_joystick(int joyport);
double read_joystick(int joyport)
{
	return readinputport(joyport) / 255.0;
}

#define JOYSTICKMODE_NORMAL			0x00
#define JOYSTICKMODE_HIRES			0x10
#define JOYSTICKMODE_HIRES_CC3MAX	0x30

#define joystick_mode()	(readinputport(12) & 0x30)

/***************************************************************************
  Hires Joystick
***************************************************************************/

static int coco_hiresjoy_ca = 1;
static double coco_hiresjoy_xtransitiontime = 0;
static double coco_hiresjoy_ytransitiontime = 0;

static double coco_hiresjoy_computetransitiontime(int inputport)
{
	double val;

	val = read_joystick(inputport);

	if (joystick_mode() == JOYSTICKMODE_HIRES_CC3MAX) {
		/* CoCo MAX 3 Interface */
		val = val * 2500.0 + 400.0;
	}
	else {
		/* Normal Hires Interface */
		val = val * 4160.0 + 592.0;
	}

	return timer_get_time() + (COCO_CPU_SPEED * val);
}

static void coco_hiresjoy_w(int data)
{
	if (!data && coco_hiresjoy_ca) {
		/* Hi to lo */
		coco_hiresjoy_xtransitiontime = coco_hiresjoy_computetransitiontime(JOYSTICK_RIGHT_X);
		coco_hiresjoy_ytransitiontime = coco_hiresjoy_computetransitiontime(JOYSTICK_RIGHT_Y);
	}
	else if (data && !coco_hiresjoy_ca) {
		/* Lo to hi */
		coco_hiresjoy_ytransitiontime = 0;
		coco_hiresjoy_ytransitiontime = 0;
	}
	coco_hiresjoy_ca = data;
}

static int coco_hiresjoy_readone(double transitiontime)
{
	return (transitiontime) ? (timer_get_time() >= transitiontime) : 1;
}

static int coco_hiresjoy_rx(void)
{
	return coco_hiresjoy_readone(coco_hiresjoy_xtransitiontime);
}

static int coco_hiresjoy_ry(void)
{
	return coco_hiresjoy_readone(coco_hiresjoy_ytransitiontime);
}

/***************************************************************************
  Sound MUX

  The sound MUX has 4 possible settings, depend on SELA and SELB inputs:

  00	- DAC (digital - analog converter)
  01	- CSN (cassette)
  10	- SND input from cartridge (NYI because we only support the FDC)
  11	- Grounded (0)

  Source - Tandy Color Computer Service Manual
***************************************************************************/

#define SOUNDMUX_STATUS_ENABLE	4
#define SOUNDMUX_STATUS_SEL2	2
#define SOUNDMUX_STATUS_SEL1	1

static void soundmux_update(void)
{
	/* This function is called whenever the MUX (selector switch) is changed
	 * It mainly turns on and off the cassette audio depending on the switch.
	 * It also calls a function into the cartridges device to tell it if it is
	 * switch on or off.
	 */

	int casstatus, new_casstatus;

	casstatus = device_status(IO_CASSETTE, 0, -1);
	new_casstatus = casstatus | WAVE_STATUS_MUTED;

	switch(soundmux_status) {
	case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
		/* CSN */
		new_casstatus &= ~WAVE_STATUS_MUTED;
		break;
	default:
		break;
	}

	coco_cartridge_enablesound(soundmux_status == (SOUNDMUX_STATUS_ENABLE|SOUNDMUX_STATUS_SEL2));

	if (casstatus != new_casstatus) {
#if LOG_CASSETTE
		logerror("CoCo: Turning cassette speaker %s\n", new_casstatus ? "on" : "off");
#endif
		device_status(IO_CASSETTE, 0, new_casstatus);
	}
}

void dragon_sound_update(void)
{
	/* Call this function whenever you need to update the sound. It will
	 * automatically mute any devices that are switched out.
	 */

	if (soundmux_status & SOUNDMUX_STATUS_ENABLE) {
		switch(soundmux_status) {
		case SOUNDMUX_STATUS_ENABLE:
			/* DAC */
			DAC_data_w(0, pia1_pb1 + (d_dac >> 1) );  /* Mixing the two sources */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL1:
			/* CSN */
			DAC_data_w(0, pia1_pb1); /* Mixing happens elsewhere */
			break;
		case SOUNDMUX_STATUS_ENABLE | SOUNDMUX_STATUS_SEL2:
			/* CART Sound */
			DAC_data_w(0, pia1_pb1); /* To do: mix in cart signal */
			break;
		default:
			/* This pia line is always connected to the output */
			DAC_data_w(0, pia1_pb1);
			break;
		}
	}
}

static void soundmux_enable_w(int data)
{
	if (data)
		soundmux_status |= SOUNDMUX_STATUS_ENABLE;
	else
		soundmux_status &= ~SOUNDMUX_STATUS_ENABLE;
	soundmux_update();
}

static void soundmux_sel1_w(int data)
{
	if (data)
		soundmux_status |= SOUNDMUX_STATUS_SEL1;
	else
		soundmux_status &= ~SOUNDMUX_STATUS_SEL1;
	soundmux_update();
}

static void soundmux_sel2_w(int data)
{
	if (data)
		soundmux_status |= SOUNDMUX_STATUS_SEL2;
	else
		soundmux_status &= ~SOUNDMUX_STATUS_SEL2;
	soundmux_update();
}

/***************************************************************************
  PIA0 ($FF00-$FF1F) (Chip U8)

  PIA0 PA0-PA7	- Keyboard/Joystick read
  PIA0 PB0-PB7	- Keyboard write
  PIA0 CA1		- M6847 HS (Horizontal Sync)
  PIA0 CA2		- SEL1 (Used by sound mux and joystick)
  PIA0 CB1		- M6847 FS (Field Sync)
  PIA0 CB2		- SEL2 (Used by sound mux and joystick)
***************************************************************************/

static READ_HANDLER ( d_pia0_ca1_r )
{
	return m6847_hs_r(0);
}

static READ_HANDLER ( d_pia0_cb1_r )
{
	return m6847_fs_r(0);
}

static WRITE_HANDLER ( d_pia0_ca2_w )
{
	joystick_axis = data;
	soundmux_sel1_w(data);
}

static WRITE_HANDLER ( d_pia0_cb2_w )
{
	joystick = data;
	soundmux_sel2_w(data);
}

static int keyboard_r(void)
{
	int porta = 0x7f;
	int joyport;
	double joyval;

	if ((input_port_0_r(0) | pia0_pb) != 0xff) porta &= ~0x01;
	if ((input_port_1_r(0) | pia0_pb) != 0xff) porta &= ~0x02;
	if ((input_port_2_r(0) | pia0_pb) != 0xff) porta &= ~0x04;
	if ((input_port_3_r(0) | pia0_pb) != 0xff) porta &= ~0x08;
	if ((input_port_4_r(0) | pia0_pb) != 0xff) porta &= ~0x10;
	if ((input_port_5_r(0) | pia0_pb) != 0xff) porta &= ~0x20;
	if ((input_port_6_r(0) | pia0_pb) != 0xff) porta &= ~0x40;


	if (!joystick && (joystick_mode() != JOYSTICKMODE_NORMAL)) {
		/* Hi res joystick */
		if (joystick_axis ? coco_hiresjoy_ry() : coco_hiresjoy_rx())
			porta |= 0x80;

	}
	else {
		/* Normal joystick */
		joyport = joystick ? (joystick_axis ? JOYSTICK_LEFT_Y : JOYSTICK_LEFT_X) : (joystick_axis ? JOYSTICK_RIGHT_Y : JOYSTICK_RIGHT_X);
		joyval = read_joystick(joyport);
		if (d_dac <= ((int) (joyval * 255.0)))
			porta |= 0x80;
	}

	porta &= ~readinputport(11);

	return porta;
}

static READ_HANDLER ( d_pia0_pa_r )
{
	return keyboard_r();
}

static void coco3_poll_keyboard(int dummy)
{
	int porta;
	porta = keyboard_r() & 0x7f;
	coco3_raise_interrupt(COCO3_INT_EI1, (porta == 0x7f) ? CLEAR_LINE : ASSERT_LINE);
}

static WRITE_HANDLER ( d_pia0_pa_w )
{
	if (joystick_mode() == JOYSTICKMODE_HIRES_CC3MAX) {
		coco_hiresjoy_w(data & 0x04);
	}
}

static WRITE_HANDLER ( d_pia0_pb_w )
{
	pia0_pb = data;
}

/* The following hacks are necessary because a large portion of cartridges
 * tie the Q line to the CART line.  Since Q pulses with every single clock
 * cycle, this would be prohibitively slow to emulate.  Thus we are only
 * going to pulse when the PIA is read from; which seems good enough (for now)
 */
READ_HANDLER( coco_pia_1_r )
{
	if (cart_line == CARTLINE_Q) {
		coco_setcartline(CARTLINE_CLEAR);
		coco_setcartline(CARTLINE_Q);
	}
	return pia_1_r(offset);
}

READ_HANDLER( coco3_pia_1_r )
{
	if (cart_line == CARTLINE_Q) {
		coco3_setcartline(CARTLINE_CLEAR);
		coco3_setcartline(CARTLINE_Q);
	}
	return pia_1_r(offset);
}

/***************************************************************************
  PIA1 ($FF20-$FF3F) (Chip U4)

  PIA1 PA0		- CASSDIN
  PIA1 PA1		- RS232 OUT
  PIA1 PA2-PA7	- DAC
  PIA1 PB0		- RS232 IN
  PIA1 PB1		- Single bit sound
  PIA1 PB2		- RAMSZ (32/64K, 16K, and 4K three position switch)
  PIA1 PB3		- M6847 CSS
  PIA1 PB4		- M6847 INT/EXT and M6847 GM0
  PIA1 PB5		- M6847 GM1
  PIA1 PB6		- M6847 GM2
  PIA1 PB7		- M6847 A/G
  PIA1 CA1		- CD (Carrier Detect; NYI)
  PIA1 CA2		- CASSMOT (Cassette Motor)
  PIA1 CB1		- CART (Cartridge Detect)
  PIA1 CB2		- SNDEN (Sound Enable)
***************************************************************************/

static READ_HANDLER ( d_pia1_cb1_r )
{
	return cart_line ? ASSERT_LINE : CLEAR_LINE;
}

static WRITE_HANDLER ( d_pia1_cb2_w )
{
	soundmux_enable_w(data);
}

static WRITE_HANDLER ( d_pia1_pa_w )
{
	/*
	 *	This port appears at $FF20
	 *
	 *	Bits
	 *  7-2:	DAC to speaker or cassette
	 *    1:	Serial out
	 */
	d_dac = data & 0xfc;
	dragon_sound_update();

	if (joystick_mode() == JOYSTICKMODE_HIRES)
		coco_hiresjoy_w(d_dac >= 0x80);
	else
		device_output(IO_CASSETTE, 0, ((int) d_dac - 0x80) * 0x100);

	device_output(IO_BITBANGER, 0, (data & 2) >> 1);
}

/*
 * This port appears at $FF23
 *
 * The CoCo 1/2 and Dragon kept the gmode and vmode separate, and the CoCo
 * 3 tied them together.  In the process, the CoCo 3 dropped support for the
 * semigraphics modes
 */

static WRITE_HANDLER( d_pia1_pb_w )
{
	m6847_ag_w(0,		data & 0x80);
	m6847_gm2_w(0,		data & 0x40);
	m6847_gm1_w(0,		data & 0x20);
	m6847_gm0_w(0,		data & 0x10);
	m6847_intext_w(0,	data & 0x10);
	m6847_css_w(0,		data & 0x08);
	schedule_full_refresh();

	/* PB1 will drive the sound output.  This is a rarely
	 * used single bit sound mode. It is always connected thus
	 * cannot be disabled.
	 *
	 * Source:  Page 31 of the Tandy Color Computer Serice Manual
	 */

	 pia1_pb1 = ((data & 0x02) ? 127 : 0);
	 dragon_sound_update();
}

static WRITE_HANDLER( coco3_pia1_pb_w )
{
	d_pia1_pb_w(0, data);
	m6847_set_cannonical_row_height();
}

static WRITE_HANDLER ( d_pia1_ca2_w )
{
	int status;

	if (tape_motor ^ data)
	{
		status = device_status(IO_CASSETTE, 0, -1);
		status &= ~WAVE_STATUS_MOTOR_INHIBIT;
		if (!data)
			status |= WAVE_STATUS_MOTOR_INHIBIT;
		device_status(IO_CASSETTE, 0, status);
		tape_motor = data;
	}
}

static READ_HANDLER ( d_pia1_pa_r )
{
	return (device_input(IO_CASSETTE, 0) >= 0) ? 1 : 0;
}

static READ_HANDLER ( d_pia1_pb_r_dragon )
{
	/* This handles the reading of the memory sense switch (pb2) for the Dragon and CoCo 1,
	 * and serial-in (pb0). Serial-in not yet implemented.
	 */

	switch( readinputport(12) & 0x18 )		/* Read dip switch setting "on motherboard" */
	{
		case 0x00: /* 32/64K: wire output of pia0_pb7 to input pia1_pb2  */
			return (pia0_pb & 0x80) >> 5;
			break;
		case 0x08: /* 16K: wire pia1_pb2 high */
			return 0x04;
			break;
		case 0x10: /* 4K: wire pia1_pb2 low */
			return 0x00;
			break;
	}
	return 0;
}

static READ_HANDLER ( d_pia1_pb_r_coco2 )
{
	/* This handles the reading of the memory sense switch (pb2) for the CoCo 2 and 3,
	 * and serial-in (pb0). Serial-in not yet implemented.
	 */

	switch( readinputport(12) & 0x18 )		/* Read dip switch setting "on motherboard" */
	{
		case 0x00: /* 32/64K: wire output of pia0_pb6 to input pia1_pb2  */
			return (pia0_pb & 0x40) >> 4;
			break;
		case 0x08: /* 16K: wire pia1_pb2 high */
			return 0x04;
			break;
		case 0x10: /* 4K: wire pia1_pb2 low */
			return 0x00;
			break;
	}
	return 0;
}

/***************************************************************************
  Misc
***************************************************************************/

READ_HANDLER(dragon_mapped_irq_r)
{
	return coco_rom[0x3ff0 + offset];
}

READ_HANDLER(coco3_mapped_irq_r)
{
	/* NPW 28-Aug-2000 - I discovered this when we moved over to the new ROMset
	 * and Tim confirmed this
	 */
	return coco_rom[0x7ff0 + offset];
}

static void d_sam_set_mpurate(int val)
{
	/* The infamous speed up poke.
	 *
	 * This was a SAM switch that occupied 4 addresses:
	 *
	 *		$FFD9	(set)	R1
	 *		$FFD8	(clear)	R1
	 *		$FFD7	(set)	R0
	 *		$FFD6	(clear)	R0
	 *
	 * R1:R0 formed the following states:
	 *		00	- slow          0.89 MHz
	 *		01	- dual speed    ???
	 *		1x	- fast          1.78 MHz
	 *
	 * R1 controlled whether the video addressing was speeded up and R0
	 * did the same for the CPU.  On pre-CoCo 3 machines, setting R1 caused
	 * the screen to display garbage because the M6847 could not display
	 * fast enough.
	 *
	 * TODO:  Make the overclock more accurate.  In dual speed, ROM was a fast
	 * access but RAM was not.  I don't know how to simulate this.
	 */
    timer_set_overclock(0, val ? 2 : 1);
}

static void d_sam_set_pageonemode(int val)
{
	/* Page mode - allowed switching between the low 32k and the high 32k,
	 * assuming that 64k wasn't enabled
	 *
	 * TODO:  Actually implement this.  Also find out what the CoCo 3 did with
	 * this (it probably ignored it)
	 */
}

static void d_sam_set_memorysize(int val)
{
	/* Memory size - allowed restricting memory accesses to something less than
	 * 32k
	 *
	 * This was a SAM switch that occupied 4 addresses:
	 *
	 *		$FFDD	(set)	R1
	 *		$FFDC	(clear)	R1
	 *		$FFDB	(set)	R0
	 *		$FFDA	(clear)	R0
	 *
	 * R1:R0 formed the following states:
	 *		00	- 4k
	 *		01	- 16k
	 *		10	- 64k
	 *		11	- static RAM (??)
	 *
	 * If something less than 64k was set, the low RAM would be smaller and
	 * mirror the other parts of the RAM
	 *
	 * TODO:  Find out what "static RAM" is
	 * TODO:  This should affect _all_ memory accesses, not just video ram
	 * TODO:  Verify that the CoCo 3 ignored this
	 */

	static int vram_sizes[] = { 0x1000, 0x4000, 0x10000, 0x10000 };
	m6847_set_ram_size(vram_sizes[val % 4]);
}

/***************************************************************************
  CoCo 3 Timer

  The CoCo 3 had a timer that had would activate when first written to, and
  would decrement over and over again until zero was reached, and at that
  point, would flag an interrupt.  At this point, the timer starts back up
  again.

  I am deducing that the timer interrupt line was asserted if the timer was
  zero and unasserted if the timer was non-zero.  Since we never truly track
  the timer, we just use timer callback (coco3_timer_callback() asserts the
  line)

  Most CoCo 3 docs, including the specs that Tandy released, say that the
  high speed timer is 70ns (half of the speed of the main clock crystal).
  However, it seems that this is in error, and the GIME timer is really a
  280ns timer (one eighth the speed of the main clock crystal.  Gault's
  FAQ agrees with this
***************************************************************************/

static double coco3_timer_counterbase;
static void *coco3_timer_counter;
static int coco3_timer_interval;	/* interval: 1=280 nsec, 0=63.5 usec */
static int coco3_timer_value;
static int coco3_timer_base;

static void coco3_timer_init(void)
{
	coco3_timer_counter = NULL;
	coco3_timer_interval = 0;
	coco3_timer_base = 0;
	coco3_timer_value = 0;
}

static int coco3_timer_actualvalue(int specified)
{
	/* This resets the timer back to the original value
	 *
	 * JK tells me that when the timer resets, it gets reset to a value that
	 * is 2 (with the 1986 GIME) above or 1 (with the 1987 GIME) above the
	 * value written into the timer.  coco3_timer_base keeps track of the value
	 * placed into the variable, so we increment that here
	 *
	 * For now, we are emulating the 1986 GIME
	 */
	return specified ? specified + 2 : 0;
}

static void coco3_timer_newvalue(void)
{
	if (coco3_timer_value == 0) {
#if LOG_INT_TMR
		logerror("CoCo3 GIME: Triggering TMR interrupt; scanline=%i time=%g\n", rastertrack_scanline(), timer_get_time());
#endif
		coco3_raise_interrupt(COCO3_INT_TMR, 1);

		/* HACKHACK - This should not happen until the next timer tick */
		coco3_raise_interrupt(COCO3_INT_TMR, 0);

		/* Every time the timer hit zero, the video hardware would do a blink */
		coco3_vh_blink();

		/* This resets the timer back to the original value
		 *
		 * JK tells me that when the timer resets, it gets reset to a value that
		 * is 2 (with the 1986 GIME) above or 1 (with the 1987 GIME) above the
		 * value written into the timer.  coco3_timer_base keeps track of the value
		 * placed into the variable, so we increment that here
		 *
		 * For now, we are emulating the 1986 GIME
		 */
		coco3_timer_value = coco3_timer_actualvalue(coco3_timer_base);
	}
}

static void coco3_timer_cannonicalize(int newvalue)
{
	int elapsed;
	double current_time;
	double duration;

	current_time = timer_get_time();

#if LOG_TIMER
	logerror("coco3_timer_cannonicalize(): Entering; current_time=%g\n", current_time);
#endif

	/* This part resolves timer issues with the CMP Carrier */
	if (coco3_timer_counter) {
		assert(coco3_timer_value > 0);

		/* Calculate how many transitions elapsed */
		elapsed = (int) ((current_time - coco3_timer_counterbase) / COCO_TIMER_CMPCARRIER);
		assert(elapsed >= 0);

#if LOG_TIMER
		logerror("coco3_timer_cannonicalize(): Recalculating; current_time=%g base=%g elapsed=%i\n", current_time, coco3_timer_counterbase, elapsed);
#endif

		/* Remove the timer.  Note that we might be called as part of a timer
		 * proc resolving.  In this case; we must _not_ call timer_remove() on
		 * ourself, due to the way the MAME core works
		 */
		if (newvalue != -1)
			timer_remove(coco3_timer_counter);
		coco3_timer_counter = NULL;

		if (elapsed) {
			coco3_timer_value -= elapsed;

			/* HACKHACK - I don't know why I have to do this */
			if (coco3_timer_value < 0)
				coco3_timer_value = 0;

			coco3_timer_newvalue();
		}
	}

	if (newvalue >= 0) {
#if LOG_TIMER
		logerror("coco3_timer_cannonicalize(): Setting timer to %i\n", newvalue);
#endif
		coco3_timer_value = newvalue;
	}

	if (coco3_timer_interval && coco3_timer_value) {
		coco3_timer_counterbase = current_time; //floor(current_time / COCO_TIMER_CMPCARRIER) * COCO_TIMER_CMPCARRIER;
		duration = coco3_timer_counterbase + (coco3_timer_value * COCO_TIMER_CMPCARRIER) + (COCO_TIMER_CMPCARRIER / 2) - current_time;
		coco3_timer_counter = timer_set(duration, -1, coco3_timer_cannonicalize);

#if LOG_TIMER
		logerror("coco3_timer_cannonicalize(): Setting CMP timer for duration %g\n", duration);
#endif
	}
}

static void coco3_timer_hblank(void)
{
	if (!coco3_timer_interval && (coco3_timer_value > 0)) {
#if LOG_DEC_TIMER
		logerror("coco3_timer_hblank(): Decrementing timer (%i ==> %i)\n", coco3_timer_value, coco3_timer_value - 1);
#endif
		coco3_timer_value--;
		coco3_timer_newvalue();
	}
}

/* Write into MSB of timer ($FF94); this causes a reset (source: Sockmaster) */
static void coco3_timer_msb_w(int data)
{
#if LOG_TIMER_SET
	logerror("coco3_timer_msb_w(): data=$%02x\n", data);
#endif

	coco3_timer_base &= 0x00ff;
	coco3_timer_base |= (data & 0x0f) << 8;
	coco3_timer_cannonicalize(coco3_timer_actualvalue(coco3_timer_base));
}

/* Write into LSB of timer ($FF95); this does not cause a reset (source: Sockmaster) */
static void coco3_timer_lsb_w(int data)
{
#if LOG_TIMER_SET
	logerror("coco3_timer_lsb_w(): data=$%02x\n", data);
#endif

	coco3_timer_base &= 0xff00;
	coco3_timer_base |= (data & 0xff);
}

static void coco3_timer_set_interval(int interval)
{
	/* interval==0 is 63.5us interval==1 if 280ns */
	if (interval)
		interval = 1;

#if LOG_TIMER_SET
	logerror("coco3_timer_set_interval(): Interval is %s\n", interval ? "280ns" : "63.5us");
#endif

	if (coco3_timer_interval != interval) {
		coco3_timer_interval = interval;
		coco3_timer_cannonicalize(-1);
	}
}

/***************************************************************************
  MMU
***************************************************************************/

WRITE_HANDLER ( dragon64_ram_w );
WRITE_HANDLER ( dragon64_ram_w )
{
	coco_ram_w(offset + 0x8000, data);
}

static void d_sam_set_maptype(int val)
{
	UINT8 *RAM = memory_region(REGION_CPU1);
	if (val) {
		cpu_setbank(1, &RAM[0x8000]);
		memory_set_bankhandler_w(1, 0, dragon64_ram_w);
	}
	else {
		cpu_setbank(1, coco_rom);
		memory_set_bankhandler_w(1, 0, MWA_ROM);
	}
}

/* Coco 3 */

int coco3_mmu_lookup(int block, int forceram);
int coco3_mmu_lookup(int block, int forceram)
{
	int result;

	if (coco3_gimereg[0] & 0x40) {
		if (coco3_gimereg[1] & 1)
			block += 8;
		result = coco3_mmu[block];
	}
	else {
		result = block + 56;
	}

	/* Are we actually in ROM?
	 *
	 * In our world, ROM is represented by memory blocks 0x40-0x47
	 *
	 * 0x40			Extended Color Basic
	 * 0x41			Color Basic
	 * 0x42			Reset Initialization
	 * 0x43			Super Extended Color Basic
	 * 0x44-0x47	Cartridge ROM
	 *
	 * This is the level where ROM is mapped, according to Tepolt (p21)
	 */
	if ((result >= 0x3c) && !coco3_enable_64k && !forceram) {
		static const int rommap[4][4] = {
			{ 0x40, 0x41, 0x46, 0x47 },
			{ 0x40, 0x41, 0x46, 0x47 },
			{ 0x40, 0x41, 0x42, 0x43 },
			{ 0x44, 0x45, 0x46, 0x47 }
		};
		result = rommap[coco3_gimereg[0] & 3][result - 0x3c];
	}

	return result;
}

int coco3_mmu_translate(int block, int offset)
{
	int forceram;

	/* Block 8 is the 0xfe00 block; and it is treated differently */
	if (block == 8) {
		if (coco3_gimereg[0] & 8)
			return 0x7fe00 + offset;
		block = 7;
		offset += 0x1e00;
		forceram = 1;
	}
	else {
		forceram = 0;
	}
	return (coco3_mmu_lookup(block, forceram) * 0x2000) + offset;
}

static void coco3_mmu_update(int lowblock, int hiblock)
{
	UINT8 *RAM = memory_region(REGION_CPU1);

	static mem_write_handler handlers[] = {
		coco3_ram_b1_w, coco3_ram_b2_w,
		coco3_ram_b3_w, coco3_ram_b4_w,
		coco3_ram_b5_w, coco3_ram_b6_w,
		coco3_ram_b7_w, coco3_ram_b8_w,
		coco3_ram_b9_w
	};

	int i;
	UINT8 *p;

	for (i = lowblock; i <= hiblock; i++) {
		p = &RAM[coco3_mmu_translate(i, 0)];
		cpu_setbank(i + 1, p);
		memory_set_bankhandler_w(i + 1, 0, ((p - RAM) >= 0x80000) ? MWA_ROM : handlers[i]);
#if LOG_MMU
		logerror("CoCo3 GIME MMU: Logical $%04x ==> Physical $%05x\n",
			(i == 8) ? 0xfe00 : i * 0x2000,
			p - RAM);
#endif
	}
}

READ_HANDLER(coco3_mmu_r)
{
	return coco3_mmu[offset] & 0x3f;
}

WRITE_HANDLER(coco3_mmu_w)
{
	data &= 0x3f;
	coco3_mmu[offset] = data;

	/* Did we modify the live MMU bank? */
	if ((offset >> 3) == (coco3_gimereg[1] & 1)) {
		offset &= 7;
		coco3_mmu_update(offset, (offset == 7) ? 8 : offset);
	}
}

/***************************************************************************
  GIME Registers (Reference: Super Extended Basic Unravelled)
***************************************************************************/

READ_HANDLER(coco3_gime_r)
{
	int result = 0;

	switch(offset) {
	case 2:	/* Read pending IRQs */
		result = gime_irq;
		if (result) {
			gime_irq = 0;
			coco3_recalc_irq();
		}
		break;

	case 3:	/* Read pending FIRQs */
		result = gime_firq;
		if (result) {
			gime_firq = 0;
			coco3_recalc_firq();
		}
		break;

	case 4:	/* Timer MSB/LSB; these arn't readable */
	case 5:
		/* JK tells me that these values are indeterminate; and $7E appears
		 * to be the value most commonly returned
		 */
		result = 0x7e;
		break;

	default:
		result = coco3_gimereg[offset];
		break;
	}
	return result;
}

WRITE_HANDLER(coco3_gime_w)
{
	coco3_gimereg[offset] = data;

#if LOG_GIME
	logerror("CoCo3 GIME: $%04x <== $%02x pc=$%04x\n", offset + 0xff90, data, cpu_get_pc());
#endif

	/* Features marked with '!' are not yet implemented */
	switch(offset) {
	case 0:
		/*	$FF90 Initialization register 0
		 *		  Bit 7 COCO 1=CoCo compatible mode
		 *		  Bit 6 MMUEN 1=MMU enabled
		 *		  Bit 5 IEN 1 = GIME chip IRQ enabled
		 *		  Bit 4 FEN 1 = GIME chip FIRQ enabled
		 *		  Bit 3 MC3 1 = RAM at FEXX is constant
		 *		  Bit 2 MC2 1 = standard SCS (Spare Chip Select)
		 *		  Bit 1 MC1 ROM map control
		 *		  Bit 0 MC0 ROM map control
		 */
		coco3_vh_sethires(data & 0x80 ? 0 : 1);
		coco3_mmu_update(0, 8);
		break;

	case 1:
		/*	$FF91 Initialization register 1
		 *		  Bit 7 Unused
		 *		  Bit 6 Unused
		 *		  Bit 5 TINS Timer input select; 1 = 280 nsec, 0 = 63.5 usec
		 *		  Bit 4 Unused
		 *		  Bit 3 Unused
		 *		  Bit 2 Unused
		 *		  Bit 1 Unused
		 *		  Bit 0 TR Task register select
		 */
		coco3_mmu_update(0, 8);
		coco3_timer_set_interval(data & 0x20);
		break;

	case 2:
		/*	$FF92 Interrupt request enable register
		 *		  Bit 7 Unused
		 *		  Bit 6 Unused
		 *		  Bit 5 TMR Timer interrupt
		 *		  Bit 4 HBORD Horizontal border interrupt
		 *		  Bit 3 VBORD Vertical border interrupt
		 *		! Bit 2 EI2 Serial data interrupt
		 *		  Bit 1 EI1 Keyboard interrupt
		 *		  Bit 0 EI0 Cartridge interrupt
		 */
#if LOG_INT_MASKING
		logerror("CoCo3 IRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
			(data & 0x20) ? "TMR " : "",
			(data & 0x10) ? "HBORD " : "",
			(data & 0x08) ? "VBORD " : "",
			(data & 0x04) ? "EI2 " : "",
			(data & 0x02) ? "EI1 " : "",
			(data & 0x01) ? "EI0 " : "");
#endif
		break;

	case 3:
		/*	$FF93 Fast interrupt request enable register
		 *		  Bit 7 Unused
		 *		  Bit 6 Unused
		 *		  Bit 5 TMR Timer interrupt
		 *		  Bit 4 HBORD Horizontal border interrupt
		 *		  Bit 3 VBORD Vertical border interrupt
		 *		! Bit 2 EI2 Serial border interrupt
		 *		  Bit 1 EI1 Keyboard interrupt
		 *		  Bit 0 EI0 Cartridge interrupt
		 */
#if LOG_INT_MASKING
		logerror("CoCo3 FIRQ: Interrupts { %s%s%s%s%s%s} enabled\n",
			(data & 0x20) ? "TMR " : "",
			(data & 0x10) ? "HBORD " : "",
			(data & 0x08) ? "VBORD " : "",
			(data & 0x04) ? "EI2 " : "",
			(data & 0x02) ? "EI1 " : "",
			(data & 0x01) ? "EI0 " : "");
#endif
		break;

	case 4:
		/*	$FF94 Timer register MSB
		 *		  Bits 4-7 Unused
		 *		  Bits 0-3 High order four bits of the timer
		 */
		coco3_timer_msb_w(data);
		break;

	case 5:
		/*	$FF95 Timer register LSB
		 *		  Bits 0-7 Low order eight bits of the timer
		 */
		coco3_timer_lsb_w(data);
		break;
	}
}

static void coco3_sam_set_maptype(int val)
{
	coco3_enable_64k = val;
	coco3_mmu_update(4, 8);
}

/***************************************************************************
  Joystick autocenter
***************************************************************************/

static int autocenter_val;

static void autocenter_timer_proc(int data)
{
	struct InputPort *in;
	int dipport, dipmask, portval;

	dipport = (data & 0xff00) >> 8;
	dipmask = data & 0x00ff;
	portval = readinputport(dipport) & dipmask;

	if (autocenter_val != portval) {
		/* Now go through all inputs, and set or reset IPF_CENTER on all
		 * joysticks
		 */
		for (in = Machine->input_ports; in->type != IPT_END; in++) {
			if (((in->type & ~IPF_MASK) > IPT_ANALOG_START)
					&& ((in->type & ~IPF_MASK) < IPT_ANALOG_END)) {
				/* We found a joystick */
				if (portval)
					in->type |= IPF_CENTER;
				else
					in->type &= ~IPF_CENTER;
			}
		}
	}
}

static void autocenter_init(int dipport, int dipmask)
{
	autocenter_val = -1;
	timer_pulse(TIME_IN_HZ(10), (dipport << 8) | dipmask, autocenter_timer_proc);
}

/***************************************************************************
  Cassette support
***************************************************************************/

static void coco_cassette_calcchunkinfo(void *file, int *chunk_size,
	int *chunk_samples)
{
	coco_wave_size = osd_fsize(file);
	*chunk_size = coco_wave_size;
	*chunk_samples = 8*8 * coco_wave_size;	/* 8 bits * 4 samples */
}

static struct cassette_args coco_cassette_args =
{
	WAVE_STATUS_MOTOR_ENABLE | WAVE_STATUS_MUTED
		| WAVE_STATUS_MOTOR_INHIBIT,				/* initial_status */
	coco_cassette_fill_wave,									/* fill_wave */
	coco_cassette_calcchunkinfo,					/* calc_chunk_info */
	4800,											/* input_smpfreq */
	COCO_WAVESAMPLES_HEADER,						/* header_samples */
	COCO_WAVESAMPLES_TRAILER,						/* trailer_samples */
	0,												/* NA */
	0,												/* NA */
	19200											/* create_smpfreq */
};

int coco_cassette_init(int id)
{
	return cassette_init(id, &coco_cassette_args);
}

void coco_cassette_exit(int id)
{
	device_close(IO_CASSETTE,id);
}

/***************************************************************************
  Cartridge Expansion Slot
 ***************************************************************************/

static void coco_cartrige_init(const struct cartridge_slot *cartinterface, const struct cartridge_callback *callbacks)
{
	coco_cart_interface = cartinterface;

	if (cartinterface)
		cartinterface->init(callbacks);
}

READ_HANDLER(coco_cartridge_r)
{
	return (coco_cart_interface && coco_cart_interface->io_r) ? coco_cart_interface->io_r(offset) : 0;
}

WRITE_HANDLER(coco_cartridge_w)
{
	if (coco_cart_interface && coco_cart_interface->io_w)
		coco_cart_interface->io_w(offset, data);
}

READ_HANDLER(coco3_cartridge_r)
{
	/* This behavior is documented in Super Extended Basic Unravelled, page 14 */
	return ((coco3_gimereg[0] & 0x04) || (offset >= 0x10)) ? coco_cartridge_r(offset) : 0;
}

WRITE_HANDLER(coco3_cartridge_w)
{
	/* This behavior is documented in Super Extended Basic Unravelled, page 14 */
	if ((coco3_gimereg[0] & 0x04) || (offset >= 0x10))
		coco_cartridge_w(offset, data);
}


static void coco_cartridge_enablesound(int enable)
{
	if (coco_cart_interface && coco_cart_interface->enablesound)
		coco_cart_interface->enablesound(enable);
}

/***************************************************************************
  Bitbanger port
***************************************************************************/

static void *bitbanger_file;
static int bitbanger_word;
static int bitbanger_line;

static void coco_bitbanger_poll(int dummy)
{
	char c;

	bitbanger_word >>= 1;
	if (bitbanger_line)
		bitbanger_word |= 0x400;

	if ((bitbanger_word & 0x403) == 0x401) {
		c = (char) (bitbanger_word >> 2);
		if (bitbanger_file)
			osd_fwrite(bitbanger_file, &c, 1);
		bitbanger_word = 0;
	}
}

int coco_bitbanger_init (int id)
{
	bitbanger_word = 0;
	bitbanger_line = 0;
	bitbanger_file = image_fopen (IO_BITBANGER, id, OSD_FILETYPE_IMAGE, OSD_FOPEN_RW_CREATE);
	return INIT_PASS;
}

void coco_bitbanger_exit (int id)
{
	if (bitbanger_file)
		osd_fclose(bitbanger_file);
}

void coco_bitbanger_output (int id, int data)
{
	bitbanger_line = data;
}

/***************************************************************************
  Machine Initialization
***************************************************************************/

static void coco_setcartline(int data)
{
	cart_line = data;
	pia_1_cb1_w(0, data ? ASSERT_LINE : CLEAR_LINE);
}

static void coco3_setcartline(int data)
{
	cart_line = data;
	pia_1_cb1_w(0, data ? ASSERT_LINE : CLEAR_LINE);
	coco3_raise_interrupt(COCO3_INT_EI0, cart_line ? 1 : 0);
}

static int count_bank(void)
{
	unsigned int	crc;
	/* This function, and all calls of it, are hacks for bankswitched games */

	crc = device_crc(IO_CARTSLOT, 0);

	switch( crc )
	{
		case 0x83bd6056:		/* Mind-Roll */
			return 1;
			break;
		case 0xBF4AD8F8:		/* Predator */
			return 3;
			break;
		case 0xDD94DD06:		/* RoboCop */
			return 7;
			break;
		default:				/* No bankswitching */
			return 0;
			break;
	}
}

static int is_Orch90(void)
{
	unsigned int	crc;
	/* This function, and all calls of it, are hacks for bankswitched games */

	crc = device_crc(IO_CARTSLOT, 0);

	return crc == 0x15FB39AF;
}

static void generic_setcartbank(int bank, UINT8 *cartpos)
{
	void *fp;

	if (count_bank() > 0) {
		/* Pin variable to proper bit width */
		bank &= count_bank();
		fp = image_fopen(IO_CARTSLOT, 0, OSD_FILETYPE_IMAGE, 0);
		if (fp) {
			if (bank)
				osd_fseek(fp, 0x4000 * bank, SEEK_SET);
			osd_fread(fp, cartpos, 0x4000);
			osd_fclose(fp);
		}
	}
}

static void coco_setcartbank(int bank)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	generic_setcartbank(bank, &ROM[0x14000]);
}

static void coco3_setcartbank(int bank)
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	generic_setcartbank(bank, &ROM[0x8C000]);
}

static const struct cartridge_callback coco_cartcallbacks =
{
	coco_setcartline,
	coco_setcartbank
};

static const struct cartridge_callback coco3_cartcallbacks =
{
	coco3_setcartline,
	coco3_setcartbank
};

static void generic_init_machine(struct pia6821_interface *piaintf, struct sam6883_interface *samintf, const struct cartridge_slot *cartinterface, const struct cartridge_callback *cartcallback)
{
	const struct cartridge_slot *cartslottype;

	cart_line = CARTLINE_CLEAR;
	pia0_irq_a = CLEAR_LINE;
	pia0_irq_b = CLEAR_LINE;
	pia1_firq_a = CLEAR_LINE;
	pia1_firq_b = CLEAR_LINE;

	pia0_pb = pia1_pb1 = soundmux_status = tape_motor = 0;
	joystick_axis = joystick = 0;
	d_dac = 0;

	pia_config(0, PIA_STANDARD_ORDERING | PIA_8BIT, &piaintf[0]);
	pia_config(1, PIA_STANDARD_ORDERING | PIA_8BIT, &piaintf[1]);
	pia_reset();

	sam_init(samintf);

	if (trailer_load) {
		trailer_load = 0;
		timer_set(0, 0, pak_load_trailer_callback);
	}

	/* HACK for bankswitching carts */
	if( is_Orch90() )
		cartslottype = &cartridge_Orch90;
	else
	    cartslottype = (count_bank() > 0) ? &cartridge_banks : &cartridge_standard;

	coco_cartrige_init(cart_inserted ? cartslottype : cartinterface, cartcallback);
	autocenter_init(12, 0x04);

	timer_pulse(TIME_IN_HZ(600), 0, coco_bitbanger_poll);
}

void dragon32_init_machine(void)
{
	coco_rom = memory_region(REGION_CPU1) + 0x8000;
	generic_init_machine(dragon_pia_intf, &dragon_sam_intf, &cartridge_fdc_dragon, &coco_cartcallbacks);
}

void coco_init_machine(void)
{
	coco_rom = memory_region(REGION_CPU1) + 0x10000;
	generic_init_machine(dragon_pia_intf, &dragon64_sam_intf, &cartridge_fdc_coco, &coco_cartcallbacks);
}

void coco2_init_machine(void)
{
	coco_rom = memory_region(REGION_CPU1) + 0x10000;
	generic_init_machine(coco2_pia_intf, &dragon64_sam_intf, &cartridge_fdc_coco, &coco_cartcallbacks);
}

void coco3_init_machine(void)
{
	int i;

	/* Tepolt verifies that the GIME registers are all cleared on initialization */

	coco3_enable_64k = 0;
	gime_irq = 0;
	gime_firq = 0;
	for (i = 0; i < 8; i++) {
		coco3_mmu[i] = coco3_mmu[i + 8] = 56 + i;
		coco3_gimereg[i] = 0;
	}

	coco_rom = memory_region(REGION_CPU1) + 0x80000;
	generic_init_machine(coco3_pia_intf, &coco3_sam_intf, &cartridge_fdc_coco, &coco3_cartcallbacks);

	coco3_mmu_update(0, 8);
	coco3_timer_init();

	coco3_interupt_line = 0;

	/* The choise of 50hz is arbitrary */
	timer_pulse(TIME_IN_HZ(50), 0, coco3_poll_keyboard);
}

void dragon_stop_machine(void)
{
	if (coco_cart_interface && coco_cart_interface->term)
		coco_cart_interface->term();
	sam_reset();
}

/***************************************************************************
  OS9 Syscalls (This is a helper not enabled by default to aid in logging
****************************************************************************/

#if LOG_OS9

static const char *os9syscalls[] = {
	"F$Link",          /* Link to Module */
	"F$Load",          /* Load Module from File */
	"F$UnLink",        /* Unlink Module */
	"F$Fork",          /* Start New Process */
	"F$Wait",          /* Wait for Child Process to Die */
	"F$Chain",         /* Chain Process to New Module */
	"F$Exit",          /* Terminate Process */
	"F$Mem",           /* Set Memory Size */
	"F$Send",          /* Send Signal to Process */
	"F$Icpt",          /* Set Signal Intercept */
	"F$Sleep",         /* Suspend Process */
	"F$SSpd",          /* Suspend Process */
	"F$ID",            /* Return Process ID */
	"F$SPrior",        /* Set Process Priority */
	"F$SSWI",          /* Set Software Interrupt */
	"F$PErr",          /* Print Error */
	"F$PrsNam",        /* Parse Pathlist Name */
	"F$CmpNam",        /* Compare Two Names */
	"F$SchBit",        /* Search Bit Map */
	"F$AllBit",        /* Allocate in Bit Map */
	"F$DelBit",        /* Deallocate in Bit Map */
	"F$Time",          /* Get Current Time */
	"F$STime",         /* Set Current Time */
	"F$CRC",           /* Generate CRC */
	"F$GPrDsc",        /* get Process Descriptor copy */
	"F$GBlkMp",        /* get System Block Map copy */
	"F$GModDr",        /* get Module Directory copy */
	"F$CpyMem",        /* Copy External Memory */
	"F$SUser",         /* Set User ID number */
	"F$UnLoad",        /* Unlink Module by name */
	"F$Alarm",         /* Color Computer Alarm Call (system wide) */
	NULL,
	NULL,
	"F$NMLink",        /* Color Computer NonMapping Link */
	"F$NMLoad",        /* Color Computer NonMapping Load */
	NULL,
	NULL,
	"F$TPS",           /* Return System's Ticks Per Second */
	"F$TimAlm",        /* COCO individual process alarm call */
	"F$VIRQ",          /* Install/Delete Virtual IRQ */
	"F$SRqMem",        /* System Memory Request */
	"F$SRtMem",        /* System Memory Return */
	"F$IRQ",           /* Enter IRQ Polling Table */
	"F$IOQu",          /* Enter I/O Queue */
	"F$AProc",         /* Enter Active Process Queue */
	"F$NProc",         /* Start Next Process */
	"F$VModul",        /* Validate Module */
	"F$Find64",        /* Find Process/Path Descriptor */
	"F$All64",         /* Allocate Process/Path Descriptor */
	"F$Ret64",         /* Return Process/Path Descriptor */
	"F$SSvc",          /* Service Request Table Initialization */
	"F$IODel",         /* Delete I/O Module */
	"F$SLink",         /* System Link */
	"F$Boot",          /* Bootstrap System */
	"F$BtMem",         /* Bootstrap Memory Request */
	"F$GProcP",        /* Get Process ptr */
	"F$Move",          /* Move Data (low bound first) */
	"F$AllRAM",        /* Allocate RAM blocks */
	"F$AllImg",        /* Allocate Image RAM blocks */
	"F$DelImg",        /* Deallocate Image RAM blocks */
	"F$SetImg",        /* Set Process DAT Image */
	"F$FreeLB",        /* Get Free Low Block */
	"F$FreeHB",        /* Get Free High Block */
	"F$AllTsk",        /* Allocate Process Task number */
	"F$DelTsk",        /* Deallocate Process Task number */
	"F$SetTsk",        /* Set Process Task DAT registers */
	"F$ResTsk",        /* Reserve Task number */
	"F$RelTsk",        /* Release Task number */
	"F$DATLog",        /* Convert DAT Block/Offset to Logical */
	"F$DATTmp",        /* Make temporary DAT image (Obsolete) */
	"F$LDAXY",         /* Load A [X,[Y]] */
	"F$LDAXYP",        /* Load A [X+,[Y]] */
	"F$LDDDXY",        /* Load D [D+X,[Y]] */
	"F$LDABX",         /* Load A from 0,X in task B */
	"F$STABX",         /* Store A at 0,X in task B */
	"F$AllPrc",        /* Allocate Process Descriptor */
	"F$DelPrc",        /* Deallocate Process Descriptor */
	"F$ELink",         /* Link using Module Directory Entry */
	"F$FModul",        /* Find Module Directory Entry */
	"F$MapBlk",        /* Map Specific Block */
	"F$ClrBlk",        /* Clear Specific Block */
	"F$DelRAM",        /* Deallocate RAM blocks */
	"F$GCMDir",        /* Pack module directory */
	"F$AlHRam",        /* Allocate HIGH RAM Blocks */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"F$RegDmp",        /* Ron Lammardo's debugging register dump call */
	"F$NVRAM",         /* Non Volatile RAM (RTC battery backed static) read/write */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	"I$Attach",        /* Attach I/O Device */
	"I$Detach",        /* Detach I/O Device */
	"I$Dup",           /* Duplicate Path */
	"I$Create",        /* Create New File */
	"I$Open",          /* Open Existing File */
	"I$MakDir",        /* Make Directory File */
	"I$ChgDir",        /* Change Default Directory */
	"I$Delete",        /* Delete File */
	"I$Seek",          /* Change Current Position */
	"I$Read",          /* Read Data */
	"I$Write",         /* Write Data */
	"I$ReadLn",        /* Read Line of ASCII Data */
	"I$WritLn",        /* Write Line of ASCII Data */
	"I$GetStt",        /* Get Path Status */
	"I$SetStt",        /* Set Path Status */
	"I$Close",         /* Close Path */
	"I$DeletX"         /* Delete from current exec dir */
};

static const char *getos9call(int call)
{
	return (call >= (sizeof(os9syscalls) / sizeof(os9syscalls[0]))) ? NULL : os9syscalls[call];
}

void log_os9call(int call)
{
	const char *mnemonic;

	mnemonic = getos9call(call);
	if (!mnemonic)
		mnemonic = "(unknown)";

	logerror("Logged OS9 Call Through SWI2 $%02x (%s): pc=$%04x\n", (void *) call, mnemonic, cpu_get_pc());
}

void os9_in_swi2(void)
{
	unsigned pc;
	pc = cpu_get_pc();
	log_os9call(cpu_readmem16(pc));
}

#endif /* LOG_OS9 */

/***************************************************************************
  Other hardware
****************************************************************************

  This section discusses hardware/accessories/enhancements to the CoCo (mainly
  CoCo 3) that exist but are not emulated yet.

  TWO MEGABYTE UPGRADE - CoCo's could be upgraded to have two megabytes of RAM.
  This worked by doing the following things (info courtesy: LCB):

		1,  All MMU registers are extended to 8 bits.  Note that even if they
			store eight bits of data, on reading they are only 6 bits.
		2.	The low two bits of $FF9B now extend the video start register.  It
			is worth noting that these two bits are write-only, and if the
			video is at the end of a 512k bank, it wraps around inside the
			current 512k bank.

  EIGHT MEGABYTE UPGRADE - This upgrade extends $FF9B so that video and the MMU
  support 8 MB of memory.  I am not sure about the details.

***************************************************************************/


