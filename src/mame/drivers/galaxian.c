/***************************************************************************

    Galaxian-derived hardware

    Galaxian is the root hardware for many, many systems developed in the
    1980-1982 timeframe. The basic design, which originated with Namco(?),
    was replicated, tweaked, bootlegged, and used numerous times.

    The basic hardware design comprises three sections on a single PCB:
    a CPU section, a sound section, and a video section.

    The CPU section is based around a Z80 (though there are modified
    designed that changed this to an S2650). The base galaxian hardware
    is designed to allow access to up to 16k of program ROM and 2k of
    working RAM.

    The sound section consists of three parts. The first part is
    a programmable 8-bit down counter that clocks a 4-bit counter which
    generates a primitive waveform whose shape is hardcoded but can be
    controlled by a pair of variable resistors. The second part is
    a set of three 555 timers which can be individually enabled and
    combined to produce square waves at fixed separated pitches. A
    fourth 555 timer is configured via a 4-bit frequency parameter to
    control the overall pitch of the other three. Finally, two single
    bit-triggered noise circuits are available. A 17-bit noise LFSR
    (which also generates stars for the video circuit) feeds into both
    circuits. A "HIT" line enables a simple on/off control of one
    filtered output, while a "FIRE" line triggers a fixed short duration
    pulse (controlled by another 555 timer) of modulated noise.

    See video/galaxian.c for a description of the video section.

****************************************************************************

    Schematics are known to exist for these games:
        * Galaxian
        * Moon Alien Part 2
        * King and Balloon

        * Moon Cresta
        * Moon Shuttle

        * Frogger
        * Amidar
        * Turtles

        * Scramble
        * The End

        * Super Cobra
        * Dark Planet
        * Lost Tomb

        * Dambusters

****************************************************************************

Main clock: XTAL = 18.432 MHz
Z80 Clock: XTAL/6 = 3.072 MHz
Horizontal video frequency: HSYNC = XTAL/3/192/2 = 16 kHz
Video frequency: VSYNC = HSYNC/132/2 = 60.606060 Hz
VBlank duration: 1/VSYNC * (20/132) = 2500 us


Notes:
-----

- The only code difference between 'galaxian' and 'galmidw' is that the
  'BONUS SHIP' text is printed on a different line.


TODO:
----

- Problems with Galaxian based on the observation of a real machine:

  - Background humming is incorrect.  It's faster on a real machine
  - Explosion sound is much softer.  Filter involved?

- $4800-4bff in Streaking/Ghost Muncher



Moon Cresta versions supported:
------------------------------

mooncrst    Nichibutsu     - later revision with better demo mode and
                              text for docking. Encrypted. No ROM/RAM check
mooncrsu    Nichibutsu USA - later revision with better demo mode and
                              text for docking. Unencrypted. No ROM/RAM check
mooncrsa    Nichibutsu     - older revision with better demo mode and
                              text for docking. Encrypted. No ROM/RAM check
mooncrs2    Nichibutsu     - probably first revision (no patches) and ROM/RAM check code.
                              This came from a bootleg board, with the logos erased
                              from the graphics
mooncrsg    Gremlin        - same docking text as mooncrst
mooncrsb    bootleg of mooncrs2. ROM/RAM check erased.


Notes about 'azurian' :
-----------------------

  bit 6 of IN1 is linked with bit 2 of IN2 (check code at 0x05b3) to set difficulty :

    bit 6  bit 2    contents of
     IN1     IN2          0x40f4            consequences            difficulty

     OFF     OFF             2          aliens move 2 frames out of 3       easy
     ON      OFF             4          aliens move 4 frames out of 5       hard
     OFF     ON              3          aliens move 3 frames out of 4       normal
     ON      ON              5          aliens move 5 frames out of 6       very hard

  aliens movements is handled by routine at 0x1d59 :

    - alien 1 moves when 0x4044 != 0 else contents of 0x40f4 is stored at 0x4044
    - alien 2 moves when 0x4054 != 0 else contents of 0x40f4 is stored at 0x4054
    - alien 3 moves when 0x4064 != 0 else contents of 0x40f4 is stored at 0x4064


Notes about 'smooncrs' :
------------------------

  Due to code at 0x2b1c and 0x3306, the game ALWAYS checks the inputs for player 1
  (even for player 2 when "Cabinet" Dip Switch is set to "Cocktail")


Notes about 'scorpnmc' :
-----------------------

  As the START buttons are also the buttons for player 1, how should I map them ?
  I've coded this the same way as in 'checkman', but I'm not sure this is correct.

  I can't tell if it's a bug, but if you reset the game when the screen is flipped,
  the screens remains flipped (the "flip screen" routine doesn't seem to be called) !


Notes about 'frogg' :
---------------------

  If bit 5 of IN0 or bit 5 of IN1 is HIGH, something strange occurs (check code
  at 0x3580) : each time you press START2 a counter at 0x47da is incremented.
  When this counter reaches 0x2f, each next time you press START2, it acts as if
  you had pressed COIN2, so credits are added !
  Bit 5 of IN0 is tested if "Cabinet" Dip Switch is set to "Upright" and
  bit 5 of IN1 is tested if "Cabinet" Dip Switch is set to "Cocktail".


TO DO :
-------

  - smooncrs : fix read/writes at/to unmapped memory (when player 2, "cocktail" mode)
               fix the ?#! bug with "bullets" (when player 2, "cocktail" mode)
  - zigzag   : full Dip Switches and Inputs
  - zigzag2  : full Dip Switches and Inputs
  - jumpbug  : full Dip Switches and Inputs
  - jumpbugb : full Dip Switches and Inputs
  - levers   : full Dip Switches and Inputs
  - kingball : full Dip Switches and Inputs
  - kingbalj : full Dip Switches and Inputs
  - frogg    : fix read/writes at/to unmapped/wrong memory
  - scprpng  : fix read/writes at/to unmapped/wrong memory

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/s2650/s2650.h"
#include "galaxian.h"
#include "machine/8255ppi.h"
#include "sound/ay8910.h"
#include "sound/sn76496.h"
#include "sound/dac.h"
#include "includes/cclimber.h"
#include "sound/discrete.h"


#define KONAMI_SOUND_CLOCK		14318000



/*************************************
 *
 *  Globals
 *
 *************************************/

static UINT8 gmgalax_selected_game;
static UINT8 zigzag_ay8910_latch;
static UINT8 kingball_speech_dip;
static UINT8 kingball_sound;
static UINT8 mshuttle_ay8910_cs;
static UINT8 scorpion_sound_data;

static UINT16 protection_state;
static UINT8 protection_result;

static UINT8 konami_sound_control;
static UINT8 sfx_sample_control;

static UINT8 irq_enabled;
static int irq_line;



/*************************************
 *
 *  Interrupts
 *
 *************************************/

static INTERRUPT_GEN( interrupt_gen )
{
	/* interrupt line is clocked at VBLANK */
	/* a flip-flop at 6F is held in the preset state based on the NMI ON signal */
	if (irq_enabled)
		cpunum_set_input_line(machine, 0, irq_line, ASSERT_LINE);
}


static WRITE8_HANDLER( irq_enable_w )
{
	/* the latched D0 bit here goes to the CLEAR line on the interrupt flip-flop */
	irq_enabled = data & 1;

	/* if CLEAR is held low, we must make sure the interrupt signal is clear */
	if (!irq_enabled)
		cpunum_set_input_line(machine, 0, irq_line, CLEAR_LINE);
}



/*************************************
 *
 *  DRIVER latch control
 *
 *************************************/

static WRITE8_HANDLER( start_lamp_w )
{
	/* offset 0 = 1P START LAMP */
	/* offset 1 = 2P START LAMP */
	set_led_status(offset, data & 1);
}


static WRITE8_HANDLER( coin_lock_w )
{
	/* many variants and bootlegs don't have this */
	coin_lockout_global_w(~data & 1);
}


static WRITE8_HANDLER( coin_count_0_w )
{
	coin_counter_w(0, data & 1);
}


static WRITE8_HANDLER( coin_count_1_w )
{
	coin_counter_w(1, data & 1);
}



/*************************************
 *
 *  General Konami sound I/O
 *
 *************************************/

static READ8_HANDLER( konami_ay8910_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x20) result &= AY8910_read_port_0_r(machine, 0);
	if (offset & 0x80) result &= AY8910_read_port_1_r(machine, 0);
	return result;
}


static WRITE8_HANDLER( konami_ay8910_w )
{
	/* the decoding here is very simplistic, and you can address all four simultaneously */
	if (offset & 0x10) AY8910_control_port_0_w(machine, 0, data);
	if (offset & 0x20) AY8910_write_port_0_w(machine, 0, data);
	if (offset & 0x40) AY8910_control_port_1_w(machine, 0, data);
	if (offset & 0x80) AY8910_write_port_1_w(machine, 0, data);
}


static WRITE8_HANDLER( konami_sound_control_w )
{
	UINT8 old = konami_sound_control;
	konami_sound_control = data;

	/* the inverse of bit 3 clocks the flip flop to signal an INT */
	/* it is automatically cleared on the acknowledge */
	if ((old & 0x08) && !(data & 0x08))
		cpunum_set_input_line(machine, 1, 0, HOLD_LINE);

	/* bit 4 is sound disable */
	sound_global_enable(~data & 0x10);
}


