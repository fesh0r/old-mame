/*

    Amstrad PC1512

    TODO:

    - rewrite VDU

    http://git.redump.net/cgit.cgi/mess/commit/?id=94fd742c9f51970583806ed37c6b3d9815f73e1a
    http://www.retroisle.com/amstrad/pcs/OriginalDocs/techmanual.php#1.11.2.2
    http://www.reenigne.org/blog/crtc-emulation-for-mess/
    http://www.seasip.info/AmstradXT/pc1512kbd.html

*/

#include "includes/pc1512.h"



//**************************************************************************
//  SYSTEM STATUS REGISTER
//**************************************************************************

//-------------------------------------------------
//  system_r -
//-------------------------------------------------

READ8_MEMBER( pc1512_state::system_r )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 0:
		if (BIT(m_port61, 7))
		{
			/*

                bit     description

                0       1
                1       8087 NDP installed
                2       1
                3       1
                4       DDM0
                5       DDM1
                6       second floppy disk drive installed
                7       0

            */

			data = m_status1;
		}
		else
		{
			data = m_kbd;
			m_kb_bits = 0;
			pic8259_ir1_w(m_pic, CLEAR_LINE);
		}
		break;

	case 1:
		data = m_port61;
		break;

	case 2:
		/*

            bit     description

            0       RAM0 / RAM4
            1       RAM1
            2       RAM2
            3       RAM3
            4       undefined
            5       8253 PIT OUT2 output
            6       external parity error (I/OCHCK from expansion bus)
            7       on-board system RAM parity error

        */

		if (BIT(m_port61, 2))
			data = m_status2 & 0x0f;
		else
			data = m_status2 >> 4;

		data |= m_pit2 << 5;
		break;
	}

	return data;
}


//-------------------------------------------------
//  system_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1512_state::system_w )
{
	switch (offset)
	{
	case 1:
		/*

            bit     description

            0       8253 GATE 2 (speaker modulate)
            1       speaker drive
            2       enable port C LSB / disable MSB
            3       undefined
            4       disable parity checking of on-board RAM
            5       prevent external parity errors from causing NMI
            6       enable incoming Keyboard Clock
            7       enable Status-1/Disable Keyboard Code on Port A

        */

		m_port61 = data;

		pit8253_gate2_w(m_pit, BIT(data, 0));

		m_speaker_drive = BIT(data, 1);
		update_speaker();

		m_kb->data_w(BIT(data, 6));
		break;

	case 4:
		/*

            bit     description

            0
            1       PA1 - 8087 NDP installed
            2
            3
            4       PA4 - DDM0
            5       PA5 - DDM1
            6       PA6 - Second Floppy disk drive installed
            7

        */

		if (BIT(data, 7))
			m_status1 = data ^ 0x8d;
		else
			m_status1 = data;
		break;

	case 5:
		/*

            bit     description

            0       PC0 (LSB) - RAM0
            1       PC1 (LSB) - RAM1
            2       PC2 (LSB) - RAM2
            3       PC3 (LSB) - RAM3
            4       PC0 (MSB) - RAM4
            5       PC1 (MSB) - Undefined
            6       PC2 (MSB) - Undefined
            7       PC3 (MSB) - Undefined

        */

		m_status2 = data;
		break;

	case 6:
		machine_reset();
		break;
	}
}



//**************************************************************************
//  MOUSE
//**************************************************************************

//-------------------------------------------------
//  mouse_r -
//-------------------------------------------------

READ8_MEMBER( pc1512_state::mouse_r )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 0:
		data = m_mouse_x;
		break;

	case 2:
		data = m_mouse_y;
		break;
	}

	return data;
}


//-------------------------------------------------
//  mouse_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1512_state::mouse_w )
{
	switch (offset)
	{
	case 0:
		m_mouse_x = 0;
		break;

	case 2:
		m_mouse_y = 0;
		break;
	}
}



//**************************************************************************
//  DIRECT MEMORY ACCESS
//**************************************************************************

