/***************************************************************************

 h8periph.c: Implementation of Hitachi H8/3002 on-board MCU functions.

 Original by The_Author & DynaChicken for the ZiNc emulator.

 Rewritten for MAME to use MAME timers and other facilities by R. Belmont

****************************************************************************/

#include "debugger.h"
#include "h83002.h"
#include "h8priv.h"

#define H8_REG_START	(0x00ffff10)

// timer registers
#define TSTR	(0x60)
#define TIER0	(0x66)
#define TIER1   (0x70)
#define TIER2   (0x7a)
#define TIER3   (0x84)
#define TIER4   (0x94)
#define TSR0	(0x67)
#define TSR1	(0x71)
#define TSR2	(0x7b)
#define TSR3	(0x85)
#define TSR4	(0x95)
#define TCR0	(0x64)
#define TCR1	(0x6e)
#define TCR2	(0x78)
#define TCR3	(0x82)
#define TCR4	(0x92)

static TIMER_CALLBACK( h8itu_timer_0_cb )
{
	timer_adjust(h8.timer[0], attotime_never, 0, attotime_zero);
	h8.h8TCNT0 = 0;
	h8.per_regs[TSR0] |= 4;
	// interrupt on overflow ?
	if(h8.per_regs[TIER0] & 4)
	{
		h8_3002_InterruptRequest(26);
	}
}

static TIMER_CALLBACK( h8itu_timer_1_cb )
{
	timer_adjust(h8.timer[1], attotime_never, 0, attotime_zero);
	h8.h8TCNT1 = 0;
	h8.per_regs[TSR1] |= 4;
	// interrupt on overflow ?
	if(h8.per_regs[TIER1] & 4)
	{
		h8_3002_InterruptRequest(30);
	}
}

static TIMER_CALLBACK( h8itu_timer_2_cb )
{
	timer_adjust(h8.timer[2], attotime_never, 0, attotime_zero);
	h8.h8TCNT2 = 0;
	h8.per_regs[TSR2] |= 4;
	// interrupt on overflow ?
	if(h8.per_regs[TIER2] & 4)
	{
		h8_3002_InterruptRequest(34);
	}
}

static TIMER_CALLBACK( h8itu_timer_3_cb )
{
	timer_adjust(h8.timer[3], attotime_never, 0, attotime_zero);
	h8.h8TCNT3 = 0;
	h8.per_regs[TSR3] |= 4;
	// interrupt on overflow ?
	if(h8.per_regs[TIER3] & 4)
	{
		h8_3002_InterruptRequest(38);
	}
}

static TIMER_CALLBACK( h8itu_timer_4_cb )
{
	timer_adjust(h8.timer[4], attotime_never, 0, attotime_zero);
	h8.h8TCNT4 = 0;
	h8.per_regs[TSR4] |= 4;
	// interrupt on overflow ?
	if(h8.per_regs[TIER4] & 4)
	{
		h8_3002_InterruptRequest(42);
	}
}

static void h8_itu_refresh_timer(int tnum)
{
	int ourTCR = 0;
	int ourTVAL = 0;
	attotime period;
	static const int tscales[4] = { 1, 2, 4, 8 };

	switch (tnum)
	{
		case 0:
			ourTCR = h8.per_regs[TCR0];
			ourTVAL = h8.h8TCNT0;
			break;
		case 1:
			ourTCR = h8.per_regs[TCR1];
			ourTVAL = h8.h8TCNT1;
			break;
		case 2:
			ourTCR = h8.per_regs[TCR2];
			ourTVAL = h8.h8TCNT2;
			break;
		case 3:
			ourTCR = h8.per_regs[TCR3];
			ourTVAL = h8.h8TCNT3;
			break;
		case 4:
			ourTCR = h8.per_regs[TCR4];
			ourTVAL = h8.h8TCNT4;
			break;
	}

	period = attotime_mul(ATTOTIME_IN_HZ(cpunum_get_clock(h8.cpu_number)), tscales[ourTCR & 3] * (65536 - ourTVAL));

	if (ourTCR & 4)
	{
		logerror("H8/3002: Timer %d is using an external clock.  Unsupported!\n", tnum);
	}

	timer_adjust(h8.timer[tnum], period, 0, attotime_zero);
}

