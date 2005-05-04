/***************************************************************************

                              -= Afega Games =-

                    driver by   Luca Elia (l.elia@tin.it)


Main  CPU   :   M68000
Video Chips :   AFEGA AFI-GFSK  (68 Pin PLCC)
                AFEGA AFI-GFLK (208 Pin PQFP)

Sound CPU   :   Z80
Sound Chips :   M6295 (AD-65)  +  YM2151 (BS901)  +  YM3014 (BS90?)

---------------------------------------------------------------------------
Year + Game                     Notes
---------------------------------------------------------------------------
97 Red Hawk                     US Version of Stagger 1
98 Sen Jin - Guardian Storm     Some text missing (protection, see service mode)
98 Stagger I
98 Bubble 2000                  By Tuning, but it seems to be the same HW
01 Fire Hawk                    By ESD with different sound hardware: 2 M6295
---------------------------------------------------------------------------

The Sen Jin protection supplies some 68k code seen in the 2760-29cf range

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"

/* Variables defined in vidhrdw: */

extern data16_t *afega_vram_0, *afega_scroll_0;
extern data16_t *afega_vram_1, *afega_scroll_1;

/* Functions defined in vidhrdw: */

WRITE16_HANDLER( afega_vram_0_w );
WRITE16_HANDLER( afega_vram_1_w );
WRITE16_HANDLER( afega_palette_w );

PALETTE_INIT( grdnstrm );

VIDEO_START( afega );
VIDEO_START( firehawk );
VIDEO_UPDATE( afega );
VIDEO_UPDATE( redhawkb );
VIDEO_UPDATE( bubl2000 );
VIDEO_UPDATE( firehawk );

/***************************************************************************


                            Memory Maps - Main CPU


***************************************************************************/

READ16_HANDLER( afega_unknown_r )
{
	/* This fixes the text in Service Mode. */
	return 0x0100;
}

WRITE16_HANDLER( afega_soundlatch_w )
{
	if (ACCESSING_LSB && Machine->sample_rate)
	{
		soundlatch_w(0,data&0xff);
		cpunum_set_input_line(1, 0, PULSE_LINE);
	}
}

static WRITE16_HANDLER( afega_scroll0_w )
{
	COMBINE_DATA(&afega_scroll_0[offset]);
}

static WRITE16_HANDLER( afega_scroll1_w )
{
	COMBINE_DATA(&afega_scroll_1[offset]);
}

/*
 Lines starting with an empty comment in the following MemoryReadAddress
 arrays are there for debug (e.g. the game does not read from those ranges
 AFAIK)
*/


static ADDRESS_MAP_START( afega, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(20) )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x080000, 0x080001) AM_READ(input_port_0_word_r)		// Buttons
	AM_RANGE(0x080002, 0x080003) AM_READ(input_port_1_word_r)		// P1 + P2
	AM_RANGE(0x080004, 0x080005) AM_READ(input_port_2_word_r)		// 2 x DSW
	AM_RANGE(0x080012, 0x080013) AM_READ(afega_unknown_r)
	AM_RANGE(0x080000, 0x08001d) AM_WRITE(MWA16_RAM)				//
	AM_RANGE(0x08001e, 0x08001f) AM_WRITE(afega_soundlatch_w)		// To Sound CPU
/**/AM_RANGE(0x084000, 0x084003) AM_RAM AM_WRITE(afega_scroll0_w)	// Scroll on redhawkb (mirror or changed?..)
/**/AM_RANGE(0x084004, 0x084007) AM_RAM AM_WRITE(afega_scroll1_w)   // Scroll on redhawkb (mirror or changed?..)
	AM_RANGE(0x080020, 0x087fff) AM_WRITE(MWA16_RAM)				//
/**/AM_RANGE(0x088000, 0x0885ff) AM_READWRITE(MRA16_RAM, afega_palette_w) AM_BASE(&paletteram16) // Palette
	AM_RANGE(0x088600, 0x08bfff) AM_WRITE(MWA16_RAM)				//
/**/AM_RANGE(0x08c000, 0x08c003) AM_RAM AM_WRITE(afega_scroll0_w) AM_BASE(&afega_scroll_0)	// Scroll
/**/AM_RANGE(0x08c004, 0x08c007) AM_RAM AM_WRITE(afega_scroll1_w) AM_BASE(&afega_scroll_1)	//
	AM_RANGE(0x08c008, 0x08ffff) AM_WRITE(MWA16_RAM)				//
