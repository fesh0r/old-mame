/***************************************************************************

	Atari Sky Diver hardware

	driver by Mike Balfour

	Games supported:
		* Sky Diver

	Known issues:
		* none at this time

****************************************************************************

	Memory Map:
	0000-00FF    R/W    PAGE ZERO RAM
	0010         R/W    H POS PLANE 1
	0011         R/W    H POS PLANE 2
	0012         R/W    H POS MAN 1
	0013         R/W    H POS MAN 2
	0014         R/W    RANGE LOAD
	0015         R/W    NOTE LOAD
	0016         R/W    NAM LD
	0017         R/W    UNUSED
	0018         R/W    V POS PLANE 1
	0019         R/W    PICTURE PLANE 1
	001A         R/W    V POS PLANE 2
	001B         R/W    PICTURE PLANE 2
	001C         R/W    V POS MAN 1
	001D         R/W    PICTURE MAN 1
	001E         R/W    V POS MAN 2
	001F         R/W    PICTURE MAN 2
	0400-077F    R/W    PLAYFIELD
	0780-07FF    R/W    MAPS TO 0000-D0
	0800-0801     W     S LAMP
	0802-0803     W     K LAMP
	0804-0805     W     START LITE 1
	0806-0807     W     START LITE 2
	0808-0809     W     Y LAMP
	080A-080B     W     D LAMP
	080C-080D     W     SOUND ENABLE
	1000-1001     W     JUMP LITE 1
	1002-1003     W     COIN LOCK OUT
	1006-1007     W     JUMP LITE 2
	1008-1009     W     WHISTLE 1
	100A-100B     W     WHISTLE 2
	100C-100D     W     NMION
	100E-100F     W     WIDTH
	1800          R     D6=LEFT 1, D7=RIGHT 1
	1801          R     D6=LEFT 2, D7=RIGHT 2
	1802          R     D6=JUMP 1, D7=CHUTE 1
	1803          R     D6=JUMP 2, D7=CHUTE 2
	1804          R     D6=(D) OPT SW: NEXT TEST, D7=(F) OPT SW
	1805          R     D6=(E) OPT SW, D7= (H) OPT SW: DIAGNOSTICS
	1806          R     D6=START 1, D7=COIN 1
	1807          R     D6=START 2, D7=COIN 2
	1808          R     D6=MISSES 2, D7=MISSES 1
	1809          R     D6=COIN 2, D7=COIN1
	180A          R     D6=HARD/EASY, D7=EXTENDED PLAY
	180B          R     D6=LANGUAGE 2, D7=LANGUAGE 1
	1810          R     D6=TEST, D7=!VBLANK
	1811          R     D6=!SLAM, D7=UNUSED
	2000          W     TIMER RESET
	2002-2003     W     I LAMP
	2004-2005     W     V LAMP
	2006-2007     W     E LAMP
	2008-2009     W     R LAMP
	200A-200B     W     OCT 1
	200C-200D     W     OCT 2
	200E-200F     W     NOISE RESET
	2800-2FFF     R     ROM 0
	3000-37FF     R     ROM 1
	3800-3FFF     R     ROM 2A
	7800-7FFF     R     ROM 2B

	If you have any questions about how this driver works, don't hesitate to
	ask.  - Mike Balfour (mab22@po.cwru.edu)

	Notes:

	The NMI interrupts are only used to read the coin switches.

***************************************************************************/

#include "driver.h"
#include "skydiver.h"

static int skydiver_nmion;



/*************************************
 *
 *	Palette generation
 *
 *************************************/

static unsigned short colortable_source[] =
{
	0x02, 0x00,
	0x02, 0x01,
	0x00, 0x02,
	0x01, 0x02
};

static PALETTE_INIT( skydiver )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff); /* white */
	palette_set_color(2,0xa0,0xa0,0xa0); /* grey */

	memcpy(colortable,colortable_source,sizeof(colortable_source));
}



/*************************************
 *
 *	Interrupt generation
 *
 *************************************/

static WRITE8_HANDLER( skydiver_nmion_w )
{
	skydiver_nmion = offset;
}


static INTERRUPT_GEN( skydiver_interrupt )
{
	/* Convert range data to divide value and write to sound */
	discrete_sound_w(0, (0x01 << (~skydiver_videoram[0x394] & 0x07)) & 0xff);	// Range 0-2

	discrete_sound_w(1,  skydiver_videoram[0x394] & 0x08);	// Range 3 - note disable
	discrete_sound_w(2, ~skydiver_videoram[0x395] & 0xff);	// Note - freq
	discrete_sound_w(3,  skydiver_videoram[0x396] & 0x0f);	// NAM - Noise Amplitude

	if (skydiver_nmion)
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}



