/**********************************************************************

  Copyright (C) Antoine Mine' 2008

   Hewlett Packard HP48 S/SX & G/GX

**********************************************************************/

#include "driver.h"
#include "timer.h"
#include "state.h"
#include "device.h"
#include "sound/dac.h"
#include "cpu/saturn/saturn.h"

#include "devices/xmodem.h"
#include "devices/kermit.h"

#include "includes/hp48.h"


/***************************************************************************
    DEBUGGING
***************************************************************************/


#define VERBOSE          0
#define VERBOSE_SERIAL   0

#define LOG(x)  do { if (VERBOSE) logerror x; } while (0)
#define LOG_SERIAL(x)  do { if (VERBOSE_SERIAL) logerror x; } while (0)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

/* model */
typedef enum {
	HP48_S,
	HP48_SX,
	HP48_G,
	HP48_GX,
} hp48_models;

/* memory module configuration */
typedef struct
{
	/* static part */
	UINT32 off_mask;             /* offset bit-mask, indicates the real size */
	read8_machine_func read;
	write8_machine_func write;
	void* data;                  /* non-NULL for banks */

	/* configurable part */
	UINT8  state;                /* one of HP48_MODULE_ */
	UINT32 base;                 /* base address */
	UINT32 mask;                 /* often improperly called size, it is an address select mask */

} hp48_module;


/* state field in hp48_module */
#define HP48_MODULE_UNCONFIGURED 0
#define	HP48_MODULE_MASK_KNOWN   1
#define HP48_MODULE_CONFIGURED   2

/* port specification */
struct hp48_port_config
{
	int port;                 /* port index: 0 or 1 (for port 1 and 2) */
	int module;               /* memory module where the port is visible */
	int max_size;             /* maximum size, in bytes 128 KB or 4 GB */

	/* device instance names */
	const char* brief_name;
	const char* name;

};


/***************************************************************************
    GLOBAL VARIABLES & CONSTANTS
***************************************************************************/

/* current HP48 model */
static hp48_models hp48_model;

#define HP48_G_SERIES ((hp48_model==HP48_G) || (hp48_model==HP48_GX))
#define HP48_S_SERIES ((hp48_model==HP48_S) || (hp48_model==HP48_SX))
#define HP48_X_SERIES ((hp48_model==HP48_SX) || (hp48_model==HP48_GX))
#define HP48_GX_MODEL (hp48_model==HP48_GX)

/* OUT register from SATURN (actually 12-bit) */
static UINT16 hp48_out;

/* keyboard interrupt */
static UINT8 hp48_kdn;

/* from highest to lowest priority: HDW, NCE2, CE1, CE2, NCE3, NCE1 */
static hp48_module hp48_modules[6];

static const char* hp48_module_names[6] = 
  { "HDW (I/O)", "NCE2 (RAM)", "CE1", "CE2", "NCE3", "NCE1 (ROM)" };

/* values returned by C=ID */
static const UINT8 hp48_module_mask_id[6] = { 0x00, 0x03, 0x05, 0x07, 0x01, 0x00 };
static const UINT8 hp48_module_addr_id[6] = { 0x19, 0xf4, 0xf6, 0xf8, 0xf2, 0x00 };

/* RAM/ROM extensions, GX/SX only (each UINT8 stores one nibble) 
   port1: SX/GX: 32/128 KB 
   port2: SX:32/128KB, GX:128/512/4096 KB
 */
static UINT32 hp48_port_size[2]; 
static UINT8  hp48_port_write[2];
static UINT8* hp48_port_data[2];
static UINT32 hp48_bank_switch;


/* I/O memory (each UINT8 stores one nibble) */
UINT8 hp48_io[64]; /* 64 nibbles */
static UINT32 hp48_io_addr; /* to known when to disable crc */

/* CRC state */
static UINT16 hp48_crc;

/* timers state */
static UINT8   hp48_timer1;
static UINT32  hp48_timer2;

#ifdef CHARDEV
#include "devices/chardev.h"
static chardev* hp48_chardev;
#endif


/***************************************************************************
    FUNCTIONS
***************************************************************************/

static void hp48_apply_modules( running_machine *machine, void* param );


/* ---------------- serial --------------------- */

#define RS232_DELAY ATTOTIME_IN_USEC( 300 )

/* end of receive event */
static TIMER_CALLBACK( hp48_rs232_byte_recv_cb )
{	
	LOG_SERIAL(( "%f hp48_rs232_byte_recv_cb: end of receive, data=%02x\n", 
		     attotime_to_double(timer_get_time()), param ));

	hp48_io[0x14] = param & 0xf; /* receive zone */
	hp48_io[0x15] = param >> 4;
	hp48_io[0x11] &= ~2; /* clear byte receiving */
	hp48_io[0x11] |= 1;  /* set byte received */

	/* interrupt */
	if ( hp48_io[0x10] & 2 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}
}

/* outside world initiates a receive event */
void hp48_rs232_start_recv_byte( running_machine *machine, UINT8 data )
{
	LOG_SERIAL(( "%f hp48_rs232_start_recv_byte: start receiving, data=%02x\n", 
		     attotime_to_double(timer_get_time()), data ));

	hp48_io[0x11] |= 2;  /* set byte receiving */
	
	/* interrupt */
	if ( hp48_io[0x10] & 1 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}

	/* schedule end of reception */
	timer_set( RS232_DELAY, NULL, data, hp48_rs232_byte_recv_cb );
}


