/*************************************************************************

    Driver for Atari/Midway Phoenix/Seattle/Flagstaff hardware games

    driver by Aaron Giles

    Games supported:
        * Wayne Gretzky's 3d Hockey    [Phoenix, Atari, ~100MHz, 4MB RAM, 1xTMU]

        * Bio Freaks                   [Seattle, Midway, ???MHz, 8MB RAM, 1xTMU]
        * CarnEvil                     [Seattle, Midway, 150MHz, 8MB RAM, 1xTMU]
        * NFL Blitz                    [Seattle, Midway, 150MHz, 8MB RAM, 1xTMU]
        * NFL Blitz 99                 [Seattle, Midway, 150MHz, 8MB RAM, 1xTMU]
        * NFL Blitz 2000               [Seattle, Midway, 150MHz, 8MB RAM, 1xTMU]
        * Mace: The Dark Age           [Seattle, Atari,  200MHz, 8MB RAM, 1xTMU]

        * California Speed             [Seattle + Widget, Atari, 150MHz, 8MB RAM, 1xTMU]
        * Vapor TRX                    [Seattle + Widget, Atari, 192MHz, 8MB RAM, 1xTMU]
        * Hyperdrive                   [Seattle + Widget, Midway, 200MHz, 8MB RAM, 1xTMU]

        * San Francisco Rush           [Flagstaff, Atari, 192MHz, 2xTMU]
        * San Francisco Rush: The Rock [Flagstaff, Atari, 192MHz, 8MB RAM, 2xTMU]

    Known bugs:
        * Blitz: hangs if POST is enabled due to TMU 1 attempted accesses
        * Carnevil: lets you set the flash brightness; need to emulate that
        * Hyperdrive: long delay after you select a track; may be a timer issue

***************************************************************************

    Phoenix hardware main board:

        * 100MHz R4700 main CPU (50MHz system clock)
        * Galileo GT64010 system controller
        * National Semiconductor PC87415 IDE controller
        * 3dfx FBI with 2MB frame buffer
        * 3dfx TMU with 4MB txture memory
        * Midway I/O ASIC
        * 4MB DRAM for main CPU
        * 512KB boot ROM
        * 16MHz ADSP 2115 audio CPU
        * 4MB DRAM for audio CPU
        * 32KB boot ROM

    Seattle hardware main board:

        * 144MHz/150MHz/192MHz/200MHz R5000 main CPU (system clock 48MHz/50MHz)
        * Galileo GT64010 system controller
        * National Semiconductor PC87415 IDE controller
        * 3dfx FBI with 2MB frame buffer
        * 3dfx TMU with 4MB txture memory
        * Midway I/O ASIC
        * 8MB DRAM for main CPU
        * 512KB boot ROM
        * 16MHz ADSP 2115 audio CPU
        * 4MB DRAM for audio CPU
        * 32KB boot ROM

    Flagstaff hardware main board:

        * 200MHz R5000 main CPU (system clock 50MHz)
        * Galileo GT64010 system controller
        * National Semiconductor PC87415 IDE controller
        * SMC91C94 ethernet controller
        * ADC0848 8 x A-to-D converters
        * 3dfx FBI with 2MB frame buffer
        * 2 x 3dfx TMU with 4MB txture memory
        * Midway I/O ASIC
        * 8MB DRAM for main CPU
        * 512KB boot ROM
        * 33MHz TMS32C031 audio CPU
        * 8MB ROM space for audio CPU
        * 512KB boot ROM

    Widget board:

        * SMC91C94 ethernet controller
        * ADC0848 8 x A-to-D converters

***************************************************************************

    Interrupt summary:

                        __________
    UART clear-to-send |          |                     __________
    -------(0x2000)--->|          |   Ethernet/Widget  |          |
                       |          |   ----(IRQ3/4/5)-->|          |
    UART data ready    |          |                    |          |
    -------(0x1000)--->|          |                    |          |
                       |          |   VSYNC            |          |
    Main-to-sound empty|  IOASIC  |   ----(IRQ3/4/5)-->|          |
    -------(0x0080)--->|          |                    |          |
                       |          |                    |          |
    Sound-to-main full |          |   IDE Controller   |   CPU    |
    -------(0x0040)--->|          |   -------(IRQ2)--->|          |
                       |          |                    |          |
    Sound FIFO empty   |          |                    |          |
    -------(0x0008)--->|          |   IOASIC Summary   |          |
                       |__________|----------(IRQ1)--->|          |
                                                       |          |
                        __________                     |          |
    Timer 3            |          |   Galileo Summary  |          |
    -------(0x0800)--->|          |----------(IRQ0)--->|          |
                       |          |                    |__________|
    Timer 2            |          |
    -------(0x0400)--->|          |
                       |          |
    Timer 1            |          |
    -------(0x0200)--->|          |
                       |          |
    Timer 0            |          |
    -------(0x0100)--->|          |
                       | Galileo  |
    DMA channel 3      |          |
    -------(0x0080)--->|          |
                       |          |
    DMA channel 2      |          |
    -------(0x0040)--->|          |
                       |          |
    DMA channel 1      |          |
    -------(0x0020)--->|          |
                       |          |
    DMA channel 0      |          |
    -------(0x0010)--->|          |
                       |__________|

**************************************************************************/

#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "cpu/mips/mips3.h"
#include "sndhrdw/dcs.h"
#include "sndhrdw/cage.h"
#include "machine/idectrl.h"
#include "machine/midwayic.h"
#include "machine/smc91c9x.h"
#include "vidhrdw/voodoo.h"



/*************************************
 *
 *  Debugging constants
 *
 *************************************/

#define LOG_GALILEO			(0)
#define LOG_TIMERS			(0)
#define LOG_DMA				(0)
#define LOG_PCI				(0)
#define LOG_WIDGET			(0)



/*************************************
 *
 *  Core constants
 *
 *************************************/

#define SYSTEM_CLOCK			50000000
#define TIMER_CLOCK				(TIME_IN_HZ(SYSTEM_CLOCK))

/* various board configurations */
#define PHOENIX_CONFIG			(0)
#define SEATTLE_CONFIG			(1)
#define SEATTLE_WIDGET_CONFIG	(2)
#define FLAGSTAFF_CONFIG		(3)

/* static interrupts */
#define GALILEO_IRQ_NUM			(0)
#define IOASIC_IRQ_NUM			(1)
#define IDE_IRQ_NUM				(2)

/* configurable interrupts */
#define ETHERNET_IRQ_SHIFT		(1)
#define WIDGET_IRQ_SHIFT		(1)
#define VBLANK_IRQ_SHIFT		(7)



/*************************************
 *
 *  Galileo constants
 *
 *************************************/

#define DMA_SECS_PER_BYTE	(TIME_IN_HZ(SYSTEM_CLOCK))

/* Galileo registers - 0x000-0x3ff */
#define GREG_CPU_CONFIG		(0x000/4)
#define GREG_RAS_1_0_LO		(0x008/4)
#define GREG_RAS_1_0_HI		(0x010/4)
#define GREG_RAS_3_2_LO		(0x018/4)
#define GREG_RAS_3_2_HI		(0x020/4)
#define GREG_CS_2_0_LO		(0x028/4)
#define GREG_CS_2_0_HI		(0x030/4)
#define GREG_CS_3_BOOT_LO	(0x038/4)
#define GREG_CS_3_BOOT_HI	(0x040/4)
#define GREG_PCI_IO_LO		(0x048/4)
#define GREG_PCI_IO_HI		(0x050/4)
#define GREG_PCI_MEM_LO		(0x058/4)
#define GREG_PCI_MEM_HI		(0x060/4)
#define GREG_INTERNAL_SPACE	(0x068/4)
#define GREG_BUSERR_LO		(0x070/4)
#define GREG_BUSERR_HI		(0x078/4)

/* Galileo registers - 0x400-0x7ff */
#define GREG_RAS0_LO		(0x400/4)
#define GREG_RAS0_HI		(0x404/4)
#define GREG_RAS1_LO		(0x408/4)
#define GREG_RAS1_HI		(0x40c/4)
#define GREG_RAS2_LO		(0x410/4)
#define GREG_RAS2_HI		(0x414/4)
#define GREG_RAS3_LO		(0x418/4)
#define GREG_RAS3_HI		(0x41c/4)
#define GREG_CS0_LO			(0x420/4)
#define GREG_CS0_HI			(0x424/4)
#define GREG_CS1_LO			(0x428/4)
#define GREG_CS1_HI			(0x42c/4)
#define GREG_CS2_LO			(0x430/4)
#define GREG_CS2_HI			(0x434/4)
#define GREG_CS3_LO			(0x438/4)
#define GREG_CS3_HI			(0x43c/4)
#define GREG_CSBOOT_LO		(0x440/4)
#define GREG_CSBOOT_HI		(0x444/4)
#define GREG_DRAM_CONFIG	(0x448/4)
#define GREG_DRAM_BANK0		(0x44c/4)
#define GREG_DRAM_BANK1		(0x450/4)
#define GREG_DRAM_BANK2		(0x454/4)
#define GREG_DRAM_BANK3		(0x458/4)
#define GREG_DEVICE_BANK0	(0x45c/4)
#define GREG_DEVICE_BANK1	(0x460/4)
#define GREG_DEVICE_BANK2	(0x464/4)
#define GREG_DEVICE_BANK3	(0x468/4)
#define GREG_DEVICE_BOOT	(0x46c/4)
#define GREG_ADDRESS_ERROR	(0x470/4)

/* Galileo registers - 0x800-0xbff */
#define GREG_DMA0_COUNT		(0x800/4)
#define GREG_DMA1_COUNT		(0x804/4)
#define GREG_DMA2_COUNT		(0x808/4)
#define GREG_DMA3_COUNT		(0x80c/4)
#define GREG_DMA0_SOURCE	(0x810/4)
#define GREG_DMA1_SOURCE	(0x814/4)
#define GREG_DMA2_SOURCE	(0x818/4)
#define GREG_DMA3_SOURCE	(0x81c/4)
#define GREG_DMA0_DEST		(0x820/4)
#define GREG_DMA1_DEST		(0x824/4)
#define GREG_DMA2_DEST		(0x828/4)
#define GREG_DMA3_DEST		(0x82c/4)
#define GREG_DMA0_NEXT		(0x830/4)
#define GREG_DMA1_NEXT		(0x834/4)
#define GREG_DMA2_NEXT		(0x838/4)
#define GREG_DMA3_NEXT		(0x83c/4)
#define GREG_DMA0_CONTROL	(0x840/4)
#define GREG_DMA1_CONTROL	(0x844/4)
#define GREG_DMA2_CONTROL	(0x848/4)
#define GREG_DMA3_CONTROL	(0x84c/4)
#define GREG_TIMER0_COUNT	(0x850/4)
#define GREG_TIMER1_COUNT	(0x854/4)
#define GREG_TIMER2_COUNT	(0x858/4)
#define GREG_TIMER3_COUNT	(0x85c/4)
#define GREG_DMA_ARBITER	(0x860/4)
#define GREG_TIMER_CONTROL	(0x864/4)