static void h8_itu_sync_timers(int tnum)
{
	int ourTCR = 0;
	attotime cycle_time, cur;
	UINT16 ratio;
	static const int tscales[4] = { 1, 2, 4, 8 };

	switch (tnum)
	{
		case 0:
			ourTCR = h8.per_regs[TCR0];
			break;
		case 1:
			ourTCR = h8.per_regs[TCR1];
			break;
		case 2:
			ourTCR = h8.per_regs[TCR2];
			break;
		case 3:
			ourTCR = h8.per_regs[TCR3];
			break;
		case 4:
			ourTCR = h8.per_regs[TCR4];
			break;
	}

	// get the time per unit
	cycle_time = attotime_mul(ATTOTIME_IN_HZ(cpunum_get_clock(h8.cpu_number)), tscales[ourTCR & 3]);
	cur = timer_timeelapsed(h8.timer[tnum]);

	ratio = attotime_to_double(cur) / attotime_to_double(cycle_time);

	switch (tnum)
	{
		case 0:
			h8.h8TCNT0 = ratio;
			break;
		case 1:
			h8.h8TCNT1 = ratio;
			break;
		case 2:
			h8.h8TCNT2 = ratio;
			break;
		case 3:
			h8.h8TCNT3 = ratio;
			break;
		case 4:
			h8.h8TCNT4 = ratio;
			break;
	}
}

UINT8 h8_itu_read8(UINT8 reg)
{
	UINT8 val;

	switch(reg)
	{
	case 0x60:
		val = h8.h8TSTR;
		break;
	case 0x68:
		h8_itu_sync_timers(0);
		val = h8.h8TCNT0>>8;
		break;
	case 0x69:
		h8_itu_sync_timers(0);
		val = h8.h8TCNT0&0xff;
		break;
	case 0x72:
		h8_itu_sync_timers(1);
		val = h8.h8TCNT1>>8;
		break;
	case 0x73:
		h8_itu_sync_timers(1);
		val = h8.h8TCNT1&0xff;
		break;
	case 0x7c:
		h8_itu_sync_timers(2);
		val = h8.h8TCNT2>>8;
		break;
	case 0x7d:
		h8_itu_sync_timers(2);
		val = h8.h8TCNT2&0xff;
		break;
	case 0x86:
		h8_itu_sync_timers(3);
		val = h8.h8TCNT3>>8;
		break;
	case 0x87:
		h8_itu_sync_timers(3);
		val = h8.h8TCNT3&0xff;
		break;
	default:
		val = h8.per_regs[reg];
		break;
	}


	return val;
}

void h8_itu_write8(UINT8 reg, UINT8 val)
{
	h8.per_regs[reg] = val;
	switch(reg)
	{
	case 0x60:
		if ((val & 1) && !(h8.h8TSTR & 1))
		{
			h8_itu_refresh_timer(0);
		}
		if ((val & 2) && !(h8.h8TSTR & 2))
		{
			h8_itu_refresh_timer(1);
		}
		if ((val & 4) && !(h8.h8TSTR & 4))
		{
			h8_itu_refresh_timer(2);
		}
		if ((val & 8) && !(h8.h8TSTR & 8))
		{
			h8_itu_refresh_timer(3);
		}
		if ((val & 0x10) && !(h8.h8TSTR & 0x10))
		{
			h8_itu_refresh_timer(4);
		}
		h8.h8TSTR = val;
		break;
	case 0x68:
		h8.h8TCNT0 = (val<<8) | (h8.h8TCNT0 & 0xff);
		if (h8.h8TSTR & 1)
		{
			h8_itu_refresh_timer(0);
		}
		break;
	case 0x69:
		h8.h8TCNT0 = (val) | (h8.h8TCNT0 & 0xff00);
		if (h8.h8TSTR & 1)
		{
			h8_itu_refresh_timer(0);
		}
		break;
	case 0x72:
		h8.h8TCNT1 = (val<<8) | (h8.h8TCNT1 & 0xff);
		if (h8.h8TSTR & 2)
		{
			h8_itu_refresh_timer(1);
		}
		break;
	case 0x73:
		h8.h8TCNT1 = (val) | (h8.h8TCNT1 & 0xff00);
		if (h8.h8TSTR & 2)
		{
			h8_itu_refresh_timer(1);
		}
		break;
	case 0x7c:
		h8.h8TCNT2 = (val<<8) | (h8.h8TCNT2 & 0xff);
		if (h8.h8TSTR & 4)
		{
			h8_itu_refresh_timer(2);
		}
		break;
	case 0x7d:
		h8.h8TCNT2 = (val) | (h8.h8TCNT2 & 0xff00);
		if (h8.h8TSTR & 4)
		{
			h8_itu_refresh_timer(2);
		}
		break;
	case 0x86:
		h8.h8TCNT3 = (val<<8) | (h8.h8TCNT3 & 0xff);
		if (h8.h8TSTR & 8)
		{
			h8_itu_refresh_timer(3);
		}
		break;
	case 0x87:
		h8.h8TCNT3 = (val) | (h8.h8TCNT3 & 0xff00);
		if (h8.h8TSTR & 8)
		{
			h8_itu_refresh_timer(3);
		}
		break;
	case 0x96:
		h8.h8TCNT4 = (val<<8) | (h8.h8TCNT4 & 0xff);
		if (h8.h8TSTR & 0x10)
		{
			h8_itu_refresh_timer(4);
		}
		break;
	case 0x97:
		h8.h8TCNT4 = (val) | (h8.h8TCNT4 & 0xff00);
		if (h8.h8TSTR & 0x10)
		{
			h8_itu_refresh_timer(4);
		}
		break;
	default:
		val = 0;
		break;
	}
}

