/***********************************************************************

Namco System 1

Preliminary driver by:
Ernesto Corvi
ernesto@imagina.com

Updates by:
Vernon C. Brooks


Notes:
- The berabohm buttons don't work too well. The real thing has a special
  pressure sensitive button, with two switches. The harder you push the
  button, the faster the two switches are closed one after another.
  Due to MAME's limited input sample rate (once per frame) it is difficult
  to measure the time between the two.

Revisions:

6-xx-2002 Acho A. Tang

- added Bakutotsu Kijuutei
	The main and sub processors work closely together and the game
	quickly runs into trouble when one of them lacks behind. Heavy
	sync and overclocking will help but there's a particularly nasty
	loop appearing in more than 40 places of the code:

	1) kick the watchdog
	2) raise a flag
	3) do some dummy tasks
	4) check whether the flag has been reset by the sub CPU
	5) go back to step 1 if not

	MAME's timing is precise and CPU slices are consistant.
	Sometimes the flag never gets reset between step 2 and 4 and the
	main CPU is trapped in the loop. To make the game run reliably
	we have to force step 5 jump back to step 4 instead of 1.

- fixed Splatter House gameplay
	Splatter House' keychip is likely a random number generator as
	well. The patched version simulates its function with Quasi-
	random. We have more freedom in MAME so I chose the
	supposingly better Mersenne Twister generator.

- adpoted Bakutotu's driver and video scheme to other System1 games
	Playfield and sprite drawing are in sync with game logic and
	more stable visually. No serious side effect so far.

New characteristics of the Namco System1 observed:

. Priority is sprite to playfield only. Sprite-sprite priority depends
  solely on memory locations. Current implementation causes minor
  glitches in Rompers and Dragon Spirit. Switching to priority bitmap
  is unsuccessful. I don't know if I set the priority mask incorrectly
  or there are limitaions to pdrawgfx().

. Sprite RAM has underlying rules. The first 4 bytes in a sprite's
  encoding are pointers to data structures of some unknown functions.
  Performing INC or DEC directly on the 8th byte(X-position) shall set
  the first bit of the 7th byte(attributes) if there's a carry.

. Screen update does not happen automatically. Playfields are updated
  when $f600 of the main CPU is written and sprites are updated
  when $f600 of the sub CPU is written. Text maps are updated every
  frame regardless of interrupt status. Beraboh Man, Blazer and
  Soukoban DX may have the two ports connected because only the main
  CPU writes to it and sub CPU has sprite data ready way ahead of time.
  Coincidentally they could also be IRQ acknowledgements though having
  no effect on gameplay.

. Sprites are buffered. Immediate data at V-blank is prepared
  for the NEXT frame. This is true in 70% of the games except those
  with their sub CPUs taking an explicit lead in writing to $f600.


Unresolved issues and deficiencies:

. Rough screen transitions. No trace of masking attempts when video
  RAM is being updated.

. There could be a timing bug in current m6800/HD63701 emulation.
  Beraboh Man uses external IRQ0 to query input and internal OCI
  to write to the DACs. The problem occurs when two digital sounds
  overlap, where the frequency of OCI will drop to about a tenth of
  the normal rate and seriously distort the sounds. IRQ0 is also
  affected as one or more buttons will stop working. Things go back
  to normal when the samples are done playing but that could take
  a long time. * hasn't been tested against later m6800 updates *

. Are sprites coming out from the right of the screen in Tank Force
  supposed to flicker??

. TODO: convert sprite drawing to utilize priority bitmap


Namco System 1 hardware
=======================

Processors:

6809  - Main CPU
6809  - Sub CPU
6809  - Sound CPU (PSG,FM)
63701 - MCU (input,EEPROM,DAC)

Inter-processor communication is done via a 2K shared memory area.

Bankswitching:

Main/Sub - a 10-bit value is written to location Ex00 to select the 8K
bank (RAM or PRG0-PRG7) which is accessed at offset x000. (x is even)

Sound - a 3-bit value is written to location C000 or C001, bits 4-6 to
select the 16K bank (SND0-SND1) which is accessed at offset 0000.

MCU - a 8-bit value is written to location D800 to select the 32K bank
(VOI0-VOI5) which is accessed at offset 4000. Bits 2-7 are a bitmask
which specify the ROM to access and bits 0-1 specify the ROM offset.

Graphics:

Visible screen resolution: 288x244 pixels (36x28 tiles)

3 scrolling 64x64 tilemapped playfields
1 scrolling 64x32 tilemapped playfield
2 fixed 36x28 tilemapped playfields

Each playfield uses one of 8 color palettes, can be enabled or disabled,
and has programmable priorities.

Each tile is a 8x8, 8 bit-per-pixel character from a selection of up to
16384 characters (CHR0-CHR7). A separate 1 bit-per-pixel character mask
(CHR8) defines the character shape.

127 displayable 32x32, 4 bit-per-pixel sprites from a selection of up to
2048 sprites (OBJ0-OBJ7). Each sprite uses one of 127 color palettes or
a special shadow/highlight effect and has programmable priorities and
x-y flipping. Sprites may also be displayed as a smaller portion of a
32x32 object with a programmable size and position. The height and width
are programmed separately and may be 4,8,16,or 32 pixels.

3 24-bit programmable RGB palette tables, 8 bits per color as follows:

127 16-color entries for the sprites
8 256-color entries for the playfields
8 256-color entries for the playfields shadow/highlight effects

Sound:

Namco custom 8 channel 16-bit stereo PSG for sound effects
registor array based 2 channel 8-bit DAC for voice
Yamaha YM2151+YM3012 FM chip for background music

Controls:

The standard hardware supports one or two 8-way joysticks with up to
three buttons for each player, two start buttons, a service switch, two
coin slots, and one dipswitch block. Game settings are accessed via
service mode and are saved in EEPROM.

Games:

Date  Name                                  Key  Screen
----- ------------------------------------- ---- ------
 4/87 Yokai Douchuuki / Shadowland          NONE H
 6/87 Dragon Spirit (old version)           136  V
??/87 Dragon Spirit (new version)           136  V
 7/87 Blazer                                144  V
 9/87 Quester                               A    V
??/87 Quester (special edition)             A    V
11/87 Pac-Mania                             151  V-FLIP
11/87 Pac-Mania (Japanese version)          151  V
12/87 Galaga '88                            153  V-FLIP
12/87 Galaga '88 (Japanese version)         153  V
 3/88 World Stadium                         154  H
 5/88 Beraboh Man                           B    H
??/88 Beraboh Man (standard NS1 hardware)   NONE H
 7/88 Marchen Maze / Alice In Wonderland    152  H
 8/88 Bakutotsu Kijuutei / Baraduke 2       155  H
10/88 World Court                           143  H
11/88 Splatter House                        181  H
12/88 Face Off                              C    H
 2/89 Rompers                               182  V
 3/89 Blast Off                             183  V
 7/89 World Stadium '89                     184  H
12/89 Dangerous Seed                        308  V
 7/90 World Stadium '90                     310  H
10/90 Pistol Daimyo no Bouken               309  H-FLIP
11/90 Souko Ban Deluxe                      311  H-FLIP
??/90 Puzzle Club (prototype)               ?    V
12/91 Tank Force                            185  H-FLIP

A - uses sub board with paddle control(s)
B - uses sub board with pressure sensitive controls
C - uses sub board with support for player 3 and 4 controls

***********************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6800/m6800.h"

/* from vidhrdw */
extern VIDEO_START( namcos1 );
extern VIDEO_UPDATE( namcos1 );

/* from machine */
WRITE_HANDLER( namcos1_bankswitch_w );
WRITE_HANDLER( namcos1_subcpu_bank_w );

WRITE_HANDLER( namcos1_cpu_control_w );
WRITE_HANDLER( namcos1_sound_bankswitch_w );

WRITE_HANDLER( namcos1_mcu_bankswitch_w );
WRITE_HANDLER( namcos1_mcu_patch_w );

extern MACHINE_INIT( namcos1 );

extern DRIVER_INIT( shadowld );
extern DRIVER_INIT( dspirit );
extern DRIVER_INIT( quester );
extern DRIVER_INIT( blazer );
extern DRIVER_INIT( pacmania );
extern DRIVER_INIT( galaga88 );
extern DRIVER_INIT( ws );
extern DRIVER_INIT( berabohm );
extern DRIVER_INIT( alice );
extern DRIVER_INIT( bakutotu );
extern DRIVER_INIT( wldcourt );
extern DRIVER_INIT( splatter );
extern DRIVER_INIT( faceoff );
extern DRIVER_INIT( rompers );
extern DRIVER_INIT( blastoff );
extern DRIVER_INIT( ws89 );
extern DRIVER_INIT( dangseed );
extern DRIVER_INIT( ws90 );
extern DRIVER_INIT( pistoldm );
extern DRIVER_INIT( soukobdx );
extern DRIVER_INIT( puzlclub );
extern DRIVER_INIT( tankfrce );


/**********************************************************************/

static WRITE_HANDLER( namcos1_sub_firq_w )
{
	cpu_set_irq_line(1, 1, HOLD_LINE);
}



static MEMORY_READ_START( main_readmem )
	{ 0x0000, 0x1fff, MRA_BANK1 },
	{ 0x2000, 0x3fff, MRA_BANK2 },
	{ 0x4000, 0x5fff, MRA_BANK3 },
	{ 0x6000, 0x7fff, MRA_BANK4 },
	{ 0x8000, 0x9fff, MRA_BANK5 },
	{ 0xa000, 0xbfff, MRA_BANK6 },
	{ 0xc000, 0xdfff, MRA_BANK7 },
	{ 0xe000, 0xffff, MRA_BANK8 },
MEMORY_END

static MEMORY_WRITE_START( main_writemem )
	{ 0x0000, 0x1fff, MWA_BANK1 },
	{ 0x2000, 0x3fff, MWA_BANK2 },
	{ 0x4000, 0x5fff, MWA_BANK3 },
	{ 0x6000, 0x7fff, MWA_BANK4 },
	{ 0x8000, 0x9fff, MWA_BANK5 },
	{ 0xa000, 0xbfff, MWA_BANK6 },
	{ 0xc000, 0xdfff, MWA_BANK7 },
	{ 0xe000, 0xefff, namcos1_bankswitch_w },
	{ 0xf000, 0xf000, namcos1_cpu_control_w },
	{ 0xf200, 0xf200, MWA_NOP }, // watchdog
	{ 0xf400, 0xf400, MWA_NOP }, // unknown
	{ 0xf600, 0xf600, MWA_NOP }, // main video update(handler installs separately)
	{ 0xfa00, 0xfa00, namcos1_sub_firq_w }, // asserts FIRQ on CPU1
	{ 0xfc00, 0xfc01, namcos1_subcpu_bank_w },
MEMORY_END

static MEMORY_READ_START( sub_readmem )
	{ 0x0000, 0x1fff, MRA_BANK9 },
	{ 0x2000, 0x3fff, MRA_BANK10 },
	{ 0x4000, 0x5fff, MRA_BANK11 },
	{ 0x6000, 0x7fff, MRA_BANK12 },
	{ 0x8000, 0x9fff, MRA_BANK13 },
	{ 0xa000, 0xbfff, MRA_BANK14 },
	{ 0xc000, 0xdfff, MRA_BANK15 },
	{ 0xe000, 0xffff, MRA_BANK16 },
MEMORY_END

static MEMORY_WRITE_START( sub_writemem )
	{ 0x0000, 0x1fff, MWA_BANK9 },
	{ 0x2000, 0x3fff, MWA_BANK10 },
	{ 0x4000, 0x5fff, MWA_BANK11 },
	{ 0x6000, 0x7fff, MWA_BANK12 },
	{ 0x8000, 0x9fff, MWA_BANK13 },
	{ 0xa000, 0xbfff, MWA_BANK14 },
	{ 0xc000, 0xdfff, MWA_BANK15 },
	{ 0xe000, 0xefff, namcos1_bankswitch_w },
	{ 0xf000, 0xf000, MWA_NOP }, // IO Chip
	{ 0xf200, 0xf200, MWA_NOP }, // watchdog
	{ 0xf400, 0xf400, MWA_NOP }, // unknown
	{ 0xf600, 0xf600, MWA_NOP }, // sub video update(handler installs separately)
MEMORY_END


