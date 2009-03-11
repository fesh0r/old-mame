/***************************************************************************

	NOTE: ****** Specbusy: press N, R, or E to boot *************


        Spectrum/Inves/TK90X etc. memory map:

    CPU:
        0000-3fff ROM
        4000-ffff RAM

        Spectrum 128/+2/+2a/+3 memory map:

        CPU:
                0000-3fff Banked ROM/RAM (banked rom only on 128/+2)
                4000-7fff Banked RAM
                8000-bfff Banked RAM
                c000-ffff Banked RAM

        TS2068 memory map: (Can't have both EXROM and DOCK active)
        The 8K EXROM can be loaded into multiple pages.

    CPU:
                0000-1fff     ROM / EXROM / DOCK (Cartridge)
                2000-3fff     ROM / EXROM / DOCK
                4000-5fff \
                6000-7fff  \
                8000-9fff  |- RAM / EXROM / DOCK
                a000-bfff  |
                c000-dfff  /
                e000-ffff /


Interrupts:

Changes:

29/1/2000   KT -    Implemented initial +3 emulation.
30/1/2000   KT -    Improved input port decoding for reading and therefore
            correct keyboard handling for Spectrum and +3.
31/1/2000   KT -    Implemented buzzer sound for Spectrum and +3.
            Implementation copied from Paul Daniel's Jupiter driver.
            Fixed screen display problems with dirty chars.
            Added support to load .Z80 snapshots. 48k support so far.
13/2/2000   KT -    Added Interface II, Kempston, Fuller and Mikrogen
            joystick support.
17/2/2000   DJR -   Added full key descriptions and Spectrum+ keys.
            Fixed Spectrum +3 keyboard problems.
17/2/2000   KT -    Added tape loading from WAV/Changed from DAC to generic
            speaker code.
18/2/2000   KT -    Added tape saving to WAV.
27/2/2000   KT -    Took DJR's changes and added my changes.
27/2/2000   KT -    Added disk image support to Spectrum +3 driver.
27/2/2000   KT -    Added joystick I/O code to the Spectrum +3 I/O handler.
14/3/2000   DJR -   Tape handling dipswitch.
26/3/2000   DJR -   Snapshot files are now classifed as snapshots not
            cartridges.
04/4/2000   DJR -   Spectrum 128 / +2 Support.
13/4/2000   DJR -   +4 Support (unofficial 48K hack).
13/4/2000   DJR -   +2a Support (rom also used in +3 models).
13/4/2000   DJR -   TK90X, TK95 and Inves support (48K clones).
21/4/2000   DJR -   TS2068 and TC2048 support (TC2048 Supports extra video
            modes but doesn't have bank switching or sound chip).
09/5/2000   DJR -   Spectrum +2 (France, Spain), +3 (Spain).
17/5/2000   DJR -   Dipswitch to enable/disable disk drives on +3 and clones.
27/6/2000   DJR -   Changed 128K/+3 port decoding (sound now works in Zub 128K).
06/8/2000   DJR -   Fixed +3 Floppy support
10/2/2001   KT  -   Re-arranged code and split into each model emulated.
            Code is split into 48k, 128k, +3, tc2048 and ts2048
            segments. 128k uses some of the functions in 48k, +3
            uses some functions in 128, and tc2048/ts2048 use some
            of the functions in 48k. The code has been arranged so
            these functions come in some kind of "override" order,
            read functions changed to use  READ8_HANDLER and write
            functions changed to use WRITE8_HANDLER.
            Added Scorpion256 preliminary.
18/6/2001   DJR -   Added support for Interface 2 cartridges.
xx/xx/2001  KS -    TS-2068 sound fixed.
            Added support for DOCK cartridges for TS-2068.
            Added Spectrum 48k Psycho modified rom driver.
            Added UK-2086 driver.
23/12/2001  KS -    48k machines are now able to run code in screen memory.
                Programs which keep their code in screen memory
                like monitors, tape copiers, decrunchers, etc.
                works now.
                Fixed problem with interrupt vector set to 0xffff (much
            more 128k games works now).
                A useful used trick on the Spectrum is to set
                interrupt vector to 0xffff (using the table
                which contain 0xff's) and put a byte 0x18 hex,
                the opcode for JR, at this address. The first
                byte of the ROM is a 0xf3 (DI), so the JR will
                jump to 0xfff4, where a long JP to the actual
                interrupt routine is put. Due to unideal
                bankswitching in MAME this JP were to 0001 what
                causes Spectrum to reset. Fixing this problem
                made much more software runing (i.e. Paperboy).
            Corrected frames per second value for 48k and 128k
            Sincalir machines.
                There are 50.08 frames per second for Spectrum
                48k what gives 69888 cycles for each frame and
                50.021 for Spectrum 128/+2/+2A/+3 what gives
                70908 cycles for each frame.
            Remaped some Spectrum+ keys.
                Presing F3 to reset was seting 0xf7 on keyboard
                input port. Problem occured for snapshots of
                some programms where it was readed as pressing
                key 4 (which is exit in Tapecopy by R. Dannhoefer
                for example).
            Added support to load .SP snapshots.
            Added .BLK tape images support.
                .BLK files are identical to .TAP ones, extension
                is an only difference.
08/03/2002  KS -    #FF port emulation added.
                Arkanoid works now, but is not playable due to
                completly messed timings.

Initialisation values used when determining which model is being emulated:
 48K        Spectrum doesn't use either port.
 128K/+2    Bank switches with port 7ffd only.
 +3/+2a     Bank switches with both ports.

Notes:
 1. No contented memory.
 2. No hi-res colour effects (need contended memory first for accurate timing).
 3. Multiface 1 and Interface 1 not supported.
 4. Horace and the Spiders cartridge doesn't run properly.
 5. Tape images not supported:
    .TZX, .SPC, .ITM, .PAN, .TAP(Warajevo), .VOC, .ZXS.
 6. Snapshot images not supported:
    .ACH, .PRG, .RAW, .SEM, .SIT, .SNX, .ZX, .ZXS, .ZX82.
 7. 128K emulation is not perfect - the 128K machines crash and hang while
    running quite a lot of games.
 8. Disk errors occur on some +3 games.
 9. Video hardware of all machines is timed incorrectly.
10. EXROM and HOME cartridges are not emulated.
11. The TK90X and TK95 roms output 0 to port #df on start up.
12. The purpose of this port is unknown (probably display mode as TS2068) and
    thus is not emulated.

Very detailed infos about the ZX Spectrum +3e can be found at

http://www.z88forever.org.uk/zxplus3e/

*******************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/speaker.h"
#include "sound/wave.h"
#include "includes/spectrum.h"
#include "eventlst.h"
#include "devices/snapquik.h"
#include "devices/cartslot.h"
#include "devices/cassette.h"
#include "formats/tzx_cas.h"

unsigned char *spectrum_screen_location = NULL;

/****************************************************************************************************/
/* Spectrum 48k functions */

