/***************************************************************************

    TMP68301 basic emulation + Interrupt Handling

    The Toshiba TMP68301 is a 68HC000 + serial I/O, parallel I/O,
    3 timers, address decoder, wait generator, interrupt controller,
    all integrated in a single chip.

***************************************************************************/

#include "emu.h"
#include "machine/tmp68301.h"

const device_type TMP68301 = &device_creator<tmp68301_device>;

tmp68301_device::tmp68301_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: device_t(mconfig, TMP68301, "TMP68301", tag, owner, clock, "tmp68301", __FILE__)
{
}

//-------------------------------------------------
//  device_config_complete - perform any
//  operations now that the configuration is
//  complete
//-------------------------------------------------

void tmp68301_device::device_config_complete()
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void tmp68301_device::device_start()
{
	int i;
	for (i = 0; i < 3; i++)
		m_tmp68301_timer[i] = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(tmp68301_device::timer_callback), this));
}

//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void tmp68301_device::device_reset()
{
	int i;

	for (i = 0; i < 3; i++)
		m_IE[i] = 0;

	machine().firstcpu->set_irq_acknowledge_callback(device_irq_acknowledge_delegate(FUNC(tmp68301_device::irq_callback),this));
}


IRQ_CALLBACK_MEMBER(tmp68301_device::irq_callback)
{
	int vector = m_irq_vector[irqline];
//  logerror("%s: irq callback returns %04X for level %x\n",machine.describe_context(),vector,int_level);
	return vector;
}

TIMER_CALLBACK_MEMBER( tmp68301_device::timer_callback )
{
	int i = param;
	UINT16 TCR  =   m_regs[(0x200 + i * 0x20)/2];
	UINT16 IMR  =   m_regs[0x94/2];      // Interrupt Mask Register (IMR)
	UINT16 ICR  =   m_regs[0x8e/2+i];    // Interrupt Controller Register (ICR7..9)
	UINT16 IVNR =   m_regs[0x9a/2];      // Interrupt Vector Number Register (IVNR)

//  logerror("s: callback timer %04X, j = %d\n",machine.describe_context(),i,tcount);

	if  (   (TCR & 0x0004) &&   // INT
			!(IMR & (0x100<<i))
		)
	{
		int level = ICR & 0x0007;

		// Interrupt Vector Number Register (IVNR)
		m_irq_vector[level]  =   IVNR & 0x00e0;
		m_irq_vector[level]  +=  4+i;

		machine().firstcpu->set_input_line(level,HOLD_LINE);
	}

	if (TCR & 0x0080)   // N/1
	{
		// Repeat
		update_timer(i);
	}
	else
	{
		// One Shot
	}
}

void tmp68301_device::update_timer( int i )
{
	UINT16 TCR  =   m_regs[(0x200 + i * 0x20)/2];
	UINT16 MAX1 =   m_regs[(0x204 + i * 0x20)/2];
	UINT16 MAX2 =   m_regs[(0x206 + i * 0x20)/2];

	int max = 0;
	attotime duration = attotime::zero;

	m_tmp68301_timer[i]->adjust(attotime::never,i);

	// timers 1&2 only
	switch( (TCR & 0x0030)>>4 )                     // MR2..1
	{
	case 1:
		max = MAX1;
		break;
	case 2:
		max = MAX2;
		break;
	}

	switch ( (TCR & 0xc000)>>14 )                   // CK2..1
	{
	case 0: // System clock (CLK)
		if (max)
		{
			int scale = (TCR & 0x3c00)>>10;         // P4..1
			if (scale > 8) scale = 8;
			duration = attotime::from_hz(machine().firstcpu->unscaled_clock()) * ((1 << scale) * max);
		}
		break;
	}

//  logerror("%s: TMP68301 Timer %d, duration %lf, max %04X\n",machine().describe_context(),i,duration,max);

	if (!(TCR & 0x0002))                // CS
	{
		if (duration != attotime::zero)
			m_tmp68301_timer[i]->adjust(duration,i);
		else
			logerror("%s: TMP68301 error, timer %d duration is 0\n",machine().describe_context(),i);
	}
}

/* Update the IRQ state based on all possible causes */
void tmp68301_device::update_irq_state()
{
	int i;

	/* Take care of external interrupts */

	UINT16 IMR  =   m_regs[0x94/2];      // Interrupt Mask Register (IMR)
	UINT16 IVNR =   m_regs[0x9a/2];      // Interrupt Vector Number Register (IVNR)

	for (i = 0; i < 3; i++)
	{
		if  (   (m_IE[i]) &&
				!(IMR & (1<<i))
			)
		{
			UINT16 ICR  =   m_regs[0x80/2+i];    // Interrupt Controller Register (ICR0..2)

			// Interrupt Controller Register (ICR0..2)
			int level = ICR & 0x0007;

			// Interrupt Vector Number Register (IVNR)
			m_irq_vector[level]  =   IVNR & 0x00e0;
			m_irq_vector[level]  +=  i;

			m_IE[i] = 0;     // Interrupts are edge triggerred

			machine().firstcpu->set_input_line(level,HOLD_LINE);
		}
	}
}

READ16_MEMBER( tmp68301_device::regs_r )
{
	return m_regs[offset];
}

WRITE16_MEMBER( tmp68301_device::regs_w )
{
	COMBINE_DATA(&m_regs[offset]);

	if (!ACCESSING_BITS_0_7)    return;

//  logerror("CPU #0 PC %06X: TMP68301 Reg %04X<-%04X & %04X\n",space.device().safe_pc(),offset*2,data,mem_mask^0xffff);

	switch( offset * 2 )
	{
		// Timers
		case 0x200:
		case 0x220:
		case 0x240:
		{
			int i = ((offset*2) >> 5) & 3;

			update_timer( i );
		}
		break;
	}
}

void tmp68301_device::external_interrupt_0()    { m_IE[0] = 1;   update_irq_state(); }
void tmp68301_device::external_interrupt_1()    { m_IE[1] = 1;   update_irq_state(); }
void tmp68301_device::external_interrupt_2()    { m_IE[2] = 1;   update_irq_state(); }
