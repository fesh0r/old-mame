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
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( vic10_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( vic10_mem, AS_PROGRAM, 8, vic10_state )
	AM_RANGE(0x0000, 0x07ff) AM_RAM
	AM_RANGE(0x0800, 0x0fff) AM_DEVREADWRITE(VIC10_EXPANSION_SLOT_TAG, vic10_expansion_slot_device, exram_r, exram_w)
	AM_RANGE(0x8000, 0x9fff) AM_DEVREADWRITE(VIC10_EXPANSION_SLOT_TAG, vic10_expansion_slot_device, lorom_r, lorom_w)
	AM_RANGE(0xd000, 0xd3ff) AM_DEVREADWRITE_LEGACY(MOS6566_TAG, vic2_port_r, vic2_port_w)
	AM_RANGE(0xd400, 0xd7ff) AM_DEVREADWRITE_LEGACY(MOS6581_TAG, sid6581_r, sid6581_w)
	AM_RANGE(0xd800, 0xdbff) AM_RAM AM_BASE(m_color_ram)
	AM_RANGE(0xdc00, 0xdcff) AM_DEVREADWRITE_LEGACY(MOS6526_TAG, mos6526_r, mos6526_w)
	AM_RANGE(0xe000, 0xffff) AM_DEVREADWRITE(VIC10_EXPANSION_SLOT_TAG, vic10_expansion_slot_device, uprom_r, uprom_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( vic10 )
//-------------------------------------------------

static INPUT_PORTS_START( vic10 )
	PORT_INCLUDE( common_cbm_keyboard )		// ROW0 -> ROW7

	PORT_INCLUDE( c64_special )				// SPECIAL

	PORT_INCLUDE( c64_controls )			// CTRLSEL, JOY0, JOY1, PADDLE0 -> PADDLE3, TRACKX, TRACKY, LIGHTX, LIGHTY, OTHER
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  vic2_interface vic_intf
//-------------------------------------------------

static const unsigned char vic10_palette[] =
{
// black, white, red, cyan
// purple, green, blue, yellow
// orange, brown, light red, dark gray,
// medium gray, light green, light blue, light gray
// taken from the vice emulator
	0x00, 0x00, 0x00,  0xfd, 0xfe, 0xfc,  0xbe, 0x1a, 0x24,  0x30, 0xe6, 0xc6,
	0xb4, 0x1a, 0xe2,  0x1f, 0xd2, 0x1e,  0x21, 0x1b, 0xae,  0xdf, 0xf6, 0x0a,
	0xb8, 0x41, 0x04,  0x6a, 0x33, 0x04,  0xfe, 0x4a, 0x57,  0x42, 0x45, 0x40,
	0x70, 0x74, 0x6f,  0x59, 0xfe, 0x59,  0x5f, 0x53, 0xfe,  0xa4, 0xa7, 0xa2
};

static PALETTE_INIT( vic10 )
{
	int i;

	for (i = 0; i < sizeof(vic10_palette) / 3; i++)
	{
		palette_set_color_rgb(machine, i, vic10_palette[i * 3], vic10_palette[i * 3 + 1], vic10_palette[i * 3 + 2]);
	}
}

UINT32 vic10_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	vic2_video_update(m_vic, bitmap, cliprect);

	return 0;
}

static INTERRUPT_GEN( vic10_frame_interrupt )
{
	cbm_common_interrupt(device);
}

READ8_MEMBER( vic10_state::vic_lightpen_x_cb )
{
	return input_port_read(machine(), "LIGHTX") & ~0x01;
}

READ8_MEMBER( vic10_state::vic_lightpen_y_cb )
{
	return input_port_read(machine(), "LIGHTY") & ~0x01;
}

READ8_MEMBER( vic10_state::vic_lightpen_button_cb )
{
	return input_port_read(machine(), "OTHER") & 0x04;
}

READ8_MEMBER( vic10_state::vic_dma_read )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);

	if (offset < 0x3000)
		return program->read_byte(offset);

	return program->read_byte(0xe000 + (offset & 0x1fff));
}

READ8_MEMBER( vic10_state::vic_dma_read_color )
{
	return m_color_ram[offset & 0x3ff] & 0x0f;
}

WRITE_LINE_MEMBER( vic10_state::vic_irq_w )
{
	m_vic_irq = state;

	check_interrupts();
}