/*
 bit 7-5: not used
 bit 4: Ear output/Speaker
 bit 3: MIC/Tape Output
 bit 2-0: border colour
*/

int spectrum_PreviousFE = 0;

WRITE8_HANDLER(spectrum_port_fe_w)
{
	const device_config *speaker = devtag_get_device(space->machine, "speaker");
	unsigned char Changed;

	Changed = spectrum_PreviousFE^data;

	/* border colour changed? */
	if ((Changed & 0x07)!=0)
	{
		/* yes - send event */
		EventList_AddItemOffset(space->machine, 0x0fe, data & 0x07, cpu_attotime_to_clocks(space->machine->cpu[0],attotime_mul(video_screen_get_scan_period(space->machine->primary_screen), video_screen_get_vpos(space->machine->primary_screen))));
	}

	if ((Changed & (1<<4))!=0)
	{
		/* DAC output state */
		speaker_level_w(speaker,(data>>4) & 0x01);
	}

	if ((Changed & (1<<3))!=0)
	{
		/* write cassette data */
		cassette_output(devtag_get_device(space->machine, "cassette"), (data & (1<<3)) ? -1.0 : +1.0);
	}

	spectrum_PreviousFE = data;
}

static ADDRESS_MAP_START (spectrum_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x5aff) AM_RAM AM_BASE(&spectrum_video_ram )
	AM_RANGE(0x5b00, 0xffff) AM_RAM
ADDRESS_MAP_END