/**/AM_RANGE(0x090000, 0x091fff) AM_READWRITE(MRA16_RAM, afega_vram_0_w) AM_BASE(&afega_vram_0)	// Layer 0
/**/AM_RANGE(0x092000, 0x093fff) AM_RAM								// ?
/**/AM_RANGE(0x09c000, 0x09c7ff) AM_READWRITE(MRA16_RAM, afega_vram_1_w) AM_BASE(&afega_vram_1)	// Layer 1
	AM_RANGE(0x0c0000, 0x0c7fff) AM_RAM								// RAM
	AM_RANGE(0x0c8000, 0x0c8fff) AM_RAM AM_SHARE(1) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)	// Sprites
	AM_RANGE(0x0c9000, 0x0cffff) AM_RAM								// RAM
	AM_RANGE(0x0f8000, 0x0f8fff) AM_RAM AM_SHARE(1)					// Sprites Mirror
ADDRESS_MAP_END


/***************************************************************************


                            Memory Maps - Sound CPU


***************************************************************************/

static ADDRESS_MAP_START( afega_sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM									// RAM
	AM_RANGE(0xf800, 0xf800) AM_READ(soundlatch_r)					// From Main CPU
	AM_RANGE(0xf808, 0xf808) AM_WRITE(YM2151_register_port_0_w)		// YM2151
	AM_RANGE(0xf809, 0xf809) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)	// YM2151
	AM_RANGE(0xf80a, 0xf80a) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)		// M6295
ADDRESS_MAP_END

static ADDRESS_MAP_START( firehawk_sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xefff) AM_ROM
	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xfff0, 0xfff0) AM_READ(soundlatch_r)
	AM_RANGE(0xfff8, 0xfff8) AM_READWRITE(OKIM6295_status_1_r, OKIM6295_data_1_w)
	AM_RANGE(0xfffa, 0xfffa) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xf800, 0xffff) AM_RAM // not used, only tested
ADDRESS_MAP_END

/***************************************************************************


                                Input Ports


***************************************************************************/

/***************************************************************************
                                Stagger I
***************************************************************************/

INPUT_PORTS_START( stagger1 )
	PORT_START_TAG("IN0")	// $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")	// $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $080004.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END

/* everything seems active high.. not low */
INPUT_PORTS_START( redhawkb )
	PORT_START_TAG("IN0")	// $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")	// $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $080004.w  -- probably just redhawk but inverted
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


/***************************************************************************
                            Sen Jin - Guardian Storm
***************************************************************************/

