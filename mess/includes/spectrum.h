extern int  spectrum_snap_load(int id);
extern void spectrum_snap_exit(int id);

extern int  load_snap(void);

typedef enum
{
	TIMEX_CART_NONE,
	TIMEX_CART_DOCK,
	TIMEX_CART_EXROM,
	TIMEX_CART_HOME
}
TIMEX_CART_TYPE;

extern TIMEX_CART_TYPE timex_cart_type;
extern UINT8 timex_cart_chunks;
extern UINT8 * timex_cart_data;
extern int  spectrum_cart_load(int id);
extern void spectrum_cart_exit(int id);

extern int  timex_cart_load(int id);
extern void timex_cart_exit(int id);
extern void ts2068_update_memory(void);

extern void spectrum_init_machine(void);
extern void spectrum_shutdown_machine(void);

extern int  spec_quick_init (int id);
extern void spec_quick_exit (int id);
extern int  spec_quick_open (int id, int mode, void *arg);

/*-----------------27/02/00 10:49-------------------
 code for WAV reading writing
--------------------------------------------------*/
extern int spectrum_cassette_init(int);
extern void spectrum_cassette_exit(int);

extern int spectrum_128_port_7ffd_data;
extern int spectrum_plus3_port_1ffd_data;
extern int ts2068_port_ff_data;
extern int ts2068_port_f4_data;
extern int PreviousFE;
extern unsigned char *spectrum_128_screen_location;
extern unsigned char *ts2068_ram;


extern void spectrum_128_update_memory(void);
extern void spectrum_plus3_update_memory(void);


extern int  spectrum_vh_start(void);
extern void spectrum_vh_stop(void);
extern void spectrum_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
extern void spectrum_eof_callback(void);

extern int spectrum_128_vh_start(void);
extern void spectrum_128_vh_stop(void);
extern void spectrum_128_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

extern void ts2068_eof_callback(void);
extern void ts2068_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

extern void tc2048_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

extern WRITE_HANDLER ( spectrum_characterram_w );
extern READ_HANDLER  ( spectrum_characterram_r );
extern WRITE_HANDLER ( spectrum_colorram_w );
extern READ_HANDLER  ( spectrum_colorram_r );

/* Spectrum screen size in pixels */
#define SPEC_UNSEEN_LINES  16   /* Non-visible scanlines before first border
                                   line. Some of these may be vertical retrace. */
#define SPEC_TOP_BORDER    48   /* Number of border lines before actual screen */
#define SPEC_DISPLAY_YSIZE 192  /* Vertical screen resolution */
#define SPEC_BOTTOM_BORDER 56   /* Number of border lines at bottom of screen */
#define SPEC_SCREEN_HEIGHT (SPEC_TOP_BORDER + SPEC_DISPLAY_YSIZE + SPEC_BOTTOM_BORDER)

#define SPEC_LEFT_BORDER   48   /* Number of left hand border pixels */
#define SPEC_DISPLAY_XSIZE 256  /* Horizontal screen resolution */
#define SPEC_RIGHT_BORDER  48   /* Number of right hand border pixels */
#define SPEC_SCREEN_WIDTH (SPEC_LEFT_BORDER + SPEC_DISPLAY_XSIZE + SPEC_RIGHT_BORDER)

#define SPEC_LEFT_BORDER_CYCLES   24   /* Cycles to display left hand border */
#define SPEC_DISPLAY_XSIZE_CYCLES 128  /* Horizontal screen resolution */
#define SPEC_RIGHT_BORDER_CYCLES  24   /* Cycles to display right hand border */
#define SPEC_RETRACE_CYCLES       48   /* Cycles taken for horizonal retrace */
#define SPEC_CYCLES_PER_LINE      224  /* Number of cycles to display a single line */

/* 128K machines take an extra 4 cycles per scan line - add this to retrace */
#define SPEC128_UNSEEN_LINES    15
#define SPEC128_RETRACE_CYCLES  52
#define SPEC128_CYCLES_PER_LINE 228

/* Border sizes for TS2068. These are guesses based on the number of cycles
   available per frame. */
#define TS2068_TOP_BORDER    32
#define TS2068_BOTTOM_BORDER 32
#define TS2068_SCREEN_HEIGHT (TS2068_TOP_BORDER + SPEC_DISPLAY_YSIZE + TS2068_BOTTOM_BORDER)

/* Double the border sizes to maintain ratio of screen to border */
#define TS2068_LEFT_BORDER   96   /* Number of left hand border pixels */
#define TS2068_DISPLAY_XSIZE 512  /* Horizontal screen resolution */
#define TS2068_RIGHT_BORDER  96   /* Number of right hand border pixels */
#define TS2068_SCREEN_WIDTH (TS2068_LEFT_BORDER + TS2068_DISPLAY_XSIZE + TS2068_RIGHT_BORDER)