static READ8_HANDLER( konami_sound_timer_r )
{
	/*
        The timer is clocked at KONAMI_SOUND_CLOCK and cascades through a
        series of counters. It first encounters a chained pair of 4-bit
        counters in an LS393, which produce an effective divide-by-256. Next
        it enters the divide-by-2 counter in an LS93, followed by the
        divide-by-8 counter. Finally, it clocks a divide-by-5 counter in an
        LS90, followed by the divide-by-2 counter. This produces an effective
        period of 16*16*2*8*5*2 = 40960 clocks.

        The clock for the sound CPU comes from output C of the first
        divide-by-16 counter, or KONAMI_SOUND_CLOCK/8. To recover the
        current counter index, we use the sound cpu clock times 8 mod
        16*16*2*8*5*2.
    */
	UINT32 cycles = (cpunum_gettotalcycles(1) * 8) % (UINT64)(16*16*2*8*5*2);
	UINT8 hibit = 0;

	/* separate the high bit from the others */
	if (cycles >= 16*16*2*8*5)
	{
		hibit = 1;
		cycles -= 16*16*2*8*5;
	}

	/* the top bits of the counter index map to various bits here */
	return (hibit << 7) |			/* B7 is the output of the final divide-by-2 counter */
		   (BIT(cycles,14) << 6) |	/* B6 is the high bit of the divide-by-5 counter */
		   (BIT(cycles,13) << 5) |	/* B5 is the 2nd highest bit of the divide-by-5 counter */
		   (BIT(cycles,11) << 4) |	/* B4 is the high bit of the divide-by-8 counter */
		   0x0e;					/* assume remaining bits are high, except B0 which is grounded */
}


static WRITE8_HANDLER( konami_sound_filter_w )
{
	int which, chan;

	/* the offset is used as data, 6 channels * 2 bits each */
	for (which = 0; which < 2; which++)
		if (sndti_exists(SOUND_AY8910, which))
			for (chan = 0; chan < 3; chan++)
			{
				UINT8 bits = (offset >> (2 * chan + 6 * (1 - which))) & 3;

				/* low bit goes to 0.22uF capacitor = 220000pF  */
				/* high bit goes to 0.047uF capacitor = 47000pF */
				discrete_sound_w(machine, NODE(3 * which + chan + 11), bits);
			}
}


static READ8_HANDLER( konami_porta_0_r )
{
//  logerror("%04X:ppi0_porta_r\n", activecpu_get_pc());
	return input_port_read(machine, "IN0");
}


static READ8_HANDLER( konami_portb_0_r )
{
//  logerror("%04X:ppi0_portb_r\n", activecpu_get_pc());
	return input_port_read(machine, "IN1");
}


static READ8_HANDLER( konami_portc_0_r )
{
	logerror("%04X:ppi0_portc_r\n", activecpu_get_pc());
	return input_port_read(machine, "IN2");
}


static READ8_HANDLER( konami_portc_1_r )
{
	logerror("%04X:ppi1_portc_r\n", activecpu_get_pc());
	return input_port_read(machine, "IN3");
}


static WRITE8_HANDLER( konami_portc_0_w )
{
	logerror("%04X:ppi0_portc_w = %02X\n", activecpu_get_pc(), data);
}


static WRITE8_HANDLER( konami_portc_1_w )
{
	logerror("%04X:ppi1_portc_w = %02X\n", activecpu_get_pc(), data);
}


static const ppi8255_interface konami_ppi8255_intf =
{
	2,
	{ konami_porta_0_r, NULL },				/* Port A read */
	{ konami_portb_0_r, NULL },				/* Port B read */
	{ konami_portc_0_r, konami_portc_1_r },	/* Port C read */
	{ NULL, soundlatch_w },					/* Port A write */
	{ NULL, konami_sound_control_w },		/* Port B write */
	{ konami_portc_0_w, konami_portc_1_w }, /* Port C write */
};



/*************************************
 *
 *  The End I/O
 *
 *************************************/

static READ8_HANDLER( theend_ppi8255_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x0100) result &= ppi8255_0_r(machine, offset & 3);
	if (offset & 0x0200) result &= ppi8255_1_r(machine, offset & 3);
	return result;
}


static WRITE8_HANDLER( theend_ppi8255_w )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	if (offset & 0x0100) ppi8255_0_w(machine, offset & 3, data);
	if (offset & 0x0200) ppi8255_1_w(machine, offset & 3, data);
}


static WRITE8_HANDLER( theend_coin_counter_w )
{
	coin_counter_w(0, data & 0x80);
}



/*************************************
 *
 *  Scramble I/O
 *
 *************************************/

static WRITE8_HANDLER( scramble_protection_w )
{
	/*
        This is not fully understood; the low 4 bits of port C are
        inputs; the upper 4 bits are outputs. Scramble main set always
        writes sequences of 3 or more nibbles to the low port and
        expects certain results in the upper nibble afterwards.
    */
	protection_state = (protection_state << 4) | (data & 0x0f);
	switch (protection_state & 0xfff)
	{
		/* scramble */
		case 0xf09:		protection_result = 0xff;	break;
		case 0xa49:		protection_result = 0xbf;	break;
		case 0x319:		protection_result = 0x4f;	break;
		case 0x5c9:		protection_result = 0x6f;	break;

		/* scrambls */
		case 0x246:		protection_result ^= 0x80;	break;
		case 0xb5f:		protection_result = 0x6f;	break;
	}
}


static READ8_HANDLER( scramble_protection_r )
{
	return protection_result;
}


static CUSTOM_INPUT( scramble_protection_alt_r )
{
	/*
        There are two additional bits that are derived from bit 7 of
        the protection result. This is just a guess but works well enough
        to boot scrambls.
    */
	return (protection_result >> 7) & 1;
}



/*************************************
 *
 *  Explorer I/O
 *
 *************************************/

static WRITE8_HANDLER( explorer_sound_control_w )
{
	cpunum_set_input_line(machine, 1, 0, ASSERT_LINE);
}


static READ8_HANDLER( explorer_sound_latch_r )
{
	cpunum_set_input_line(machine, 1, 0, CLEAR_LINE);
	return soundlatch_r(machine, 0);
}



/*************************************
 *
 *  SF-X I/O
 *
 *************************************/

static READ8_HANDLER( sfx_sample_io_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x04) result &= ppi8255_2_r(machine, offset & 3);
	return result;
}


static WRITE8_HANDLER( sfx_sample_io_w )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	if (offset & 0x04) ppi8255_2_w(machine, offset & 3, data);
	if (offset & 0x10) DAC_0_signed_data_w(machine, offset, data);
}


static WRITE8_HANDLER( sfx_sample_control_w )
{
	UINT8 old = sfx_sample_control;
	sfx_sample_control = data;

	/* the inverse of bit 0 clocks the flip flop to signal an INT */
	/* it is automatically cleared on the acknowledge */
	if ((old & 0x01) && !(data & 0x01))
		cpunum_set_input_line(machine, 1, 0, HOLD_LINE);
}


static const ppi8255_interface sfx_ppi8255_intf =
{
	3,
	{ konami_porta_0_r, NULL, soundlatch2_r },		/* Port A read */
	{ konami_portb_0_r, NULL, NULL },				/* Port B read */
	{ konami_portc_0_r, konami_portc_1_r, NULL },	/* Port C read */
	{ NULL, soundlatch_w, NULL },					/* Port A write */
	{ NULL, konami_sound_control_w, NULL },			/* Port B write */
	{ konami_portc_0_w, konami_portc_1_w, NULL },	/* Port C write */
};



/*************************************
 *
 *  Frogger I/O
 *
 *************************************/

static READ8_HANDLER( frogger_ppi8255_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x1000) result &= ppi8255_1_r(machine, (offset >> 1) & 3);
	if (offset & 0x2000) result &= ppi8255_0_r(machine, (offset >> 1) & 3);
	return result;
}


static WRITE8_HANDLER( frogger_ppi8255_w )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	if (offset & 0x1000) ppi8255_1_w(machine, (offset >> 1) & 3, data);
	if (offset & 0x2000) ppi8255_0_w(machine, (offset >> 1) & 3, data);
}


static READ8_HANDLER( frogger_ay8910_r )
{
	/* the decoding here is very simplistic */
	UINT8 result = 0xff;
	if (offset & 0x40) result &= AY8910_read_port_0_r(machine, 0);
	return result;
}


static WRITE8_HANDLER( frogger_ay8910_w )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	if (offset & 0x80) AY8910_control_port_0_w(machine, 0, data);
	if (offset & 0x40) AY8910_write_port_0_w(machine, 0, data);
}


static READ8_HANDLER( frogger_sound_timer_r )
{
	/* same as regular Konami sound but with bits 3,5 swapped */
	UINT8 konami_value = konami_sound_timer_r(machine, 0);
	return BITSWAP8(konami_value, 7,6,3,4,5,2,1,0);
}


static WRITE8_HANDLER( froggrmc_sound_control_w )
{
	cpunum_set_input_line(machine, 1, 0, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
}



/*************************************
 *
 *  Frog (Falcon) I/O
 *
 *************************************/

static READ8_HANDLER( frogf_ppi8255_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x1000) result &= ppi8255_0_r(machine, (offset >> 3) & 3);
	if (offset & 0x2000) result &= ppi8255_1_r(machine, (offset >> 3) & 3);
	return result;
}


static WRITE8_HANDLER( frogf_ppi8255_w )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	if (offset & 0x1000) ppi8255_0_w(machine, (offset >> 3) & 3, data);
	if (offset & 0x2000) ppi8255_1_w(machine, (offset >> 3) & 3, data);
}



/*************************************
 *
 *  Turtles I/O
 *
 *************************************/

static READ8_HANDLER( turtles_ppi8255_0_r ) { return ppi8255_0_r(machine, (offset >> 4) & 3); }
static READ8_HANDLER( turtles_ppi8255_1_r ) { return ppi8255_1_r(machine, (offset >> 4) & 3); }
static WRITE8_HANDLER( turtles_ppi8255_0_w ) { ppi8255_0_w(machine, (offset >> 4) & 3, data); }
static WRITE8_HANDLER( turtles_ppi8255_1_w ) { ppi8255_1_w(machine, (offset >> 4) & 3, data); }



/*************************************
 *
 *  Scorpion sound I/O
 *
 *************************************/

static READ8_HANDLER( scorpion_ay8910_r )
{
	/* the decoding here is very simplistic, and you can address both simultaneously */
	UINT8 result = 0xff;
	if (offset & 0x08) result &= AY8910_read_port_2_r(machine, 0);
	if (offset & 0x20) result &= AY8910_read_port_0_r(machine, 0);
	if (offset & 0x80) result &= AY8910_read_port_1_r(machine, 0);
	return result;
}


