/***************************************************************************

    mc10.c

    TRS-80 Radio Shack MicroColor Computer

    Nate Woods

***************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "video/m6847.h"
#include "includes/mc10.h"
#include "devices/cassette.h"
#include "formats/coco_cas.h"



/*****************************************************************************
 Address maps
*****************************************************************************/


static ADDRESS_MAP_START( mc10_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0100, 0x3fff) AM_NOP /* unused */
//  AM_RANGE(0x4000, 0x8fff) AM_RAM /* RAM, 4k or 20k total */
	AM_RANGE(0x9000, 0xbffe) AM_NOP /* unused */
	AM_RANGE(0xbfff, 0xbfff) AM_READWRITE(mc10_bfff_r, mc10_bfff_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM AM_REGION(REGION_CPU1, 0x0000) /* ROM */
ADDRESS_MAP_END


static ADDRESS_MAP_START( mc10_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(M6803_PORT1, M6803_PORT1) AM_READWRITE(mc10_port1_r, mc10_port1_w)
	AM_RANGE(M6803_PORT2, M6803_PORT2) AM_READWRITE(mc10_port2_r, mc10_port2_w)
ADDRESS_MAP_END



/*****************************************************************************
 Inputs
*****************************************************************************/


/* MC-10 keyboard

       PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
  PA6: Ctl N/c Brk N/c N/c N/c N/c Shift
  PA5: 8   9   :   ;   ,   -   .   /
  PA4: 0   1   2   3   4   5   6   7
  PA3: X   Y   Z   N/c N/c N/c Ent Space
  PA2: P   Q   R   S   T   U   V   W
  PA1: H   I   J   K   L   M   N   O
  PA0: @   A   B   C   D   E   F   G
 */

/*  Port                                        Key description                 Emulated key                  Natural key     Shift 1         Shift 2 (Ctrl) */
static INPUT_PORTS_START( mc10 )
	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@     INPUT")        PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A     \xE2\x86\x90") PORT_CODE(KEYCODE_A)          PORT_CHAR('A')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B     ABS")          PORT_CODE(KEYCODE_B)          PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C     INT")          PORT_CODE(KEYCODE_C)          PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D     GOSUB")        PORT_CODE(KEYCODE_D)          PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E     SET")          PORT_CODE(KEYCODE_E)          PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F     RETURN")       PORT_CODE(KEYCODE_F)          PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G     IF")           PORT_CODE(KEYCODE_G)          PORT_CHAR('G')

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H     THEN")         PORT_CODE(KEYCODE_H)          PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I     NEXT")         PORT_CODE(KEYCODE_I)          PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J     GOTO")         PORT_CODE(KEYCODE_J)          PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K     SOUND")        PORT_CODE(KEYCODE_K)          PORT_CHAR('K')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L     PEEK")         PORT_CODE(KEYCODE_L)          PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M     COS")          PORT_CODE(KEYCODE_M)          PORT_CHAR('M')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N     SIN")          PORT_CODE(KEYCODE_N)          PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O     STEP")         PORT_CODE(KEYCODE_O)          PORT_CHAR('O')

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P     INKEY$")       PORT_CODE(KEYCODE_P)          PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q     L.DEL")        PORT_CODE(KEYCODE_Q)          PORT_CHAR('Q')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R     RESET")        PORT_CODE(KEYCODE_R)          PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S     \xE2\x86\x92") PORT_CODE(KEYCODE_S)          PORT_CHAR('S')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T     READ")         PORT_CODE(KEYCODE_T)          PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U     FOR")          PORT_CODE(KEYCODE_U)          PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V     RND")          PORT_CODE(KEYCODE_V)          PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W     \xE2\x86\x91") PORT_CODE(KEYCODE_W)          PORT_CHAR('W')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X     SQN")          PORT_CODE(KEYCODE_X)          PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y     RESTORE")      PORT_CODE(KEYCODE_Y)          PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z     \xE2\x86\x93") PORT_CODE(KEYCODE_Z)          PORT_CHAR('Z')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x38, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER")              PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE")              PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(' ')

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0")                  PORT_CODE(KEYCODE_0)          PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !  RUN")          PORT_CODE(KEYCODE_1)          PORT_CHAR('1')  PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"  CONT")        PORT_CODE(KEYCODE_2)          PORT_CHAR('2')  PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #  CSAVE")        PORT_CODE(KEYCODE_3)          PORT_CHAR('3')  PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $  CLOAD")        PORT_CODE(KEYCODE_4)          PORT_CHAR('4')  PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %  NEW")          PORT_CODE(KEYCODE_5)          PORT_CHAR('5')  PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &  LIST")         PORT_CODE(KEYCODE_6)          PORT_CHAR('6')  PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '  CLEAR")        PORT_CODE(KEYCODE_7)          PORT_CHAR('7')  PORT_CHAR('\'')

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (  CLS")          PORT_CODE(KEYCODE_8)          PORT_CHAR('8')  PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )  PRINT")        PORT_CODE(KEYCODE_9)          PORT_CHAR('9')  PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":  *  END")          PORT_CODE(KEYCODE_MINUS)      PORT_CHAR(':')  PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +  POKE")         PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';')  PORT_CHAR('+')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <  TAN")          PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',')  PORT_CHAR('<')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =  STOP")         PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('-')  PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >  LOG")          PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.')  PORT_CHAR('>')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/  ?  SQR")          PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/')  PORT_CHAR('?')

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONTROL")            PORT_CODE(KEYCODE_LSHIFT)     PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK")              PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(UCHAR_MAMEKEY(CANCEL))
	PORT_BIT(0x78, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")              PORT_CODE(KEYCODE_RSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)

	PORT_INCLUDE( m6847_artifacting )
INPUT_PORTS_END


/* Alice uses an AZERTY keyboard */
/*  Port                                        Key description                 Emulated key                  Natural key     Shift 1         Shift 2 (Ctrl) */
static INPUT_PORTS_START( alice )
	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("@     INPUT")        PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q     \xE2\x86\x90") PORT_CODE(KEYCODE_A)          PORT_CHAR('Q')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B     ABS")          PORT_CODE(KEYCODE_B)          PORT_CHAR('B')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C     INT")          PORT_CODE(KEYCODE_C)          PORT_CHAR('C')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D     GOSUB")        PORT_CODE(KEYCODE_D)          PORT_CHAR('D')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E     SET")          PORT_CODE(KEYCODE_E)          PORT_CHAR('E')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F     RETURN")       PORT_CODE(KEYCODE_F)          PORT_CHAR('F')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G     IF")           PORT_CODE(KEYCODE_G)          PORT_CHAR('G')

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H     THEN")         PORT_CODE(KEYCODE_H)          PORT_CHAR('H')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I     NEXT")         PORT_CODE(KEYCODE_I)          PORT_CHAR('I')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J     GOTO")         PORT_CODE(KEYCODE_J)          PORT_CHAR('J')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K     SOUND")        PORT_CODE(KEYCODE_K)          PORT_CHAR('K')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L     PEEK")         PORT_CODE(KEYCODE_L)          PORT_CHAR('L')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/  ?  COS")          PORT_CODE(KEYCODE_M)          PORT_CHAR('/')  PORT_CHAR('?')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N     SIN")          PORT_CODE(KEYCODE_N)          PORT_CHAR('N')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O     STEP")         PORT_CODE(KEYCODE_O)          PORT_CHAR('O')

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P     INKEY$")       PORT_CODE(KEYCODE_P)          PORT_CHAR('P')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A     L.DEL")        PORT_CODE(KEYCODE_Q)          PORT_CHAR('A')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R     RESET")        PORT_CODE(KEYCODE_R)          PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S     \xE2\x86\x92") PORT_CODE(KEYCODE_S)          PORT_CHAR('S')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T     READ")         PORT_CODE(KEYCODE_T)          PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U     FOR")          PORT_CODE(KEYCODE_U)          PORT_CHAR('U')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V     RND")          PORT_CODE(KEYCODE_V)          PORT_CHAR('V')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z     \xE2\x86\x91") PORT_CODE(KEYCODE_W)          PORT_CHAR('Z')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(UP))

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X     SQN")          PORT_CODE(KEYCODE_X)          PORT_CHAR('X')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y     RESTORE")      PORT_CODE(KEYCODE_Y)          PORT_CHAR('Y')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W     \xE2\x86\x93") PORT_CODE(KEYCODE_Z)          PORT_CHAR('W')  PORT_CHAR('~')  PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x38, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER")              PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR(13)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE")              PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(' ')

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0")                  PORT_CODE(KEYCODE_0)          PORT_CHAR('0')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1  !  RUN")          PORT_CODE(KEYCODE_1)          PORT_CHAR('1')  PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2  \"  CONT")        PORT_CODE(KEYCODE_2)          PORT_CHAR('2')  PORT_CHAR('\"')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3  #  CSAVE")        PORT_CODE(KEYCODE_3)          PORT_CHAR('3')  PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4  $  CLOAD")        PORT_CODE(KEYCODE_4)          PORT_CHAR('4')  PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5  %  NEW")          PORT_CODE(KEYCODE_5)          PORT_CHAR('5')  PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6  &  LIST")         PORT_CODE(KEYCODE_6)          PORT_CHAR('6')  PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7  '  CLEAR")        PORT_CODE(KEYCODE_7)          PORT_CHAR('7')  PORT_CHAR('\'')

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8  (  CLS")          PORT_CODE(KEYCODE_8)          PORT_CHAR('8')  PORT_CHAR('(')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9  )  PRINT")        PORT_CODE(KEYCODE_9)          PORT_CHAR('9')  PORT_CHAR(')')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":  *  END")          PORT_CODE(KEYCODE_MINUS)      PORT_CHAR(':')  PORT_CHAR('*')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M     POKE")         PORT_CODE(KEYCODE_COLON)      PORT_CHAR('M')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",  <  TAN")          PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',')  PORT_CHAR('<')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-  =  STOP")         PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('-')  PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".  >  LOG")          PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.')  PORT_CHAR('>')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";  +  SQR")          PORT_CODE(KEYCODE_SLASH)      PORT_CHAR(';')  PORT_CHAR('+')

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CONTROL")            PORT_CODE(KEYCODE_LSHIFT)     PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK")              PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(UCHAR_MAMEKEY(CANCEL))
	PORT_BIT(0x78, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT")              PORT_CODE(KEYCODE_RSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)

	PORT_INCLUDE( m6847_artifacting )
INPUT_PORTS_END



/*****************************************************************************
 Machine definitions
*****************************************************************************/


static MACHINE_DRIVER_START( mc10 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6803, XTAL_3_579545MHz)  /* 0,894886 Mhz */
	MDRV_CPU_PROGRAM_MAP(mc10_mem, 0)
	MDRV_CPU_IO_MAP(mc10_io, 0)
	MDRV_SCREEN_REFRESH_RATE(60)

	MDRV_MACHINE_START(mc10)

	/* video hardware */
	MDRV_VIDEO_START(mc10)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END



/*****************************************************************************
 ROM definitions
*****************************************************************************/


ROM_START(mc10)
	ROM_REGION(0x2000, REGION_CPU1, 0)
	ROM_LOAD("mc10.rom", 0x0000, 0x2000, CRC(11fda97e) SHA1(4afff2b4c120334481aab7b02c3552bf76f1bc43))
ROM_END


ROM_START(alice)
	ROM_REGION(0x2000, REGION_CPU1, 0)
	ROM_LOAD("alice.rom", 0x0000, 0x2000, CRC(f876abe9) SHA1(c2166b91e6396a311f486832012aa43e0d2b19f8))
ROM_END



/*****************************************************************************
 Devices
*****************************************************************************/


static void mc10_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) coco_cassette_formats; break;

		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_CASSETTE_DEFAULT_STATE:		info->i = CASSETTE_PLAY; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}



/*****************************************************************************
 System config
*****************************************************************************/


SYSTEM_CONFIG_START(mc10)
	CONFIG_DEVICE( mc10_cassette_getinfo )
	CONFIG_RAM        (  4 * 1024 )   /* standard */
	CONFIG_RAM_DEFAULT( 20 * 1024 )   /* with 16K memory expansion */
SYSTEM_CONFIG_END



/*****************************************************************************
 Drivers
*****************************************************************************/


/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG   COMPANY               FULLNAME */
COMP( 1983, mc10,     0,		0,		mc10,     mc10,     0,        mc10,    "Tandy Radio Shack",  "MC-10", GAME_SUPPORTS_SAVE )
COMP( 1983, alice,    mc10,		0,		mc10,     alice,    0,        mc10,    "Matra & Hachette",	 "Alice", GAME_SUPPORTS_SAVE )