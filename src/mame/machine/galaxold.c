/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "emu.h"
#include "machine/7474.h"
#include "includes/galaxold.h"


IRQ_CALLBACK_MEMBER(galaxold_state::hunchbkg_irq_callback)
{
	//galaxold_state *state = device->machine().driver_data<galaxold_state>();
	/* for some reason a call to cputag_set_input_line
	 * is significantly delayed ....
	 *
	 * state->m_maincpu->set_input_line(0, CLEAR_LINE);
	 *
	 * Therefore we reset the line without any detour ....
	 */
	device.machine().firstcpu->set_input_line(0, CLEAR_LINE);
	//cpu_set_info(device->machine().firstcpu, CPUINFO_INT_INPUT_STATE + state->m_irq_line, CLEAR_LINE);
	return 0x03;
}

/* FIXME: remove trampoline */
WRITE_LINE_MEMBER(galaxold_state::galaxold_7474_9m_2_q_callback)
{
	/* Q bar clocks the other flip-flop,
	   Q is VBLANK (not visible to the CPU) */
	downcast<ttl7474_device *>(machine().device("7474_9m_1"))->clock_w(state);
}

WRITE_LINE_MEMBER(galaxold_state::galaxold_7474_9m_1_callback)
{
	/* Q goes to the NMI line */
	m_maincpu->set_input_line(m_irq_line, state ? CLEAR_LINE : ASSERT_LINE);
}

WRITE8_MEMBER(galaxold_state::galaxold_nmi_enable_w)
{
	ttl7474_device *target = machine().device<ttl7474_device>("7474_9m_1");
	target->preset_w(data ? 1 : 0);
}


TIMER_DEVICE_CALLBACK_MEMBER(galaxold_state::galaxold_interrupt_timer)
{
	ttl7474_device *target = machine().device<ttl7474_device>("7474_9m_2");

	/* 128V, 64V and 32V go to D */
	target->d_w(((param & 0xe0) != 0xe0) ? 1 : 0);

	/* 16V clocks the flip-flop */
	target->clock_w(((param & 0x10) == 0x10) ? 1 : 0);

	param = (param + 0x10) & 0xff;

	timer.adjust(m_screen->time_until_pos(param), param);
}


void galaxold_state::machine_reset_common(int line)
{
	ttl7474_device *ttl7474_9m_1 = machine().device<ttl7474_device>("7474_9m_1");
	ttl7474_device *ttl7474_9m_2 = machine().device<ttl7474_device>("7474_9m_2");
	m_irq_line = line;

	/* initalize main CPU interrupt generator flip-flops */
	ttl7474_9m_2->preset_w(1);
	ttl7474_9m_2->clear_w (1);

	ttl7474_9m_1->clear_w (1);
	ttl7474_9m_1->d_w     (0);
	ttl7474_9m_1->preset_w(0);

	/* start a timer to generate interrupts */
	timer_device *int_timer = machine().device<timer_device>("int_timer");
	int_timer->adjust(m_screen->time_until_pos(0));
}

MACHINE_RESET_MEMBER(galaxold_state,galaxold)
{
	machine_reset_common(INPUT_LINE_NMI);
}

MACHINE_RESET_MEMBER(galaxold_state,devilfsg)
{
	machine_reset_common(0);
}

MACHINE_RESET_MEMBER(galaxold_state,hunchbkg)
{
	machine_reset_common(0);
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(galaxold_state::hunchbkg_irq_callback),this));
}

WRITE8_MEMBER(galaxold_state::galaxold_coin_lockout_w)
{
	coin_lockout_global_w(machine(), ~data & 1);
}


WRITE8_MEMBER(galaxold_state::galaxold_coin_counter_w)
{
	coin_counter_w(machine(), offset, data & 0x01);
}

WRITE8_MEMBER(galaxold_state::galaxold_coin_counter_1_w)
{
	coin_counter_w(machine(), 1, data & 0x01);
}

WRITE8_MEMBER(galaxold_state::galaxold_coin_counter_2_w)
{
	coin_counter_w(machine(), 2, data & 0x01);
}


WRITE8_MEMBER(galaxold_state::galaxold_leds_w)
{
	set_led_status(machine(), offset,data & 1);
}


#ifdef UNUSED_FUNCTION
READ8_MEMBER(galaxold_state::checkmaj_protection_r)
{
	switch (space.device().safe_pc())
	{
	case 0x0f15:  return 0xf5;
	case 0x0f8f:  return 0x7c;
	case 0x10b3:  return 0x7c;
	case 0x10e0:  return 0x00;
	case 0x10f1:  return 0xaa;
	case 0x1402:  return 0xaa;
	default:
		logerror("Unknown protection read. PC=%04X\n",space.device().safe_pc());
	}

	return 0;
}