static WRITE8_HANDLER( scorpion_ay8910_w )
{
	/* the decoding here is very simplistic, and you can address all six simultaneously */
	if (offset & 0x04) AY8910_control_port_2_w(machine, 0, data);
	if (offset & 0x08) AY8910_write_port_2_w(machine, 0, data);
	if (offset & 0x10) AY8910_control_port_0_w(machine, 0, data);
	if (offset & 0x20) AY8910_write_port_0_w(machine, 0, data);
	if (offset & 0x40) AY8910_control_port_1_w(machine, 0, data);
	if (offset & 0x80) AY8910_write_port_1_w(machine, 0, data);
}


static READ8_HANDLER( scorpion_protection_r )
{
	UINT16 paritybits;
	UINT8 parity = 0;

	/* compute parity of the current (bitmask & $CE29) */
	for (paritybits = protection_state & 0xce29; paritybits != 0; paritybits >>= 1)
		if (paritybits & 1)
			parity++;

	/* only the low bit matters for protection, but bit 2 is also checked */
	return parity;
}


static WRITE8_HANDLER( scorpion_protection_w )
{
	/* bit 5 low is a reset */
	if (!(data & 0x20))
		protection_state = 0x0000;

	/* bit 4 low is a clock */
	if (!(data & 0x10))
	{
		/* each clock shifts left one bit and ORs in the inverse of the parity */
		protection_state = (protection_state << 1) | (~scorpion_protection_r(machine, 0) & 1);
	}
}


static READ8_HANDLER( scorpion_sound_status_r )
{
	logerror("%04X:scorpion_sound_status_r()\n", safe_activecpu_get_pc());
	return 1;
}


static WRITE8_HANDLER( scorpion_sound_data_w )
{
	scorpion_sound_data = data;
//  logerror("%04X:scorpion_sound_data_w(%02X)\n", safe_activecpu_get_pc(), data);
}


static WRITE8_HANDLER( scorpion_sound_control_w )
{
	if (!(data & 0x04))
		mame_printf_debug("Secondary sound = %02X\n", scorpion_sound_data);
//  logerror("%04X:scorpion_sound_control_w(%02X)\n", safe_activecpu_get_pc(), data);
}



/*************************************
 *
 *  Ghostmuncher Galaxian I/O
 *
 *************************************/

static INPUT_CHANGED( gmgalax_game_changed )
{
	/* new value is the selected game */
	gmgalax_selected_game = newval;

	/* select the bank and graphics bank based on it */
	memory_set_bank(1, gmgalax_selected_game);
	galaxian_gfxbank_w(machine, 0, gmgalax_selected_game);

	/* reset the starts */
	galaxian_stars_enable_w(machine, 0, 0);

	/* reset the CPU */
	cpunum_set_input_line(machine, 0, INPUT_LINE_RESET, PULSE_LINE);
}


static CUSTOM_INPUT( gmgalax_port_r )
{
	const char *portname = param;
	if (gmgalax_selected_game != 0)
		portname += strlen(portname) + 1;
	return input_port_read(machine, portname);
}



/*************************************
 *
 *  Zig Zag I/O
 *
 *************************************/

static WRITE8_HANDLER( zigzag_bankswap_w )
{
	memory_set_bank(1, data & 1);
	memory_set_bank(2, ~data & 1);
}


static WRITE8_HANDLER( zigzag_ay8910_w )
{
	switch (offset & 0x300)
	{
		case 0x000:
			/* control lines */
			/* bit 0 = WRITE */
			/* bit 1 = C/D */
			if ((offset & 1) != 0)
			{
				if ((offset & 2) == 0)
					AY8910_write_port_0_w(machine, 0, zigzag_ay8910_latch);
				else
					AY8910_control_port_0_w(machine, 0, zigzag_ay8910_latch);
			}
			break;

		case 0x100:
			/* data latch */
			zigzag_ay8910_latch = offset & 0xff;
			break;

		case 0x200:
			/* unknown */
			break;
	}
}



/*************************************
 *
 *  Azurian I/O
 *
 *************************************/

static CUSTOM_INPUT( azurian_port_r )
{
	return (input_port_read(machine, "FAKE") >> (FPTR)param) & 1;
}



/*************************************
 *
 *  King & Balloon I/O
 *
 *************************************/

static CUSTOM_INPUT( kingball_muxbit_r )
{
	/* multiplex the service mode switch with a speech DIP switch */
	return (input_port_read(machine, "FAKE") >> kingball_speech_dip) & 1;
}


static CUSTOM_INPUT( kingball_noise_r )
{
	/* bit 5 is the NOISE line from the sound circuit.  The code just verifies
       that it's working, doesn't actually use return value, so we can just use
       rand() */
	return mame_rand(machine) & 1;
}


static WRITE8_HANDLER( kingball_speech_dip_w )
{
	kingball_speech_dip = data;
}


static WRITE8_HANDLER( kingball_sound1_w )
{
	kingball_sound = (kingball_sound & ~0x01) | data;
}


static WRITE8_HANDLER( kingball_sound2_w )
{
	kingball_sound = (kingball_sound & ~0x02) | (data << 1);
	soundlatch_w(machine, 0, kingball_sound | 0xf0);
}


static WRITE8_HANDLER( kingball_dac_w )
{
	DAC_0_data_w(machine, offset, data ^ 0xff);
}



/*************************************
 *
 *  Moon Shuttle I/O
 *
 *************************************/

static WRITE8_HANDLER( mshuttle_ay8910_cs_w )
{
	mshuttle_ay8910_cs = data & 1;
}


static WRITE8_HANDLER( mshuttle_ay8910_control_w )
{
	if (!mshuttle_ay8910_cs)
		AY8910_control_port_0_w(machine, offset, data);
}


static WRITE8_HANDLER( mshuttle_ay8910_data_w )
{
	if (!mshuttle_ay8910_cs)
		AY8910_write_port_0_w(machine, offset, data);
}


static READ8_HANDLER( mshuttle_ay8910_data_r )
{
	if (!mshuttle_ay8910_cs)
		return AY8910_read_port_0_r(machine, offset);
	return 0xff;
}



/*************************************
 *
 *  Jump Bug I/O
 *
 *************************************/

static READ8_HANDLER( jumpbug_protection_r )
{
	switch (offset)
	{
		case 0x0114:  return 0x4f;
		case 0x0118:  return 0xd3;
		case 0x0214:  return 0xcf;
		case 0x0235:  return 0x02;
		case 0x0311:  return 0xff;  /* not checked */
	}
	logerror("Unknown protection read. Offset: %04X  PC=%04X\n",0xb000+offset,activecpu_get_pc());
	return 0xff;
}



/*************************************
 *
 *  Checkman I/O
 *
 *************************************/

static WRITE8_HANDLER( checkman_sound_command_w )
{
	soundlatch_w(machine, 0, data);
	cpunum_set_input_line(machine, 1, INPUT_LINE_NMI, PULSE_LINE);
}


static TIMER_CALLBACK( checkmaj_irq0_gen )
{
	cpunum_set_input_line(machine, 1, 0, HOLD_LINE);
}


static READ8_HANDLER( checkmaj_protection_r )
{
	switch (activecpu_get_pc())
	{
		case 0x0f15:  return 0xf5;
		case 0x0f8f:  return 0x7c;
		case 0x10b3:  return 0x7c;
		case 0x10e0:  return 0x00;
		case 0x10f1:  return 0xaa;
		case 0x1402:  return 0xaa;
		default:
			logerror("Unknown protection read. PC=%04X\n",activecpu_get_pc());
	}

	return 0;
}



/*************************************
 *
 *  Dingo I/O
 *
 *************************************/

static READ8_HANDLER( dingo_3000_r )
{
	return 0xaa;
}


static READ8_HANDLER( dingo_3035_r )
{
	return 0x8c;
}


static READ8_HANDLER( dingoe_3001_r )
{
	return 0xaa;
}



/*************************************
 *
 *  Memory maps
 *
 *************************************/

/*
0000-3fff


4000-7fff
  4000-47ff -> RAM read/write (10 bits = 0x400)
  4800-4fff -> n/c
  5000-57ff -> /VRAM RD or /VRAM WR (10 bits = 0x400)
  5800-5fff -> /OBJRAM RD or /OBJRAM WR (8 bits = 0x100)
  6000-67ff -> /SW0 or /DRIVER
  6800-6fff -> /SW1 or /SOUND
  7000-77ff -> /DIPSW or LATCH
  7800-7fff -> /WDR or /PITCH

/DRIVER: (write 6000-67ff)
  D0 = data bit
  A0-A2 = decoder
  6000 -> 1P START
  6001 -> 2P START
  6002 -> COIN LOCKOUT
  6003 -> COIN COUNTER
  6004 -> 1M resistor (controls 555 timer @ 9R)
  6005 -> 470k resistor (controls 555 timer @ 9R)
  6006 -> 220k resistor (controls 555 timer @ 9R)
  6007 -> 100k resistor (controls 555 timer @ 9R)

/SOUND: (write 6800-6fff)
  D0 = data bit
  A0-A2 = decoder
  6800 -> FS1 (enables 555 timer at 8R)
  6801 -> FS2 (enables 555 timer at 8S)
  6802 -> FS3 (enables 555 timer at 8T)
  6803 -> HIT
  6804 -> n/c
  6805 -> FIRE
  6806 -> VOL1
  6807 -> VOL2

LATCH: (write 7000-77ff)
  D0 = data bit
  A0-A2 = decoder
  7000 -> n/c
  7001 -> NMI ON
  7002 -> n/c
  7003 -> n/c
  7004 -> STARS ON
  7005 -> n/c
  7006 -> HFLIP
  7007 -> VFLIP

/PITCH: (write 7800-7fff)
  loads latch at 9J
*/