INPUT_PORTS_START( grdnstrm )
	PORT_START_TAG("IN0")	// $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")	// IN1 - $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $080004.w
	PORT_SERVICE( 0x0001, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Bombs" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "5" )
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0300, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPSETTING(      0x0200, "Horizontally" )
	PORT_DIPSETTING(      0x0100, "Vertically" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************
                                Bubble 2000
***************************************************************************/

INPUT_PORTS_START( bubl2000 )
	PORT_START_TAG("IN0")	// $080000.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_COIN1    )
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_COIN2    )
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_START1   )
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_START2   )
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT(  0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")	// $080002.w
	PORT_BIT(  0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT(  0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT(  0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT(  0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT(  0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT(  0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT(  0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT(  0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT(  0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT(  0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT(  0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT(  0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT(  0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")	// $080004.w
	PORT_DIPNAME( 0x0001, 0x0001, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000c, 0x000c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00c0, 0x00c0, "Free Credit" )
	PORT_DIPSETTING(      0x0080, "500k" )
	PORT_DIPSETTING(      0x00c0, "800k" )
	PORT_DIPSETTING(      0x0040, "1000k" )
	PORT_DIPSETTING(      0x0000, "1500k" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unused ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c00, 0x1c00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x1800, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x1c00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0c00, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x1400, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( 1C_4C ) )
//  PORT_DIPSETTING(      0x0000, "Disabled" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 1C_4C ) )
//  PORT_DIPSETTING(      0x0000, "Disabled" )
INPUT_PORTS_END


/***************************************************************************
                                Fire Hawk
***************************************************************************/

INPUT_PORTS_START( firehawk )
	PORT_START_TAG("IN0")	// $080000.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN1")	// $080002.w
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN  )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN  )

	PORT_START_TAG("IN2") 	// $080004.w
	PORT_DIPNAME( 0x0001, 0x0001, "Show Dip-Switch Settings" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x000e, 0x000e, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( Very_Easy) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Easy ) )
//  PORT_DIPSETTING(      0x000a, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x000e, DEF_STR( Normal ) )
//  PORT_DIPSETTING(      0x0000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Hardest ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Number of Bombs" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0000, "3" )
	PORT_DIPNAME( 0x00c0, 0x00c0, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0080, "2" )
	PORT_DIPSETTING(      0x00c0, "3" )
	PORT_DIPSETTING(      0x0040, "4" )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Region ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( English ) )
	PORT_DIPSETTING(      0x0000, "China" )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1800, 0x1800, "Continue Coins" )
	PORT_DIPSETTING(      0x1800, "1 Coin" )
	PORT_DIPSETTING(      0x0800, "2 Coins" )
	PORT_DIPSETTING(      0x1000, "3 Coins" )
	PORT_DIPSETTING(      0x0000, "4 Coins" )
	PORT_DIPNAME( 0xe000, 0xe000, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0xc000, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(      0xe000, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(      0x6000, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0xa000, DEF_STR( 1C_3C ) )
INPUT_PORTS_END


/***************************************************************************


                            Graphics Layouts


***************************************************************************/

static struct GfxLayout layout_8x8x4 =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4)	},
	{ STEP8(0,8*4)	},
	8*8*4
};

static struct GfxLayout layout_16x16x4 =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxLayout layout_16x16x8 =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ STEP4(RGN_FRAC(0,2),1), STEP4(RGN_FRAC(1,2),1)	},
	{ STEP8(0,4),   STEP8(8*8*4*2,4)	},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};


static struct GfxLayout layout_16x16x4_swapped =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ STEP4(0,1)	},
	{ 4,0,12,8,20,16,28,24, 512+4,512+0,512+12,512+8,512+20,512+16,512+28,512+24},
	{ STEP8(0,8*4), STEP8(8*8*4*1,8*4)	},
	16*16*4
};

static struct GfxDecodeInfo grdnstrm_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 256*1, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x8, 256*3, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,   256*2, 16 }, // [2] Layer 1
	{ -1 }
};

static struct GfxDecodeInfo stagger1_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4, 256*1, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4,   256*0, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,     256*2, 16 }, // [2] Layer 1
	{ -1 }
};

static struct GfxDecodeInfo redhawkb_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &layout_16x16x4_swapped,   256*1, 16 }, // [0] Sprites
	{ REGION_GFX2, 0, &layout_16x16x4_swapped,   256*0, 16 }, // [1] Layer 0
	{ REGION_GFX3, 0, &layout_8x8x4,     256*2, 16 }, // [2] Layer 1
	{ -1 }
};


/***************************************************************************


                                Machine Drivers


***************************************************************************/