/* Zig Zag can swap ROMs 2 and 3 as a form of copy protection */
WRITE8_MEMBER(galaxold_state::zigzag_sillyprotection_w)
{
	if (data)
	{
		/* swap ROM 2 and 3! */
		membank("bank1")->set_entry(1);
		membank("bank2")->set_entry(0);
	}
	else
	{
		membank("bank1")->set_entry(0);
		membank("bank2")->set_entry(1);
	}
}

DRIVER_INIT_MEMBER(galaxold_state,zigzag)
{
	UINT8 *RAM = memregion("maincpu")->base();
	membank("bank1")->configure_entries(0, 2, &RAM[0x2000], 0x1000);
	membank("bank2")->configure_entries(0, 2, &RAM[0x2000], 0x1000);
	membank("bank1")->set_entry(0);
	membank("bank2")->set_entry(1);
}



READ8_MEMBER(galaxold_state::dingo_3000_r)
{
	return 0xaa;
}

READ8_MEMBER(galaxold_state::dingo_3035_r)
{
	return 0x8c;
}

READ8_MEMBER(galaxold_state::dingoe_3001_r)
{
	return 0xaa;
}


DRIVER_INIT_MEMBER(galaxold_state,dingoe)
{
	offs_t i;
	UINT8 *rom = memregion("maincpu")->base();

	for (i = 0; i < 0x3000; i++)
	{
		UINT8 data_xor;

		/* XOR bit 2 with 4 and 5 with 0 */
		data_xor = BIT(rom[i], 2) << 4 | BIT(rom[i], 5) << 0;
		rom[i] ^= data_xor;


		/* Invert bit 1 */
		if (~rom[i] & 0x02)
			rom[i] = rom[i] | 0x02;
		else
			rom[i] = rom[i] & 0xfd;


		/* Swap bit0 with bit4 */
		if ((i & 0x0f) == 0x02 || (i & 0x0f) == 0x0a || (i & 0x0f) == 0x03 || (i & 0x0f) == 0x0b || (i & 0x0f) == 0x06 || (i & 0x0f) == 0x0e || (i & 0x0f) == 0x07 || (i & 0x0f) == 0x0f)   /* Swap Bit 0 and 4 */
			rom[i] = BITSWAP8(rom[i],7,6,5,0,3,2,1,4);
	}

	m_maincpu->space(AS_PROGRAM).install_legacy_read_handler(0x3001, 0x3001, FUNC(dingoe_3001_r));   /* Protection check */

}
#endif


READ8_MEMBER(galaxold_state::scramblb_protection_1_r)
{
	switch (space.device().safe_pc())
	{
	case 0x01da: return 0x80;
	case 0x01e4: return 0x00;
	default:
		logerror("%04x: read protection 1\n",space.device().safe_pc());
		return 0;
	}
}

READ8_MEMBER(galaxold_state::scramblb_protection_2_r)
{
	switch (space.device().safe_pc())
	{
	case 0x01ca: return 0x90;
	default:
		logerror("%04x: read protection 2\n",space.device().safe_pc());
		return 0;
	}
}


WRITE8_MEMBER(galaxold_state::_4in1_bank_w)
{
	m__4in1_bank = data & 0x03;
	galaxold_gfxbank_w(space, 0, m__4in1_bank);
	membank("bank1")->set_entry(m__4in1_bank);
}

CUSTOM_INPUT_MEMBER(galaxold_state::_4in1_fake_port_r)
{
	static const char *const portnames[] = { "FAKE1", "FAKE2", "FAKE3", "FAKE4" };
	int bit_mask = (FPTR)param;

	return (ioport(portnames[m__4in1_bank])->read() & bit_mask) ? 0x01 : 0x00;
}

#ifdef UNUSED_FUNCTION
DRIVER_INIT_MEMBER(galaxold_state,pisces)
{
	/* the coin lockout was replaced */
	m_maincpu->space(AS_PROGRAM).install_legacy_write_handler(0x6002, 0x6002, FUNC(galaxold_gfxbank_w));
}

DRIVER_INIT_MEMBER(galaxold_state,checkmaj)
{
	/* for the title screen */
	m_maincpu->space(AS_PROGRAM).install_legacy_read_handler(0x3800, 0x3800, FUNC(checkmaj_protection_r));
}

