/***************************************************************************
TRS80 memory map

0000-2fff ROM                 R   D0-D7
3000-37ff ROM on Model III        R   D0-D7
          unused on Model I
37de      UART status             R/W D0-D7
37df      UART data           R/W D0-D7
37e0      interrupt latch address (lnw80 = for the realtime clock)
37e1      select disk drive 0         W
37e2      cassette drive latch address    W
37e3      select disk drive 1         W
37e4      select which cassette unit      W   D0-D1 (D0 selects unit 1, D1 selects unit 2)
37e5      select disk drive 2         W
37e7      select disk drive 3         W
37e0-37e3 floppy motor            W   D0-D3
          or floppy head select   W   D3
37e8      send a byte to printer          W   D0-D7
37e8      read printer status             R   D7
37ec-37ef FDC WD179x              R/W D0-D7
37ec      command             W   D0-D7
37ec      status              R   D0-D7
37ed      track               R/W D0-D7
37ee      sector              R/W D0-D7
37ef      data                R/W D0-D7
3800-38ff keyboard matrix         R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM               R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI

Printer: Level II usually 37e8; System80 uses port FD; Model 4 uses port F8.
Uart: TR1602, equivalent to the uart used in the Exidy Sorcerer.
System80 has non-addressable dip switches to set the UART control register.
System80 and LNW80 have non-addressable links to set the baud rate. Receive and Transmit clocks are tied together.
It is assumed that the TRS80L2 UART setup is identical to the System80, apart from the address ports used.
Due to the above, the only working emulated UART is for the Model 3.

Cassette baud rates: 	Model I level I - 250 baud
			Model I level II and all clones - 500 baud
			Model III/4 - 500 and 1500 baud selectable at boot time
			- When it says "Cass?" press L for 500 baud, or Enter otherwise.
			LNW-80 - 500 baud @1.77MHz and 1000 baud @4MHz.

I/O ports
FF:
- bits 0 and 1 are for writing a cassette
- bit 2 must be high to turn the cassette motor on, enables cassette data paths on a system-80
- bit 3 switches the display between 64 or 32 characters per line
- bit 6 remembers the 32/64 screen mode (inverted)
- bit 7 is for reading from a cassette

FE:
- bit 0 is for selecting inverse video of the whole screen on a lnw80
- bit 2 enables colour on a lnw80
- bit 3 is for selecting roms (low) or 16k hires area (high) on a lnw80
- bit 4 selects internal cassette player (low) or external unit (high) on a system-80

FD:
- Read printer status on a system-80
- Write to printer on a system-80

F9:
- UART data (write) status (read) on a system-80

F8:
- UART data (read) status (write) on a system-80
- Write to printer (Model III/4)
- Read printer status (Model III/4)

EB:
- UART data (read and write) on a Model III/4

EA:
- UART status (read and write) on a Model III/4

E9:
- UART Configuration jumpers (read) on a Model III/4
- Set baud rate (Model III/4)

E8:
- UART Modem Status register (read) on a Model III/4
- UART Master Reset (write) on a Model III/4

Model 4 - C0-CF = hard drive (optional)
	- 90-93 write sound (optional)
	- 80-8F hires graphics (optional)

About the Model II - this runs a boostrap rom, and boots a floppy. The rom is then bankswitched
out, and RAM takes its place. There is no rom basic, and no cassette support. Display sizes are
80x24 and 40x24. It could have 32k or 64k RAM. 2x RS-232C ports. 1x Parallel printer port. 1x
Expansion interface for more floppy drives. The drives use 8-inch floppies holding 0.5mb of data.
It was extremely expensive, non-standard, and not many were sold. A rom dump does not seem to exist.

About the Model 4 - This has 4 memory maps.
		We emulate Map 1 (see address map for Model 3).
		Map 2 - RAM=0..37FF, Keyboard=3800..3Bff, Video=3C00..3FFF, RAM=4000..FFFF
		Map 3 - RAM=0..F3FF, Keyboard=F400..F7FF, Video=F800..FFFF
		Map 4 - RAM=0..FFFF

About the ht1080z - This was made for schools in Hungary. Each comes with a BASIC extension roms
		which activated Hungarian features. To activate - start emulation - enter SYSTEM
		Enter /12288 and the extensions will be installed and you are returned to READY.
		The ht1080z is identical to the System 80, apart from the character rom.
		The ht1080z2 has a modified extension rom and character generator.

About the RTC - The time is incremented while ever the cursor is flashing. It is stored in a series
		of bytes in the computer's work area. The bytes are in a certain order, this is:
		seconds, minutes, hours, year, day, month. On a model 1, the seconds are stored at
		0x4041, while on the model 4 it is 0x4217. A reboot always sets the time to zero.

***************************************************************************

Not dumped (to our knowledge):
 TRS80 Japanese bios
 TRS80 Katakana Character Generator
 TRS80 Small English Character Generator
 TRS80 Model III old version Character Generator
 TRS80 Model 4P boot disk (being worked on)
 TRS80 Model II bios and boot disk

Not emulated:
 TRS80 Japanese kana/ascii switch and alternate keyboard
 TRS80 Model III/4 Hard drive, Graphics board, Alternate Character set
 LNW80 1.77 / 4.0 MHz switch (this is a physical switch)

Virtual floppy disk formats are JV1, JV3, and DMK. Only the JV1 is emulated.
There don't seem to be any JV1 boot disks for Model III/4.

***************************************************************************/

