/**********************************************************************

    Victor 9000 / ACT Sirius 1 emulation

    Copyright MESS Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

/*

    Sector format
    -------------

    Header sync
    Sector header (header ID, track ID, sector ID, and checksum)
    Gap 1
    Data Sync
    Data field (data sync, data ID, data bytes, and checksum)
    Gap 2

    Track format
    ------------

    ZONE        LOWER HEAD  UPPER HEAD  SECTORS     ROTATIONAL
    NUMBER      TRACKS      TRACKS      PER TRACK   PERIOD (MS)

    0           0-3         unused      19          237.9
    1           4-15        0-7         18          224.5
    2           16-26       8-18        17          212.2
    3           27-37       19-29       16          199.9
    4           38-48       30-40       15          187.6
    5           49-59       41-51       14          175.3
    6           60-70       52-62       13          163.0
    7           71-79       63-74       12          149.6
    8           unused      75-79       11          144.0

*/

/*

    TODO:

    - floppy 8048
    - keyboard
    - hires graphics
    - character attributes
    - brightness/contrast
    - MC6852
    - codec sound
    - hard disk (Tandon TM502, TM603SE)

*/

#include "includes/victor9k.h"



//**************************************************************************
//  READ/WRITE HANDLERS
//**************************************************************************

//-------------------------------------------------
//  floppy_p1_r -
//-------------------------------------------------

READ8_MEMBER( victor9k_state::floppy_p1_r )
{
	/*

	    bit     description

	    0       L0MS0
	    1       L0MS1
	    2       L0MS2
	    3       L0MS3
	    4       L1MS0
	    5       L1MS1
	    6       L1MS2
	    7       L1MS3

	*/

	return m_lms;
}


//-------------------------------------------------
//  floppy_p2_r -
//-------------------------------------------------

READ8_MEMBER( victor9k_state::floppy_p2_r )
{
	/*

	    bit     description

	    0
	    1
	    2
	    3
	    4
	    5
	    6       RDY0
	    7       RDY1

	*/

	UINT8 data = 0;

	data |= m_rdy0 << 6;
	data |= m_rdy1 << 7;

	return data;
}


//-------------------------------------------------
//  floppy_p2_w -
//-------------------------------------------------

WRITE8_MEMBER( victor9k_state::floppy_p2_w )
{
	/*

	    bit     description

	    0       START0
	    1       STOP0
	    2       START1
	    3       STOP1
	    4       SEL1
	    5       SEL0
	    6
	    7

	*/

	if (BIT(data, 0)) m_floppy0->mon_w(0);
	if (BIT(data, 1)) m_floppy0->mon_w(1);
	if (BIT(data, 2)) m_floppy1->mon_w(0);
	if (BIT(data, 3)) m_floppy1->mon_w(1);

	int sel0 = BIT(data, 5);

	if (m_sel0 && !sel0)
	{
		m_da0 = m_da;
		//m_floppy0->set_rpm();
	}

	m_sel0 = sel0;

	int sel1 = BIT(data, 4);

	if (m_sel1 && !sel1)
	{
		m_da1 = m_da;
		//m_floppy1->set_rpm();
	}

	m_sel1 = sel1;
}


//-------------------------------------------------
//  tach0_r -
//-------------------------------------------------

READ8_MEMBER( victor9k_state::tach0_r )
{
	return m_tach0;
}


//-------------------------------------------------
//  tach1_r -
//-------------------------------------------------

READ8_MEMBER( victor9k_state::tach1_r )
{
	return m_tach1;
}


//-------------------------------------------------
//  da_w -
//-------------------------------------------------

WRITE8_MEMBER( victor9k_state::da_w )
{
	m_da = data;
}



//**************************************************************************
//  ADDRESS MAPS
//**************************************************************************

//-------------------------------------------------
//  ADDRESS_MAP( victor9k_mem )
//-------------------------------------------------

