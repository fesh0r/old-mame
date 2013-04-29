/***************************************************************************

  a7800.c

  Driver file to handle emulation of the Atari 7800.

  Dan Boris

    2002/05/13 kubecj   added more banks for bankswitching
                            added PAL machine description
                            changed clock to be precise

    2012/10/25 Robert Tuccitto  NTSC Color Generator utilized for
                color palette with hue shift/start
                based on observation of several
                systems across multiple displays

    2012/11/09 Robert Tuccitto  Fixed 3 degree hue begin point
                miscalculation of color palette

    2012/12/05 Robert Tuccitto  Implemented proper IRE and phase
               value to the palette

    2012/12/14 Robert Tuccitto  Adjusted colorburst/tint/hue of entire
               palette to closer reflect default hardware configuration
               setting of ~180 degrees.  Palette settings now correspond
               documented and calculated settings as follows:

               Contrast = 0.0526 --> 0.05
               Brightness = 0.0 --> 0.00
               Color = 0.2162 --> 0.22
               Phase = 25.714 --> 25.7
               Colorburst/Hue = 180 degrees

    2013/02/27 Robert Tuccitto  Palette rebuild due to misaligned
               data references.  Corrected PAL color sequence order.

    2013/03/19 Robert Tuccitto  Stripped palette to raw video output
               values removing YIQ/YUV infer data.

    2013/04/02 Robert Tuccitto  Corrected rotation values and errors
           including duplicate entries for palette.

    2013/04/07 Robert Tuccitto  Address map locations for the XBOARD
           added.

***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "sound/pokey.h"
#include "sound/tiaintf.h"
#include "imagedev/cartslot.h"
#include "machine/6532riot.h"
#include "includes/a7800.h"


#define A7800_NTSC_Y1   XTAL_14_31818MHz
#define CLK_PAL 1773447


/***************************************************************************
    ADDRESS MAPS
***************************************************************************/

static ADDRESS_MAP_START( a7800_mem, AS_PROGRAM, 8, a7800_state )
	AM_RANGE(0x0000, 0x001f) AM_MIRROR(0x300) AM_READWRITE(a7800_TIA_r, a7800_TIA_w)
	AM_RANGE(0x0020, 0x003f) AM_MIRROR(0x300) AM_READWRITE(a7800_MARIA_r, a7800_MARIA_w)
	AM_RANGE(0x0040, 0x00ff) AM_READ_BANK("bank5") AM_WRITE(a7800_RAM0_w)   /* RAM (6116 block 0) */
	AM_RANGE(0x0140, 0x01ff) AM_RAMBANK("bank6")    /* RAM (6116 block 1) */
	AM_RANGE(0x0280, 0x02ff) AM_DEVREADWRITE("riot", riot6532_device, read, write)
	AM_RANGE(0x0450, 0x045f) /*XBOARD POKEY1*/
	AM_RANGE(0x0460, 0x046f) /*XBOARD POKEY2*/
	AM_RANGE(0x0470, 0x047f) /*XBOARD CTRL*/
	AM_RANGE(0x0480, 0x04ff) AM_MIRROR(0x100) AM_RAM    /* RIOT RAM */
	AM_RANGE(0x1800, 0x27ff) AM_RAM
	AM_RANGE(0x2800, 0x2fff) AM_RAMBANK("bank7")    /* MAINRAM */
	AM_RANGE(0x3000, 0x37ff) AM_RAMBANK("bank7")    /* MAINRAM */
	AM_RANGE(0x3800, 0x3fff) AM_RAMBANK("bank7")    /* MAINRAM */
	AM_RANGE(0x4000, 0x7fff) AM_ROMBANK("bank1")                        /* f18 hornet */
	AM_RANGE(0x8000, 0x9fff) AM_ROMBANK("bank2")                        /* sc */
	AM_RANGE(0xa000, 0xbfff) AM_ROMBANK("bank3")                        /* sc + ac */
	AM_RANGE(0xc000, 0xdfff) AM_ROMBANK("bank4")                        /* ac */
	AM_RANGE(0xe000, 0xffff) AM_ROM
	AM_RANGE(0x4000, 0xffff) AM_WRITE(a7800_cart_w) /* XBOARD SRAM */
