/***************************************************************************
TRS80 memory map

0000-2fff ROM					  R   D0-D7
3000-37ff ROM on Model III		  R   D0-D7
		  unused on Model I
37e0-37e3 floppy motor			  W   D0-D3
		  or floppy head select   W   D3
37e4-37eb printer / RS232?? 	  R/W D0-D7
37ec-37ef FDC WD179x			  R/W D0-D7
37ec	  command				  W   D0-D7
37ec	  status				  R   D0-D7
37ed	  track 				  R/W D0-D7
37ee	  sector				  R/W D0-D7
37ef	  data					  R/W D0-D7
3800-38ff keyboard matrix		  R   D0-D7
3900-3bff unused - kbd mirrored
3c00-3fff video RAM 			  R/W D0-D5,D7 (or D0-D7)
4000-ffff RAM

Interrupts:
IRQ mode 1
NMI
***************************************************************************/
#include "includes/trs80.h"
#include "devices/basicdsk.h"

#define FW	TRS80_FONT_W
#define FH	TRS80_FONT_H

static ADDRESS_MAP_START( readmem_level1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff)	AM_READ(MRA8_ROM)
	AM_RANGE(0x3800, 0x38ff)	AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff)	AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0x7fff)	AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_level1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff)	AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3c00, 0x3fff)	AM_WRITE(trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0x7fff)	AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_level1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xfe, 0xfe)	AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff)	AM_READ(trs80_port_ff_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_level1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xff, 0xff)	AM_WRITE(trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_model1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff)	AM_READ(MRA8_ROM)
	AM_RANGE(0x3000, 0x37df)	AM_READ(MRA8_NOP)
	AM_RANGE(0x37e0, 0x37e3)	AM_READ(trs80_irq_status_r)
	AM_RANGE(0x30e4, 0x37e7)	AM_READ(MRA8_NOP)
	AM_RANGE(0x37e8, 0x37eb)	AM_READ(trs80_printer_r)
	AM_RANGE(0x37ec, 0x37ec)	AM_READ(wd179x_status_r)
	AM_RANGE(0x37ed, 0x37ed)	AM_READ(wd179x_track_r)
	AM_RANGE(0x37ee, 0x37ee)	AM_READ(wd179x_sector_r)
	AM_RANGE(0x37ef, 0x37ef)	AM_READ(wd179x_data_r)
	AM_RANGE(0x37f0, 0x37ff)	AM_READ(MRA8_NOP)
	AM_RANGE(0x3800, 0x38ff)	AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3900, 0x3bff)	AM_READ(MRA8_NOP)
	AM_RANGE(0x3c00, 0x3fff)	AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0xffff)	AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_model1, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff)	AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3000, 0x37df)	AM_WRITE(MWA8_NOP)
	AM_RANGE(0x37e0, 0x37e3)	AM_WRITE(trs80_motor_w)
	AM_RANGE(0x37e4, 0x37e7)	AM_WRITE(MWA8_NOP)
	AM_RANGE(0x37e8, 0x37eb)	AM_WRITE(trs80_printer_w)
	AM_RANGE(0x37ec, 0x37ec)	AM_WRITE(wd179x_command_w)
	AM_RANGE(0x37ed, 0x37ed)	AM_WRITE(wd179x_track_w)
	AM_RANGE(0x37ee, 0x37ee)	AM_WRITE(wd179x_sector_w)
	AM_RANGE(0x37ef, 0x37ef)	AM_WRITE(wd179x_data_w)
	AM_RANGE(0x37f0, 0x37ff)	AM_WRITE(MWA8_NOP)
	AM_RANGE(0x3800, 0x3bff)	AM_WRITE(MWA8_NOP)
	AM_RANGE(0x3c00, 0x3fff)	AM_WRITE(trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff)	AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( readport_model1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xfe, 0xfe)	AM_READ(trs80_port_xx_r)
	AM_RANGE(0xff, 0xff)	AM_READ(trs80_port_ff_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_model1, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xff, 0xff)	AM_WRITE(trs80_port_ff_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem_model3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff)	AM_READ(MRA8_ROM)
	AM_RANGE(0x3800, 0x38ff)	AM_READ(trs80_keyboard_r)
	AM_RANGE(0x3c00, 0x3fff)	AM_READ(MRA8_RAM)
	AM_RANGE(0x4000, 0xffff)	AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem_model3, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x37ff)	AM_WRITE(MWA8_ROM)
	AM_RANGE(0x3800, 0x38ff)	AM_WRITE(MWA8_NOP)
	AM_RANGE(0x3c00, 0x3fff)	AM_WRITE(trs80_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0x4000, 0xffff)	AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readport_model3, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xe0, 0xe3)	AM_READ(trs80_irq_status_r)
	AM_RANGE(0xf0, 0xf0)	AM_READ(wd179x_status_r)
	AM_RANGE(0xf1, 0xf1)	AM_READ(wd179x_track_r)
	AM_RANGE(0xf2, 0xf2)	AM_READ(wd179x_sector_r)
	AM_RANGE(0xf3, 0xf3)	AM_READ(wd179x_data_r)
	AM_RANGE(0xff, 0xff)	AM_READ(trs80_port_ff_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writeport_model3, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0xe0, 0xe3)	AM_WRITE(trs80_irq_mask_w)
	AM_RANGE(0xe4, 0xe4)	AM_WRITE(trs80_motor_w)
	AM_RANGE(0xf0, 0xf0)	AM_WRITE(wd179x_command_w)
	AM_RANGE(0xf1, 0xf1)	AM_WRITE(wd179x_track_w)
	AM_RANGE(0xf2, 0xf2)	AM_WRITE(wd179x_sector_w)
	AM_RANGE(0xf3, 0xf3)	AM_WRITE(wd179x_data_w)
	AM_RANGE(0xff, 0xff)	AM_WRITE(trs80_port_ff_w)