/*************************************
 *
 *	Sound handlers
 *
 *************************************/

static WRITE8_HANDLER( skydiver_sound_enable_w )
{
	discrete_sound_w(9, offset);
}

static WRITE8_HANDLER( skydiver_whistle_w )
{
	discrete_sound_w(5 + (offset / 2), offset & 0x01);
}

static WRITE8_HANDLER( skydiver_oct_w )
{
	discrete_sound_w(7 + (offset / 2), offset & 0x01);
}

static WRITE8_HANDLER( skydiver_noise_reset_w )
{
	discrete_sound_w(4, !offset);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_READ(skydiver_wram_r)
	AM_RANGE(0x0080, 0x00ff) AM_READ(MRA8_RAM)		/* RAM B1 */
	AM_RANGE(0x0400, 0x07ff) AM_READ(MRA8_RAM)		/* RAMs K1,M1,P1,J1,N1,K/L1,L1,H/J1 */
	AM_RANGE(0x1800, 0x1800) AM_READ(input_port_0_r)
	AM_RANGE(0x1801, 0x1801) AM_READ(input_port_1_r)
	AM_RANGE(0x1802, 0x1802) AM_READ(input_port_2_r)
	AM_RANGE(0x1803, 0x1803) AM_READ(input_port_3_r)
	AM_RANGE(0x1804, 0x1804) AM_READ(input_port_4_r)
	AM_RANGE(0x1805, 0x1805) AM_READ(input_port_5_r)
	AM_RANGE(0x1806, 0x1806) AM_READ(input_port_6_r)
	AM_RANGE(0x1807, 0x1807) AM_READ(input_port_7_r)
	AM_RANGE(0x1808, 0x1808) AM_READ(input_port_8_r)
	AM_RANGE(0x1809, 0x1809) AM_READ(input_port_9_r)
	AM_RANGE(0x180a, 0x180a) AM_READ(input_port_10_r)
	AM_RANGE(0x180b, 0x180b) AM_READ(input_port_11_r)
	AM_RANGE(0x1810, 0x1810) AM_READ(input_port_12_r)
	AM_RANGE(0x1811, 0x1811) AM_READ(input_port_13_r)
	AM_RANGE(0x2000, 0x2000) AM_READ(watchdog_reset_r)
	AM_RANGE(0x2800, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf800, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_WRITE(skydiver_wram_w)
	AM_RANGE(0x0080, 0x00ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x0400, 0x07ff) AM_WRITE(skydiver_videoram_w) AM_BASE(&skydiver_videoram)
	AM_RANGE(0x0800, 0x0801) AM_WRITE(skydiver_lamp_s_w)
	AM_RANGE(0x0802, 0x0803) AM_WRITE(skydiver_lamp_k_w)
	AM_RANGE(0x0804, 0x0805) AM_WRITE(skydiver_start_lamp_1_w)
	AM_RANGE(0x0806, 0x0807) AM_WRITE(skydiver_start_lamp_2_w)
	AM_RANGE(0x0808, 0x0809) AM_WRITE(skydiver_lamp_y_w)
	AM_RANGE(0x080a, 0x080b) AM_WRITE(skydiver_lamp_d_w)
	AM_RANGE(0x080c, 0x080d) AM_WRITE(skydiver_sound_enable_w)
	// AM_RANGE(0x1000, 0x1001) AM_WRITE(skydiver_jump1_lamps_w)
	AM_RANGE(0x1002, 0x1003) AM_WRITE(skydiver_coin_lockout_w)
	// AM_RANGE(0x1006, 0x1007) AM_WRITE(skydiver_jump2_lamps_w)
	AM_RANGE(0x1008, 0x100b) AM_WRITE(skydiver_whistle_w)
	AM_RANGE(0x100c, 0x100d) AM_WRITE(skydiver_nmion_w)
	AM_RANGE(0x100e, 0x100f) AM_WRITE(skydiver_width_w)
	AM_RANGE(0x2000, 0x2000) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0x2002, 0x2003) AM_WRITE(skydiver_lamp_i_w)
	AM_RANGE(0x2004, 0x2005) AM_WRITE(skydiver_lamp_v_w)
	AM_RANGE(0x2006, 0x2007) AM_WRITE(skydiver_lamp_e_w)
	AM_RANGE(0x2008, 0x2009) AM_WRITE(skydiver_lamp_r_w)
	AM_RANGE(0x200a, 0x200d) AM_WRITE(skydiver_oct_w)
	AM_RANGE(0x200e, 0x200f) AM_WRITE(skydiver_noise_reset_w)
	AM_RANGE(0x2800, 0x3fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x7800, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf800, 0xffff) AM_WRITE(MWA8_ROM)
ADDRESS_MAP_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( skydiver )
	PORT_START /* IN0 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START /* IN1 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)

	PORT_START /* IN2 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )	/* Jump 1 */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_BUTTON2 )	/* Chute 1 */

	PORT_START /* IN3 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)	/* Jump 2 */
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)	/* Chute 2 */

	PORT_START /* IN4 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("(D) OPT SW NEXT TEST") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_NAME("(F) OPT SW") PORT_CODE(KEYCODE_F)

	PORT_START /* IN5 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_NAME("(E) OPT SW") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_NAME("(H) OPT SW DIAGNOSTICS") PORT_CODE(KEYCODE_H)

	PORT_START /* IN6 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START /* IN7 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)

	PORT_START /* IN8 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0xc0, "6" )

	PORT_START /* IN9 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x80, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START /* IN10 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hard ) )
	PORT_DIPNAME( 0x80, 0x00, "Extended Play" )
	PORT_DIPSETTING(    0x80, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START /* IN11 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPSETTING(    0x40, DEF_STR( French ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Spanish ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( German ) )

	PORT_START /* IN12 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_SERVICE( 0x40, IP_ACTIVE_LOW )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_VBLANK )

	PORT_START /* IN13 */
	PORT_BIT (0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_ADJUSTER( 67, "Whistle 1 Freq" )

	PORT_START
	PORT_ADJUSTER( 75, "Whistle 2 Freq" )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics definitions
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	64,
	1,
	{ 0 },
	{ 7, 6, 5, 4, 15, 14, 13, 12 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};


static struct GfxLayout motion_layout =
{
	16,16,
	32,
	1,
	{ 0 },
	{ 4, 5, 6, 7, 4 + 0x400*8, 5 + 0x400*8, 6 + 0x400*8, 7 + 0x400*8,
	  12, 13, 14, 15, 12 + 0x400*8, 13 + 0x400*8, 14 + 0x400*8, 15 + 0x400*8 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
	  8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	8*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,    0, 4 },
	{ REGION_GFX2, 0, &motion_layout, 0, 4 },
	{ -1 }
};



/************************************************************************/
/* skydiver Sound System Analog emulation                               */
/* Jan 2004, Derrick Renaud                                             */
/************************************************************************/

const struct discrete_555_astbl_desc skydiverWhistl555 =
{
	DISC_555_OUT_SQW | DISC_555_OUT_AC,
	5,		// B+ voltage of 555
	5.0 - 1.7,	// High output voltage of 555 (Usually v555 - 1.7)
	5.0 * 2.0 /3.0,	// normally 2/3 of v555
	5.0 / 3.0	// normally 1/3 of v555
};

const struct discrete_lfsr_desc skydiver_lfsr={
	16,				/* Bit Length */
	0,				/* Reset Value */
	0,				/* Use Bit 0 as XOR input 0 */
	14,				/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,			/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,			/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,		/* Feedback stage3 replaces the shifted register contents */
	0x000001,			/* Everything is shifted into the first bit only */
	DISC_LFSR_FLAG_OUT_INVERT,	/* Output is inverted, Active Low Reset */
	15				/* Output bit */
};

/* Nodes - Inputs */
#define SKYDIVER_RANGE_DATA	NODE_01
#define SKYDIVER_NOTE_DATA	NODE_02
#define SKYDIVER_RANGE3_EN	NODE_03
#define SKYDIVER_NOISE_DATA	NODE_04
#define SKYDIVER_NOISE_RST	NODE_05
#define SKYDIVER_WHISTLE1_EN	NODE_06
#define SKYDIVER_WHISTLE2_EN	NODE_07
#define SKYDIVER_OCT1_EN	NODE_08
#define SKYDIVER_OCT2_EN	NODE_09
#define SKYDIVER_SOUND_EN	NODE_10
/* Nodes - Sounds */
#define SKYDIVER_NOTE_SND	NODE_11
#define SKYDIVER_NOISE_SND	NODE_12
#define SKYDIVER_WHISTLE1_SND	NODE_13
#define SKYDIVER_WHISTLE2_SND	NODE_14

static DISCRETE_SOUND_START(skydiver_sound_interface)
	/************************************************/
	/* skydiver  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect  V-ampIn  Gain ratio        Relative  */
	/* Note     3.8     3.8/260.5          1000.0   */
	/* Noise    3.8     3.8/680             383.1   */
	/* Whistle  5.0     5.0/1500            228.5   */
	/************************************************/

	/************************************************/
	/* Input register mapping for skydiver          */
	/************************************************/
	/*              NODE                  ADDR  MASK     GAIN        OFFSET  INIT */
	DISCRETE_INPUT (SKYDIVER_RANGE_DATA,  0x00, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_RANGE3_EN,   0x01, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_NOTE_DATA,   0x02, 0x000f,                      0.0)
	DISCRETE_INPUTX(SKYDIVER_NOISE_DATA,  0x03, 0x000f,  383.1/15.0, 0.0,    0.0)
	DISCRETE_INPUT (SKYDIVER_NOISE_RST,   0x04, 0x000f,                      1.0)
	DISCRETE_INPUT (SKYDIVER_WHISTLE1_EN, 0x05, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_WHISTLE2_EN, 0x06, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_OCT1_EN,     0x07, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_OCT2_EN,     0x08, 0x000f,                      0.0)
	DISCRETE_INPUT (SKYDIVER_SOUND_EN,    0x09, 0x000f,                      0.0)
	/************************************************/

	/************************************************/
	/* The note generator has a selectable range    */
	/* and selectable frequency.                    */
	/* The base frequency is                        */
	/* 12.096MHz / 2 / range / note freq / 2        */
	/* The final /2 is just to give a 50% duty,     */
	/* so we can just start by 12.096MHz/4/range    */
	/* The octave is selected by 3 bits selecting   */
	/* 000 64H  = 12096MHz / 2 / 128                */
	/* 001 32H  = 12096MHz / 2 / 64                 */
	/* 010 16H  = 12096MHz / 2 / 32                 */
	/* 011  8H  = 12096MHz / 2 / 16                 */
	/* 100  4H  = 12096MHz / 2 / 8                  */
	/* 101  2H  = 12096MHz / 2 / 4                  */
	/* 110  1H  = 12096MHz / 2 / 2                  */
	/* 111 6MHz = 12096MHz / 2 / 1                  */
	/* We will convert the 3 range bits to a        */
	/* divide value in the driver before sending    */
	/* to the sound interface.                      */
	/*                                              */
	/* note data: 0xff = off,                       */
	/*            0xfe = /2,                        */
	/*            0x00 = /256                       */
	/* We will send the note data bit inverted to   */
	/* sound interface so it is easier to work with.*/
	/*                                              */
	/* The note generator is disabled by a low on   */
	/* RANGE3.                                      */
	/************************************************/
	// We will disable the divide if SKYDIVER_RANGE_DATA = 0
	DISCRETE_DIVIDE(NODE_20, SKYDIVER_RANGE_DATA, 12096000.0 /2.0 / 2.0, SKYDIVER_RANGE_DATA)
	DISCRETE_ADDER2(NODE_21, 1, SKYDIVER_NOTE_DATA, 1)
	// We will disable the divide if SKYDIVER_NOTE_DATA = 0
	DISCRETE_DIVIDE(NODE_22, SKYDIVER_NOTE_DATA, NODE_20, NODE_21)	// freq
	DISCRETE_SQUAREWAVE(SKYDIVER_NOTE_SND, SKYDIVER_RANGE3_EN, NODE_22, 1000.0, 50.0, 0, 0.0)

	/************************************************/
	/* Noise circuit is built around a noise        */
	/* generator built from 2 shift registers that  */
	/* are clocked by the 1V signal.                */
	/* 1V = HSYNC/2                                 */
	/*    = 15750/2                                 */
	/* Output is binary weighted with 4 bits of     */
	/* volume.                                      */
	/************************************************/
	DISCRETE_LFSR_NOISE(SKYDIVER_NOISE_SND, SKYDIVER_NOISE_RST, SKYDIVER_NOISE_RST, 15750.0/2.0, SKYDIVER_NOISE_DATA, 0, 0, &skydiver_lfsr)

	/************************************************/
	/* Whistle circuit is a 555 capacitor charge    */
	/* waveform.  The original game pot varies from */
	/* 0-250k, but we are going to limit it because */
	/* below 50k the frequency is too high.         */
	/* When triggered it starts at it's highest     */
	/* frequency, then decays at the rate set by    */
	/* a 68k resistor and 22uf capacitor.           */
	/************************************************/
	DISCRETE_ADJUSTMENT(NODE_30, 1, 50000, 250000, DISC_LINADJ, 15)	/* R66 */
	DISCRETE_MULTADD(NODE_31, 1, SKYDIVER_WHISTLE1_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_32, SKYDIVER_WHISTLE1_EN, NODE_31, 1.0, NODE_31, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_SWITCH(NODE_33, 1, SKYDIVER_OCT1_EN, 1e-8, 1e-8 + 3.3e-9)	/* Cap C73 & C58 */
	DISCRETE_555_ASTABLE(NODE_34, SKYDIVER_WHISTLE1_EN, 100000, NODE_30, NODE_33, NODE_32, &skydiverWhistl555)
	DISCRETE_MULTIPLY(SKYDIVER_WHISTLE1_SND, SKYDIVER_WHISTLE1_EN, NODE_34, 228.5/3.3)

	DISCRETE_ADJUSTMENT(NODE_35, 1, 50000, 250000, DISC_LINADJ, 14)	/* R65 */
	DISCRETE_MULTADD(NODE_36, 1, SKYDIVER_WHISTLE2_EN, 3.05-0.33, 0.33)
	DISCRETE_RCDISC2(NODE_37, SKYDIVER_WHISTLE2_EN, NODE_36, 1.0, NODE_36, 68000.0, 2.2e-5)	/* CV */
	DISCRETE_SWITCH(NODE_38, 1, SKYDIVER_OCT2_EN, 1e-8, 1e-8 + 3.3e-9)	/* Cap C72 & C59 */
	DISCRETE_555_ASTABLE(NODE_39, SKYDIVER_WHISTLE2_EN, 100000, NODE_35, NODE_38, NODE_37, &skydiverWhistl555)
	DISCRETE_MULTIPLY(SKYDIVER_WHISTLE2_SND, SKYDIVER_WHISTLE2_EN, NODE_39, 228.5/3.3)

	/************************************************/
	/* Final gain and ouput.                        */
	/************************************************/
	DISCRETE_ADDER4(NODE_90, SKYDIVER_SOUND_EN, SKYDIVER_NOTE_SND, SKYDIVER_NOISE_SND, SKYDIVER_WHISTLE1_SND, SKYDIVER_WHISTLE2_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0 + 383.1 + 228.5 + 228.5))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static MACHINE_DRIVER_START( skydiver )

	/* basic machine hardware */
	MDRV_CPU_ADD(M6800,3000000/4)	   /* ???? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(skydiver_interrupt,8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_INIT(skydiver)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(3)
	MDRV_COLORTABLE_LENGTH(sizeof(colortable_source) / sizeof(colortable_source[0]))

	MDRV_PALETTE_INIT(skydiver)
	MDRV_VIDEO_START(skydiver)
	MDRV_VIDEO_UPDATE(skydiver)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("discrete", DISCRETE, skydiver_sound_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( skydiver )
	ROM_REGION( 0x10000, REGION_CPU1, 0 ) /* 64k for code */
	ROM_LOAD( "33167-02.f1", 0x2800, 0x0800, CRC(25a5c976) SHA1(50fbf5dceab5d78292dc14bf25f2076e8139a594) )
	ROM_LOAD( "33164-02.e1", 0x3000, 0x0800, CRC(a348ac39) SHA1(7401cbd2f7236bd1d6ad0e39eb3de2b7d75e8f45) )
	ROM_LOAD( "33165-02.d1", 0x3800, 0x0800, CRC(a1fc5504) SHA1(febaa78936de7703b708c0d1f350fe288e0a106b) )
	ROM_LOAD( "33166-02.c1", 0x7800, 0x0800, CRC(3d26da2b) SHA1(e515d5c13814b9732a6ca109272500a60edc208a) )
	ROM_RELOAD(              0xf800, 0x0800 )

	ROM_REGION( 0x0400, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "33163-01.h5", 0x0000, 0x0400, CRC(5b9bb7c2) SHA1(319f45b6dff96739f73f2089361239da47042dcd) )

	ROM_REGION( 0x0800, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "33176-01.l5", 0x0000, 0x0400, CRC(6b082a01) SHA1(8facc94843ea041d205137056bd2035cf968125b) )
	ROM_LOAD( "33177-01.k5", 0x0400, 0x0400, CRC(f5541af0) SHA1(0967269518b6eac3c4e9ddaee39303086476c580) )
ROM_END



/*************************************
 *
 *	Game driver
 *
 *************************************/

GAME( 1978, skydiver, 0, skydiver, skydiver, 0, ROT0, "Atari", "Sky Diver" )