static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_BANK17 }, /* Banked ROMs */
	{ 0x4000, 0x4001, YM2151_status_port_0_r },
	{ 0x5000, 0x50ff, namcos1_wavedata_r }, /* PSG ( Shared ) */
	{ 0x5100, 0x513f, namcos1_sound_r },    /* PSG ( Shared ) */
	{ 0x5140, 0x54ff, MRA_RAM },    /* Sound RAM 1 - ( Shared ) */
	{ 0x7000, 0x77ff, MRA_BANK18 }, /* Sound RAM 2 - ( Shared ) */
	{ 0x8000, 0x9fff, MRA_RAM },    /* Sound RAM 3 */
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },    /* Banked ROMs */
	{ 0x4000, 0x4000, YM2151_register_port_0_w },
	{ 0x4001, 0x4001, YM2151_data_port_0_w },
	{ 0x5000, 0x50ff, namcos1_wavedata_w,&namco_wavedata }, /* PSG ( Shared ) */
	{ 0x5100, 0x513f, namcos1_sound_w,&namco_soundregs },   /* PSG ( Shared ) */
	{ 0x5140, 0x54ff, MWA_RAM },    /* Sound RAM 1 - ( Shared ) */
	{ 0x7000, 0x77ff, MWA_BANK18 }, /* Sound RAM 2 - ( Shared ) */
	{ 0x8000, 0x9fff, MWA_RAM },    /* Sound RAM 3 */
	{ 0xc000, 0xc001, namcos1_sound_bankswitch_w }, /* bank selector */
	{ 0xd001, 0xd001, MWA_NOP },    /* watchdog? */
	{ 0xe000, 0xe000, MWA_NOP },    /* IRQ clear ? */
MEMORY_END

static READ_HANDLER( dsw_r )
{
	int ret = readinputport(2);
	if(offset&2) ret>>=4;
	return 0xf0 | ret;
}

static WRITE_HANDLER( namcos1_coin_w )
{
	coin_lockout_global_w(~data & 1);
	coin_counter_w(0,data & 2);
	coin_counter_w(1,data & 4);
}

static int dac0_value ,dac1_value, dac0_gain=0, dac1_gain=0;

static void namcos1_update_DACs(void)
{
	DAC_signed_data_16_w(0,0x8000+(dac0_value * dac0_gain)+(dac1_value * dac1_gain));
}

static WRITE_HANDLER( namcos1_dac_gain_w )
{
	int value;
	/* DAC0 */
	value = (data&1)|((data>>1)&2); /* GAIN0,GAIN1 */
	dac0_gain = 0x0101 * (value+1) /4 /2;
	/* DAC1 */
	value = (data>>3)&3; /* GAIN2,GAIN3 */
	dac1_gain = 0x0101 * (value+1) / 4 /2;
	namcos1_update_DACs();
}

static WRITE_HANDLER( namcos1_dac0_w )
{
	dac0_value = data-0x80; /* shift zero point */
	namcos1_update_DACs();
}

static WRITE_HANDLER( namcos1_dac1_w )
{
	dac1_value = data-0x80; /* shift zero point */
	namcos1_update_DACs();
}

static int num=0, strobe=0;

static READ_HANDLER( quester_in0_r )
{
	int ret;

	if (!num)
		ret = (readinputport(0)&0x90) | strobe | (readinputport(4)&0x0f);
	else
		ret = (readinputport(0)&0x90) | strobe | (readinputport(5)&0x0f);

	strobe ^= 0x40;

	return ret;
}

static READ_HANDLER( quester_in1_r )
{
	int ret;

	if (!num)
		ret = (readinputport(1)&0x90) | num | (readinputport(4)>>4);
	else
		ret = (readinputport(1)&0x90) | num | (readinputport(5)>>4);

	if (!strobe) num ^= 0x20;

	return ret;
}

static READ_HANDLER( faceoff_in0_r )
{
	int ret;

	if (!num)
		ret = (readinputport(0)&0x80) | (readinputport(4)&0x1f);
	else if (num==3)
		ret = (readinputport(0)&0x80) | (readinputport(5)&0x1f);
	else
		ret = (readinputport(0)&0x80) | (readinputport(6)&0x1f);

	return ret;
}

static READ_HANDLER( faceoff_in1_r )
{
	int ret;

	if (strobe)
	{
		if (!num)
			ret = (readinputport(1)&0x80) | strobe | ((readinputport(7)&0x07)<<3);
		else
			ret = (readinputport(1)&0x80) | strobe | (readinputport(7)&0x18);
	}
	else
	{
		if (num==0) num=3;
		else if (num==3) num=4;
		else if (num==4) num=0;
		ret = (readinputport(1)&0x80) | num;
	}

	strobe ^= 0x40;

	return ret;
}

static MEMORY_READ_START( mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM }, /* built in RAM */
	{ 0x1400, 0x1400, input_port_0_r },
	{ 0x1401, 0x1401, input_port_1_r },
	{ 0x1000, 0x1003, dsw_r },
	{ 0x4000, 0xbfff, MRA_BANK20 }, /* banked ROM */
	{ 0xc000, 0xc7ff, MRA_BANK19 },
	{ 0xc800, 0xcfff, MRA_RAM }, /* EEPROM */
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( mcu_writemem )
	{ 0x0000, 0x001f, hd63701_internal_registers_w },
	{ 0x0080, 0x00ff, MWA_RAM }, /* built in RAM */
	{ 0x4000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc000, namcos1_mcu_patch_w },
	{ 0xc000, 0xc7ff, MWA_BANK19 },
	{ 0xc800, 0xcfff, MWA_RAM, &generic_nvram, &generic_nvram_size }, /* EEPROM */
	{ 0xd000, 0xd000, namcos1_dac0_w },
	{ 0xd400, 0xd400, namcos1_dac1_w },
	{ 0xd800, 0xd800, namcos1_mcu_bankswitch_w }, /* BANK selector */
	{ 0xf000, 0xf000, MWA_NOP }, /* IRQ clear ? */
MEMORY_END

static MEMORY_READ_START( quester_mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM }, /* built in RAM */
	{ 0x1400, 0x1400, quester_in0_r },
	{ 0x1401, 0x1401, quester_in1_r },
	{ 0x1000, 0x1003, dsw_r },
	{ 0x4000, 0xbfff, MRA_BANK20 }, /* banked ROM */
	{ 0xc000, 0xc7ff, MRA_BANK19 },
	{ 0xc800, 0xcfff, MRA_RAM }, /* EEPROM */
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START( faceoff_mcu_readmem )
	{ 0x0000, 0x001f, hd63701_internal_registers_r },
	{ 0x0080, 0x00ff, MRA_RAM }, /* built in RAM */
	{ 0x1400, 0x1400, faceoff_in0_r },
	{ 0x1401, 0x1401, faceoff_in1_r },
	{ 0x1000, 0x1003, dsw_r },
	{ 0x4000, 0xbfff, MRA_BANK20 }, /* banked ROM */
	{ 0xc000, 0xc7ff, MRA_BANK19 },
	{ 0xc800, 0xcfff, MRA_RAM }, /* EEPROM */
	{ 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static PORT_READ_START( mcu_readport )
	{ HD63701_PORT1, HD63701_PORT1, input_port_3_r },
	{ 0x0101, 0x0101, MRA_NOP }, // unknown port read(added to cut the number of error warnings)
PORT_END

static PORT_WRITE_START( mcu_writeport )
	{ HD63701_PORT1, HD63701_PORT1, namcos1_coin_w },
	{ HD63701_PORT2, HD63701_PORT2, namcos1_dac_gain_w },
PORT_END



/* Standard Namco System 1 input port definition */
INPUT_PORTS_START( ns1 )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Auto Data Sampling" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN2 : mcu PORT2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Bakutotsu Kijyutei input port definition - dip switches are different */
INPUT_PORTS_START( bakutotu )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Service_Mode ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Freeze" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x80, 0x80, "Sprite Viewer" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* IN2 : mcu PORT2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Dragon Spirit input port definition - dip switches are different */
INPUT_PORTS_START( dspirit )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x7f, 0x7f, "Life" )
	PORT_DIPSETTING(    0x7f, "2" )
	PORT_DIPSETTING(    0x69, "3" )
	PORT_SERVICE( 0x08, IP_ACTIVE_LOW )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Quester input port definition - paddle controls */
INPUT_PORTS_START( quester )
	PORT_START      /* IN0 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )     /* paddle */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x0f, IP_ACTIVE_LOW, IPT_UNUSED )     /* paddle */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Auto Data Sampling" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN4 - fake input port for player 1 paddle */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL, 30, 15, 0, 0 )

	PORT_START      /* IN5 - fake input port for player 2 paddle */
	PORT_ANALOG( 0xff, 0x00, IPT_DIAL | IPF_PLAYER2, 30, 15, 0, 0 )
INPUT_PORTS_END

/* Face Off input port definition - 4 player controls */
INPUT_PORTS_START( faceoff )
	PORT_START      /* IN0 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x7f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Auto Data Sampling" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN4 - fake input port for player 1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN5 - fake input port for player 2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER2 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN6 - fake input port for player 3 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER3 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER3 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER3 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START      /* IN7 - fake input port for player 4 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER4 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_PLAYER4 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_PLAYER4 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_PLAYER4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1        | IPF_PLAYER4 )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Beraboh Man input port definition - controls are different */
INPUT_PORTS_START( berabohm )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_COCKTAIL )
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_SPECIAL )    /* timing from the buttons interface */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START      /* DSW1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "Auto Data Sampling" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_DIPNAME( 0x40, 0x40, "Freeze" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin lockout */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 1 */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )   /* OUT:coin counter 2 */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_SERVICE, "Service Button", KEYCODE_F1, IP_JOY_NONE )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	/*
	buttons (pressure sensitive)
	each button has two switches: the first is closed as soon as the button is
	pressed, the second a little later, depending on how hard the button is
	pressed.
	bits 0-5 control strength (0x00 = max 0x3f = min)
	bit 6 indicates the button is pressed
	bit 7 is not actually read by the game but I use it to simulate the second
		  switch
	*/
	PORT_START
	PORT_BIT( 0x3f, 0x00, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )

	PORT_START
	PORT_BIT( 0x3f, 0x00, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 )

	PORT_START
	PORT_BIT( 0x3f, 0x00, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 | IPF_COCKTAIL )

	PORT_START
	PORT_BIT( 0x3f, 0x00, IPT_SPECIAL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_COCKTAIL )
INPUT_PORTS_END

static struct GfxLayout tilelayout =
{
	8,8,    /* 8*8 characters */
	16384,  /* 16384 characters max */
	8,      /* 8 bits per pixel */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },     /* bitplanes offset */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64 },
	64*8    /* every char takes 64 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,  /* 32*32 sprites */
	2048,   /* 2048 sprites max */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },  /* the bitplanes are packed */
	{  0*4,  1*4,  2*4,  3*4,  4*4,  5*4,  6*4,  7*4,
	   8*4,  9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,
	 256*4,257*4,258*4,259*4,260*4,261*4,262*4,263*4,
	 264*4,265*4,266*4,267*4,268*4,269*4,270*4,271*4},
	{ 0*4*16, 1*4*16,  2*4*16,  3*4*16,  4*4*16,  5*4*16,  6*4*16,  7*4*16,
	  8*4*16, 9*4*16, 10*4*16, 11*4*16, 12*4*16, 13*4*16, 14*4*16, 15*4*16,
	 32*4*16,33*4*16, 34*4*16, 35*4*16, 36*4*16, 37*4*16, 38*4*16, 39*4*16,
	 40*4*16,41*4*16, 42*4*16, 43*4*16, 44*4*16, 45*4*16, 46*4*16, 47*4*16 },
	32*4*8*4  /* every sprite takes 512 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &tilelayout,  128*16,   6 },  /* characters */
	{ REGION_GFX3, 0, &spritelayout,     0, 128 },  /* sprites 32/16/8/4 dots */
	{ -1 } /* end of array */
};