/* KT: more accurate keyboard reading */
/* DJR: Spectrum+ keys added */
READ8_HANDLER(spectrum_port_fe_r)
{
   int lines = offset>>8;
   int data = 0xff;

   int cs_extra1 = input_port_read(space->machine, "PLUS0")  & 0x1f;
   int cs_extra2 = input_port_read(space->machine, "PLUS1")  & 0x1f;
   int cs_extra3 = input_port_read(space->machine, "PLUS2") & 0x1f;
   int ss_extra1 = input_port_read(space->machine, "PLUS3") & 0x1f;
   int ss_extra2 = input_port_read(space->machine, "PLUS4") & 0x1f;

   /* Caps - V */
   if ((lines & 1)==0)
   {
		data &= input_port_read(space->machine, "LINE0");
		/* CAPS for extra keys */
		if (cs_extra1 != 0x1f || cs_extra2 != 0x1f || cs_extra3 != 0x1f)
			data &= ~0x01;
   }

   /* A - G */
   if ((lines & 2)==0)
		data &= input_port_read(space->machine, "LINE1");

   /* Q - T */
   if ((lines & 4)==0)
		data &= input_port_read(space->machine, "LINE2");

   /* 1 - 5 */
   if ((lines & 8)==0)
		data &= input_port_read(space->machine, "LINE3") & cs_extra1;

   /* 6 - 0 */
   if ((lines & 16)==0)
		data &= input_port_read(space->machine, "LINE4") & cs_extra2;

   /* Y - P */
   if ((lines & 32)==0)
		data &= input_port_read(space->machine, "LINE5") & ss_extra1;

   /* H - Enter */
   if ((lines & 64)==0)
		data &= input_port_read(space->machine, "LINE6");

	/* B - Space */
	if ((lines & 128)==0)
	{
		data &= input_port_read(space->machine, "LINE7") & cs_extra3 & ss_extra2;
		/* SYMBOL SHIFT for extra keys */
		if (ss_extra1 != 0x1f || ss_extra2 != 0x1f)
			data &= ~0x02;
	}

	data |= (0xe0); /* Set bits 5-7 - as reset above */

	/* cassette input from wav */
	if (cassette_input(devtag_get_device(space->machine, "cassette")) > 0.0038 )
	{
		data &= ~0x40;
	}

	/* Issue 2 Spectrums default to having bits 5, 6 & 7 set.
    Issue 3 Spectrums default to having bits 5 & 7 set and bit 6 reset. */
	if (input_port_read(space->machine, "CONFIG") & 0x80)
		data ^= (0x40);
	return data;
}

/* kempston joystick interface */
READ8_HANDLER(spectrum_port_1f_r)
{
	return input_port_read(space->machine, "KEMPSTON") & 0x1f;
}

/* fuller joystick interface */
READ8_HANDLER(spectrum_port_7f_r)
{
	return input_port_read(space->machine, "FULLER") | (0xff^0x8f);
}

/* mikrogen joystick interface */
READ8_HANDLER(spectrum_port_df_r)
{
	return input_port_read(space->machine, "MIKROGEN") | (0xff^0x1f);
}

static  READ8_HANDLER ( spectrum_port_ula_r )
{
	return video_screen_get_vpos(space->machine->primary_screen)<193 ? spectrum_video_ram[(video_screen_get_vpos(space->machine->primary_screen)&0xf8)<<2]:0xff;
}

/* ports are not decoded full.
The function decodes the ports appropriately */
static ADDRESS_MAP_START (spectrum_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x00, 0x00) AM_READWRITE(spectrum_port_fe_r,spectrum_port_fe_w) AM_MIRROR(0xfffe) AM_MASK(0xffff) 
	AM_RANGE(0x1f, 0x1f) AM_READ(spectrum_port_1f_r) AM_MIRROR(0xff00)
	AM_RANGE(0x7f, 0x7f) AM_READ(spectrum_port_7f_r) AM_MIRROR(0xff00)
	AM_RANGE(0xdf, 0xdf) AM_READ(spectrum_port_df_r) AM_MIRROR(0xff00)
	AM_RANGE(0x01, 0x01) AM_READ(spectrum_port_ula_r) AM_MIRROR(0xfffe)
ADDRESS_MAP_END

