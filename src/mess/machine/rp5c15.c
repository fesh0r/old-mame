/*

	Ricoh RP5C15 Real-time Clock
	Written by Barry Rodewald
	Started 04/07/07

	Registers are separated into two banks, switchable through
	the LSB of the MODE register.

	Date and time are stored in BCD, one digit to each register

	Register	BANK 0					BANK 1
	0			Timer seconds			CLKOUT
	1			Timer seconds (10s)		RTC Adjustment
	2			Timer minutes			Alarm minutes
	3			Timer minutes (10s)		Alarm minutes (10s)
	4			Timer hours				Alarm hours
	5			Timer hours (10s)		Alarm hours (10s)
	6			Timer day-of-week		Alarm day-of-week
	7			Timer day				Alarm day
	8			Timer day (10s)			Alarm day (10s)
	9			Timer month				(unused)
	10			Timer month (10s)		12/24 hour clock selection
	11			Timer year				Years until next leap year
	12			Timer year (10s)		(unused)
	13			MODE register			MODE register
	14			TEST register			TEST register
	15			RESET register			RESET register


	CLKOUT (W/O) - on the X68000, this is connected to the Timer LED
		xxxxx000 = always high (on)
		xxxxx001 = 16384Hz
		xxxxx010 =  1024Hz
		xxxxx011 =   128Hz
		xxxxx100 =    16Hz
		xxxxx101 =     1Hz
		xxxxx110 =  1/60Hz
		xxxxx111 =  always low (off)

	Adjustment register (R/W)
		bit 0 = if set, will reset the seconds to zero, and if the
		        seconds were > 30, increment the minutes by 1.
		        Always returns 0.

	12/24 hour clock register (R/W) (not implemented yet)
		bit 0 = selects 12 or 24 hour clock
		        presumably, on reading, returns AM/PM status.

	Leap year register
		bit 0,1 = returns number of years until next leap year.
		          Is simply year % 4.

	MODE register (R/W)
		bit 0 = if set, selects BANK 1, if reset, selects BANK 0
		bit 2 = Alarm enable
		bit 3 = Timer enable

	TEST register (not implemented yet)
		bits 0-3 = generally all set to 0, use is unknown.

	RESET register (not implemented yet)
		bit 0 = Alarm reset
		bit 1 = Timer reset
		bit 2 = if set, 16Hz pulse is off
		bit 3 = if set, 1Hz pulse is off

*/

#include "rp5c15.h"
#include "deprecat.h"

static struct rp5c15 rtc;

static emu_timer* rtc_timer;

TIMER_CALLBACK(rtc_alarm_pulse)
{
	if(rtc.pulse16_state == 0)  // low
	{
		rtc.pulse16_state = 1;  // make high
		rtc.pulse_count++;
	}
	else
	{
		rtc.pulse16_state = 0;  // make low
	}

	if(rtc.pulse_count == 8)
	{
		rtc.pulse1_state = 0;
	}
	if(rtc.pulse_count == 16)
	{
		rtc.pulse_count = 0;
		rtc.pulse1_state = 1;
		rtc_add_second();
	}
}

void rp5c15_init(const struct rp5c15_interface* intf)
{
	mame_system_time systm;
	mame_system_tm time;

	rtc.alarm_callback = intf->alarm_irq_callback;

	mame_get_base_datetime(Machine,&systm);
	time = systm.local_time;

	// date/time is stored as BCD
	rtc.systime.sec_1 = time.second % 10;
	rtc.systime.sec_10 = time.second / 10;
	rtc.systime.min_1 = time.minute % 10;
	rtc.systime.min_10 = time.minute / 10;
	rtc.systime.hour_1 = time.hour % 10;
	rtc.systime.hour_10 = time.hour / 10;
	rtc.systime.day_1 = time.mday % 10;
	rtc.systime.day_10 = time.mday / 10;
	rtc.systime.month_1 = (time.month+1) % 10;
	rtc.systime.month_10 = (time.month+1) / 10;
	rtc.systime.year_1 = (time.year - 1980) % 10;
	rtc.systime.year_10 = (time.year - 1980) / 10;
	rtc.systime.dayofweek = time.weekday;
	rtc.alarm.min_1 = time.minute % 10;
	rtc.alarm.min_10 = time.minute / 10;
	rtc.alarm.hour_1 = time.hour % 10;
	rtc.alarm.hour_10 = time.hour / 10;
	rtc.alarm.day_1 = time.mday % 10;
	rtc.alarm.day_10 = time.mday / 10;
	rtc.alarm.dayofweek = time.weekday;
	rtc.leap = time.year % 4;

	rtc.mode = 0x08;  // Timer enabled, Alarm disable, BANK 0 selected (defaults are guessed)
	rtc.reset = 0x00;  // enable both 1Hz and 16Hz alarm counters by default
	rtc.test = 0x00;
	rtc.pulse_count = 0;

	rtc_timer = timer_alloc(rtc_alarm_pulse, NULL);
	timer_adjust(rtc_timer, attotime_zero, 0, ATTOTIME_IN_HZ(32));
}

