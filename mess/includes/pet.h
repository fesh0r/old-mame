/***************************************************************************
	commodore pet series computer

    peter.trauner@jk.uni-linz.ac.at
***************************************************************************/
#ifndef __PET_H_
#define __PET_H_

#include "driver.h"

#include "praster.h"
#include "crtc6845.h"

//  commodore pet discrete video circuit

/* call to init videodriver */
extern void pet_init (UINT8 *memory);

#define pet_vh_start praster_vh_start
#define pet_vh_stop praster_vh_stop
#define pet_vh_screenrefresh praster_vh_screenrefresh
#define pet_raster_irq praster_raster_irq

/*#define crtc6845_update praster_2_update */
#define pet_videoram_w praster_2_videoram_w


#define KEY_ATSIGN (input_port_0_r(0)&0x8000)
#define KEY_1 (input_port_0_r(0)&0x4000)
#define KEY_2 (input_port_0_r(0)&0x2000)
#define KEY_3 (input_port_0_r(0)&0x1000)
#define KEY_4 (input_port_0_r(0)&0x800)
#define KEY_5 (input_port_0_r(0)&0x400)
#define KEY_6 (input_port_0_r(0)&0x200)
#define KEY_7 (input_port_0_r(0)&0x100)
#define KEY_8 (input_port_0_r(0)&0x80)
#define KEY_9 (input_port_0_r(0)&0x40)
#define KEY_0 (input_port_0_r(0)&0x20)
#define KEY_ARROW_LEFT (input_port_0_r(0)&0x10)
#define KEY_OPENBRACE (input_port_0_r(0)&8)
#define KEY_CLOSEBRACE (input_port_0_r(0)&4)
#define KEY_REVERSE (input_port_0_r(0)&2)
#define KEY_Q (input_port_0_r(0)&1)

#define KEY_W (input_port_1_r(0)&0x8000)
#define KEY_E (input_port_1_r(0)&0x4000)
#define KEY_R (input_port_1_r(0)&0x2000)
#define KEY_T (input_port_1_r(0)&0x1000)
#define KEY_Y (input_port_1_r(0)&0x800)
#define KEY_U (input_port_1_r(0)&0x400)
#define KEY_I (input_port_1_r(0)&0x200)
#define KEY_O (input_port_1_r(0)&0x100)
#define KEY_P (input_port_1_r(0)&0x80)
#define KEY_ARROW_UP (input_port_1_r(0)&0x40)
#define KEY_SMALLER (input_port_1_r(0)&0x20)
#define KEY_BIGGER (input_port_1_r(0)&0x10)
#define KEY_SHIFTLOCK (input_port_1_r(0)&8)
#define KEY_A (input_port_1_r(0)&4)
#define KEY_S (input_port_1_r(0)&2)
#define KEY_D (input_port_1_r(0)&1)

#define KEY_F (input_port_2_r(0)&0x8000)
#define KEY_G (input_port_2_r(0)&0x4000)
#define KEY_H (input_port_2_r(0)&0x2000)
#define KEY_J (input_port_2_r(0)&0x1000)
#define KEY_K (input_port_2_r(0)&0x800)
#define KEY_L (input_port_2_r(0)&0x400)
#define KEY_COLON (input_port_2_r(0)&0x200)
#define KEY_STOP (input_port_2_r(0)&0x100)
#define KEY_RETURN (input_port_2_r(0)&0x80)
#define KEY_LEFT_SHIFT ((input_port_2_r(0)&0x40)||KEY_SHIFTLOCK)
#define KEY_Z (input_port_2_r(0)&0x20)
#define KEY_X (input_port_2_r(0)&0x10)
#define KEY_C (input_port_2_r(0)&8)
#define KEY_V (input_port_2_r(0)&4)
#define KEY_B (input_port_2_r(0)&2)
#define KEY_N (input_port_2_r(0)&1)

#define KEY_M (input_port_3_r(0)&0x8000)
#define KEY_COMMA (input_port_3_r(0)&0x4000)
#define KEY_SEMICOLON (input_port_3_r(0)&0x2000)
#define KEY_QUESTIONMARK (input_port_3_r(0)&0x1000)
#define KEY_RIGHT_SHIFT ((input_port_3_r(0)&0x800)\
                        ||KEY_CURSOR_LEFT||KEY_CURSOR_UP)
