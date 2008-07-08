/*****************************************************************************
 *
 * includes/pcw.h
 *
 ****************************************************************************/

#ifndef PCW_H_
#define PCW_H_


#define PCW_BORDER_HEIGHT 8
#define PCW_BORDER_WIDTH 8
#define PCW_NUM_COLOURS 2
#define PCW_DISPLAY_WIDTH 720
#define PCW_DISPLAY_HEIGHT 256

#define PCW_SCREEN_WIDTH	(PCW_DISPLAY_WIDTH + (PCW_BORDER_WIDTH<<1))
#define PCW_SCREEN_HEIGHT	(PCW_DISPLAY_HEIGHT  + (PCW_BORDER_HEIGHT<<1))


/*----------- defined in drivers/pcw.c -----------*/

extern unsigned int roller_ram_addr;
extern unsigned short roller_ram_offset;
extern unsigned char pcw_vdu_video_control_register;

/*----------- defined in video/pcw.c -----------*/

extern VIDEO_START( pcw );
extern VIDEO_UPDATE( pcw );
extern PALETTE_INIT( pcw );


#endif /* PCW_H_ */
