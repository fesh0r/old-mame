/******************************************************************************
 * Enterprise 128k driver
 *
 * Kevin Thacker 1999.
 *
 * James Boulton [EP help]
 * Jean-Pierre Malisse [EP help]
 *
 * EP Hardware: Z80 (CPU), Dave (Sound Chip + I/O)
 * Nick (Graphics), WD1772 (FDC). 128k ram.
 *
 * For an 8-bit machine, this kicks ass! A sound
 * Chip which is as powerful, or more powerful than
 * the C64 SID, and a graphics chip capable of some
 * really nice graphics. Pity it doesn't have HW sprites!
 ******************************************************************************/

#include "driver.h"
#include "sndhrdw/dave.h"
#include "includes/enterp.h"
#include "vidhrdw/epnick.h"
#include "includes/wd179x.h"
#include "cpuintrf.h"
#include "includes/basicdsk.h"
/* for CPCEMU style disk images */
#include "includes/dsk.h"

/* there are 64us per line, although in reality
   about 50 are visible. */
/* there are 312 lines per screen, although in reality
   about 35*8 are visible */
#define ENTERPRISE_SCREEN_WIDTH (50*16)
#define ENTERPRISE_SCREEN_HEIGHT	(35*8)

/* Enterprise bank allocations */
#define MEM_EXOS_0		0
#define MEM_EXOS_1		1
#define MEM_CART_0		4
#define MEM_CART_1		5
#define MEM_CART_2		6
#define MEM_CART_3		7
#define MEM_EXDOS_0 	0x020
#define MEM_EXDOS_1 	0x021
/* basic 64k ram */
#define MEM_RAM_0				((unsigned int)0x0fc)
#define MEM_RAM_1				((unsigned int)0x0fd)
#define MEM_RAM_2				((unsigned int)0x0fe)
#define MEM_RAM_3				((unsigned int)0x0ff)
/* additional 64k ram */
#define MEM_RAM_4				((unsigned int)0x0f8)
#define MEM_RAM_5				((unsigned int)0x0f9)
#define MEM_RAM_6				((unsigned int)0x0fa)
#define MEM_RAM_7				((unsigned int)0x0fb)

/* base of all ram allocated - 128k */
unsigned char *Enterprise_RAM;


WRITE_HANDLER ( Nick_reg_w );


/* The Page index for each 16k page is programmed into
Dave. This index is a 8-bit number. The array below
defines what data is pointed to by each of these page index's
that can be selected. If NULL, when reading return floating
bus byte, when writing ignore */
static unsigned char * Enterprise_Pages_Read[256];
static unsigned char * Enterprise_Pages_Write[256];

/* index of keyboard line to read */
static int Enterprise_KeyboardLine = 0;

/* set read/write pointers for CPU page */
void	Enterprise_SetMemoryPage(int CPU_Page, int EP_Page)
{
	cpu_setbank((CPU_Page+1), Enterprise_Pages_Read[EP_Page & 0x0ff]);
	cpu_setbank((CPU_Page+5), Enterprise_Pages_Write[EP_Page & 0x0ff]);
}

/* EP specific handling of dave register write */
static void enterprise_dave_reg_write(int RegIndex, int Data)
{
	switch (RegIndex)
	{

	case 0x010:
		{
		  /* set CPU memory page 0 */
			Enterprise_SetMemoryPage(0, Data);
		}
		break;

	case 0x011:
		{
		  /* set CPU memory page 1 */
			Enterprise_SetMemoryPage(1, Data);
		}
		break;

	case 0x012:
		{
		  /* set CPU memory page 2 */
			Enterprise_SetMemoryPage(2, Data);
		}
		break;

	case 0x013:
		{
		  /* set CPU memory page 3 */
			Enterprise_SetMemoryPage(3, Data);
		}
		break;

	case 0x015:
		{
		  /* write keyboard line */
			Enterprise_KeyboardLine = Data & 15;
		}
		break;

	default:
		break;
	}
}

static void enterprise_dave_reg_read(int RegIndex)
{
	switch (RegIndex)
	{
	case 0x015:
		{
		  /* read keyboard line */
			Dave_setreg(0x015,
				readinputport(Enterprise_KeyboardLine));
		}
		break;

		case 0x016:
		{
				int ExternalJoystickInputs;
				int ExternalJoystickPortInput = readinputport(10);

				if (Enterprise_KeyboardLine<=4)
				{
						ExternalJoystickInputs = ExternalJoystickPortInput>>(4-Enterprise_KeyboardLine);
				}
				else
				{
						ExternalJoystickInputs = 1;
				}

				Dave_setreg(0x016, (0x0fe | (ExternalJoystickInputs & 0x01)));
		}
		break;

	default:
		break;
	}
}

