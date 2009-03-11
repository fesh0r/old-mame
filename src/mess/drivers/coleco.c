/***************************************************************************

  coleco.c

  Driver file to handle emulation of the Colecovision.

  Marat Fayzullin (ColEm source)
  Marcel de Kogel (AdamEm source)
  Mike Balfour
  Ben Bruscella
  Sean Young

  NEWS:
    - Modified memory map, now it has only 1k of RAM mapped on 8k Slot
    - Modified I/O map, now it is handled as on a real ColecoVision:
        The I/O map is broken into 4 write and 4 read ports:
            80-9F (W) = Set both controllers to keypad mode
            80-9F (R) = Not Connected

            A0-BF (W) = Video Chip (TMS9928A), A0=0 -> Write Register 0 , A0=1 -> Write Register 1
            A0-BF (R) = Video Chip (TMS9928A), A0=0 -> Read Register 0 , A0=1 -> Read Register 1

            C0-DF (W) = Set both controllers to joystick mode
            C0-DF (R) = Not Connected

            E0-FF (W) = Sound Chip (SN76489A)
            E0-FF (R) = Read Controller data, A1=0 -> read controller 1, A1=1 -> read controller 2

    - Modified paddle handler, now it is handled as on a real ColecoVision
    - Added support for a Driving Controller (Expansion Module #2), enabled via configuration
    - Added support for a Roller Controller (Trackball), enabled via configuration
    - Added support for two Super Action Controller, enabled via configuracion

    EXTRA CONTROLLERS INFO:

        -Driving Controller (Expansion Module #2). It consist of a steering wheel and a gas pedal. Only one
         can be used on a real ColecoVision. The gas pedal is not analog, internally it is just a switch.
         On a real ColecoVision, when the Driving Controller is enabled, the controller 1 do not work because
         have been replaced by the Driving Controller, and controller 2 have to be used to start game, gear
         shift, etc.
         Driving Controller is just a spinner on controller 1 socket similar to the one on Roller Controller
         and Super Action Controllers so you can use Roller Controller or Super Action Controllers to play
         games requiring Driving Controller.

        -Roller Controller. Basically a trackball with four buttons (the two fire buttons from player 1 and
         the two fire buttons from player 2). Only one Roller Controller can be used on a real ColecoVision.
     Roller Controller is connected to both controller sockets and both controllers are conected to the Roller
     Controller, it uses the spinner pins of both sockets to generate the X and Y signals (X from controller 1
     and the Y from controller 2)

        -Super Action Controllers. It is a hand controller with a keypad, four buttons (the two from
         the player pad and two more), and a spinner. This was made primarily for two player sport games, but
         will work for every other ColecoVision game.

***************************************************************************/

/*

	TODO:

	- Dina SG-1000 mode

*/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/sn76496.h"
#include "video/tms9928a.h"
#include "devices/cartslot.h"

static int joy_mode;
static int joy_status[2];

/* Read/Write Handlers */

