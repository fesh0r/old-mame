

#define KC85_4_CLOCK 1750000
#define KC85_3_CLOCK 1750000

#define KC85_4_SCREEN_PIXEL_RAM_SIZE 0x04000
#define KC85_4_SCREEN_COLOUR_RAM_SIZE 0x04000

#define KC85_PALETTE_SIZE 24
#define KC85_SCREEN_WIDTH 320
#define KC85_SCREEN_HEIGHT 256

int	kc_quickload_load(int id);


void kc85_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);

void	kc85_video_set_blink_state(int data);

int kc85_4_vh_start(void);
void kc85_4_vh_stop(void);
void kc85_4_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void kc85_4_shutdown_machine(void);
void kc85_4_init_machine(void);

int	kc85_3_vh_start(void);
void kc85_3_vh_stop(void);
void kc85_3_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void kc85_3_shutdown_machine(void);
void kc85_3_init_machine(void);

void kc85_4d_init_machine(void);
void kc85_4d_shutdown_machine(void);

/* cassette */
int kc_cassette_device_init(int id);
void kc_cassette_device_exit(int id);

READ_HANDLER(kc85_4_84_r);
WRITE_HANDLER(kc85_4_84_w);

READ_HANDLER(kc85_4_86_r);
WRITE_HANDLER(kc85_4_86_w);

READ_HANDLER(kc85_unmapped_r);

READ_HANDLER(kc85_pio_data_r);

WRITE_HANDLER(kc85_module_w);

WRITE_HANDLER(kc85_4_pio_data_w);
WRITE_HANDLER(kc85_3_pio_data_w);

READ_HANDLER(kc85_pio_control_r);
WRITE_HANDLER(kc85_pio_control_w);

READ_HANDLER(kc85_ctc_r);
WRITE_HANDLER(kc85_ctc_w);

/* select video ram to display */
void kc85_4_video_ram_select_bank(int bank);
/* select video ram which is visible in address space */
unsigned char *kc85_4_get_video_ram_base(int bank, int colour);

/* this is a fake keyboard layout. 
The keys are converted into codes which are transmitted by the keyboard to the base-unit */
/* key code can be calculated as (line*8)+bit_index */