static void namcos1_sound_interrupt( int irq )
{
	cpu_set_irq_line( 2, M6809_FIRQ_LINE , irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	1,          /* 1 chip */
	3579580,    /* 3.58 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ namcos1_sound_interrupt },
	{ 0 }
};

static struct namco_interface namco_interface =
{
	24000/2,    /* sample rate (approximate value) */
	8,          /* number of voices */
	50,         /* playback volume */
	-1,         /* memory region */
	1           /* stereo */
};

/*
	namcos1 has tow 8bit dac channel. But They are mixed before pre-amp.
	And,they are connected with pre-amp through active LPF.
	LFP info : Fco = 3.3KHz , g = -12dB/oct
*/
static struct DACinterface dac_interface =
{
	1,          /* 2 channel , but they are mixed by the driver */
	{ 100   }   /* mixing level */
};


static MACHINE_DRIVER_START( ns1 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main",M6809,49152000/24) /* Not sure if divided by 32 or 24 */
	MDRV_CPU_MEMORY(main_readmem, main_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD_TAG("sub",M6809,49152000/24) /* Not sure if divided by 32 or 24 */
	MDRV_CPU_MEMORY(sub_readmem, sub_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD(M6809,49152000/32) /* Not sure if divided by 32 or 24 */
	MDRV_CPU_MEMORY(sound_readmem, sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_CPU_ADD_TAG("MCU",HD63701,49152000/32) /* Not sure if divided by 32 or 24 */
	MDRV_CPU_MEMORY(mcu_readmem, mcu_writemem)
	MDRV_CPU_PORTS(mcu_readport, mcu_writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60) // 60.606060?
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(240) // necessary to ensure the 1st and 2nd CPU's work in tandem
	MDRV_MACHINE_INIT(namcos1)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(36*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 36*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(128*16+6*256+6*256+1)

	MDRV_VIDEO_START(namcos1)
	MDRV_VIDEO_UPDATE(namcos1)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
	MDRV_SOUND_ADD(NAMCO, namco_interface)
	MDRV_SOUND_ADD(DAC, dac_interface)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( bakutotu )

	MDRV_IMPORT_FROM(ns1)

	//MDRV_CPU_REPLACE("main",M6809,49152000/24) // reduces screen flickers(optional)
	//MDRV_CPU_REPLACE("sub",M6809,49152000/24) // reduces screen flickers(optional)

	// heavy sync required to prevent CPUs from fighting for video RAM access and going into deadlocks
	MDRV_INTERLEAVE(640)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( quester )

	MDRV_IMPORT_FROM(ns1)

	MDRV_CPU_MODIFY("MCU")
	MDRV_CPU_MEMORY(quester_mcu_readmem,mcu_writemem)

MACHINE_DRIVER_END


static MACHINE_DRIVER_START( faceoff )

	MDRV_IMPORT_FROM(ns1)

	MDRV_CPU_MODIFY("MCU")
	MDRV_CPU_MEMORY(faceoff_mcu_readmem,mcu_writemem)

MACHINE_DRIVER_END


/***********************************************************************

  Game drivers

***********************************************************************/
/* load half size ROM to full size space */
#define ROM_LOAD_HS(name,start,length,crc) \
	ROM_LOAD(name,start,length,crc) \
	ROM_RELOAD(start+length,length)

/* Shadowland */
ROM_START( shadowld )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "yd1_s0.bin",         0x0c000, 0x10000, 0xa9cb51fb )
	ROM_LOAD( "yd1_s1.bin",         0x1c000, 0x10000, 0x65d1dc0d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "yd3_p7.bin",      0x00000, 0x10000, 0xf1c271a0 )
	ROM_LOAD_HS( "yd3_p6.bin",      0x20000, 0x10000, 0x93d6811c )
	ROM_LOAD_HS( "yd1_p5.bin",      0x40000, 0x10000, 0x29a78bd6 )
	ROM_LOAD_HS( "yd1_p3.bin",      0x80000, 0x10000, 0xa4f27c24 )
	ROM_LOAD_HS( "yd1_p2.bin",      0xa0000, 0x10000, 0x62e5bbec )
	ROM_LOAD_HS( "yd1_p1.bin",      0xc0000, 0x10000, 0xa8ea6bd3 )
	ROM_LOAD_HS( "yd1_p0.bin",      0xe0000, 0x10000, 0x07e49883 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */
	/* 0x00000 - 0x08000 = RAM6 ( 4 * 8k ) */
	/* 0x08000 - 0x0c000 = RAM1 ( 2 * 8k ) */
	/* 0x0c000 - 0x14000 = RAM3 ( 4 * 8k ) */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "yd_voi-0.bin",       0x20000, 0x10000, 0x448bc6a1 ) // yd1.v0 + yd1.v3
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "yd_voi-1.bin",       0x30000, 0x20000, 0x7809035c ) // yd1.v1 + yd1.v4
	ROM_LOAD( "yd_voi-2.bin",       0x50000, 0x20000, 0x73bffc16 ) // yd1.v2 + yd1.v5

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "yd_chr-8.bin",       0x00000, 0x20000, 0x0c8e69d0 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "yd_chr-0.bin",       0x00000, 0x20000, 0x717441dd )
	ROM_LOAD( "yd_chr-1.bin",       0x20000, 0x20000, 0xc1be6e35 )
	ROM_LOAD( "yd_chr-2.bin",       0x40000, 0x20000, 0x2df8d8cc )
	ROM_LOAD( "yd_chr-3.bin",       0x60000, 0x20000, 0xd4e15c9e )
	ROM_LOAD( "yd_chr-4.bin",       0x80000, 0x20000, 0xc0041e0d )
	ROM_LOAD( "yd_chr-5.bin",       0xa0000, 0x20000, 0x7b368461 )
	ROM_LOAD( "yd_chr-6.bin",       0xc0000, 0x20000, 0x3ac6a90e )
	ROM_LOAD( "yd_chr-7.bin",       0xe0000, 0x20000, 0x8d2cffa5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "yd_obj-0.bin",       0x00000, 0x20000, 0xefb8efe3 )
	ROM_LOAD( "yd_obj-1.bin",       0x20000, 0x20000, 0xbf4ee682 )
	ROM_LOAD( "yd_obj-2.bin",       0x40000, 0x20000, 0xcb721682 )
	ROM_LOAD( "yd_obj-3.bin",       0x60000, 0x20000, 0x8a6c3d1c )
	ROM_LOAD( "yd_obj-4.bin",       0x80000, 0x20000, 0xef97bffb )
	ROM_LOAD_HS( "yd3_obj5.bin",    0xa0000, 0x10000, 0x1e4aa460 )
ROM_END

/* Youkai Douchuuki (Shadowland Japan) */
ROM_START( youkaidk )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "yd1.sd0",            0x0c000, 0x10000, 0xa9cb51fb )
	ROM_LOAD( "yd1.sd1",            0x1c000, 0x10000, 0x65d1dc0d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "yd2_p7b.bin",     0x00000, 0x10000, 0xa05bf3ae )
	ROM_LOAD_HS( "yd1_p6.bin",      0x20000, 0x10000, 0x785a2772 )
	ROM_LOAD_HS( "yd1_p5.bin",      0x40000, 0x10000, 0x29a78bd6 )
	ROM_LOAD_HS( "yd1_p3.bin",      0x80000, 0x10000, 0xa4f27c24 )
	ROM_LOAD_HS( "yd1_p2.bin",      0xa0000, 0x10000, 0x62e5bbec )
	ROM_LOAD_HS( "yd1_p1.bin",      0xc0000, 0x10000, 0xa8ea6bd3 )
	ROM_LOAD_HS( "yd1_p0.bin",      0xe0000, 0x10000, 0x07e49883 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "yd_voi-0.bin",       0x20000, 0x10000, 0x448bc6a1 ) // yd1.v0 + yd1.v3
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "yd_voi-1.bin",       0x30000, 0x20000, 0x7809035c ) // yd1.v1 + yd1.v4
	ROM_LOAD( "yd_voi-2.bin",       0x50000, 0x20000, 0x73bffc16 ) // yd1.v2 + yd1.v5

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "yd_chr-8.bin",       0x00000, 0x20000, 0x0c8e69d0 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "yd_chr-0.bin",       0x00000, 0x20000, 0x717441dd )
	ROM_LOAD( "yd_chr-1.bin",       0x20000, 0x20000, 0xc1be6e35 )
	ROM_LOAD( "yd_chr-2.bin",       0x40000, 0x20000, 0x2df8d8cc )
	ROM_LOAD( "yd_chr-3.bin",       0x60000, 0x20000, 0xd4e15c9e )
	ROM_LOAD( "yd_chr-4.bin",       0x80000, 0x20000, 0xc0041e0d )
	ROM_LOAD( "yd_chr-5.bin",       0xa0000, 0x20000, 0x7b368461 )
	ROM_LOAD( "yd_chr-6.bin",       0xc0000, 0x20000, 0x3ac6a90e )
	ROM_LOAD( "yd_chr-7.bin",       0xe0000, 0x20000, 0x8d2cffa5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "yd_obj-0.bin",       0x00000, 0x20000, 0xefb8efe3 )
	ROM_LOAD( "yd_obj-1.bin",       0x20000, 0x20000, 0xbf4ee682 )
	ROM_LOAD( "yd_obj-2.bin",       0x40000, 0x20000, 0xcb721682 )
	ROM_LOAD( "yd_obj-3.bin",       0x60000, 0x20000, 0x8a6c3d1c )
	ROM_LOAD( "yd_obj-4.bin",       0x80000, 0x20000, 0xef97bffb )
ROM_END

/* Youkai Douchuuki (Shadowland Japan old version) */
ROM_START( yokaidko )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "yd1.sd0",            0x0c000, 0x10000, 0xa9cb51fb )
	ROM_LOAD( "yd1.sd1",            0x1c000, 0x10000, 0x65d1dc0d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "yd2_p7.bin",      0x00000, 0x10000, 0x3d39098c )
	ROM_LOAD_HS( "yd1_p6.bin",      0x20000, 0x10000, 0x785a2772 )
	ROM_LOAD_HS( "yd1_p5.bin",      0x40000, 0x10000, 0x29a78bd6 )
	ROM_LOAD_HS( "yd1_p3.bin",      0x80000, 0x10000, 0xa4f27c24 )
	ROM_LOAD_HS( "yd1_p2.bin",      0xa0000, 0x10000, 0x62e5bbec )
	ROM_LOAD_HS( "yd1_p1.bin",      0xc0000, 0x10000, 0xa8ea6bd3 )
	ROM_LOAD_HS( "yd1_p0.bin",      0xe0000, 0x10000, 0x07e49883 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "yd_voi-0.bin",       0x20000, 0x10000, 0x448bc6a1 ) // yd1.v0 + yd1.v3
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "yd_voi-1.bin",       0x30000, 0x20000, 0x7809035c ) // yd1.v1 + yd1.v4
	ROM_LOAD( "yd_voi-2.bin",       0x50000, 0x20000, 0x73bffc16 ) // yd1.v2 + yd1.v5

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "yd_chr-8.bin",       0x00000, 0x20000, 0x0c8e69d0 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "yd_chr-0.bin",       0x00000, 0x20000, 0x717441dd )
	ROM_LOAD( "yd_chr-1.bin",       0x20000, 0x20000, 0xc1be6e35 )
	ROM_LOAD( "yd_chr-2.bin",       0x40000, 0x20000, 0x2df8d8cc )
	ROM_LOAD( "yd_chr-3.bin",       0x60000, 0x20000, 0xd4e15c9e )
	ROM_LOAD( "yd_chr-4.bin",       0x80000, 0x20000, 0xc0041e0d )
	ROM_LOAD( "yd_chr-5.bin",       0xa0000, 0x20000, 0x7b368461 )
	ROM_LOAD( "yd_chr-6.bin",       0xc0000, 0x20000, 0x3ac6a90e )
	ROM_LOAD( "yd_chr-7.bin",       0xe0000, 0x20000, 0x8d2cffa5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "yd_obj-0.bin",       0x00000, 0x20000, 0xefb8efe3 )
	ROM_LOAD( "yd_obj-1.bin",       0x20000, 0x20000, 0xbf4ee682 )
	ROM_LOAD( "yd_obj-2.bin",       0x40000, 0x20000, 0xcb721682 )
	ROM_LOAD( "yd_obj-3.bin",       0x60000, 0x20000, 0x8a6c3d1c )
	ROM_LOAD( "yd_obj-4.bin",       0x80000, 0x20000, 0xef97bffb )
ROM_END

