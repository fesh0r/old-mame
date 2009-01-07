/***************************************************************************
Acorn Atom:

Memory map.

CPU: 65C02
        0000-00ff Zero page
        0100-01ff Stack
        0200-1fff RAM (expansion)
        0a00-0a04 FDC 8271
        2000-21ff RAM (dos catalogue buffer)
        2200-27ff RAM (dos seq file buffer)
        2800-28ff RAM (float buffer)
        2900-7fff RAM (text RAM)
        8000-97ff VDG 6847
        9800-9fff RAM (expansion)
        a000-afff ROM (extension)
        b000-b003 PPIA 8255
        b003-b7ff NOP
        b800-bbff VIA 6522
        bc00-bfdf NOP
        bfe0-bfe2 MOUSE - extension??
        bfe3-bfff NOP
        c000-cfff ROM (basic)
        d000-dfff ROM (float)
        e000-efff ROM (dos)
        f000-ffff ROM (kernel)

Video:      MC6847

Sound:      Buzzer
Floppy:     FDC8271

Hardware:   PPIA 8255

    output  b000    0 - 3 keyboard row, 4 - 7 graphics mode
            b002    0 cas output, 1 enable 2.4kHz, 2 buzzer, 3 colour set

    input   b001    0 - 5 keyboard column, 6 CTRL key, 7 SHIFT key
            b002    4 2.4kHz input, 5 cas input, 6 REPT key, 7 60 Hz input

            VIA 6522


    DOS:

    The original location of the 8271 memory mapped registers is 0xa00-0x0a04.
    (This is the memory range assigned by Acorn in their design.)

    This is in the middle of the area for expansion RAM. Many Atom owners
    thought this was a bad design and have modified their Atom's and dos rom
    to use a different memory area.

    The atom driver in MESS uses the original memory area.



***************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/m6502/m6502.h"

/* Components */
#include "machine/centroni.h"
#include "machine/i8271.h"
#include "machine/8255ppi.h"
#include "machine/6522via.h"
#include "video/m6847.h"

/* Devices */
#include "devices/basicdsk.h"
#include "devices/flopdrv.h"
#include "devices/printer.h"
#include "devices/cassette.h"
#include "devices/snapquik.h"

#include "includes/atom.h"


/* functions */

/* port i/o functions */

/* memory w/r functions */

static ADDRESS_MAP_START( atom_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x09ff) AM_RAM
	AM_RANGE(0x0a00, 0x0a04) AM_READWRITE(atom_8271_r, atom_8271_w)
	AM_RANGE(0x0a05, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0x97ff) AM_RAM AM_BASE(&videoram) /* VDG 6847 */
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xb000, 0xb003) AM_DEVREADWRITE( PPI8255, "ppi8255", ppi8255_r, ppi8255_w) /* PPIA 8255 */
	AM_RANGE(0xb800, 0xbbff) AM_DEVREADWRITE(VIA6522, "via6522_0", via_r, via_w)			/* VIA 6522 */
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xdfff) AM_ROM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( atomeb_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x09ff) AM_RAM
	AM_RANGE(0x0a00, 0x0a04) AM_READWRITE(atom_8271_r, atom_8271_w)
	AM_RANGE(0x0a05, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0x97ff) AM_RAM AM_BASE(&videoram) AM_SIZE(&videoram_size) /* VDG 6847 */
	AM_RANGE(0x9800, 0x9fff) AM_RAM
	AM_RANGE(0xa000, 0xafff) AM_READ(SMH_BANK1)	/* eprom data from eprom box */
	AM_RANGE(0xb000, 0xb003) AM_DEVREADWRITE( PPI8255, "ppi8255", ppi8255_r, ppi8255_w) /* PPIA 8255 */
	AM_RANGE(0xb800, 0xbbff) AM_DEVREADWRITE(VIA6522, "via6522_0", via_r, via_w)			/* VIA 6522 */
	AM_RANGE(0xbfff, 0xbfff) AM_READWRITE(atom_eprom_box_r, atom_eprom_box_w)
	AM_RANGE(0xc000, 0xcfff) AM_ROM
	AM_RANGE(0xd000, 0xdfff) AM_ROM
	AM_RANGE(0xe000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END


/* graphics output */

/* keyboard input */

/* Not implemented: BREAK */

static INPUT_PORTS_START (atom)
	PORT_START("vblank") /* VBLANK */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START("KEY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_3)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_MINUS)      PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_G)          PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Q)          PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc")          PORT_CODE(KEYCODE_TILDE)      PORT_CHAR(27)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_2)          PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_F)          PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Z)          PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\x95") PORT_CODE(KEYCODE_UP)         PORT_CHAR(UCHAR_MAMEKEY(UP)) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_1)          PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_E)          PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_Y)          PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x87\x94") PORT_CODE(KEYCODE_RIGHT)      PORT_CHAR(UCHAR_MAMEKEY(RIGHT)) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_0)          PORT_CHAR('0')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_D)          PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_N)          PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_X)          PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Lock")         PORT_CODE(KEYCODE_LCONTROL)   PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK)) PORT_TOGGLE
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete")       PORT_CODE(KEYCODE_DEL)        PORT_CHAR(8)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_9)          PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_C)          PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_M)          PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_W)          PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR('^')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Copy")         PORT_CODE(KEYCODE_TAB)        PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_8)          PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_B)          PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_L)          PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_V)          PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR(']')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return")       PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(13)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_7)          PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_A)          PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_K)          PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_U)          PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('\\')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_6)          PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('@')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_J)          PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_T)          PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR('[')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_5)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_I)          PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_S)          PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space")        PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(32)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_4)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_H)          PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD)                           PORT_CODE(KEYCODE_R)          PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEY10") /* CTRL & SHIFT */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl")         PORT_CODE(KEYCODE_CAPSLOCK)   PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift")        PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)

	PORT_START("KEY11") /* REPT key */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rept")         PORT_CODE(KEYCODE_RCONTROL)   PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/* sound output */

