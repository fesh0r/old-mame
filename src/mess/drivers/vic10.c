/*

    TODO:

    - memory mapping with PLA
    - PLA dump

*/



#include "includes/vic10.h"



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  check_interrupts -
//-------------------------------------------------

void vic10_state::check_interrupts()
{
	m_maincpu->set_input_line(INPUT_LINE_IRQ0, m_cia_irq | m_vic_irq | m_exp_irq);
}


//**************************************************************************
//  MEMORY MANAGEMENT
//**************************************************************************

//-------------------------------------------------
//  read -
//-------------------------------------------------

READ8_MEMBER( vic10_state::read )
{
	// TODO this is really handled by the PLA

	UINT8 data = m_vic->bus_r();
	int lorom = 1, uprom = 1, exram = 1;

	if (offset < 0x800)
	{
		data = m_ram->pointer()[offset];
	}
	else if (offset < 0x1000)
	{
		exram = 0;
	}
	else if (offset >= 0x8000 && offset < 0xa000)
	{
		lorom = 0;
	}
	else if (offset >= 0xd000 && offset < 0xd400)
	{
		data = m_vic->read(space, offset & 0x3f);
	}
	else if (offset >= 0xd400 && offset < 0xd800)
	{
		data = m_sid->read(space, offset & 0x1f);
	}
	else if (offset >= 0xd800 && offset < 0xdc00)
	{
		data = m_color_ram[offset & 0x3ff];
	}
	else if (offset >= 0xdc00 && offset < 0xe000)
	{
		data = m_cia->read(space, offset & 0x0f);
	}
	else if (offset >= 0xe000)
	{
		uprom = 0;
	}

	return m_exp->cd_r(space, offset, data, lorom, uprom, exram);
}


//-------------------------------------------------
//  write -
//-------------------------------------------------

WRITE8_MEMBER( vic10_state::write )
{
	// TODO this is really handled by the PLA

	int lorom = 1, uprom = 1, exram = 1;

	if (offset < 0x800)
	{
		m_ram->pointer()[offset] = data;
	}
	else if (offset < 0x1000)
	{
		exram = 0;
	}
	else if (offset >= 0xd000 && offset < 0xd400)
	{
		m_vic->write(space, offset & 0x3f, data);
	}
	else if (offset >= 0xd400 && offset < 0xd800)
	{
		m_sid->write(space, offset & 0x1f, data);
	}
	else if (offset >= 0xd800 && offset < 0xdc00)
	{
		m_color_ram[offset & 0x3ff] = data & 0x0f;
	}
	else if (offset >= 0xdc00 && offset < 0xe000)
	{
		m_cia->write(space, offset & 0x0f, data);
	}

	m_exp->cd_w(space, offset, data, lorom, uprom, exram);
}


//-------------------------------------------------
//  vic_videoram_r -
//-------------------------------------------------

READ8_MEMBER( vic10_state::vic_videoram_r )
{
	address_space &program = m_maincpu->space(AS_PROGRAM);

	if (offset < 0x3000)
		return program.read_byte(offset);

	return program.read_byte(0xe000 + (offset & 0x1fff));
}


//-------------------------------------------------
//  vic_colorram_r -
//-------------------------------------------------