/* map derived from schematics */
static ADDRESS_MAP_START( galaxian_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x0400) AM_RAM
	AM_RANGE(0x5000, 0x53ff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5800, 0x58ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0x6000, 0x6000) AM_MIRROR(0x07ff) AM_READ_PORT("IN0")
	AM_RANGE(0x6000, 0x6001) AM_MIRROR(0x07f8) AM_WRITE(start_lamp_w)
	AM_RANGE(0x6002, 0x6002) AM_MIRROR(0x07f8) AM_WRITE(coin_lock_w)
	AM_RANGE(0x6003, 0x6003) AM_MIRROR(0x07f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0x6004, 0x6007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0x6800, 0x6800) AM_MIRROR(0x07ff) AM_READ_PORT("IN1")
	AM_RANGE(0x6800, 0x6807) AM_MIRROR(0x07f8) AM_WRITE(galaxian_sound_w)
	AM_RANGE(0x7000, 0x7000) AM_MIRROR(0x07ff) AM_READ_PORT("IN2")
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7800, 0x7800) AM_MIRROR(0x07ff) AM_WRITE(galaxian_pitch_w)
	AM_RANGE(0x7800, 0x7800) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END


/* map derived from schematics */
static ADDRESS_MAP_START( mooncrst_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_MIRROR(0x0400) AM_RAM
	AM_RANGE(0x9000, 0x93ff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9800, 0x98ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xa000, 0xa000) AM_MIRROR(0x07ff) AM_READ_PORT("IN0")
	AM_RANGE(0xa000, 0xa002) AM_MIRROR(0x07f8) AM_WRITE(galaxian_gfxbank_w)
	AM_RANGE(0xa003, 0xa003) AM_MIRROR(0x07f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0xa004, 0xa007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0xa800, 0xa800) AM_MIRROR(0x07ff) AM_READ_PORT("IN1")
	AM_RANGE(0xa800, 0xa807) AM_MIRROR(0x07f8) AM_WRITE(galaxian_sound_w)
	AM_RANGE(0xb000, 0xb000) AM_MIRROR(0x07ff) AM_READ_PORT("IN2")
	AM_RANGE(0xb000, 0xb000) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0xb004, 0xb004) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0xb006, 0xb006) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xb007, 0xb007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xb800, 0xb800) AM_MIRROR(0x07ff) AM_WRITE(galaxian_pitch_w)
	AM_RANGE(0xb800, 0xb800) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END


/* map derived from schematics */
#if 0
static ADDRESS_MAP_START( dambustr_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
//  AM_RANGE(0x8000, 0x8000) AM_WRITE(dambustr_bg_color_w)
//  AM_RANGE(0x8001, 0x8001) AM_WRITE(dambustr_bg_split_line_w)
	AM_RANGE(0xc000, 0xc3ff) AM_MIRROR(0x0400) AM_RAM
	AM_RANGE(0xd000, 0xd3ff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xd800, 0xd8ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xe000, 0xe000) AM_MIRROR(0x07ff) AM_READ_PORT("IN0")
	AM_RANGE(0xe004, 0xe007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_lfo_freq_w)
	AM_RANGE(0xe800, 0xe800) AM_MIRROR(0x07ff) AM_READ_PORT("IN1")
	AM_RANGE(0xe800, 0xe807) AM_MIRROR(0x07f8) AM_WRITE(galaxian_sound_w)
	AM_RANGE(0xf000, 0xf000) AM_MIRROR(0x07ff) AM_READ_PORT("IN2")
	AM_RANGE(0xf001, 0xf001) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0xf004, 0xf004) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0xf006, 0xf006) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xf007, 0xf007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xf800, 0xf800) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
	AM_RANGE(0xf800, 0xf800) AM_MIRROR(0x07ff) AM_WRITE(galaxian_pitch_w)
ADDRESS_MAP_END
#endif


/* map derived from schematics */
static ADDRESS_MAP_START( theend_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x4800, 0x4bff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x50ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0x6801, 0x6801) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_MIRROR(0x07f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0x6803, 0x6803) AM_MIRROR(0x07f8) AM_WRITE(scramble_background_enable_w)
	AM_RANGE(0x6804, 0x6804) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6805, 0x6805) AM_MIRROR(0x07f8) //POUT2
	AM_RANGE(0x6806, 0x6806) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7000, 0x7000) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
	AM_RANGE(0x8000, 0xffff) AM_READWRITE(theend_ppi8255_r, theend_ppi8255_w)
ADDRESS_MAP_END


/* map derived from schematics */
static ADDRESS_MAP_START( scobra_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_MIRROR(0x4000) AM_RAM
	AM_RANGE(0x8800, 0x8bff) AM_MIRROR(0x4400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9000, 0x90ff) AM_MIRROR(0x4700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0x9800, 0x9803) AM_MIRROR(0x47fc) AM_READWRITE(ppi8255_0_r, ppi8255_0_w)
	AM_RANGE(0xa000, 0xa003) AM_MIRROR(0x47fc) AM_READWRITE(ppi8255_1_r, ppi8255_1_w)
	AM_RANGE(0xa801, 0xa801) AM_MIRROR(0x47f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0xa802, 0xa802) AM_MIRROR(0x47f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0xa803, 0xa803) AM_MIRROR(0x47f8) AM_WRITE(scramble_background_enable_w)
	AM_RANGE(0xa804, 0xa804) AM_MIRROR(0x47f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0xa805, 0xa805) AM_MIRROR(0x47f8) /* POUT2 */
	AM_RANGE(0xa806, 0xa806) AM_MIRROR(0x47f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xa807, 0xa807) AM_MIRROR(0x47f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xb000, 0xb000) AM_MIRROR(0x47ff) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END


/* map derived from schematics */
static ADDRESS_MAP_START( frogger_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8800) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
	AM_RANGE(0xa800, 0xabff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0xb000, 0xb0ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xb808, 0xb808) AM_MIRROR(0x07e3) AM_WRITE(irq_enable_w)
	AM_RANGE(0xb80c, 0xb80c) AM_MIRROR(0x07e3) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xb810, 0xb810) AM_MIRROR(0x07e3) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xb818, 0xb818) AM_MIRROR(0x07e3) AM_WRITE(coin_count_0_w)	/* IOPC7 */
	AM_RANGE(0xb81c, 0xb81c) AM_MIRROR(0x07e3) AM_WRITE(coin_count_1_w)	/* POUT1 */
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(frogger_ppi8255_r, frogger_ppi8255_w)
ADDRESS_MAP_END


/* map derived from schematics */
static ADDRESS_MAP_START( turtles_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_MIRROR(0x4000) AM_RAM
	AM_RANGE(0x9000, 0x93ff) AM_MIRROR(0x4400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9800, 0x98ff) AM_MIRROR(0x4700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xa000, 0xa000) AM_MIRROR(0x47c7) AM_WRITE(scramble_background_red_w)
	AM_RANGE(0xa008, 0xa008) AM_MIRROR(0x47c7) AM_WRITE(irq_enable_w)
	AM_RANGE(0xa010, 0xa010) AM_MIRROR(0x47c7) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xa018, 0xa018) AM_MIRROR(0x47c7) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xa020, 0xa020) AM_MIRROR(0x47c7) AM_WRITE(scramble_background_green_w)
	AM_RANGE(0xa028, 0xa028) AM_MIRROR(0x47c7) AM_WRITE(scramble_background_blue_w)
	AM_RANGE(0xa030, 0xa030) AM_MIRROR(0x47c7) AM_WRITE(coin_count_0_w)
	AM_RANGE(0xa038, 0xa038) AM_MIRROR(0x47c7) AM_WRITE(coin_count_1_w)
	AM_RANGE(0xa800, 0xa800) AM_MIRROR(0x47ff) AM_READ(watchdog_reset_r)
	AM_RANGE(0xb000, 0xb03f) AM_MIRROR(0x47cf) AM_READWRITE(turtles_ppi8255_0_r, turtles_ppi8255_0_w)
	AM_RANGE(0xb800, 0xb83f) AM_MIRROR(0x47cf) AM_READWRITE(turtles_ppi8255_1_r, turtles_ppi8255_1_w)
ADDRESS_MAP_END


/* this is the same as theend, except for separate RGB background controls
   and some extra ROM space at $7000 and $C000 */
static ADDRESS_MAP_START( sfx_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x4800, 0x4bff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x50ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0x6800, 0x6800) AM_MIRROR(0x07f8) AM_WRITE(scramble_background_red_w)
	AM_RANGE(0x6801, 0x6801) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0x6802, 0x6802) AM_MIRROR(0x07f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0x6803, 0x6803) AM_MIRROR(0x07f8) AM_WRITE(scramble_background_blue_w)
	AM_RANGE(0x6804, 0x6804) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x6805, 0x6805) AM_MIRROR(0x07f8) AM_WRITE(scramble_background_green_w)
	AM_RANGE(0x6806, 0x6806) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x6807, 0x6807) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x7000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READWRITE(theend_ppi8255_r, theend_ppi8255_w)
	AM_RANGE(0xc000, 0xefff) AM_ROM
ADDRESS_MAP_END


/* changes from galaxian map:
    galaxian sound removed
    $4800-$57ff: cointains video and object RAM (normally at $5000-$5fff)
    $5800-$5fff: AY-8910 access added
    $6002-$6006: graphics banking controls replace coin lockout, coin counter, and lfo
    $7002: coin counter (moved from $6003)
    $8000-$afff: additional ROM area
    $b000-$bfff: protection
*/
static ADDRESS_MAP_START( jumpbug_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x47ff) AM_RAM
	AM_RANGE(0x4800, 0x4bff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x5000, 0x50ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0x5800, 0x5800) AM_MIRROR(0x00ff) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x5900, 0x5900) AM_MIRROR(0x00ff) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_MIRROR(0x07ff) AM_READ_PORT("IN0")
	AM_RANGE(0x6002, 0x6006) AM_MIRROR(0x07f8) AM_WRITE(galaxian_gfxbank_w)
	AM_RANGE(0x6800, 0x6800) AM_MIRROR(0x07ff) AM_READ_PORT("IN1")
	AM_RANGE(0x7000, 0x7000) AM_MIRROR(0x07ff) AM_READ_PORT("IN2")
	AM_RANGE(0x7001, 0x7001) AM_MIRROR(0x07f8) AM_WRITE(irq_enable_w)
	AM_RANGE(0x7002, 0x7002) AM_MIRROR(0x07f8) AM_WRITE(coin_count_0_w)
	AM_RANGE(0x7004, 0x7004) AM_MIRROR(0x07f8) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0x7006, 0x7006) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0x7007, 0x7007) AM_MIRROR(0x07f8) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0x8000, 0xafff) AM_ROM
	AM_RANGE(0xb000, 0xbfff) AM_READ(jumpbug_protection_r)