ADDRESS_MAP_END


/***************************************************************************
    INPUT PORTS
***************************************************************************/

static INPUT_PORTS_START( a7800 )
	PORT_START("joysticks")            /* IN0 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP)    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT)  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT) PORT_PLAYER(1) PORT_8WAY

	PORT_START("buttons")              /* IN1 */
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_BUTTON1)       PORT_PLAYER(2)
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_BUTTON1)       PORT_PLAYER(1)
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_BUTTON2)       PORT_PLAYER(2)
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_BUTTON2)       PORT_PLAYER(1)
	PORT_BIT(0xF0, IP_ACTIVE_LOW, IPT_UNUSED)

	PORT_START("vblank")               /* IN2 */
	PORT_BIT(0x7F, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_CUSTOM) PORT_VBLANK("screen")

	PORT_START("console_buttons")      /* IN3 */
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Reset")         PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME("Select")        PORT_CODE(KEYCODE_S)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_OTHER)  PORT_NAME(DEF_STR(Pause))  PORT_CODE(KEYCODE_O)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_DIPNAME(0x40, 0x00, "Left Difficulty Switch")
	PORT_DIPSETTING(0x40, "A" )
	PORT_DIPSETTING(0x00, "B" )
	PORT_DIPNAME(0x80, 0x00, "Right Difficulty Switch")
	PORT_DIPSETTING(0x80, "A" )
	PORT_DIPSETTING(0x00, "B" )
INPUT_PORTS_END


/***************************************************************************
    PALETTE
***************************************************************************/

#define NTSC_GREY \
	MAKE_RGB(0x00,0x00,0x00), MAKE_RGB(0x12,0x12,0x12), MAKE_RGB(0x24,0x24,0x24), MAKE_RGB(0x36,0x36,0x36), \
	MAKE_RGB(0x47,0x47,0x47), MAKE_RGB(0x59,0x59,0x59), MAKE_RGB(0x6B,0x6B,0x6B), MAKE_RGB(0x7D,0x7D,0x7D), \
	MAKE_RGB(0x8F,0x8F,0x8F), MAKE_RGB(0xA1,0xA1,0xA1), MAKE_RGB(0xB2,0xB2,0xB2), MAKE_RGB(0xC4,0xC4,0xC4), \
	MAKE_RGB(0xD6,0xD6,0xD6), MAKE_RGB(0xE8,0xE8,0xE8), MAKE_RGB(0xFA,0xFA,0xFA), MAKE_RGB(0xFF,0xFF,0xFF   )

#define NTSC_GOLD \
	MAKE_RGB(0x1A,0x07,0x00), MAKE_RGB(0x2C,0x19,0x00), MAKE_RGB(0x3E,0x2B,0x00), MAKE_RGB(0x50,0x3D,0x00), \
	MAKE_RGB(0x61,0x4E,0x00), MAKE_RGB(0x73,0x60,0x00), MAKE_RGB(0x85,0x72,0x03), MAKE_RGB(0x97,0x84,0x15), \
	MAKE_RGB(0xA9,0x96,0x27), MAKE_RGB(0xBB,0xA8,0x39), MAKE_RGB(0xCD,0xB9,0x4A), MAKE_RGB(0xDE,0xCB,0x5C), \
	MAKE_RGB(0xF0,0xDD,0x6E), MAKE_RGB(0xFF,0xEF,0x80), MAKE_RGB(0xFF,0xFF,0x92), MAKE_RGB(0xFF,0xFF,0xA4   )