/* Dragon Spirit */
ROM_START( dspirit )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "ds1_s0.bin",         0x0c000, 0x10000, 0x27100065 )
	ROM_LOAD( "ds1_s1.bin",         0x1c000, 0x10000, 0xb398645f )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "ds3_p7.bin",      0x00000, 0x10000, 0x820bedb2 )
	ROM_LOAD_HS( "ds3_p6.bin",      0x20000, 0x10000, 0xfcc01bd1 )
	ROM_LOAD_HS( "ds1_p5.bin",      0x40000, 0x10000, 0x9a3a1028 )
	ROM_LOAD_HS( "ds1_p4.bin",      0x60000, 0x10000, 0xf3307870 )
	ROM_LOAD_HS( "ds1_p3.bin",      0x80000, 0x10000, 0xc6e5954b )
	ROM_LOAD_HS( "ds1_p2.bin",      0xa0000, 0x10000, 0x3c9b0100 )
	ROM_LOAD_HS( "ds1_p1.bin",      0xc0000, 0x10000, 0xf7e3298a )
	ROM_LOAD_HS( "ds1_p0.bin",      0xe0000, 0x10000, 0xb22a2856 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xb0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "ds1_v0.bin",      0x10000, 0x10000, 0x313b3508 )
	ROM_LOAD( "ds_voi-1.bin",       0x30000, 0x20000, 0x54790d4e )
	ROM_LOAD( "ds_voi-2.bin",       0x50000, 0x20000, 0x05298534 )
	ROM_LOAD( "ds_voi-3.bin",       0x70000, 0x20000, 0x13e84c7e )
	ROM_LOAD( "ds_voi-4.bin",       0x90000, 0x20000, 0x34fbb8cd )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "ds_chr-8.bin",       0x00000, 0x20000, 0x946eb242 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "ds_chr-0.bin",       0x00000, 0x20000, 0x7bf28ac3 )
	ROM_LOAD( "ds_chr-1.bin",       0x20000, 0x20000, 0x03582fea )
	ROM_LOAD( "ds_chr-2.bin",       0x40000, 0x20000, 0x5e05f4f9 )
	ROM_LOAD( "ds_chr-3.bin",       0x60000, 0x20000, 0xdc540791 )
	ROM_LOAD( "ds_chr-4.bin",       0x80000, 0x20000, 0xffd1f35c )
	ROM_LOAD( "ds_chr-5.bin",       0xa0000, 0x20000, 0x8472e0a3 )
	ROM_LOAD( "ds_chr-6.bin",       0xc0000, 0x20000, 0xa799665a )
	ROM_LOAD( "ds_chr-7.bin",       0xe0000, 0x20000, 0xa51724af )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ds_obj-0.bin",       0x00000, 0x20000, 0x03ec3076 )
	ROM_LOAD( "ds_obj-1.bin",       0x20000, 0x20000, 0xe67a8fa4 )
	ROM_LOAD( "ds_obj-2.bin",       0x40000, 0x20000, 0x061cd763 )
	ROM_LOAD( "ds_obj-3.bin",       0x60000, 0x20000, 0x63225a09 )
	ROM_LOAD_HS( "ds1_o4.bin",      0x80000, 0x10000, 0xa6246fcb )
ROM_END

/* Dragon Spirit (old version) */
ROM_START( dspirito )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "ds1_s0.bin",         0x0c000, 0x10000, 0x27100065 )
	ROM_LOAD( "ds1_s1.bin",         0x1c000, 0x10000, 0xb398645f )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "ds1_p7.bin",      0x00000, 0x10000, 0xf4c0d75e )
	ROM_LOAD_HS( "ds1_p6.bin",      0x20000, 0x10000, 0xa82737b4 )
	ROM_LOAD_HS( "ds1_p5.bin",      0x40000, 0x10000, 0x9a3a1028 )
	ROM_LOAD_HS( "ds1_p4.bin",      0x60000, 0x10000, 0xf3307870 )
	ROM_LOAD_HS( "ds1_p3.bin",      0x80000, 0x10000, 0xc6e5954b )
	ROM_LOAD_HS( "ds1_p2.bin",      0xa0000, 0x10000, 0x3c9b0100 )
	ROM_LOAD_HS( "ds1_p1.bin",      0xc0000, 0x10000, 0xf7e3298a )
	ROM_LOAD_HS( "ds1_p0.bin",      0xe0000, 0x10000, 0xb22a2856 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xb0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "ds1_v0.bin",      0x10000, 0x10000, 0x313b3508 )
	ROM_LOAD( "ds_voi-1.bin",       0x30000, 0x20000, 0x54790d4e )
	ROM_LOAD( "ds_voi-2.bin",       0x50000, 0x20000, 0x05298534 )
	ROM_LOAD( "ds_voi-3.bin",       0x70000, 0x20000, 0x13e84c7e )
	ROM_LOAD( "ds_voi-4.bin",       0x90000, 0x20000, 0x34fbb8cd )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "ds_chr-8.bin",       0x00000, 0x20000, 0x946eb242 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "ds_chr-0.bin",       0x00000, 0x20000, 0x7bf28ac3 )
	ROM_LOAD( "ds_chr-1.bin",       0x20000, 0x20000, 0x03582fea )
	ROM_LOAD( "ds_chr-2.bin",       0x40000, 0x20000, 0x5e05f4f9 )
	ROM_LOAD( "ds_chr-3.bin",       0x60000, 0x20000, 0xdc540791 )
	ROM_LOAD( "ds_chr-4.bin",       0x80000, 0x20000, 0xffd1f35c )
	ROM_LOAD( "ds_chr-5.bin",       0xa0000, 0x20000, 0x8472e0a3 )
	ROM_LOAD( "ds_chr-6.bin",       0xc0000, 0x20000, 0xa799665a )
	ROM_LOAD( "ds_chr-7.bin",       0xe0000, 0x20000, 0xa51724af )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ds_obj-0.bin",       0x00000, 0x20000, 0x03ec3076 )
	ROM_LOAD( "ds_obj-1.bin",       0x20000, 0x20000, 0xe67a8fa4 )
	ROM_LOAD( "ds_obj-2.bin",       0x40000, 0x20000, 0x061cd763 )
	ROM_LOAD( "ds_obj-3.bin",       0x60000, 0x20000, 0x63225a09 )
	ROM_LOAD_HS( "ds1_o4.bin",      0x80000, 0x10000, 0xa6246fcb )
ROM_END

/* Blazer */
ROM_START( blazer )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x1c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "bz1_s0.bin",         0x0c000, 0x10000, 0x6c3a580b )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "bz1_p7.bin",  0x00000, 0x10000, 0x2d4cbb95 )
	ROM_LOAD( "bz_prg-6.bin",       0x20000, 0x20000, 0x81c48fc0 )
	ROM_LOAD( "bz_prg-5.bin",       0x40000, 0x20000, 0x900da191 )
	ROM_LOAD( "bz_prg-4.bin",       0x60000, 0x20000, 0x65ef6f05 )
	ROM_LOAD_HS( "bz1_p3.bin",      0x80000, 0x10000, 0x81b32a1a )
	ROM_LOAD_HS( "bz1_p2.bin",      0xa0000, 0x10000, 0x5d700aed )
	ROM_LOAD_HS( "bz1_p1.bin",      0xc0000, 0x10000, 0xc54bbbf4 )
	ROM_LOAD_HS( "bz1_p0.bin",      0xe0000, 0x10000, 0xa7dd195b )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xb0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "bz1_v0.bin",      0x10000, 0x10000, 0x3d09d32e )
	ROM_LOAD( "bz_voi-1.bin",       0x30000, 0x20000, 0x2043b141 )
	ROM_LOAD( "bz_voi-2.bin",       0x50000, 0x20000, 0x64143442 )
	ROM_LOAD( "bz_voi-3.bin",       0x70000, 0x20000, 0x26cfc510 )
	ROM_LOAD( "bz_voi-4.bin",       0x90000, 0x20000, 0xd206b1bd )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "bz_chr-8.bin",       0x00000, 0x20000, 0xdb28bfca )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "bz_chr-0.bin",       0x00000, 0x20000, 0xd346ba61 )
	ROM_LOAD( "bz_chr-1.bin",       0x20000, 0x20000, 0xe45eb2ea )
	ROM_LOAD( "bz_chr-2.bin",       0x40000, 0x20000, 0x599079ee )
	ROM_LOAD( "bz_chr-3.bin",       0x60000, 0x20000, 0xd5182e36 )
	ROM_LOAD( "bz_chr-4.bin",       0x80000, 0x20000, 0xe788259e )
	ROM_LOAD( "bz_chr-5.bin",       0xa0000, 0x20000, 0x107e6814 )
	ROM_LOAD( "bz_chr-6.bin",       0xc0000, 0x20000, 0x0312e2ba )
	ROM_LOAD( "bz_chr-7.bin",       0xe0000, 0x20000, 0xd9d9a90f )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "bz_obj-0.bin",       0x00000, 0x20000, 0x22aee927 )
	ROM_LOAD( "bz_obj-1.bin",       0x20000, 0x20000, 0x7cb10112 )
	ROM_LOAD( "bz_obj-2.bin",       0x40000, 0x20000, 0x34b23bb7 )
	ROM_LOAD( "bz_obj-3.bin",       0x60000, 0x20000, 0x9bc1db71 )
	ROM_FILL(                       0x80000, 0x80000, 0xff )
ROM_END

/* Quester */
ROM_START( quester )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "qs1_s0.bin",         0x0c000, 0x10000, 0xc2ef3af9 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "qs1_p7b.bin",     0x00000, 0x10000, 0xf358a944 )
	ROM_LOAD( "qs1_p5.bin",         0x40000, 0x10000, 0xc8e11f30 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "qs1_v0.bin",      0x10000, 0x10000, 0x6a2f3038 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "qs1_c8.bin",         0x00000, 0x10000, 0x06730d54 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "qs1_c0.bin",         0x00000, 0x20000, 0xca69bd7a )
	ROM_LOAD( "qs1_c1.bin",         0x20000, 0x20000, 0xd660ba71 )
	ROM_LOAD( "qs1_c2.bin",         0x40000, 0x20000, 0x4686f656 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "qs1_o0.bin",         0x00000, 0x10000, 0xe24f0bf1 )
	ROM_LOAD( "qs1_o1.bin",         0x20000, 0x10000, 0xe4aab0ca )
ROM_END

/* Pac-Mania */
ROM_START( pacmania )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "pnx_s0.bin",         0x0c000, 0x10000, 0xc10370fa )
	ROM_LOAD( "pnx_s1.bin",         0x1c000, 0x10000, 0xf761ed5a )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "pnx_p7.bin",      0x00000, 0x10000, 0x462fa4fd )
	ROM_LOAD( "pn_prg-6.bin",       0x20000, 0x20000, 0xfe94900c )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "pnx_v0.bin",      0x10000, 0x10000, 0x1ad5788f )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "pn1_c8.bin",         0x00000, 0x10000, 0xf3afd65d )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "pn_chr-0.bin",       0x00000, 0x20000, 0x7c57644c )
	ROM_LOAD( "pn_chr-1.bin",       0x20000, 0x20000, 0x7eaa67ed )
	ROM_LOAD( "pn_chr-2.bin",       0x40000, 0x20000, 0x27e739ac )
	ROM_LOAD( "pn_chr-3.bin",       0x60000, 0x20000, 0x1dfda293 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "pn_obj-0.bin",       0x00000, 0x20000, 0xfda57e8b )
	ROM_LOAD( "pnx_obj1.bin",       0x20000, 0x20000, 0x4c08affe )
ROM_END

/* Pac-Mania (Japan) diff o1,s0,s1,p7,v0 */
ROM_START( pacmanij )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "pn1_s0.bin",         0x0c000, 0x10000, 0xd5ef5eee )
	ROM_LOAD( "pn1_s1.bin",         0x1c000, 0x10000, 0x411bc134 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "pn1_p7.bin",      0x00000, 0x10000, 0x2aa99e2b )
	ROM_LOAD( "pn_prg-6.bin",       0x20000, 0x20000, 0xfe94900c )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "pn1_v0.bin",      0x10000, 0x10000, 0xe2689f79 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "pn1_c8.bin",         0x00000, 0x10000, 0xf3afd65d )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "pn_chr-0.bin",       0x00000, 0x20000, 0x7c57644c )
	ROM_LOAD( "pn_chr-1.bin",       0x20000, 0x20000, 0x7eaa67ed )
	ROM_LOAD( "pn_chr-2.bin",       0x40000, 0x20000, 0x27e739ac )
	ROM_LOAD( "pn_chr-3.bin",       0x60000, 0x20000, 0x1dfda293 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "pn_obj-0.bin",       0x00000, 0x20000, 0xfda57e8b )
	ROM_LOAD( "pn_obj-1.bin",       0x20000, 0x20000, 0x27bdf440 )
ROM_END

/* Galaga '88 */
ROM_START( galaga88 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "g81_s0.bin",         0x0c000, 0x10000, 0x164a3fdc )
	ROM_LOAD( "g81_s1.bin",         0x1c000, 0x10000, 0x16a4b784 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "g8x_p7.bin",      0x00000, 0x10000, 0xdf75b7fc )
	ROM_LOAD_HS( "g8x_p6.bin",      0x20000, 0x10000, 0x7e3471d3 )
	ROM_LOAD_HS( "g81_p5.bin",      0x40000, 0x10000, 0x4fbd3f6c )
	ROM_LOAD_HS( "g81_p1.bin",      0xc0000, 0x10000, 0xe68cb351 )
	ROM_LOAD_HS( "g81_p0.bin",      0xe0000, 0x10000, 0x0f0778ca )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "g81_v0.bin",      0x10000, 0x10000, 0x86921dd4 )
	ROM_LOAD_HS( "g81_v1.bin",      0x30000, 0x10000, 0x9c300e16 )
	ROM_LOAD_HS( "g81_v2.bin",      0x50000, 0x10000, 0x5316b4b0 )
	ROM_LOAD_HS( "g81_v3.bin",      0x70000, 0x10000, 0xdc077af4 )
	ROM_LOAD_HS( "g81_v4.bin",      0x90000, 0x10000, 0xac0279a7 )
	ROM_LOAD_HS( "g81_v5.bin",      0xb0000, 0x10000, 0x014ddba1 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "g8_chr-8.bin",       0x00000, 0x20000, 0x3862ed0a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "g8_chr-0.bin",       0x00000, 0x20000, 0x68559c78 )
	ROM_LOAD( "g8_chr-1.bin",       0x20000, 0x20000, 0x3dc0f93f )
	ROM_LOAD( "g8_chr-2.bin",       0x40000, 0x20000, 0xdbf26f1f )
	ROM_LOAD( "g8_chr-3.bin",       0x60000, 0x20000, 0xf5d6cac5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "g8_obj-0.bin",       0x00000, 0x20000, 0xd7112e3f )
	ROM_LOAD( "g8_obj-1.bin",       0x20000, 0x20000, 0x680db8e7 )
	ROM_LOAD( "g8_obj-2.bin",       0x40000, 0x20000, 0x13c97512 )
	ROM_LOAD( "g8_obj-3.bin",       0x60000, 0x20000, 0x3ed3941b )
	ROM_LOAD( "g8_obj-4.bin",       0x80000, 0x20000, 0x370ff4ad )
	ROM_LOAD( "g8_obj-5.bin",       0xa0000, 0x20000, 0xb0645169 )
