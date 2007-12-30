/******************************************************************************

 ******************************************************************************/
#include "driver.h"
#include "inputx.h"
#include "video/m6847.h"
#include "includes/apf.h"

#include "machine/6821pia.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "formats/apf_apt.h"
#include "sound/speaker.h"

 /*
0000- 2000-2003 PIA of M1000. is itself repeated until 3fff.
She controls " keypads ", the way of video and the loudspeaker. Putting to 0 one of the 4 least
significant bits of 2002, the corresponding line of keys is ***reflxed mng in 2000 (32 keys altogether).
The other four bits of 2002 control the video way. Bit 0 of 2003 controls the interruptions. Bit 3 of 2003 controls
the loudspeaker. 4000-47ff ROM of M1000. is repeated until 5fff and in e000-ffff. 6000-6003 PIA of the APF.
is itself repeated until 63ff. She controls the keyboard of the APF and the cassette. The value of the
number that represents the three bits less significant of 6002 they determine the line of eight
keys that it is ***reflxed mng in 6000. Bit 4 of 6002 controls the motor of the cassette. Bit 5 of
6002 activates or deactivates the recording. Bit 5 of 6002 indicates the level of recording in
the cassette. 6400-64ff would be the interface Here series, optional. 6500-66ff would be the
disk controller Here, optional. 6800-77ff ROM (" cartridge ") 7800-7fff Probably ROM. The BASIC
leaves frees this zone. 8000-9fff ROM (" cartridge ") A000-BFFF ram (8 Kb) C000-DFFF ram
additional (8 Kb) E000-FFFF ROM of M1000 (to see 4000-47FF) The interruption activates with
the vertical synchronism of the video. routine that it executes is in the ROM of M1000
and puts the video in way text during a short interval, so that the first line is seen of
text screen in the superior part of the graphical screen.
*/

/* 6600, 6500-6503 wd179x disc controller? 6400, 6401 */
static unsigned char keyboard_data;
static unsigned char pad_data;

static  READ8_HANDLER(apf_m1000_pia_in_a_func)
{
	return pad_data;
}

static  READ8_HANDLER(apf_m1000_pia_in_b_func)
{
	return 0x0ff;
}

static  READ8_HANDLER(apf_m1000_pia_in_ca1_func)
{
	return 0;
}

static  READ8_HANDLER(apf_m1000_pia_in_cb1_func)
{
	return 0x00;
}

static  READ8_HANDLER(apf_m1000_pia_in_ca2_func)
{
	return 0;
}

static  READ8_HANDLER(apf_m1000_pia_in_cb2_func)
{
	return 0x00;
}


static WRITE8_HANDLER(apf_m1000_pia_out_a_func)
{
}

static unsigned char previous_mode;

static WRITE8_HANDLER(apf_m1000_pia_out_b_func)
{
	pad_data = 0x0ff;

	/* bit 7..4 video control */
	/* bit 3..0 keypad line select */
	if (data & 0x08)
		pad_data &= readinputportbytag("joy3");
	if (data & 0x04)
		pad_data &= readinputportbytag("joy2");
	if (data & 0x02)
		pad_data &= readinputportbytag("joy1");
	if (data & 0x01)
		pad_data &= readinputportbytag("joy0");

	/* 1b standard, 5b, db */
	/* 0001 */
	/* 0101 */
	/* 1101 */

	/* multi colour graphics mode */
	/* 158 = 1001 multi-colour graphics */
	/* 222 = 1101 mono graphics */
	//	if (((previous_mode^data) & 0x0f0)!=0)
	{
		extern UINT8 apf_m6847_attr;

		/* not sure if this is correct - need to check */
		apf_m6847_attr = 0x00;
		if (data & 0x80)	apf_m6847_attr |= M6847_AG;
		if (data & 0x40)	apf_m6847_attr |= M6847_GM2;
		if (data & 0x20)	apf_m6847_attr |= M6847_GM1;
		if (data & 0x10)	apf_m6847_attr |= M6847_GM0;
		previous_mode = data;
	}
}

