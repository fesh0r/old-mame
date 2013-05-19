/****************************************************************************

    TI-99/8 Speech synthesizer subsystem

    The TI-99/8 contains a speech synthesizer inside the console, so we cannot
    reuse the spchsyn implementation of the P-Box speech synthesizer.
    Accordingly, this is not a ti_expansion_card_device.

    Michael Zapf
    February 2012: Rewritten as class

*****************************************************************************/

#include "speech8.h"
#include "sound/wave.h"
#include "machine/spchrom.h"

#define TMS5220_ADDRESS_MASK 0x3FFFFUL  /* 18-bit mask for tms5220 address */

#define VERBOSE 1
#define LOG logerror

#define SPEECHSYN_TAG "speechsyn"

#define REAL_TIMING 0

/*
    For comments on real timing see ti99/spchsyn.c
*/
/****************************************************************************/

ti998_spsyn_device::ti998_spsyn_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
: bus8z_device(mconfig, TI99_SPEECH8, "TI-99/8 Speech synthesizer (onboard)", tag, owner, clock, "ti99_speech8", __FILE__)
{
}

/*
    Memory read
*/
#if REAL_TIMING
// ======  This is the version with real timing =======
READ8Z_MEMBER( ti998_spsyn_device::readz )
{
	if ((offset & m_select_mask)==m_select_value)
	{
		m_vsp->wsq_w(TRUE);
		m_vsp->rsq_w(FALSE);
		*value = m_vsp->read(offset) & 0xff;
		if (VERBOSE>4) LOG("speech8: read value = %02x\n", *value);
	}
}

/*
    Memory write
*/
WRITE8_MEMBER( ti998_spsyn_device::write )
{
	if ((offset & m_select_mask)==(m_select_value | 0x0400))
	{
		m_vsp->rsq_w(m_vsp, TRUE);
		m_vsp->wsq_w(m_vsp, FALSE);
		if (VERBOSE>4) LOG("speech8: write value = %02x\n", data);
		m_vsp->write(offset, data);
	}
}

#else
// ======  This is the version without real timing =======

READ8Z_MEMBER( ti998_spsyn_device::readz )
{
	if ((offset & m_select_mask)==m_select_value)
	{
		machine().device("maincpu")->execute().adjust_icount(-(18+3));      /* this is just a minimum, it can be more */
		*value = m_vsp->status_r(space, offset, 0xff) & 0xff;
		if (VERBOSE>4) LOG("speech8: read value = %02x\n", *value);
	}
}

/*
    Memory write
*/
WRITE8_MEMBER( ti998_spsyn_device::write )
{
	if ((offset & m_select_mask)==(m_select_value | 0x0400))
	{
		machine().device("maincpu")->execute().adjust_icount(-(54+3));      /* this is just an approx. minimum, it can be much more */

		/* RN: the stupid design of the tms5220 core means that ready is cleared */
		/* when there are 15 bytes in FIFO.  It should be 16.  Of course, if */
		/* it were the case, we would need to store the value on the bus, */
		/* which would be more complex. */
		if (!m_vsp->readyq_r())
		{
			attotime time_to_ready = attotime::from_double(m_vsp->time_to_ready());
			int cycles_to_ready = machine().device<cpu_device>("maincpu")->attotime_to_cycles(time_to_ready);
			if (VERBOSE>8) LOG("speech8: time to ready: %f -> %d\n", time_to_ready.as_double(), (int) cycles_to_ready);

			machine().device("maincpu")->execute().adjust_icount(-cycles_to_ready);
			machine().scheduler().timer_set(attotime::zero, FUNC_NULL);
		}
		if (VERBOSE>4) LOG("speech8: write value = %02x\n", data);
		m_vsp->data_w(space, offset, data);
	}
}
#endif

/**************************************************************************/

WRITE_LINE_MEMBER( ti998_spsyn_device::speech8_ready )
{
	// The TMS5200 implementation uses TRUE/FALSE, not ASSERT/CLEAR semantics
	m_ready((state==0)? ASSERT_LINE : CLEAR_LINE);
	if (VERBOSE>5) LOG("spchsyn: READY = %d\n", (state==0));

#if REAL_TIMING
	// Need to do that here (see explanations in spchsyn.c)
	if (state==0)
	{
		m_vsp->rsq_w(TRUE);
		m_vsp->wsq_w(TRUE);
	}
#endif
}

void ti998_spsyn_device::device_start()
{
	const speech8_config *conf = reinterpret_cast<const speech8_config *>(static_config());
	m_ready.resolve(conf->ready, *this);
	m_vsp = subdevice<tms5220_device>(SPEECHSYN_TAG);
}

void ti998_spsyn_device::device_reset()
{
	m_select_mask = 0xfc01;
	m_select_value = 0x9000;
	if (VERBOSE>4) LOG("speech8: reset\n");
}

// Unlike the TI-99/4A, the 99/8 uses the TMS5220
MACHINE_CONFIG_FRAGMENT( ti998_speech )
	MCFG_DEVICE_ADD("vsm", SPEECHROM, 0)

	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(SPEECHSYN_TAG, TMS5220, 640000L)
	MCFG_TMS52XX_READYQ_HANDLER(WRITELINE(ti998_spsyn_device, speech8_ready))
	MCFG_TMS52XX_SPEECHROM("vsm")
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( ti998_speech )
	ROM_REGION(0x8000, "vsm", 0)
	ROM_LOAD_OPTIONAL("spchrom.bin", 0x0000, 0x8000, BAD_DUMP CRC(58b155f7) SHA1(382292295c00dff348d7e17c5ce4da12a1d87763)) /* system speech ROM */
ROM_END

machine_config_constructor ti998_spsyn_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( ti998_speech );
}

const rom_entry *ti998_spsyn_device::device_rom_region() const
{
	return ROM_NAME( ti998_speech );
}
const device_type TI99_SPEECH8 = &device_creator<ti998_spsyn_device>;