#define KEY_SPACE (input_port_3_r(0)&0x400)
#define KEY_HOME (input_port_3_r(0)&0x200)
#define KEY_CURSOR_DOWN ((input_port_3_r(0)&0x100)||KEY_CURSOR_UP)
#define KEY_CURSOR_RIGHT ((input_port_3_r(0)&0x80)||KEY_CURSOR_LEFT)
#define KEY_PAD_DEL (input_port_3_r(0)&0x40)
#define KEY_PAD_7 (input_port_3_r(0)&0x20)
#define KEY_PAD_8 (input_port_3_r(0)&0x10)
#define KEY_PAD_9 (input_port_3_r(0)&8)
#define KEY_PAD_SLASH (input_port_3_r(0)&4)
#define KEY_PAD_4 (input_port_3_r(0)&2)
#define KEY_PAD_5 (input_port_3_r(0)&1)

#define KEY_PAD_6 (input_port_4_r(0)&0x8000)
#define KEY_PAD_ASTERIX (input_port_4_r(0)&0x4000)
#define KEY_PAD_1 (input_port_4_r(0)&0x2000)
#define KEY_PAD_2 (input_port_4_r(0)&0x1000)
#define KEY_PAD_3 (input_port_4_r(0)&0x800)
#define KEY_PAD_PLUS (input_port_4_r(0)&0x400)
#define KEY_PAD_0 (input_port_4_r(0)&0x200)
#define KEY_PAD_POINT (input_port_4_r(0)&0x100)
#define KEY_PAD_MINUS (input_port_4_r(0)&0x80)
#define KEY_PAD_EQUALS (input_port_4_r(0)&0x40)
#define KEY_CURSOR_UP (input_port_4_r(0)&0x20)
#define KEY_CURSOR_LEFT (input_port_4_r(0)&0x10)

/* business keyboard ************************************************/

#define KEY_B_ARROW_LEFT (input_port_0_r(0)&0x8000)
#define KEY_B_1 (input_port_0_r(0)&0x4000)
#define KEY_B_2 (input_port_0_r(0)&0x2000)
#define KEY_B_3 (input_port_0_r(0)&0x1000)
#define KEY_B_4 (input_port_0_r(0)&0x800)
#define KEY_B_5 (input_port_0_r(0)&0x400)
#define KEY_B_6 (input_port_0_r(0)&0x200)
#define KEY_B_7 (input_port_0_r(0)&0x100)
#define KEY_B_8 (input_port_0_r(0)&0x80)
#define KEY_B_9 (input_port_0_r(0)&0x40)
#define KEY_B_0 (input_port_0_r(0)&0x20)
#define KEY_B_COLON (input_port_0_r(0)&0x10)
#define KEY_B_MINUS (input_port_0_r(0)&8)
#define KEY_B_ARROW_UP (input_port_0_r(0)&4)
#define KEY_B_CURSOR_RIGHT ((input_port_0_r(0)&2)||KEY_B_CURSOR_LEFT)
#define KEY_B_STOP (input_port_0_r(0)&1)

#define KEY_B_TAB (input_port_1_r(0)&0x8000)
#define KEY_B_Q (input_port_1_r(0)&0x4000)
#define KEY_B_W (input_port_1_r(0)&0x2000)
#define KEY_B_E (input_port_1_r(0)&0x1000)
#define KEY_B_R (input_port_1_r(0)&0x800)
#define KEY_B_T (input_port_1_r(0)&0x400)
#define KEY_B_Y (input_port_1_r(0)&0x200)
#define KEY_B_U (input_port_1_r(0)&0x100)
#define KEY_B_I (input_port_1_r(0)&0x80)
#define KEY_B_O (input_port_1_r(0)&0x40)
#define KEY_B_P (input_port_1_r(0)&0x20)
#define KEY_B_OPENBRACE (input_port_1_r(0)&0x10)
#define KEY_B_BACKSLASH (input_port_1_r(0)&8)
#define KEY_B_CURSOR_DOWN ((input_port_1_r(0)&4)||KEY_B_CURSOR_UP)
#define KEY_B_DEL (input_port_1_r(0)&2)
#define KEY_B_ESCAPE (input_port_1_r(0)&1)