static WRITE8_HANDLER(apf_m1000_pia_out_ca2_func)
{
}

static WRITE8_HANDLER(apf_m1000_pia_out_cb2_func)
{
	speaker_level_w(0, data);
}

/* use bit 0 to identify state of irq from pia 0 */
/* use bit 1 to identify state of irq from pia 0 */
/* use bit 2 to identify state of irq from pia 1 */
/* use bit 3 to identify state of irq from pia 1 */
/* use bit 4 to identify state of irq from video */

unsigned char apf_ints;

void apf_update_ints(void)
{
	cpunum_set_input_line(0, 0, apf_ints ? HOLD_LINE : CLEAR_LINE);
}

static void	apf_m1000_irq_a_func(int state)
{
	if (state)
	{
		apf_ints|=1;
	}
	else
	{
		apf_ints&=~1;
	}

	apf_update_ints();
}


static void	apf_m1000_irq_b_func(int state)
{
	//logerror("pia 0 irq b %d\n",state);

	if (state)
	{
		apf_ints|=2;
	}
	else
	{
		apf_ints&=~2;
	}

	apf_update_ints();

}

static const pia6821_interface apf_m1000_pia_interface=
{
	apf_m1000_pia_in_a_func,
	apf_m1000_pia_in_b_func,
	apf_m1000_pia_in_ca1_func,
	apf_m1000_pia_in_cb1_func,
	apf_m1000_pia_in_ca2_func,
	apf_m1000_pia_in_cb2_func,
	apf_m1000_pia_out_a_func,
	apf_m1000_pia_out_b_func,
	apf_m1000_pia_out_ca2_func,
	apf_m1000_pia_out_cb2_func,
	apf_m1000_irq_a_func,
	apf_m1000_irq_b_func
};


static  READ8_HANDLER(apf_imagination_pia_in_a_func)
{
	return keyboard_data;
}

static READ8_HANDLER(apf_imagination_pia_in_b_func)
{
	unsigned char data;

	data = 0x000;

	if (cassette_input(image_from_devtype_and_index(IO_CASSETTE,0)) > 0.0038)
		data =(1<<7);

	return data;
}

static  READ8_HANDLER(apf_imagination_pia_in_ca1_func)
{
	return 0x00;
}

static  READ8_HANDLER(apf_imagination_pia_in_cb1_func)
{
	return 0x00;
}

static  READ8_HANDLER(apf_imagination_pia_in_ca2_func)
{
	return 0x00;
}

static  READ8_HANDLER(apf_imagination_pia_in_cb2_func)
{
	return 0x00;
}


static WRITE8_HANDLER(apf_imagination_pia_out_a_func)
{
}

static WRITE8_HANDLER(apf_imagination_pia_out_b_func)
{
	/* bits 2..0 = keyboard line */
	/* bit 3 = ??? */
	/* bit 4 = cassette motor */
	/* bit 5 = ?? */
	/* bit 6 = cassette write data */
	/* bit 7 = ??? */

	int keyboard_line;

	keyboard_line = data & 0x07;

// TODO: convert to readinputbytag
	keyboard_data = readinputport(keyboard_line+4);

	/* bit 4: cassette motor control */
	cassette_change_state(image_from_devtype_and_index(IO_CASSETTE, 0),
		(data & 0x10) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED,
		CASSETTE_MASK_MOTOR);

	/* bit 6: cassette write */
	cassette_output(image_from_devtype_and_index(IO_CASSETTE, 0),
		(data & 0x40) ? -1.0 : 1.0);
}

static WRITE8_HANDLER(apf_imagination_pia_out_ca2_func)
{
}

static WRITE8_HANDLER(apf_imagination_pia_out_cb2_func)
{
}

static void	apf_imagination_irq_a_func(int state)
{
	if (state)
	{
		apf_ints|=4;
	}
	else
	{
		apf_ints&=~4;
	}

	apf_update_ints();

}