/* end of send event */
static TIMER_CALLBACK( hp48_rs232_byte_sent_cb )
{	
	const device_config *xmodem = device_list_find_by_tag( machine->config->devicelist, XMODEM, "rs232-x" );
	const device_config *kermit = device_list_find_by_tag( machine->config->devicelist, KERMIT, "rs232-k" );

	LOG_SERIAL(( "%f hp48_rs232_byte_sent_cb: end of send, data=%02x\n", 
		     attotime_to_double(timer_get_time()), param ));

	hp48_io[0x12] &= ~3; /* clear byte sending and buffer full */

	/* interrupt */
	if ( hp48_io[0x10] & 4 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}

	/* protocol action */
	if ( xmodem && image_exists( xmodem ) ) xmodem_receive_byte( xmodem, param );
	else if ( kermit && image_exists( kermit ) ) kermit_receive_byte( kermit, param );
#ifdef CHARDEV
	else chardev_out( hp48_chardev, param );
#endif
}

/* CPU initiates a send event */
static void hp48_rs232_send_byte( running_machine *machine )
{
	const device_config *xmodem = device_list_find_by_tag( machine->config->devicelist, XMODEM, "rs232-x" );
	const device_config *kermit = device_list_find_by_tag( machine->config->devicelist, KERMIT, "rs232-k" );
	
	UINT8 data = HP48_IO_8(0x16); /* byte to send */

	LOG_SERIAL(( "%05x %f hp48_rs232_send_byte: start sending, data=%02x\n", 
		     activecpu_get_previouspc(), attotime_to_double(timer_get_time()), data ));

	hp48_io[0x12] |= 3;           /* set byte sending and send buffer full */

	/* schedule transmission */
	if ( (xmodem && image_exists( xmodem )) || (kermit && image_exists( kermit )) )
	{
		timer_set( RS232_DELAY, NULL, data, hp48_rs232_byte_sent_cb );
	}
#ifdef CHARDEV
	else
	{
		chardev_out( hp48_chardev, data );
	}
#endif
}


#ifdef CHARDEV

static TIMER_CALLBACK( hp48_chardev_byte_recv_cb )
{
	UINT8 data = chardev_in( hp48_chardev );

	LOG_SERIAL(( "%f hp48_chardev_byte_recv_cb: end of receive, data=%02x\n", 
		     attotime_to_double(timer_get_time()), data ));

	hp48_io[0x14] = data & 0xf; /* receive zone */
	hp48_io[0x15] = data >> 4;
	hp48_io[0x11] &= ~2; /* clear byte receiving */
	hp48_io[0x11] |= 1;  /* set byte received */

	/* interrupt */
	if ( hp48_io[0x10] & 2 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}
}

static void hp48_chardev_start_recv_byte( running_machine *machine, chardev_err status )
{
	if ( status != CHARDEV_OK ) return;

	LOG_SERIAL(( "%f hp48_chardev_start_recv_byte: start receiving\n", 
		     attotime_to_double(timer_get_time()) ));

	hp48_io[0x11] |= 2;  /* set byte receiving */
	
	/* interrupt */
	if ( hp48_io[0x10] & 1 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}

	/* schedule end of reception */
	timer_set( RS232_DELAY, NULL, 0, hp48_chardev_byte_recv_cb );
}

static void hp48_chardev_ready_to_send( running_machine *machine )
{
	hp48_io[0x12] &= ~3;  	

	/* interrupt */
	if ( hp48_io[0x10] & 4 )
	{
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
	}
}

static const chardev_interface hp48_chardev_iface = 
{ hp48_chardev_start_recv_byte, hp48_chardev_ready_to_send };

#endif


/* ------ Saturn's IN / OUT registers ---------- */


/* CPU sets OUT register (keyboard + beeper) */
void hp48_reg_out( running_machine* machine, int out )
{
	LOG(( "%05x %f hp48_reg_out: %03x\n",
	      activecpu_get_previouspc(), attotime_to_double(timer_get_time()), out ));

	/* bits 0-8: keyboard lines */
	hp48_out = out & 0x1ff;
	
	/* bits 9-10: unused */
	
	/* bit 11: beeper */
	dac_data_w( 0, (out & 0x800) ? 0x80 : 00 );
}

static int hp48_get_in( running_machine *machine )
{
	int in = 0;

	/* regular keys */
	if ( (hp48_out >> 0) & 1 ) in |= input_port_read( machine, "LINE0" );
	if ( (hp48_out >> 1) & 1 ) in |= input_port_read( machine, "LINE1" );
	if ( (hp48_out >> 2) & 1 ) in |= input_port_read( machine, "LINE2" );
	if ( (hp48_out >> 3) & 1 ) in |= input_port_read( machine, "LINE3" );
	if ( (hp48_out >> 4) & 1 ) in |= input_port_read( machine, "LINE4" );
	if ( (hp48_out >> 5) & 1 ) in |= input_port_read( machine, "LINE5" );
	if ( (hp48_out >> 6) & 1 ) in |= input_port_read( machine, "LINE6" );
	if ( (hp48_out >> 7) & 1 ) in |= input_port_read( machine, "LINE7" );
	if ( (hp48_out >> 8) & 1 ) in |= input_port_read( machine, "LINE8" );
	
	/* on key */
	in |= input_port_read( machine, "ON" );

	return in;
}

/* CPU reads IN register (keyboard) */
int hp48_reg_in( running_machine* machine )
{
	int in = hp48_get_in( machine );
	LOG(( "%05x %f hp48_reg_in: %04x\n",
	      activecpu_get_previouspc(), attotime_to_double(timer_get_time()), in ));
	return in;
}

