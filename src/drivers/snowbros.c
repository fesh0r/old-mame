/***************************************************************************

  Snow Brothers (Toaplan) / SemiCom Hardware
  uses Kaneko's Pandora sprite chip (also used in DJ Boy, Air Buster ..)

Snow Bros Nick & Tom
Toaplan, 1990

	PCB Layout
	----------
	MIN16-02

	|------------------------------------------|
	| VOL     YM3812  6116  4464  4464         |
	| LA4460  YM3014        4464  4464         |
	|       458   SBROS-4.29         SBROS1.40 |
	|   2003       Z80B     PANDORA            |
	|J                    D41101C-1 LS07  LS32 |
	|A SBROS-3A.5 SBROS-2A.6        LS139 LS174|
	|M                  LS245 LS74  LS04  16MHz|
	|M   6264     6264  F32   LS74  LS74       |
	|A      68000       LS20  F138  LS04  12MHz|
	|                   LS04  LS148 LS251 LS00 |
	| LS273 LS245 LS245 LS158 LS257 LS257 LS32 |
	|                                          |
	| LS273  6116  6116 LS157   DSW2  DSW1     |
	|------------------------------------------|

	Notes:
	       68k clock: 8.000MHz
	      Z80B clock: 6.000MHz
	    YM3812 clock: 3.000MHz
	           VSync: 57.5Hz
	           HSync: 15.68kHz

  driver by Mike Coates

  Hyper Pacman addition by David Haywood
   + some bits by Nicola Salmoria


Stephh's notes (hyperpac):

  - According to the "Language" Dip Switch, this game is a Korean game.
     (although the Language Dipswitch doesn't affect language, but yes
      I believe SemiCom to be a Korean Company)
  - There is no "cocktail mode", nor way to flip the screen.

todo:

make the originals work.
they're probably all this hardware or a varation on it, they don't work
(most point the interrupt vectors directly at a small area of ram which I'd
guess is shared with the Philips 87c52 mcu, more more plus doesn't point the
vectors there but does have a jump there in the code). See hyperpac for an
example, the protection data for that game was extracted from the bootleg.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"


WRITE16_HANDLER( snowbros_flipscreen_w );
VIDEO_UPDATE( snowbros );
VIDEO_UPDATE( wintbob );

static data16_t *hyperpac_ram;

static INTERRUPT_GEN( snowbros_interrupt )
{
	cpunum_set_input_line(0, cpu_getiloops() + 2, HOLD_LINE);	/* IRQs 4, 3, and 2 */
}


/* Sound Routines */

READ16_HANDLER( snowbros_68000_sound_r )
{
	int ret;

	/* If the sound CPU is running, read the YM3812 status, otherwise
	   just make it pass the test */
	if (Machine->sample_rate != 0)
	{
		ret = soundlatch_r(offset);
	}
	else
	{
		ret = 3;
	}

	return ret;
}


static WRITE16_HANDLER( snowbros_68000_sound_w )
{
	if (ACCESSING_LSB)
	{
		soundlatch_w(offset,data & 0xff);
		cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
	}
}

static WRITE16_HANDLER( semicom_soundcmd_w )
{
	if (ACCESSING_LSB) soundlatch_w(0,data & 0xff);
}


/* Snow Bros Memory Map */

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x300000, 0x300001) AM_READ(snowbros_68000_sound_r)
	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x500004, 0x500005) AM_READ(input_port_2_word_r)
	AM_RANGE(0x600000, 0x6001ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x700000, 0x701fff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x103fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x200000, 0x200001) AM_WRITE(watchdog_reset16_w)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(snowbros_68000_sound_w)
	AM_RANGE(0x400000, 0x400001) AM_WRITE(snowbros_flipscreen_w)
	AM_RANGE(0x600000, 0x6001ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x700000, 0x701fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x800000, 0x800001) AM_WRITE(MWA16_NOP)	/* IRQ 4 acknowledge? */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(MWA16_NOP)	/* IRQ 3 acknowledge? */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(MWA16_NOP)	/* IRQ 2 acknowledge? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0x8000, 0x87ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_READ(YM3812_status_port_0_r)
	AM_RANGE(0x04, 0x04) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x02, 0x02) AM_WRITE(YM3812_control_port_0_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(YM3812_write_port_0_w)
	AM_RANGE(0x04, 0x04) AM_WRITE(soundlatch_w)	/* goes back to the main CPU, checked during boot */
ADDRESS_MAP_END

/* SemiCom Memory Map

the SemiCom games have slightly more ram and are protected
sound hardware is also different

*/

static ADDRESS_MAP_START( hyperpac_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x500000, 0x500001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x500002, 0x500003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x500004, 0x500005) AM_READ(input_port_2_word_r)

	AM_RANGE(0x600000, 0x6001ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x700000, 0x701fff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hyperpac_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(MWA16_RAM) AM_BASE(&hyperpac_ram)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(semicom_soundcmd_w)
//	AM_RANGE(0x400000, 0x400001) ???
	AM_RANGE(0x600000, 0x6001ff) AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x700000, 0x701fff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)

	AM_RANGE(0x800000, 0x800001) AM_WRITE(MWA16_NOP)	/* IRQ 4 acknowledge? */
	AM_RANGE(0x900000, 0x900001) AM_WRITE(MWA16_NOP)	/* IRQ 3 acknowledge? */
	AM_RANGE(0xa00000, 0xa00001) AM_WRITE(MWA16_NOP)	/* IRQ 2 acknowledge? */
