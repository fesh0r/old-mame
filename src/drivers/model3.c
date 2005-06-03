/*
    Sega Model 3
    PowerPC 603e + tilemaps + Real3D 1000 + 68000 + 2x SCSP
    Preliminary driver by Andrew Gardiner, R. Belmont and Ville Linde

    Hardware info from Team Supermodel: Bart Trzynadlowski, Ville Linde, and Stefano Teso

    Hardware revisions
    ------------------
    Step 1.0: 66 MHz PPC
    Step 1.5: 100 MHz PPC, faster 3D engine
    Step 2.0: 166 MHz PPC, even faster 3D engine
    Step 2.1: 166 MHz PPC, same 3D engine as 2.0, differences unknown

===================================================================================

Tilemap generator notes:

    0xF1000000-0xF111FFFF       Tilegen VRAM
    0xF1180000-0xF11800FF (approx.) Tilegen regs

Offsets in tilemap VRAM:

    0       Tile pattern VRAM (can be 4 or 8 bpp, layout is normal chunky-pixel format)
    0xF8000 Layer 0 (top)
    0xFA000 Layer 1
    0xFC000 Layer 2
    0xFE000 Layer 3 (bottom)

    0x100000    Palette (1-5-5-5 A-G-B-R format)

Offsets in tilemap registers:

    0x10: IRQ ack for VBlank

    0x20: layer depths

    xxxxxxxxDCBAxxxxxxxxxxxxxxxxxxxx
    3  2   2   2   1   1   8   4   0
    1  8   4   0   6   2

    A: 0 = layer A is 8-bit, 1 = layer A is 4-bit
    B: 0 = layer B is 8-bit, 1 = layer B is 4-bit
    C: 0 = layer C is 8-bit, 1 = layer C is 4-bit
    D: 0 = layer D is 8-bit, 1 = layer D is 4-bit

Tilemap entry formats (16-bit wide):
15                                                              0
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
| T0|PHI|T14|T13|T12|T11|T10| T9| T8| T7| T6| T5| T4| T3| T2| T1|
+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

    T0 - T14 = pattern name (multiply by 32 or 64 to get a byte offset
               into pattern VRAM.  Yes, bit 15 is T0.)
    To get the palette offset, use the top 7 (8 bpp) or 11 (4 bpp)
    bits of the tilemap entry, *excluding* bit 15.  So PHI-T9
    are the palette select bits for 8bpp and PHI-T5 for 4bpp.

===================================================================================

Harley Davidson (Rev.A)
Sega, 1997

This game runs on Sega Model3 Step2 hardware.

COMM Board
----------

171-7053B  837-11861  NEP-16T MODEL3 COMMUNICATION BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|   LATTICE                     NKK N341256SJ-20     NKK N341256SJ-20     NKK N341256SJ-20    40MHz |
|   PLSI 2032   JP3                                                                      JP1        |
|               JP4             NKK N341256SJ-20     NKK N341256SJ-20     NKK N341256SJ-20          |
|                                                                                             JP2   |
|                                                                                                   |
|   PALCE16V8                                                                                       |
|   315-6075A      68000FN12       315-5804             315-5917              315-5917              |
|                                  (QFP144)             (QFP80)               (QFP80)               |
|   PALCE16V8                                                                                       |
|   315-6074                                                                                        |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|
JP1: 1-2
JP2: 2-3
JP3: not shorted
JP4: shorted


ROM Board
---------

171-7427B  837-13022 MODEL3 STEP2 ROM BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|        MC88915FN70                                     GAL16V8B                                   |
|    JP13                                                315-6090A  JP1  JP11  JP2           JP7 2-3|
|     2-3                                                           1-2  1-2   2-3           JP8 2-3|
|                                                                                            JP9 2-3|
| VROM01.26   VROM00.27    CROM03.1    CROM02.2                           SROM0.21                  |
|                                                                                                   |
| VROM03.28   VROM02.29    CROM01.3    CROM00.4                                                     |
|                                                                                                   |
| VROM05.30   VROM04.31    CROM13.5    CROM12.6     CROM2.18 CROM0.20         SROM1.22  SROM3.24    |
|                                                                                                   |
| VROM07.32   VROM06.33    CROM11.7    CROM10.8                                                     |
|                                                                                                   |
| VROM11.34   VROM10.35    CROM23.9    CROM22.10                                                    |
|                                                                                                   |
| VROM13.36   VROM12.37    CROM21.11   CROM20.12                                                    |
|                                                                                                   |
| VROM15.38   VROM14.39    CROM33.13   CROM32.14    CROM3.17 CROM1.19         SROM2.23  SROM4.25    |
|                                                                                                   |
| VROM17.40   VROM16.41    CROM31.15   CROM30.16                                                    |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|

VROM00.27 MPR-20378 \
VROM01.26 MPR-20377 |
VROM02.29 MPR-20380 |
VROM03.28 MPR-20379 |
VROM04.31 MPR-20382 |
VROM05.30 MPR-20381 |
VROM06.33 MPR-20384 |
VROM07.32 MPR-20383  \ 32MBit DIP42 MASK ROM
VROM10.35 MPR-20386  /
VROM11.34 MPR-20385 |
VROM12.37 MPR-20388 |
VROM13.36 MPR-20387 |
VROM14.39 MPR-20390 |
VROM15.38 MPR-20389 |
VROM16.41 MPR-20392 |
VROM17.40 MPR-20391 /

CROM00.4  MPR-20364 \
CROM01.3  MPR-20363 |
CROM02.2  MPR-20362 |
CROM03.1  MPR-20361  \ 32MBit DIP42 MASK ROM
CROM10.8  MPR-20368  /
CROM11.7  MPR-20367 |
CROM12.6  MPR-20366 |
CROM13.5  MPR-20365 /
CROM20.12 not populated
CROM21.11 not populated
CROM22.10 not populated
CROM23.9  not populated
CROM30.16 EPR-20412 \
CROM31.15 EPR-20411 |  16MBit DIP42 EPROM (27C160)
CROM32.14 EPR-20410 |
CROM33.13 EPR-20409 /

CROM0.20  EPR-20396A \
CROM1.19  EPR-20395A | 16MBit DIP42 EPROM (27C160)
CROM2.18  EPR-20394A |
CROM3.17  EPR-20393A /

SROM0.21  EPR-20397    4MBit DIP40 EPROM (27C4096)
SROM1.22  MPR-20373 \
SROM2.23  MPR-20374 |  32MBit DIP42 MASK ROM
SROM3.24  MPR-20375 |
SROM4.25  MPR-20376 /

The jumpers are used to configure the ROM types (All ROMs are 16 bit)
On Model 3 step 2.0 ROM boards, JP13 selects the size of the VROMs.
1-2 jumpered -> 16MBit (read as 27C160)
2-3 jumpered -> 32MBit (read as 27C322)

JP1, JP11 & JP2 seems to select the size of the CROMs. The jumpers are tied to +5V and ground and to PAL 315-6090A.
Many lines from the PAL are tied to the highest address line of the CROMs.
The contents of the PAL is unknown, so it's difficult to determine what the jumpers do exactly.
It's assumed the PAL does the size selection based on certain pins on the PAL being high or low which are set by the jumpers.
JP1  1-2 \
JP11 1-2 | Selects CROMs 00-03 & 10-13 as 32MBit
JP2  2-3 / and CROMs 30-33 as 16MBit
Alternative jumper configurations are not known.

JP7 selects the size of SROM0 (jumper pin 2 connected to pin 39(A17) of SROM0)
1-2 jumpered -> 1MBit (read as 27C1024)
2-3 jumpered -> 4MBit (read as 27C4096)

JP8 and JP9 seems to select the size of SROM1-4.
JP8 pin 2 is tied to pin 32 of SROM1-4 (highest address line A20)
JP9 pin 2 is tied to some logic.
JP8 & JP9 pin 3 are hardwired together.
JP8 and JP9 jumpered 2-3 selects SROM1-4 as 32MBit.
Alternative jumper configurations are not known.


CPU Board
---------

837-12715 171-7331C MODEL3 STEP2 CPU BOARD
|---------------------------------------------------------------------------------------------------|
| TC59S1616AFT-12  TC59S1616AFT-12                                                                  |
| (TSOP50)         (TSOP50)                                              GAL16V8D                   |
|                                                                        315-6089A                  |
| TC59S1616AFT-12  TC59S1616AFT-12                                       (PLCC20)                   |
| (TSOP50)         (TSOP50)                          45.158MHz        68000-12                      |
|                                                                     (PLCC68)                      |
|           MOTOROLA            PowerPC603ev                                      4066              |
|           XPC106ARX66CE       (QFP240,             SEGA             SEGA            TDA1386T      |
|           MPC106+               HEATSINKED)        315-5687 "SCSP"  315-5687 "SCSP"               |
|           (BGA304)                                 (QFP128)         (QFP128)              LMC6484 |
| PALCE16V8                                                                                         |
| 315-6102          33.333MHz                                                                       |
| (PLCC20)                     MPC950                HM514270CJ7      HM514270CJ7      TDA1386T     |
|                              (QFP32)                                                              |
|                                                                                                   |
| SEGA                            SEGA                                3771                          |
| 315-5894       SYMBIOS          315-5893                                                          |
| (QFP240)       53C810           (QFP240)                                                          |
|                (not populated)                                                                    |
|                                                                                                   |
|                                                    SEGA             SEGA       71AJC46A           |
|                                                    315-5296         315-5649   (SOIC8)            |
|                CY7C199          32MHz              (QFP100)         (QFP100)                      |
|                                                                                                   |
|                                                                                                   |
|                                                                                                   |
|                CY7C199                                                                            |
|                                                                             CN25                  |
|                                                    RTC72423             (Connector for )          |
| KM4132G271AQ-10                                                         (Protection PCB)          |
|                                                                         (   not used   )          |
|              32MHz                                 BATT_3V                                        |
|                          NEC D71051-10                                                            |
|                                                                                                   |
|              24.576MHz      SW1   SW2    LH52B256  LH52B256            DIPSW(8)   SW4    SW3      |
|---------------------------------------------------------------------------------------------------|


Video Board
-----------

837-12716 171-7332H MODEL3 STEP2 VIDEO BOARD
|---------------------------------------------------------------------------------------------------|
|                                                                                                   |
|  D4811650GF  D4811650GF                   33MHz   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                         D4811650GF                M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                              M5M410092FP                                          |
|                                              (TQFP128)                                  M5M4V4169 |
|                           SEGA                            SEGA                          M5M4V4169 |
|              SEGA         315-6058                        315-6060                                |
|              315-6057     (BGA)                           (BGA)                         M5M4V4169 |
|  SEGA        (BGA)                                                                      M5M4V4169 |
|  315-6022                                                                                         |
|  (QFP208)                KM4132G271AQ-10     SEGA                      SEGA                       |
|                          (QFP100)            315-6059                  315-6060         M5M4V4169 |
|                                              (BGA)                     (BGA)            M5M4V4169 |
|                                                                                                   |
|                                                                                                   |
|            M5M410092FP                       M5M410092FP                                M5M4V4169 |
|  SEGA      (TQFP128)                         (TQFP128)                                  M5M4V4169 |
|  315-6061              M5M410092FP                                                                |
|  (BGA)                 (TQFP128)                                                        M5M4V4169 |
|            M5M410092FP                                                                  M5M4V4169 |
|            (TQFP128)       M5M410092FP                                                            |
|                            (TQFP128)         SEGA                      SEGA             M5M4V4169 |
|     SEGA      M5M410092FP                    315-6059                  315-6060         M5M4V4169 |
|     315-5648  (TQFP128)      M5M410092FP     (BGA)                     (BGA)                      |
|     (QFP64)                  (TQFP128)                    SEGA                                    |
|                                                           315-6060                      M5M4V4169 |
|                                                           (BGA)                         M5M4V4169 |
|                                                                                                   |
|                                                                                         M5M4V4169 |
|                                                                                         M5M4V4169 |
|  ADV7120KP30                                                                                      |
|                                                                                                   |
|                                                                                                   |
|            M5M410092FP                       M5M410092FP                                M5M4V4169 |
|  SEGA      (TQFP128)                         (TQFP128)                                  M5M4V4169 |
|  315-6061              M5M410092FP                                                                |
|  (BGA)                 (TQFP128)                                                        M5M4V4169 |
|            M5M410092FP                                                                  M5M4V4169 |
|            (TQFP128)       M5M410092FP                                                            |
|                            (TQFP128)         SEGA                      SEGA             M5M4V4169 |
|     SEGA      M5M410092FP                    315-6059                  315-6060         M5M4V4169 |
|     315-5648  (TQFP128)      M5M410092FP     (BGA)                     (BGA)                      |
|     (QFP64)                  (TQFP128)                    SEGA                                    |
|                                                           315-6060                      M5M4V4169 |
|                                                           (BGA)                         M5M4V4169 |
|                                                                                                   |
|                                                                                         M5M4V4169 |
|                                                                                         M5M4V4169 |
|  ADV7120KP30                                                                                      |
|  (PLCC44)                                                                                         |
|                                                   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                                   M5M4V4169  M5M4V4169  M5M4V4169  M5M4V4169      |
|                                                                                                   |
|---------------------------------------------------------------------------------------------------|

===================================================================================

Scud Race
Sega, 1996

Tis game runs on Sega Model 3 Step 1.5 hardware


Sound Board (bolted to outside of the metal box)
-----------

PCB Layout
----------

PCB Number: 837-10084 DIGITAL AUDIO BD (C) SEGA 1993
---------------------------------------------------------------
84256     D71051GU-10                 D6376    SM5840
EPR19612
Z80             16MHz     SEGA                D65654GF102
                          315-5762            (QFP100)
                          (PLCC68)
                     20MHz           KM68257
                                     KM68257          84256
                                     KM68257
                                          12.288MHz   MPR19603
                                                      MPR19604
                                                      MPR19605
DSW1                                                  MPR19606
----------------------------------------------------------------


Jumpers
JP1: 1-2 (Select 16M ROM on bank 1 - ic57 & ic58)
JP2: 1-2 (Select 16M ROM on bank 2 - ic59 & ic60)



ROM Board
---------

PCB Number: 837-11860 MODEL3 ROM BOARD (C) SEGA 1995

(For dumping reference)

Jumpers    centre pin joins
-------------------------------------------------------
JP3: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP4: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP5: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP6: 2-3   pin2 of ic 1 to 16 and pin 39 of ic 17 to 20
JP7: 2-3   pin2 of ic 22 to 25 and pin 39 ic ic21
JP8: 2-3   pin32 of ic 22 to 25
JP9: 2-3   pin32 of ic 22 to 25
Jumper pos. 1 is +5V

JP1: 1-2   gnd
JP2: 2-3   +5v
Jumper pos. 1 is GND
Jumper pos. 3 is +5V

           pin1 joins
-------------------------------
JP10: 1-2  pin32 of ic 26 to 41

All CROM ROMs are 32M MASK
ALL VROM ROMs are 16M MASK

*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "vidhrdw/generic.h"
#include "machine/eeprom.h"
#include "machine/53c810.h"

int model3_irq_enable;
int model3_irq_state;
int model3_step;
int model3_draw_crosshair = 0;

static UINT64 *work_ram;
static UINT64 *model3_backup;
static int model3_crom_bank = 0;
static int model3_controls_bank;
static data32_t real3d_device_id;

extern UINT64 *paletteram64;

/* defined in machine/model3.c */
extern void model3_machine_init(int step);
extern int model3_tap_read(void);
extern void model3_tap_write(int tck, int tms, int tdi, int trst);
extern void model3_tap_reset(void);
extern READ32_HANDLER(rtc72421_r);
extern WRITE32_HANDLER(rtc72421_w);


/* defined in vidhrdw/model3.c */
READ64_HANDLER(model3_char_r);
WRITE64_HANDLER(model3_char_w);
READ64_HANDLER(model3_tile_r);
WRITE64_HANDLER(model3_tile_w);
READ64_HANDLER(model3_vid_reg_r);
WRITE64_HANDLER(model3_vid_reg_w);
READ64_HANDLER(model3_palette_r);
WRITE64_HANDLER(model3_palette_w);
VIDEO_START(model3);
VIDEO_UPDATE(model3);
WRITE64_HANDLER(real3d_cmd_w);
WRITE64_HANDLER(real3d_display_list_w);
WRITE64_HANDLER(real3d_polygon_ram_w);
void real3d_display_list_end(void);
void real3d_display_list1_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_display_list2_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_vrom_texture_dma(UINT32 src, UINT32 dst, int length, int byteswap);
void real3d_texture_fifo_dma(UINT32 src, int length, int byteswap);
void real3d_polygon_ram_dma(UINT32 src, UINT32 dst, int length, int byteswap);

static void real3d_dma_callback(UINT32 src, UINT32 dst, int length, int byteswap);

#define BYTE_REVERSE32(x)		(((x >> 24) & 0xff) | \
								((x >> 8) & 0xff00) | \
								((x << 8) & 0xff0000) | \
								((x << 24) & 0xff000000))

static UINT32 pci_device_get_reg(int device, int reg)
{
	switch(device)
	{
		case 13:		/* Real3D Controller chip */
			switch(reg)
			{
				case 0:		return real3d_device_id;	/* PCI Vendor ID & Device ID */
				default:
					osd_die("pci_device_get_reg: Real3D controller, unknown reg %02X", reg);
					break;
			}
			break;

		case 14:		/* NCR 53C810 SCSI Controller */
			switch(reg)
			{
				case 0:		return 0x00011000;		/* PCI Vendor ID (0x1000 = LSI Logic) */
				default:
					osd_die("pci_device_get_reg: SCSI Controller, unknown reg %02X", reg);
					break;
			}
			break;
		case 16:		/* ??? (Used by Daytona 2) */
			switch(reg)
			{
				case 0:		return 0x182711db;		/* PCI Vendor ID & Device ID, 315-6183 ??? */
				default:
					osd_die("pci_device_get_reg: Device 16, unknown reg %02X", reg);
					break;
			}
			break;

		default:
			osd_die("pci_device_get_reg: Unknown device %d, reg %02X\n", device, reg);
			break;
	}

	return 0;
}

static void pci_device_set_reg(int device, int reg, UINT32 value)
{
	switch(device)
	{
		case 11:		/* Unknown device for now !!! */
			switch(reg)
			{
				case 0x01:		/* ??? */
					break;
				case 0x04:		/* ??? */
					break;
				case 0x10:		/* ??? */
					break;
				case 0x11:
					break;
				case 0x14:		/* ??? */
					break;
				default:
					osd_die("pci_device_set_reg: Unknown device (11), unknown reg %02X %08X", reg, value);
					break;
			}
			break;

		case 13:		/* Real3D Controller chip */
			switch(reg)
			{
				case 0x01:		/* ??? */
					break;
				case 0x03:
					break;
				case 0x04:		/* ??? */
					break;
				default:
					osd_die("pci_device_set_reg: Real3D controller, unknown reg %02X %08X", reg, value);
					break;
			}
			break;

		case 14:		/* NCR 53C810 SCSI Controller */
			switch(reg)
			{
				case 0x04/4:	/* Status / Command */
					break;
				case 0x0c/4:	/* Header Type / Latency Timer / Cache Line Size */
					break;
				case 0x14/4:	/* Base Address One (Memory) */
					break;
				default:
					osd_die("pci_device_set_reg: SCSI Controller, unknown reg %02X, %08X", reg, value);
					break;
			}
			break;

		case 16:		/* ??? (Used by Daytona 2) */
			switch(reg)
			{
				case 4:			/* Base address ? (set to 0xC3000000) */
					break;
				default:
					osd_die("pci_device_set_reg: Device 16, unknown reg %02X, %08X", reg, value);
					break;
			}
			break;

		default:
			osd_die("pci_device_set_reg: Unknown device %d, reg %02X, %08X\n", device, reg, value);
			break;
	}
}

/*****************************************************************************/
/* Motorola MPC105 PCI Bridge/Memory Controller */

static data32_t mpc105_regs[0x40];
static data32_t mpc105_addr;
static int pci_bus;
static int pci_device;
static int pci_function;
static int pci_reg;

static READ64_HANDLER( mpc105_addr_r )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)mpc105_addr << 32;
	}
	return 0;
}

static WRITE64_HANDLER( mpc105_addr_w )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		UINT32 d = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc105_addr = data >> 32;

		pci_bus = (d >> 16) & 0xff;
		pci_device = (d >> 11) & 0x1f;
		pci_function = (d >> 8) & 0x7;
		pci_reg = (d >> 2) & 0x3f;
	}
}

static READ64_HANDLER( mpc105_data_r )
{
	return BYTE_REVERSE32(pci_device_get_reg(pci_device, pci_reg));
}

static WRITE64_HANDLER( mpc105_data_w )
{
	if (!(mem_mask & 0xffffffff))
	{
		pci_device_set_reg(pci_device, pci_reg, BYTE_REVERSE32((UINT32)data));
	}
}

static READ64_HANDLER( mpc105_reg_r )
{
	return ((UINT64)(mpc105_regs[(offset*2)+0]) << 32) |
			(UINT64)(mpc105_regs[(offset*2)+1]);
}

static WRITE64_HANDLER( mpc105_reg_w )
{
	mpc105_regs[(offset*2)+0] = (UINT32)(data >> 32);
	mpc105_regs[(offset*2)+1] = (UINT32)data;
}

static void mpc105_init(void)
{
	/* set reset values */
	memset(mpc105_regs, 0, sizeof(mpc105_regs));
	mpc105_regs[0x00/4] = 0x00011057;		/* Vendor ID & Device ID */
	mpc105_regs[0x04/4] = 0x00800006;		/* PCI Command & PCI Status */
	mpc105_regs[0x08/4] = 0x00060000;		/* Class code */
	mpc105_regs[0xa8/4] = 0x0010ff00;		/* Processor interface configuration 1 */
	mpc105_regs[0xac/4] = 0x060c000c;		/* Processor interface configuration 2 */
	mpc105_regs[0xb8/4] = 0x04000000;
	mpc105_regs[0xf0/4] = 0x0000ff02;		/* Memory control configuration 1 */
	mpc105_regs[0xf4/4] = 0x00030000;		/* Memory control configuration 2 */
	mpc105_regs[0xfc/4] = 0x00000010;		/* Memory control configuration 4 */
}

/*****************************************************************************/
/* Motorola MPC106 PCI Bridge/Memory Controller */

static data32_t mpc106_regs[0x40];
static data32_t mpc106_addr;

static READ64_HANDLER( mpc106_addr_r )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)mpc106_addr << 32;
	}
	return 0;
}

static WRITE64_HANDLER( mpc106_addr_w )
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		UINT32 d = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc105_addr = data >> 32;

		pci_bus = (d >> 16) & 0xff;
		pci_device = (d >> 11) & 0x1f;
		pci_function = (d >> 8) & 0x7;
		pci_reg = (d >> 2) & 0x3f;
	}
}

static READ64_HANDLER( mpc106_data_r )
{
	if(pci_device == 0) {
		return ((UINT64)(BYTE_REVERSE32(mpc106_regs[(pci_reg/2)+1])) << 32) |
			   ((UINT64)(BYTE_REVERSE32(mpc106_regs[(pci_reg/2)+0])));
	}
	return BYTE_REVERSE32(pci_device_get_reg(pci_device, pci_reg));
}

static WRITE64_HANDLER( mpc106_data_w )
{
	if(pci_device == 0) {
		mpc106_regs[(pci_reg/2)+1] = BYTE_REVERSE32((UINT32)(data >> 32));
		mpc106_regs[(pci_reg/2)+0] = BYTE_REVERSE32((UINT32)(data));
		return;
	}
	if (!(mem_mask & 0xffffffff))
	{
		pci_device_set_reg(pci_device, pci_reg, BYTE_REVERSE32((UINT32)data));
	}
}

static READ64_HANDLER( mpc106_reg_r )
{
	return ((UINT64)(mpc106_regs[(offset*2)+0]) << 32) |
			(UINT64)(mpc106_regs[(offset*2)+1]);
}

static WRITE64_HANDLER( mpc106_reg_w )
{
	mpc106_regs[(offset*2)+0] = (UINT32)(data >> 32);
	mpc106_regs[(offset*2)+1] = (UINT32)data;
}