static void irq_handler(int irq)
{
	cpunum_set_input_line(1,0,irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface afega_ym2151_intf =
{
	irq_handler
};

INTERRUPT_GEN( interrupt_afega )
{
	switch ( cpu_getiloops() )
	{
		case 0:		irq2_line_hold();	break;
		case 1:		irq4_line_hold();	break;
	}
}

static MACHINE_DRIVER_START( stagger1 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)
	MDRV_CPU_PROGRAM_MAP(afega,0)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(afega_sound_cpu,0)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0+16, 256-16-1)
	MDRV_GFXDECODE(stagger1_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(afega)
	MDRV_VIDEO_UPDATE(afega)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YM2151, 4000000)
	MDRV_SOUND_CONFIG(afega_ym2151_intf)
	MDRV_SOUND_ROUTE(0, "left", 0.30)
	MDRV_SOUND_ROUTE(1, "right", 0.30)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.70)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.70)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( redhawkb )
	/* basic machine hardware */
	MDRV_IMPORT_FROM(stagger1)
	/* video hardware */
	MDRV_GFXDECODE(redhawkb_gfxdecodeinfo)
	MDRV_VIDEO_UPDATE(redhawkb)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( grdnstrm )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(stagger1)

	/* video hardware */
	MDRV_GFXDECODE(grdnstrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)
	MDRV_COLORTABLE_LENGTH(768 + 16*256)

	MDRV_PALETTE_INIT(grdnstrm)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( bubl2000 )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(grdnstrm)

	/* video hardware */
	MDRV_VIDEO_UPDATE(bubl2000)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( firehawk )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)
	MDRV_CPU_PROGRAM_MAP(afega,0)
	MDRV_CPU_VBLANK_INT(interrupt_afega,2)

	MDRV_CPU_ADD(Z80,4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(firehawk_sound_cpu,0)

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(8, 256-8-1, 16, 256-16-1)
	MDRV_GFXDECODE(grdnstrm_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)
	MDRV_COLORTABLE_LENGTH(768 + 16*256)

	MDRV_PALETTE_INIT(grdnstrm)
	MDRV_VIDEO_START(firehawk)
	MDRV_VIDEO_UPDATE(firehawk)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

/***************************************************************************


                                ROMs Loading


***************************************************************************/

/* Address lines scrambling */

static void decryptcode( int a23, int a22, int a21, int a20, int a19, int a18, int a17, int a16, int a15, int a14, int a13, int a12,
	int a11, int a10, int a9, int a8, int a7, int a6, int a5, int a4, int a3, int a2, int a1, int a0 )
{
	int i;
	data8_t *RAM = memory_region( REGION_CPU1 );
	size_t  size = memory_region_length( REGION_CPU1 );
	data8_t *buffer = malloc( size );

	if( buffer )
	{
		memcpy( buffer, RAM, size );
		for( i = 0; i < size; i++ )
		{
			RAM[ i ] = buffer[ BITSWAP24( i, a23, a22, a21, a20, a19, a18, a17, a16, a15, a14, a13, a12,
				a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1, a0 ) ];
		}
		free( buffer );
	}
}

/***************************************************************************

                                    Stagger I
(AFEGA 1998)

Parts:

1 MC68HC000P10
1 Z80
2 Lattice ispLSI 1032E

***************************************************************************/

ROM_START( stagger1 )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2.bin", 0x000000, 0x020000, CRC(8555929b) SHA1(b405d81c2a45191111b1a4458ac6b5c0a129b8f1) )
	ROM_LOAD16_BYTE( "3.bin", 0x000001, 0x020000, CRC(5b0b63ac) SHA1(239f793b6845a88d1630da790a2762da730a450d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "7.bin", 0x00000, 0x80000, CRC(048f7683) SHA1(7235b7dcfbb72abf44e60b114e3f504f16d29ebf) )
	ROM_LOAD16_BYTE( "6.bin", 0x00001, 0x80000, CRC(051d4a77) SHA1(664182748e72b3e44202caa20f337d02e946ca62) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x4 */
	ROM_LOAD( "4.bin", 0x00000, 0x80000, CRC(46463d36) SHA1(4265bc4d24ff64e39d9273965701c740d7e3fee0) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	// Unused

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END


/***************************************************************************

                            Red Hawk (c)1997 Afega



    6116         ym2145(?)  MSM6295   5    4MHz
    1
    Z80        pLSI1032    4
                                      76C88
       6116   76C256                  76C88
       6116   76C256
    2  76C256 76C256
    3  76C256 76C256

    68000-10        6116
                    6116
   SW1                             6
   SW21                pLSI1032    7
    12MHz

***************************************************************************/

ROM_START( redhawk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "2", 0x000000, 0x020000, CRC(3ef5f326) SHA1(e89c7c24a05886a14995d7c399958dc00ad35d63) )
	ROM_LOAD16_BYTE( "3", 0x000001, 0x020000, CRC(9b3a10ef) SHA1(d03480329b23474e5a9e42a75b09d2140eed4443) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "6", 0x000001, 0x080000, CRC(61560164) SHA1(d727ab2d037dab40745dec9c4389744534fdf07d) )
	ROM_LOAD16_BYTE( "7", 0x000000, 0x080000, CRC(66a8976d) SHA1(dd9b89cf29eb5557845599d55ef3a15f53c070a4) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "4", 0x000000, 0x080000, CRC(d6427b8a) SHA1(556de1b5ce29d1c3c54bb315dcaa4dd0848ca462) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	// Unused

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END

static DRIVER_INIT( redhawk )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 16, 15, 14, 17, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

ROM_START( redhawkb )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rhb-1.bin", 0x000000, 0x020000, CRC(e733ea07) SHA1(b1ffeda633d5e701f0e97c79930a54d7b89a85c5) )
	ROM_LOAD16_BYTE( "rhb-2.bin", 0x000001, 0x020000, CRC(f9fa5684) SHA1(057ea3eebbaa1a208a72beef21b9368df7032ce1) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "1.bin", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD( "rhb-3.bin", 0x000000, 0x080000, CRC(0318d68b) SHA1(c773de7b6f9c706e62349dc73af4339d1a3f9af6) )
	ROM_LOAD( "rhb-4.bin", 0x080000, 0x080000, CRC(ba21c1ef) SHA1(66b0dee67acb5b3a21c7dba057be4093a92e10a9) )

	ROM_REGION( 0x080000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "rhb-5.bin", 0x000000, 0x080000, CRC(d0eaf6f2) SHA1(6e946e13b06df897a63e885c9842816ec908a709) )

	ROM_REGION( 0x080000, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "5", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END


/***************************************************************************

                            Sen Jin - Guardian Storm

(C) Afega 1998

CPU: 68HC000FN10 (68000, 68 pin PLCC)
SND: Z84C000FEC (Z80, 44 pin PQFP), AD-65 (44 pin PQFP, Probably OKI M6295),
     BS901 (Probably YM2151 or YM3812, 24 pin DIP), BS901 (possibly YM3014 or similar? 16 pin DIP)
OSC: 12.000MHz (near 68000), 4.000MHz (Near Z84000)
RAM: LH52B256 x 8, 6116 x 7
DIPS: 2 x 8 position

Other Chips: AFEGA AFI-GFSK (68 pin PLCC, located next to 68000)
             AFEGA AFI-GFLK (208 pin PQFP)

ROMS:
GST-01.U92   27C512, \
GST-02.U95   27C2000  > Sound Related, all located near Z80
GST-03.U4    27C512  /

GST-04.112   27C2000 \
GST-05.107   27C2000  /Main Program

GST-06.C13   read as 27C160  label = AF1-SP (Sprites?)
GST-07.C08   read as 27C160  label = AF1=B2 (Backgrounds?)
GST-08.C03   read as 27C160  label = AF1=B1 (Backgrounds?)

***************************************************************************/

ROM_START( grdnstrm )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "gst-04.112", 0x000000, 0x040000, CRC(922c931a) SHA1(1d1511033c8c424535a73f5c5bf58560a8b1842e) )
	ROM_LOAD16_BYTE( "gst-05.107", 0x000001, 0x040000, CRC(d22ca2dc) SHA1(fa21c8ec804570d64f4b167b7f65fd5811435e46) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "gst-01.u92", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD( "gst-06.c13", 0x000000, 0x200000, CRC(7d4d4985) SHA1(15c6c1aecd3f12050c1db2376f929f1a26a1d1cf) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "gst-07.c08", 0x000000, 0x200000, CRC(d68588c2) SHA1(c5f397d74a6ecfd2e375082f82e37c5a330fba62) )
	ROM_LOAD( "gst-08.c03", 0x200000, 0x200000, CRC(f8b200a8) SHA1(a6c43dd57b752d87138d7125b47dc0df83df8987) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "gst-03.u4",  0x00000, 0x10000, CRC(a1347297) SHA1(583f4da991eeedeb523cf4fa3b6900d40e342063) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "gst-02.u95", 0x00000, 0x40000, CRC(e911ce33) SHA1(a29c4dea98a22235122303325c63c15fadd3431d) )
ROM_END

static DRIVER_INIT( grdnstrm )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 16, 17, 14, 15, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}


/***************************************************************************

                            Bubble 2000 (c)1998 Tuning

Bubble 2000
Tuning, 1998

CPU   : TMP68HC000P-10 (68000)
SOUND : Z840006 (Z80, 44 pin QFP), YM2151, OKI M6295
OSC   : 4.000MHZ, 12.000MHz
DIPSW : 8 position (x2)
RAM   : 6116 (x5, gfx related?) 6116 (x1, sound program ram), 6116 (x1, near rom3)
        64256 (x4, gfx related?), 62256 (x2, main program ram), 6264 (x2, gfx related?)
PALs/PROMs: None
Custom: Unknown 208 pin QFP labelled LTC2 (Graphics generator)
        Unknown 68 pin PLCC labelled LTC1 (?, near rom 2 and rom 3)
ROMs  :

Filename    Type        Possible Use
----------------------------------------------
rom01.92    27C512      Sound Program
rom02.95    27C020      Oki Samples
rom03.4     27C512      ? (located near rom 1 and 2 and near LTC1)
rom04.1     27C040   \
rom05.3     27C040    |
rom06.6     27C040    |
rom07.9     27C040    | Gfx
rom08.11    27C040    |
rom09.14    27C040    |
rom12.2     27C040    |
rom13.7     27C040   /

rom10.112   27C040   \  Main Program
rom11.107   27C040   /




***************************************************************************/

ROM_START( bubl2000 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "rom10.112", 0x00000, 0x20000, CRC(87f960d7) SHA1(d22fe1740217ac20963bd9003245850598ccecf2) )
	ROM_LOAD16_BYTE( "rom11.107", 0x00001, 0x20000, CRC(b386041a) SHA1(cac36e22a39b5be0c5cd54dce5c912ff811edb28) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "rom01.92", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) ) /* same as the other games on this driver */

	ROM_REGION( 0x080000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "rom08.11", 0x000000, 0x040000, CRC(519dfd82) SHA1(116b06f6e7b283a5417338f716bbaab6cfadb41d) )
	ROM_LOAD16_BYTE( "rom09.14", 0x000001, 0x040000, CRC(04fcb5c6) SHA1(7594fa6bf98fc01b8848473a222a621c7c9ff00d) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "rom06.6",  0x000000, 0x080000, CRC(ac1aabf5) SHA1(abce6ba381b189ab3ec703a8ef74bccbe10876e0) )
	ROM_LOAD( "rom07.9",  0x080000, 0x080000, CRC(69aff769) SHA1(89b98c1023710861e622c8a186b6ec48f5109d42) )
	ROM_LOAD( "rom13.7",  0x100000, 0x080000, CRC(3a5b7226) SHA1(1127740c5bc2f830d73a77c8831e1b0db6606375) )
	ROM_LOAD( "rom04.1",  0x180000, 0x080000, CRC(46acd054) SHA1(1bd7a1b6b2ce6a3daa8c92843c546beb377af8fb) )
	ROM_LOAD( "rom05.3",  0x200000, 0x080000, CRC(37deb6a1) SHA1(3a8a3d961800bb15fd389429b92fa1e5b5f416df) )
	ROM_LOAD( "rom12.2",  0x280000, 0x080000, CRC(1fdc59dd) SHA1(d38e21c878241b4315a36e0590397211ca63f2c4) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "rom03.4",  0x00000, 0x10000, CRC(f4c15588) SHA1(a21ae71c0a8c7c1df63f9905fd86303bc2d3991c) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "rom02.95", 0x00000, 0x40000, CRC(859a86e5) SHA1(7b51964227411a40aac54b9cd9ff64f091bdf2b0) )
ROM_END


/*

Hot Bubble
Afega, 199?

PCB Layout
----------

Bottom Board

|------------------------------------------|
| BS902  BS901   Z80                  4MHz |
|                                          |
|        6116    6295                      |
|                                   62256  |
|      6116                         62256  |
|      6116                                |
|J           6116           |------------| |
|A           6116           |   68000    | |
|M                          |------------| |
|M  DSW2   6264                            |
|A         6264                            |
|                                          |
|         |-------|                        |
|         |       |                        |
|         |       |                        |
| DSW1    |       |     62256   62256      |
|         |-------|                        |
|              6116     62256   62256      |
|12MHz         6116                        |
|------------------------------------------|
Notes:
      68000 - running at 12.000MHz
      Z80   - running at 4.000MHz
      62256 - 32K x8 SRAM
      6264  - 8K x8 SRAM
      6116  - 2K x8 SRAM
      BS901 - YM2151, running at 4.000MHz
      BS902 - YM3012
      6295  - OKI MSM6295 running at 1.000MHz [4/4], sample rate = 1000000 / 132
      *     - Unknown QFP208
      VSync - 56.2Hz (measured on 68000 IPL1)

Top Board

|---------------------------|
|                           |
|   S1     S2        T1     |
|                           |
|   CR5    CR7       C1     |
|                           |
|   CR6   +CR8       C2     |
|                           |
|          BR1       BR3    |
|                           |
|         +BR2      +BR4    |
|                           |
|  CR1     CR3     |------| |
|                  |  *   | |
|  CR2    +CR4     |      | |
|                  |------| |
|---------------------------|
Notes:
      * - Unknown PLCC68 IC
      + - Not populated

*/

ROM_START( hotbubl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )		/* 68000 Code */
	ROM_LOAD16_BYTE( "c1.uc1",  0x00001, 0x40000, CRC(7bb240e9) SHA1(99048fa275182c3da3bfb0dedd790f4b5858bd92) )
	ROM_LOAD16_BYTE( "c2.uc9",  0x00000, 0x40000, CRC(7917b95d) SHA1(0344bae9c373c5943e7693720e5e531bc2e0d7ee) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )		/* Z80 Code */
	ROM_LOAD( "s1.uc14", 0x00000, 0x10000, CRC(5d8cf28e) SHA1(2a440bf5136f95af137b6688e566a14e65be94b1) ) /* same as the other games on this driver */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* Sprites, 16x16x4 */
	ROM_LOAD16_BYTE( "br1.uc3",  0x000000, 0x080000, CRC(6fc18de4) SHA1(57b4823fc41637780f64eadd1ddf61db531a2599) )
	ROM_LOAD16_BYTE( "br3.uc10", 0x000001, 0x080000, CRC(bb677240) SHA1(d7a26bcd33d491cee441edda6d092a1d08308b0e) )

	ROM_REGION( 0x300000, REGION_GFX2, ROMREGION_DISPOSE )	/* Layer 0, 16x16x8 */
	ROM_LOAD( "cr6.uc16",  0x100000, 0x080000, CRC(99d6523c) SHA1(0b628585d749e175d5a4dc600af1ba9cb936bfeb) )
	ROM_LOAD( "cr7.uc19",  0x080000, 0x080000, CRC(a89d9ce4) SHA1(5965b2b4b67bc91bc0e7474e593c7e1953b75adc) )
	ROM_LOAD( "cr5.uc15",  0x000000, 0x080000, CRC(65bd5159) SHA1(627ccc0ab131e643c3c52ee9bb41c7a85153c35e) )

	ROM_LOAD( "cr2.uc7",  0x280000, 0x080000, CRC(27ad6fc8) SHA1(00b1a5c5e1a245590b300b9baf71585d41813e3e) )
	ROM_LOAD( "cr3.uc12", 0x200000, 0x080000, CRC(c841a4f6) SHA1(9b0ee5623c87a0cfc63d3741a65d399bd6593f18) )
	ROM_LOAD( "cr1.uc6",  0x180000, 0x080000, CRC(fc9101d2) SHA1(1d5b8484264b6d73fe032946096a469226cce901) )

	ROM_REGION( 0x10000, REGION_GFX3, ROMREGION_DISPOSE )	/* Layer 1, 8x8x4 */
	ROM_LOAD( "t1.uc2",  0x00000, 0x10000, CRC(ce683a93) SHA1(aeee2671051f1badf2255375cd7c5fa847d1746c) )

	ROM_REGION( 0x40000, REGION_SOUND1, ROMREGION_SOUNDONLY )	/* Samples */
	ROM_LOAD( "s2.uc18", 0x00000, 0x40000, CRC(401c980f) SHA1(e47710c47cfeecce3ccf87f845b219a9c9f21ee3) )
ROM_END

static DRIVER_INIT( bubl2000 )
{
	decryptcode( 23, 22, 21, 20, 19, 18, 13, 14, 15, 16, 17, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 );
}

/*

Fire Hawk - ESD
---------------

- To enter test mode, hold on button 1 at boot up


PCB Layout
----------

ESD-PROT-002
|------------------------------------------------|
|      FHAWK_S1.U40   FHAWK_S2.U36               |
|      6116     6295  FHAWK_S3.U41               |
|               6295                 FHAWK_G1.UC6|
|    PAL   Z80                       FHAWK_G2.UC5|
|    4MHz                             |--------| |
|                                     | ACTEL  | |
|J   6116             62256           |A54SX16A| |
|A   6116             62256           |        | |
|M                                    |(QFP208)| |
|M                                    |--------| |
|A     DSW1              FHAWK_G3.UC2            |
|      DSW2                           |--------| |
|      DSW3                           | ACTEL  | |
|                     6116            |A54SX16A| |
|                     6116            |        | |
|      62256                          |(QFP208)| |
| FHAWK_P1.U59                        |--------| |
| FHAWK_P2.U60  PAL                  62256  62256|
|                                                |
|12MHz 62256   68000                 62256  62256|
|------------------------------------------------|
Notes:
      68000 clock: 12.000MHz
        Z80 clock: 4.000MHz
      6295 clocks: 1.000MHz (both), sample rate = 1000000 / 132 (both)
            VSync: 56Hz

*/

ROM_START( firehawk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )	/* 68000 Code */
	ROM_LOAD16_BYTE( "fhawk_p1.u59", 0x00001, 0x80000, CRC(d6d71a50) SHA1(e947720a0600d049b7ea9486442e1ba5582536c2) )
	ROM_LOAD16_BYTE( "fhawk_p2.u60", 0x00000, 0x80000, CRC(9f35d245) SHA1(5a22146f16bff7db924550970ed2a3048bc3edab) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )	/* Z80 Code */
	ROM_LOAD( "fhawk_s1.u40", 0x00000, 0x20000, CRC(c6609c39) SHA1(fe9b5f6c3ab42c48cb493fecb1181901efabdb58) )

	ROM_REGION( 0x200000, REGION_GFX1,ROMREGION_DISPOSE ) /* Sprites, 16x16x4 */
	ROM_LOAD( "fhawk_g3.uc2", 0x00000, 0x200000,  CRC(cae72ff4) SHA1(7dca7164015228ea039deffd234778d0133971ab) )

	ROM_REGION( 0x400000, REGION_GFX2, ROMREGION_DISPOSE ) /* Layer 0, 16x16x8 */
	ROM_LOAD( "fhawk_g1.uc6", 0x000000, 0x200000, CRC(2ab0b06b) SHA1(25362f6a517f188c62bac28b1a7b7b49622b1518) )
	ROM_LOAD( "fhawk_g2.uc5", 0x200000, 0x200000, CRC(d11bfa20) SHA1(15142004ab49f7f1e666098211dff0835c61df8d) )

	ROM_REGION( 0x00100, REGION_GFX3, ROMREGION_DISPOSE | ROMREGION_ERASEFF )	/* Layer 1, 8x8x4 */
	// Unused

	ROM_REGION( 0x040000, REGION_SOUND1, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "fhawk_s2.u36", 0x00000, 0x40000, CRC(d16aaaad) SHA1(96ca173ca433164ed0ae51b41b42343bd3cfb5fe) )

	ROM_REGION( 0x040000, REGION_SOUND2, ROMREGION_SOUNDONLY ) /* Samples */
	ROM_LOAD( "fhawk_s3.u41", 0x00000, 0x40000, CRC(3fdcfac2) SHA1(c331f2ea6fd682cfb00f73f9a5b995408eaab5cf) )
ROM_END

/***************************************************************************


                                Game Drivers


***************************************************************************/

GAMEX( 1998, stagger1, 0,        stagger1, stagger1, 0,        ROT270, "Afega", "Stagger I (Japan)",                GAME_NOT_WORKING )
GAMEX( 1997, redhawk,  stagger1, stagger1, stagger1, redhawk,  ROT270, "Afega", "Red Hawk (US)", GAME_NOT_WORKING )
GAMEX( 1997, redhawkb, stagger1, redhawkb, redhawkb, 0,        ROT0, "bootleg", "Red Hawk (bootleg)", GAME_NOT_WORKING )
GAMEX( 1998, grdnstrm, 0,        grdnstrm, grdnstrm, grdnstrm, ROT270, "Afega", "Sen Jin - Guardian Storm (Korea)", GAME_NOT_WORKING )
GAME ( 1998, bubl2000, 0,        bubl2000, bubl2000, bubl2000, ROT0,   "Tuning", "Bubble 2000" ) // on a tuning board (bootleg?)
GAME ( 1998, hotbubl,  bubl2000, bubl2000, bubl2000, bubl2000, ROT0,   "Pandora", "Hot Bubble" ) // on an afega board ..
GAMEX( 2001, firehawk, 0,        firehawk, firehawk, 0,        ORIENTATION_FLIP_Y, "ESD", "Fire Hawk", GAME_NOT_WORKING )