/* key detect */
static void hp48_update_kdn( running_machine *machine )
{	
	int in = hp48_get_in( machine );

	/* interrupt on raising edge */
	if ( in && !hp48_kdn ) 
	{
		LOG(( "%f hp48_update_kdn: interrupt\n", attotime_to_double(timer_get_time()) ));
		hp48_io[0x19] |= 8;                                              /* service request */
		cpunum_set_input_line( machine, 0, SATURN_WAKEUP_LINE, PULSE_LINE );     /* wake-up */
		cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );      /* interrupt */
	}

	hp48_kdn = (in!=0);
}

/* periodic keyboard polling, generates an interrupt */
static TIMER_CALLBACK( hp48_kbd_cb )
{
	/* NMI for ON key */
	if ( input_port_read( machine, "ON" ) )
	{
		LOG(( "%f hp48_kbd_cb: keyboard interrupt, on key\n", 
		      attotime_to_double(timer_get_time()) ));	
		hp48_io[0x19] |= 8;                                          /* set service request */
		cpunum_set_input_line( machine, 0, SATURN_WAKEUP_LINE, PULSE_LINE );     /* wake-up */
		cpunum_set_input_line( machine, 0, SATURN_NMI_LINE, PULSE_LINE );      /* interrupt */
		return;
	}

        /* regular keys */
	hp48_update_kdn( machine );
}

/* RSI opcode */
void hp48_rsi( running_machine *machine )
{
	LOG(( "%05x %f hp48_rsi\n", activecpu_get_previouspc(), attotime_to_double(timer_get_time()) ));

	/* enables interrupts on key repeat 
	   (normally, there is only one interrupt, when the key is pressed)
	*/
	hp48_kdn = 0;
}


/* ------------- annonciators ------------ */

static void hp48_update_annunciators( running_machine *machine, void* param )
{
	/* bit 0: left shift
	   bit 1: right shift
	   bit 2: alpha
	   bit 3: alert
	   bit 4: busy
	   bit 5: transmit
	   bit 7: master enable
	*/
	int markers = HP48_IO_8(0xb);
	output_set_value( "lshift0",   (markers & 0x81) == 0x81 );
	output_set_value( "rshift0",   (markers & 0x82) == 0x82 );
	output_set_value( "alpha0",    (markers & 0x84) == 0x84 );
	output_set_value( "alert0",    (markers & 0x88) == 0x88 );
	output_set_value( "busy0",     (markers & 0x90) == 0x90 );
	output_set_value( "transmit0", (markers & 0xb0) == 0xb0 );
}


/* ------------- I/O registers ----------- */

/* Some part of the I/O registers are simple r/w registers. We store them in hp48_io.
   Special cases are registers that:
   - have a different meaning on read and write
   - perform some action on read / write
 */

static WRITE8_HANDLER ( hp48_io_w )
{
	LOG(( "%05x %f hp48_io_w: off=%02x data=%x\n", 
	      activecpu_get_previouspc(), attotime_to_double(timer_get_time()), offset, data ));

	switch( offset )
	{

	/* CRC register */
	case 0x04: hp48_crc = (hp48_crc & 0xfff0) | data; break;
	case 0x05: hp48_crc = (hp48_crc & 0xff0f) | (data << 4); break;
	case 0x06: hp48_crc = (hp48_crc & 0xf0ff) | (data << 8); break;
	case 0x07: hp48_crc = (hp48_crc & 0x0fff) | (data << 12); break;

	/* annunciators */
	case 0x0b:
	case 0x0c:
		hp48_io[offset] = data;
		hp48_update_annunciators( machine, NULL );
		break;

	/* cntrl ROM */
	case 0x29:
	{
		int old_cntrl = hp48_io[offset] & 8;
		hp48_io[offset] = data;
		if ( old_cntrl != (data & 8) ) 
		{
			hp48_apply_modules( machine, NULL );
		}
		break;
	}

	/* timers */
	case 0x37: hp48_timer1 = data; break;
	case 0x38: hp48_timer2 = (hp48_timer2 & 0xfffffff0) | data; break;
	case 0x39: hp48_timer2 = (hp48_timer2 & 0xffffff0f) | (data << 4); break;
	case 0x3a: hp48_timer2 = (hp48_timer2 & 0xfffff0ff) | (data << 8); break;
	case 0x3b: hp48_timer2 = (hp48_timer2 & 0xffff0fff) | (data << 12); break;
	case 0x3c: hp48_timer2 = (hp48_timer2 & 0xfff0ffff) | (data << 16); break;
	case 0x3d: hp48_timer2 = (hp48_timer2 & 0xff0fffff) | (data << 20); break;
	case 0x3e: hp48_timer2 = (hp48_timer2 & 0xf0ffffff) | (data << 24); break;
	case 0x3f: hp48_timer2 = (hp48_timer2 & 0x0fffffff) | (data << 28); break;

	/* cards */
	case 0x0e:
		LOG(( "%05x: card control write %02x\n", activecpu_get_previouspc(), data ));
		/* bit 0: software interrupt */
		if ( data & 1 )
		{
			LOG(( "%f hp48_io_w: software interrupt requested\n", 
			      attotime_to_double(timer_get_time()) ));
			cpunum_set_input_line( machine, 0, SATURN_IRQ_LINE, PULSE_LINE );
			data &= ~1;
		}

		/* XXX not implemented 
		    bit 1: card test?
		 */

		hp48_io[0x0e] = data;
		break;

	case 0x0f:
		LOG(( "%05x: card info write %02x\n", activecpu_get_previouspc(), data ));
		hp48_io[0x0f] = data;
		break;

	/* serial */
	case 0x13:
		hp48_io[0x11] &= ~4; /* clear error status */
		break;
	case 0x16:
		/* first nibble of sent data */
		hp48_io[offset] = data;
		break;
	case 0x17:
		/* second nibble of sent data */
		hp48_io[offset] = data;
		hp48_rs232_send_byte( machine );
		break;

	/* XXX not implemented:

	   - 0x0d: RS232c speed:
	      bits 0-2: speed
       	        000 = 1200 bauds
	        010 = 2400 bauds
	        100 = 4800 bauds
	        110 = 9600 bauds
              bit 3: ?

           - 0x1a: I/R input
              bit 0: irq
	      bit 1: irq enable
	      bit 2: 1=RS232c mode 0=direct mode
	      bit 3: receiving

	   - 0x1c: I/R output control
	      bit 0: buffer full
	      bit 1: transmitting
	      bit 2: irq enable on buffer empty
	      bit 3: led on (direct mode)

	   - 0x1d: I/R output buffer
	*/

	default: hp48_io[offset] = data;
	}

}