#ifdef UNUSED_FUNCTION
UINT8 h8_debugger_itu_read8(UINT8 reg)
{
	UINT8 val;
	val = 0;
	return val;
}
#endif


static UINT8 h8_ISR_r(void)
{
	UINT8 res = 0;

	int i;
	for (i = 0; i < 6; i++)
		if (h8.h8_IRQrequestL & (1 << (12+i)))	res |= (1 << i);

	return res;
}

static void h8_ISR_w(UINT8 val)
{
	int i;
	for (i = 0; i < 6; i++)
		if ((~val) & (1 << i))	h8.h8_IRQrequestL &= ~(1 << (12+i));
}


UINT8 h8_register_read8(UINT32 address)
{
	UINT8 val;
	UINT8 reg;

	address &= 0xffffff;

	reg = address & 0xff;

	if(reg >= 0x60 && reg <= 0x9f)
	{
		return h8_itu_read8(reg);
	}
	else
	{
		switch(reg)
		{
		case 0xb4: // serial port A status
			val = h8.per_regs[reg];
			val |= 0xc4;		// transmit finished, receive ready, no errors
			break;
		case 0xb5: // serial port A receive
			val = io_read_byte(H8_SERIAL_A);
			break;
		case 0xbc: // serial port B status
			val = h8.per_regs[reg];
			val |= 0xc4;		// transmit finished, receive ready, no errors
			break;
		case 0xbd: // serial port B receive
			val = io_read_byte(H8_SERIAL_B);
			break;
		case 0xe0:
			val = io_read_byte_8(H8_ADC_0_H);
			break;
		case 0xe1:
			val = io_read_byte_8(H8_ADC_0_L);
			break;
		case 0xe2:
			val = io_read_byte_8(H8_ADC_1_H);
			break;
		case 0xe3:
			val = io_read_byte_8(H8_ADC_1_L);
			break;
		case 0xe4:
			val = io_read_byte_8(H8_ADC_2_H);
			break;
		case 0xe5:
			val = io_read_byte_8(H8_ADC_2_L);
			break;
		case 0xe6:
			val = io_read_byte_8(H8_ADC_3_H);
			break;
		case 0xe7:
			val = io_read_byte_8(H8_ADC_3_L);
			break;
		case 0xe8:		// adc status
			val = 0x80;
			break;
		case 0xc7:    		// port 4 data
			val = io_read_byte_8(H8_PORT4);
			break;
		case 0xcb:    		// port 6 data
			val = io_read_byte_8(H8_PORT6);
			break;
		case 0xce:		// port 7 data
			val = io_read_byte_8(H8_PORT7);
			break;
		case 0xcf:		// port 8 data
			val = io_read_byte_8(H8_PORT8);
			break;
		case 0xd2:		// port 9 data
			val = io_read_byte_8(H8_PORT9);
			break;
		case 0xd3:		// port a data
			val = io_read_byte_8(H8_PORTA);
			break;
		case 0xd6:		// port b data
			val = io_read_byte_8(H8_PORTB);
			break;
		case 0xf6:
			val = h8_ISR_r();
			break;

		default:
			val = h8.per_regs[reg];
			break;
		}
	}

	return val;
}

