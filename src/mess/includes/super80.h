

PALETTE_INIT( super80m );
READ8_HANDLER( super80v_low_r );
WRITE8_HANDLER( super80v_low_w );
READ8_HANDLER( super80v_high_r );
WRITE8_HANDLER( super80v_high_w );
VIDEO_UPDATE( super80 );
VIDEO_UPDATE( super80m );
VIDEO_UPDATE( super80v );
VIDEO_EOF( super80m );
READ8_HANDLER( super80v_11_r );
WRITE8_HANDLER( super80v_10_w );
WRITE8_HANDLER( super80v_11_w );
WRITE8_HANDLER( super80_f1_w );

extern UINT8 *pcgram;
extern UINT8 super80v_vid_col;
extern UINT8 super80v_rom_pcg;
extern UINT8 super80_mhz;
