/******************************************************************************

  Driver for the ES-2xx series electronic typewriters made by Nakajima.

Nakajima was the OEM manufacturer for a series of typewriters which were
sold by different brands around the world. The PCB layouts for these
machines are the same. The models differed in the amount of (static) RAM:
128KB or 256KB; and in the system rom (mainly only different language
support).


Model   |  SRAM | Language | Branded model
--------+-------+----------+----------------------
ES-210N | 128KB | German   | Walther ES-210
ES-220  | 256KB | English  | NTS DreamWriter T400
ES-210N | 128KB | Spanish  | Dator 3000
ES-250  | xxxKB | English  | NTS DreamWriter T200


The LCD is driven by 6x Sanyo LC7940 and 1x Sanyo LC7942.


The keyboard matrix:

   |         |       |        |        |    |    |    |         |
   |         |       |        |        |    |    |    |         |
-- LSHIFT --   ----- LEFT --- ENTER --   --   --   -- RSHIFT --   ---
   |         |       |        |        |    |    |    |         |
-- 3 ------- Q ----- W ------ E ------ S -- D --   -- 2 -------   ---
   |         |       |        |        |    |    |    |         |
-- 4 ------- Z ----- X ------ A ------ R -- F --   --   -------   ---
   |         |       |        |        |    |    |    |         |
--   ------- B ----- V ------ T ------ G -- C -- Y --   -------   ---
   |         |       |        |        |    |    |    |         |
-- CTRL ---- 1 ----- TAB ----   ------   --   --   -- CAPS ----   ---
   |         |       |        |        |    |    |    |         |
-- ALT ----- CAN --- SPACE --   ------ 5 --   --   -- ` -------   ---
   |         |       |        |        |    |    |    |         |
--   ------- INS --- RIGHT -- \ ------ H -- N -- / -- DOWN ---- 6 ---
   |         |       |        |        |    |    |    |         |
--   ------- ORGN -- UP ----- WP ----- M -- K -- U -- 7 ------- = ---
   |         |       |        |        |    |    |    |         |
--   ------- ] ----- [ ------ ' ------ J -- , -- I -- - ------- 8 ---
   |         |       |        |        |    |    |    |         |
--   ------- BACK -- P ------ ; ------ O -- . -- L -- 9 ------- 0 ---
   |         |       |        |        |    |    |    |         |
   |         |       |        |        |    |    |    |         |



NTS information from http://web.archive.org/web/19980205154137/nts.dreamwriter.com/dreamwt4.html:

File Management & Memory:

?? Uniquely name up to 128 files
?? Recall, rename or delete files
?? Copy files to and from PCMCIA Memory card
?? PCMCIA Memory expansion cards available for 60 or 250 pages of text
?? Working memory allows up to 20 pages of text (50KB) to be displayed
?? Storage memory allows up to 80 pages of text (128KB) in total
?? DreamLink software exports and imports documents in RTF retaining all
  formatting to Macintosh or Windows PC to all commonly used Word Processing programs
?? Transfer cable provided compatible to both Macintosh and Windows PC's.
?? T400 is field upgradeable to IR with the optional Infrared module.

Hardware:

?? LCD Screen displays 8 lines by 80 characters raised and tilted 30 degrees
?? Contrast Dial and feet adjust to user preference
?? Parallel and Serial ports( IR Upgrade Optional) for connectivity to printers, Macintosh and Windows PC's
?? Full size 64 key keyboard with color coded keys and quick reference menu bar
?? NiCad rechargeable batteries for up to 8 hours of continuous use prior to recharging
?? AC adapter for recharging batteries is lightweight and compact design
?? NEC V20HL 9.83 MHz processor for fast response time
?? Durable solid state construction weighing 2.2 lbs including battery pack
?? Dimensions approximately 11" wide by 8" long by 1" deep
?? FCC and CSA approved


I/O Map:

0000 - unknown
       0x00 written during boot sequence

0010 - unknown
       0x17 written during boot sequence

0011 - unknown
       0x1e written during boot sequence

0012 - unknown
       0x1f written during boot sequence

0013 - unknown
       0x1e written during boot sequence

0014 - unknown
       0x1d written during boot sequence

0015 - unknown
       0x1c written during boot sequence

0016 - unknown
       0x01 written during boot sequence

0017 - unknown
       0x00 written during boot sequence

0020 - unknown
       0x00 written during boot sequence

0030 - unknown
       Looking at code at C0769 bit 5 this seems to be used
       as some kind of clock for data that was written to
       I/O port 0040h.

0040 - unknown
       0xff written during boot sequence

0050 - counter low?
0051 - counter high?
0052 - counter enable/disable?

0060 - Irq enable/disable (?)
       0xff written at start of boot sequence
       0x7e written just before enabling interrupts

0061 - unknown
       0xFE written in irq 0xFB handler

0090 - Interrupt source clear
       b7 - 1 = clear interrupt source for irq vector 0xf8
       b6 - 1 = clear interrupt source for irq vector 0xf9
       b5 - 1 = clear interrupt source for irq vector 0xfa
       b4 - 1 = clear interrupt source for irq vector 0xfb
       b3 - 1 = clear interrupt source for irq vector 0xfc
       b2 - 1 = clear interrupt source for irq vector 0xfd
       b1 - 1 = clear interrupt source for irq vector 0xfe
       b0 - 1 = clear interrupt source for irq vector 0xff

00A0 - unknown
       Read during initial boot sequence, expects to have bit 3 set at least once durnig the boot sequence

00D0 - 00DC - Keyboard??

00DD - unknown
       0xf8 written during boot sequence

00DE - unknown
       0xf0 written during boot sequence


IRQ 0xF8:
The handler clears the irq active bit and copies a word from 6D79 to 6D85 and
from 6D7B to 6D87 then enters an endless loop (with interrupts disabled).
Perhaps power on/off related??


IRQ 0xF9: (T400: C049A)
Purpose unknown. IRQ handler clear bit 0 of 6DA9.


IRQ 0xFA: (T400: C04AE)
Purpose unknown. Expects 6D4F to be set up properly. Enables irq 0xFD. Reads
from input port 0xB0 and resets and sets bit 0 of output port 0x61.


IRQ 0xFB: (T400: C04D1)
Purpose unknown. Reads from input port 0xB0, sets bit 7 of 6D28 when
non-zero data was read.


IRQ 0xFC: (T400: C0550)
Purpose unknown. Reads from input port 0xC1 and 0xC0 and possibly outputs
something to output port 0xC1 depending on data read from 0xC1.


IRQ 0xFD: (T400: C0724)
Purpose unknown. Clears bit 3 of 70A5.


IRQ 0xFE: (T400: C0738)
Purpose unknown. Expects 6D4F to be set up properly. Enables irq 0xf9 or outputs
a byte to ports 0x40 and 0x30. No endless loop.


IRQ 0xFF:
This does some simple processing and then enters an endless loop (with interrupts
disabled). Perhaps power on/off related??

******************************************************************************/

