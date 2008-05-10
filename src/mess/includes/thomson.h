/**********************************************************************

  Copyright (C) Antoine Mine' 2006

  Thomson 8-bit computers

**********************************************************************/

#ifndef _THOMSON_H_
#define _THOMSON_H_


/*************************** common ********************************/

/* input ports (first port number of each class) */
#define THOM_INPUT_LIGHTPEN  0 /* 3 ports: analog X, analog Y, and button */
#define THOM_INPUT_GAME      3 /* 2-5 ports: joystick, mouse */
#define THOM_INPUT_KEYBOARD  8 /* 8-10 lines */
#define THOM_INPUT_CONFIG   18 /* machine-specific options */
#define THOM_INPUT_FCONFIG  19 /* floppy / network options */
#define THOM_INPUT_VCONFIG  20 /* video options */
#define THOM_INPUT_MCONFIG  21 /* modem / speech options */

/* 6821 PIAs */
#define THOM_PIA_SYS    0  /* system PIA */
#define THOM_PIA_GAME   1  /* music & game PIA (joypad + sound) */
#define THOM_PIA_IO     2  /* CC 90-232 I/O extension (parallel & RS-232) */
#define THOM_PIA_MODEM  3  /* MD 90-120 MODEM extension */

/* sound ports */
#define THOM_SOUND_BUZ    0 /* 1-bit buzzer */
#define THOM_SOUND_GAME   1 /* 6-bit game port DAC */
#define THOM_SOUND_SPEECH 2 /* speach synthesis */

/* serial devices */
#define THOM_SERIAL_CC90323  0 /* RS232 port in I/O extension */
#define THOM_SERIAL_RF57232  1 /* RS232 extension */
#define THOM_SERIAL_MODEM    2 /* modem extension */


/* bank-switching */
#define THOM_CART_BANK  2 /* cartridge ROM */
#define THOM_RAM_BANK   3 /* data RAM */
#define THOM_FLOP_BANK  4 /* external floppy controller ROM */
#define THOM_BASE_BANK  5 /* system RAM */


/* serial */
extern DEVICE_START( thom_serial );
extern DEVICE_IMAGE_LOAD( thom_serial );
extern DEVICE_IMAGE_UNLOAD( thom_serial );


/***************************** TO7 / T9000 *************************/

/* cartridge bank-switching */
extern DEVICE_IMAGE_LOAD( to7_cartridge );
extern WRITE8_HANDLER ( to7_cartridge_w );
extern READ8_HANDLER  ( to7_cartridge_r );

/* dispatch MODEM or speech synthesis extension */
extern READ8_HANDLER ( to7_modem_mea8000_r );
extern WRITE8_HANDLER ( to7_modem_mea8000_w );

/* MIDI extension (actually an 6850 ACIA) */
extern READ8_HANDLER  ( to7_midi_r );
extern WRITE8_HANDLER ( to7_midi_w );

extern MACHINE_START ( to7 );
extern MACHINE_RESET ( to7 );


/***************************** TO7/70 ******************************/

/* gate-array */
extern READ8_HANDLER  ( to770_gatearray_r );
extern WRITE8_HANDLER ( to770_gatearray_w );

extern MACHINE_START ( to770 );
extern MACHINE_RESET ( to770 );


/***************************** MO5 ******************************/

/* gate-array */
extern READ8_HANDLER  ( mo5_gatearray_r );
extern WRITE8_HANDLER ( mo5_gatearray_w );

/* cartridge / extended RAM bank-switching */
extern DEVICE_IMAGE_LOAD( mo5_cartridge );
extern WRITE8_HANDLER ( mo5_ext_w );
extern WRITE8_HANDLER ( mo5_cartridge_w );
extern READ8_HANDLER  ( mo5_cartridge_r );

extern MACHINE_START ( mo5 );
extern MACHINE_RESET ( mo5 );


/***************************** TO9 ******************************/

/* IEEE extension */
extern WRITE8_HANDLER ( to9_ieee_w );
extern READ8_HANDLER  ( to9_ieee_r );

/* ROM bank-switching */
extern WRITE8_HANDLER ( to9_cartridge_w );
extern READ8_HANDLER  ( to9_cartridge_r );

