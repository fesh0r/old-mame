/**********************************************************************
 * RIOT emulation
 *
 * TODO:
 * - test if it works ;)
 * - unify the kim1 and a2600 drivers
 **********************************************************************/
#include "driver.h"
#include "mess/machine/riot.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x) if( errorlog ) fprintf x
#else
#define LOG(x) /* x */
#endif

struct RIOT {
	UINT8	dria;		/* Data register A input */
	UINT8	droa;		/* Data register A output */
	UINT8	ddra;		/* Data direction register A; 1 bits = output */
	UINT8	drib;		/* Data register B input */
	UINT8	drob;		/* Data register B output */
	UINT8	ddrb;		/* Data direction register B; 1 bits = output */
	UINT8	irqen;		/* IRQ enabled ? */
	UINT8	state;		/* current timer state (bit 7) */
	long double	clock;		/* baseclock/1(,8,64,1024) */
	void	*timer; 	/* timer callback */
    double  baseclock;  /* copied from interface */
	int (*port_a_r)(int chip);
	int (*port_b_r)(int chip);
	void (*port_a_w)(int chip, int data);
	void (*port_b_w)(int chip, int data);
	void (*irq_callback)(int chip);
};

static struct RIOT riot[MAX_RIOTS];

void riot_init(struct RIOTinterface *r)
{
	int i;
	for( i = 0; i < MAX_RIOTS && i < r->num_chips; i++ )
	{
		memset(&riot[i], 0, sizeof(struct RIOT));
		riot[i].baseclock = r->baseclock[i];
		riot[i].port_a_r = r->port_a_r[i];
		riot[i].port_b_r = r->port_b_r[i];
		riot[i].port_a_w = r->port_a_w[i];
		riot[i].port_b_w = r->port_b_w[i];
		riot[i].irq_callback = r->irq_callback[i];
    }
	LOG((errorlog, "RIOT - successfully initialised\n"));
}

