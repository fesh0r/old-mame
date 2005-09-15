/******************************************************************************
 Synertek SYM1
 (kim1 variant)
 PeT mess@utanet.at May 2000
******************************************************************************/

#include "driver.h"

#define VERBOSE_DBG 0
#include "includes/cbm.h"
#include "includes/riot6532.h"
#include "machine/6522via.h"

#include "includes/sym1.h"

static ADDRESS_MAP_START( readmem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03ff) AM_READ( MRA8_RAM )
	AM_RANGE( 0x8000, 0x8fff) AM_READ( MRA8_ROM )
	AM_RANGE( 0xa000, 0xa00f) AM_READ( via_0_r )
	AM_RANGE( 0xa400, 0xa40f) AM_READ( riot_0_r )
	AM_RANGE( 0xa600, 0xa67f) AM_READ( MRA8_RAM )
//	{ 0xab00, 0xab0f, via_1_r },
//	{ 0xac00, 0xac0f, via_2_r },
	AM_RANGE( 0xf000, 0xffff) AM_READ( MRA8_ROM )
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x03ff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x8000, 0x8fff) AM_WRITE( MWA8_ROM )
	AM_RANGE( 0xa000, 0xa00f) AM_WRITE( via_0_w )
	AM_RANGE( 0xa400, 0xa40f) AM_WRITE( riot_0_w )
	AM_RANGE( 0xa600, 0xa67f) AM_WRITE( MWA8_RAM )
//	{ 0xab00, 0xab0f, via_1_w },
//	{ 0xac00, 0xac0f, via_2_w },
	AM_RANGE( 0xf000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

INPUT_PORTS_START( sym1 )
	PORT_START			/* IN0 */
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("0     USR0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("4     USR4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("8     JUMP") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("C     CALC") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("CR    S DBL") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("GO    LD P") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("LD 2  LD 1") PORT_CODE(KEYCODE_F1)
	PORT_BIT (0x80, 0x80, IPT_UNUSED )

	PORT_START			/* IN1 */
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("1     USR1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("5     USR5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("9     VER") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("D     DEP") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("-     +") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("REG   SAV P") PORT_CODE(KEYCODE_F4)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("SAV 2 SAV 1") PORT_CODE(KEYCODE_F2)
	PORT_BIT (0x80, 0x80, IPT_UNUSED )

	PORT_START			/* IN2 */
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("2     USR2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("6     USR6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("A     ASCII") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("E     EXEC") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("Right Left") PORT_CODE(KEYCODE_RIGHT) PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("MEM   WP") PORT_CODE(KEYCODE_F5)
	PORT_BIT (0xc0, 0xc0, IPT_UNUSED )

	PORT_START			/* IN3 */
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("3     USR3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("7     USR7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("B     B MOV") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("F     FILL") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT (0xe0, 0xe0, IPT_UNUSED )

	PORT_START			/* IN4 */
	PORT_BIT(0x80, 0x80, IPT_KEYBOARD) PORT_NAME("DEBUG OFF") PORT_CODE(KEYCODE_F6)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("DEBUG ON") PORT_CODE(KEYCODE_F7)

	PORT_DIPNAME(0x03, 0x03, "RAM")
	PORT_DIPSETTING( 0x00, "1 KBYTE")
	PORT_DIPSETTING( 0x01, "2 KBYTE")
	PORT_DIPSETTING( 0x02, "3 KBYTE")
	PORT_DIPSETTING( 0x03, "4 KBYTE")
INPUT_PORTS_END



static MACHINE_DRIVER_START( sym1 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6502, 1000000)        /* 1 MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(sym1_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( sym1 )

	/* video hardware (well, actually there was no video ;) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(800, 600)
	MDRV_VISIBLE_AREA(0, 800-1, 0, 600-1)
/*	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 640-1, 0, 480-1) */
	MDRV_PALETTE_LENGTH( 242*3 + 32768 )
	MDRV_PALETTE_INIT( sym1 )

	MDRV_VIDEO_START( sym1 )
	MDRV_VIDEO_UPDATE( sym1 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(sym1)
	ROM_REGION(0x10000,REGION_CPU1, 0)
//	ROM_LOAD("basicv11", 0xc000, 0x2000, CRC(075b0bbd))
	ROM_LOAD("sym1", 0x8000, 0x1000, CRC(7a4b1e12))
	ROM_RELOAD(0xf000, 0x1000)
ROM_END

static void sym1_cbmcartslot_getinfo(struct IODevice *dev)
{
	cbmcartslot_device_getinfo(dev);
	dev->file_extensions = "60\00080\0c0\0";
}

SYSTEM_CONFIG_START(sym1)
	CONFIG_DEVICE(sym1_cbmcartslot_getinfo)
#if 0
	CONFIG_DEVICE(kim1_cassette_getinfo)
#endif
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT	COMPAT	MACHINE   INPUT     INIT	CONFIG	COMPANY   FULLNAME */
COMPX( 1978, sym1,	  0, 		0,		sym1,	  sym1, 	sym1,	sym1,	"Synertek Systems Corp",  "SYM-1/SY-VIM-1", GAME_NOT_WORKING)