#define KC_KEYBOARD \
	/* start of keyboard scan-codes */ \
	/* codes 0-7 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "W", KEYCODE_W, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "A", KEYCODE_A, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "2", KEYCODE_2, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR LEFT", KEYCODE_LEFT, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "HOME", KEYCODE_HOME, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "-", KEYCODE_MINUS, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F2", KEYCODE_F2, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Y", KEYCODE_Y, IP_JOY_NONE) \
	/* codes 8-15 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "E", KEYCODE_E, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "S", KEYCODE_S, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "3", KEYCODE_3, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "^/ss", KEYCODE_NONE, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CLR", KEYCODE_BACKSPACE, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, ":",KEYCODE_COLON, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F3", KEYCODE_F3, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "X", KEYCODE_X, IP_JOY_NONE) \
	/* codes 16-23 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "T", KEYCODE_T, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F", KEYCODE_F, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "5", KEYCODE_5, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "P", KEYCODE_P, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "DEL", KEYCODE_DEL, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "0",KEYCODE_0, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F5", KEYCODE_F5, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "V", KEYCODE_V, IP_JOY_NONE) \
	/* codes 24-31 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "U", KEYCODE_U, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "H", KEYCODE_H, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "7", KEYCODE_7, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "O", KEYCODE_O, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "INS", KEYCODE_INSERT, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "9", KEYCODE_9, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "BRK", KEYCODE_ESC, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "N", KEYCODE_N, IP_JOY_NONE) \
	/* codes 32-39 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "I", KEYCODE_I, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "J", KEYCODE_J, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "8", KEYCODE_8, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SPACE", KEYCODE_SPACE, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "K", KEYCODE_K, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, ",", KEYCODE_COMMA, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "STOP", KEYCODE_END, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "M", KEYCODE_M, IP_JOY_NONE) \
	/* codes 40-47 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Z", KEYCODE_Z, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "G", KEYCODE_G, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "6", KEYCODE_6, IP_JOY_NONE) \
	PORT_BIT (0x08, 0x00, IPT_UNUSED) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "L", KEYCODE_L, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, ".", KEYCODE_STOP, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F6", KEYCODE_F6, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "B", KEYCODE_B, IP_JOY_NONE) \
	/* codes 48-56 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "R", KEYCODE_R, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "D", KEYCODE_D, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "4", KEYCODE_4, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "_", KEYCODE_MINUS_PAD, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "+", KEYCODE_EQUALS, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "/", KEYCODE_SLASH, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F4", KEYCODE_F4, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "C", KEYCODE_C, IP_JOY_NONE) \
	/* codes 56-63 */ \
	PORT_START \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "Q", KEYCODE_Q, IP_JOY_NONE) \
	PORT_BITX(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT LOCK", KEYCODE_CAPSLOCK, IP_JOY_NONE) \
	PORT_BITX(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD, "1", KEYCODE_1, IP_JOY_NONE) \
	PORT_BITX(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR DOWN", KEYCODE_DOWN, IP_JOY_NONE) \
	PORT_BITX(0x010, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR UP", KEYCODE_UP, IP_JOY_NONE) \
	PORT_BITX(0x020, IP_ACTIVE_HIGH, IPT_KEYBOARD, "CURSOR RIGHT", KEYCODE_RIGHT, IP_JOY_NONE) \
	PORT_BITX(0x040, IP_ACTIVE_HIGH, IPT_KEYBOARD, "F1", KEYCODE_F1, IP_JOY_NONE) \
	PORT_BITX(0x080, IP_ACTIVE_HIGH, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE) \
	/* end of keyboard scan-codes */ \
	PORT_START \
	/* has a single shift key. Mapped here to left and right shift. */ \
	/* shift is connected to the transmit chip inside the keyboard and affects bit 0 */ \
	/* of the scan-code sent directly */ \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE) \
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)


/*** MODULE SYSTEM ***/
/* read from xx80 port */
READ_HANDLER(kc85_module_r);
/* write to xx80 port */
WRITE_HANDLER(kc85_module_w);


/*** DISC INTERFACE **/
#include "includes/nec765.h"
#include "includes/basicdsk.h"

/* IO_FLOPPY device */

/* for IO_ device init */
int kc85_floppy_init(int id);

/* used to setup machine */

#define KC_DISC_INTERFACE_PORT_R \
	{0x0f0, 0x0f3, kc85_disc_interface_ram_r},

#define KC_DISC_INTERFACE_PORT_W \
	{0x0f0, 0x0f3, kc85_disc_interface_ram_w}, \
	{0x0f4, 0x0f4, kc85_disc_interface_latch_w},

#define KC_DISC_INTERFACE_CPU \
{ \
	CPU_Z80,  /* type */ \
	4000000, \
	readmem_kc85_disc_hw,		   /* MemoryReadAddress */ \
	writemem_kc85_disc_hw,		   /* MemoryWriteAddress */ \
	readport_kc85_disc_hw,		   /* IOReadPort */ \
	writeport_kc85_disc_hw,		   /* IOWritePort */ \
	0,		/* VBlank  Interrupt */ \
	0,		/* vblanks per frame */ \
	0, 0,	/* every scanline */ \
    0 \
}

#define KC_DISC_INTERFACE_ROM



/* these are internal to the disc interface */

/* disc hardware internal i/o */
READ_HANDLER(kc85_disk_hw_ctc_r);
/* disc hardware internal i/o */
WRITE_HANDLER(kc85_disk_hw_ctc_w);
/* 4-bit input latch: DMA Data Request, FDC Int, FDD Ready.. */
READ_HANDLER(kc85_disc_hw_input_gate_r);
/* output port to set NEC765 terminal count input */
WRITE_HANDLER(kc85_disc_hw_terminal_count_w);

/* these are used by the kc85 to control the disc interface */
/* xxf4 - latch used to reset cpu in disc interface */
WRITE_HANDLER(kc85_disc_interface_latch_w);
/* xxf0-xxf3 write to kc85 disc interface ram */
WRITE_HANDLER(kc85_disc_interface_ram_w);
/* xxf0-xxf3 read from kc85 disc interface ram */
READ_HANDLER(kc85_disc_interface_ram_r);




