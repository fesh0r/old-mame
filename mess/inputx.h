/*********************************************************************

	inputx.h

	Secondary input related functions for MESS specific functionality

*********************************************************************/

#ifndef INPUTX_H
#define INPUTX_H

#include "mame.h"

/***************************************************************************

	Constants

***************************************************************************/

/* input categorizations */
enum
{
	INPUT_CATEGORY_INTERNAL,
	INPUT_CATEGORY_KEYBOARD,
	INPUT_CATEGORY_CONTROLLER,
	INPUT_CATEGORY_CONFIG,
	INPUT_CATEGORY_DIPSWITCH,
	INPUT_CATEGORY_MISC
};

/***************************************************************************

	Macros

***************************************************************************/

#define UCHAR_SHIFT_1		(0xf700)
#define UCHAR_SHIFT_2		(0xf701)
#define UCHAR_MAMEKEY_BEGIN	(0xf702)
#define UCHAR_MAMEKEY_END	(UCHAR_MAMEKEY_BEGIN + __code_key_last)
#define UCHAR_MAMEKEY(code)	(UCHAR_MAMEKEY_BEGIN + KEYCODE_##code)

#define UCHAR_SHIFT_BEGIN	(UCHAR_SHIFT_1)
#define UCHAR_SHIFT_END		(UCHAR_SHIFT_2)

#define PORT_UCHAR(uchar) \
	{ (UINT16) ((uchar) >> 16), (UINT16) ((uchar) >> 0), IPT_UCHAR, 0 },

#define PORT_KEY0(mask,default,name,key1,key2)						\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\

#define PORT_KEY1(mask,default,name,key1,key2,uchar)				\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\
	PORT_UCHAR(uchar)												\

#define PORT_KEY2(mask,default,name,key1,key2,uchar1,uchar2)		\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\
	PORT_UCHAR(uchar1)												\
	PORT_UCHAR(uchar2)												\

#define PORT_KEY3(mask,default,name,key1,key2,uchar1,uchar2,uchar3)	\
	PORT_BIT_NAME(mask, default, IPT_KEYBOARD, name)				\
	PORT_CODE(key1,key2)											\
	PORT_UCHAR(uchar1)												\
	PORT_UCHAR(uchar2)												\
	PORT_UCHAR(uchar3)												\

/* config definition */
#define PORT_CONFNAME(mask,default,name) \
	PORT_BIT_NAME(mask, default, IPT_CONFIG_NAME, name)

#define PORT_CONFSETTING(default,name) \
	PORT_BIT_NAME(0, default, IPT_CONFIG_SETTING, name)



/***************************************************************************

	Prototypes

***************************************************************************/

/* these are called by the core; they should not be called from FEs */
void inputx_init(void);
void inputx_update(unsigned short *ports);

#ifdef MAME_DEBUG
int inputx_validitycheck(const struct GameDriver *gamedrv);
#endif

/* these can be called from FEs */
int inputx_can_post(void);
int inputx_can_post_key(unicode_char_t ch);

void inputx_post(const unicode_char_t *text);
void inputx_postc(unicode_char_t ch);
void inputx_postn(const unicode_char_t *text, size_t text_len);
void inputx_post_utf16(const utf16_char_t *text);
void inputx_postn_utf16(const utf16_char_t *text, size_t text_len);
void inputx_post_utf8(const char *text);
void inputx_postn_utf8(const char *text, size_t text_len);

int input_categorize_port(const struct InputPort *in);
int input_has_input_category(int category);
int input_player_number(const struct InputPort *in);
int input_count_players(void);

#endif /* INPUTX_H */