ADDRESS_MAP_END


static ADDRESS_MAP_START( frogf_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8bff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9000, 0x90ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xa802, 0xa802) AM_MIRROR(0x07f1) AM_WRITE(galaxian_flip_screen_x_w)
	AM_RANGE(0xa804, 0xa804) AM_MIRROR(0x07f1) AM_WRITE(irq_enable_w)
	AM_RANGE(0xa806, 0xa806) AM_MIRROR(0x07f1) AM_WRITE(galaxian_flip_screen_y_w)
	AM_RANGE(0xa808, 0xa808) AM_MIRROR(0x07f1) AM_WRITE(coin_count_1_w)
	AM_RANGE(0xa80e, 0xa80e) AM_MIRROR(0x07f1) AM_WRITE(coin_count_0_w)
	AM_RANGE(0xb800, 0xb800) AM_MIRROR(0x07ff) AM_READ(watchdog_reset_r)
	AM_RANGE(0xc000, 0xffff) AM_READWRITE(frogf_ppi8255_r, frogf_ppi8255_w)
ADDRESS_MAP_END


/* mooncrst */
static ADDRESS_MAP_START( mshuttle_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
	AM_RANGE(0x9000, 0x93ff) AM_MIRROR(0x0400) AM_READWRITE(SMH_RAM, galaxian_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9800, 0x98ff) AM_MIRROR(0x0700) AM_READWRITE(SMH_RAM, galaxian_objram_w) AM_BASE(&spriteram)
	AM_RANGE(0xa000, 0xa000) AM_READ_PORT("IN0")
	AM_RANGE(0xa000, 0xa000) AM_WRITE(irq_enable_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(galaxian_stars_enable_w)
	AM_RANGE(0xa002, 0xa002) AM_WRITE(galaxian_flip_screen_xy_w)
	AM_RANGE(0xa004, 0xa004) AM_WRITE(cclimber_sample_trigger_w)
	AM_RANGE(0xa007, 0xa007) AM_WRITE(mshuttle_ay8910_cs_w)
	AM_RANGE(0xa800, 0xa800) AM_READ_PORT("IN1")
	AM_RANGE(0xa800, 0xa800) AM_WRITE(cclimber_sample_rate_w)
	AM_RANGE(0xb000, 0xb000) AM_READ_PORT("IN2")
	AM_RANGE(0xb000, 0xb000) AM_WRITE(cclimber_sample_volume_w)
	AM_RANGE(0xb800, 0xb800) AM_READ(watchdog_reset_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mshuttle_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x0f)
	AM_RANGE(0x08, 0x08) AM_WRITE(mshuttle_ay8910_control_w)
	AM_RANGE(0x09, 0x09) AM_WRITE(mshuttle_ay8910_data_w)
	AM_RANGE(0x0c, 0x0c) AM_READ(mshuttle_ay8910_data_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Sound CPU memory maps
 *
 *************************************/

/* Konami Frogger with 1 x AY-8910A */
static ADDRESS_MAP_START( frogger_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x7fff)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x4000, 0x43ff) AM_MIRROR(0x1c00) AM_RAM
    AM_RANGE(0x6000, 0x6fff) AM_MIRROR(0x1000) AM_WRITE(konami_sound_filter_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( frogger_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0xff) AM_READWRITE(frogger_ay8910_r, frogger_ay8910_w)
ADDRESS_MAP_END


/* Konami generic with 2 x AY-8910A */
static ADDRESS_MAP_START( konami_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x2fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_MIRROR(0x6c00) AM_RAM
    AM_RANGE(0x9000, 0x9fff) AM_MIRROR(0x6000) AM_WRITE(konami_sound_filter_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( konami_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0xff) AM_READWRITE(konami_ay8910_r, konami_ay8910_w)
ADDRESS_MAP_END


/* Checkman with 1 x AY-8910A */
static ADDRESS_MAP_START( checkman_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( checkman_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x03, 0x03) AM_READ(soundlatch_r)
	AM_RANGE(0x04, 0x04) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x05, 0x05) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x06, 0x06) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END


/* Checkman alternate with 1 x AY-8910A */
static ADDRESS_MAP_START( checkmaj_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xa002, 0xa002) AM_READ(AY8910_read_port_0_r)
ADDRESS_MAP_END


/* King and Balloon with DAC */
static ADDRESS_MAP_START( kingball_sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0x3fff)
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0x1c00) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( kingball_sound_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_MIRROR(0xff) AM_READWRITE(soundlatch_r, kingball_dac_w)
ADDRESS_MAP_END


/* SF-X sample player */
static ADDRESS_MAP_START( sfx_sample_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_MIRROR(0x6c00) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sfx_sample_portmap, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0xff) AM_READWRITE(sfx_sample_io_r, sfx_sample_io_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Graphics layouts
 *
 *************************************/

static const gfx_layout galaxian_charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(0,1) },
	{ STEP8(0,8) },
	8*8
};

static const gfx_layout galaxian_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ STEP8(0,1), STEP8(8*8,1) },
	{ STEP8(0,8), STEP8(16*8,8) },
	16*16
};



/*************************************
 *
 *  Graphics decoding
 *
 *************************************/

GFXDECODE_START(galaxian)
	GFXDECODE_SCALE(REGION_GFX1, 0x0000, galaxian_charlayout,   0, 8, GALAXIAN_XSCALE,1)
	GFXDECODE_SCALE(REGION_GFX1, 0x0000, galaxian_spritelayout, 0, 8, GALAXIAN_XSCALE,1)
GFXDECODE_END

GFXDECODE_START(gmgalax)
	GFXDECODE_SCALE(REGION_GFX1, 0x0000, galaxian_charlayout,   0, 16, GALAXIAN_XSCALE,1)
	GFXDECODE_SCALE(REGION_GFX1, 0x0000, galaxian_spritelayout, 0, 16, GALAXIAN_XSCALE,1)
GFXDECODE_END

/* separate character and sprite ROMs */
GFXDECODE_START(pacmanbl)
	GFXDECODE_SCALE(REGION_GFX1, 0x0000, galaxian_charlayout,   0, 8, GALAXIAN_XSCALE,1)
	GFXDECODE_SCALE(REGION_GFX2, 0x0000, galaxian_spritelayout, 0, 8, GALAXIAN_XSCALE,1)
GFXDECODE_END



/*************************************
 *
 *  Sound configuration
 *
 *************************************/

static struct AY8910interface frogger_ay8910_interface =
{
	soundlatch_r,
	frogger_sound_timer_r
};

static struct AY8910interface konami_ay8910_interface =
{
	soundlatch_r,
	konami_sound_timer_r
};

static struct AY8910interface explorer_ay8910_interface_1 =
{
	konami_sound_timer_r
};

static struct AY8910interface explorer_ay8910_interface_2 =
{
	explorer_sound_latch_r
};

static struct AY8910interface sfx_ay8910_interface =
{
	0,
	0,
	soundlatch2_w,
	sfx_sample_control_w
};

static struct AY8910interface scorpion_ay8910_interface =
{
	0,
	0,
	scorpion_sound_data_w,
	scorpion_sound_control_w,
};

static struct AY8910interface checkmaj_ay8910_interface =
{
	soundlatch_r
};


static const discrete_mixer_desc konami_sound_mixer_desc =
	{DISC_MIXER_IS_RESISTOR,
		{RES_K(5.1), RES_K(5.1), RES_K(5.1), RES_K(5.1), RES_K(5.1), RES_K(5.1)},
		{0,0,0,0,0,0},	/* no variable resistors   */
		{0,0,0,0,0,0},  /* no node capacitors      */
		0, RES_K(200),  /* actually after an opamp */
		0,
		CAP_U(0.15),
		0, 1};

static DISCRETE_SOUND_START( konami_sound )

	DISCRETE_INPUTX_STREAM(NODE_01, 0, 1.0, 0)
	DISCRETE_INPUTX_STREAM(NODE_02, 1, 1.0, 0)
	DISCRETE_INPUTX_STREAM(NODE_03, 2, 1.0, 0)

	DISCRETE_INPUTX_STREAM(NODE_04, 3, 1.0, 0)
	DISCRETE_INPUTX_STREAM(NODE_05, 4, 1.0, 0)
	DISCRETE_INPUTX_STREAM(NODE_06, 5, 1.0, 0)

	DISCRETE_INPUT_DATA(NODE_11)
	DISCRETE_INPUT_DATA(NODE_12)
	DISCRETE_INPUT_DATA(NODE_13)

	DISCRETE_INPUT_DATA(NODE_14)
	DISCRETE_INPUT_DATA(NODE_15)
	DISCRETE_INPUT_DATA(NODE_16)

	DISCRETE_RCFILTER_SW(NODE_21, 1, NODE_01, NODE_11, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)
	DISCRETE_RCFILTER_SW(NODE_22, 1, NODE_02, NODE_12, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)
	DISCRETE_RCFILTER_SW(NODE_23, 1, NODE_03, NODE_13, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)

	DISCRETE_RCFILTER_SW(NODE_24, 1, NODE_04, NODE_14, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)
	DISCRETE_RCFILTER_SW(NODE_25, 1, NODE_05, NODE_15, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)
	DISCRETE_RCFILTER_SW(NODE_26, 1, NODE_06, NODE_16, 1000, CAP_U(0.22), CAP_U(0.047), 0, 0)

	DISCRETE_MIXER6(NODE_30, 1, NODE_21, NODE_22, NODE_23, NODE_24, NODE_25, NODE_26, &konami_sound_mixer_desc)

	/* FIXME the amplifier M51516L has a decay circuit */
	/* This is handled with sound_global_enable but    */
	/* belongs here.                                   */

	DISCRETE_OUTPUT(NODE_30, 5.0 )

DISCRETE_SOUND_END



/*************************************
 *
 *  Core machine driver pieces
 *
 *************************************/

static MACHINE_DRIVER_START( galaxian_base )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, GALAXIAN_PIXEL_CLOCK/3/2)
	MDRV_CPU_PROGRAM_MAP(galaxian_map,0)
	MDRV_CPU_VBLANK_INT("main", interrupt_gen)

	MDRV_WATCHDOG_VBLANK_INIT(8)

	/* video hardware */
	MDRV_GFXDECODE(galaxian)
	MDRV_PALETTE_LENGTH(32)

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_RAW_PARAMS(GALAXIAN_PIXEL_CLOCK, GALAXIAN_HTOTAL, GALAXIAN_HBEND, GALAXIAN_HBSTART, GALAXIAN_VTOTAL, GALAXIAN_VBEND, GALAXIAN_VBSTART)

	MDRV_PALETTE_INIT(galaxian)
	MDRV_VIDEO_START(galaxian)
	MDRV_VIDEO_UPDATE(galaxian)

	/* blinking frequency is determined by 555 counter with Ra=100k, Rb=10k, C=10uF */
	MDRV_TIMER_ADD_PERIODIC("stars", galaxian_stars_blink_timer, NSEC(PERIOD_OF_555_ASTABLE_NSEC(100000, 10000, 0.00001)))

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( galaxian_sound )

	/* sound hardware */
	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(galaxian_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( konami_sound_1x_ay8910 )

	/* 2nd CPU to drive sound */
	MDRV_CPU_ADD_TAG("sound", Z80, KONAMI_SOUND_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(frogger_sound_map,0)
	MDRV_CPU_IO_MAP(frogger_sound_portmap,0)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910.0", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_CONFIG(frogger_ay8910_interface)
	MDRV_SOUND_ROUTE_EX(0, "konami", 1.0, 0)
	MDRV_SOUND_ROUTE_EX(1, "konami", 1.0, 1)
	MDRV_SOUND_ROUTE_EX(2, "konami", 1.0, 2)

	MDRV_SOUND_ADD_TAG("konami", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(konami_sound)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( konami_sound_2x_ay8910 )

	/* 2nd CPU to drive sound */
	MDRV_CPU_ADD_TAG("sound", Z80, KONAMI_SOUND_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(konami_sound_map,0)
	MDRV_CPU_IO_MAP(konami_sound_portmap,0)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910.0", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_ROUTE_EX(0, "konami", 1.0, 0)
	MDRV_SOUND_ROUTE_EX(1, "konami", 1.0, 1)
	MDRV_SOUND_ROUTE_EX(2, "konami", 1.0, 2)

	MDRV_SOUND_ADD_TAG("8910.1", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_CONFIG(konami_ay8910_interface)
	MDRV_SOUND_ROUTE_EX(0, "konami", 1.0, 3)
	MDRV_SOUND_ROUTE_EX(1, "konami", 1.0, 4)
	MDRV_SOUND_ROUTE_EX(2, "konami", 1.0, 5)

	MDRV_SOUND_ADD_TAG("konami", DISCRETE, 0)
	MDRV_SOUND_CONFIG_DISCRETE(konami_sound)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( galaxian )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(galaxian_sound)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pacmanbl )
	MDRV_IMPORT_FROM(galaxian)

	/* separate tile/sprite ROMs */
	MDRV_GFXDECODE(pacmanbl)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( zigzag )
	MDRV_IMPORT_FROM(galaxian_base)

	/* separate tile/sprite ROMs */
	MDRV_GFXDECODE(pacmanbl)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 1789750)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gmgalax )
	MDRV_IMPORT_FROM(galaxian)

	/* banked video hardware */
	MDRV_GFXDECODE(gmgalax)
	MDRV_PALETTE_LENGTH(64)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mooncrst )
	MDRV_IMPORT_FROM(galaxian)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mooncrst_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( jumpbug )
	MDRV_IMPORT_FROM(galaxian_base)

	MDRV_WATCHDOG_VBLANK_INIT(0)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(jumpbug_map,0)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 1789750)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( checkman )
	MDRV_IMPORT_FROM(mooncrst)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1620000)	/* 1.62 MHz */
	MDRV_CPU_PROGRAM_MAP(checkman_sound_map,0)
	MDRV_CPU_IO_MAP(checkman_sound_portmap,0)
	MDRV_CPU_VBLANK_INT("main", irq0_line_hold)	/* NMIs are triggered by the main CPU */

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 1789750)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( checkmaj )
	MDRV_IMPORT_FROM(galaxian)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 1620000)
	MDRV_CPU_PROGRAM_MAP(checkmaj_sound_map,0)

	MDRV_TIMER_ADD_SCANLINE("irq0", checkmaj_irq0_gen, "main", 0, 8)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, 1620000)
	MDRV_SOUND_CONFIG(checkmaj_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( mshuttle )
	MDRV_IMPORT_FROM(galaxian_base)

	/* basic machine hardware */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mshuttle_map,0)
	MDRV_CPU_IO_MAP(mshuttle_portmap,0)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, GALAXIAN_PIXEL_CLOCK/3/4)
	MDRV_SOUND_CONFIG(cclimber_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(SAMPLES, 0)
	MDRV_SOUND_CONFIG(cclimber_samples_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( kingball )
	MDRV_IMPORT_FROM(mooncrst)

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,5000000/2)
	MDRV_CPU_PROGRAM_MAP(kingball_sound_map,0)
	MDRV_CPU_IO_MAP(kingball_sound_portmap,0)

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( frogger )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_1x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(frogger_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( froggrmc )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_1x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(mooncrst_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( froggers )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_1x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(theend_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( frogf )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_1x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(frogf_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( turtles )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_2x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(turtles_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( theend )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_2x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(theend_map,0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( explorer )
	MDRV_IMPORT_FROM(galaxian_base)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(theend_map,0)

	/* 2nd CPU to drive sound */
	MDRV_CPU_ADD(Z80,KONAMI_SOUND_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(konami_sound_map,0)
	MDRV_CPU_IO_MAP(konami_sound_portmap,0)

	/* sound hardware */
	MDRV_SOUND_ADD_TAG("8910.0", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	MDRV_SOUND_ADD_TAG("8910.1", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_CONFIG(explorer_ay8910_interface_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( scorpion )
	MDRV_IMPORT_FROM(theend)

	/* extra AY8910 with I/O ports */
	MDRV_SOUND_ADD_TAG("8910.2", AY8910, KONAMI_SOUND_CLOCK/8)
	MDRV_SOUND_CONFIG(scorpion_ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( sfx )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_2x_ay8910)

	MDRV_WATCHDOG_VBLANK_INIT(0)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(sfx_map,0)

	/* 3rd CPU for the sample player */
	MDRV_CPU_ADD(Z80, KONAMI_SOUND_CLOCK/8)
	MDRV_CPU_PROGRAM_MAP(sfx_sample_map,0)
	MDRV_CPU_IO_MAP(sfx_sample_portmap,0)

	/* port on 1st 8910 is used for communication */
	MDRV_SOUND_MODIFY("8910.0")
	MDRV_SOUND_CONFIG(sfx_ay8910_interface)

	/* DAC for the sample player */
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( scobra )
	MDRV_IMPORT_FROM(galaxian_base)
	MDRV_IMPORT_FROM(konami_sound_2x_ay8910)

	/* alternate memory map */
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(scobra_map,0)
MACHINE_DRIVER_END



/*************************************
 *
 *  Decryption helpers
 *
 *************************************/

static void decode_mooncrst(int length, UINT8 *dest)
{
	UINT8 *rom = memory_region(REGION_CPU1);
	int offs;

	for (offs = 0; offs < length; offs++)
	{
		UINT8 data = rom[offs];
		UINT8 res = data;
		if (BIT(data,1)) res ^= 0x40;
		if (BIT(data,5)) res ^= 0x04;
		if ((offs & 1) == 0) res = BITSWAP8(res,7,2,5,4,3,6,1,0);
		dest[offs] = res;
	}
}


static void decode_checkman(void)
{
	/*
                             Encryption Table
                             ----------------
        +---+---+---+------+------+------+------+------+------+------+------+
        |A2 |A1 |A0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0    |
        +---+---+---+------+------+------+------+------+------+------+------+
        | 0 | 0 | 0 |D7    |D6    |D5    |D4    |D3    |D2    |D1    |D0^^D6|
        | 0 | 0 | 1 |D7    |D6    |D5    |D4    |D3    |D2    |D1^^D5|D0    |
        | 0 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D4|D1^^D6|D0    |
        | 0 | 1 | 1 |D7    |D6    |D5    |D4^^D2|D3    |D2    |D1    |D0^^D5|
        | 1 | 0 | 0 |D7    |D6^^D4|D5^^D1|D4    |D3    |D2    |D1    |D0    |
        | 1 | 0 | 1 |D7    |D6^^D0|D5^^D2|D4    |D3    |D2    |D1    |D0    |
        | 1 | 1 | 0 |D7    |D6    |D5    |D4    |D3    |D2^^D0|D1    |D0    |
        | 1 | 1 | 1 |D7    |D6    |D5    |D4^^D1|D3    |D2    |D1    |D0    |
        +---+---+---+------+------+------+------+------+------+------+------+

        For example if A2=1, A1=1 and A0=0 then D2 to the CPU would be an XOR of
        D2 and D0 from the ROM's. Note that D7 and D3 are not encrypted.

        Encryption PAL 16L8 on cardridge
                 +--- ---+
            OE --|   U   |-- VCC
         ROMD0 --|       |-- D0
         ROMD1 --|       |-- D1
         ROMD2 --|VER 5.2|-- D2
            A0 --|       |-- NOT USED
            A1 --|       |-- A2
         ROMD4 --|       |-- D4
         ROMD5 --|       |-- D5
         ROMD6 --|       |-- D6
           GND --|       |-- M1 (NOT USED)
                 +-------+
        Pin layout is such that links can replace the PAL if encryption is not used.
    */
	static const UINT8 xortable[8][4] =
	{
		{ 6,0,6,0 },
		{ 5,1,5,1 },
		{ 4,2,6,1 },
		{ 2,4,5,0 },
		{ 4,6,1,5 },
		{ 0,6,2,5 },
		{ 0,2,0,2 },
		{ 1,4,1,4 }
	};
	UINT8 *rombase = memory_region(REGION_CPU1);
	UINT32 romlength = memory_region_length(REGION_CPU1);
	UINT32 offs;

	for (offs = 0; offs < romlength; offs++)
	{
		UINT8 data = rombase[offs];
		UINT32 line = offs & 0x07;

		data ^= (BIT(data,xortable[line][0]) << xortable[line][1]) | (BIT(data,xortable[line][2]) << xortable[line][3]);
		rombase[offs] = data;
	}
}


static void decode_dingoe(void)
{
	UINT8 *rombase = memory_region(REGION_CPU1);
	UINT32 romlength = memory_region_length(REGION_CPU1);
	UINT32 offs;

	for (offs = 0; offs < romlength; offs++)
	{
		UINT8 data = rombase[offs];

		/* XOR bit 4 with bit 2, and bit 0 with bit 5, and invert bit 1 */
		data ^= BIT(data, 2) << 4;
		data ^= BIT(data, 5) << 0;
		data ^= 0x02;

		/* Swap bit0 with bit4 */
		if (offs & 0x02)
			data = BITSWAP8(data, 7,6,5,0,3,2,1,4);
		rombase[offs] = data;
	}
}


static void decode_frogger_sound(void)
{
	UINT8 *rombase = memory_region(REGION_CPU2);
	UINT32 offs;

	/* the first ROM of the sound CPU has data lines D0 and D1 swapped */
	for (offs = 0; offs < 0x0800; offs++)
		rombase[offs] = BITSWAP8(rombase[offs], 7,6,5,4,3,2,0,1);
}


static void decode_frogger_gfx(void)
{
	UINT8 *rombase = memory_region(REGION_GFX1);
	UINT32 offs;

	/* the 2nd gfx ROM has data lines D0 and D1 swapped */
	for (offs = 0x0800; offs < 0x1000; offs++)
		rombase[offs] = BITSWAP8(rombase[offs], 7,6,5,4,3,2,0,1);
}


static void decode_anteater_gfx(void)
{
	UINT32 romlength = memory_region_length(REGION_GFX1);
	UINT8 *rombase = memory_region(REGION_GFX1);
	UINT8 *scratch = malloc_or_die(romlength);
	UINT32 offs;

	memcpy(scratch, rombase, romlength);
	for (offs = 0; offs < romlength; offs++)
	{
		UINT32 srcoffs = offs & 0x9bf;
		srcoffs |= (BIT(offs,4) ^ BIT(offs,9) ^ (BIT(offs,2) & BIT(offs,10))) << 6;
		srcoffs |= (BIT(offs,2) ^ BIT(offs,10)) << 9;
		srcoffs |= (BIT(offs,0) ^ BIT(offs,6) ^ 1) << 10;
		rombase[offs] = scratch[srcoffs];
	}
	free(scratch);
}


static void decode_losttomb_gfx(void)
{
	UINT32 romlength = memory_region_length(REGION_GFX1);
	UINT8 *rombase = memory_region(REGION_GFX1);
	UINT8 *scratch = malloc_or_die(romlength);
	UINT32 offs;

	memcpy(scratch, rombase, romlength);
	for (offs = 0; offs < romlength; offs++)
	{
		UINT32 srcoffs = offs & 0xa7f;
		srcoffs |= ((BIT(offs,1) & BIT(offs,8)) | ((1 ^ BIT(offs,1)) & (BIT(offs,10)))) << 7;
		srcoffs |= (BIT(offs,7) ^ (BIT(offs,1) & (BIT(offs,7) ^ BIT(offs,10)))) << 8;
		srcoffs |= ((BIT(offs,1) & BIT(offs,7)) | ((1 ^ BIT(offs,1)) & (BIT(offs,8)))) << 10;
		rombase[offs] = scratch[srcoffs];
	}
	free(scratch);
}



/*************************************
 *
 *  Driver configuration
 *
 *************************************/

static void common_init(
	running_machine *machine,
	galaxian_draw_bullet_func draw_bullet,
	galaxian_draw_background_func draw_background,
	galaxian_extend_tile_info_func extend_tile_info,
	galaxian_extend_sprite_info_func extend_sprite_info)
{
	irq_line = INPUT_LINE_NMI;
	galaxian_frogger_adjust = FALSE;
	galaxian_sfx_tilemap = FALSE;
	galaxian_sprite_clip_start = 16;
	galaxian_sprite_clip_end = 255;
	galaxian_draw_bullet_ptr = (draw_bullet != NULL) ? draw_bullet : galaxian_draw_bullet;
	galaxian_draw_background_ptr = (draw_background != NULL) ? draw_background : galaxian_draw_background;
	galaxian_extend_tile_info_ptr = extend_tile_info;
	galaxian_extend_sprite_info_ptr = extend_sprite_info;
}


static void konami_common_init(
	running_machine *machine,
	galaxian_draw_bullet_func draw_bullet,
	galaxian_draw_background_func draw_background,
	galaxian_extend_tile_info_func extend_tile_info,
	galaxian_extend_sprite_info_func extend_sprite_info)
{
	/* basic configuration */
	common_init(machine, draw_bullet, draw_background, extend_tile_info, extend_sprite_info);

	/* configure Konami sound */
	ppi8255_init(&konami_ppi8255_intf);
}


static void unmap_galaxian_sound(offs_t base)
{
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, base + 0x0004, base + 0x0007, 0, 0x07f8, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, base + 0x0800, base + 0x0807, 0, 0x07f8, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, base + 0x1800, base + 0x1800, 0, 0x07ff, SMH_UNMAP);
}



/*************************************
 *
 *  Galaxian-derived games
 *
 *************************************/

static DRIVER_INIT( galaxian )
{
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, NULL, NULL);
}


static DRIVER_INIT( nolock )
{
	/* same as galaxian... */
	DRIVER_INIT_CALL(galaxian);

	/* ...but coin lockout disabled/disconnected */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, SMH_UNMAP);
}


static DRIVER_INIT( azurian )
{
	/* yellow bullets instead of white ones */
	common_init(machine, scramble_draw_bullet, galaxian_draw_background, NULL, NULL);

	/* coin lockout disabled */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, SMH_UNMAP);
}


static DRIVER_INIT( gmgalax )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, gmgalax_extend_tile_info, gmgalax_extend_sprite_info);

	/* ROM is banked */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_configure_bank(1, 0, 2, memory_region(REGION_CPU1) + 0x10000, 0x4000);

	/* callback when the game select is toggled */
	gmgalax_game_changed(machine, NULL, 0, 0);
	state_save_register_global(gmgalax_selected_game);
}


static DRIVER_INIT( pisces )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, pisces_extend_tile_info, pisces_extend_sprite_info);

	/* coin lockout replaced by graphics bank */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, galaxian_gfxbank_w);
}


static DRIVER_INIT( batman2 )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, batman2_extend_tile_info, upper_extend_sprite_info);

	/* coin lockout replaced by graphics bank */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, galaxian_gfxbank_w);
}