#define NTSC_ORANGE \
	MAKE_RGB(0x31,0x00,0x00), MAKE_RGB(0x43,0x07,0x00), MAKE_RGB(0x54,0x19,0x00), MAKE_RGB(0x66,0x2B,0x00), \
	MAKE_RGB(0x78,0x3D,0x00), MAKE_RGB(0x8A,0x4F,0x0F), MAKE_RGB(0x9C,0x61,0x21), MAKE_RGB(0xAE,0x72,0x33), \
	MAKE_RGB(0xC0,0x84,0x45), MAKE_RGB(0xD1,0x96,0x57), MAKE_RGB(0xE3,0xA8,0x69), MAKE_RGB(0xF5,0xBA,0x7B), \
	MAKE_RGB(0xFF,0xCC,0x8C), MAKE_RGB(0xFF,0xDE,0x9E), MAKE_RGB(0xFF,0xEF,0xB0), MAKE_RGB(0xFF,0xFF,0xC2   )

#define NTSC_RED_ORANGE \
	MAKE_RGB(0x3E,0x00,0x00), MAKE_RGB(0x50,0x00,0x00), MAKE_RGB(0x62,0x0A,0x07), MAKE_RGB(0x73,0x1C,0x19), \
	MAKE_RGB(0x85,0x2D,0x2B), MAKE_RGB(0x97,0x3F,0x3C), MAKE_RGB(0xA9,0x51,0x4E), MAKE_RGB(0xBB,0x63,0x60), \
	MAKE_RGB(0xCD,0x75,0x72), MAKE_RGB(0xDF,0x87,0x84), MAKE_RGB(0xF0,0x99,0x96), MAKE_RGB(0xFF,0xAA,0xA7), \
	MAKE_RGB(0xFF,0xBC,0xB9), MAKE_RGB(0xFF,0xCE,0xCB), MAKE_RGB(0xFF,0xE0,0xDD), MAKE_RGB(0xFF,0xF2,0xEF   )

#define NTSC_PINK \
	MAKE_RGB(0x3F,0x00,0x16), MAKE_RGB(0x51,0x00,0x28), MAKE_RGB(0x62,0x00,0x39), MAKE_RGB(0x74,0x11,0x4B), \
	MAKE_RGB(0x86,0x23,0x5D), MAKE_RGB(0x98,0x35,0x6F), MAKE_RGB(0xAA,0x47,0x81), MAKE_RGB(0xBC,0x59,0x93), \
	MAKE_RGB(0xCE,0x6B,0xA5), MAKE_RGB(0xDF,0x7C,0xB6), MAKE_RGB(0xF1,0x8E,0xC8), MAKE_RGB(0xFF,0xA0,0xDA), \
	MAKE_RGB(0xFF,0xB2,0xEC), MAKE_RGB(0xFF,0xC4,0xFE), MAKE_RGB(0xFF,0xD6,0xFF), MAKE_RGB(0xFF,0xE8,0xFF   )

#define NTSC_PURPLE \
	MAKE_RGB(0x33,0x00,0x44), MAKE_RGB(0x45,0x00,0x56), MAKE_RGB(0x57,0x00,0x68), MAKE_RGB(0x69,0x0E,0x7A), \
	MAKE_RGB(0x7B,0x20,0x8B), MAKE_RGB(0x8D,0x32,0x9D), MAKE_RGB(0x9E,0x44,0xAF), MAKE_RGB(0xB0,0x56,0xC1), \
	MAKE_RGB(0xC2,0x67,0xD3), MAKE_RGB(0xD4,0x79,0xE5), MAKE_RGB(0xE6,0x8B,0xF7), MAKE_RGB(0xF8,0x9D,0xFF), \
	MAKE_RGB(0xFF,0xAF,0xFF), MAKE_RGB(0xFF,0xC1,0xFF), MAKE_RGB(0xFF,0xD3,0xFF), MAKE_RGB(0xFF,0xE4,0xFF   )

