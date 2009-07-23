/***************************************************************************

        DEC VT100 driver by Miodrag Milanovic

        29/04/2009 Preliminary driver.

        TODO: keyboard doesn't work properly, kb uart comms issue?
        TODO: vt100 gives a '2' error on startup indicating bad nvram checksum
              adding the serial nvram support should fix this
        TODO: support for the on-AVO character set roms
        TODO: finish support for the on-cpu board alternate character set rom

****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "sound/speaker.h"
#include "video/vtvideo.h"
#include "vt100.lh"

static UINT8 *vt100_ram;
static UINT8 vt100_keyboard_int;
static UINT8 vt100_receiver_int;
static UINT8 vt100_vertical_int;

static double vt100_send_baud_rate;
static double vt100_recv_baud_rate;

static ADDRESS_MAP_START(vt100_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE( 0x0000, 0x1fff ) AM_ROM  // ROM ( 4 * 2K)
    AM_RANGE( 0x2000, 0x2bff ) AM_RAM AM_BASE(&vt100_ram) // Screen and scratch RAM
    AM_RANGE( 0x2c00, 0x2fff ) AM_RAM  // AVO Screen RAM
    AM_RANGE( 0x3000, 0x3fff ) AM_RAM  // AVO Attribute RAM (4 bits wide)
    // 0x4000, 0x7fff is unassigned
    AM_RANGE( 0x8000, 0x9fff ) AM_ROM  // Program memory expansion ROM (4 * 2K)
    AM_RANGE( 0xa000, 0xbfff ) AM_ROM  // Program memory expansion ROM (1 * 8K)
    // 0xc000, 0xffff is unassigned
ADDRESS_MAP_END

// 0 - XMIT flag H
// 1 - Advance Video L
// 2 - Graphics Flag L
// 3 - Option present H
// 4 - Even field L
// 5 - NVR data H
// 6 - LBA 7 H
// 7 - Keyboard TBMT H
static READ8_HANDLER(vt100_flags_r)
{
	UINT8 retVal = 0;	
 	retVal |= lba7_r(devtag_get_device(space->machine, "vt100_video"),0) * 0x40;	
	retVal |= vt100_keyboard_int * 0x80;	
	return retVal;
}

static UINT8 vt100_key_scan = 0;

static TIMER_CALLBACK(keyboard_callback)
{
	int i;
	static const char *const keynames[] = { "LINE0", "LINE1", "LINE2", "LINE3", 
											"LINE4", "LINE5", "LINE6", "LINE7", 
											"LINE8", "LINE9", "LINEA", "LINEB", 
											"LINEC", "LINED", "LINEE", "LINEF" };
	UINT8 code;
	if (vt100_key_scan == 1) {
		for(i = 0; i < 16; i++) 
		{
			code = 	input_port_read(machine, keynames[i]);
			if (code!=0xff) {
				vt100_keyboard_int = 1;
				cputag_set_input_line(machine, "maincpu", 0, HOLD_LINE);
			}
		}
	}
}


static WRITE8_HANDLER(vt100_keyboard_w)
{
	
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	
	output_set_value("online_led",BIT(data,5) ? 0 : 1);
	output_set_value("local_led", BIT(data,5));
	output_set_value("locked_led",BIT(data,4) ? 0 : 1);
	output_set_value("l1_led", 	  BIT(data,3) ? 0 : 1);
	output_set_value("l2_led", 	  BIT(data,2) ? 0 : 1);
	output_set_value("l3_led", 	  BIT(data,1) ? 0 : 1);
	output_set_value("l4_led", 	  BIT(data,0) ? 0 : 1);	
	vt100_key_scan = BIT(data,6);
	speaker_level_w(speaker, BIT(data,7));
}

static READ8_HANDLER(vt100_keyboard_r)
{
	return 0x7f;
}
static WRITE8_HANDLER(vt100_baud_rate_w)
{
	double baud_rate[] = { 50, 75, 110, 134.5, 150, 200, 300, 600, 1200, 
		1800, 2000, 2400, 3600, 4800, 9600, 19200 };
	
	vt100_send_baud_rate = baud_rate[(data >>4) & 0x0f];
	vt100_recv_baud_rate = baud_rate[data & 0x0f];
}

static WRITE8_HANDLER(vt100_nvr_latch_w)
{
}

static ADDRESS_MAP_START( vt100_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	// 0x00, 0x01 PUSART  (Intel 8251)	
	// AM_RANGE (0x00, 0x01) 
	// 0x02 Baud rate generator	
	AM_RANGE (0x02, 0x02) AM_WRITE(vt100_baud_rate_w)
	// 0x22 Modem buffer
	// AM_RANGE (0x22, 0x22) 
	// 0x42 Flags buffer
	AM_RANGE (0x42, 0x42) AM_READ(vt100_flags_r)
	// 0x42 Brightness D/A latch
	AM_RANGE (0x42, 0x42) AM_DEVWRITE("vt100_video", vt_video_brightness_w)
	// 0x62 NVR latch
	AM_RANGE (0x62, 0x62) AM_WRITE(vt100_nvr_latch_w)
	// 0x82 Keyboard UART data output
	AM_RANGE (0x82, 0x82) AM_READ(vt100_keyboard_r)
	// 0x82 Keyboard UART data input
	AM_RANGE (0x82, 0x82) AM_WRITE(vt100_keyboard_w)
	// 0xA2 Video processor DC012
	AM_RANGE (0xa2, 0xa2) AM_DEVWRITE("vt100_video", dc012_w)
	// 0xC2 Video processor DC011
	AM_RANGE (0xc2, 0xc2) AM_DEVWRITE("vt100_video", dc011_w)
	// 0xE2 Graphics port
	// AM_RANGE (0xe2, 0xe2) 
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( vt100 )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED) 
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 7") PORT_CODE(KEYCODE_7_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 8") PORT_CODE(KEYCODE_8_PAD)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num .") PORT_CODE(KEYCODE_DEL_PAD)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 9") PORT_CODE(KEYCODE_9_PAD)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num Enter") PORT_CODE(KEYCODE_ENTER_PAD)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num ,") PORT_CODE(KEYCODE_PLUS_PAD)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 3") PORT_CODE(KEYCODE_3_PAD)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF1") PORT_CODE(KEYCODE_F1)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("PF2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 2") PORT_CODE(KEYCODE_2_PAD)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 5") PORT_CODE(KEYCODE_5_PAD)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 6") PORT_CODE(KEYCODE_6_PAD)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED) 
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_F6)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 0") PORT_CODE(KEYCODE_0_PAD)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 1") PORT_CODE(KEYCODE_1_PAD)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num 4") PORT_CODE(KEYCODE_4_PAD)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Num -") PORT_CODE(KEYCODE_MINUS_PAD)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("~") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line feed") PORT_CODE(KEYCODE_RALT)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_START("LINE9")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_START("LINEA")
		PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
		PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
		PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)
		PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("No scroll") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_START("LINEB")
		PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Setup") PORT_CODE(KEYCODE_F5)
	PORT_START("LINEC")
		PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_START("LINED")
		PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT)
	PORT_START("LINEE")
		PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Caps lock") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_START("LINEF")
		PORT_BIT(0x7F, IP_ACTIVE_LOW,  IPT_UNUSED)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_UNUSED) // Always return 0x7f on last scan line
INPUT_PORTS_END

static VIDEO_UPDATE( vt100 )
{
	const device_config	*devconf = devtag_get_device(screen->machine, "vt100_video");
	vt_video_update( devconf, bitmap, cliprect);
	return 0;
}


//Interrupts
// in latch A3 - keyboard
//          A4 - receiver
//          A5 - vertical fequency
//          all other set to 1
static IRQ_CALLBACK(vt100_irq_callback)
{	
	UINT8 retVal = 0xc7 | (0x08 * vt100_keyboard_int) | (0x10 * vt100_receiver_int) | (0x20 * vt100_vertical_int);
	vt100_receiver_int = 0;
	return retVal;
} 

static MACHINE_RESET(vt100) 
{
	vt100_keyboard_int = 0;
	vt100_receiver_int = 0;
	vt100_vertical_int = 0;
	output_set_value("online_led",1);
	output_set_value("local_led", 1);
	output_set_value("locked_led",1);
	output_set_value("l1_led", 	  1);
	output_set_value("l2_led", 	  1);
	output_set_value("l3_led", 	  1);
	output_set_value("l4_led", 	  1);	
	
	vt100_key_scan = 0;
	timer_pulse(machine, ATTOTIME_IN_HZ(240), NULL, 0, keyboard_callback);	
	
	cpu_set_irq_callback(cputag_get_cpu(machine, "maincpu"), vt100_irq_callback);	
}

static READ8_DEVICE_HANDLER (vt100_read_video_ram_r )
{
	return vt100_ram[offset];
}

static WRITE8_DEVICE_HANDLER (vt100_clear_video_interrupt)
{
	vt100_vertical_int = 0;	
}

static const vt_video_interface vt100_video_interface = {
	"screen",
	"gfx1",
	DEVCB_HANDLER(vt100_read_video_ram_r),
	DEVCB_HANDLER(vt100_clear_video_interrupt)
};

static INTERRUPT_GEN( vt100_vertical_interrupt )
{
	vt100_vertical_int = 1;
	cpu_set_input_line(device, 0, HOLD_LINE);
}

#define XTAL_24_8832MHz	 24883200

static MACHINE_DRIVER_START( vt100 )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",8080, XTAL_24_8832MHz / 9)
	MDRV_CPU_PROGRAM_MAP(vt100_mem)
	MDRV_CPU_IO_MAP(vt100_io)
	MDRV_CPU_VBLANK_INT("screen", vt100_vertical_interrupt)

	MDRV_MACHINE_RESET(vt100)
	
    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*10, 30*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 80*10-1, 0, 30*10-1)
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(monochrome_green)

	MDRV_DEFAULT_LAYOUT( layout_vt100 )
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(vt100)
	
	MDRV_VT100_VIDEO_ADD("vt100_video", vt100_video_interface)
	
	/* audio hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END

/* VT1xx models:
 * VT100 - 1978 base model. the 'later' rom is from 1979 or 1980.
 *    The vt100 had a whole series of -XX models branching off of it; the
       ones I know of are described here, as well as anything special about
	   them:
 *    VT100-AA - standard model with 120vac cable \__voltage can be switched
 *    VT100-AB - standard model with 240vac cable /  inside any VT100 unit
 *    VT100-W* - word processing series:
 *     VT100-WA/WB - has special LA120 AVO board preinstalled, WP romset?,
        English WP keyboard, no alt charset rom, LA120? 23-069E2 AVO rom.
       (The WA and WB variants are called the '-02' variant on the schematics)
 *    VT100-WC thru WZ: foreign language word processing series:
      (The WC thru WK variants are called the '-03' variant on the schematics)
 *     VT100-WC/WD - has AVO board preinstalled, WP romset?, French Canadian
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WE/WF - has AVO board preinstalled, WP romset?, French
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WG/WH - has AVO board preinstalled, WP romset?, Dutch
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WJ/WK - has AVO board preinstalled, WP romset?, German
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
 *     VT100-WY/WZ - has AVO board preinstalled, WP romset?, English
        WP keyboard, has 23-094E2 alt charset rom, 23-093E2 AVO rom.
       The WP romset supports english, french, dutch and german languages but
        will only display text properly in the non-english languages if the
        23-094E2 alt charset rom AND the foreign language 23-093E2
        AVO rom are populated.
 *    VT100-NA/NB - ? romset with DECFORM keycaps
 *    VT100 with vt1xx-ac kit - adds serial printer interface (SPI)
       pcb, replaces roms with the 095e2/096e2/139e2/140e2 SPI set
 * VT101 - 1981 cost reduced unexpandable vt100; Is the same as a stock
   unexpanded vt100. It has no AVO nor the upgrade connector for it, and no
   video input port.) Has its own firmware.
 * VT102 - 1981 cost reduced unexpandable vt100 with built in AVO and SPI
   Is the same as a stock vt100 with the AVO and SPI expansions installed,
   but all on one pcb. Does NOT support the AVO extended character roms, nor
   the word processing rom set. Has its own firmware.
 * VT103 - 1980 base model vt100 with an integrated TU58 tape drive, and an
   LSI-11 backplane, which an LSI-11 cpu card is used in, hence the computer
   is effectively a tiny lsi-11 (pdp-11) built in a vt100 case. uses same roms
   as vt100 for the vt100 portion, and tu58 has its own cpu and rom. It can
   have the normal vt100 romset variant, and also can have the multiple word
   processing variations (which use the same roms as the vt100 ones do).
 * VT104 doesn't exist.
 * VT105 - 1978 vt100 with the WG waveform generator board installed 
   (for simple chart-type line-compare-made raster graphics using some built
   in functions), AVO optional; was intended for use on the MINC analog data
   acquisition computer.
 * VT110 - 1978 vt100 with a DPM01 DECDataway serial multiplexer installed
    The DPM01 supposedly has its own processor and roms.
 * vt125 - 1982? base model (stock vt100 firmware plus extra gfx board
   firmware and processor) vt100 with the ReGIS graphical language board
   (aka GPO) installed (very advanced full framebuffer raster graphics, with
   color, text rotation and scaling, etc. , has backwards compatibility mode
   for vt105), AVO optional
 * vt131 - 1982 cost reduced version of vt132, no longer has the vt100
   expansion backplane; has the AVO advanced video board built in, as well
   as the parallel port interface board, and supports serial block mode.
 * vt132 - 1980? base vt100 with AVO, SPI, and its own 23-099e2/23-100e2
   AVO character rom set. Has its own base firmware roms which support block
   serial mode.
 * vt180 - 1980 vt10x (w/vt100 expansion backplane) with a z80 daughterboard
   installed; 
   The daughterboard has two roms on it: 23-017e3-00 and 23-021e3-00
   (both are 0x1000 long, 2332 mask roms)
 * vk100 'gigi'- graphical terminal similar to the vt125, but somewhat more
   primitive; more or less a vt125 without the screen; 
   very little info so far, more research needed
   see vk100.c for current driver for this
   
 * Upgrade kits for vt1xx:
 * VT1xx-AA : 20ma current loop interface pcb for vt100
 * VT1xx-AB : AVO board (AVO roms could be optionally ordered along with 
              this board if needed)
 * VT1xx-AC : SPT serial printer board (includes a special romset)
 * VT1xx-CA : later 20ma current loop interface pcb for vt100, vt101/vt102/vt131
 * VT1xx-CB or CL: GPO "ReGIS" board vt100->vt125 upgrade kit
 
 * Info about mask roms and other nasties:
 * A normal 2716 rom has pin 18: /CE; pin 20: /OE; pin 21: VPP (acts as CE2)
 * The vt100 23-031e2/23-061e2, 23-032e2, 23-033e2, and 23-034e2 mask roms
   have the follwing enables:
       23-031e2/23-061e2: pin 18:  CS2; pin 20:  CS1; pin 21:  CS3
       23-032e2:          pin 18: /CS2; pin 20:  CS1; pin 21:  CS3
       23-033e2:          pin 18:  CS2; pin 20:  CS1; pin 21: /CS3
       23-034e2:          pin 18: /CS2; pin 20:  CS1; pin 21: /CS3
       (This is cute because it technically means the roms can be put in the
       4 sockets in ANY ORDER and will still work properly since the cs2 and
       cs3 pins make them self-decode and activate at their proper address)
       (This same cute trick is almost certainly also done with the
       23-180e2, 181e2, 182e2 183e2 romset, as well as the
       23-095e2,096e2,139e2,140e2 set and probably others as well)
 * The vt100/103 23-018e2-00 character set rom at location e4 is a 24 pin 2316 mask rom with enables as such: pin 18: CS2; pin 20: /CS1; pin 21: /CS3
 * The optional 23-094e2-00 alternate character set rom at location e9 is a 24 pin 2316 mask rom with enables as such: pin 18: /CS2; pin 20: /CS1; pin 21: /CS3
       Supposedly the 23-094e2 rom is meant for vt100-WC or -WF systems, (which are french canadian and french respectively), implying that it has european language specific accented characters on it. It is probably used in all the -W* systems.
       Pin 21 can be jumpered to +5v for this socket at location e9 by removing jumper w4 and inserting jumper w5, allowing a normal 2716 eprom to be used.
 * The optional AVO character set roms (see below) have: pin 18: /CS2*; pin 20: /CS1; pin 21: CS3 hence they match a normal 2716
   *(this is marked on the image as if it was CS2 but the input is tied to gnd meaning it must be /CS2)
  
 * The AVO itself can hold up to four character set roms on it (see http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf 
   and http://vt100.net/dec/ek-vt1ac-ug-002.pdf )
   at least eight of these AVO roms were made, and are used as such:
 * No roms - normal vt100 system with AVO installed
 * 23-069E2 (location e21) - meant for vt100-wa and -wb 'LA120' 'word processing' systems (the mapping of the rom for this system is different than for the ones below)
 * 23-099E2 (location e21) and 23-100E2 (location e17) - meant for vt132
 * 23-093E2 (location e21) - meant for vt100 wc thru wz 'foreign language' word processing systems
 * 23-184E2 and 23-185E2 - meant for vt100 with STP printer option board installed, version 1, comes with vt1xx-ac kit
 * 23-186E2 and 23-187E2 - meant for vt100 with STP printer option board installed, version 2, comes with vt1xx-ac kit
 */