static ADDRESS_MAP_START( victor9k_mem, AS_PROGRAM, 8, victor9k_state )
//  AM_RANGE(0x00000, 0xdffff) AM_RAM
	AM_RANGE(0xe0000, 0xe0001) AM_DEVREADWRITE(I8259A_TAG, pic8259_device, read, write)
	AM_RANGE(0xe0020, 0xe0023) AM_DEVREADWRITE(I8253_TAG, pit8253_device, read, write)
	AM_RANGE(0xe0040, 0xe0043) AM_DEVREADWRITE(UPD7201_TAG, upd7201_device, cd_ba_r, cd_ba_w)
	AM_RANGE(0xe8000, 0xe8000) AM_DEVREADWRITE(HD46505S_TAG, mc6845_device, status_r, address_w)
	AM_RANGE(0xe8001, 0xe8001) AM_DEVREADWRITE(HD46505S_TAG, mc6845_device, register_r, register_w)
	AM_RANGE(0xe8020, 0xe802f) AM_DEVREADWRITE(M6522_1_TAG, via6522_device, read, write)
	AM_RANGE(0xe8040, 0xe804f) AM_DEVREADWRITE(M6522_2_TAG, via6522_device, read, write)
	AM_RANGE(0xe8060, 0xe8061) AM_DEVREADWRITE(MC6852_TAG, mc6852_device, read, write)
	AM_RANGE(0xe8080, 0xe808f) AM_DEVREADWRITE(M6522_3_TAG, via6522_device, read, write)
	AM_RANGE(0xe80a0, 0xe80af) AM_DEVREADWRITE(M6522_4_TAG, via6522_device, read, write)
	AM_RANGE(0xe80c0, 0xe80cf) AM_DEVREADWRITE(M6522_6_TAG, via6522_device, read, write)
	AM_RANGE(0xe80e0, 0xe80ef) AM_DEVREADWRITE(M6522_5_TAG, via6522_device, read, write)
	AM_RANGE(0xf0000, 0xf0fff) AM_MIRROR(0x1000) AM_RAM AM_SHARE("video_ram")
	AM_RANGE(0xfe000, 0xfffff) AM_ROM AM_REGION(I8088_TAG, 0)
ADDRESS_MAP_END


//-------------------------------------------------
//  ADDRESS_MAP( floppy_io )
//-------------------------------------------------

static ADDRESS_MAP_START( floppy_io, AS_IO, 8, victor9k_state )
	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) AM_READ(floppy_p1_r) AM_WRITENOP
	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) AM_READWRITE(floppy_p2_r, floppy_p2_w)
	AM_RANGE(MCS48_PORT_T0, MCS48_PORT_T0) AM_READ(tach0_r)
	AM_RANGE(MCS48_PORT_T1, MCS48_PORT_T1) AM_READ(tach1_r)
	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) AM_WRITE(da_w)
ADDRESS_MAP_END



//**************************************************************************
//  INPUT PORTS
//**************************************************************************

//-------------------------------------------------
//  INPUT_PORTS( victor9k )
//-------------------------------------------------

static INPUT_PORTS_START( victor9k )
	// defined in machine/victor9kb.c
INPUT_PORTS_END



//**************************************************************************
//  DEVICE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MC6845_INTERFACE( hd46505s_intf )
//-------------------------------------------------

#define CODE_NON_DISPLAY    0x1000
#define CODE_UNDERLINE      0x2000
#define CODE_LOW_INTENSITY  0x4000
#define CODE_REVERSE_VIDEO  0x8000

static MC6845_UPDATE_ROW( victor9k_update_row )
{
	victor9k_state *state = device->machine().driver_data<victor9k_state>();
	address_space &program = state->m_maincpu->space(AS_PROGRAM);
	const rgb_t *palette = palette_entry_list_raw(bitmap.palette());

	if (BIT(ma, 13))
	{
		fatalerror("Graphics mode not supported!\n");
	}
	else
	{
		UINT16 video_ram_addr = (ma & 0xfff) << 1;

		for (int sx = 0; sx < x_count; sx++)
		{
			UINT16 code = (state->m_video_ram[video_ram_addr + 1] << 8) | state->m_video_ram[video_ram_addr];
			UINT32 char_ram_addr = (BIT(ma, 12) << 16) | ((code & 0xff) << 5) | (ra << 1);
			UINT16 data = program.read_word(char_ram_addr);

			for (int x = 0; x <= 10; x++)
			{
				int color = BIT(data, x);

				if (sx == cursor_x) color = 1;

				bitmap.pix32(y, x + sx*10) = palette[color];
			}

			video_ram_addr += 2;
			video_ram_addr &= 0xfff;
		}
	}
}

