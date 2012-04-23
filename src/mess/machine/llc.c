/***************************************************************************

        LLC driver by Miodrag Milanovic

        17/04/2009 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "cpu/z80/z80.h"
#include "includes/llc.h"
#include "machine/ram.h"



static READ8_DEVICE_HANDLER (llc1_port_b_r)
{
	llc_state *state = device->machine().driver_data<llc_state>();
	UINT8 retVal = 0;
	if (state->m_s_code!=0) {
		if (state->m_llc1_key_state==0) {
			state->m_llc1_key_state = 1;
			retVal = 0x5F;
		} else {
			if (state->m_llc1_key_state == 1) {
				state->m_llc1_key_state = 2;
				retVal = 0;
			} else {
				state->m_llc1_key_state = 0;
				retVal = state->m_s_code;
				state->m_s_code =0;
			}
		}
	} else {
		state->m_llc1_key_state = 0;
		retVal = 0;
	}
	return retVal;
}

static READ8_DEVICE_HANDLER (llc1_port_a_r)
{
	return 0;
}


Z80CTC_INTERFACE( llc1_ctc_intf )
{
	0,
	DEVCB_CPU_INPUT_LINE("maincpu", INPUT_LINE_IRQ0),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

Z80CTC_INTERFACE( llc2_ctc_intf )
{
	0,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static TIMER_CALLBACK(keyboard_callback)
{
	llc_state *state = machine.driver_data<llc_state>();
	int i,j;
	UINT8 c;
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4",
		"LINE5", "LINE6", "LINE7", "LINE8", "LINE9", "LINE10", "LINE11"
	};

	for(i = 0; i < 12; i++)
	{
		c = input_port_read(machine, keynames[i]);
		if (c != 0)
		{
			for(j = 0; j < 8; j++)
			{
				if (c == (1 << j))
				{
					state->m_s_code = j + i*8;
					break;
				}
			}
		}
	}
}

/* Driver initialization */
DRIVER_INIT(llc1)
{
}

MACHINE_RESET( llc1 )
{
}

MACHINE_START(llc1)
{
	machine.scheduler().timer_pulse(attotime::from_hz(5), FUNC(keyboard_callback));
}

DRIVER_INIT(llc2)
{
	llc_state *state = machine.driver_data<llc_state>();
	state->m_video_ram.set_target( machine.device<ram_device>(RAM_TAG)->pointer() + 0xc000,state->m_video_ram.bytes());
}

MACHINE_RESET( llc2 )
{
	llc_state *state = machine.driver_data<llc_state>();
	address_space *space = machine.device("maincpu")->memory().space(AS_PROGRAM);

	space->unmap_write(0x0000, 0x3fff);
	state->membank("bank1")->set_base(machine.root_device().memregion("maincpu")->base());

	space->unmap_write(0x4000, 0x5fff);
	state->membank("bank2")->set_base(machine.root_device().memregion("maincpu")->base() + 0x4000);

	space->unmap_write(0x6000, 0xbfff);
	state->membank("bank3")->set_base(machine.root_device().memregion("maincpu")->base() + 0x6000);

	space->install_write_bank(0xc000, 0xffff, "bank4");
	state->membank("bank4")->set_base(machine.device<ram_device>(RAM_TAG)->pointer() + 0xc000);

}

WRITE8_MEMBER(llc_state::llc2_rom_disable_w)
{
	address_space *mem_space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	UINT8 *ram = machine().device<ram_device>(RAM_TAG)->pointer();

	mem_space->install_write_bank(0x0000, 0xbfff, "bank1");
	membank("bank1")->set_base(ram);

	mem_space->install_write_bank(0x4000, 0x5fff, "bank2");
	membank("bank2")->set_base(ram + 0x4000);

	mem_space->install_write_bank(0x6000, 0xbfff, "bank3");
	membank("bank3")->set_base(ram + 0x6000);

	mem_space->install_write_bank(0xc000, 0xffff, "bank4");
	membank("bank4")->set_base(ram + 0xc000);

}

WRITE8_MEMBER(llc_state::llc2_basic_enable_w)
{

	address_space *mem_space = machine().device("maincpu")->memory().space(AS_PROGRAM);
	if (data & 0x02) {
		mem_space->unmap_write(0x4000, 0x5fff);
		membank("bank2")->set_base(machine().root_device().memregion("maincpu")->base() + 0x10000);
	} else {
		mem_space->install_write_bank(0x4000, 0x5fff, "bank2");
		membank("bank2")->set_base(machine().device<ram_device>(RAM_TAG)->pointer() + 0x4000);
	}

}
static READ8_DEVICE_HANDLER (llc2_port_b_r)
{
	return 0;
}

static UINT8 key_pos(UINT8 val) {
	if (BIT(val,0)) return 1;
	if (BIT(val,1)) return 2;
	if (BIT(val,2)) return 3;
	if (BIT(val,3)) return 4;
	if (BIT(val,4)) return 5;
	if (BIT(val,5)) return 6;
	if (BIT(val,6)) return 7;
	if (BIT(val,7)) return 8;
	return 0;
}
static READ8_DEVICE_HANDLER (llc2_port_a_r)
{
	UINT8 *k7659 = device->machine().root_device().memregion("k7659")->base();
	UINT8 retVal = 0;
	UINT8 a1 = input_port_read(device->machine(), "A1");
	UINT8 a2 = input_port_read(device->machine(), "A2");
	UINT8 a3 = input_port_read(device->machine(), "A3");
	UINT8 a4 = input_port_read(device->machine(), "A4");
	UINT8 a5 = input_port_read(device->machine(), "A5");
	UINT8 a6 = input_port_read(device->machine(), "A6");
	UINT8 a7 = input_port_read(device->machine(), "A7");
	UINT8 a8 = input_port_read(device->machine(), "A8");
	UINT8 a9 = input_port_read(device->machine(), "A9");
	UINT8 a10 = input_port_read(device->machine(), "A10");
	UINT8 a11 = input_port_read(device->machine(), "A11");
	UINT8 a12 = input_port_read(device->machine(), "A12");
	UINT16 code = 0;
	if (a1!=0) {
		code = 0x10 + key_pos(a1);
	} else if (a2!=0) {
		code = 0x20 + key_pos(a2);
	} else if (a3!=0) {
		code = 0x30 + key_pos(a3);
	} else if (a4!=0) {
		code = 0x40 + key_pos(a4);
	} else if (a5!=0) {
		code = 0x50 + key_pos(a5);
	} else if (a6!=0) {
		code = 0x60 + key_pos(a6);
	} else if (a7!=0) {
		code = 0x70 + key_pos(a7);
	} else if (a9!=0) {
		code = 0x80 + key_pos(a9);
	} else if (a10!=0) {
		code = 0x90 + key_pos(a10);
	} else if (a11!=0) {
		code = 0xA0 + key_pos(a11);
	}
	if (code!=0) {
		if (BIT(a8,6) || BIT(a8,7)) {
			code |= 0x100;
		} else if (BIT(a12,6)) {
			code |= 0x200;
		}
		retVal = k7659[code] | 0x80;
	}
	return retVal;
}


Z80PIO_INTERFACE( llc2_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(llc2_port_a_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(llc2_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL
};

Z80PIO_INTERFACE( llc1_z80pio_intf )
{
	DEVCB_NULL,	/* callback when change interrupt status */
	DEVCB_HANDLER(llc1_port_a_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_HANDLER(llc1_port_b_r),
	DEVCB_NULL,
	DEVCB_NULL
};