ROM_END

ROM_START( galag88b )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "g81_s0.bin",         0x0c000, 0x10000, 0x164a3fdc )
	ROM_LOAD( "g81_s1.bin",         0x1c000, 0x10000, 0x16a4b784 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "g8x_p7.bin",      0x00000, 0x10000, 0xdf75b7fc )
	ROM_LOAD_HS( "g8x_p6x.bin",     0x20000, 0x10000, 0x403d01c1 )
	ROM_LOAD_HS( "g81_p5.bin",      0x40000, 0x10000, 0x4fbd3f6c )
	ROM_LOAD_HS( "g81_p1.bin",      0xc0000, 0x10000, 0xe68cb351 )
	ROM_LOAD_HS( "g81_p0.bin",      0xe0000, 0x10000, 0x0f0778ca )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "g81_v0.bin",      0x10000, 0x10000, 0x86921dd4 )
	ROM_LOAD_HS( "g81_v1.bin",      0x30000, 0x10000, 0x9c300e16 )
	ROM_LOAD_HS( "g81_v2.bin",      0x50000, 0x10000, 0x5316b4b0 )
	ROM_LOAD_HS( "g81_v3.bin",      0x70000, 0x10000, 0xdc077af4 )
	ROM_LOAD_HS( "g81_v4.bin",      0x90000, 0x10000, 0xac0279a7 )
	ROM_LOAD_HS( "g81_v5.bin",      0xb0000, 0x10000, 0x014ddba1 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "g8_chr-8.bin",       0x00000, 0x20000, 0x3862ed0a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "g8_chr-0.bin",       0x00000, 0x20000, 0x68559c78 )
	ROM_LOAD( "g8_chr-1.bin",       0x20000, 0x20000, 0x3dc0f93f )
	ROM_LOAD( "g8_chr-2.bin",       0x40000, 0x20000, 0xdbf26f1f )
	ROM_LOAD( "g8_chr-3.bin",       0x60000, 0x20000, 0xf5d6cac5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "g8_obj-0.bin",       0x00000, 0x20000, 0xd7112e3f )
	ROM_LOAD( "g8_obj-1.bin",       0x20000, 0x20000, 0x680db8e7 )
	ROM_LOAD( "g8_obj-2.bin",       0x40000, 0x20000, 0x13c97512 )
	ROM_LOAD( "g8_obj-3.bin",       0x60000, 0x20000, 0x3ed3941b )
	ROM_LOAD( "g8_obj-4.bin",       0x80000, 0x20000, 0x370ff4ad )
	ROM_LOAD( "g8_obj-5.bin",       0xa0000, 0x20000, 0xb0645169 )
ROM_END

/* Galaga '88 (Japan) */
ROM_START( galag88j )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "g81_s0.bin",         0x0c000, 0x10000, 0x164a3fdc )
	ROM_LOAD( "g81_s1.bin",         0x1c000, 0x10000, 0x16a4b784 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "g81_p7.bin",      0x00000, 0x10000, 0x7c10965d )
	ROM_LOAD_HS( "g81_p6.bin",      0x20000, 0x10000, 0xe7203707 )
	ROM_LOAD_HS( "g81_p5.bin",      0x40000, 0x10000, 0x4fbd3f6c )
	ROM_LOAD_HS( "g81_p1.bin",      0xc0000, 0x10000, 0xe68cb351 )
	ROM_LOAD_HS( "g81_p0.bin",      0xe0000, 0x10000, 0x0f0778ca )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0xd0000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "g81_v0.bin",      0x10000, 0x10000, 0x86921dd4 )
	ROM_LOAD_HS( "g81_v1.bin",      0x30000, 0x10000, 0x9c300e16 )
	ROM_LOAD_HS( "g81_v2.bin",      0x50000, 0x10000, 0x5316b4b0 )
	ROM_LOAD_HS( "g81_v3.bin",      0x70000, 0x10000, 0xdc077af4 )
	ROM_LOAD_HS( "g81_v4.bin",      0x90000, 0x10000, 0xac0279a7 )
	ROM_LOAD_HS( "g81_v5.bin",      0xb0000, 0x10000, 0x014ddba1 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "g8_chr-8.bin",       0x00000, 0x20000, 0x3862ed0a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "g8_chr-0.bin",       0x00000, 0x20000, 0x68559c78 )
	ROM_LOAD( "g8_chr-1.bin",       0x20000, 0x20000, 0x3dc0f93f )
	ROM_LOAD( "g8_chr-2.bin",       0x40000, 0x20000, 0xdbf26f1f )
	ROM_LOAD( "g8_chr-3.bin",       0x60000, 0x20000, 0xf5d6cac5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "g8_obj-0.bin",       0x00000, 0x20000, 0xd7112e3f )
	ROM_LOAD( "g8_obj-1.bin",       0x20000, 0x20000, 0x680db8e7 )
	ROM_LOAD( "g8_obj-2.bin",       0x40000, 0x20000, 0x13c97512 )
	ROM_LOAD( "g8_obj-3.bin",       0x60000, 0x20000, 0x3ed3941b )
	ROM_LOAD( "g8_obj-4.bin",       0x80000, 0x20000, 0x370ff4ad )
	ROM_LOAD( "g8_obj-5.bin",       0xa0000, 0x20000, 0xb0645169 )
ROM_END

/* World Stadium */
ROM_START( ws )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "ws1_snd0.bin",       0x0c000, 0x10000, 0x45a87810 )
	ROM_LOAD( "ws1_snd1.bin",       0x1c000, 0x10000, 0x31bf74c1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "ws1_prg7.bin",    0x00000, 0x10000, 0x28712eba )
	ROM_LOAD_HS( "ws1_prg2.bin",    0xa0000, 0x10000, 0xbb09fa9b )
	ROM_LOAD_HS( "ws1_prg1.bin",    0xc0000, 0x10000, 0xdfd72bed )
	ROM_LOAD_HS( "ws1_prg0.bin",    0xe0000, 0x10000, 0xb0234298 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "ws1_voi0.bin",    0x10000, 0x10000, 0xf6949199 )
	ROM_LOAD( "ws_voi-1.bin",       0x30000, 0x20000, 0x210e2af9 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "ws_chr-8.bin",       0x00000, 0x20000, 0xd1897b9b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "ws_chr-0.bin",       0x00000, 0x20000, 0x3e3e96b4 )
	ROM_LOAD( "ws_chr-1.bin",       0x20000, 0x20000, 0x897dfbc1 )
	ROM_LOAD( "ws_chr-2.bin",       0x40000, 0x20000, 0xe142527c )
	ROM_LOAD( "ws_chr-3.bin",       0x60000, 0x20000, 0x907d4dc8 )
	ROM_LOAD( "ws_chr-4.bin",       0x80000, 0x20000, 0xafb11e17 )
	ROM_LOAD( "ws_chr-6.bin",       0xc0000, 0x20000, 0xa16a17c2 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ws_obj-0.bin",       0x00000, 0x20000, 0x12dc83a6 )
	ROM_LOAD( "ws_obj-1.bin",       0x20000, 0x20000, 0x68290a46 )
	ROM_LOAD( "ws_obj-2.bin",       0x40000, 0x20000, 0xcd5ba55d )
	ROM_LOAD_HS( "ws1_obj3.bin",    0x60000, 0x10000, 0xf2ed5309 )
ROM_END

/* Beraboh Man */
ROM_START( berabohm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "bm1_s0.bin",         0x0c000, 0x10000, 0xd5d53cb1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "bm1_p7c.bin",        0x10000, 0x10000, 0x9694d7b2 )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD_HS( "bm1-p6.bin",      0x20000, 0x10000, 0xa51b69a5 )
	ROM_LOAD( "bm1_p4.bin",         0x60000, 0x20000, 0xf6cfcb8c )
	ROM_LOAD( "bm1_p1.bin",         0xc0000, 0x20000, 0xb15f6407 )
	ROM_LOAD( "bm1_p0.bin",         0xe0000, 0x20000, 0xb57ff8c1 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x70000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "bm1_v0.bin",      0x10000, 0x10000, 0x4e40d0ca )
	ROM_LOAD(    "bm_voi-1.bin",    0x30000, 0x20000, 0xbe9ce0a8 )
	ROM_LOAD_HS( "bm1_v2.bin",      0x50000, 0x10000, 0x41225d04 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "bm_chr-8.bin",       0x00000, 0x20000, 0x92860e95 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "bm_chr-0.bin",       0x00000, 0x20000, 0xeda1d92e )
	ROM_LOAD( "bm_chr-1.bin",       0x20000, 0x20000, 0x8ae1891e )
	ROM_LOAD( "bm_chr-2.bin",       0x40000, 0x20000, 0x774cdf4e )
	ROM_LOAD( "bm_chr-3.bin",       0x60000, 0x20000, 0x6d81e6c9 )
	ROM_LOAD( "bm_chr-4.bin",       0x80000, 0x20000, 0xf4597683 )
	ROM_LOAD( "bm_chr-5.bin",       0xa0000, 0x20000, 0x0e0abde0 )
	ROM_LOAD( "bm_chr-6.bin",       0xc0000, 0x20000, 0x4a61f08c )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "bm_obj-0.bin",       0x00000, 0x20000, 0x15724b94 )
	ROM_LOAD( "bm_obj-1.bin",       0x20000, 0x20000, 0x5d21f962 )
	ROM_LOAD( "bm_obj-2.bin",       0x40000, 0x20000, 0x5d48e924 )
	ROM_LOAD( "bm_obj-3.bin",       0x60000, 0x20000, 0xcbe56b7f )
	ROM_LOAD( "bm_obj-4.bin",       0x80000, 0x20000, 0x76dcc24c )
	ROM_LOAD( "bm_obj-5.bin",       0xa0000, 0x20000, 0xfe70201d )
	ROM_LOAD( "bm_obj-7.bin",       0xe0000, 0x20000, 0x377c81ed )
ROM_END

ROM_START( beraboho )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "bm1_s0.bin",         0x0c000, 0x10000, 0xd5d53cb1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "bm1_p7b.bin",        0x10000, 0x10000, 0xe0c36ddd )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD_HS( "bm1-p6.bin",      0x20000, 0x10000, 0xa51b69a5 )
	ROM_LOAD( "bm1_p4.bin",         0x60000, 0x20000, 0xf6cfcb8c )
	ROM_LOAD( "bm1_p1.bin",         0xc0000, 0x20000, 0xb15f6407 )
	ROM_LOAD( "bm1_p0.bin",         0xe0000, 0x20000, 0xb57ff8c1 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x70000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "bm1_v0.bin",      0x10000, 0x10000, 0x4e40d0ca )
	ROM_LOAD(    "bm_voi-1.bin",    0x30000, 0x20000, 0xbe9ce0a8 )
	ROM_LOAD_HS( "bm1_v2.bin",      0x50000, 0x10000, 0x41225d04 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "bm_chr-8.bin",       0x00000, 0x20000, 0x92860e95 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "bm_chr-0.bin",       0x00000, 0x20000, 0xeda1d92e )
	ROM_LOAD( "bm_chr-1.bin",       0x20000, 0x20000, 0x8ae1891e )
	ROM_LOAD( "bm_chr-2.bin",       0x40000, 0x20000, 0x774cdf4e )
	ROM_LOAD( "bm_chr-3.bin",       0x60000, 0x20000, 0x6d81e6c9 )
	ROM_LOAD( "bm_chr-4.bin",       0x80000, 0x20000, 0xf4597683 )
	ROM_LOAD( "bm_chr-5.bin",       0xa0000, 0x20000, 0x0e0abde0 )
	ROM_LOAD( "bm_chr-6.bin",       0xc0000, 0x20000, 0x4a61f08c )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "bm_obj-0.bin",       0x00000, 0x20000, 0x15724b94 )
	ROM_LOAD( "bm_obj-1.bin",       0x20000, 0x20000, 0x5d21f962 )
	ROM_LOAD( "bm_obj-2.bin",       0x40000, 0x20000, 0x5d48e924 )
	ROM_LOAD( "bm_obj-3.bin",       0x60000, 0x20000, 0xcbe56b7f )
	ROM_LOAD( "bm_obj-4.bin",       0x80000, 0x20000, 0x76dcc24c )
	ROM_LOAD( "bm_obj-5.bin",       0xa0000, 0x20000, 0xfe70201d )
	ROM_LOAD( "bm_obj-7.bin",       0xe0000, 0x20000, 0x377c81ed )