static DRIVER_INIT( frogg )
{
	/* same as galaxian... */
	common_init(machine, galaxian_draw_bullet, frogger_draw_background, frogger_extend_tile_info, frogger_extend_sprite_info);

	/* ...but needs a full 2k of RAM */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x47ff, 0, 0, SMH_BANK1, SMH_BANK1);
	memory_set_bankptr(1, auto_malloc(0x800));
}



/*************************************
 *
 *  Moon Cresta-derived games
 *
 *************************************/

static DRIVER_INIT( mooncrst )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, mooncrst_extend_tile_info, mooncrst_extend_sprite_info);

	/* decrypt program code */
	decode_mooncrst(0x8000, memory_region(REGION_CPU1));
}


static DRIVER_INIT( mooncrsu )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, mooncrst_extend_tile_info, mooncrst_extend_sprite_info);
}


static DRIVER_INIT( mooncrgx )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, mooncrst_extend_tile_info, mooncrst_extend_sprite_info);

	/* LEDs and coin lockout replaced by graphics banking */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x6002, 0, 0x7f8, galaxian_gfxbank_w);
}


static DRIVER_INIT( moonqsr )
{
	UINT8 *decrypt = auto_malloc(0x8000);

	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, moonqsr_extend_tile_info, moonqsr_extend_sprite_info);

	/* decrypt program code */
	decode_mooncrst(0x8000, decrypt);
	memory_set_decrypted_region(0, 0x0000, 0x7fff, decrypt);
}


