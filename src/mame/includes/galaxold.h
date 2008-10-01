/***************************************************************************

  Galaxian hardware family

  This include file is used by the following drivers:
    - galaxian.c
    - scramble.c
    - scobra.c
    - frogger.c
    - amidar.c

***************************************************************************/

#include "machine/8255ppi.h"

/*----------- defined in video/galaxold.c -----------*/

extern UINT8 *galaxold_videoram;
extern UINT8 *galaxold_spriteram;
extern UINT8 *galaxold_spriteram2;
extern UINT8 *galaxold_attributesram;
extern UINT8 *galaxold_bulletsram;
extern UINT8 *rockclim_videoram;
extern UINT8 *racknrol_tiles_bank;


extern size_t galaxold_spriteram_size;
extern size_t galaxold_spriteram2_size;
extern size_t galaxold_bulletsram_size;
extern UINT8 galaxold_stars_on;

PALETTE_INIT( galaxold );
PALETTE_INIT( scrambold );
PALETTE_INIT( darkplnt );
PALETTE_INIT( rescue );
PALETTE_INIT( minefld );
PALETTE_INIT( stratgyx );
PALETTE_INIT( mariner );
PALETTE_INIT( rockclim );
PALETTE_INIT( dambustr );
PALETTE_INIT( turtles );

VIDEO_START( dambustr );

WRITE8_HANDLER( galaxold_videoram_w );
READ8_HANDLER( galaxold_videoram_r );

WRITE8_HANDLER( rockclim_videoram_w );
WRITE8_HANDLER( rockclim_scroll_w );
READ8_HANDLER( rockclim_videoram_r );
WRITE8_HANDLER( dambustr_bg_split_line_w );
WRITE8_HANDLER( dambustr_bg_color_w );

WRITE8_HANDLER( galaxold_attributesram_w );

WRITE8_HANDLER( galaxold_stars_enable_w );
WRITE8_HANDLER( scrambold_background_enable_w );
WRITE8_HANDLER( scrambold_background_red_w );
WRITE8_HANDLER( scrambold_background_green_w );
WRITE8_HANDLER( scrambold_background_blue_w );
WRITE8_HANDLER( hotshock_flip_screen_w );
WRITE8_HANDLER( darkplnt_bullet_color_w );
WRITE8_HANDLER( racknrol_tiles_bank_w );

VIDEO_START( galaxold_plain );
VIDEO_START( galaxold );
VIDEO_START( mooncrst );
VIDEO_START( pisces );
VIDEO_START( batman2 );
VIDEO_START( dkongjrm );
VIDEO_START( scrambold );
VIDEO_START( theend );
VIDEO_START( darkplnt );
VIDEO_START( rescue );
VIDEO_START( minefld );
VIDEO_START( stratgyx );
VIDEO_START( mimonkey );
VIDEO_START( mariner );
VIDEO_START( ckongs );
VIDEO_START( newsin7 );
VIDEO_START( rockclim );
VIDEO_START( drivfrcg );
VIDEO_START( bongo );
VIDEO_START( scorpion );
VIDEO_START( racknrol );
VIDEO_START( ad2083 );

void galaxold_init_stars(running_machine *machine, int colors_offset);
void galaxold_draw_stars(running_machine *machine, bitmap_t *bitmap, const rectangle *cliprect);

VIDEO_UPDATE( galaxold );
VIDEO_UPDATE( dambustr );

WRITE8_HANDLER( galaxold_gfxbank_w );
WRITE8_HANDLER( galaxold_flip_screen_x_w );
WRITE8_HANDLER( galaxold_flip_screen_y_w );
WRITE8_HANDLER( gteikob2_flip_screen_x_w );
WRITE8_HANDLER( gteikob2_flip_screen_y_w );


/*----------- defined in machine/galaxold.c -----------*/

DRIVER_INIT( zigzag );
DRIVER_INIT( pisces );
DRIVER_INIT( checkmaj );
DRIVER_INIT( dingo );
DRIVER_INIT( dingoe );
DRIVER_INIT( kingball );
DRIVER_INIT( mooncrsu );
DRIVER_INIT( mooncrst );
DRIVER_INIT( mooncrgx );
DRIVER_INIT( moonqsr );
DRIVER_INIT( checkman );
DRIVER_INIT( azurian );
DRIVER_INIT( 4in1 );
DRIVER_INIT( ladybugg );
DRIVER_INIT( gmgalax );