static READ8_HANDLER ( hp48_io_r )
{
	UINT8 data = 0;

	switch( offset )
	{
		
	/* CRC register */
	case 0x04: data = hp48_crc & 0xf; break;
	case 0x05: data = (hp48_crc >> 4) & 0xf; break;
	case 0x06: data = (hp48_crc >> 8) & 0xf; break;
	case 0x07: data = (hp48_crc >> 12) & 0xf; break;

	/* battery test */
	case 0x08:
		data = 0;
		if ( hp48_io[0x9] & 8 ) /* test enable */
		{
			/* XXX not implemented:
			   bit 3: battery in port 2			   
			   bit 2: battery in port 1
			 */
			switch ( input_port_read( machine, "BATTERY" ) )
			{
			case 1: data = 2; break; /* low */
			case 2: data = 3; break; /* low | critical */
			}
		}
		break;

	/* remaining lines in main bitmap */
	case 0x28:
	case 0x29:
	{
		int last_line = HP48_IO_8(0x28) & 0x3f; /* last line of main bitmap before menu */
		int cur_line = video_screen_get_vpos( machine->primary_screen ); 
		if ( last_line <= 1 ) last_line = 0x3f;
		data = ( cur_line >= 0 && cur_line <= last_line ) ? last_line - cur_line : 0;
		if ( offset == 0x29 )
		{
			data >>= 4;
			data |= HP48_IO_8(0x29) & 0xc0;
		}
		else
		{
			data &= 0xf;
		}
		break;
	}

	/* timers */
	case 0x37: data = hp48_timer1; break;
	case 0x38: data = hp48_timer2 & 0xf; break;
	case 0x39: data = (hp48_timer2 >> 4) & 0xf; break;
	case 0x3a: data = (hp48_timer2 >> 8) & 0xf; break;
	case 0x3b: data = (hp48_timer2 >> 12) & 0xf; break;
	case 0x3c: data = (hp48_timer2 >> 16) & 0xf; break;
	case 0x3d: data = (hp48_timer2 >> 20) & 0xf; break;
	case 0x3e: data = (hp48_timer2 >> 24) & 0xf; break;
	case 0x3f: data = (hp48_timer2 >> 28) & 0xf; break;

	/* serial */
	case 0x15: 
	{
                /* second nibble of received data */

		const device_config *xmodem = device_list_find_by_tag( machine->config->devicelist, XMODEM, "rs232-x" );
		const device_config *kermit = device_list_find_by_tag( machine->config->devicelist, KERMIT, "rs232-k" );

		hp48_io[0x11] &= ~1;  /* clear byte received */
		data = hp48_io[offset];
		
		/* protocol action */
		if ( xmodem && image_exists( xmodem ) ) xmodem_byte_transmitted( xmodem );
		else if ( kermit && image_exists( kermit ) ) kermit_byte_transmitted( kermit );
		break;
	}

	/* cards */
	case 0x0e: /* detection */
		data = hp48_io[0x0e];
		LOG(( "%05x: card control read %02x\n", activecpu_get_previouspc(), data ));
		break;
	case 0x0f: /* card info */
		data = 0;
		if ( HP48_G_SERIES )
		{
			if ( hp48_port_size[1] ) data |= 1;
			if ( hp48_port_size[0] ) data |= 2;
			if ( hp48_port_size[1] && hp48_port_write[1] ) data |= 4;
			if ( hp48_port_size[0] && hp48_port_write[0] ) data |= 8;
		}
		else
		{
			if ( hp48_port_size[0] ) data |= 1;
			if ( hp48_port_size[1] ) data |= 2;
			if ( hp48_port_size[0] && hp48_port_write[0] ) data |= 4;
			if ( hp48_port_size[1] && hp48_port_write[1] ) data |= 8;
		}
		LOG(( "%05x: card info read %02x\n", activecpu_get_previouspc(), data ));
		break;


	default: data = hp48_io[offset];
	}

	LOG(( "%05x %f hp48_io_r: off=%02x data=%x\n", 
	      activecpu_get_previouspc(), attotime_to_double(timer_get_time()), offset, data ));
	return data;
}


/* ---------- GX's bank switcher --------- */

static READ8_HANDLER ( hp48_bank_r )
{
	/* bit 0: ignored, bits 2-5: bank number, bit 6: enable */
	offset &= 0x7e;
	if ( hp48_bank_switch != offset )
	{
		LOG(( "%05x %f hp48_bank_r: off=%03x\n", activecpu_get_previouspc(), attotime_to_double(timer_get_time()), offset ));
		hp48_bank_switch = offset;
		hp48_apply_modules( machine, NULL );
	}
	return 0;
}


