#include <ctype.h>
#include <assert.h>
#include <wctype.h>
#include "inputx.h"
#include "inptport.h"
#include "mame.h"

#define NUM_CODES		128
#define NUM_SIMUL_KEYS	(UCHAR_SHIFT_END - UCHAR_SHIFT_BEGIN + 1)

#ifdef MAME_DEBUG
#define LOG_INPUTX	0
#else
#define LOG_INPUTX	0
#endif

struct InputCode
{
	UINT16 port[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipt[NUM_SIMUL_KEYS];
};

enum
{
	STATUS_KEYDOWN = 1	
};

struct KeyBuffer
{
	int begin_pos;
	int end_pos;
	int status;
	unicode_char_t buffer[4096];
};

#define INVALID_CHAR '?'

/***************************************************************************

	Code assembling

***************************************************************************/

#if LOG_INPUTX
static const char *charstr(unicode_char_t ch)
{
	static char buf[3];

	switch(ch) {
	case '\0':	strcpy(buf, "\\0");		break;
	case '\r':	strcpy(buf, "\\r");		break;
	case '\n':	strcpy(buf, "\\n");		break;
	case '\t':	strcpy(buf, "\\t");		break;
	default:
		buf[0] = (char) ch;
		buf[1] = '\0';
		break;
	}
	return buf;
}
#endif

static int scan_keys(const struct GameDriver *gamedrv, struct InputCode *codes, UINT16 *ports, const struct InputPortTiny **ipts, int keys, int shift)
{
	int result = 0;
	const struct InputPortTiny *ipt;
	const struct InputPortTiny *ipt_key = NULL;
	int key_shift = 0;
	UINT16 port = (UINT16) -1;
	unicode_char_t code;

	assert(keys < NUM_SIMUL_KEYS);

	ipt = gamedrv->input_ports;
	while(ipt->type != IPT_END)
	{
		switch(ipt->type) {
		case IPT_EXTENSION:
			break;

		case IPT_KEYBOARD:
			ipt_key = ipt;
			key_shift = 0;
			break;

		case IPT_UCHAR:
			assert(ipt_key);

			result = 1;
			if (key_shift == shift)
			{
				code = ipt->mask;
				code <<= 16;
				code |= ipt->default_value;

				if ((code >= UCHAR_SHIFT_BEGIN) && (code <= UCHAR_SHIFT_END))
				{
					ports[keys] = port;
					ipts[keys] = ipt_key;
					scan_keys(gamedrv, codes, ports, ipts, keys+1, code - UCHAR_SHIFT_1 + 1);
				}
				else if (code < NUM_CODES)
				{
					memcpy(codes[code].port, ports, sizeof(ports[0]) * keys);
					memcpy((void *) codes[code].ipt, ipts, sizeof(ipts[0]) * keys);
					codes[code].port[keys] = port;
					codes[code].ipt[keys] = ipt_key;
#if LOG_INPUTX
					if (gamedrv == Machine->gamedrv)
						logerror("inputx: code=%i (%s) port=%i ipt->name='%s'\n", (int) code, charstr(code), port, ipt->name);
#endif
				}
			}
			key_shift++;
			break;

		case IPT_PORT:
			port++;
			/* fall through */

		default:
			ipt_key = NULL;
			break;
		}
		ipt++;
	}
	return result;
}

static unicode_char_t unicode_tolower(unicode_char_t c)
{
	return (c < 128) ? tolower((char) c) : c;
}

#define CODE_BUFFER_SIZE	(sizeof(struct InputCode) * NUM_CODES)

static int build_codes(const struct GameDriver *gamedrv, struct InputCode *codes, int map_lowercase)
{
	UINT16 ports[NUM_SIMUL_KEYS];
	const struct InputPortTiny *ipts[NUM_SIMUL_KEYS];
	int switch_upper;
	unicode_char_t c;

	memset(codes, 0, CODE_BUFFER_SIZE);

	if (!scan_keys(gamedrv, codes, ports, ipts, 0, 0))
		return 0;

	if (map_lowercase)
	{
		/* special case; scan to see if upper case characters are specified, but not lower case */
		switch_upper = 1;
		for (c = 'A'; c <= 'Z'; c++)
		{
			if (!inputx_can_post_key(c) || inputx_can_post_key(unicode_tolower(c)))
			{
				switch_upper = 0;
				break;
			}
		}

		if (switch_upper)
			memcpy(&codes['a'], &codes['A'], sizeof(codes[0]) * 26);
	}
	return 1;
}

/***************************************************************************

	Alternative key translations

***************************************************************************/

static const struct
{
	unicode_char_t ch;
	const char *str;
} alternate_charmap[] =
{
	{ 0x00a0,	" " },		/* non breaking space */
	{ 0x00a1,	"!" },		/* inverted exclaimation mark */
	{ 0x00a6,	"|" },		/* broken bar */
	{ 0x00a9,	"(c)" },	/* copyright sign */
	{ 0x00ab,	"<<" },		/* left pointing double angle */
	{ 0x00ae,	"(r)" },	/* registered sign */
	{ 0x00bb,	">>" },		/* right pointing double angle */
	{ 0x00bc,	"1/4" },	/* vulgar fraction one quarter */
	{ 0x00bd,	"1/2" },	/* vulgar fraction one half */
	{ 0x00be,	"3/4" },	/* vulgar fraction three quarters */
	{ 0x00bf,	"?" },		/* inverted question mark */
	{ 0x00c0,	"A" },		/* 'A' grave */
	{ 0x00c1,	"A" },		/* 'A' acute */
	{ 0x00c2,	"A" },		/* 'A' circumflex */
	{ 0x00c3,	"A" },		/* 'A' tilde */
	{ 0x00c4,	"A" },		/* 'A' diaeresis */
	{ 0x00c5,	"A" },		/* 'A' ring above */
	{ 0x00c6,	"AE" },		/* 'AE' ligature */
	{ 0x00c7,	"C" },		/* 'C' cedilla */
	{ 0x00c8,	"E" },		/* 'E' grave */
	{ 0x00c9,	"E" },		/* 'E' acute */
	{ 0x00ca,	"E" },		/* 'E' circumflex */
	{ 0x00cb,	"E" },		/* 'E' diaeresis */
	{ 0x00cc,	"I" },		/* 'I' grave */
	{ 0x00cd,	"I" },		/* 'I' acute */
	{ 0x00ce,	"I" },		/* 'I' circumflex */
	{ 0x00cf,	"I" },		/* 'I' diaeresis */
	{ 0x00d0,	"D" },		/* 'ETH' */
	{ 0x00d1,	"N" },		/* 'N' tilde */
	{ 0x00d2,	"O" },		/* 'O' grave */
	{ 0x00d3,	"O" },		/* 'O' acute */
	{ 0x00d4,	"O" },		/* 'O' circumflex */
	{ 0x00d5,	"O" },		/* 'O' tilde */
	{ 0x00d6,	"O" },		/* 'O' diaeresis */
	{ 0x00d7,	"X" },		/* multiplication sign */
	{ 0x00d8,	"O" },		/* 'O' stroke */
	{ 0x00d9,	"U" },		/* 'U' grave */
	{ 0x00da,	"U" },		/* 'U' acute */
	{ 0x00db,	"U" },		/* 'U' circumflex */
	{ 0x00dc,	"U" },		/* 'U' diaeresis */
	{ 0x00dd,	"Y" },		/* 'Y' acute */
	{ 0x00df,	"SS" },		/* sharp S */
	{ 0x00e0,	"a" },		/* 'a' grave */
	{ 0x00e1,	"a" },		/* 'a' acute */
	{ 0x00e2,	"a" },		/* 'a' circumflex */
	{ 0x00e3,	"a" },		/* 'a' tilde */
	{ 0x00e4,	"a" },		/* 'a' diaeresis */
	{ 0x00e5,	"a" },		/* 'a' ring above */
	{ 0x00e6,	"ae" },		/* 'ae' ligature */
	{ 0x00e7,	"c" },		/* 'c' cedilla */
	{ 0x00e8,	"e" },		/* 'e' grave */
	{ 0x00e9,	"e" },		/* 'e' acute */
	{ 0x00ea,	"e" },		/* 'e' circumflex */
	{ 0x00eb,	"e" },		/* 'e' diaeresis */
	{ 0x00ec,	"i" },		/* 'i' grave */
	{ 0x00ed,	"i" },		/* 'i' acute */
	{ 0x00ee,	"i" },		/* 'i' circumflex */
	{ 0x00ef,	"i" },		/* 'i' diaeresis */
	{ 0x00f0,	"d" },		/* 'eth' */
	{ 0x00f1,	"n" },		/* 'n' tilde */
	{ 0x00f2,	"o" },		/* 'o' grave */
	{ 0x00f3,	"o" },		/* 'o' acute */
	{ 0x00f4,	"o" },		/* 'o' circumflex */
	{ 0x00f5,	"o" },		/* 'o' tilde */
	{ 0x00f6,	"o" },		/* 'o' diaeresis */
	{ 0x00f8,	"o" },		/* 'o' stroke */
	{ 0x00f9,	"u" },		/* 'u' grave */
	{ 0x00fa,	"u" },		/* 'u' acute */
	{ 0x00fb,	"u" },		/* 'u' circumflex */
	{ 0x00fc,	"u" },		/* 'u' diaeresis */
	{ 0x00fd,	"y" },		/* 'y' acute */
	{ 0x00ff,	"y" },		/* 'y' diaeresis */
	{ 0x2010,	"-" },		/* hyphen */
	{ 0x2011,	"-" },		/* non-breaking hyphen */
	{ 0x2012,	"-" },		/* figure dash */
	{ 0x2013,	"-" },		/* en dash */
	{ 0x2014,	"-" },		/* em dash */
	{ 0x2015,	"-" },		/* horizontal dash */
	{ 0x2018,	"\'" },		/* left single quotation mark */
	{ 0x2019,	"\'" },		/* right single quotation mark */
	{ 0x201a,	"\'" },		/* single low quotation mark */
	{ 0x201b,	"\'" },		/* single high reversed quotation mark */
	{ 0x201c,	"\"" },		/* left double quotation mark */
	{ 0x201d,	"\"" },		/* right double quotation mark */
	{ 0x201e,	"\"" },		/* double low quotation mark */
	{ 0x201f,	"\"" },		/* double high reversed quotation mark */
	{ 0x2024,	"." },		/* one dot leader */
	{ 0x2025,	".." },		/* two dot leader */
	{ 0x2026,	"..." },	/* horizontal ellipsis */
	{ 0x2047,	"??" },		/* double question mark */
	{ 0x2048,	"?!" },		/* question exclamation mark */
	{ 0x2049,	"!?" }		/* exclamation question mark */		
};

static const char *find_alternate(unicode_char_t target_char)
{
	int low = 0;
	int high = sizeof(alternate_charmap) / sizeof(alternate_charmap[0]);
	int i;
	unicode_char_t ch;

	while(high > low)
	{
		i = (high + low) / 2;
		ch = alternate_charmap[i].ch;
		if (ch < target_char)
			low = i + 1;
		else if (ch > target_char)
			high = i;
		else
			return alternate_charmap[i].str;
	}
	return NULL;
}

/***************************************************************************

	Validity checks

***************************************************************************/

#ifdef MAME_DEBUG
int inputx_validitycheck(const struct GameDriver *gamedrv)
{
	char buf[CODE_BUFFER_SIZE];
	struct InputCode *codes;
	const struct InputPortTiny *ipt;
	int port_count, i, j;
	const char *str;
	int error = 0;
	unicode_char_t last_char = 0;

	if (gamedrv)
	{
		if (gamedrv->flags & GAME_COMPUTER)
		{
			codes = (struct InputCode *) buf;
			build_codes(gamedrv, codes, FALSE);

			port_count = 0;
			for (ipt = gamedrv->input_ports; ipt->type != IPT_END; ipt++)
			{
				if (ipt->type == IPT_PORT)
					port_count++;
			}

			for (i = 0; i < NUM_CODES; i++)
			{
				for (j = 0; j < NUM_SIMUL_KEYS; j++)
				{
					if (codes[i].port[j] >= port_count)
					{
						printf("%s: invalid inputx translation for code %i port %i\n", gamedrv->name, i, j);
						error = 1;
					}
				}
			}
		}
	}
	else
	{
		/* check to make sure that alternate_charmap is in order */
		for (i = 0; i < sizeof(alternate_charmap) / sizeof(alternate_charmap[0]); i++)
		{
			if (last_char >= alternate_charmap[i].ch)
			{
				printf("inputx: alternate_charmap is out of order; 0x%08x should be higher than 0x%08x\n", alternate_charmap[i].ch, last_char);
				error = 1;
			}
			last_char = alternate_charmap[i].ch;
		}

		/* check to make sure that I can look up everything on alternate_charmap */
		for (i = 0; i < sizeof(alternate_charmap) / sizeof(alternate_charmap[0]); i++)
		{
			str = find_alternate(alternate_charmap[i].ch);
			if (str != alternate_charmap[i].str)
			{
				printf("inputx: expected find_alternate(0x%08x) to return '%s', but instead got '%s'\n", alternate_charmap[i].ch, alternate_charmap[i].str, str);
				error = 1;
			}
		}
	}
	return error;
}
#endif

/***************************************************************************

	Core

***************************************************************************/

static struct InputCode *codes;
static struct KeyBuffer *keybuffer;
static void *inputx_timer;
static int (*queue_chars)(const unicode_char_t *text, size_t text_len);
static int (*accept_char)(unicode_char_t ch);

static void inputx_timerproc(int dummy);

void inputx_init(void)
{
	struct SystemConfigurationParamBlock params;

	codes = NULL;
	inputx_timer = NULL;
	queue_chars = NULL;
	accept_char = NULL;

	/* posting keys directly only makes sense for a computer */
	if (Machine->gamedrv->flags & GAME_COMPUTER)
	{
		/* check driver for QUEUE_CHARS and/or ACCEPT_CHAR callbacks */
		memset(&params, 0, sizeof(params));
		if (Machine->gamedrv->sysconfig_ctor)
			Machine->gamedrv->sysconfig_ctor(&params);
		queue_chars = params.queue_chars;
		accept_char = params.accept_char;

		keybuffer = auto_malloc(sizeof(struct KeyBuffer));
		if (!keybuffer)
			goto error;
		memset(keybuffer, 0, sizeof(*keybuffer));

		if (!queue_chars)
		{
			/* only need to compute codes if QUEUE_CHARS not present */
			codes = (struct InputCode *) auto_malloc(CODE_BUFFER_SIZE);
			if (!codes)
				goto error;
			if (!build_codes(Machine->gamedrv, codes, TRUE))
				goto error;
		}

		inputx_timer = timer_alloc(inputx_timerproc);
	}
	return;

error:
	codes = NULL;
	keybuffer = NULL;
	queue_chars = NULL;
	accept_char = NULL;
}

int inputx_can_post(void)
{
	return queue_chars || codes;
}

static struct KeyBuffer *get_buffer(void)
{
	assert(inputx_can_post());
	return (struct KeyBuffer *) keybuffer;
}

static int can_post_key_directly(unicode_char_t ch)
{
	int rc;

	if (queue_chars)
	{
		rc = accept_char ? accept_char(ch) : TRUE;
	}
	else
	{
		assert(codes);
		rc = ((ch < NUM_CODES) && codes[ch].ipt[0] != NULL);
	}
	return rc;
}

static int can_post_key_alternate(unicode_char_t ch)
{
	const char *s;

	s = find_alternate(ch);
	if (!s)
		return 0;

	while(*s)
	{
		if (!can_post_key_directly(*s))
			return 0;
		s++;
	}
	return 1;
}

int inputx_can_post_key(unicode_char_t ch)
{
	
	return inputx_can_post() && (can_post_key_directly(ch) || can_post_key_alternate(ch));
}

static double choose_delay(unicode_char_t ch)
{
	double delay;

	if (queue_chars)
	{
		/* systems with queue_chars can afford a much smaller delay */
		delay = TIME_IN_SEC(0.01);
	}
	else
	{
		switch(ch) {
		case '\r':
			delay = TIME_IN_SEC(0.2);
			break;

		default:
			delay = TIME_IN_SEC(0.05);
			break;
		}
	}
	return delay;
}

static void internal_post_key(unicode_char_t ch)
{
	struct KeyBuffer *keybuf;

	keybuf = get_buffer();

	/* need to start up the timer? */
	if (keybuf->begin_pos == keybuf->end_pos)
	{
		timer_adjust(inputx_timer, choose_delay(ch), 0, 0);
		keybuf->status &= ~STATUS_KEYDOWN;
	}

	keybuf->buffer[keybuf->end_pos++] = ch;
	keybuf->end_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
}

static int buffer_full(void)
{
	struct KeyBuffer *keybuf;
	keybuf = get_buffer();
	return ((keybuf->end_pos + 1) % (sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]))) == keybuf->begin_pos;
}