static void mpc106_init(void)
{
	/* set reset values */
	memset(mpc106_regs, 0, sizeof(mpc106_regs));
	mpc106_regs[0x00/4] = 0x00021057;		/* Vendor ID & Device ID */
	mpc106_regs[0x04/4] = 0x00800006;		/* PCI Command & PCI Status */
	mpc106_regs[0x08/4] = 0x00060000;		/* Class code */
	mpc106_regs[0x0c/4] = 0x00000800;		/* Cache line size */
	mpc106_regs[0x70/4] = 0x00cd0000;		/* Output driver control */
	mpc106_regs[0xa8/4] = 0x0010ff00;		/* Processor interface configuration 1 */
	mpc106_regs[0xac/4] = 0x060c000c;		/* Processor interface configuration 2 */
	mpc106_regs[0xb8/4] = 0x04000000;
	mpc106_regs[0xc0/4] = 0x00000100;		/* Error enabling 1 */
	mpc106_regs[0xe0/4] = 0x00420fff;		/* Emulation support configuration 1 */
	mpc106_regs[0xe8/4] = 0x00200000;		/* Emulation support configuration 2 */
	mpc106_regs[0xf0/4] = 0x0000ff02;		/* Memory control configuration 1 */
	mpc106_regs[0xf4/4] = 0x00030000;		/* Memory control configuration 2 */
	mpc106_regs[0xfc/4] = 0x00000010;		/* Memory control configuration 4 */
}

/*****************************************************************************/

static READ64_HANDLER(scsi_r)
{
	int reg = offset*8;
	UINT64 r = 0;
	if (!(mem_mask & U64(0xff00000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+0) << 56;
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+1) << 48;
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+2) << 40;
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+3) << 32;
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+4) << 24;
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		r |= (UINT64)lsi53c810_reg_r(reg+5) << 16;
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		r |= (UINT64)lsi53c810_reg_r(reg+6) << 8;
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		r |= (UINT64)lsi53c810_reg_r(reg+7) << 0;
	}

	return r;
}

static WRITE64_HANDLER(scsi_w)
{
	int reg = offset*8;
	if (!(mem_mask & U64(0xff00000000000000))) {
		lsi53c810_reg_w(reg+0, data >> 56);
	}
	if (!(mem_mask & U64(0x00ff000000000000))) {
		lsi53c810_reg_w(reg+1, data >> 48);
	}
	if (!(mem_mask & U64(0x0000ff0000000000))) {
		lsi53c810_reg_w(reg+2, data >> 40);
	}
	if (!(mem_mask & U64(0x000000ff00000000))) {
		lsi53c810_reg_w(reg+3, data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ff000000))) {
		lsi53c810_reg_w(reg+4, data >> 24);
	}
	if (!(mem_mask & U64(0x0000000000ff0000))) {
		lsi53c810_reg_w(reg+5, data >> 16);
	}
	if (!(mem_mask & U64(0x000000000000ff00))) {
		lsi53c810_reg_w(reg+6, data >> 8);
	}
	if (!(mem_mask & U64(0x00000000000000ff))) {
		lsi53c810_reg_w(reg+7, data >> 0);
	}
}

static void scsi_irq_callback(void)
{
	model3_irq_state |= model3_irq_enable & ~0x60;	/* FIXME: enable only SCSI interrupt */
	cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
}

/*****************************************************************************/
/* Real3D DMA */

static data32_t dma_data;
static data32_t dma_status;
static data32_t dma_source;
static data32_t dma_dest;
static data32_t dma_endian;
static data32_t dma_irq;

static READ64_HANDLER( real3d_dma_r )
{
	switch(offset)
	{
		case 1:
			return (dma_irq << 24) | (dma_endian << 8);
		case 2:
			if(!(mem_mask & U64(0x00000000ffffffff))) {
				return dma_data;
			}
			break;
	}
	printf("real3d_dma_r: %08X, %08X%08X\n", offset, (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
	return 0;
}

static WRITE64_HANDLER( real3d_dma_w )
{
	switch(offset)
	{
		case 0:
			if(!(mem_mask & U64(0xffffffff00000000))) {		/* DMA source address */
				dma_source = BYTE_REVERSE32((UINT32)(data >> 32));
				return;
			}
			if(!(mem_mask & U64(0x00000000ffffffff))) {		/* DMA destination address */
				dma_dest = BYTE_REVERSE32((UINT32)(data));
				return;
			}
			break;
		case 1:
			if(!(mem_mask & U64(0xffffffff00000000)))		/* DMA length */
			{
				int length = BYTE_REVERSE32((UINT32)(data >> 32)) * 4;
				if (dma_endian & 0x80)
				{
					real3d_dma_callback(dma_source, dma_dest, length, 0);
				}
				else
				{
					real3d_dma_callback(dma_source, dma_dest, length, 1);
				}
				dma_irq |= 0x01;
				scsi_irq_callback();
				return;
			}
			else if(!(mem_mask & U64(0x0000000000ff0000)))
			{
				if(data & 0x10000) {
					dma_irq &= ~0x1;
				}
				return;
			}
			else if(!(mem_mask & U64(0x000000000000ff00)))
			{
				dma_endian = (data >> 8) & 0xff;
				return;
			}
			break;
		case 2:
			if(!(mem_mask & U64(0xffffffff00000000))) {		/* DMA command */
				UINT32 cmd = BYTE_REVERSE32((UINT32)(data >> 32));
				if(cmd & 0x20000000) {
					dma_data = BYTE_REVERSE32(real3d_device_id);	/* (PCI Vendor & Device ID) */
				}
				else if(cmd & 0x80000000) {
					dma_status ^= 0xffffffff;
					dma_data = dma_status;
				}
				return;
			}
			if(!(mem_mask & U64(0x00000000ffffffff))) {		/* ??? */
				dma_data = 0xffffffff;
				return;
			}
			return;
			break;
	}
	osd_die("real3d_dma_w: %08X, %08X%08X, %08X%08X\n", offset, (UINT32)(data >> 32), (UINT32)(data), (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
}

static void real3d_dma_callback(UINT32 src, UINT32 dst, int length, int byteswap)
{
	switch(dst >> 24)
	{
		case 0x88:		/* Display List End Trigger */
			real3d_display_list_end();
			break;
		case 0x8c:		/* Display List RAM 2 */
			real3d_display_list2_dma(src, dst, length, byteswap);
			break;
		case 0x8e:		/* Display List RAM 1 */
			real3d_display_list1_dma(src, dst, length, byteswap);
			break;
		case 0x90:		/* VROM Texture Download */
			real3d_vrom_texture_dma(src, dst, length, byteswap);
			break;
		case 0x94:		/* Texture FIFO */
			real3d_texture_fifo_dma(src, length, byteswap);
			break;
		case 0x98:		/* Polygon RAM */
			real3d_polygon_ram_dma(src, dst, length, byteswap);
			break;
		case 0x9c:		/* Unknown */
			break;
		default:
			osd_die("dma_callback: %08X, %08X, %d at %08X\n", src, dst, length, activecpu_get_pc());
			break;
	}
}

/*****************************************************************************/

/* this is a 93C46 but with reset delay that is needed by Lost World */
static struct EEPROM_interface eeprom_interface =
{
	6,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxx",	/* lock command */
	"*10011xxxx",	/* unlock command */
	1,				/* enable_multi_read */
	5				/* reset_delay (Lost World needs this, very similar to wbeachvl in playmark.c) */
};

static void eeprom_handler(mame_file *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)	EEPROM_load(file);
	}
}

static NVRAM_HANDLER( model3 )
{
	if( stricmp(Machine->gamedrv->name, "lostwsga") == 0 ||
		stricmp(Machine->gamedrv->name, "dirtdvls") == 0 ||
		stricmp(Machine->gamedrv->name, "von2") == 0 ||
		stricmp(Machine->gamedrv->name, "von254g") == 0)
	{
		eeprom_handler(file, read_or_write);
	} else {
		nvram_handler_93C46(file, read_or_write);
	}

	if (read_or_write)
	{
		mame_fwrite(file, model3_backup, 0x1ffff);
	}
	else
	{
		if (file)
		{
			mame_fread(file, model3_backup, 0x1ffff);
		}
	}
}

static void model3_init(int step)
{
	model3_step = step;
	cpu_setbank( 1, memory_region( REGION_USER1 ) + 0x800000 ); /* banked CROM */

	model3_machine_init(step);	// step 1.5
	model3_tap_reset();

	if(step < 0x20) {
		if( stricmp(Machine->gamedrv->name, "vs215") == 0 ||
			stricmp(Machine->gamedrv->name, "vs29815") == 0 ||
			stricmp(Machine->gamedrv->name, "bass") == 0)
		{
			mpc106_init();
		} else {
			mpc105_init();
		}
		lsi53c810_init((UINT8*)work_ram, scsi_irq_callback, real3d_dma_callback);
		real3d_device_id = 0x16c311db;	/* PCI Vendor ID (11db = SEGA), Device ID (16c3 = 315-5827) */
	}
	else {
		mpc106_init();
		real3d_device_id = 0x178611db;	/* PCI Vendor ID (11db = SEGA), Device ID (1786 = 315-6022) */
	}
}

static MACHINE_INIT(model3_10) { model3_init(0x10); }
static MACHINE_INIT(model3_15) { model3_init(0x15); }
static MACHINE_INIT(model3_20) { model3_init(0x20); }
static MACHINE_INIT(model3_21) { model3_init(0x21); }

static data32_t eeprom_bit = 0;

static UINT64 controls_2;
static UINT64 controls_3;
static data8_t model3_serial_fifo1;
static data8_t model3_serial_fifo2;
static int lightgun_reg_sel;
static int adc_channel;

static READ64_HANDLER( model3_ctrl_r )
{
	switch( offset )
	{
		case 0:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return (UINT64)model3_controls_bank << 56;
			}
			else if (!(mem_mask & 0xff000000))
			{
				if(model3_controls_bank & 0x1) {
					eeprom_bit = EEPROM_read_bit() << 5;
					return ((readinputport(1) & ~0x20) | eeprom_bit) << 24;
				}
				else {
					return (readinputport(0)) << 24;
				}
			}
			break;

		case 1:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return (UINT64)readinputport(2) << 56;
			}
			else if (!(mem_mask & 0xff000000))
			{
				return readinputport(3) << 24;
			}
			break;

		case 2:
			return U64(0xffffffffffffffff);

		case 3:
			return U64(0xffffffffffffffff);		/* Dip switches */

		case 4:
			return U64(0xffffffffffffffff);

		case 5:
			if (!(mem_mask & 0xff000000))					/* Serial comm RX FIFO 1 */
			{
				return (UINT64)model3_serial_fifo1 << 24;
			}
			break;

		case 6:
			if (!(mem_mask & U64(0xff00000000000000)))		/* Serial comm RX FIFO 2 */
			{
				return (UINT64)model3_serial_fifo2 << 56;
			}
			else if (!(mem_mask & 0xff000000))				/* Serial comm full/empty flags */
			{
				return 0x0c << 24;
			}
			break;

		case 7:
			if (!(mem_mask & 0xff000000))		/* ADC Data read */
			{
				UINT8 adc_data = readinputport(5 + adc_channel);
				adc_channel++;
				adc_channel &= 0x7;
				return (UINT64)adc_data << 24;
			}
			break;
	}

	osd_die("ctrl_r: %02X, %08X%08X\n", offset, (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
	return 0;
}

static WRITE64_HANDLER( model3_ctrl_w )
{
	switch(offset)
	{
		case 0:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				int reg = (data >> 56) & 0xff;
				EEPROM_write_bit((reg & 0x20) ? 1 : 0);
				EEPROM_set_clock_line((reg & 0x80) ? ASSERT_LINE : CLEAR_LINE);
				EEPROM_set_cs_line((reg & 0x40) ? CLEAR_LINE : ASSERT_LINE);
				model3_controls_bank = reg & 0xff;
			}
			return;

		case 2:
			COMBINE_DATA(&controls_2);
			return;

		case 3:
			COMBINE_DATA(&controls_3);
			return;

		case 4:
			if (!(mem_mask & U64(0xff00000000000000)))	/* Port 4 direction */
			{

			}
			if (!(mem_mask & 0xff000000))				/* Serial comm TX FIFO 1 */
			{											/* Used for reading the light gun in Lost World */
				switch(data >> 24)
				{
					case 0x00:
						lightgun_reg_sel = model3_serial_fifo2;
						break;
					case 0x87:
						model3_serial_fifo1 = 0;
						switch(lightgun_reg_sel)		/* read lightrun register */
						{
							case 0:		/* player 1 gun X-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(6) & 0xff;
								break;
							case 1:		/* player 1 gun X-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(6) >> 8) & 0x3;
								break;
							case 2:		/* player 1 gun Y-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(5) & 0xff;
								break;
							case 3:		/* player 1 gun Y-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(5) >> 8) & 0x3;
								break;
							case 4:		/* player 2 gun X-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(8) & 0xff;
								break;
							case 5:		/* player 2 gun X-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(8) >> 8) & 0x3;
								break;
							case 6:		/* player 2 gun Y-position, lower 8-bits */
								model3_serial_fifo2 = readinputport(7) & 0xff;
								break;
							case 7:		/* player 2 gun Y-position, upper 2-bits */
								model3_serial_fifo2 = (readinputport(7) >> 8) & 0x3;
								break;
							case 8:		/* gun offscreen (bit set = gun offscreen, bit clear = gun on screen) */
								model3_serial_fifo2 = 0;	/* bit 0 = player 1, bit 1 = player 2 */
								if(readinputport(9) & 0x1) {
									model3_serial_fifo2 |= 0x01;
								}
								break;
						}
						break;
					default:
						//printf("Lightgun: Unknown command %02X at %08X\n", (UINT32)(data >> 24), activecpu_get_pc());
						break;
				}
			}
			return;

		case 5:
			if (!(mem_mask & U64(0xff00000000000000)))	/* Serial comm TX FIFO 2 */
			{
				model3_serial_fifo2 = data >> 56;
				return;
			}
			break;

		case 7:
			if (!(mem_mask & U64(0xff000000)))	/* ADC Channel selection */
			{
				adc_channel = (data >> 24) & 0xf;
			}
			return;
	}

	osd_die("ctrl_w: %02X, %08X%08X, %08X%08X\n", offset, (UINT32)(data >> 32), (UINT32)(data), (UINT32)(mem_mask >> 32), (UINT32)(mem_mask));
}

static READ64_HANDLER( model3_sys_r )
{
	switch (offset)
	{
		case 0x08/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				return ((UINT64)model3_crom_bank << 56);
			}
			break;

		case 0x10/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				UINT64 res = model3_tap_read();

				return res<<61;
			}
			else if (!(mem_mask & 0xff000000))
			{
				return (model3_irq_enable<<24);
			}
			else logerror("m3_sys: Unk sys_r @ 0x10: mask = %x\n", (UINT32)mem_mask);
			break;
		case 0x18/8:
			return (UINT64)model3_irq_state<<56 | 0xff000000;
			break;
	}

	logerror("Unknown model3 sys_r: offs %08X mask %08X\n", offset, (UINT32)mem_mask);
	return 0;
}

static WRITE64_HANDLER( model3_sys_w )
{
	switch (offset)
	{
		case 0x10/8:
			if (!(mem_mask & 0xff000000))
			{
				model3_irq_enable = (data>>24)&0xff;
			}
			else logerror("m3_sys: unknown mask on IRQen write\n");
			break;
		case 0x08/8:
			if (!(mem_mask & U64(0xff00000000000000)))
			{
				model3_crom_bank = data >> 56;

				data >>= 56;
				data = (~data) & 7;
				cpu_setbank( 1, memory_region( REGION_USER1 ) + 0x800000 + (data * 0x800000)); /* banked CROM */
			}
			if (!(mem_mask & 0xff000000))
			{
				data >>= 24;
				model3_tap_write(
					(data >> 6) & 1,// TCK
					(data >> 2) & 1,// TMS
					(data >> 5) & 1,// TDI
					(data >> 7) & 1	// TRST
					);
			}
			break;
	}
}

static READ64_HANDLER( model3_rtc_r )
{
	UINT64 r = 0;
	if(!(mem_mask & U64(0xff00000000000000))) {
		r |= (UINT64)rtc72421_r((offset*2)+0, (UINT32)(mem_mask >> 32)) << 32;
	}
	if(!(mem_mask & U64(0x00000000ff000000))) {
		r |= (UINT64)rtc72421_r((offset*2)+1, (UINT32)(mem_mask));
	}
	return r;
}

static WRITE64_HANDLER( model3_rtc_w )
{
	if(!(mem_mask & U64(0xff00000000000000))) {
		rtc72421_w((offset*2)+0, (UINT32)(data >> 32), (UINT32)(mem_mask >> 32));
	}
	if(!(mem_mask & U64(0x00000000ff000000))) {
		rtc72421_w((offset*2)+1, (UINT32)(data), (UINT32)(mem_mask));
	}
}

static UINT64 real3d_status = 0;
static READ64_HANDLER(real3d_status_r)
{
	real3d_status ^= U64(0xffffffffffffffff);
	return real3d_status;
}

/* SCSP interface */
static READ64_HANDLER(model3_sound_r)
{
	return U64(0xffffffffffffffff);
}

static WRITE64_HANDLER(model3_sound_w)
{
	model3_irq_state &= ~0x40;
}



static UINT64 network_ram[0x10000];
static READ64_HANDLER(network_r)
{
	printf("network_r: %02X at %08X\n", offset, activecpu_get_pc());
	return network_ram[offset];
}

static WRITE64_HANDLER(network_w)
{
	COMBINE_DATA(network_ram + offset);
	printf("network_w: %02X, %08X%08X at %08X\n", offset, (UINT32)(data >> 32), (UINT32)(data), activecpu_get_pc());
}

static int prot_data_ptr = 0;

static UINT16 vs299_prot_data[] =
{
	0xc800, 0x4a20, 0x5041, 0x4e41, 0x4920, 0x4154, 0x594c, 0x4220,
	0x4152, 0x4953, 0x204c, 0x5241, 0x4547, 0x544e, 0x4e49, 0x2041,
	0x4547, 0x4d52, 0x4e41, 0x2059, 0x4e45, 0x4c47, 0x4e41, 0x2044,
	0x454e, 0x4854, 0x5245, 0x414c, 0x444e, 0x2053, 0x5246, 0x4e41,
	0x4543, 0x4320, 0x4c4f, 0x4d4f, 0x4942, 0x2041, 0x4150, 0x4152,
	0x5547, 0x5941, 0x4220, 0x4c55, 0x4147, 0x4952, 0x2041, 0x5053,
	0x4941, 0x204e, 0x5243, 0x414f, 0x4954, 0x2041, 0x4542, 0x474c,
	0x5549, 0x204d, 0x494e, 0x4547, 0x4952, 0x2041, 0x4153, 0x4455,
	0x2049, 0x4f4b, 0x4552, 0x2041, 0x4544, 0x4d4e, 0x5241, 0x204b,
	0x4f52, 0x414d, 0x494e, 0x2041, 0x4353, 0x544f, 0x414c, 0x444e,
	0x5520, 0x4153, 0x5320, 0x554f, 0x4854, 0x4641, 0x4952, 0x4143,
	0x4d20, 0x5845, 0x4349, 0x204f, 0x5559, 0x4f47, 0x4c53, 0x5641,
	0x4149, 0x4620, 0x5f43, 0x4553, 0x4147
};

static READ64_HANDLER(model3_security_r)
{
	switch(offset)
	{
		case 0x00/8:	return 0;		/* status */
		case 0x1c/8:					/* security board data read */
		{
			if (stricmp(Machine->gamedrv->name, "vs299") == 0 ||
				stricmp(Machine->gamedrv->name, "vs2v991") == 0)
			{
				return (UINT64)vs299_prot_data[prot_data_ptr++] << 48;
			}
			else
			{
				return U64(0xffffffffffffffff);
			}
		}
	}
	return U64(0xffffffffffffffff);
}

static WRITE64_HANDLER(daytona2_rombank_w)
{
	UINT8 bankh, bankl;
	if (!(mem_mask & U64(0xff00000000000000)))
	{
		data >>= 56;
		bankh = ((~data) >> 2) & 0x3;
		bankl = (~data) & 0x3;

		cpu_setbank( 1, memory_region( REGION_USER1 ) + 0x800000 + (bankh * 0x1000000) + (bankl * 0x800000));

		//bankl = (~data) & 3;
		//bankh = (~data >> 2) & 3;

		//cpu_setbank( 1, memory_region( REGION_USER1 ) + 0x800000 + (bankh * 0x1000000) + (bankl * 0x400000)); /* banked CROM */
	}
}

static ADDRESS_MAP_START( model3_mem, ADDRESS_SPACE_PROGRAM, 64)
	AM_RANGE(0x00000000, 0x007fffff) AM_RAM	AM_BASE(&work_ram)	/* work RAM */

	AM_RANGE(0x84000000, 0x8400003f) AM_READ( real3d_status_r )
	AM_RANGE(0x88000000, 0x88000007) AM_WRITE( real3d_cmd_w )
	AM_RANGE(0x8e000000, 0x8e0fffff) AM_WRITE( real3d_display_list_w )
	AM_RANGE(0x98000000, 0x980fffff) AM_WRITE( real3d_polygon_ram_w )

	AM_RANGE(0xf0040000, 0xf004003f) AM_READWRITE( model3_ctrl_r, model3_ctrl_w )
	AM_RANGE(0xf0080000, 0xf0080007) AM_READWRITE( model3_sound_r, model3_sound_w )
	AM_RANGE(0xf00c0000, 0xf00dffff) AM_RAM	AM_BASE(&model3_backup)	/* backup SRAM */
	AM_RANGE(0xf0100000, 0xf010003f) AM_READWRITE( model3_sys_r, model3_sys_w )
	AM_RANGE(0xf0140000, 0xf014003f) AM_READWRITE( model3_rtc_r, model3_rtc_w )

	AM_RANGE(0xf1000000, 0xf10f7fff) AM_READWRITE( model3_char_r, model3_char_w )	/* character RAM */
	AM_RANGE(0xf10f8000, 0xf10fffff) AM_READWRITE( model3_tile_r, model3_tile_w )	/* tilemaps */
	AM_RANGE(0xf1100000, 0xf111ffff) AM_READWRITE( model3_palette_r, model3_palette_w ) AM_BASE(&paletteram64) /* palette */
	AM_RANGE(0xf1180000, 0xf11800ff) AM_READWRITE( model3_vid_reg_r, model3_vid_reg_w )

	AM_RANGE(0xfe040000, 0xfe04003f) AM_READWRITE( model3_ctrl_r, model3_ctrl_w )
	AM_RANGE(0xfe080000, 0xfe080007) AM_READWRITE( model3_sound_r, model3_sound_w )
	AM_RANGE(0xfe0c0000, 0xfe0dffff) AM_RAM	AM_BASE(&model3_backup)	/* backup SRAM */
	AM_RANGE(0xfe100000, 0xfe10003f) AM_READWRITE( model3_sys_r, model3_sys_w )
	AM_RANGE(0xfe140000, 0xfe14003f) AM_READWRITE( model3_rtc_r, model3_rtc_w )

	AM_RANGE(0xfe180000, 0xfe19ffff) AM_RAM							/* Security Board RAM */
	AM_RANGE(0xfe1a0000, 0xfe1a003f) AM_READ( model3_security_r )	/* Security board */

	AM_RANGE(0xff800000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

#define MODEL3_SYSTEM_CONTROLS_1 \
PORT_START_TAG("IN0") \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME( DEF_STR( Service_Mode )) PORT_CODE(KEYCODE_F2) /* Test Button */ \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0xc0, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define MODEL3_SYSTEM_CONTROLS_2 \
PORT_START_TAG("IN1") \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button B") PORT_CODE(KEYCODE_8) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Button B") PORT_CODE(KEYCODE_7) \
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )


INPUT_PORTS_START( model3 )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */
INPUT_PORTS_END