void h8_register_write8(UINT32 address, UINT8 val)
{
	UINT8 reg;

	address &= 0xffffff;

	reg = address & 0xff;

	if(reg >= 0x60 && reg <= 0x9f)
	{
		h8_itu_write8(reg, val);
	}

	switch (reg)
	{
		case 0xb3:
			io_write_byte(H8_SERIAL_A, val);
			break;
		case 0xbb:
			io_write_byte(H8_SERIAL_B, val);
			break;
		case 0xc7:
			io_write_byte_8(H8_PORT4, val);
			break;
		case 0xcb:    		// port 6 data
			io_write_byte_8(H8_PORT6, val);
			break;
		case 0xce:		// port 7 data
			io_write_byte_8(H8_PORT7, val);
			break;
		case 0xcf:		// port 8 data
			io_write_byte_8(H8_PORT8, val);
			break;
		case 0xd2:		// port 9 data
			io_write_byte_8(H8_PORT9, val);
			break;
		case 0xd3:		// port a data
			io_write_byte_8(H8_PORTA, val);
			break;
		case 0xd6:		// port b data
			io_write_byte_8(H8_PORTB, val);
			break;
		case 0xf6:
			h8_ISR_w(val);
			break;
	}

	h8.per_regs[reg] = val;
}

static void h8_3007_itu_refresh_timer(int tnum)
{
	attotime period;
	static const int tscales[4] = { 1, 2, 4, 8 };
	int ourTCR = h8.per_regs[0x68+(tnum*8)];

	period = attotime_mul(ATTOTIME_IN_HZ(cpunum_get_clock(h8.cpu_number)), tscales[ourTCR & 3]);

	if (ourTCR & 4)
	{
		logerror("H8/3007: Timer %d is using an external clock.  Unsupported!\n", tnum);
	}

	timer_adjust(h8.timer[tnum], period, 0, attotime_zero);
}

INLINE void h8itu_3007_timer_cb(int tnum)
{
	int base = 0x68 + (tnum*8);
	UINT16 count = (h8.per_regs[base + 0x2]<<8) | h8.per_regs[base + 0x3];
	count++;

//  logerror("h8/3007 timer %d count = %04x\n",tnum,count);

	 // GRA match
	if ((h8.per_regs[base + 0x1] & 0x03) && (count == ((h8.per_regs[base + 0x4]<<8) | h8.per_regs[base + 0x5])))
	{
		if ((h8.per_regs[base + 0x0] & 0x60) == 0x20)
		{
//          logerror("h8/3007 timer %d GRA match, restarting\n",tnum);
			count = 0;
			h8_3007_itu_refresh_timer(tnum);
		}
		else
		{
//          logerror("h8/3007 timer %d GRA match, stopping\n",tnum);
			timer_adjust(h8.timer[tnum], attotime_never, 0, attotime_zero);
		}

		h8.per_regs[0x64] |= 1<<tnum;
		if(h8.per_regs[0x64] & (4<<tnum))	// interrupt enable
		{
//          logerror("h8/3007 timer %d GRA INTERRUPT\n",tnum);
			h8_3002_InterruptRequest(24+tnum*4);
		}
	}
	// GRB match
	if ((h8.per_regs[base + 0x1] & 0x30) && (count == ((h8.per_regs[base + 0x6]<<8) | h8.per_regs[base + 0x7])))
	{
		if ((h8.per_regs[base + 0x0] & 0x60) == 0x40)
		{
//          logerror("h8/3007 timer %d GRB match, restarting\n",tnum);
			count = 0;
			h8_3007_itu_refresh_timer(tnum);
		}
		else
		{
//          logerror("h8/3007 timer %d GRB match, stopping\n",tnum);
			timer_adjust(h8.timer[tnum], attotime_never, 0, attotime_zero);
		}

		h8.per_regs[0x65] |= 1<<tnum;
		if(h8.per_regs[0x65] & (4<<tnum))	// interrupt enable
		{
//          logerror("h8/3007 timer %d GRB INTERRUPT\n",tnum);
			h8_3002_InterruptRequest(25+tnum*4);
		}
	}
	// Overflow
	if (((h8.per_regs[base + 0x1] & 0x33) == 0) && (count == 0))
	{
//      logerror("h8/3007 timer %d OVF match, restarting\n",tnum);
		h8.per_regs[0x66] |= 1<<tnum;
		if(h8.per_regs[0x66] & (4<<tnum))	// interrupt enable
		{
//          logerror("h8/3007 timer %d OVF INTERRUPT\n",tnum);
			h8_3002_InterruptRequest(26+tnum*4);
		}
	}

	h8.per_regs[base + 0x2] = count >> 8;
	h8.per_regs[base + 0x3] = count & 0xff;
}