void inputx_postn(const unicode_char_t *text, size_t text_len)
{
	int last_cr = 0;
	unicode_char_t ch;
	const char *s;

	if (inputx_can_post())
	{
		while((text_len > 0) && !buffer_full())
		{
			ch = *(text++);
			text_len--;

			/* change all eolns to '\r' */
			if ((ch != '\n') || !last_cr)
			{
				if (ch == '\n')
					ch = '\r';
				else
					last_cr = (ch == '\r');
#if LOG_INPUTX
				logerror("inputx_postn(): code=%i (%s) port=%i ipt->name='%s'\n", (int) ch, charstr(ch), codes[ch].port[0], codes[ch].ipt[0] ? codes[ch].ipt[0]->name : "<null>");
#endif
				if (can_post_key_directly(ch))
				{
					internal_post_key(ch);
				}
				else if (can_post_key_alternate(ch))
				{
					s = find_alternate(ch);
					assert(s);
					while(*s)
						internal_post_key(*(s++));
				}
			}
			else
			{
				last_cr = 0;
			}
		}
	}
}

static void inputx_timerproc(int dummy)
{
	struct KeyBuffer *keybuf;
	double delay;

	keybuf = get_buffer();

	if (queue_chars)
	{
		/* the driver has a queue_chars handler */
		while(queue_chars(&keybuf->buffer[keybuf->begin_pos], 1))
		{
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
	}
	else
	{
		/* the driver does not have a queue_chars handler */
		if (keybuf->status & STATUS_KEYDOWN)
		{
			keybuf->status &= ~STATUS_KEYDOWN;
			keybuf->begin_pos++;
			keybuf->begin_pos %= sizeof(keybuf->buffer) / sizeof(keybuf->buffer[0]);
		}
		else
		{
			keybuf->status |= STATUS_KEYDOWN;
		}
	}

	/* need to make sure timerproc is called again if buffer not empty */
	if (keybuf->begin_pos != keybuf->end_pos)
	{
		delay = choose_delay(keybuf->buffer[keybuf->begin_pos]);
		timer_adjust(inputx_timer, delay, 0, 0);
	}
}

void inputx_update(unsigned short *ports)
{
	const struct KeyBuffer *keybuf;
	const struct InputCode *code;
	const struct InputPortTiny *ipt;
	unicode_char_t ch;
	int i;
	int value;

	if (inputx_can_post())
	{
		keybuf = get_buffer();
		if ((keybuf->status & STATUS_KEYDOWN) && (keybuf->begin_pos != keybuf->end_pos))
		{
			ch = keybuf->buffer[keybuf->begin_pos];
			code = &codes[ch];

			for (i = 0; code->ipt[i] && (i < sizeof(code->ipt) / sizeof(code->ipt[0])); i++)
			{
				ipt = code->ipt[i];
				value = ports[code->port[i]];

				switch(ipt->default_value) {
				case IP_ACTIVE_LOW:
					value &= ~ipt->mask;
					break;
				case IP_ACTIVE_HIGH:
					value |= ipt->mask;
					break;
				}
				ports[code->port[i]] = value;
			}
		}
	}
}

/***************************************************************************

	Alternative calls

***************************************************************************/

void inputx_post(const unicode_char_t *text)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn(text, len);
}

