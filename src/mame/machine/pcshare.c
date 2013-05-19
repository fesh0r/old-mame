/***************************************************************************

    machine/pcshare.c

    Functions to emulate general aspects of the machine
    (RAM, ROM, interrupts, I/O ports)

    The information herein is heavily based on
    'Ralph Browns Interrupt List'
    Release 52, Last Change 20oct96

    TODO:
    clean up (maybe split) the different pieces of hardware
    PIC, PIT, DMA... add support for LPT, COM (almost done)
    emulation of a serial mouse on a COM port (almost done)
    support for Game Controller port at 0x0201
    support for XT harddisk (under the way, see machine/pc_hdc.c)
    whatever 'hardware' comes to mind,
    maybe SoundBlaster? EGA? VGA?

***************************************************************************/

#include "emu.h"
#include "cpu/i386/i386.h"
#include "machine/pcshare.h"
#include "machine/pckeybrd.h"
#include "machine/8042kbdc.h"
#include "machine/mc146818.h"


/******************
DMA8237 Controller
******************/


WRITE_LINE_MEMBER( pcat_base_state::pc_dma_hrq_changed )
{
	m_maincpu->set_input_line(INPUT_LINE_HALT, state ? ASSERT_LINE : CLEAR_LINE);

	/* Assert HLDA */
	m_dma8237_1->i8237_hlda_w( state );
}


READ8_MEMBER(pcat_base_state::pc_dma_read_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM); // get the right address space

	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	return prog_space.read_byte(page_offset + offset);
}


WRITE8_MEMBER(pcat_base_state::pc_dma_write_byte)
{
	address_space& prog_space = m_maincpu->space(AS_PROGRAM); // get the right address space
	offs_t page_offset = (((offs_t) m_dma_offset[0][m_dma_channel]) << 16)
		& 0xFF0000;

	prog_space.write_byte(page_offset + offset, data);
}

READ8_MEMBER(pcat_base_state::dma_page_select_r)
{
	UINT8 data = m_at_pages[offset % 0x10];

	switch(offset % 8)
	{
	case 1:
		data = m_dma_offset[(offset / 8) & 1][2];
		break;
	case 2:
		data = m_dma_offset[(offset / 8) & 1][3];
		break;
	case 3:
		data = m_dma_offset[(offset / 8) & 1][1];
		break;
	case 7:
		data = m_dma_offset[(offset / 8) & 1][0];
		break;
	}
	return data;
}


WRITE8_MEMBER(pcat_base_state::dma_page_select_w)
{
	m_at_pages[offset % 0x10] = data;

	switch(offset % 8)
	{
	case 1:
		m_dma_offset[(offset / 8) & 1][2] = data;
		break;
	case 2:
		m_dma_offset[(offset / 8) & 1][3] = data;
		break;
	case 3:
		m_dma_offset[(offset / 8) & 1][1] = data;
		break;
	case 7:
		m_dma_offset[(offset / 8) & 1][0] = data;
		break;
	}
}

void pcat_base_state::set_dma_channel(int channel, int state)
{
	if (!state) m_dma_channel = channel;
}

WRITE_LINE_MEMBER( pcat_base_state::pc_dack0_w ) { set_dma_channel(0, state); }
WRITE_LINE_MEMBER( pcat_base_state::pc_dack1_w ) { set_dma_channel(1, state); }
WRITE_LINE_MEMBER( pcat_base_state::pc_dack2_w ) { set_dma_channel(2, state); }
WRITE_LINE_MEMBER( pcat_base_state::pc_dack3_w ) { set_dma_channel(3, state); }

static I8237_INTERFACE( dma8237_1_config )
{
	DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, pc_dma_hrq_changed),
	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pcat_base_state, pc_dma_read_byte),
	DEVCB_DRIVER_MEMBER(pcat_base_state, pc_dma_write_byte),
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, pc_dack0_w), DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, pc_dack1_w), DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, pc_dack2_w), DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, pc_dack3_w) }
};

static I8237_INTERFACE( dma8237_2_config )
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL },
	{ DEVCB_NULL, DEVCB_NULL, DEVCB_NULL, DEVCB_NULL }
};


