#include "driver.h"
#include "osdepend.h"
#define NC_NUM_COLOURS 4

#define NC_SCREEN_WIDTH        480
#define NC_SCREEN_HEIGHT       64

#define NC200_SCREEN_WIDTH		480
#define NC200_SCREEN_HEIGHT		128

#define NC200_NUM_COLOURS 4

int  nc_vh_start(void);
void nc_vh_stop(void);
void nc_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
void nc_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom);

void nc_set_card_present_state(int);

int  nc_pcmcia_card_load(int);
void nc_pcmcia_card_exit(int);
int  nc_serial_init(int);
enum
{
        NC_TYPE_1xx, /* nc100/nc150 */
        NC_TYPE_200  /* nc200 */
};

void nc200_video_set_backlight(int state);


