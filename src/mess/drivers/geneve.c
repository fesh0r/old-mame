/*
    MESS Driver for the Myarc Geneve 9640.
    Raphael Nabet, 2003.

    The Geneve has two operation modes.  One is compatible with the TI-99/4a,
    the other is not.


    General architecture:

    TMS9995@12MHz (including 256 bytes of on-chip 16-bit RAM and a timer),
    V9938, SN76496 (compatible with TMS9919), TMS9901, MM58274 RTC, 512 kbytes
    of 1-wait-state CPU RAM (expandable to almost 2 Mbytes), 32 kbytes of
    0-wait-state CPU RAM (expandable to 64 kbytes), 128 kbytes of VRAM
    (expandable to 192 kbytes).


    Memory map:

    64-kbyte CPU address space is mapped to a 2-Mbyte virtual address space.
    8 8-kbyte pages are available simultaneously, out of a total of 256.

    Page map (regular console):
    >00->3f: 512kbytes of CPU RAM (pages >36 and >37 are used to emulate
      cartridge CPU ROMs in ti99 mode, and pages >38 through >3f are used to
      emulate console and cartridge GROMs in ti99 mode)
    >40->7f: optional Memex RAM (except >7a and >7c that are mirrors of >ba and
      >bc?)
    >80->b7: PE-bus space using spare address lines (AMA-AMC)?  Used by RAM
        extension (Memex or PE-Bus 512k card).
    >b8->bf: PE-bus space (most useful pages are >ba: DSR space and >bc: speech
      synthesizer page; other pages are used by RAM extension)
    >c0->e7: optional Memex RAM
    >e8->ef: 64kbytes of 0-wait-state RAM (with 32kbytes of SRAM installed,
      only even pages (>e8, >ea, >ec and >ee) are available)
    >f0->f1: boot ROM
    >f2->fe: optional Memex RAM? (except >fa and >fc that are mirrors of >ba
      and >bc?)
    >ff: always empty?

    Page map (genmod console):
    >00->39, >40->b9, >bb, >bd->ef, f2->fe: Memex RAM (except >3a, >3c, >7a,
      >7c, >fa and >fc that are mirrors of >ba and >bc?) (I don't know if
      >e8->ef is on the Memex board or the Geneve motherboard)
    >ba: DSR space
    >bc: speech synthesizer space
    >f0->f1: boot ROM
    >ff: always empty?

    >00->3f: switch-selectable(?): 512kbytes of onboard RAM (1-wait-state DRAM)
      or first 512kbytes of the Memex Memory board (0-wait-state SRAM).  The
      latter setting is incompatible with TI mode.

    Note that >bc is actually equivalent to >8000->9fff on the /4a bus,
    therefore the speech synthesizer is only available at offsets >1800->1bff
    (read port) and >1c00->1fff (write port).  OTOH, if you installed a FORTI
    sound card, it will be available in the same page at offsets >0400->7ff.


    Unpaged locations (ti99 mode):
    >8000->8007: memory page registers (>8000 for page 0, >8001 for page 1,
      etc.  register >8003 is ignored (this page is hard-wired to >36->37).
    >8008: key buffer
    >8009->800f: ???
    >8010->801f: clock chip
    >8400->9fff: sound, VDP, speech, and GROM registers (according to one
      source, the GROM and sound registers are only available if page >3
      is mapped in at location >c000 (register 6).  I am not sure the Speech
      registers are implemented, though I guess they are.)
    Note that >8020->83ff is mapped to regular CPU RAM according to map
    register 4.

    Unpaged locations (native mode):
    >f100: VDP data port (read/write)
    >f102: VDP address/status port (read/write)
    >f104: VDP port 2
    >f106: VDP port 3
    >f108->f10f: VDP mirror (used by Barry Boone's converted Tomy cartridges)
    >f110->f117: memory page registers (>f110 for page 0, >f111 for page 1,
      etc.)
    >f118: key buffer
    >f119->f11f: ???
    >f120: sound chip
    >f121->f12f: ???
    >f130->f13f: clock chip

    Unpaged locations (tms9995 locations):
    >f000->f0fb and >fffc->ffff: tms9995 RAM
    >fffa->fffb: tms9995 timer register
    Note: accessing tms9995 locations will also read/write corresponding paged
      locations.


    GROM emulator:

    The GPL interface is accessible only in TI99 mode.  GPL data is fetched
    from pages >38->3f.  It uses a straight 16-bit address pointer.  The
    address pointer is incremented when the read data and write data ports
    are accessed, and when the second byte of the GPL address is written.  A
    weird consequence of this is the fact that GPL data is always off by one
    byte, i.e. GPL bytes 0, 1, 2... are stored in bytes 1, 2, 3... of pages
    >38->3f (byte 0 of page >38 corresponds to GPL address >ffff).

    I think that I have once read that the geneve GROM emulator does not
    emulate wrap-around within a GROM, i.e. address >1fff is followed by >2000
    (instead of >0000 with a real GROM).

    There are two ways to implement GPL address load and store.
    One is maintaining 2 flags (one for address read and one for address write)
    that tell if you are accessing address LSB: these flags must be cleared
    whenever data is read, and the read flag must be cleared when the write
    address port is written to.
    The other is to shift the register and always read/write the MSByte of the
    address pointer.  The LSByte is copied to the MSbyte when the address is
    read, whereas the former MSByte is copied to the LSByte when the address is
    written.  The address pointer must be incremented after the address is
    written to.  It will not harm if it is incremented after the address is
    read, provided the LSByte has been cleared.


    Cartridge emulator:

    The cartridge interface is in the >6000->7fff area.

    If CRU bit @>F7C is set, the cartridge area is always mapped to virtual
    page >36.  Writes in the >6000->6fff area are ignored if the CRU bit @>F7D
    is 0, whereas writes in the >7000->7fff area are ignored if the CRU bit
    @>F7E is 0.

    If CRU bit @>F7C is clear, the cartridge area is mapped either to virtual
    page >36 or to page >37 according to which page is currently selected.
    Writing data to address >6000->7fff will select page >36 if A14 is 0,
    >37 if A14 is 1.


    CRU map:

    Base >0000: tms9901
    tms9901 pin assignment:
    int1: external interrupt (INTA on PE-bus) (connected to tms9995 (int4/EC)*
      pin as well)
    int2: VDP interrupt
    int3-int7: joystick port inputs (fire, left, right, down, up)
    int8: keyboard interrupt (asserted when key buffer full)
    int9/p13: unused
    int10/p12: third mouse button
    int11/p11: clock interrupt?
    int12/p10: INTB from PE-bus
    int13/p9: (used as output)
    int14/p8: unused
    int15/p7: (used as output)
    p0: PE-bus reset line
    p1: VDP reset line
    p2: joystick select (0 for joystick 1, 1 for joystick 2)
    p3-p5: unused
    p6: keyboard reset line
    p7/int15: external mem cycles 0=long, 1=short
    p9/int13: vdp wait cycles 1=add 15 cycles, 0=add none

    Base >1EE0 (>0F70): tms9995 flags and geneve mode register
    bits 0-1: tms9995 flags
    Bits 2-4: tms9995 interrupt state register
    Bits 5-15: tms9995 user flags - overlaps geneve mode, but hopefully the
      geneve registers are write-only, so no real conflict happens
    TMS9995 user flags:
    Bit 5: 0 if NTSC, 1 if PAL video
    Bit 6: unused???
    Bit 7: some keyboard flag, set to 1 if caps is on
    Geneve gate array + TMS9995 user flags:
    Bit 8: 1 = allow keyboard clock
    Bit 9: 0 = clear keyboard input buffer, 1 = allow keyboard buffer to be
      loaded
    Bit 10: 1 = geneve mode, 0 = ti99 mode
    Bit 11: 1 = ROM mode, 0 = map mode
    Bit 12: 0 = Enable cartridge paging
    Bit 13: 0 = protect cartridge range >6000->6fff
    Bit 14: 0 = protect cartridge range >7000->7fff
    bit 15: 1 = add 1 extra wait state when accessing 0-wait-state SRAM???


    Keyboard interface:

    The XT keyboard interface is described in various places on the internet,
    like (http://www-2.cs.cmu.edu/afs/cs/usr/jmcm/www/info/key2.txt).  It is a
    synchronous unidirectional serial interface: the data line is driven by the
    keyboard to send data to the CPU; the CTS/clock line has a pull up resistor
    and can be driven low by both keyboard and CPU.  To send data to the CPU,
    the keyboard pulses the clock line low 9 times, and the Geneve samples all
    8 bits of data (plus one start bit) on each falling edge of the clock.
    When the key code buffer is full, the Geneve gate array asserts the kbdint*
    line (connected to 9901 INT8*).  The Geneve gate array will hold the
    CTS/clock line low as long as the keyboard buffer is full or CRU bit @>F78
    is 0.  Writing a 0 to >F79 will clear the Geneve keyboard buffer, and
    writing a 1 will resume normal operation: you need to write a 0 to >F78
    before clearing >F79, or the keyboard will be enabled to send data the gate
    array when >F79 is is set to 0, and any such incoming data from the
    keyboard will be cleared as soon as it is buffered by the gate array.
*/

