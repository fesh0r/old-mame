/***************************************************************************

	Amiga keyboard controller emulation

***************************************************************************/


#include "driver.h"
#include "amiga.h"
#include "amigakbd.h"
#include "machine/6526cia.h"


#define KEYBOARD_BUFFER_SIZE	256

static int kbd_index[4] = { 0, 1, 2, 3 };
static UINT8 *key_buf = NULL;
static int key_buf_pos = 0;
static int key_cur_pos = 0;
static emu_timer *kbd_timer;

static void kbd_sendscancode( running_machine *machine, UINT8 scancode )
{
	int j;

	/* send over to the cia A */
	for( j = 0; j < 8; j++ )
	{
		cia_set_input_cnt( machine, 0, 0 );	/* lower cnt */
		cia_set_input_sp( 0, ( scancode >> j ) & 1 ); /* set the serial data */
		cia_set_input_cnt( machine, 0, 1 );	/* raise cnt */
	}
}

static TIMER_CALLBACK( kbd_update_callback )
{
	UINT8	scancode;

	(void)param;

	/* if we don't have pending data, bail */

	if ( key_buf_pos == key_cur_pos )
		return;

	/* fetch the next scan code and send it to the Amiga */
	scancode = key_buf[key_cur_pos++];
	key_cur_pos %= KEYBOARD_BUFFER_SIZE;
	kbd_sendscancode( machine, scancode );

	/* if we still have more data, schedule another update */
	if ( key_buf_pos != key_cur_pos )
	{
		timer_adjust_oneshot(kbd_timer, attotime_div(video_screen_get_frame_period(machine->primary_screen),4), 0);
	}
}

static INPUT_CHANGED( kbd_update )
{
	int	index = *( (int*)(param) ), i;
	UINT32	oldvalue = oldval * field->mask, newvalue = newval * field->mask;
	UINT32	delta = oldvalue ^ newvalue;

	/* Special case Page UP, which we will use as Action Replay button */
	if ( (index == 3) && ( delta & 0x80000000 ) && ( newvalue & 0x80000000 ) )
	{
		const amiga_machine_interface *amiga_intf = amiga_get_interface();

		if ( amiga_intf != NULL && amiga_intf->nmi_callback )
		{
			(*amiga_intf->nmi_callback)();
		}
	}
	else
	{
		int key_buf_was_empty = ( key_buf_pos == key_cur_pos ) ? 1 : 0;

		for( i = 0; i < 32; i++ )
		{
			if ( delta & ( 1 << i ) )
			{
				int down = ( newvalue & ( 1 << i ) ) ? 0 : 1;
				int scancode = ( ( (index*32)+i ) << 1 ) | down;
				int amigacode = ~scancode;

				/* add the keycode to the buffer */
				key_buf[key_buf_pos++] = amigacode & 0xff;
				key_buf_pos %= KEYBOARD_BUFFER_SIZE;
			}
		}

		/* if the buffer was empty and we have new data, start a timer to send the keystrokes */
		if ( key_buf_was_empty && ( key_buf_pos != key_cur_pos ) )
		{
			timer_adjust_oneshot(kbd_timer, attotime_div(video_screen_get_frame_period(field->port->machine->primary_screen),4), 0);
		}
	}
}

void amigakbd_init( void )
{
	/* allocate a keyboard buffer */
	key_buf = auto_malloc( KEYBOARD_BUFFER_SIZE );
	key_buf_pos = 0;
	key_cur_pos = 0;
	kbd_timer = timer_alloc(kbd_update_callback, NULL);
	timer_reset( kbd_timer, attotime_never );
}

/*********************************************************************************************/