WRITE8_HANDLER( galaxold_nmi_enable_w );

MACHINE_RESET( galaxold );
MACHINE_RESET( devilfsg );

WRITE8_HANDLER( galaxold_coin_lockout_w );
WRITE8_HANDLER( galaxold_coin_counter_w );
#define galaxold_coin_counter_0_w galaxold_coin_counter_w
WRITE8_HANDLER( galaxold_coin_counter_1_w );
WRITE8_HANDLER( galaxold_coin_counter_2_w );
WRITE8_HANDLER( galaxold_leds_w );

READ8_HANDLER( scramblb_protection_1_r );
READ8_HANDLER( scramblb_protection_2_r );

WRITE8_HANDLER( kingball_speech_dip_w );
WRITE8_HANDLER( kingball_sound1_w );
WRITE8_HANDLER( kingball_sound2_w );

WRITE8_HANDLER( _4in1_bank_w );
CUSTOM_INPUT( _4in1_fake_port_r );

WRITE8_HANDLER( zigzag_sillyprotection_w );

INTERRUPT_GEN( hunchbks_vh_interrupt );
INTERRUPT_GEN( gmgalax_vh_interrupt );


/*----------- defined in machine/scramble.c -----------*/

extern const ppi8255_interface scramble_ppi_0_intf;
extern const ppi8255_interface scramble_ppi_1_intf;
extern const ppi8255_interface stratgyx_ppi_1_intf;
extern const ppi8255_interface scramble_protection_ppi_1_intf;
extern const ppi8255_interface mrkougar_ppi_1_intf;


DRIVER_INIT( scramble_ppi );
DRIVER_INIT( scobra );
DRIVER_INIT( atlantis );
DRIVER_INIT( scramble );
DRIVER_INIT( stratgyx );
DRIVER_INIT( tazmani2 );
DRIVER_INIT( ckongs );
DRIVER_INIT( mariner );
DRIVER_INIT( frogger );
DRIVER_INIT( froggers );
DRIVER_INIT( devilfsh );
DRIVER_INIT( mars );
DRIVER_INIT( hotshock );
DRIVER_INIT( cavelon );
DRIVER_INIT( darkplnt );
DRIVER_INIT( mimonkey );
DRIVER_INIT( mimonsco );
DRIVER_INIT( mimonscr );
DRIVER_INIT( anteater );
DRIVER_INIT( rescue );
DRIVER_INIT( minefld );
DRIVER_INIT( losttomb );
DRIVER_INIT( superbon );
DRIVER_INIT( hustler );
DRIVER_INIT( billiard );
DRIVER_INIT( mrkougar );
DRIVER_INIT( mrkougb );
DRIVER_INIT( ad2083 );

MACHINE_RESET( scramble );
MACHINE_RESET( sfx );
MACHINE_RESET( monsterz );
MACHINE_RESET( explorer );

READ8_HANDLER( triplep_pip_r );
READ8_HANDLER( triplep_pap_r );

READ8_HANDLER( hunchbks_mirror_r );
WRITE8_HANDLER( hunchbks_mirror_w );

CUSTOM_INPUT( darkplnt_custom_r );


/*----------- defined in audio/scramble.c -----------*/

void scramble_sh_init(void);
void sfx_sh_init(void);

WRITE8_HANDLER( scramble_filter_w );
WRITE8_HANDLER( frogger_filter_w );

READ8_HANDLER( scramble_portB_r );
READ8_HANDLER( frogger_portB_r );

READ8_HANDLER( hotshock_soundlatch_r );

WRITE8_DEVICE_HANDLER( scramble_sh_irqtrigger_w );
WRITE8_HANDLER( sfx_sh_irqtrigger_w );
WRITE8_DEVICE_HANDLER( mrkougar_sh_irqtrigger_w );
WRITE8_HANDLER( froggrmc_sh_irqtrigger_w );
WRITE8_HANDLER( hotshock_sh_irqtrigger_w );

MACHINE_DRIVER_EXTERN( ad2083_audio );