#include "driver.h"
#include "cpu/tms9900/tms9900.h"
#include "sound/sn76496.h"
#include "sound/tms5220.h"
#include "deprecat.h"
#include "video/v9938.h"
#include "includes/geneve.h"
#include "machine/ti99_4x.h"
#include "machine/tms9901.h"
#include "machine/tms9902.h"
#include "audio/spchroms.h"
#include "machine/99_peb.h"
#include "machine/994x_ser.h"
#include "machine/99_dsk.h"
#include "machine/99_ide.h"
#include "devices/flopdrv.h"
#include "machine/smartmed.h"
#include "devices/harddriv.h"
#include "machine/idectrl.h"
#include "machine/smc92x4.h"
#include "machine/mm58274c.h"


/*
    memory map
*/

static ADDRESS_MAP_START(memmap, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(geneve_r, geneve_w)
ADDRESS_MAP_END


/*
    CRU map
*/

static ADDRESS_MAP_START(cru_map, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x00ff) AM_DEVREAD("tms9901", tms9901_cru_r)
	AM_RANGE(0x0100, 0x01ff) AM_READ(geneve_peb_cru_r)

	AM_RANGE(0x0000, 0x07ff) AM_DEVWRITE("tms9901", tms9901_cru_w)
	AM_RANGE(0x0800, 0x0fff) AM_WRITE(geneve_peb_mode_cru_w)
ADDRESS_MAP_END


/*
    Input ports, used by machine code for keyboard and joystick emulation.
*/

/* 101-key PC XT keyboard + TI joysticks */
static INPUT_PORTS_START(geneve)

	PORT_START("CFG")	/* config */
		PORT_DIPNAME( config_boot_mask << config_boot_bit, 0x0000, "Boot ROM")
			PORT_DIPSETTING( 0x0000, "1.0" )
			PORT_DIPSETTING( 1 << config_boot_bit, "0.9" )
		PORT_DIPNAME( config_speech_mask << config_speech_bit, 1 << config_speech_bit, "Speech synthesis")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_speech_bit, DEF_STR( On ) )
		PORT_DIPNAME( config_fdc_mask << config_fdc_bit, fdc_kind_hfdc << config_fdc_bit, "Floppy disk controller")
			PORT_DIPSETTING( fdc_kind_none << config_fdc_bit, DEF_STR( None ) )
			PORT_DIPSETTING( fdc_kind_TI << config_fdc_bit, "Texas Instruments SD" )