/* ---------------- timers --------------- */

static TIMER_CALLBACK( hp48_timer1_cb )
{
	if ( !(hp48_io[0x2f] & 1) ) return; /* timer enable */

	hp48_timer1 = (hp48_timer1 - 1) & 0xf;

	/* wake-up on carry */
	if ( (hp48_io[0x2e] & 4) && (hp48_timer1 == 0xf) )
	{
		LOG(( "wake-up on timer1\n" ));
		hp48_io[0x2e] |= 8;                                      /* set service request */
		hp48_io[0x18] |= 4;                                      /* set service request */
		cpunum_set_input_line( machine, 0, SATURN_WAKEUP_LINE, PULSE_LINE ); /* wake-up */
	}
	/* interrupt on carry */
	if ( (hp48_io[0x2e] & 2) && (hp48_timer1 == 0xf) )
	{
		LOG(( "generate timer1 interrupt\n" ));
		hp48_io[0x2e] |= 8; /* set service request */
		hp48_io[0x18] |= 4; /* set service request */
		cpunum_set_input_line(machine, 0, SATURN_NMI_LINE, PULSE_LINE);
	}
}

static TIMER_CALLBACK( hp48_timer2_cb )
{
	if ( !(hp48_io[0x2f] & 1) ) return; /* timer enable */

	hp48_timer2 = (hp48_timer2 - 1) & 0xffffffff;

	/* wake-up on carry */
	if ( (hp48_io[0x2f] & 4) && (hp48_timer2 == 0xffffffff) )
	{
		LOG(( "wake-up on timer2\n" ));
		hp48_io[0x2f] |= 8;                                      /* set service request */
		hp48_io[0x18] |= 4;                                      /* set service request */
		cpunum_set_input_line( machine, 0, SATURN_WAKEUP_LINE, PULSE_LINE ); /* wake-up */
	}
	/* interrupt on carry */
	if ( (hp48_io[0x2f] & 2) && (hp48_timer2 == 0xffffffff) )
	{
		LOG(( "generate timer2 interrupt\n" ));
		hp48_io[0x2f] |= 8;                                      /* set service request */
		hp48_io[0x18] |= 4;                                      /* set service request */
		cpunum_set_input_line(machine, 0, SATURN_NMI_LINE, PULSE_LINE);
	}
}




/* --------- memory controller ----------- */

/* 
   Clark (S series) and York (G series) CPUs have 6 daisy-chainedmodules


   <-- highest --------- priority ------------- lowest -->

   CPU ---------------------------------------------------
            |      |      |          |          |        |
           HDW    NCE2   CE1        CE2        NCE3     NCE1


   However, controller usage is different in both series:


      controller     S series        G series
                    (Clark CPU)     (York CPU)

         HDW         I/O RAM          I/O RAM
        NCE2           RAM              RAM
         CE1           port1        bank switcher
         CE2           port2           port1
        NCE3          unused           port2
        NCE1           ROM              ROM


   - NCE1 (ROM) cannot be configured, it is always visible at addresses
   00000-7ffff not covered by higher priority modules.

   - only the address of HDW (I/O) can be configured, its size is constant
   (64 nibbles)

   - other modules can have their address & size set

 */


/* remap all modules according to hp48_modules */
static void hp48_apply_modules( running_machine *machine, void* param )
{
	int i;
	int nce2_enable = 1;

	hp48_io_addr = 0x100000;

	if ( HP48_G_SERIES )
	{
		/* port 2 bank switch */
		if ( hp48_port_size[1] > 0 )
		{
			int off = (hp48_bank_switch << 16) % hp48_port_size[1];
			hp48_modules[4].data = hp48_port_data[1] + off;
		}

		/* ROM A19 (hi 256 KB) / NCE2 (port 2) control switch */
		if ( hp48_io[0x29] & 8 )
		{
			/* A19 */
			hp48_modules[5].off_mask = 0xfffff;
			nce2_enable = 0;
		}
		else
		{
			/* NCE2 */
			hp48_modules[5].off_mask = 0x7ffff;
			nce2_enable = hp48_bank_switch >> 6; 
		}
	}

	/* S series ROM mapping compatibility */
	if ( HP48_S_SERIES || !(hp48_io[0x29] & 8) )
	{
		hp48_modules[5].off_mask = 0x7ffff;
	}
	else
	{
		hp48_modules[5].off_mask = 0xfffff;
	}

	/* from lowest to highest priority */
	for ( i = 5; i >= 0; i-- )
	{
		UINT32 select_mask = hp48_modules[i].mask;
		UINT32 nselect_mask = ~select_mask & 0xfffff;
		UINT32 base = hp48_modules[i].base;
		UINT32 off_mask = hp48_modules[i].off_mask;
		UINT32 mirror = nselect_mask & ~off_mask;
		UINT32 end = base + (off_mask & nselect_mask);

		if ( hp48_modules[i].state != HP48_MODULE_CONFIGURED ) continue;

		if ( (i == 4) && !nce2_enable ) continue;

		/* our code assumes that the 20-bit select_mask is all 1s followed by all 0s */
		if ( nselect_mask & (nselect_mask+1) )
		{
			logerror( "hp48_mem_config: invalid mask %05x for module %s\n",
				  select_mask, hp48_module_names[i] );
			continue;
		}
		memory_install_read8_handler( machine, 0, ADDRESS_SPACE_PROGRAM, base, end, 0, mirror, hp48_modules[i].read );
		memory_install_write8_handler( machine, 0, ADDRESS_SPACE_PROGRAM, base, end, 0, mirror, hp48_modules[i].write );

		LOG(( "hp48_mem_config: module %s configured at %05x-%05x, mirror %05x\n",
		      hp48_module_names[i], base, end, mirror ));

		if ( hp48_modules[i].data )
		{
			memory_set_bankptr( i, hp48_modules[i].data );
		}

		if ( i == 0 )
		{
			hp48_io_addr = base;
		}
	}
}