void inputx_postc(unicode_char_t ch)
{
	inputx_postn(&ch, 1);
}

void inputx_postn_utf16(const utf16_char_t *text, size_t text_len)
{
	size_t len = 0;
	unicode_char_t c;
	utf16_char_t w1, w2;
	unicode_char_t buf[256];

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		w1 = *(text++);
		text_len--;

		if ((w1 >= 0xd800) && (w1 <= 0xdfff))
		{
			if (w1 <= 0xDBFF)
			{
				w2 = 0;
				if (text_len > 0)
				{
					w2 = *(text++);
					text_len--;
				}
				if ((w2 >= 0xdc00) && (w2 <= 0xdfff))
				{
					c = w1 & 0x03ff;
					c <<= 10;
					c |= w2 & 0x03ff;
				}
				else
				{
					c = INVALID_CHAR;
				}
			}
			else
			{
				c = INVALID_CHAR;
			}
		}
		else
		{
			c = w1;
		}
		buf[len++] = c;
	}
	inputx_postn(buf, len);
}

void inputx_post_utf16(const utf16_char_t *text)
{
	size_t len = 0;
	while(text[len])
		len++;
	inputx_postn_utf16(text, len);
}

void inputx_postn_utf8(const char *text, size_t text_len)
{
	size_t len = 0;
	unicode_char_t buf[256];
	unicode_char_t minchar;
	unicode_char_t c, auxchar;
	size_t auxlen;

	while(text_len > 0)
	{
		if (len == (sizeof(buf) / sizeof(buf[0])))
		{
			inputx_postn(buf, len);
			len = 0;
		}

		c = (unsigned char) *text;
		text_len--;
		text++;

		if (c < 0x80)
		{
			c &= 0x7f;
			auxlen = 0;
			minchar = 0x00000000;
		}
		else if ((c >= 0xc0) & (c < 0xe0))
		{
			c &= 0x1f;
			auxlen = 1;
			minchar = 0x00000080;
		}
		else if ((c >= 0xe0) & (c < 0xf0))
		{
			c &= 0x0f;
			auxlen = 2;
			minchar = 0x00000800;
		}
		else if ((c >= 0xf0) & (c < 0xf8))
		{
			c &= 0x07;
			auxlen = 3;
			minchar = 0x00010000;
		}
		else if ((c >= 0xf8) & (c < 0xfc))
		{
			c &= 0x03;
			auxlen = 4;
			minchar = 0x00200000;
		}
		else if ((c >= 0xfc) & (c < 0xfe))
		{
			c &= 0x01;
			auxlen = 5;
			minchar = 0x04000000;
		}
		else
		{
			c = INVALID_CHAR;
			auxlen = 0;
			minchar = 0x00000000;
		}

		while(auxlen && text_len)
		{
			auxchar = *text;
			if ((auxchar & 0xc0) != 0x80)
				break;

			c = c << 6;
			c |= auxchar & 0x3f;

			text++;
			text_len--;
			auxlen--;
		}
		if (auxlen || (c < minchar))
			c = INVALID_CHAR;

		buf[len++] = c;
	}
	inputx_postn(buf, len);
}