/* Galileo registers - 0xc00-0xfff */
#define GREG_PCI_COMMAND	(0xc00/4)
#define GREG_PCI_TIMEOUT	(0xc04/4)
#define GREG_PCI_RAS_1_0	(0xc08/4)
#define GREG_PCI_RAS_3_2	(0xc0c/4)
#define GREG_PCI_CS_2_0		(0xc10/4)
#define GREG_PCI_CS_3_BOOT	(0xc14/4)
#define GREG_INT_STATE		(0xc18/4)
#define GREG_INT_MASK		(0xc1c/4)
#define GREG_PCI_INT_MASK	(0xc24/4)
#define GREG_CONFIG_ADDRESS	(0xcf8/4)
#define GREG_CONFIG_DATA	(0xcfc/4)

/* Galileo interrupts */
#define GINT_SUMMARY_SHIFT	(0)
#define GINT_MEMOUT_SHIFT	(1)
#define GINT_DMAOUT_SHIFT	(2)
#define GINT_CPUOUT_SHIFT	(3)
#define GINT_DMA0COMP_SHIFT	(4)
#define GINT_DMA1COMP_SHIFT	(5)
#define GINT_DMA2COMP_SHIFT	(6)
#define GINT_DMA3COMP_SHIFT	(7)
#define GINT_T0EXP_SHIFT	(8)
#define GINT_T1EXP_SHIFT	(9)
#define GINT_T2EXP_SHIFT	(10)
#define GINT_T3EXP_SHIFT	(11)
#define GINT_MASRDERR_SHIFT	(12)
#define GINT_SLVWRERR_SHIFT	(13)
#define GINT_MASWRERR_SHIFT	(14)
#define GINT_SLVRDERR_SHIFT	(15)
#define GINT_ADDRERR_SHIFT	(16)
#define GINT_MEMERR_SHIFT	(17)
#define GINT_MASABORT_SHIFT	(18)
#define GINT_TARABORT_SHIFT	(19)
#define GINT_RETRYCTR_SHIFT	(20)



/*************************************
 *
 *  Widget board constants
 *
 *************************************/

/* Widget registers */
#define WREG_ETHER_ADDR		(0x00/4)
#define WREG_INTERRUPT		(0x04/4)
#define WREG_ANALOG			(0x10/4)
#define WREG_ETHER_DATA		(0x14/4)

/* Widget interrupts */
#define WINT_ETHERNET_SHIFT	(2)



/*************************************
 *
 *  Structures
 *
 *************************************/

struct galileo_timer
{
	void *			timer;
	UINT32			count;
	UINT8			active;
};


struct galileo_data
{
	/* raw register data */
	UINT32			reg[0x1000/4];

	/* timer info */
	struct galileo_timer timer[4];

	/* DMA info */
	UINT8			dma_pending_on_vblank[4];

	/* PCI info */
	UINT32			pci_bridge_regs[0x40];
	UINT32			pci_3dfx_regs[0x40];
	UINT32			pci_ide_regs[0x40];
};


struct widget_data
{
	/* ethernet register address */
	UINT8			ethernet_addr;

	/* IRQ information */
	UINT8			irq_num;
	UINT8			irq_mask;
};



/*************************************
 *
 *  Local variables
 *
 *************************************/

static data32_t *rambase;
static data32_t *rombase;

static struct galileo_data galileo;
static struct widget_data widget;

static UINT8 board_config;

static UINT8 ethernet_irq_num;
static UINT8 ethernet_irq_state;

static UINT8 vblank_irq_num;
static UINT8 vblank_latch;
static UINT8 vblank_state;
static data32_t *interrupt_config;
static data32_t *interrupt_enable;

static data32_t *asic_reset;

static data8_t pending_analog_read;
static data8_t status_leds;

static data32_t *generic_speedup;
static data32_t *generic_speedup2;

static data32_t cmos_write_enabled;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void update_vblank_irq(void);
static void galileo_reset(void);
static void galileo_timer_callback(int param);
static void galileo_perform_dma(int which);
static void widget_reset(void);
static void update_widget_irq(void);



/*************************************
 *
 *  Machine init
 *
 *************************************/

static MACHINE_INIT( seattle )
{
	/* set the fastest DRC options, but strict verification */
	cpunum_set_info_int(0, CPUINFO_INT_MIPS3_DRC_OPTIONS, MIPS3DRC_FASTEST_OPTIONS + MIPS3DRC_STRICT_VERIFY);

	/* allocate timers for the galileo */
	galileo.timer[0].timer = timer_alloc(galileo_timer_callback);
	galileo.timer[1].timer = timer_alloc(galileo_timer_callback);
	galileo.timer[2].timer = timer_alloc(galileo_timer_callback);
	galileo.timer[3].timer = timer_alloc(galileo_timer_callback);

	vblank_irq_num = 0;

	/* reset either the DCS2 board or the CAGE board */
	if (mame_find_cpu_index("dcs2") != -1)
	{
		dcs_reset_w(1);
		dcs_reset_w(0);
	}
	else if (mame_find_cpu_index("cage") != -1)
	{
		cage_control_w(0);
		cage_control_w(3);
	}

	/* reset the other devices */
	galileo_reset();
	ide_controller_reset(0);
	voodoo_reset();
	if (board_config == SEATTLE_WIDGET_CONFIG)
		widget_reset();
	if (board_config == FLAGSTAFF_CONFIG)
		smc91c94_reset();
}



/*************************************
 *
 *  IDE interrupts
 *
 *************************************/

static void ide_interrupt(int state)
{
	cpunum_set_input_line(0, IDE_IRQ_NUM, state);
}


static struct ide_interface ide_intf =
{
	ide_interrupt
};



/*************************************
 *
 *  Ethernet interrupts
 *
 *************************************/

static void ethernet_interrupt(int state)
{
	ethernet_irq_state = state;
	if (board_config == FLAGSTAFF_CONFIG)
	{
		UINT8 assert = ethernet_irq_state && (*interrupt_enable & (1 << ETHERNET_IRQ_SHIFT));
		if (ethernet_irq_num != 0)
			cpunum_set_input_line(0, ethernet_irq_num, assert ? ASSERT_LINE : CLEAR_LINE);
	}
	else if (board_config == SEATTLE_WIDGET_CONFIG)
		update_widget_irq();
}


static struct smc91c9x_interface ethernet_intf =
{
	ethernet_interrupt
};



/*************************************
 *
 *  I/O ASIC interrupts
 *
 *************************************/

static void ioasic_irq(int state)
{
	cpunum_set_input_line(0, IOASIC_IRQ_NUM, state);
}



/*************************************
 *
 *  Configurable interrupts
 *
 *************************************/

static READ32_HANDLER( interrupt_state_r )
{
	data32_t result = 0;
	result |= ethernet_irq_state << ETHERNET_IRQ_SHIFT;
	result |= vblank_latch << VBLANK_IRQ_SHIFT;
	return result;
}


static READ32_HANDLER( interrupt_state2_r )
{
	data32_t result = interrupt_state_r(offset, mem_mask);
	result |= vblank_state << 8;
	return result;
}


static WRITE32_HANDLER( interrupt_config_w )
{
	int irq;
	COMBINE_DATA(interrupt_config);

	/* VBLANK: clear anything pending on the old IRQ */
	if (vblank_irq_num != 0)
		cpunum_set_input_line(0, vblank_irq_num, CLEAR_LINE);

	/* VBLANK: compute the new IRQ vector */
	irq = (*interrupt_config >> (2*VBLANK_IRQ_SHIFT)) & 3;
	vblank_irq_num = (irq != 0) ? (2 + irq) : 0;

	/* Widget board case */
	if (board_config == SEATTLE_WIDGET_CONFIG)
	{
		/* Widget: clear anything pending on the old IRQ */
		if (widget.irq_num != 0)
			cpunum_set_input_line(0, widget.irq_num, CLEAR_LINE);

		/* Widget: compute the new IRQ vector */
		irq = (*interrupt_config >> (2*WIDGET_IRQ_SHIFT)) & 3;
		widget.irq_num = (irq != 0) ? (2 + irq) : 0;
	}

	/* Flagstaff board case */
	if (board_config == FLAGSTAFF_CONFIG)
	{
		/* Ethernet: clear anything pending on the old IRQ */
		if (ethernet_irq_num != 0)
			cpunum_set_input_line(0, ethernet_irq_num, CLEAR_LINE);

		/* Ethernet: compute the new IRQ vector */
		irq = (*interrupt_config >> (2*ETHERNET_IRQ_SHIFT)) & 3;
		ethernet_irq_num = (irq != 0) ? (2 + irq) : 0;
	}

	/* update the states */
	update_vblank_irq();
	ethernet_interrupt(ethernet_irq_state);
}


static WRITE32_HANDLER( seattle_interrupt_enable_w )
{
	data32_t old = *interrupt_enable;
	COMBINE_DATA(interrupt_enable);
	if (old != *interrupt_enable)
	{
		if (vblank_latch)
			update_vblank_irq();
		if (ethernet_irq_state)
			ethernet_interrupt(ethernet_irq_state);
	}
}



/*************************************
 *
 *  VBLANK interrupts
 *
 *************************************/

static void update_vblank_irq(void)
{
	int state = CLEAR_LINE;

	/* skip if no interrupt configured */
	if (vblank_irq_num == 0)
		return;

	/* if the VBLANK has been latched, and the interrupt is enabled, assert */
	if (vblank_latch && (*interrupt_enable & (1 << VBLANK_IRQ_SHIFT)))
		state = ASSERT_LINE;
	cpunum_set_input_line(0, vblank_irq_num, state);
}


static WRITE32_HANDLER( vblank_clear_w )
{
	/* clear the latch and update the IRQ */
	vblank_latch = 0;
	update_vblank_irq();
}