/****************************************************************************************************/
INPUT_PORTS_START( spectrum )
	PORT_START("LINE0") /* [0] 0xFEFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z  COPY    :      LN       BEEP") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X  CLEAR   Pound  EXP      INK") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C  CONT    ?      LPRINT   PAPER") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V  CLS     /      LLIST    FLASH") PORT_CODE(KEYCODE_V)

	PORT_START("LINE1") /* [1] 0xFDFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A  NEW     STOP   READ     ~") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S  SAVE    NOT    RESTORE  |") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D  DIM     STEP   DATA     \\") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F  FOR     TO     SGN      {") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G  GOTO    THEN   ABS      }") PORT_CODE(KEYCODE_G)

	PORT_START("LINE2") /* [2] 0xFBFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q  PLOT    <=     SIN      ASN") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W  DRAW    <>     COS      ACS") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E  REM     >=     TAN      ATN") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R  RUN     <      INT      VERIFY") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T  RAND    >      RND      MERGE") PORT_CODE(KEYCODE_T)

	/* interface II uses this port for joystick */
	PORT_START("LINE3") /* [3] 0xF7FE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1          !      BLUE     DEF FN") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2          @      RED      FN") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3          #      MAGENTA  LINE") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4          $      GREEN    OPEN#") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5          %      CYAN     CLOSE#") PORT_CODE(KEYCODE_5)

	/* protek clashes with interface II! uses 5 = left, 6 = down, 7 = up, 8 = right, 0 = fire */
	PORT_START("LINE4") /* [4] 0xEFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0          _      BLACK    FORMAT") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9          )               POINT") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8          (               CAT") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7          '      WHITE    ERASE") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6          &      YELLOW   MOVE") PORT_CODE(KEYCODE_6)

	PORT_START("LINE5") /* [5] 0xDFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P  PRINT   \"      TAB      (c)") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O  POKE    ;      PEEK     OUT") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I  INPUT   AT     CODE     IN") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U  IF      OR     CHR$     ]") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y  RETURN  AND    STR$     [") PORT_CODE(KEYCODE_Y)

	PORT_START("LINE6") /* [6] 0xBFFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L  LET     =      USR      ATTR") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K  LIST    +      LEN      SCREEN$") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J  LOAD    -      VAL      VAL$") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H  GOSUB   ^      SQR      CIRCLE") PORT_CODE(KEYCODE_H)

	PORT_START("LINE7") /* [7] 0x7FFE */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SYMBOL SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M  PAUSE   .      PI       INVERSE") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N  NEXT    ,      INKEY$   OVER") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B  BORDER  *      BIN      BRIGHT") PORT_CODE(KEYCODE_B)

	PORT_START("PLUS0") /* [8] Spectrum+ Keys (set CAPS + 1-5) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EDIT          (CAPS + 1)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CAPS LOCK     (CAPS + 2)") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TRUE VID      (CAPS + 3)") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("INV VID       (CAPS + 4)") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor left   (CAPS + 5)") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("PLUS1") /* [9] Spectrum+ Keys (set CAPS + 6-0) */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("DEL           (CAPS + 0)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("GRAPH         (CAPS + 9)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor right  (CAPS + 8)") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor up     (CAPS + 7)") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cursor down   (CAPS + 6)") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("PLUS2") /* [10] Spectrum+ Keys (set CAPS + SPACE and CAPS + SYMBOL */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BREAK") PORT_CODE(KEYCODE_PAUSE)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("EXT MODE") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("PLUS3") /* [11] Spectrum+ Keys (set SYMBOL SHIFT + O/P */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\"") PORT_CODE(KEYCODE_F4)
	/*        PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "\"", KEYCODE_QUOTE,  CODE_NONE ) */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0xfc, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("PLUS4") /* [12] Spectrum+ Keys (set SYMBOL SHIFT + N/M */
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0xf3, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("KEMPSTON") /* [13] Kempston joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("KEMPSTON JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START("FULLER") /* [14] Fuller joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("FULLER JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START("MIKROGEN") /* [15] Mikrogen joystick interface */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK UP") PORT_CODE(JOYCODE_Y_UP_SWITCH)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK DOWN") PORT_CODE(JOYCODE_Y_DOWN_SWITCH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK RIGHT") PORT_CODE(JOYCODE_X_RIGHT_SWITCH)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK LEFT") PORT_CODE(JOYCODE_X_LEFT_SWITCH)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("MIKROGEN JOYSTICK FIRE") PORT_CODE(JOYCODE_BUTTON1)

	PORT_START("NMI") 
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("NMI") PORT_CODE(KEYCODE_F12)

	PORT_START("CONFIG") /* [16] */
	PORT_DIPNAME(0x80, 0x00, "Hardware Version")
	PORT_DIPSETTING(0x00, "Issue 2" )
	PORT_DIPSETTING(0x80, "Issue 3" )
	PORT_DIPNAME(0x40, 0x00, "End of .TAP action")
	PORT_DIPSETTING(0x00, "Disable .TAP support" )
	PORT_DIPSETTING(0x40, "Rewind tape to start (to reload earlier levels)" )
	PORT_DIPNAME(0x20, 0x00, "+3/+2a etc. Disk Drive")
	PORT_DIPSETTING(0x00, "Enabled" )
	PORT_DIPSETTING(0x20, "Disabled" )
	PORT_BIT(0x1f, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END


static INTERRUPT_GEN( spec_interrupt )
{
	cpu_set_input_line(device, 0, HOLD_LINE);
}

static const cassette_config spectrum_cassette_config =
{
	tzx_cassette_formats,
	NULL,
	CASSETTE_STOPPED | CASSETTE_SPEAKER_ENABLED | CASSETTE_MOTOR_ENABLED
};

MACHINE_DRIVER_START( spectrum )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 3500000)        /* 3.5 MHz */
	MDRV_CPU_PROGRAM_MAP(spectrum_mem, 0)
	MDRV_CPU_IO_MAP(spectrum_io, 0)
	MDRV_CPU_VBLANK_INT("screen", spec_interrupt)
	MDRV_QUANTUM_TIME(HZ(60))

	MDRV_MACHINE_RESET( spectrum )

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(50.08)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(SPEC_SCREEN_WIDTH, SPEC_SCREEN_HEIGHT)
	MDRV_SCREEN_VISIBLE_AREA(0, SPEC_SCREEN_WIDTH-1, 0, SPEC_SCREEN_HEIGHT-1)
	MDRV_PALETTE_LENGTH(16)
	MDRV_PALETTE_INIT( spectrum )

	MDRV_VIDEO_START( spectrum )
	MDRV_VIDEO_UPDATE( spectrum )
	MDRV_VIDEO_EOF( spectrum )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_WAVE_ADD("wave", "cassette")
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	/* devices */
	MDRV_SNAPSHOT_ADD("snapshot", spectrum, "sna,z80,sp", 0)
	MDRV_QUICKLOAD_ADD("quickload", spectrum, "scr", 0)
	MDRV_CASSETTE_ADD( "cassette", spectrum_cassette_config )
	
	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(spectrum)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_SYSTEM_BIOS(0, "en", "English")
	ROMX_LOAD("spectrum.rom", 0x0000, 0x4000, CRC(ddee531f) SHA1(5ea7c2b824672e914525d1d5c419d71b84a426a2), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "sp", "Spanish")
	ROMX_LOAD("48e.rom", 0x0000, 0x4000, CRC(f051746e) SHA1(9e535e2e24231ccb65e33d107f6d0ceb23e99477), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "bsrom118", "BusySoft Upgrade v1.18")
	ROMX_LOAD("bsrom118.rom", 0x0000, 0x4000, CRC(1511cddb) SHA1(ab3c36daad4325c1d3b907b6dc9a14af483d14ec), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "bsrom140", "BusySoft Upgrade v1.40")
	ROMX_LOAD("bsrom140.rom", 0x0000, 0x4000, CRC(07017c6d) SHA1(2ee2dbe6ab96b60d7af1d6cb763b299374c21776), ROM_BIOS(4))
	ROM_SYSTEM_BIOS(4, "groot", "De Groot's Upgrade")
	ROMX_LOAD("groot.rom", 0x0000, 0x4000, CRC(abf18c45) SHA1(51165cde68e218512d3145467074bc7e786bf307), ROM_BIOS(5))
	ROM_SYSTEM_BIOS(5, "48turbo", "48 Turbo")
	ROMX_LOAD("48turbo.rom", 0x0000, 0x4000, CRC(56189781) SHA1(e62a431b0938af414b7ab8b1349a18b3c4407f70), ROM_BIOS(6))
	ROM_SYSTEM_BIOS(6, "psycho", "Maly's Psycho Upgrade")
	ROMX_LOAD("psycho.rom", 0x0000, 0x4000, CRC(cd60b589) SHA1(0853e25857d51dd41b20a6dbc8e80f028c5befaa), ROM_BIOS(7))
	ROM_SYSTEM_BIOS(7, "turbo23", "Turbo 2.3")
	ROMX_LOAD("turbo2_3.rom", 0x0000, 0x4000, CRC(fd3b0413) SHA1(84ea64af06adaf05e68abe1d69454b4fc6888505), ROM_BIOS(8))
	ROM_SYSTEM_BIOS(8, "turbo44", "Turbo 4.4")
	ROMX_LOAD("turbo4_4.rom", 0x0000, 0x4000, CRC(338b6e87) SHA1(21ad93ffe41a4458704c866cca2754f066f6a560), ROM_BIOS(9))
	ROM_SYSTEM_BIOS(9, "imc", "Ian Collier's Upgrade")
	ROMX_LOAD("imc.rom", 0x0000, 0x4000, CRC(d1be99ee) SHA1(dee814271c4d51de257d88128acdb324fb1d1d0d), ROM_BIOS(10))
	ROM_SYSTEM_BIOS(10, "plus4", "ZX Spectrum +4")
	ROMX_LOAD("plus4.rom",0x0000,0x4000, CRC(7e0f47cb) SHA1(c103e89ef58e6ade0c01cea0247b332623bd9a30), ROM_BIOS(11))
	ROM_SYSTEM_BIOS(11, "deutch", "German unofficial (Andrew Owen)")
	ROMX_LOAD("deutch.rom",0x0000,0x4000, CRC(1a9190f4) SHA1(795c20324311dd5a56300e6e4ec49b0a694ac0b3), ROM_BIOS(12))
	ROM_SYSTEM_BIOS(12, "hdt", "HDT-ISO HEX-DEC-TEXT")
	ROMX_LOAD("hdt-iso.rom",0x0000,0x4000, CRC(b81c570c) SHA1(2a9745ba3b369a84c4913c98ede66ec87cb8aec1), ROM_BIOS(13))
	ROM_SYSTEM_BIOS(13, "sc", "The Sea Change Minimal ROM V1.78")
	ROMX_LOAD("sc01.rom",0x0000,0x4000, CRC(73b4057a) SHA1(c58ff44a28db47400f09ed362ca0527591218136), ROM_BIOS(14))
	ROM_SYSTEM_BIOS(14, "gosh", "GOSH Wonderful ZX Spectrum ROM V1.32")
	ROMX_LOAD("gw03.rom",0x0000,0x4000, CRC(5585d7c2) SHA1(a701c3d4b698f7d2be537dc6f79e06e4dbc95929), ROM_BIOS(15))
	ROM_SYSTEM_BIOS(15, "1986es", "1986ES Snapshot")
	ROMX_LOAD("1986es.rom",0x0000,0x4000, CRC(9e0fdaaa) SHA1(f9d23f25640c51bcaa63e21ed5dd66bb2d5f63d4), ROM_BIOS(16))
	ROM_SYSTEM_BIOS(16, "jgh", "JGH ROM V0.74 (C) J.G.Harston")
	ROMX_LOAD("jgh.rom",0x0000,0x4000, CRC(7224138e) SHA1(d7f02ed66455f1c08ac0c864c7038a92a88ba94a), ROM_BIOS(17))
	ROM_SYSTEM_BIOS(17, "iso22", "ISO ROM V2.2")
	ROMX_LOAD("isomoje.rom",0x0000,0x4000, CRC(62ab3640) SHA1(04adbdb1380d6ccd4ab26ddd61b9ccbba462a60f), ROM_BIOS(18))
	ROM_SYSTEM_BIOS(18, "iso8", "ISO ROM 8")
	ROMX_LOAD("iso8bm.rom",0x0000,0x4000, CRC(43e9c2fd) SHA1(5752e6f789769475711b91e0a75911fa5232c767), ROM_BIOS(19))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(specide)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("zxide.rom", 0x0000, 0x4000, CRC(bd48db54) SHA1(54c2aa958902b5395c260770a0b25c7ba5685de9))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(spec80k)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("80-lec.rom", 0x0000, 0x4000, CRC(5b5c92b1) SHA1(bb7a77d66e95d2e28ebb610e543c065e0d428619))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk90x)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("tk90x.rom",0x0000,0x4000, CRC(3e785f6f) SHA1(9a943a008be13194fb006bddffa7d22d2277813f))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(tk95)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("tk95.rom",0x0000,0x4000, CRC(17368e07) SHA1(94edc401d43b0e9a9cdc1d35de4b6462dc414ab3))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(inves)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("inves.rom",0x0000,0x4000, CRC(8ff7a4d1) SHA1(d020440638aff4d39467128413ef795677be9c23))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* Romanian clones */
