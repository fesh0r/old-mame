/******************************************************************************
 *  Microtan 65
 *
 *  system driver
 *
 *  Juergen Buchmueller <pullmoll@t-online.de>, Jul 2000
 *
 *  Thanks go to Geoff Macdonald <mail@geoff.org.uk>
 *  for his site http:://www.geo255.redhotant.com
 *  and to Fabrice Frances <frances@ensica.fr>
 *  for his site http://www.ifrance.com/oric/microtan.html
 *
 *  Microtan65 memory map
 *
 *  range     short     description
 *  0000-01FF SYSRAM    system ram
 *                      0000-003f variables
 *                      0040-00ff basic
 *                      0100-01ff stack
 *  0200-03ff VIDEORAM  display
 *  0400-afff RAM       main memory
 *  bc00-bc01 AY8912-0  sound chip #0
 *  bc02-bc03 AY8912-1  sound chip #1
 *  bc04      SPACEINV  space invasion sound (?)
 *  bfc0-bfcf VIA6522-0 VIA 6522 #0
 *  bfd0-bfd3 SIO       serial i/o
 *  bfe0-bfef VIA6522-1 VIA 6522 #0
 *  bff0      GFX_KBD   R: chunky graphics on W: reset KBD interrupt
 *  bff1      NMI       W: start delayed NMI
 *  bff2      HEX       W: hex. keypad column
 *  bff3      KBD_GFX   R: ASCII KBD / hex. keypad row W: chunky graphics off
 *  c000-e7ff BASIC     BASIC Interpreter ROM
 *  f000-f7ff XBUG      XBUG ROM
 *  f800-ffff TANBUG    TANBUG ROM
 *
 *****************************************************************************/

#include "includes/microtan.h"

#ifndef VERBOSE
#define VERBOSE 0
#endif

#if VERBOSE
#define LOG(x)  logerror x
#else
#define LOG(x)  /* x */
#endif

static MEMORY_READ_START( readmem_mtan )
    { 0x0000, 0x03ff, MRA_RAM },