/* machine definition */
static MACHINE_DRIVER_START( atom )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", M65C02, 1000000)        /* 0,894886 MHz */
	MDRV_CPU_PROGRAM_MAP(atom_mem, 0)

	MDRV_MACHINE_RESET( atom )

	MDRV_PPI8255_ADD( "ppi8255", atom_8255_int )

	/* video hardware */
	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_REFRESH_RATE(M6847_PAL_FRAMES_PER_SECOND)

	MDRV_VIDEO_START(atom)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_PRINTER_ADD("printer")

	/* quickload */
	MDRV_QUICKLOAD_ADD(atom, "atm", 0)

	/* cassette */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )

	/* via */
	MDRV_VIA6522_ADD("via6522_0", 1000000, atom_6522_interface)

	/* i8271 */
	MDRV_I8271_ADD("i8271", atom_8271_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( atomeb )
	MDRV_IMPORT_FROM( atom )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP(atomeb_mem, 0 )

	MDRV_MACHINE_RESET( atomeb )
MACHINE_DRIVER_END


ROM_START (atom)
	ROM_REGION (0x10000, "main",0)
	ROM_LOAD ("akernel.rom", 0xf000, 0x1000, CRC(c604db3d) SHA1(2621f27d652d4673e0a79aa669e729b8c3051ab6))
	ROM_LOAD ("dosrom.rom", 0xe000, 0x1000, CRC(c431a9b7) SHA1(71ea0a4b8d9c3caf9718fc7cc279f4306a23b39c))
	ROM_LOAD ("afloat.rom", 0xd000, 0x1000, CRC(81d86af7) SHA1(ebcde5b36cb3a3344567cbba4c7b9fde015f4802))
	ROM_LOAD ("abasic.rom", 0xc000, 0x1000, CRC(43798b9b) SHA1(a8ea19f10d4c98fbc1b666e5968f06d46af9a84c))
ROM_END

ROM_START (atomeb)
	ROM_REGION (0x10000+0x09000, "main",0)
	ROM_LOAD ("akernel.rom", 0xf000, 0x1000, CRC(c604db3d) SHA1(2621f27d652d4673e0a79aa669e729b8c3051ab6))
	ROM_LOAD ("dosrom.rom", 0xe000, 0x1000, CRC(c431a9b7) SHA1(71ea0a4b8d9c3caf9718fc7cc279f4306a23b39c))
	ROM_LOAD ("afloat.rom", 0xd000, 0x1000, CRC(81d86af7) SHA1(ebcde5b36cb3a3344567cbba4c7b9fde015f4802))
	ROM_LOAD ("abasic.rom", 0xc000, 0x1000, CRC(43798b9b) SHA1(a8ea19f10d4c98fbc1b666e5968f06d46af9a84c))
	/* roms from another oric emulator */
	ROM_LOAD ("axr1.rom",0x010000,0x1000, CRC(868fda8b) SHA1(f8417787c28818a7646b9b59d706ef890255049f))
	ROM_LOAD ("pcharme.rom",0x011000,0x1000, CRC(9e8bd79f) SHA1(66c57622448b448aa6080911dccb03456d0e3b81))
	ROM_LOAD ("gags.rom",0x012000,0x1000, CRC(35e1d713) SHA1(94cc2887ad9fea1849d1d53c64d0668e77696ef4))
	ROM_LOAD ("werom.rom",0x013000,0x1000, CRC(dfcb3bf8) SHA1(85a19146844da2d6f03e1cde37ee17429eedeb0d))
	ROM_LOAD ("unknown.rom",0x014000,0x1000, CRC(013b8f93) SHA1(b4341f116a6d1e0cbcd39d64e0b5d14a90dc0356))
	ROM_LOAD ("combox.rom",0x015000,0x1000, CRC(9c8210ab) SHA1(ea293f49a98721cdbdf985d6f2fe636290ef0e75))
	ROM_LOAD ("salfaa.rom",0x016000,0x1000, CRC(ef857b25) SHA1(b3812427233060972fa01faf3ce381a21576a5ed))
	ROM_LOAD ("mousebox.rom",0x017000,0x01000, CRC(0dff30e4) SHA1(b7c0b9c23fcc5cfdc06cb2d2a9e7c2658e248ef7))
	ROM_LOAD ("atomicw.rom",0x018000,0x1000, CRC(a3fd737d) SHA1(d418d9322c69c49106ed2c268ad0864c0f2c4c1b))
ROM_END


static void atom_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME(atom_floppy); break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "ssd"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(atom)
	CONFIG_DEVICE(atom_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT      CONFIG   COMPANY   FULLNAME */
COMP( 1979, atom,     0,        0,		atom,     atom,     0,        atom,    "Acorn",  "Atom" , 0)
COMP( 1979, atomeb,   atom,     0,		atomeb,   atom,     0,        atom,    "Acorn",  "Atom with Eprom Box" , 0)