ROM_END

/* Marchen Maze */
ROM_START( mmaze )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "mm_snd-0.bin",       0x0c000, 0x10000, 0x25d25e07 )
	ROM_LOAD( "mm_snd-1.bin",       0x1c000, 0x10000, 0x2c5849c8 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "mm1_p7.bin",      0x00000, 0x10000, 0x085e58cc )
	ROM_LOAD_HS( "mm1_p6.bin",      0x20000, 0x10000, 0xeaf530d8 )
	ROM_LOAD( "mm_prg-2.bin",       0xa0000, 0x20000, 0x91bde09f )
	ROM_LOAD( "mm_prg-1.bin",       0xc0000, 0x20000, 0x6ba14e41 )
	ROM_LOAD( "mm_prg-0.bin",       0xe0000, 0x20000, 0xe169a911 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "mm_voi-0.bin",       0x20000, 0x10000, 0xee974cff )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "mm_voi-1.bin",       0x30000, 0x20000, 0xd09b5830 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "mm_chr-8.bin",       0x00000, 0x20000, 0xa3784dfe )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "mm_chr-0.bin",       0x00000, 0x20000, 0x43ff2dfc )
	ROM_LOAD( "mm_chr-1.bin",       0x20000, 0x20000, 0xb9b4b72d )
	ROM_LOAD( "mm_chr-2.bin",       0x40000, 0x20000, 0xbee28425 )
	ROM_LOAD( "mm_chr-3.bin",       0x60000, 0x20000, 0xd9f41e5c )
	ROM_LOAD( "mm_chr-4.bin",       0x80000, 0x20000, 0x3484f4ae )
	ROM_LOAD( "mm_chr-5.bin",       0xa0000, 0x20000, 0xc863deba )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "mm_obj-0.bin",       0x00000, 0x20000, 0xd4b7e698 )
	ROM_LOAD( "mm_obj-1.bin",       0x20000, 0x20000, 0x1ce49e04 )
	ROM_LOAD( "mm_obj-2.bin",       0x40000, 0x20000, 0x3d3d5de3 )
	ROM_LOAD( "mm_obj-3.bin",       0x60000, 0x20000, 0xdac57358 )
ROM_END

/* Bakutotsu Kijuutei */
ROM_START( bakutotu )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "bk1_s0.bin",         0x0c000, 0x10000, 0xc35d7df6 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "bk1_prg7.bin",       0x10000, 0x10000, 0xfac1c1bf )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD(    "bk1_p6.bin",      0x20000, 0x20000, 0x57a3ce42 )
	ROM_LOAD_HS( "bk1_p5.bin",      0x40000, 0x10000, 0xdceed7cb )
	ROM_LOAD_HS( "bk1_p4.bin",      0x60000, 0x10000, 0x96446d48 )
	ROM_LOAD(    "bk1_p3.bin",      0x80000, 0x20000, 0xe608234f )
	ROM_LOAD_HS( "bk1_p2.bin",      0xa0000, 0x10000, 0x7a686daa )
	ROM_LOAD_HS( "bk1_p1.bin",      0xc0000, 0x10000, 0xd389d6d4 )
	ROM_LOAD(    "bk_prg-0.bin",    0xe0000, 0x20000, 0x4529c362 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "bk1_v0.bin",      0x10000, 0x10000, 0x008e290e )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "bk_chr-8.bin",       0x00000, 0x20000, 0x6c8d4029 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "bk_chr-0.bin",       0x00000, 0x20000, 0x4e011058 )
	ROM_LOAD( "bk_chr-1.bin",       0x20000, 0x20000, 0x496fcb9b )
	ROM_LOAD( "bk_chr-2.bin",       0x40000, 0x20000, 0xdc812e28 )
	ROM_LOAD( "bk_chr-3.bin",       0x60000, 0x20000, 0x2b6120f4 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "bk_obj-0.bin",       0x00000, 0x20000, 0x88c627c1 )

	// obj1 and 2 may not exist on the real board
	ROM_LOAD( "bk_obj-3.bin",       0x20000, 0x20000, 0xf7d1909a ) // dummy mirroring obj3
	ROM_LOAD( "bk_obj-4.bin",       0x40000, 0x20000, 0x27ed1441 ) // dummy mirroring obj4

	ROM_LOAD( "bk_obj-3.bin",       0x60000, 0x20000, 0xf7d1909a )
	ROM_LOAD( "bk_obj-4.bin",       0x80000, 0x20000, 0x27ed1441 )
	ROM_LOAD( "bk_obj-5.bin",       0xa0000, 0x20000, 0x790560c0 )
	ROM_LOAD( "bk_obj-6.bin",       0xc0000, 0x20000, 0x2cd4d2ea )
	ROM_LOAD( "bk_obj-7.bin",       0xe0000, 0x20000, 0x809aa0e6 )
ROM_END

/* World Court */
ROM_START( wldcourt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "wc1_snd0.bin",       0x0c000, 0x10000, 0x17a6505d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "wc1_prg7.bin",    0x00000, 0x10000, 0x8a7c6cac )
	ROM_LOAD_HS( "wc1_prg6.bin",    0x20000, 0x10000, 0xe9216b9e )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "wc1_voi0.bin",    0x10000, 0x10000, 0xb57919f7 )
	ROM_LOAD( "wc1_voi1.bin",       0x30000, 0x20000, 0x97974b4b )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "wc1_chr8.bin",       0x00000, 0x20000, 0x23e1c399 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "wc1_chr0.bin",       0x00000, 0x20000, 0x9fb07b9b )
	ROM_LOAD( "wc1_chr1.bin",       0x20000, 0x20000, 0x01bfbf60 )
	ROM_LOAD( "wc1_chr2.bin",       0x40000, 0x20000, 0x7e8acf45 )
	ROM_LOAD( "wc1_chr3.bin",       0x60000, 0x20000, 0x924e9c81 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "wc1_obj0.bin",       0x00000, 0x20000, 0x70d562f8 )
	ROM_LOAD( "wc1_obj1.bin",       0x20000, 0x20000, 0xba8b034a )
	ROM_LOAD( "wc1_obj2.bin",       0x40000, 0x20000, 0xc2bd5f0f )
	ROM_LOAD( "wc1_obj3.bin",       0x60000, 0x10000, 0x1aa2dbc8 )
ROM_END

/* Splatter House */
ROM_START( splatter )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "sh1_snd0.bin",       0x0c000, 0x10000, 0x90abd4ad )
	ROM_LOAD( "sh1_snd1.bin",       0x1c000, 0x10000, 0x8ece9e0a )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "sh1_prg7.bin",    0x00000, 0x10000, 0x24c8cbd7 )
	ROM_LOAD_HS( "sh1_prg6.bin",    0x20000, 0x10000, 0x97a3e664 )
	ROM_LOAD_HS( "sh1_prg5.bin",    0x40000, 0x10000, 0x0187de9a )
	ROM_LOAD_HS( "sh1_prg4.bin",    0x60000, 0x10000, 0x350dee5b )
	ROM_LOAD_HS( "sh1_prg3.bin",    0x80000, 0x10000, 0x955ce93f )
	ROM_LOAD_HS( "sh1_prg2.bin",    0xa0000, 0x10000, 0x434dbe7d )
	ROM_LOAD_HS( "sh1_prg1.bin",    0xc0000, 0x10000, 0x7a3efe09 )
	ROM_LOAD_HS( "sh1_prg0.bin",    0xe0000, 0x10000, 0x4e07e6d9 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x90000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "sh_voi-0.bin",       0x20000, 0x10000, 0x2199cb66 )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "sh_voi-1.bin",       0x30000, 0x20000, 0x9b6472af )
	ROM_LOAD( "sh_voi-2.bin",       0x50000, 0x20000, 0x25ea75b6 )
	ROM_LOAD( "sh_voi-3.bin",       0x70000, 0x20000, 0x5eebcdb4 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "sh_chr-8.bin",       0x00000, 0x20000, 0x321f483b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "sh_chr-0.bin",       0x00000, 0x20000, 0x4dd2ef05 )
	ROM_LOAD( "sh_chr-1.bin",       0x20000, 0x20000, 0x7a764999 )
	ROM_LOAD( "sh_chr-2.bin",       0x40000, 0x20000, 0x6e6526ee )
	ROM_LOAD( "sh_chr-3.bin",       0x60000, 0x20000, 0x8d05abdb )
	ROM_LOAD( "sh_chr-4.bin",       0x80000, 0x20000, 0x1e1f8488 )
	ROM_LOAD( "sh_chr-5.bin",       0xa0000, 0x20000, 0x684cf554 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "sh_obj-0.bin",       0x00000, 0x20000, 0x1cedbbae )
	ROM_LOAD( "sh_obj-1.bin",       0x20000, 0x20000, 0xe56e91ee )
	ROM_LOAD( "sh_obj-2.bin",       0x40000, 0x20000, 0x3dfb0230 )
	ROM_LOAD( "sh_obj-3.bin",       0x60000, 0x20000, 0xe4e5a581 )
	ROM_LOAD( "sh_obj-4.bin",       0x80000, 0x20000, 0xb2422182 )
	ROM_LOAD( "sh_obj-5.bin",       0xa0000, 0x20000, 0x24d0266f )
	ROM_LOAD( "sh_obj-6.bin",       0xc0000, 0x20000, 0x80830b0e )
	ROM_LOAD( "sh_obj-7.bin",       0xe0000, 0x20000, 0x08b1953a )
ROM_END

/* Face Off */
ROM_START( faceoff )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "fo1_s0.bin",         0x0c000, 0x10000, 0x9a00d97d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "fo1_p7.bin",      0x00000, 0x10000, 0x6791d221 )
	ROM_LOAD_HS( "fo1_p6.bin",      0x20000, 0x10000, 0xa48ee82b )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "fo1_v0.bin",      0x10000, 0x10000, 0xe6edf63e )
	ROM_LOAD_HS( "fo1_v1.bin",      0x30000, 0x10000, 0x132a5d90 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "fo1_c8.bin",         0x00000, 0x10000, 0xd397216c )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "fo1_c0.bin",         0x00000, 0x20000, 0x27884ac0 )
	ROM_LOAD( "fo1_c1.bin",         0x20000, 0x20000, 0x4d423499 )
	ROM_LOAD( "fo1_c2.bin",         0x40000, 0x20000, 0xd62d86f1 )
	ROM_LOAD( "fo1_c3.bin",         0x60000, 0x20000, 0xc2a08694 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "fo1_o0.bin",         0x00000, 0x20000, 0x41af669d )
	ROM_LOAD( "fo1_o1.bin",         0x20000, 0x20000, 0xad5fbaa7 )
	ROM_LOAD( "fo1_o2.bin",         0x40000, 0x20000, 0xc1f7eb52 )
	ROM_LOAD( "fo1_o3.bin",         0x60000, 0x20000, 0xaa95d2e0 )
	ROM_LOAD( "fo1_o4.bin",         0x80000, 0x20000, 0x985f04c7 )
ROM_END

/* Rompers */
ROM_START( rompers )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "rp1_snd0.bin",       0x0c000, 0x10000, 0xc7c8d649 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "rp1prg7b.bin",    0x00000, 0x10000, 0x49d057e2 )
	ROM_LOAD_HS( "rp1prg6b.bin",    0x20000, 0x10000, 0x80821065 )
	ROM_LOAD_HS( "rp1_prg5.bin",    0x40000, 0x10000, 0x98bd4133 )
	ROM_LOAD_HS( "rp1_prg4.bin",    0x60000, 0x10000, 0x0918f06d )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "rp_voi-0.bin",       0x20000, 0x10000, 0x11caef7e )
	ROM_CONTINUE(                   0x10000, 0x10000 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "rp1_chr8.bin",       0x00000, 0x10000, 0x69cfe46a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "rp_chr-0.bin",       0x00000, 0x20000, 0x41b10ef3 )
	ROM_LOAD( "rp_chr-1.bin",       0x20000, 0x20000, 0xc18cd24e )
	ROM_LOAD( "rp_chr-2.bin",       0x40000, 0x20000, 0x6c9a3c79 )
	ROM_LOAD( "rp_chr-3.bin",       0x60000, 0x20000, 0x473aa788 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "rp_obj-0.bin",       0x00000, 0x20000, 0x1dcbf8bb )
	ROM_LOAD( "rp_obj-1.bin",       0x20000, 0x20000, 0xcb98e273 )
	ROM_LOAD( "rp_obj-2.bin",       0x40000, 0x20000, 0x6ebd191e )
	ROM_LOAD( "rp_obj-3.bin",       0x60000, 0x20000, 0x7c9828a1 )
	ROM_LOAD( "rp_obj-4.bin",       0x80000, 0x20000, 0x0348220b )
	ROM_LOAD( "rp1_obj5.bin",       0xa0000, 0x10000, 0x9e2ba243 )
	ROM_LOAD( "rp1_obj6.bin",       0xc0000, 0x10000, 0x6bf2aca6 )