static void vblank_assert(int state)
{
	/* cache the raw state */
	vblank_state = state;

	/* latch on the correct polarity transition */
	if ((state && !(*interrupt_enable & 0x100)) || (!state && (*interrupt_enable & 0x100)))
	{
		vblank_latch = 1;
		update_vblank_irq();
	}

	/* if we have stalled DMA, restart */
	if (state)
	{
		if (galileo.dma_pending_on_vblank[0]) { cpuintrf_push_context(0); galileo_perform_dma(0); cpuintrf_pop_context(); }
		if (galileo.dma_pending_on_vblank[1]) { cpuintrf_push_context(0); galileo_perform_dma(1); cpuintrf_pop_context(); }
		if (galileo.dma_pending_on_vblank[2]) { cpuintrf_push_context(0); galileo_perform_dma(2); cpuintrf_pop_context(); }
		if (galileo.dma_pending_on_vblank[3]) { cpuintrf_push_context(0); galileo_perform_dma(3); cpuintrf_pop_context(); }
	}
}



/*************************************
 *
 *  PCI bridge accesses
 *
 *************************************/

static data32_t pci_bridge_r(UINT8 reg, UINT8 type)
{
	data32_t result = galileo.pci_bridge_regs[reg];

	switch (reg)
	{
		case 0x00:		/* ID register: 0x0146 = GT64010, 0x11ab = Galileo */
			result = 0x014611ab;
			break;

		case 0x02:		/* Base Class:Sub Class:Reserved:Revision */
			result = 0x06000003;
			break;
	}

	if (LOG_PCI)
		logerror("%08X:PCI bridge read: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, result);
	return result;
}


static void pci_bridge_w(UINT8 reg, UINT8 type, data32_t data)
{
	galileo.pci_bridge_regs[reg] = data;
	if (LOG_PCI)
		logerror("%08X:PCI bridge write: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, data);
}



/*************************************
 *
 *  PCI 3dfx accesses
 *
 *************************************/

static data32_t pci_3dfx_r(UINT8 reg, UINT8 type)
{
	data32_t result = galileo.pci_3dfx_regs[reg];

	switch (reg)
	{
		case 0x00:		/* ID register: 0x0001 = SST-1, 0x121a = 3dfx */
			result = 0x0001121a;
			break;

		case 0x02:		/* Base Class:Sub Class:Reserved:Revision */
			result = 0x00000001;
			break;
	}

	if (LOG_PCI)
		logerror("%08X:PCI 3dfx read: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, result);
	return result;
}


static void pci_3dfx_w(UINT8 reg, UINT8 type, data32_t data)
{
	galileo.pci_3dfx_regs[reg] = data;

	switch (reg)
	{
		case 0x04:		/* address register */
			galileo.pci_3dfx_regs[reg] &= 0xff000000;
			if (data != 0x08000000)
				logerror("3dfx not mapped where we expect it!\n");
			break;

		case 0x10:		/* initEnable register */
			voodoo_set_init_enable(data);
			break;
	}
	if (LOG_PCI)
		logerror("%08X:PCI 3dfx write: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, data);
}



/*************************************
 *
 *  PCI IDE accesses
 *
 *************************************/

static data32_t pci_ide_r(UINT8 reg, UINT8 type)
{
	data32_t result = galileo.pci_ide_regs[reg];

	switch (reg)
	{
		case 0x00:		/* ID register: 0x0002 = PC87415, 0x100b = National Semiconductor */
			result = 0x0002100b;
			break;

		case 0x02:		/* Base Class:Sub Class:Reserved:Revision */
			result = 0x01010001;
			break;
	}

	if (LOG_PCI)
		logerror("%08X:PCI IDE read: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, result);
	return result;
}


static void pci_ide_w(UINT8 reg, UINT8 type, data32_t data)
{
	galileo.pci_ide_regs[reg] = data;
	if (LOG_PCI)
		logerror("%08X:PCI bridge write: reg %d type %d = %08X\n", activecpu_get_pc(), reg, type, data);
}



/*************************************
 *
 *  Galileo timers & interrupts
 *
 *************************************/

static void update_galileo_irqs(void)
{
	int state = CLEAR_LINE;

	/* if any unmasked interrupts are live, we generate */
	if (galileo.reg[GREG_INT_STATE] & galileo.reg[GREG_INT_MASK])
		state = ASSERT_LINE;
	cpunum_set_input_line(0, GALILEO_IRQ_NUM, state);

	if (LOG_GALILEO)
		logerror("Galileo IRQ %s\n", (state == ASSERT_LINE) ? "asserted" : "cleared");
}


static void galileo_timer_callback(int which)
{
	struct galileo_timer *timer = &galileo.timer[which];

	if (LOG_TIMERS)
		logerror("timer %d fired\n", which);

	/* copy the start value from the registers */
	timer->count = galileo.reg[GREG_TIMER0_COUNT + which];
	if (which != 0)
		timer->count &= 0xffffff;

	/* if we're a timer, adjust the timer to fire again */
	if (galileo.reg[GREG_TIMER_CONTROL] & (2 << (2 * which)))
		timer_adjust(timer->timer, TIMER_CLOCK * timer->count, which, 0);
	else
		timer->active = timer->count = 0;

	/* trigger the interrupt */
	galileo.reg[GREG_INT_STATE] |= 1 << (GINT_T0EXP_SHIFT + which);
	update_galileo_irqs();
}



/*************************************
 *
 *  Galileo DMA handler
 *
 *************************************/

static int galileo_dma_fetch_next(int which)
{
	offs_t address = 0;
	data32_t data;

	/* no-op for unchained mode */
	if (!(galileo.reg[GREG_DMA0_CONTROL + which] & 0x200))
		address = galileo.reg[GREG_DMA0_NEXT + which];

	/* if we hit the end address, signal an interrupt */
	if (address == 0)
	{
		if (galileo.reg[GREG_DMA0_CONTROL + which] & 0x400)
		{
			galileo.reg[GREG_INT_STATE] |= 1 << (GINT_DMA0COMP_SHIFT + which);
			update_galileo_irqs();
		}
		galileo.reg[GREG_DMA0_CONTROL + which] &= ~0x5000;
		return 0;
	}

	/* fetch the byte count */
	data = program_read_dword(address); address += 4;
	galileo.reg[GREG_DMA0_COUNT + which] = data;

	/* fetch the source address */
	data = program_read_dword(address); address += 4;
	galileo.reg[GREG_DMA0_SOURCE + which] = data;

	/* fetch the dest address */
	data = program_read_dword(address); address += 4;
	galileo.reg[GREG_DMA0_DEST + which] = data;

	/* fetch the next record address */
	data = program_read_dword(address); address += 4;
	galileo.reg[GREG_DMA0_NEXT + which] = data;
	return 1;
}


static void galileo_perform_dma(int which)
{
	do
	{
		offs_t srcaddr = galileo.reg[GREG_DMA0_SOURCE + which];
		offs_t dstaddr = galileo.reg[GREG_DMA0_DEST + which];
		data32_t bytesleft = galileo.reg[GREG_DMA0_COUNT + which] & 0xffff;
		int srcinc, dstinc, i;

		galileo.dma_pending_on_vblank[which] = 0;
		galileo.reg[GREG_DMA0_CONTROL + which] |= 0x5000;

		/* determine src/dst inc */
		switch ((galileo.reg[GREG_DMA0_CONTROL + which] >> 2) & 3)
		{
			default:
			case 0:		srcinc = 1;		break;
			case 1:		srcinc = -1;	break;
			case 2:		srcinc = 0;		break;
		}
		switch ((galileo.reg[GREG_DMA0_CONTROL + which] >> 4) & 3)
		{
			default:
			case 0:		dstinc = 1;		break;
			case 1:		dstinc = -1;	break;
			case 2:		dstinc = 0;		break;
		}

		if (LOG_DMA)
			logerror("Performing DMA%d: src=%08X dst=%08X bytes=%04X sinc=%d dinc=%d\n", which, srcaddr, dstaddr, bytesleft, srcinc, dstinc);

		/* special case: transfer ram to voodoo */
		if (bytesleft % 4 == 0 && srcaddr % 4 == 0 && srcaddr < 0x007fffff && dstaddr >= 0x08000000 && dstaddr < 0x09000000)
		{
			data32_t *src = &rambase[srcaddr/4];
			bytesleft /= 4;

			/* if the voodoo is blocked, stall until the next VBLANK */
			if (voodoo_fifo_words_left() < bytesleft)
			{
				if (LOG_DMA)
					logerror("DMA%d: blocked on voodoo\n", which);
				galileo.dma_pending_on_vblank[which] = 1;
				return;
			}

			/* transfer to registers */
			if (dstaddr < 0x8400000)
			{
				dstaddr = (dstaddr & 0x3fffff) / 4;
				for (i = 0; i < bytesleft; i++)
				{
					voodoo_regs_w(dstaddr, *src, 0);
					src += srcinc;
					dstaddr += dstinc;
				}
			}

			/* transfer to framebuf */
			else if (dstaddr < 0x8800000)
			{
				dstaddr = (dstaddr & 0x3fffff) / 4;
				for (i = 0; i < bytesleft; i++)
				{
					voodoo_framebuf_w(dstaddr, *src, 0);
					src += srcinc;
					dstaddr += dstinc;
				}
			}

			/* transfer to textureram */
			else
			{
				dstaddr = (dstaddr & 0x7fffff) / 4;
				for (i = 0; i < bytesleft; i++)
				{
					voodoo_textureram_w(dstaddr, *src, 0);
					src += srcinc;
					dstaddr += dstinc;
				}
			}
		}

		/* standard transfer */
		else
		{
			for (i = 0; i < bytesleft; i++)
			{
				program_write_byte(dstaddr, program_read_byte(srcaddr));
				srcaddr += srcinc;
				dstaddr += dstinc;
			}
		}

		/* interrupt? */
		if (!(galileo.reg[GREG_DMA0_CONTROL + which] & 0x400))
		{
			galileo.reg[GREG_INT_STATE] |= 1 << (GINT_DMA0COMP_SHIFT + which);
			update_galileo_irqs();
		}
	} while (galileo_dma_fetch_next(which));

	galileo.reg[GREG_DMA0_CONTROL + which] &= ~0x5000;
}



/*************************************
 *
 *  Galileo system controller
 *
 *************************************/

static void galileo_reset(void)
{
	memset(&galileo.reg, 0, sizeof(galileo.reg));
}


static READ32_HANDLER( galileo_r )
{
	data32_t result = galileo.reg[offset];

	/* switch off the offset for special cases */
	switch (offset)
	{
		case GREG_TIMER0_COUNT:
		case GREG_TIMER1_COUNT:
		case GREG_TIMER2_COUNT:
		case GREG_TIMER3_COUNT:
		{
			int which = offset % 4;
			struct galileo_timer *timer = &galileo.timer[which];

			result = timer->count;
			if (timer->active)
			{
				UINT32 elapsed = (UINT32)(timer_timeelapsed(timer->timer) / TIMER_CLOCK);
				result = (result > elapsed) ? (result - elapsed) : 0;
			}

			/* eat some time for those which poll this register */
			activecpu_eat_cycles(100);

			if (LOG_TIMERS)
				logerror("%08X:hires_timer_r = %08X\n", activecpu_get_pc(), result);
			break;
		}

		case GREG_PCI_COMMAND:
			// code at 40188 loops until this returns non-zero in bit 0
			result = 0x0001;
			break;

		case GREG_CONFIG_DATA:
		{
			int bus = (galileo.reg[GREG_CONFIG_ADDRESS] >> 16) & 0xff;
			int unit = (galileo.reg[GREG_CONFIG_ADDRESS] >> 11) & 0x1f;
			int func = (galileo.reg[GREG_CONFIG_ADDRESS] >> 8) & 7;
			int reg = (galileo.reg[GREG_CONFIG_ADDRESS] >> 2) & 0x3f;
			int type = galileo.reg[GREG_CONFIG_ADDRESS] & 3;

			/* unit 0 is the PCI bridge */
			if (unit == 0 && func == 0)
				result = pci_bridge_r(reg, type);

			/* unit 8 is the 3dfx card */
			else if (unit == 8 && func == 0)
				result = pci_3dfx_r(reg, type);

			/* unit 9 is the IDE controller */
			else if (unit == 9 && func == 0)
				result = pci_ide_r(reg, type);

			/* anything else, just log */
			else
			{
				result = ~0;
				logerror("%08X:PCIBus read: bus %d unit %d func %d reg %d type %d = %08X\n", activecpu_get_pc(), bus, unit, func, reg, type, result);
			}
			break;
		}

		case GREG_CONFIG_ADDRESS:
		case GREG_INT_STATE:
		case GREG_INT_MASK:
		case GREG_TIMER_CONTROL:
			if (LOG_GALILEO)
				logerror("%08X:Galileo read from offset %03X = %08X\n", activecpu_get_pc(), offset*4, result);
			break;

		default:
			logerror("%08X:Galileo read from offset %03X = %08X\n", activecpu_get_pc(), offset*4, result);
			break;
	}

	return result;
}


static WRITE32_HANDLER( galileo_w )
{
	UINT32 oldata = galileo.reg[offset];
	COMBINE_DATA(&galileo.reg[offset]);

	/* switch off the offset for special cases */
	switch (offset)
	{
		case GREG_DMA0_CONTROL:
		case GREG_DMA1_CONTROL:
		case GREG_DMA2_CONTROL:
		case GREG_DMA3_CONTROL:
		{
			int which = offset % 4;

			if (LOG_DMA)
				logerror("%08X:Galileo write to offset %03X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, ~mem_mask);

			/* keep the read only activity bit */
			galileo.reg[offset] &= ~0x4000;
			galileo.reg[offset] |= (oldata & 0x4000);

			/* fetch next record */
			if (data & 0x2000)
				galileo_dma_fetch_next(which);
			galileo.reg[offset] &= ~0x2000;

			/* if enabling, start the DMA */
			if (!(oldata & 0x1000) && (data & 0x1000))
				galileo_perform_dma(which);
			break;
		}

		case GREG_TIMER0_COUNT:
		case GREG_TIMER1_COUNT:
		case GREG_TIMER2_COUNT:
		case GREG_TIMER3_COUNT:
		{
			int which = offset % 4;
			struct galileo_timer *timer = &galileo.timer[which];

			if (which != 0)
				data &= 0xffffff;
			if (!timer->active)
				timer->count = data;
			if (LOG_TIMERS)
				logerror("%08X:timer/counter %d count = %08X [start=%08X]\n", activecpu_get_pc(), offset % 4, data, timer->count);
			break;
		}

		case GREG_TIMER_CONTROL:
		{
			int which, mask;

			if (LOG_TIMERS)
				logerror("%08X:timer/counter control = %08X\n", activecpu_get_pc(), data);
			for (which = 0, mask = 0x01; which < 4; which++, mask <<= 2)
			{
				struct galileo_timer *timer = &galileo.timer[which];
				if (!timer->active && (data & mask))
				{
					timer->active = 1;
					if (timer->count == 0)
					{
						timer->count = galileo.reg[GREG_TIMER0_COUNT + which];
						if (which != 0)
							timer->count &= 0xffffff;
					}
					timer_adjust(timer->timer, TIMER_CLOCK * timer->count, which, 0);
					if (LOG_TIMERS)
						logerror("Adjusted timer to fire in %f secs\n", TIMER_CLOCK * timer->count);
				}
				else if (timer->active && !(data & mask))
				{
					UINT32 elapsed = (UINT32)(timer_timeelapsed(timer->timer) / TIMER_CLOCK);
					timer->active = 0;
					timer->count = (timer->count > elapsed) ? (timer->count - elapsed) : 0;
					timer_adjust(timer->timer, TIME_NEVER, which, 0);
					if (LOG_TIMERS)
						logerror("Disabled timer\n");
				}
			}
			break;
		}

		case GREG_INT_STATE:
			if (LOG_GALILEO)
				logerror("%08X:Galileo write to IRQ clear = %08X & %08X\n", offset*4, data, ~mem_mask);
			galileo.reg[offset] = oldata & data;
			update_galileo_irqs();
			break;

		case GREG_CONFIG_DATA:
		{
			int bus = (galileo.reg[GREG_CONFIG_ADDRESS] >> 16) & 0xff;
			int unit = (galileo.reg[GREG_CONFIG_ADDRESS] >> 11) & 0x1f;
			int func = (galileo.reg[GREG_CONFIG_ADDRESS] >> 8) & 7;
			int reg = (galileo.reg[GREG_CONFIG_ADDRESS] >> 2) & 0x3f;
			int type = galileo.reg[GREG_CONFIG_ADDRESS] & 3;

			/* unit 0 is the PCI bridge */
			if (unit == 0 && func == 0)
				pci_bridge_w(reg, type, data);

			/* unit 8 is the 3dfx card */
			else if (unit == 8 && func == 0)
				pci_3dfx_w(reg, type, data);

			/* unit 9 is the IDE controller */
			else if (unit == 9 && func == 0)
				pci_ide_w(reg, type, data);

			/* anything else, just log */
			else
				logerror("%08X:PCIBus write: bus %d unit %d func %d reg %d type %d = %08X\n", activecpu_get_pc(), bus, unit, func, reg, type, data);
			break;
		}

		case GREG_DMA0_COUNT:	case GREG_DMA1_COUNT:	case GREG_DMA2_COUNT:	case GREG_DMA3_COUNT:
		case GREG_DMA0_SOURCE:	case GREG_DMA1_SOURCE:	case GREG_DMA2_SOURCE:	case GREG_DMA3_SOURCE:
		case GREG_DMA0_DEST:	case GREG_DMA1_DEST:	case GREG_DMA2_DEST:	case GREG_DMA3_DEST:
		case GREG_DMA0_NEXT:	case GREG_DMA1_NEXT:	case GREG_DMA2_NEXT:	case GREG_DMA3_NEXT:
		case GREG_CONFIG_ADDRESS:
		case GREG_INT_MASK:
			if (LOG_GALILEO)
				logerror("%08X:Galileo write to offset %03X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, ~mem_mask);
			break;

		default:
			logerror("%08X:Galileo write to offset %03X = %08X & %08X\n", activecpu_get_pc(), offset*4, data, ~mem_mask);
			break;
	}
}



/*************************************
 *
 *  Analog input handling (ADC0848)
 *
 *************************************/

static READ32_HANDLER( analog_port_r )
{
	return pending_analog_read;
}


static WRITE32_HANDLER( analog_port_w )
{
	if (data < 8 || data > 15)
		logerror("%08X:Unexpected analog port select = %08X\n", activecpu_get_pc(), data);
	pending_analog_read = readinputport(4 + (data & 7));
}



/*************************************
 *
 *  CarnEvil gun handling
 *
 *************************************/

INLINE void get_crosshair_xy(int player, int *x, int *y)
{
	*x = (((readinputport(4 + player * 2) & 0xff) << 4) * Machine->visible_area.max_x) / 0xfff;
	*y = (((readinputport(5 + player * 2) & 0xff) << 2) * Machine->visible_area.max_y) / 0x3ff;
}


static VIDEO_UPDATE( carnevil )
{
	int beamx, beamy;

	/* first do common video update */
	video_update_voodoo(bitmap, cliprect);

	/* now draw the crosshairs */
	get_crosshair_xy(0, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect, 0);
	get_crosshair_xy(1, &beamx, &beamy);
	draw_crosshair(bitmap, beamx, beamy, cliprect, 1);
}


static READ32_HANDLER( carnevil_gun_r )
{
	data32_t result = 0;

	switch (offset)
	{
		case 0:		/* low 8 bits of X */
			result = (readinputport(4) << 4) & 0xff;
			break;

		case 1:		/* upper 4 bits of X */
			result = (readinputport(4) >> 4) & 0x0f;
			result |= (readinputport(8) & 0x03) << 4;
			result |= 0x40;
			break;

		case 2:		/* low 8 bits of Y */
			result = (readinputport(5) << 2) & 0xff;
			break;

		case 3:		/* upper 4 bits of Y */
			result = (readinputport(5) >> 6) & 0x03;
			break;

		case 4:		/* low 8 bits of X */
			result = (readinputport(6) << 4) & 0xff;
			break;

		case 5:		/* upper 4 bits of X */
			result = (readinputport(6) >> 4) & 0x0f;
			result |= (readinputport(8) & 0x30);
			result |= 0x40;
			break;

		case 6:		/* low 8 bits of Y */
			result = (readinputport(7) << 2) & 0xff;
			break;

		case 7:		/* upper 4 bits of Y */
			result = (readinputport(7) >> 6) & 0x03;
			break;
	}
	return result;
}


static WRITE32_HANDLER( carnevil_gun_w )
{
	logerror("carnevil_gun_w(%d) = %02X\n", offset, data);
}



/*************************************
 *
 *  Ethernet access
 *
 *************************************/

static READ32_HANDLER( ethernet_r )
{
	if (!(offset & 8))
		return smc91c94_r(offset & 7, mem_mask | 0x0000);
	else
		return smc91c94_r(offset & 7, mem_mask | 0xff00);
}


static WRITE32_HANDLER( ethernet_w )
{
	if (!(offset & 8))
		smc91c94_w(offset & 7, data & 0xffff, mem_mask | 0x0000);
	else
		smc91c94_w(offset & 7, data & 0x00ff, mem_mask | 0xff00);
}



/*************************************
 *
 *  Widget board access
 *
 *************************************/

static void widget_reset(void)
{
	UINT8 saved_irq = widget.irq_num;
	memset(&widget, 0, sizeof(widget));
	widget.irq_num = saved_irq;
	smc91c94_reset();
}


static void update_widget_irq(void)
{
	UINT8 state = ethernet_irq_state << WINT_ETHERNET_SHIFT;
	UINT8 mask = widget.irq_mask;
	UINT8 assert = ((mask & state) != 0) && (*interrupt_enable & (1 << WIDGET_IRQ_SHIFT));

	/* update the IRQ state */
	if (widget.irq_num != 0)
		cpunum_set_input_line(0, widget.irq_num, assert ? ASSERT_LINE : CLEAR_LINE);
}


static READ32_HANDLER( widget_r )
{
	data32_t result = ~0;

	switch (offset)
	{
		case WREG_ETHER_ADDR:
			result = widget.ethernet_addr;
			break;

		case WREG_INTERRUPT:
			result = ethernet_irq_state << WINT_ETHERNET_SHIFT;
			result = ~result;
			break;

		case WREG_ANALOG:
			result = analog_port_r(0, mem_mask);
			break;

		case WREG_ETHER_DATA:
			result = smc91c94_r(widget.ethernet_addr & 7, mem_mask & 0xffff);
			break;
	}

	if (LOG_WIDGET)
		logerror("Widget read (%02X) = %08X & %08X\n", offset*4, result, ~mem_mask);
	return result;
}


static WRITE32_HANDLER( widget_w )
{
	if (LOG_WIDGET)
		logerror("Widget write (%02X) = %08X & %08X\n", offset*4, data, ~mem_mask);

	switch (offset)
	{
		case WREG_ETHER_ADDR:
			widget.ethernet_addr = data;
			break;

		case WREG_INTERRUPT:
			widget.irq_mask = data;
			update_widget_irq();
			break;

		case WREG_ANALOG:
			analog_port_w(0, data, mem_mask);
			break;

		case WREG_ETHER_DATA:
			smc91c94_w(widget.ethernet_addr & 7, data & 0xffff, mem_mask & 0xffff);
			break;
	}
}



/*************************************
 *
 *  CMOS access
 *
 *************************************/

static WRITE32_HANDLER( cmos_w )
{
	if (cmos_write_enabled)
		COMBINE_DATA(generic_nvram32 + offset);
	cmos_write_enabled = 0;
}


static READ32_HANDLER( cmos_r )
{
	return generic_nvram32[offset];
}


static WRITE32_HANDLER( cmos_protect_w )
{
	cmos_write_enabled = 1;
}


static READ32_HANDLER( cmos_protect_r )
{
	return cmos_write_enabled;
}



/*************************************
 *
 *  Misc accesses
 *
 *************************************/

static WRITE32_HANDLER( seattle_watchdog_w )
{
	activecpu_eat_cycles(100);
}


static WRITE32_HANDLER( asic_reset_w )
{
	COMBINE_DATA(asic_reset);
	if (!(*asic_reset & 0x0002))
		midway_ioasic_reset();
}


static WRITE32_HANDLER( asic_fifo_w )
{
	midway_ioasic_fifo_w(data);
}


static READ32_HANDLER( status_leds_r )
{
	return status_leds | 0xffffff00;
}


static WRITE32_HANDLER( status_leds_w )
{
	if (!(mem_mask & 0x000000ff))
		status_leds = data;
}



/*************************************
 *
 *  Speedups
 *
 *************************************/

static READ32_HANDLER( generic_speedup_r )
{
	activecpu_eat_cycles(100);
	return *generic_speedup;
}


static WRITE32_HANDLER( generic_speedup_w )
{
	activecpu_eat_cycles(100);
	COMBINE_DATA(generic_speedup);
}


static READ32_HANDLER( generic_speedup2_r )
{
	activecpu_eat_cycles(100);
	return *generic_speedup2;
}



/*************************************
 *
 *  Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( seattle_map, ADDRESS_SPACE_PROGRAM, 32 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x00000000, 0x007fffff) AM_MIRROR(0xa0000000) AM_RAM AM_BASE(&rambase)	// wg3dh only has 4MB; sfrush, blitz99 8MB
	AM_RANGE(0x88000000, 0x883fffff) AM_MIRROR(0x20000000) AM_READWRITE(voodoo_regs_r, voodoo_regs_w)
	AM_RANGE(0x88400000, 0x887fffff) AM_MIRROR(0x20000000) AM_READWRITE(voodoo_framebuf_r, voodoo_framebuf_w)
	AM_RANGE(0x88800000, 0x88ffffff) AM_MIRROR(0x20000000) AM_WRITE(voodoo_textureram_w)
	AM_RANGE(0xaa000000, 0xaa0003ff) AM_READWRITE(ide_controller32_0_r, ide_controller32_0_w)
	AM_RANGE(0xaa00040c, 0xaa00040f) AM_NOP						// IDE-related, but annoying
	AM_RANGE(0xaa000f00, 0xaa000f07) AM_READWRITE(ide_bus_master32_0_r, ide_bus_master32_0_w)
	AM_RANGE(0xac000000, 0xac000fff) AM_READWRITE(galileo_r, galileo_w)
	AM_RANGE(0xb3000000, 0xb3000003) AM_WRITE(asic_fifo_w)
	AM_RANGE(0xb6000000, 0xb600003f) AM_READWRITE(midway_ioasic_r, midway_ioasic_w)
	AM_RANGE(0xb6100000, 0xb611ffff) AM_READWRITE(cmos_r, cmos_w) AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xb7000000, 0xb7000003) AM_READWRITE(cmos_protect_r, cmos_protect_w)
	AM_RANGE(0xb7100000, 0xb7100003) AM_WRITE(seattle_watchdog_w)
	AM_RANGE(0xb7300000, 0xb7300003) AM_READWRITE(MRA32_RAM, seattle_interrupt_enable_w) AM_BASE(&interrupt_enable)
	AM_RANGE(0xb7400000, 0xb7400003) AM_READWRITE(MRA32_RAM, interrupt_config_w) AM_BASE(&interrupt_config)
	AM_RANGE(0xb7500000, 0xb7500003) AM_READ(interrupt_state_r)
	AM_RANGE(0xb7600000, 0xb7600003) AM_READ(interrupt_state2_r)
	AM_RANGE(0xb7700000, 0xb7700003) AM_WRITE(vblank_clear_w)
	AM_RANGE(0xb7800000, 0xb7800003) AM_NOP
	AM_RANGE(0xb7900000, 0xb7900003) AM_READWRITE(status_leds_r, status_leds_w)
	AM_RANGE(0xb7f00000, 0xb7f00003) AM_READWRITE(MRA32_RAM, asic_reset_w) AM_BASE(&asic_reset)
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_MIRROR(0x20000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_BASE(&rombase)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( wg3dh )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0002, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown0040" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* 3d cam */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( mace )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_SERVICE( 0x0040, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x0000, "Resolution" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Low ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Medium ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* 3d cam */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( sfrush )
	PORT_START	    /* DIPs */
	PORT_BIT(0x0001, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_DIPNAME( 0x0002, 0x0002, "Boot ROM Test" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown0040" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )	/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )	/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )	/* abort */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 	/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)	/* reverse */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )	/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )	/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* view 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	/* view 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)	/* view 3 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)	/* music */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)	/* track 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)	/* track 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)	/* track 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(3)	/* track 4 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)	/* 4th gear */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(3)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(5)
INPUT_PORTS_END


INPUT_PORTS_START( sfrushrk )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Calibrate at startup" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0001, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Boot ROM Test" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )	/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )	/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )	/* abort */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 	/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)	/* reverse */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )	/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )	/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* view 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	/* view 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)	/* view 3 */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)	/* music */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)	/* track 1 */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)	/* track 2 */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)	/* track 3 */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(3)	/* track 4 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* 1st gear */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	/* 2nd gear */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* 3rd gear */
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)	/* 4th gear */
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(3)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(5)
INPUT_PORTS_END