INPUT_PORTS_START( lostwsga )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0xfe, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// lightgun X-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// lightgun Y-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// lightgun X-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_X ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START	// lightgun Y-axis
	PORT_BIT( 0x3ff, 0x200, IPT_LIGHTGUN_Y ) PORT_MINMAX(0x00,0x3ff) PORT_SENSITIVITY(50) PORT_KEYDELTA(10) PORT_PLAYER(2)

	PORT_START	// fake button to shoot offscreen
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( scud )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* View Button 3 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* View Button 4 */
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Shift 1 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* Shift 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 )	/* Shift 3 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON8 )	/* Shift 4 */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( bass )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)		/* Cast */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)		/* Select */
	PORT_BIT( 0xfc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START		/* Rod Y */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Rod X */
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START		/* Reel */
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Stick Y */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE_V ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START		/* Stick X */
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( harley )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* Shift down */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* Shift up */
	PORT_BIT( 0xcc, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// front brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// back brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL3 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END

INPUT_PORTS_START( daytona2 )
	MODEL3_SYSTEM_CONTROLS_1
	MODEL3_SYSTEM_CONTROLS_2

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* View Button 1 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* View Button 2 */
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )	/* View Button 3 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )	/* View Button 4 */
	PORT_BIT( 0x50, IP_ACTIVE_LOW, IPT_BUTTON5 )	/* Shift 1 */
	PORT_BIT( 0x60, IP_ACTIVE_LOW, IPT_BUTTON6 )	/* Shift 2 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON7 )	/* Shift 3 */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON8 )	/* Shift 4 */

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )		/* Dip switches */

	PORT_START	// steering
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// accelerator
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)

	PORT_START	// brake
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(30) PORT_KEYDELTA(10) PORT_PLAYER(1)
INPUT_PORTS_END




#define ROM_LOAD64_WORD(name,offset,length,hash)      ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(6))
#define ROM_LOAD64_WORD_SWAP(name,offset,length,hash) ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_REVERSE| ROM_SKIP(6))
#define ROM_LOAD_VROM(name, offset, length, hash)     ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(14) )
#define ROM_REGION64_BE(length,type,flags)	      ROM_REGION(length, type, (flags) | ROMREGION_64BIT | ROMREGION_BE)