ADDRESS_MAP_END

static ADDRESS_MAP_START( hyperpac_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xcfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf001, 0xf001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xf008, 0xf008) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( hyperpac_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xcfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xf001, 0xf001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xf002, 0xf002) AM_WRITE(OKIM6295_data_0_w)
//	AM_RANGE(0xf006, 0xf006) ???
ADDRESS_MAP_END

INPUT_PORTS_START( snowbros )
	PORT_START	/* 500001 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Region ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Europe ) )
	PORT_DIPSETTING(    0x01, "America (Romstar license)" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x01)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )	PORT_DIPCONDITION(0,0x01,PORTCOND_EQUALS,0x00)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Must be low or game stops! */
													/* probably VBlank */

	PORT_START	/* 500003 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k and every 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( snowbroj )
	PORT_START	/* 500001 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* Must be low or game stops! */
													/* probably VBlank */

	PORT_START	/* 500003 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "100k and every 200k " )
	PORT_DIPSETTING(    0x0c, "100k Only" )
	PORT_DIPSETTING(    0x08, "200k Only" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPNAME( 0x40, 0x40, "Invulnerability" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500005 */
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( hyperpac )
	PORT_START	/* 500000.w */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Lives ) )	// "Language" in the "test mode"
	PORT_DIPSETTING(      0x0002, "3" )					// "Korean"
	PORT_DIPSETTING(      0x0000, "5" )					// "English"
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Hardest ) )			// DEF_STR( Very_Hard )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500002.w */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500004.w */
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cookbib2 )
	PORT_START	/* 500000.w */
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Lives ) )	// "Language" in the "test mode"
	PORT_DIPSETTING(      0x0002, "3" )					// "Korean"
	PORT_DIPSETTING(      0x0000, "5" )					// "English"
	PORT_DIPNAME( 0x001c, 0x001c, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0060, 0x0060, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Hardest ) )			// DEF_STR( Very_Hard )
	PORT_SERVICE( 0x0080, IP_ACTIVE_LOW )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(1)	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500002.w */
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)	// jump
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)	// fire
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_BUTTON3 ) PORT_PLAYER(2)	// test mode only?
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* 500004.w */
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

/* SnowBros */

static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP8(0,4), STEP8(8*32,4) },
	{ STEP8(0,32), STEP8(16*32,32) },
	32*32
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,  0, 16 },
	{ -1 } /* end of array */
};

/* Winter Bobble */

static struct GfxLayout tilelayout_wb =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ STEP4(3*4,-4), STEP4(7*4,-4), STEP4(11*4,-4), STEP4(15*4,-4) },
	{ STEP16(0,64) },
	16*64
};

static struct GfxDecodeInfo gfxdecodeinfo_wb[] =
{
	{ REGION_GFX1, 0, &tilelayout_wb,  0, 16 },
	{ -1 }
};

/* SemiCom */

static struct GfxLayout hyperpac_tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0, 1, 2, 3 },
	{ 4, 0, 8*32+4, 8*32+0, 20,16, 8*32+20, 8*32+16,
	  12, 8, 8*32+12, 8*32+8, 28, 24, 8*32+28, 8*32+24 },
	{ 0*32, 2*32, 1*32, 3*32, 16*32+0*32, 16*32+2*32, 16*32+1*32, 16*32+3*32,
	  4*32, 6*32, 5*32, 7*32, 16*32+4*32, 16*32+6*32, 16*32+5*32, 16*32+7*32 },
	32*32
};

static struct GfxDecodeInfo hyperpac_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &hyperpac_tilelayout,  0, 16 },
	{ -1 } /* end of array */
};

/* handler called by the 3812/2151 emulator when the internal timers cause an IRQ */
static void irqhandler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

/* SnowBros Sound */

static struct YM3812interface ym3812_interface =
{
	1,			/* 1 chip */
	3000000,	/* 3 MHz - confirmed */
	{ 100 },	/* volume */
	{ irqhandler },
};

/* SemiCom Sound */

static struct YM2151interface ym2151_interface =
{
	1,
	4000000,	/* 4 MHz??? */
	{ YM3012_VOL(10,MIXER_PAN_LEFT,10,MIXER_PAN_RIGHT) },
	{ irqhandler }
};

static struct OKIM6295interface okim6295_interface =
{
	1,			/* 1 chip */
	{ 7575 },		/* 7575Hz playback? */
	{ REGION_SOUND1 },
	{ 100 }
};


MACHINE_INIT (semiprot)
{
	data16_t *PROTDATA = (data16_t*)memory_region(REGION_USER1);
	int i;

	for (i = 0;i < 0x200/2;i++)
	hyperpac_ram[0xf000/2 + i] = PROTDATA[i];
}