#define NTSC_PURPLE_BLUE \
	MAKE_RGB(0x1E,0x00,0x65), MAKE_RGB(0x30,0x00,0x77), MAKE_RGB(0x41,0x01,0x89), MAKE_RGB(0x53,0x13,0x9B), \
	MAKE_RGB(0x65,0x25,0xAC), MAKE_RGB(0x77,0x37,0xBE), MAKE_RGB(0x89,0x48,0xD0), MAKE_RGB(0x9B,0x5A,0xE2), \
	MAKE_RGB(0xAD,0x6C,0xF4), MAKE_RGB(0xBE,0x7E,0xFF), MAKE_RGB(0xD0,0x90,0xFF), MAKE_RGB(0xE2,0xA2,0xFF), \
	MAKE_RGB(0xF4,0xB3,0xFF), MAKE_RGB(0xFF,0xC5,0xFF), MAKE_RGB(0xFF,0xD7,0xFF), MAKE_RGB(0xFF,0xE9,0xFF   )

#define NTSC_BLUE1 \
	MAKE_RGB(0x02,0x00,0x72), MAKE_RGB(0x14,0x00,0x84), MAKE_RGB(0x26,0x0C,0x96), MAKE_RGB(0x38,0x1E,0xA7), \
	MAKE_RGB(0x4A,0x30,0xB9), MAKE_RGB(0x5B,0x42,0xCB), MAKE_RGB(0x6D,0x54,0xDD), MAKE_RGB(0x7F,0x66,0xEF), \
	MAKE_RGB(0x91,0x78,0xFF), MAKE_RGB(0xA3,0x89,0xFF), MAKE_RGB(0xB5,0x9B,0xFF), MAKE_RGB(0xC7,0xAD,0xFF), \
	MAKE_RGB(0xD8,0xBF,0xFF), MAKE_RGB(0xEA,0xD1,0xFF), MAKE_RGB(0xFC,0xE3,0xFF), MAKE_RGB(0xFF,0xF4,0xFF   )

#define NTSC_BLUE2 \
	MAKE_RGB(0x00,0x00,0x68), MAKE_RGB(0x00,0x0B,0x7A), MAKE_RGB(0x0A,0x1D,0x8C), MAKE_RGB(0x1C,0x2E,0x9E), \
	MAKE_RGB(0x2E,0x40,0xB0), MAKE_RGB(0x40,0x52,0xC2), MAKE_RGB(0x51,0x64,0xD4), MAKE_RGB(0x63,0x76,0xE5), \
	MAKE_RGB(0x75,0x88,0xF7), MAKE_RGB(0x87,0x99,0xFF), MAKE_RGB(0x99,0xAB,0xFF), MAKE_RGB(0xAB,0xBD,0xFF), \
	MAKE_RGB(0xBD,0xCF,0xFF), MAKE_RGB(0xCE,0xE1,0xFF), MAKE_RGB(0xE0,0xF3,0xFF), MAKE_RGB(0xF2,0xFF,0xFF   )

#define NTSC_LIGHT_BLUE \
	MAKE_RGB(0x00,0x0A,0x4A), MAKE_RGB(0x00,0x1C,0x5C), MAKE_RGB(0x00,0x2E,0x6E), MAKE_RGB(0x05,0x40,0x80), \
	MAKE_RGB(0x17,0x52,0x92), MAKE_RGB(0x29,0x64,0xA4), MAKE_RGB(0x3B,0x75,0xB5), MAKE_RGB(0x4C,0x87,0xC7), \
	MAKE_RGB(0x5E,0x99,0xD9), MAKE_RGB(0x70,0xAB,0xEB), MAKE_RGB(0x82,0xBD,0xFD), MAKE_RGB(0x94,0xCF,0xFF), \
	MAKE_RGB(0xA6,0xE0,0xFF), MAKE_RGB(0xB8,0xF2,0xFF), MAKE_RGB(0xC9,0xFF,0xFF), MAKE_RGB(0xDB,0xFF,0xFF   )