#include "driver.h"
#include "cpu/nec/nec.h"
#include "sound/speaker.h"


typedef struct _nakajies_state nakajies_state;
struct _nakajies_state {
	/* Device lookups */
	const device_config *cpu;

	/* IRQ handling */
	UINT8	irq_enabled;
	UINT8	irq_active;
};


#define X301	19660000


static ADDRESS_MAP_START( nakajies210_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x1ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0 )
ADDRESS_MAP_END


static ADDRESS_MAP_START( nakajies220_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x3ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0 )
ADDRESS_MAP_END


static ADDRESS_MAP_START( nakajies250_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x00000, 0x3ffff ) AM_RAM
	AM_RANGE( 0x80000, 0xfffff ) AM_ROM AM_REGION( "bios", 0x80000 )
ADDRESS_MAP_END


/*********************************************
  IRQ Handling
*********************************************/

static void nakajies_update_irqs( running_machine *machine )
{
	nakajies_state *state = machine->driver_data;
	UINT8 irq = state->irq_enabled & state->irq_active;
	UINT8 vector = 0xff;

	logerror("nakajies_update_irqs: irq_enabled = %02x, irq_active = %02x\n", state->irq_enabled, state->irq_active );

	/* Assuming irq 0xFF has the highest priority and 0xF8 the lowest */
	while( vector >= 0xf8 && ! ( irq & 0x01 ) )
	{
		irq >>= 1;
		vector -= 1;
	}

	if ( vector >= 0xf8 )
	{
		cpu_set_input_line_and_vector( state->cpu, 0, ASSERT_LINE, vector );
	}
	else
	{
		cpu_set_input_line( state->cpu, 0, CLEAR_LINE );
	}
}


static READ8_HANDLER( irq_clear_r )
{
	return 0x00;
}


static WRITE8_HANDLER( irq_clear_w )
{
	nakajies_state *state = space->machine->driver_data;

	state->irq_active &= ~data;
	nakajies_update_irqs( space->machine );
}


static READ8_HANDLER( irq_enable_r )
{
	nakajies_state *state = space->machine->driver_data;

	return state->irq_enabled;
}


static WRITE8_HANDLER( irq_enable_w )
{
	nakajies_state *state = space->machine->driver_data;

	state->irq_enabled = data;
	nakajies_update_irqs( space->machine );
}


static READ8_HANDLER( unk_a0_r )
{
	return 0xff;
}