INPUT_PORTS_START( calspeed )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Boot ROM Test" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )	/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )	/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )	/* start */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 	/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )	/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )	/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )	/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )	/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)	/* radio */
	PORT_BIT( 0x000c, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* road cam */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	/* tailgate cam */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)	/* sky cam */
	PORT_BIT( 0x0f80, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* 1st gear */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	/* 2nd gear */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* 3rd gear */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)	/* 4th gear */

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(5)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )
INPUT_PORTS_END


INPUT_PORTS_START( vaportrx )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Unknown0001" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Unknown0002" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Boot ROM Test" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )					/* coin 1 */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )					/* coin 2 */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	/* left trigger */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) 					/* tilt */
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_SERVICE2 )					/* test */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )					/* service coin */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )					/* coin 3 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )					/* coin 4 */
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x000f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	/* right trigger */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* left thumb */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* right thumb */
	PORT_BIT( 0x0180, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	/* left view */
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	/* right view */
	PORT_BIT( 0xf000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )
INPUT_PORTS_END


INPUT_PORTS_START( biofreak )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0001, "Hilink download??" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0002, 0x0002, "Boot ROM Test" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0004, 0x0004, "Unknown0004" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0008, 0x0008, "Unknown0008" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0010, 0x0010, "Unknown0010" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0020, 0x0020, "Unknown0020" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0040, 0x0040, "Unknown0040" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0080, 0x0080, "Unknown0080" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0100, 0x0100, "Unknown0100" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0200, 0x0200, "Unknown0200" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0400, 0x0400, "Unknown0400" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x0800, 0x0800, "Unknown0800" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x1000, 0x1000, "Unknown1000" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x2000, 0x2000, "Unknown2000" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x4000, 0x4000, "Unknown4000" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))
	PORT_DIPNAME( 0x8000, 0x8000, "Unknown8000" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	/* LP = P1 left punch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)	/* F  = P1 ??? */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* RP = P1 right punch */
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* LP = P1 left punch */
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)	/* F  = P1 ??? */
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	/* RP = P1 right punch */
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)	/* LK = P1 left kick */
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(1)	/* RK = P1 right kick */
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(1)	/* T  = P1 ??? */
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)	/* LK = P2 left kick */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_PLAYER(2)	/* RK = P2 right kick */
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_PLAYER(2)	/* T  = P2 ??? */
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( blitz )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x000e, "Mode 1" )
	PORT_DIPSETTING(      0x0008, "Mode 2" )
	PORT_DIPSETTING(      0x0009, "Mode 3" )
	PORT_DIPSETTING(      0x0002, "Mode 4" )
	PORT_DIPSETTING(      0x000c, "Mode ECA" )
