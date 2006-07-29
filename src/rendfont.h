/***************************************************************************

    rendfont.h

    Rendering system font management.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef RENDFONT_H
#define RENDFONT_H

#include "render.h"



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

render_font *render_font_alloc(const char *filename);
void render_font_free(render_font *font);
render_texture *render_font_get_char_texture_and_bounds(render_font *font, float height, float aspect, UINT16 ch, render_bounds *bounds);
float render_font_get_char_width(render_font *font, float height, float aspect, UINT16 ch);
float render_font_get_string_width(render_font *font, float height, float aspect, const char *string);
float render_font_get_wstring_width(render_font *font, float height, float aspect, const UINT16 *wstring);

#endif	/* RENDFONT_H */