static READ8_HANDLER( paddle_1_r )
{
	UINT8 inport6 = input_port_read_safe(space->machine, "IN6", 0);

	/* Keypad and fire 1 (SAC Yellow Button) */
	if (joy_mode==0)
	{
		UINT8 inport0 = input_port_read(space->machine, "IN0");
		UINT8 inport1 = input_port_read(space->machine, "IN1");

		/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
		than one, a real ColecoVision think that it is a third button, so we are going to emulate
		the right behaviour */

		UINT8 data = 0x0F;	/* No key pressed by default */

		if (!(inport6 & 0x01)) /* If Driving Controller enabled -> no keypad 1*/
		{
		    if (!(inport0 & 0x01)) data &= 0x0A; /* 0 */
		    if (!(inport0 & 0x02)) data &= 0x0D; /* 1 */
		    if (!(inport0 & 0x04)) data &= 0x07; /* 2 */
		    if (!(inport0 & 0x08)) data &= 0x0C; /* 3 */
		    if (!(inport0 & 0x10)) data &= 0x02; /* 4 */
		    if (!(inport0 & 0x20)) data &= 0x03; /* 5 */
		    if (!(inport0 & 0x40)) data &= 0x0E; /* 6 */
		    if (!(inport0 & 0x80)) data &= 0x05; /* 7 */
		    if (!(inport1 & 0x01)) data &= 0x01; /* 8 */
		    if (!(inport1 & 0x02)) data &= 0x0B; /* 9 */
		    if (!(inport1 & 0x04)) data &= 0x06; /* # */
		    if (!(inport1 & 0x08)) data &= 0x09; /* . */
		}
		return (inport1 & 0x70) | (data);
	}
	/* Joystick and fire 2 (SAC Red Button) */
	else
	{
		UINT8 data = input_port_read(space->machine, "IN2") & 0xCF;

		if (inport6 & 0x07) /* If Extra Contollers enabled */
		{
		    if (joy_status[0] == 0) data |= 0x30; /* Spinner Move Left*/
		    else if (joy_status[0] == 1) data |= 0x20; /* Spinner Move Right */
		}

		return data | 0x80;
	}
}

static READ8_HANDLER( paddle_2_r )
{
	/* Keypad and fire 1 */
	if (joy_mode == 0)
	{
		UINT8 inport3 = input_port_read(space->machine, "IN3");
		UINT8 inport4 = input_port_read(space->machine, "IN4");

		/* Numeric pad buttons are not independent on a real ColecoVision, if you push more
		than one, a real ColecoVision think that it is a third button, so we are going to emulate
		the right behaviour */

		UINT8 data = 0x0F;	/* No key pressed by default */

		if (!(inport3 & 0x01)) data &= 0x0A; /* 0 */
		if (!(inport3 & 0x02)) data &= 0x0D; /* 1 */
		if (!(inport3 & 0x04)) data &= 0x07; /* 2 */
		if (!(inport3 & 0x08)) data &= 0x0C; /* 3 */
		if (!(inport3 & 0x10)) data &= 0x02; /* 4 */
		if (!(inport3 & 0x20)) data &= 0x03; /* 5 */
		if (!(inport3 & 0x40)) data &= 0x0E; /* 6 */
		if (!(inport3 & 0x80)) data &= 0x05; /* 7 */
		if (!(inport4 & 0x01)) data &= 0x01; /* 8 */
		if (!(inport4 & 0x02)) data &= 0x0B; /* 9 */
		if (!(inport4 & 0x04)) data &= 0x06; /* # */
		if (!(inport4 & 0x08)) data &= 0x09; /* . */

		return (inport4 & 0x70) | (data);
	}
	/* Joystick and fire 2*/
	else
	{
		UINT8 data = input_port_read(space->machine, "IN5") & 0xCF;
		UINT8 inport6 = input_port_read_safe(space->machine, "IN6", 0);

		if (inport6 & 0x02) /* If Roller Controller enabled */
		{
		    if (joy_status[1] == 0) data |= 0x30;
		    else if (joy_status[1] == 1) data |= 0x20;
		}

		return data | 0x80;
	}
}

static WRITE8_HANDLER( paddle_off_w )
{
	joy_mode = 0;
}

static WRITE8_HANDLER( paddle_on_w )
{
	joy_mode = 1;
}

/* Memory Maps */