//  PORT_DIPSETTING(      0x0004, "Not Used 1" )        /* Marked as Unused in the manual */
//  PORT_DIPSETTING(      0x0008, "Not Used 2" )        /* Marked as Unused in the manual */
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0030, 0x0030, "Curency Type" )
	PORT_DIPSETTING(      0x0030, DEF_STR( USA ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( French ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( German ) )
//  PORT_DIPSETTING(      0x0000, "Not Used" )      /* Marked as Unused in the manual */
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))	/* Marked as Unused in the manual */
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, "One Time" )
	PORT_DIPSETTING(      0x0000, "Continuous" )
	PORT_DIPNAME( 0x0100, 0x0100, "Joysticks" )
	PORT_DIPSETTING(      0x0100, "8-Way" )
	PORT_DIPSETTING(      0x0000, "49-Way" )
	PORT_DIPNAME( 0x0600, 0x0200, "Graphics Mode" )
	PORT_DIPSETTING(      0x0200, "512x385 @ 25KHz" )
	PORT_DIPSETTING(      0x0400, "512x256 @ 15KHz" )
//  PORT_DIPSETTING(      0x0600, "0" )         /* Marked as Unused in the manual */
//  PORT_DIPSETTING(      0x0000, "3" )         /* Marked as Unused in the manual */
	PORT_DIPNAME( 0x1800, 0x1800, "Graphics Speed" )
	PORT_DIPSETTING(      0x0000, "45 MHz" )
	PORT_DIPSETTING(      0x0800, "47 MHz" )
	PORT_DIPSETTING(      0x1000, "49 MHz" )
	PORT_DIPSETTING(      0x1800, "51 MHz" )
	PORT_DIPNAME( 0x2000, 0x0000, "Bill Validator" )
	PORT_DIPSETTING(      0x2000, DEF_STR( None ) )
	PORT_DIPSETTING(      0x0000, "One" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power On Self Test" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( blitz99 )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA 1" )
	PORT_DIPSETTING(      0x003c, "USA 2" )
	PORT_DIPSETTING(      0x003a, "USA 3" )
	PORT_DIPSETTING(      0x0038, "USA 4" )
	PORT_DIPSETTING(      0x0036, "USA 5" )
	PORT_DIPSETTING(      0x0034, "USA 6" )
	PORT_DIPSETTING(      0x0032, "USA 7" )
	PORT_DIPSETTING(      0x0030, "USA ECA" )
	PORT_DIPSETTING(      0x002e, "France 1" )
	PORT_DIPSETTING(      0x002c, "France 2" )
	PORT_DIPSETTING(      0x002a, "France 3" )
	PORT_DIPSETTING(      0x0028, "France 4" )
	PORT_DIPSETTING(      0x0026, "France 5" )
	PORT_DIPSETTING(      0x0024, "France 6" )
	PORT_DIPSETTING(      0x0022, "France 7" )
	PORT_DIPSETTING(      0x0020, "France ECA" )
	PORT_DIPSETTING(      0x001e, "German 1" )
	PORT_DIPSETTING(      0x001c, "German 2" )
	PORT_DIPSETTING(      0x001a, "German 3" )
	PORT_DIPSETTING(      0x0018, "German 4" )
	PORT_DIPSETTING(      0x0016, "German 5" )
