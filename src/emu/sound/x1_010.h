#pragma once

#ifndef __X1_010_H__
#define __X1_010_H__


typedef struct _x1_010_interface x1_010_interface;
struct _x1_010_interface
{
	int adr;	/* address */
};


READ8_HANDLER ( seta_sound_r );
WRITE8_HANDLER( seta_sound_w );

READ16_HANDLER ( seta_sound_word_r );
WRITE16_HANDLER( seta_sound_word_w );

void seta_sound_enable_w(int);

#endif /* __X1_010_H__ */