void enterprise_dave_interrupt(int state)
{
	if (state)
		cpu_set_irq_line(0,0,HOLD_LINE);
	else
		cpu_set_irq_line(0,0,CLEAR_LINE);
}

/* enterprise interface to dave - ok, so Dave chip is unique
to Enterprise. But these functions make it nice and easy to see
whats going on. */
DAVE_INTERFACE	enterprise_dave_interface=
{
	enterprise_dave_reg_read,
		enterprise_dave_reg_write,
		enterprise_dave_interrupt,
};


static void enterp_wd177x_callback(int);

void Enterprise_Initialise()
{
	int i;

	for (i=0; i<256; i++)
	{
		/* reads to memory pages that are not set returns 0x0ff */
		Enterprise_Pages_Read[i] = Enterprise_RAM+0x020000;
		/* writes to memory pages that are not set are ignored */
		Enterprise_Pages_Write[i] = Enterprise_RAM+0x024000;
	}

	/* setup dummy read area so it will always return 0x0ff */
	memset(Enterprise_RAM+0x020000, 0x0ff, 0x04000);

	/* set read pointers */
	/* exos */
	Enterprise_Pages_Read[MEM_EXOS_0] = &memory_region(REGION_CPU1)[0x010000];
	Enterprise_Pages_Read[MEM_EXOS_1] = &memory_region(REGION_CPU1)[0x014000];
	/* basic */
	Enterprise_Pages_Read[MEM_CART_0] = &memory_region(REGION_CPU1)[0x018000];
	/* ram */
	Enterprise_Pages_Read[MEM_RAM_0] = Enterprise_RAM;
	Enterprise_Pages_Read[MEM_RAM_1] = Enterprise_RAM + 0x04000;
	Enterprise_Pages_Read[MEM_RAM_2] = Enterprise_RAM + 0x08000;
	Enterprise_Pages_Read[MEM_RAM_3] = Enterprise_RAM + 0x0c000;
	Enterprise_Pages_Read[MEM_RAM_4] = Enterprise_RAM + 0x010000;
	Enterprise_Pages_Read[MEM_RAM_5] = Enterprise_RAM + 0x014000;
	Enterprise_Pages_Read[MEM_RAM_6] = Enterprise_RAM + 0x018000;
	Enterprise_Pages_Read[MEM_RAM_7] = Enterprise_RAM + 0x01c000;
	/* exdos */
	  Enterprise_Pages_Read[MEM_EXDOS_0] = &memory_region(REGION_CPU1)[0x01c000];
	  Enterprise_Pages_Read[MEM_EXDOS_1] = &memory_region(REGION_CPU1)[0x020000];

	/* set write pointers */
	Enterprise_Pages_Write[MEM_RAM_0] = Enterprise_RAM;
	Enterprise_Pages_Write[MEM_RAM_1] = Enterprise_RAM + 0x04000;
	Enterprise_Pages_Write[MEM_RAM_2] = Enterprise_RAM + 0x08000;
	Enterprise_Pages_Write[MEM_RAM_3] = Enterprise_RAM + 0x0c000;
	Enterprise_Pages_Write[MEM_RAM_4] = Enterprise_RAM + 0x010000;
	Enterprise_Pages_Write[MEM_RAM_5] = Enterprise_RAM + 0x014000;
	Enterprise_Pages_Write[MEM_RAM_6] = Enterprise_RAM + 0x018000;
	Enterprise_Pages_Write[MEM_RAM_7] = Enterprise_RAM + 0x01c000;


	Dave_Init();

	Dave_SetIFace(&enterprise_dave_interface);

	memory_set_bankhandler_r(1, 0, MRA_BANK1);
	memory_set_bankhandler_r(2, 0, MRA_BANK2);
	memory_set_bankhandler_r(3, 0, MRA_BANK3);
	memory_set_bankhandler_r(4, 0, MRA_BANK4);

	memory_set_bankhandler_w(5, 0, MWA_BANK5);
	memory_set_bankhandler_w(6, 0, MWA_BANK6);
	memory_set_bankhandler_w(7, 0, MWA_BANK7);
	memory_set_bankhandler_w(8, 0, MWA_BANK8);

	Dave_reg_w(0x010,0);
	Dave_reg_w(0x011,0);
	Dave_reg_w(0x012,0);
	Dave_reg_w(0x013,0);

	cpu_irq_line_vector_w(0,0,0x0ff);

	wd179x_init(WD_TYPE_177X,enterp_wd177x_callback);

	floppy_drive_set_geometry(0, FLOPPY_DRIVE_DS_80);
}

