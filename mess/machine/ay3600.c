/***************************************************************************

  AY3600.c

  Machine file to handle emulation of the AY-3600 keyboard controller.

  TODO:
	-Make the caps lock functional, remove dependency on input_port_1.
	 Caps lock now functional. Barry Nelson 04/05/2001
	 Still dependent on input_port_1 though.
	-Rename variables from a2 to AY3600
	 Done. Barry Nelson 04/05/2001
	Find the correct MAGIC_KEY_REPEAT_NUMBER
	Use the keyboard ROM for building a remapping table?

***************************************************************************/

#include "driver.h"
#include "machine/ay3600.h"
#include "includes/apple2.h"

#ifdef MAME_DEBUG
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif /* MAME_DEBUG */

static void AY3600_poll(int dummy);

static unsigned char ay3600_key_remap[2][7*8][4] = {
/* caps lock off  norm ctrl shft both */
	{
		{ 0x7f,0x7f,0x7f,0x7f },	/* Backspace	*/
		{ 0x08,0x08,0x08,0x08 },	/* Left 		*/
		{ 0x09,0x09,0x09,0x09 },	/* Tab			*/
		{ 0x0a,0x0a,0x0a,0x0a },	/* Down 		*/
		{ 0x0b,0x0b,0x0b,0x0b },	/* Up			*/
		{ 0x0d,0x0d,0x0d,0x0d },	/* Enter		*/
		{ 0x15,0x15,0x15,0x15 },	/* Right		*/
		{ 0x1b,0x1b,0x1b,0x1b },	/* Escape		*/
		{ 0x20,0x20,0x20,0x20 },	/* Space		*/
		{ 0x27,0x27,0x22,0x22 },	/* ' "          */
		{ 0x2c,0x2c,0x3c,0x3c },	/* , <			*/
		{ 0x2d,0x2d,0x5f,0x1f },	/* - _			*/
		{ 0x2e,0x2e,0x3e,0x3e },	/* . >			*/
		{ 0x2f,0x2f,0x3f,0x3f },	/* / ?			*/
		{ 0x30,0x30,0x29,0x29 },	/* 0 )			*/
		{ 0x31,0x31,0x21,0x21 },	/* 1 !			*/
		{ 0x32,0x00,0x40,0x00 },	/* 2 @			*/
		{ 0x33,0x33,0x23,0x23 },	/* 3 #			*/
		{ 0x34,0x34,0x24,0x24 },	/* 4 $			*/
		{ 0x35,0x35,0x25,0x25 },	/* 5 %			*/
		{ 0x36,0x1e,0x5e,0x1e },	/* 6 ^			*/
		{ 0x37,0x37,0x26,0x26 },	/* 7 &			*/
		{ 0x38,0x38,0x2a,0x2a },	/* 8 *			*/
		{ 0x39,0x39,0x28,0x28 },	/* 9 (			*/
		{ 0x3b,0x3b,0x3a,0x3a },	/* ; :			*/
		{ 0x3d,0x3d,0x2b,0x2b },	/* = +			*/
		{ 0x5b,0x1b,0x7b,0x1b },	/* [ {			*/
		{ 0x5c,0x1c,0x7c,0x1c },	/* \ |			*/
		{ 0x5d,0x1d,0x7d,0x1d },	/* ] }			*/
		{ 0x60,0x60,0x7e,0x7e },	/* ` ~			*/
		{ 0x61,0x01,0x41,0x01 },	/* a A			*/
		{ 0x62,0x02,0x42,0x02 },	/* b B			*/
		{ 0x63,0x03,0x43,0x03 },	/* c C			*/
		{ 0x64,0x04,0x44,0x04 },	/* d D			*/
		{ 0x65,0x05,0x45,0x05 },	/* e E			*/
		{ 0x66,0x06,0x46,0x06 },	/* f F			*/
		{ 0x67,0x07,0x47,0x07 },	/* g G			*/
		{ 0x68,0x08,0x48,0x08 },	/* h H			*/
		{ 0x69,0x09,0x49,0x09 },	/* i I			*/
		{ 0x6a,0x0a,0x4a,0x0a },	/* j J			*/
		{ 0x6b,0x0b,0x4b,0x0b },	/* k K			*/
		{ 0x6c,0x0c,0x4c,0x0c },	/* l L			*/
		{ 0x6d,0x0d,0x4d,0x0d },	/* m M			*/
		{ 0x6e,0x0e,0x4e,0x0e },	/* n N			*/
		{ 0x6f,0x0f,0x4f,0x0f },	/* o O			*/
		{ 0x70,0x10,0x50,0x10 },	/* p P			*/
		{ 0x71,0x11,0x51,0x11 },	/* q Q			*/
		{ 0x72,0x12,0x52,0x12 },	/* r R			*/
		{ 0x73,0x13,0x53,0x13 },	/* s S			*/
		{ 0x74,0x14,0x54,0x14 },	/* t T			*/
		{ 0x75,0x15,0x55,0x15 },	/* u U			*/
		{ 0x76,0x16,0x56,0x16 },	/* v V			*/
		{ 0x77,0x17,0x57,0x17 },	/* w W			*/
		{ 0x78,0x18,0x58,0x18 },	/* x X			*/
		{ 0x79,0x19,0x59,0x19 },	/* y Y			*/
		{ 0x7a,0x1a,0x5a,0x1a } 	/* z Z			*/
	},
/* caps lock on   norm ctrl shft both */
	{
		{ 0x7f,0x7f,0x7f,0x7f },	/* Backspace	*/
		{ 0x08,0x08,0x08,0x08 },	/* Left 		*/
		{ 0x09,0x09,0x09,0x09 },	/* Tab			*/
		{ 0x0a,0x0a,0x0a,0x0a },	/* Down 		*/
		{ 0x0b,0x0b,0x0b,0x0b },	/* Up			*/
		{ 0x0d,0x0d,0x0d,0x0d },	/* Enter		*/
		{ 0x15,0x15,0x15,0x15 },	/* Right		*/
		{ 0x1b,0x1b,0x1b,0x1b },	/* Escape		*/
		{ 0x20,0x20,0x20,0x20 },	/* Space		*/
		{ 0x27,0x27,0x22,0x22 },	/* ' "          */
		{ 0x2c,0x2c,0x3c,0x3c },	/* , <			*/
		{ 0x2d,0x2d,0x5f,0x1f },	/* - _			*/
		{ 0x2e,0x2e,0x3e,0x3e },	/* . >			*/
		{ 0x2f,0x2f,0x3f,0x3f },	/* / ?			*/
		{ 0x30,0x30,0x29,0x29 },	/* 0 )			*/
		{ 0x31,0x31,0x21,0x21 },	/* 1 !			*/
		{ 0x32,0x00,0x40,0x00 },	/* 2 @			*/
		{ 0x33,0x33,0x23,0x23 },	/* 3 #			*/
		{ 0x34,0x34,0x24,0x24 },	/* 4 $			*/
		{ 0x35,0x35,0x25,0x25 },	/* 5 %			*/
		{ 0x36,0x1e,0x5e,0x1e },	/* 6 ^			*/
		{ 0x37,0x37,0x26,0x26 },	/* 7 &			*/
		{ 0x38,0x38,0x2a,0x2a },	/* 8 *			*/
		{ 0x39,0x39,0x28,0x28 },	/* 9 (			*/
		{ 0x3b,0x3b,0x3a,0x3a },	/* ; :			*/
		{ 0x3d,0x3d,0x2b,0x2b },	/* = +			*/
		{ 0x5b,0x1b,0x7b,0x1b },	/* [ {			*/
		{ 0x5c,0x1c,0x7c,0x1c },	/* \ |			*/
		{ 0x5d,0x1d,0x7d,0x1d },	/* ] }			*/
		{ 0x60,0x60,0x7e,0x7e },	/* ` ~			*/
		{ 0x41,0x01,0x61,0x01 },	/* A a			*/
		{ 0x42,0x02,0x62,0x02 },	/* B b			*/
		{ 0x43,0x03,0x63,0x03 },	/* C c			*/
		{ 0x44,0x04,0x64,0x04 },	/* D d			*/
		{ 0x45,0x05,0x65,0x05 },	/* E e			*/
		{ 0x46,0x06,0x66,0x06 },	/* F f			*/
		{ 0x47,0x07,0x67,0x07 },	/* G g			*/
		{ 0x48,0x08,0x68,0x08 },	/* H h			*/
		{ 0x49,0x09,0x69,0x09 },	/* I i			*/
		{ 0x4a,0x0a,0x6a,0x0a },	/* J j			*/
		{ 0x4b,0x0b,0x6b,0x0b },	/* K k			*/
		{ 0x4c,0x0c,0x6c,0x0c },	/* L l			*/
		{ 0x4d,0x0d,0x6d,0x0d },	/* M m			*/
		{ 0x4e,0x0e,0x6e,0x0e },	/* N n			*/
		{ 0x4f,0x0f,0x6f,0x0f },	/* O o			*/
		{ 0x50,0x10,0x70,0x10 },	/* P p			*/
		{ 0x51,0x11,0x71,0x11 },	/* Q q			*/
		{ 0x52,0x12,0x72,0x12 },	/* R r			*/
		{ 0x53,0x13,0x73,0x13 },	/* S s			*/
		{ 0x54,0x14,0x74,0x14 },	/* T t			*/
		{ 0x55,0x15,0x75,0x15 },	/* U u			*/
		{ 0x56,0x16,0x76,0x16 },	/* V v			*/
		{ 0x57,0x17,0x77,0x17 },	/* W w			*/
		{ 0x58,0x18,0x78,0x18 },	/* X x			*/
		{ 0x59,0x19,0x79,0x19 },	/* Y y			*/
		{ 0x5a,0x1a,0x7a,0x1a } 	/* Z z			*/
	}
};

