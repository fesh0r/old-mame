/* machine/odyssey2.c */
extern int odyssey2_framestart;
extern int odyssey2_videobank;

DRIVER_INIT( odyssey2 );
MACHINE_INIT( odyssey2 );
DEVICE_LOAD( odyssey2_cart );


/* vidhrdw/odyssey2.c */
extern int odyssey2_vh_hpos;

extern UINT8 odyssey2_colors[];

VIDEO_START( odyssey2 );
VIDEO_UPDATE( odyssey2 );
PALETTE_INIT( odyssey2 );
void odyssey2_vh_write(int data);
void odyssey2_vh_update(int data);
READ8_HANDLER ( odyssey2_t1_r );
INTERRUPT_GEN( odyssey2_line );
READ8_HANDLER ( odyssey2_video_r );
WRITE8_HANDLER ( odyssey2_video_w );

/* sndhrdw/odyssey2.c */
extern int odyssey2_sh_channel;
extern struct CustomSound_interface odyssey2_sound_interface;
int odyssey2_sh_start(const struct MachineSound* driver);
void odyssey2_sh_update( int param, INT16 *buffer, int length );

/* i/o ports */
READ8_HANDLER ( odyssey2_bus_r );
WRITE8_HANDLER ( odyssey2_bus_w );

READ8_HANDLER( odyssey2_getp1 );
WRITE8_HANDLER ( odyssey2_putp1 );

READ8_HANDLER( odyssey2_getp2 );
WRITE8_HANDLER ( odyssey2_putp2 );

READ8_HANDLER( odyssey2_getbus );
WRITE8_HANDLER ( odyssey2_putbus );