#define NTSC_TURQUOISE \
	MAKE_RGB(0x00,0x1A,0x1E), MAKE_RGB(0x00,0x2C,0x2F), MAKE_RGB(0x00,0x3D,0x41), MAKE_RGB(0x00,0x4F,0x53), \
	MAKE_RGB(0x0A,0x61,0x65), MAKE_RGB(0x1B,0x73,0x77), MAKE_RGB(0x2D,0x85,0x89), MAKE_RGB(0x3F,0x97,0x9A), \
	MAKE_RGB(0x51,0xA9,0xAC), MAKE_RGB(0x63,0xBA,0xBE), MAKE_RGB(0x75,0xCC,0xD0), MAKE_RGB(0x87,0xDE,0xE2), \
	MAKE_RGB(0x98,0xF0,0xF4), MAKE_RGB(0xAA,0xFF,0xFF), MAKE_RGB(0xBC,0xFF,0xFF), MAKE_RGB(0xCE,0xFF,0xFF   )

#define NTSC_GREEN_BLUE \
	MAKE_RGB(0x00,0x24,0x00), MAKE_RGB(0x00,0x36,0x00), MAKE_RGB(0x00,0x48,0x0F), MAKE_RGB(0x00,0x5A,0x20), \
	MAKE_RGB(0x09,0x6B,0x32), MAKE_RGB(0x1A,0x7D,0x44), MAKE_RGB(0x2C,0x8F,0x56), MAKE_RGB(0x3E,0xA1,0x68), \
	MAKE_RGB(0x50,0xB3,0x7A), MAKE_RGB(0x62,0xC5,0x8C), MAKE_RGB(0x74,0xD7,0x9D), MAKE_RGB(0x86,0xE8,0xAF), \
	MAKE_RGB(0x97,0xFA,0xC1), MAKE_RGB(0xA9,0xFF,0xD3), MAKE_RGB(0xBB,0xFF,0xE5), MAKE_RGB(0xCD,0xFF,0xF7   )

#define NTSC_GREEN \
	MAKE_RGB(0x00,0x27,0x00), MAKE_RGB(0x00,0x39,0x00), MAKE_RGB(0x00,0x4B,0x00), MAKE_RGB(0x02,0x5D,0x00), \
	MAKE_RGB(0x14,0x6F,0x04), MAKE_RGB(0x26,0x81,0x16), MAKE_RGB(0x38,0x92,0x28), MAKE_RGB(0x49,0xA4,0x39), \
	MAKE_RGB(0x5B,0xB6,0x4B), MAKE_RGB(0x6D,0xC8,0x5D), MAKE_RGB(0x7F,0xDA,0x6F), MAKE_RGB(0x91,0xEC,0x81), \
	MAKE_RGB(0xA3,0xFE,0x93), MAKE_RGB(0xB5,0xFF,0xA5), MAKE_RGB(0xC6,0xFF,0xB6), MAKE_RGB(0xD8,0xFF,0xC8   )

#define NTSC_YELLOW_GREEN \
	MAKE_RGB(0x00,0x23,0x00), MAKE_RGB(0x00,0x35,0x00), MAKE_RGB(0x06,0x47,0x00), MAKE_RGB(0x18,0x58,0x00), \
	MAKE_RGB(0x29,0x6A,0x00), MAKE_RGB(0x3B,0x7C,0x00), MAKE_RGB(0x4D,0x8E,0x06), MAKE_RGB(0x5F,0xA0,0x18), \
	MAKE_RGB(0x71,0xB2,0x2A), MAKE_RGB(0x83,0xC3,0x3C), MAKE_RGB(0x94,0xD5,0x4E), MAKE_RGB(0xA6,0xE7,0x60), \
	MAKE_RGB(0xB8,0xF9,0x72), MAKE_RGB(0xCA,0xFF,0x83), MAKE_RGB(0xDC,0xFF,0x95), MAKE_RGB(0xEE,0xFF,0xA7   )

