#ifndef __VECTREX_H__
#define __VECTREX_H__

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* From machine/vectrex.c */
extern unsigned char *vectrex_ram;
extern READ_HANDLER  ( vectrex_mirrorram_r );
extern WRITE_HANDLER ( vectrex_mirrorram_w );
extern int vectrex_init_cart (int id, void *fp, int open_mode);

/* From machine/vectrex.c */
extern int vectrex_imager_status;
extern UINT32 vectrex_beam_color;
extern unsigned char vectrex_via_out[2];
extern double imager_freq;
extern void *imager_timer;

extern void vectrex_imager_right_eye (int param);
extern void vectrex_configuration(void);
extern READ_HANDLER (v_via_pa_r);
extern READ_HANDLER(v_via_pb_r );
extern void v_via_irq (int level);
extern WRITE_HANDLER ( vectrex_psg_port_w );

/* for spectrum 1+ */
extern READ_HANDLER( s1_via_pb_r );

/* From vidhrdw/vectrex.c */
extern VIDEO_START( vectrex );
extern VIDEO_UPDATE( vectrex );

extern VIDEO_START( raaspec );
extern VIDEO_UPDATE( raaspec );

extern WRITE_HANDLER  ( raaspec_led_w );

/* from vidhrdw/vectrex.c */
extern void vectrex_add_point_stereo (int x, int y, rgb_t color, int intensity);
extern void vectrex_add_point (int x, int y, rgb_t color, int intensity);
extern void (*vector_add_point_function) (int, int, rgb_t, int);

#endif
