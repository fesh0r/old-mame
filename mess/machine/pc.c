/***************************************************************************

	machine/pc.c

	Functions to emulate general aspects of the machine
	(RAM, ROM, interrupts, I/O ports)

	The information herein is heavily based on
	'Ralph Browns Interrupt List'
	Release 52, Last Change 20oct96

	TODO:
	clean up (maybe split) the different pieces of hardware
	PIC, PIT, DMA... add support for LPT, COM (almost done)
	emulation of a serial mouse on a COM port (almost done)
	support for Game Controller port at 0x0201
	support for XT harddisk (under the way, see machine/pc_hdc.c)
	whatever 'hardware' comes to mind,
	maybe SoundBlaster? EGA? VGA?

***************************************************************************/
#include <assert.h>
#include "driver.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"

#include "includes/pic8259.h"
#include "includes/pit8253.h"
#include "includes/mc146818.h"
#include "includes/dma8237.h"
#include "includes/uart8250.h"
#include "includes/vga.h"
#include "includes/pc_cga.h"
#include "includes/pc_mda.h"
#include "includes/pc_aga.h"

#include "includes/pc_flopp.h"
#include "includes/pc_mouse.h"
#include "includes/pckeybrd.h"

#include "includes/pclpt.h"
#include "includes/centroni.h"

#include "includes/pc_hdc.h"
#include "includes/nec765.h"
#include "includes/amstr_pc.h"
#include "includes/europc.h"
#include "includes/ibmpc.h"

#include "includes/pc.h"
#include "includes/state.h"

#define VERBOSE_DBG 0       /* general debug messages */
#if VERBOSE_DBG
#define DBG_LOG(N,M,A) \
	if(VERBOSE_DBG>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define DBG_LOG(n,m,a)
#endif

// preliminary machines setup 
//#define MESS_MENU
#ifdef MESS_MENU
#include "menu.h"
#include "menuentr.h"
#endif

#define FDC_DMA 2

/* called when a interrupt is set/cleared from com hardware */
static void pc_com_interrupt(int nr, int state)
{
	static const int irq[4]={4,3,4,3};
	/* issue COM1/3 IRQ4, COM2/4 IRQ3 */
	if (state)
	{
		pic8259_0_issue_irq(irq[nr]);
	}
}

/* called when com registers read/written - used to update peripherals that
are connected */
static void pc_com_refresh_connected(int n, int data)
{
	/* mouse connected to this port? */
	if (readinputport(3) & (0x80>>n))
		pc_mouse_handshake_in(n,data);
}

/* PC interface to PC-com hardware. Done this way because PCW16 also
uses PC-com hardware and doesn't have the same setup! */
static uart8250_interface com_interface[4]=
{
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	},
	{
		TYPE8250,
		1843200,
		pc_com_interrupt,
		NULL,
		pc_com_refresh_connected
	}
};

/*
 * timer0	heartbeat IRQ
 * timer1	DRAM refresh (ignored)
 * timer2	PIO port C pin 4 and speaker polling
 */
static PIT8253_CONFIG pc_pit8253_config={
	TYPE8253,
	{
		{
			4770000/4,				/* heartbeat IRQ */
			pic8259_0_issue_irq,
			NULL
		}, {
			4770000/4,				/* dram refresh */
			NULL,
			NULL
		}, {
			4770000/4,				/* pio port c pin 4, and speaker polling enough */
			NULL,
			pc_sh_speaker_change_clock
		}
	}
};

static PC_LPT_CONFIG lpt_config[3]={
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	},
	{
		1,
		LPT_UNIDIRECTIONAL,
		NULL
	}
};

static CENTRONICS_CONFIG cent_config[3]={
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	},
	{
		PRINTER_IBM,
		pc_lpt_handshake_in
	}
};