//-------------------------------------------------
//  dma_page_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1512_state::dma_page_w )
{
	/*

        bit     description

        0       Address bit A16
        1       Address bit A17
        2       Address bit A18
        3       Address bit A19
        4
        5
        6
        7

    */

	switch (offset)
	{
	case 1:
		m_dma_page[2] = data & 0x0f;
		break;

	case 2:
		m_dma_page[3] = data & 0x0f;
		break;

	case 3:
		m_dma_page[0] = m_dma_page[1] = data & 0x0f;
		break;
	}
}



//**************************************************************************
//  INTERRUPTS
//**************************************************************************

//-------------------------------------------------
//  nmi_mask_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1512_state::nmi_mask_w )
{
	m_nmi_enable = BIT(data, 7);
}



//**************************************************************************
//  PRINTER
//**************************************************************************

//-------------------------------------------------
//  printer_r -
//-------------------------------------------------

READ8_MEMBER( pc1512_state::printer_r )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 0:
		data = m_printer_data;
		break;

	case 1:
		/*

            bit     description

            0       LK1 fitted
            1       LK2 fitted
            2       LK3 fitted
            3       printer error
            4       printer selected
            5       paper out
            6       printer acknowledge
            7       printer busy

        */

		data |= input_port_read(machine(), "LK") & 0x07;

		data |= centronics_fault_r(m_centronics) << 3;
		data |= centronics_vcc_r(m_centronics) << 4;
		data |= centronics_pe_r(m_centronics) << 5;
		data |= centronics_ack_r(m_centronics) << 6;
		data |= centronics_busy_r(m_centronics) << 7;
		break;

	case 2:
		data = m_printer_control;
		break;
	}

	return data;
}


//-------------------------------------------------
//  printer_w -
//-------------------------------------------------

WRITE8_MEMBER( pc1512_state::printer_w )
{
	switch (offset)
	{
	case 0:
		m_printer_data = data;
		centronics_data_w(m_centronics, 0, data);
		break;

	case 2:
		/*

            bit     description

            0       Data Strobe
            1       Select Auto Feed
            2       Reset Printer
            3       Select Printer
            4       Enable Int on ACK
            5
            6
            7

        */

		m_printer_control = data;

		centronics_strobe_w(m_centronics, BIT(data, 0));
		centronics_autofeed_w(m_centronics, BIT(data, 1));
		centronics_init_w(m_centronics, BIT(data, 2));

		m_ack_int_enable = BIT(data, 4);
		update_ack();
		break;
	}
}



//**************************************************************************
//  FLOPPY
//**************************************************************************

//-------------------------------------------------
//  fdc_r -
//-------------------------------------------------

READ8_MEMBER( pc1512_state::fdc_r )
{
	UINT8 data = 0;

	switch (offset)
	{
	case 4:
		data = upd765_status_r(m_fdc, 0);
		break;

	case 5:
		data = upd765_data_r(m_fdc, 0);
		break;
	}

	return data;
}


//-------------------------------------------------
//  fdc_w -
//-------------------------------------------------

void pc1512_state::set_fdc_dsr(UINT8 data)
{
	/*

        bit     description

        0       Drive Select Bit 0 (DS0)
        1       Drive Select Bit 1 (DS1)
        2       765A reset
        3       Allow 765A FDC to interrupt and request DMA
        4       Switch motor(s) on and enable drive 0 selection
        5       Switch motor(s) on and enable drive 1 selection
        6
        7

    */

	m_fdc_dsr = data;

	m_nden = BIT(data, 3);
	update_fdc_int();
	update_fdc_drq();
	update_fdc_tc();

	upd765_reset_w(m_fdc, BIT(data, 2));

	floppy_mon_w(m_floppy0, BIT(data, 4) ? CLEAR_LINE : ASSERT_LINE);
	floppy_mon_w(m_floppy1, BIT(data, 5) ? CLEAR_LINE : ASSERT_LINE);
}