int riot_r(int chip, int offset)
{
	int data = 0xff;
	LOG((errorlog, "RIOT read - offset is $%02x\n", offset));
	switch( offset )
    {
	case 0x0: case 0x8: /* Data register A */
		if( riot[chip].port_a_r )
			data = (*riot[chip].port_a_r)(chip);
		/* mask input bits and combine with output bits */
		data = (data & ~riot[chip].ddra) | (riot[chip].droa & riot[chip].ddra);
		LOG((errorlog, "riot(%d) DRA   read : $%02x\n", chip, data));
		break;
	case 0x1: case 0x9: /* Data direction register A */
		data = riot[chip].ddra;
		LOG((errorlog, "riot(%d) DDRA  read : $%02x\n", chip, data));
		break;
	case 0x2: case 0xa: /* Data register B */
		if( riot[chip].port_b_r )
			data = (*riot[chip].port_b_r)(chip);
		/* mask input bits and combine with output bits */
		data = (data & ~riot[chip].ddrb) | (riot[chip].drob & riot[chip].ddrb);
        LOG((errorlog, "riot(%d) DRB   read : $%02x\n", chip, data));
		break;
	case 0x3: case 0xb: /* Data direction register B */
		data = riot[chip].ddrb;
		LOG((errorlog, "riot(%d) DDRB  read : $%02x\n", chip, data));
		break;
	case 0x4: case 0xc: /* Timer count read (not supported?) */
		LOG((errorlog, "riot(%d) TIMR  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
		data = (int)(256 * timer_timeleft(riot[chip].timer) / TIME_IN_HZ(riot[chip].clock));
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		LOG((errorlog, "riot(%d) TIMR  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
		break;
	case 0x5: case 0xd: /* Timer count read (not supported?) */
		data = (int)(256 * timer_timeleft(riot[chip].timer) / TIME_IN_HZ(riot[chip].clock));
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		LOG((errorlog, "riot(%d) TIMR  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
		break;
	case 0x6: case 0xe: /* Timer count read */
		data = (int)(256 * timer_timeleft(riot[chip].timer) / TIME_IN_HZ(riot[chip].clock));
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		LOG((errorlog, "riot(%d) TIMR  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
		break;
	case 0x7: case 0xf: /* Timer status read */
		data = riot[chip].state;
		riot[chip].state &= ~0x80;
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		LOG((errorlog, "riot(%d) STAT  read : $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":"    ")));
		break;
    }
	return data;
}

static void riot_timer_cb(int chip)
{
	LOG((errorlog, "riot(%d) timer expired\n", chip));
	riot[chip].state |= 0x80;
	if( riot[chip].irqen ) /* with IRQ? */
	{
		if( riot[chip].irq_callback )
			(*riot[chip].irq_callback)(chip);
	}

}

void riot_w(int chip, int offset, int data)
{
	LOG((errorlog, "RIOT write - offset is $%02x\n", offset));
	switch( offset )
    {
	case 0x0: case 0x8: /* Data register A */
		LOG((errorlog, "riot(%d) DRA  write: $%02x\n", chip, data));
		riot[chip].droa = data;
		if( riot[chip].port_a_w )
			(*riot[chip].port_a_w)(chip,data);
		break;
	case 0x1: case 0x9: /* Data direction register A */
		LOG((errorlog, "riot(%d) DDRA  write: $%02x\n", chip, data));
		riot[chip].ddra = data;
		break;
	case 0x2: case 0xa: /* Data register B */
		LOG((errorlog, "riot(%d) DRB   write: $%02x\n", chip, data));
		riot[chip].drob = data;
		if( riot[chip].port_b_w )
			(*riot[chip].port_b_w)(chip,data);
        break;
	case 0x3: case 0xb: /* Data direction register B */
		LOG((errorlog, "riot(%d) DDRB  write: $%02x\n", chip, data));
		riot[chip].ddrb = data;
		break;
	case 0x4: case 0xc: /* Timer 1 start */
		LOG((errorlog, "riot(%d) TMR1  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
		riot[chip].state &= ~0x80;
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		if( riot[chip].timer )
			timer_remove(riot[chip].timer);
		riot[chip].clock = (double)riot[chip].baseclock / 1;
		riot[chip].timer = timer_pulse(TIME_IN_HZ((data+1) * riot[chip].clock / 256 / 256), chip, riot_timer_cb);
		break;
	case 0x5: case 0xd: /* Timer 8 start */
		LOG((errorlog, "riot(%d) TMR8  write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
		riot[chip].state &= ~0x80;
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		if( riot[chip].timer )
			timer_remove(riot[chip].timer);
		riot[chip].clock = (double)riot[chip].baseclock / 8;
		riot[chip].timer = timer_pulse(TIME_IN_HZ((data+1) * riot[chip].clock / 256 / 256), chip, riot_timer_cb);
        break;
	case 0x6: case 0xe: /* Timer 64 start */
		LOG((errorlog, "riot(%d) TMR64 write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
		riot[chip].state &= ~0x80;
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		if( riot[chip].timer )
			timer_remove(riot[chip].timer);
		riot[chip].clock = (double)riot[chip].baseclock / 64;
		riot[chip].timer = timer_pulse(TIME_IN_HZ((data+1) * riot[chip].clock / 256 / 256), chip, riot_timer_cb);
        break;
	case 0x7: case 0xf: /* Timer 1024 start */
		LOG((errorlog, "riot(%d) TMR1K write: $%02x%s\n", chip, data, (char*)((offset & 8) ? " (IRQ)":" ")));
		riot[chip].state &= ~0x80;
		riot[chip].irqen = (offset & 8) ? 1 : 0;
		if( riot[chip].timer )
			timer_remove(riot[chip].timer);
		riot[chip].clock = (double)riot[chip].baseclock / 1024;
		riot[chip].timer = timer_pulse(TIME_IN_HZ((data+1) * riot[chip].clock / 256 / 256), chip, riot_timer_cb);
        break;
    }
}


int riot_0_r(int offs) { return riot_r(0,offs); }
void riot_0_w(int offs, int data) { riot_w(0,offs,data); }
int riot_1_r(int offs) { return riot_r(1,offs); }
void riot_1_w(int offs, int data) { riot_w(1,offs,data); }
int riot_2_r(int offs) { return riot_r(2,offs); }
void riot_2_w(int offs, int data) { riot_w(2,offs,data); }
int riot_3_r(int offs) { return riot_r(3,offs); }
void riot_3_w(int offs, int data) { riot_w(3,offs,data); }

