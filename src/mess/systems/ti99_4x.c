/*
  MESS Driver for TI-99/4 and TI-99/4A Home Computers.
  Raphael Nabet, 1999.

  see machine/ti99_4x.c for some details and references

  NOTE!!!!!!!!!!  Until the new TMS5220 drivers are added, you should uncomment two lines
  in tms5220interface like this :

  static struct TMS5220interface tms5220interface =
  {
    640000L,   640kHz -> 8kHz output
    10000,     Volume.  I don't know the best value.
    NULL,      no IRQ callback
    //2,         memory region where the speech ROM is.  -1 means no speech ROM
    //0          memory size of speech rom (0 -> take memory region length)
  };
*/

/*
  TI99/4 preliminary infos :

References :
  * ...

Similar to TI99/4a, except for the following :
  * tms9918/9928 has no bitmap mode
  * smaller, 40-key keyboard
  * system ROMs are 5kb larger (I guess it has an extra GROM).  There are many small differences in
    the contents.

Historical notes : TI made several last minute design changes.
  * TI99/4 prototypes had an extra port for an I/R joystick and keypad interface.
  * early TI99/4 prototypes were designed for a tms9985, not a tms9900.
*/

#include "driver.h"
#include "mess/vidhrdw/tms9928a.h"


/* protos for support code in machine/ti99.c */

void ti99_init_machine(void);
void ti99_stop_machine(void);

int ti99_load_rom (void);
int ti99_id_rom (const char *name, const char *gamename);
int ti99_vh_start(void);
int ti99_vblank_interrupt(void);

int ti99_rw_null8bits(int offset);
void ti99_ww_null8bits(int offset, int data);

int ti99_rw_xramlow(int offset);
void ti99_ww_xramlow(int offset, int data);
int ti99_rw_xramhigh(int offset);
void ti99_ww_xramhigh(int offset, int data);

int ti99_rw_cartmem(int offset);
void ti99_ww_cartmem(int offset, int data);

int ti99_rw_scratchpad(int offset);
void ti99_ww_scratchpad(int offset, int data);

void ti99_ww_wsnd(int offset, int data);
int ti99_rw_rvdp(int offset);
void ti99_ww_wvdp(int offset, int data);
int ti99_rw_rspeech(int offset);
void ti99_ww_wspeech(int offset, int data);
int ti99_rw_rgpl(int addr);
void ti99_ww_wgpl(int offset, int data);

int ti99_R9901_0(int offset);
int ti99_R9901_1(int offset);
void ti99_W9901_0(int offset, int data);
void ti99_W9901_S(int offset, int data);
void ti99_W9901_F(int offset, int data);

int ti99_R9901_2(int offset);
int ti99_R9901_3(int offset);
void ti99_KeyC2(int offset, int data);
void ti99_KeyC1(int offset, int data);
void ti99_KeyC0(int offset, int data);
void ti99_AlphaW(int offset, int data);
void ti99_CS1_motor(int offset, int data);
void ti99_CS2_motor(int offset, int data);
void ti99_audio_gate(int offset, int data);
void ti99_CS_output(int offset, int data);

int ti99_rw_disk(int offset);
void ti99_ww_disk(int offset, int data);
int ti99_DSKget(int offset);
void ti99_DSKROM(int offset, int data);
void ti99_DSKhold(int offset, int data);
void ti99_DSKheads(int offset, int data);
void ti99_DSKsel(int offset, int data);
void ti99_DSKside(int offset, int data);


/*
  memory map
*/