WRITE8_MEMBER( pc1512_state::fdc_w )
{
	switch (offset)
	{
	case 2:
		set_fdc_dsr(data);
		break;

	case 5:
		upd765_data_w(m_fdc, 0, data);
		break;
	}
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( pc1512_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( pc1512_mem, AS_PROGRAM, 16, pc1512_state )
	AM_RANGE(0x00000, 0x9ffff) AM_RAM
//  AM_RANGE(0xb8000, 0xbffff) AM_READWRITE(videoram_r, videoram_w)
	AM_RANGE(0xfc000, 0xfffff) AM_ROM AM_REGION(I8086_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( pc1512_io )
//-------------------------------------------------

static ADDRESS_MAP_START( pc1512_io, AS_IO, 16, pc1512_state )
	ADDRESS_MAP_GLOBAL_MASK(0x3ff)
	AM_RANGE(0x000, 0x00f) AM_DEVREADWRITE8_LEGACY(I8237A5_TAG, i8237_r, i8237_w, 0xffff)
	AM_RANGE(0x020, 0x021) AM_DEVREADWRITE8_LEGACY(I8259A2_TAG, pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x040, 0x043) AM_DEVREADWRITE8_LEGACY(I8253_TAG, pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x060, 0x06f) AM_READWRITE8(system_r, system_w, 0xffff)
	AM_RANGE(0x070, 0x071) AM_DEVREADWRITE8(MC146818_TAG, mc146818_device, read, write, 0xffff)
	AM_RANGE(0x078, 0x07f) AM_READWRITE8(mouse_r, mouse_w, 0xffff)
	AM_RANGE(0x080, 0x083) AM_WRITE8(dma_page_w, 0xffff)
	AM_RANGE(0x0a0, 0x0a1) AM_WRITE8(nmi_mask_w, 0xff00)
	AM_RANGE(0x378, 0x37b) AM_READWRITE8(printer_r, printer_w, 0xffff)
//  AM_RANGE(0x3d0, 0x3df) AM_READWRITE8(vdu_r, vdu_w, 0xffff)
	AM_RANGE(0x3f0, 0x3f7) AM_READWRITE8(fdc_r, fdc_w, 0xffff)
	AM_RANGE(0x3f8, 0x3ff) AM_DEVREADWRITE8_LEGACY(INS8250_TAG, ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_CHANGED( mouse_button_1_changed )
//-------------------------------------------------

static INPUT_CHANGED( mouse_button_1_changed )
{
	pc1512_state *state = field.machine().driver_data<pc1512_state>();

	state->m_kb->m1_w(newval);
}


//-------------------------------------------------
//  INPUT_CHANGED( mouse_button_2_changed )
//-------------------------------------------------

static INPUT_CHANGED( mouse_button_2_changed )
{
	pc1512_state *state = field.machine().driver_data<pc1512_state>();

	state->m_kb->m2_w(newval);
}


//-------------------------------------------------
//  INPUT_CHANGED( mouse_x_changed )
//-------------------------------------------------

static INPUT_CHANGED( mouse_x_changed )
{
	pc1512_state *state = field.machine().driver_data<pc1512_state>();

	if (newval > oldval)
		state->m_mouse_x++;
	else
		state->m_mouse_x--;
}


//-------------------------------------------------
//  INPUT_CHANGED( mouse_y_changed )
//-------------------------------------------------

static INPUT_CHANGED( mouse_y_changed )
{
	pc1512_state *state = field.machine().driver_data<pc1512_state>();

	if (newval > oldval)
		state->m_mouse_y++;
	else
		state->m_mouse_y--;
}


//-------------------------------------------------
//  INPUT_PORTS( pc1512 )
//-------------------------------------------------

static INPUT_PORTS_START( pc1512 )
	PORT_START("MOUSEB")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("Left Mouse Button") PORT_CODE(MOUSECODE_BUTTON1) PORT_CHANGED(mouse_button_1_changed, 0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME("Right Mouse Button") PORT_CODE(MOUSECODE_BUTTON2) PORT_CHANGED(mouse_button_2_changed, 0)

	PORT_START("MOUSEX")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1) PORT_CHANGED(mouse_x_changed, 0)

	PORT_START("MOUSEY")
	PORT_BIT( 0xff, 0x00, IPT_MOUSE_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(5) PORT_MINMAX(0, 255) PORT_PLAYER(1) PORT_CHANGED(mouse_y_changed, 0)

	PORT_START("LK")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Language ) )
	PORT_DIPSETTING(	0x07, DEF_STR( English ) )
	PORT_DIPSETTING(	0x06, DEF_STR( German ) )
	PORT_DIPSETTING(	0x05, DEF_STR( French ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) )
	PORT_DIPSETTING(	0x03, "Danish" )
	PORT_DIPSETTING(	0x02, "Swedish" )
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) )
	PORT_DIPSETTING(	0x00, "Diagnostic Mode" )
	PORT_DIPNAME( 0x08, 0x08, "Memory Size")
	PORT_DIPSETTING( 0x08, "512 KB" )
	PORT_DIPSETTING( 0x00, "640 KB" )
	PORT_DIPNAME( 0x10, 0x10, "ROM Size")
	PORT_DIPSETTING( 0x10, "16 KB" )
	PORT_DIPSETTING( 0x00, "32 KB" )
/*  PORT_DIPNAME( 0x60, 0x60, "Character Set")
    PORT_DIPSETTING( 0x60, "Default (Codepage 437)" )
    PORT_DIPSETTING( 0x40, "Portugese (Codepage 865)" )
    PORT_DIPSETTING( 0x20, "Norwegian (Codepage 860)" )
    PORT_DIPSETTING( 0x00, "Greek")*/
	PORT_DIPNAME( 0x80, 0x80, "Floppy Ready Line")
	PORT_DIPSETTING( 0x80, "Connected" )
	PORT_DIPSETTING( 0x00, "Not connected" )

	PORT_INCLUDE( pcvideo_pc1512 )
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  PC1512_KEYBOARD_INTERFACE( kb_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( pc1512_state::kbdata_w )
{
	m_kbdata = state;
}

WRITE_LINE_MEMBER( pc1512_state::kbclk_w )
{
	if (m_kbclk && !state)
	{
		m_kbd <<= 1;
		m_kbd |= m_kbdata;
		m_kb_bits++;

		if (m_kb_bits == 8)
		{
			pic8259_ir1_w(m_pic, ASSERT_LINE);
		}
	}

	m_kbclk = state;
}

static PC1512_KEYBOARD_INTERFACE( kb_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, kbdata_w),
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, kbclk_w)
};