/******************
8259 IRQ controller
******************/

READ8_MEMBER( pcat_base_state::get_slave_ack )
{
	if (offset==2) { // IRQ = 2
		return m_pic8259_2->acknowledge();
	}
	return 0x00;
}

IRQ_CALLBACK_MEMBER(pcat_base_state::irq_callback)
{
	return m_pic8259_1->acknowledge();
}

WRITE_LINE_MEMBER( pcat_base_state::at_pit8254_out0_changed )
{
	if (m_pic8259_1 )
	{
		m_pic8259_1->ir0_w(state);
	}
}


WRITE_LINE_MEMBER( pcat_base_state::at_pit8254_out2_changed )
{
	//at_speaker_set_input( state ? 1 : 0 );
}


static const struct pit8253_config at_pit8254_config =
{
	{
		{
			4772720/4,              /* heartbeat IRQ */
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, at_pit8254_out0_changed)
		}, {
			4772720/4,              /* dram refresh */
			DEVCB_NULL,
			DEVCB_NULL
		}, {
			4772720/4,              /* pio port c pin 4, and speaker polling enough */
			DEVCB_NULL,
			DEVCB_DRIVER_LINE_MEMBER(pcat_base_state, at_pit8254_out2_changed)
		}
	}
};

/*************************************************************
 *
 * Keyboard
 *
 *************************************************************/

READ8_MEMBER(pcat_base_state::get_out2)
{
	return pit8253_get_output( m_pit8254, 2 );
}

static const struct kbdc8042_interface at8042 =
{
	KBDC8042_AT386,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_RESET),
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_A20),
	DEVCB_DEVICE_LINE_MEMBER("pic8259_1", pic8259_device, ir1_w),
	DEVCB_NULL,

	DEVCB_NULL,
	DEVCB_DRIVER_MEMBER(pcat_base_state,get_out2)
};


ADDRESS_MAP_START( pcat32_io_common, AS_IO, 32, pcat_base_state )
	AM_RANGE(0x0000, 0x001f) AM_DEVREADWRITE8("dma8237_1", i8237_device, i8237_r, i8237_w, 0xffffffff)
	AM_RANGE(0x0020, 0x003f) AM_DEVREADWRITE8("pic8259_1", pic8259_device, read, write, 0xffffffff)
	AM_RANGE(0x0040, 0x005f) AM_DEVREADWRITE8_LEGACY("pit8254", pit8253_r, pit8253_w, 0xffffffff)
	AM_RANGE(0x0060, 0x006f) AM_DEVREADWRITE8("kbdc", kbdc8042_device, data_r, data_w, 0xffffffff)
	AM_RANGE(0x0070, 0x007f) AM_RAM //AM_DEVREADWRITE8("rtc", mc146818_device, read, write, 0xffffffff)
	AM_RANGE(0x0080, 0x009f) AM_READWRITE8(dma_page_select_r,dma_page_select_w, 0xffffffff)//TODO
	AM_RANGE(0x00a0, 0x00bf) AM_DEVREADWRITE8("pic8259_2", pic8259_device, read, write, 0xffffffff)
	AM_RANGE(0x00c0, 0x00df) AM_DEVREADWRITE8("dma8237_2", i8237_device, i8237_r, i8237_w, 0xffff)
ADDRESS_MAP_END

MACHINE_CONFIG_FRAGMENT(pcat_common)
	MCFG_PIC8259_ADD( "pic8259_1", INPUTLINE("maincpu", 0), VCC, READ8(pcat_base_state, get_slave_ack) )
	MCFG_PIC8259_ADD( "pic8259_2", DEVWRITELINE("pic8259_1", pic8259_device, ir2_w), GND, NULL )
	MCFG_I8237_ADD( "dma8237_1", XTAL_14_31818MHz/3, dma8237_1_config )
	MCFG_I8237_ADD( "dma8237_2", XTAL_14_31818MHz/3, dma8237_2_config )
	MCFG_PIT8254_ADD( "pit8254", at_pit8254_config )
//  MCFG_MC146818_ADD( "rtc", MC146818_STANDARD )

	MCFG_KBDC8042_ADD("kbdc", at8042)
MACHINE_CONFIG_END