ROM_START( scud )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "epr19731.17",  0x0600006,  0x80000,  CRC(3EE6447E) SHA1(124697791d90c1b352dd6e33bd3b45535aa92bb5) )
	ROM_LOAD64_WORD_SWAP( "epr19732.18",  0x0600004,  0x80000,  CRC(23E864BB) SHA1(0f34d963ee681ca1006f3dec12b593d961e3e442) )
	ROM_LOAD64_WORD_SWAP( "epr19733.19",  0x0600002,  0x80000,  CRC(6565E29A) SHA1(4fb4f3e77fa46a825900a63095307714e71c08f3) )
	ROM_LOAD64_WORD_SWAP( "epr19734.20",  0x0600000,  0x80000,  CRC(BE897336) SHA1(690e17f2f9a5fbe63686d197552a298efcc8c8c5) )

	// CROM0
	ROM_LOAD64_WORD_SWAP( "mpr19658.01",  0x0800006,  0x400000, CRC(d523235c) SHA1(0dbfe746b2bdc185768d82c50a329c4c58ad4a29) )
	ROM_LOAD64_WORD_SWAP( "mpr19659.02",  0x0800004,  0x400000, CRC(c47e7002) SHA1(9644694e6d117564f92650f32f94ce4d7b5523fa) )
	ROM_LOAD64_WORD_SWAP( "mpr19660.03",  0x0800002,  0x400000, CRC(d999c935) SHA1(ef5429e90314d7a789d8ccbad4d0efaeaff9741a) )
	ROM_LOAD64_WORD_SWAP( "mpr19661.04",  0x0800000,  0x400000, CRC(8e3fd241) SHA1(df2596f483c759f068c75337320d369d80189ea1) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
	ROM_LOAD64_WORD_SWAP( "mpr19662.05",  0x1800006,  0x400000, CRC(3c700eff) SHA1(2ebb149a3d8a9de95afe091b3a1776f4dc3fc579) )
	ROM_LOAD64_WORD_SWAP( "mpr19663.06",  0x1800004,  0x400000, CRC(f6af1ca4) SHA1(c78237b8f568792202d927ba0af86df6df80f87a) )
	ROM_LOAD64_WORD_SWAP( "mpr19664.07",  0x1800002,  0x400000, CRC(b9d11294) SHA1(69b6f5708f423fb11337184a3646597356554058) )
	ROM_LOAD64_WORD_SWAP( "mpr19665.08",  0x1800000,  0x400000, CRC(f97c78f9) SHA1(39aa69e365bf597e5e9185aaf4a044b485ebad8d) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr19666.09",  0x2800006,  0x400000, CRC(b53dc97f) SHA1(a4fbc7aade153e6f5fc1dd40ba97d462f643c2c4) )
	ROM_LOAD64_WORD_SWAP( "mpr19667.10",  0x2800004,  0x400000, CRC(a8676799) SHA1(78734b194e2797ac7efc40f3d0a2ff09dc93409e) )
	ROM_LOAD64_WORD_SWAP( "mpr19668.11",  0x2800002,  0x400000, CRC(0b4dd8d5) SHA1(b5668ce7ac5a4ac844a0a5a07df9649df9ad9615) )
	ROM_LOAD64_WORD_SWAP( "mpr19669.12",  0x2800000,  0x400000, CRC(cdc43c61) SHA1(b096d0eb302a9285a8ee396fdbd7b8c546049fd4) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
	ROM_LOAD_VROM( "mpr19672.26", 0x0000002,  0x200000, CRC(588c29fd) SHA1(5f58c885b506592106aa15208fc1db9d55ab4481) )
	ROM_LOAD_VROM( "mpr19673.27", 0x0000000,  0x200000, CRC(156abaa9) SHA1(6ef9c042e9ee34090192c1c99c98d19f18efcfba) )
	ROM_LOAD_VROM( "mpr19674.28", 0x0000006,  0x200000, CRC(c7b0f98c) SHA1(632dbc4cb225d91c82f6a1874517ed0b03b7a0c5) )
	ROM_LOAD_VROM( "mpr19675.29", 0x0000004,  0x200000, CRC(ff113396) SHA1(af90bb696a3c1585318150cb83ea2ed85cdb67a1) )
	ROM_LOAD_VROM( "mpr19676.30", 0x000000a,  0x200000, CRC(fd852ead) SHA1(854204c33aec8fb9c014db06e4106be37ecdaf0d) )
	ROM_LOAD_VROM( "mpr19677.31", 0x0000008,  0x200000, CRC(c6ac0347) SHA1(c792da72af8bf9d011305c9ab7a6230b9e2c5316) )
	ROM_LOAD_VROM( "mpr19678.32", 0x000000e,  0x200000, CRC(b8819cfe) SHA1(b99f8d0626bc38c75058e94d2461dbec6029589d) )
	ROM_LOAD_VROM( "mpr19679.33", 0x000000c,  0x200000, CRC(e126c3e3) SHA1(5440540c2432a9ff5bd8e36467af46c456d16844) )
	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr19680.34", 0x0000002,  0x200000, CRC(00ea5cef) SHA1(3aed46182c0e99c0b72b26c718e2fa20fa7d2e44) )
	ROM_LOAD_VROM( "mpr19681.35", 0x0000000,  0x200000, CRC(c949325f) SHA1(146de7abf764adc1840b84294cbd473f191cbcb8) )
	ROM_LOAD_VROM( "mpr19682.36", 0x0000006,  0x200000, CRC(ce5ca065) SHA1(2f518186b29e7cf5fa1c6b036427b8015cfb681e) )
	ROM_LOAD_VROM( "mpr19683.37", 0x0000004,  0x200000, CRC(e5856419) SHA1(3f8f5b8b36d417090955d34553dcf6d8d9f34558) )
	ROM_LOAD_VROM( "mpr19684.38", 0x000000a,  0x200000, CRC(56f6ec97) SHA1(dfd251dba77b39342457036fcbe4683d24029600) )
	ROM_LOAD_VROM( "mpr19685.39", 0x0000008,  0x200000, CRC(42b49304) SHA1(4e185d0f97de44a25b5f982a46f0c3d1dab406c2) )
	ROM_LOAD_VROM( "mpr19686.40", 0x000000e,  0x200000, CRC(84eed592) SHA1(cc03094770945096d81bc981bff77b540452b045) )
	ROM_LOAD_VROM( "mpr19687.41", 0x000000c,  0x200000, CRC(776ce694) SHA1(d1e56ebd0011aa3a54a5829c6bd0f5343b283fa0) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "epr19692.21", 0x080000,  0x080000,  CRC(a94f5521) SHA1(22b6a17d44fec8bf796e1790bcabc41f34c89baf) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
	ROM_LOAD32_WORD( "mpr19670.22", 0x000000, 0x400000, CRC(bd31cc06) SHA1(d1c85d0cf79b92de5bcbe20dfb8b626ad72de019) )
	ROM_LOAD32_WORD( "mpr19671.24", 0x000002, 0x400000, CRC(8e8526ab) SHA1(3d2cbb09bd185660feea4dd80bee5af2e2a19aa6) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr19612.2", 0x000000,  0x20000,  CRC(13978fd4) SHA1(bb597914a34308376239afab6e04fc231e39e379) )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr19603.57",  0x000000, 0x200000, CRC(b1b1765f) SHA1(cdcb4d6e6507322f84ac5153b386c3eb5d031e22) )
        ROM_LOAD( "mpr19604.58",  0x200000, 0x200000, CRC(6ac85b49) SHA1(3e74ae6e9ac7b208e2cd5ebdf80bb3cee19d436d) )
        ROM_LOAD( "mpr19605.59",  0x400000, 0x200000, CRC(bec891eb) SHA1(357849d2842ac77f9945eb4a0ca89253e474f617) )
        ROM_LOAD( "mpr19606.60",  0x600000, 0x200000, CRC(adad46b2) SHA1(360b23870f1d15ab527fae1bb731da6e7a8b19c1) )
ROM_END

ROM_START( scuda )	/* step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
	ROM_LOAD64_WORD_SWAP( "ep19688.17",  0x0600006,  0x80000,  CRC(a4c85103) SHA1(b2e57f86d0a49e3e88fa7d6a77bbd99039c034bb) )
	ROM_LOAD64_WORD_SWAP( "ep19689.18",  0x0600004,  0x80000,  CRC(cbce6d62) SHA1(b6051af013ee80406cfadb0c8acf24b8825ccaf2) )
	ROM_LOAD64_WORD_SWAP( "ep19690.19",  0x0600002,  0x80000,  CRC(25f007fe) SHA1(d3814637f6278c30ea277e30e66ad06e37d37e15) )
	ROM_LOAD64_WORD_SWAP( "ep19691.20",  0x0600000,  0x80000,  CRC(83523b89) SHA1(c881c71c961948c8ff3fab33e1db23cff4db0ed8) )

	// CROM0
	ROM_LOAD64_WORD_SWAP( "mpr19658.01",  0x0800006,  0x400000, CRC(d523235c) SHA1(0dbfe746b2bdc185768d82c50a329c4c58ad4a29) )
	ROM_LOAD64_WORD_SWAP( "mpr19659.02",  0x0800004,  0x400000, CRC(c47e7002) SHA1(9644694e6d117564f92650f32f94ce4d7b5523fa) )
	ROM_LOAD64_WORD_SWAP( "mpr19660.03",  0x0800002,  0x400000, CRC(d999c935) SHA1(ef5429e90314d7a789d8ccbad4d0efaeaff9741a) )
	ROM_LOAD64_WORD_SWAP( "mpr19661.04",  0x0800000,  0x400000, CRC(8e3fd241) SHA1(df2596f483c759f068c75337320d369d80189ea1) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	// CROM1
	ROM_LOAD64_WORD_SWAP( "mpr19662.05",  0x1800006,  0x400000, CRC(3c700eff) SHA1(2ebb149a3d8a9de95afe091b3a1776f4dc3fc579) )
	ROM_LOAD64_WORD_SWAP( "mpr19663.06",  0x1800004,  0x400000, CRC(f6af1ca4) SHA1(c78237b8f568792202d927ba0af86df6df80f87a) )
	ROM_LOAD64_WORD_SWAP( "mpr19664.07",  0x1800002,  0x400000, CRC(b9d11294) SHA1(69b6f5708f423fb11337184a3646597356554058) )
	ROM_LOAD64_WORD_SWAP( "mpr19665.08",  0x1800000,  0x400000, CRC(f97c78f9) SHA1(39aa69e365bf597e5e9185aaf4a044b485ebad8d) )

	// CROM2
	ROM_LOAD64_WORD_SWAP( "mpr19666.09",  0x2800006,  0x400000, CRC(b53dc97f) SHA1(a4fbc7aade153e6f5fc1dd40ba97d462f643c2c4) )
	ROM_LOAD64_WORD_SWAP( "mpr19667.10",  0x2800004,  0x400000, CRC(a8676799) SHA1(78734b194e2797ac7efc40f3d0a2ff09dc93409e) )
	ROM_LOAD64_WORD_SWAP( "mpr19668.11",  0x2800002,  0x400000, CRC(0b4dd8d5) SHA1(b5668ce7ac5a4ac844a0a5a07df9649df9ad9615) )
	ROM_LOAD64_WORD_SWAP( "mpr19669.12",  0x2800000,  0x400000, CRC(cdc43c61) SHA1(b096d0eb302a9285a8ee396fdbd7b8c546049fd4) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
	ROM_LOAD_VROM( "mpr19672.26", 0x02,  0x200000, CRC(588c29fd) SHA1(5f58c885b506592106aa15208fc1db9d55ab4481) )
	ROM_LOAD_VROM( "mpr19673.27", 0x00,  0x200000, CRC(156abaa9) SHA1(6ef9c042e9ee34090192c1c99c98d19f18efcfba) )
	ROM_LOAD_VROM( "mpr19674.28", 0x06,  0x200000, CRC(c7b0f98c) SHA1(632dbc4cb225d91c82f6a1874517ed0b03b7a0c5) )
	ROM_LOAD_VROM( "mpr19675.29", 0x04,  0x200000, CRC(ff113396) SHA1(af90bb696a3c1585318150cb83ea2ed85cdb67a1) )
	ROM_LOAD_VROM( "mpr19676.30", 0x0a,  0x200000, CRC(fd852ead) SHA1(854204c33aec8fb9c014db06e4106be37ecdaf0d) )
	ROM_LOAD_VROM( "mpr19677.31", 0x08,  0x200000, CRC(c6ac0347) SHA1(c792da72af8bf9d011305c9ab7a6230b9e2c5316) )
	ROM_LOAD_VROM( "mpr19678.32", 0x0e,  0x200000, CRC(b8819cfe) SHA1(b99f8d0626bc38c75058e94d2461dbec6029589d) )
	ROM_LOAD_VROM( "mpr19679.33", 0x0c,  0x200000, CRC(e126c3e3) SHA1(5440540c2432a9ff5bd8e36467af46c456d16844) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
	ROM_LOAD_VROM( "mpr19680.34", 0x02,  0x200000, CRC(00ea5cef) SHA1(3aed46182c0e99c0b72b26c718e2fa20fa7d2e44) )
	ROM_LOAD_VROM( "mpr19681.35", 0x00,  0x200000, CRC(c949325f) SHA1(146de7abf764adc1840b84294cbd473f191cbcb8) )
	ROM_LOAD_VROM( "mpr19682.36", 0x06,  0x200000, CRC(ce5ca065) SHA1(2f518186b29e7cf5fa1c6b036427b8015cfb681e) )
	ROM_LOAD_VROM( "mpr19683.37", 0x04,  0x200000, CRC(e5856419) SHA1(3f8f5b8b36d417090955d34553dcf6d8d9f34558) )
	ROM_LOAD_VROM( "mpr19684.38", 0x0a,  0x200000, CRC(56f6ec97) SHA1(dfd251dba77b39342457036fcbe4683d24029600) )
	ROM_LOAD_VROM( "mpr19685.39", 0x08,  0x200000, CRC(42b49304) SHA1(4e185d0f97de44a25b5f982a46f0c3d1dab406c2) )
	ROM_LOAD_VROM( "mpr19686.40", 0x0e,  0x200000, CRC(84eed592) SHA1(cc03094770945096d81bc981bff77b540452b045) )
	ROM_LOAD_VROM( "mpr19687.41", 0x0c,  0x200000, CRC(776ce694) SHA1(d1e56ebd0011aa3a54a5829c6bd0f5343b283fa0) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
	ROM_LOAD16_WORD_SWAP( "epr19692.21", 0x080000,  0x080000,  CRC(a94f5521) SHA1(22b6a17d44fec8bf796e1790bcabc41f34c89baf) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
	ROM_LOAD32_WORD( "mpr19670.22", 0x000000, 0x400000, CRC(bd31cc06) SHA1(d1c85d0cf79b92de5bcbe20dfb8b626ad72de019) )
	ROM_LOAD32_WORD( "mpr19671.24", 0x000002, 0x400000, CRC(8e8526ab) SHA1(3d2cbb09bd185660feea4dd80bee5af2e2a19aa6) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
	ROM_LOAD( "epr19612.2", 0x000000,  0x20000,  CRC(13978fd4) SHA1(bb597914a34308376239afab6e04fc231e39e379) )

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr19603.57",  0x000000, 0x200000, CRC(b1b1765f) SHA1(cdcb4d6e6507322f84ac5153b386c3eb5d031e22) )
        ROM_LOAD( "mpr19604.58",  0x200000, 0x200000, CRC(6ac85b49) SHA1(3e74ae6e9ac7b208e2cd5ebdf80bb3cee19d436d) )
        ROM_LOAD( "mpr19605.59",  0x400000, 0x200000, CRC(bec891eb) SHA1(357849d2842ac77f9945eb4a0ca89253e474f617) )
        ROM_LOAD( "mpr19606.60",  0x600000, 0x200000, CRC(adad46b2) SHA1(360b23870f1d15ab527fae1bb731da6e7a8b19c1) )
ROM_END

ROM_START( vf3 )	/* step 1.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr19227c.17",  0x600006, 0x080000, CRC(a7df4d75) SHA1(1b1186227f830556c5e2b6ca4c2bf20673b22f94) )
        ROM_LOAD64_WORD_SWAP( "epr19228c.18",  0x600004, 0x080000, CRC(9c5727e2) SHA1(f9f8b8cf27fdce08ab2975dbaa8c7a03f5c064fb) )
        ROM_LOAD64_WORD_SWAP( "epr19229c.19",  0x600002, 0x080000, CRC(731b6b78) SHA1(e39f92f721c2771f2d1f5b67625659e006f6fe0a) )
        ROM_LOAD64_WORD_SWAP( "epr19230c.20",  0x600000, 0x080000, CRC(736a9431) SHA1(0f62a122f349c0b0aab43863a30284a8fe4b7ba9) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr19193.1",    0x800006, 0x400000, CRC(7bab33d2) SHA1(243a09959f3c4311070f1de760ee63958cd47660) )
        ROM_LOAD64_WORD_SWAP( "mpr19194.2",    0x800004, 0x400000, CRC(66254702) SHA1(843ac4f6791f312f3138f8f38d38c8e4d2bab305) )
        ROM_LOAD64_WORD_SWAP( "mpr19195.3",    0x800002, 0x400000, CRC(bd5e27a3) SHA1(778c67bf7b5c7e3ae52fe12308a81b095563f52b) )
        ROM_LOAD64_WORD_SWAP( "mpr19196.4",    0x800000, 0x400000, CRC(f386b850) SHA1(168d21382359acb8f1d52d722de8c6b9a9210378) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19197.5",    0x1800006, 0x400000, CRC(a22d76c9) SHA1(ad2d67a62436ccc6479e2a218ab09d2fc22c367d) )
        ROM_LOAD64_WORD_SWAP( "mpr19198.6",    0x1800004, 0x400000, CRC(d8ee5032) SHA1(3e9274142874ace76dba2bc9b5351cfdfb3a50cd) )
        ROM_LOAD64_WORD_SWAP( "mpr19199.7",    0x1800002, 0x400000, CRC(9f80d6fe) SHA1(97b9076d413e28d00e9c45fcc7dad6f534ca8874) )
        ROM_LOAD64_WORD_SWAP( "mpr19200.8",    0x1800000, 0x400000, CRC(74941091) SHA1(914db3955f355779147d86446f5976121191ea6d) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr19201.9",    0x2800006, 0x400000, CRC(7c4a8c31) SHA1(473b7bef932d7d54a5dc06bd80d286f2e2e96d44) )
        ROM_LOAD64_WORD_SWAP( "mpr19202.10",   0x2800004, 0x400000, CRC(aaa086c6) SHA1(01871c8e5454aed80e907fde199cfb23a57aa1c2) )
        ROM_LOAD64_WORD_SWAP( "mpr19203.11",   0x2800002, 0x400000, CRC(0afa6334) SHA1(1bb70e823fb6e05df069cbfafed2e57bda8776b9) )
        ROM_LOAD64_WORD_SWAP( "mpr19204.12",   0x2800000, 0x400000, CRC(2f93310a) SHA1(3dfc5b72a78967d7772da4098adb41f18b5294d4) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr19205.13",   0x3800006, 0x400000, CRC(199c328e) SHA1(1ef1f09ff1f5253bf03e06c5b6e42be9599b9ea5) )
        ROM_LOAD64_WORD_SWAP( "mpr19206.14",   0x3800004, 0x400000, CRC(71a98d73) SHA1(dda617f9f5f986e3369fa3d3090c423eefdf913c) )
        ROM_LOAD64_WORD_SWAP( "mpr19207.15",   0x3800002, 0x400000, CRC(2ce1612d) SHA1(736f559d460f0069c7a2d5ba7cddf9135737d6e2) )
        ROM_LOAD64_WORD_SWAP( "mpr19208.16",   0x3800000, 0x400000, CRC(08f30f71) SHA1(393525b19cdecddfbd62c6209203db5f3edfd9a8) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19211.26",   0x000002, 0x200000, CRC(9c8f5df1) SHA1(d47c8bd0189c8e617a3ed9f75ee3812a229f56c0) )
        ROM_LOAD_VROM( "mpr19212.27",   0x000000, 0x200000, CRC(75036234) SHA1(01a20a6a62408017bff8f2e76dbd21c00275bc70) )
        ROM_LOAD_VROM( "mpr19213.28",   0x000006, 0x200000, CRC(67b123cf) SHA1(b84c4f83c25edcc8ac929d3f9cf51da713045071) )
        ROM_LOAD_VROM( "mpr19214.29",   0x000004, 0x200000, CRC(a6f5576b) SHA1(e994b3ef8e6eb07e8f3bbe474410c06d6c42354b) )
        ROM_LOAD_VROM( "mpr19215.30",   0x00000a, 0x200000, CRC(c6fd9f0d) SHA1(1f3299706d6ac73836c069a7ed2866d412f60369) )
        ROM_LOAD_VROM( "mpr19216.31",   0x000008, 0x200000, CRC(201bb1ed) SHA1(7ffd72ff56159529d74f01f8da0ba4798f109806) )
        ROM_LOAD_VROM( "mpr19217.32",   0x00000e, 0x200000, CRC(4dadd41a) SHA1(7a1e0908962afcfc737132478c0e45d153d94ecb) )
        ROM_LOAD_VROM( "mpr19218.33",   0x00000c, 0x200000, CRC(cff91953) SHA1(41e95704a65958377c3bbd9d00d90a5ad4552f66) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19219.34",   0x000002, 0x200000, CRC(c610d521) SHA1(cb146fe78d89176e9dd5c773644614cdc2ef57ce) )
        ROM_LOAD_VROM( "mpr19220.35",   0x000000, 0x200000, CRC(e62924d0) SHA1(4d1ac11a5977a4e9cf942c9f1204960c0a895347) )
        ROM_LOAD_VROM( "mpr19221.36",   0x000006, 0x200000, CRC(24f83e3c) SHA1(c587428fa47e849881bf45487af086db6b09264e) )
        ROM_LOAD_VROM( "mpr19222.37",   0x000004, 0x200000, CRC(61a6aa7d) SHA1(cc26020b2f904f68822111073b595ee0cc8b2e0c) )
        ROM_LOAD_VROM( "mpr19223.38",   0x00000a, 0x200000, CRC(1a8c1980) SHA1(43b8efb019c8a20fe38f95050fe60dfe9bf322f0) )
        ROM_LOAD_VROM( "mpr19224.39",   0x000008, 0x200000, CRC(0a79a1bd) SHA1(1df71cf77ea8611462380a449eb99199664b3da3) )
        ROM_LOAD_VROM( "mpr19225.40",   0x00000e, 0x200000, CRC(91a985eb) SHA1(5a842a260e4a78f5463222db44f13b068fa70b23) )
        ROM_LOAD_VROM( "mpr19226.41",   0x00000c, 0x200000, CRC(00091722) SHA1(ef86db36b4b91a66b3e401c3c91735b9d28da2e2) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr19231.21",   0x000000, 0x020000, CRC(f2ae2558) SHA1(7ab5fc508ffd1b0c9f3e70f4d3edeae44e3a0dc4) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr19209.22",   0x000000, 0x400000, CRC(3715e38c) SHA1(b11dbf8a5840990e9697c53b4796cd70ad91f6a1) )
        ROM_LOAD( "mpr19210.24",   0x400000, 0x400000, CRC(c03d6502) SHA1(4ca49fe5dd5105ca5f78f4740477beb64137d4be) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vf3tb )	/* step 1.0? */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20126.17",        0x600006, 0x080000, CRC(27ecd3b0) SHA1(a9b913294ac925adb501d3b47f346006b70dfcd6) )
        ROM_LOAD64_WORD_SWAP( "epr20127.18",        0x600004, 0x080000, CRC(5c0f694b) SHA1(ca346d6b249bb7a3015f016d25bfb3050360c8ec) )
        ROM_LOAD64_WORD_SWAP( "epr20128.19",        0x600002, 0x080000, CRC(ffbdbdc5) SHA1(3cbcc5ce3fcb563f11dc87ac514de2325c6cc9f2) )
        ROM_LOAD64_WORD_SWAP( "epr20129.20",        0x600000, 0x080000, CRC(0db897ce) SHA1(68f5005082c69fab254d43485669dd6b95a6cc9b) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20130.1",        0x800006, 0x200000, CRC(19e1eaca) SHA1(e63afe7d8f5e653d0efd026fe20da0850f908d5e) )
        ROM_LOAD64_WORD_SWAP( "mpr20131.2",        0x800004, 0x200000, CRC(d86ec71b) SHA1(da8ffb1a8000b8e656893c29dad8458e04c91df6) )
        ROM_LOAD64_WORD_SWAP( "mpr20132.3",        0x800002, 0x200000, CRC(91ebc0fb) SHA1(0c37d29c15ec7137dd5398c010a289ffce9ee1e8) )
        ROM_LOAD64_WORD_SWAP( "mpr20133.4",        0x800000, 0x200000, CRC(d9a10a89) SHA1(a1c1dff2cde54cd128690bb5896448786e5243b9) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19197.5",    0x1800006, 0x400000, CRC(a22d76c9) SHA1(ad2d67a62436ccc6479e2a218ab09d2fc22c367d) )
        ROM_LOAD64_WORD_SWAP( "mpr19198.6",    0x1800004, 0x400000, CRC(d8ee5032) SHA1(3e9274142874ace76dba2bc9b5351cfdfb3a50cd) )
        ROM_LOAD64_WORD_SWAP( "mpr19199.7",    0x1800002, 0x400000, CRC(9f80d6fe) SHA1(97b9076d413e28d00e9c45fcc7dad6f534ca8874) )
        ROM_LOAD64_WORD_SWAP( "mpr19200.8",    0x1800000, 0x400000, CRC(74941091) SHA1(914db3955f355779147d86446f5976121191ea6d) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr19201.9",    0x2800006, 0x400000, CRC(7c4a8c31) SHA1(473b7bef932d7d54a5dc06bd80d286f2e2e96d44) )
        ROM_LOAD64_WORD_SWAP( "mpr19202.10",   0x2800004, 0x400000, CRC(aaa086c6) SHA1(01871c8e5454aed80e907fde199cfb23a57aa1c2) )
        ROM_LOAD64_WORD_SWAP( "mpr19203.11",   0x2800002, 0x400000, CRC(0afa6334) SHA1(1bb70e823fb6e05df069cbfafed2e57bda8776b9) )
        ROM_LOAD64_WORD_SWAP( "mpr19204.12",   0x2800000, 0x400000, CRC(2f93310a) SHA1(3dfc5b72a78967d7772da4098adb41f18b5294d4) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr19205.13",   0x3800006, 0x400000, CRC(199c328e) SHA1(1ef1f09ff1f5253bf03e06c5b6e42be9599b9ea5) )
        ROM_LOAD64_WORD_SWAP( "mpr19206.14",   0x3800004, 0x400000, CRC(71a98d73) SHA1(dda617f9f5f986e3369fa3d3090c423eefdf913c) )
        ROM_LOAD64_WORD_SWAP( "mpr19207.15",   0x3800002, 0x400000, CRC(2ce1612d) SHA1(736f559d460f0069c7a2d5ba7cddf9135737d6e2) )
        ROM_LOAD64_WORD_SWAP( "mpr19208.16",   0x3800000, 0x400000, CRC(08f30f71) SHA1(393525b19cdecddfbd62c6209203db5f3edfd9a8) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19211.26",   0x000002, 0x200000, CRC(9c8f5df1) SHA1(d47c8bd0189c8e617a3ed9f75ee3812a229f56c0) )
        ROM_LOAD_VROM( "mpr19212.27",   0x000000, 0x200000, CRC(75036234) SHA1(01a20a6a62408017bff8f2e76dbd21c00275bc70) )
        ROM_LOAD_VROM( "mpr19213.28",   0x000006, 0x200000, CRC(67b123cf) SHA1(b84c4f83c25edcc8ac929d3f9cf51da713045071) )
        ROM_LOAD_VROM( "mpr19214.29",   0x000004, 0x200000, CRC(a6f5576b) SHA1(e994b3ef8e6eb07e8f3bbe474410c06d6c42354b) )
        ROM_LOAD_VROM( "mpr19215.30",   0x00000a, 0x200000, CRC(c6fd9f0d) SHA1(1f3299706d6ac73836c069a7ed2866d412f60369) )
        ROM_LOAD_VROM( "mpr19216.31",   0x000008, 0x200000, CRC(201bb1ed) SHA1(7ffd72ff56159529d74f01f8da0ba4798f109806) )
        ROM_LOAD_VROM( "mpr19217.32",   0x00000e, 0x200000, CRC(4dadd41a) SHA1(7a1e0908962afcfc737132478c0e45d153d94ecb) )
        ROM_LOAD_VROM( "mpr19218.33",   0x00000c, 0x200000, CRC(cff91953) SHA1(41e95704a65958377c3bbd9d00d90a5ad4552f66) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19219.34",   0x000002, 0x200000, CRC(c610d521) SHA1(cb146fe78d89176e9dd5c773644614cdc2ef57ce) )
        ROM_LOAD_VROM( "mpr19220.35",   0x000000, 0x200000, CRC(e62924d0) SHA1(4d1ac11a5977a4e9cf942c9f1204960c0a895347) )
        ROM_LOAD_VROM( "mpr19221.36",   0x000006, 0x200000, CRC(24f83e3c) SHA1(c587428fa47e849881bf45487af086db6b09264e) )
        ROM_LOAD_VROM( "mpr19222.37",   0x000004, 0x200000, CRC(61a6aa7d) SHA1(cc26020b2f904f68822111073b595ee0cc8b2e0c) )
        ROM_LOAD_VROM( "mpr19223.38",   0x00000a, 0x200000, CRC(1a8c1980) SHA1(43b8efb019c8a20fe38f95050fe60dfe9bf322f0) )
        ROM_LOAD_VROM( "mpr19224.39",   0x000008, 0x200000, CRC(0a79a1bd) SHA1(1df71cf77ea8611462380a449eb99199664b3da3) )
        ROM_LOAD_VROM( "mpr19225.40",   0x00000e, 0x200000, CRC(91a985eb) SHA1(5a842a260e4a78f5463222db44f13b068fa70b23) )
        ROM_LOAD_VROM( "mpr19226.41",   0x00000c, 0x200000, CRC(00091722) SHA1(ef86db36b4b91a66b3e401c3c91735b9d28da2e2) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr19231.21",   0x000000, 0x020000, CRC(f2ae2558) SHA1(7ab5fc508ffd1b0c9f3e70f4d3edeae44e3a0dc4) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr19209.22",   0x000000, 0x400000, CRC(3715e38c) SHA1(b11dbf8a5840990e9697c53b4796cd70ad91f6a1) )
        ROM_LOAD( "mpr19210.24",   0x400000, 0x400000, CRC(c03d6502) SHA1(4ca49fe5dd5105ca5f78f4740477beb64137d4be) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( bass )	/* step 1.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20643.17",  0x600006, 0x080000, CRC(daf02716) SHA1(b968f8ca602c78b9ca49969ff01f9440f175049a) )
        ROM_LOAD64_WORD_SWAP( "epr20644.18",  0x600004, 0x080000, CRC(c28db2b6) SHA1(0b12fe9e5189714b1aca79c4bba4be57a9e0d5fd) )
        ROM_LOAD64_WORD_SWAP( "epr20645.19",  0x600002, 0x080000, CRC(8eefa2b0) SHA1(130ce2a58daa2eba6e56e1f49488017ab871bb7d) )
        ROM_LOAD64_WORD_SWAP( "epr20646.20",  0x600000, 0x080000, CRC(d740ae06) SHA1(f6c00b9f3f8cf67c8bd8fff73c64bc0c14d364c4) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20256.1",   0x800006, 0x400000, CRC(115302ac) SHA1(45f60aa9f91c9a5821a14647e5ac4d53caf71d5f) )
        ROM_LOAD64_WORD_SWAP( "mpr20257.2",   0x800004, 0x400000, CRC(025bc06d) SHA1(e774021d8d884871e840100ba6f4c16299233a51) )
        ROM_LOAD64_WORD_SWAP( "mpr20258.3",   0x800002, 0x400000, CRC(7b78b071) SHA1(f2b29a1238c9eae0a7a68c91a9728ac31f05ef7d) )
        ROM_LOAD64_WORD_SWAP( "mpr20259.4",   0x800000, 0x400000, CRC(40052562) SHA1(2361fd299b76b1c0d112f1fed85bde16e1564382) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20260.5",   0x1800006, 0x400000, CRC(c56b4c10) SHA1(a0a81d4f05df5b8584c2dca53993c01a35d38812) )
        ROM_LOAD64_WORD_SWAP( "mpr20261.6",   0x1800004, 0x400000, CRC(b1e9d44a) SHA1(dfe8c5ed848afd48040775bb5a440c590188272c) )
        ROM_LOAD64_WORD_SWAP( "mpr20262.7",   0x1800002, 0x400000, CRC(52b0674d) SHA1(c9f817b46dc7fcd04dfc2bbc4b1d82b1f41fe258) )
        ROM_LOAD64_WORD_SWAP( "mpr20263.8",   0x1800000, 0x400000, CRC(1cf4cba9) SHA1(2884bb00990ab4bdad0d524937123c2936523cbb) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20264.9",   0x2800006, 0x400000, CRC(8d995196) SHA1(ff410d80353956ee8e770d7b9e9dabac87ee76cc) )
        ROM_LOAD64_WORD_SWAP( "mpr20265.10",  0x2800004, 0x400000, CRC(28f76e3e) SHA1(5446e0a0df60d77112bd71c726291fdbba7df284) )
        ROM_LOAD64_WORD_SWAP( "mpr20266.11",  0x2800002, 0x400000, CRC(abd2db85) SHA1(ebe752071562c532e6ad494f285ff4d0b5050611) )
        ROM_LOAD64_WORD_SWAP( "mpr20267.12",  0x2800000, 0x400000, CRC(48989191) SHA1(ddbf787ec5dae298ab29847f117eae2ce1ff935e) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr20270.26",  0x000002, 0x200000, CRC(df68a7a7) SHA1(5d610962dd87c010a094fe2ce8d13408595b4ae4) )
        ROM_LOAD_VROM( "mpr20271.27",  0x000000, 0x200000, CRC(4b01c3a4) SHA1(8d47109e7f410c9d34d57b22adfe1c3092e70074) )
        ROM_LOAD_VROM( "mpr20272.28",  0x000006, 0x200000, CRC(a658da23) SHA1(b96270c64cf75625960fa7c03411af595880353f) )
        ROM_LOAD_VROM( "mpr20273.29",  0x000004, 0x200000, CRC(577e9ffa) SHA1(b004fa10a073e6f4715b417da817051752db5636) )
        ROM_LOAD_VROM( "mpr20274.30",  0x00000a, 0x200000, CRC(7c7056ae) SHA1(79f6e0ac65f9e80875946b2e73cf9437ecf73407) )
        ROM_LOAD_VROM( "mpr20275.31",  0x000008, 0x200000, CRC(e739f77a) SHA1(6547c4bc0925af6e07beab54377a174a9c17e9fa) )
        ROM_LOAD_VROM( "mpr20276.32",  0x00000e, 0x200000, CRC(cbf966c0) SHA1(7c63506d01b52c8ab86fe0dc9ac774e2d540f7c5) )
        ROM_LOAD_VROM( "mpr20277.33",  0x00000c, 0x200000, CRC(9c75200b) SHA1(b39f571eeab11a619ab964d78a2ba0aa7b1dd24f) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr20278.34",  0x000002, 0x200000, CRC(db3991ba) SHA1(7854a047741042f087a146882e27f2624f1f9e98) )
        ROM_LOAD_VROM( "mpr20279.35",  0x000000, 0x200000, CRC(995a11b8) SHA1(eddd32fc3688d12458c5ffb3b3e70459947889a2) )
        ROM_LOAD_VROM( "mpr20280.36",  0x000006, 0x200000, CRC(c2c8f9f5) SHA1(dd30c1fbece0a3dc8dad2d9d87e58a9f3798f4a2) )
        ROM_LOAD_VROM( "mpr20281.37",  0x000004, 0x200000, CRC(da84b967) SHA1(dfc13942adc9cf438e70470cb17f4d1f846c4c1a) )
        ROM_LOAD_VROM( "mpr20282.38",  0x00000a, 0x200000, CRC(1869ff49) SHA1(1368123edfd9c93d1ee591bf40ea110deeac88cf) )
        ROM_LOAD_VROM( "mpr20283.39",  0x000008, 0x200000, CRC(7d8fb469) SHA1(ad95fec786e9181d91a6ea18808bbf2772e9be6a) )
        ROM_LOAD_VROM( "mpr20284.40",  0x00000e, 0x200000, CRC(5c7f3a6f) SHA1(d242bc7ad213a79203cd6a060229c356ec0867e7) )
        ROM_LOAD_VROM( "mpr20285.41",  0x00000c, 0x200000, CRC(4aadc573) SHA1(65aef06c8c48196a0c1f630529ae2248323c5747) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20313.21",  0x000000, 0x080000, CRC(863a7857) SHA1(72384dc6d7613806ab6bb84d935a3b0497e9e9d2) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20268.22",  0x000000, 0x400000, CRC(3631e93e) SHA1(3991d6cf03e4f39733d467c483857eac874505d1) )
        ROM_LOAD( "mpr20269.24",  0x400000, 0x400000, CRC(105a3181) SHA1(022cbce1d01366461a584ff6225ded40bcb9000b) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( lostwsga )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr19939.17",     0x600006, 0x080000, CRC(8788b939) SHA1(30932057f763545568526f85977aa0afc4b66e7d) )
        ROM_LOAD64_WORD_SWAP( "epr19938.18",     0x600004, 0x080000, CRC(38afe27a) SHA1(718a238ee246eeed9fa698b58493806932d0e7cb) )
        ROM_LOAD64_WORD_SWAP( "epr19937.19",     0x600002, 0x080000, CRC(9dbf5712) SHA1(ca5923bb5a0b7702391dcacc20e863a7f615929d) )
        ROM_LOAD64_WORD_SWAP( "epr19936.20",     0x600000, 0x080000, CRC(2f1ca664) SHA1(9138e28ad6c0219b4b3a4609136ed69f484de9a3) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr19918.1",     0x800006, 0x400000, CRC(95b690e9) SHA1(50b71aa41372b9acda1db2e0b7ac70707a5cca4b) )
        ROM_LOAD64_WORD_SWAP( "mpr19919.2",     0x800004, 0x400000, CRC(ff119949) SHA1(2f2648c2eeb8be2838188c5ce65a932c5e6803a6) )
        ROM_LOAD64_WORD_SWAP( "mpr19920.3",     0x800002, 0x400000, CRC(8df33574) SHA1(671cfee1e5f304a480d8f4a9d44d0315b6839f99) )
        ROM_LOAD64_WORD_SWAP( "mpr19921.4",     0x800000, 0x400000, CRC(9af3227f) SHA1(f1232ee486b974409f25d94ade1b1f50258cc609) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19922.5",     0x1800006, 0x400000, CRC(4dfd7fc6) SHA1(7fe7445dbf2c1a0b03c1aec40c4e27b391178472) )
        ROM_LOAD64_WORD_SWAP( "mpr19923.6",     0x1800004, 0x400000, CRC(ed515cb2) SHA1(7604f7bd277a2fc0855effe24b03702878076c13) )
        ROM_LOAD64_WORD_SWAP( "mpr19924.7",     0x1800002, 0x400000, CRC(4ee3ddc5) SHA1(37b1ca13a7442ae2dedd0ced6c222846c3690984) )
        ROM_LOAD64_WORD_SWAP( "mpr19925.8",     0x1800000, 0x400000, CRC(cfa4bb49) SHA1(2584fe57f0e117ff64eb34bc4bb782912a379bd3) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr19926.9",     0x2800006, 0x400000, CRC(05a232e0) SHA1(712db7664be2efb6c93181194316a7436c40e638) )
        ROM_LOAD64_WORD_SWAP( "mpr19927.10",     0x2800004, 0x400000, CRC(0c96ef11) SHA1(789878204f3b07def187a8e6c471530021a8504a) )
        ROM_LOAD64_WORD_SWAP( "mpr19928.11",     0x2800002, 0x400000, CRC(9afd5d4a) SHA1(e81982b7ca21a4a76d3ca5e307a88ce3c8063a6c) )
        ROM_LOAD64_WORD_SWAP( "mpr19929.12",     0x2800000, 0x400000, CRC(16491f63) SHA1(3ce750db13c6af5696dc14868487334cc7eb9276) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr19930.13",     0x3800006, 0x400000, CRC(b598c2f2) SHA1(bd9f3b729bec539a9f9b3e021efa9f4248337456) )
        ROM_LOAD64_WORD_SWAP( "mpr19931.14",     0x3800004, 0x400000, CRC(448a5007) SHA1(d842d5552867026a1f5d6d22aa5c08e2209f5487) )
        ROM_LOAD64_WORD_SWAP( "mpr19932.15",     0x3800002, 0x400000, CRC(04389385) SHA1(e645159672c6edbaab5bb3eda3bfbac98bd36210) )
        ROM_LOAD64_WORD_SWAP( "mpr19933.16",     0x3800000, 0x400000, CRC(8e2acd3b) SHA1(9a87086c06d3d22ade96d6057709008663aa3cfa) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19902.26",     0x02, 0x200000, CRC(178bd471) SHA1(dc2cb409081e4fd1176470869e025320449a8d02) )
        ROM_LOAD_VROM( "mpr19903.27",     0x00, 0x200000, CRC(fe575871) SHA1(db7aec4997b0c9d9a77a611139d53bcfba4bf258) )
        ROM_LOAD_VROM( "mpr19904.28",     0x06, 0x200000, CRC(57971d7d) SHA1(710705d53e499c5cec6374438d8393a31277f8b7) )
        ROM_LOAD_VROM( "mpr19905.29",     0x04, 0x200000, CRC(6fa122ee) SHA1(d3e373e7c3f72cee0658820848993c5fd0d4752d) )
        ROM_LOAD_VROM( "mpr19906.30",     0x0a, 0x200000, CRC(a5b16dd9) SHA1(956a3dbbb101effe92c7a9be3207b9882cf09882) )
        ROM_LOAD_VROM( "mpr19907.31",     0x08, 0x200000, CRC(84a425cd) SHA1(156541d9cacc0c57ac8d4e60f5ed85a87c7608e7) )
        ROM_LOAD_VROM( "mpr19908.32",     0x0e, 0x200000, CRC(7702aa7c) SHA1(0ef4f56c95a6779a14b7df7c4e7b83bd219cb67d) )
        ROM_LOAD_VROM( "mpr19909.33",     0x0c, 0x200000, CRC(8fca65f9) SHA1(17350103ddf0a2efdcde1c1f17d28800334d723f) )
	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
		ROM_LOAD_VROM( "mpr19910.34",     0x02, 0x200000, CRC(1ef585e2) SHA1(69583635a52aace7986dd8e8139482d685547b7a) )
        ROM_LOAD_VROM( "mpr19911.35",     0x00, 0x200000, CRC(ca26a48d) SHA1(407434d74cb205211261e08bb633f5fc1863e495) )
        ROM_LOAD_VROM( "mpr19912.36",     0x06, 0x200000, CRC(ffe000e0) SHA1(d5a2fe8a6ddd5efb934af9f369b24e4508be3143) )
        ROM_LOAD_VROM( "mpr19913.37",     0x04, 0x200000, CRC(c003049e) SHA1(d3e26531fac33e36c01cdbc0d66f41b918af4c4d) )
        ROM_LOAD_VROM( "mpr19914.38",     0x0a, 0x200000, CRC(3c21a953) SHA1(1968ba68298e9e73840aa8737dd6c7ad7220cff0) )
        ROM_LOAD_VROM( "mpr19915.39",     0x08, 0x200000, CRC(fd0f2a2b) SHA1(f47bcbc0a4564682578dde454b4f42ca1a8c6b87) )
        ROM_LOAD_VROM( "mpr19916.40",     0x0e, 0x200000, CRC(10b0c52e) SHA1(1076352f9a0484815a4f14e66485337a6d5b565e) )
        ROM_LOAD_VROM( "mpr19917.41",     0x0c, 0x200000, CRC(3035833b) SHA1(e55a225aa1268bcfcc3381d48fc7aaf75f6e1839) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr19940.21",     0x080000, 0x080000, CRC(b06ffe5f) SHA1(1b49c2fbc3f188168828daf7f7f56a04c394e832) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr19934.22",     0x000000, 0x200000, CRC(661b09d0) SHA1(cc4a30aa9fe17964808a7ae967de8f64da36379a) )
        ROM_LOAD( "mpr19935.24",     0x200000, 0x200000, CRC(6436e483) SHA1(f81b846e7898ec0172b50d74bb7f9062b81f38a8) )
        ROM_LOAD( "mpr19934.22",     0x400000, 0x200000, CRC(661b09d0) SHA1(cc4a30aa9fe17964808a7ae967de8f64da36379a) )
        ROM_LOAD( "mpr19935.24",     0x600000, 0x200000, CRC(6436e483) SHA1(f81b846e7898ec0172b50d74bb7f9062b81f38a8) )


	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20467.17",   0x400006, 0x100000, CRC(25d7ae73) SHA1(433a7c1dac1bd5524b018da2ed09f937d527ac3e) )
        ROM_LOAD64_WORD_SWAP( "epr20468.18",   0x400004, 0x100000, CRC(f0f0b6ea) SHA1(b3f545e5a4dd45b97df938093251cc7845c2a1f9) )
        ROM_LOAD64_WORD_SWAP( "epr20469.19",   0x400002, 0x100000, CRC(9d7521f6) SHA1(9efcb5e6a9add4331c5ea60998afce792b8d6623) )
        ROM_LOAD64_WORD_SWAP( "epr20470.20",   0x400000, 0x100000, CRC(2f62b292) SHA1(b38db0d7b690e7f9344adf7ee36037196bb521d2) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr19769.1", 0x800006, 0x400000, CRC(dc020031) SHA1(35eba49a237c1c647dbf13024e664e2cb09f38b5) )
        ROM_LOAD64_WORD_SWAP( "mpr19770.2", 0x800004, 0x400000, CRC(91f690b0) SHA1(e70482b255bdc37def897842313c2cb592dd3c6c) )
        ROM_LOAD64_WORD_SWAP( "mpr19771.3", 0x800002, 0x400000, CRC(189c510f) SHA1(9ebe3c4d98fc57744104608feef7c4e00c0dfd15) )
        ROM_LOAD64_WORD_SWAP( "mpr19772.4", 0x800000, 0x400000, CRC(6db7b9d0) SHA1(098daad081c8e3ab5dc88e5a8d453b82101c5fc4) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19773.5", 0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr19774.6", 0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr19775.7", 0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr19776.8", 0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr19777.9", 0x2800006, 0x400000, CRC(c8f216a6) SHA1(0ec700ef15094f6746bbc886e7045329ccebb5d1) )
        ROM_LOAD64_WORD_SWAP( "mpr19778.10", 0x2800004, 0x400000, CRC(2192b189) SHA1(63f81ab0bd099ef77470cc19fd6d218de72d7876) )
        ROM_LOAD64_WORD_SWAP( "mpr19779.11", 0x2800002, 0x400000, CRC(2242b21b) SHA1(b5834c19a0a54fe38cac22cfbc2a1e14543aee9d) )
        ROM_LOAD64_WORD_SWAP( "mpr19780.12", 0x2800000, 0x400000, CRC(38508791) SHA1(0e6d629a6e4d11368a367d74f44bc805805f4365) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr19781.13", 0x3800006, 0x400000, CRC(783213f4) SHA1(042c0c0d8604d9d73f581064b0b31234ec7b81b2) )
        ROM_LOAD64_WORD_SWAP( "mpr19782.14", 0x3800004, 0x400000, CRC(43b43eef) SHA1(8a8b40581a13f56ab2da75f049ba3101a4d3adb4) )
        ROM_LOAD64_WORD_SWAP( "mpr19783.15", 0x3800002, 0x400000, CRC(47c3d726) SHA1(99ca84b9318c45721ccc5053a909b6ef67a6671c) )
        ROM_LOAD64_WORD_SWAP( "mpr19784.16", 0x3800000, 0x400000, CRC(a1cc70be) SHA1(c0bb9e78ae9a45c03945b9bc647da129f6171812) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr19807.21", 0x000000, 0x080000, CRC(9641cbaf) SHA1(aaffde7678b40bc940be04fb107efc4d0d416ea1) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr19785.22", 0x000000, 0x400000, CRC(e7d190e3) SHA1(f263af149e303429f469a3ab601b87461256aaa7) )
        ROM_LOAD( "mpr19786.24", 0x400000, 0x400000, CRC(b08d889b) SHA1(790b5b2d62a28c39d43aeec9ffb365ccd9dc93af) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs215 )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr19900.17", 0x600006, 0x080000, CRC(8fb6045d) SHA1(88497eafc23ba70ab4a43de552a16caccd8dccbe) )
        ROM_LOAD64_WORD_SWAP( "epr19899.18", 0x600004, 0x080000, CRC(8cc2be9f) SHA1(ec82b1312c8d58adb200f4d7f6f9a9c8214415d5) )
        ROM_LOAD64_WORD_SWAP( "epr19898.19", 0x600002, 0x080000, CRC(4389d9ce) SHA1(a5f412417484fdd70dc3dfb2f0cb5554ed4fc7f3) )
        ROM_LOAD64_WORD_SWAP( "epr19897.20", 0x600000, 0x080000, CRC(25a722a9) SHA1(190b7d002009d91ac4521d180f7519cbeb49da30) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr19769.1", 0x800006, 0x400000, CRC(dc020031) SHA1(35eba49a237c1c647dbf13024e664e2cb09f38b5) )
        ROM_LOAD64_WORD_SWAP( "mpr19770.2", 0x800004, 0x400000, CRC(91f690b0) SHA1(e70482b255bdc37def897842313c2cb592dd3c6c) )
        ROM_LOAD64_WORD_SWAP( "mpr19771.3", 0x800002, 0x400000, CRC(189c510f) SHA1(9ebe3c4d98fc57744104608feef7c4e00c0dfd15) )
        ROM_LOAD64_WORD_SWAP( "mpr19772.4", 0x800000, 0x400000, CRC(6db7b9d0) SHA1(098daad081c8e3ab5dc88e5a8d453b82101c5fc4) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19773.5", 0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr19774.6", 0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr19775.7", 0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr19776.8", 0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr19777.9", 0x2800006, 0x400000, CRC(c8f216a6) SHA1(0ec700ef15094f6746bbc886e7045329ccebb5d1) )
        ROM_LOAD64_WORD_SWAP( "mpr19778.10", 0x2800004, 0x400000, CRC(2192b189) SHA1(63f81ab0bd099ef77470cc19fd6d218de72d7876) )
        ROM_LOAD64_WORD_SWAP( "mpr19779.11", 0x2800002, 0x400000, CRC(2242b21b) SHA1(b5834c19a0a54fe38cac22cfbc2a1e14543aee9d) )
        ROM_LOAD64_WORD_SWAP( "mpr19780.12", 0x2800000, 0x400000, CRC(38508791) SHA1(0e6d629a6e4d11368a367d74f44bc805805f4365) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr19781.13", 0x3800006, 0x400000, CRC(783213f4) SHA1(042c0c0d8604d9d73f581064b0b31234ec7b81b2) )
        ROM_LOAD64_WORD_SWAP( "mpr19782.14", 0x3800004, 0x400000, CRC(43b43eef) SHA1(8a8b40581a13f56ab2da75f049ba3101a4d3adb4) )
        ROM_LOAD64_WORD_SWAP( "mpr19783.15", 0x3800002, 0x400000, CRC(47c3d726) SHA1(99ca84b9318c45721ccc5053a909b6ef67a6671c) )
        ROM_LOAD64_WORD_SWAP( "mpr19784.16", 0x3800000, 0x400000, CRC(a1cc70be) SHA1(c0bb9e78ae9a45c03945b9bc647da129f6171812) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr19807.21", 0x000000, 0x080000, CRC(9641cbaf) SHA1(aaffde7678b40bc940be04fb107efc4d0d416ea1) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr19785.22", 0x000000, 0x400000, CRC(e7d190e3) SHA1(f263af149e303429f469a3ab601b87461256aaa7) )
        ROM_LOAD( "mpr19786.24", 0x400000, 0x400000, CRC(b08d889b) SHA1(790b5b2d62a28c39d43aeec9ffb365ccd9dc93af) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs298 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20917.17", 0x400006, 0x100000, CRC(c3bbb270) SHA1(16b2342031ff72408f2290e775df5c8aa344c2e4) )
        ROM_LOAD64_WORD_SWAP( "epr20918.18", 0x400004, 0x100000, CRC(0e9cdc5b) SHA1(356816d0380c791b9d812ce17fa95123d15bb5e9) )
        ROM_LOAD64_WORD_SWAP( "epr20919.19", 0x400002, 0x100000, CRC(7a0713d2) SHA1(595f962ae852e48fb24aa08d0b8603692acfb1b9) )
        ROM_LOAD64_WORD_SWAP( "epr20920.20", 0x400000, 0x100000, CRC(428d05fc) SHA1(451e78c7b381e7d84dbac2a3d68ebbd6f1490bad) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20891.1", 0x800006, 0x400000, CRC(9ecb0b39) SHA1(97b2ba2a863b7559923efff315aab04f7dca33b0) )
        ROM_LOAD64_WORD_SWAP( "mpr20892.2", 0x800004, 0x400000, CRC(8e5d3fe7) SHA1(5fe7ad8577ce46fdd2ea741eb2a98028eee61a82) )
        ROM_LOAD64_WORD_SWAP( "mpr20893.3", 0x800002, 0x400000, CRC(5c83dcaa) SHA1(2d4794bc6c3bfd4913ee045692b6aec5680825e0) )
        ROM_LOAD64_WORD_SWAP( "mpr20894.4", 0x800000, 0x400000, CRC(09c065cc) SHA1(9a47c6fd45630549066f58b6872ad885908c6e38) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19773.5", 0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr19774.6", 0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr19775.7", 0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr19776.8", 0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20895.9",  0x2800006, 0x400000, CRC(9b51cbf5) SHA1(670cfee991b997d4f7c3d51c48dad1ee032f93ef) )
        ROM_LOAD64_WORD_SWAP( "mpr20896.10", 0x2800004, 0x400000, CRC(bf1cbd5e) SHA1(a677247d7c94f8d36ffece7824026047db1188e1) )
        ROM_LOAD64_WORD_SWAP( "mpr20897.11", 0x2800002, 0x400000, CRC(c5cf067a) SHA1(ee9503bee3af238434590f439c87219fe45c91b9) )
        ROM_LOAD64_WORD_SWAP( "mpr20898.12", 0x2800000, 0x400000, CRC(94040d37) SHA1(4a35a0e1cdfdf18a8b01ea11ae88815bf8e92ff3) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr20899.13", 0x3800006, 0x400000, CRC(65422425) SHA1(ad4aba5996c2851f14741c5d0f3d7b65e7e765c5) )
        ROM_LOAD64_WORD_SWAP( "mpr20900.14", 0x3800004, 0x400000, CRC(7a38b571) SHA1(664be2a614e3ce2cc75fdfb9baff55b6e9d77998) )
        ROM_LOAD64_WORD_SWAP( "mpr20901.15", 0x3800002, 0x400000, CRC(3492ddc8) SHA1(2a36b91ca58b7cc1c8f7a337d2e40b671780ddeb) )
        ROM_LOAD64_WORD_SWAP( "mpr20902.16", 0x3800000, 0x400000, CRC(f4d3ff3a) SHA1(da3ceba113bca0ea7bd8f67d39bdd7d0cbe5ff7f) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20921.21", 0x000000, 0x080000, CRC(30f032a7) SHA1(d29c9631bd50fabe3d86343f44c37ee535db14a0) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20903.22", 0x000000, 0x400000, CRC(e343e131) SHA1(cb144516e8c6f1e68bcb774a26cdc494383d3e1b) )
        ROM_LOAD( "mpr20904.24", 0x400000, 0x400000, CRC(21a91b84) SHA1(cd2d7231b8652ff38376b672c47127ce054d1f32) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs29815 )	/* Step 1.5 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20909.17", 0x600006, 0x080000, CRC(3dff0d7e) SHA1(c6a6a103f499cd451796ae2480b8c38c3e87a143) )
        ROM_LOAD64_WORD_SWAP( "epr20910.18", 0x600004, 0x080000, CRC(dc75a2e3) SHA1(f1b13674ae20b5b964be593171b9d6008d5a51b7) )
        ROM_LOAD64_WORD_SWAP( "epr20911.19", 0x600002, 0x080000, CRC(acb8fd97) SHA1(2a0ae502283fc8b19ae2bb85b95f66bf80e1bcdf) )
        ROM_LOAD64_WORD_SWAP( "epr20912.20", 0x600000, 0x080000, CRC(cd2c0538) SHA1(17b66f0cfa0530be3091f974ec959917f2805be1) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20891.1", 0x800006, 0x400000, CRC(9ecb0b39) SHA1(97b2ba2a863b7559923efff315aab04f7dca33b0) )
        ROM_LOAD64_WORD_SWAP( "mpr20892.2", 0x800004, 0x400000, CRC(8e5d3fe7) SHA1(5fe7ad8577ce46fdd2ea741eb2a98028eee61a82) )
        ROM_LOAD64_WORD_SWAP( "mpr20893.3", 0x800002, 0x400000, CRC(5c83dcaa) SHA1(2d4794bc6c3bfd4913ee045692b6aec5680825e0) )
        ROM_LOAD64_WORD_SWAP( "mpr20894.4", 0x800000, 0x400000, CRC(09c065cc) SHA1(9a47c6fd45630549066f58b6872ad885908c6e38) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr19773.5", 0x1800006, 0x400000, CRC(4e381ae7) SHA1(8ada8de80e019d521d8a3dbdc832745478c84a3d) )
        ROM_LOAD64_WORD_SWAP( "mpr19774.6", 0x1800004, 0x400000, CRC(1d61d287) SHA1(f0ab5f687570fa3e33a87da9130859f804c8fc01) )
        ROM_LOAD64_WORD_SWAP( "mpr19775.7", 0x1800002, 0x400000, CRC(a6b32bd9) SHA1(5e2e1e779ff11620a3cf2a8756f2ea08e46d0839) )
        ROM_LOAD64_WORD_SWAP( "mpr19776.8", 0x1800000, 0x400000, CRC(5b31c7c1) SHA1(7821c5af52a9551be5358aae7df8cfddd58f0fb6) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20895.9", 0x2800006, 0x400000, CRC(9b51cbf5) SHA1(670cfee991b997d4f7c3d51c48dad1ee032f93ef) )
        ROM_LOAD64_WORD_SWAP( "mpr20896.10", 0x2800004, 0x400000, CRC(bf1cbd5e) SHA1(a677247d7c94f8d36ffece7824026047db1188e1) )
        ROM_LOAD64_WORD_SWAP( "mpr20897.11", 0x2800002, 0x400000, CRC(c5cf067a) SHA1(ee9503bee3af238434590f439c87219fe45c91b9) )
        ROM_LOAD64_WORD_SWAP( "mpr20898.12", 0x2800000, 0x400000, CRC(94040d37) SHA1(4a35a0e1cdfdf18a8b01ea11ae88815bf8e92ff3) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr20899.13", 0x3800006, 0x400000, CRC(65422425) SHA1(ad4aba5996c2851f14741c5d0f3d7b65e7e765c5) )
        ROM_LOAD64_WORD_SWAP( "mpr20900.14", 0x3800004, 0x400000, CRC(7a38b571) SHA1(664be2a614e3ce2cc75fdfb9baff55b6e9d77998) )
        ROM_LOAD64_WORD_SWAP( "mpr20901.15", 0x3800002, 0x400000, CRC(3492ddc8) SHA1(2a36b91ca58b7cc1c8f7a337d2e40b671780ddeb) )
        ROM_LOAD64_WORD_SWAP( "mpr20902.16", 0x3800000, 0x400000, CRC(f4d3ff3a) SHA1(da3ceba113bca0ea7bd8f67d39bdd7d0cbe5ff7f) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x600000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr19787.26", 0x000002, 0x200000, CRC(856cc4ad) SHA1(a8856ee407c10b021f00f5fd2180dfae4cad2dad) )
        ROM_LOAD_VROM( "mpr19788.27", 0x000000, 0x200000, CRC(72ef970a) SHA1(ee4af92444e7da61094b5eb5b8469b78aeeb8a32) )
        ROM_LOAD_VROM( "mpr19789.28", 0x000006, 0x200000, CRC(076add9a) SHA1(117156fd7b802807ee2908dc8a1edb1cd79f1730) )
        ROM_LOAD_VROM( "mpr19790.29", 0x000004, 0x200000, CRC(74ce238c) SHA1(f6662793d1f2f3c36d7548b912cd40b2ce58753e) )
        ROM_LOAD_VROM( "mpr19791.30", 0x00000a, 0x200000, CRC(75a98f96) SHA1(9c4cdc5a782cac957fc43952e8f3e35d54f23d1c) )
        ROM_LOAD_VROM( "mpr19792.31", 0x000008, 0x200000, CRC(85c81633) SHA1(e4311ca92e77502626b312d38a5069ceed9e679f) )
        ROM_LOAD_VROM( "mpr19793.32", 0x00000e, 0x200000, CRC(7f288cc4) SHA1(71e42ac564c85402b35777e28fff2dce161cbf46) )
        ROM_LOAD_VROM( "mpr19794.33", 0x00000c, 0x200000, CRC(e0c1c370) SHA1(8efc2940857109bbe74fb7ce1e6fb11d601630f2) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr19795.34", 0x000002, 0x200000, CRC(90989b20) SHA1(bf96f5770ffaae3b625a906461b8ea755baf9756) )
        ROM_LOAD_VROM( "mpr19796.35", 0x000000, 0x200000, CRC(5d1aab8d) SHA1(e004c30b9bc8fcad23459e162e9db0c1afd0d5b1) )
        ROM_LOAD_VROM( "mpr19797.36", 0x000006, 0x200000, CRC(f5edc891) SHA1(e8a23be9a81568892f95e7c66c28ea8d7bd0f508) )
        ROM_LOAD_VROM( "mpr19798.37", 0x000004, 0x200000, CRC(ae2da90f) SHA1(8243d1f81faa57d5ff5e1f64bcd33cff59219b69) )
        ROM_LOAD_VROM( "mpr19799.38", 0x00000a, 0x200000, CRC(92b18ad7) SHA1(7be46d7ae2233337d866938ac803589156bfde94) )
        ROM_LOAD_VROM( "mpr19800.39", 0x000008, 0x200000, CRC(4a57b16c) SHA1(341952460b2f7718e63d5f43a86f507de52bf421) )
        ROM_LOAD_VROM( "mpr19801.40", 0x00000e, 0x200000, CRC(beb79a00) SHA1(63385ff70bf9ae223e6acfa1b6cb2d641afa2790) )
        ROM_LOAD_VROM( "mpr19802.41", 0x00000c, 0x200000, CRC(f2c3a7b7) SHA1(d72fafe75baa3542ee27fed05230cd5da99aa459) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20921.21", 0x000000, 0x080000, CRC(30f032a7) SHA1(d29c9631bd50fabe3d86343f44c37ee535db14a0) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20903.22", 0x000000, 0x400000, CRC(e343e131) SHA1(cb144516e8c6f1e68bcb774a26cdc494383d3e1b) )
        ROM_LOAD( "mpr20904.24", 0x400000, 0x400000, CRC(21a91b84) SHA1(cd2d7231b8652ff38376b672c47127ce054d1f32) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs2v991 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21535b.17",    0x400006, 0x100000, CRC(76c5fa8e) SHA1(862438198cb7fdd20beeba53e707a7c59e618ad9) )
        ROM_LOAD64_WORD_SWAP( "epr21536b.18",    0x400004, 0x100000, CRC(1f2bd190) SHA1(19843e6c5626de03eba3cba79c03ce9f2471c183) )
        ROM_LOAD64_WORD_SWAP( "epr21537b.19",    0x400002, 0x100000, CRC(a8b3fa5c) SHA1(884042590da9eef0fc2557f715c5d6811edb4ce1) )
        ROM_LOAD64_WORD_SWAP( "epr21538b.20",    0x400000, 0x100000, CRC(b3f0ce2a) SHA1(940b76afe3e66bcd026e74f58a08b13c3925f449) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21497.1", 0x800006, 0x400000, CRC(8ea759a1) SHA1(0d444fa360d93f48e5d6607362a231f97a7685d4) )
        ROM_LOAD64_WORD_SWAP( "mpr21498.2", 0x800004, 0x400000, CRC(4f53d6e0) SHA1(c8cd14f46d4ac7afdf55035a20d2e9a5ce2b6cde) )
        ROM_LOAD64_WORD_SWAP( "mpr21499.3", 0x800002, 0x400000, CRC(2cc4c1f1) SHA1(fd0fd747368e798095119a21d82f14778aeaa45e) )
        ROM_LOAD64_WORD_SWAP( "mpr21500.4", 0x800000, 0x400000, CRC(8c43964b) SHA1(cf3a6e9402f9ba532fca73f6838478558fb9a3ba) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21501.5", 0x1800006, 0x400000, CRC(08bc2185) SHA1(6c4c977f68a73d605bdacdc0d76ca89bc7030c04) )
        ROM_LOAD64_WORD_SWAP( "mpr21502.6", 0x1800004, 0x400000, CRC(921486be) SHA1(bb1261272992cf86e83e0c788788765f05b43bbf) )
        ROM_LOAD64_WORD_SWAP( "mpr21503.7", 0x1800002, 0x400000, CRC(c9e1de6b) SHA1(d200c3da2c9bc6d4ed60dfa60a77056d25b19037) )
        ROM_LOAD64_WORD_SWAP( "mpr21504.8", 0x1800000, 0x400000, CRC(7aae557e) SHA1(2128d7dfa52e639858d37eb6100875b9ce3d056f) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr21505.9", 0x2800006, 0x400000, CRC(e169ff72) SHA1(9d407b424403261a224ea15b9476eba16406c4a4) )
        ROM_LOAD64_WORD_SWAP( "mpr21506.10", 0x2800004, 0x400000, CRC(2c1477c7) SHA1(81ab7d9cef5127e1f0e16f9a94a9ea2acc4530a4) )
        ROM_LOAD64_WORD_SWAP( "mpr21507.11", 0x2800002, 0x400000, CRC(1d8eb68b) SHA1(634693f066059c738526913498bb18be2f7cd086) )
        ROM_LOAD64_WORD_SWAP( "mpr21508.12", 0x2800000, 0x400000, CRC(2e8f798e) SHA1(8298df90101dd5850db8fccb7661ca2bc6806b3f) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr21509.13", 0x3800006, 0x400000, CRC(9a65e6b4) SHA1(e96c4bc2782b73490dffd5dcb11b9020077b11a3) )
        ROM_LOAD64_WORD_SWAP( "mpr21510.14", 0x3800004, 0x400000, CRC(f47489a4) SHA1(8412505002628d7ae3ab766a13e2068a018f3bf3) )
        ROM_LOAD64_WORD_SWAP( "mpr21511.15", 0x3800002, 0x400000, CRC(5ad9660c) SHA1(da387449292322a89af1cb6746d0fb8cea17575f) )
        ROM_LOAD64_WORD_SWAP( "mpr21512.16", 0x3800000, 0x400000, CRC(7cb2b05c) SHA1(16edc6642c74d9cef883559ca6ec562d985a43d6) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21515.26",     0x000002, 0x200000, CRC(8ce9910b) SHA1(7a0d0696e4456d9ebf131041917c5214b7d2e3ec) )
        ROM_LOAD_VROM( "mpr21516.27",     0x000000, 0x200000, CRC(8971a753) SHA1(00dfdb83a65f4fde337618c346157bb89f398531) )
        ROM_LOAD_VROM( "mpr21517.28",     0x000006, 0x200000, CRC(55a4533b) SHA1(b5701bbf7780bb9fc386cef4c1835606ab792f91) )
        ROM_LOAD_VROM( "mpr21518.29",     0x000004, 0x200000, CRC(4134026c) SHA1(2dfe1cbb354affe465c31a18c3ffb83a9bf555c9) )
        ROM_LOAD_VROM( "mpr21519.30",     0x00000a, 0x200000, CRC(ef6757de) SHA1(d41bbfcc551a4589bac577e311c67f2cba0a49aa) )
        ROM_LOAD_VROM( "mpr21520.31",     0x000008, 0x200000, CRC(c53be8cc) SHA1(b12dc0327a00b7e056254d2f11f96dbf396a0c91) )
        ROM_LOAD_VROM( "mpr21521.32",     0x00000e, 0x200000, CRC(abb501dc) SHA1(88cb40b0f795e0de1ff56e1f31bf834fad0c7885) )
        ROM_LOAD_VROM( "mpr21522.33",     0x00000c, 0x200000, CRC(e3b79973) SHA1(4b6ca16a23bb3e195ca60bee81b2d069f371ff70) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21523.34",     0x000002, 0x200000, CRC(fe4d1eac) SHA1(d222743d25ca92904ec212c66d03b3e3ff0ddbd9) )
        ROM_LOAD_VROM( "mpr21524.35",     0x000000, 0x200000, CRC(8633b6e9) SHA1(65ec24eb29613831dd28e5338cac14696b0d975d) )
        ROM_LOAD_VROM( "mpr21525.36",     0x000006, 0x200000, CRC(3c490167) SHA1(6fd46049723e0790b2231301cfa23071cd6ff1f6) )
        ROM_LOAD_VROM( "mpr21526.37",     0x000004, 0x200000, CRC(5fe5f9b0) SHA1(c708918cfc60f5fd9f6ec49ec1cd3167f2876e30) )
        ROM_LOAD_VROM( "mpr21527.38",     0x00000a, 0x200000, CRC(10d0fe7e) SHA1(63693b0de43e2eb6efbb3d2dfbe0e2f5bc6810dc) )
        ROM_LOAD_VROM( "mpr21528.39",     0x000008, 0x200000, CRC(4e346a6c) SHA1(ae34038d5bf6f63ec5ad2e8dd8e06db66147c40e) )
        ROM_LOAD_VROM( "mpr21529.40",     0x00000e, 0x200000, CRC(9a731a00) SHA1(eca98b142acc02fb28387675e1cb1bc7e4e59b86) )
        ROM_LOAD_VROM( "mpr21530.41",     0x00000c, 0x200000, CRC(78400d5e) SHA1(9b4546848dbe213f33b02e8ea42743e60a0f763f) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21539.21",    0x000000, 0x080000, CRC(a1d3e00e) SHA1(e03bb31967929a12de9ae21923914e0e3bd96aaa) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21513.22", 0x000000, 0x400000, CRC(cca1cc00) SHA1(ba1fa3b8ef3bff7e116901a0a4bd80d2ae4018bf) )
        ROM_LOAD( "mpr21514.24", 0x400000, 0x400000, CRC(6cedd292) SHA1(c1f44715697a8bac9d39926bcd6558ec9a9b2319) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( vs299 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21535.17", 0x400006, 0x100000, CRC(976a00bf) SHA1(d4be52ff59faa877b169f96ac509a2196cefb908) )
        ROM_LOAD64_WORD_SWAP( "epr21536.18", 0x400004, 0x100000, CRC(9af2b0d5) SHA1(6ec296014228782f372611fe774014d252956b63) )
        ROM_LOAD64_WORD_SWAP( "epr21537.19", 0x400002, 0x100000, CRC(fb37dc16) SHA1(205e7d6dae21ba2cd4c8e37c2acab680c3f5a9b4) )
        ROM_LOAD64_WORD_SWAP( "epr21538.20", 0x400000, 0x100000, CRC(02df6ac8) SHA1(2259b528264dd30fa38ea06934e2d38b44b32981) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21497.1", 0x800006, 0x400000, CRC(8ea759a1) SHA1(0d444fa360d93f48e5d6607362a231f97a7685d4) )
        ROM_LOAD64_WORD_SWAP( "mpr21498.2", 0x800004, 0x400000, CRC(4f53d6e0) SHA1(c8cd14f46d4ac7afdf55035a20d2e9a5ce2b6cde) )
        ROM_LOAD64_WORD_SWAP( "mpr21499.3", 0x800002, 0x400000, CRC(2cc4c1f1) SHA1(fd0fd747368e798095119a21d82f14778aeaa45e) )
        ROM_LOAD64_WORD_SWAP( "mpr21500.4", 0x800000, 0x400000, CRC(8c43964b) SHA1(cf3a6e9402f9ba532fca73f6838478558fb9a3ba) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21501.5", 0x1800006, 0x400000, CRC(08bc2185) SHA1(6c4c977f68a73d605bdacdc0d76ca89bc7030c04) )
        ROM_LOAD64_WORD_SWAP( "mpr21502.6", 0x1800004, 0x400000, CRC(921486be) SHA1(bb1261272992cf86e83e0c788788765f05b43bbf) )
        ROM_LOAD64_WORD_SWAP( "mpr21503.7", 0x1800002, 0x400000, CRC(c9e1de6b) SHA1(d200c3da2c9bc6d4ed60dfa60a77056d25b19037) )
        ROM_LOAD64_WORD_SWAP( "mpr21504.8", 0x1800000, 0x400000, CRC(7aae557e) SHA1(2128d7dfa52e639858d37eb6100875b9ce3d056f) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr21505.9", 0x2800006, 0x400000, CRC(e169ff72) SHA1(9d407b424403261a224ea15b9476eba16406c4a4) )
        ROM_LOAD64_WORD_SWAP( "mpr21506.10", 0x2800004, 0x400000, CRC(2c1477c7) SHA1(81ab7d9cef5127e1f0e16f9a94a9ea2acc4530a4) )
        ROM_LOAD64_WORD_SWAP( "mpr21507.11", 0x2800002, 0x400000, CRC(1d8eb68b) SHA1(634693f066059c738526913498bb18be2f7cd086) )
        ROM_LOAD64_WORD_SWAP( "mpr21508.12", 0x2800000, 0x400000, CRC(2e8f798e) SHA1(8298df90101dd5850db8fccb7661ca2bc6806b3f) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr21509.13", 0x3800006, 0x400000, CRC(9a65e6b4) SHA1(e96c4bc2782b73490dffd5dcb11b9020077b11a3) )
        ROM_LOAD64_WORD_SWAP( "mpr21510.14", 0x3800004, 0x400000, CRC(f47489a4) SHA1(8412505002628d7ae3ab766a13e2068a018f3bf3) )
        ROM_LOAD64_WORD_SWAP( "mpr21511.15", 0x3800002, 0x400000, CRC(5ad9660c) SHA1(da387449292322a89af1cb6746d0fb8cea17575f) )
        ROM_LOAD64_WORD_SWAP( "mpr21512.16", 0x3800000, 0x400000, CRC(7cb2b05c) SHA1(16edc6642c74d9cef883559ca6ec562d985a43d6) )

	// mirror CROM0 to CROM
	ROM_COPY(REGION_USER1, 0x800000, 0x000000, 0x400000)

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x1000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21515.26",     0x000002, 0x200000, CRC(8ce9910b) SHA1(7a0d0696e4456d9ebf131041917c5214b7d2e3ec) )
        ROM_LOAD_VROM( "mpr21516.27",     0x000000, 0x200000, CRC(8971a753) SHA1(00dfdb83a65f4fde337618c346157bb89f398531) )
        ROM_LOAD_VROM( "mpr21517.28",     0x000006, 0x200000, CRC(55a4533b) SHA1(b5701bbf7780bb9fc386cef4c1835606ab792f91) )
        ROM_LOAD_VROM( "mpr21518.29",     0x000004, 0x200000, CRC(4134026c) SHA1(2dfe1cbb354affe465c31a18c3ffb83a9bf555c9) )
        ROM_LOAD_VROM( "mpr21519.30",     0x00000a, 0x200000, CRC(ef6757de) SHA1(d41bbfcc551a4589bac577e311c67f2cba0a49aa) )
        ROM_LOAD_VROM( "mpr21520.31",     0x000008, 0x200000, CRC(c53be8cc) SHA1(b12dc0327a00b7e056254d2f11f96dbf396a0c91) )
        ROM_LOAD_VROM( "mpr21521.32",     0x00000e, 0x200000, CRC(abb501dc) SHA1(88cb40b0f795e0de1ff56e1f31bf834fad0c7885) )
        ROM_LOAD_VROM( "mpr21522.33",     0x00000c, 0x200000, CRC(e3b79973) SHA1(4b6ca16a23bb3e195ca60bee81b2d069f371ff70) )

	ROM_REGION( 0x1000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21523.34",     0x000002, 0x200000, CRC(fe4d1eac) SHA1(d222743d25ca92904ec212c66d03b3e3ff0ddbd9) )
        ROM_LOAD_VROM( "mpr21524.35",     0x000000, 0x200000, CRC(8633b6e9) SHA1(65ec24eb29613831dd28e5338cac14696b0d975d) )
        ROM_LOAD_VROM( "mpr21525.36",     0x000006, 0x200000, CRC(3c490167) SHA1(6fd46049723e0790b2231301cfa23071cd6ff1f6) )
        ROM_LOAD_VROM( "mpr21526.37",     0x000004, 0x200000, CRC(5fe5f9b0) SHA1(c708918cfc60f5fd9f6ec49ec1cd3167f2876e30) )
        ROM_LOAD_VROM( "mpr21527.38",     0x00000a, 0x200000, CRC(10d0fe7e) SHA1(63693b0de43e2eb6efbb3d2dfbe0e2f5bc6810dc) )
        ROM_LOAD_VROM( "mpr21528.39",     0x000008, 0x200000, CRC(4e346a6c) SHA1(ae34038d5bf6f63ec5ad2e8dd8e06db66147c40e) )
        ROM_LOAD_VROM( "mpr21529.40",     0x00000e, 0x200000, CRC(9a731a00) SHA1(eca98b142acc02fb28387675e1cb1bc7e4e59b86) )
        ROM_LOAD_VROM( "mpr21530.41",     0x00000c, 0x200000, CRC(78400d5e) SHA1(9b4546848dbe213f33b02e8ea42743e60a0f763f) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21539.21", 0x000000, 0x080000, CRC(a1d3e00e) SHA1(e03bb31967929a12de9ae21923914e0e3bd96aaa) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21513.22", 0x000000, 0x400000, CRC(cca1cc00) SHA1(ba1fa3b8ef3bff7e116901a0a4bd80d2ae4018bf) )
        ROM_LOAD( "mpr21514.24", 0x400000, 0x400000, CRC(6cedd292) SHA1(c1f44715697a8bac9d39926bcd6558ec9a9b2319) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( von2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20683b.17",    0x000006, 0x200000, CRC(59d9c974) SHA1(c45594ed474a9e8fd074e0d9d5fa6662bc88dee6) )
        ROM_LOAD64_WORD_SWAP( "epr20684b.18",    0x000004, 0x200000, CRC(1fc15431) SHA1(c68c77dfcf5e2702214d64095ce07076d3702a5e) )
        ROM_LOAD64_WORD_SWAP( "epr20685b.19",    0x000002, 0x200000, CRC(ae82cb35) SHA1(b4563f325945cc943a46bdc094e0169fcf82023d) )
        ROM_LOAD64_WORD_SWAP( "epr20686b.20",    0x000000, 0x200000, CRC(3ea4de9f) SHA1(0d09e0a256e531c1e4115355e9ce29fa8016c458) )

	// CROM0:
        ROM_LOAD64_WORD_SWAP( "mpr20647.1",  0x800006, 0x400000, CRC(e8586380) SHA1(67dd49975b31ba2c3f889ff38a3bc4663145934a) )
        ROM_LOAD64_WORD_SWAP( "mpr20648.2",  0x800004, 0x400000, CRC(107309e0) SHA1(61657814a30020c0d4ea77625cb8f11a1db7e866) )
        ROM_LOAD64_WORD_SWAP( "mpr20649.3",  0x800002, 0x400000, CRC(b8fd56ba) SHA1(5e5051d4b752463e1da632f8294a6c8f9250dbc8) )
        ROM_LOAD64_WORD_SWAP( "mpr20650.4",  0x800000, 0x400000, CRC(81f96649) SHA1(0d7aba7654237b68de6e43811832fafaf61e2bec) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20651.5",  0x1800006, 0x400000, CRC(8373cab3) SHA1(1d36612668a3004e2448f99ab27d7184ff859478) )
        ROM_LOAD64_WORD_SWAP( "mpr20652.6",  0x1800004, 0x400000, CRC(64c6fbb6) SHA1(c8682bda20d3119b4f95bbd2dbde301bfd036608) )
        ROM_LOAD64_WORD_SWAP( "mpr20653.7",  0x1800002, 0x400000, CRC(858e6bba) SHA1(22b71826799249a577124a49d5a276908a53ce61) )
        ROM_LOAD64_WORD_SWAP( "mpr20654.8",  0x1800000, 0x400000, CRC(763ef905) SHA1(4d5f6b1770cf9bf6cecd4d3a91a822e5cc658464) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20655.9",  0x2800006, 0x400000, CRC(f0a471e9) SHA1(8a40c9381e8b3733be297738c825b82abcb476d0) )
        ROM_LOAD64_WORD_SWAP( "mpr20656.10", 0x2800004, 0x400000, CRC(466bee13) SHA1(bc2087a138037188f462fa1cecc898e5efb3e8b8) )
        ROM_LOAD64_WORD_SWAP( "mpr20657.11", 0x2800002, 0x400000, CRC(14bf8964) SHA1(84444f7c489344ad1dd980b860364b5a4ed53038) )
        ROM_LOAD64_WORD_SWAP( "mpr20658.12", 0x2800000, 0x400000, CRC(b80175b9) SHA1(26dc97f6a6e8415cbb7e9e1f64389d80a2b761a1) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr20659.13", 0x3800006, 0x400000, CRC(edb63e7b) SHA1(761abcfc213e813967d053475c965459a9724a24) )
        ROM_LOAD64_WORD_SWAP( "mpr20660.14", 0x3800004, 0x400000, CRC(d961d385) SHA1(7e341c2cf24715c5cecb276c42166bf426860819) )
        ROM_LOAD64_WORD_SWAP( "mpr20661.15", 0x3800002, 0x400000, CRC(50e6189e) SHA1(04be5ff1379af4972edec3b320f148bdf09bfbb5) )
        ROM_LOAD64_WORD_SWAP( "mpr20662.16", 0x3800000, 0x400000, CRC(7130cb61) SHA1(39de0e3c2086f339156bfd734a196b667df7f5ac) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mp20667.26",   0x000002, 0x400000, CRC(321e006f) SHA1(687165bd2d2d22f861cd79083adcab62eb827c0f) )
        ROM_LOAD_VROM( "mp20668.27",   0x000000, 0x400000, CRC(c2dd8053) SHA1(52bc88d172d335b47e3ae3d582233382e9608de2) )
        ROM_LOAD_VROM( "mp20669.28",   0x000006, 0x400000, CRC(63432497) SHA1(b072741fe9ba49f1a7eed03301c8b1956af94d26) )
        ROM_LOAD_VROM( "mp20670.29",   0x000004, 0x400000, CRC(f7b554fd) SHA1(84fb08413345e0f3afb6e20c723aa8aa8156fdc7) )
        ROM_LOAD_VROM( "mp20671.30",   0x00000a, 0x400000, CRC(fee1a49b) SHA1(a024a0564df65e065e8b1830e85513d17ebd8635) )
        ROM_LOAD_VROM( "mp20672.31",   0x000008, 0x400000, CRC(e4b8c6e6) SHA1(674d4d26285f2825050fd27dd3382ca6245d54c7) )
        ROM_LOAD_VROM( "mp20673.32",   0x00000e, 0x400000, CRC(e7b6403b) SHA1(0f74f7a916c091d49eed8222050981a6b73d4bdd) )
        ROM_LOAD_VROM( "mp20674.33",   0x00000c, 0x400000, CRC(9be22e13) SHA1(a00b0c69b6ed086f3f61d4f767df6c4ddea45052) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mp20675.34",   0x000002, 0x400000, CRC(6a7c3862) SHA1(f77145c2a5e373f567783cf5db70e25b71e77bf5) )
        ROM_LOAD_VROM( "mp20676.35",   0x000000, 0x400000, CRC(dd299648) SHA1(c222c10cb23753ac3d6d1c779b2d026a64c61bc4) )
        ROM_LOAD_VROM( "mp20677.36",   0x000006, 0x400000, CRC(3fc5f330) SHA1(778c1932b093a4de96c76ea704463b7c67cdcb33) )
        ROM_LOAD_VROM( "mp20678.37",   0x000004, 0x400000, CRC(62f794a1) SHA1(fc7adafb49056b23b6cc483978ffe4fd3635977d) )
        ROM_LOAD_VROM( "mp20679.38",   0x00000a, 0x400000, CRC(35a37c53) SHA1(cd727a8914c3c01e302378048e3998b4cd849c4a) )
        ROM_LOAD_VROM( "mp20680.39",   0x000008, 0x400000, CRC(81fec46e) SHA1(43b3fbb544d920a87f77437860e32a628ae2865b) )
        ROM_LOAD_VROM( "mp20681.40",   0x00000e, 0x400000, CRC(d517873b) SHA1(8e50dd149716ae6b0b8d7ac99cd425a17b3c0a46) )
        ROM_LOAD_VROM( "mp20682.41",   0x00000c, 0x400000, CRC(5b43250c) SHA1(fccb40cd03c096360ca3c565e8621d4110b273ab) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20687.21", 0x000000, 0x080000, CRC(fa084de5) SHA1(8a760b76bc12d60d4727f93106830f19179c9046) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
	/* WARNING: mpr numbers here are a guess based on how other sets are ordered and may not be right.
       If restoring a real PCB, go by the IC numbers in the extension! (.22, .24) */
        ROM_LOAD( "mpr20663.22",	0x000000, 0x400000, CRC(977eb6a4) SHA1(9dbba51630cbef2351d79b82ab6ae3af4aed99f0) )
        ROM_LOAD( "mpr20665.24",	0x400000, 0x400000, CRC(0efc0ca8) SHA1(1414becad21eb7d03d816a8cba47506f941b3c29) )
        ROM_LOAD( "mpr20664.23",	0x800000, 0x400000, CRC(89220782) SHA1(18a3585af960a76eb08f187223e9b69ad16809a1) )
        ROM_LOAD( "mpr20666.25",	0xc00000, 0x400000, CRC(3ecb2606) SHA1(a38d1f61933c8873deaff0a913c657b768f9783d) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( von254g )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21788.17",  0x000006, 0x200000, CRC(97066bcf) SHA1(234c45ee1f23b22f61893825eebf31d867cf420f) )
        ROM_LOAD64_WORD_SWAP( "epr21789.18",  0x000004, 0x200000, CRC(3069108f) SHA1(f4e82da677458423abcf07c9c5a837005ed8f1c4) )
        ROM_LOAD64_WORD_SWAP( "epr21790.19",  0x000002, 0x200000, CRC(2ae1efd3) SHA1(fc3957a140d741138a8eeacc19eedbb237f629cd) )
        ROM_LOAD64_WORD_SWAP( "epr21791.20",  0x000000, 0x200000, CRC(d0bb3ca3) SHA1(7d00205b5366d7a6f9ecc10f8f7dcf335789a043) )

	// CROM0:
        ROM_LOAD64_WORD_SWAP( "mpr20647.1",  0x800006, 0x400000, CRC(e8586380) SHA1(67dd49975b31ba2c3f889ff38a3bc4663145934a) )
        ROM_LOAD64_WORD_SWAP( "mpr20648.2",  0x800004, 0x400000, CRC(107309e0) SHA1(61657814a30020c0d4ea77625cb8f11a1db7e866) )
        ROM_LOAD64_WORD_SWAP( "mpr20649.3",  0x800002, 0x400000, CRC(b8fd56ba) SHA1(5e5051d4b752463e1da632f8294a6c8f9250dbc8) )
        ROM_LOAD64_WORD_SWAP( "mpr20650.4",  0x800000, 0x400000, CRC(81f96649) SHA1(0d7aba7654237b68de6e43811832fafaf61e2bec) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20651.5",  0x1800006, 0x400000, CRC(8373cab3) SHA1(1d36612668a3004e2448f99ab27d7184ff859478) )
        ROM_LOAD64_WORD_SWAP( "mpr20652.6",  0x1800004, 0x400000, CRC(64c6fbb6) SHA1(c8682bda20d3119b4f95bbd2dbde301bfd036608) )
        ROM_LOAD64_WORD_SWAP( "mpr20653.7",  0x1800002, 0x400000, CRC(858e6bba) SHA1(22b71826799249a577124a49d5a276908a53ce61) )
        ROM_LOAD64_WORD_SWAP( "mpr20654.8",  0x1800000, 0x400000, CRC(763ef905) SHA1(4d5f6b1770cf9bf6cecd4d3a91a822e5cc658464) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20655.9",  0x2800006, 0x400000, CRC(f0a471e9) SHA1(8a40c9381e8b3733be297738c825b82abcb476d0) )
        ROM_LOAD64_WORD_SWAP( "mpr20656.10", 0x2800004, 0x400000, CRC(466bee13) SHA1(bc2087a138037188f462fa1cecc898e5efb3e8b8) )
        ROM_LOAD64_WORD_SWAP( "mpr20657.11", 0x2800002, 0x400000, CRC(14bf8964) SHA1(84444f7c489344ad1dd980b860364b5a4ed53038) )
        ROM_LOAD64_WORD_SWAP( "mpr20658.12", 0x2800000, 0x400000, CRC(b80175b9) SHA1(26dc97f6a6e8415cbb7e9e1f64389d80a2b761a1) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr20659.13", 0x3800006, 0x400000, CRC(edb63e7b) SHA1(761abcfc213e813967d053475c965459a9724a24) )
        ROM_LOAD64_WORD_SWAP( "mpr20660.14", 0x3800004, 0x400000, CRC(d961d385) SHA1(7e341c2cf24715c5cecb276c42166bf426860819) )
        ROM_LOAD64_WORD_SWAP( "mpr20661.15", 0x3800002, 0x400000, CRC(50e6189e) SHA1(04be5ff1379af4972edec3b320f148bdf09bfbb5) )
        ROM_LOAD64_WORD_SWAP( "mpr20662.16", 0x3800000, 0x400000, CRC(7130cb61) SHA1(39de0e3c2086f339156bfd734a196b667df7f5ac) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mp20667.26",   0x000002, 0x400000, CRC(321e006f) SHA1(687165bd2d2d22f861cd79083adcab62eb827c0f) )
        ROM_LOAD_VROM( "mp20668.27",   0x000000, 0x400000, CRC(c2dd8053) SHA1(52bc88d172d335b47e3ae3d582233382e9608de2) )
        ROM_LOAD_VROM( "mp20669.28",   0x000006, 0x400000, CRC(63432497) SHA1(b072741fe9ba49f1a7eed03301c8b1956af94d26) )
        ROM_LOAD_VROM( "mp20670.29",   0x000004, 0x400000, CRC(f7b554fd) SHA1(84fb08413345e0f3afb6e20c723aa8aa8156fdc7) )
        ROM_LOAD_VROM( "mp20671.30",   0x00000a, 0x400000, CRC(fee1a49b) SHA1(a024a0564df65e065e8b1830e85513d17ebd8635) )
        ROM_LOAD_VROM( "mp20672.31",   0x000008, 0x400000, CRC(e4b8c6e6) SHA1(674d4d26285f2825050fd27dd3382ca6245d54c7) )
        ROM_LOAD_VROM( "mp20673.32",   0x00000e, 0x400000, CRC(e7b6403b) SHA1(0f74f7a916c091d49eed8222050981a6b73d4bdd) )
        ROM_LOAD_VROM( "mp20674.33",   0x00000c, 0x400000, CRC(9be22e13) SHA1(a00b0c69b6ed086f3f61d4f767df6c4ddea45052) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mp20675.34",   0x000002, 0x400000, CRC(6a7c3862) SHA1(f77145c2a5e373f567783cf5db70e25b71e77bf5) )
        ROM_LOAD_VROM( "mp20676.35",   0x000000, 0x400000, CRC(dd299648) SHA1(c222c10cb23753ac3d6d1c779b2d026a64c61bc4) )
        ROM_LOAD_VROM( "mp20677.36",   0x000006, 0x400000, CRC(3fc5f330) SHA1(778c1932b093a4de96c76ea704463b7c67cdcb33) )
        ROM_LOAD_VROM( "mp20678.37",   0x000004, 0x400000, CRC(62f794a1) SHA1(fc7adafb49056b23b6cc483978ffe4fd3635977d) )
        ROM_LOAD_VROM( "mp20679.38",   0x00000a, 0x400000, CRC(35a37c53) SHA1(cd727a8914c3c01e302378048e3998b4cd849c4a) )
        ROM_LOAD_VROM( "mp20680.39",   0x000008, 0x400000, CRC(81fec46e) SHA1(43b3fbb544d920a87f77437860e32a628ae2865b) )
        ROM_LOAD_VROM( "mp20681.40",   0x00000e, 0x400000, CRC(d517873b) SHA1(8e50dd149716ae6b0b8d7ac99cd425a17b3c0a46) )
        ROM_LOAD_VROM( "mp20682.41",   0x00000c, 0x400000, CRC(5b43250c) SHA1(fccb40cd03c096360ca3c565e8621d4110b273ab) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20687.21", 0x000000, 0x080000, CRC(fa084de5) SHA1(8a760b76bc12d60d4727f93106830f19179c9046) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
	/* WARNING: mpr numbers here are a guess based on how other sets are ordered and may not be right.
       If restoring a real PCB, go by the IC numbers in the extension! (.22, .24) */
        ROM_LOAD( "mpr20663.22",	0x000000, 0x400000, CRC(977eb6a4) SHA1(9dbba51630cbef2351d79b82ab6ae3af4aed99f0) )
        ROM_LOAD( "mpr20665.24",	0x400000, 0x400000, CRC(0efc0ca8) SHA1(1414becad21eb7d03d816a8cba47506f941b3c29) )
        ROM_LOAD( "mpr20664.23",	0x800000, 0x400000, CRC(89220782) SHA1(18a3585af960a76eb08f187223e9b69ad16809a1) )
        ROM_LOAD( "mpr20666.25",	0xc00000, 0x400000, CRC(3ecb2606) SHA1(a38d1f61933c8873deaff0a913c657b768f9783d) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END


ROM_START( swtrilgy )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21379a.17", 0x000006, 0x200000, CRC(24dc1555) SHA1(0a4b458bb09238de0f38ba2805512b5dbee7d58e) )
        ROM_LOAD64_WORD_SWAP( "epr21380a.18", 0x000004, 0x200000, CRC(780fb4e7) SHA1(6650e114bad0e4c3f67b744599dba9845da82f11) )
        ROM_LOAD64_WORD_SWAP( "epr21381a.19", 0x000002, 0x200000, CRC(2dd34e28) SHA1(b9d2034aee6be2313f7286091f4660bbba87e376) )
        ROM_LOAD64_WORD_SWAP( "epr21382a.20", 0x000000, 0x200000, CRC(69baf117) SHA1(ceba6b9b092953b00777e5309bbba527270b0c40) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21339.01",  0x800006, 0x400000, CRC(c0ce5037) SHA1(1765339ac94b7e7e54217cc9703610f6a39eda4f) )
        ROM_LOAD64_WORD_SWAP( "mpr21340.02",  0x800004, 0x400000, CRC(ad36040e) SHA1(d40b888f892aa40b1d7f375cf7523667f3b7ee12) )
        ROM_LOAD64_WORD_SWAP( "mpr21341.03",  0x800002, 0x400000, CRC(b2a269e4) SHA1(b32769251118d3ae5c1c30864b58e27365b9602d) )
        ROM_LOAD64_WORD_SWAP( "mpr21342.04",  0x800000, 0x400000, CRC(339525ce) SHA1(e93ee61705612ae7017d9d99b26a6b5c2d20a15c) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21343.05",  0x1800006, 0x400000, CRC(12552d07) SHA1(7ed31feecda71f44ca7d3409d753d75c36a03ae8) )
        ROM_LOAD64_WORD_SWAP( "mpr21344.06",  0x1800004, 0x400000, CRC(87453d76) SHA1(5793ac0b4ae02364821d82e8cf30baf676bc7649) )
        ROM_LOAD64_WORD_SWAP( "mpr21345.07",  0x1800002, 0x400000, CRC(6c183a21) SHA1(416e56b76464df4aed552fe2e7262334e5841b17) )
        ROM_LOAD64_WORD_SWAP( "mpr21346.08",  0x1800000, 0x400000, CRC(c8733594) SHA1(1196ec899c64c58f21d56bff56e432b156aacb31) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr21347.09",  0x2800006, 0x400000, CRC(ecb6b934) SHA1(49e394adc3c339a13df6679457b910b9e0a078c1) )
        ROM_LOAD64_WORD_SWAP( "mpr21348.10",  0x2800004, 0x400000, CRC(1f7cc5f5) SHA1(6ac1bef009ba86e97541f4d6bbdb935fb8a22f5a) )
        ROM_LOAD64_WORD_SWAP( "mpr21349.11",  0x2800002, 0x400000, CRC(3d39454b) SHA1(1c55339a0694fc817e7ee2f2087c7548361c3f8b) )
        ROM_LOAD64_WORD_SWAP( "mpr21350.12",  0x2800000, 0x400000, CRC(486195e7) SHA1(b3725da2317561b8570666e459737022370256a8) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21359.26",  0x000002, 0x400000, CRC(34ef4122) SHA1(26e0726e1ab722ba0e12624efd01af3a40fc320b) )
        ROM_LOAD_VROM( "mpr21360.27",  0x000000, 0x400000, CRC(2882b95e) SHA1(e553661c98da3e23318920576488b8ff97430f44) )
        ROM_LOAD_VROM( "mpr21361.28",  0x000006, 0x400000, CRC(9b61c3c1) SHA1(93b4acb9340176b578f8222fcaf8fc67fd874556) )
        ROM_LOAD_VROM( "mpr21362.29",  0x000004, 0x400000, CRC(01a92169) SHA1(0911d5a656a5b5de5fefab77ea34a1b495863610) )
        ROM_LOAD_VROM( "mpr21363.30",  0x00000a, 0x400000, CRC(e7d18fed) SHA1(3e77e09db4f00780a5bcf6e644bfdc72b9d4ac83) )
        ROM_LOAD_VROM( "mpr21364.31",  0x000008, 0x400000, CRC(cb6a5468) SHA1(3ca093646b565eb6298c3d66da83664f718fe76a) )
        ROM_LOAD_VROM( "mpr21365.32",  0x00000e, 0x400000, CRC(ad5449d8) SHA1(e7f1b4b6ebbe578f292b5a71258c79767f57cf90) )
        ROM_LOAD_VROM( "mpr21366.33",  0x00000c, 0x400000, CRC(defb6b95) SHA1(a5de55c8e4bcbf2aef93972e3aba22ba64e46fdb) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21367.34",  0x000002, 0x400000, CRC(dfd51029) SHA1(44c2e6c8e36217bb4fc0743912f0b046a1944a74) )
        ROM_LOAD_VROM( "mpr21368.35",  0x000000, 0x400000, CRC(ae90fd21) SHA1(a519add40e29e6b737f50d9314a6009d4c696a9f) )
        ROM_LOAD_VROM( "mpr21369.36",  0x000006, 0x400000, CRC(bf17eeb4) SHA1(12d5f0c9c6ad27a225dbecdc7b94ade0e90a8f00) )
        ROM_LOAD_VROM( "mpr21370.37",  0x000004, 0x400000, CRC(2321592a) SHA1(d4270b872e1a5ff82220014c65b726309305ecb0) )
        ROM_LOAD_VROM( "mpr21371.38",  0x00000a, 0x400000, CRC(a68782fd) SHA1(610530f804876206fdd2c2f9ff159db9813fabea) )
        ROM_LOAD_VROM( "mpr21372.39",  0x000008, 0x400000, CRC(fc3f4e8b) SHA1(47240f14d81458e104452125eabf44619e026ff9) )
        ROM_LOAD_VROM( "mpr21373.40",  0x00000e, 0x400000, CRC(b76ad261) SHA1(de5a39a23ac6b12b17f16f2b3e82d1f5470ae600) )
        ROM_LOAD_VROM( "mpr21374.41",  0x00000c, 0x400000, CRC(ae6c4d28) SHA1(b57733cfaa63ba018b0c3c9c935c12c48cc7f184) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21383.21",  0x000000, 0x080000, CRC(544d1e28) SHA1(8b4c99cf9ad0cf15d2d3da578bbc08705bafb829) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21355.22",  0x000000, 0x400000, CRC(c1b2d326) SHA1(118d9e02cdb9f500bd677b1de8331b29c57ca02f) )
        ROM_LOAD( "mpr21357.24",  0x400000, 0x400000, CRC(02703fab) SHA1(c312f3d7967229660a7fb81b4fcd16c204d671cd) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( dirtdvls )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr21062a.17", 0x000006, 0x200000, CRC(64b55254) SHA1(0e5de3786edad77dde08652ac837dc9125e7851c) )
        ROM_LOAD64_WORD_SWAP( "epr21063a.18", 0x000004, 0x200000, CRC(6ab7eb32) SHA1(3a4226d4c786e7b64688af3b8883b4039b8c8407) )
        ROM_LOAD64_WORD_SWAP( "epr21064a.19", 0x000002, 0x200000, CRC(2a01f9ad) SHA1(d936d8eeecbcc502e35799d484c36f5da9457013) )
        ROM_LOAD64_WORD_SWAP( "epr21065a.20", 0x000000, 0x200000, CRC(3223db1a) SHA1(ad34d16475a571ffed48539ef98736357cb327b0) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr21023.1",   0x800006, 0x400000, CRC(932a3724) SHA1(146dfe897caa8a4385c527bc7c649e9dbd2ce0c0) )
        ROM_LOAD64_WORD_SWAP( "mpr21024.2",   0x800004, 0x400000, CRC(ede859b0) SHA1(cecd595a6ba60e248b7bf47778ba4da7658dcf93) )
        ROM_LOAD64_WORD_SWAP( "mpr21025.3",   0x800002, 0x400000, CRC(6591c66e) SHA1(feaae431692a3bab867b79d52bc3934f77c4022b) )
        ROM_LOAD64_WORD_SWAP( "mpr21026.4",   0x800000, 0x400000, CRC(f4937e3f) SHA1(21559ef991789ede4b4e7297e2a71f33f7cc7090) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr21027.5",   0x1800006, 0x400000, CRC(74e1496a) SHA1(0988058a109216e8b97045dde9d1099688193a13) )
        ROM_LOAD64_WORD_SWAP( "mpr21028.6",   0x1800004, 0x400000, CRC(db11f50a) SHA1(78bf2418bcea1ed30da9af936e9f95e9c76ce919) )
        ROM_LOAD64_WORD_SWAP( "mpr21029.7",   0x1800002, 0x400000, CRC(89867d8a) SHA1(89ebd5bc5d98fbd63d4cad407033419a39b1d60a) )
        ROM_LOAD64_WORD_SWAP( "mpr21030.8",   0x1800000, 0x400000, CRC(f8e51bec) SHA1(fe8a06ef21dd646e3ad6fa382e3f3d30db4cbd91) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr21034.26",  0x000002, 0x400000, CRC(acba5ca6) SHA1(be213ca40d17f18e725349585f95d677e53c1bfc) )
        ROM_LOAD_VROM( "mpr21035.27",  0x000000, 0x400000, CRC(618b7d6a) SHA1(0968b72c8d7fc4b2635062647da5d36a58e69b08) )
        ROM_LOAD_VROM( "mpr21036.28",  0x000006, 0x400000, CRC(0e665bb2) SHA1(3b18ea93ed1d71873ff635358c3143e4f515bab9) )
        ROM_LOAD_VROM( "mpr21037.29",  0x000004, 0x400000, CRC(90b98493) SHA1(3f98855caec5895c8651ed88e07f2dcec5a6c66a) )
        ROM_LOAD_VROM( "mpr21038.30",  0x00000a, 0x400000, CRC(9b59d2c2) SHA1(3f14cfc905a018e0aa2b2ad4918cd4ee2ef65c7b) )
        ROM_LOAD_VROM( "mpr21039.31",  0x000008, 0x400000, CRC(61407b07) SHA1(d7676a03110ca694cc53c1d3a6c781d2f8cee98b) )
        ROM_LOAD_VROM( "mpr21040.32",  0x00000e, 0x400000, CRC(b550c229) SHA1(b13ea462914bb13388e11bed9a9b2e696a8eb759) )
        ROM_LOAD_VROM( "mpr21041.33",  0x00000c, 0x400000, CRC(8f1ac988) SHA1(11b628c85533a307298765641eb87c305bde64d1) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr21042.34",  0x000002, 0x400000, CRC(1dab621d) SHA1(cf0e59be7b5a12146f5562e208009054074151cd) )
        ROM_LOAD_VROM( "mpr21043.35",  0x000000, 0x400000, CRC(707015c8) SHA1(125ff08cc555a4c8d9863e7433fad7949230630d) )
        ROM_LOAD_VROM( "mpr21044.36",  0x000006, 0x400000, CRC(776f9580) SHA1(0529532975d74da851a2fd1ce9810e218d751d5f) )
        ROM_LOAD_VROM( "mpr21045.37",  0x000004, 0x400000, CRC(a28ad02f) SHA1(8734568153dbf304193491e746b19a423a547f0d) )
        ROM_LOAD_VROM( "mpr21046.38",  0x00000a, 0x400000, CRC(05c995ae) SHA1(d96391360692d30c456324dcd51511bf095a58cb) )
        ROM_LOAD_VROM( "mpr21047.39",  0x000008, 0x400000, CRC(06b7826f) SHA1(cfdeb56964bd31196fde01b1f5cc294c8b49c215) )
        ROM_LOAD_VROM( "mpr21048.40",  0x00000e, 0x400000, CRC(96849974) SHA1(347e2216ea1225eda92693dcd80eb97df88caabf) )
        ROM_LOAD_VROM( "mpr21049.41",  0x00000c, 0x400000, CRC(91e8161a) SHA1(1edc0bc856e5d72f714bd0544814727f4ff12e7a) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr21066.21",  0x000000, 0x080000, CRC(f7ed2582) SHA1(a4f80d5f82c86f0bdb74bcda5dc69b83b475c542) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr21031.22",  0x000000, 0x400000, CRC(32f6b23a) SHA1(8cd092733b85aecf607c2f4b683c42e388a70906) )
        ROM_LOAD( "mpr21033.24",  0x400000, 0x400000, CRC(253d3c70) SHA1(bfbc42d08cf46d89c87505f53e31b8a53e8a729a) )
        ROM_LOAD( "mpr21032.23",  0x800000, 0x400000, CRC(3d3ff407) SHA1(5e298e24cb3050f8683658cef41ce59948e79166) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x800000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END

ROM_START( daytona2 )	/* Step 2.1 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20861a.17", 0x000006, 0x200000, CRC(89ba8e78) SHA1(7d27124b976a63fdadd16551a664b2cc8cc08e79) )
        ROM_LOAD64_WORD_SWAP( "epr20862a.18", 0x000004, 0x200000, CRC(e1b2ca61) SHA1(a5aa3416554b9d62469af3fefa9c2bacd69b4707) )
        ROM_LOAD64_WORD_SWAP( "epr20863a.19", 0x000002, 0x200000, CRC(1deb4686) SHA1(dd6fcc95afa36148089b766c24c53a2af4d392d4) )
        ROM_LOAD64_WORD_SWAP( "epr20864a.20", 0x000000, 0x200000, CRC(5250f3a8) SHA1(d75b86cb320b854fcf53e1b76331f87806b9ae84) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20845.1",   0x800006, 0x400000, CRC(ab70d79e) SHA1(938c492f1ea7208d27dd0ea44c4d826625e652d8) )
        ROM_LOAD64_WORD_SWAP( "mpr20846.2",   0x800004, 0x400000, CRC(aabf4392) SHA1(22d10c3e7896ff6e5cbbfd38e85ffa3daec895c0) )
        ROM_LOAD64_WORD_SWAP( "mpr20847.3",   0x800002, 0x400000, CRC(96f3e97a) SHA1(4f6a4dd7d1eecbad65dc10c2f47db4bb949b0dfc) )
        ROM_LOAD64_WORD_SWAP( "mpr20848.4",   0x800000, 0x400000, CRC(7a22a63b) SHA1(00c0b7c179f02936847d9e3fc2507d822efc7202) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20849.5",   0x1800006, 0x400000, CRC(3c3cc882) SHA1(2e34b999155e3bac7978428abaf77be9fedcc306) )
        ROM_LOAD64_WORD_SWAP( "mpr20850.6",   0x1800004, 0x400000, CRC(aa71a7a6) SHA1(27f160c6ecdc4dc99198879976ad1ddc46dd28ea) )
        ROM_LOAD64_WORD_SWAP( "mpr20851.7",   0x1800002, 0x400000, CRC(ba128235) SHA1(e307addb5c172c248c0472574148d5b54f1e0b78) )
        ROM_LOAD64_WORD_SWAP( "mpr20852.8",   0x1800000, 0x400000, CRC(e1000d4d) SHA1(5a09106616a7c116ee770025d1eedff8a0cda5ca) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20853.9",   0x2800006, 0x400000, CRC(3245ee68) SHA1(f957df447bab0559076bc8a14eeaa080040acb85) )
        ROM_LOAD64_WORD_SWAP( "mpr20854.10",  0x2800004, 0x400000, CRC(68d94cdf) SHA1(298236225ab5d0265ec615e71d084c79932d0438) )
        ROM_LOAD64_WORD_SWAP( "mpr20855.11",  0x2800002, 0x400000, CRC(f1ff0794) SHA1(22782a01de0b699e7663aeb659169581830e69fd) )
        ROM_LOAD64_WORD_SWAP( "mpr20856.12",  0x2800000, 0x400000, CRC(0367a242) SHA1(e3bb0ad7abddd8e81e4052dae169dfa245acbfa8) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "mpr20857.13",  0x3800006, 0x400000, CRC(1eab9c62) SHA1(7c5cbab2761609cdf6ab20c2e198135fa9ae1067) )
        ROM_LOAD64_WORD_SWAP( "mpr20858.14",  0x3800004, 0x400000, CRC(407fbad5) SHA1(594d45311d1daf00773b466c64ece4975df4dd5a) )
        ROM_LOAD64_WORD_SWAP( "mpr20859.15",  0x3800002, 0x400000, CRC(e14f5c46) SHA1(d80ccd2c5ea1f34280241aaa5c947397b0344820) )
        ROM_LOAD64_WORD_SWAP( "mpr20860.16",  0x3800000, 0x400000, CRC(e5ce2939) SHA1(7e5626cfb68402de80a05e4c3b18bdfd8bb40f85) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr20870.26",  0x000002, 0x400000, CRC(7c9e573d) SHA1(0c488dcca8946b846a179dd66ca09d9eabedc21c) )
        ROM_LOAD_VROM( "mpr20871.27",  0x000000, 0x400000, CRC(47a1b789) SHA1(8f817fefcfe99c27b30bb3bd9276b0a0947e5992) )
        ROM_LOAD_VROM( "mpr20872.28",  0x000006, 0x400000, CRC(2f55b423) SHA1(4fa5b00715163b8f68a01627eb04dadbcbb6e89a) )
        ROM_LOAD_VROM( "mpr20873.29",  0x000004, 0x400000, CRC(c9000e48) SHA1(a64a13e3116e36309ed37ec8651ef8a38b27b71f) )
        ROM_LOAD_VROM( "mpr20874.30",  0x00000a, 0x400000, CRC(26a9cca2) SHA1(e3b68daaef95e004217c6f7cf3c6d256d083b994) )
        ROM_LOAD_VROM( "mpr20875.31",  0x000008, 0x400000, CRC(bfefd21e) SHA1(8422b16569e029fead5cfca1ab31a04179a0f4b2) )
        ROM_LOAD_VROM( "mpr20876.32",  0x00000e, 0x400000, CRC(fa701b87) SHA1(4134da01818cd6776a3c3396ac4f9b9e815722f2) )
        ROM_LOAD_VROM( "mpr20877.33",  0x00000c, 0x400000, CRC(2cd072f1) SHA1(97881da77abc1aee309de64b53a0da3d22c4112c) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr20878.34",  0x000002, 0x400000, CRC(e6d5bc01) SHA1(d8d2e202830693ac2f32304c484276193eb550a3) )
        ROM_LOAD_VROM( "mpr20879.35",  0x000000, 0x400000, CRC(f1d727ec) SHA1(aecd2096bee590e8ccab9710845e8f0675bd06b0) )
        ROM_LOAD_VROM( "mpr20880.36",  0x000006, 0x400000, CRC(8b370602) SHA1(2c461ae94e0bd0fea623724b82e08238f947e26b) )
        ROM_LOAD_VROM( "mpr20881.37",  0x000004, 0x400000, CRC(397322e7) SHA1(9d1c24a063e3e00ca4ee02162dc0fc6de48de2d7) )
        ROM_LOAD_VROM( "mpr20882.38",  0x00000a, 0x400000, CRC(9185be51) SHA1(b9f5ce29d9dda45733a649aa049124990f536fd9) )
        ROM_LOAD_VROM( "mpr20883.39",  0x000008, 0x400000, CRC(d1e39e83) SHA1(8d8ccd7f23004e43ede71f73069979a3834bc010) )
        ROM_LOAD_VROM( "mpr20884.40",  0x00000e, 0x400000, CRC(63c4639a) SHA1(d2b47f7bb8244e0a25c15d025d1bb295101f8875) )
        ROM_LOAD_VROM( "mpr20885.41",  0x00000c, 0x400000, CRC(61c292ca) SHA1(a2c7e81a8a8ded8d0fd33ffea74a8d2cc8f22520) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20865.21",  0x000000, 0x020000, CRC(b70c2699) SHA1(9ec3f59eda18c03530a5ab7a54c09c1e14cb1c4d) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20866.22",  0x000000, 0x400000, CRC(91f40c1c) SHA1(0c9bee8e9a8bb5ffb2699922204ec26b489eee84) )
        ROM_LOAD( "mpr20868.24",  0x400000, 0x400000, CRC(fa0c7ec0) SHA1(e3570318b67b9e9819830ad73529a627ac2f2821) )
        ROM_LOAD( "mpr20867.23",  0x800000, 0x400000, CRC(a579c884) SHA1(ffa626381b1b2c0b963f8f0bad508c052364e657) )
        ROM_LOAD( "mpr20869.25",  0xc00000, 0x400000, CRC(1f338832) SHA1(77160ea4d336aa88725b868c0035a267e92030b3) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
        ROM_LOAD( "epr20886.ic2", 0x000000, 0x020000, CRC(65b05f98) SHA1(b83a2a6e7ec3d2fcd34ce701ffa66d99f6feb86d) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr20887.ic18", 0x000000, 0x400000, CRC(faaf0e79) SHA1(81dd1e3c04d9882b35347ebb3a164f5d83ff36d1) )
        ROM_LOAD( "mpr20888.ic20", 0x400000, 0x400000, CRC(b495fe65) SHA1(1573ced683534bb279a6dd69f6460745b728eac0) )
        ROM_LOAD( "mpr20889.ic22", 0x800000, 0x400000, CRC(18eec79e) SHA1(341982d89952ed85c921c627c294609bf83ec44b) )
        ROM_LOAD( "mpr20890.ic24", 0xc00000, 0x400000, CRC(aac96fa2) SHA1(bc68cd48eae50d3558d3c5a0302a3930639e3019) )
ROM_END

ROM_START( srally2 )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20632.17", 0x000006, 0x200000, CRC(6829a801) SHA1(2aa3834f6a8c53f5db57ab52994b8ab3fde2d7c2) )
        ROM_LOAD64_WORD_SWAP( "epr20633.18", 0x000004, 0x200000, CRC(f5a24f24) SHA1(6f741bc53d51ff4b5535dbee35aa490f159945ec) )
        ROM_LOAD64_WORD_SWAP( "epr20634.19", 0x000002, 0x200000, CRC(45a09245) SHA1(1e7e844d38e9cba59c2a7e6f2e6ca2bba2c8b352) )
        ROM_LOAD64_WORD_SWAP( "epr20635.20", 0x000000, 0x200000, CRC(7937473f) SHA1(d5bb57a08019a4523f976c418526efffc6ef988b) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20602.1",  0x800006, 0x400000, CRC(60cfa72a) SHA1(2cb9e16d50979461d39e0493273ee38d23900b45) )
        ROM_LOAD64_WORD_SWAP( "mpr20603.2",  0x800004, 0x400000, CRC(ad0d8eb8) SHA1(5bcd0a72b2d48d1834f296b7b37a56ae13c461fa) )
        ROM_LOAD64_WORD_SWAP( "mpr20604.3",  0x800002, 0x400000, CRC(99c5f396) SHA1(dc586591a684bde224704f8356640343eb7651b3) )
        ROM_LOAD64_WORD_SWAP( "mpr20605.4",  0x800000, 0x400000, CRC(00513401) SHA1(c6ac665cea999dcf483e9837daefd7fcdd755043) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20606.5",  0x1800006, 0x400000, CRC(072498fd) SHA1(f7804a7c2139901367b06823bf0a6472e24078a6) )
        ROM_LOAD64_WORD_SWAP( "mpr20607.6",  0x1800004, 0x400000, CRC(6da85aa3) SHA1(04fce70fa69aca615e5b009be99f7a657070b0c0) )
        ROM_LOAD64_WORD_SWAP( "mpr20608.7",  0x1800002, 0x400000, CRC(0c9b0571) SHA1(2e13ab15d7fb6104316dc86770172269c8280306) )
        ROM_LOAD64_WORD_SWAP( "mpr20609.8",  0x1800000, 0x400000, CRC(c03cc0e5) SHA1(8ee99dd37587dd1aa24d746ef14180515306e5d9) )

	// CROM2
        ROM_LOAD64_WORD_SWAP( "mpr20610.9",  0x2800006, 0x400000, CRC(b6e0ff4e) SHA1(9b6ab7925b64b34f0d16ff0ba72dd0a0e6d23290) )
        ROM_LOAD64_WORD_SWAP( "mpr20611.10", 0x2800004, 0x400000, CRC(5d9f8ba2) SHA1(42bec0ca6cf8d0192183dd08c4a92157c34d3651) )
        ROM_LOAD64_WORD_SWAP( "mpr20612.11", 0x2800002, 0x400000, CRC(721a44b6) SHA1(151e4f9e2ed4095620c88bf54bf69f13f441e5ce) )
        ROM_LOAD64_WORD_SWAP( "mpr20613.12", 0x2800000, 0x400000, CRC(2938c0d9) SHA1(95c444ef0b53afd129d512f8a8961938b0fd703e) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr20616.26",  0x000002, 0x400000, CRC(e11dcf8b) SHA1(bbc384e95d52d883cbc68122f5a3c44dee98d54c) )
        ROM_LOAD_VROM( "mpr20617.27",  0x000000, 0x400000, CRC(96acef3f) SHA1(a72ed7ac3f8132562c3d2e9bdf7b5c596082cb29) )
        ROM_LOAD_VROM( "mpr20618.28",  0x000006, 0x400000, CRC(6c281281) SHA1(ddf056386e7e44f2d2d034f405c55bc5a3109848) )
        ROM_LOAD_VROM( "mpr20619.29",  0x000004, 0x400000, CRC(0fa65819) SHA1(b9ce62a149f019b04eb72e0d6c9339472f403823) )
        ROM_LOAD_VROM( "mpr20620.30",  0x00000a, 0x400000, CRC(ee79585f) SHA1(ea1797667a6ef6e38cf48543a63fd69a097b1e50) )
        ROM_LOAD_VROM( "mpr20621.31",  0x000008, 0x400000, CRC(3a99148f) SHA1(0a2e5b4af85a4ce370c8defeb8d81def1279e693) )
        ROM_LOAD_VROM( "mpr20622.32",  0x00000e, 0x400000, CRC(0618f056) SHA1(873063ab147b6b513683d4b4fe6a874f920ba818) )
        ROM_LOAD_VROM( "mpr20623.33",  0x00000c, 0x400000, CRC(ccf31b85) SHA1(b25ea13706734a2e5c2f7587f494d59b6ad43c1f) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr20624.34",  0x000002, 0x400000, CRC(90f30936) SHA1(43be3fe5a4200ab4200a49d571d9114e894db700) )
        ROM_LOAD_VROM( "mpr20625.35",  0x000000, 0x400000, CRC(04f804fa) SHA1(6537733d6a0c12e084937366bbdcc1b3bea3b4aa) )
        ROM_LOAD_VROM( "mpr20626.36",  0x000006, 0x400000, CRC(2d6c97d6) SHA1(201d01556e8381f21670b9637eb479e8f88e880e) )
        ROM_LOAD_VROM( "mpr20627.37",  0x000004, 0x400000, CRC(a14ee871) SHA1(c846e39760570703ec97ae085992b2de0daef458) )
        ROM_LOAD_VROM( "mpr20628.38",  0x00000a, 0x400000, CRC(bba829a3) SHA1(ad44d53e9cf5bc1f6a03fe961d5f13410261d912) )
        ROM_LOAD_VROM( "mpr20629.39",  0x000008, 0x400000, CRC(ead2eb31) SHA1(cfb2ff20d5fcdaa9e0b075f84e94d07ff069d17e) )
        ROM_LOAD_VROM( "mpr20630.40",  0x00000e, 0x400000, CRC(cc5881b8) SHA1(6d9b973c442c5d3bb872624412b8dfbef0677b34) )
        ROM_LOAD_VROM( "mpr20631.41",  0x00000c, 0x400000, CRC(5cb69ffd) SHA1(654d8187634726c8218bf84304765a40c2d4b117) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20636.21", 0x000000, 0x080000, CRC(7139ebf8) SHA1(3e06e8aa5c3eaf371073caa51e5fc5b42826f015) )

	// Samples
	ROM_REGION( 0x800000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20614.22", 0x000000, 0x400000, CRC(a3930e4a) SHA1(6a34f5b7817db8304454235997eaa453528bc655) )
        ROM_LOAD( "mpr20615.24", 0x400000, 0x400000, CRC(62e8a94a) SHA1(abed71b1c6eb2563fe58e6598c10dd266340e5e0) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */
        ROM_LOAD( "epr20641.2", 0x000000, 0x020000, CRC(c9b82035) SHA1(1e438f8104f79c2956bb1aeb710b01b6dc59101e) )

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
        ROM_LOAD( "mpr20637.57", 0x000000, 0x400000, CRC(d66e8a02) SHA1(f5d2bf4c97139fa56d14ffe2885a86e8f17ee965) )
        ROM_LOAD( "mpr20638.58", 0x400000, 0x400000, CRC(d1513382) SHA1(b4d5b7680e2e73b361530d689ffdb0bab62e9ee4) )
        ROM_LOAD( "mpr20639.59", 0x800000, 0x400000, CRC(f6603b7b) SHA1(9f31a2562168e5eba51864935e1c15db4e3114fb) )
        ROM_LOAD( "mpr20640.60", 0xc00000, 0x400000, CRC(9eea07b7) SHA1(bdcf136f29e1435c9d82718730ef209d8cfe74d8) )
ROM_END

ROM_START( harley )	/* Step 2.0 */
	ROM_REGION64_BE( 0x4800000, REGION_USER1, 0 ) /* program + data ROMs */
	// CROM
        ROM_LOAD64_WORD_SWAP( "epr20393a.17", 0x000006, 0x200000, CRC(b5646556) SHA1(4bff0e140e1d1df7459f7194aa4a335bc4592203) )
        ROM_LOAD64_WORD_SWAP( "epr20394a.18", 0x000004, 0x200000, CRC(ce29e2b6) SHA1(482aaf5480b219b8ac6e4e36a6d64359e1834f44) )
        ROM_LOAD64_WORD_SWAP( "epr20395a.19", 0x000002, 0x200000, CRC(761f4976) SHA1(3e451a99b87f05d6fa35efdd944cd4c2033dd5fd) )
        ROM_LOAD64_WORD_SWAP( "epr20396a.20", 0x000000, 0x200000, CRC(16b0106b) SHA1(3b8449805b73ff6ff1a1c8b134cd861b77b7f8d8) )

	// CROM0
        ROM_LOAD64_WORD_SWAP( "mpr20361.1",   0x800006, 0x400000, CRC(ddb66c2f) SHA1(e47450cc38af99e2870aac1f598e057a0c6efe47) )
        ROM_LOAD64_WORD_SWAP( "mpr20362.2",   0x800004, 0x400000, CRC(f7e60dfd) SHA1(e87b1a35513d6c9392643e8e97e2bc3eccdf21cb) )
        ROM_LOAD64_WORD_SWAP( "mpr20363.3",   0x800002, 0x400000, CRC(3e3cc6ff) SHA1(ae0640ac00b6a09010984197a1aeb43817576181) )
        ROM_LOAD64_WORD_SWAP( "mpr20364.4",   0x800000, 0x400000, CRC(a2a68ef2) SHA1(4d6ddd85d3350e56276c7cb11dd428d2c13a0374) )

	// CROM1
        ROM_LOAD64_WORD_SWAP( "mpr20365.5",   0x1800006, 0x400000, CRC(7dd50361) SHA1(731ed1b660ebdaf1c9d583b374181ddc1496322b) )
        ROM_LOAD64_WORD_SWAP( "mpr20366.6",   0x1800004, 0x400000, CRC(45e3850e) SHA1(54e6a285c594418355ab80cdfa4f5881cf74e39a) )
        ROM_LOAD64_WORD_SWAP( "mpr20367.7",   0x1800002, 0x400000, CRC(6c3f9748) SHA1(0bec4c32c7e5fe715bb4572bf54c559583637170) )
        ROM_LOAD64_WORD_SWAP( "mpr20368.8",   0x1800000, 0x400000, CRC(100c9846) SHA1(63b2ab000b5e4996c68724e3f3457a50e6185295) )

	// CROM3
        ROM_LOAD64_WORD_SWAP( "epr20409.13",  0x4000006, 0x200000, CRC(58caaa75) SHA1(ce27ff9109f4fd69191b6aec3298013261f657e5) )
        ROM_LOAD64_WORD_SWAP( "epr20410.14",  0x4000004, 0x200000, CRC(98b126f2) SHA1(f102ce6fdcd5c7eebc2c802b80a3ee861fc50f19) )
        ROM_LOAD64_WORD_SWAP( "epr20411.15",  0x4000002, 0x200000, CRC(848daaf7) SHA1(85486e8fbac4237a2fed31120cc15239d5037d9a) )
        ROM_LOAD64_WORD_SWAP( "epr20412.16",  0x4000000, 0x200000, CRC(0d51bb34) SHA1(24ce007e1abdd26d10b3d2a294503bf70a48db2d) )

	ROM_REGION( 0x4000000, REGION_USER2, 0 )  /* Eventually Video ROMs */

	ROM_REGION( 0x2000000, REGION_USER3, 0 )  /* Video ROMs Part 1 */
        ROM_LOAD_VROM( "mpr20377.26",  0x000002, 0x400000, CRC(4d2887e5) SHA1(6c965262bbeb8c5a10cdcf5ee31f2f50616ad6f0) )
        ROM_LOAD_VROM( "mpr20378.27",  0x000000, 0x400000, CRC(5ad7c0ec) SHA1(19eee9a512abf857e2fea40261f90d727f79961a) )
        ROM_LOAD_VROM( "mpr20379.28",  0x000006, 0x400000, CRC(1e51c9f0) SHA1(e62f6b4b5378f2724cd7ae9a0c87a9de31d0ff37) )
        ROM_LOAD_VROM( "mpr20380.29",  0x000004, 0x400000, CRC(e10d35ae) SHA1(d99615b9d6b6c89cb1f9bfeba4f3aebefbaf857c) )
        ROM_LOAD_VROM( "mpr20381.30",  0x00000a, 0x400000, CRC(76cd36a2) SHA1(026dfe1c996a923b8114defb96a3b200e2c72279) )
        ROM_LOAD_VROM( "mpr20382.31",  0x000008, 0x400000, CRC(f089ae37) SHA1(d4366617fc956b2ad653409981227f238314c7eb) )
        ROM_LOAD_VROM( "mpr20383.32",  0x00000e, 0x400000, CRC(9e96d3be) SHA1(a674544eb5e1792f82d2b58ab26e9be2cf3a4e25) )
        ROM_LOAD_VROM( "mpr20384.33",  0x00000c, 0x400000, CRC(5bdfbb52) SHA1(31470f761170e489582db36124d7740d8dbb94aa) )

	ROM_REGION( 0x2000000, REGION_USER4, 0 )  /* Video ROMs Part 2 */
        ROM_LOAD_VROM( "mpr20385.34",  0x000002, 0x400000, CRC(12db1729) SHA1(58fd946d8d63cf8b8da741a68d18c58f6894d661) )
        ROM_LOAD_VROM( "mpr20386.35",  0x000000, 0x400000, CRC(db2ccaf8) SHA1(873fc1f293a50bb98e463449c8591d2cfc3cd93f) )
        ROM_LOAD_VROM( "mpr20387.36",  0x000006, 0x400000, CRC(c5dde91b) SHA1(8155f49918cd57bc625fbc149ffdd9074f5906f0) )
        ROM_LOAD_VROM( "mpr20388.37",  0x000004, 0x400000, CRC(aeaa862e) SHA1(e78a7e446df2f15570e1dc64a5f8b22ac09ac6a0) )
        ROM_LOAD_VROM( "mpr20389.38",  0x00000a, 0x400000, CRC(49bb6593) SHA1(ffed9c3748405e4835998f644077cf8581bd00c7) )
        ROM_LOAD_VROM( "mpr20390.39",  0x000008, 0x400000, CRC(1d4a8efe) SHA1(7a4515906a351737fb5b3a221bad839e61ea03a2) )
        ROM_LOAD_VROM( "mpr20391.40",  0x00000e, 0x400000, CRC(5dc452dc) SHA1(203d5d8008bea8d2f43566fff6971b3f7add75bc) )
        ROM_LOAD_VROM( "mpr20392.41",  0x00000c, 0x400000, CRC(892208cb) SHA1(12b5309f5f66d7c2165b285e0a9710ee0d9c99f4) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* 68000 code */
        ROM_LOAD16_WORD_SWAP( "epr20397.21",  0x000000, 0x080000, CRC(5b20b54a) SHA1(26fa5aedc6ccc37f2c0879e1a0f9fbac2331e12e) )

	// Samples
	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )	/* SCSP samples */
        ROM_LOAD( "mpr20373.22",  0x000000, 0x400000, CRC(c684e8a3) SHA1(2be1db24c3b221976cbcc6ad3d8cb7c6f4e3a13e) )
        ROM_LOAD( "mpr20375.24",  0x400000, 0x400000, CRC(906ace86) SHA1(8edd2183d83897eda0578a34938b926672c21953) )
        ROM_LOAD( "mpr20374.23",  0x800000, 0x400000, CRC(fcf6ea21) SHA1(9102323cf867f9a87fe362b78d8e1be8a2809fd3) )
        ROM_LOAD( "mpr20376.25",  0xc00000, 0x400000, CRC(deeed366) SHA1(6d4809960c34865374d146605bb3e009394f7a8c) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )	/* Z80 code */

	ROM_REGION( 0x1000000, REGION_SOUND2, 0 )	/* DSB samples */
ROM_END


/* IRQs */
/*
    0x80: Unused ?
    0x40: SCSP
    0x20: Unused ?
    0x10: Network
    0x08: V-blank end ?
    0x04: ???
    0x02: V-blank start
    0x01: Unused ?
*/
//static int model3_vblank = 0;
static INTERRUPT_GEN(model3_interrupt)
{
//  if (model3_vblank == 0) {
		model3_irq_state = 0x42;
//  } else {
//      model3_irq_state = 0x0d;
//  }
	cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);

//  model3_vblank++;
//  model3_vblank &= 3;
}

static ppc_config model3_1x =
{
	PPC_MODEL_603E		/* 603e, Stretch, 1.3 */
};

static ppc_config model3_2x =
{
	PPC_MODEL_603EV		/* 603e-PID7t, Goldeneye, 2.1 */
};

static MACHINE_DRIVER_START( model3_10 )
	MDRV_CPU_ADD(PPC603, 66000000)
	MDRV_CPU_CONFIG(model3_1x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,1)

 	MDRV_INTERLEAVE(10)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model3_10)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_15 )
	MDRV_CPU_ADD(PPC603, 100000000)
	MDRV_CPU_CONFIG(model3_1x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model3_15)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_20 )
	MDRV_CPU_ADD(PPC603, 166000000)
	MDRV_CPU_CONFIG(model3_2x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model3_20)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model3_21 )
	MDRV_CPU_ADD(PPC603, 166000000)
	MDRV_CPU_CONFIG(model3_2x)
	MDRV_CPU_PROGRAM_MAP(model3_mem, 0)
 	MDRV_CPU_VBLANK_INT(model3_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(model3_21)
	MDRV_NVRAM_HANDLER(model3)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(496, 384)
	MDRV_VISIBLE_AREA(0, 495, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(model3)
	MDRV_VIDEO_UPDATE(model3)
MACHINE_DRIVER_END

static void interleave_vroms(void)
{
	int start;
	int i,j,x;
	UINT16 *vrom1 = (UINT16*)memory_region(REGION_USER3);
	UINT16 *vrom2 = (UINT16*)memory_region(REGION_USER4);
	UINT16 *vrom = (UINT16*)memory_region(REGION_USER2);
	int vrom_length = memory_region_length(REGION_USER3);

	if( vrom_length <= 0x1000000 ) {
		start = 0x1000000;
	} else {
		start = 0;
	}

	j=0;
	for(i=start; i < 0x2000000; i+=16) {
		for(x=0; x < 8; x++) {
			vrom[i+x+0] = vrom1[(j+x)^1];
		}
		for(x=0; x < 8; x++) {
			vrom[i+x+8] = vrom2[(j+x)^1];
		}
		j+=8;
	}
}

static DRIVER_INIT( model3_10 )
{
	interleave_vroms();

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_w );
}

static DRIVER_INIT( model3_15 )
{
	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc105_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc105_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc105_reg_w );
}

static DRIVER_INIT( model3_20 )
{
	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc2000000, 0xc20000ff, 0, 0, real3d_dma_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc2000000, 0xc20000ff, 0, 0, real3d_dma_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( lostwsga )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_15();
	/* TODO: there's an M68K device at 0xC0000000 - FF, maybe lightgun controls ? */
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc1000000, 0xc10000ff, 0, 0, scsi_w );

	rom[0x7374f0/4] = 0x38840004;		/* This seems to be an actual bug in the original code */

	model3_draw_crosshair = 1;
}

static DRIVER_INIT( scud )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_15();
	/* TODO: network device at 0xC0000000 - FF */
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	rom[(0x71275c^4)/4] = 0x60000000;
	rom[(0x71277c^4)/4] = 0x60000000;
}

static DRIVER_INIT( vf3 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_10();

	rom[(0x713c7c^4)/4] = 0x60000000;
	rom[(0x713e54^4)/4] = 0x60000000;
	rom[(0x7125b0^4)/4] = 0x60000000;
	rom[(0x7125d0^4)/4] = 0x60000000;

}

static DRIVER_INIT( vs215 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x70dde0^4)/4] = 0x60000000;
	rom[(0x70e6f0^4)/4] = 0x60000000;
	rom[(0x70e710^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( vs29815 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x6028ec^4)/4] = 0x60000000;
	rom[(0x60290c^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( bass )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	rom[(0x7999a8^4)/4] = 0x60000000;
	rom[(0x7999c8^4)/4] = 0x60000000;

	interleave_vroms();
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xff000000, 0xff7fffff, 0, 0, MRA64_BANK1 );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf9000000, 0xf90000ff, 0, 0, scsi_w );

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0800cf8, 0xf0800cff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfec00000, 0xfedfffff, 0, 0, mpc106_addr_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf0c00cf8, 0xf0c00cff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xfee00000, 0xfeffffff, 0, 0, mpc106_data_w );
	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xf8fff000, 0xf8fff0ff, 0, 0, mpc106_reg_w );
}

static DRIVER_INIT( vs2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x705884^4)/4] = 0x60000000;
	rom[(0x7058a4^4)/4] = 0x60000000;
}

static DRIVER_INIT( vs298 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}


static DRIVER_INIT( vs2v991 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}

static DRIVER_INIT( vs299 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);

	init_model3_20();

	rom[(0x603868^4)/4] = 0x60000000;
	rom[(0x603888^4)/4] = 0x60000000;
}

static DRIVER_INIT( harley )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	memory_install_read64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00fffff, 0, 0, network_r );
	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc0000000, 0xc00fffff, 0, 0, network_w );

	rom[(0x50fb84^4)/4] = 0x60000000;
	rom[(0x4f736c^4)/4] = 0x60000000;
	rom[(0x4f738c^4)/4] = 0x60000000;
	rom[(0x50e8d4^4)/4] = 0x60000000;
	rom[(0x50e8f4^4)/4] = 0x60000000;
}

static DRIVER_INIT( srally2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x7c0c4^4)/4] = 0x60000000;
	rom[(0x7c0c8^4)/4] = 0x60000000;
	rom[(0x7c0cc^4)/4] = 0x60000000;
}

static DRIVER_INIT( swtrilgy )
{
//  UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

/*  rom[(0xf0e48^4)/4] = 0x60000000;
    rom[(0x043dc^4)/4] = 0x48000090;
    rom[(0x029a0^4)/4] = 0x60000000;
    rom[(0x02a0c^4)/4] = 0x60000000;
    rom[(0xa36dc^4)/4] = 0x60000000;
    rom[(0xa36f4^4)/4] = 0x60000000;
    rom[(0xa3708^4)/4] = 0x60000000;
    rom[(0xa371c^4)/4] = 0x60000000;
    rom[(0xa3730^4)/4] = 0x60000000;*/
}

static DRIVER_INIT( von2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x189168^4)/4] = 0x60000000;
	rom[(0x1890ac^4)/4] = 0x60000000;
	rom[(0x1890b8^4)/4] = 0x60000000;
	rom[(0x1888a8^4)/4] = 0x60000000;
	rom[(0x1891c8^4)/4] = 0x60000000;
}

static DRIVER_INIT( dirtdvls )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	rom[(0x0600a0^4)/4] = 0x60000000;
	rom[(0x0608a4^4)/4] = 0x60000000;
	rom[(0x0608b0^4)/4] = 0x60000000;
	rom[(0x060960^4)/4] = 0x60000000;
	rom[(0x0609c0^4)/4] = 0x60000000;
	rom[(0x001e24^4)/4] = 0x60000000;
}

static DRIVER_INIT( daytona2 )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	init_model3_20();

	memory_install_write64_handler( 0, ADDRESS_SPACE_PROGRAM, 0xc3800000, 0xc3800007, 0, 0, daytona2_rombank_w );

	//rom[(0x68468c^4)/4] = 0x60000000;
	rom[(0x6063c4^4)/4] = 0x60000000;
	rom[(0x616434^4)/4] = 0x60000000;
	rom[(0x69f4e4^4)/4] = 0x60000000;
}





/* Model 3 Step 1.0 */
GAMEX( 1996, vf3,            0, model3_10, model3,        vf3, ROT0, "Sega", "Virtua Fighter 3", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, vf3tb,        vf3, model3_10, model3,  model3_10, ROT0, "Sega", "Virtua Fighter 3 Team Battle", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, bass,           0, model3_10, bass,         bass, ROT0, "Sega", "Sega Bass Fishing", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 1.5 */
GAMEX( 1996, scud,           0, model3_15, scud,         scud, ROT0, "Sega", "Scud Race (Australia)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1996, scuda,       scud, model3_15, scud,         scud, ROT0, "Sega", "Scud Race (Export)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, lostwsga,       0, model3_15, lostwsga, lostwsga, ROT0, "Sega", "The Lost World", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, vs215,        vs2, model3_15, model3,      vs215, ROT0, "Sega", "Virtua Striker 2 (Step 1.5)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, vs29815,    vs298, model3_15, model3,    vs29815, ROT0, "Sega", "Virtua Striker 2 '98 (Step 1.5)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 2.0 */
GAMEX( 1997, vs2,            0, model3_20, model3,        vs2, ROT0, "Sega", "Virtua Striker 2 (Step 2.0)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1997, harley,         0, model3_20, harley,     harley, ROT0, "Sega", "Harley-Davidson and L.A. Riders", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, srally2,        0, model3_20, model3,    srally2, ROT0, "Sega", "Sega Rally 2", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, von2,           0, model3_20, model3,       von2, ROT0, "Sega", "Virtual On 2: Oratorio Tangram", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, von254g,     von2, model3_20, model3,  model3_20, ROT0, "Sega", "Virtual On 2: Oratorio Tangram (ver 5.4g)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, vs298,          0, model3_20, model3,      vs298, ROT0, "Sega", "Virtua Striker 2 '98 (Step 2.0)", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, vs2v991,        0, model3_20, model3,    vs2v991, ROT0, "Sega", "Virtua Striker 2 '99.1", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1999, vs299,    vs2v991, model3_20, model3,      vs299, ROT0, "Sega", "Virtua Striker 2 '99", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )

/* Model 3 Step 2.1 */
GAMEX( 1998, daytona2,       0, model3_21, daytona2, daytona2, ROT0, "Sega", "Daytona USA 2", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, dirtdvls,       0, model3_21, model3,   dirtdvls, ROT0, "Sega", "Dirt Devils", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAMEX( 1998, swtrilgy,       0, model3_21, model3,   swtrilgy, ROT0, "Sega/LucasArts", "Star Wars Trilogy", GAME_NOT_WORKING | GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