READ8_MEMBER( vic10_state::vic_colorram_r )
{
	return m_color_ram[offset];
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( vic10_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( vic10_mem, AS_PROGRAM, 8, vic10_state )
	AM_RANGE(0x0000, 0xffff) AM_READWRITE(read, write)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( vic_videoram_map )
//-------------------------------------------------

static ADDRESS_MAP_START( vic_videoram_map, AS_0, 8, vic10_state )
	AM_RANGE(0x0000, 0x3fff) AM_READ(vic_videoram_r)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( vic_colorram_map )
//-------------------------------------------------

static ADDRESS_MAP_START( vic_colorram_map, AS_1, 8, vic10_state )
	AM_RANGE(0x000, 0x3ff) AM_READ(vic_colorram_r)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( vic10 )
//-------------------------------------------------

static INPUT_PORTS_START( vic10 )
	PORT_INCLUDE( common_cbm_keyboard )     // ROW0 -> ROW7

	PORT_INCLUDE( c64_special )             // SPECIAL

	PORT_INCLUDE( c64_controls )            // CTRLSEL, JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, LIGHTX, LIGHTY, OTHER
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  vic2_interface vic_intf
//-------------------------------------------------

INTERRUPT_GEN_MEMBER(vic10_state::vic10_frame_interrupt)
{
	cbm_common_interrupt(&device);
}

WRITE_LINE_MEMBER( vic10_state::vic_irq_w )
{
	m_vic_irq = state;

	check_interrupts();
}

static MOS6566_INTERFACE( vic_intf )
{
	SCREEN_TAG,
	M6510_TAG,
	DEVCB_DRIVER_LINE_MEMBER(vic10_state, vic_irq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  sid6581_interface sid_intf
//-------------------------------------------------

READ8_MEMBER( vic10_state::sid_potx_r )
{
	UINT8 data = 0xff;

	switch (m_cia->pa_r() >> 6)
	{
	case 1: data = m_joy1->pot_x_r(); break;
	case 2: data = m_joy2->pot_x_r(); break;
	case 3: break; // TODO pot1 and pot2 in series
	}

	return data;
}

READ8_MEMBER( vic10_state::sid_poty_r )
{
	UINT8 data = 0xff;

	switch (m_cia->pa_r() >> 6)
	{
	case 1: data = m_joy1->pot_y_r(); break;
	case 2: data = m_joy2->pot_y_r(); break;
	case 3: break; // TODO pot1 and pot2 in series
	}

	return data;
}

static MOS6581_INTERFACE( sid_intf )
{
	DEVCB_DRIVER_MEMBER(vic10_state, sid_potx_r),
	DEVCB_DRIVER_MEMBER(vic10_state, sid_poty_r)
};


//-------------------------------------------------
//  MOS6526_INTERFACE( cia_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( vic10_state::cia_irq_w )
{
	m_cia_irq = state;

	check_interrupts();
}

READ8_MEMBER( vic10_state::cia_pa_r )
{
	/*

	    bit     description

	    PA0     COL0, JOY B0
	    PA1     COL1, JOY B1
	    PA2     COL2, JOY B2
	    PA3     COL3, JOY B3
	    PA4     COL4, BTNB
	    PA5     COL5
	    PA6     COL6
	    PA7     COL7

	*/

	UINT8 cia0portb = m_cia->pb_r();

	return cbm_common_cia0_port_a_r(m_cia, cia0portb);
}

READ8_MEMBER( vic10_state::cia_pb_r )
{
	/*

	    bit     description

	    PB0     JOY A0
	    PB1     JOY A1
	    PB2     JOY A2
	    PB3     JOY A3
	    PB4     BTNA/_LP
	    PB5
	    PB6
	    PB7

	*/

	UINT8 cia0porta = m_cia->pa_r();

	return cbm_common_cia0_port_b_r(m_cia, cia0porta);
}

WRITE8_MEMBER( vic10_state::cia_pb_w )
{
	/*

	    bit     description

	    PB0     ROW0
	    PB1     ROW1
	    PB2     ROW2
	    PB3     ROW3
	    PB4     ROW4
	    PB5     ROW5
	    PB6     ROW6
	    PB7     ROW7

	*/

	m_vic->lp_w(BIT(data, 4));
}

static MOS6526_INTERFACE( cia_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(vic10_state, cia_irq_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(VIC10_EXPANSION_SLOT_TAG, vic10_expansion_slot_device, sp_w),
	DEVCB_DEVICE_LINE_MEMBER(VIC10_EXPANSION_SLOT_TAG, vic10_expansion_slot_device, cnt_w),
	DEVCB_DRIVER_MEMBER(vic10_state, cia_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(vic10_state, cia_pb_r),
	DEVCB_DRIVER_MEMBER(vic10_state, cia_pb_w)
};


//-------------------------------------------------
//  M6510_INTERFACE( cpu_intf )
//-------------------------------------------------

READ8_MEMBER( vic10_state::cpu_r )
{
	/*

	    bit     description

	    P0      EXPANSION PORT
	    P1
	    P2
	    P3
	    P4      CASS SENS
	    P5      0

	*/

	UINT8 data = 0;

	// expansion port
	data |= m_exp->p0_r();

	// cassette sense
	data |= m_cassette->sense_r() << 4;

	return data;
}

WRITE8_MEMBER( vic10_state::cpu_w )
{
	/*

	    bit     description

	    P0      EXPANSION PORT
	    P1
	    P2
	    P3      CASS WRT
	    P4
	    P5      CASS MOTOR

	*/

	if (BIT(offset, 0))
	{
		m_exp->p0_w(BIT(data, 0));
	}

	// cassette write
	m_cassette->write(BIT(data, 3));

	// cassette motor
	m_cassette->motor_w(BIT(data, 5));
}

//-------------------------------------------------
//  PET_DATASSETTE_PORT_INTERFACE( datassette_intf )
//-------------------------------------------------

static PET_DATASSETTE_PORT_INTERFACE( datassette_intf )
{
	DEVCB_DEVICE_LINE_MEMBER(MOS6526_TAG, mos6526_device, flag_w)
};


//-------------------------------------------------
//  C64_EXPANSION_INTERFACE( expansion_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( vic10_state::exp_irq_w )
{
	m_exp_irq = state;

	check_interrupts();
}

static VIC10_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(vic10_state, exp_irq_w),
	DEVCB_DEVICE_LINE_MEMBER(MOS6526_TAG, mos6526_device, sp_w),
	DEVCB_DEVICE_LINE_MEMBER(MOS6526_TAG, mos6526_device, cnt_w),
	DEVCB_CPU_INPUT_LINE(M6510_TAG, INPUT_LINE_RESET)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( vic10 )
//-------------------------------------------------

void vic10_state::machine_start()
{
	// allocate memory
	m_color_ram.allocate(0x400);

	// initialize memory
	UINT8 data = 0xff;

	for (offs_t offset = 0; offset < m_ram->size(); offset++)
	{
		m_ram->pointer()[offset] = data;
		if (!(offset % 64)) data ^= 0xff;
	}

	// state saving
	save_item(NAME(m_cia_irq));
	save_item(NAME(m_vic_irq));
	save_item(NAME(m_exp_irq));
}


//-------------------------------------------------
//  MACHINE_RESET( vic10 )
//-------------------------------------------------

void vic10_state::machine_reset()
{
	m_maincpu->reset();

	m_exp->reset();
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( vic10 )
//-------------------------------------------------

static MACHINE_CONFIG_START( vic10, vic10_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, VIC6566_CLOCK)
	MCFG_CPU_PROGRAM_MAP(vic10_mem)
	MCFG_M6510_PORT_CALLBACKS(READ8(vic10_state, cpu_r), WRITE8(vic10_state, cpu_w))
	MCFG_M6510_PORT_PULLS(0x10, 0x20)
	MCFG_CPU_VBLANK_INT_DRIVER(SCREEN_TAG, vic10_state,  vic10_frame_interrupt)
	MCFG_QUANTUM_PERFECT_CPU(M6510_TAG)

	// video hardware
	MCFG_MOS6566_ADD(MOS6566_TAG, SCREEN_TAG, VIC6566_CLOCK, vic_intf, vic_videoram_map, vic_colorram_map)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6581_TAG, SID6581, VIC6566_CLOCK)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526_ADD(MOS6526_TAG, VIC6566_CLOCK, 60, cia_intf)
	MCFG_PET_DATASSETTE_PORT_ADD(PET_DATASSETTE_PORT_TAG, datassette_intf, cbm_datassette_devices, NULL, NULL)
	MCFG_VCS_CONTROL_PORT_ADD(CONTROL1_TAG, vcs_control_port_devices, NULL, NULL)
	MCFG_VCS_CONTROL_PORT_ADD(CONTROL2_TAG, vcs_control_port_devices, NULL, NULL)
	MCFG_VIC10_EXPANSION_SLOT_ADD(VIC10_EXPANSION_SLOT_TAG, VIC6566_CLOCK, expansion_intf, vic10_expansion_cards, NULL, NULL)

	// software list
	MCFG_SOFTWARE_LIST_ADD("cart_list", "vic10")

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("4K")
MACHINE_CONFIG_END



//**************************************************************************
//  ROM DEFINITIONS
//**************************************************************************

ROM_START( vic10 )
	ROM_REGION( 0x100, "pla", 0 )
	ROM_LOAD( "6703.u4", 0x000, 0x100, NO_DUMP )
ROM_END



//**************************************************************************
//  GAME DRIVERS
//**************************************************************************

COMP( 1982, vic10,      0,    0,    vic10, vic10, driver_device,     0, "Commodore Business Machines", "VIC-10 / Max Machine / UltiMax (NTSC)", GAME_SUPPORTS_SAVE )