static struct MemoryReadAddress readmem[] =
{
  { 0x0000, 0x1fff, MRA_ROM },          /*system ROM*/
  { 0x2000, 0x3fff, ti99_rw_xramlow },  /*lower 8kb of RAM extension*/
  { 0x4000, 0x5fff, ti99_rw_disk },     /*DSR ROM... only disk is emulated */
  { 0x6000, 0x7fff, ti99_rw_cartmem },  /*cartidge memory... some RAM is actually possible*/
  { 0x8000, 0x82ff, ti99_rw_scratchpad },   /*RAM PAD, mapped to 0x8300-0x83ff*/
  { 0x8300, 0x83ff, MRA_RAM },          /*RAM PAD*/
  { 0x8400, 0x87ff, ti99_rw_null8bits },/*soundchip write*/
  { 0x8800, 0x8bff, ti99_rw_rvdp },     /*vdp read*/
  { 0x8C00, 0x8fff, ti99_rw_null8bits },/*vdp write*/
  { 0x9000, 0x93ff, ti99_rw_rspeech },  /*speech read*/
  { 0x9400, 0x97ff, ti99_rw_null8bits },/*speech write*/
  { 0x9800, 0x9bff, ti99_rw_rgpl },     /*GPL read*/
  { 0x9c00, 0x9fff, ti99_rw_null8bits },/*GPL write*/
  { 0xa000, 0xffff, ti99_rw_xramhigh }, /*upper 24kb of RAM extension*/
  { -1 }    /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
  { 0x0000, 0x1fff, MWA_ROM },          /*system ROM*/
  { 0x2000, 0x3fff, ti99_ww_xramlow },  /*lower 8kb of memory expansion card*/
  { 0x4000, 0x5fff, ti99_ww_disk },     /*DSR ROM... only disk is emulated ! */
  { 0x6000, 0x7fff, ti99_ww_cartmem },  /*cartidge memory... some RAM or paging system is possible*/
  { 0x8000, 0x82ff, ti99_ww_scratchpad },   /*RAM PAD, mapped to 0x8300-0x83ff*/
  { 0x8300, 0x83ff, MWA_RAM },          /*RAM PAD*/
  { 0x8400, 0x87ff, ti99_ww_wsnd },     /*soundchip write*/
  { 0x8800, 0x8bff, ti99_ww_null8bits },/*vdp read*/
  { 0x8C00, 0x8fff, ti99_ww_wvdp },     /*vdp write*/
  { 0x9000, 0x93ff, ti99_ww_null8bits },/*speech read*/
  { 0x9400, 0x97ff, ti99_ww_wspeech },  /*speech write*/
  { 0x9800, 0x9bff, ti99_ww_null8bits },/*GPL read*/
  { 0x9c00, 0x9fff, ti99_ww_wgpl },     /*GPL write*/
  { 0xa000, 0xffff, ti99_ww_xramhigh }, /*upper 24kb of RAM extension*/
  { -1 }    /* end of table */
};


/*
  CRU map
*/

static struct IOWritePort writeport[] =
{
  {0x0000, 0x0000, ti99_W9901_0},
  {0x0001, 0x000e, ti99_W9901_S},
  {0x000f, 0x000f, ti99_W9901_F},

  {0x0012, 0x0012, ti99_KeyC2},
  {0x0013, 0x0013, ti99_KeyC1},
  {0x0014, 0x0014, ti99_KeyC0},
  {0x0015, 0x0015, ti99_AlphaW},
  {0x0016, 0x0016, ti99_CS1_motor},
  {0x0017, 0x0017, ti99_CS2_motor},
  {0x0018, 0x0018, ti99_audio_gate},
  {0x0019, 0x0019, ti99_CS_output},

  /*{0x0F01, 0x0F0e, ti99_W9901_S},*/ /* this mirror is used by a system routine */

  {0x0880, 0x0880, ti99_DSKROM},
  /*{0x0881, 0x0881, ti99_DSKmotor},*/
  {0x0882, 0x0882, ti99_DSKhold},
  {0x0883, 0x0883, ti99_DSKheads},
  {0x0884, 0x0886, ti99_DSKsel},
  {0x0887, 0x0887, ti99_DSKside},
  { -1 }    /* end of table */
};

static struct IOReadPort readport[] =
{
  {0x0000, 0x0000, ti99_R9901_0},
  {0x0001, 0x0001, ti99_R9901_1},

  {0x0002, 0x0002, ti99_R9901_2},
  {0x0003, 0x0003, ti99_R9901_3},

  {0x0110, 0x0110, ti99_DSKget},

  { -1 }    /* end of table */
};


/*
  Input ports, used by machine code for TI keyboard and joystick emulation.
*/

/* TI99/4a : 48-key keyboard, plus two optional joysticks (2 shift keys) */
INPUT_PORTS_START(ti99_4a)

  PORT_START    /* col 0 */
    PORT_BITX(0x88, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "CTRL", KEYCODE_LCONTROL, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
    /* TI99/4a has a second shift key which maps the same */
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_RSHIFT, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "FCTN", KEYCODE_LALT, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "= + QUIT", KEYCODE_EQUALS, IP_JOY_NONE)

  PORT_START    /* col 1 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

  PORT_START    /* col 2 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 # ERASE", KEYCODE_3, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ", <", KEYCODE_COMMA, IP_JOY_NONE)

  PORT_START    /* col 3 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)

  PORT_START    /* col 4 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)

  PORT_START    /* col 5 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A (incomplete vertical bar)", KEYCODE_A, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "; :", KEYCODE_COLON, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "/ -", KEYCODE_SLASH, IP_JOY_NONE)

  PORT_START    /* col 6 : "wired handset 1" (= joystick 1) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

  PORT_START    /* col 7 : "wired handset 2" (= joystick 2) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

  PORT_START    /* one more port for Alpha line */
    PORT_BITX(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD | IPF_TOGGLE, "Alpha Lock", KEYCODE_CAPSLOCK, IP_JOY_NONE)