//-------------------------------------------------
//  I8237_INTERFACE( dmac_intf )
//-------------------------------------------------

void pc1512_state::update_fdc_tc()
{
	if (m_nden)
		upd765_tc_w(m_fdc, m_neop);
	else
		upd765_tc_w(m_fdc, 1);
}

WRITE_LINE_MEMBER( pc1512_state::hrq_w )
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	i8237_hlda_w(m_dmac, state);
}

WRITE_LINE_MEMBER( pc1512_state::eop_w )
{
	m_neop = !state;
	update_fdc_tc();
}

READ8_MEMBER( pc1512_state::memr_r )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	offs_t page_offset = m_dma_page[m_dma_channel] << 16;

	return program->read_byte(page_offset + offset);
}

WRITE8_MEMBER( pc1512_state::memw_w )
{
	address_space *program = m_maincpu->memory().space(AS_PROGRAM);
	offs_t page_offset = m_dma_page[m_dma_channel] << 16;

	program->write_byte(page_offset + offset, data);
}

WRITE8_MEMBER( pc1512_state::dma0_w )
{
	m_dreq0 = 0;
	i8237_dreq0_w(m_dmac, m_dreq0);
}

READ8_MEMBER( pc1512_state::fdc_dack_r )
{
	UINT8 data = 0;

	if (m_nden) data = upd765_dack_r(m_fdc, 0);

	return data;
}

WRITE8_MEMBER( pc1512_state::fdc_dack_w )
{
	if (m_nden) upd765_dack_w(m_fdc, 0, data);
}

WRITE_LINE_MEMBER( pc1512_state::dack0_w )
{
	if (!state) m_dma_channel = 0;
}

WRITE_LINE_MEMBER( pc1512_state::dack1_w )
{
	if (!state) m_dma_channel = 1;
}

WRITE_LINE_MEMBER( pc1512_state::dack2_w )
{
	if (!state) m_dma_channel = 2;
}

WRITE_LINE_MEMBER( pc1512_state::dack3_w )
{
	if (!state) m_dma_channel = 3;
}