#define KEY_B_SHIFTLOCK (input_port_2_r(0)&0x8000)
#define KEY_B_A (input_port_2_r(0)&0x4000)
#define KEY_B_S (input_port_2_r(0)&0x2000)
#define KEY_B_D (input_port_2_r(0)&0x1000)
#define KEY_B_F (input_port_2_r(0)&0x800)
#define KEY_B_G (input_port_2_r(0)&0x400)
#define KEY_B_H (input_port_2_r(0)&0x200)
#define KEY_B_J (input_port_2_r(0)&0x100)
#define KEY_B_K (input_port_2_r(0)&0x80)
#define KEY_B_L (input_port_2_r(0)&0x40)
#define KEY_B_SEMICOLON (input_port_2_r(0)&0x20)
#define KEY_B_ATSIGN (input_port_2_r(0)&0x10)
#define KEY_B_CLOSEBRACE (input_port_2_r(0)&8)
#define KEY_B_RETURN (input_port_2_r(0)&4)
#define KEY_B_REVERSE (input_port_2_r(0)&2)
#define KEY_B_LEFT_SHIFT ((input_port_2_r(0)&1)||KEY_B_SHIFTLOCK)

#define KEY_B_Z (input_port_3_r(0)&0x8000)
#define KEY_B_X (input_port_3_r(0)&0x4000)
#define KEY_B_C (input_port_3_r(0)&0x2000)
#define KEY_B_V (input_port_3_r(0)&0x1000)
#define KEY_B_B (input_port_3_r(0)&0x800)
#define KEY_B_N (input_port_3_r(0)&0x400)
#define KEY_B_M (input_port_3_r(0)&0x200)
#define KEY_B_COMMA (input_port_3_r(0)&0x100)
#define KEY_B_POINT (input_port_3_r(0)&0x80)
#define KEY_B_SLASH (input_port_3_r(0)&0x40)
#define KEY_B_RIGHT_SHIFT ((input_port_3_r(0)&0x20)\
                        ||KEY_B_CURSOR_LEFT||KEY_B_CURSOR_UP)
#define KEY_B_REPEAT (input_port_3_r(0)&0x10)
#define KEY_B_HOME (input_port_3_r(0)&8)
#define KEY_B_SPACE (input_port_3_r(0)&4)
#define KEY_B_PAD_7 (input_port_3_r(0)&2)
#define KEY_B_PAD_8 (input_port_3_r(0)&1)

#define KEY_B_PAD_9 (input_port_4_r(0)&0x8000)
#define KEY_B_PAD_4 (input_port_4_r(0)&0x4000)
#define KEY_B_PAD_5 (input_port_4_r(0)&0x2000)
#define KEY_B_PAD_6 (input_port_4_r(0)&0x1000)
#define KEY_B_PAD_1 (input_port_4_r(0)&0x800)
#define KEY_B_PAD_2 (input_port_4_r(0)&0x400)
#define KEY_B_PAD_3 (input_port_4_r(0)&0x200)
#define KEY_B_PAD_0 (input_port_4_r(0)&0x100)
#define KEY_B_PAD_POINT (input_port_4_r(0)&0x80)
#define KEY_B_CURSOR_UP (input_port_4_r(0)&0x40)
#define KEY_B_CURSOR_LEFT (input_port_4_r(0)&0x20)

#define QUICKLOAD		(input_port_5_r(0)&0x8000)
#define DATASSETTE (input_port_5_r(0)&0x4000)
#define DATASSETTE_TONE (input_port_5_r(0)&0x2000)

#define DATASSETTE_PLAY		(input_port_5_r(0)&0x1000)
#define DATASSETTE_RECORD	(input_port_5_r(0)&0x800)
#define DATASSETTE_STOP		(input_port_5_r(0)&0x400)

#define BUSINESS_KEYBOARD (input_port_5_r(0)&0x200)

#define MEMORY (input_port_5_r(0)&0x180)
#define MEMORY_4 0
#define MEMORY_8 0x80
#define MEMORY_16 0x100
#define MEMORY_32 0x180

#define CBM8096_MEMORY (input_port_5_r(0)&8)
#define M6809_SELECT (input_port_5_r(0)&4)
#define IEEE8ON (input_port_5_r(0)&2)
#define IEEE9ON (input_port_5_r(0)&1)

extern UINT8 *pet_memory;
extern UINT8 *pet_videoram;
extern UINT8 *superpet_memory;

WRITE_HANDLER(cbm8096_w);
extern READ_HANDLER(superpet_r);
extern WRITE_HANDLER(superpet_w);

extern void pet_driver_init (void);
extern void pet_basic1_driver_init (void);
extern void pet40_driver_init (void);
extern void cbm80_driver_init (void);
extern void superpet_driver_init(void);
extern void pet_driver_shutdown (void);
extern void pet_init_machine (void);
extern void pet_shutdown_machine (void);
extern void pet_frame_interrupt (int param);

int pet_rom_id (int id);
void pet_rom_load(void);

void pet_state(PRASTER *This);

#endif
