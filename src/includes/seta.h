/***************************************************************************

							-= Seta Hardware =-

***************************************************************************/

/* Variables and functions defined in drivers/seta.c */

void seta_coin_lockout_w(int data);


/* Variables and functions defined in vidhrdw/seta.c */

extern data16_t *seta_vram_0, *seta_vram_1, *seta_vctrl_0;
extern data16_t *seta_vram_2, *seta_vram_3, *seta_vctrl_2;
extern data16_t *seta_vregs;

extern int seta_tiles_offset;

WRITE16_HANDLER( seta_vram_0_w );
WRITE16_HANDLER( seta_vram_1_w );
WRITE16_HANDLER( seta_vram_2_w );
WRITE16_HANDLER( seta_vram_3_w );
WRITE16_HANDLER( seta_vregs_w );

PALETTE_INIT( blandia );
PALETTE_INIT( gundhara );
PALETTE_INIT( jjsquawk );
PALETTE_INIT( usclssic );
PALETTE_INIT( zingzip );

VIDEO_START( seta_no_layers);
VIDEO_START( seta_1_layer);
VIDEO_START( seta_1_layer_offset_0x02);
VIDEO_START( seta_2_layers);
VIDEO_START( seta_2_layers_offset_0x02);
VIDEO_START( oisipuzl_2_layers );

VIDEO_UPDATE( seta );
VIDEO_UPDATE( seta_no_layers );


/* Variables and functions defined in vidhrdw/seta2.c */

extern data16_t *seta2_vregs;

WRITE16_HANDLER( seta2_vregs_w );

PALETTE_INIT( seta2 );
VIDEO_UPDATE( seta2 );


/* Variables and functions defined in sndhrdw/seta.c */

extern int seta_samples_bank;

READ_HANDLER ( seta_sound_r );
WRITE_HANDLER( seta_sound_w );

READ16_HANDLER ( seta_sound_word_r );
WRITE16_HANDLER( seta_sound_word_w );

void seta_sound_enable_w(int);

int seta_sh_start(const struct MachineSound *msound, UINT32 clock);

extern struct CustomSound_interface seta_sound_interface;


/* Variables and functions defined in vidhrdw/ssv.c */

extern data16_t *ssv_scroll;

extern int ssv_special;

extern int ssv_tile_code[16];

extern int ssv_sprites_offsx, ssv_sprites_offsy;
extern int ssv_tilemap_offsx, ssv_tilemap_offsy;

READ16_HANDLER( ssv_vblank_r );
WRITE16_HANDLER( ssv_scroll_w );
WRITE16_HANDLER( paletteram16_xrgb_swap_word_w );
void ssv_enable_video(int enable);

PALETTE_INIT( ssv );
VIDEO_UPDATE( ssv );