static I8237_INTERFACE( dmac_intf )
{
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, hrq_w),
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, eop_w),
	DEVCB_DRIVER_MEMBER(pc1512_state, memr_r),
	DEVCB_DRIVER_MEMBER(pc1512_state, memw_w),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_DRIVER_MEMBER(pc1512_state, fdc_dack_r), DEVCB_NULL },
	{ DEVCB_DRIVER_MEMBER(pc1512_state, dma0_w), DEVCB_NULL, DEVCB_DRIVER_MEMBER(pc1512_state, fdc_dack_w), DEVCB_NULL },
	{ DEVCB_DRIVER_LINE_MEMBER(pc1512_state, dack0_w), DEVCB_DRIVER_LINE_MEMBER(pc1512_state, dack1_w),
	  DEVCB_DRIVER_LINE_MEMBER(pc1512_state, dack2_w), DEVCB_DRIVER_LINE_MEMBER(pc1512_state, dack3_w) }
};


//-------------------------------------------------
//  pic8259_interface pic_intf
//-------------------------------------------------

static IRQ_CALLBACK( pc1512_irq_callback )
{
	pc1512_state *state = device->machine().driver_data<pc1512_state>();

	return pic8259_acknowledge(state->m_pic);
}

static const struct pic8259_interface pic_intf =
{
	DEVCB_CPU_INPUT_LINE(I8086_TAG, INPUT_LINE_IRQ0),
	DEVCB_LINE_VCC,
	DEVCB_NULL
};


//-------------------------------------------------
//  pit8253_config pit_intf
//-------------------------------------------------

void pc1512_state::update_speaker()
{
	speaker_level_w(m_speaker, m_speaker_drive & m_pit2);
}

WRITE_LINE_MEMBER( pc1512_state::pit1_w )
{
	if (!m_pit1 && state && !m_dreq0)
	{
		m_dreq0 = 1;
		i8237_dreq0_w(m_dmac, m_dreq0);
	}

	m_pit1 = state;
}

WRITE_LINE_MEMBER( pc1512_state::pit2_w )
{
	m_pit2 = state;
	update_speaker();
}

static const struct pit8253_config pit_intf =
{
	{
		{
			XTAL_28_63636MHz/24,
			DEVCB_LINE_VCC,
			DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir0_w)
		}, {
			XTAL_28_63636MHz/24,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(pc1512_state, pit1_w)
		}, {
			XTAL_28_63636MHz/24,
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(pc1512_state, pit2_w)
		}
	}
};


//-------------------------------------------------
//  mc146818_interface rtc_intf
//-------------------------------------------------

static const struct mc146818_interface rtc_intf =
{
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir2_w)
};


//-------------------------------------------------
//  upd765_interface fdc_intf
//-------------------------------------------------

void pc1512_state::update_fdc_int()
{
	if (m_nden)
		pic8259_ir6_w(m_pic, m_dint);
	else
		pic8259_ir6_w(m_pic, CLEAR_LINE);
}

void pc1512_state::update_fdc_drq()
{
	if (m_nden)
		i8237_dreq2_w(m_dmac, m_ddrq);
	else
		i8237_dreq2_w(m_dmac, CLEAR_LINE);
}

static const floppy_interface floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	LEGACY_FLOPPY_OPTIONS_NAME(pc),
	"floppy_5_25",
	NULL
};

WRITE_LINE_MEMBER( pc1512_state::fdc_int_w )
{
	m_dint = state;
	update_fdc_int();
}

WRITE_LINE_MEMBER( pc1512_state::fdc_drq_w )
{
	m_ddrq = state;
	update_fdc_drq();
}

static UPD765_GET_IMAGE( pc1512_fdc_get_image )
{
	pc1512_state *state = device->machine().driver_data<pc1512_state>();

	if (BIT(state->m_fdc_dsr, 0))
		return state->m_floppy1;
	else
		return state->m_floppy0;
}

static const upd765_interface fdc_intf =
{
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, fdc_int_w),
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, fdc_drq_w),
	pc1512_fdc_get_image,
	UPD765_RDY_PIN_NOT_CONNECTED,
	{ FLOPPY_0, FLOPPY_1, NULL, NULL }
};


//-------------------------------------------------
//  ins8250_interface uart_intf
//-------------------------------------------------

static const ins8250_interface uart_intf =
{
	XTAL_1_8432MHz,
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir4_w),
	NULL,
	NULL,
	NULL
};


//-------------------------------------------------
//  centronics_interface centronics_intf
//-------------------------------------------------