DRIVER_INIT_MEMBER(galaxold_state,dingo)
{
	m_maincpu->space(AS_PROGRAM).install_legacy_read_handler(0x3000, 0x3000, FUNC(dingo_3000_r));
	m_maincpu->space(AS_PROGRAM).install_legacy_read_handler(0x3035, 0x3035, FUNC(dingo_3035_r));
}


UINT8 galaxold_state::decode_mooncrst(UINT8 data,offs_t addr)
{
	UINT8 res;

	res = data;
	if (BIT(data,1)) res ^= 0x40;
	if (BIT(data,5)) res ^= 0x04;
	if ((addr & 1) == 0)
		res = (res & 0xbb) | (BIT(res,6) << 2) | (BIT(res,2) << 6);
	return res;
}

DRIVER_INIT_MEMBER(galaxold_state,mooncrsu)
{
	m_maincpu->space(AS_PROGRAM).install_legacy_write_handler(0xa000, 0xa002, FUNC(galaxold_gfxbank_w));
}

DRIVER_INIT_MEMBER(galaxold_state,mooncrst)
{
	offs_t i, len = memregion("maincpu")->bytes();
	UINT8 *rom = memregion("maincpu")->base();


	for (i = 0;i < len;i++)
		rom[i] = decode_mooncrst(rom[i],i);

	DRIVER_INIT_CALL(mooncrsu);
}

DRIVER_INIT_MEMBER(galaxold_state,mooncrgx)
{
	m_maincpu->space(AS_PROGRAM).install_legacy_write_handler(0x6000, 0x6002, FUNC(galaxold_gfxbank_w));
}

DRIVER_INIT_MEMBER(galaxold_state,moonqsr)
{
	offs_t i;
	address_space &space = m_maincpu->space(AS_PROGRAM);
	UINT8 *rom = memregion("maincpu")->base();
	UINT8 *decrypt = auto_alloc_array(machine(), UINT8, 0x8000);

	space.set_decrypted_region(0x0000, 0x7fff, decrypt);

	for (i = 0;i < 0x8000;i++)
		decrypt[i] = decode_mooncrst(rom[i],i);
}

DRIVER_INIT_MEMBER(galaxold_state,checkman)
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

	offs_t i, len = memregion("maincpu")->bytes();
	UINT8 *rom = memregion("maincpu")->base();


	for (i = 0; i < len; i++)
	{
		UINT8 data_xor;
		int line = i & 0x07;

		data_xor = (BIT(rom[i],xortable[line][0]) << xortable[line][1]) |
					(BIT(rom[i],xortable[line][2]) << xortable[line][3]);

		rom[i] ^= data_xor;
	}
}
#endif

DRIVER_INIT_MEMBER(galaxold_state,4in1)
{
	address_space &space = m_maincpu->space(AS_PROGRAM);
	offs_t i, len = memregion("maincpu")->bytes();
	UINT8 *RAM = memregion("maincpu")->base();

	/* Decrypt Program Roms */
	for (i = 0; i < len; i++)
		RAM[i] = RAM[i] ^ (i & 0xff);

	/* games are banked at 0x0000 - 0x3fff */
	membank("bank1")->configure_entries(0, 4, &RAM[0x10000], 0x4000);

	_4in1_bank_w(space, 0, 0); /* set the initial CPU bank */

	save_item(NAME(m__4in1_bank));
}

INTERRUPT_GEN_MEMBER(galaxold_state::hunchbks_vh_interrupt)
{
	generic_pulse_irq_line_and_vector(device.execute(),0,0x03,1);
}

DRIVER_INIT_MEMBER(galaxold_state,ladybugg)
{
	/* Doesn't actually use the bank, but it mustn't have a coin lock! */
	m_maincpu->space(AS_PROGRAM).install_write_handler(0x6002, 0x6002, write8_delegate(FUNC(galaxold_state::galaxold_gfxbank_w),this));
}

DRIVER_INIT_MEMBER(galaxold_state,bullsdrtg)
{
	int i;

	// patch char supposed to be space
	UINT8 *gfxrom = memregion("gfx1")->base();
	for (i = 0; i < 8; i++)
	{
		gfxrom[i] = 0;
	}

	// patch gfx for charset (seems to be 1bpp with bitplane data in correct rom)
	for (i = 0*8; i < 27*8; i++)
	{
		gfxrom[0x1000 + i] = 0;
	}

	// patch gfx for digits (seems to be 1bpp with bitplane data in correct rom)
	for (i = 48*8; i < (48+10)*8; i++ )
	{
		gfxrom[0x1000 + i] = 0;
	}
}
