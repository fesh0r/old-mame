/**********************************************************************

    rtc4543.c - Epson R4543 real-time clock chip emulation
    by R. Belmont

    TODO: writing (not done by System 12 or 23 so no test case)

**********************************************************************/

#include "rtc4543.h"

//**************************************************************************
//  MACROS / CONSTANTS
//**************************************************************************

#define VERBOSE 0

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

// device type definition
const device_type RTC4543 = &device_creator<rtc4543_device>;


//-------------------------------------------------
//  rtc4543_device - constructor
//-------------------------------------------------

rtc4543_device::rtc4543_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
    : device_t(mconfig, RTC4543, "Epson R4543", tag, owner, clock),
	  device_rtc_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void rtc4543_device::device_start()
{
	// allocate timers
	m_clock_timer = timer_alloc();
	m_clock_timer->adjust(attotime::from_hz(clock() / 32768), 0, attotime::from_hz(clock() / 32768));

	// state saving
	save_item(NAME(m_ce));
	save_item(NAME(m_clk));
	save_item(NAME(m_wr));
	save_item(NAME(m_data));
	save_item(NAME(m_regs));
	save_item(NAME(m_curreg));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void rtc4543_device::device_reset()
{
	set_current_time(machine());

    m_ce = 0;
    m_wr = 0;
    m_clk = 0;
    m_data = 0;
    m_curreg = 0;
}


//-------------------------------------------------
//  device_timer - handler timer events
//-------------------------------------------------

void rtc4543_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
    advance_seconds();
}


INLINE UINT8 make_bcd(UINT8 data)
{
	return ((data / 10) << 4) | (data % 10);
}

//-------------------------------------------------
//  rtc_clock_updated -
//-------------------------------------------------

void rtc4543_device::rtc_clock_updated(int year, int month, int day, int day_of_week, int hour, int minute, int second)
{
	static const int weekday[7] = { 7, 1, 2, 3, 4, 5, 6 };

    m_regs[0] = make_bcd(second);               // seconds (BCD, 0-59) in bits 0-6, bit 7 = battery low
    m_regs[1] = make_bcd(minute);               // minutes (BCD, 0-59)
    m_regs[2] = make_bcd(hour);                 // hour (BCD, 0-23)
    m_regs[3] = make_bcd(weekday[day_of_week]);	// low nibble = day of the week
    m_regs[3] |= (make_bcd(day) & 0x0f)<<4;	    // high nibble = low digit of day
    m_regs[4] = (make_bcd(day) >> 4);			// low nibble = high digit of day
    m_regs[4] |= (make_bcd(month & 0x0f)<<4);	// high nibble = low digit of month
    m_regs[5] = make_bcd(month & 0x0f) >> 4;    // low nibble = high digit of month
    m_regs[5] |= (make_bcd(year % 10) << 4);	// high nibble = low digit of year
    m_regs[6] = make_bcd(year % 100) >> 4;	// low nibble = tens digit of year (BCD, 0-9)
}

//-------------------------------------------------
//  ce_w - chip enable write
//-------------------------------------------------

WRITE_LINE_MEMBER( rtc4543_device::ce_w )
{
	if (VERBOSE) printf("RTC4543 '%s' CE: %u\n", tag(), state);

	if (!state && m_ce)         // complete transfer
	{
	}
	else if (state && !m_ce)    // start new data transfer
	{
        m_curreg = 0;
        m_bit = 8;      // force immediate reload of output data
	}

	m_ce = state;
}

//-------------------------------------------------
//  wr_w - data direction line write
//-------------------------------------------------

WRITE_LINE_MEMBER( rtc4543_device::wr_w )
{
	if (VERBOSE) logerror("RTC4543 '%s' WR: %u\n", tag(), state);

	m_wr = state;
}

//-------------------------------------------------
//  clk_w - serial clock write
//-------------------------------------------------

WRITE_LINE_MEMBER( rtc4543_device::clk_w )
{
	if (VERBOSE) logerror("RTC4543 '%s' CLK: %u\n", tag(), state);

	if (!m_ce) return;

	if (!m_clk && state) // rising edge - read data becomes valid here
	{
        if (m_bit > 7)  // reload data?
        {
            m_bit = 0;
            m_data = m_regs[m_curreg++];
        }
        else            // no reload, just continue with the current byte
        {
            m_data <<= 1;
        }

        m_bit++;
	}
	else if (m_clk && !state) // falling edge - write data becomes valid here
	{
	}

	m_clk = state;
}


//-------------------------------------------------
//  data_w - I/O write
//-------------------------------------------------

WRITE_LINE_MEMBER( rtc4543_device::data_w )
{
	if (VERBOSE) logerror("RTC4543 '%s' I/O: %u\n", tag(), state);

	m_data |= (state & 1);
}


//-------------------------------------------------
//  data_r - I/O read
//-------------------------------------------------

READ_LINE_MEMBER( rtc4543_device::data_r )
{
	return (m_data & 0x80) ? 1 : 0;
}

