/*****************************************************************************
 *
 * includes/ssystem3.h
 *
 ****************************************************************************/

#ifndef SSYSTEM3_H_
#define SSYSTEM3_H_


/*----------- defined in drivers/ssystem3.c -----------*/

void ssystem3_playfield_getfigure(int x, int y, int *figure, int *black);

/*----------- defined in video/ssystem3.c -----------*/

extern PALETTE_INIT( ssystem3 );
extern VIDEO_START( ssystem3 );
extern VIDEO_UPDATE( ssystem3 );

void ssystem3_lcd_reset(void) ;
void ssystem3_lcd_write(running_machine *machine, int clock, int data);

#endif /* SSYSTEM3_H_ */