ROM_END

/* Rompers (old version) */
ROM_START( romperso )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "rp1_snd0.bin",       0x0c000, 0x10000, 0xc7c8d649 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "rp1_prg7.bin",    0x00000, 0x10000, 0x8d49f28a )
	ROM_LOAD_HS( "rp1_prg6.bin",    0x20000, 0x10000, 0xfc183345 )
	ROM_LOAD_HS( "rp1_prg5.bin",    0x40000, 0x10000, 0x98bd4133 )
	ROM_LOAD_HS( "rp1_prg4.bin",    0x60000, 0x10000, 0x0918f06d )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "rp_voi-0.bin",       0x20000, 0x10000, 0x11caef7e )
	ROM_CONTINUE(                   0x10000, 0x10000 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "rp1_chr8.bin",       0x00000, 0x10000, 0x69cfe46a )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "rp_chr-0.bin",       0x00000, 0x20000, 0x41b10ef3 )
	ROM_LOAD( "rp_chr-1.bin",       0x20000, 0x20000, 0xc18cd24e )
	ROM_LOAD( "rp_chr-2.bin",       0x40000, 0x20000, 0x6c9a3c79 )
	ROM_LOAD( "rp_chr-3.bin",       0x60000, 0x20000, 0x473aa788 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "rp_obj-0.bin",       0x00000, 0x20000, 0x1dcbf8bb )
	ROM_LOAD( "rp_obj-1.bin",       0x20000, 0x20000, 0xcb98e273 )
	ROM_LOAD( "rp_obj-2.bin",       0x40000, 0x20000, 0x6ebd191e )
	ROM_LOAD( "rp_obj-3.bin",       0x60000, 0x20000, 0x7c9828a1 )
	ROM_LOAD( "rp_obj-4.bin",       0x80000, 0x20000, 0x0348220b )
	ROM_LOAD( "rp1_obj5.bin",       0xa0000, 0x10000, 0x9e2ba243 )
	ROM_LOAD( "rp1_obj6.bin",       0xc0000, 0x10000, 0x6bf2aca6 )
ROM_END

/* Blast Off */
ROM_START( blastoff )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "bo1-snd0.bin",       0x0c000, 0x10000, 0x2ecab76e )
	ROM_LOAD( "bo1-snd1.bin",       0x1c000, 0x10000, 0x048a6af1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "bo1prg7b.bin",       0x10000, 0x10000, 0xb630383c )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD( "bo1_prg6.bin",       0x20000, 0x20000, 0xd60da63e )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x70000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "bo_voi-0.bin",       0x20000, 0x10000, 0x47065e18 )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "bo_voi-1.bin",       0x30000, 0x20000, 0x0308b18e )
	ROM_LOAD( "bo_voi-2.bin",       0x50000, 0x20000, 0x88cab230 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "bo_chr-8.bin",       0x00000, 0x20000, 0xe8b5f2d4 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "bo_chr-0.bin",       0x00000, 0x20000, 0xbdc0afb5 )
	ROM_LOAD( "bo_chr-1.bin",       0x20000, 0x20000, 0x963d2639 )
	ROM_LOAD( "bo_chr-2.bin",       0x40000, 0x20000, 0xacdb6894 )
	ROM_LOAD( "bo_chr-3.bin",       0x60000, 0x20000, 0x214ec47f )
	ROM_LOAD( "bo_chr-4.bin",       0x80000, 0x20000, 0x08397583 )
	ROM_LOAD( "bo_chr-5.bin",       0xa0000, 0x20000, 0x20402429 )
	ROM_LOAD( "bo_chr-7.bin",       0xe0000, 0x20000, 0x4c5c4603 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "bo_obj-0.bin",       0x00000, 0x20000, 0xb3308ae7 )
	ROM_LOAD( "bo_obj-1.bin",       0x20000, 0x20000, 0xc9c93c47 )
	ROM_LOAD( "bo_obj-2.bin",       0x40000, 0x20000, 0xeef77527 )
	ROM_LOAD( "bo_obj-3.bin",       0x60000, 0x20000, 0xe3d9ed58 )
	ROM_LOAD( "bo1_obj4.bin",       0x80000, 0x20000, 0xc2c1b9cb )
ROM_END

/* World Stadium '89 */
ROM_START( ws89 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "w91_snd0.bin",       0x0c000, 0x10000, 0x52b84d5a )
	ROM_LOAD( "ws1_snd1.bin",       0x1c000, 0x10000, 0x31bf74c1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "w91_prg7.bin",    0x00000, 0x10000, 0x611ed964 )
	ROM_LOAD_HS( "w91_prg2.bin",    0xa0000, 0x10000, 0x522e5441 )
	ROM_LOAD_HS( "w91_prg1.bin",    0xc0000, 0x10000, 0x7ad8768f )
	ROM_LOAD_HS( "ws1_prg0.bin",    0xe0000, 0x10000, 0xb0234298 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "ws1_voi0.bin",    0x10000, 0x10000, 0xf6949199 )
	ROM_LOAD( "ws_voi-1.bin",       0x30000, 0x20000, 0x210e2af9 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "ws_chr-8.bin",       0x00000, 0x20000, 0xd1897b9b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "ws_chr-0.bin",       0x00000, 0x20000, 0x3e3e96b4 )
	ROM_LOAD( "ws_chr-1.bin",       0x20000, 0x20000, 0x897dfbc1 )
	ROM_LOAD( "ws_chr-2.bin",       0x40000, 0x20000, 0xe142527c )
	ROM_LOAD( "ws_chr-3.bin",       0x60000, 0x20000, 0x907d4dc8 )
	ROM_LOAD( "ws_chr-4.bin",       0x80000, 0x20000, 0xafb11e17 )
	ROM_LOAD( "ws_chr-6.bin",       0xc0000, 0x20000, 0xa16a17c2 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ws_obj-0.bin",       0x00000, 0x20000, 0x12dc83a6 )
	ROM_LOAD( "ws_obj-1.bin",       0x20000, 0x20000, 0x68290a46 )
	ROM_LOAD( "ws_obj-2.bin",       0x40000, 0x20000, 0xcd5ba55d )
	ROM_LOAD_HS( "w91_obj3.bin",    0x60000, 0x10000, 0x8ee76105 )
ROM_END

/* Dangerous Seed */
ROM_START( dangseed )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "dr1_snd0.bin",       0x0c000, 0x20000, 0xbcbbb21d )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "dr1_prg7.bin",       0x10000, 0x10000, 0xd7d2f653 )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD_HS( "dr1_prg6.bin",    0x20000, 0x10000, 0xcc68262b )
	ROM_LOAD( "dr_prg-5.bin",       0x40000, 0x20000, 0x7986bbdd )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "dr_voi-0.bin",       0x20000, 0x10000, 0xde4fdc0e )
	ROM_CONTINUE(                   0x10000, 0x10000 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "dr_chr-8.bin",       0x00000, 0x20000, 0x0fbaa10e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "dr_chr-0.bin",       0x00000, 0x20000, 0x419bacc7 )
	ROM_LOAD( "dr_chr-1.bin",       0x20000, 0x20000, 0x55ce77e1 )
	ROM_LOAD( "dr_chr-2.bin",       0x40000, 0x20000, 0x6f913419 )
	ROM_LOAD( "dr_chr-3.bin",       0x60000, 0x20000, 0xfe1f1a25 )
	ROM_LOAD( "dr_chr-4.bin",       0x80000, 0x20000, 0xc34471bc )
	ROM_LOAD( "dr_chr-5.bin",       0xa0000, 0x20000, 0x715c0720 )
	ROM_LOAD( "dr_chr-6.bin",       0xc0000, 0x20000, 0x5c1b71fa )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "dr_obj-0.bin",       0x00000, 0x20000, 0xabb95644 )
	ROM_LOAD( "dr_obj-1.bin",       0x20000, 0x20000, 0x24d6db51 )
	ROM_LOAD( "dr_obj-2.bin",       0x40000, 0x20000, 0x7e3a78c0 )
ROM_END

/* World Stadium '90 */
ROM_START( ws90 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "w91_snd0.bin",       0x0c000, 0x10000, 0x52b84d5a )
	ROM_LOAD( "ws1_snd1.bin",       0x1c000, 0x10000, 0x31bf74c1 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "w901prg7.bin",    0x00000, 0x10000, 0x37ae1b25 )
	ROM_LOAD_HS( "w901prg2.bin",    0xa0000, 0x10000, 0xb9e98e2f )
	ROM_LOAD_HS( "w91_prg1.bin",    0xc0000, 0x10000, 0x7ad8768f )
	ROM_LOAD_HS( "ws1_prg0.bin",    0xe0000, 0x10000, 0xb0234298 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "ws1_voi0.bin",    0x10000, 0x10000, 0xf6949199 )
	ROM_LOAD( "ws_voi-1.bin",       0x30000, 0x20000, 0x210e2af9 )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "ws_chr-8.bin",       0x00000, 0x20000, 0xd1897b9b )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "ws_chr-0.bin",       0x00000, 0x20000, 0x3e3e96b4 )
	ROM_LOAD( "ws_chr-1.bin",       0x20000, 0x20000, 0x897dfbc1 )
	ROM_LOAD( "ws_chr-2.bin",       0x40000, 0x20000, 0xe142527c )
	ROM_LOAD( "ws_chr-3.bin",       0x60000, 0x20000, 0x907d4dc8 )
	ROM_LOAD( "ws_chr-4.bin",       0x80000, 0x20000, 0xafb11e17 )
	ROM_LOAD( "ws_chr-6.bin",       0xc0000, 0x20000, 0xa16a17c2 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "ws_obj-0.bin",       0x00000, 0x20000, 0x12dc83a6 )
	ROM_LOAD( "ws_obj-1.bin",       0x20000, 0x20000, 0x68290a46 )
	ROM_LOAD( "ws_obj-2.bin",       0x40000, 0x20000, 0xcd5ba55d )
	ROM_LOAD_HS( "w901obj3.bin",    0x60000, 0x10000, 0x7d0b8961 )
ROM_END

/* Pistol Daimyo no Bouken */
ROM_START( pistoldm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "pd1_snd0.bin",       0x0c000, 0x20000, 0x026da54e )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "pd1prg7b.bin",       0x10000, 0x10000, 0x7189b797 )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD( "pd1_prg0.bin",       0xe0000, 0x20000, 0x9db9b89c )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x70000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "pd_voi-0.bin",       0x20000, 0x10000, 0xad1b8128 )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "pd_voi-1.bin",       0x30000, 0x20000, 0x2871c494 )
	ROM_LOAD( "pd_voi-2.bin",       0x50000, 0x20000, 0xe783f0c4 )


	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "pd_chr-8.bin",       0x00000, 0x20000, 0xa5f516db )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "pd_chr-0.bin",       0x00000, 0x20000, 0xadbbaf5c )
	ROM_LOAD( "pd_chr-1.bin",       0x20000, 0x20000, 0xb4e4f554 )
	ROM_LOAD( "pd_chr-2.bin",       0x40000, 0x20000, 0x84592540 )
	ROM_LOAD( "pd_chr-3.bin",       0x60000, 0x20000, 0x450bdaa9 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "pd_obj-0.bin",       0x00000, 0x20000, 0x7269821d )
	ROM_LOAD( "pd_obj-1.bin",       0x20000, 0x20000, 0x4f9738e5 )
	ROM_LOAD( "pd_obj-2.bin",       0x40000, 0x20000, 0x33208776 )
	ROM_LOAD( "pd_obj-3.bin",       0x60000, 0x20000, 0x0dbd54ef )
	ROM_LOAD( "pd_obj-4.bin",       0x80000, 0x20000, 0x58e838e2 )
	ROM_LOAD( "pd_obj-5.bin",       0xa0000, 0x20000, 0x414f9a9d )
	ROM_LOAD( "pd_obj-6.bin",       0xc0000, 0x20000, 0x91b4e6e0 )
	ROM_LOAD( "pd_obj-7.bin",       0xe0000, 0x20000, 0x00d4a8f0 )