static void	apf_imagination_irq_b_func(int state)
{
	if (state)
	{
		apf_ints|=8;
	}
	else
	{
		apf_ints&=~8;
	}

	apf_update_ints();

}

static const pia6821_interface apf_imagination_pia_interface=
{
	apf_imagination_pia_in_a_func,
	apf_imagination_pia_in_b_func,
	apf_imagination_pia_in_ca1_func,
	apf_imagination_pia_in_cb1_func,
	apf_imagination_pia_in_ca2_func,
	apf_imagination_pia_in_cb2_func,
	apf_imagination_pia_out_a_func,
	apf_imagination_pia_out_b_func,
	apf_imagination_pia_out_ca2_func,
	apf_imagination_pia_out_cb2_func,
	apf_imagination_irq_a_func,
	apf_imagination_irq_b_func
};


extern unsigned char *apf_video_ram;

static void apf_common_init(void)
{
	apf_ints = 0;
	pia_config(0,&apf_m1000_pia_interface);
	pia_reset();
}

static MACHINE_START( apf_imagination )
{
	pia_config(1,&apf_imagination_pia_interface);
	apf_common_init();
	wd17xx_init(WD_TYPE_179X, NULL, NULL);
}

static MACHINE_START( apf_m1000 )
{
	apf_common_init();
}

static WRITE8_HANDLER(apf_dischw_w)
{
	int drive;

	/* bit 3 is index of drive to select */
	drive = (data>>3) & 0x01;

	wd17xx_set_drive(drive);

	logerror("disc w %04x %04x\n",offset,data);
}

static  READ8_HANDLER(serial_r)
{
	logerror("serial r %04x\n",offset);
	return 0x00;
}

static WRITE8_HANDLER(serial_w)
{
	logerror("serial w %04x %04x\n",offset,data);
}