ROM_START(hc85)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("hc85.rom",0x0000,0x4000, CRC(3ab60fb5) SHA1(a4189db0bcdf8b39ed782b398828efb408fc4817))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(hc90)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("hc90.rom",0x0000,0x4000, CRC(78c14d9a) SHA1(25ef81905bed90497a749770170c24632efb2039))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(hc91)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("hc91.rom",0x0000,0x4000, CRC(8bf53761) SHA1(967d5179ba2823e9c8dd9ddfb0430465aaddb554))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(cip03)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("cip03.rom",0x0000,0x4000, CRC(c7d0cd3c) SHA1(811055b44fc74076137e2bf8db206b2a70287cc2))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(jet)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("jet.rom",0x0000,0x4000, CRC(e56a7d11) SHA1(e76be9ee71bae6aa1c2ff969276fb599ed68cb50))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* Czechoslovakian clones*/

/* TODO: need to add memory handling for 80K RAM */

ROM_START(dgama87)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("dgama87.rom",0x0000,0x4000, CRC(43104909) SHA1(f62d1f3f35fda467cae468e890995614f6ec2357))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(dgama88)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("dgama88.rom",0x0000,0x4000, CRC(4ec7e078) SHA1(09a91f85e82efa7f974d1b88c69636a02063d563))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(dgama89)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_SYSTEM_BIOS(0, "default", "Original")
	ROMX_LOAD("dgama89.rom",0x0000,0x4000, CRC(45c29401) SHA1(8466a9da0169666210ccff5d43376d70bae0ae9b), ROM_BIOS(1))
	ROM_SYSTEM_BIOS(1, "g81", "Gama 81")
	ROMX_LOAD("g81.rom",0x0000,0x4000, CRC(c169a63b) SHA1(71652005c2e7a4301caa7e95ae989b69cb5a6a0d), ROM_BIOS(2))
	ROM_SYSTEM_BIOS(2, "iso", "ISO")
	ROMX_LOAD("iso.rom",0x0000,0x4000, CRC(2ee3a992) SHA1(2e39995dd032036d33a6dd88a38b750057bca19d), ROM_BIOS(3))
	ROM_SYSTEM_BIOS(3, "isopolak", "ISO Polak")
	ROMX_LOAD("isopolak.rom",0x0000,0x4000, CRC(5e3f1f66) SHA1(61713117c944fc6afcb96c647bdba5ad36fd6a4b), ROM_BIOS(4))	
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(didakt90)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("didakt90.rom",0x0000,0x4000, CRC(76f2db1e) SHA1(daee355a8ee58bc406873c1dd81eecb6161dd4bd))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(didakm91)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("didakm91.rom",0x0000,0x4000, CRC(beab69b8) SHA1(71d4d1a05fb936f616bcb05c3a276f79343ecd4d))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(didaktk)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("didaktk.rom",0x0000,0x4000, CRC(8ec8a625) SHA1(cba35517d33a5c97e3d9110f12a417c6c5cdeca8))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(didakm93)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("didakm93.rom",0x0000,0x4000, CRC(ec274b1b) SHA1(a3470d8d1a996ee2a1ffff8bd8044da6e907e07e))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(mistrum)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("mistrum.rom",0x0000,0x4000, CRC(d496103e) SHA1(cca1c5b059dc3a29ca4282e8621e34a65efaa1a3))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/* Russian clones */