static MC6845_INTERFACE( hd46505s_intf )
{
	false,
	10,
	NULL,
	victor9k_update_row,
	NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(I8259A_TAG, pic8259_device, ir7_w),
	NULL
};


//-------------------------------------------------
//  pit8253_interface pit_intf
//-------------------------------------------------

WRITE_LINE_MEMBER(victor9k_state::mux_serial_b_w)
{
}

WRITE_LINE_MEMBER(victor9k_state::mux_serial_a_w)
{
}

static const struct pit8253_interface pit_intf =
{
	{
		{
			2500000,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(victor9k_state, mux_serial_b_w)
		}, {
			2500000,
			DEVCB_LINE_VCC,
			DEVCB_DRIVER_LINE_MEMBER(victor9k_state, mux_serial_a_w)
		}, {
			100000,
			DEVCB_LINE_VCC,
			DEVCB_DEVICE_LINE_MEMBER(I8259A_TAG, pic8259_device, ir2_w)
		}
	}
};



//-------------------------------------------------
//  PIC8259
//-------------------------------------------------

/*

    pin     signal      description

    IR0     SYN         sync detect
    IR1     COMM        serial communications (7201)
    IR2     TIMER       8253 timer
    IR3     PARALLEL    all 6522 IRQ (including disk)
    IR4     IR4         expansion IR4
    IR5     IR5         expansion IR5
    IR6     KBINT       keyboard data ready
    IR7     VINT        vertical sync or nonspecific interrupt

*/

IRQ_CALLBACK_MEMBER( victor9k_state::victor9k_irq_callback )
{
	return m_pic->inta_r();
}


//-------------------------------------------------
//  UPD7201_INTERFACE( mpsc_intf )
//-------------------------------------------------