static WRITE8_HANDLER(apf_wd179x_command_w)
{
	wd17xx_command_w(offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_track_w)
{
	wd17xx_track_w(offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_sector_w)
{
	wd17xx_sector_w(offset,~data);
}

static WRITE8_HANDLER(apf_wd179x_data_w)
{
	wd17xx_data_w(offset,~data);
}

static READ8_HANDLER(apf_wd179x_status_r)
{
	return ~wd17xx_status_r(offset);
}

static READ8_HANDLER(apf_wd179x_track_r)
{
	return ~wd17xx_track_r(offset);
}

static READ8_HANDLER(apf_wd179x_sector_r)
{
	return ~wd17xx_sector_r(offset);
}

static READ8_HANDLER(apf_wd179x_data_r)
{
	return wd17xx_data_r(offset);
}

static ADDRESS_MAP_START(apf_imagination_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x003ff) AM_RAM AM_BASE(&apf_video_ram) AM_MIRROR(0x1c00)
	AM_RANGE( 0x02000, 0x03fff) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE( 0x04000, 0x047ff) AM_ROM AM_REGION(REGION_CPU1, 0x10000) AM_MIRROR(0x1800)
	AM_RANGE( 0x06000, 0x063ff) AM_READWRITE(pia_1_r, pia_1_w)
	AM_RANGE( 0x06400, 0x064ff) AM_READWRITE(serial_r, serial_w)
	AM_RANGE( 0x06500, 0x06500) AM_READWRITE(apf_wd179x_status_r, apf_wd179x_command_w)
	AM_RANGE( 0x06501, 0x06501) AM_READWRITE(apf_wd179x_track_r, apf_wd179x_track_w)
	AM_RANGE( 0x06502, 0x06502) AM_READWRITE(apf_wd179x_sector_r, apf_wd179x_sector_w)
	AM_RANGE( 0x06503, 0x06503) AM_READWRITE(apf_wd179x_data_r, apf_wd179x_data_w)
	AM_RANGE( 0x06600, 0x06600) AM_WRITE( apf_dischw_w)
	AM_RANGE( 0x06800, 0x077ff) AM_ROM
	AM_RANGE( 0x07800, 0x07fff) AM_NOP
	AM_RANGE( 0x08000, 0x09fff) AM_ROM
	AM_RANGE( 0x0a000, 0x0dfff) AM_RAM
	AM_RANGE( 0x0e000, 0x0e7ff) AM_ROM AM_REGION(REGION_CPU1, 0x10000) AM_MIRROR(0x1800)
ADDRESS_MAP_END

static ADDRESS_MAP_START(apf_m1000_map, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x00000, 0x003ff) AM_RAM AM_BASE(&apf_video_ram)  AM_MIRROR(0x1c00)
	AM_RANGE( 0x02000, 0x03fff) AM_READWRITE(pia_0_r, pia_0_w)
	AM_RANGE( 0x04000, 0x047ff) AM_ROM AM_REGION(REGION_CPU1, 0x10000) AM_MIRROR(0x1800)
	AM_RANGE( 0x06800, 0x077ff) AM_ROM
	AM_RANGE( 0x08000, 0x09fff) AM_ROM
	AM_RANGE( 0x0a000, 0x0dfff) AM_RAM
	AM_RANGE( 0x0e000, 0x0e7ff) AM_ROM AM_REGION(REGION_CPU1, 0x10000) AM_MIRROR(0x1800)
ADDRESS_MAP_END

/* The following input ports definitions are wrong and can't be debugged unless the driver
   is capable of running more cartridges. However each of the controllers supported by the M-1000
   have these features:

   1 8-way joystick
   1 big red fire button on the upper side
   12-keys keypad with the following layout

   7 8 9 0
   4 5 6 Cl
   1 2 3 En

   On the control panel of the M-1000 there are two big buttons: a Reset key and the Power switch

   Reference: http://www.nausicaa.net/~lgreenf/apfpage2.htm
*/

static INPUT_PORTS_START( apf_m1000 )

	/*
	   There must be a bug lurking somewhere, because the lines 0-3 are not detected correctly:
	   Using another known APF emulator, this simple Basic program can be used to read
	   the joysticks and the keyboard:

	   10 PRINT KEY$(n);
	   20 GOTO 10

	   where n = 0, 1 or 2 - 0 = keyboard, 1,2 = joysticks #1 and #2

	   When reading the keyboard KEY$(0) returns the character associated to the key, with the
	   following exceptions:

	   Ctrl =    CHR$(1)
	   Rept =    CHR$(2)
	   Here Is = CHR$(4)
	   Rubout =  CHR$(8)

	   When reading the joysticks, KEY$() = "N", "S", "E", "W" for the directions
	                                        "0" - "9" for the keypad digits
	                                        "?" for "Cl"
	                                        "!" for "En"

	   Current code doesn't behaves this way...

	*/

	/* line 0 */
	PORT_START_TAG("joy0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("w") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("e") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("r") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("t") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("u") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("i") PORT_CODE(KEYCODE_I)

	/* line 1 */
	PORT_START_TAG("joy1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("o") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("p") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("a") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("s") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("d") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("f") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("g") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("h") PORT_CODE(KEYCODE_H)

	/* line 2 */
	PORT_START_TAG("joy2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("j") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("k") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("l") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("x") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("c") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("v") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("b") PORT_CODE(KEYCODE_B)

	/* line 3 */
	PORT_START_TAG("joy3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("n") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("m") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_OTHER) PORT_NAME("6") PORT_CODE(KEYCODE_6)

INPUT_PORTS_END


static INPUT_PORTS_START( apf_imagination )

	PORT_INCLUDE( apf_m1000 )

	/* Reference: http://www.nausicaa.net/~lgreenf/apfpage2.htm */

	/* keyboard line 0 */
	PORT_START_TAG("key0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X")               PORT_CODE(KEYCODE_X)          PORT_CHAR('x')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z")               PORT_CODE(KEYCODE_Z)          PORT_CHAR('z')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q       IF")      PORT_CODE(KEYCODE_Q)          PORT_CHAR('q')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2   \"    LET")   PORT_CODE(KEYCODE_2)          PORT_CHAR('2') PORT_CHAR('\"')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A")               PORT_CODE(KEYCODE_A)          PORT_CHAR('a')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1   !   GOSUB")   PORT_CODE(KEYCODE_1)          PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W       STEP")    PORT_CODE(KEYCODE_W)          PORT_CHAR('w')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S")               PORT_CODE(KEYCODE_S)          PORT_CHAR('s')

	/* keyboard line 1 */
	PORT_START_TAG("key1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C")               PORT_CODE(KEYCODE_C)          PORT_CHAR('c')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V")               PORT_CODE(KEYCODE_V)          PORT_CHAR('v')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R       READ")    PORT_CODE(KEYCODE_R)          PORT_CHAR('r')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3   #   DATA")    PORT_CODE(KEYCODE_3)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F")               PORT_CODE(KEYCODE_F)          PORT_CHAR('f')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4   $   INPUT")   PORT_CODE(KEYCODE_4)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E       STOP")    PORT_CODE(KEYCODE_E)          PORT_CHAR('e')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D")               PORT_CODE(KEYCODE_D)          PORT_CHAR('d')

	/* keyboard line 2 */
	PORT_START_TAG("key2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N   ^")           PORT_CODE(KEYCODE_N)          PORT_CHAR('n') PORT_CHAR('^')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B")               PORT_CODE(KEYCODE_B)          PORT_CHAR('b')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T       NEXT")    PORT_CODE(KEYCODE_T)          PORT_CHAR('t')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6   &   FOR")     PORT_CODE(KEYCODE_6)          PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G")               PORT_CODE(KEYCODE_G)          PORT_CHAR('g')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5   %   DIM")     PORT_CODE(KEYCODE_5)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y       PRINT")   PORT_CODE(KEYCODE_Y)          PORT_CHAR('y')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H")               PORT_CODE(KEYCODE_H)          PORT_CHAR('h')

	/* keyboard line 3 */
	PORT_START_TAG("key3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M   ]")           PORT_CODE(KEYCODE_M)          PORT_CHAR('m') PORT_CHAR(']')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(",   <")           PORT_CODE(KEYCODE_COMMA)      PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I       LIST")    PORT_CODE(KEYCODE_I)          PORT_CHAR('i')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7   '   RETURN")  PORT_CODE(KEYCODE_7)          PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K   [")           PORT_CODE(KEYCODE_K)          PORT_CHAR('k') PORT_CHAR('[')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8   (   THEN")    PORT_CODE(KEYCODE_8)          PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U       END")     PORT_CODE(KEYCODE_U)          PORT_CHAR('u')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J")               PORT_CODE(KEYCODE_J)          PORT_CHAR('j')

	/* keyboard line 4 */
	PORT_START_TAG("key4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/   ?")           PORT_CODE(KEYCODE_SLASH)      PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".   >")           PORT_CODE(KEYCODE_STOP)       PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O   _   REM")     PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('_')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0       GOTO")    PORT_CODE(KEYCODE_0)          PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L   \\")          PORT_CODE(KEYCODE_L)          PORT_CHAR('l') PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9   )   ON")      PORT_CODE(KEYCODE_9)          PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P   @   USING")   PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('@')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";   +")           PORT_CODE(KEYCODE_COLON)      PORT_CHAR(';') PORT_CHAR('+')

	/* keyboard line 5 */
	PORT_START_TAG("key5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Space")           PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(32)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(":   *")           PORT_CODE(KEYCODE_MINUS)      PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Return")          PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(13)