INPUT_PORTS_END


/* TI99/4 : 41-key keyboard, plus two optional joysticks  (2 space keys) */
/* I *ASSUMED* the map was a subset of the ti99/4a */
INPUT_PORTS_START(ti99_4)

  PORT_START    /* col 0 */
    PORT_BITX(0xD9, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(SPACE)", KEYCODE_SPACE, IP_JOY_NONE)

  PORT_START    /* col 1 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "x X (DOWN)", KEYCODE_X, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "w W ~", KEYCODE_W, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "s S (LEFT)", KEYCODE_S, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "2 @ INS", KEYCODE_2, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "9 ( BACK", KEYCODE_9, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "o O '", KEYCODE_O, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "l L", KEYCODE_L, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ". >", KEYCODE_STOP, IP_JOY_NONE)

  PORT_START    /* col 2 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "c C `", KEYCODE_C, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "e E (UP)", KEYCODE_E, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "d D (RIGHT)", KEYCODE_D, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "3 # ERASE", KEYCODE_3, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "8 * REDO", KEYCODE_8, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "i I ?", KEYCODE_I, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "k K", KEYCODE_K, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)

  PORT_START    /* col 3 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "v V", KEYCODE_V, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "r R [", KEYCODE_R, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "f F {", KEYCODE_F, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4 $ CLEAR", KEYCODE_4, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7 & AID", KEYCODE_7, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "u U _", KEYCODE_U, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "j J", KEYCODE_J, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "m M", KEYCODE_M, IP_JOY_NONE)

  PORT_START    /* col 4 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "b B", KEYCODE_B, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "t T ]", KEYCODE_T, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "g G }", KEYCODE_G, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5 % BEGIN", KEYCODE_5, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "6 ^ PROC'D", KEYCODE_6, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "y Y", KEYCODE_Y, IP_JOY_NONE)
    PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "h H", KEYCODE_H, IP_JOY_NONE)
    PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "n N", KEYCODE_N, IP_JOY_NONE)

  PORT_START    /* col 5 */
    PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "z Z \\", KEYCODE_Z, IP_JOY_NONE)
    PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "q Q", KEYCODE_Q, IP_JOY_NONE)
    PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "a A (incomplete vertical bar)", KEYCODE_A, IP_JOY_NONE)
    PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "1 ! DEL", KEYCODE_1, IP_JOY_NONE)
    PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "0 )", KEYCODE_0, IP_JOY_NONE)
    PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "p P \"", KEYCODE_P, IP_JOY_NONE)
    PORT_BITX(0x03, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)

  PORT_START    /* col 6 : "wired handset 1" (= joystick 1) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER1/*, "(1UP)", IP_KEY_NONE, OSD_JOY_UP*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER1/*, "(1DOWN)", IP_KEY_NONE, OSD_JOY_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER1/*, "(1RIGHT)", IP_KEY_NONE, OSD_JOY_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER1/*, "(1LEFT)", IP_KEY_NONE, OSD_JOY_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1/*, "(1FIRE)", IP_KEY_NONE, OSD_JOY_FIRE, 0*/)

  PORT_START    /* col 7 : "wired handset 2" (= joystick 2) */
    PORT_BITX(0xE0, IP_ACTIVE_LOW, IPT_UNUSED, "unused", IP_KEY_NONE, IP_JOY_NONE)
    PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_PLAYER2/*, "(2UP)", IP_KEY_NONE, OSD_JOY2_UP, 0*/)
    PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_PLAYER2/*, "(2DOWN)", IP_KEY_NONE, OSD_JOY2_DOWN, 0*/)
    PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2/*, "(2RIGHT)", IP_KEY_NONE, OSD_JOY2_RIGHT, 0*/)
    PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2/*, "(2LEFT)", IP_KEY_NONE, OSD_JOY2_LEFT, 0*/)
    PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2/*, "(2FIRE)", IP_KEY_NONE, OSD_JOY2_FIRE, 0*/)

INPUT_PORTS_END


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 }    /* end of array */
};

#if 1
/*
	Use normal palette.
*/



#else