//  PORT_DIPSETTING(      0x0014, "German 5" )
//  PORT_DIPSETTING(      0x0012, "German 5" )
	PORT_DIPSETTING(      0x0010, "German ECA" )
	PORT_DIPSETTING(      0x000e, "U.K. 1 ECA" )
	PORT_DIPSETTING(      0x000c, "U.K. 2 ECA" )
	PORT_DIPSETTING(      0x000a, "U.K. 3 ECA" )
	PORT_DIPSETTING(      0x0008, "U.K. 4" )
	PORT_DIPSETTING(      0x0006, "U.K. 5" )
	PORT_DIPSETTING(      0x0004, "U.K. 6 ECA" )
	PORT_DIPSETTING(      0x0002, "U.K. 7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0100, 0x0100, "Joysticks" )
	PORT_DIPSETTING(      0x0100, "8-Way" )
	PORT_DIPSETTING(      0x0000, "49-Way" )
	PORT_DIPNAME( 0x0600, 0x0200, "Graphics Mode" )
	PORT_DIPSETTING(      0x0200, "512x385 @ 25KHz" )
	PORT_DIPSETTING(      0x0400, "512x256 @ 15KHz" )
//  PORT_DIPSETTING(      0x0600, "0" )         /* Marked as Unused in the manual */
//  PORT_DIPSETTING(      0x0000, "3" )         /* Marked as Unused in the manual */
	PORT_DIPNAME( 0x1800, 0x1800, "Graphics Speed" )
	PORT_DIPSETTING(      0x0000, "45 MHz" )
	PORT_DIPSETTING(      0x0800, "47 MHz" )
	PORT_DIPSETTING(      0x1000, "49 MHz" )
	PORT_DIPSETTING(      0x1800, "51 MHz" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Players ) )
	PORT_DIPSETTING(      0x2000, "2" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power On Self Test" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(3)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(4)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END


INPUT_PORTS_START( carnevil )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x003e, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA 1" )
	PORT_DIPSETTING(      0x003c, "USA 2" )
	PORT_DIPSETTING(      0x003a, "USA 3" )
	PORT_DIPSETTING(      0x0038, "USA 4" )
	PORT_DIPSETTING(      0x0036, "USA 5" )
	PORT_DIPSETTING(      0x0034, "USA 6" )
	PORT_DIPSETTING(      0x0032, "USA 7" )
	PORT_DIPSETTING(      0x0030, "USA ECA" )
	PORT_DIPSETTING(      0x002e, "France 1" )
	PORT_DIPSETTING(      0x002c, "France 2" )
	PORT_DIPSETTING(      0x002a, "France 3" )
	PORT_DIPSETTING(      0x0028, "France 4" )
	PORT_DIPSETTING(      0x0026, "France 5" )
	PORT_DIPSETTING(      0x0024, "France 6" )
	PORT_DIPSETTING(      0x0022, "France 7" )
	PORT_DIPSETTING(      0x0020, "France ECA" )
	PORT_DIPSETTING(      0x001e, "German 1" )
	PORT_DIPSETTING(      0x001c, "German 2" )
	PORT_DIPSETTING(      0x001a, "German 3" )
	PORT_DIPSETTING(      0x0018, "German 4" )
	PORT_DIPSETTING(      0x0016, "German 5" )
//  PORT_DIPSETTING(      0x0014, "German 5" )
//  PORT_DIPSETTING(      0x0012, "German 5" )
	PORT_DIPSETTING(      0x0010, "German ECA" )
	PORT_DIPSETTING(      0x000e, "U.K. 1" )
	PORT_DIPSETTING(      0x000c, "U.K. 2" )
	PORT_DIPSETTING(      0x000a, "U.K. 3" )
	PORT_DIPSETTING(      0x0008, "U.K. 4" )
	PORT_DIPSETTING(      0x0006, "U.K. 5" )
	PORT_DIPSETTING(      0x0004, "U.K. 6" )
	PORT_DIPSETTING(      0x0002, "U.K. 7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0100, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0600, 0x0400, "Resolution" )
//  PORT_DIPSETTING(      0x0600, "0" )
//  PORT_DIPSETTING(      0x0200, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Low ) )
//  PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1800, 0x1800, "Graphics Speed" )
	PORT_DIPSETTING(      0x0000, "45 MHz" )
	PORT_DIPSETTING(      0x0800, "47 MHz" )
	PORT_DIPSETTING(      0x1000, "49 MHz" )
	PORT_DIPSETTING(      0x1800, "51 MHz" )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x2000, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power On Self Test" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0780, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10)

	PORT_START				/* fake analog X */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_X ) PORT_MINMAX(0,255) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake analog Y */
	PORT_BIT( 0xff, 0x80, IPT_LIGHTGUN_Y ) PORT_MINMAX(0,255) PORT_SENSITIVITY(70) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START				/* fake switches */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( hyprdriv )
	PORT_START	    /* DIPs */
	PORT_DIPNAME( 0x0001, 0x0000, "Coinage Source" )
	PORT_DIPSETTING(      0x0001, "Dipswitch" )
	PORT_DIPSETTING(      0x0000, "CMOS" )
	PORT_DIPNAME( 0x003e, 0x0034, DEF_STR( Coinage ))
	PORT_DIPSETTING(      0x003e, "USA 10" )
	PORT_DIPSETTING(      0x003c, "USA 11" )
	PORT_DIPSETTING(      0x003a, "USA 12" )
	PORT_DIPSETTING(      0x0038, "USA 13" )
	PORT_DIPSETTING(      0x0036, "USA 9" )
	PORT_DIPSETTING(      0x0034, "USA 1" )
	PORT_DIPSETTING(      0x0032, "USA 2" )
	PORT_DIPSETTING(      0x0030, "USA ECA" )
	PORT_DIPSETTING(      0x002e, "France 1" )
	PORT_DIPSETTING(      0x002c, "France 2" )
	PORT_DIPSETTING(      0x002a, "France 3" )
	PORT_DIPSETTING(      0x0028, "France 4" )
	PORT_DIPSETTING(      0x0026, "France 5" )
	PORT_DIPSETTING(      0x0024, "France 6" )
	PORT_DIPSETTING(      0x0022, "France 7" )
	PORT_DIPSETTING(      0x0020, "France ECA" )
	PORT_DIPSETTING(      0x001e, "German 1" )
	PORT_DIPSETTING(      0x001c, "German 2" )
	PORT_DIPSETTING(      0x001a, "German 3" )
	PORT_DIPSETTING(      0x0018, "German 4" )
	PORT_DIPSETTING(      0x0016, "German 5" )
	PORT_DIPSETTING(      0x0014, "German 5" )
	PORT_DIPSETTING(      0x0012, "German 5" )
	PORT_DIPSETTING(      0x0010, "German ECA" )
	PORT_DIPSETTING(      0x000e, "U.K. 1 ECA" )
	PORT_DIPSETTING(      0x000c, "U.K. 2 ECA" )
	PORT_DIPSETTING(      0x000a, "U.K. 3 ECA" )
	PORT_DIPSETTING(      0x0008, "U.K. 4" )
	PORT_DIPSETTING(      0x0006, "U.K. 5" )
	PORT_DIPSETTING(      0x0004, "U.K. 6 ECA" )
	PORT_DIPSETTING(      0x0002, "U.K. 7 ECA" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Free_Play ))
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Power Up Test Loop" )
	PORT_DIPSETTING(      0x0080, DEF_STR( No ))
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Unknown ))
	PORT_DIPSETTING(      0x0100, "0" )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPNAME( 0x0600, 0x0200, "Resolution" )
	PORT_DIPSETTING(      0x0600, "0" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Medium ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Low ) )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x1800, 0x0000, "Graphics Speed" )
	PORT_DIPSETTING(      0x0000, "45 MHz" )
	PORT_DIPSETTING(      0x0800, "47 MHz" )
	PORT_DIPSETTING(      0x1000, "49 MHz" )
	PORT_DIPSETTING(      0x1800, "51 MHz" )
	PORT_DIPNAME( 0x2000, 0x2000, "Brake" )
	PORT_DIPSETTING(      0x2000, "Enabled" )
	PORT_DIPSETTING(      0x0000, "Disabled" )
	PORT_DIPNAME( 0x4000, 0x0000, "Power On Self Test" )
	PORT_DIPSETTING(      0x0000, DEF_STR( No ))
	PORT_DIPSETTING(      0x4000, DEF_STR( Yes ))
	PORT_DIPNAME( 0x8000, 0x8000, "Test Switch" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ))
	PORT_DIPSETTING(      0x0000, DEF_STR( On ))

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_TILT ) /* Slam Switch */
	PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test switch */
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_VOLUME_DOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_VOLUME_UP )
	PORT_BIT( 0x6000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SPECIAL )	/* Bill */

	PORT_START
	PORT_BIT( 0x0003, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT ) PORT_2WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xffff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00ff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(25)
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(20) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(100) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x10,0xf0) PORT_SENSITIVITY(25) PORT_KEYDELTA(25)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_SPECIAL )
INPUT_PORTS_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static struct mips3_config config =
{
	16384,			/* code cache size */
	16384,			/* data cache size */
	SYSTEM_CLOCK	/* system clock rate */
};