/* reset the configuration */
static void hp48_reset_modules( running_machine *machine )
{
	int i;
	/* fixed size for HDW */
	hp48_modules[0].state = HP48_MODULE_MASK_KNOWN;
	hp48_modules[0].mask = 0xfffc0;
	/* unconfigure NCE2, CE1, CE2, NCE3 */
	for ( i = 1; i < 5; i++ )
	{
		hp48_modules[i].state = HP48_MODULE_UNCONFIGURED;
	}

	/* fixed configuration for NCE1 */
	hp48_modules[5].state = HP48_MODULE_CONFIGURED;
	hp48_modules[5].base = 0;
	hp48_modules[5].mask = 0;

	hp48_apply_modules( machine, NULL );
}


/* RESET opcode */
void hp48_mem_reset( running_machine* machine )
{
	LOG(( "%05x %f hp48_mem_reset\n", activecpu_get_previouspc(), attotime_to_double(timer_get_time()) ));
	hp48_reset_modules( machine );
}


/* CONFIG opcode */
void hp48_mem_config( running_machine* machine, int v )
{	
	int i;

	LOG(( "%05x %f hp48_mem_config: %05x\n", activecpu_get_previouspc(), attotime_to_double(timer_get_time()), v ));

	/* find the highest priority unconfigured module (except non-configurable NCE1)... */
	for ( i = 0; i < 5; i++ )
	{
		/* ... first call sets the address mask */
		if ( hp48_modules[i].state == HP48_MODULE_UNCONFIGURED )
		{
			hp48_modules[i].mask = v & 0xff000;
			hp48_modules[i].state = HP48_MODULE_MASK_KNOWN;
			break;
		}

		/* ... second call sets the base address */
		if ( hp48_modules[i].state == HP48_MODULE_MASK_KNOWN ) 
		{
			hp48_modules[i].base = v & hp48_modules[i].mask;
			hp48_modules[i].state = HP48_MODULE_CONFIGURED;
			LOG(( "hp48_mem_config: module %s configured base=%05x, mask=%05x\n",
			      hp48_module_names[i], hp48_modules[i].base, hp48_modules[i].mask ));
			hp48_apply_modules( machine, NULL );
			break;
		}
	}
}


/* UNCFG opcode */
void hp48_mem_unconfig( running_machine* machine, int v )
{	
	int i;
	LOG(( "%05x %f hp48_mem_unconfig: %05x\n", activecpu_get_previouspc(), attotime_to_double(timer_get_time()), v ));

	/* find the highest priority fully configured module at address v (except NCE1)... */
	for ( i = 0; i < 5; i++ )
	{
		/* ... and unconfigure it */
		if ( hp48_modules[i].state == HP48_MODULE_CONFIGURED && 
		     (hp48_modules[i].base == (v & hp48_modules[i].mask)) )
		{
			hp48_modules[i].state = i> 0 ? HP48_MODULE_UNCONFIGURED : HP48_MODULE_MASK_KNOWN;
			LOG(( "hp48_mem_unconfig: module %s\n", hp48_module_names[i] ));
			hp48_apply_modules( machine, NULL );
			break;
		}
	}
}


/* C=ID opcode */
int  hp48_mem_id( running_machine* machine )
{
	int i;
	int data = 0; /* 0 = everything is configured */

	/* find the highest priority unconfigured module (except NCE1)... */
	for ( i = 0; i < 5; i++ )
	{
		/* ... mask need to be configured */
		if ( hp48_modules[i].state == HP48_MODULE_UNCONFIGURED )
		{
			data = hp48_module_mask_id[i] | (hp48_modules[i].mask & ~0xff);
			break;
		}

		/* ... address need to be configured */
		if ( hp48_modules[i].state == HP48_MODULE_MASK_KNOWN ) 
		{
			data = hp48_module_addr_id[i] | (hp48_modules[i].base & ~0x3f);
			break;
		}
	}

	LOG(( "%05x %f hp48_mem_id = %02x\n", 
	      activecpu_get_previouspc(), attotime_to_double(timer_get_time()), data ));

	return data; /* everything is configured */
}



/* --------- CRC ---------- */

/* each memory read by the CPU updates the internal CRC state */
void hp48_mem_crc( running_machine* machine, int addr, int data )
{
	/* no CRC for I/O RAM */
	if ( addr >= hp48_io_addr && addr < hp48_io_addr + 0x40 ) return;

	hp48_crc = (hp48_crc >> 4) ^ (((hp48_crc ^ data) & 0xf) * 0x1081);
}



/* ------ utilities ------- */


/* decodes size bytes into 2*size nibbles (lowest significant first) */
static void hp48_decode_nibble( UINT8* dst, UINT8* src, int size )
{
	int i;
	for ( i=size-1; i >= 0; i-- ) 
	{
		dst[2*i+1] = src[i] >> 4;
		dst[2*i] = src[i] & 0xf;
	}
}

/* inverse of hp48_decode_nibble  */
static void hp48_encode_nibble( UINT8* dst, UINT8* src, int size )
{
	int i;
	for ( i=0; i < size; i++ ) 
	{
		dst[i] = (src[2*i] & 0xf) | (src[2*i+1] << 4);
	}
}



/* ----- card images ------ */