static TIMER_CALLBACK( h8itu_3007_timer_0_cb )
{
	h8itu_3007_timer_cb(0);
}
static TIMER_CALLBACK( h8itu_3007_timer_1_cb )
{
	h8itu_3007_timer_cb(1);
}
static TIMER_CALLBACK( h8itu_3007_timer_2_cb )
{
	h8itu_3007_timer_cb(2);
}


UINT8 h8_3007_itu_read8(UINT8 reg)
{
	UINT8 val;

	switch(reg)
	{
	case 0x60:
		val = h8.h8TSTR | 0xf8;
		break;
	default:
		val = h8.per_regs[reg];
		break;
	}

	return val;
}

void h8_3007_itu_write8(UINT8 reg, UINT8 val)
{
//  logerror("%06x: h8/3007 reg %02x = %02x\n",activecpu_get_pc(),reg,val);
	h8.per_regs[reg] = val;
	switch(reg)
	{
	case 0x60:
		if ((val & 1) && !(h8.h8TSTR & 1))
		{
			h8_3007_itu_refresh_timer(0);
		}
		if ((val & 2) && !(h8.h8TSTR & 2))
		{
			h8_3007_itu_refresh_timer(1);
		}
		if ((val & 4) && !(h8.h8TSTR & 4))
		{
			h8_3007_itu_refresh_timer(2);
		}
		h8.h8TSTR = val;
		break;
	default:
		val = 0;
		break;
	}
}

UINT8 h8_3007_register_read8(UINT32 address)
{
	UINT8 val;
	UINT8 reg;

	address &= 0xffffff;

	reg = address & 0xff;

	if(reg >= 0x60 && reg <= 0x7f)
	{
		return h8_3007_itu_read8(reg);
	}
	else
	{
		switch(reg)
		{
		case 0xb4: // serial port A status
			val = h8.per_regs[reg];
			val |= 0xc4;		// transmit finished, receive ready, no errors
			break;
		case 0xb5: // serial port A receive
			val = io_read_byte(H8_SERIAL_A);
			break;
		case 0xbc: // serial port B status
			val = h8.per_regs[reg];
			val |= 0xc4;		// transmit finished, receive ready, no errors
			break;
		case 0xbd: // serial port B receive
			val = io_read_byte(H8_SERIAL_B);
			break;
		case 0xe0:
			val = io_read_byte_8(H8_ADC_0_H);
			break;
		case 0xe1:
			val = io_read_byte_8(H8_ADC_0_L);
			break;
		case 0xe2:
			val = io_read_byte_8(H8_ADC_1_H);
			break;
		case 0xe3:
			val = io_read_byte_8(H8_ADC_1_L);
			break;
		case 0xe4:
			val = io_read_byte_8(H8_ADC_2_H);
			break;
		case 0xe5:
			val = io_read_byte_8(H8_ADC_2_L);
			break;
		case 0xe6:
			val = io_read_byte_8(H8_ADC_3_H);
			break;
		case 0xe7:
			val = io_read_byte_8(H8_ADC_3_L);
			break;
		case 0xe8:		// adc status
			val = 0x80;
			break;

		case 0xd3:    		// port 4 data
			val = io_read_byte_8(H8_PORT4);
			break;
		case 0xd5:    		// port 6 data
			val = io_read_byte_8(H8_PORT6);
			break;
		case 0xd6:		// port 7 data
			val = io_read_byte_8(H8_PORT7);
			break;
		case 0xd7:		// port 8 data
			val = io_read_byte_8(H8_PORT8);
			break;
		case 0xd8:		// port 9 data
			val = io_read_byte_8(H8_PORT9);
			break;
		case 0xd9:		// port a data
			val = io_read_byte_8(H8_PORTA);
			break;
		case 0xda:		// port b data
			val = io_read_byte_8(H8_PORTB);
			break;
		default:
			val = h8.per_regs[reg];
			break;
		}
	}

	return val;
}