/* Core includes */
#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/wave.h"
#include "machine/ctronics.h"
#include "machine/ay31015.h"
#include "includes/trs80.h"

/* Components */
#include "machine/wd17xx.h"
#include "sound/speaker.h"

/* Devices */
#include "devices/flopdrv.h"
#include "formats/trs_dsk.h"
#include "devices/cassette.h"
#include "formats/trs_cas.h"

UINT8 *gfxram;
UINT8 trs80_model4;

static ADDRESS_MAP_START( trs80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(trs80_videoram_r, trs80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x4000, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( trs80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_ff_r, trs80_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( model1_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x377f) AM_ROM	// sys80,ht1080 needs up to 375F
	AM_RANGE(0x37de, 0x37de) AM_READWRITE(sys80_f9_r, sys80_f8_w)
	AM_RANGE(0x37df, 0x37df) AM_READWRITE(trs80m4_eb_r, trs80m4_eb_w)
	AM_RANGE(0x37e0, 0x37e3) AM_READWRITE(trs80_irq_status_r, trs80_motor_w)
	AM_RANGE(0x37e4, 0x37e7) AM_WRITE(trs80_cassunit_w)
	AM_RANGE(0x37e8, 0x37eb) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0x37ec, 0x37ec) AM_DEVREADWRITE("wd179x", trs80_wd179x_r, wd17xx_command_w)
	AM_RANGE(0x37ed, 0x37ed) AM_DEVREADWRITE("wd179x", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0x37ee, 0x37ee) AM_DEVREADWRITE("wd179x", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0x37ef, 0x37ef) AM_DEVREADWRITE("wd179x", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0x3800, 0x38ff) AM_MIRROR(0x300) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(trs80_videoram_r, trs80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( model1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_ff_r, trs80_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sys80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf8, 0xf8) AM_READWRITE(trs80m4_eb_r, sys80_f8_w)
	AM_RANGE(0xf9, 0xf9) AM_READWRITE(sys80_f9_r, trs80m4_eb_w)
	AM_RANGE(0xfd, 0xfd) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0xfe, 0xfe) AM_WRITE(sys80_fe_w)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_ff_r, trs80_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( lnw80_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( lnw80_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xe8, 0xe8) AM_READWRITE(trs80m4_e8_r, trs80m4_e8_w)
	AM_RANGE(0xe9, 0xe9) AM_READ_PORT("E9")
	AM_RANGE(0xea, 0xea) AM_READWRITE(trs80m4_ea_r, trs80m4_ea_w)
	AM_RANGE(0xeb, 0xeb) AM_READWRITE(trs80m4_eb_r, trs80m4_eb_w)
	AM_RANGE(0xfe, 0xfe) AM_READWRITE(lnw80_fe_r, lnw80_fe_w)
	AM_RANGE(0xff, 0xff) AM_READWRITE(trs80_ff_r, trs80_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( model3_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37e7) AM_ROM
	AM_RANGE(0x37e8, 0x37e9) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0x37ea, 0x37ff) AM_ROM
	AM_RANGE(0x3800, 0x38ff) AM_MIRROR(0x300) AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff) AM_READWRITE(trs80_videoram_r, trs80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x4000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( model3_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x84, 0x87) AM_WRITE(trs80m4_84_w)
	AM_RANGE(0x88, 0x89) AM_WRITE(trs80m4_88_w)
	AM_RANGE(0x90, 0x93) AM_WRITE(trs80m4_90_w)
	AM_RANGE(0xe0, 0xe3) AM_READWRITE(trs80m4_e0_r, trs80m4_e0_w)
	AM_RANGE(0xe4, 0xe4) AM_READWRITE(trs80m4_e4_r, trs80m4_e4_w)
	AM_RANGE(0xe8, 0xe8) AM_READWRITE(trs80m4_e8_r, trs80m4_e8_w)
	AM_RANGE(0xe9, 0xe9) AM_READ_PORT("E9") AM_WRITE(trs80m4_e9_w)
	AM_RANGE(0xea, 0xea) AM_READWRITE(trs80m4_ea_r, trs80m4_ea_w)
	AM_RANGE(0xeb, 0xeb) AM_READWRITE(trs80m4_eb_r, trs80m4_eb_w)
	AM_RANGE(0xec, 0xef) AM_READWRITE(trs80m4_ec_r, trs80m4_ec_w)
	AM_RANGE(0xf0, 0xf0) AM_DEVREADWRITE("wd179x", trs80_wd179x_r, wd17xx_command_w)
	AM_RANGE(0xf1, 0xf1) AM_DEVREADWRITE("wd179x", wd17xx_track_r, wd17xx_track_w)
	AM_RANGE(0xf2, 0xf2) AM_DEVREADWRITE("wd179x", wd17xx_sector_r, wd17xx_sector_w)
	AM_RANGE(0xf3, 0xf3) AM_DEVREADWRITE("wd179x", wd17xx_data_r, wd17xx_data_w)
	AM_RANGE(0xf4, 0xf4) AM_WRITE(trs80m4_f4_w)
	AM_RANGE(0xf8, 0xfb) AM_READWRITE(trs80_printer_r, trs80_printer_w)
	AM_RANGE(0xfc, 0xff) AM_READWRITE(trs80m4_ff_r, trs80m4_ff_w)
ADDRESS_MAP_END



/**************************************************************************
   w/o SHIFT                             with SHIFT
   +-------------------------------+     +-------------------------------+
   | 0   1   2   3   4   5   6   7 |     | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |   |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |  |4 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|5 | 8 | 9 | : | ; | , | - | . | / |  |5 | 8 | 9 | * | + | < | = | > | ? |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|  |6 |ENT|CLR|BRK|UP |DN |LFT|RGT|SPC|
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|  |7 |SHF|ALT|PUP|PDN|INS|DEL|CTL|END|
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
NB: row 7 contains some originally unused bits
    only the shift bit was there in the TRS80
	
2008-05 FP: 
NB2: 3:3 -> 3:7 have no correspondent keys (see 
	below) in the usual 53-keys keyboard ( + 
	12-keys keypad) for Model I, III and 4. Where 
	are the symbols above coming from?
NB3: the 12-keys keypad present in later models 
	is mapped to the corrispondent keys of the 
	keyboard: '0' -> '9', 'Enter', '.'
NB4: when it was added a 15-key keypad, there were
	three functions key 'F1', 'F2', 'F3'. I found no
	doc about their position in the matrix above, 
	but the schematics of the clone System-80 MkII
	(which had 4 function keys) put these keys in
	3:4 -> 3:7. Right now they're not implemented 
	below.

***************************************************************************/

static INPUT_PORTS_START( trs80 )
	PORT_START("CONFIG") /* IN0 */
	PORT_CONFNAME(	  0x80, 0x00,	"Floppy Disc Drives")
	PORT_CONFSETTING(	0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(	0x80, DEF_STR( On ) )
	PORT_BIT(0x7f, 0x7f, IPT_UNUSED)

	PORT_START("LINE0") /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)		PORT_CHAR('@')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)				PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) 			PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C) 			PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D) 			PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) 			PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F) 			PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G) 			PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("LINE1") /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H) 			PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I) 			PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J) 			PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K) 			PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L) 			PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) 			PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) 			PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O) 			PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("LINE2") /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) 			PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q) 			PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) 			PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) 			PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) 			PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) 			PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V) 			PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W) 			PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("LINE3") /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X) 			PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y) 			PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) 			PORT_CHAR('z') PORT_CHAR('Z')
	/* on Model I and Model III keyboards, there are only 53 keys (+ 12 keypad keys) and these are not connected:
	on Model I, they produce arrows and '_', on Model III either produce garbage or overlap with other keys;
	on Model 4 (which has a 15-key with 3 function keys) here are mapped 'F1', 'F2', 'F3'    */
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("(n/c)")

	PORT_START("LINE4") /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)				PORT_CHAR('0') PORT_CHAR(UCHAR_MAMEKEY(0_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) 			PORT_CHAR('1') PORT_CHAR('!') PORT_CHAR(UCHAR_MAMEKEY(1_PAD))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) 			PORT_CHAR('2') PORT_CHAR('"') PORT_CHAR(UCHAR_MAMEKEY(2_PAD))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) 			PORT_CHAR('3') PORT_CHAR('#') PORT_CHAR(UCHAR_MAMEKEY(3_PAD))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) 			PORT_CHAR('4') PORT_CHAR('$') PORT_CHAR(UCHAR_MAMEKEY(4_PAD))
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) 			PORT_CHAR('5') PORT_CHAR('%') PORT_CHAR(UCHAR_MAMEKEY(5_PAD))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) 			PORT_CHAR('6') PORT_CHAR('&') PORT_CHAR(UCHAR_MAMEKEY(6_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) 			PORT_CHAR('7') PORT_CHAR('\'') PORT_CHAR(UCHAR_MAMEKEY(7_PAD))

	PORT_START("LINE5") /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) 			PORT_CHAR('8') PORT_CHAR('(') PORT_CHAR(UCHAR_MAMEKEY(8_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) 			PORT_CHAR('9') PORT_CHAR(')') PORT_CHAR(UCHAR_MAMEKEY(9_PAD))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_MINUS) 		PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON) 		PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA) 		PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_EQUALS) 		PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)		PORT_CHAR('.') PORT_CHAR('>') PORT_CHAR(UCHAR_MAMEKEY(DEL_PAD))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH) 		PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("LINE6") /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)							PORT_CHAR(13) PORT_CHAR(UCHAR_MAMEKEY(ENTER_PAD))
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_HOME)		PORT_CHAR(UCHAR_MAMEKEY(F8)) // 3rd line, 1st key from right
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_END)		PORT_CHAR(UCHAR_MAMEKEY(F9)) // 1st line, 1st key from right
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP)	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)				PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	/* backspace do the same as cursor left */
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(KEYCODE_BACKSPACE)	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)				PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)				PORT_CHAR(' ')

	PORT_START("LINE7") /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT)				PORT_CHAR(UCHAR_SHIFT_1)
	/* These keys are only on a Model 4. The one marked CTL seems to be another shift key (4 in total). */
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)				PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("CTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x80, 0x00, IPT_UNUSED)
INPUT_PORTS_END

static INPUT_PORTS_START( trs80m3 )
	PORT_INCLUDE (trs80)
	PORT_START("E9")	// these are the power-on uart settings
	PORT_BIT(0x07, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x88, 0x08, "Parity")
	PORT_DIPSETTING(    0x08, DEF_STR(None))
	PORT_DIPSETTING(    0x00, "Odd")
	PORT_DIPSETTING(    0x80, "Even")
	PORT_DIPNAME( 0x10, 0x10, "Stop Bits")
	PORT_DIPSETTING(    0x10, "2")
	PORT_DIPSETTING(    0x00, "1")
	PORT_DIPNAME( 0x60, 0x60, "Bits")
	PORT_DIPSETTING(    0x00, "5")
	PORT_DIPSETTING(    0x20, "6")
	PORT_DIPSETTING(    0x40, "7")
	PORT_DIPSETTING(    0x60, "8")
INPUT_PORTS_END


static const cassette_config trs80l2_cassette_config =
{
	trs80l2_cassette_formats,
	NULL,
	CASSETTE_PLAY
};

static const ay31015_config trs80_ay31015_config =
{
	AY_3_1015,
	0.0,
	0.0,
	NULL,
	NULL,
	NULL
};

static MACHINE_DRIVER_START( trs80 )		// the original model I, level I, with no extras
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 1796000)        /* 1.796 MHz */
	MDRV_CPU_PROGRAM_MAP(trs80_map)
	MDRV_CPU_IO_MAP(trs80_io)

	MDRV_MACHINE_RESET( trs80 )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(64*FW, 16*FH)
	MDRV_SCREEN_VISIBLE_AREA(0*FW,64*FW-1,0*FH,16*FH-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START( trs80 )
	MDRV_VIDEO_UPDATE( trs80 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MDRV_CASSETTE_ADD( "cassette", default_cassette_config )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( model1 )		// model I, level II
	MDRV_IMPORT_FROM( trs80 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( model1_map)
	MDRV_CPU_IO_MAP( model1_io)
	MDRV_CPU_PERIODIC_INT(trs80_rtc_interrupt, 40)

	/* devices */
	MDRV_CASSETTE_MODIFY( "cassette", trs80l2_cassette_config )
	MDRV_QUICKLOAD_ADD("quickload", trs80_cmd, "cmd", 0.5)
	MDRV_WD179X_ADD("wd179x", trs80_wd17xx_interface )
	MDRV_CENTRONICS_ADD("centronics", standard_centronics)
	MDRV_AY31015_ADD( "tr1602", trs80_ay31015_config )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model3 )
	MDRV_IMPORT_FROM( model1 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( model3_map)
	MDRV_CPU_IO_MAP( model3_io)
	MDRV_CPU_PERIODIC_INT(trs80_rtc_interrupt, 30)

	MDRV_VIDEO_UPDATE( trs80m4 )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(80*8, 240)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,239)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( sys80 )
	MDRV_IMPORT_FROM( model1 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_IO_MAP( sys80_io)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ht1080z )
	MDRV_IMPORT_FROM( sys80 )
	MDRV_VIDEO_UPDATE( ht1080z )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( lnw80 )
	MDRV_IMPORT_FROM( model1 )
	MDRV_CPU_MODIFY( "maincpu" )
	MDRV_CPU_PROGRAM_MAP( lnw80_map)
	MDRV_CPU_IO_MAP( lnw80_io)
	MDRV_MACHINE_RESET( lnw80 )

	MDRV_PALETTE_LENGTH(8)
	MDRV_PALETTE_INIT(lnw80)
	MDRV_VIDEO_UPDATE(lnw80)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(80*FW, 16*FH)
	MDRV_SCREEN_VISIBLE_AREA(0,80*FW-1,0,16*FH-1)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( radionic )
	MDRV_IMPORT_FROM( model1 )
	MDRV_VIDEO_UPDATE( radionic )
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_SIZE(64*8, 16*16)
	MDRV_SCREEN_VISIBLE_AREA(0*8,64*8-1,0*16,16*16-1)
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("level1.rom",   0x0000, 0x1000, CRC(70d06dff) SHA1(20d75478fbf42214381e05b14f57072f3970f765))

	ROM_REGION(0x00400, "gfx1",0)
	ROM_LOAD("trs80m1.chr",  0x0000, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(trs80l2)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_SYSTEM_BIOS(0, "level2", "Radio Shack Level II Basic")
	ROMX_LOAD("trs80.z33",   0x0000, 0x1000, CRC(37c59db2) SHA1(e8f8f6a4460a6f6755873580be6ff70cebe14969), ROM_BIOS(1))
	ROMX_LOAD("trs80.z34",   0x1000, 0x1000, CRC(05818718) SHA1(43c538ca77623af6417474ca5b95fb94205500c1), ROM_BIOS(1))
	ROMX_LOAD("trs80.zl2",   0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "rsl2", "R/S L2 Basic")
	ROMX_LOAD("trs80alt.z33",0x0000, 0x1000, CRC(be46faf5) SHA1(0e63fc11e207bfd5288118be5d263e7428cc128b), ROM_BIOS(2))
	ROMX_LOAD("trs80alt.z34",0x1000, 0x1000, CRC(6c791c2d) SHA1(2a38e0a248f6619d38f1a108eea7b95761cf2aee), ROM_BIOS(2))
	ROMX_LOAD("trs80alt.zl2",0x2000, 0x1000, CRC(55b3ad13) SHA1(6279f6a68f927ea8628458b278616736f0b3c339), ROM_BIOS(2))

	ROM_REGION(0x00400, "gfx1",0)
	ROM_LOAD("trs80m1.chr",  0x0000, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(radionic)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("ep1.bin",      0x0000, 0x1000, CRC(e8908f44) SHA1(7a5a60c3afbeb6b8434737dd302332179a7fca59))
	ROM_LOAD("ep2.bin",      0x1000, 0x1000, CRC(46e88fbf) SHA1(a3ca32757f269e09316e1e91ba1502774e2f5155))
	ROM_LOAD("ep3.bin",      0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369))
	ROM_LOAD("ep4.bin",      0x3000, 0x0400, CRC(70f90f26) SHA1(cbee70da04a3efac08e50b8e3a270262c2440120))
	ROM_CONTINUE(0x3000, 0x400)
	ROM_CONTINUE(0x3000, 0x600)
	ROM_IGNORE(0x200)

	ROM_REGION(0x01000, "gfx1",0)
	ROM_LOAD("trschar.bin",  0x0000, 0x1000, CRC(02e767b6) SHA1(c431fcc6bd04ce2800ca8c36f6f8aeb2f91ce9f7))
ROM_END

ROM_START(sys80)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("sys80rom.1",   0x0000, 0x1000, CRC(8f5214de) SHA1(d8c052be5a2d0ec74433043684791d0554bf203b))
	ROM_LOAD("sys80rom.2",   0x1000, 0x1000, CRC(46e88fbf) SHA1(a3ca32757f269e09316e1e91ba1502774e2f5155))
	ROM_LOAD("trs80.zl2",    0x2000, 0x1000, CRC(306e5d66) SHA1(1e1abcfb5b02d4567cf6a81ffc35318723442369))
	/* This rom turns the system80 into the "blue label" version. SYSTEM then /12288 to activate. */
	ROM_LOAD("sys80.ext",    0x3000, 0x0800, CRC(2a851e33) SHA1(dad21ec60973eb66e499fe0ecbd469118826a715))

	ROM_REGION(0x00400, "gfx1",0)
	ROM_LOAD("trs80m1.chr",  0x0000, 0x0400, CRC(0033f2b9) SHA1(0d2cd4197d54e2e872b515bbfdaa98efe502eda7))
ROM_END

ROM_START(lnw80)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("lnw_a.bin",    0x0000, 0x0800, CRC(e09f7e91) SHA1(cd28e72efcfebde6cf1c7dbec4a4880a69e683da))
	ROM_LOAD("lnw_a1.bin",   0x0800, 0x0800, CRC(ac297d99) SHA1(ccf31d3f9d02c3b68a0ee3be4984424df0e83ab0))
	ROM_LOAD("lnw_b.bin",    0x1000, 0x0800, CRC(c4303568) SHA1(13e3d81c6f0de0e93956fa58c465b5368ea51682))
	ROM_LOAD("lnw_b1.bin",   0x1800, 0x0800, CRC(3a5ea239) SHA1(8c489670977892d7f2bfb098f5df0b4dfa8fbba6))
	ROM_LOAD("lnw_c.bin",    0x2000, 0x0800, CRC(2ba025d7) SHA1(232efbe23c3f5c2c6655466ebc0a51cf3697be9b))
	ROM_LOAD("lnw_c1.bin",   0x2800, 0x0800, CRC(ed547445) SHA1(20102de89a3ee4a65366bc2d62be94da984a156b))

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("lnw_chr.bin",  0x0000, 0x0800, CRC(c89b27df) SHA1(be2a009a07e4378d070002a558705e9a0de59389))

	ROM_REGION(0x04400, "gfx2",0)
	ROM_FILL(0, 0x4400, 0xff)	/* 0x4000 for gfxram + 0x400 for videoram */
ROM_END

ROM_START(trs80m3)
/* ROMS we have and are missing:
HAVE	TRS-80 Model III Level 1 ROM (U104)
MISSING	TRS-80 Model III Level 2 (ENGLISH) ROM A (U104) ver. CRC BBC4
MISSING	TRS-80 Model III Level 2 (ENGLISH) ROM A (U104) ver. CRC DA75
HAVE	TRS-80 Model III Level 2 (ENGLISH) ROM A (U104) ver. CRC 9639
HAVE	TRS-80 Model III Level 2 (ENGLISH) ROM B (U105) ver. CRC 407C
MISSING	TRS-80 Model III Level 2 (ENGLISH) ROM C (U106) ver. CRC 2B91 - early mfg. #80040316
MISSING	TRS-80 Model III Level 2 (ENGLISH) ROM C (U106) ver. CRC 278A - no production REV A
HAVE	TRS-80 Model III Level 2 (ENGLISH) ROM C (U106) ver. CRC 2EF8 - Manufacturing #80040316 REV B
HAVE	TRS-80 Model III Level 2 (ENGLISH) ROM C (U106) ver. CRC 2F84 - Manufacturing #80040316 REV C
MISSING	TRS-80 Model III Level 2 (ENGLISH) ROM C ver. CRC 2764 - Network III v1
HAVE	TRS-80 Model III Level 2 (ENGLISH) ROM C ver. CRC 276A - Network III v2
MISSING	TRS-80 Model III Level 2 (BELGIUM) CRC ????
Note: Be careful when dumping rom C: if dumped on the trs-80 m3 with software, bytes 0x7e8 and 0x7e9 (addresses 0x37e8, 0x0x37e9)
      will read as 0xFF 0xFF; on the original rom, these bytes are 0x00 0x00 (for eproms) or 0xAA 0xAA (for mask roms), those two bytes are used for printer status on the trs-80 and are mapped on top of the rom; This problem should be avoided by pulling the rom chips and dumping them directly.
*/
	ROM_REGION(0x10000, "maincpu",0)
	ROM_SYSTEM_BIOS(0, "trs80m3_revc", "Level 2 bios, RomC Rev C")
	ROMX_LOAD("8041364.u104", 0x0000, 0x2000, CRC(ec0c6daa) SHA1(257cea6b9b46912d4681251019ec2b84f1b95fc8), ROM_BIOS(1)) // Label: "SCM91248C // Tandy (c) 80 // 8041364 // 8134" (Level 2 bios ROM A '9639')
	ROMX_LOAD("8040332.u105", 0x2000, 0x1000, CRC(ed4ee921) SHA1(ec0a19d4b72f71e51965de63250009c3c4e4cab3), ROM_BIOS(1)) // Label: "SCM91619P // Tandy (c) 80 // 8040332 // QQ8117", (Level 2 bios ROM B '407c')
	ROMX_LOAD("8040316c.u106", 0x3000, 0x0800, CRC(c8f79433) SHA1(6f395bba822d39d3cd2b73c8ea25aab4c4c26da7), ROM_BIOS(1)) // Label: "SCM91692P // Tandy (c) 81 // 8040316-C // QQ8220" (Level 2 bios ROM C REV C '2f84')
	ROM_SYSTEM_BIOS(1, "trs80m3_revb", "Level 2 bios, RomC Rev B")
	ROMX_LOAD("8041364.u104", 0x0000, 0x2000, CRC(ec0c6daa) SHA1(257cea6b9b46912d4681251019ec2b84f1b95fc8), ROM_BIOS(2)) // Label: "SCM91248C // Tandy (c) 80 // 8041364 // 8134" (Level 2 bios ROM A '9639')
	ROMX_LOAD("8040332.u105", 0x2000, 0x1000, CRC(ed4ee921) SHA1(ec0a19d4b72f71e51965de63250009c3c4e4cab3), ROM_BIOS(2)) // Label: "SCM91619P // Tandy (c) 80 // 8040332 // QQ8117", (Level 2 bios ROM B '407c')
	ROMX_LOAD("8040316b.u106", 0x3000, 0x0800, CRC(84a5702d) SHA1(297dca756a9d3c6fd13e0fa6f93d172ff795b520), ROM_BIOS(2)) // Label: "SCM91692P // Tandy (c) 80 // 8040316B // QQ8040" (Level 2 bios ROM C REV B '2ef8')
	ROM_SYSTEM_BIOS(2, "trs80m3_n3v2", "Level 2 bios, Network III v2 (student)")
	ROMX_LOAD("8041364.u104", 0x0000, 0x2000, CRC(ec0c6daa) SHA1(257cea6b9b46912d4681251019ec2b84f1b95fc8), ROM_BIOS(3)) // Label: "SCM91248C // Tandy (c) 80 // 8041364 // 8134" (Level 2 bios ROM A '9639')
	ROMX_LOAD("8040332.u105", 0x2000, 0x1000, CRC(ed4ee921) SHA1(ec0a19d4b72f71e51965de63250009c3c4e4cab3), ROM_BIOS(3)) // Label: "SCM91619P // Tandy (c) 80 // 8040332 // QQ8117" (Level 2 bios ROM B '407c')
	ROMX_LOAD("276a.u106", 0x3000, 0x0800, CRC(7d38720a) SHA1(bef621e5ae2a8c1f9e7f6325b7841f5ab8ab7e6a), ROM_BIOS(3)) // 2716 EPROM Label: "MOD.III // ROM C // (276A)" (Network III v2 ROM C '276a')
	ROM_SYSTEM_BIOS(3, "trs80m3_l1", "Level 1 bios")
	ROMX_LOAD("8040032.u104", 0x0000, 0x1000, CRC(6418d641) SHA1(f823ab6ceb102588d27e5f5c751e31175289291c), ROM_BIOS(4) ) // Label: "8040032 // (M) QQ8028 // SCM91616P"; Silkscreen: "TANDY // (C) '80"; (Level 1 bios)

	ROM_REGION(0x00800, "gfx1",0)	/* correct for later systems; the trs80m3_l1 bios uses the non-a version of this rom, dump is pending */
	ROM_LOAD("8044316.u36", 0x0000, 0x0800, NO_DUMP) // Label: "(M) // SCM91665P // 8044316 // QQ8029" ('no-letter' revision)
	ROM_LOAD("8044316a.u36", 0x0000, 0x0800, CRC(444c8b60) SHA1(c52ee41439bd5e57c3b113ebfd61c951e2af4446)) // Label: "Tandy (C) 81 // 8044316A // 8206" (rev A)
ROM_END

// for model 4 and 4p info, see http://vt100.net/mirror/harte/Radio%20Shack/TRS-80%20Model%204_4P%20Soft%20Tech%20Ref.pdf
ROM_START(trs80m4)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("trs80m4.rom",  0x0000, 0x3800, BAD_DUMP CRC(1a92d54d) SHA1(752555fdd0ff23abc9f35c6e03d9d9b4c0e9677b)) // should be split into 3 roms, roms A, B, C, exactly like trs80m3; in fact, roms A and B are shared between both systems.

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("8044316a.u36", 0x0000, 0x0800, CRC(444c8b60) SHA1(c52ee41439bd5e57c3b113ebfd61c951e2af4446)) // according to parts catalog, this is the correct rom for both model 3 and 4
ROM_END

ROM_START(trs80m4p) // uses a completely different memory map scheme to the others; the trs-80 model 3 roms are loaded from a boot disk, the only rom on the machine is a bootloader; bootloader can be banked out of 0x0000-0x1000 space which is replaced with ram; see the tech ref pdf, pdf page 62
// Currently this fails miserably due to lack of ram banking; it does some i/o stuff, copies some of the int vectors to 0x4000, then does some more i/o stuff and fills the entire 0x4000-43ff space with 0x20 (space? is it trying to clear the screen?), then immediately afterward it executes a RET; since the stack pointer is still pointing to 0x40A2, it RETs to 0x2020, which is in unmapped, nop-filled space.
// Clearly there's some major ram-bank related stuff that isn't working at all here.
	ROM_REGION(0x10000, "maincpu",0)
	ROM_SYSTEM_BIOS(0, "trs80m4p", "Level 2 bios, gate array machine")
	ROMX_LOAD("8075332.u69", 0x0000, 0x1000, CRC(3a738aa9) SHA1(6393396eaa10a84b9e9f0cf5930aba73defc5c52), ROM_BIOS(1)) // Label: "SCM95060P // 8075332 // TANDY (C) 1983 // 8421" at location U69 (may be located at U70 on some pcb revisions)
	ROM_SYSTEM_BIOS(1, "trs80m4p_hack", "Disk loader hack")
	ROMX_LOAD("trs80m4p_loader_hack.rom", 0x0000, 0x01f8, CRC(7ff336f4) SHA1(41184f5240b4b54f3804f5a22b4d78bbba52ed1d), ROM_BIOS(2))

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("8049007.u103", 0x0000, 0x0800, CRC(1ac44bea) SHA1(c9426ab2b2aa5380dc97a7b9c048ccd1bbde92ca)) // Label: "SCM95987P // 8049007 // TANDY (C) 1983 // 8447" at location U103 (may be located at U43 on some pcb revisions)
ROM_END

ROM_START(ht1080z)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("ht1080z.rom",  0x0000, 0x3000, CRC(2bfef8f7) SHA1(7a350925fd05c20a3c95118c1ae56040c621be8f))
	ROM_LOAD("sys80.ext",    0x3000, 0x0800, CRC(2a851e33) SHA1(dad21ec60973eb66e499fe0ecbd469118826a715))

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht1080z.chr",  0x0000, 0x0800, CRC(e8c59d4f) SHA1(a15f30a543e53d3e30927a2e5b766fcf80f0ae31))
ROM_END

ROM_START(ht1080z2)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("ht1080z.rom",  0x0000, 0x3000, CRC(2bfef8f7) SHA1(7a350925fd05c20a3c95118c1ae56040c621be8f))
	ROM_LOAD("ht1080z2.ext", 0x3000, 0x0800, CRC(07415ac6) SHA1(b08746b187946e78c4971295c0aefc4e3de97115))

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht1080z2.chr", 0x0000, 0x0800, CRC(6728f0ab) SHA1(1ba949f8596f1976546f99a3fdcd3beb7aded2c5))
ROM_END

ROM_START(ht108064)
	ROM_REGION(0x10000, "maincpu",0)
	ROM_LOAD("ht108064.rom", 0x0000, 0x3000, CRC(48985a30) SHA1(e84cf3121f9e0bb9e1b01b095f7a9581dcfaaae4))
	ROM_LOAD("ht108064.ext", 0x3000, 0x0800, CRC(fc12bd28) SHA1(0da93a311f99ec7a1e77486afe800a937778e73b))

	ROM_REGION(0x00800, "gfx1",0)
	ROM_LOAD("ht108064.chr", 0x0000, 0x0800, CRC(e76b73a4) SHA1(6361ee9667bf59d50059d09b0baf8672fdb2e8af))
ROM_END

static DRIVER_INIT( trs80 )
{
	trs80_mode = 0;
	trs80_model4 = 0;
}

static DRIVER_INIT( trs80l2 )
{
	trs80_mode = 2;
	trs80_model4 = 0;
}

static DRIVER_INIT( trs80m4 )
{
	trs80_mode = 0;
	trs80_model4 = 1;
}

static DRIVER_INIT( lnw80 )
{
	trs80_mode = 0;
	trs80_model4 = 0;
	gfxram = memory_region(machine, "gfx2");
	videoram = memory_region(machine, "gfx2")+0x4000;
}

static void trs8012_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 4; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_trs80; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}

static SYSTEM_CONFIG_START(trs8012)
	CONFIG_DEVICE(trs8012_floppy_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT  COMPAT  MACHINE	  INPUT    INIT      CONFIG       COMPANY  FULLNAME */
COMP( 1977, trs80,    0,	0,	trs80,    trs80,   trs80,    0,		"Tandy Radio Shack",  "TRS-80 Model I (Level I Basic)" , 0 )
COMP( 1978, trs80l2,  trs80,	0,	model1,   trs80,   trs80l2,  trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (Level II Basic)" , 0 )
COMP( 1983, radionic, trs80,	0,	radionic, trs80,   trs80,    trs8012,	"Komtek",  "Radionic" , 0 )
COMP( 1980, sys80,    trs80,	0,	sys80,    trs80,   trs80l2,  trs8012,	"EACA Computers Ltd.","System-80" , 0 )
COMP( 1981, lnw80,    trs80,	0,	lnw80,    trs80m3, lnw80,    trs8012,	"LNW Research","LNW-80", 0 )
COMP( 1980, trs80m3,  trs80,	0,	model3,   trs80m3, trs80m4,  trs8012,	"Tandy Radio Shack",  "TRS-80 Model III", 0 )
COMP( 1980, trs80m4,  trs80,	0,	model3,   trs80m3, trs80m4,  trs8012,	"Tandy Radio Shack",  "TRS-80 Model 4", 0 )
COMP( 1983, trs80m4p, trs80,	0,	model3,   trs80m3, trs80m4,  trs8012,	"Tandy Radio Shack",  "TRS-80 Model 4P", GAME_NOT_WORKING )
COMP( 1983, ht1080z,  trs80,	0,	ht1080z,  trs80,   trs80l2,  trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z Series I" , 0 )
COMP( 1984, ht1080z2, trs80,	0,	ht1080z,  trs80,   trs80l2,  trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z Series II" , 0 )
COMP( 1985, ht108064, trs80,	0,	ht1080z,  trs80,   trs80,    trs8012,	"Hiradastechnika Szovetkezet",  "HT-1080Z/64" , 0 )