#if HAS_99CCFDC
			PORT_DIPSETTING( fdc_kind_CC << config_fdc_bit, "CorComp" )
#endif
			PORT_DIPSETTING( fdc_kind_BwG << config_fdc_bit, "SNUG's BwG" )
			PORT_DIPSETTING( fdc_kind_hfdc << config_fdc_bit, "Myarc's HFDC" )
		PORT_DIPNAME( config_ide_mask << config_ide_bit, /*1 << config_ide_bit*/0, "Nouspickel's IDE card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_ide_bit, DEF_STR( On ) )
		PORT_DIPNAME( config_rs232_mask << config_rs232_bit, 1 << config_rs232_bit, "TI RS232 card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_rs232_bit, DEF_STR( On ) )
		PORT_DIPNAME( config_usbsm_mask << config_usbsm_bit, 1 << config_usbsm_bit, "Nouspickel's USB-SM card")
			PORT_DIPSETTING( 0x0000, DEF_STR( Off ) )
			PORT_DIPSETTING( 1 << config_usbsm_bit, DEF_STR( On ) )

	PORT_START("JOY")	/* col 1: "wired handset 1" (= joystick 1) */
		PORT_BIT(0x0080, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(1UP)", CODE_NONE, OSD_JOY_UP*/) PORT_PLAYER(1)
		PORT_BIT(0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(1DOWN)", CODE_NONE, OSD_JOY_DOWN, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(1RIGHT)", CODE_NONE, OSD_JOY_RIGHT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(1LEFT)", CODE_NONE, OSD_JOY_LEFT, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0008, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(1FIRE)", CODE_NONE, OSD_JOY_FIRE, 0*/) PORT_PLAYER(1)
		PORT_BIT(0x0007, IP_ACTIVE_HIGH, IPT_UNUSED)
			/* col 2: "wired handset 2" (= joystick 2) */
		PORT_BIT(0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP/*, "(2UP)", CODE_NONE, OSD_JOY2_UP, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN/*, "(2DOWN)", CODE_NONE, OSD_JOY2_DOWN, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT/*, "(2RIGHT)", CODE_NONE, OSD_JOY2_RIGHT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT/*, "(2LEFT)", CODE_NONE, OSD_JOY2_LEFT, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0800, IP_ACTIVE_LOW, IPT_BUTTON1/*, "(2FIRE)", CODE_NONE, OSD_JOY2_FIRE, 0*/) PORT_PLAYER(2)
		PORT_BIT(0x0700, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START("MOUSEX") /* Mouse - X AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSEY") /* Mouse - Y AXIS */
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0) PORT_PLAYER(1)

	PORT_START("MOUSE0") /* mouse buttons */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1) PORT_NAME("mouse button 1")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2) PORT_NAME("mouse button 2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON3) PORT_NAME("mouse button 3")

	PORT_START("KEY0")	/* IN3 */
	PORT_BIT ( 0x0001, 0x0000, IPT_UNUSED ) 	/* unused scancode 0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC) /* Esc                       01  81 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1 !") PORT_CODE(KEYCODE_1) /* 1                           02  82 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2 @") PORT_CODE(KEYCODE_2) /* 2                           03  83 */
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3 #") PORT_CODE(KEYCODE_3) /* 3                           04  84 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4 $") PORT_CODE(KEYCODE_4) /* 4                           05  85 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5 %") PORT_CODE(KEYCODE_5) /* 5                           06  86 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6 ^") PORT_CODE(KEYCODE_6) /* 6                           07  87 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7 &") PORT_CODE(KEYCODE_7) /* 7                           08  88 */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8 *") PORT_CODE(KEYCODE_8) /* 8                           09  89 */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9 (") PORT_CODE(KEYCODE_9) /* 9                           0A  8A */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0 )") PORT_CODE(KEYCODE_0) /* 0                           0B  8B */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("- _") PORT_CODE(KEYCODE_MINUS) /* -                           0C  8C */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS) /* =                          0D  8D */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) /* Backspace                 0E  8E */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) /* Tab                       0F  8F */

	PORT_START("KEY1")	/* IN4 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) /* Q                         10  90 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) /* W                         11  91 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) /* E                         12  92 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) /* R                         13  93 */
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) /* T                         14  94 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) /* Y                         15  95 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) /* U                         16  96 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) /* I                         17  97 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) /* O                         18  98 */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) /* P                         19  99 */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[ {") PORT_CODE(KEYCODE_OPENBRACE) /* [                           1A  9A */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("] }") PORT_CODE(KEYCODE_CLOSEBRACE) /* ]                          1B  9B */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) /* Enter                     1C  9C */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L-Ctrl") PORT_CODE(KEYCODE_LCONTROL) /* Left Ctrl                 1D  9D */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) /* A                         1E  9E */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) /* S                         1F  9F */

	PORT_START("KEY2")	/* IN5 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) /* D                         20  A0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) /* F                         21  A1 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) /* G                         22  A2 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) /* H                         23  A3 */
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) /* J                         24  A4 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) /* K                         25  A5 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) /* L                         26  A6 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("; :") PORT_CODE(KEYCODE_COLON) /* ;                           27  A7 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("' \"") PORT_CODE(KEYCODE_QUOTE) /* '                          28  A8 */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("` ~") PORT_CODE(KEYCODE_TILDE) /* `                           29  A9 */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L-Shift") PORT_CODE(KEYCODE_LSHIFT) /* Left Shift                 2A  AA */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\ |") PORT_CODE(KEYCODE_BACKSLASH) /* \                          2B  AB */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) /* Z                         2C  AC */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) /* X                         2D  AD */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) /* C                         2E  AE */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) /* V                         2F  AF */

	PORT_START("KEY3")	/* IN6 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) /* B                         30  B0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) /* N                         31  B1 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) /* M                         32  B2 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) /* ,                           33  B3 */
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) /* .                            34  B4 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) /* /                           35  B5 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R-Shift") PORT_CODE(KEYCODE_RSHIFT) /* Right Shift                36  B6 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP * (PrtScr)") PORT_CODE(KEYCODE_ASTERISK	) /* Keypad *  (PrtSc)          37  B7 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Alt") PORT_CODE(KEYCODE_LALT) /* Left Alt                 38  B8 */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) /* Space                     39  B9 */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) /* Caps Lock                   3A  BA */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) /* F1                          3B  BB */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) /* F2                          3C  BC */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) /* F3                          3D  BD */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) /* F4                          3E  BE */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5) /* F5                          3F  BF */

	PORT_START("KEY4")	/* IN7 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6) /* F6                          40  C0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7) /* F7                          41  C1 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8) /* F8                          42  C2 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9) /* F9                          43  C3 */
	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F10") PORT_CODE(KEYCODE_F10) /* F10                       44  C4 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NumLock") PORT_CODE(KEYCODE_NUMLOCK) /* Num Lock                  45  C5 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ScrLock (F14)") PORT_CODE(KEYCODE_SCRLOCK) /* Scroll Lock             46  C6 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 7 (Home)") PORT_CODE(KEYCODE_7_PAD		) /* Keypad 7  (Home)           47  C7 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 8 (Up)") PORT_CODE(KEYCODE_8_PAD		) /* Keypad 8  (Up arrow)       48  C8 */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 9 (PgUp)") PORT_CODE(KEYCODE_9_PAD		) /* Keypad 9  (PgUp)           49  C9 */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP -") PORT_CODE(KEYCODE_MINUS_PAD) /* Keypad -                   4A  CA */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 4 (Left)") PORT_CODE(KEYCODE_4_PAD		) /* Keypad 4  (Left arrow)     4B  CB */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 5") PORT_CODE(KEYCODE_5_PAD) /* Keypad 5                   4C  CC */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 6 (Right)") PORT_CODE(KEYCODE_6_PAD		) /* Keypad 6  (Right arrow)    4D  CD */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP +") PORT_CODE(KEYCODE_PLUS_PAD) /* Keypad +                    4E  CE */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 1 (End)") PORT_CODE(KEYCODE_1_PAD		) /* Keypad 1  (End)            4F  CF */

	PORT_START("KEY5")	/* IN8 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 2 (Down)") PORT_CODE(KEYCODE_2_PAD		) /* Keypad 2  (Down arrow)     50  D0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 3 (PgDn)") PORT_CODE(KEYCODE_3_PAD		) /* Keypad 3  (PgDn)           51  D1 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP 0 (Ins)") PORT_CODE(KEYCODE_0_PAD		) /* Keypad 0  (Ins)            52  D2 */
	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KP . (Del)") PORT_CODE(KEYCODE_DEL_PAD		) /* Keypad .  (Del)            53  D3 */
	PORT_BIT ( 0x0030, 0x0000, IPT_UNUSED )
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(84/102)\\") PORT_CODE(KEYCODE_BACKSLASH2) /* Backslash 2             56  D6 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)F11") PORT_CODE(KEYCODE_F11) /* F11                      57  D7 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)F12") PORT_CODE(KEYCODE_F12) /* F12                      58  D8 */
	PORT_BIT ( 0xfe00, 0x0000, IPT_UNUSED )

	PORT_START("KEY6")	/* IN9 */
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)KP Enter") PORT_CODE(KEYCODE_ENTER_PAD) /* PAD Enter                 60  e0 */
	PORT_BIT(0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)R-Control") PORT_CODE(KEYCODE_RCONTROL) /* Right Control             61  e1 */
	PORT_BIT(0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)ALTGR") PORT_CODE(KEYCODE_RALT) /* ALTGR                     64  e4 */

	PORT_BIT(0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)KP /") PORT_CODE(KEYCODE_SLASH_PAD) /* PAD Slash                 62  e2 */

	PORT_BIT(0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Home") PORT_CODE(KEYCODE_HOME) /* Home                       66  e6 */
	PORT_BIT(0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Cursor Up") PORT_CODE(KEYCODE_UP) /* Up                          67  e7 */
	PORT_BIT(0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Page Up") PORT_CODE(KEYCODE_PGUP) /* Page Up                 68  e8 */
	PORT_BIT(0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Cursor Left") PORT_CODE(KEYCODE_LEFT) /* Left                        69  e9 */
	PORT_BIT(0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Cursor Right") PORT_CODE(KEYCODE_RIGHT) /* Right                     6a  ea */
	PORT_BIT(0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)End") PORT_CODE(KEYCODE_END) /* End                      6b  eb */
	PORT_BIT(0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Cursor Down") PORT_CODE(KEYCODE_DOWN) /* Down                        6c  ec */
	PORT_BIT(0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Page Down") PORT_CODE(KEYCODE_PGDN) /* Page Down                 6d  ed */
	PORT_BIT(0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Insert") PORT_CODE(KEYCODE_INSERT) /* Insert                     6e  ee */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Delete") PORT_CODE(KEYCODE_DEL) /* Delete                        6f  ef */

	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)PrtScr (F13)") PORT_CODE(KEYCODE_PRTSCR) /* Print Screen             63  e3 */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("(101)Pause (F15)") PORT_CODE(KEYCODE_PAUSE) /* Pause                      65  e5 */

	PORT_START("KEY7")	/* IN10 */
	PORT_BIT ( 0xffff, 0x0000, IPT_UNUSED )
#if 0
	PORT_BIT(0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Print Screen") PORT_CODE(KEYCODE_PRTSCR) /* Print Screen alternate        77  f7 */
	PORT_BIT(0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left Win") /* Left Win                    7d  fd */
	PORT_BIT(0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right Win") /* Right Win                  7e  fe */
	PORT_BIT(0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Menu") /* Menu                        7f  ff */
#endif

INPUT_PORTS_END




static const tms5220_interface geneve_tms5220interface =
{
	DEVCB_NULL,					/* no IRQ callback */
#if 1
	spchroms_read,				/* speech ROM read handler */
	spchroms_load_address,		/* speech ROM load address handler */
	spchroms_read_and_branch	/* speech ROM read and branch handler */
#endif
};

static const mm58274c_interface geneve_mm58274c_interface =
{
	1,	/*  mode 24*/
	0   /*  first day of week */
};


static const mm58274c_interface floppy_mm58274c_interface =
{
	1,	/*  mode 24*/
	0   /*  first day of week */
};

static const floppy_config geneve_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(ti99),
	KEEP_GEOMETRY
};

static MACHINE_DRIVER_START(geneve_60hz)
	/* basic machine hardware */
	/* TMS9995 CPU @ 12.0 MHz */
	MDRV_CPU_ADD("maincpu", TMS9995, 12000000)
	MDRV_CPU_PROGRAM_MAP(memmap)
	MDRV_CPU_IO_MAP(cru_map)
	MDRV_CPU_VBLANK_INT_HACK(geneve_hblank_interrupt, 262)	/* 262.5 in 60Hz, 312.5 in 50Hz */

	MDRV_MACHINE_START( geneve )
	MDRV_MACHINE_RESET( geneve )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)	/* or 50Hz */
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(512 + 32, (212 + 28) * 2)
	MDRV_SCREEN_VISIBLE_AREA(0, 512 + 32 - 1, 0, (212 + 28) * 2 - 1)

	MDRV_PALETTE_LENGTH(512)

	MDRV_PALETTE_INIT(v9938)
	MDRV_VIDEO_START(geneve)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76496", SN76496, 3579545)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.75)
	MDRV_SOUND_ADD("tms5220", TMS5220, 680000L)
	MDRV_SOUND_CONFIG(geneve_tms5220interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_IDE_CONTROLLER_ADD( "ide", ti99_ide_interrupt )	/* FIXME */

	MDRV_IMPORT_FROM( smc92x4_hd )

	MDRV_IDE_HARDDISK_ADD( "ide_harddisk" )

	/* rtc */
	MDRV_MM58274C_ADD("mm58274c", geneve_mm58274c_interface)
	MDRV_MM58274C_ADD("mm58274c_floppy", floppy_mm58274c_interface)

	/* tms9901 */
	MDRV_TMS9901_ADD("tms9901", tms9901reset_param_ti99)

	MDRV_WD179X_ADD("wd179x", ti99_wd17xx_interface )

	MDRV_FLOPPY_4_DRIVES_ADD(geneve_floppy_config)

	MDRV_SMARTMEDIA_ADD("smartmedia")

	MDRV_TI99_4_RS232_CARD_ADD("rs232")
MACHINE_DRIVER_END


/*
    ROM loading

    Note that we use the same ROMset for 50Hz and 60Hz versions.
*/

ROM_START(geneve)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, "maincpu", 0)
	ROM_LOAD("genbt100.bin", offset_rom_geneve, 0x4000, CRC(8001e386) SHA1(b44618b54dabac3882543e18555d482b299e0109)) /* CPU ROMs */
	ROM_LOAD_OPTIONAL("genbt090.bin", offset_altrom_geneve, 0x4000, CRC(b2e20df9) SHA1(2d5d09177afe97d63ceb3ad59b498b1c9e2153f7)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
#if HAS_99CCFDC
	ROM_LOAD_OPTIONAL("ccfdc.bin", offset_ccfdc_dsr, 0x4000, BAD_DUMP CRC(f69cc69d)) /* CorComp disk DSR ROM */
#endif
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

ROM_START(genmod)
	/*CPU memory space*/
	ROM_REGION(region_cpu1_len_geneve, "maincpu", 0)
	ROM_LOAD("gnmbt100.bin", offset_rom_geneve, 0x4000, CRC(19b89479) SHA1(6ef297eda78dc705946f6494e9d7e95e5216ec47)) /* CPU ROMs */

	/*DSR ROM space*/
	ROM_REGION(region_dsr_len, region_dsr, 0)
	ROM_LOAD_OPTIONAL("disk.bin", offset_fdc_dsr, 0x2000, CRC(8f7df93f) SHA1(ed91d48c1eaa8ca37d5055bcf67127ea51c4cad5)) /* TI disk DSR ROM */
#if HAS_99CCFDC
	ROM_LOAD_OPTIONAL("ccfdc.bin", offset_ccfdc_dsr, 0x4000, BAD_DUMP CRC(f69cc69d)) /* CorComp disk DSR ROM */
#endif
	ROM_LOAD_OPTIONAL("bwg.bin", offset_bwg_dsr, 0x8000, CRC(06f1ec89) SHA1(6ad77033ed268f986d9a5439e65f7d391c4b7651)) /* BwG disk DSR ROM */
	ROM_LOAD_OPTIONAL("hfdc.bin", offset_hfdc_dsr, 0x4000, CRC(66fbe0ed) SHA1(11df2ecef51de6f543e4eaf8b2529d3e65d0bd59)) /* HFDC disk DSR ROM */
	ROM_LOAD_OPTIONAL("rs232.bin", offset_rs232_dsr, 0x1000, CRC(eab382fb) SHA1(ee609a18a21f1a3ddab334e8798d5f2a0fcefa91)) /* TI rs232 DSR ROM */

	/*TMS5220 ROM space*/
	ROM_REGION(0x8000, region_speech_rom, 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE      INPUT    INIT      CONFIG  COMPANY     FULLNAME */
COMP( 1987,geneve,   0,		0,		geneve_60hz,  geneve,  geneve,	0,	"Myarc",	"Geneve 9640" , 0)
COMP( 1990,genmod,   geneve,	0,		geneve_60hz,  geneve,  genmod,	0,	"Myarc",	"Geneve 9640 (with Genmod modification)" , 0)