void pc_mda_init(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb0000, 0xbffff, MRA_RAM );
	install_mem_write_handler(0, 0xb0000, 0xbffff, pc_mda_videoram_w );
	videoram=memory_region(REGION_CPU1)+0xb0000; videoram_size=0x10000;

	install_port_read_handler(0, 0x3b0, 0x3bf, pc_MDA_r );
	install_port_write_handler(0, 0x3b0, 0x3bf, pc_MDA_w );
}

void pc_cga_init(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb8000, 0xbbfff, MRA_RAM );
	install_mem_write_handler(0, 0xb8000, 0xbbfff, pc_cga_videoram_w );
	videoram=memory_region(REGION_CPU1)+0xb8000; videoram_size=0x4000;

	install_port_read_handler(0, 0x3d0, 0x3df, pc_CGA_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc_CGA_w );
}

void pc_vga_init(void)
{
#if 0
        int i; 
        UINT8 *memory=memory_region(REGION_CPU1)+0xc0000;
        UINT8 chksum;

		/* oak vga */
        /* plausibility check of retrace signals goes wrong */
        memory[0x00f5]=memory[0x00f6]=memory[0x00f7]=0x90;
        memory[0x00f8]=memory[0x00f9]=memory[0x00fa]=0x90;
        for (chksum=0, i=0;i<0x7fff;i++) {
                chksum+=memory[i];
        }
        memory[i]=0x100-chksum;
#endif

#if 0
        for (chksum=0, i=0;i<0x8000;i++) {
                chksum+=memory[i];
        }
        printf("checksum %.2x\n",chksum);
#endif

	install_mem_read_handler(0, 0xa0000, 0xaffff, MRA_BANK1 );
	install_mem_read_handler(0, 0xb0000, 0xb7fff, MRA_BANK2 );
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA_BANK3 );
	install_mem_read_handler(0, 0xc0000, 0xc7fff, MRA_ROM );

	install_mem_write_handler(0, 0xa0000, 0xaffff, MWA_BANK1 );
	install_mem_write_handler(0, 0xb0000, 0xb7fff, MWA_BANK2 );
	install_mem_write_handler(0, 0xb8000, 0xbffff, MWA_BANK3 );
	install_mem_write_handler(0, 0xc0000, 0xc7fff, MWA_ROM );

	install_port_read_handler(0, 0x3b0, 0x3bf, vga_port_03b0_r );
	install_port_read_handler(0, 0x3c0, 0x3cf, vga_port_03c0_r );
	install_port_read_handler(0, 0x3d0, 0x3df, vga_port_03d0_r );

	install_port_write_handler(0, 0x3b0, 0x3bf, vga_port_03b0_w );
	install_port_write_handler(0, 0x3c0, 0x3cf, vga_port_03c0_w );
	install_port_write_handler(0, 0x3d0, 0x3df, vga_port_03d0_w );
}

#define SETUP_MEMORY_640 1
#define SETUP_SER_MOUSE 1
#define SETUP_FDC_SSDD 1
#define SETUP_FDC_DSDD 2
#define SETUP_FDC_DSDD80 3
#define SETUP_FDC_3DSDD 4
#define SETUP_FDC_DSHD 5
#define SETUP_FDC_3DSHD 6
#define SETUP_FDC_3DSED 7