void inputx_post_utf8(const char *text)
{
	inputx_postn_utf8(text, strlen(text));
}

/***************************************************************************

	Other stuff

	This stuff is here more out of convienience than anything else
***************************************************************************/

int inputx_orient_ports(struct InputPort *port, struct InputPort **arranged_ports)
{
	int i, j, span;
	struct InputPort *port0;
	struct InputPort *port1;
	struct InputPort *port2;
	struct InputPort *port3;

	static UINT32 port_formations[][4] =
	{
		{ IPT_AD_STICK_X,			IPT_EXTENSION,				IPT_AD_STICK_Y,			IPT_EXTENSION },
		{ IPT_JOYSTICK_LEFT,		IPT_JOYSTICK_RIGHT,			IPT_JOYSTICK_UP,		IPT_JOYSTICK_DOWN },
		{ IPT_JOYSTICKLEFT_LEFT,	IPT_JOYSTICKLEFT_RIGHT,		IPT_JOYSTICKLEFT_UP,	IPT_JOYSTICKLEFT_DOWN },
		{ IPT_JOYSTICKRIGHT_LEFT,	IPT_JOYSTICKRIGHT_RIGHT,	IPT_JOYSTICKRIGHT_UP,	IPT_JOYSTICKRIGHT_DOWN },
		{ IPT_TRACKBALL_X,			IPT_EXTENSION,				IPT_TRACKBALL_Y,		IPT_EXTENSION },
		{ IPT_LIGHTGUN_X,			IPT_EXTENSION,				IPT_LIGHTGUN_Y,			IPT_EXTENSION }
	};

	arranged_ports[0] = NULL;
	arranged_ports[1] = NULL;
	arranged_ports[2] = NULL;
	arranged_ports[3] = NULL;

	for(i = 0; i < sizeof(port_formations) / sizeof(port_formations[0]); i++)
	{
		for (j = 0; j < 4; j++)
		{
			if ((port->type & ~IPF_MASK) == port_formations[i][j])
			{
				span = 1;

				port0 = port;
				port1 = (++port);
				if ((port1->type & ~IPF_MASK) == IPT_PORT)
					port1 = ++port;

				if ((port1->type & ~IPF_MASK) == port_formations[i][j^1])
				{
					arranged_ports[j^0] = port0;
					arranged_ports[j^1] = port1;
					span = 2;

					port2 = ++port;
					if ((port2->type & ~IPF_MASK) == IPT_PORT)
						port2 = (++port);
					if (port2->type != IPT_END)
					{
						port3 = ++port;
						if ((port3->type & ~IPF_MASK) == IPT_PORT)
							port3 = ++port;


						if (((port2->type & ~IPF_MASK) == port_formations[i][j^2])
								&& ((port3->type & ~IPF_MASK) == port_formations[i][j^3]))
						{
							arranged_ports[j^2] = port2;
							arranged_ports[j^3] = port3;
							span = 4;
						}
						else if (((port2->type & ~IPF_MASK) == port_formations[i][j^3])
								&& ((port3->type & ~IPF_MASK) == port_formations[i][j^2]))
						{
							arranged_ports[j^3] = port2;
							arranged_ports[j^2] = port3;
							span = 4;
						}
					}
				}
				return span;
			}
		}
	}
	return 1;
}