static int rp5c15_read(int offset, UINT16 mem_mask)
{
	if((rtc.mode & 0x01) == 0x00)  // BANK 0 selected
	{
		switch(offset)
		{
			case 0:
				return rtc.systime.sec_1;
			case 1:
				return rtc.systime.sec_10;
			case 2:
				return rtc.systime.min_1;
			case 3:
				return rtc.systime.min_10;
			case 4:
				return rtc.systime.hour_1;
			case 5:
				return rtc.systime.hour_10;
			case 6:
				return rtc.systime.dayofweek;
			case 7:
				return rtc.systime.day_1;
			case 8:
				return rtc.systime.day_10;
			case 9:
				return rtc.systime.month_1;
			case 10:
				return rtc.systime.month_10;
			case 11:
				return rtc.systime.year_1;
			case 12:
				return rtc.systime.year_10;
			case 13:
				return rtc.mode & 0x0f;
		}
	}
	else  // BANK 1 selected
	{
		switch(offset)
		{
			case 0:  // CLKOUT
				return rtc.clkout & 0x07;
			case 1:  // Adjustment (always returns 0)
				return 0;
			case 2:
				return rtc.alarm.min_1;
			case 3:
				return rtc.alarm.min_10;
			case 4:
				return rtc.alarm.hour_1;
			case 5:
				return rtc.alarm.hour_10;
			case 6:
				return rtc.alarm.dayofweek;
			case 7:
				return rtc.alarm.day_1;
			case 8:
				return rtc.alarm.day_10;
			case 10:  // 12/24 hour selection (returns AM/PM?)
				return 0;
			case 11:  // Leap year
				return rtc.leap;
			case 13:
				return rtc.mode & 0x0f;
			default:
				return 0;
		}
	}
	return 0;
}

static void rp5c15_write(int offset, int data, UINT16 mem_mask)
{
	if(offset == 13)
	{
		rtc.mode = data & 0x0f;
		return;
	}
	if((rtc.mode & 0x01) == 0x00)  // BANK 0 selected
	{
		switch(offset)
		{
			case 0:
				rtc.systime.sec_1 = data & 0x0f;
				break;
			case 1:
				rtc.systime.sec_10 = data & 0x0f;
				break;
			case 2:
				rtc.systime.min_1 = data & 0x0f;
				break;
			case 3:
				rtc.systime.min_10 = data & 0x0f;
				break;
			case 4:
				rtc.systime.hour_1 = data & 0x0f;
				break;
			case 5:
				rtc.systime.hour_10 = data & 0x0f;
				break;
			case 6:
				rtc.systime.dayofweek = data & 0x0f;
				break;
			case 7:
				rtc.systime.day_1 = data & 0x0f;
				break;
			case 8:
				rtc.systime.day_10 = data & 0x0f;
				break;
			case 9:
				rtc.systime.month_1 = data & 0x0f;
				break;
			case 10:
				rtc.systime.month_10 = data & 0x0f;
				break;
			case 11:
				rtc.systime.year_1 = data & 0x0f;
				break;
			case 12:
				rtc.systime.year_10 = data & 0x0f;
				break;
		}
	}
	else  // BANK 1 selected
	{
		switch(offset)
		{
			case 0:
				rtc.clkout = data & 0x07;
				break;
			case 1:  // Adjustment  (resets seconds to 0, increases minute by 1 if seconds are > 30
				if(rtc.systime.sec_10 >= 3)
					rtc_add_minute();
				rtc.systime.sec_1 = 0;
				rtc.systime.sec_10 = 0;
				break;
			case 2:
				rtc.alarm.min_1 = data & 0x0f;
				break;
			case 3:
				rtc.alarm.min_10 = data & 0x0f;
				break;
			case 4:
				rtc.alarm.hour_1 = data & 0x0f;
				break;
			case 5:
				rtc.alarm.hour_10 = data & 0x0f;
				break;
			case 6:
				rtc.alarm.dayofweek = data & 0x0f;
				break;
			case 7:
				rtc.alarm.day_1 = data & 0x0f;
				break;
			case 8:
				rtc.alarm.day_10 = data & 0x0f;
				break;
			case 10:  // 12/24 hour clock selection
				rtc.hour24 = data;
				break;

		}
	}
}

