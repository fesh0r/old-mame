#include "sound/custom.h"

/* flags for init_pc_common */
#define PCCOMMON_KEYBOARD_PC	0
#define PCCOMMON_KEYBOARD_AT	1

void init_pc_common(UINT32 flags);

void pc_keyboard(void);
UINT8 pc_keyb_read(void);
void pc_keyb_set_clock(int on);
void pc_keyb_clear(void);

