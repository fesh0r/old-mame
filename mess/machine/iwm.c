/* *************************************************************************
 * IWM (Integrated Woz Machine)
 *
 * Implementation of the IWM chip
 *
 * Nate Woods
 * Raphael Nabet
 *
 * Writing this code would not be possible if it weren't for the work of the
 * XGS and KEGS emulators which also contain IWM emulations
 *
 * This chip was used as the floppy disk controller for early Macs and the
 * Apple IIgs
 *
 * TODO
 * - Implement the unimplemented IWM modes (IWM_MODE_CLOCKSPEED,
 *   IWM_MODE_BITCELLTIME, IWM_MODE_HANDSHAKEPROTOCOLIWM_MODE_LATCHMODE
 * - Support 5 1/4" floppy drives, abstract drive interface
 *
 * CHANGES
 * - Separated the drive emulation from the IWM emulation.  This should enable us
 *  to implement other floppy drive interfaces (cf the weird interpretation
 *  of the enable line by Lisa2).  (R. Nabet 2000/11/18)
 * - Write support added (R. Nabet 2000/11)
 * *************************************************************************/


#include "iwm.h"

#ifdef MAME_DEBUG
#define LOG_IWM			1
#define LOG_IWM_EXTRA	0
#else
#define LOG_IWM			0
#define LOG_IWM_EXTRA	0
#endif

static int iwm_lines;		/* flags from IWM_MOTOR - IWM_Q7 */



/*
 * IWM mode (iwm_mode):	The IWM mode has the following values:
 *
 * Bit 7	  Reserved
 * Bit 6	  Reserved
 * Bit 5	  Reserved
 * Bit 4	! Clock speed
 *				0=7MHz;	used by apple IIgs
 *				1=8MHz;	used by mac (I believe)
 * Bit 3	! Bit cell time
 *				0=4usec/bit	(used for 5.25" drives)
 *				1=2usec/bit (used for 3.5" drives)
 * Bit 2	  Motor-off delay
 *				0=leave on for 1 sec after system turns it off
 *				1=turn off immediately
 * Bit 1	! Handshake protocol
 *				0=synchronous (software supplies timing for writing data; used for 5.25" drives)
 *				1=asynchronous (IWM supplies timing; used for 3.5" drives)
 * Bit 0	! Latch mode
 *				0=read data stays valid for 7usec (used for 5.25" drives)
 *				1=read data stays valid for full byte time (used for 3.5" drives)
 */
enum {
	IWM_MODE_CLOCKSPEED			= 0x10,
	IWM_MODE_BITCELLTIME		= 0x08,
	IWM_MODE_MOTOROFFDELAY		= 0x04,
	IWM_MODE_HANDSHAKEPROTOCOL	= 0x02,
	IWM_MODE_LATCHMODE			= 0x01
};

static int iwm_mode;		/* 0-31 */

iwm_interface iwm_intf;

/*
	IWM code
*/

void iwm_init(const iwm_interface *intf)
{
	iwm_lines = 0;
	iwm_mode = 0x1f;	/* default value needed by Lisa 2 - no, I don't know if it is true */
	iwm_intf = * intf;
}

/* R. Nabet : Next two functions look more like a hack than a real feature of the IWM */
/* I left them there because I had no idea what they intend to do.
They never get called when booting the Mac Plus driver. */
static int iwm_enable2(void)
{
	return (iwm_lines & IWM_PH1) && (iwm_lines & IWM_PH3);
}

static int iwm_readenable2handshake(void)
{
	static int val = 0;

	if (val++ > 3)
		val = 0;

	return val ? 0xc0 : 0x80;
}

static int iwm_statusreg_r(void)
{
	/* IWM status:
	 *
	 * Bit 7	Sense input (write protect for 5.25" drive and general status line for 3.5")
	 * Bit 6	Reserved
	 * Bit 5	Drive enable (is 1 if drive is on)
	 * Bits 4-0	Same as IWM mode bits 4-0
	 */

	int result;
	int status;

	status = /*iwm_enable2() ? 1 :*/ (*iwm_intf.read_status)();
	result = (status << 7) | (((iwm_lines & IWM_MOTOR) ? 1 : 0) << 5) | iwm_mode;
	return result;
}

static void iwm_modereg_w(int data)
{
	iwm_mode = data & 0x1f;	/* Write mode register */

	#if LOG_IWM_EXTRA
	logerror("iwm_modereg_w: iwm_mode=0x%02x\n", (int) iwm_mode);
	#endif
}

