/*********************************************************************

	uimess.h

	MESS supplement to ui.c.

*********************************************************************/

#ifndef __UIMESS_H__
#define __UIMESS_H__

#include "ui.h"
#include "uimenu.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct _ui_mess_private;
typedef struct _ui_mess_private ui_mess_private;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/* initialize the MESS-specific	UI */
void ui_mess_init(running_machine *machine);

/* MESS-specific in-"game" UI */
int ui_mess_handler_ingame(running_machine *machine);

/* returns whether the "new UI" is active or not */
int ui_mess_use_new_ui(running_machine *machine);

/* image info screen */
void ui_menu_image_info(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* file manager */
void menu_file_manager(running_machine *machine, ui_menu *menu, void *parameter, void *state);

/* tape control */
void menu_tape_control(running_machine *machine, ui_menu *menu, void *parameter, void *state);
astring *tapecontrol_gettime(astring *dest, const device_config *device, int *curpos, int *endpos);

/* paste */
void ui_mess_paste(running_machine *machine);

/* returns whether IPT_KEYBOARD input should be disabled */
int ui_mess_keyboard_disabled(running_machine *machine);

/* returns whether the natural keyboard is active */
int ui_mess_get_use_natural_keyboard(running_machine *machine);

/* specifies whether the natural keyboard is active */
void ui_mess_set_use_natural_keyboard(running_machine *machine, int use_natural_keyboard);

#endif /* __UIMESS_H__ */