static UPD7201_INTERFACE( mpsc_intf )
{
	0, 0, 0, 0,

	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, serial_port_device, rx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, serial_port_device, tx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, rs232_port_device, dtr_w),
	DEVCB_DEVICE_LINE_MEMBER(RS232_A_TAG, rs232_port_device, rts_w),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, serial_port_device, rx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, serial_port_device, tx),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, rs232_port_device, dtr_w),
	DEVCB_DEVICE_LINE_MEMBER(RS232_B_TAG, rs232_port_device, rts_w),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DEVICE_LINE_MEMBER(I8259A_TAG, pic8259_device, ir1_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  MC6852_INTERFACE( ssda_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( victor9k_state::ssda_irq_w )
{
	m_ssda_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static MC6852_INTERFACE( ssda_intf )
{
	0,
	0,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(HC55516_TAG, hc55516_device, digit_w),
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, ssda_irq_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};


//-------------------------------------------------
//  via6522_interface via1_intf
//-------------------------------------------------

WRITE8_MEMBER( victor9k_state::via1_pa_w )
{
	/*

	    bit     description

	    PA0     DIO1
	    PA1     DIO2
	    PA2     DIO3
	    PA3     DIO4
	    PA4     DIO5
	    PA5     DIO6
	    PA6     DIO7
	    PA7     DIO8

	*/

	m_ieee488->dio_w(data);
}

READ8_MEMBER( victor9k_state::via1_pb_r )
{
	/*

	    bit     description

	    PB0     DAV
	    PB1     EOI
	    PB2     REN
	    PB3     ATN
	    PB4     IFC
	    PB5     SRQ
	    PB6     NRFD
	    PB7     NDAC

	*/

	UINT8 data = 0;

	// IEEE-488
	data |= m_ieee488->dav_r();
	data |= m_ieee488->eoi_r() << 1;
	data |= m_ieee488->ren_r() << 2;
	data |= m_ieee488->atn_r() << 3;
	data |= m_ieee488->ifc_r() << 4;
	data |= m_ieee488->srq_r() << 5;
	data |= m_ieee488->nrfd_r() << 6;
	data |= m_ieee488->ndac_r() << 7;

	return data;
}

WRITE8_MEMBER( victor9k_state::via1_pb_w )
{
	/*

	    bit     description

	    PB0     DAV
	    PB1     EOI
	    PB2     REN
	    PB3     ATN
	    PB4     IFC
	    PB5     SRQ
	    PB6     NRFD
	    PB7     NDAC

	*/

	// IEEE-488
	m_ieee488->dav_w(BIT(data, 0));
	m_ieee488->eoi_w(BIT(data, 1));
	m_ieee488->ren_w(BIT(data, 2));
	m_ieee488->atn_w(BIT(data, 3));
	m_ieee488->ifc_w(BIT(data, 4));
	m_ieee488->srq_w(BIT(data, 5));
	m_ieee488->nrfd_w(BIT(data, 6));
	m_ieee488->ndac_w(BIT(data, 7));
}

WRITE_LINE_MEMBER( victor9k_state::codec_vol_w )
{
}

WRITE_LINE_MEMBER( victor9k_state::via1_irq_w )
{
	m_via1_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via1_intf =
{
	DEVCB_DEVICE_MEMBER(IEEE488_TAG, ieee488_device, dio_r),
	DEVCB_DRIVER_MEMBER(victor9k_state, via1_pb_r),
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, nrfd_r),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(IEEE488_TAG, ieee488_device, ndac_r),
	DEVCB_NULL,

	DEVCB_DRIVER_MEMBER(victor9k_state, via1_pa_w),
	DEVCB_DRIVER_MEMBER(victor9k_state, via1_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, codec_vol_w),

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via1_irq_w)
};


//-------------------------------------------------
//  via6522_interface via2_intf
//-------------------------------------------------

READ8_MEMBER( victor9k_state::via2_pa_r )
{
	/*

	    bit     description

	    PA0
	    PA1
	    PA2     RIA
	    PA3     DSRA
	    PA4     RIB
	    PA5     DSRB
	    PA6     KBDATA
	    PA7     VERT

	*/

	UINT8 data = 0;

	// serial
	data |= m_rs232a->ri_r() << 2;
	data |= m_rs232a->dsr_r() << 3;
	data |= m_rs232b->ri_r() << 4;
	data |= m_rs232b->dsr_r() << 5;

	// keyboard data
	data |= m_kb->kbdata_r() << 6;

	// vertical sync
	data |= m_crtc->vsync_r() << 7;

	return data;
}

WRITE8_MEMBER( victor9k_state::via2_pa_w )
{
	/*

	    bit     description

	    PA0     _INT/EXTA
	    PA1     _INT/EXTB
	    PA2
	    PA3
	    PA4
	    PA5
	    PA6
	    PA7

	*/
}

WRITE8_MEMBER( victor9k_state::via2_pb_w )
{
	/*

	    bit     description

	    PB0     TALK/LISTEN
	    PB1     KBACKCTL
	    PB2     BRT0
	    PB3     BRT1
	    PB4     BRT2
	    PB5     CONT0
	    PB6     CONT1
	    PB7     CONT2

	*/

	// keyboard acknowledge
	m_kb->kback_w(BIT(data, 1));

	// brightness
	m_brt = (data >> 2) & 0x07;

	// contrast
	m_cont = data >> 5;
}

WRITE_LINE_MEMBER( victor9k_state::via2_irq_w )
{
	m_via2_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via2_intf =
{
	DEVCB_DRIVER_MEMBER(victor9k_state, via2_pa_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(VICTOR9K_KEYBOARD_TAG, victor9k_keyboard_device, kbrdy_r),
	DEVCB_NULL, // SRQ/BUSY
	DEVCB_DEVICE_LINE_MEMBER(VICTOR9K_KEYBOARD_TAG, victor9k_keyboard_device, kbdata_r),

	DEVCB_DRIVER_MEMBER(victor9k_state, via2_pa_w),
	DEVCB_DRIVER_MEMBER(victor9k_state, via2_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via2_irq_w)
};


//-------------------------------------------------
//  via6522_interface via3_intf
//-------------------------------------------------

READ8_MEMBER( victor9k_state::via3_pa_r )
{
	/*

	    bit     description

	    PA0     J5-16
	    PA1     J5-18
	    PA2     J5-20
	    PA3     J5-22
	    PA4     J5-24
	    PA5     J5-26
	    PA6     J5-28
	    PA7     J5-30

	*/

	return 0;
}

WRITE8_MEMBER( victor9k_state::via3_pa_w )
{
	/*

	    bit     description

	    PA0     J5-16
	    PA1     J5-18
	    PA2     J5-20
	    PA3     J5-22
	    PA4     J5-24
	    PA5     J5-26
	    PA6     J5-28
	    PA7     J5-30

	*/
}

READ8_MEMBER( victor9k_state::via3_pb_r )
{
	/*

	    bit     description

	    PB0     J5-32
	    PB1     J5-34
	    PB2     J5-36
	    PB3     J5-38
	    PB4     J5-40
	    PB5     J5-42
	    PB6     J5-44
	    PB7     J5-46

	*/

	return 0;
}

WRITE8_MEMBER( victor9k_state::via3_pb_w )
{
	/*

	    bit     description

	    PB0     J5-32
	    PB1     J5-34
	    PB2     J5-36
	    PB3     J5-38
	    PB4     J5-40
	    PB5     J5-42
	    PB6     J5-44
	    PB7     J5-46

	*/

	// codec clock output
	m_ssda->rx_clk_w(BIT(data, 7));
	m_ssda->tx_clk_w(BIT(data, 7));
}

WRITE_LINE_MEMBER( victor9k_state::via3_irq_w )
{
	m_via3_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via3_intf =
{
	DEVCB_DRIVER_MEMBER(victor9k_state, via3_pa_r),
	DEVCB_DRIVER_MEMBER(victor9k_state, via3_pb_r),
	DEVCB_NULL, // J5-12
	DEVCB_NULL, // J5-48
	DEVCB_NULL, // J5-14
	DEVCB_NULL, // J5-50

	DEVCB_DRIVER_MEMBER(victor9k_state, via3_pa_w),
	DEVCB_DRIVER_MEMBER(victor9k_state, via3_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via3_irq_w)
};


//-------------------------------------------------
//  via6522_interface via4_intf
//-------------------------------------------------

WRITE8_MEMBER( victor9k_state::via4_pa_w )
{
	/*

	    bit     description

	    PA0     L0MS0
	    PA1     L0MS1
	    PA2     L0MS2
	    PA3     L0MS3
	    PA4     ST0A
	    PA5     ST0B
	    PA6     ST0C
	    PA7     ST0D

	*/

	m_lms = (m_lms & 0xf0) | (data & 0x0f);
	m_st[0] = data >> 4;
}

WRITE8_MEMBER( victor9k_state::via4_pb_w )
{
	/*

	    bit     description

	    PB0     L1MS0
	    PB1     L1MS1
	    PB2     L1MS2
	    PB3     L1MS3
	    PB4     ST1A
	    PB5     ST1B
	    PB6     ST1C
	    PB7     ST1D

	*/

	m_lms = (data << 4) | (m_lms & 0x0f);
	m_st[1] = data >> 4;
}

READ_LINE_MEMBER( victor9k_state::ds0_r )
{
	return m_ds0;
}

READ_LINE_MEMBER( victor9k_state::ds1_r )
{
	return m_ds1;
}

WRITE_LINE_MEMBER( victor9k_state::mode_w )
{
}

WRITE_LINE_MEMBER( victor9k_state::via4_irq_w )
{
	m_via4_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via4_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, ds0_r),
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, ds1_r),
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_MEMBER(victor9k_state, via4_pa_w),
	DEVCB_DRIVER_MEMBER(victor9k_state, via4_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, mode_w),
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via4_irq_w)
};


//-------------------------------------------------
//  via6522_interface via5_intf
//-------------------------------------------------

READ8_MEMBER( victor9k_state::via5_pa_r )
{
	/*

	    bit     description

	    PA0     E0
	    PA1     E1
	    PA2     I1
	    PA3     E2
	    PA4     E4
	    PA5     E5
	    PA6     I7
	    PA7     E6

	*/

	return 0;
}

WRITE8_MEMBER( victor9k_state::via5_pb_w )
{
	/*

	    bit     description

	    PB0     WD0
	    PB1     WD1
	    PB2     WD2
	    PB3     WD3
	    PB4     WD4
	    PB5     WD5
	    PB6     WD6
	    PB7     WD7

	*/
}

READ_LINE_MEMBER( victor9k_state::brdy_r )
{
	return m_brdy;
}

READ_LINE_MEMBER( victor9k_state::rdy0_r )
{
	return m_rdy0;
}

READ_LINE_MEMBER( victor9k_state::rdy1_r )
{
	return m_rdy1;
}

WRITE_LINE_MEMBER( victor9k_state::via5_irq_w )
{
	m_via5_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via5_intf =
{
	DEVCB_DRIVER_MEMBER(victor9k_state, via5_pa_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, brdy_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, rdy0_r),
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, rdy1_r),

	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(victor9k_state, via5_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via5_irq_w)
};


//-------------------------------------------------
//  via6522_interface via1_intf
//-------------------------------------------------

READ8_MEMBER( victor9k_state::via6_pa_r )
{
	/*

	    bit     description

	    PA0
	    PA1     _TRK0D0
	    PA2
	    PA3     _TRK0D1
	    PA4
	    PA5
	    PA6     WPS
	    PA7     _SYNC

	*/

	UINT8 data = 0;

	// track 0 drive A sense
	data |= m_floppy0->trk00_r() << 1;

	// track 0 drive B sense
	data |= m_floppy1->trk00_r() << 3;

	// write protect sense
	data |= (m_drive ? m_floppy1->wpt_r() : m_floppy0->wpt_r()) << 6;

	// disk sync detect
	data |= m_sync << 7;

	return data;
}

WRITE8_MEMBER( victor9k_state::via6_pa_w )
{
	/*

	    bit     description

	    PA0     LED0A
	    PA1
	    PA2     LED1A
	    PA3
	    PA4     SIDE SELECT
	    PA5     DRIVE SELECT
	    PA6
	    PA7

	*/

	// LED, drive A
	output_set_led_value(LED_A, BIT(data, 0));

	// LED, drive B
	output_set_led_value(LED_B, BIT(data, 2));

	// dual side select
	m_side = BIT(data, 4);

	// select drive A/B
	m_drive = BIT(data, 5);
}

READ8_MEMBER( victor9k_state::via6_pb_r )
{
	/*

	    bit     description

	    PB0     RDY0
	    PB1     RDY1
	    PB2
	    PB3     _DS1
	    PB4     _DS0
	    PB5     SINGLE/_DOUBLE SIDED
	    PB6
	    PB7

	*/

	UINT8 data = 0;

	// motor speed status, drive A
	data |= m_rdy0;

	// motor speed status, drive B
	data |= m_rdy1 << 1;

	// door B sense
	data |= m_ds1 << 3;

	// door A sense
	data |= m_ds0 << 4;

	// single/double sided
	data |= (m_drive ? m_floppy1->twosid_r() : m_floppy0->twosid_r()) << 5;

	return data;
}

WRITE8_MEMBER( victor9k_state::via6_pb_w )
{
	/*

	    bit     description

	    PB0
	    PB1
	    PB2     _SCRESET
	    PB3
	    PB4
	    PB5
	    PB6     STP0
	    PB7     STP1

	*/

	// motor speed controller reset
	if (!BIT(data, 2))
		m_fdc_cpu->reset();

	// stepper enable A
	m_stp[0] = BIT(data, 6);

	// stepper enable B
	m_stp[1] = BIT(data, 7);
}

READ_LINE_MEMBER( victor9k_state::gcrerr_r )
{
	return m_gcrerr;
}

WRITE_LINE_MEMBER( victor9k_state::drw_w )
{
}

WRITE_LINE_MEMBER( victor9k_state::erase_w )
{
}

WRITE_LINE_MEMBER( victor9k_state::via6_irq_w )
{
	m_via6_irq = state;

	m_pic->ir3_w(m_ssda_irq || m_via1_irq || m_via2_irq || m_via3_irq || m_via4_irq || m_via5_irq || m_via6_irq);
}

static const via6522_interface via6_intf =
{
	DEVCB_DRIVER_MEMBER(victor9k_state, via6_pa_r),
	DEVCB_DRIVER_MEMBER(victor9k_state, via6_pb_r),
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, gcrerr_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,

	DEVCB_DRIVER_MEMBER(victor9k_state, via6_pa_w),
	DEVCB_DRIVER_MEMBER(victor9k_state, via6_pb_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, drw_w),
	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, erase_w),

	DEVCB_DRIVER_LINE_MEMBER(victor9k_state, via6_irq_w)
};


//-------------------------------------------------
//  VICTOR9K_KEYBOARD_INTERFACE( kb_intf )
//-------------------------------------------------

WRITE_LINE_MEMBER( victor9k_state::kbrdy_w )
{
	m_via2->write_cb1(state);

	m_pic->ir6_w(state ? CLEAR_LINE : ASSERT_LINE);
}


//-------------------------------------------------
//  SLOT_INTERFACE( victor9k_floppies )
//-------------------------------------------------

void victor9k_state::ready0_cb(floppy_image_device *device, int state)
{
	m_rdy0 = state;

	m_via5->write_ca2(m_rdy0);
}

int victor9k_state::load0_cb(floppy_image_device *device)
{
	m_ds0 = 0;

	m_via4->write_ca1(m_ds0);

	return IMAGE_INIT_PASS;
}

void victor9k_state::unload0_cb(floppy_image_device *device)
{
	m_ds0 = 1;

	m_via4->write_ca1(m_ds0);
}

void victor9k_state::ready1_cb(floppy_image_device *device, int state)
{
	m_rdy1 = state;

	m_via5->write_cb2(m_rdy1);
}

int victor9k_state::load1_cb(floppy_image_device *device)
{
	m_ds1 = 0;

	m_via4->write_cb1(m_ds1);

	return IMAGE_INIT_PASS;
}

void victor9k_state::unload1_cb(floppy_image_device *device)
{
	m_ds1 = 1;

	m_via4->write_cb1(m_ds1);
}

static SLOT_INTERFACE_START( victor9k_floppies )
	SLOT_INTERFACE( "525qd", FLOPPY_525_QD )
SLOT_INTERFACE_END


//-------------------------------------------------
//  rs232_port_interface rs232a_intf
//-------------------------------------------------

static const rs232_port_interface rs232a_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, dcda_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, ria_w),
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, ctsa_w)
};