static READ_HANDLER ( enterprise_wd177x_read )
{
	switch (offset & 0x03)
	{
	case 0:
		return wd179x_status_r(offset);
	case 1:
		return wd179x_track_r(offset);
	case 2:
		return wd179x_sector_r(offset);
	case 3:
		return wd179x_data_r(offset);
	default:
		break;
	}

	return 0x0ff;
}

static WRITE_HANDLER (	enterprise_wd177x_write )
{
	switch (offset & 0x03)
	{
	case 0:
		wd179x_command_w(offset, data);
		return;
	case 1:
		wd179x_track_w(offset, data);
		return;
	case 2:
		wd179x_sector_w(offset, data);
		return;
	case 3:
		wd179x_data_w(offset, data);
		return;
	default:
		break;
	}
}



/* I've done this because the ram is banked in 16k blocks, and
the rom can be paged into bank 0 and bank 3. */
MEMORY_READ_START( readmem_enterprise )
	{ 0x00000, 0x03fff, MRA_BANK1 },
	{ 0x04000, 0x07fff, MRA_BANK2 },
	{ 0x08000, 0x0bfff, MRA_BANK3 },
	{ 0x0c000, 0x0ffff, MRA_BANK4 },
MEMORY_END


MEMORY_WRITE_START( writemem_enterprise )
	{ 0x00000, 0x03fff, MWA_BANK5 },
	{ 0x04000, 0x07fff, MWA_BANK6 },
	{ 0x08000, 0x0bfff, MWA_BANK7 },
	{ 0x0c000, 0x0ffff, MWA_BANK8 },
MEMORY_END

/* bit 0 - select drive 0,
   bit 1 - select drive 1,
   bit 2 - select drive 2,
   bit 3 - select drive 3
   bit 4 - side
   bit 5 - mfm/fm select
   bit 6 - disk change reset
   bit 7 - in use
*/

int EXDOS_GetDriveSelection(int data)
{
	 if (data & 0x01)
	 {
		return 0;
	 }

	 if (data & 0x02)
	 {
		return 1;
	 }

	 if (data & 0x04)
	 {
		return 2;
	 }

	 if (data & 0x08)
	 {
		return 3;
	 }

	 return 0;
}

static char EXDOS_CARD_R = 0;

static void enterp_wd177x_callback(int State)
{
   if (State==WD179X_IRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x02;
   }

   if (State==WD179X_IRQ_SET)
   {
		EXDOS_CARD_R |= 0x02;
   }

   if (State==WD179X_DRQ_CLR)
   {
		EXDOS_CARD_R &= ~0x080;
   }

   if (State==WD179X_DRQ_SET)
   {
		EXDOS_CARD_R |= 0x080;
   }
}



static WRITE_HANDLER ( exdos_card_w )
{
	/* drive side */
	int head = (data>>4) & 0x01;

	int drive = EXDOS_GetDriveSelection(data);

	wd179x_set_drive(drive);
	wd179x_set_side(head);
}

/* bit 0 - ??
   bit 1 - IRQ from WD1772
   bit 2 - ??
   bit 3 - ??
   bit 4 - ??
   bit 5 - ??
   bit 6 - Disk change signal from disk drive
   bit 7 - DRQ from WD1772
*/


static READ_HANDLER ( exdos_card_r )
{
	return EXDOS_CARD_R;
}

PORT_READ_START( readport_enterprise )
	{ 0x010, 0x017, enterprise_wd177x_read },
	{ 0x018, 0x018, exdos_card_r },
	{ 0x01c, 0x01c, exdos_card_r },
	{ 0x0a0, 0x0bf, Dave_reg_r },
PORT_END


PORT_WRITE_START( writeport_enterprise )
	{ 0x010, 0x017, enterprise_wd177x_write },
	{ 0x018, 0x018, exdos_card_w },
	{ 0x01c, 0x01c, exdos_card_w },
	{ 0x080, 0x08f, Nick_reg_w },
	{ 0x0a0, 0x0bf, Dave_reg_w },
PORT_END