static DRIVER_INIT( pacmanbl )
{
	/* same as galaxian... */
	DRIVER_INIT_CALL(galaxian);

	/* ...but coin lockout disabled/disconnected */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, SMH_UNMAP);

	/* also shift the sprite clip offset */
	galaxian_sprite_clip_start = 7;
	galaxian_sprite_clip_end = 246;
}


static DRIVER_INIT( devilfsg )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, NULL, NULL);

	/* IRQ line is INT, not NMI */
	irq_line = 0;
}


static DRIVER_INIT( zigzag )
{
	/* video extensions */
	common_init(machine, NULL, galaxian_draw_background, NULL, NULL);

	/* make ROMs 2 & 3 swappable */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x2fff, 0, 0, SMH_BANK1);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3fff, 0, 0, SMH_BANK2);
	memory_configure_bank(1, 0, 2, memory_region(REGION_CPU1) + 0x2000, 0x1000);
	memory_configure_bank(2, 0, 2, memory_region(REGION_CPU1) + 0x2000, 0x1000);

	/* handler for doing the swaps */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7002, 0x7002, 0, 0x07f8, zigzag_bankswap_w);
	zigzag_bankswap_w(machine, 0, 0);

	/* coin lockout disabled */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6002, 0x6002, 0, 0x7f8, SMH_UNMAP);

	/* remove the galaxian sound hardware */
	unmap_galaxian_sound(0x6000);

	/* install our AY-8910 handler */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4800, 0x4fff, 0, 0, zigzag_ay8910_w);
}