//-------------------------------------------------
//  rs232_port_interface rs232b_intf
//-------------------------------------------------

static const rs232_port_interface rs232b_intf =
{
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, dcdb_w),
	DEVCB_NULL,
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, rib_w),
	DEVCB_DEVICE_LINE_MEMBER(UPD7201_TAG, z80dart_device, ctsb_w)
};



//**************************************************************************
//  MACHINE INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_START( victor9k )
//-------------------------------------------------

void victor9k_state::machine_start()
{
	// set interrupt callback
	m_maincpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(victor9k_state::victor9k_irq_callback),this));

	// set floppy callbacks
	m_floppy0->setup_ready_cb(floppy_image_device::ready_cb(FUNC(victor9k_state::ready0_cb), this));
	m_floppy0->setup_load_cb(floppy_image_device::load_cb(FUNC(victor9k_state::load0_cb), this));
	m_floppy0->setup_unload_cb(floppy_image_device::unload_cb(FUNC(victor9k_state::unload0_cb), this));
	m_floppy1->setup_ready_cb(floppy_image_device::ready_cb(FUNC(victor9k_state::ready1_cb), this));
	m_floppy1->setup_load_cb(floppy_image_device::load_cb(FUNC(victor9k_state::load1_cb), this));
	m_floppy1->setup_unload_cb(floppy_image_device::unload_cb(FUNC(victor9k_state::unload1_cb), this));

	// memory banking
	address_space &program = m_maincpu->space(AS_PROGRAM);
	program.install_ram(0x00000, m_ram->size() - 1, m_ram->pointer());
}