/* Not sure about the following, could be also bit 0x10 or 0x40: using other known APF emulators as benchmark,
   it looks like that Line Feed is not intercepted by KEY$(). Unless a better method is devised (APF assembly?)
   I'll stick with this assignment.
*/
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Line Feed")       PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR(10)
	PORT_BIT(0x10, 0x10, IPT_UNUSED)                                                                       /* ??? */
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-   =   RESTORE") PORT_CODE(KEYCODE_EQUALS)     PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x40, IPT_UNUSED)                                                                       /* ??? */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rubout")          PORT_CODE(KEYCODE_QUOTE)      PORT_CHAR(8)

	/* line 6 */
	PORT_START_TAG("key6")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift")     PORT_CODE(KEYCODE_RSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift")      PORT_CODE(KEYCODE_LSHIFT)     PORT_CHAR(UCHAR_SHIFT_1)
/* This key displays the glyph "[", but a quick test reveals that its ASCII code is 27. */
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Esc")             PORT_CODE(KEYCODE_TAB)        PORT_CHAR(27)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Ctrl")            PORT_CODE(KEYCODE_LCONTROL)   PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Rept")            PORT_CODE(KEYCODE_ENTER)      PORT_CHAR(UCHAR_MAMEKEY(TAB))
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break")           PORT_CODE(KEYCODE_BACKSLASH)  PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Here Is")         PORT_CODE(KEYCODE_BACKSPACE)  PORT_CHAR(UCHAR_MAMEKEY(HOME))
/* It's very likely these inputs are actually disconnected: if connected they act as a duplicate of key "X" and "Z" */
//	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Another X")       PORT_CODE(KEYCODE_8_PAD)
//	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Another Z")       PORT_CODE(KEYCODE_9_PAD)
	PORT_BIT(0x40, 0x40, IPT_UNUSED)
	PORT_BIT(0x80, 0x80, IPT_UNUSED)

	/* line 7 */
	PORT_START_TAG("key7")
	PORT_BIT(0xff, 0xff, IPT_UNUSED)                                                                       /* ??? */