void pc1512_state::update_ack()
{
	if (m_ack_int_enable)
		pic8259_ir7_w(m_pic, m_ack);
	else
		pic8259_ir7_w(m_pic, CLEAR_LINE);
}

WRITE_LINE_MEMBER( pc1512_state::ack_w )
{
	m_ack = state;
	update_ack();
}

static const centronics_interface centronics_intf =
{
	1,
	DEVCB_DRIVER_LINE_MEMBER(pc1512_state, ack_w),
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  isa8bus_interface isabus_intf
//-------------------------------------------------

static SLOT_INTERFACE_START( pc1512_isa8_cards )
SLOT_INTERFACE_END

static const isa8bus_interface isabus_intf =
{
	// interrupts
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir2_w),
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir3_w),
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir4_w),
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir5_w),
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir6_w),
	DEVCB_DEVICE_LINE(I8259A2_TAG, pic8259_ir7_w),

	// dma request
	DEVCB_DEVICE_LINE(I8237A5_TAG, i8237_dreq1_w),
	DEVCB_DEVICE_LINE(I8237A5_TAG, i8237_dreq2_w),
	DEVCB_DEVICE_LINE(I8237A5_TAG, i8237_dreq3_w)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( pc1512 )
//-------------------------------------------------

void pc1512_state::machine_start()
{
	// register CPU IRQ callback
	device_set_irq_callback(m_maincpu, pc1512_irq_callback);

	// set RAM size
	size_t ram_size = ram_get_size(m_ram);

	if (ram_size < 640 * 1024)
	{
		address_space *program = m_maincpu->memory().space(AS_PROGRAM);
		program->unmap_readwrite(ram_size, 0x9ffff);
	}

	// state saving
	save_item(NAME(m_pit1));
	save_item(NAME(m_pit2));
	save_item(NAME(m_status1));
	save_item(NAME(m_status2));
	save_item(NAME(m_port61));
	save_item(NAME(m_nmi_enable));
	save_item(NAME(m_kbd));
	save_item(NAME(m_kb_bits));
	save_item(NAME(m_kbclk));
	save_item(NAME(m_kbdata));
	save_item(NAME(m_mouse_x));
	save_item(NAME(m_mouse_y));
	save_item(NAME(m_dma_page));
	save_item(NAME(m_dma_channel));
	save_item(NAME(m_dreq0));
	save_item(NAME(m_nden));
	save_item(NAME(m_dint));
	save_item(NAME(m_ddrq));
	save_item(NAME(m_fdc_dsr));
	save_item(NAME(m_neop));
	save_item(NAME(m_ack_int_enable));
	save_item(NAME(m_ack));
	save_item(NAME(m_printer_data));
	save_item(NAME(m_printer_control));
	save_item(NAME(m_toggle));
	save_item(NAME(m_speaker_drive));
}


//-------------------------------------------------
//  MACHINE_RESET( pc1512 )
//-------------------------------------------------

void pc1512_state::machine_reset()
{
	m_nmi_enable = 0;
	m_toggle = 0;
	m_kb_bits = 0;

	set_fdc_dsr(0);
}



//**************************************************************************
//  MACHINE DRIVERS
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( pc1512 )
//-------------------------------------------------

static MACHINE_CONFIG_START( pc1512, pc1512_state )
	MCFG_CPU_ADD(I8086_TAG, I8086, XTAL_24MHz/3)
	MCFG_CPU_PROGRAM_MAP(pc1512_mem)
	MCFG_CPU_IO_MAP(pc1512_io)

	// video
	MCFG_FRAGMENT_ADD( pcvideo_pc1512 )

	// sound
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEAKER_TAG, SPEAKER_SOUND, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	// devices
	MCFG_PC1512_KEYBOARD_ADD(kb_intf)
	MCFG_I8237_ADD(I8237A5_TAG, XTAL_24MHz/6, dmac_intf)
	MCFG_PIC8259_ADD(I8259A2_TAG, pic_intf)
	MCFG_PIT8253_ADD(I8253_TAG, pit_intf)
	MCFG_MC146818_IRQ_ADD(MC146818_TAG, MC146818_IGNORE_CENTURY, rtc_intf)
	MCFG_UPD765A_ADD(UPD765AC2_TAG, fdc_intf)
	MCFG_INS8250_ADD(INS8250_TAG, uart_intf)
	MCFG_CENTRONICS_ADD(CENTRONICS_TAG, centronics_intf)
	MCFG_LEGACY_FLOPPY_2_DRIVES_ADD(floppy_intf)

	// ISA8 bus
	MCFG_ISA8_BUS_ADD("isa", I8086_TAG, isabus_intf)
	MCFG_ISA8_SLOT_ADD("isa", "isa1", pc1512_isa8_cards, NULL, NULL)
	MCFG_ISA8_SLOT_ADD("isa", "isa2", pc1512_isa8_cards, NULL, NULL)
	MCFG_ISA8_SLOT_ADD("isa", "isa3", pc1512_isa8_cards, NULL, NULL)

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("512K")
	MCFG_RAM_EXTRA_OPTIONS("544K,576K,608K,640K")

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "pc1512")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( pc1512 )
//-------------------------------------------------