//**************************************************************************
//  MACHINE CONFIGURATION
//**************************************************************************

//-------------------------------------------------
//  MACHINE_CONFIG( victor9k )
//-------------------------------------------------

static MACHINE_CONFIG_START( victor9k, victor9k_state )
	// basic machine hardware
	MCFG_CPU_ADD(I8088_TAG, I8088, XTAL_30MHz/6)
	MCFG_CPU_PROGRAM_MAP(victor9k_mem)

	MCFG_CPU_ADD(I8048_TAG, I8048, XTAL_30MHz/6)
	MCFG_CPU_IO_MAP(floppy_io)

	// video hardware
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) // not accurate
	MCFG_SCREEN_UPDATE_DEVICE(HD46505S_TAG, hd6845_device, screen_update)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT_OVERRIDE(driver_device, monochrome_green)

	MCFG_MC6845_ADD(HD46505S_TAG, HD6845, SCREEN_TAG, 1000000, hd46505s_intf) // HD6845 == HD46505S

	// sound hardware
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(HC55516_TAG, HC55516, 100000)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	// devices
	MCFG_IEEE488_BUS_ADD()
	MCFG_IEEE488_NRFD_CALLBACK(DEVWRITELINE(M6522_1_TAG, via6522_device, write_ca1))
	MCFG_IEEE488_NDAC_CALLBACK(DEVWRITELINE(M6522_1_TAG, via6522_device, write_ca2))
	MCFG_PIC8259_ADD(I8259A_TAG, INPUTLINE(I8088_TAG, INPUT_LINE_IRQ0), VCC, NULL)
	MCFG_PIT8253_ADD(I8253_TAG, pit_intf)
	MCFG_UPD7201_ADD(UPD7201_TAG, XTAL_30MHz/30, mpsc_intf)
	MCFG_MC6852_ADD(MC6852_TAG, XTAL_30MHz/30, ssda_intf)
	MCFG_VIA6522_ADD(M6522_1_TAG, XTAL_30MHz/30, via1_intf)
	MCFG_VIA6522_ADD(M6522_2_TAG, XTAL_30MHz/30, via2_intf)
	MCFG_VIA6522_ADD(M6522_3_TAG, XTAL_30MHz/30, via3_intf)
	MCFG_VIA6522_ADD(M6522_4_TAG, XTAL_30MHz/30, via4_intf)
	MCFG_VIA6522_ADD(M6522_5_TAG, XTAL_30MHz/30, via5_intf)
	MCFG_VIA6522_ADD(M6522_6_TAG, XTAL_30MHz/30, via6_intf)
	MCFG_FLOPPY_DRIVE_ADD(I8048_TAG":0", victor9k_floppies, "525qd", floppy_image_device::default_floppy_formats)
	MCFG_FLOPPY_DRIVE_ADD(I8048_TAG":1", victor9k_floppies, "525qd", floppy_image_device::default_floppy_formats)
	MCFG_RS232_PORT_ADD(RS232_A_TAG, rs232a_intf, default_rs232_devices, NULL)
	MCFG_RS232_PORT_ADD(RS232_B_TAG, rs232b_intf, default_rs232_devices, NULL)
	MCFG_VICTOR9K_KEYBOARD_ADD(WRITELINE(victor9k_state, kbrdy_w))

	// internal ram
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("128K")
	MCFG_RAM_EXTRA_OPTIONS("256K,384K,512K,640K,768K,896K")

	// software list
	MCFG_SOFTWARE_LIST_ADD("flop_list", "victor9k_flop")
