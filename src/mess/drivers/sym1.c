/******************************************************************************
 Synertek Systems Corp. SYM-1

 Early driver by PeT mess@utanet.at May 2000
 Rewritten by Dirk Best October 2007

******************************************************************************/


#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "includes/sym1.h"

/* Peripheral chips */
#include "machine/6532riot.h"
#include "machine/6522via.h"
#include "machine/74145.h"

/* Layout */
#include "sym1.lh"



/* pointers to memory locations */
UINT8 *sym1_monitor;
static UINT8 *sym1_ram_1k;
static UINT8 *sym1_ram_2k;
static UINT8 *sym1_ram_3k;
static UINT8 *sym1_riot_ram;



/******************************************************************************
 Memory Maps
******************************************************************************/


static ADDRESS_MAP_START( sym1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM                              /* U12/U13 RAM */
	AM_RANGE(0x0400, 0x07ff) AM_RAMBANK(2) AM_BASE(&sym1_ram_1k)
	AM_RANGE(0x0800, 0x0bff) AM_RAMBANK(3) AM_BASE(&sym1_ram_2k)
	AM_RANGE(0x0c00, 0x0fff) AM_RAMBANK(4) AM_BASE(&sym1_ram_3k)
	AM_RANGE(0x8000, 0x8fff) AM_ROM AM_BASE(&sym1_monitor)       /* U20 Monitor ROM */
	AM_RANGE(0xa000, 0xa00f) AM_DEVREADWRITE(VIA6522, "via6522_0", via_r, via_w)      /* U25 VIA #1 */
	AM_RANGE(0xa400, 0xa40f) AM_DEVREADWRITE(RIOT6532, "riot", riot6532_r, riot6532_w)  /* U27 RIOT */
	AM_RANGE(0xa600, 0xa67f) AM_RAMBANK(5) AM_BASE(&sym1_riot_ram)  /* U27 RIOT RAM */
	AM_RANGE(0xa800, 0xa80f) AM_DEVREADWRITE(VIA6522, "via6522_1", via_r, via_w)      /* U28 VIA #2 */
	AM_RANGE(0xac00, 0xac0f) AM_DEVREADWRITE(VIA6522, "via6522_2", via_r, via_w)      /* U29 VIA #3 */
ADDRESS_MAP_END



/******************************************************************************
 Input Ports
******************************************************************************/


static INPUT_PORTS_START( sym1 )
	PORT_START("ROW-0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0     USR 0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4     USR 4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8     JUMP")  PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C     CALC")  PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CR    S DBL") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GO    LD P")  PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("LD 2  LD 1")  PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("ROW-1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1     USR 1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5     USR 5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9     VER")   PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D     DEP")   PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-     +")     PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("REG   SAV P") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SAV 2 SAV 1") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW-2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2     USR 2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6     USR 6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A     ASCII") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E     EXEC")  PORT_CODE(KEYCODE_E)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x92     \xE2\x86\x90") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MEM   WP")    PORT_CODE(KEYCODE_F5)
	PORT_BIT(0xc0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("ROW-3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3     USR 3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7     USR 7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B     B MOV") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F     FILL")  PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")       PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("IN4")			/* IN4 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEBUG OFF") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEBUG ON")  PORT_CODE(KEYCODE_F7)

	PORT_START("WP")
	PORT_DIPNAME(0x01, 0x01, "6532 RAM WP")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x01, DEF_STR(On))
	PORT_DIPNAME(0x02, 0x02, "1K RAM WP")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x02, DEF_STR(On))
	PORT_DIPNAME(0x04, 0x04, "2K RAM WP")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x04, DEF_STR(On))
	PORT_DIPNAME(0x08, 0x08, "3K RAM WP")
	PORT_DIPSETTING(0x00, DEF_STR(Off))
	PORT_DIPSETTING(0x08, DEF_STR(On))
INPUT_PORTS_END



/******************************************************************************
 Machine Drivers
******************************************************************************/


static MACHINE_DRIVER_START( sym1 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M6502, SYM1_CLOCK)  /* 1 MHz */
	MDRV_CPU_PROGRAM_MAP(sym1_map, 0)
	MDRV_MACHINE_RESET(sym1)

	MDRV_DEFAULT_LAYOUT(layout_sym1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	/* devices */
	MDRV_RIOT6532_ADD("riot", SYM1_CLOCK, sym1_r6532_interface)
	MDRV_TTL74145_ADD("ttl74145", sym1_ttl74145_intf)
	MDRV_VIA6522_ADD("via6522_0", 0, sym1_via0)
	MDRV_VIA6522_ADD("via6522_1", 0, sym1_via1)
	MDRV_VIA6522_ADD("via6522_2", 0, sym1_via2)
MACHINE_DRIVER_END



/******************************************************************************
 ROM Definitions
******************************************************************************/


ROM_START( sym1 )
	ROM_REGION(0x10000, "main", 0)
//  ROM_LOAD("basicv11", 0xc000, 0x2000, CRC(075b0bbd))
	ROM_LOAD("sym1", 0x8000, 0x1000, CRC(7a4b1e12) SHA1(cebdf815105592658cfb7af262f2101d2aeab786) )
ROM_END



/******************************************************************************
 System Config
******************************************************************************/


static SYSTEM_CONFIG_START( sym1 )
	CONFIG_RAM_DEFAULT(4 * 1024) /* 4KB RAM */
	CONFIG_RAM        (3 * 1024) /* 3KB RAM */
	CONFIG_RAM        (2 * 1024) /* 2KB RAM */
	CONFIG_RAM        (1 * 1024) /* 1KB RAM */
SYSTEM_CONFIG_END



/******************************************************************************
 Drivers
******************************************************************************/


/*    YEAR  NAME  PARENT COMPAT MACHINE INPUT INIT  CONFIG COMPANY                   FULLNAME          FLAGS */
COMP( 1978, sym1, 0,     0,     sym1,   sym1, sym1, sym1,  "Synertek Systems Corp.", "SYM-1/SY-VIM-1", 0 )