#define NTSC_ORANGE_GREEN \
	MAKE_RGB(0x00,0x17,0x00), MAKE_RGB(0x0F,0x29,0x00), MAKE_RGB(0x21,0x3B,0x00), MAKE_RGB(0x33,0x4D,0x00), \
	MAKE_RGB(0x45,0x5F,0x00), MAKE_RGB(0x57,0x71,0x00), MAKE_RGB(0x68,0x83,0x00), MAKE_RGB(0x7A,0x94,0x0B), \
	MAKE_RGB(0x8C,0xA6,0x1D), MAKE_RGB(0x9E,0xB8,0x2F), MAKE_RGB(0xB0,0xCA,0x41), MAKE_RGB(0xC2,0xDC,0x52), \
	MAKE_RGB(0xD4,0xEE,0x64), MAKE_RGB(0xE5,0xFF,0x76), MAKE_RGB(0xF7,0xFF,0x88), MAKE_RGB(0xFF,0xFF,0x9A   )

#define NTSC_LIGHT_ORANGE \
	MAKE_RGB(0x19,0x07,0x00), MAKE_RGB(0x2B,0x19,0x00), MAKE_RGB(0x3D,0x2B,0x00), MAKE_RGB(0x4F,0x3D,0x00), \
	MAKE_RGB(0x61,0x4F,0x00), MAKE_RGB(0x73,0x61,0x00), MAKE_RGB(0x84,0x73,0x02), MAKE_RGB(0x96,0x84,0x14), \
	MAKE_RGB(0xA8,0x96,0x26), MAKE_RGB(0xBA,0xA8,0x38), MAKE_RGB(0xCC,0xBA,0x4A), MAKE_RGB(0xDE,0xCC,0x5C), \
	MAKE_RGB(0xF0,0xDE,0x6E), MAKE_RGB(0xFF,0xEF,0x7F), MAKE_RGB(0xFF,0xFF,0x91), MAKE_RGB(0xFF,0xFF,0xA3   )

static const rgb_t a7800_palette[256*3] =
{
	NTSC_GREY,
	NTSC_GOLD,
	NTSC_ORANGE,
	NTSC_RED_ORANGE,
	NTSC_PINK,
	NTSC_PURPLE,
	NTSC_PURPLE_BLUE,
	NTSC_BLUE1,
	NTSC_BLUE2,
	NTSC_LIGHT_BLUE,
	NTSC_TURQUOISE,
	NTSC_GREEN_BLUE,
	NTSC_GREEN,
	NTSC_YELLOW_GREEN,
	NTSC_ORANGE_GREEN,
	NTSC_LIGHT_ORANGE
};

static const rgb_t a7800p_palette[256*3] =
{
	NTSC_GREY,
	NTSC_ORANGE_GREEN,
	NTSC_GOLD,
	NTSC_ORANGE,
	NTSC_RED_ORANGE,
	NTSC_PINK,
	NTSC_PURPLE,
	NTSC_PURPLE_BLUE,
	NTSC_BLUE1,
	NTSC_BLUE2,
	NTSC_LIGHT_BLUE,
	NTSC_TURQUOISE,
	NTSC_GREEN_BLUE,
	NTSC_GREEN,
	NTSC_YELLOW_GREEN,
	NTSC_ORANGE_GREEN
};


/* Initialise the palette */
void a7800_state::palette_init()
{
	palette_set_colors(machine(), 0, a7800_palette, ARRAY_LENGTH(a7800_palette));
}


PALETTE_INIT_MEMBER(a7800_state,a7800p)
{
	palette_set_colors(machine(), 0, a7800p_palette, ARRAY_LENGTH(a7800p_palette));
}


/***************************************************************************
    MACHINE DRIVERS
***************************************************************************/

