extern data8_t *iqblock_bgvideoram;
extern data8_t *iqblock_fgvideoram;
extern int iqblock_videoenable;
extern int iqblock_vidhrdw_type;
WRITE8_HANDLER( iqblock_fgvideoram_w );
WRITE8_HANDLER( iqblock_bgvideoram_w );
READ8_HANDLER( iqblock_bgvideoram_r );
WRITE8_HANDLER( iqblock_fgscroll_w );

VIDEO_START( iqblock );
VIDEO_UPDATE( iqblock );
