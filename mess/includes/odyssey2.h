/* machine/odyssey2.c */
extern int odyssey2_framestart;
extern int odyssey2_videobank;

extern MACHINE_INIT( odyssey2 );
extern int odyssey2_load_rom (int id);


/* vidhrdw/odyssey2.c */
extern int odyssey2_vh_hpos;

extern UINT8 odyssey2_colors[];

extern VIDEO_START( odyssey2 );
extern VIDEO_UPDATE( odyssey2 );
extern PALETTE_INIT( odyssey2 );
extern void odyssey2_vh_write(int data);
extern void odyssey2_vh_update(int data);
extern READ_HANDLER ( odyssey2_t1_r );
extern INTERRUPT_GEN( odyssey2_line );
extern READ_HANDLER ( odyssey2_video_r );
extern WRITE_HANDLER ( odyssey2_video_w );




/* i/o ports */
extern READ_HANDLER ( odyssey2_bus_r );
extern WRITE_HANDLER ( odyssey2_bus_w );

extern READ_HANDLER( odyssey2_getp1 );
extern WRITE_HANDLER ( odyssey2_putp1 );

extern READ_HANDLER( odyssey2_getp2 );
extern WRITE_HANDLER ( odyssey2_putp2 );

extern READ_HANDLER( odyssey2_getbus );
extern WRITE_HANDLER ( odyssey2_putbus );