static int iwm_read_reg(void)
{
	int result = 0;

	switch(iwm_lines & (IWM_Q6 | IWM_Q7))
	{
	case 0:
		/* Read data register */
		/*if (iwm_enable2() || !(iwm_lines & IWM_MOTOR)) {
			result = 0xff;
		}
		else*/
		{
			/*
			 * Right now, this function assumes latch mode; which is always used for
			 * 3.5 inch drives.  Eventually we should check to see if latch mode is
			 * off
			 */
			#if LOG_IWM
				if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
					logerror("iwm_read_reg(): latch mode off not implemented\n");
			#endif

			result = (*iwm_intf.read_data)();
		}
		break;

	case IWM_Q6:
		/* Read status register */
		result = iwm_statusreg_r();
		break;

	case IWM_Q7:
		/* Read handshake register */
		result = iwm_enable2() ? iwm_readenable2handshake() : 0x80;
		break;
	}
	return result;
}

static void iwm_write_reg(int data)
{
	switch(iwm_lines & (IWM_Q6 | IWM_Q7))
	{
	case IWM_Q6 | IWM_Q7:
		if (!(iwm_lines & IWM_MOTOR))
		{
			iwm_modereg_w(data);
		}
		else /*if (!iwm_enable2())*/
		{
			/*
			 * Right now, this function assumes latch mode; which is always used for
			 * 3.5 inch drives.  Eventually we should check to see if latch mode is
			 * off
			 */
			#if LOG_IWM
				if ((iwm_mode & IWM_MODE_LATCHMODE) == 0)
					logerror("iwm_write_reg(): latch mode off not implemented\n");
			#endif

			(*iwm_intf.write_data)(data);
		}
		break;
	}
}

static void iwm_turnmotoroff(int dummy)
{
	iwm_lines &= ~IWM_MOTOR;

	(*iwm_intf.set_enable_lines)(0);

	#if LOG_IWM_EXTRA
		logerror("iwm_turnmotoroff(): Turning motor off\n");
	#endif
}

static void iwm_access(int offset)
{
#if LOG_IWM_EXTRA
	{
		static const char *lines[] =
		{
			"IWM_PH0",
			"IWM_PH1",
			"IWM_PH2",
			"IWM_PH3",
			"IWM_MOTOR",
			"IWM_DRIVE",
			"IWM_Q6",
			"IWM_Q7"
		};
		logerror("iwm_access(): %s line %s\n",
			(offset & 1) ? "setting" : "clearing", lines[offset >> 1]);
	}
#endif

	if (offset & 1)
		iwm_lines |= (1 << (offset >> 1));
	else
		iwm_lines &= ~(1 << (offset >> 1));

	if (offset < 0x08)
		(*iwm_intf.set_lines)(iwm_lines & 0x0f);

	switch(offset)
	{
	case 0x08:
		/* Turn off motor */
		if (iwm_mode & IWM_MODE_MOTOROFFDELAY)
		{
			/* Immediately */
			iwm_lines &= ~IWM_MOTOR;

			(*iwm_intf.set_enable_lines)(0);

			#if LOG_IWM_EXTRA
				logerror("iwm_access(): Turning motor off\n");
			#endif
		}
		else
		{
			/* One second delay */
			timer_set(TIME_IN_SEC(1), 0, iwm_turnmotoroff);
		}
		break;

	case 0x09:
		/* Turn on motor */
		iwm_lines |= IWM_MOTOR;

		(*iwm_intf.set_enable_lines)((iwm_lines & IWM_DRIVE) ? 2 : 1);

		#if LOG_IWM_EXTRA
			logerror("iwm_access(): Turning motor on\n");
		#endif
		break;

	case 0x0A:
		/* turn off IWM_DRIVE */
		if (iwm_lines & IWM_MOTOR)
			(*iwm_intf.set_enable_lines)(1);
		break;

	case 0x0B:
		/* turn on IWM_DRIVE */
		if (iwm_lines & IWM_MOTOR)
			(*iwm_intf.set_enable_lines)(2);
		break;
	}
}

int iwm_r(int offset)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_r: offset=%i\n", offset);
#endif

	iwm_access(offset);
	return (offset & 1) ? 0 : iwm_read_reg();
}

void iwm_w(int offset, int data)
{
	offset &= 15;

#if LOG_IWM_EXTRA
	logerror("iwm_w: offset=%i data=0x%02x\n", offset, data);
#endif

	iwm_access(offset);
	if ( offset & 1 )
		iwm_write_reg(data);
}


int iwm_get_lines(void)
{
	return iwm_lines & 0x0f;
}