INPUT_PORTS_END



static MACHINE_DRIVER_START( apf_imagination )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6800, 3750000)        /* 7.8336 Mhz */
	MDRV_CPU_PROGRAM_MAP(apf_imagination_map, 0)
	MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
	MDRV_INTERLEAVE(0)

	MDRV_MACHINE_START( apf_imagination )

	/* video hardware */
	MDRV_VIDEO_START(apf)
	MDRV_VIDEO_UPDATE(m6847)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( apf_m1000 )
	MDRV_IMPORT_FROM( apf_imagination )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( apf_m1000_map, 0 )
	MDRV_MACHINE_START( apf_m1000 )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apfimag)
	ROM_REGION(0x10000+0x0800,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x00800, CRC(2a331a33) SHA1(387b90882cd0b66c192d9cbaa3bec250f897e4f1))
	ROM_LOAD("basic_68.rom",0x06800, 0x01000, CRC(ef049ab8) SHA1(c4c12aade95dd89a4750fe7f89d57256c93da068))
	ROM_LOAD("basic_80.rom",0x08000, 0x02000, CRC(a4c69fae) SHA1(7f98aa482589bf7c5a26d338fec105e797ba43f6))
ROM_END

ROM_START(apfm1000)
	ROM_REGION(0x10000+0x0800,REGION_CPU1,0)
	ROM_LOAD("apf_4000.rom",0x010000, 0x0800, CRC(2a331a33) SHA1(387b90882cd0b66c192d9cbaa3bec250f897e4f1))
ROM_END

static void apfimag_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cassette */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) apf_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}

static void apfimag_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_apfimag_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "apd"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( apfimag )
	CONFIG_DEVICE(apfimag_cassette_getinfo)
	CONFIG_DEVICE(apfimag_floppy_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR	NAME		PARENT	COMPAT	MACHINE				INPUT				INIT    CONFIG		COMPANY               FULLNAME */
COMP(1977, apfimag,	0,		0,		apf_imagination,	apf_imagination,	0,		apfimag,	"APF Electronics Inc",  "APF Imagination Machine" ,GAME_NOT_WORKING)
COMP(1978,	apfm1000,	0,		0,		apf_m1000,			apf_m1000,			0,		NULL,		"APF Electronics Inc",  "APF M-1000" ,GAME_NOT_WORKING)
