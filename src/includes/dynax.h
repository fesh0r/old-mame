/***************************************************************************


					Interrupts


***************************************************************************/

/* Variables defined in drivers: */

extern UINT8 dynax_blitter_irq,	dynax_blitter2_irq;

/* Functions defined in drivers: */

void sprtmtch_update_irq(void);
void jantouki_update_irq(void);

/***************************************************************************


					Video Blitter(s)


***************************************************************************/

/* Functions defined in vidhrdw: */

WRITE_HANDLER( dynax_blitter_rev2_w );
WRITE_HANDLER( jantouki_blitter_rev2_w );
WRITE_HANDLER( jantouki_blitter2_rev2_w );


WRITE_HANDLER( dynax_blit_pen_w );
WRITE_HANDLER( dynax_blit2_pen_w );
WRITE_HANDLER( dynax_blit_backpen_w );
WRITE_HANDLER( dynax_blit_dest_w );
WRITE_HANDLER( dynax_blit2_dest_w );
WRITE_HANDLER( dynax_blit_palbank_w );
WRITE_HANDLER( dynax_blit2_palbank_w );
WRITE_HANDLER( dynax_blit_palette01_w );
WRITE_HANDLER( dynax_blit_palette23_w );
WRITE_HANDLER( dynax_blit_palette45_w );
WRITE_HANDLER( dynax_blit_palette67_w );
WRITE_HANDLER( dynax_layer_enable_w );
WRITE_HANDLER( jantouki_layer_enable_w );
WRITE_HANDLER( dynax_flipscreen_w );
WRITE_HANDLER( dynax_extra_scrollx_w );
WRITE_HANDLER( dynax_extra_scrolly_w );

WRITE_HANDLER( hanamai_layer_half_w );
WRITE_HANDLER( hnoridur_layer_half2_w );
WRITE_HANDLER( hanamai_priority_w );
WRITE_HANDLER( mjdialq2_blit_dest_w );
WRITE_HANDLER( mjdialq2_layer_enable_w );

VIDEO_START( hanamai );
VIDEO_START( hnoridur );
VIDEO_START( mcnpshnt );
VIDEO_START( sprtmtch );
VIDEO_START( mjdialq2 );
VIDEO_START( jantouki );

VIDEO_UPDATE( hanamai );
VIDEO_UPDATE( hnoridur );
VIDEO_UPDATE( sprtmtch );
VIDEO_UPDATE( mjdialq2 );
VIDEO_UPDATE( jantouki );

PALETTE_INIT( sprtmtch );