/*
Enterprise Keyboard Matrix

		Bit
Line	0	 1	  2    3	4	 5	  6    7
0		n	 \	  b    c	v	 x	  z    SHFT
1		h	 N/C  g    d	f	 s	  a    CTRL
2		u	 q	  y    r	t	 e	  w    TAB
3		7	 1	  6    4	5	 3	  2    N/C
4		F4	 F8   F3   F6	F5	 F7   F2   F1
5		8	 N/C  9    -	0	 ^	  DEL  N/C
6		j	 N/C  k    ;	l	 :	  ]    N/C
7		STOP DOWN RGHT UP	HOLD LEFT RETN ALT
8		m	 ERSE ,    /	.	 SHFT SPCE INS
9		i	 N/C  o    @	p	 [	  N/C  N/C

N/C - Not connected or just dont know!
*/


INPUT_PORTS_START( ep128 )
	/* keyboard line 0 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n", KEYCODE_N,IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\", KEYCODE_SLASH,IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "b", KEYCODE_B,IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "c", KEYCODE_C, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "v", KEYCODE_V, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "x", KEYCODE_X, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "z", KEYCODE_Z, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)

	 /* keyboard line 1 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "h", KEYCODE_H, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "g", KEYCODE_G, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "d", KEYCODE_D, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "f", KEYCODE_F, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s", KEYCODE_S, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "a", KEYCODE_A, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)

	 /* keyboard line 2 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "u", KEYCODE_U, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "q", KEYCODE_Q, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y", KEYCODE_Y, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "r", KEYCODE_R, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "t", KEYCODE_T, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "e", KEYCODE_E, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w", KEYCODE_W, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "TAB", KEYCODE_TAB, IP_JOY_NONE)

	 /* keyboard line 3 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)

	 /* keyboard line 4 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "f4", KEYCODE_F4, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "f8", KEYCODE_F8, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "f3", KEYCODE_F3, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "f6", KEYCODE_F6, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "f5", KEYCODE_F5, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f7", KEYCODE_F7, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "f2", KEYCODE_F2, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "f1", KEYCODE_F1, IP_JOY_NONE)

	 /* keyboard line 5 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "0", KEYCODE_0, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "^", KEYCODE_EQUALS, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL", KEYCODE_BACKSPACE, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c",IP_KEY_NONE, IP_JOY_NONE)

	 /* keyboard line 6 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "j", KEYCODE_J,IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "k", KEYCODE_K, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";", KEYCODE_COLON, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "l", KEYCODE_L, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, ":", KEYCODE_QUOTE, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "]", KEYCODE_CLOSEBRACE, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)

	 /* keyboard line 7 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "STOP", KEYCODE_END, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "DOWN", KEYCODE_DOWN, JOYCODE_1_DOWN)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "RIGHT", KEYCODE_RIGHT, JOYCODE_1_RIGHT)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "UP", KEYCODE_UP, JOYCODE_1_UP)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "HOLD", KEYCODE_HOME, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "LEFT", KEYCODE_LEFT, JOYCODE_1_LEFT)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "RETURN", KEYCODE_ENTER, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "ALT", KEYCODE_LALT, IP_JOY_NONE)


	 /* keyboard line 8 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m", KEYCODE_M, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "ERASE", KEYCODE_DEL, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "INSERT", KEYCODE_INSERT, IP_JOY_NONE)


	 /* keyboard line 9 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "i", KEYCODE_I, IP_JOY_NONE)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o", KEYCODE_O, IP_JOY_NONE)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "@", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "p", KEYCODE_P, IP_JOY_NONE)
	 PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "[", KEYCODE_OPENBRACE, IP_JOY_NONE)
	 PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)
	 PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "n/c", IP_KEY_NONE, IP_JOY_NONE)

	 /* external joystick 1 */
	 PORT_START
	 PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXTERNAL JOYSTICK 1 RIGHT", KEYCODE_RIGHT, JOYCODE_1_RIGHT)
	 PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXTERNAL JOYSTICK 1 LEFT", KEYCODE_LEFT, JOYCODE_1_LEFT)
	 PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXTERNAL JOYSTICK 1 DOWN", KEYCODE_DOWN, JOYCODE_1_DOWN)
	 PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXTERNAL JOYSTICK 1 UP", KEYCODE_UP, JOYCODE_1_UP)
	 PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "EXTERNAL JOYSTICK 1 FIRE", KEYCODE_SPACE, JOYCODE_1_BUTTON1)


INPUT_PORTS_END

int	enterprise_dsk_floppy_init(int id)
{
	 if (device_filename(IO_FLOPPY,id)==NULL)
		 return INIT_PASS;


	 return dsk_floppy_load(id);
 }



