/* machine/lviv.c */
extern unsigned char * lviv_video_ram;
READ_HANDLER ( lviv_io_r );
WRITE_HANDLER ( lviv_io_w );
extern MACHINE_INIT( lviv );
extern int lviv_tape_init(int, void *fp, int open_mode);

/* vidhrdw/lviv.c */
extern VIDEO_START( lviv );
extern VIDEO_UPDATE( lviv );
extern unsigned char lviv_palette[8*3];
extern unsigned short lviv_colortable[1][4];
extern PALETTE_INIT( lviv );
extern void lviv_update_palette (UINT8);