READ8_MEMBER( vic10_state::vic_rdy_cb )
{
	return input_port_read(machine(), "CYCLES") & 0x07;
}

static const vic2_interface vic_intf =
{
	SCREEN_TAG,
	M6510_TAG,
	VIC6567,
	DEVCB_DRIVER_MEMBER(vic10_state, vic_lightpen_x_cb),
	DEVCB_DRIVER_MEMBER(vic10_state, vic_lightpen_y_cb),
	DEVCB_DRIVER_MEMBER(vic10_state, vic_lightpen_button_cb),
	DEVCB_DRIVER_MEMBER(vic10_state, vic_dma_read),
	DEVCB_DRIVER_MEMBER(vic10_state, vic_dma_read_color),
	DEVCB_DRIVER_LINE_MEMBER(vic10_state, vic_irq_w),
	DEVCB_DRIVER_MEMBER(vic10_state, vic_rdy_cb)
};


//-------------------------------------------------
//  sid6581_interface sid_intf
//-------------------------------------------------

static int vic10_paddle_read( device_t *device, int which )
{
	running_machine &machine = device->machine();
	vic10_state *state = device->machine().driver_data<vic10_state>();

	int pot1 = 0xff, pot2 = 0xff, pot3 = 0xff, pot4 = 0xff, temp;
	UINT8 cia0porta = mos6526_pa_r(state->m_cia, 0);
	int controller1 = input_port_read(machine, "CTRLSEL") & 0x07;
	int controller2 = input_port_read(machine, "CTRLSEL") & 0x70;
	// Notice that only a single input is defined for Mouse & Lightpen in both ports
	switch (controller1)
	{
		case 0x01:
			if (which)
				pot2 = input_port_read(machine, "PADDLE2");
			else
				pot1 = input_port_read(machine, "PADDLE1");
			break;

		case 0x02:
			if (which)
				pot2 = input_port_read(machine, "TRACKY");
			else
				pot1 = input_port_read(machine, "TRACKX");
			break;

		case 0x03:
			if (which && (input_port_read(machine, "JOY1_2B") & 0x20))	// Joy1 Button 2
				pot1 = 0x00;
			break;

		case 0x04:
			if (which)
				pot2 = input_port_read(machine, "LIGHTY");
			else
				pot1 = input_port_read(machine, "LIGHTX");
			break;

		case 0x06:
			if (which && (input_port_read(machine, "OTHER") & 0x04))	// Lightpen Signal
				pot2 = 0x00;
			break;

		case 0x00:
		case 0x07:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	switch (controller2)
	{
		case 0x10:
			if (which)
				pot4 = input_port_read(machine, "PADDLE4");
			else
				pot3 = input_port_read(machine, "PADDLE3");
			break;

		case 0x20:
			if (which)
				pot4 = input_port_read(machine, "TRACKY");
			else
				pot3 = input_port_read(machine, "TRACKX");
			break;

		case 0x30:
			if (which && (input_port_read(machine, "JOY2_2B") & 0x20))	// Joy2 Button 2
				pot4 = 0x00;
			break;

		case 0x40:
			if (which)
				pot4 = input_port_read(machine, "LIGHTY");
			else
				pot3 = input_port_read(machine, "LIGHTX");
			break;

		case 0x60:
			if (which && (input_port_read(machine, "OTHER") & 0x04))	// Lightpen Signal
				pot4 = 0x00;
			break;

		case 0x00:
		case 0x70:
			break;

		default:
			logerror("Invalid Controller Setting %d\n", controller1);
			break;
	}

	if (input_port_read(machine, "CTRLSEL") & 0x80)		// Swap
	{
		temp = pot1; pot1 = pot3; pot3 = temp;
		temp = pot2; pot2 = pot4; pot4 = temp;
	}

	switch (cia0porta & 0xc0)
	{
		case 0x40:
			return which ? pot2 : pot1;

		case 0x80:
			return which ? pot4 : pot3;

		case 0xc0:
			return which ? pot2 : pot1;

		default:
			return 0;
	}
}

static const sid6581_interface sid_intf =
{
	vic10_paddle_read
};


//-------------------------------------------------
//  mos6526_interface cia_intf
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

	UINT8 cia0portb = mos6526_pb_r(m_cia, 0);

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

	UINT8 cia0porta = mos6526_pa_r(m_cia, 0);

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

	vic2_lightpen_write(m_vic, BIT(data, 4));
}