static unsigned int *ay3600_keys;

static UINT8 keycode;
static UINT8 keywaiting;
static UINT8 keystilldown;

#define A2_KEY_NORMAL  0
#define A2_KEY_CONTROL 1
#define A2_KEY_SHIFT   2
#define A2_KEY_BOTH    3
#define MAGIC_KEY_REPEAT_NUMBER 80

#define AY3600_KEYS_LENGTH	256

/***************************************************************************
  AY3600_init
***************************************************************************/
int AY3600_init(void)
{
	/* Init the key remapping table */
	ay3600_keys = auto_malloc(AY3600_KEYS_LENGTH * sizeof(*ay3600_keys));
	if (!ay3600_keys)
		return 1;
	memset(ay3600_keys, 0, AY3600_KEYS_LENGTH * sizeof(*ay3600_keys));

	/* We poll the keyboard periodically to scan the keys.  This is
	actually consistent with how the AY-3600 keyboard controller works. */
	timer_pulse(TIME_IN_HZ(60), 0, AY3600_poll);

	/* Set Caps Lock light to ON, since that's how we default it. */
	set_led_status(1,1);

	keywaiting = 0;
	keycode = 0;
	keystilldown = 0;
	return 0;
}

/***************************************************************************
  AY3600_poll
***************************************************************************/
static void AY3600_poll(int dummy)
{
	int switchkey;	/* Normal, Shift, Control, or both */
	int port, bit, data;
	int any_key_pressed = 0;   /* Have we pressed a key? True/False */
	int caps_lock = 0;
	int curkey;

	static int last_key = 0xff; 	/* Necessary for special repeat key behaviour */
	static unsigned int last_time = 65001;	/* Necessary for special repeat key behaviour */

	static unsigned int time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;

	/* Check for these special cases because they affect the emulated key codes */

	/* Check caps lock and set LED here */
	if (pressed_specialkey(SPECIALKEY_CAPSLOCK))
	{
		caps_lock = 1;
		set_led_status(1,1);
	}
	else
	{
		caps_lock = 0;
		set_led_status(1,0);
	}

	switchkey = A2_KEY_NORMAL;

	/* Shift key check */
	if (pressed_specialkey(SPECIALKEY_SHIFT))
		switchkey |= A2_KEY_SHIFT;

	/* Control key check - only one control key on the left side on the Apple */
	if (pressed_specialkey(SPECIALKEY_CONTROL))
	{
		switchkey |= A2_KEY_CONTROL;

		/* Reset key check */
		if (pressed_specialkey(SPECIALKEY_RESET) &&
			(pressed_specialkey(SPECIALKEY_BUTTON0)
			|| pressed_specialkey(SPECIALKEY_BUTTON1)))
		{
			machine_reset();
		}
	}

	/* Run through real keys and see what's being pressed */
	for( port = 0; port < 7; port++ )
	{
		data = readinputport(1 + port);
		for( bit = 0; bit < 8; bit++ )
		{
			curkey = ay3600_key_remap[caps_lock][port*8+bit][switchkey];
			if( data & (1 << bit) )
			{
				any_key_pressed = 1;

				/* Prevent overflow */
				if( ay3600_keys[curkey] < 65000 )
					ay3600_keys[curkey]++;

				/* On every key press, reset the time until repeat and the key to repeat */
				if( ay3600_keys[curkey] == 1 )
				{
					time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;
					last_key = curkey;
					last_time = 1;
				}
			}
			else
			{
				ay3600_keys[curkey] = 0;
			}
		}
	}

	if( !any_key_pressed )
	{
		/* If no keys have been pressed, reset the repeat time and return */
		time_until_repeat = MAGIC_KEY_REPEAT_NUMBER;
		last_key = 0xff;
		last_time = 65001;
	}
	else
	{
		/* Otherwise, count down the repeat time */
		if( time_until_repeat > 0 )
			time_until_repeat--;

		/* Even if a key has been released, repeat it if it was the last key pressed */
		/* If we should output a key, set the appropriate Apple II data lines */
		if( time_until_repeat == 0 ||
			time_until_repeat == MAGIC_KEY_REPEAT_NUMBER-1 )
		{
			keywaiting = 1;
			keycode = last_key;
		}
	}
	keystilldown = (last_key == keycode);
}

/***************************************************************************
  AY3600_keydata_strobe_r ($C00x)
***************************************************************************/
int AY3600_keydata_strobe_r(void)
{
	int rc;
	rc = keycode | (keywaiting ? 0x80 : 0x00);
	LOG(("AY3600_keydata_strobe_r(): rc=0x%02x\n", rc));
	return rc;
}


/***************************************************************************
  AY3600_anykey_clearstrobe_r ($C01x)
***************************************************************************/
int AY3600_anykey_clearstrobe_r(void)
{
	int rc;
	keywaiting = 0;
	rc = keycode | (keystilldown ? 0x80 : 0x00);
	LOG(("AY3600_anykey_clearstrobe_r(): rc=0x%02x\n", rc));
	return rc;
}

/***************************************************************************
  QUEUE_CHARS( AY3600 )
***************************************************************************/

QUEUE_CHARS( AY3600 )
{
	if (keywaiting)
		return 0;
	keycode = text[0];
	keywaiting = 1;
	return 1;
}

/***************************************************************************
  ACCEPT_CHAR( AY3600 )
***************************************************************************/

ACCEPT_CHAR( AY3600 )
{
	return (ch <= 0x7f);
}