/* system gate-array */
extern READ8_HANDLER  ( to9_gatearray_r );
extern WRITE8_HANDLER ( to9_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( to9_vreg_r );
extern WRITE8_HANDLER ( to9_vreg_w );

/* keyboard */
extern READ8_HANDLER  ( to9_kbd_r );
extern WRITE8_HANDLER ( to9_kbd_w );

extern MACHINE_START ( to9 );
extern MACHINE_RESET ( to9 );


/***************************** TO8 ******************************/

/* bank-switching */
#define TO8_SYS_LO      5 /* system RAM low 2 Kb */
#define TO8_SYS_HI      6 /* system RAM hi 2 Kb */
#define TO8_DATA_LO     7 /* data RAM low 2 Kb */
#define TO8_DATA_HI     8 /* data RAM hi 2 Kb */
#define TO8_BIOS_BANK   9 /* BIOS ROM */

extern UINT8 to8_data_vpage;
extern UINT8 to8_cart_vpage;

extern WRITE8_HANDLER ( to8_cartridge_w );
extern READ8_HANDLER  ( to8_cartridge_r );

/* system gate-array */
extern READ8_HANDLER  ( to8_gatearray_r );
extern WRITE8_HANDLER ( to8_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( to8_vreg_r );
extern WRITE8_HANDLER ( to8_vreg_w );

/* floppy */
extern READ8_HANDLER  ( to8_floppy_r );
extern WRITE8_HANDLER ( to8_floppy_w );

extern MACHINE_START ( to8 );
extern MACHINE_RESET ( to8 );


/***************************** TO9+ ******************************/

extern MACHINE_START ( to9p );
extern MACHINE_RESET ( to9p );


/***************************** MO6 ******************************/

extern READ8_HANDLER  ( mo6_cartridge_r );
extern WRITE8_HANDLER ( mo6_cartridge_w );
extern WRITE8_HANDLER ( mo6_ext_w );

/* system gate-array */
extern READ8_HANDLER  ( mo6_gatearray_r );
extern WRITE8_HANDLER ( mo6_gatearray_w );

/* video gate-array */
extern READ8_HANDLER  ( mo6_vreg_r );
extern WRITE8_HANDLER ( mo6_vreg_w );

extern MACHINE_START ( mo6 );
extern MACHINE_RESET ( mo6 );


/***************************** MO5 NR ******************************/

/* network */
extern READ8_HANDLER  ( mo5nr_net_r );
extern WRITE8_HANDLER ( mo5nr_net_w );

/* printer */
extern READ8_HANDLER  ( mo5nr_prn_r );
extern WRITE8_HANDLER ( mo5nr_prn_w );

extern MACHINE_START ( mo5nr );
extern MACHINE_RESET ( mo5nr );


/*
   TO7 video:
   one line (64 us) =
      56 left border pixels ( 7 us)
   + 320 active pixels (40 us)
   +  56 right border pixels ( 7 us)
   +     horizontal retrace (10 us)

   one image (20 ms) =
      47 top border lines (~3 ms)
   + 200 active lines (12.8 ms)
   +  47 bottom border lines (~3 ms)
   +     vertical retrace (~1 ms)

   TO9 and up introduced a half (160 pixels) and double (640 pixels)
   horizontal mode, but still in 40 us (no change in refresh rate).
*/


/***************************** dimensions **************************/

/* original screen dimension (may be different from emulated screen!) */
#define THOM_ACTIVE_WIDTH  320
#define THOM_BORDER_WIDTH   56
#define THOM_ACTIVE_HEIGHT 200
#define THOM_BORDER_HEIGHT  47
#define THOM_TOTAL_WIDTH   432
#define THOM_TOTAL_HEIGHT  294

/* Emulated screen dimension may be doubled to allow hi-res 640x200 mode.
   Emulated screen can have smaller borders.
 */

/* maximum number of video pages:
   1 for TO7 generation (including MO5)
   4 for TO8 generation (including TO9, MO6)
 */
#define THOM_NB_PAGES 4

/* page 0 is banked */
#define THOM_VRAM_BANK 1

extern UINT8* thom_vram;

/*********************** video signals *****************************/

struct thom_vsignal {
  unsigned count;  /* pixel counter */
  unsigned init;   /* 1 -> active vertical windos, 0 -> border/VBLANK */
  unsigned inil;   /* 1 -> active horizontal window, 0 -> border/HBLANK */
  unsigned lt3;    /* bit 3 of us counter */
  unsigned line;   /* line counter */
};

/* current video position */
extern struct thom_vsignal thom_get_vsignal ( void );


/************************* lightpen ********************************/

/* specific TO7 / T9000 lightpen code (no video gate-array) */
extern unsigned to7_lightpen_gpl ( int decx, int decy );

/* video position corresponding to lightpen (with some offset) */
extern struct thom_vsignal thom_get_lightpen_vsignal ( int xdec, int ydec,
						       int xdec2 );

/* specify a lightpencall-back function, called nb times per frame */
extern void thom_set_lightpen_callback ( int nb, void (*cb) ( int step ) );


/***************************** commons *****************************/

extern VIDEO_START  ( thom );
extern VIDEO_UPDATE ( thom );
extern PALETTE_INIT ( thom );
extern VIDEO_EOF    ( thom );

/* pass video init signal */
extern void thom_set_init_callback ( void (*cb) ( int init ) );

/* TO7 TO7/70 MO5 video bank switch */
extern void thom_set_mode_point ( int point );

/* set the palette index for the border color */
extern void thom_set_border_color ( unsigned color );

/* set one of 16 palette indices to one of 4096 colors */
extern void thom_set_palette ( unsigned index, UINT16 color );


/* video modes */
#define THOM_VMODE_TO770       0
#define THOM_VMODE_MO5         1
#define THOM_VMODE_BITMAP4     2
#define THOM_VMODE_BITMAP4_ALT 3
#define THOM_VMODE_80          4
#define THOM_VMODE_BITMAP16    5
#define THOM_VMODE_PAGE1       6
#define THOM_VMODE_PAGE2       7
#define THOM_VMODE_OVERLAY     8
#define THOM_VMODE_OVERLAY3    9
#define THOM_VMODE_TO9        10
#define THOM_VMODE_80_TO9     11
#define THOM_VMODE_NB         12

/* change the current video-mode */
extern void thom_set_video_mode ( unsigned mode );

/* select which video page shown among the 4 available */
extern void thom_set_video_page ( unsigned page );

/* to tell there is some floppy activity, stays up for a few frames */
extern void thom_floppy_active ( int write );


/***************************** TO7 / T9000 *************************/

extern WRITE8_HANDLER ( to7_vram_w );


/***************************** TO7/70 ******************************/

extern WRITE8_HANDLER ( to770_vram_w );


/***************************** TO8 ******************************/

/* write to video memory through system space (always page 1) */
WRITE8_HANDLER ( to8_sys_lo_w );
WRITE8_HANDLER ( to8_sys_hi_w );

/* write to video memory through data space */
WRITE8_HANDLER ( to8_data_lo_w );
WRITE8_HANDLER ( to8_data_hi_w );

/* write to video memory page through cartridge addresses space */
WRITE8_HANDLER ( to8_vcart_w );


#endif /* _THOMSON_H_ */