static const mos6526_interface cia_intf =
{
	10,
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
//  m6502_interface cpu_intf
//-------------------------------------------------

READ8_MEMBER( vic10_state::cpu_r )
{
	/*

        bit     description

        P0      EXPANSION PORT
        P1      1
        P2      1
        P3
        P4      CASS SENS
        P5

    */

	UINT8 data = 0x06;

	data |= m_exp->p0_r();

	data |= ((m_cassette->get_state() & CASSETTE_MASK_UISTATE) == CASSETTE_STOPPED) << 4;

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
	m_cassette->output(BIT(data, 3) ? -(0x5a9e >> 1) : +(0x5a9e >> 1));

	// cassette motor
	if (!BIT(data, 5))
	{
		m_cassette->change_state(CASSETTE_MOTOR_ENABLED, CASSETTE_MASK_MOTOR);
		m_cassette_timer->adjust(attotime::zero, 0, attotime::from_hz(44100));
	}
	else
	{
		m_cassette->change_state(CASSETTE_MOTOR_DISABLED, CASSETTE_MASK_MOTOR);
		m_cassette_timer->reset();
	}
}

static const m6502_interface cpu_intf =
{
	NULL,
	NULL,
	DEVCB_DRIVER_MEMBER(vic10_state, cpu_r),
	DEVCB_DRIVER_MEMBER(vic10_state, cpu_w)
};


//-------------------------------------------------
//  TIMER_DEVICE_CALLBACK( cassette_tick )
//-------------------------------------------------

static TIMER_DEVICE_CALLBACK( cassette_tick )
{
	vic10_state *state = timer.machine().driver_data<vic10_state>();

	mos6526_flag_w(state->m_cia, state->m_cassette->input() > +0.0);
}


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
	DEVCB_DEVICE_LINE(MOS6526_TAG, mos6526_sp_w),
	DEVCB_DEVICE_LINE(MOS6526_TAG, mos6526_cnt_w),
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
	// state saving
	save_item(NAME(m_cia_irq));
	save_item(NAME(m_vic_irq));
	save_item(NAME(m_exp_irq));
}


//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( vic10 )
//-------------------------------------------------

static MACHINE_CONFIG_START( vic10, vic10_state )
	// basic hardware
	MCFG_CPU_ADD(M6510_TAG, M6510, XTAL_8MHz/8)
	MCFG_CPU_PROGRAM_MAP(vic10_mem)
	MCFG_CPU_CONFIG(cpu_intf)
	MCFG_CPU_VBLANK_INT(SCREEN_TAG, vic10_frame_interrupt)
	MCFG_QUANTUM_TIME(attotime::from_hz(60))

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(VIC6567_VRETRACERATE)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(VIC6567_COLUMNS, VIC6567_LINES)
	MCFG_SCREEN_VISIBLE_AREA(0, VIC6567_VISIBLECOLUMNS - 1, 0, VIC6567_VISIBLELINES - 1)
	MCFG_SCREEN_UPDATE_DRIVER(vic10_state, screen_update)

	MCFG_PALETTE_INIT(vic10)
	MCFG_PALETTE_LENGTH(ARRAY_LENGTH(vic10_palette) / 3)

	MCFG_VIC2_ADD(MOS6566_TAG, vic_intf)

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(MOS6581_TAG, SID6581, XTAL_8MHz/8)
	MCFG_SOUND_CONFIG(sid_intf)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MCFG_SOUND_ADD("dac", DAC, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_MOS6526R1_ADD(MOS6526_TAG, XTAL_8MHz/8, cia_intf)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cbm_cassette_interface)
	MCFG_TIMER_ADD(TIMER_C1531_TAG, cassette_tick)
	MCFG_VIC10_EXPANSION_SLOT_ADD(VIC10_EXPANSION_SLOT_TAG, expansion_intf, vic10_expansion_cards, NULL, NULL)

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

COMP( 1982, vic10,		0,    0,    vic10, vic10,     0, "Commodore Business Machines", "VIC-10 / Max Machine / UltiMax (NTSC)", GAME_NOT_WORKING )