void rtc_add_second()  // add one second to current time
{
	if((rtc.mode & 0x08) == 0x00) // if timer is not enabled
		return;
	rtc.systime.sec_1++;
	if(rtc.systime.sec_1 < 10)
		return;
	rtc.systime.sec_1 = 0;
	rtc.systime.sec_10++;
	if(rtc.systime.sec_10 < 6)
		return;
	rtc.systime.sec_10 = 0;
	rtc_add_minute();
}

void rtc_add_minute()
{
	rtc.systime.min_1++;
	if(rtc.systime.min_1 < 10)
		return;
	rtc.systime.min_1 = 0;
	rtc.systime.min_10++;
	if(rtc.systime.min_10 < 6)
		return;
	rtc.systime.min_10 = 0;
	rtc.systime.hour_1++;
	if(rtc.systime.hour_1 < 10 && rtc.systime.hour_10 < 2)
		return;
	if(rtc.systime.hour_10 < 3 && rtc.systime.hour_1 < 4)
		return;
	rtc.systime.hour_1 = 0;
	rtc.systime.hour_10++;
	if(rtc.systime.hour_10 < 3)
		return;
	rtc.systime.hour_10 = 0;
	rtc_add_day();
}

void rtc_add_day()
{
	int d,m;

	rtc.systime.dayofweek++;
	if(rtc.systime.dayofweek >= 7)
		rtc.systime.dayofweek = 0;
	d = (rtc.systime.day_10 << 4) | rtc.systime.day_1;
	m = (rtc.systime.month_10 << 4) | rtc.systime.month_1;
	d++;
	if((d & 0x0f) >= 10)
	{
		d &= 0xf0;
		d += 0x10;
	}
	rtc.systime.day_1 = (d & 0x0f);
	rtc.systime.day_10 = (d & 0xf0) >> 4;
	switch(m)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 0x10:
		case 0x12:
			if(d > 0x31)
			{
				rtc.systime.day_1 = 1;
				rtc.systime.day_10 = 0;
				rtc_add_month();
			}
			break;
		case 4:
		case 6:
		case 9:
		case 11:
			if(d > 0x30)
			{
				rtc.systime.day_1 = 1;
				rtc.systime.day_10 = 0;
				rtc_add_month();
			}
			break;
		case 2:
			if(rtc.leap == 0)  // this is a leap year
			{
				if(d > 0x29)
				{
					rtc.systime.day_1 = 1;
					rtc.systime.day_10 = 0;
					rtc_add_month();
				}
			}
			else  // not a leap year
			{
				if(d > 0x28)
				{
					rtc.systime.day_1 = 1;
					rtc.systime.day_10 = 0;
					rtc_add_month();
				}
			}
			break;
	}
}

void rtc_add_month()
{
	rtc.systime.month_1++;
	if(rtc.systime.month_1 < 10 && rtc.systime.month_10 < 1)
		return;
	if(rtc.systime.month_1 < 3 && rtc.systime.month_10 < 2)
		return;
	rtc.systime.month_1 = 0;
	rtc.systime.month_10++;
	if(rtc.systime.month_10 < 2)
		return;
	rtc.systime.month_1 = 1;
	rtc.systime.month_10 = 0;
	rtc.systime.year_1++;
	if(rtc.systime.year_1 < 10)
		return;
	rtc.systime.year_1 = 0;
	rtc.systime.year_10++;
	if(rtc.systime.year_10 < 10)
		return;
	rtc.systime.year_10 = 0;
}

READ16_HANDLER(rp5c15_r) { return rp5c15_read(offset,mem_mask); }
WRITE16_HANDLER(rp5c15_w) { rp5c15_write(offset,data,mem_mask); }
