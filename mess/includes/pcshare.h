#include "driver.h"

struct _PC_SETUP;
extern struct _PC_SETUP pc_setup[], pc_setup_pc1512[], pc_setup_pc1640[], 
	pc_setup_europc[], pc_setup_t1000hx[];
extern struct _PC_SETUP pc_setup_at[];
void pc_init_setup(struct _PC_SETUP *setup);

void init_pc_common(void);
void pc_cga_init(void);
void pc_mda_init(void);
void pc_vga_init(void);

void pc_keyboard(void);
UINT8 pc_keyb_read(void);
void pc_keyb_set_clock(int on);

extern WRITE_HANDLER ( pc_COM1_w );
extern READ_HANDLER ( pc_COM1_r );
extern WRITE_HANDLER ( pc_COM2_w );
extern READ_HANDLER ( pc_COM2_r );
extern WRITE_HANDLER ( pc_COM3_w );
extern READ_HANDLER ( pc_COM3_r );
extern WRITE_HANDLER ( pc_COM4_w );
extern READ_HANDLER ( pc_COM4_r );

/* from sndhrdw/pc.c */
extern struct CustomSound_interface pc_sound_interface;
extern int  pc_sh_init(const char *name);
extern int  pc_sh_custom_start(const struct MachineSound *driver);
extern void pc_sh_custom_update(void);
extern void pc_sh_stop(void);
extern void pc_sh_update(int param, INT16 *buff, int length);
extern void pc_sh_speaker(int mode);
void pc_sh_speaker_change_clock(double pc_clock);

/* from machine/pc_fdc.c */
void pc_fdc_setup(void);

extern WRITE_HANDLER ( pc_JOY_w );
extern READ_HANDLER ( pc_JOY_r );

#define PC_JOYSTICK \
	PORT_START	/* IN15 */\
	PORT_BIT ( 0xf, 0xf,	 IPT_UNUSED ) \
	PORT_BITX( 0x0010, 0x0000, IPT_BUTTON1,	"Joystick 1 Button 1", CODE_DEFAULT, CODE_NONE)\
	PORT_BITX( 0x0020, 0x0000, IPT_BUTTON2,	"Joystick 1 Button 2", CODE_DEFAULT, CODE_NONE)\
	PORT_BITX( 0x0040, 0x0000, IPT_BUTTON1|IPF_PLAYER2,	"Joystick 2 Button 1", CODE_NONE, JOYCODE_2_BUTTON1)\
	PORT_BITX( 0x0080, 0x0000, IPT_BUTTON2|IPF_PLAYER2,	"Joystick 2 Button 2", CODE_NONE, JOYCODE_2_BUTTON2)\
		\
	PORT_START	/* IN16 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,KEYCODE_LEFT,KEYCODE_RIGHT,JOYCODE_1_LEFT,JOYCODE_1_RIGHT)\
		\
	PORT_START /* IN17 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE,100,1,1,0xff,KEYCODE_UP,KEYCODE_DOWN,JOYCODE_1_UP,JOYCODE_1_DOWN)\
		\
	PORT_START	/* IN18 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_X|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_LEFT,JOYCODE_2_RIGHT)\
		\
	PORT_START /* IN19 */\
	PORT_ANALOGX(0xff,0x80,IPT_AD_STICK_Y|IPF_CENTER|IPF_REVERSE|IPF_PLAYER2,100,1,1,0xff,CODE_NONE,CODE_NONE,JOYCODE_2_UP,JOYCODE_2_DOWN)