ROM_START(blitz)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("blitz.rom",0x0000,0x4000, CRC(91e535a8) SHA1(14f09d45dc3803cbdb05c33adb28eb12dbad9dd0))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(byte)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("byte.rom",0x0000,0x4000, CRC(c13ba473) SHA1(99f40727185abbb2413f218d69df021ae2e99e45))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(orizon)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("orizon.rom",0x0000,0x4000, CRC(ed4d9787) SHA1(3e8b29862e06be03344393c320a64a109fd9aff5))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(quorum48)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("quorum48.rom",0x0000,0x4000, CRC(48085b0e) SHA1(8e01581643f7bdfa773f68207a6437911b631e53))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(magic6)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("magic6.rom",0x0000,0x4000, CRC(cb63ae06) SHA1(533ad1f50534e6bdeec50eb5a9a4976c3d010dc7))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START(compani1)
	ROM_REGION(0x10000,"maincpu",0)
	ROM_LOAD("compani1.rom",0x0000,0x4000, CRC(bcfa6068) SHA1(40074b55c91a947698598e9d6ac5b8495e8cc840))
	ROM_CART_LOAD("cart", 0x0000, 0x4000, ROM_NOCLEAR | ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT       INIT    CONFIG      COMPANY     FULLNAME */
COMP( 1982, spectrum, 0,        0,		spectrum,		spectrum,	0,		0,	"Sinclair Research",	"ZX Spectrum" , 0)
COMP( 1987, spec80k,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"ZX Spectrum 80K" , GAME_COMPUTER_MODIFIED)
COMP( 1995, specide,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"ZX Spectrum IDE" , GAME_COMPUTER_MODIFIED)
COMP( 1986, inves,    spectrum, 0,		spectrum,		spectrum,	0,		0,	"Investronica",	"Inves Spectrum 48K+" , 0)
COMP( 1985, tk90x,    spectrum, 0,		spectrum,		spectrum,	0,		0,	"Micro Digital",	"TK-90x Color Computer" , 0)
COMP( 1986, tk95,     spectrum, 0,		spectrum,		spectrum,	0,		0,	"Micro Digital",	"TK-95 Color Computer" , 0)
COMP( 1985, hc85,     spectrum, 0,		spectrum,		spectrum,	0,		0,	"ICE-Felix",	"HC-85" , 0)
COMP( 1990, hc90,     spectrum, 0,		spectrum,		spectrum,	0,		0,	"ICE-Felix",	"HC-90" , 0)
COMP( 1991, hc91,     spectrum, 0,		spectrum,		spectrum,	0,		0,	"ICE-Felix",	"HC-91" , 0)
COMP( 1988, cip03,    spectrum, 0,		spectrum,		spectrum,	0,		0,	"Electronica",	"CIP-03" , 0)
COMP( 1990, jet,      spectrum, 0,		spectrum,		spectrum,	0,		0,	"Electromagnetica",	"JET" , 0)
COMP( 1987, dgama87,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik Gama 87" , 0)
COMP( 1988, dgama88,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik Gama 88" , 0)
COMP( 1989, dgama89,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik Gama 89" , 0)
COMP( 1990, didakt90, spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik 90" , 0)
COMP( 1991, didakm91, spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik M 91" , 0)
COMP( 1992, didaktk,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik Kompakt" , 0)
COMP( 1993, didakm93, spectrum, 0,		spectrum,		spectrum,	0,		0,	"Didaktik Skalica",	"Didaktik M 93" , 0)
COMP( 1988, mistrum,  spectrum, 0,		spectrum,		spectrum,	0,		0,	"Amaterske RADIO",	"Mistrum" , 0)
COMP( 1990, blitz,    spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Blic" , 0)
COMP( 1990, byte,     spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Byte" , 0)
COMP( 199?, orizon,   spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Orizon-Micro" , 0)
COMP( 1993, quorum48, spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Kvorum 48K" , 0)
COMP( 1993, magic6,   spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Magic 6" , 0)
COMP( 1990, compani1, spectrum, 0,		spectrum,		spectrum,	0,		0,	"",	"Kompanion 1" , 0)