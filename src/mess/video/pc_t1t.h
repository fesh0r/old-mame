#ifndef PC_T1T_H
#define PC_T1T_H

#define T1000_SCREEN_NAME	"t1000_screen"
#define T1000_MC6845_NAME	"mc6845_t1000"

MACHINE_DRIVER_EXTERN( pcvideo_t1000 );
MACHINE_DRIVER_EXTERN( pcvideo_pcjr );

 READ8_HANDLER ( pc_t1t_videoram_r );
WRITE8_HANDLER ( pc_T1T_w );
 READ8_HANDLER (	pc_T1T_r );
WRITE8_HANDLER( pc_pcjr_w );

#endif /* PC_T1T_H */
