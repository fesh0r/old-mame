#include "driver.h"
#include "includes/uart8250.h"
#include "includes/pc_mouse.h"

static struct {

	PC_MOUSE_PROTOCOL protocol;

	int input_base;
	int serial_port; // -1 for deactivating yet
	int inputs;

	UINT8 queue[256];
	UINT8 head, tail, mb;

	void *timer;

} pc_mouse= { TYPE_MICROSOFT_MOUSE,12,-1 };

void pc_mouse_initialise(void)
{
	pc_mouse.head = pc_mouse.tail = 0;
	pc_mouse.timer = NULL;
	pc_mouse.inputs=UART8250_HANDSHAKE_IN_DSR|UART8250_HANDSHAKE_IN_CTS;
	if (pc_mouse.serial_port!=-1)
		uart8250_handshake_in(pc_mouse.serial_port, pc_mouse.inputs);
}

void pc_mouse_set_serial_port(int uart_index)
{
	pc_mouse.serial_port = uart_index;
}

void pc_mouse_set_input_base(int base)
{
	pc_mouse.input_base = base;
}

/* add data to queue */
void	pc_mouse_queue_data(int data)
{
	pc_mouse.queue[pc_mouse.head] = data;
	pc_mouse.head = ++pc_mouse.head % 256;
}

void	pc_mouse_set_protocol(PC_MOUSE_PROTOCOL protocol)
{
	pc_mouse.protocol = protocol;
}

/**************************************************************************
 *	Check for mouse moves and buttons. Build delta x/y packets
 **************************************************************************/
static void pc_mouse_scan(int n)
{
	static int ox = 0, oy = 0;
	int nx,ny;
    int dx, dy, nb;
	int can_scan = 0;
	int mbc;

	nx = readinputport((pc_mouse.input_base+1));
	if (nx>=0x800) nx-=0x1000;
	else if (nx<=-0x800) nx+=0x1000;

	dx = nx - ox;
	ox = nx;

	ny = readinputport((pc_mouse.input_base+2));
	if (ny>=0x800) ny-=0x1000;
	else if (ny<=-0x800) ny+=0x1000;

	dy = ny - oy;
	oy = ny;

	nb = readinputport(pc_mouse.input_base);
	mbc = nb^pc_mouse.mb;
	pc_mouse.mb = nb;

	if ( (pc_mouse.head==pc_mouse.tail) &&( dx || dy || (mbc!=0) ) )
	{
		can_scan = 1;
	}

	/* check if there is any delta or mouse buttons changed */
	if (can_scan)
	{
		switch (pc_mouse.protocol)
		{

		default:
		case TYPE_MICROSOFT_MOUSE:
			{
				/* split deltas into packtes of -128..+127 max */
				do
				{
					UINT8 m0, m1, m2;
					int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
					int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;
					m0 = 0x40 | ((nb << 4) & 0x30) | ((ddx >> 6) & 0x03) | ((ddy >> 4) & 0x0c);
					m1 = ddx & 0x3f;
					m2 = ddy & 0x3f;

					/* KT - changed to use a function */
					pc_mouse_queue_data(m0 | 0x40);
					pc_mouse_queue_data(m1 & 0x03f);
					pc_mouse_queue_data(m2 & 0x03f);
					dx -= ddx;
					dy -= ddy;
				} while( dx || dy );
			}
			break;

			/* mouse systems mouse
			from "PC Mouse information" by Tomi Engdahl */

			/*
			The data is sent in 5 byte packets in following format:
					D7      D6      D5      D4      D3      D2      D1      D0

			1.      1       0       0       0       0       LB      CB      RB
			2.      X7      X6      X5      X4      X3      X2      X1      X0
			3.      Y7      Y6      Y5      Y4      Y3      Y4      Y1      Y0
			4.      X7'     X6'     X5'     X4'     X3'     X2'     X1'     X0'
			5.      Y7'     Y6'     Y5'     Y4'     Y3'     Y4'     Y1'     Y0'

			LB is left button state (0=pressed, 1=released)
			CB is center button state (0=pressed, 1=released)
			RB is right button state (0=pressed, 1=released)
			X7-X0 movement in X direction since last packet in signed byte
				  format (-128..+127), positive direction right
			Y7-Y0 movement in Y direction since last packet in signed byte
				  format (-128..+127), positive direction up
			X7'-X0' movement in X direction since sending of X7-X0 packet in signed byte
				  format (-128..+127), positive direction right
			Y7'-Y0' movement in Y direction since sending of Y7-Y0 in signed byte
				  format (-128..+127), positive direction up

			The last two bytes in the packet (bytes 4 and 5) contains information about movement data changes which have occured after data butes 2 and 3 have been sent. */

			case TYPE_MOUSE_SYSTEMS:
			{

				dy =-dy;

				do
				{
					int ddx = (dx < -128) ? -128 : (dx > 127) ? 127 : dx;
					int ddy = (dy < -128) ? -128 : (dy > 127) ? 127 : dy;

					/* KT - changed to use a function */
					pc_mouse_queue_data(0x080 | (pc_mouse.mb & 0x07));
					pc_mouse_queue_data(ddx);
					pc_mouse_queue_data(ddy);
					/* for now... */
					pc_mouse_queue_data(0);
					pc_mouse_queue_data(0);
					dx -= ddx;
					dy -= ddy;
				} while( dx || dy );

			}
			break;
		}
    }

	if( pc_mouse.tail != pc_mouse.head )
	{
		int data;

		data = pc_mouse.queue[pc_mouse.tail];

		uart8250_receive(n, data);
		pc_mouse.tail = ++pc_mouse.tail & 255;
    }
}



/**************************************************************************
 *	Check for mouse control line changes and (de-)install timer
 **************************************************************************/
void pc_mouse_handshake_in(int n, int outputs)
{
    int new_msr = 0x00;

	if (n!=pc_mouse.serial_port) return;

    /* check if mouse port has DTR set */
	if( outputs & UART8250_HANDSHAKE_OUT_DTR )
		new_msr |= UART8250_HANDSHAKE_IN_DSR;	/* set DSR */

	/* check if mouse port has RTS set */
	if( outputs & UART8250_HANDSHAKE_OUT_RTS )
		new_msr |= UART8250_HANDSHAKE_IN_CTS;	/* set CTS */

	/* CTS changed state? */
	if (((pc_mouse.inputs^new_msr) & UART8250_HANDSHAKE_IN_CTS)!=0)
	{
		/* CTS just went to 1? */
		if ((new_msr & 0x010)!=0)
		{
			/* set CTS is now 1 */

			/* reset mouse */
			pc_mouse.head = pc_mouse.tail = pc_mouse.mb = 0;
			pc_mouse.queue[pc_mouse.head] = 'M';  /* put 'M' into the buffer.. hmm */
			pc_mouse.head = ++pc_mouse.head % 256;
			/* start a timer to scan the mouse input */
			pc_mouse.timer = timer_pulse(TIME_IN_HZ(240),
									  pc_mouse.serial_port, pc_mouse_scan);
		}
		else
		{
			/* CTS just went to 0 */
 			if( pc_mouse.timer )
				timer_remove(pc_mouse.timer);
			pc_mouse.timer = NULL;
			pc_mouse.head = pc_mouse.tail = 0;
		}
	}

	pc_mouse.inputs=new_msr;
	uart8250_handshake_in(pc_mouse.serial_port, new_msr);
}