ROM_END

/* Souko Ban Deluxe */
ROM_START( boxyboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "sb1_snd0.bin",       0x0c000, 0x10000, 0xbf46a106 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "sbx_prg7.bin",    0x00000, 0x10000, 0x7787c72e )
	ROM_LOAD( "sb1_prg1.bin",       0xc0000, 0x20000, 0x5d1fdd94 )
	ROM_LOAD( "sb1_prg0.bin",       0xe0000, 0x20000, 0x8af8cb73 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "sb1_voi0.bin",    0x10000, 0x10000, 0x63d9cedf )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "sb1_chr8.bin",       0x00000, 0x10000, 0x5692b297 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "sb1_chr0.bin",       0x00000, 0x20000, 0x267f1331 )
	ROM_LOAD( "sb1_chr1.bin",       0x20000, 0x20000, 0xe5ff61ad )
	ROM_LOAD( "sb1_chr2.bin",       0x40000, 0x20000, 0x099b746b )
	ROM_LOAD( "sb1_chr3.bin",       0x60000, 0x20000, 0x1551bb7c )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "sb1_obj0.bin",       0x00000, 0x10000, 0xed810da4 )
ROM_END

ROM_START( soukobdx )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "sb1_snd0.bin",       0x0c000, 0x10000, 0xbf46a106 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD_HS( "sb1_prg7.bin",    0x00000, 0x10000, 0xc3bd418a )
	ROM_LOAD( "sb1_prg1.bin",       0xc0000, 0x20000, 0x5d1fdd94 )
	ROM_LOAD( "sb1_prg0.bin",       0xe0000, 0x20000, 0x8af8cb73 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD_HS( "sb1_voi0.bin",    0x10000, 0x10000, 0x63d9cedf )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "sb1_chr8.bin",       0x00000, 0x10000, 0x5692b297 )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "sb1_chr0.bin",       0x00000, 0x20000, 0x267f1331 )
	ROM_LOAD( "sb1_chr1.bin",       0x20000, 0x20000, 0xe5ff61ad )
	ROM_LOAD( "sb1_chr2.bin",       0x40000, 0x20000, 0x099b746b )
	ROM_LOAD( "sb1_chr3.bin",       0x60000, 0x20000, 0x1551bb7c )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "sb1_obj0.bin",       0x00000, 0x10000, 0xed810da4 )
ROM_END

/* Puzzle Club */
ROM_START( puzlclub )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "pc1_s0.bin",         0x0c000, 0x10000, 0x44737c02 )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "pc1_p7.bin",         0x10000, 0x10000, 0xf0638260 )
	ROM_LOAD( "pc1_p1.bin",         0xc0000, 0x10000, 0xdfd9108a )
	ROM_LOAD( "pc1_p0.bin",         0xe0000, 0x10000, 0x2db477c8 )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x30000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	/* no voices */

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "pc1-c8.bin",         0x00000, 0x20000, 0x4e196bcd )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "pc1-c0.bin",         0x00000, 0x20000, 0xad7b134e )
	ROM_LOAD( "pc1-c1.bin",         0x20000, 0x20000, 0x10cb3207 )
	ROM_LOAD( "pc1-c2.bin",         0x40000, 0x20000, 0xd98d2c8f )
	ROM_LOAD( "pc1-c3.bin",         0x60000, 0x20000, 0x91a61d96 )
	ROM_LOAD( "pc1-c4.bin",         0x80000, 0x20000, 0xf1c95296 )
	ROM_LOAD( "pc1-c5.bin",         0xa0000, 0x20000, 0xbc443c27 )
	ROM_LOAD( "pc1-c6.bin",         0xc0000, 0x20000, 0xec0a3dc5 )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	/* no sprites */
ROM_END

/* Tank Force */
ROM_START( tankfrce )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "tf1_snd0.bin",       0x0c000, 0x20000, 0x4d9cf7aa )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "tf1prg7.bin",        0x10000, 0x10000, 0x2ec28a87 )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD( "tf1_prg1.bin",       0xc0000, 0x20000, 0x4a8bb251 )
	ROM_LOAD( "tf1_prg0.bin",       0xe0000, 0x20000, 0x2ae4b9eb )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "tf1_voi0.bin",       0x20000, 0x10000, 0xf542676a )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "tf1_voi1.bin",       0x30000, 0x20000, 0x615d09cd )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "tf1_chr8.bin",       0x00000, 0x20000, 0x7d53b31e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "tf1_chr0.bin",       0x00000, 0x20000, 0x9e91794e )
	ROM_LOAD( "tf1_chr1.bin",       0x20000, 0x20000, 0x76e1bc56 )
	ROM_LOAD( "tf1_chr2.bin",       0x40000, 0x20000, 0xfcb645d9 )
	ROM_LOAD( "tf1_chr3.bin",       0x60000, 0x20000, 0xa8dbf080 )
	ROM_LOAD( "tf1_chr4.bin",       0x80000, 0x20000, 0x51fedc8c )
	ROM_LOAD( "tf1_chr5.bin",       0xa0000, 0x20000, 0xe6c6609a )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "tf1_obj0.bin",       0x00000, 0x20000, 0x4bedd51a )
	ROM_LOAD( "tf1_obj1.bin",       0x20000, 0x20000, 0xdf674d6d )
ROM_END

/* Tank Force (Japan) */
ROM_START( tankfrcj )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for the main cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )       /* 64k for the sub cpu */
	/* Nothing loaded here. Bankswitching makes sure this gets the necessary code */

	ROM_REGION( 0x2c000, REGION_CPU3, 0 )       /* 176k for the sound cpu */
	ROM_LOAD( "tf1_snd0.bin",       0x0c000, 0x20000, 0x4d9cf7aa )

	ROM_REGION( 0x100000, REGION_USER1, 0 ) /* 1M for ROMs */
	ROM_LOAD( "tf1_prg7.bin",       0x10000, 0x10000, 0x9dfa0dd5 )
	ROM_CONTINUE(                   0x00000, 0x10000 )
	ROM_LOAD( "tf1_prg1.bin",       0xc0000, 0x20000, 0x4a8bb251 )
	ROM_LOAD( "tf1_prg0.bin",       0xe0000, 0x20000, 0x2ae4b9eb )

	ROM_REGION( 0x14000, REGION_USER2, 0 )  /* 80k for RAM */

	ROM_REGION( 0x50000, REGION_CPU4, 0 )       /* the MCU & voice */
	ROM_LOAD( "ns1-mcu.bin",        0x0f000, 0x01000, 0xffb5c0bd )
	ROM_LOAD( "tf1_voi0.bin",       0x20000, 0x10000, 0xf542676a )
	ROM_CONTINUE(                   0x10000, 0x10000 )
	ROM_LOAD( "tf1_voi1.bin",       0x30000, 0x20000, 0x615d09cd )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )  /* character mask */
	ROM_LOAD( "tf1_chr8.bin",       0x00000, 0x20000, 0x7d53b31e )

	ROM_REGION( 0x100000, REGION_GFX2, ROMREGION_DISPOSE ) /* characters */
	ROM_LOAD( "tf1_chr0.bin",       0x00000, 0x20000, 0x9e91794e )
	ROM_LOAD( "tf1_chr1.bin",       0x20000, 0x20000, 0x76e1bc56 )
	ROM_LOAD( "tf1_chr2.bin",       0x40000, 0x20000, 0xfcb645d9 )
	ROM_LOAD( "tf1_chr3.bin",       0x60000, 0x20000, 0xa8dbf080 )
	ROM_LOAD( "tf1_chr4.bin",       0x80000, 0x20000, 0x51fedc8c )
	ROM_LOAD( "tf1_chr5.bin",       0xa0000, 0x20000, 0xe6c6609a )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "tf1_obj0.bin",       0x00000, 0x20000, 0x4bedd51a )
	ROM_LOAD( "tf1_obj1.bin",       0x20000, 0x20000, 0xdf674d6d )
ROM_END


GAME( 1987, shadowld, 0,        ns1,     ns1,     shadowld, ROT0,   "Namco", "Shadow Land" )
GAME( 1987, youkaidk, shadowld, ns1,     ns1,     shadowld, ROT0,   "Namco", "Yokai Douchuuki (Japan new version)" )
GAME( 1987, yokaidko, shadowld, ns1,     ns1,     shadowld, ROT0,   "Namco", "Yokai Douchuuki (Japan old version)" )
GAME( 1987, dspirit,  0,        ns1,     dspirit, dspirit,  ROT270, "Namco", "Dragon Spirit (new version)" )
GAME( 1987, dspirito, dspirit,  ns1,     dspirit, dspirit,  ROT270, "Namco", "Dragon Spirit (old version)" )
GAME( 1987, blazer,   0,        ns1,     ns1,     blazer,   ROT270, "Namco", "Blazer (Japan)" )
GAME( 1987, quester,  0,        quester, quester, quester,  ROT270, "Namco", "Quester (Japan)" )
GAME( 1987, pacmania, 0,        ns1,     ns1,     pacmania, ROT90,  "Namco", "Pac-Mania" )
GAME( 1987, pacmanij, pacmania, ns1,     ns1,     pacmania, ROT270, "Namco", "Pac-Mania (Japan)" )
GAME( 1987, galaga88, 0,        ns1,     ns1,     galaga88, ROT90,  "Namco", "Galaga '88 (set 1)" )
GAME( 1987, galag88b, galaga88, ns1,     ns1,     galaga88, ROT90,  "Namco", "Galaga '88 (set 2)" )
GAME( 1987, galag88j, galaga88, ns1,     ns1,     galaga88, ROT270, "Namco", "Galaga '88 (Japan)" )
GAME( 1988, ws,       0,        ns1,     ns1,     ws,       ROT0,   "Namco", "World Stadium (Japan)" )
GAME( 1988, berabohm, 0,        ns1,     berabohm,berabohm, ROT0,   "Namco", "Beraboh Man (Japan version C)" )
GAME( 1988, beraboho, berabohm, ns1,     berabohm,berabohm, ROT0,   "Namco", "Beraboh Man (Japan version B)" )
//GAME( 1988, alice,    0,        ns1,     ns1,     alice,    ROT0,   "Namco", "Alice In Wonderland" )
GAME( 1988, mmaze,    0,        ns1,     ns1,     alice,    ROT0,   "Namco", "Marchen Maze (Japan)" )
GAME( 1988, bakutotu, 0,        bakutotu,bakutotu,bakutotu, ROT0,   "Namco", "Bakutotsu Kijuutei" )
GAME( 1988, wldcourt, 0,        ns1,     ns1,     wldcourt, ROT0,   "Namco", "World Court (Japan)" )
GAME( 1988, splatter, 0,        ns1,     ns1,     splatter, ROT0,   "Namco", "Splatter House (Japan)" )
GAME( 1988, faceoff,  0,        faceoff, faceoff, faceoff,  ROT0,   "Namco", "Face Off (Japan)" )
GAME( 1989, rompers,  0,        ns1,     ns1,     rompers,  ROT270, "Namco", "Rompers (Japan)" )
GAME( 1989, romperso, rompers,  ns1,     ns1,     rompers,  ROT270, "Namco", "Rompers (Japan old version)" )
GAME( 1989, blastoff, 0,        ns1,     ns1,     blastoff, ROT270, "Namco", "Blast Off (Japan)" )
GAME( 1989, ws89,     ws,       ns1,     ns1,     ws89,     ROT0,   "Namco", "World Stadium '89 (Japan)" )
GAME( 1989, dangseed, 0,        ns1,     ns1,     dangseed, ROT270, "Namco", "Dangerous Seed (Japan)" )
GAME( 1990, ws90,     ws,       ns1,     ns1,     ws90,     ROT0,   "Namco", "World Stadium '90 (Japan)" )
GAME( 1990, pistoldm, 0,        ns1,     ns1,     pistoldm, ROT180, "Namco", "Pistol Daimyo no Bouken (Japan)" )
GAME( 1990, boxyboy,  0,        ns1,     ns1,     soukobdx, ROT180, "Namco", "Boxy Boy (US)" )
GAME( 1990, soukobdx, boxyboy,  ns1,     ns1,     soukobdx, ROT180, "Namco", "Souko Ban Deluxe (Japan)" )
GAME( 1990, puzlclub, 0,        ns1,     ns1,     puzlclub, ROT270, "Namco", "Puzzle Club (Japan prototype)" )
GAME( 1991, tankfrce, 0,        ns1,     ns1,     tankfrce, ROT180, "Namco", "Tank Force (US)" )
GAME( 1991, tankfrcj, tankfrce, ns1,     ns1,     tankfrce, ROT180, "Namco", "Tank Force (Japan)" )

