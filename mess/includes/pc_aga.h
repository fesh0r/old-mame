/*
  pc cga/mda combi adapters

  one type hardware switchable between cga and mda/hercules
  another type software switchable between cga and mda/hercules

  some support additional modes like
  commodore pc10 320x200 in 16 colors


	// aga
	// 256 8x8 thick chars
	// 256 8x8 thin chars
	// 256 9x14 in 8x16 chars, line 3 is connected to a10
    ROM_LOAD("aga.chr",     0x00000, 0x02000, 0xaca81498)
	// hercules font of above
    ROM_LOAD("hercules.chr", 0x00000, 0x1000, 0x7e8c9d76)

*/

#include "driver.h"

extern struct GfxLayout europc_cga_charlayout;
extern struct GfxLayout europc_mda_charlayout;
extern struct GfxDecodeInfo europc_gfxdecodeinfo[];
extern struct GfxDecodeInfo aga_gfxdecodeinfo[];
void pc_aga_init_palette(unsigned char *sys_palette, unsigned short *sys_colortable,const unsigned char *color_prom);


typedef enum AGA_MODE { AGA_OFF, AGA_COLOR, AGA_MONO } AGA_MODE;
void pc_aga_set_mode(AGA_MODE mode);

extern void pc_aga_timer(void);
extern int  pc_aga_vh_start(void);
extern void pc_aga_vh_stop(void);
extern void pc_aga_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

extern WRITE_HANDLER ( pc_aga_videoram_w );
READ_HANDLER( pc_aga_videoram_r );

extern WRITE_HANDLER ( pc200_videoram_w );
READ_HANDLER( pc200_videoram_r );

extern WRITE_HANDLER ( pc_aga_mda_w );
extern READ_HANDLER ( pc_aga_mda_r );
extern WRITE_HANDLER ( pc_aga_cga_w );
extern READ_HANDLER ( pc_aga_cga_r );

extern WRITE_HANDLER( pc200_cga_w );
extern READ_HANDLER ( pc200_cga_r );
extern void pc200_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