static struct CustomSound_interface dave_custom_sound=
{
	Dave_sh_start,
	Dave_sh_stop,
	Dave_sh_update
};

/* 4Mhz clock, although it can be changed to 8 Mhz! */

static struct MachineDriver machine_driver_ep128 =
{
	/* basic machine hardware */
	{
		/* MachineCPU */
		{
			CPU_Z80,							/* type */
			4000000,							/* clock: See Note Above */
			readmem_enterprise, 				/* MemoryReadAddress */
			writemem_enterprise,				/* MemoryWriteAddress */
			readport_enterprise,				/* IOReadPort */
			writeport_enterprise,				/* IOWritePort */
						0, 0,
						0, 0,
		},
	},
	50, 										/* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,				/* vblank duration */
	1,											/* cpu slices per frame */
	enterprise_init_machine,					/* init machine */
	enterprise_shutdown_machine,
	/* video hardware */
	ENTERPRISE_SCREEN_WIDTH,					/* screen width */
	ENTERPRISE_SCREEN_HEIGHT,					/* screen height */
	{ 0,(ENTERPRISE_SCREEN_WIDTH-1),0,(ENTERPRISE_SCREEN_HEIGHT-1)}, /* rectangle: visible_area */
	0, /*enterprise_gfxdecodeinfo,*/			/* graphics decode info */
	NICK_PALETTE_SIZE,							/* total colours */
	NICK_COLOURTABLE_SIZE,						/* color table len */
	nick_init_palette,							/* convert color prom */

	VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2,							/* video attributes */
	0,											/* MachineLayer */
	enterprise_vh_start,
	enterprise_vh_stop,
	enterprise_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		/* MachineSound */
		/* change to dave eventually */
		{ SOUND_CUSTOM, &dave_custom_sound},
	}
};



ROM_START( ep128 )
		/* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
		ROM_REGION(0x24000,REGION_CPU1,0)
		ROM_LOAD("exos.rom",0x10000,0x8000,  0xd421795f)
		ROM_LOAD("exbas.rom",0x18000,0x4000, 0x683cf455)
		ROM_LOAD("exdos.rom",0x1c000,0x8000, 0xd1d7e157)
ROM_END

ROM_START( ep128a )
		/* 128k ram + 32k rom (OS) + 16k rom (BASIC) + 32k rom (EXDOS) */
		ROM_REGION(0x24000,REGION_CPU1,0)
		ROM_LOAD("exos21.rom",0x10000,0x8000,  0x982a3b44)
		ROM_LOAD("exbas.rom",0x18000,0x4000, 0x683cf455)
		ROM_LOAD("exdos.rom",0x1c000,0x8000, 0xd1d7e157)
ROM_END

#define io_ep128a io_ep128

static const struct IODevice io_ep128[] = {
#if 0
	{
		IO_FLOPPY,				/* type */
		4,						/* count */
		"dsk\0",                /* file extensions */
		IO_RESET_NONE,			/* reset if file changed */
		0,
		enterprise_floppy_init, /* init */
		basicdsk_floppy_exit,	/* exit */
		NULL,					/* info */
		NULL,					/* open */
		NULL,					/* close */
		floppy_status,			/* status */
		NULL,					/* seek */
		NULL,					/* tell */
		NULL,					/* input */
		NULL,					/* output */
		NULL,					/* input_chunk */
		NULL					/* output_chunk */
	},
#endif
	{
		IO_FLOPPY,					/* type */
		4,							/* count */
		"dsk\0",                    /* file extensions */
		IO_RESET_NONE,				/* reset if file changed */
		0,
		enterprise_dsk_floppy_init,			/* init */
		dsk_floppy_exit,			/* exit */
		NULL,						/* info */
		NULL,						/* open */
		NULL,						/* close */
                floppy_status,                                           /* status */
                NULL,                                           /* seek */
		NULL,						/* tell */
		NULL,						/* input */
		NULL,						/* output */
		NULL,						/* input_chunk */
		NULL						/* output_chunk */
	},
	{ IO_END }
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	  PARENT	MACHINE   INPUT 	INIT	  COMPANY	FULLNAME */
COMPX( 1984, ep128,   0,		ep128,	  ep128,	0,		  "Intelligent Software", "Enterprise 128", GAME_IMPERFECT_SOUND )
COMPX( 1984, ep128a,  ep128,	ep128,	  ep128,	0,		  "Intelligent Software", "Enterprise 128 (EXOS 2.1)", GAME_IMPERFECT_SOUND )