void h8_3007_register_write8(UINT32 address, UINT8 val)
{
	UINT8 reg;

	address &= 0xffffff;

	reg = address & 0xff;

	h8.per_regs[reg] = val;

	if(reg >= 0x60 && reg <= 0x7f)
	{
		h8_3007_itu_write8(reg, val);
	}
	else
	{
		switch (reg)
		{
			case 0xb3:
				io_write_byte(H8_SERIAL_A, val);
				break;
			case 0xbb:
				io_write_byte(H8_SERIAL_B, val);
				break;
			case 0xd3:
				io_write_byte_8(H8_PORT4, val);
				break;
			case 0xd5:		// port 6 data
				io_write_byte_8(H8_PORT6, val);
				break;
			case 0xd6:		// port 7 data
				io_write_byte_8(H8_PORT7, val);
				break;
			case 0xd7:		// port 8 data
				io_write_byte_8(H8_PORT8, val);
				break;
			case 0xd8:		// port 9 data
				io_write_byte_8(H8_PORT9, val);
				break;
			case 0xd9:		// port a data
				io_write_byte_8(H8_PORTA, val);
				break;
			case 0xda:		// port b data
				io_write_byte_8(H8_PORTB, val);
				break;
		}
	}
}

UINT8 h8_3007_register1_read8(UINT32 address)
{
	switch (address)
	{
		case 0xfee012:	return h8.per_regs[0xF2];	// SYSCR
		case 0xfee016:	return h8_ISR_r();			// ISR
		case 0xfee018:	return h8.per_regs[0xF8];	// IPRA
	}

	logerror("cpu #%d (PC=%08X): unmapped I/O(1) byte read from %08X\n",cpu_getactivecpu(),activecpu_get_pc(),address);
	return 0;
}

void h8_3007_register1_write8(UINT32 address, UINT8 val)
{
	switch (address)
	{
		case 0xfee012:	h8.per_regs[0xF2] = val;	return;	// SYSCR
		case 0xfee016:	h8_ISR_w(val);				return;	// ISR
		case 0xfee018:	h8.per_regs[0xF8] = val;	return;	// IPRA
	}
	logerror("cpu #%d (PC=%08X): unmapped I/O(1) byte write to %08X = %02X\n",cpu_getactivecpu(),activecpu_get_pc(),address,val);
}

void h8_3007_itu_init(void)
{
	h8.timer[0] = timer_alloc(h8itu_3007_timer_0_cb, NULL);
	h8.timer[1] = timer_alloc(h8itu_3007_timer_1_cb, NULL);
	h8.timer[2] = timer_alloc(h8itu_3007_timer_2_cb, NULL);

	h8_itu_reset();
}

void h8_itu_init(void)
{
	h8.timer[0] = timer_alloc(h8itu_timer_0_cb, NULL);
	h8.timer[1] = timer_alloc(h8itu_timer_1_cb, NULL);
	h8.timer[2] = timer_alloc(h8itu_timer_2_cb, NULL);
	h8.timer[3] = timer_alloc(h8itu_timer_3_cb, NULL);
	h8.timer[4] = timer_alloc(h8itu_timer_4_cb, NULL);

	h8_itu_reset();

	h8.cpu_number = cpu_getactivecpu();
}

void h8_itu_reset(void)
{
	// stop all the timers
	timer_adjust(h8.timer[0], attotime_never, 0, attotime_zero);
	timer_adjust(h8.timer[1], attotime_never, 0, attotime_zero);
	timer_adjust(h8.timer[2], attotime_never, 0, attotime_zero);
	timer_adjust(h8.timer[3], attotime_never, 0, attotime_zero);
	timer_adjust(h8.timer[4], attotime_never, 0, attotime_zero);
}