MACHINE_CONFIG_END



//**************************************************************************
//  ROMS
//**************************************************************************

//-------------------------------------------------
//  ROM( victor9k )
//-------------------------------------------------

ROM_START( victor9k )
	ROM_REGION( 0x2000, I8088_TAG, 0 )
	ROM_DEFAULT_BIOS( "univ" )
	ROM_SYSTEM_BIOS( 0, "old", "Older" )
	ROMX_LOAD( "102320.7j", 0x0000, 0x1000, CRC(3d615fd7) SHA1(b22f7e5d66404185395d8effbf57efded0079a92), ROM_BIOS(1) )
	ROMX_LOAD( "102322.8j", 0x1000, 0x1000, CRC(9209df0e) SHA1(3ee8e0c15186bbd5768b550ecc1fa3b6b1dbb928), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "univ", "Universal" )
	ROMX_LOAD( "v9000 univ. fe f3f7 13db.7j", 0x0000, 0x1000, CRC(25c7a59f) SHA1(8784e9aa7eb9439f81e18b8e223c94714e033911), ROM_BIOS(2) )
	ROMX_LOAD( "v9000 univ. ff f3f7 39fe.8j", 0x1000, 0x1000, CRC(496c7467) SHA1(eccf428f62ef94ab85f4a43ba59ae6a066244a66), ROM_BIOS(2) )

	ROM_REGION( 0x400, I8048_TAG, 0)
	ROM_LOAD( "36080.5d", 0x000, 0x400, CRC(9bf49f7d) SHA1(b3a11bb65105db66ae1985b6f482aab6ea1da38b) )

	ROM_REGION( 0x800, "gcr", 0 )
	ROM_LOAD( "100836-001.4k", 0x000, 0x800, CRC(adc601bd) SHA1(6eeff3d2063ae2d97452101aa61e27ef83a467e5) )
ROM_END



//**************************************************************************
//  SYSTEM DRIVERS
//**************************************************************************

//    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS
COMP( 1982, victor9k, 0,      0,      victor9k, victor9k, driver_device, 0,    "Victor Business Products", "Victor 9000",   GAME_NOT_WORKING )