static DRIVER_INIT( jumpbug )
{
	/* video extensions */
	common_init(machine, scramble_draw_bullet, jumpbug_draw_background, jumpbug_extend_tile_info, jumpbug_extend_sprite_info);
}


static DRIVER_INIT( checkman )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, mooncrst_extend_tile_info, mooncrst_extend_sprite_info);

	/* move the interrupt enable from $b000 to $b001 */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xb000, 0, 0x7f8, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb001, 0xb001, 0, 0x7f8, irq_enable_w);

	/* attach the sound command handler */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x00, 0x00, 0, 0xffff, checkman_sound_command_w);

	/* decrypt program code */
	decode_checkman();
}


static DRIVER_INIT( checkmaj )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, NULL, NULL);

	/* attach the sound command handler */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7800, 0x7800, 0, 0x7ff, checkman_sound_command_w);

	/* for the title screen */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3800, 0x3800, 0, 0, checkmaj_protection_r);
}


static DRIVER_INIT( dingo )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, NULL, NULL);

	/* attach the sound command handler */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7800, 0x7800, 0, 0x7ff, checkman_sound_command_w);

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3000, 0, 0, dingo_3000_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3035, 0x3035, 0, 0, dingo_3035_r);
}


static DRIVER_INIT( dingoe )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, mooncrst_extend_tile_info, mooncrst_extend_sprite_info);

	/* move the interrupt enable from $b000 to $b001 */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xb000, 0, 0x7f8, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb001, 0xb001, 0, 0x7f8, irq_enable_w);

	/* attach the sound command handler */
	memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x00, 0x00, 0, 0xffff, checkman_sound_command_w);

	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x3001, 0x3001, 0, 0, dingoe_3001_r);	/* Protection check */

	/* decrypt program code */
	decode_dingoe();
}


static DRIVER_INIT( skybase )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, pisces_extend_tile_info, pisces_extend_sprite_info);

	/* coin lockout replaced by graphics bank */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa002, 0xa002, 0, 0x7f8, galaxian_gfxbank_w);

	/* needs a full 2k of RAM */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, SMH_BANK1, SMH_BANK1);
	memory_set_bankptr(1, auto_malloc(0x800));

	/* extend ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x5fff, 0, 0, SMH_BANK2);
	memory_set_bankptr(2, memory_region(REGION_CPU1));
}


static DRIVER_INIT( mshuttle )
{
	/* video extensions */
	common_init(machine, mshuttle_draw_bullet, galaxian_draw_background, mshuttle_extend_tile_info, mshuttle_extend_sprite_info);

	/* IRQ line is INT, not NMI */
	irq_line = 0;

	/* decrypt the code */
	mshuttle_decode();
}


static DRIVER_INIT( mshuttlj )
{
	/* video extensions */
	common_init(machine, mshuttle_draw_bullet, galaxian_draw_background, mshuttle_extend_tile_info, mshuttle_extend_sprite_info);

	/* IRQ line is INT, not NMI */
	irq_line = 0;

	/* decrypt the code */
	cclimbrj_decode();
}


static DRIVER_INIT( kingball )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, NULL, NULL);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xb000, 0, 0x7f8, kingball_sound1_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb001, 0xb001, 0, 0x7f8, irq_enable_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb002, 0xb002, 0, 0x7f8, kingball_sound2_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb003, 0xb003, 0, 0x7f8, kingball_speech_dip_w);

	state_save_register_global(kingball_speech_dip);
	state_save_register_global(kingball_sound);
}


static DRIVER_INIT( scorpnmc )
{
	/* video extensions */
	common_init(machine, galaxian_draw_bullet, galaxian_draw_background, batman2_extend_tile_info, upper_extend_sprite_info);

	/* move the interrupt enable from $b000 to $b001 */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb000, 0xb000, 0, 0x7f8, SMH_UNMAP);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb001, 0xb001, 0, 0x7f8, irq_enable_w);

	/* extra ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5000, 0x67ff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x5000);

	/* install RAM at $4000-$4800 */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x47ff, 0, 0, SMH_BANK2, SMH_BANK2);
	memory_set_bankptr(2, auto_malloc(0x800));

	/* doesn't appear to use original RAM */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, SMH_UNMAP, SMH_UNMAP);
}



/*************************************
 *
 *  Konami games
 *
 *************************************/

static DRIVER_INIT( theend )
{
	/* video extensions */
	konami_common_init(machine, theend_draw_bullet, galaxian_draw_background, NULL, NULL);

	/* coin counter on the upper bit of port C */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6802, 0x6802, 0, 0x7f8, SMH_UNMAP);
	ppi8255_set_portCwrite(0, theend_coin_counter_w);
}


static DRIVER_INIT( scramble )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);

	/* configure protection */
	ppi8255_set_portCread (1, scramble_protection_r);
	ppi8255_set_portCwrite(1, scramble_protection_w);
}


static DRIVER_INIT( explorer )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);

	/* watchdog works for writes as well? (or is it just disabled?) */
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x7000, 0, 0x7ff, watchdog_reset_w);

	/* I/O appears to be direct, not via PPIs */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0xffff, 0, 0, SMH_UNMAP, SMH_UNMAP);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8000, 0, 0xffc, input_port_0_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8001, 0x8001, 0, 0xffc, input_port_1_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8002, 0x8002, 0, 0xffc, input_port_2_r);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8003, 0x8003, 0, 0xffc, input_port_3_r);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x8000, 0, 0xfff, soundlatch_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x9000, 0x9000, 0, 0xfff, explorer_sound_control_w);
}


static DRIVER_INIT( sfx )
{
	/* basic configuration */
	common_init(machine, scramble_draw_bullet, scramble_draw_background, upper_extend_tile_info, NULL);
	galaxian_sfx_tilemap = TRUE;

	/* sfx uses 3 x 8255, so we need a non-standard interface */
	ppi8255_init(&sfx_ppi8255_intf);

	/* sound board has space for extra ROM */
	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x0000, 0x3fff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU2));
}


static DRIVER_INIT( atlantis )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);

	/* watchdog is at $7800? (or is it just disabled?) */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7000, 0x7000, 0, 0x7ff, SMH_UNMAP);
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x7800, 0x7800, 0, 0x7ff, watchdog_reset_r);
}


static DRIVER_INIT( scobra )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);
}


static DRIVER_INIT( losttomb )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);

	/* decrypt */
	decode_losttomb_gfx();
}


static DRIVER_INIT( frogger )
{
	/* video extensions */
	konami_common_init(machine, NULL, frogger_draw_background, frogger_extend_tile_info, frogger_extend_sprite_info);
	galaxian_frogger_adjust = TRUE;

	/* decrypt */
	decode_frogger_sound();
	decode_frogger_gfx();
}


static DRIVER_INIT( froggrmc )
{
	/* video extensions */
	common_init(machine, NULL, frogger_draw_background, frogger_extend_tile_info, frogger_extend_sprite_info);

	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa800, 0xa800, 0, 0x7ff, soundlatch_w);
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb001, 0xb001, 0, 0x7f8, froggrmc_sound_control_w);

	/* actually needs 2k of RAM */
	memory_install_readwrite8_handler(0, ADDRESS_SPACE_PROGRAM, 0x8000, 0x87ff, 0, 0, SMH_BANK1, SMH_BANK1);
	memory_set_bankptr(1, auto_malloc(0x800));

	/* decrypt */
	decode_frogger_sound();
}


static DRIVER_INIT( froggers )
{
	/* video extensions */
	konami_common_init(machine, NULL, frogger_draw_background, frogger_extend_tile_info, frogger_extend_sprite_info);

	/* decrypt */
	decode_frogger_sound();
}


static DRIVER_INIT( turtles )
{
	/* video extensions */
	konami_common_init(machine, NULL, turtles_draw_background, NULL, NULL);
}


#ifdef UNUSED_CODE
static DRIVER_INIT( amidar )
{
	/* no existing amidar sets run on Amidar hardware as described by Amidar schematics! */
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, amidar_draw_background, NULL, NULL);
}
#endif


static DRIVER_INIT( scorpion )
{
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, batman2_extend_tile_info, upper_extend_sprite_info);

	/* hook up AY8910 */
	memory_install_readwrite8_handler(1, ADDRESS_SPACE_IO, 0x00, 0xff, 0, 0, scorpion_ay8910_r, scorpion_ay8910_w);

	/* configure protection */
	ppi8255_set_portCwrite(1, scorpion_protection_w);
	ppi8255_set_portCread(1, scorpion_protection_r);

	/* extra ROM */
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0x5800, 0x67ff, 0, 0, SMH_BANK1);
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x5800);

	/* no background related */
//  memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0x6803, 0x6803, 0, 0, SMH_NOP);

	memory_install_read8_handler(1, ADDRESS_SPACE_PROGRAM, 0x3000, 0x3000, 0, 0, scorpion_sound_status_r);
/*
{
    const UINT8 *rom = memory_region(REGION_SOUND1);
    int i;

    for (i = 0; i < 0x2c; i++)
    {
        UINT16 addr = (rom[2*i] << 8) | rom[2*i+1];
        UINT16 endaddr = (rom[2*i+2] << 8) | rom[2*i+3];
        int j;
        printf("Cmd %02X -> %04X-%04X:", i, addr, endaddr - 1);
        for (j = 0; j < 32 && addr < endaddr; j++)
            printf(" %02X", rom[addr++]);
        printf("\n");
    }
}
*/
}


static DRIVER_INIT( anteater )
{
	/* video extensions */
	konami_common_init(machine, scramble_draw_bullet, scramble_draw_background, NULL, NULL);

	/* decode graphics */
	decode_anteater_gfx();
}


#include "galdrvr.c"