/* port information configurations */
struct hp48_port_config hp48sx_port1_config = { 0, 2,    128*1024, "p1", "port1" };
struct hp48_port_config hp48sx_port2_config = { 1, 3,    128*1024, "p2", "port2" };
struct hp48_port_config hp48gx_port1_config = { 0, 3,    128*1024, "p1", "port1" };
struct hp48_port_config hp48gx_port2_config = { 1, 4, 4*1024*1024, "p2", "port2" };

/* helper for load and create */
static void hp48_fill_port( const device_config* image )
{
	struct hp48_port_config* conf = (struct hp48_port_config*) image->static_config;
	int size = hp48_port_size[conf->port];
	LOG(( "hp48_fill_port: %s module=%i size=%i rw=%i\n", conf->name, conf->module, size, hp48_port_write[conf->port] ));
	hp48_port_data[conf->port] = malloc( 2 * size );
	memset( hp48_port_data[conf->port], 0, 2 * size );
	hp48_modules[conf->module].off_mask = 2 * (( size > 128 * 1024 ) ? 128 * 1024 : size) - 1;
	hp48_modules[conf->module].read     = SMH_BANK((FPTR)conf->module);
	hp48_modules[conf->module].write    = hp48_port_write[conf->port] ? (void*)SMH_BANK((FPTR)conf->module) : SMH_NOP;
	hp48_modules[conf->module].data     = hp48_port_data[conf->port];
	hp48_apply_modules( image->machine, NULL );
}

/* helper for start and unload */
static void hp48_unfill_port( const device_config* image )
{
	struct hp48_port_config* conf = (struct hp48_port_config*) image->static_config;
	hp48_modules[conf->module].off_mask = 0x00fff;  /* 2 KB */
	hp48_modules[conf->module].read     = SMH_UNMAP;
	hp48_modules[conf->module].write    = SMH_UNMAP;
	hp48_modules[conf->module].data     = NULL;
}


static DEVICE_IMAGE_LOAD( hp48_port )
{
	struct hp48_port_config* conf = (struct hp48_port_config*) image->static_config;
	int size = image_length( image );
	if ( size == 0 ) size = conf->max_size; /* default size */

	/* check size */
	if ( (size < 32*1024) || (size > conf->max_size) || (size & (size-1)) )
	{
		logerror( "hp48: image size for %s should be a power of two between %i and %i\n", conf->name, 32*1024, conf->max_size );
		return INIT_FAIL;
	}

	hp48_port_size[conf->port] = size;
	hp48_port_write[conf->port] = image_is_writable( image );
	hp48_fill_port( image );
	image_fread( image, hp48_port_data[conf->port], hp48_port_size[conf->port] );
	hp48_decode_nibble( hp48_port_data[conf->port], hp48_port_data[conf->port], hp48_port_size[conf->port] );
	return INIT_PASS;
}

static DEVICE_IMAGE_CREATE( hp48_port )
{
	struct hp48_port_config* conf = (struct hp48_port_config*) image->static_config;
	int size = conf->max_size; 
        /* XXX defaults to max_size; get user-specified size instead */

	/* check size */
	/* size must be a power of 2 between 32K and max_size */
	if ( (size < 32*1024) || (size > conf->max_size) || (size & (size-1)) )
	{
		logerror( "hp48: image size for %s should be a power of two between %i and %i\n", conf->name, 32*1024, conf->max_size );
		return INIT_FAIL;
	}	

	hp48_port_size[conf->port] = size;	
	hp48_port_write[conf->port] = 1;
	hp48_fill_port( image );
	return INIT_PASS;
}

static DEVICE_IMAGE_UNLOAD( hp48_port )
{
	struct hp48_port_config* conf = (struct hp48_port_config*) image->static_config;
	LOG(( "hp48_port image unload: %s size=%i rw=%i\n", 
	      conf->name, hp48_port_size[conf->port] ,hp48_port_write[conf->port] ));
	if ( hp48_port_write[conf->port] )
	{
		hp48_encode_nibble( hp48_port_data[conf->port], hp48_port_data[conf->port], hp48_port_size[conf->port] );
		image_fseek( image, 0, SEEK_SET );
		image_fwrite( image, hp48_port_data[conf->port], hp48_port_size[conf->port] );
	}
	free( hp48_port_data[conf->port] );
	hp48_unfill_port( image );
	hp48_apply_modules( image->machine, NULL );
}

static DEVICE_START( hp48_port )
{
	hp48_unfill_port( device );
	return DEVICE_START_OK;
}

DEVICE_GET_INFO( hp48_port )
{
	struct hp48_port_config* conf = device ? (struct hp48_port_config*) device->static_config : NULL;
	switch ( state ) 
	{
	case DEVINFO_INT_TOKEN_BYTES:                 info->i = 1;                                               break;
	case DEVINFO_INT_INLINE_CONFIG_BYTES:         info->i = 0;                                               break;
	case DEVINFO_INT_CLASS:	                      info->i = DEVICE_CLASS_PERIPHERAL;                         break;
	case DEVINFO_INT_IMAGE_TYPE:	              info->i = IO_MEMCARD;                                      break;
	case DEVINFO_INT_IMAGE_READABLE:              info->i = 1;                                               break;
	case DEVINFO_INT_IMAGE_WRITEABLE:	      info->i = 1;                                               break;
	case DEVINFO_INT_IMAGE_CREATABLE:	      info->i = 1;                                               break;
	case DEVINFO_FCT_START:		              info->start = DEVICE_START_NAME( hp48_port );              break;
	case DEVINFO_FCT_IMAGE_LOAD:		      info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( hp48_port );    break;
	case DEVINFO_FCT_IMAGE_UNLOAD:		      info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME( hp48_port );  break;
	case DEVINFO_FCT_IMAGE_CREATE:		      info->f = (genf *) DEVICE_IMAGE_CREATE_NAME( hp48_port );  break;
	case DEVINFO_STR_IMAGE_BRIEF_INSTANCE_NAME:   info->s = conf ? conf->brief_name : "";                    break;
	case DEVINFO_STR_IMAGE_INSTANCE_NAME:         info->s = conf ? conf->name : "";                          break;
	case DEVINFO_STR_NAME:		              info->s = "HP48 memory card";	                         break;
	case DEVINFO_STR_FAMILY:                      info->s = "HP48 memory card";	                         break;
	case DEVINFO_STR_SOURCE_FILE:		      info->s = __FILE__;                                        break;
	case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	      info->s = "crd";                                           break;
	}
}