ADDRESS_MAP_END


/**************************************************************************
   w/o SHIFT							 with SHIFT
   +-------------------------------+	 +-------------------------------+
   | 0	 1	 2	 3	 4	 5	 6	 7 |	 | 0   1   2   3   4   5   6   7 |
+--+---+---+---+---+---+---+---+---+  +--+---+---+---+---+---+---+---+---+
|0 | @ | A | B | C | D | E | F | G |  |0 | ` | a | b | c | d | e | f | g |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|1 | H | I | J | K | L | M | N | O |  |1 | h | i | j | k | l | m | n | o |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|2 | P | Q | R | S | T | U | V | W |  |2 | p | q | r | s | t | u | v | w |
|  +---+---+---+---+---+---+---+---+  |  +---+---+---+---+---+---+---+---+
|3 | X | Y | Z | [ | \ | ] | ^ | _ |  |3 | x | y | z | { | | | } | ~ |	 |
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
***************************************************************************/

INPUT_PORTS_START( trs80 )
	PORT_START /* IN0 */
	PORT_DIPNAME(	  0x80, 0x80,	"Floppy Disc Drives")
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x80, DEF_STR( On ) )
	PORT_DIPNAME(	  0x40, 0x40,	"Video RAM") PORT_CODE(KEYCODE_F1)
	PORT_DIPSETTING(	0x40, "7 bit" )
	PORT_DIPSETTING(	0x00, "8 bit" )
	PORT_DIPNAME(	  0x20, 0x00,	"Virtual Tape") PORT_CODE(KEYCODE_F2)
	PORT_DIPSETTING(	0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x20, DEF_STR( On ) )
	PORT_BIT(	  0x08, 0x00, IPT_KEYBOARD) PORT_NAME("NMI") PORT_CODE(KEYCODE_F4)
	PORT_BIT(	  0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Tape start") PORT_CODE(KEYCODE_F5)
	PORT_BIT(	  0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Tape stop") PORT_CODE(KEYCODE_F6)
	PORT_BIT(	  0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Tape rewind") PORT_CODE(KEYCODE_F7)

	PORT_START /* KEY ROW 0 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0.0: @   ") PORT_CODE(KEYCODE_ASTERISK)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("0.1: A  a") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("0.2: B  b") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("0.3: C  c") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("0.4: D  d") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("0.5: E  e") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("0.6: F  f") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("0.7: G  g") PORT_CODE(KEYCODE_G)

	PORT_START /* KEY ROW 1 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("1.0: H  h") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1.1: I  i") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("1.2: J  j") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("1.3: K  k") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("1.4: L  l") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("1.5: M  m") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("1.6: N  n") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("1.7: O  o") PORT_CODE(KEYCODE_O)

	PORT_START /* KEY ROW 2 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("2.0: P  p") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("2.1: Q  q") PORT_CODE(KEYCODE_Q)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2.2: R  r") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("2.3: S  s") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("2.4: T  t") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("2.5: U  u") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("2.6: V  v") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("2.7: W  w") PORT_CODE(KEYCODE_W)

	PORT_START /* KEY ROW 3 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("3.0: X  x") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("3.1: Y  y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("3.2: Z  z") PORT_CODE(KEYCODE_Z)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3.3: [  {") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("3.4: \\  |") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("3.5: ]  }") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("3.6: ^  ~") PORT_CODE(KEYCODE_TILDE)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("3.7: _   ") PORT_CODE(KEYCODE_EQUALS)

	PORT_START /* KEY ROW 4 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("4.0: 0   ") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("4.1: 1  !") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("4.2: 2  \"") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("4.3: 3  #") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4.4: 4  $") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("4.5: 5  %") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("4.6: 6  &") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("4.7: 7  '") PORT_CODE(KEYCODE_7)

	PORT_START /* KEY ROW 5 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("5.0: 8  (") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("5.1: 9  )") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("5.2: :  *") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("5.3: ;  +") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("5.4: ,  <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5.5: -  =") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("5.6: .  >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("5.7: /  ?") PORT_CODE(KEYCODE_SLASH)

	PORT_START /* KEY ROW 6 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("6.0: ENTER") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("6.1: CLEAR") PORT_CODE(KEYCODE_HOME)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("6.2: BREAK") PORT_CODE(KEYCODE_END)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("6.3: UP") PORT_CODE(KEYCODE_UP)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("6.4: DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: LEFT") PORT_CODE(KEYCODE_LEFT)
	/* backspace should do the same as cursor left */
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("6.5: (BSP)") PORT_CODE(KEYCODE_BACKSPACE)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6.6: RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("6.7: SPACE") PORT_CODE(KEYCODE_SPACE)

	PORT_START /* KEY ROW 7 */
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("7.0: SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("7.1: (ALT)") PORT_CODE(KEYCODE_LALT)
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("7.2: (PGUP)") PORT_CODE(KEYCODE_PGUP)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("7.3: (PGDN)") PORT_CODE(KEYCODE_PGDN)
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("7.4: (INS)") PORT_CODE(KEYCODE_INSERT)
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("7.5: (DEL)") PORT_CODE(KEYCODE_DEL)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("7.6: (CTRL)") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7.7: (ALTGR)") PORT_CODE(KEYCODE_RALT)

INPUT_PORTS_END

static struct GfxLayout trs80_charlayout_normal_width =
{
	FW,FH,			/* 6 x 12 characters */
	256,			/* 256 characters */
	1,				/* 1 bits per pixel */
	{ 0 },			/* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static struct GfxLayout trs80_charlayout_double_width =
{
	FW*2,FH,	   /* FW*2 x FH*3 characters */
	256,		   /* 256 characters */
	1,			   /* 1 bits per pixel */
	{ 0 },		   /* no bitplanes; 1 bit per pixel */
	/* x offsets double width: use each bit twice */
	{ 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8,
	   6*8, 7*8, 8*8, 9*8,10*8,11*8 },
	8*FH		   /* every char takes FH bytes */
};

static struct GfxDecodeInfo trs80_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &trs80_charlayout_normal_width, 0, 4 },
	{ REGION_GFX1, 0, &trs80_charlayout_double_width, 0, 4 },
	{ -1 } /* end of array */
};

static unsigned char trs80_palette[] =
{
   0x00,0x00,0x00,
   0xff,0xff,0xff
};

static unsigned short trs80_colortable[] =
{
	0,1 	/* white on black */
};



/* Initialise the palette */
static PALETTE_INIT( trs80 )
{
	palette_set_colors(0, trs80_palette, sizeof(trs80_palette)/3);
	memcpy(colortable,trs80_colortable,sizeof(trs80_colortable));
}

static INT16 speaker_levels[3] = {0.0*32767,0.46*32767,0.85*32767};

static struct Speaker_interface speaker_interface =
{
	1,					/* one speaker */
	{ 100 },			/* mixing levels */
	{ 3 },				/* optional: number of different levels */
	{ speaker_levels }	/* optional: level lookup table */
};


static MACHINE_DRIVER_START( level1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, 1796000)        /* 1.796 Mhz */
	MDRV_CPU_PROGRAM_MAP(readmem_level1,writemem_level1)
	MDRV_CPU_IO_MAP(readport_level1,writeport_level1)
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 1)
	MDRV_CPU_PERIODIC_INT(trs80_timer_interrupt, 40)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( trs80 )
	MDRV_MACHINE_STOP( trs80 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*FW, 16*FH)
	MDRV_VISIBLE_AREA(0*FW,64*FW-1,0*FH,16*FH-1)
	MDRV_GFXDECODE( trs80_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(sizeof(trs80_palette)/sizeof(trs80_palette[0])/3)
	MDRV_COLORTABLE_LENGTH(sizeof(trs80_colortable)/sizeof(trs80_colortable[0]))
	MDRV_PALETTE_INIT( trs80 )

	MDRV_VIDEO_START( trs80 )
	MDRV_VIDEO_UPDATE( trs80 )

	/* sound hardware */
	MDRV_SOUND_ADD(SPEAKER, speaker_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model1 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( readmem_model1, writemem_model1 )
	MDRV_CPU_IO_MAP( readport_model1,writeport_model1 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( model3 )
	MDRV_IMPORT_FROM( level1 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_PROGRAM_MAP( readmem_model3, writemem_model3 )
	MDRV_CPU_IO_MAP( readport_model3,writeport_model3 )
	MDRV_CPU_VBLANK_INT(trs80_frame_interrupt, 2)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(trs80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("level1.rom",  0x0000, 0x1000, CRC(70d06dff))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(trs80l2)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80.z33",   0x0000, 0x1000, CRC(83dbbbe2))
	ROM_LOAD("trs80.z34",   0x1000, 0x1000, CRC(05818718))
	ROM_LOAD("trs80.zl2",   0x2000, 0x1000, CRC(306e5d66))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(trs80l2a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80alt.z33",0x0000, 0x1000, CRC(be46faf5))
	ROM_LOAD("trs80alt.z34",0x1000, 0x1000, CRC(6c791c2d))
	ROM_LOAD("trs80alt.zl2",0x2000, 0x1000, CRC(55b3ad13))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


ROM_START(sys80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("sys80rom.1",  0x0000, 0x1000, CRC(8f5214de))
	ROM_LOAD("sys80rom.2",  0x1000, 0x1000, CRC(46e88fbf))
	ROM_LOAD("trs80.zl2",  0x2000, 0x1000, CRC(306e5d66))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END

ROM_START(lnw80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("lnw_a.bin",  0x0000, 0x0800, CRC(e09f7e91))
	ROM_LOAD("lnw_a1.bin", 0x0800, 0x0800, CRC(ac297d99))
	ROM_LOAD("lnw_b.bin",  0x1000, 0x0800, CRC(c4303568))
	ROM_LOAD("lnw_b1.bin", 0x1800, 0x0800, CRC(3a5ea239))
	ROM_LOAD("lnw_c.bin",  0x2000, 0x0800, CRC(2ba025d7))
	ROM_LOAD("lnw_c1.bin", 0x2800, 0x0800, CRC(ed547445))

	ROM_REGION(0x01000, REGION_GFX1,0)
	ROM_LOAD("lnw_chr.bin",0x0800, 0x0800, CRC(c89b27df))
ROM_END

ROM_START(trs80m3)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("trs80m3.rom", 0x0000, 0x3800, CRC(bddbf843))

	ROM_REGION(0x00c00, REGION_GFX1,0)
	ROM_LOAD("trs80m1.chr", 0x0800, 0x0400, CRC(0033f2b9))
ROM_END


static void trs80_cassette_getinfo(struct IODevice *dev)
{
	/* cassette */
	dev->type = IO_CASSETTE;
	dev->count = 1;
	dev->file_extensions = "cas\0";
	dev->readable = 1;
	dev->writeable = 0;
	dev->creatable = 0;
	dev->load = device_load_trs80_cas;
	dev->unload = device_unload_trs80_cas;
}

static void trs80_quickload_getinfo(struct IODevice *dev)
{
	/* quickload */
	quickload_device_getinfo(dev, quickload_load_trs80_cmd, 0.5);
	dev->file_extensions = "cmd\0";
}

SYSTEM_CONFIG_START(trs80)
	CONFIG_DEVICE(trs80_quickload_getinfo)
	CONFIG_DEVICE(trs80_cassette_getinfo)
SYSTEM_CONFIG_END


static void trs8012_floppy_getinfo(struct IODevice *dev)
{
	/* floppy */
	legacybasicdsk_device_getinfo(dev);
	dev->count = 4;
	dev->file_extensions = "dsk\0";
	dev->load = device_load_trs80_floppy;
}

SYSTEM_CONFIG_START(trs8012)
	CONFIG_IMPORT_FROM(trs80)
	CONFIG_DEVICE(trs8012_floppy_getinfo)
SYSTEM_CONFIG_END


/*	   YEAR  NAME	   PARENT	 COMPAT	MACHINE   INPUT	 INIT	   CONFIG	COMPANY	 FULLNAME */
COMP ( 1977, trs80,    0,		 0,		level1,   trs80,	 trs80,    trs80,	"Tandy Radio Shack",  "TRS-80 Model I (Level I Basic)" )
COMP ( 1978, trs80l2,  trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (Radio Shack Level II Basic)" )
COMP ( 1978, trs80l2a, trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model I (R/S L2 Basic)" )
COMP ( 1980, sys80,    trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"EACA Computers Ltd.","System-80" )
COMPX( 1981, lnw80,    trs80,	 0,		model1,   trs80,	 trs80,    trs8012,	"LNW Research","LNW-80", GAME_NOT_WORKING )
COMPX( 1980, trs80m3,  trs80,	 0,		model3,   trs80,	 trs80,    trs8012,	"Tandy Radio Shack",  "TRS-80 Model III", GAME_NOT_WORKING )