/* ROM definition */
ROM_START( vt100 ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated
// This romset is also used for the vt103, vt105, vt125, and ?vt132?
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	//ROM_LOAD( "23-031e2-00.e56", 0x0000, 0x0800, NO_DUMP) // version 1 1978 'earlier rom', dump needed, correct for earlier vt100s
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1979 or 1980 'later rom', correct for later vt100s
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ("23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional ?word processing? alternate character set rom
ROM_END

ROM_START( vt100wp ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt100/MP00633_VT100_Mar80.pdf
// This is the standard vt100 cpu board, with the ?word processing? romset
// the vt103 can also use this rom set (-04 and -05 revs have it by default, -05 rev also has the optional alt charset rom by default)
// NOTE: according to dunnington's ROMList, this is actually the VT132 romset, but I'm not convinced.
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-180e2-00.e56", 0x0000, 0x0800, NO_DUMP) 
	ROM_LOAD( "23-181e2-00.e52", 0x0800, 0x0800, NO_DUMP) 
	ROM_LOAD( "23-182e2-00.e45", 0x1000, 0x0800, NO_DUMP) 
	ROM_LOAD( "23-183e2-00.e40", 0x1800, 0x0800, NO_DUMP) 
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional ?word processing? alternate character set rom
ROM_END

ROM_START( vt100spt ) // This is from the VT180 technical manual at http://www.bitsavers.org/pdf/dec/terminal/vt180/EK-VT18X-TM-001_VT180_Technical_Man_Feb83.pdf
// This is the standard vt100 cpu board, but with the rom set included with the VT1xx-AC kit  
// which is only used when the STP 'parallel port expansion' card is installed into the terminal board.
// This romset adds the Set-up C page to the setup menu (press keypad 5 twice once you hit set-up)
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-095e2-00.e56", 0x0000, 0x0800, NO_DUMP) 
	ROM_LOAD( "23-096e2-00.e52", 0x0800, 0x0800, NO_DUMP) 
	ROM_LOAD( "23-139e2-00.e45", 0x1000, 0x0800, NO_DUMP) // revision 2?; revision 1 is 23-097e2
	ROM_LOAD( "23-140e2-00.e40", 0x1800, 0x0800, NO_DUMP) // revision 2?; revision 1 is 23-098e2
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional ?word processing? alternate character set rom
ROM_END

/*
ROM_START( vt101 ) // This is from the schematics at http://www.mainecoon.com/classiccmp/VT1XX-EB/
// It is 8085 based instead of 8080; I don't know exactly what vtxxx model it is from, maybe the 101/102/131
// It can be populated with two banks of two eproms each, each bank either contains 2k or 4k eproms depending on the w2/w3 and w4/w5 jumpers.
// It also has two proms on the cpu board, which the original vt100 does not.
// I can't tell for sure if its the vt101 or not since the vt101 schematics and tech manual are not scanned yet.
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-003e3-00.e10", 0x0000, 0x1000, NO_DUMP) // "EPROM 0" bank 0
	ROM_LOAD( "23-004e3-00.e4", 0x1000, 0x1000, NO_DUMP) // "EPROM 1" bank 0
	ROM_LOAD( "23-005e3-00.e9", 0x2000, 0x1000, NO_DUMP) // "EPROM 2" bank 1
	ROM_LOAD( "23-006e3-00.e3", 0x3000, 0x1000, NO_DUMP) // "EPROM 3" bank 1
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("23-???e2-00.e34", 0x0000, 0x0800, NO_DUMP) // ? gfx rom
	ROM_REGION(0x0400, "proms",0)
	ROM_LOAD ("23-312a1-07.e26", 0x0000, 0x0200, NO_DUMP) // "PROM A"; handles 8085 i/o? mapping (usart, timer, dma, comm, etc)
	ROM_LOAD ("23-313a1-07.e15", 0x0200, 0x0200, NO_DUMP) // "PROM B"; handles firmware rom mapping and memory size/page select; bit 0 = ram page, bits 1-3 unused, bits 4-7 select one eprom each
ROM_END
*/

ROM_START( vt103 ) // This is from the schematics at http://www.bitsavers.org/pdf/dec/terminal/vt103/MP00731_VT103_Aug80.pdf
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with an
// LSI-11 backplane (instead of a normal VT100 one, hence it cannot use the AVO, WG, or VT180 Z80 boards) and 
// DEC TU58 dual 256k tape drive integrated; It was intended that you would put an LSI-11 cpu card in there, which
// Would talk to the terminal as its input/output device. Several LSI-11 cpu cards were available?
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
	ROM_REGION(0x0800, "tapecpu",0) // rom for the 8085 cpu in the integrated serial tu58-xa drive
	ROM_LOAD ( "23-089e2-00.e1", 0x0000, 0x0800, NO_DUMP)
	ROM_REGION(0x80000, "lsi11cpu",0) // rom for the LSI-11 cpu board
	ROM_LOAD_OPTIONAL ( "unknown.bin", 0x00000, 0x80000, NO_DUMP)
ROM_END

ROM_START( vt105 ) // This is from anecdotal evidence and vt100.net, as the vt105 schematics are not scanned
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// WG waveform generator board factory installed; this makes the terminal act like a vt55 with vt100 terminal capability
// The VT105 was intended for use on the MINC analog data acquisition computer
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END

ROM_START( vt110 ) 
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// DECDataway DPM01 board, which adds 4 or 5 special network-addressable 50ohm? current loop serial lines
// and may add its own processor and ram to control them. see http://bitsavers.org/pdf/dec/terminal/EK-VT110_UG-001_Dec78.pdf
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
//DECDataway board roms go here!
ROM_END

ROM_START( vt125 ) // This is from bitsavers and vt100.net, as the vt125 schematics are not scanned
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// special ReGIS cpu+ram card installed which provides a framebuffer, text rotation, custom ram fonts, and many other features.
// Comes with a custom STP card as well.
// VT125 upgrade kit (upgrade from vt100 or vt105) was called VT1xx-CB or CL
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END

ROM_START( vt131 ) // VT101/VT102 'unupgradable/low cost' board with built in AVO and SPI, and vt132-style block serial mode
// ROMS have the set up page C in them
// Probably 8085 based instead of 8080
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-225e4-00.bin", 0x0000, 0x2000, BAD_DUMP CRC(7ddf75cb) SHA1(a3530bb562b5c6ea8ba23f0988b35ef404abcb93)) // A11 stuck high
	ROM_LOAD( "23-226e4-00.bin", 0x2000, 0x2000, BAD_DUMP CRC(339d4e4e) SHA1(f1b08f2c6bbc2b234f3f43bd800a2615f6dd18d3)) // A11 stuck high
	ROM_REGION(0x0800, "gfx1",0)
	ROM_LOAD ("23-280e2-00.bin", 0x0000, 0x0800, BAD_DUMP CRC(71b4172e) SHA1(5a82c7dc313bb92b9829eb8350840e072825a797)) // called "VT131 ROM" in the vt101 quick reference guide; CS floating/stuck low (contents are almost total garbage)
// This board may have proms on it as well
ROM_END

/*
ROM_START( vt132 ) // This is from anecdotal evidence and vt100.net, as the vt132 schematics are not scanned
// VT100 board with ? roms, AVO, SPI, custom firmware with block serial mode; dunnington insists these roms
// should be the 23-180e2/181e2/182e2/183e2 set, but I'm not convinced.
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-???e2-00.e56", 0x0000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-???e2-00.e52", 0x0800, 0x0800, NO_DUMP)
	ROM_LOAD( "23-???e2-00.e45", 0x1000, 0x0800, NO_DUMP)
	ROM_LOAD( "23-???e2-00.e40", 0x1800, 0x0800, NO_DUMP)
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
ROM_END
*/
ROM_START( vt180 ) 
// This is the standard VT100 cpu board with the 'normal' roms (but later rev of eprom 0) populated but with a
// Z80 daughterboard added to the expansion slot, and replacing the SPT adapter (SPT roms are replaced with the normal set)
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "23-061e2-00.e56", 0x0000, 0x0800, CRC(3dae97ff) SHA1(e3437850c33565751b86af6c2fe270a491246d15)) // version 2 1980 'later rom'
	ROM_LOAD( "23-032e2-00.e52", 0x0800, 0x0800, CRC(3d86db99) SHA1(cdd8bdecdc643442f6e7d2c83cf002baf8101867))
	ROM_LOAD( "23-033e2-00.e45", 0x1000, 0x0800, CRC(384dac0a) SHA1(22aaf5ab5f9555a61ec43f91d4dea3029f613e64))
	ROM_LOAD( "23-034e2-00.e40", 0x1800, 0x0800, CRC(4643184d) SHA1(27e6c19d9932bf13fdb70305ef4d806e90d60833))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD ( "23-018e2-00.e4", 0x0000, 0x0800, BAD_DUMP CRC(6958458b) SHA1(103429674fc01c215bbc2c91962ae99231f8ae53)) // probably correct but needs redump
	ROM_LOAD_OPTIONAL ( "23-094e2-00.e9", 0x0800, 0x0800, NO_DUMP) // optional (comes default with some models) alternate character set rom
	ROM_REGION(0x10000, "z80cpu",0) // z80 daughterboard
	ROM_LOAD ( "23-017e3-00.bin", 0x0000, 0x1000, NO_DUMP) // fix location once dumped
	ROM_LOAD ( "23-021e3-00.bin", 0x0000, 0x1000, NO_DUMP) // fix location once dumped
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1978, vt100,  0,       0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT100",		GAME_NOT_WORKING)
COMP( 1978, vt100wp,  vt100,       0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT100-Wx",		GAME_NOT_WORKING)
COMP( 1978, vt100spt,  vt100,       0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT100 w/VT1xx-AC SPT",		GAME_NOT_WORKING)
//COMP( 1980, vt101,  0,       0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT101",		GAME_NOT_WORKING)
//COMP( 1980, vt102,  0,       0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT102",		GAME_NOT_WORKING)
COMP( 1981, vt103,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT103",		GAME_NOT_WORKING)
COMP( 1978, vt105,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT105",		GAME_NOT_WORKING)
COMP( 1978, vt110,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT110",		GAME_NOT_WORKING)
COMP( 1981, vt125,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT125",		GAME_NOT_WORKING)
COMP( 1981, vt131,  0,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT131",		GAME_NOT_WORKING)
//COMP( 1979, vt132,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT125",		GAME_NOT_WORKING)
COMP( 1983, vt180,  vt100,   0, 	vt100, 		vt100, 	 0,  	  0,  	 "DEC",   "VT180",		GAME_NOT_WORKING)