/***************************************************************************
    MACHINES
***************************************************************************/

DRIVER_INIT( hp48 )
{
	int i;
	LOG(( "hp48: driver init called\n" ));
	for ( i = 0; i < 6; i++ )
	{
		hp48_modules[i].off_mask = 0x00fff;  /* 2 KB */
		hp48_modules[i].read     = SMH_UNMAP;
		hp48_modules[i].write    = SMH_UNMAP;
		hp48_modules[i].data     = NULL;
	}
	hp48_port_size[0] = 0;
	hp48_port_size[1] = 0;
}

MACHINE_RESET( hp48 )
{
	LOG(( "hp48: machine reset called\n" ));
	hp48_reset_modules( machine );
	hp48_update_annunciators( machine, NULL );
}

static void hp48_machine_start( running_machine *machine, hp48_models model )
{
	UINT8* rom, *ram;
	int ram_size, rom_size, i;

	LOG(( "hp48_machine_start: model %i\n", model ));

	hp48_model = model;

	/* internal RAM */
	ram_size = HP48_GX_MODEL ? (128 * 1024) : (32 * 1024);
	generic_nvram_size = 2 * ram_size;
	generic_nvram = auto_malloc( generic_nvram_size );
	ram = (UINT8*) generic_nvram;
	memset( generic_nvram, 0, generic_nvram_size );

	/* ROM load */
	rom_size = HP48_S_SERIES ? (256 * 1024) : (512 * 1024);
	rom = auto_malloc( 2 * rom_size );
	hp48_decode_nibble( rom, memory_region( machine, "main" ), rom_size );

	/* init state */
	memset( generic_nvram, 0, generic_nvram_size );
	memset( hp48_io, 0, sizeof( hp48_io ) );
	hp48_out = 0;
	hp48_kdn = 0;
	hp48_crc = 0;
	hp48_timer1 = 0;
	hp48_timer2 = 0;
	hp48_bank_switch = 0;

	/* static module configuration */

	/* I/O RAM */
	hp48_modules[0].off_mask = 0x0003f;  /* 32 B */
	hp48_modules[0].read     = hp48_io_r;
	hp48_modules[0].write    = hp48_io_w;

	/* internal RAM */
	hp48_modules[1].off_mask = 2 * ram_size - 1;
	hp48_modules[1].read     = SMH_BANK1;
	hp48_modules[1].write    = SMH_BANK1;
	hp48_modules[1].data     = ram;

	if ( HP48_G_SERIES ) 
	{
                /* bank switcher */
		hp48_modules[2].off_mask = 0x00fff;  /* 2 KB */
		hp48_modules[2].read     = hp48_bank_r;
	}

	/* ROM */
	hp48_modules[5].off_mask = 2 * rom_size - 1;
	hp48_modules[5].read     = SMH_BANK5;
	hp48_modules[5].data     = rom;

	/* timers */
	timer_pulse( ATTOTIME_IN_HZ( 16 ),   NULL, 0, hp48_timer1_cb );
	timer_pulse( ATTOTIME_IN_HZ( 8192 ), NULL, 0, hp48_timer2_cb );

	/* 1ms keyboard polling */
	timer_pulse( ATTOTIME_IN_MSEC( 1 ), NULL, 0, hp48_kbd_cb );

	/* save state */
	state_save_register_global( hp48_out );
	state_save_register_global( hp48_kdn );
	state_save_register_global( hp48_io_addr );
	state_save_register_global( hp48_crc );
	state_save_register_global( hp48_timer1 );
	state_save_register_global( hp48_timer2 );
	state_save_register_global( hp48_bank_switch );
	for ( i = 0; i < 6; i++ ) 
	{
		state_save_register_item( "globals", i, hp48_modules[i].state );
		state_save_register_item( "globals", i, hp48_modules[i].base );
		state_save_register_item( "globals", i, hp48_modules[i].mask );
	}
	state_save_register_global_array( hp48_io );
	state_save_register_global_pointer( generic_nvram, generic_nvram_size );

	state_save_register_postload( machine, hp48_update_annunciators, NULL );
	state_save_register_postload( machine, hp48_apply_modules, NULL );

#ifdef CHARDEV
	/* direct I/O */
	hp48_chardev = chardev_open_pty( machine, &hp48_chardev_iface );
#endif
}


MACHINE_START( hp48s )
{
	hp48_machine_start( machine, HP48_S );
}


MACHINE_START( hp48sx )
{
	hp48_machine_start( machine, HP48_SX );
}


MACHINE_START( hp48g )
{
	hp48_machine_start( machine, HP48_G );
}

MACHINE_START( hp48gx )
{
	hp48_machine_start( machine, HP48_GX );
}