INPUT_PORTS_START( amiga_keyboard )
	PORT_START_TAG("amiga_keyboard_0")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_TILDE)		// 00
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_1)		// 01
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_2)		// 02
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_3)		// 03
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_4)		// 04
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_5)		// 05
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_6)		// 06
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_7)		// 07
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_8)		// 08
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_9)		// 09
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_0)		// 0A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_MINUS)		// 0B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_EQUALS)		// 0C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_BACKSLASH)	// 0D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[0])					// 0E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_0_PAD)		// 0F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_Q)		// 10
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_W)		// 11
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_E)		// 12
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_R)		// 13
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_T)		// 14
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_Y)		// 15
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_U)		// 16
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_I)		// 17
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_O)		// 18
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_P)		// 19
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_OPENBRACE)	// 1A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_CLOSEBRACE)	// 1B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[0])					// 1C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_1_PAD)		// 1D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_2_PAD)		// 1E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[0]) PORT_CODE(KEYCODE_3_PAD)		// 1F

	PORT_START_TAG("amiga_keyboard_1")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_A)		// 20
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_S)		// 21
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_D)		// 22
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_F)		// 23
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_G)		// 24
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_H)		// 25
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_J)		// 26
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_K)		// 27
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_L)		// 28
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_COLON)		// 29
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_QUOTE)		// 2A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[1])					// 2B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[1])					// 2C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_4_PAD)		// 2D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_5_PAD)		// 2E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_6_PAD)		// 2F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[1])					// 30
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_Z)		// 31
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_X)		// 32
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_C)		// 33
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_V)		// 34
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_B)		// 35
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_N)		// 36
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_M)		// 37
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_COMMA)		// 38
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_STOP)		// 39
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_SLASH)		// 3A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[1])					// 3B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_DEL_PAD)		// 3C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_7_PAD)		// 3D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_8_PAD)		// 3E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[1]) PORT_CODE(KEYCODE_9_PAD)		// 3F

	PORT_START_TAG("amiga_keyboard_2")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_SPACE)		// 40
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_BACKSPACE)	// 41
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_TAB)		// 42
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_ENTER_PAD)	// 43
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_ENTER)		// 44
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_ESC)		// 45
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_DEL)		// 46
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 47
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 48
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 49
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_MINUS_PAD)	// 4A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 4B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_UP)		// 4C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_DOWN)		// 4D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_RIGHT)		// 4E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_LEFT)		// 4F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F1)		// 50
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F2)		// 51
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F3)		// 52
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F4)		// 53
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F5)		// 54
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F6)		// 55
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F7)		// 56
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F8)		// 57
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F9)		// 58
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_F10)		// 59
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 5A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[2])					// 5B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_SLASH_PAD)	// 5C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_ASTERISK)		// 5D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_PLUS_PAD)		// 5E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[2]) PORT_CODE(KEYCODE_INSERT)		// 5F

	PORT_START_TAG("amiga_keyboard_3")
	PORT_BIT( 0x00000001, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_LSHIFT)		// 60
	PORT_BIT( 0x00000002, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_RSHIFT)		// 61
	PORT_BIT( 0x00000004, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_CAPSLOCK)		// 62
	PORT_BIT( 0x00000008, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_LCONTROL)		// 63
	PORT_BIT( 0x00000010, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_LALT)		// 64
	PORT_BIT( 0x00000020, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_RALT)		// 65
	PORT_BIT( 0x00000040, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_LWIN)		// 66
	PORT_BIT( 0x00000080, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3]) PORT_CODE(KEYCODE_RWIN)		// 67
	PORT_BIT( 0x00000100, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 68
	PORT_BIT( 0x00000200, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 69
	PORT_BIT( 0x00000400, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 6A
	PORT_BIT( 0x00000800, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 6B
	PORT_BIT( 0x00001000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 6C
	PORT_BIT( 0x00002000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 6D
	PORT_BIT( 0x00004000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 6E
	PORT_BIT( 0x00008000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 6F
	PORT_BIT( 0x00010000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 70
	PORT_BIT( 0x00020000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 71
	PORT_BIT( 0x00040000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 72
	PORT_BIT( 0x00080000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 73
	PORT_BIT( 0x00100000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 74
	PORT_BIT( 0x00200000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 75
	PORT_BIT( 0x00400000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 76
	PORT_BIT( 0x00800000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 77
	PORT_BIT( 0x01000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 78
	PORT_BIT( 0x02000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3]) 					// 79
	PORT_BIT( 0x04000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 7A
	PORT_BIT( 0x08000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 7B
	PORT_BIT( 0x10000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 7C
	PORT_BIT( 0x20000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 7D
	PORT_BIT( 0x40000000, IP_ACTIVE_HIGH, IPT_UNUSED) PORT_CHANGED(kbd_update, &kbd_index[3])					// 7E
	PORT_BIT( 0x80000000, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_CHANGED(kbd_update, &kbd_index[3])	PORT_CODE(KEYCODE_PGUP)		// 7F NMI button

INPUT_PORTS_END
