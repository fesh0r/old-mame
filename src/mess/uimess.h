/*********************************************************************

	uimess.h

	MESS supplement to ui.c.

*********************************************************************/

#ifndef UIMESS_H
#define UIMESS_H

#include "ui.h"

int mess_ui_active(void);
void mess_ui_update(running_machine *machine);
int mess_use_new_ui(void);
int mess_disable_builtin_ui(running_machine *machine);

/* image info screen */
int ui_sprintf_image_info(running_machine *machine, char *buf);
UINT32 ui_menu_image_info(UINT32 state);

/* file manager */
int filemanager(int selected);

/* tape control */
int tapecontrol(int selected);

/* paste */
void ui_paste(running_machine *machine);

#endif /* UIMESS_H */