static MACHINE_CONFIG_START( a7800_ntsc, a7800_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M6502, A7800_NTSC_Y1/8) /* 1.79 MHz (switches to 1.19 MHz on TIA or RIOT access) */
	MCFG_CPU_PROGRAM_MAP(a7800_mem)
	MCFG_TIMER_DRIVER_ADD_SCANLINE("scantimer", a7800_state, a7800_interrupt, "screen", 0, 1)


	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_SIZE(640,262)
	MCFG_SCREEN_VISIBLE_AREA(0,319,25,45+204)
	MCFG_SCREEN_UPDATE_DRIVER(a7800_state, screen_update_a7800)

	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(a7800_palette))


	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_TIA_ADD("tia", 31400)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_POKEY_ADD("pokey", A7800_NTSC_Y1/8)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MCFG_RIOT6532_ADD("riot", A7800_NTSC_Y1/12, a7800_r6532_interface)

	MCFG_CARTSLOT_ADD("cart")
	MCFG_CARTSLOT_EXTENSION_LIST("bin,a78")
	MCFG_CARTSLOT_NOT_MANDATORY
	MCFG_CARTSLOT_LOAD(a7800_state,a7800_cart)
	MCFG_CARTSLOT_PARTIALHASH(a7800_partialhash)
	MCFG_CARTSLOT_INTERFACE("a7800_cart")

	/* software lists */
	MCFG_SOFTWARE_LIST_ADD("cart_list","a7800")
	MCFG_SOFTWARE_LIST_FILTER("cart_list","NTSC")
MACHINE_CONFIG_END


static MACHINE_CONFIG_DERIVED( a7800_pal, a7800_ntsc )

	/* basic machine hardware */
	MCFG_CPU_MODIFY("maincpu")
	MCFG_CPU_CLOCK(CLK_PAL)
//  MCFG_TIMER_ADD_SCANLINE("scantimer", a7800_interrupt, "screen", 0, 1)

	MCFG_SCREEN_MODIFY( "screen" )
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_SIZE(640,312)
	MCFG_SCREEN_VISIBLE_AREA(0,319,50,50+225)
	MCFG_PALETTE_INIT_OVERRIDE(a7800_state, a7800p )

	/* sound hardware */
	MCFG_DEVICE_REMOVE("pokey")
	MCFG_POKEY_ADD("pokey", CLK_PAL)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* devices */
	MCFG_DEVICE_REMOVE("riot")
	MCFG_RIOT6532_ADD("riot", 3546894/3, a7800_r6532_interface)

	/* software lists */
	MCFG_DEVICE_REMOVE("cart_list")
	MCFG_SOFTWARE_LIST_ADD("cart_list","a7800")
	MCFG_SOFTWARE_LIST_FILTER("cart_list","PAL")
MACHINE_CONFIG_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( a7800 )
	ROM_REGION(0x40000, "maincpu", 0)
	ROM_FILL(0x0000, 0x40000, 0xff)
	ROM_SYSTEM_BIOS( 0, "a7800", "Atari 7800" )
	ROMX_LOAD("7800.u7", 0xf000, 0x1000, CRC(5d13730c) SHA1(d9d134bb6b36907c615a594cc7688f7bfcef5b43), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "a7800pr", "Atari 7800 (prototype with Asteroids)" )
	ROMX_LOAD("c300558-001a.u7", 0xc000, 0x4000, CRC(a0e10edf) SHA1(14584b1eafe9721804782d4b1ac3a4a7313e455f), ROM_BIOS(2))
ROM_END

ROM_START( a7800p )
	ROM_REGION(0x40000, "maincpu", 0)
	ROM_FILL(0x0000, 0x40000, 0xff)
	ROM_LOAD("7800pal.rom", 0xc000, 0x4000, CRC(d5b61170) SHA1(5a140136a16d1d83e4ff32a19409ca376a8df874))
ROM_END


/***************************************************************************
    GAME DRIVERS
***************************************************************************/

/*    YEAR  NAME      PARENT    COMPAT  MACHINE     INPUT     INIT          COMPANY   FULLNAME */
CONS( 1986, a7800,    0,        0,      a7800_ntsc, a7800, a7800_state,    a7800_ntsc,  "Atari",  "Atari 7800 (NTSC)" , 0)
CONS( 1986, a7800p,   a7800,    0,      a7800_pal,  a7800, a7800_state,    a7800_pal,   "Atari",  "Atari 7800 (PAL)" , 0)