typedef enum { 
	SETUP_END,
	SETUP_HEADER,
	SETUP_COMMENT,
	SETUP_MEMORY,
	SETUP_GRAPHIC0,
	SETUP_KEYB,
	SETUP_FDC0, 
	SETUP_FDC0D0, SETUP_FDC0D1, SETUP_FDC0D2, SETUP_FDC0D3,
	SETUP_HDC0, SETUP_HDC0D0, SETUP_HDC0D1,
	SETUP_RTC,
	SETUP_SER0, SETUP_SER0CHIP, SETUP_SER0DEV, 
	SETUP_SER1, SETUP_SER1CHIP, SETUP_SER1DEV, 
	SETUP_SER2, SETUP_SER2CHIP, SETUP_SER2DEV, 
	SETUP_SER3, SETUP_SER3CHIP, SETUP_SER3DEV,
	SETUP_SERIAL_MOUSE,
	SETUP_PAR0, SETUP_PAR0TYPE, SETUP_PAR0DEV, 
	SETUP_PAR1, SETUP_PAR1TYPE, SETUP_PAR1DEV, 
	SETUP_PAR2, SETUP_PAR2TYPE, SETUP_PAR2DEV,
	SETUP_GAME0, SETUP_GAME0C0, SETUP_GAME0C1,
	SETUP_MPU0, SETUP_MPU0D0,
	SETUP_FM, SETUP_FM_TYPE, SETUP_FM_PORT,
	SETUP_CMS, SETUP_CMS_TEXT,
	SETUP_PCJR_SOUND,
	SETUP_AMSTRAD_JOY, SETUP_AMSTRAD_MOUSE
} PC_ID;
typedef struct _PC_SETUP {
	PC_ID id;
	int def, mask; 
} PC_SETUP;


PC_SETUP pc_setup_at[]= {
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
}, pc_setup_t1000hx[]= {
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			4, 16 },
	{ SETUP_KEYB,				5, 32 },
	{ SETUP_MEMORY,				1, 3 }, //?
	{ SETUP_PCJR_SOUND,			1, 2 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				4, 16 },	
	{ SETUP_FDC0D2,				0, 31 },	
	{ SETUP_FDC0D3,				0, 31 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_GAME0,				1, 2 },
	{ SETUP_GAME0C0,			1, 3 },
	{ SETUP_GAME0C1,			1, 3 },
	{ SETUP_END }
};

static PC_SETUP pc1512[]={
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			6, 64 },
	{ SETUP_KEYB,				6, 64 },
	{ SETUP_AMSTRAD_JOY,		1, 3 },
	{ SETUP_AMSTRAD_MOUSE,		1, 3 },
	{ SETUP_MEMORY,				1, 3 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				2, 5 },	
	{ SETUP_FDC0D1,				0, 5 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_RTC,				1, 2 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_END }
};

static PC_SETUP pc1640[]={
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_GRAPHIC0,			5, 32 },
	{ SETUP_KEYB,				6, 64 },
	{ SETUP_AMSTRAD_JOY,		1, 3 },
	{ SETUP_AMSTRAD_MOUSE,		1, 3 },
	{ SETUP_MEMORY,				1, 3 },
	{ SETUP_FDC0,				1, 2 },
	{ SETUP_FDC0D0,				2, 5 },	
	{ SETUP_FDC0D1,				0, 5 },	
	{ SETUP_HDC0,				1, 3 },
	{ SETUP_HDC0D0,				1, 3 },
	{ SETUP_HDC0D1,				0, 3 },
	{ SETUP_RTC,				1, 2 },
	{ SETUP_SER0,				1, 2 },
	{ SETUP_SER0CHIP,			0, 1 },
	{ SETUP_SER0DEV,			0, 1 },
	{ SETUP_PAR0,				1, 2 },
	{ SETUP_PAR0TYPE,			0, 1 },
	{ SETUP_PAR0DEV,			1, 7 },
	{ SETUP_END }
};

static PC_SETUP pc_setup[]={
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
};

static PC_SETUP europc[]={
	{ SETUP_HEADER },
	{ SETUP_COMMENT },
	{ SETUP_END }
};