static ADDRESS_MAP_START( coleco_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x6000, 0x63ff) AM_RAM AM_MIRROR(0x1c00)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( coleco_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x1f) AM_WRITE(paddle_off_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x1e) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xa1, 0xa1) AM_MIRROR(0x1e) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x1f) AM_WRITE(paddle_on_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x1f) AM_DEVWRITE("sn76489a", sn76496_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x1d) AM_READ(paddle_1_r)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x1d) AM_READ(paddle_2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( czz50_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x6000, 0x63ff) AM_RAM AM_MIRROR(0x1c00)
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( czz50_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x80, 0x80) AM_MIRROR(0x1f) AM_WRITE(paddle_off_w)
	AM_RANGE(0xa0, 0xa0) AM_MIRROR(0x1e) AM_READWRITE(TMS9928A_vram_r, TMS9928A_vram_w)
	AM_RANGE(0xa1, 0xa1) AM_MIRROR(0x1e) AM_READWRITE(TMS9928A_register_r, TMS9928A_register_w)
	AM_RANGE(0xc0, 0xc0) AM_MIRROR(0x1f) AM_WRITE(paddle_on_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x1f) AM_DEVWRITE("sn76489a", sn76496_w)
	AM_RANGE(0xe0, 0xe0) AM_MIRROR(0x1d) AM_READ(paddle_1_r)
	AM_RANGE(0xe2, 0xe2) AM_MIRROR(0x1d) AM_READ(paddle_2_r)
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( coleco )
    PORT_START("IN0")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 (pad 1)") PORT_CODE(KEYCODE_0)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 (pad 1)") PORT_CODE(KEYCODE_1)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2 (pad 1)") PORT_CODE(KEYCODE_2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3 (pad 1)") PORT_CODE(KEYCODE_3)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 (pad 1)") PORT_CODE(KEYCODE_4)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 (pad 1)") PORT_CODE(KEYCODE_5)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 (pad 1)") PORT_CODE(KEYCODE_6)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 (pad 1)") PORT_CODE(KEYCODE_7)

    PORT_START("IN1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 (pad 1)") PORT_CODE(KEYCODE_8)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9 (pad 1)") PORT_CODE(KEYCODE_9)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("# (pad 1)") PORT_CODE(KEYCODE_MINUS)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". (pad 1)") PORT_CODE(KEYCODE_EQUALS)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN3")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0 (pad 2)") PORT_CODE(KEYCODE_0_PAD)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1 (pad 2)") PORT_CODE(KEYCODE_1_PAD)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2 (pad 2)") PORT_CODE(KEYCODE_2_PAD)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3 (pad 2)") PORT_CODE(KEYCODE_3_PAD)
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4 (pad 2)") PORT_CODE(KEYCODE_4_PAD)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5 (pad 2)") PORT_CODE(KEYCODE_5_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6 (pad 2)") PORT_CODE(KEYCODE_6_PAD)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7 (pad 2)") PORT_CODE(KEYCODE_7_PAD)

    PORT_START("IN4")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8 (pad 2)") PORT_CODE(KEYCODE_8_PAD)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9 (pad 2)") PORT_CODE(KEYCODE_9_PAD)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("# (pad 2)") PORT_CODE(KEYCODE_MINUS_PAD)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(". (pad 2)") PORT_CODE(KEYCODE_PLUS_PAD)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN5")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN6")
    PORT_DIPNAME( 0x07, 0x00, "Extra Controllers" )
    PORT_DIPSETTING(	0x00, DEF_STR( None ) )
    PORT_DIPSETTING(	0x01, "Driving Controller" )
    PORT_DIPSETTING(	0x02, "Roller Controller" )
    PORT_DIPSETTING(	0x04, "Super Action Controllers" )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("SAC Blue Button P1")	PORT_CODE(KEYCODE_Z)
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("SAC Purple Button P1")	PORT_CODE(KEYCODE_X)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("SAC Blue Button P2")	PORT_CODE(KEYCODE_Q) PORT_PLAYER(2)
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("SAC Purple Button P2")	PORT_CODE(KEYCODE_W) PORT_PLAYER(2)

    PORT_START("IN7")	// Extra Controls (Driving Controller, SAC P1 slider, Roller Controller X Axis)
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_L) PORT_CODE_INC(KEYCODE_J) PORT_RESET

    PORT_START("IN8")	// Extra Controls (SAC P2 slider, Roller Controller Y Axis)
    PORT_BIT( 0x0f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_CODE_DEC(KEYCODE_I) PORT_CODE_INC(KEYCODE_K) PORT_RESET PORT_PLAYER(2)
INPUT_PORTS_END

static INPUT_PORTS_START( czz50 )
    PORT_START("IN0")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')

    PORT_START("IN1")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("#") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('#')
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME(".") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('.')
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )		PORT_PLAYER(1)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN2")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(1)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(1)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(1)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(1)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )		PORT_PLAYER(1)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN3")
    PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

    PORT_START("IN4")
    PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START("IN5")
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )	PORT_PLAYER(2)
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )	PORT_PLAYER(2)
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )	PORT_PLAYER(2)
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )	PORT_PLAYER(2)
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )		PORT_PLAYER(2)
    PORT_BIT( 0xb0, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/***************************************************************************

  The interrupts come from the vdp. The vdp (tms9928a) interrupt can go up
  and down; the Coleco only uses nmi interrupts (which is just a pulse). They
  are edge-triggered: as soon as the vdp interrupt line goes up, an interrupt
  is generated. Nothing happens when the line stays up or goes down.

  To emulate this correctly, we set a callback in the tms9928a (they
  can occur mid-frame). At every frame we call the TMS9928A_interrupt
  because the vdp needs to know when the end-of-frame occurs, but we don't
  return an interrupt.

***************************************************************************/

static INTERRUPT_GEN( coleco_interrupt )
{
    TMS9928A_interrupt(device->machine);
}

static void coleco_vdp_interrupt(running_machine *machine, int state)
{
	static int last_state = 0;

    // only if it goes up
	if (state && !last_state) cpu_set_input_line(machine->cpu[0], INPUT_LINE_NMI, PULSE_LINE);
	last_state = state;
}

static TIMER_CALLBACK( paddle_callback )
{
    int port7 = input_port_read_safe(machine, "IN7", 0);
    int port8 = input_port_read_safe(machine, "IN8", 0);

    if (port7 == 0)
		joy_status[0] = 0;
    else if (port7 & 0x08)
		joy_status[0] = -1;
    else
		joy_status[0] = 1;

    if (port8 == 0)
		joy_status[1] = 0;
    else if (port8 & 0x08)
		joy_status[1] = -1;
    else
		joy_status[1] = 1;

    if (joy_status[0] || joy_status[1])
		cpu_set_input_line(machine->cpu[0], INPUT_LINE_IRQ0, HOLD_LINE);
}

/* Machine Initialization */

static const TMS9928a_interface tms9928a_interface =
{
	TMS99x8A,
	0x4000,
	0, 0,
	coleco_vdp_interrupt
};

static MACHINE_START( coleco )
{
	TMS9928A_configure(&tms9928a_interface);
}

static MACHINE_RESET( coleco )
{
    cpu_set_input_line_vector(machine->cpu[0], INPUT_LINE_IRQ0, 0xff);
	memset(&memory_region(machine, "maincpu")[0x6000], 0xff, 0x400);	// initialize RAM
    timer_pulse(machine, ATTOTIME_IN_MSEC(20), NULL, 0, paddle_callback);
}

//static int coleco_cart_verify(const UINT8 *cartdata, size_t size)
//{
//	int retval = IMAGE_VERIFY_FAIL;
//
//	/* Verify the file is in Colecovision format */
//	if ((cartdata[0] == 0xAA) && (cartdata[1] == 0x55)) /* Production Cartridge */
//		retval = IMAGE_VERIFY_PASS;
//	if ((cartdata[0] == 0x55) && (cartdata[1] == 0xAA)) /* "Test" Cartridge. Some games use this method to skip ColecoVision title screen and delay */
//		retval = IMAGE_VERIFY_PASS;
//
//	return retval;
//}

static DEVICE_IMAGE_LOAD( czz50_cart )
{
	int size = image_length(image);
	UINT8 *ptr = memory_region(image->machine, "maincpu") + 0x8000;

	if (image_fread(image, ptr, size ) != size)
	{
		return INIT_FAIL;
	}

	return INIT_PASS;
}

/* Machine Drivers */

static MACHINE_DRIVER_START( coleco )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", Z80, XTAL_7_15909MHz/2)	// 3.579545 MHz
	MDRV_CPU_PROGRAM_MAP(coleco_map, 0)
	MDRV_CPU_IO_MAP(coleco_io_map, 0)
	MDRV_CPU_VBLANK_INT("screen", coleco_interrupt)

	MDRV_MACHINE_START(coleco)
	MDRV_MACHINE_RESET(coleco)

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, XTAL_7_15909MHz/2)	/* 3.579545 MHz */
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,col,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( czz50 )
	// basic machine hardware
	MDRV_CPU_ADD("maincpu", Z80, XTAL_7_15909MHz/2)	// ???
	MDRV_CPU_PROGRAM_MAP(czz50_map, 0)
	MDRV_CPU_IO_MAP(czz50_io_map, 0)
	MDRV_CPU_VBLANK_INT("screen", coleco_interrupt)

	MDRV_MACHINE_START(coleco)
	MDRV_MACHINE_RESET(coleco)

    // video hardware
	MDRV_IMPORT_FROM(tms9928a)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/262)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("sn76489a", SN76489A, XTAL_7_15909MHz/2)	// ???
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* cartridge */
	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("rom,col,bin")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(czz50_cart)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( dina )
	MDRV_IMPORT_FROM(czz50)
	MDRV_SCREEN_MODIFY("screen")
	MDRV_SCREEN_REFRESH_RATE((float)XTAL_10_738635MHz/2/342/313)
