/*****************************************************************************
 *
 * includes/abc80x.h
 *
 ****************************************************************************/

#ifndef __ABC80X__
#define __ABC80X__

#define ABC800_X01	XTAL_12MHz
#define ABC806_X02	XTAL_32_768kHz

#define SCREEN_TAG	"main"
#define E0516_TAG	"j13"
#define MC6845_TAG	"mc6845"

/*----------- defined in video/abc80x.c -----------*/

MACHINE_DRIVER_EXTERN(abc800m_video);
MACHINE_DRIVER_EXTERN(abc800c_video);
MACHINE_DRIVER_EXTERN(abc802_video);
MACHINE_DRIVER_EXTERN(abc806_video);

WRITE8_HANDLER( abc800m_hrs_w );
WRITE8_HANDLER( abc800m_hrc_w );

WRITE8_HANDLER( abc800c_hrs_w );
WRITE8_HANDLER( abc800c_hrc_w );

WRITE8_HANDLER( abc806_hrs_w );
WRITE8_HANDLER( abc806_hrc_w );
READ8_HANDLER( abc806_videoram_r );
WRITE8_HANDLER( abc806_videoram_w );
READ8_HANDLER( abc806_colorram_r );
WRITE8_HANDLER( abc806_colorram_w );
READ8_HANDLER( abc806_fgctlprom_r );
WRITE8_HANDLER( abc806_fgctlprom_w );
WRITE8_HANDLER( abc806_sync_w );

void abc802_mux80_40_w(int level);

#endif /* __ABC80X__ */