/*  { 0x0400, 0x1fff, MRA_RAM },    */  /* TANEX 7K RAM */
/*  { 0x2000, 0xbbff, MRA_RAM },    */  /* TANRAM 40K RAM */
    { 0xbc00, 0xbc00, MRA_NOP },
    { 0xbc01, 0xbc01, AY8910_read_port_0_r },
    { 0xbc02, 0xbc02, MRA_NOP },
    { 0xbc03, 0xbc03, AY8910_read_port_1_r },
    { 0xbfc0, 0xbfcf, microtan_via_0_r },
    { 0xbfd0, 0xbfd3, microtan_sio_r },
    { 0xbfe0, 0xbfef, microtan_via_1_r },
    { 0xbff0, 0xbfff, microtan_bffx_r },
    { 0xc000, 0xe7ff, MRA_ROM },
    { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( writemem_mtan )
    { 0x0000, 0x01ff, MWA_RAM },
    { 0x0200, 0x03ff, microtan_videoram_w, &videoram, &videoram_size },
/*  { 0x0400, 0x1fff, MRA_RAM },    */  /* TANEX 7K RAM */
/*  { 0x2000, 0xbbff, MRA_RAM },    */  /* TANRAM 40K RAM */
    { 0xbc00, 0xbc00, AY8910_control_port_0_w },
    { 0xbc01, 0xbc01, AY8910_write_port_0_w },
    { 0xbc02, 0xbc02, AY8910_control_port_1_w },
    { 0xbc03, 0xbc03, AY8910_write_port_1_w },
    { 0xbc04, 0xbc04, microtan_sound_w },
    { 0xbfc0, 0xbfcf, microtan_via_0_w },
    { 0xbfd0, 0xbfd3, microtan_sio_w },
    { 0xbfe0, 0xbfef, microtan_via_1_w },
    { 0xbff0, 0xbfff, microtan_bffx_w },
    { 0xc000, 0xe7ff, MWA_ROM },
    { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

INPUT_PORTS_START( microtan )
    PORT_START /* DIP switches */
    PORT_BITX(0x03, 0x00, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Memory size", KEYCODE_NONE, IP_JOY_NONE )
    PORT_DIPSETTING(0x00, "1K" )
    PORT_DIPSETTING(0x01, "1K + 7K TANEX" )
    PORT_DIPSETTING(0x02, "1K + 7K TANEX + 40K TANRAM" )
    PORT_BIT(0xfe, 0xfe, IPT_UNUSED)

    PORT_START /* KEY ROW 0 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "ESC",       KEYCODE_ESC,         IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "1  !",      KEYCODE_1,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "2  \"",     KEYCODE_2,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "3  #",      KEYCODE_3,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "4  $",      KEYCODE_4,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "5  %",      KEYCODE_5,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "6  &",      KEYCODE_6,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "7  '",      KEYCODE_7,           IP_JOY_NONE )

    PORT_START /* KEY ROW 1 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "8  *",      KEYCODE_8,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "9  (",      KEYCODE_9,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "0  )",      KEYCODE_0,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "-  _",      KEYCODE_MINUS,       IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "=  +",      KEYCODE_EQUALS,      IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "`  ~",      KEYCODE_TILDE,       IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "BACK SPACE",KEYCODE_BACKSPACE,   IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "TAB",       KEYCODE_TAB,         IP_JOY_NONE )

    PORT_START /* KEY ROW 2 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "q  Q",      KEYCODE_Q,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "w  W",      KEYCODE_W,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "e  E",      KEYCODE_E,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "r  R",      KEYCODE_R,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "t  T",      KEYCODE_T,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "y  Y",      KEYCODE_Y,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "u  U",      KEYCODE_U,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "i  I",      KEYCODE_I,           IP_JOY_NONE )

    PORT_START /* KEY ROW 3 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "o  O",      KEYCODE_O,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "p  P",      KEYCODE_P,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "[  {",      KEYCODE_OPENBRACE,   IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "]  }",      KEYCODE_CLOSEBRACE,  IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "RETURN",    KEYCODE_ENTER,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "DEL",       KEYCODE_DEL,         IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "CTRL",      KEYCODE_LCONTROL,    IP_JOY_NONE )
    PORT_BITX(0x80, 0x80, IPT_KEYBOARD | IPF_TOGGLE,
                                        "CAPS LOCK", KEYCODE_CAPSLOCK,    IP_JOY_NONE )

    PORT_START /* KEY ROW 4 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "a  A",      KEYCODE_A,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "s  S",      KEYCODE_S,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "d  D",      KEYCODE_D,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "f  F",      KEYCODE_F,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "g  G",      KEYCODE_G,           IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "h  H",      KEYCODE_H,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "j  J",      KEYCODE_J,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "k  K",      KEYCODE_K,           IP_JOY_NONE )

    PORT_START /* KEY ROW 5 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "l  L",      KEYCODE_L,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, ";  :",      KEYCODE_COLON,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "'  \"",     KEYCODE_QUOTE,       IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "\\  |",     KEYCODE_ASTERISK,    IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "SHIFT (L)", KEYCODE_LSHIFT,      IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "z  Z",      KEYCODE_Z,           IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "x  X",      KEYCODE_X,           IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "c  C",      KEYCODE_C,           IP_JOY_NONE )

    PORT_START /* KEY ROW 6 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "v  V",      KEYCODE_V,           IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "b  B",      KEYCODE_B,           IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "n  N",      KEYCODE_N,           IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "m  M",      KEYCODE_M,           IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, ",  <",      KEYCODE_COMMA,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ".  >",      KEYCODE_STOP,        IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "/  ?",      KEYCODE_SLASH,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "SHIFT (R)", KEYCODE_RSHIFT,      IP_JOY_NONE )

    PORT_START /* KEY ROW 7 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "LINE FEED", IP_KEY_NONE,         IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "SPACE",     KEYCODE_SPACE,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "- (KP)",    KEYCODE_MINUS_PAD,   IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, ", (KP)",    KEYCODE_PLUS_PAD,    IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "ENTER (KP)",KEYCODE_ENTER_PAD,   IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, ". (KP)",    KEYCODE_DEL_PAD,     IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "0 (KP)",    KEYCODE_0_PAD,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "1 (KP)",    KEYCODE_1_PAD,       IP_JOY_NONE )

    PORT_START /* KEY ROW 8 */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "2 (KP)",    KEYCODE_2_PAD,       IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "3 (KP)",    KEYCODE_3_PAD,       IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "4 (KP)",    KEYCODE_4_PAD,       IP_JOY_NONE )
    PORT_BITX(0x08, 0x00, IPT_KEYBOARD, "5 (KP)",    KEYCODE_5_PAD,       IP_JOY_NONE )
    PORT_BITX(0x10, 0x00, IPT_KEYBOARD, "6 (KP)",    KEYCODE_6_PAD,       IP_JOY_NONE )
    PORT_BITX(0x20, 0x00, IPT_KEYBOARD, "7 (KP)",    KEYCODE_7_PAD,       IP_JOY_NONE )
    PORT_BITX(0x40, 0x00, IPT_KEYBOARD, "8 (KP)",    KEYCODE_8_PAD,       IP_JOY_NONE )
    PORT_BITX(0x80, 0x00, IPT_KEYBOARD, "9 (KP)",    KEYCODE_9_PAD,       IP_JOY_NONE )

    PORT_START /* VIA #1 PORT A */
    PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1                    )
    PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2                    )
    PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1                   )
    PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON2                   )
    PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_4WAY )
    PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_4WAY )
    PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
    PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_4WAY )

    PORT_START /* tape control */
    PORT_BITX(0x01, 0x00, IPT_KEYBOARD, "TAPE STOP", KEYCODE_F5,          IP_JOY_NONE )
    PORT_BITX(0x02, 0x00, IPT_KEYBOARD, "TAPE PLAY", KEYCODE_F6,          IP_JOY_NONE )
    PORT_BITX(0x04, 0x00, IPT_KEYBOARD, "TAPE REW",  KEYCODE_F7,          IP_JOY_NONE )
    PORT_BIT (0xf8, 0x80, IPT_UNUSED)