static ADDRESS_MAP_START( nakajies_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE( 0x0060, 0x0060 ) AM_READWRITE( irq_enable_r, irq_enable_w )
	AM_RANGE( 0x0090, 0x0090 ) AM_READWRITE( irq_clear_r, irq_clear_w )
	AM_RANGE( 0x00a0, 0x00a0 ) AM_READ( unk_a0_r )
ADDRESS_MAP_END


static INPUT_CHANGED( trigger_irq )
{
	nakajies_state *state = field->port->machine->driver_data;
	UINT8 irqs = input_port_read( field->port->machine, "debug" );

	state->irq_active |= irqs;
	nakajies_update_irqs( field->port->machine );
}


static INPUT_PORTS_START( nakajies )
	PORT_START( "debug" )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_1 ) PORT_NAME( "irq 0xff" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_2 ) PORT_NAME( "irq 0xfe" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_3 ) PORT_NAME( "irq 0xfd" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_4 ) PORT_NAME( "irq 0xfc" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_5 ) PORT_NAME( "irq 0xfb" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_6 ) PORT_NAME( "irq 0xfa" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_7 ) PORT_NAME( "irq 0xf9" ) PORT_CHANGED( trigger_irq, NULL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE( KEYCODE_8 ) PORT_NAME( "irq 0xf8" ) PORT_CHANGED( trigger_irq, NULL )
INPUT_PORTS_END


static MACHINE_RESET( nakajies )
{
	nakajies_state *state = machine->driver_data;

	state->cpu = devtag_get_device( machine, "v20hl" );
	state->irq_enabled = 0;
	state->irq_active = 0;
}


static MACHINE_DRIVER_START( nakajies210 )
	MDRV_CPU_ADD( "v20hl", V20, X301 / 2 )
	MDRV_CPU_PROGRAM_MAP( nakajies210_map)
	MDRV_CPU_IO_MAP( nakajies_io_map)

	MDRV_DRIVER_DATA( nakajies_state )

	MDRV_MACHINE_RESET( nakajies )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_REFRESH_RATE( 50 )	/* Wild guess */
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 80 * 6, 8 * 8 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 6 * 80 - 1, 0, 8 * 8 - 1 )
	MDRV_PALETTE_LENGTH( 2 )

	/* sound */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "speaker", SPEAKER, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nakajies220 )
	MDRV_CPU_ADD( "v20hl", V20, X301 / 2 )
	MDRV_CPU_PROGRAM_MAP( nakajies220_map)
	MDRV_CPU_IO_MAP( nakajies_io_map)

	MDRV_DRIVER_DATA( nakajies_state )

	MDRV_MACHINE_RESET( nakajies )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_REFRESH_RATE( 50 )	/* Wild guess */
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 80 * 6, 8 * 8 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 6 * 80 - 1, 0, 8 * 8 - 1 )
	MDRV_PALETTE_LENGTH( 2 )

	/* sound */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "speaker", SPEAKER, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nakajies250 )
	MDRV_CPU_ADD( "v20hl", V20, X301 / 2 )
	MDRV_CPU_PROGRAM_MAP( nakajies250_map)
	MDRV_CPU_IO_MAP( nakajies_io_map)

	MDRV_DRIVER_DATA( nakajies_state )

	MDRV_MACHINE_RESET( nakajies )

	MDRV_SCREEN_ADD( "screen", LCD )
	MDRV_SCREEN_REFRESH_RATE( 50 )  /* Wild guess */
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_SIZE( 80 * 6, 8 * 8 )
	MDRV_SCREEN_VISIBLE_AREA( 0, 6 * 80 - 1, 0, 8 * 8 - 1 )
	MDRV_PALETTE_LENGTH( 2 )

	/* sound */
	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( "speaker", SPEAKER, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END


ROM_START(drwrt400)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("t4_ir_2.1.ic303", 0x00000, 0x80000, CRC(f0f45fd2) SHA1(3b4d5722b3e32e202551a1be8ae36f34ad705ddd))
ROM_END


ROM_START(wales210)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("wales210.ic303", 0x00000, 0x80000, CRC(a8e8d991) SHA1(9a133b37b2fbf689ae1c7ab5c7f4e97cd33fd596))		/* 27c4001 */
ROM_END


ROM_START(dator3k)
	ROM_REGION( 0x80000, "bios", 0 )
	ROM_LOAD("dator3000.ic303", 0x00000, 0x80000, CRC(b67fffeb) SHA1(e48270d15bef9453bcb6ba8aa949fd2ab3feceed))
ROM_END


ROM_START(drwrt200)
	ROM_REGION( 0x100000, "bios", 0 )
	ROM_LOAD("drwrt200.bin", 0x000000, 0x100000, CRC(3c39483c) SHA1(48293e6bdb7e7322d76da7174b716243c0ab7c2c) )
ROM_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE      INPUT     INIT    CONFIG  COMPANY    FULLNAME            FLAGS */
COMP( 199?, wales210,        0, 0,      nakajies210, nakajies, 0,      0,      "Walther", "ES-210",           GAME_NOT_WORKING )	/* German */
COMP( 199?, dator3k,  wales210, 0,      nakajies210, nakajies, 0,      0,      "Dator",   "Dator 3000",       GAME_NOT_WORKING )	/* Spanish */
COMP( 1996, drwrt400, wales210, 0,      nakajies220, nakajies, 0,      0,      "NTS",     "DreamWriter T400", GAME_NOT_WORKING )	/* English */
COMP( 199?, drwrt200, wales210, 0,      nakajies250, nakajies, 0,      0,      "NTS",     "DreamWriter T200", GAME_NOT_WORKING )	/* English */