MACHINE_DRIVER_END

/* ROMs */

ROM_START (coleco)
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "coleco.rom", 0x0000, 0x2000, CRC(3aa93ef3) SHA1(45bedc4cbdeac66c7df59e9e599195c778d86a92) )
	ROM_CART_LOAD("cart", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START (colecoa)
    // differences to 0x3aa93ef3 modified characters, added a pad 2 related fix
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "colecoa.rom", 0x0000, 0x2000, CRC(39bb16fc) SHA1(99ba9be24ada3e86e5c17aeecb7a2d68c5edfe59) )
	ROM_CART_LOAD("cart", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START (colecob)
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "svi603.rom", 0x0000, 0x2000, CRC(19e91b82) SHA1(8a30abe5ffef810b0f99b86db38b1b3c9d259b78) )
	ROM_CART_LOAD("cart", 0x8000, 0x8000, ROM_NOMIRROR | ROM_OPTIONAL)
ROM_END

ROM_START( czz50 )
    ROM_REGION( 0x10000, "maincpu", 0 )
    ROM_LOAD( "czz50.rom", 0x0000, 0x2000, CRC(4999abc6) SHA1(96aecec3712c94517103d894405bc98a7dafa440) )
    ROM_CONTINUE(		  0x8000, 0x2000 )
ROM_END

#define rom_dina rom_czz50
#define rom_prsarcde rom_czz50


/* System Drivers */

//    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT     INIT    CONFIG  COMPANY				FULLNAME							FLAGS
CONS( 1982, coleco,   0,		0,		coleco,   coleco,   0,		0,	"Coleco",			"ColecoVision",						0 )
CONS( 1982, colecoa,  coleco,	0,		coleco,   coleco,   0,		0,	"Coleco",			"ColecoVision (Thick Characters)",	0 )
CONS( 1983, colecob,  coleco,	0,		coleco,   coleco,   0,		0,	"Spectravideo",		"SVI-603 Coleco Game Adapter",		0 )
CONS( 1986, czz50,	  0,		0,		czz50,	  czz50,	0,		0,	"Bit Corporation",	"Chuang Zao Zhe 50",				0 )
CONS( 1988, dina,	  czz50,	0,		dina,	  czz50,	0,		0,	"Telegames",		"Dina",								0 )
CONS( 1988, prsarcde, czz50,	0,		czz50,	  czz50,	0,		0,	"Telegames",		"Personal Arcade",					0 )
