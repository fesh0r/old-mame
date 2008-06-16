/*********************************************************************

    ui.c

    Functions used to handle MAME's user interface.

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#include "driver.h"
#include "osdepend.h"
#include "video/vector.h"
#include "profiler.h"
#include "cheat.h"
#include "render.h"
#include "rendfont.h"
#include "ui.h"
#include "uimenu.h"
#include "uigfx.h"

#ifdef MESS
#include "mess.h"
#include "uimess.h"
#include "inputx.h"
#endif /* MESS */

#include <ctype.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

enum
{
	INPUT_TYPE_DIGITAL = 0,
	INPUT_TYPE_ANALOG = 1,
	INPUT_TYPE_ANALOG_DEC = 2,
	INPUT_TYPE_ANALOG_INC = 3
};

enum
{
	ANALOG_ITEM_KEYSPEED = 0,
	ANALOG_ITEM_CENTERSPEED,
	ANALOG_ITEM_REVERSE,
	ANALOG_ITEM_SENSITIVITY,
	ANALOG_ITEM_COUNT
};

enum
{
	LOADSAVE_NONE,
	LOADSAVE_LOAD,
	LOADSAVE_SAVE
};



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _slider_state slider_state;
struct _slider_state
{
	INT32			minval;				/* minimum value */
	INT32			defval;				/* default value */
	INT32			maxval;				/* maximum value */
	INT32			incval;				/* increment value */
	INT32			(*update)(running_machine *machine, INT32 newval, char *buffer, void *arg); /* callback */
	void *			arg;				/* argument */
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* font for rendering */
static render_font *ui_font;

/* current UI handler */
static UINT32 (*ui_handler_callback)(running_machine *, UINT32);
static UINT32 ui_handler_param;

/* flag to track single stepping */
static int single_step;

/* FPS counter display */
static int showfps;
static osd_ticks_t showfps_end;

/* profiler display */
static int show_profiler;

/* popup text display */
static osd_ticks_t popup_text_end;

/* messagebox buffer */
static char messagebox_text[4096];
static rgb_t messagebox_backcolor;

/* slider info */
static slider_state slider_list[100];
static int slider_count;
static int slider_current;

static int display_rescale_message;
static int allow_rescale;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void ui_exit(running_machine *machine);
static int rescale_notifier(running_machine *machine, int width, int height);

/* text generators */
static int sprintf_disclaimer(running_machine *machine, char *buffer);
static int sprintf_warnings(running_machine *machine, char *buffer);

/* UI handlers */
static UINT32 handler_messagebox(running_machine *machine, UINT32 state);
static UINT32 handler_messagebox_ok(running_machine *machine, UINT32 state);
static UINT32 handler_messagebox_anykey(running_machine *machine, UINT32 state);
static UINT32 handler_ingame(running_machine *machine, UINT32 state);
static UINT32 handler_slider(running_machine *machine, UINT32 state);
static UINT32 handler_load_save(running_machine *machine, UINT32 state);

/* slider controls */
static void slider_init(running_machine *machine);
static void slider_display(const char *text, int minval, int maxval, int defval, int curval);
static void slider_draw_bar(float leftx, float topy, float width, float height, float percentage, float default_percentage);
static INT32 slider_volume(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_mixervol(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_adjuster(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_overclock(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_refresh(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_brightness(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_contrast(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_gamma(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_xscale(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_yscale(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_xoffset(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_yoffset(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_flicker(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_beam(running_machine *machine, INT32 newval, char *buffer, void *arg);
static char *slider_get_screen_desc(const device_config *screen);
#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine *machine, INT32 newval, char *buffer, void *arg);
static INT32 slider_crossoffset(running_machine *machine, INT32 newval, char *buffer, void *arg);
#endif


/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    ui_set_handler - set a callback/parameter
    pair for the current UI handler
-------------------------------------------------*/

INLINE UINT32 ui_set_handler(UINT32 (*callback)(running_machine *, UINT32), UINT32 param)
{
	ui_handler_callback = callback;
	ui_handler_param = param;
	return param;
}


/*-------------------------------------------------
    slider_config - configure a slider entry
-------------------------------------------------*/

INLINE void slider_config(slider_state *state, INT32 minval, INT32 defval, INT32 maxval, INT32 incval,
						  INT32 (*update)(running_machine *, INT32, char *, void *), void *arg)
{
	state->minval = minval;
	state->defval = defval;
	state->maxval = maxval;
	state->incval = incval;
	state->update = update;
	state->arg = arg;
}


/*-------------------------------------------------
    is_breakable_char - is a given unicode
    character a possible line break?
-------------------------------------------------*/

INLINE int is_breakable_char(unicode_char ch)
{
	/* regular spaces and hyphens are breakable */
	if (ch == ' ' || ch == '-')
		return TRUE;

	/* In the following character sets, any character is breakable:
        Hiragana (3040-309F)
        Katakana (30A0-30FF)
        Bopomofo (3100-312F)
        Hangul Compatibility Jamo (3130-318F)
        Kanbun (3190-319F)
        Bopomofo Extended (31A0-31BF)
        CJK Strokes (31C0-31EF)
        Katakana Phonetic Extensions (31F0-31FF)
        Enclosed CJK Letters and Months (3200-32FF)
        CJK Compatibility (3300-33FF)
        CJK Unified Ideographs Extension A (3400-4DBF)
        Yijing Hexagram Symbols (4DC0-4DFF)
        CJK Unified Ideographs (4E00-9FFF) */
	if (ch >= 0x3040 && ch <= 0x9fff)
		return TRUE;

	/* Hangul Syllables (AC00-D7AF) are breakable */
	if (ch >= 0xac00 && ch <= 0xd7af)
		return TRUE;

	/* CJK Compatibility Ideographs (F900-FAFF) are breakable */
	if (ch >= 0xf900 && ch <= 0xfaff)
		return TRUE;

	return FALSE;
}



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    ui_init - set up the user interface
-------------------------------------------------*/

int ui_init(running_machine *machine)
{
	/* make sure we clean up after ourselves */
	add_exit_callback(machine, ui_exit);

	/* allocate the font */
	ui_font = render_font_alloc("ui.bdf");

	/* initialize the other UI bits */
	ui_menu_init(machine);
	ui_gfx_init(machine);

	/* reset globals */
	single_step = FALSE;
	ui_set_handler(handler_messagebox, 0);

	/* request callbacks when the renderer does resizing */
	render_set_rescale_notify(machine, rescale_notifier);
	allow_rescale = 0;
	display_rescale_message = FALSE;
	return 0;
}


/*-------------------------------------------------
    ui_exit - clean up ourselves on exit
-------------------------------------------------*/

static void ui_exit(running_machine *machine)
{
	/* free the font */
	if (ui_font != NULL)
		render_font_free(ui_font);
	ui_font = NULL;
}


/*-------------------------------------------------
    rescale_notifier - notifier to trigger a
    rescale message during long operations
-------------------------------------------------*/

static int rescale_notifier(running_machine *machine, int width, int height)
{
	/* always allow "small" rescaling */
	if (width < 500 || height < 500)
		return TRUE;

	/* if we've currently disallowed rescaling, turn on a message next frame */
	if (allow_rescale == 0)
		display_rescale_message = TRUE;

	/* only allow rescaling once we're sure the message is visible */
	return (allow_rescale == 1);
}


/*-------------------------------------------------
    ui_display_startup_screens - display the
    various startup screens
-------------------------------------------------*/

int ui_display_startup_screens(running_machine *machine, int first_time, int show_disclaimer)
{
#ifdef MESS
	const int maxstate = 4;
#else
	const int maxstate = 3;
#endif
	int str = options_get_int(mame_options(), OPTION_SECONDS_TO_RUN);
	int show_gameinfo = !options_get_bool(mame_options(), OPTION_SKIP_GAMEINFO);
	int show_warnings = TRUE;
	int state;

	/* disable everything if we are using -str for 300 or fewer seconds, or if we're the empty driver,
       or if we are debugging */
	if (!first_time || (str > 0 && str < 60*5) || machine->gamedrv == &driver_empty || machine->debug_mode)
		show_gameinfo = show_warnings = show_disclaimer = FALSE;

	/* initialize the on-screen display system */
	slider_init(machine);

	/* loop over states */
	ui_set_handler(handler_ingame, 0);
	for (state = 0; state < maxstate && !mame_is_scheduled_event_pending(machine) && !ui_menu_is_force_game_select(); state++)
	{
		/* default to standard colors */
		messagebox_backcolor = UI_FILLCOLOR;

		/* pick the next state */
		switch (state)
		{
			case 0:
				if (show_disclaimer && sprintf_disclaimer(machine, messagebox_text))
					ui_set_handler(handler_messagebox_ok, 0);
				break;

			case 1:
				if (show_warnings && sprintf_warnings(machine, messagebox_text))
				{
					ui_set_handler(handler_messagebox_ok, 0);
					if (machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
						messagebox_backcolor = UI_REDCOLOR;
				}
				break;

			case 2:
				if (show_gameinfo && sprintf_game_info(machine, messagebox_text))
					ui_set_handler(handler_messagebox_anykey, 0);
				break;
#ifdef MESS
			case 3:
				break;
#endif
		}

		/* clear the input memory */
		input_code_poll_switches(TRUE);
		while (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID) ;

		/* loop while we have a handler */
		while (ui_handler_callback != handler_ingame && !mame_is_scheduled_event_pending(machine) && !ui_menu_is_force_game_select())
			video_frame_update(machine, FALSE);

		/* clear the handler and force an update */
		ui_set_handler(handler_ingame, 0);
		video_frame_update(machine, FALSE);
	}

	/* if we're the empty driver, force the menus on */
	if (ui_menu_is_force_game_select())
		ui_set_handler(ui_menu_ui_handler, 0);

	return 0;
}


/*-------------------------------------------------
    ui_set_startup_text - set the text to display
    at startup
-------------------------------------------------*/

void ui_set_startup_text(running_machine *machine, const char *text, int force)
{
	static osd_ticks_t lastupdatetime = 0;
	osd_ticks_t curtime = osd_ticks();

	/* copy in the new text */
	strncpy(messagebox_text, text, sizeof(messagebox_text));
	messagebox_backcolor = UI_FILLCOLOR;

	/* don't update more than 4 times/second */
	if (force || (curtime - lastupdatetime) > osd_ticks_per_second() / 4)
	{
		lastupdatetime = curtime;
		video_frame_update(machine, FALSE);
	}
}


/*-------------------------------------------------
    ui_update_and_render - update the UI and
    render it; called by video.c
-------------------------------------------------*/

void ui_update_and_render(running_machine *machine)
{
	/* always start clean */
	render_container_empty(render_container_get_ui());

	/* if we're paused, dim the whole screen */
	if (mame_get_phase(machine) >= MAME_PHASE_RESET && (single_step || mame_is_paused(machine)))
	{
		int alpha = (1.0f - options_get_float(mame_options(), OPTION_PAUSE_BRIGHTNESS)) * 255.0f;
		if (ui_menu_is_force_game_select())
			alpha = 255;
		if (alpha > 255)
			alpha = 255;
		if (alpha >= 0)
			render_ui_add_rect(0.0f, 0.0f, 1.0f, 1.0f, MAKE_ARGB(alpha,0x00,0x00,0x00), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	}

	/* call the current UI handler */
	assert(ui_handler_callback != NULL);
	ui_handler_param = (*ui_handler_callback)(machine, ui_handler_param);

	/* cancel takes us back to the ingame handler */
	if (ui_handler_param == UI_HANDLER_CANCEL)
		ui_set_handler(handler_ingame, 0);

	/* add a message if we are rescaling */
	if (display_rescale_message)
	{
		display_rescale_message = FALSE;
		if (allow_rescale == 0)
			allow_rescale = 2;
		ui_draw_text_box("Updating Artwork...", JUSTIFY_CENTER, 0.5f, 0.5f, messagebox_backcolor);
	}

	/* decrement the frame counter if it is non-zero */
	else if (allow_rescale != 0)
		allow_rescale--;

#ifdef MESS
	/* let MESS display its stuff */
	mess_ui_update(machine);
#endif /* MESS */
}


/*-------------------------------------------------
    ui_get_font - return the UI font
-------------------------------------------------*/

render_font *ui_get_font(void)
{
	return ui_font;
}


/*-------------------------------------------------
    ui_get_line_height - return the current height
    of a line
-------------------------------------------------*/

float ui_get_line_height(void)
{
	INT32 raw_font_pixel_height = render_font_get_pixel_height(ui_font);
	INT32 target_pixel_width, target_pixel_height;
	float one_to_one_line_height;
	float target_aspect;
	float scale_factor;

	/* get info about the UI target */
	render_target_get_bounds(render_get_ui_target(), &target_pixel_width, &target_pixel_height, &target_aspect);

	/* compute the font pixel height at the nominal size */
	one_to_one_line_height = (float)raw_font_pixel_height / (float)target_pixel_height;

	/* determine the scale factor */
	scale_factor = UI_TARGET_FONT_HEIGHT / one_to_one_line_height;

	/* if our font is small-ish, do integral scaling */
	if (raw_font_pixel_height < 24)
	{
		/* do we want to scale smaller? only do so if we exceed the threshhold */
		if (scale_factor <= 1.0f)
		{
			if (one_to_one_line_height < UI_MAX_FONT_HEIGHT || raw_font_pixel_height < 12)
				scale_factor = 1.0f;
		}

		/* otherwise, just ensure an integral scale factor */
		else
			scale_factor = floor(scale_factor);
	}

	/* otherwise, just make sure we hit an even number of pixels */
	else
	{
		INT32 height = scale_factor * one_to_one_line_height * (float)target_pixel_height;
		scale_factor = (float)height / (one_to_one_line_height * (float)target_pixel_height);
	}

	return scale_factor * one_to_one_line_height;
}


/*-------------------------------------------------
    ui_get_char_width - return the width of a
    single character
-------------------------------------------------*/

float ui_get_char_width(unicode_char ch)
{
	return render_font_get_char_width(ui_font, ui_get_line_height(), render_get_ui_aspect(), ch);
}


/*-------------------------------------------------
    ui_get_string_width - return the width of a
    character string
-------------------------------------------------*/

float ui_get_string_width(const char *s)
{
	return render_font_get_utf8string_width(ui_font, ui_get_line_height(), render_get_ui_aspect(), s);
}


/*-------------------------------------------------
    ui_draw_outlined_box - add primitives to draw
    an outlined box with the given background
    color
-------------------------------------------------*/

void ui_draw_outlined_box(float x0, float y0, float x1, float y1, rgb_t backcolor)
{
	float hw = UI_LINE_WIDTH * 0.5f;
	render_ui_add_rect(x0, y0, x1, y1, backcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x0 + hw, y0 + hw, x1 - hw, y0 + hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x1 - hw, y0 + hw, x1 - hw, y1 - hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x1 - hw, y1 - hw, x0 + hw, y1 - hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(x0 + hw, y1 - hw, x0 + hw, y0 + hw, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


/*-------------------------------------------------
    ui_draw_text - simple text renderer
-------------------------------------------------*/

void ui_draw_text(const char *buf, float x, float y)
{
	ui_draw_text_full(buf, x, y, 1.0f - x, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}


/*-------------------------------------------------
    ui_draw_text_full - full featured text
    renderer with word wrapping, justification,
    and full size computation
-------------------------------------------------*/

void ui_draw_text_full(const char *origs, float x, float y, float wrapwidth, int justify, int wrap, int draw, rgb_t fgcolor, rgb_t bgcolor, float *totalwidth, float *totalheight)
{
	float lineheight = ui_get_line_height();
	const char *ends = origs + strlen(origs);
	const char *s = origs;
	const char *linestart;
	float cury = y;
	float maxwidth = 0;

	/* if we don't want wrapping, guarantee a huge wrapwidth */
	if (wrap == WRAP_NEVER)
		wrapwidth = 1000000.0f;
	if (wrapwidth <= 0)
		return;

	/* loop over lines */
	while (*s != 0)
	{
		const char *lastbreak = NULL;
		int line_justify = justify;
		unicode_char schar;
		int scharcount;
		float lastbreak_width = 0;
		float curwidth = 0;
		float curx = x;

		/* get the current character */
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		/* if the line starts with a tab character, center it regardless */
		if (schar == '\t')
		{
			s += scharcount;
			line_justify = JUSTIFY_CENTER;
		}

		/* remember the starting position of the line */
		linestart = s;

		/* loop while we have characters and are less than the wrapwidth */
		while (*s != 0 && curwidth <= wrapwidth)
		{
			float chwidth;

			/* get the current chcaracter */
			scharcount = uchar_from_utf8(&schar, s, ends - s);
			if (scharcount == -1)
				break;

			/* if we hit a newline, stop immediately */
			if (schar == '\n')
				break;

			/* get the width of this character */
			chwidth = ui_get_char_width(schar);

			/* if we hit a space, remember the location and width *without* the space */
			if (schar == ' ')
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}

			/* add the width of this character and advance */
			curwidth += chwidth;
			s += scharcount;

			/* if we hit any non-space breakable character, remember the location and width
               *with* the breakable character */
			if (schar != ' ' && is_breakable_char(schar) && curwidth <= wrapwidth)
			{
				lastbreak = s;
				lastbreak_width = curwidth;
			}
		}

		/* if we accumulated too much for the current width, we need to back off */
		if (curwidth > wrapwidth)
		{
			/* if we're word wrapping, back up to the last break if we can */
			if (wrap == WRAP_WORD)
			{
				/* if we hit a break, back up to there with the appropriate width */
				if (lastbreak != NULL)
				{
					s = lastbreak;
					curwidth = lastbreak_width;
				}

				/* if we didn't hit a break, back up one character */
				else if (s > linestart)
				{
					/* get the previous character */
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
						break;

					curwidth -= ui_get_char_width(schar);
				}
			}

			/* if we're truncating, make sure we have enough space for the ... */
			else if (wrap == WRAP_TRUNCATE)
			{
				/* add in the width of the ... */
				curwidth += 3.0f * ui_get_char_width('.');

				/* while we are above the wrap width, back up one character */
				while (curwidth > wrapwidth && s > linestart)
				{
					/* get the previous character */
					s = (const char *)utf8_previous_char(s);
					scharcount = uchar_from_utf8(&schar, s, ends - s);
					if (scharcount == -1)
						break;

					curwidth -= ui_get_char_width(schar);
				}
			}
		}

		/* align according to the justfication */
		if (line_justify == JUSTIFY_CENTER)
			curx += (wrapwidth - curwidth) * 0.5f;
		else if (line_justify == JUSTIFY_RIGHT)
			curx += wrapwidth - curwidth;

		/* track the maximum width of any given line */
		if (curwidth > maxwidth)
			maxwidth = curwidth;

		/* if opaque, add a black box */
		if (draw == DRAW_OPAQUE)
			render_ui_add_rect(curx, cury, curx + curwidth, cury + lineheight, bgcolor, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

		/* loop from the line start and add the characters */
		while (linestart < s)
		{
			/* get the current character */
			unicode_char linechar;
			int linecharcount = uchar_from_utf8(&linechar, linestart, ends - linestart);
			if (linecharcount == -1)
				break;

			if (draw != DRAW_NONE)
			{
				render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, linechar);
				curx += ui_get_char_width(linechar);
			}
			linestart += linecharcount;
		}

		/* append ellipses if needed */
		if (wrap == WRAP_TRUNCATE && *s != 0 && draw != DRAW_NONE)
		{
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
			render_ui_add_char(curx, cury, lineheight, render_get_ui_aspect(), fgcolor, ui_font, '.');
			curx += ui_get_char_width('.');
		}

		/* if we're not word-wrapping, we're done */
		if (wrap != WRAP_WORD)
			break;

		/* advance by a row */
		cury += lineheight;

		/* skip past any spaces at the beginning of the next line */
		scharcount = uchar_from_utf8(&schar, s, ends - s);
		if (scharcount == -1)
			break;

		if (schar == '\n')
			s += scharcount;
		else
			while (*s && isspace(schar))
			{
				s += scharcount;
				scharcount = uchar_from_utf8(&schar, s, ends - s);
				if (scharcount == -1)
					break;
			}
	}

	/* report the width and height of the resulting space */
	if (totalwidth)
		*totalwidth = maxwidth;
	if (totalheight)
		*totalheight = cury - y;
}


/*-------------------------------------------------
    ui_draw_text_box - draw a multiline text
    message with a box around it
-------------------------------------------------*/

void ui_draw_text_box(const char *text, int justify, float xpos, float ypos, rgb_t backcolor)
{
	float target_width, target_height;
	float target_x, target_y;

	/* compute the multi-line target width/height */
	ui_draw_text_full(text, 0, 0, 1.0f - 2.0f * UI_BOX_LR_BORDER,
				justify, WRAP_WORD, DRAW_NONE, ARGB_WHITE, ARGB_BLACK, &target_width, &target_height);
	if (target_height > 1.0f - 2.0f * UI_BOX_TB_BORDER)
		target_height = floor((1.0f - 2.0f * UI_BOX_TB_BORDER) / ui_get_line_height()) * ui_get_line_height();

	/* determine the target location */
	target_x = xpos - 0.5f * target_width;
	target_y = ypos - 0.5f * target_height;

	/* make sure we stay on-screen */
	if (target_x < UI_BOX_LR_BORDER)
		target_x = UI_BOX_LR_BORDER;
	if (target_x + target_width + UI_BOX_LR_BORDER > 1.0f)
		target_x = 1.0f - UI_BOX_LR_BORDER - target_width;
	if (target_y < UI_BOX_TB_BORDER)
		target_y = UI_BOX_TB_BORDER;
	if (target_y + target_height + UI_BOX_TB_BORDER > 1.0f)
		target_y = 1.0f - UI_BOX_TB_BORDER - target_height;

	/* add a box around that */
	ui_draw_outlined_box(target_x - UI_BOX_LR_BORDER,
					 target_y - UI_BOX_TB_BORDER,
					 target_x + target_width + UI_BOX_LR_BORDER,
					 target_y + target_height + UI_BOX_TB_BORDER, backcolor);
	ui_draw_text_full(text, target_x, target_y, target_width,
				justify, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
}


/*-------------------------------------------------
    ui_popup_time - popup a message for a specific
    amount of time
-------------------------------------------------*/

void CLIB_DECL ui_popup_time(int seconds, const char *text, ...)
{
	va_list arg;

	/* extract the text */
	va_start(arg,text);
	vsprintf(messagebox_text, text, arg);
	messagebox_backcolor = UI_FILLCOLOR;
	va_end(arg);

	/* set a timer */
	popup_text_end = osd_ticks() + osd_ticks_per_second() * seconds;
}


/*-------------------------------------------------
    ui_show_fps_temp - show the FPS counter for
    a specific period of time
-------------------------------------------------*/

void ui_show_fps_temp(double seconds)
{
	if (!showfps)
		showfps_end = osd_ticks() + seconds * osd_ticks_per_second();
}


/*-------------------------------------------------
    ui_set_show_fps - show/hide the FPS counter
-------------------------------------------------*/

void ui_set_show_fps(int show)
{
	showfps = show;
	if (!show)
	{
		showfps = 0;
		showfps_end = 0;
	}
}


/*-------------------------------------------------
    ui_get_show_fps - return the current FPS
    counter visibility state
-------------------------------------------------*/

int ui_get_show_fps(void)
{
	return showfps || (showfps_end != 0);
}


/*-------------------------------------------------
    ui_set_show_profiler - show/hide the profiler
-------------------------------------------------*/

void ui_set_show_profiler(int show)
{
	if (show)
	{
		show_profiler = TRUE;
		profiler_start();
	}
	else
	{
		show_profiler = FALSE;
		profiler_stop();
	}
}


/*-------------------------------------------------
    ui_get_show_profiler - return the current
    profiler visibility state
-------------------------------------------------*/

int ui_get_show_profiler(void)
{
	return show_profiler;
}


/*-------------------------------------------------
    ui_show_menu - show the menus
-------------------------------------------------*/

void ui_show_menu(void)
{
	ui_set_handler(ui_menu_ui_handler, 0);
}


/*-------------------------------------------------
    ui_is_menu_active - return TRUE if the menu
    UI handler is active
-------------------------------------------------*/

int ui_is_menu_active(void)
{
	return (ui_handler_callback == ui_menu_ui_handler);
}


/*-------------------------------------------------
    ui_is_slider_active - return TRUE if the slider
    UI handler is active
-------------------------------------------------*/

int ui_is_slider_active(void)
{
	return (ui_handler_callback == handler_slider);
}



/***************************************************************************
    TEXT GENERATORS
***************************************************************************/

/*-------------------------------------------------
    sprintf_disclaimer - print the disclaimer
    text to the given buffer
-------------------------------------------------*/

static int sprintf_disclaimer(running_machine *machine, char *buffer)
{
	char *bufptr = buffer;
	bufptr += sprintf(bufptr, "Usage of emulators in conjunction with ROMs you don't own is forbidden by copyright law.\n\n");
	bufptr += sprintf(bufptr, "IF YOU ARE NOT LEGALLY ENTITLED TO PLAY \"%s\" ON THIS EMULATOR, PRESS ESC.\n\n", machine->gamedrv->description);
	bufptr += sprintf(bufptr, "Otherwise, type OK or move the joystick left then right to continue");
	return bufptr - buffer;
}


/*-------------------------------------------------
    sprintf_warnings - print the warning flags
    text to the given buffer
-------------------------------------------------*/

static int sprintf_warnings(running_machine *machine, char *buffer)
{
#define WARNING_FLAGS (	GAME_NOT_WORKING | \
						GAME_UNEMULATED_PROTECTION | \
						GAME_WRONG_COLORS | \
						GAME_IMPERFECT_COLORS | \
						GAME_NO_SOUND |  \
						GAME_IMPERFECT_SOUND |  \
						GAME_IMPERFECT_GRAPHICS | \
						GAME_NO_COCKTAIL)
	char *bufptr = buffer;
	int i;

	/* if no warnings, nothing to return */
	if (rom_load_warnings() == 0 && !(machine->gamedrv->flags & WARNING_FLAGS))
		return 0;

	/* add a warning if any ROMs were loaded with warnings */
	if (rom_load_warnings() > 0)
	{
		bufptr += sprintf(bufptr, "One or more ROMs/CHDs for this game are incorrect. The " GAMENOUN " may not run correctly.\n");
		if (machine->gamedrv->flags & WARNING_FLAGS)
			*bufptr++ = '\n';
	}

	/* if we have at least one warning flag, print the general header */
	if (machine->gamedrv->flags & WARNING_FLAGS)
	{
		bufptr += sprintf(bufptr, "There are known problems with this " GAMENOUN "\n\n");

		/* add one line per warning flag */
#ifdef MESS
		if (machine->gamedrv->flags & GAME_COMPUTER)
			bufptr += sprintf(bufptr, "%s\n\n%s\n", "The emulated system is a computer: ", "The keyboard emulation may not be 100% accurate.");
#endif
		if (machine->gamedrv->flags & GAME_IMPERFECT_COLORS)
			bufptr += sprintf(bufptr, "The colors aren't 100%% accurate.\n");
		if (machine->gamedrv->flags & GAME_WRONG_COLORS)
			bufptr += sprintf(bufptr, "The colors are completely wrong.\n");
		if (machine->gamedrv->flags & GAME_IMPERFECT_GRAPHICS)
			bufptr += sprintf(bufptr, "The video emulation isn't 100%% accurate.\n");
		if (machine->gamedrv->flags & GAME_IMPERFECT_SOUND)
			bufptr += sprintf(bufptr, "The sound emulation isn't 100%% accurate.\n");
		if (machine->gamedrv->flags & GAME_NO_SOUND)
			bufptr += sprintf(bufptr, "The game lacks sound.\n");
		if (machine->gamedrv->flags & GAME_NO_COCKTAIL)
			bufptr += sprintf(bufptr, "Screen flipping in cocktail mode is not supported.\n");

		/* if there's a NOT WORKING or UNEMULATED PROTECTION warning, make it stronger */
		if (machine->gamedrv->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
		{
			const game_driver *maindrv;
			const game_driver *clone_of;
			int foundworking;

			/* add the strings for these warnings */
			if (machine->gamedrv->flags & GAME_NOT_WORKING)
				bufptr += sprintf(bufptr, "THIS " CAPGAMENOUN " DOESN'T WORK. You won't be able to make it work correctly.  Don't bother.\n");
			if (machine->gamedrv->flags & GAME_UNEMULATED_PROTECTION)
				bufptr += sprintf(bufptr, "The game has protection which isn't fully emulated.\n");

			/* find the parent of this driver */
			clone_of = driver_get_clone(machine->gamedrv);
			if (clone_of != NULL && !(clone_of->flags & GAME_IS_BIOS_ROOT))
 				maindrv = clone_of;
			else
				maindrv = machine->gamedrv;

			/* scan the driver list for any working clones and add them */
			foundworking = 0;
			for (i = 0; drivers[i] != NULL; i++)
				if (drivers[i] == maindrv || driver_get_clone(drivers[i]) == maindrv)
					if ((drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)) == 0)
					{
						/* this one works, add a header and display the name of the clone */
						if (foundworking == 0)
							bufptr += sprintf(bufptr, "\n\nThere are working clones of this game. They are:\n\n");
						bufptr += sprintf(bufptr, "%s\n", drivers[i]->name);
						foundworking = 1;
					}
		}
	}

	/* add the 'press OK' string */
	bufptr += sprintf(bufptr, "\n\nType OK or move the joystick left then right to continue");
	return bufptr - buffer;
}


/*-------------------------------------------------
    sprintf_game_info - print the game info text
    to the given buffer
-------------------------------------------------*/

int sprintf_game_info(running_machine *machine, char *buffer)
{
	int scrcount = video_screen_count(machine->config);
	char *bufptr = buffer;
	int cpunum, sndnum;
	int count;

	/* print description, manufacturer, and CPU: */
	bufptr += sprintf(bufptr, "%s\n%s %s\n\nCPU:\n", machine->gamedrv->description, machine->gamedrv->year, machine->gamedrv->manufacturer);

	/* loop over all CPUs */
	for (cpunum = 0; cpunum < MAX_CPU && machine->config->cpu[cpunum].type != CPU_DUMMY; cpunum += count)
	{
		cpu_type type = machine->config->cpu[cpunum].type;
		int clock = machine->config->cpu[cpunum].clock;

		/* count how many identical CPUs we have */
		for (count = 1; cpunum + count < MAX_CPU; count++)
			if (machine->config->cpu[cpunum + count].type != type ||
		        machine->config->cpu[cpunum + count].clock != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			bufptr += sprintf(bufptr, "%d" UTF8_MULTIPLY, count);
		bufptr += sprintf(bufptr, "%s", cputype_name(type));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			bufptr += sprintf(bufptr, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else
			bufptr += sprintf(bufptr, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
	}

	/* loop over all sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND && machine->config->sound[sndnum].type != SOUND_DUMMY; sndnum += count)
	{
		sound_type type = machine->config->sound[sndnum].type;
		int clock = sndnum_clock(sndnum);

		/* append the Sound: string */
		if (sndnum == 0)
			bufptr += sprintf(bufptr, "\nSound:\n");

		/* count how many identical sound chips we have */
		for (count = 1; sndnum + count < MAX_SOUND; count++)
			if (machine->config->sound[sndnum + count].type != type ||
		        sndnum_clock(sndnum + count) != clock)
		    	break;

		/* if more than one, prepend a #x in front of the CPU name */
		if (count > 1)
			bufptr += sprintf(bufptr, "%d" UTF8_MULTIPLY, count);
		bufptr += sprintf(bufptr, "%s", sndnum_name(sndnum));

		/* display clock in kHz or MHz */
		if (clock >= 1000000)
			bufptr += sprintf(bufptr, " %d.%06d" UTF8_NBSP "MHz\n", clock / 1000000, clock % 1000000);
		else if (clock != 0)
			bufptr += sprintf(bufptr, " %d.%03d" UTF8_NBSP "kHz\n", clock / 1000, clock % 1000);
		else
			*bufptr++ = '\n';
	}

	/* display screen information */
	bufptr += sprintf(bufptr, "\nVideo:\n");
	if (scrcount == 0)
		buffer += sprintf(bufptr, "None\n");
	else
	{
		const device_config *screen;

		for (screen = video_screen_first(machine->config); screen != NULL; screen = video_screen_next(screen))
		{
			const screen_config *scrconfig = screen->inline_config;

			if (scrcount > 1)
				bufptr += sprintf(bufptr, "%s: ", slider_get_screen_desc(screen));

			if (scrconfig->type == SCREEN_TYPE_VECTOR)
				bufptr += sprintf(bufptr, "Vector\n");
			else
			{
				const rectangle *visarea = video_screen_get_visible_area(screen);

				bufptr += sprintf(bufptr, "%d " UTF8_MULTIPLY " %d (%s) %f" UTF8_NBSP "Hz\n",
						visarea->max_x - visarea->min_x + 1,
						visarea->max_y - visarea->min_y + 1,
						(machine->gamedrv->flags & ORIENTATION_SWAP_XY) ? "V" : "H",
						ATTOSECONDS_TO_HZ(video_screen_get_frame_period(screen).attoseconds));
			}
		}
	}

	return bufptr - buffer;
}



/***************************************************************************
    UI HANDLERS
***************************************************************************/

/*-------------------------------------------------
    handler_messagebox - displays the current
    messagebox_text string but handles no input
-------------------------------------------------*/

static UINT32 handler_messagebox(running_machine *machine, UINT32 state)
{
	ui_draw_text_box(messagebox_text, JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);
	return 0;
}


/*-------------------------------------------------
    handler_messagebox_ok - displays the current
    messagebox_text string and waits for an OK
-------------------------------------------------*/

static UINT32 handler_messagebox_ok(running_machine *machine, UINT32 state)
{
	/* draw a standard message window */
	ui_draw_text_box(messagebox_text, JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	/* an 'O' or left joystick kicks us to the next state */
	if (state == 0 && (input_code_pressed_once(KEYCODE_O) || input_ui_pressed(machine, IPT_UI_LEFT)))
		state++;

	/* a 'K' or right joystick exits the state */
	else if (state == 1 && (input_code_pressed_once(KEYCODE_K) || input_ui_pressed(machine, IPT_UI_RIGHT)))
		state = UI_HANDLER_CANCEL;

	/* if the user cancels, exit out completely */
	else if (input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		mame_schedule_exit(machine);
		state = UI_HANDLER_CANCEL;
	}

	return state;
}


/*-------------------------------------------------
    handler_messagebox_anykey - displays the
    current messagebox_text string and waits for
    any keypress
-------------------------------------------------*/

static UINT32 handler_messagebox_anykey(running_machine *machine, UINT32 state)
{
	/* draw a standard message window */
	ui_draw_text_box(messagebox_text, JUSTIFY_LEFT, 0.5f, 0.5f, messagebox_backcolor);

	/* if the user cancels, exit out completely */
	if (input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		mame_schedule_exit(machine);
		state = UI_HANDLER_CANCEL;
	}

	/* if any key is pressed, just exit */
	else if (input_code_poll_switches(FALSE) != INPUT_CODE_INVALID)
		state = UI_HANDLER_CANCEL;

	return state;
}


/*-------------------------------------------------
    handler_ingame - in-game handler takes care
    of the standard keypresses
-------------------------------------------------*/

static UINT32 handler_ingame(running_machine *machine, UINT32 state)
{
	int is_paused = mame_is_paused(machine);

	/* first draw the FPS counter */
	if (showfps || osd_ticks() < showfps_end)
	{
		ui_draw_text_full(video_get_speed_text(machine), 0.0f, 0.0f, 1.0f,
					JUSTIFY_RIGHT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ARGB_BLACK, NULL, NULL);
	}
	else
		showfps_end = 0;

	/* draw the profiler if visible */
	if (show_profiler)
		ui_draw_text_full(profiler_get_text(), 0.0f, 0.0f, 1.0f, JUSTIFY_LEFT, WRAP_WORD, DRAW_OPAQUE, ARGB_WHITE, ARGB_BLACK, NULL, NULL);

	/* let the cheat engine display its stuff */
	if (options_get_bool(mame_options(), OPTION_CHEAT))
		cheat_display_watches();

	/* display any popup messages */
	if (osd_ticks() < popup_text_end)
		ui_draw_text_box(messagebox_text, JUSTIFY_CENTER, 0.5f, 0.9f, messagebox_backcolor);
	else
		popup_text_end = 0;

	/* if we're single-stepping, pause now */
	if (single_step)
	{
		mame_pause(machine, TRUE);
		single_step = FALSE;
	}

#ifdef MESS
	if (mess_disable_builtin_ui(machine))
		return 0;
#endif /* MESS */

	/* if the user pressed ESC, stop the emulation */
	if (input_ui_pressed(machine, IPT_UI_CANCEL))
		mame_schedule_exit(machine);

	/* turn on menus if requested */
	if (input_ui_pressed(machine, IPT_UI_CONFIGURE))
		return ui_set_handler(ui_menu_ui_handler, 0);

	/* if the on-screen display isn't up and the user has toggled it, turn it on */
	if (!machine->debug_mode && input_ui_pressed(machine, IPT_UI_ON_SCREEN_DISPLAY))
		return ui_set_handler(handler_slider, 0);

	/* handle a reset request */
	if (input_ui_pressed(machine, IPT_UI_RESET_MACHINE))
		mame_schedule_hard_reset(machine);
	if (input_ui_pressed(machine, IPT_UI_SOFT_RESET))
		mame_schedule_soft_reset(machine);

	/* handle a request to display graphics/palette */
	if (input_ui_pressed(machine, IPT_UI_SHOW_GFX))
	{
		if (!is_paused)
			mame_pause(machine, TRUE);
		return ui_set_handler(ui_gfx_ui_handler, is_paused);
	}

	/* handle a save state request */
	if (input_ui_pressed(machine, IPT_UI_SAVE_STATE))
	{
		mame_pause(machine, TRUE);
		return ui_set_handler(handler_load_save, LOADSAVE_SAVE);
	}

	/* handle a load state request */
	if (input_ui_pressed(machine, IPT_UI_LOAD_STATE))
	{
		mame_pause(machine, TRUE);
		return ui_set_handler(handler_load_save, LOADSAVE_LOAD);
	}

	/* handle a save snapshot request */
	if (input_ui_pressed(machine, IPT_UI_SNAPSHOT))
		video_save_active_screen_snapshots(machine);

	/* toggle pause */
	if (input_ui_pressed(machine, IPT_UI_PAUSE))
	{
		/* with a shift key, it is single step */
		if (is_paused && (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT)))
		{
			single_step = TRUE;
			mame_pause(machine, FALSE);
		}
		else
			mame_pause(machine, !mame_is_paused(machine));
	}

	/* toggle movie recording */
	if (input_ui_pressed(machine, IPT_UI_RECORD_MOVIE))
	{
		if (!video_mng_is_movie_active(machine))
		{
			video_mng_begin_recording(machine, NULL);
			popmessage("REC START");
		}
		else
		{
			video_mng_end_recording(machine);
			popmessage("REC STOP");
		}
	}

	/* toggle profiler display */
	if (input_ui_pressed(machine, IPT_UI_SHOW_PROFILER))
		ui_set_show_profiler(!ui_get_show_profiler());

	/* toggle FPS display */
	if (input_ui_pressed(machine, IPT_UI_SHOW_FPS))
		ui_set_show_fps(!ui_get_show_fps());

	/* toggle crosshair display */
	if (input_ui_pressed(machine, IPT_UI_TOGGLE_CROSSHAIR))
		crosshair_toggle(machine);

	/* increment frameskip? */
	if (input_ui_pressed(machine, IPT_UI_FRAMESKIP_INC))
	{
		/* get the current value and increment it */
		int newframeskip = video_get_frameskip() + 1;
		if (newframeskip > MAX_FRAMESKIP)
			newframeskip = -1;
		video_set_frameskip(newframeskip);

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);
	}

	/* decrement frameskip? */
	if (input_ui_pressed(machine, IPT_UI_FRAMESKIP_DEC))
	{
		/* get the current value and decrement it */
		int newframeskip = video_get_frameskip() - 1;
		if (newframeskip < -1)
			newframeskip = MAX_FRAMESKIP;
		video_set_frameskip(newframeskip);

		/* display the FPS counter for 2 seconds */
		ui_show_fps_temp(2.0);
	}

	/* toggle throttle? */
	if (input_ui_pressed(machine, IPT_UI_THROTTLE))
		video_set_throttle(!video_get_throttle());

	/* check for fast forward */
	if (input_type_pressed(machine, IPT_UI_FAST_FORWARD, 0))
	{
		video_set_fastforward(TRUE);
		ui_show_fps_temp(0.5);
	}
	else
		video_set_fastforward(FALSE);

#ifdef MESS
	/* paste? */
	if (input_ui_pressed(machine, IPT_UI_PASTE))
		ui_paste(machine);
#endif /* MESS */

	return 0;
}


/*-------------------------------------------------
    handler_slider - displays the current slider
    and calls the slider handler
-------------------------------------------------*/

static UINT32 handler_slider(running_machine *machine, UINT32 state)
{
	slider_state *cur = &slider_list[slider_current];
	INT32 increment = 0, newval;
	char textbuf[256];

	/* left/right control the increment */
	if (input_ui_pressed_repeat(machine, IPT_UI_LEFT,6))
		increment = -cur->incval;
	if (input_ui_pressed_repeat(machine, IPT_UI_RIGHT,6))
		increment = cur->incval;

	/* alt goes to 1, shift goes 10x smaller, control goes 10x larger */
	if (increment != 0)
	{
		if (input_code_pressed(KEYCODE_LALT) || input_code_pressed(KEYCODE_RALT))
			increment = (increment < 0) ? -1 : 1;
		if (input_code_pressed(KEYCODE_LSHIFT) || input_code_pressed(KEYCODE_RSHIFT))
			increment = (increment < -10 || increment > 10) ? (increment / 10) : ((increment < 0) ? -1 : 1);
		if (input_code_pressed(KEYCODE_LCONTROL) || input_code_pressed(KEYCODE_RCONTROL))
			increment *= 10;
	}

	/* determine the new value */
	newval = (*cur->update)(machine, 0, NULL, cur->arg) + increment;

	/* select resets to the default value */
	if (input_ui_pressed(machine, IPT_UI_SELECT))
		newval = cur->defval;

	/* clamp within bounds */
	if (newval < cur->minval)
		newval = cur->minval;
	if (newval > cur->maxval)
		newval = cur->maxval;

	/* update the new data and get the text */
	(*cur->update)(machine, newval, textbuf, cur->arg);

	/* display the UI */
	slider_display(textbuf, cur->minval, cur->maxval, cur->defval, newval);

	/* up/down select which slider to control */
	if (input_ui_pressed_repeat(machine, IPT_UI_DOWN,6))
		slider_current = (slider_current + 1) % slider_count;
	if (input_ui_pressed_repeat(machine, IPT_UI_UP,6))
		slider_current = (slider_current + slider_count - 1) % slider_count;

	/* the slider toggle or ESC will cancel out of our display */
	if (input_ui_pressed(machine, IPT_UI_ON_SCREEN_DISPLAY) || input_ui_pressed(machine, IPT_UI_CANCEL))
		return UI_HANDLER_CANCEL;

	/* the menu key will take us directly to the menu */
	if (input_ui_pressed(machine, IPT_UI_CONFIGURE))
		return ui_set_handler(ui_menu_ui_handler, 0);

	return 0;
}


/*-------------------------------------------------
    handler_load_save - leads the user through
    specifying a game to save or load
-------------------------------------------------*/

static UINT32 handler_load_save(running_machine *machine, UINT32 state)
{
	char filename[20];
	input_code code;
	char file = 0;

	/* if we're not in the middle of anything, skip */
	if (state == LOADSAVE_NONE)
		return 0;

	/* okay, we're waiting for a key to select a slot; display a message */
	if (state == LOADSAVE_SAVE)
		ui_draw_message_window("Select position to save to");
	else
		ui_draw_message_window("Select position to load from");

	/* check for cancel key */
	if (input_ui_pressed(machine, IPT_UI_CANCEL))
	{
		/* display a popup indicating things were cancelled */
		if (state == LOADSAVE_SAVE)
			popmessage("Save cancelled");
		else
			popmessage("Load cancelled");

		/* reset the state */
		mame_pause(machine, FALSE);
		return UI_HANDLER_CANCEL;
	}

	/* check for A-Z or 0-9 */
	for (code = KEYCODE_A; code <= (input_code)KEYCODE_Z; code++)
		if (input_code_pressed_once(code))
			file = code - KEYCODE_A + 'a';
	if (file == 0)
		for (code = KEYCODE_0; code <= (input_code)KEYCODE_9; code++)
			if (input_code_pressed_once(code))
				file = code - KEYCODE_0 + '0';
	if (file == 0)
		for (code = KEYCODE_0_PAD; code <= (input_code)KEYCODE_9_PAD; code++)
			if (input_code_pressed_once(code))
				file = code - KEYCODE_0_PAD + '0';
	if (file == 0)
		return state;

	/* display a popup indicating that the save will proceed */
	sprintf(filename, "%c", file);
	if (state == LOADSAVE_SAVE)
	{
		popmessage("Save to position %c", file);
		mame_schedule_save(machine, filename);
	}
	else
	{
		popmessage("Load from position %c", file);
		mame_schedule_load(machine, filename);
	}

	/* remove the pause and reset the state */
	mame_pause(machine, FALSE);
	return UI_HANDLER_CANCEL;
}



/***************************************************************************
    SLIDER CONTROLS
***************************************************************************/

/*-------------------------------------------------
    slider_init - initialize the list of slider
    controls
-------------------------------------------------*/

static void slider_init(running_machine *machine)
{
	int numscreens = video_screen_count(machine->config);
	const input_port_config *port;
	const input_field_config *field;
	const device_config *device;
	int numitems, item;

	slider_count = 0;

	/* add overall volume */
	slider_config(&slider_list[slider_count++], -32, 0, 0, 1, slider_volume, NULL);

	/* add per-channel volume */
	numitems = sound_get_user_gain_count();
	for (item = 0; item < numitems; item++)
	{
		INT32 maxval = 2000;
		INT32 defval = sound_get_default_gain(item) * 1000.0f + 0.5f;

		if (defval > 1000)
			maxval = 2 * defval;

		slider_config(&slider_list[slider_count++], 0, defval, maxval, 20, slider_mixervol, (void *)(FPTR)item);
	}

	/* add analog adjusters */
	for (port = machine->portconfig; port != NULL; port = port->next)
		for (field = port->fieldlist; field != NULL; field = field->next)
			if (field->type == IPT_ADJUSTER)
			{
				void *param = (void *)field;
				slider_config(&slider_list[slider_count++], 0, field->defvalue, 100, 1, slider_adjuster, param);
			}

	if (options_get_bool(mame_options(), OPTION_CHEAT))
	{
		/* add CPU overclocking */
		numitems = cpu_gettotalcpu();
		for (item = 0; item < numitems; item++)
			slider_config(&slider_list[slider_count++], 10, 1000, 2000, 1, slider_overclock, (void *)(FPTR)item);
	}

	for (item = 0; item < numscreens; item++)
	{
		const device_config *screen = device_list_find_by_index(machine->config->devicelist, VIDEO_SCREEN, item);
		const screen_config *scrconfig = screen->inline_config;
		int defxscale = floor(scrconfig->xscale * 1000.0f + 0.5f);
		int defyscale = floor(scrconfig->yscale * 1000.0f + 0.5f);
		int defxoffset = floor(scrconfig->xoffset * 1000.0f + 0.5f);
		int defyoffset = floor(scrconfig->yoffset * 1000.0f + 0.5f);
		void *param = (void *)screen;

		/* add refresh rate tweaker */
		if (options_get_bool(mame_options(), OPTION_CHEAT))
			slider_config(&slider_list[slider_count++], -10000, 0, 10000, 1000, slider_refresh, param);

		/* add standard brightness/contrast/gamma controls per-screen */
		slider_config(&slider_list[slider_count++], 100, 1000, 2000, 10, slider_brightness, param);
		slider_config(&slider_list[slider_count++], 100, 1000, 2000, 50, slider_contrast, param);
		slider_config(&slider_list[slider_count++], 100, 1000, 3000, 50, slider_gamma, param);

		/* add scale and offset controls per-screen */
		slider_config(&slider_list[slider_count++], 500, (defxscale == 0) ? 1000 : defxscale, 1500, 2, slider_xscale, param);
		slider_config(&slider_list[slider_count++], -500, defxoffset, 500, 2, slider_xoffset, param);
		slider_config(&slider_list[slider_count++], 500, (defyscale == 0) ? 1000 : defyscale, 1500, 2, slider_yscale, param);
		slider_config(&slider_list[slider_count++], -500, defyoffset, 500, 2, slider_yoffset, param);
	}

	for (device = video_screen_first(machine->config); device != NULL; device = video_screen_next(device))
	{
		const screen_config *scrconfig = device->inline_config;
		if (scrconfig->type == SCREEN_TYPE_VECTOR)
		{
			/* add flicker control */
			slider_config(&slider_list[slider_count++], 0, 0, 1000, 10, slider_flicker, NULL);
			slider_config(&slider_list[slider_count++], 10, 100, 1000, 10, slider_beam, NULL);
			break;
		}
	}

#ifdef MAME_DEBUG
	/* add crosshair adjusters */
	for (port = machine->portconfig; port != NULL; port = port->next)
		for (field = port->fieldlist; field != NULL; field = field->next)
			if (field->crossaxis != CROSSHAIR_AXIS_NONE && field->player == 0)
			{
				void *param = (void *)field;
				slider_config(&slider_list[slider_count++], -3000, 1000, 3000, 100, slider_crossscale, param);
				slider_config(&slider_list[slider_count++], -3000, 0, 3000, 100, slider_crossoffset, param);
			}
#endif
}


/*-------------------------------------------------
    slider_display - display a slider box with
    text
-------------------------------------------------*/

static void slider_display(const char *text, int minval, int maxval, int defval, int curval)
{
	float percentage = (float)(curval - minval) / (float)(maxval - minval);
	float default_percentage = (float)(defval - minval) / (float)(maxval - minval);
	float space_width = ui_get_char_width(' ');
	float line_height = ui_get_line_height();
	float ui_width, ui_height;
	float text_height;

	/* leave a spaces' worth of room along the left/right sides, and a lines' worth on the top/bottom */
	ui_width = 1.0f - 2.0f * space_width;
	ui_height = 1.0f - 2.0f * line_height;

	/* determine the text height */
	ui_draw_text_full(text, 0, 0, ui_width - 2 * UI_BOX_LR_BORDER,
				JUSTIFY_CENTER, WRAP_WORD, DRAW_NONE, ARGB_WHITE, ARGB_BLACK, NULL, &text_height);

	/* add a box around the whole area */
	ui_draw_outlined_box(space_width,
					 line_height + ui_height - text_height - line_height - 2 * UI_BOX_TB_BORDER,
					 space_width + ui_width,
					 line_height + ui_height, UI_FILLCOLOR);

	/* draw the thermometer */
	slider_draw_bar(2.0f * space_width, line_height + ui_height - UI_BOX_TB_BORDER - text_height - line_height * 0.75f,
			ui_width - 2.0f * space_width, line_height * 0.75f, percentage, default_percentage);

	/* draw the actual text */
	ui_draw_text_full(text, space_width + UI_BOX_LR_BORDER, line_height + ui_height - UI_BOX_TB_BORDER - text_height, ui_width - 2.0f * UI_BOX_LR_BORDER,
				JUSTIFY_CENTER, WRAP_WORD, DRAW_NORMAL, ARGB_WHITE, ARGB_BLACK, NULL, &text_height);
}


/*-------------------------------------------------
    slider_draw_bar - draw a slider thermometer
-------------------------------------------------*/

static void slider_draw_bar(float leftx, float topy, float width, float height, float percentage, float default_percentage)
{
	float current_x, default_x;
	float bar_top, bar_bottom;

	/* compute positions */
	bar_top = topy + 0.125f * height;
	bar_bottom = topy + 0.875f * height;
	default_x = leftx + width * default_percentage;
	current_x = leftx + width * percentage;

	/* fill in the percentage */
	render_ui_add_rect(leftx, bar_top, current_x, bar_bottom, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	/* draw the top and bottom lines */
	render_ui_add_line(leftx, bar_top, leftx + width, bar_top, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(leftx, bar_bottom, leftx + width, bar_bottom, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));

	/* draw default marker */
	render_ui_add_line(default_x, topy, default_x, bar_top, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
	render_ui_add_line(default_x, bar_bottom, default_x, topy + height, UI_LINE_WIDTH, ARGB_WHITE, PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA));
}


/*-------------------------------------------------
    slider_volume - global volume slider callback
-------------------------------------------------*/

static INT32 slider_volume(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	if (buffer != NULL)
	{
		sound_set_attenuation(newval);
		sprintf(buffer, "Master Volume %3ddB", sound_get_attenuation());
	}
	return sound_get_attenuation();
}


/*-------------------------------------------------
    slider_mixervol - single channel volume
    slider callback
-------------------------------------------------*/

static INT32 slider_mixervol(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	int which = (FPTR)arg;
	if (buffer != NULL)
	{
		sound_set_user_gain(which, (float)newval * 0.001f);
		sprintf(buffer, "%s Volume %4.2f", sound_get_user_gain_name(which), sound_get_user_gain(which));
	}
	return floor(sound_get_user_gain(which) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_adjuster - analog adjuster slider
    callback
-------------------------------------------------*/

static INT32 slider_adjuster(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const input_field_config *field = arg;
	input_field_user_settings settings;

	input_field_get_user_settings(field, &settings);
	if (buffer != NULL)
	{
		settings.value = newval;
		input_field_set_user_settings(field, &settings);
		sprintf(buffer, "%s %d%%", field->name, settings.value);
	}
	return settings.value;
}


/*-------------------------------------------------
    slider_overclock - CPU overclocker slider
    callback
-------------------------------------------------*/

static INT32 slider_overclock(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	int which = (FPTR)arg;
	if (buffer != NULL)
	{
		cpunum_set_clockscale(machine, which, (float)newval * 0.001f);
		sprintf(buffer, "Overclock CPU %d %3.0f%%", which, floor(cpunum_get_clockscale(which) * 100.0f + 0.5f));
	}
	return floor(cpunum_get_clockscale(which) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_refresh - refresh rate slider callback
-------------------------------------------------*/

static INT32 slider_refresh(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	const screen_config *scrconfig = screen->inline_config;
	double defrefresh = ATTOSECONDS_TO_HZ(scrconfig->refresh);
	double refresh;

	if (buffer != NULL)
	{
		int width = video_screen_get_width(screen);
		int height = video_screen_get_height(screen);
		const rectangle *visarea = video_screen_get_visible_area(screen);

		video_screen_configure(screen, width, height, visarea, HZ_TO_ATTOSECONDS(defrefresh + (double)newval * 0.001));
		sprintf(buffer, "%s Refresh Rate %.3ffps", slider_get_screen_desc(screen), ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds));
	}
	refresh = ATTOSECONDS_TO_HZ(video_screen_get_frame_period(machine->primary_screen).attoseconds);
	return floor((refresh - defrefresh) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_brightness - screen brightness slider
    callback
-------------------------------------------------*/

static INT32 slider_brightness(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_brightness(container, (float)newval * 0.001f);
		sprintf(buffer, "%s Brightness %.3f", slider_get_screen_desc(screen), render_container_get_brightness(container));
	}
	return floor(render_container_get_brightness(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_contrast - screen contrast slider
    callback
-------------------------------------------------*/

static INT32 slider_contrast(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_contrast(container, (float)newval * 0.001f);
		sprintf(buffer, "%s Contrast %.3f", slider_get_screen_desc(screen), render_container_get_contrast(container));
	}
	return floor(render_container_get_contrast(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_gamma - screen gamma slider callback
-------------------------------------------------*/

static INT32 slider_gamma(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_gamma(container, (float)newval * 0.001f);
		sprintf(buffer, "%s Gamma %.3f", slider_get_screen_desc(screen), render_container_get_gamma(container));
	}
	return floor(render_container_get_gamma(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_xscale - screen horizontal scale slider
    callback
-------------------------------------------------*/

static INT32 slider_xscale(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_xscale(container, (float)newval * 0.001f);
		sprintf(buffer, "%s %s %.3f", slider_get_screen_desc(screen), "Horiz Stretch", render_container_get_xscale(container));
	}
	return floor(render_container_get_xscale(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_yscale - screen vertical scale slider
    callback
-------------------------------------------------*/

static INT32 slider_yscale(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_yscale(container, (float)newval * 0.001f);
		sprintf(buffer, "%s %s %.3f", slider_get_screen_desc(screen), "Vert Stretch", render_container_get_yscale(container));
	}
	return floor(render_container_get_yscale(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_xoffset - screen horizontal position
    slider callback
-------------------------------------------------*/

static INT32 slider_xoffset(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_xoffset(container, (float)newval * 0.001f);
		sprintf(buffer, "%s %s %.3f", slider_get_screen_desc(screen), "Horiz Position", render_container_get_xoffset(container));
	}
	return floor(render_container_get_xoffset(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_yoffset - screen vertical position
    slider callback
-------------------------------------------------*/

static INT32 slider_yoffset(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	const device_config *screen = arg;
	render_container *container = render_container_get_screen(screen);
	if (buffer != NULL)
	{
		render_container_set_yoffset(container, (float)newval * 0.001f);
		sprintf(buffer, "%s %s %.3f", slider_get_screen_desc(screen), "Vert Position", render_container_get_yoffset(container));
	}
	return floor(render_container_get_yoffset(container) * 1000.0f + 0.5f);
}


/*-------------------------------------------------
    slider_flicker - vector flicker slider
    callback
-------------------------------------------------*/

static INT32 slider_flicker(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	if (buffer != NULL)
	{
		vector_set_flicker((float)newval * 0.1f);
		sprintf(buffer, "Vector Flicker %1.2f", vector_get_flicker());
	}
	return floor(vector_get_flicker() * 10.0f + 0.5f);
}


/*-------------------------------------------------
    slider_beam - vector beam width slider
    callback
-------------------------------------------------*/

static INT32 slider_beam(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	if (buffer != NULL)
	{
		vector_set_beam((float)newval * 0.01f);
		sprintf(buffer, "%s %1.2f", "Beam Width", vector_get_beam());
	}
	return floor(vector_get_beam() * 100.0f + 0.5f);
}


/*-------------------------------------------------
    slider_get_screen_desc - returns the
    description for a given screen index
-------------------------------------------------*/

static char *slider_get_screen_desc(const device_config *screen)
{
	int screen_count = video_screen_count(screen->machine->config);
	static char descbuf[256];

	if (screen_count > 1)
		sprintf(descbuf, "Screen '%s'", screen->tag);
	else
		strcpy(descbuf, "Screen");

	return descbuf;
}


/*-------------------------------------------------
    slider_crossscale - crosshair scale slider
    callback
-------------------------------------------------*/

#ifdef MAME_DEBUG
static INT32 slider_crossscale(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	input_field_config *field = arg;

	if (buffer != NULL)
	{
		field->crossscale = (float)newval * 0.001f;
		sprintf(buffer, "%s %s %1.3f", "Crosshair Scale", (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y", (float)newval * 0.001f);
	}
	return floor(field->crossscale * 1000.0f + 0.5f);
}
#endif


/*-------------------------------------------------
    slider_crossoffset - crosshair scale slider
    callback
-------------------------------------------------*/

#ifdef MAME_DEBUG
static INT32 slider_crossoffset(running_machine *machine, INT32 newval, char *buffer, void *arg)
{
	input_field_config *field = arg;

	if (buffer != NULL)
	{
		field->crossoffset = (float)newval * 0.001f;
		sprintf(buffer, "%s %s %1.3f", "Crosshair Offset", (field->crossaxis == CROSSHAIR_AXIS_X) ? "X" : "Y", (float)newval * 0.001f);
	}
	return field->crossoffset;
}
#endif