ROM_START( pc1512 )
	ROM_REGION16_LE( 0x4000, I8086_TAG, 0)
	ROM_LOAD16_BYTE( "40043.ic129", 0x0000, 0x2000, CRC(f72f1582) SHA1(7781d4717917262805d514b331ba113b1e05a247) )
	ROM_LOAD16_BYTE( "40044.ic132", 0x0001, 0x2000, CRC(668fcc94) SHA1(74002f5cc542df442eec9e2e7a18db3598d8c482) )

	ROM_REGION( 0x08100, "gfx1", 0 )
	ROM_LOAD( "40045.ic127", 0x00000, 0x02000, CRC(dd5e030f) SHA1(7d858bbb2e8d6143aa67ab712edf5f753c2788a7) )
ROM_END


//-------------------------------------------------
//  ROM( pc1512v2 )
//-------------------------------------------------

ROM_START( pc1512v2 )
	ROM_REGION16_LE( 0x4000, I8086_TAG, 0)
	ROM_LOAD16_BYTE( "40043v2.ic129", 0x0000, 0x2000, CRC(1aec54fa) SHA1(b12fd73cfc35a240ed6da4dcc4b6c9910be611e0) )
	ROM_LOAD16_BYTE( "40044v2.ic132", 0x0001, 0x2000, CRC(d2d4d2de) SHA1(c376fd1ad23025081ae16c7949e88eea7f56e1bb) )

	ROM_REGION( 0x08100, "gfx1", 0 )
	ROM_LOAD( "40078.ic127", 0x00000, 0x02000, CRC(ae9c0d04) SHA1(bc8dc4dcedeea5bc1c04986b1f105ad93cb2ebcd) )
ROM_END


//-------------------------------------------------
//  ROM( pc1512v3 )
//-------------------------------------------------

ROM_START( pc1512v3 )
	ROM_REGION16_LE( 0x4000, I8086_TAG, 0)
	ROM_LOAD16_BYTE( "40043-2.ic129", 0x0000, 0x2000, CRC(ea527e6e) SHA1(b77fa44767a71a0b321a88bb0a394f1125b7c220) )
	ROM_LOAD16_BYTE( "40044-2.ic130", 0x0001, 0x2000, CRC(532c3854) SHA1(18a17b710f9eb079d9d7216d07807030f904ceda) )

	ROM_REGION( 0x08100, "gfx1", 0 )
	ROM_LOAD( "40078.ic127", 0x00000, 0x02000, CRC(ae9c0d04) SHA1(bc8dc4dcedeea5bc1c04986b1f105ad93cb2ebcd) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME        PARENT      COMPAT  MACHINE     INPUT       INIT    COMPANY         FULLNAME        FLAGS
COMP( 1986, pc1512,		0,			0,		pc1512,		pc1512,		0,		"Amstrad plc",	"PC1512 (V1)",	GAME_IMPERFECT_GRAPHICS )
COMP( 1987, pc1512v2,	pc1512,		0,		pc1512,		pc1512,		0,		"Amstrad plc",	"PC1512 (V2)",	GAME_NOT_WORKING )
COMP( 1989, pc1512v3,	pc1512,		0,		pc1512,		pc1512,		0,		"Amstrad plc",	"PC1512 (V3)",	GAME_NOT_WORKING )