MACHINE_DRIVER_START( seattle_common )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", R5000LE, SYSTEM_CLOCK*3)
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(seattle_map,0)

	MDRV_FRAMES_PER_SECOND(57)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(seattle)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 400)
	MDRV_VISIBLE_AREA(0, 511, 0, 399)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(voodoo_1x4mb)
	MDRV_VIDEO_STOP(voodoo)
	MDRV_VIDEO_UPDATE(voodoo)

	/* sound hardware */
MACHINE_DRIVER_END


MACHINE_DRIVER_START( phoenixsa )
	MDRV_IMPORT_FROM(seattle_common)
	MDRV_CPU_REPLACE("main", R4700LE, SYSTEM_CLOCK*2)
	MDRV_IMPORT_FROM(dcs2_audio)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( seattle150 )
	MDRV_IMPORT_FROM(seattle_common)
	MDRV_CPU_REPLACE("main", R5000LE, SYSTEM_CLOCK*3)
	MDRV_IMPORT_FROM(dcs2_audio)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( seattle200 )
	MDRV_IMPORT_FROM(seattle_common)
	MDRV_CPU_REPLACE("main", R5000LE, SYSTEM_CLOCK*4)
	MDRV_IMPORT_FROM(dcs2_audio)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( carnevil )
	MDRV_IMPORT_FROM(seattle150)
	MDRV_VIDEO_UPDATE(carnevil)
MACHINE_DRIVER_END


MACHINE_DRIVER_START( flagstaff )
	MDRV_IMPORT_FROM(seattle_common)
	MDRV_CPU_REPLACE("main", R5000LE, SYSTEM_CLOCK*4)
	MDRV_VIDEO_START(voodoo_2x4mb)
	MDRV_IMPORT_FROM(cage_seattle)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( wg3dh )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version L1.1 */
	ROM_LOAD16_BYTE( "soundl11.u95", 0x000000, 0x8000, CRC(c589458c) SHA1(0cf970a35910a74cdcf3bd8119bfc0c693e19b00) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* Boot Code Version L1.2 */
	ROM_LOAD( "wg3dh_12.u32", 0x000000, 0x80000, CRC(15e4cea2) SHA1(72c0db7dc53ce645ba27a5311b5ce803ad39f131) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "wg3dh", 0, MD5(424dbda376e8c45ec873b79194bdb924) SHA1(c12875036487a9324734012e601d1f234d2e783e) )
ROM_END


ROM_START( mace )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version L1.1 */
	ROM_LOAD16_BYTE( "soundl11.u95", 0x000000, 0x8000, CRC(c589458c) SHA1(0cf970a35910a74cdcf3bd8119bfc0c693e19b00) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "maceboot.u32", 0x000000, 0x80000, CRC(effe3ebc) SHA1(7af3ca3580d6276ffa7ab8b4c57274e15ee6bcbb) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "mace", 0, BAD_DUMP MD5(276577faa5632eb23dc5a97c11c0a1b1) SHA1(e2cce4ff2e15267b7008422252bdf62b188cf743) )
ROM_END


ROM_START( sfrush )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* Boot Code Version L1.0 */
	ROM_LOAD( "hdboot.u32", 0x000000, 0x80000, CRC(39a35f1b) SHA1(c46d83448399205d38e6e41dd56abbc362254254) )

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )	/* TMS320C31 boot ROM  Version L1.0 */
	ROM_LOAD32_BYTE( "sndboot.u69", 0x000000, 0x080000, CRC(7e52cdc7) SHA1(f735063e19d2ca672cef6d761a2a47df272e8c59) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 )	/* TMS320C31 sound ROMs */
	ROM_LOAD32_WORD( "sfrush.u62",  0x400000, 0x200000, CRC(5d66490e) SHA1(bd39ea3b45d44cae6ca5890f365653326bbecd2d) )
	ROM_LOAD32_WORD( "sfrush.u61",  0x400002, 0x200000, CRC(f3a00ee8) SHA1(c1ac780efc32b2e30522d7cc3e6d92e7daaadddd) )
	ROM_LOAD32_WORD( "sfrush.u53",  0x800000, 0x200000, CRC(71f8ddb0) SHA1(c24bef801f43bae68fda043c4356e8cf1298ca97) )
	ROM_LOAD32_WORD( "sfrush.u49",  0x800002, 0x200000, CRC(dfb0a54c) SHA1(ed34f9485f7a7e5bb73bf5c6428b27548e12db12) )

	DISK_REGION( REGION_DISKS )	/* Hard Drive Version L1.06 */
	DISK_IMAGE( "sfrush", 0, MD5(7a77addb141fc11fd5ca63850382e0d1) SHA1(0e5805e255e91f08c9802a04b42056d61ba5eb41) )
ROM_END


ROM_START( sfrushrk )
	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* Boot Code */
	ROM_LOAD( "boot.bin",   0x000000, 0x080000, CRC(0555b3cf) SHA1(a48abd6d06a26f4f9b6c52d8c0af6095b6be57fd) )

	ROM_REGION32_LE( 0x200000, REGION_USER2, 0 )	/* TMS320C31 boot ROM */
	ROM_LOAD32_BYTE( "audboot.bin",    0x000000, 0x080000, CRC(c70c060d) SHA1(dd014bd13efdf5adc5450836bd4650351abefc46) )

	ROM_REGION32_LE( 0x1000000, REGION_USER3, 0 )	/* TMS320C31 sound ROMs */
	ROM_LOAD32_WORD( "audio.u62",  0x400000, 0x200000, CRC(cacf09e3) SHA1(349af1767cb0ee2a0eb9d7c2ab078fcae5fec8e7) )
	ROM_LOAD32_WORD( "audio.u61",  0x400002, 0x200000, CRC(ea895d29) SHA1(1edde0497f2abd1636c5d7bcfbc03bcff321261c) )
	ROM_LOAD32_WORD( "audio.u53",  0x800000, 0x200000, CRC(51c89a14) SHA1(6bc62bcda224040a4596d795132874828011a038) )
	ROM_LOAD32_WORD( "audio.u49",  0x800002, 0x200000, CRC(e6b684d3) SHA1(1f5bab7fae974cecc8756dd23e3c7aa2cf6e7dc7) )

	DISK_REGION( REGION_DISKS )	/* Hard Drive Version 1.2 */
	DISK_IMAGE( "sfrushrk", 0, MD5(425c83a4fd389d820aceabf2c72e6107) SHA1(75aba7be869996ff522163466c97f88f78904fe0) )
ROM_END


ROM_START( calspeed )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "caspd1_2.u32", 0x000000, 0x80000, CRC(0a235e4e) SHA1(b352f10fad786260b58bd344b5002b6ea7aaf76d) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "calspeed", 0, MD5(dc8c919af86a1ab88a0b05ea2b6c74b3) SHA1(e6cbc8290af2df9704838a925cb43b6972b80d95) )
ROM_END


ROM_START( vaportrx )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "vaportrx.snd", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "vtrxboot.bin", 0x000000, 0x80000, CRC(ee487a6c) SHA1(fb9efda85047cf615f24f7276a9af9fd542f3354) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "vaportrx", 0, MD5(eb8dcf83fe8b7122481d24ad8fbc8a9a) SHA1(f6ddb8eb66d979d49799e39fa4d749636693a1b0) )
ROM_END


ROM_START( vaportrp )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "vaportrx.snd", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "vtrxboot.bin", 0x000000, 0x80000, CRC(ee487a6c) SHA1(fb9efda85047cf615f24f7276a9af9fd542f3354) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "vaportrp", 0, MD5(fac4d37e049bc649696f4834044860e6) SHA1(75e2eaf81c69d2a337736dbead804ac339fd0675) )
ROM_END


ROM_START( biofreak )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "biofreak.u32", 0x000000, 0x80000, CRC(cefa00bb) SHA1(7e171610ede1e8a448fb8d175f9cb9e7d549de28) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "biofreak", 0, MD5(f4663a3fd0ceed436756710b97d283e4) SHA1(88b87cb651b97eac117c9342127938e30dc8c138) )
ROM_END


ROM_START( blitz )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* Boot Code Version 1.2 */
	ROM_LOAD( "blitz1_2.u32", 0x000000, 0x80000, CRC(38dbecf5) SHA1(7dd5a5b3baf83a7f8f877ff4cd3f5e8b5201b36f) )

	DISK_REGION( REGION_DISKS )	/* Hard Drive Version 1.21 */
	DISK_IMAGE( "blitz", 0, MD5(9cec59456c4d239ba05c7802082489e4) SHA1(0f001488b3709d40cee5e278603df2bbae1116b8) )
ROM_END