static MACHINE_DRIVER_START( snowbros )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M68000, 8000000) /* 8 Mhz - confirmed */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(snowbros_interrupt,3)

	MDRV_CPU_ADD_TAG("sound", Z80, 6000000) /* 6 MHz - confirmed */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_FRAMES_PER_SECOND(57.5) /* ~57.5 - confirmed */
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(snowbros)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("3812", YM3812, ym3812_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( wintbob )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(snowbros)
	MDRV_CPU_REPLACE("main", M68000, 10000000) /* faster cpu on bootleg? otherwise the gfx break up */

	/* video hardware */
	MDRV_GFXDECODE(gfxdecodeinfo_wb)
	MDRV_VIDEO_UPDATE(wintbob)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( semicom )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(snowbros)
	MDRV_CPU_REPLACE("main", M68000, 16000000) /* 16mhz or 12mhz ? */
	MDRV_CPU_PROGRAM_MAP(hyperpac_readmem,hyperpac_writemem)

	MDRV_CPU_REPLACE("sound", Z80, 4000000) /* 4.0 MHz ??? */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(hyperpac_sound_readmem,hyperpac_sound_writemem)

	MDRV_GFXDECODE(hyperpac_gfxdecodeinfo)

	/* sound hardware */
	MDRV_SOUND_REPLACE("3812",YM2151, ym2151_interface)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( semiprot )
	MDRV_IMPORT_FROM(semicom)
	MDRV_MACHINE_INIT ( semiprot )
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( snowbros )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sn6.bin",  0x00000, 0x20000, CRC(4899ddcf) SHA1(47d750d3022a80e47ffabe47566bb2556cc8d477) )
	ROM_LOAD16_BYTE( "sn5.bin",  0x00001, 0x20000, CRC(ad310d3f) SHA1(f39295b38d99087dbb9c5b00bf9cb963337a50e2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "sbros-4.29",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sbros-1.41",   0x00000, 0x80000, CRC(16f06b3a) SHA1(c64d3b2d32f0f0fcf1d8c5f02f8589d59ddfd428) )
	/* where were these from, a bootleg? */
//	ROM_LOAD( "ch0",          0x00000, 0x20000, CRC(36d84dfe) SHA1(5d45a750220930bc409de30f19282bb143fbf94f) )
//	ROM_LOAD( "ch1",          0x20000, 0x20000, CRC(76347256) SHA1(48ec03965905adaba5e50eb3e42a2813f7883bb4) )
//	ROM_LOAD( "ch2",          0x40000, 0x20000, CRC(fdaa634c) SHA1(1271c74df7da7596caf67caae3c51b4c163a49f4) )
//	ROM_LOAD( "ch3",          0x60000, 0x20000, CRC(34024aef) SHA1(003a9b9ee3aaab3d787894d3d4126d372b19d2a8) )
ROM_END

ROM_START( snowbroa )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sbros-3a.5",  0x00000, 0x20000, CRC(10cb37e1) SHA1(786be4640f8df2c81a32decc189ea7657ace00c6) )
	ROM_LOAD16_BYTE( "sbros-2a.6",  0x00001, 0x20000, CRC(ab91cc1e) SHA1(8cff61539dc7d35fcbf110d3e54fc1883e7b8509) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "sbros-4.29",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sbros-1.41",   0x00000, 0x80000, CRC(16f06b3a) SHA1(c64d3b2d32f0f0fcf1d8c5f02f8589d59ddfd428) )
ROM_END

ROM_START( snowbrob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "sbros3-a",     0x00000, 0x20000, CRC(301627d6) SHA1(0d1dc70091c87e9c27916d4232ff31b7381a64e1) )
	ROM_LOAD16_BYTE( "sbros2-a",     0x00001, 0x20000, CRC(f6689f41) SHA1(e4fd27b930a31479c0d99e0ddd23d5db34044666) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "sbros-4.29",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sbros-1.41",   0x00000, 0x80000, CRC(16f06b3a) SHA1(c64d3b2d32f0f0fcf1d8c5f02f8589d59ddfd428) )
ROM_END

ROM_START( snowbroc )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "3-a.ic5",  0x00000, 0x20000, CRC(e1bc346b) SHA1(a20c343d9ed2ad4f785d21076499008edad251f9) )
	ROM_LOAD16_BYTE( "2-a.ic6",  0x00001, 0x20000, CRC(1be27f9d) SHA1(76dd14480b9274831e51016f7bb57459d7b15cf9) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "sbros-4.29",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "sbros-1.41",   0x00000, 0x80000, CRC(16f06b3a) SHA1(c64d3b2d32f0f0fcf1d8c5f02f8589d59ddfd428) )
ROM_END

ROM_START( snowbroj )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "snowbros.3",   0x00000, 0x20000, CRC(3f504f9e) SHA1(700758b114c3fde6ea8f84222af0850dba13cd3b) )
	ROM_LOAD16_BYTE( "snowbros.2",   0x00001, 0x20000, CRC(854b02bc) SHA1(4ad1548eef94dcb95119cb4a7dcdefa037591b5b) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "sbros-4.29",   0x0000, 0x8000, CRC(e6eab4e4) SHA1(d08187d03b21192e188784cb840a37a7bdb5ad32) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* The gfx ROM (snowbros.1) was bad, I'm using the ones from the other sets. */
	ROM_LOAD( "sbros-1.41",   0x00000, 0x80000, CRC(16f06b3a) SHA1(c64d3b2d32f0f0fcf1d8c5f02f8589d59ddfd428) )
ROM_END

ROM_START( wintbob )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )	/* 6*64k for 68000 code */
	ROM_LOAD16_BYTE( "wb03.bin", 0x00000, 0x10000, CRC(df56e168) SHA1(20dbabdd97e6f3d4bf6500bf9e8476942cb48ae3) )
	ROM_LOAD16_BYTE( "wb01.bin", 0x00001, 0x10000, CRC(05722f17) SHA1(9356e2488ea35e0a2978689f2ca6dfa0d57fd2ed) )
	ROM_LOAD16_BYTE( "wb04.bin", 0x20000, 0x10000, CRC(53be758d) SHA1(56cf85ba23fe699031d73e8f367a1b8ac837d5f8) )
	ROM_LOAD16_BYTE( "wb02.bin", 0x20001, 0x10000, CRC(fc8e292e) SHA1(857cfeb0be121e64e6117120514ae1f2ffeae4d6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for z80 sound code */
	ROM_LOAD( "wb05.bin",     0x0000, 0x10000, CRC(53fe59df) SHA1(a99053e82b9fed76f744fa9f67078294641c6317) )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )
	/* probably identical data to Snow Bros, in a different format */
	ROM_LOAD16_BYTE( "wb13.bin",     0x00000, 0x10000, CRC(426921de) SHA1(5107c58e7e08d71895baa67fe260b17ebd61389c) )
	ROM_LOAD16_BYTE( "wb06.bin",     0x00001, 0x10000, CRC(68204937) SHA1(fd2ef93df5fd8aa2d36072858dbcfce41157ef3e) )
	ROM_LOAD16_BYTE( "wb12.bin",     0x20000, 0x10000, CRC(ef4e04c7) SHA1(17158b61b3c158e0491db9abb2e1a8c20d981d37) )
	ROM_LOAD16_BYTE( "wb07.bin",     0x20001, 0x10000, CRC(53f40978) SHA1(058bbf3b7877f0cd320383e0386c5959e0d6589b) )
	ROM_LOAD16_BYTE( "wb11.bin",     0x40000, 0x10000, CRC(41cb4563) SHA1(94f1d12d299ac08fc8522139e1927f0cf739be75) )
	ROM_LOAD16_BYTE( "wb08.bin",     0x40001, 0x10000, CRC(9497b88c) SHA1(367c6106276f3816528341f11f3a97ae458d25cd) )
	ROM_LOAD16_BYTE( "wb10.bin",     0x60000, 0x10000, CRC(5fa22b1e) SHA1(1164003d873e9738a3ca133cce689c7120061e3c) )
	ROM_LOAD16_BYTE( "wb09.bin",     0x60001, 0x10000, CRC(9be718ca) SHA1(5c195e4f13efbdb229201d2408d018861bf389cc) )
ROM_END

/* SemiCom Games */

ROM_START( hyperpac )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hyperpac.h12", 0x00001, 0x20000, CRC(2cf0531a) SHA1(c4321d728845035507352d0bcf4348d28b92e85e) )
	ROM_LOAD16_BYTE( "hyperpac.i12", 0x00000, 0x20000, CRC(9c7d85b8) SHA1(432d5fbe8bef875ce4a9aeb74a7b57dc79c709fd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , CRC(03faf88e) SHA1(a8da883d4b765b809452bbffca37ff224edbe86d) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, CRC(fb9f468d) SHA1(52857b1a04c64ac853340ebb8e92d98eabea8bc1) )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, CRC(bd8673da) SHA1(8466355894da4d2c9a54d03a833cc9b4ec0c67eb) )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, CRC(5d90cd82) SHA1(56be68478a81bb4e1011990da83334929a0ac886) )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, CRC(61d86e63) SHA1(974c634607993924fa098eff106b1b288bec1e26) )
ROM_END

ROM_START( hyperpcb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "hpacuh12.bin", 0x00001, 0x20000, CRC(633ab2c6) SHA1(534435fa602adebf651e1d42f7c96b01eb6634ef) )
	ROM_LOAD16_BYTE( "hpacui12.bin", 0x00000, 0x20000, CRC(23dc00d1) SHA1(8d4d00f450b94912adcbb24073f9b3b01eab0450) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "hyperpac.u1", 0x00000, 0x10000 , CRC(03faf88e) SHA1(a8da883d4b765b809452bbffca37ff224edbe86d) ) // was missing from this set, using the one from the original

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "hyperpac.j15", 0x00000, 0x40000, CRC(fb9f468d) SHA1(52857b1a04c64ac853340ebb8e92d98eabea8bc1) )

	ROM_REGION( 0x0c0000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "hyperpac.a4", 0x000000, 0x40000, CRC(bd8673da) SHA1(8466355894da4d2c9a54d03a833cc9b4ec0c67eb) )
	ROM_LOAD( "hyperpac.a5", 0x040000, 0x40000, CRC(5d90cd82) SHA1(56be68478a81bb4e1011990da83334929a0ac886) )
	ROM_LOAD( "hyperpac.a6", 0x080000, 0x40000, CRC(61d86e63) SHA1(974c634607993924fa098eff106b1b288bec1e26) )
ROM_END

ROM_START( moremorp )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "mmp_u52.bin",  0x00001, 0x40000, CRC(66baf9b2) SHA1(f1d383a94ef4313cb02c59ace17b9562eddcfb3c) )
	ROM_LOAD16_BYTE( "mmp_u74.bin",  0x00000, 0x40000, CRC(7c6fede5) SHA1(41bc539a6efe9eb2304243701857b972d2170bcf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "mmp_u35.bin", 0x00000, 0x10000 , CRC(4d098cad) SHA1(a79d417e7525a25dd6697da9f3d1de269e759d2e) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP ) /* can't be dumped */

	ROM_REGION( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
	   shared ram, the MCU puts it there */
	ROM_LOAD16_WORD_SWAP( "protdata.bin", 0x00000, 0x200 , CRC(782dd2aa) SHA1(2587734271e0c85cb76bcdee171366c4e6fc9f81) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "mmp_u14.bin", 0x00000, 0x40000, CRC(211a2566) SHA1(48138547822a8e76c101dd4189d581f80eee1e24) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "mmp_u75.bin", 0x000000, 0x80000, CRC(af9e824e) SHA1(2b68813bf025a34b8958033108e4f8d39fd618cb) )
	ROM_LOAD( "mmp_u76.bin", 0x080000, 0x80000, CRC(c42af064) SHA1(f9d755e7cb52828d8594f7871932daf11443689f) )
	ROM_LOAD( "mmp_u77.bin", 0x100000, 0x80000, CRC(1d7396e1) SHA1(bde7e925051408dd2371b5da8235a6a4cae8cf6a) )
	ROM_LOAD( "mmp_u78.bin", 0x180000, 0x80000, CRC(5508d80b) SHA1(1b9a70a502b237fa11d1d55dce761e2def18873a) )
ROM_END

ROM_START( 3in1semi )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "u52",  0x00001, 0x40000, CRC(b0e4a0f7) SHA1(e1f8b8ef020a85fcd7817814cf6c5d560e9e608d) )
	ROM_LOAD16_BYTE( "u74",  0x00000, 0x40000, CRC(266862c4) SHA1(2c5c513fee99bdb6e0ae3e0e644e516bdaddd629) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "u35", 0x00000, 0x10000 , CRC(e40481da) SHA1(1c1fabcb67693235eaa6ff59ae12a35854b5564a) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "u14", 0x00000, 0x40000, CRC(c83c11be) SHA1(c05d96d61e5b8245232c85cbbcb7cc1e4e066492) )

	ROM_REGION( 0x200000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "u75", 0x000000, 0x80000, CRC(b66a0db6) SHA1(a4e604eb3c0a5b16b4b0bb99219045bf2146287c) )
	ROM_LOAD( "u76", 0x080000, 0x80000, CRC(5f4b48ea) SHA1(e9dd1100d55b021b060990988c1e5271ce1ae35b) )
	ROM_LOAD( "u77", 0x100000, 0x80000, CRC(d44211e3) SHA1(53af19dec03e76912632450414cdbcbb31cc094c) )
	ROM_LOAD( "u78", 0x180000, 0x80000, CRC(af596afc) SHA1(875d7a51ff5c741cae4483d8da33df9cae8de52a) )
ROM_END

ROM_START( cookbib2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "cookbib2.02",  0x00001, 0x40000, CRC(b2909460) SHA1(2438638af870cfc105631d2b5e5a27a64ab5394d) )
	ROM_LOAD16_BYTE( "cookbib2.01",  0x00000, 0x40000, CRC(65aafde2) SHA1(01f9f261527c35182f0445d641d987aa86ad750f) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "cookbib2.07", 0x00000, 0x10000 , CRC(f59f1c9a) SHA1(2830261fd55249e015514fcb4cf8392e83b7fd0d) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* Intel 87C52 MCU Code */
	ROM_LOAD( "87c52.mcu", 0x00000, 0x10000 , NO_DUMP ) /* can't be dumped */

	ROM_REGION( 0x200, REGION_USER1, 0 ) /* Data from Shared RAM */
	/* this is not a real rom but instead the data extracted from
	   shared ram, the MCU puts it there */
	ROM_LOAD16_WORD_SWAP( "protdata.bin", 0x00000, 0x200 , CRC(ae6d8ed5) SHA1(410cdacb9b90ea345c0e4be85e60a138f45a51f1) )

	ROM_REGION( 0x020000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "cookbib2.06", 0x00000, 0x20000, CRC(5e6f76b8) SHA1(725800143dfeaa6093ed5fcc5b9f15678ae9e547) )

	ROM_REGION( 0x140000, REGION_GFX1, 0 ) /* Sprites */
	ROM_LOAD( "cookbib2.05", 0x000000, 0x80000, CRC(89fb38ce) SHA1(1b39dd9c2743916b8d8af590bd92fe4819c2454b) )
	ROM_LOAD( "cookbib2.04", 0x080000, 0x80000, CRC(f240111f) SHA1(b2c3b6e3d916fc68e1fd258b1279b6c39e1f0108) )
	ROM_LOAD( "cookbib2.03", 0x100000, 0x40000, CRC(e1604821) SHA1(bede6bdd8331128b9f2b229d718133470bf407c9) )
ROM_END

READ16_HANDLER ( moremorp_0a_read )
{
	return 0x000a;
}

static DRIVER_INIT( moremorp )
{
//	data16_t *PROTDATA = (data16_t*)memory_region(REGION_USER1);
//	int i;

//	for (i = 0;i < 0x200/2;i++)
//		hyperpac_ram[0xf000/2 + i] = PROTDATA[i];

	/* explicit check in the code */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000, 0x200001, 0, 0, moremorp_0a_read );
}


static DRIVER_INIT( cookbib2 )
{
//	data16_t *HCROM = (data16_t*)memory_region(REGION_CPU1);
//	data16_t *PROTDATA = (data16_t*)memory_region(REGION_USER1);
//	int i;
//	hyperpac_ram[0xf000/2] = 0x46fc;
//	hyperpac_ram[0xf002/2] = 0x2700;

// verified on real hardware, need to move this to a file really

//	static data16_t cookbib2_mcu68k[] =
//	{
//		// moved to protdata.bin
//	};




//for (i = 0;i < sizeof(cookbib2_mcu68k)/sizeof(cookbib2_mcu68k[0]);i++)
//		hyperpac_ram[0xf000/2 + i] = cookbib2_mcu68k[i];

//	for (i = 0;i < 0x200/2;i++)
//		hyperpac_ram[0xf000/2 + i] = PROTDATA[i];


	// trojan is actually buggy and gfx flicker like crazy
	// but we can pause the system after bootup with HALT line of 68k to get the table before
	// it goes nuts

	//	hyperpac_ram[0xf07a/2] = 0x4e73;
	//	hyperpac_ram[0xf000/2] = 0x4e73;

#if 0

	/* interrupt wait loop? */
	HCROM[0x014942/2] = 0x4eb9;
	HCROM[0x014944/2] = 0x0004;
	HCROM[0x014946/2] = 0x8000;
	HCROM[0x014948/2] = 0x4e71;

	/* interrupt wait loop? */
	HCROM[0x014968/2] = 0x4eb9;
	HCROM[0x01496a/2] = 0x0004;
	HCROM[0x01496c/2] = 0x8100;
	HCROM[0x01496e/2] = 0x4e71;

	/* interrupt wait loop? */
	HCROM[0x014560/2] = 0x4eb9;
	HCROM[0x014562/2] = 0x0004;
	HCROM[0x014564/2] = 0x8200;
	HCROM[0x014566/2] = 0x4e71;

	/* new code for interrupt wait */
	HCROM[0x048000/2] = 0x4a79;
	HCROM[0x048002/2] = 0x0010;
	HCROM[0x048004/2] = 0x2462;
	HCROM[0x048006/2] = 0x66f8;
	HCROM[0x048008/2] = 0x4eb9;
	HCROM[0x04800a/2] = 0x0004;
	HCROM[0x04800c/2] = 0x8300;
	HCROM[0x04800e/2] = 0x4e75;

	/* new code for interrupt wait */
	HCROM[0x048100/2] = 0x4a79;
	HCROM[0x048102/2] = 0x0010;
	HCROM[0x048104/2] = 0x2460;
	HCROM[0x048106/2] = 0x66f8;
	HCROM[0x048108/2] = 0x4eb9;
	HCROM[0x04810a/2] = 0x0004;
	HCROM[0x04810c/2] = 0x8300;
	HCROM[0x04810e/2] = 0x4e75;

	/* new code for interrupt wait */
	HCROM[0x048200/2] = 0x4a79;
	HCROM[0x048202/2] = 0x0010;
	HCROM[0x048204/2] = 0x2490;
	HCROM[0x048206/2] = 0x66f8;
	HCROM[0x048208/2] = 0x4eb9;
	HCROM[0x04820a/2] = 0x0004;
	HCROM[0x04820c/2] = 0x8300;
	HCROM[0x04820e/2] = 0x4e75;



	/* put registers on stack */
	HCROM[0x048300/2] = 0x48e7;
	HCROM[0x048302/2] = 0xfffe;

	/* wipe sprite ram (fill with 0x0002) */

	/* put the address we want to write TO in A2 */
	HCROM[0x048304/2] = 0x45f9;
	HCROM[0x048306/2] = 0x0070;
	HCROM[0x048308/2] = 0x0000;

	/* put the number of words we want to clear into D0 */
	HCROM[0x04830a/2] = 0x203c;
	HCROM[0x04830c/2] = 0x0000;
	HCROM[0x04830e/2] = 0x1000;

	/* write 0x0002 to A2 */
	HCROM[0x048310/2] = 0x34bc;
	HCROM[0x048312/2] = 0x0002;


	/* add 1 to write address a2 */
	HCROM[0x048314/2] = 0xd5fc;
	HCROM[0x048316/2] = 0x0000;
	HCROM[0x048318/2] = 0x0002;

	/* decrease counter d0 */
	HCROM[0x04831a/2] = 0x5380;

	/* compare d0 to 0 */
	HCROM[0x04831c/2] = 0x0c80;
	HCROM[0x04831e/2] = 0x0000;
	HCROM[0x048320/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x048322/2] = 0x66ec;

	/* ram has been wiped */

	/* put the address we want to read protection data  in A2 */
	HCROM[0x048324/2] = 0x45f9;
	HCROM[0x048326/2] = 0x0010;
//	HCROM[0x048328/2] = 0xf000;
//	HCROM[0x048328/2] = 0xf000+0xb4;
	HCROM[0x048328/2] = 0xf000+0xb4+0xb4;

	/* put the address of spriteram  in A0 */
	HCROM[0x04832a/2] = 0x41f9;
	HCROM[0x04832c/2] = 0x0070;
	HCROM[0x04832e/2] = 0x0000;

	/* put the number of rows into D3 */
	HCROM[0x048330/2] = 0x263c;
	HCROM[0x048332/2] = 0x0000;
	HCROM[0x048334/2] = 0x0012;

	/* put the y co-ordinate of rows into D6 */
	HCROM[0x048336/2] = 0x2c3c;
	HCROM[0x048338/2] = 0x0000;
	HCROM[0x04833a/2] = 0x0014;

	/* put the number of bytes per row into D2 */
	HCROM[0x04833c/2] = 0x243c;
	HCROM[0x04833e/2] = 0x0000;
	HCROM[0x048340/2] = 0x000a;

	/* put the x co-ordinate of rows into D5 */
	HCROM[0x048342/2] = 0x2a3c;
	HCROM[0x048344/2] = 0x0000;
	HCROM[0x048346/2] = 0x0010;

	// move content of a2 to d4 (byte)
	HCROM[0x048348/2] = 0x1812;

	HCROM[0x04834a/2] = 0xe84c; // shift d4 right by 4

	HCROM[0x04834c/2] = 0x0244; // mask with 0x000f
	HCROM[0x04834e/2] = 0x000f; //

	/* jump to character draw to draw first bit */
	HCROM[0x048350/2] = 0x4eb9;
	HCROM[0x048352/2] = 0x0004;
	HCROM[0x048354/2] = 0x8600;

	// increase x-cord
	HCROM[0x048356/2] = 0x0645;
	HCROM[0x048358/2] = 0x000a;


	/* add 0x10 to draw address a0 */
	HCROM[0x04835a/2] = 0xd1fc;
	HCROM[0x04835c/2] = 0x0000;
	HCROM[0x04835e/2] = 0x0010;


	// move content of a2 to d4 (byte)
	HCROM[0x048360/2] = 0x1812;

	HCROM[0x048362/2] = 0x0244; // mask with 0x000f
	HCROM[0x048364/2] = 0x000f; //

	/* jump to character draw to draw second bit */
	HCROM[0x048366/2] = 0x4eb9;
	HCROM[0x048368/2] = 0x0004;
	HCROM[0x04836a/2] = 0x8600;

	// increase x-cord
	HCROM[0x04836c/2] = 0x0645;
	HCROM[0x04836e/2] = 0x000c;

	/* add 0x10 to draw address a0 */
	HCROM[0x048370/2] = 0xd1fc;
	HCROM[0x048372/2] = 0x0000;
	HCROM[0x048374/2] = 0x0010;

// newcode
	/* add 1 to read address a2 */
	HCROM[0x048376/2] = 0xd5fc;
	HCROM[0x048378/2] = 0x0000;
	HCROM[0x04837a/2] = 0x0001;

	/* decrease counter d2 (row count)*/
	HCROM[0x04837c/2] = 0x5382;

	/* compare d2 to 0 */
	HCROM[0x04837e/2] = 0x0c82;
	HCROM[0x048380/2] = 0x0000;
	HCROM[0x048382/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x048384/2] = 0x66c2;

	// increase y-cord d6
	HCROM[0x048386/2] = 0x0646;
	HCROM[0x048388/2] = 0x000c;

	/* decrease counter d3 */
	HCROM[0x04838a/2] = 0x5383;

	/* compare d3 to 0 */
	HCROM[0x04838c/2] = 0x0c83;
	HCROM[0x04838e/2] = 0x0000;
	HCROM[0x048390/2] = 0x0000;

	/* if its not 0 then branch back */
	HCROM[0x048392/2] = 0x66a8;

	/* get back registers from stack*/
	HCROM[0x048394/2] = 0x4cdf;
	HCROM[0x048396/2] = 0x7fff;

	/* rts */
	HCROM[0x048398/2] = 0x4e75;

	/* Draw a character! */
	/* D6 = y-coordinate
	   D5 = x-coordinate
	   D4 = value to draw

	   A0 = spriteram base */

	// 0002 0002 0002 0010 00xx 00yy 00nn 000n

	// 357c 0020 000c
	// 337c = a1
	// move.w #$20, (#$c, A2)

	HCROM[0x048600/2] = 0x317c;
	HCROM[0x048602/2] = 0x0010;
	HCROM[0x048604/2] = 0x0006;

	HCROM[0x048606/2] = 0x3145;
	HCROM[0x048608/2] = 0x0008;

	HCROM[0x04860a/2] = 0x3146;
	HCROM[0x04860c/2] = 0x000a;

/* get true value */

	/* put lookuptable address in  A3 */
	HCROM[0x04860e/2] = 0x47f9;
	HCROM[0x048610/2] = 0x0004;
	HCROM[0x048612/2] = 0x8800;

	HCROM[0x048614/2] = 0x3004; // d4 -> d0
	HCROM[0x048616/2] = 0xe348;

	HCROM[0x048618/2] = 0x3173;
	HCROM[0x04861a/2] = 0x0000;
	HCROM[0x04861c/2] = 0x000c;

/* not value */

	HCROM[0x04861e/2] = 0x317c;
	HCROM[0x048620/2] = 0x0000;
	HCROM[0x048622/2] = 0x000e;

	/* rts */
	HCROM[0x048624/2] = 0x4e75;


	/* table used for lookup by the draw routine to get real tile numbers */

	HCROM[0x048800/2] = 0x0010;
	HCROM[0x048802/2] = 0x0011;
	HCROM[0x048804/2] = 0x0012;
	HCROM[0x048806/2] = 0x0013;
	HCROM[0x048808/2] = 0x0014;
	HCROM[0x04880a/2] = 0x0015;
	HCROM[0x04880c/2] = 0x0016;
	HCROM[0x04880e/2] = 0x0017;
	HCROM[0x048810/2] = 0x0018;
	HCROM[0x048812/2] = 0x0019;
	HCROM[0x048814/2] = 0x0021;
	HCROM[0x048816/2] = 0x0022;
	HCROM[0x048818/2] = 0x0023;
	HCROM[0x04881a/2] = 0x0024;
	HCROM[0x04881c/2] = 0x0025;
	HCROM[0x04881e/2] = 0x0026;



/*
10 0
11 1
12 2
13 3
14 4
15 5
16 6
17 7
18 8
19 9
21 a
22 b
23 c
24 d
25 e
26 f
*/





	{
		FILE *fp;

		fp=fopen("cookie", "w+b");
		if (fp)
		{
			fwrite(HCROM, 0x80000, 1, fp);
			fclose(fp);
		}
	}
#endif
}


static DRIVER_INIT( hyperpac )
{
	/* simulate RAM initialization done by the protection MCU */
	/* not verified on real hardware */
	hyperpac_ram[0xe000/2] = 0x4ef9;
	hyperpac_ram[0xe002/2] = 0x0000;
	hyperpac_ram[0xe004/2] = 0x062c;

	hyperpac_ram[0xe080/2] = 0xfedc;
	hyperpac_ram[0xe082/2] = 0xba98;
	hyperpac_ram[0xe084/2] = 0x7654;
	hyperpac_ram[0xe086/2] = 0x3210;
}

GAME( 1990, snowbros, 0,        snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 1)" )
GAME( 1990, snowbroa, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 2)" )
GAME( 1990, snowbrob, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 3)" )
GAME( 1990, snowbroc, snowbros, snowbros, snowbros, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (set 4)" )
GAME( 1990, snowbroj, snowbros, snowbros, snowbroj, 0, ROT0, "Toaplan", "Snow Bros. - Nick & Tom (Japan)" )
GAME( 1990, wintbob,  snowbros, wintbob,  snowbros, 0, ROT0, "bootleg", "The Winter Bobble" )
/* SemiCom Games */
GAME( 1995, hyperpac, 0,        semicom, hyperpac, hyperpac, ROT0, "SemiCom", "Hyper Pacman" )
GAME( 1995, hyperpcb, hyperpac, semicom, hyperpac, 0,        ROT0, "bootleg", "Hyper Pacman (bootleg)" )
GAME( 1996, cookbib2, 0,        semiprot, cookbib2, cookbib2, ROT0, "SemiCom", "Cookie and Bibi 2" )
GAME( 1999, moremorp, 0,        semiprot, hyperpac, moremorp, ROT0, "SemiCom / Exit", "More More Plus" )

/* the following don't work, they either point the interrupts at an area of ram probably shared by
   some kind of mcu which puts 68k code there, or jump to the area in the interrupts */
GAMEX(1997, 3in1semi, 0,        semicom, hyperpac, 0,        ROT0, "SemiCom", "3-in-1 (SemiCom)", GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