/*
	My palette.  Nicer than the default TMS9928A_palette, but does not work quite well...
*/
static unsigned char TMS9928A_palette[] =
{
  0, 0, 0,
  0, 0, 0,
  64, 179, 64,
  96, 192, 96,
  64, 64, 192,
  96, 96, 244,
  192, 64, 64,
  64, 244, 244,
  244, 64, 64,
  255, 128, 64,
  224, 192, 64,
  255, 224, 64,
  64, 128, 64,
  192, 64, 192,
  224, 224, 224,
  255, 255, 255
};
/*
Color           Y	R-Y	B-Y	R	G	B
0 Transparent
1 Black         0.00	0.47	0.47	0.00	0.00	0.00
2 Medium green  0.53	0.07	0.20	0.13	0.53	0.26
3 Light green   0.67	0.17	0.27	0.37	0.67	0.47
4 Dark blue     0.40	0.40	1.00	0.33	0.40	0.93
5 Light blue    0.53	0.43	0.93	0.49	0.53	0.99
6 Dark red      0.47	0.83	0.30	0.89	0.47	0.30
7 Cyan          0.73	0.00	0.70	0.26	0.73	0.96
8 Medium red    0.53	0.93	0.27
9 Light red     0.67	0.93	0.27
A Dark yellow   0.73	0.57	0.07
B Light yellow  0.80	0.57	0.17
C Dark green    0.47	0.13	0.23
D Magenta       0.53	0.73	0.67
E Gray          0.80	0.47	0.47
F White         1.00	0.47	0.47
*/
static unsigned short TMS9928A_colortable[] =
{
  0, 1,
  0, 2,
  0, 3,
  0, 4,
  0, 5,
  0, 6,
  0, 7,
  0, 8,
  0, 9,
  0,10,
  0,11,
  0,12,
  0,13,
  0,14,
  0,15
};

static void tms9928A_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *)
{
	memcpy(palette, & TMS9928A_palette, sizeof(TMS9928A_palette));
	memcpy(colortable, & TMS9928A_colortable, sizeof(TMS9928A_colortable));
}

#endif


/*
  TMS9919 sound chip parameters.
*/
static struct SN76496interface tms9919interface =
{
	1,            /* one sound chip */
	{ 3579545 },  /* clock speed. connected to the TMS9918A CPUCLK pin... */
	{ 10000 }     /* Volume.  I don't know the best value. */
};

static struct TMS5220interface tms5220interface =
{
	640000L,      /* 640kHz -> 8kHz output */
	10000,        /* Volume.  I don't know the best value. */
	NULL,         /* no IRQ callback */
	//REGION_GFX2,  /* memory region where the speech ROM is.  -1 means no speech ROM */
	//0             /* memory size of speech rom (0 -> take memory region length) */
};

/*
  we use 2 DACs to emulate "audio gate" and tape output, even thought
  a) there was no DAC in an actual TI99
  b) these are 2-level output (whereas a DAC provides 256-level output...)
*/
static struct DACinterface aux_sound_intf =
{
  2,  /* total number of DACs */
  {
    10000,  /* volume for tape output */
    10000   /* volume for audio gate*/
  }
};





/*
  machine description.
*/
static struct MachineDriver machine_driver_ti99_4 =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,    /* 3.0 Mhz*/
			readmem, writemem, readport, writeport,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,                    /* screen width */
	192,                    /* screen height */
	{ 0, 256-1, 0, 192-1},  /* visible_area */
	gfxdecodeinfo,          /* graphics decode info (???)*/
	TMS9928A_PALETTE_SIZE,  /* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE, /* length in shorts of the color lookup table */
	tms9928A_init_palette,  /* palette init */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ti99_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		}
	}
};


/*
  ROM loading
*/
ROM_START(ti99_4)
	/*CPU memory space*/
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD_WIDE("994rom.bin", 0x0000, 0x2000, 0x00000000)      /* system ROMs */
	ROM_LOAD_WIDE("disk.bin", 0x4000, 0x2000, 0x00000000)        /* disk DSR ROM */

	/*GPL memory space*/
	ROM_REGIONX(0x10000,REGION_GFX1)
	ROM_LOAD("994grom.bin", 0x0000, 0x8000, 0x00000000)     /* system GROMs */

	/*TMS5220 ROM space*/
	ROM_REGIONX(0x8000,REGION_GFX2)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x00000000)     /* system speech ROM */
ROM_END

struct GameDriver ti99_4_driver =
{
	__FILE__,
	0,
	"ti99_4",
	"TI99/4 Home Computer",
	"1978 (prototypes), oct. 1979-1981",
	"Texas Instrument",
	"R Nabet, based on Ed Swartz's V9T9.",
	0,
	&machine_driver_ti99_4,
	0,  /* optional function to be called during initialization */

