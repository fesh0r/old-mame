/***************************************************************************

        Pecom driver by Miodrag Milanovic

        08/11/2008 Preliminary driver.

****************************************************************************/

#include "emu.h"
#include "cpu/cosmac/cosmac.h"
#include "sound/cdp1869.h"
#include "machine/ram.h"
#include "includes/pecom.h"

TIMER_CALLBACK_MEMBER(pecom_state::reset_tick)
{

	m_reset = 1;
}

void pecom_state::machine_start()
{
	m_reset_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(pecom_state::reset_tick),this));
}

void pecom_state::machine_reset()
{
	UINT8 *rom = machine().root_device().memregion(CDP1802_TAG)->base();
	address_space &space = machine().device(CDP1802_TAG)->memory().space(AS_PROGRAM);


	space.unmap_write(0x0000, 0x3fff);
	space.install_write_bank(0x4000, 0x7fff, "bank2");
	space.unmap_write(0xf000, 0xf7ff);
	space.unmap_write(0xf800, 0xffff);
	space.install_read_bank (0xf000, 0xf7ff, "bank3");
	space.install_read_bank (0xf800, 0xffff, "bank4");
	membank("bank1")->set_base(rom + 0x8000);
	membank("bank2")->set_base(machine().device<ram_device>(RAM_TAG)->pointer() + 0x4000);
	membank("bank3")->set_base(rom + 0xf000);
	membank("bank4")->set_base(rom + 0xf800);

	m_reset = 0;
	m_dma = 0;
	m_reset_timer->adjust(attotime::from_msec(5));
}

READ8_MEMBER(pecom_state::pecom_cdp1869_charram_r)
{
	return m_cdp1869->char_ram_r(space, offset);
}

WRITE8_MEMBER(pecom_state::pecom_cdp1869_charram_w)
{
	return m_cdp1869->char_ram_w(space, offset, data);
}

READ8_MEMBER(pecom_state::pecom_cdp1869_pageram_r)
{
	return m_cdp1869->page_ram_r(space, offset);
}

WRITE8_MEMBER(pecom_state::pecom_cdp1869_pageram_w)
{
	return m_cdp1869->page_ram_w(space, offset, data);
}

WRITE8_MEMBER(pecom_state::pecom_bank_w)
{
	address_space &space2 = machine().device(CDP1802_TAG)->memory().space(AS_PROGRAM);
	UINT8 *rom = memregion(CDP1802_TAG)->base();
	machine().device(CDP1802_TAG)->memory().space(AS_PROGRAM).install_write_bank(0x0000, 0x3fff, "bank1");
	membank("bank1")->set_base(machine().device<ram_device>(RAM_TAG)->pointer() + 0x0000);

	if (data==2)
	{
		space2.install_read_handler (0xf000, 0xf7ff, read8_delegate(FUNC(pecom_state::pecom_cdp1869_charram_r),this));
		space2.install_write_handler(0xf000, 0xf7ff, write8_delegate(FUNC(pecom_state::pecom_cdp1869_charram_w),this));
		space2.install_read_handler (0xf800, 0xffff, read8_delegate(FUNC(pecom_state::pecom_cdp1869_pageram_r),this));
		space2.install_write_handler(0xf800, 0xffff, write8_delegate(FUNC(pecom_state::pecom_cdp1869_pageram_w),this));
	}
	else
	{
		space2.unmap_write(0xf000, 0xf7ff);
		space2.unmap_write(0xf800, 0xffff);
		space2.install_read_bank (0xf000, 0xf7ff, "bank3");
		space2.install_read_bank (0xf800, 0xffff, "bank4");
		membank("bank3")->set_base(rom + 0xf000);
		membank("bank4")->set_base(rom + 0xf800);
	}
}

READ8_MEMBER(pecom_state::pecom_keyboard_r)
{
	static const char *const keynames[] = {
		"LINE0", "LINE1", "LINE2", "LINE3", "LINE4", "LINE5", "LINE6", "LINE7",
		"LINE8", "LINE9", "LINE10", "LINE11", "LINE12", "LINE13", "LINE14", "LINE15", "LINE16",
		"LINE17", "LINE18", "LINE19", "LINE20", "LINE21", "LINE22", "LINE23", "LINE24","LINE25"
	};
	/*
       INP command BUS -> M(R(X)) BUS -> D
       so on each input, address is also set, 8 lower bits are used as input for keyboard
       Address is available on address bus during reading of value from port, and that is
       used to determine keyboard line reading
    */
	UINT16 addr = machine().device(CDP1802_TAG)->state().state_int(COSMAC_R0 + machine().device(CDP1802_TAG)->state().state_int(COSMAC_X));
	/* just in case somone is reading non existing ports */
	if (addr<0x7cca || addr>0x7ce3) return 0;
	return ioport(keynames[addr - 0x7cca])->read() & 0x03;
}

/* CDP1802 Interface */

READ_LINE_MEMBER(pecom_state::clear_r)
{
	return m_reset;
}

READ_LINE_MEMBER(pecom_state::ef2_r)
{
	device_t *device = machine().device(CASSETTE_TAG);
	int shift = BIT(machine().root_device().ioport("CNT")->read(), 1);
	double cas = dynamic_cast<cassette_image_device *>(device)->input();

	return (cas > 0.0) | shift;
}

/*
static COSMAC_EF_READ( pecom64_ef_r )
{
    int flags = 0x0f;
    double valcas = (cassette_device_image(device->machine()));->input()
    UINT8 val = device->machine().root_device().ioport("CNT")->read();

    if ((val & 0x04)==0x04 && pecom_prev_caps_state==0)
    {
        pecom_caps_state = (pecom_caps_state==4) ? 0 : 4; // Change CAPS state
    }
    pecom_prev_caps_state = val & 0x04;
    if (valcas!=0.0)
    { // If there is any cassette signal
        val = (val & 0x0D); // remove EF2 from SHIFT
        flags -= EF2;
        if ( valcas > 0.00) flags += EF2;
    }
    flags -= (val & 0x0b) + pecom_caps_state;
    return flags;
}
*/
WRITE_LINE_MEMBER(pecom_state::pecom64_q_w)
{
	device_t *device = machine().device(CASSETTE_TAG);
	dynamic_cast<cassette_image_device *>(device)->output(state ? -1.0 : +1.0);
}

static COSMAC_SC_WRITE( pecom64_sc_w )
{
	switch (sc)
	{
	case COSMAC_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case COSMAC_STATE_CODE_S1_EXECUTE:
		break;

	case COSMAC_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		device->machine().device(CDP1802_TAG)->execute().set_input_line(COSMAC_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;
	case COSMAC_STATE_CODE_S3_INTERRUPT:
		break;
	}
}

COSMAC_INTERFACE( pecom64_cdp1802_config )
{
	DEVCB_LINE_VCC,
	DEVCB_DRIVER_LINE_MEMBER(pecom_state,clear_r),
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pecom_state,ef2_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_DRIVER_LINE_MEMBER(pecom_state,pecom64_q_w),
	DEVCB_NULL,
	DEVCB_NULL,
	pecom64_sc_w,
	DEVCB_NULL,
	DEVCB_NULL
};