ROM_START( blitz99 )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "blitz99.u32", 0x000000, 0x80000, CRC(777119b2) SHA1(40d255181c2f3a787919c339e83593fd506779a5) )

	DISK_REGION( REGION_DISKS )	/* Hard Drive Version 1.30 */
	DISK_IMAGE( "blitz99", 0, MD5(4bb6caf8f985e90d99989eede5504188) SHA1(4675751875943b756c8db6997fd288938a7999bb) )
ROM_END


ROM_START( blitz2k )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )	/* Boot Code Version 1.4 */
	ROM_LOAD( "bltz2k14.u32", 0x000000, 0x80000, CRC(ac4f0051) SHA1(b8125c17370db7bfd9b783230b4ef3d5b22a2025) )

	DISK_REGION( REGION_DISKS )	/* Hard Drive Version 1.5 */
	DISK_IMAGE( "blitz2k", 0, MD5(7778a82f35c05ed797b315439843246c) SHA1(153a7df368833cd5f5a52c3fe17045c5549a0c17) )
ROM_END


ROM_START( carnevil )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "sound102.u95", 0x000000, 0x8000, CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "boot.u32", 0x000000, 0x80000, CRC(82c07f2e) SHA1(fa51c58022ce251c53bad12fc6ffadb35adb8162) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "carnevil", 0, BAD_DUMP MD5(6eafae86091c0a915cf8cfdc3d73adc2) SHA1(5e6524d4b97de141c38e301a17e8af15661cb5d6) )
ROM_END


ROM_START( hyprdriv )
	ROM_REGION16_LE( 0x410000, REGION_SOUND1, 0 )	/* ADSP-2115 data Version 1.02 */
	ROM_LOAD16_BYTE( "seattle.snd", 0x000000, 0x8000, BAD_DUMP CRC(bec7d3ae) SHA1(db80aa4a645804a4574b07b9f34dec6b6b64190d) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "hyprdrve.u32", 0x000000, 0x80000, CRC(3e18cb80) SHA1(b18cc4253090ee1d65d72a7ec0c426ed08c4f238) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "hyprdriv", 0, MD5(480c43735b0b83eb10c0223283d4226c) SHA1(2e42fecbb8722c736cccdca7ed3b21fbc75e345a) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

static void init_common(int ioasic, int serialnum, int yearoffs, int config)
{
	/* initialize the subsystems */
	ide_controller_init(0, &ide_intf);
	midway_ioasic_init(ioasic, serialnum, yearoffs, ioasic_irq);

	/* set our VBLANK callback */
	voodoo_set_vblank_callback(vblank_assert);

	/* switch off the configuration */
	board_config = config;
	switch (config)
	{
		case PHOENIX_CONFIG:
			/* original Phoenix board only has 4MB of RAM */
			memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0x00400000, 0x007fffff, 0, 0xa0000000, MRA32_NOP);
			memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x00400000, 0x007fffff, 0, 0xa0000000, MWA32_NOP);
			break;

		case SEATTLE_WIDGET_CONFIG:
			/* set up the widget board */
			memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0xb6c00000, 0xb6c0001f, 0, 0, widget_r);
			memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb6c00000, 0xb6c0001f, 0, 0, widget_w);
			smc91c94_init(&ethernet_intf);
			break;

		case FLAGSTAFF_CONFIG:
			/* set up the analog inputs */
			memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0xb4000000, 0xb4000003, 0, 0, analog_port_r);
			memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb4000000, 0xb4000003, 0, 0, analog_port_w);

			/* set up the ethernet controller */
			memory_install_read32_handler (0, ADDRESS_SPACE_PROGRAM, 0xb6c00000, 0xb6c0003f, 0, 0, ethernet_r);
			memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb6c00000, 0xb6c0003f, 0, 0, ethernet_w);
			smc91c94_init(&ethernet_intf);
			break;
	}
}


static DRIVER_INIT( wg3dh )
{
	dcs2_init(0x3839);
	init_common(MIDWAY_IOASIC_STANDARD, 310/* others? */, 80, PHOENIX_CONFIG);

	/* speedups */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x80115e00, 0x80115e03, 0, 0, generic_speedup_r);
	generic_speedup = &rambase[0x115e00/4];
}


static DRIVER_INIT( mace )
{
	dcs2_init(0x3839);
	init_common(MIDWAY_IOASIC_MACE, 319/* others? */, 80, SEATTLE_CONFIG);

	/* no obvious speedups */
}


static DRIVER_INIT( sfrush )
{
	cage_init(REGION_USER2, 0x5236);
	init_common(MIDWAY_IOASIC_STANDARD, 315/* no alternates */, 100, FLAGSTAFF_CONFIG);

	/* speedups */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x8012498c, 0x8012498f, 0, 0, generic_speedup_r);
	generic_speedup = &rambase[0x12498c/4];
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x80120000, 0x80120003, 0, 0, generic_speedup2_r);
	generic_speedup2 = &rambase[0x120000/4];
}


static DRIVER_INIT( sfrushrk )
{
	cage_init(REGION_USER2, 0x5329);
	init_common(MIDWAY_IOASIC_SFRUSHRK, 331/* unknown */, 100, FLAGSTAFF_CONFIG);

	/* speedups */
//  memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x8012498c, 0x8012498f, 0, 0, generic_speedup_r);
//  generic_speedup = &rambase[0x12498c/4];
//  memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x80120000, 0x80120003, 0, 0, generic_speedup2_r);
//  generic_speedup2 = &rambase[0x120000/4];
}


static DRIVER_INIT( calspeed )
{
	dcs2_init(0x39c0);
	init_common(MIDWAY_IOASIC_CALSPEED, 328/* others? */, 100, SEATTLE_WIDGET_CONFIG);
	midway_ioasic_set_auto_ack(1);

	/* speedups */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x802e6480, 0x802e6483, 0, 0, generic_speedup_r);
	generic_speedup = &rambase[0x2e6480/4];
}


static DRIVER_INIT( vaportrx )
{
	dcs2_init(0x39c2);
	init_common(MIDWAY_IOASIC_VAPORTRX, 324/* 334? unknown */, 100, SEATTLE_WIDGET_CONFIG);

	/* speedups */
}


static DRIVER_INIT( biofreak )
{
	dcs2_init(0x3835);
	init_common(MIDWAY_IOASIC_STANDARD, 231/* no alternates */, 80, SEATTLE_CONFIG);

	/* speedups */
//  memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x802502bc, 0x802502bf, 0, 0, generic_speedup_w);
//  generic_speedup = &rambase[0x2502bc/4];
}


static DRIVER_INIT( blitz )
{
	dcs2_init(0x39c2);
	init_common(MIDWAY_IOASIC_BLITZ99, 444/* or 528 */, 80, SEATTLE_CONFIG);

	/* for some reason, the code in the ROM appears buggy; this is a small patch to fix it */
	rombase[0x934/4] += 4;

	/* speedups */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x80243d58, 0x80243d5b, 0, 0, generic_speedup_w);
	generic_speedup = &rambase[0x243d58/4];
}


static DRIVER_INIT( blitz99 )
{
	dcs2_init(0x0afb);
	init_common(MIDWAY_IOASIC_BLITZ99, 481/* or 484 or 520 */, 80, SEATTLE_CONFIG);

	/* speedups */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x802502bc, 0x802502bf, 0, 0, generic_speedup_w);
	generic_speedup = &rambase[0x2502bc/4];
}


static DRIVER_INIT( blitz2k )
{
	dcs2_init(0x0b5d);
	init_common(MIDWAY_IOASIC_BLITZ99, 494/* or 498 */, 80, SEATTLE_CONFIG);

	/* speedups */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x8024e8d8, 0x8024e8db, 0, 0, generic_speedup_w);
	generic_speedup = &rambase[0x24e8d8/4];
}


static DRIVER_INIT( carnevil )
{
	dcs2_init(0x0af7);
	init_common(MIDWAY_IOASIC_CARNEVIL, 469/* 469 or 486 or 528 */, 80, SEATTLE_CONFIG);

	/* set up the gun */
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb6800000, 0xb680001f, 0, 0, carnevil_gun_r);
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb6800000, 0xb680001f, 0, 0, carnevil_gun_w);

	/* speedups */
	memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0x801a2bac, 0x801a2baf, 0, 0, generic_speedup_w);
	generic_speedup = &rambase[0x1a2bac/4];
}


static DRIVER_INIT( hyprdriv )
{
	dcs2_init(0x0af7);
	init_common(MIDWAY_IOASIC_HYPRDRIV, 469/* unknown */, 80, SEATTLE_WIDGET_CONFIG);

	/* speedups */
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

/* Atari */
GAME ( 1996, wg3dh,    0,        phoenixsa,  wg3dh,    wg3dh,    ROT0, "Atari Games",  "Wayne Gretzky's 3D Hockey" )
GAME ( 1996, mace,     0,        seattle150, mace,     mace,     ROT0, "Atari Games",  "Mace: The Dark Age" )
GAME ( 1996, sfrush,   0,        flagstaff,  sfrush,   sfrush,   ROT0, "Atari Games",  "San Francisco Rush" )
GAMEX( 1996, sfrushrk, 0,        flagstaff,  sfrushrk, sfrushrk, ROT0, "Atari Games",  "San Francisco Rush: The Rock", GAME_NOT_WORKING )
GAME ( 1998, calspeed, 0,        seattle150, calspeed, calspeed, ROT0, "Atari Games",  "California Speed" )
GAME ( 1998, vaportrx, 0,        seattle200, vaportrx, vaportrx, ROT0, "Atari Games",  "Vapor TRX" )
GAME ( 1998, vaportrp, vaportrx, seattle200, vaportrx, vaportrx, ROT0, "Atari Games",  "Vapor TRX (prototype)" )

/* Midway */
GAME ( 1997, biofreak, 0,        seattle150, biofreak, biofreak, ROT0, "Midway Games", "BioFreaks (prototype)" )
GAME ( 1997, blitz,    0,        seattle150, blitz,    blitz,    ROT0, "Midway Games", "NFL Blitz" )
GAME ( 1998, blitz99,  0,        seattle150, blitz99,  blitz99,  ROT0, "Midway Games", "NFL Blitz '99" )
GAME ( 1999, blitz2k,  0,        seattle150, blitz99,  blitz2k,  ROT0, "Midway Games", "NFL Blitz 2000 Gold Edition" )
GAME ( 1998, carnevil, 0,        carnevil,   carnevil, carnevil, ROT0, "Midway Games", "CarnEvil" )
GAME ( 1998, hyprdriv, 0,        seattle200, hyprdriv, hyprdriv, ROT0, "Midway Games", "Hyperdrive" )