	rom_ti99_4,
	ti99_load_rom,  /* load rom_file images */
	ti99_id_rom,    /* identify rom images */
	0,
	3,              /* number of ROM slots */
	/* a TI99 console only had one cartidge slot, but cutting the ROMs in 3 files seems to be
	the only way to handle cartidge until I use a header format.
	Note that there sometimes was a speech ROM slot in the speech synthesizer, and you could plug
	up to 16 additonnal DSR roms and quite a lot of GROMs in the side port.  None of these is
	emulated. */
	3,            /* number of floppy drives supported */
	0,            /* number of hard drives supported */
	0/*2*/,       /* number of cassette drives supported */
	0,            /* rom decoder */
	0,            /* opcode decoder */
	0,            /* pointer to sample names */
	0,            /* sound_prom */

	input_ports_ti99_4,

	0,            /* color_prom */
	0,            /* color palette - obsolete */
	0,            /* color lookup table - obsolete */

	GAME_COMPUTER | ORIENTATION_DEFAULT,  /* orientation */

	0,            /* hiscore load */
	0,            /* hiscore save */
};



/*
  machine description.
*/
static struct MachineDriver machine_driver_ti99_4a =
{
	/* basic machine hardware */
	{
		{
			CPU_TMS9900,
			3000000,    /* 3.0 Mhz*/
			readmem, writemem, readport, writeport,
			ti99_vblank_interrupt, 1,
			0, 0,
			0
		},
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,
	ti99_init_machine,
	ti99_stop_machine,

	/* video hardware */
	256,                    /* screen width */
	192,                    /* screen height */
	{ 0, 256-1, 0, 192-1},  /* visible_area */
	gfxdecodeinfo,          /* graphics decode info (???)*/
	TMS9928A_PALETTE_SIZE,  /* palette is 3*total_colors bytes long */
	TMS9928A_COLORTABLE_SIZE, /* length in shorts of the color lookup table */
	tms9928A_init_palette,  /* palette init */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	ti99_vh_start,
	TMS9928A_stop,
	TMS9928A_refresh,

	/* sound hardware */
	0,
	0,0,0,
	{
		{
			SOUND_SN76496,
			&tms9919interface
		},
		{
			SOUND_TMS5220,
			&tms5220interface
		},
		{
			SOUND_DAC,
			&aux_sound_intf
		}
	}
};


/*
  ROM loading
*/
ROM_START(ti99_4a)
	/*CPU memory space*/
	ROM_REGIONX(0x10000,REGION_CPU1)
	ROM_LOAD_WIDE("994arom.bin", 0x0000, 0x2000, 0xdb8f33e5)     /* system ROMs */
	ROM_LOAD_WIDE("disk.bin", 0x4000, 0x2000, 0x8f7df93f)        /* disk DSR ROM */

	/*GPL memory space*/
	ROM_REGIONX(0x10000,REGION_GFX1)
	ROM_LOAD("994agrom.bin", 0x0000, 0x6000, 0xaf5c2449)    /* system GROMs */

	/*TMS5220 ROM space*/
	ROM_REGIONX(0x8000,REGION_GFX2)
	ROM_LOAD("spchrom.bin", 0x0000, 0x8000, 0x58b155f7)     /* system speech ROM */
ROM_END

struct GameDriver ti99_4a_driver =
{
	__FILE__,
	0,
	"ti99_4a",
	"TI99/4A Home Computer",
	"1981-1983",
	"Texas Instrument",
	"R Nabet, based on Ed Swartz's V9T9.",
	0,
	&machine_driver_ti99_4a,
	0,  /* optional function to be called during initialization */

	rom_ti99_4a,
	ti99_load_rom,  /* load rom_file images */
	ti99_id_rom,    /* identify rom images */
	0,
	3,              /* number of ROM slots */
	/* a TI99 console only had one cartidge slot, but cutting the ROMs in 3 files seems to be
	the only way to handle cartidge until I use a header format.
	Note that there sometimes was a speech ROM slot in the speech synthesizer, and you could plug
	up to 16 additonnal DSR roms and quite a lot of GROMs in the side port.  None of these is
	emulated. */
	3,            /* number of floppy drives supported */
	0,            /* number of hard drives supported */
	0/*2*/,       /* number of cassette drives supported */
	0,            /* rom decoder */
	0,            /* opcode decoder */
	0,            /* pointer to sample names */
	0,            /* sound_prom */

	input_ports_ti99_4a,

	0,            /* color_prom */
	0,            /* color palette - obsolete */
	0,            /* color lookup table - obsolete */

	GAME_COMPUTER | ORIENTATION_DEFAULT,  /* orientation */

	0,            /* hiscore load */
	0,            /* hiscore save */
};