INPUT_PORTS_END

static struct GfxLayout char_layout =
{
    8, 16,      /* 8 x 16 graphics */
    128,        /* 128 codes */
    1,          /* 1 bit per pixel */
    { 0 },      /* no bitplanes */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
      8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
    8 * 16      /* code takes 8 times 16 bits */
};

static struct GfxLayout chunky_layout =
{
    8, 16,      /* 8 x 16 graphics */
    256,        /* 256 codes */
    1,          /* 1 bit per pixel */
    { 0 },      /* no bitplanes */
    { 0, 1, 2, 3, 4, 5, 6, 7 },
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
      8*8, 9*8,10*8,11*8,12*8,13*8,14*8,15*8 },
    8 * 16      /* code takes 8 times 16 bits */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { REGION_GFX1, 0, &char_layout, 0, 1 },
    { REGION_GFX2, 0, &chunky_layout, 0, 1 },
MEMORY_END   /* end of array */

static struct Wave_interface wave_interface = {
    1,
    { 25 }
};

static struct DACinterface dac_interface =
{
    1,          /* number of DACs */
    { 100 }     /* volume */
};

static struct AY8910interface ay8910_interface =
{
    2,  /* 2 chips */
    1000000,    /* 1 MHz ?? */
    { 50, 50 },
    { 0 },
    { 0 },
    { 0 },
    { 0 }
};

static struct MachineDriver machine_driver_microtan =
{
    /* basic machine hardware */
    {
        {
            CPU_M6502,
            750000,    /* 750 kHz */
            readmem_mtan,writemem_mtan,
            0,0,
            microtan_interrupt, 1
        }
    },
    /* frames per second, VBL duration */
    60, DEFAULT_60HZ_VBLANK_DURATION,
    1,              /* single CPU */
    microtan_init_machine,
    microtan_exit_machine,           /* stop machine */

    /* video hardware - include overscan */
    32*8, 16*16, { 0*8, 32*8 - 1, 0*16, 16*16 - 1},
    gfxdecodeinfo,
    2, 2,
    microtan_init_colors,       /* convert color prom */

    VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,   /* video flags */
    0,                      /* obsolete */
    microtan_vh_start,
    microtan_vh_stop,
    microtan_vh_screenrefresh,

    /* sound hardware */
    0,0,0,0,
    {
        {
            SOUND_WAVE,
            &wave_interface
        },
        {
            SOUND_DAC,
            &dac_interface
        },
        {
            SOUND_AY8910,
            &ay8910_interface
        },
    }
};

ROM_START(microtan)
    ROM_REGION(0x10000,REGION_CPU1,0)
        ROM_LOAD("tanex_j2.rom", 0xc000, 0x1000, 0x3e09d384)
        ROM_LOAD("tanex_h2.rom", 0xd000, 0x1000, 0x75105113)
        ROM_LOAD("tanex_d3.rom", 0xe000, 0x0800, 0xee6e8412)
        ROM_LOAD("tanex_e2.rom", 0xe800, 0x0800, 0xbd87fd34)
        ROM_LOAD("tanex_g2.rom", 0xf000, 0x0800, 0x9fd233ee)
        ROM_LOAD("tanbug_2.rom", 0xf800, 0x0400, 0x7e215313)
        ROM_LOAD("tanbug.rom",   0xfc00, 0x0400, 0xc8221d9e)
    ROM_REGION(0x00800,REGION_GFX1,0)
        ROM_LOAD("charset.rom",  0x0000, 0x0800, 0x3b3c5360)
    ROM_REGION(0x01000,REGION_GFX2,0)
        /* initialized in init_microtan */
ROM_END



static const struct IODevice io_microtan[] = {
    IO_CASSETTE_WAVE(1,"tap\0",0,microtan_cassette_init,microtan_cassette_exit),
    {
        IO_SNAPSHOT,        /* type */
        1,                  /* count */
        "m65\0",            /* file extensions */
        IO_RESET_ALL,       /* reset if file changed */
        0,
        microtan_snapshot_init, /* init */
        microtan_snapshot_exit, /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    {
        IO_QUICKLOAD,       /* type */
        1,                  /* count */
        "hex\0",            /* file extensions */
        IO_RESET_ALL,       /* reset if file changed */
        0,
        microtan_hexfile_init,  /* init */
        microtan_hexfile_exit,  /* exit */
        NULL,               /* info */
        NULL,               /* open */
        NULL,               /* close */
        NULL,               /* status */
        NULL,               /* seek */
        NULL,               /* tell */
        NULL,               /* input */
        NULL,               /* output */
        NULL,               /* input_chunk */
        NULL                /* output_chunk */
    },
    { IO_END }
};

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      COMPANY      FULLNAME */
COMP( 1979, microtan, 0,        microtan, microtan, microtan, "Tangerine", "Microtan 65" )