#ifdef MESS_MENU
/* must be reentrant! */
static void pc_update_setup(int id, int value)
{
	int flag=0;
	int v;

	/* avoid conflicts */
	switch (id) {
	case SETUP_SER0DEV: case SETUP_SER1DEV: case SETUP_SER2DEV: case SETUP_SER3DEV:
		if (value==SETUP_SER_MOUSE) {
			v=menu_get_value(m_setup, SETUP_SER0DEV);
			if ((id!=SETUP_SER0DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER0DEV, 0);

			v=menu_get_value(m_setup,SETUP_SER1DEV);
			if ((id!=SETUP_SER1DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER1DEV, 0);

			v=menu_get_value(m_setup, SETUP_SER2DEV);
			if ((id!=SETUP_SER2DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER2DEV, 0);

			v=menu_get_value(m_setup, SETUP_SER3DEV);
			if ((id!=SETUP_SER3DEV)&&(v==SETUP_SER_MOUSE) ) 
				menu_set_value(m_setup, SETUP_SER3DEV, 0);
		}
		break;
	case SETUP_FDC0:
		menu_set_visibility(m_setup, SETUP_FDC0D0, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D1, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D2, value!=0);
		menu_set_visibility(m_setup, SETUP_FDC0D3, value!=0);
		break;
	case SETUP_HDC0:
		menu_set_visibility(m_setup, SETUP_HDC0D0, value!=0);
		menu_set_visibility(m_setup, SETUP_HDC0D1, value!=0);
		break;
	case SETUP_SER0:
		menu_set_visibility(m_setup, SETUP_SER0CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER0DEV, value!=0);
		break;
	case SETUP_SER1:
		menu_set_visibility(m_setup, SETUP_SER1CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER1DEV, value!=0);
		break;
	case SETUP_SER2:
		menu_set_visibility(m_setup, SETUP_SER2CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER2DEV, value!=0);
		break;
	case SETUP_SER3:
		menu_set_visibility(m_setup, SETUP_SER3CHIP, value!=0);
		menu_set_visibility(m_setup, SETUP_SER3DEV, value!=0);
		break;
	case SETUP_PAR0:
		menu_set_visibility(m_setup, SETUP_PAR0TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR0DEV, value!=0);
		break;
	case SETUP_PAR1:
		menu_set_visibility(m_setup, SETUP_PAR1TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR1DEV, value!=0);
		break;
	case SETUP_PAR2:
		menu_set_visibility(m_setup, SETUP_PAR2TYPE, value!=0);
		menu_set_visibility(m_setup, SETUP_PAR2DEV, value!=0);
		break;
	}

	switch (id) {
	case SETUP_MEMORY:
		switch (value) {
		case SETUP_MEMORY_640: flag=1; break;
		}
		install_mem_read_handler(0, 0x80000, 0x9ffff, flag?MRA_RAM:MRA_ROM );
		install_mem_write_handler(0, 0x80000, 0x9ffff, flag?MWA_RAM:MWA_NOP );
		break;
	case SETUP_SER0DEV:
	}
}

static struct {
	PC_ID id;
	MENU_TEXT text;
} text_entries[]= {
	{ SETUP_HEADER, { "Machine Setup", MENU_TEXT_CENTER } },
	{ SETUP_COMMENT, { "(Reset is neccessary for several options)", MENU_TEXT_CENTER } },
	{ SETUP_CMS_TEXT, { " (Were also on Soundblaster 1.x)", MENU_TEXT_LEFT } },
};

static struct {
	PC_ID id;
	MENU_SELECTION selection;
} selection_entries[]= {
	{ SETUP_MEMORY, { "Memory", 1, pc_update_setup,
	  { "512KB", "640KB" } } },
	{ SETUP_GRAPHIC0, { "Graphics Adapter", 0, pc_update_setup,
	  { "CGA", "MDA", "Hercules", 
		"Tseng Labs ET4000", "PC Junior/Tandy 1000", 
		"Paradise EGA", "PC1512" } } },
	{ SETUP_KEYB, { "Keyboard", 4, pc_update_setup,
	  { "None", "US", "International", "MF2", "MF2 International", 
		"Tandy 1000", "Amstrad PC1512/1640" } } },
	{ SETUP_FDC0, { "Floppy controller", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x3f0 IRQ:6 DMA:2" } } },
	{ SETUP_FDC0D0, { " Floppy Drive 0", 2, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D1, { " Floppy Drive 1", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D2, { " Floppy Drive 2", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_FDC0D3, { " Floppy Drive 3", 0, pc_update_setup,
	  { DEF_STR( Off ), 
		"5.25 Inch, SS, DD (160, 180KB)", 
		"5.25 Inch, DS, DD (320, 360KB)",
		"5.25 Inch, DS, DD, 80 tracks (720KB)",
		"3.5 Inch, DS, DD, 80 tracks (720KB)" } } },
	{ SETUP_HDC0, { "Harddisk controller 0", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x320" } } },
	{ SETUP_HDC0D0, { " Harddisk 0", 0, pc_update_setup, 
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_HDC0D1, { " Harddisk 1", 0, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_RTC, { "MC146818 RTC with batterie bufferd CMOS RAM", 0, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER0, { "Serial Port at IO:0x3f8 IRQ:4 (COM1)", 1, pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER0CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER0DEV, { " Device", 1,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER1, { "Serial Port at IO:0x2f8 IRQ:3 (COM2)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER1CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER1DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER2, { "Serial Port at IO:0x3e8 IRQ:4 (COM3)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER2CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER2DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SER3, { "Serial Port at IO:0x2e8 IRQ:3 (COM4)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_SER3CHIP, { " Type", 0,  pc_update_setup,
	  { "UART 8250", "UART 16550" } } },
	{ SETUP_SER3DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Mouse" } } },
	{ SETUP_SERIAL_MOUSE, { "Serial Mouse Protocol", 0, pc_update_setup,
	  { "Microsoft", "Mouse Systems" } } },
	{ SETUP_PAR0, { "Parallel Port at IO:0x3bc (LPT1)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR0TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR0DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_PAR1, { "Parallel Port at IO:0x378 (LPT2)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR1TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR1DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_PAR2, { "Parallel Port at IO:0x278 (LPT3)", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_PAR2TYPE, { " Type", 0,  pc_update_setup,
	  { "Standard (unidirectional)", "Bidirectional" } } },
	{ SETUP_PAR2DEV, { " Device", 0,  pc_update_setup,
	  { "None", "Printer", "Covox Sound System/DAC" } } },
	{ SETUP_GAME0, { "Game Port  at IO:0x200", 0,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_GAME0C0, { " Controller 0", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_GAME0C1, { " Controller 1", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_MPU0, { "MPU 401 Midi Interface at IO:0x330", 0, pc_update_setup, 
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_MPU0D0, { " Device", 0,  pc_update_setup,
	  { "None", "MT32 Synthesizer" } } },
	{ SETUP_FM, { "FM Chip", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_FM_TYPE, { " Type", 0,  pc_update_setup,
	  { "YM3812/OPL II (Adlib)", 
		"2 YM3812/OPL II (Soundblaster Pro Variant 1)",
		"OPL III" } } },
	{ SETUP_FM_PORT, { " Port" , 2, pc_update_setup,
	  { "IO:0x378(Adlib)", "IO:0x378/0x210 (Soundblaster)", "IO:0x378/0x220 (Soundblaster)",
		"IO:0x378/0x230 (Soundblaster)",
		"IO:0x378/0x240 (Soundblaster)" } } },
	{ SETUP_CMS, { "2 SAA1099 (Creative Music System/Gameblaster)", 1, pc_update_setup,
	  { DEF_STR( Off ), "IO:0x220" } } },
	{ SETUP_PCJR_SOUND, { "PC Junior Sound", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_AMSTRAD_JOY, { "Amstrad Joystick", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
	{ SETUP_AMSTRAD_MOUSE, { "Amstrad Mouse", 1,  pc_update_setup,
	  { DEF_STR( Off ), DEF_STR( On ) } } },
};

static MENU_ENTRY_TYPE pc_find_entry(int id, void **result)
{
	int i;
	for (i=0; i<ARRAY_LENGTH(text_entries); i++) {
		if (text_entries[i].id==id) {
			*result=&text_entries[i].text;
			return MENU_ENTRY_TEXT;
		}
	}
	for (i=0; i<ARRAY_LENGTH(selection_entries); i++) {
		if (selection_entries[i].id==id) {
			*result=&selection_entries[i].selection;
			return MENU_ENTRY_SELECTION;
		}
	}
	exit(1);
}
#endif

static void pc_add_entry(int id, int def, int mask)
{
#ifdef MESS_MENU
	void *entry;
	int type=pc_find_entry(id, &entry);
	int i,j;

	switch (type) {
	case MENU_ENTRY_SELECTION:
		((MENU_SELECTION*)entry)->value=def;
		mask=~mask;
		for (i=0, j=0; i<ARRAY_LENGTH(((MENU_SELECTION*)entry)->options);i++) {
			if ((1<<i)&mask) ((MENU_SELECTION*)entry)->options[i]=0;
			if (((MENU_SELECTION*)entry)->options[i]) j++;
		}
		((MENU_SELECTION*)entry)->editable=j>1;
		break;
	}
	menu_add_entry(m_setup, id, 1, type, entry);
#endif
}

void pc_init_setup(PC_SETUP *setup) 
{
	int i;

	for (i=0; setup[i].id!=SETUP_END; i++ ) {
		pc_add_entry(setup[i].id, setup[i].def, setup[i].mask);
	}
}

static DMA8237_CONFIG dma= { DMA8237_PC };

void init_pc_common(void)
{
	pit8253_config(0,&pc_pit8253_config);
	/* FDC hardware */
	pc_fdc_setup();

	/* com hardware */
	uart8250_init(0,com_interface);
	uart8250_reset(0);
	uart8250_init(1,com_interface+1);
	uart8250_reset(1);
	uart8250_init(2,com_interface+2);
	uart8250_reset(2);
	uart8250_init(3,com_interface+3);
	uart8250_reset(3);

	pc_lpt_config(0, lpt_config);
	centronics_config(0, cent_config);
	pc_lpt_set_device(0, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(1, lpt_config+1);
	centronics_config(1, cent_config+1);
	pc_lpt_set_device(1, &CENTRONICS_PRINTER_DEVICE);
	pc_lpt_config(2, lpt_config+2);
	centronics_config(2, cent_config+2);
	pc_lpt_set_device(2, &CENTRONICS_PRINTER_DEVICE);

	/* serial mouse */
	pc_mouse_set_protocol(TYPE_MICROSOFT_MOUSE);
	pc_mouse_set_input_base(12);
	pc_mouse_set_serial_port(0);
	pc_mouse_initialise();

	/* PC-XT keyboard */
	ppi8255_init(&pc_ppi8255_interface);
	at_keyboard_init();
	at_keyboard_set_scan_code_set(1);
	at_keyboard_set_input_port_base(4);

	state_add_function(pc_harddisk_state);
//	state_add_function(nec765_state);
}

void init_pccga(void)
{
	pc_cga_init();
	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
	pc_rtc_init();
}

void init_bondwell(void)
{
	pc_init_setup(pc_setup);
	pc_cga_init();
	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
//	at_keyboard_set_type(AT_KEYBOARD_TYPE_MF2);
}

void init_pcmda(void)
{
	pc_init_setup(pc_setup);
	pc_mda_init();
	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
}

void init_europc(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x2000];
	UINT8 *rom = &memory_region(REGION_CPU1)[0];
	int i;

    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	/*
	  fix century rom bios bug !
	  if year <79 month (and not CENTURY) is loaded with 0x20
	*/
	if (rom[0xff93e]==0xb6){ // mov dh,
		UINT8 a;
		rom[0xff93e]=0xb5; // mov ch,
		for (i=0xf8000, a=0; i<0xfffff; i++ ) a+=rom[i];
		rom[0xfffff]=256-a;
	}

	pc_init_setup(europc);
	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);

	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
	europc_rtc_init();
	pc_aga_set_mode(AGA_COLOR);
//	europc_rtc_set_time();
}

void init_pc200(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x2000];
	int i;

//	pc_init_setup(pc1512);
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb0000, 0xbffff, pc200_videoram_r );
	install_mem_write_handler(0, 0xb0000, 0xbffff, pc200_videoram_w );
	videoram_size=0x10000;
	videoram=memory_region(REGION_CPU1)+0xb0000;

	// 0x3dd, 0x3d8, 0x3d4, 0x3de are also in mda mode present!?
	install_port_read_handler(0, 0x3d0, 0x3df, pc200_cga_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc200_cga_w );

	install_port_read_handler(0, 0x3b0, 0x3bf, pc_MDA_r );
	install_port_write_handler(0, 0x3b0, 0x3bf, pc_MDA_w );

	install_port_read_handler(0, 0x278, 0x27b, pc200_port378_r );

	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	pc_aga_set_mode(AGA_COLOR);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC); //?
}

void init_pc1512(void)
{
	UINT8 *gfx = &memory_region(REGION_GFX1)[0x1000];
	int i;

	pc_init_setup(pc1512);
    /* just a plain bit pattern for graphics data generation */
    for (i = 0; i < 256; i++)
		gfx[i] = i;

	install_mem_read_handler(0, 0xb8000, 0xbbfff, MRA_BANK1 );
	install_mem_write_handler(0, 0xb8000, 0xbbfff, pc1512_videoram_w );

	install_port_read_handler(0, 0x3d0, 0x3df, pc1512_r );
	install_port_write_handler(0, 0x3d0, 0x3df, pc1512_w );

	install_port_read_handler(0, 0x278, 0x27b, pc_parallelport2_r );


	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);
	mc146818_init(MC146818_IGNORE_CENTURY);
}

extern void init_pc1640(void)
{
	pc_init_setup(pc1640);
	vga_init(input_port_0_r);
	install_mem_read_handler(0, 0xa0000, 0xaffff, MRA_BANK1 );
	install_mem_read_handler(0, 0xb0000, 0xb7fff, MRA_BANK2 );
	install_mem_read_handler(0, 0xb8000, 0xbffff, MRA_BANK3 );

	install_mem_write_handler(0, 0xa0000, 0xaffff, MWA_BANK1 );
	install_mem_write_handler(0, 0xb0000, 0xb7fff, MWA_BANK2 );
	install_mem_write_handler(0, 0xb8000, 0xbffff, MWA_BANK3 );

	install_port_read_handler(0, 0x3b0, 0x3bf, vga_port_03b0_r );
	install_port_read_handler(0, 0x3c0, 0x3cf, paradise_ega_03c0_r );
	install_port_read_handler(0, 0x3d0, 0x3df, pc1640_port3d0_r );

	install_port_write_handler(0, 0x3b0, 0x3bf, vga_port_03b0_w );
	install_port_write_handler(0, 0x3c0, 0x3cf, vga_port_03c0_w );
	install_port_write_handler(0, 0x3d0, 0x3df, vga_port_03d0_w );

	install_port_read_handler(0, 0x278, 0x27b, pc1640_port278_r );
	install_port_read_handler(0, 0x4278, 0x427b, pc1640_port4278_r );

	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);

	mc146818_init(MC146818_IGNORE_CENTURY);
}

void init_pc_vga(void)
{
	pc_init_setup(pc_setup);
	init_pc_common();
	dma8237_config(dma8237,&dma);
	dma8237_reset(dma8237);
	at_keyboard_set_type(AT_KEYBOARD_TYPE_PC);

	vga_init(input_port_0_r);
	pc_vga_init();
}

void pc_mda_init_machine(void)
{
//	pc_keyboard_init();
	dma8237_reset(dma8237);
}

void pc_cga_init_machine(void)
{
//	pc_keyboard_init();
	dma8237_reset(dma8237);
}

void pc_aga_init_machine(void)
{
//	pc_keyboard_init();
	dma8237_reset(dma8237);
}

void pc_vga_init_machine(void)
{
	vga_reset();
//	pc_keyboard_init();
	dma8237_reset(dma8237);
}

/***********************************/
/* PC interface to PC COM hardware */
/* Done this way because PCW16 also has PC-com hardware but it
is connected in a different way */

int pc_COM_r(int n, int offset)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_r",("COM%d $%02x: disabled\n", n+1, 0x0ff));
		return 0x0ff;
    }

	return uart8250_r(n, offset);
}

void pc_COM_w(int n, int offset, int data)
{
	/* enabled? */
	if( !(input_port_2_r(0) & (0x80 >> n)) )
	{
		DBG_LOG(1,"COM_w",("COM%d $%02x: disabled\n", n+1, data));
		return;
    }

	uart8250_w(n,offset, data);
}

READ_HANDLER(pc_COM1_r)
{
	return pc_COM_r(0, offset);
}

READ_HANDLER(pc_COM2_r)
{
	return pc_COM_r(1, offset);
}

READ_HANDLER(pc_COM3_r)
{
	return pc_COM_r(2, offset);
}

READ_HANDLER(pc_COM4_r)
{
	return pc_COM_r(3, offset);
}


WRITE_HANDLER(pc_COM1_w)
{
	uart8250_w(0, offset,data);
}

WRITE_HANDLER(pc_COM2_w)
{
	uart8250_w(1, offset,data);
}

WRITE_HANDLER(pc_COM3_w)
{
	uart8250_w(2, offset,data);
}

WRITE_HANDLER(pc_COM4_w)
{
	uart8250_w(3, offset,data);
}

/*
   keyboard seams to permanently sent data clocked by the mainboard
   clock line low for longer means "resync", keyboard sends 0xaa as answer
   will become automatically 0x00 after a while
*/
static struct {
	UINT8 data;
	int on;
} pc_keyb= { 0 };

UINT8 pc_keyb_read(void)
{
	return pc_keyb.data;
}

static void pc_keyb_timer(int param)
{
	pc_keyboard_init();
	pc_keyboard();
}

void pc_keyb_set_clock(int on)
{
	if ((!pc_keyb.on)&&on) {
		timer_set(1/200.0, 0, pc_keyb_timer);
	}
	pc_keyb.on=on;
	pc_keyboard();
}

void pc_keyboard(void)
{
	int data;

	at_keyboard_polling();

//	if( !pic8259_0_irq_pending(1) && ((pc_port[0x61]&0xc0)==0xc0) ) // amstrad problems
	if( !pic8259_0_irq_pending(1) && pc_keyb.on)
	{
		if ( (data=at_keyboard_read())!=-1) {
			pc_keyb.data = data;
			DBG_LOG(1,"KB_scancode",("$%02x\n", pc_keyb.data));
			pic8259_0_issue_irq(1);
		}
	}
}

/**************************************************************************
 *
 *      Interrupt handlers.
 *
 **************************************************************************/
int pc_mda_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_mda_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}

int pc_cga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}

	pc_cga_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}

int pc_aga_frame_interrupt (void)
{
	pc_aga_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}

int pc_vga_frame_interrupt (void)
{
	static int turboswitch=-1;

	if (turboswitch !=(input_port_3_r(0)&2) ) {
		if (input_port_3_r(0)&2)
			timer_set_overclock(0, 1);
		else
			timer_set_overclock(0, 4.77/12);
		turboswitch=input_port_3_r(0)&2;
	}
//	vga_timer();

    if( !onscrd_active() && !setup_active() )
		pc_keyboard();

    return ignore_interrupt ();
}
