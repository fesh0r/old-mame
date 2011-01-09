#include "video/mc6845.h"
#include "machine/z80pio.h"

/* Bits in shared variable:
    d5 cassette LED
    d4 super80v rom or pcg bankswitch (1=pcg ram, 0=char gen rom)
    d2 super80v video or colour bankswitch (1=video ram, 0=colour ram)
    d2 super80 screen off (=2mhz) or on (bursts of 2mhz at 50hz = 1mhz) */



class super80_state : public driver_device
{
public:
	super80_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 super80_shared;
	device_t *z80pio;
	device_t *speaker;
	device_t *cassette;
	device_t *printer;
	UINT8 keylatch;
	UINT8 cass_data[4];
	UINT8 int_sw;
	UINT8 last_data;
	UINT16 vidpg;
	UINT8 current_palette;
	UINT8 current_charset;
	const UINT8 *FNT;
	UINT8 s_options;
	UINT8 mc6845_cursor[16];
	UINT8 mc6845_reg[32];
	UINT8 mc6845_ind;
	device_t *mc6845;
	UINT8 framecnt;
	UINT8 speed;
	UINT8 flash;
	UINT16 cursor;
	UINT8 *videoram;
	UINT8 *colorram;
	UINT8 *pcgram;
};


/*----------- defined in video/super80.c -----------*/

PALETTE_INIT( super80m );
VIDEO_UPDATE( super80 );
VIDEO_UPDATE( super80d );
VIDEO_UPDATE( super80e );
VIDEO_UPDATE( super80m );
VIDEO_START( super80 );
VIDEO_EOF( super80m );

READ8_HANDLER( super80v_low_r );
READ8_HANDLER( super80v_high_r );
WRITE8_HANDLER( super80v_low_w );
WRITE8_HANDLER( super80v_high_w );
WRITE8_HANDLER( super80v_10_w );
WRITE8_HANDLER( super80v_11_w );
WRITE8_HANDLER( super80_f1_w );
VIDEO_START( super80v );
VIDEO_UPDATE( super80v );
MC6845_UPDATE_ROW( super80v_update_row );

/*----------- defined in machine/super80.c -----------*/

READ8_HANDLER( super80_dc_r );
READ8_HANDLER( super80_f2_r );
WRITE8_HANDLER( super80_dc_w );
WRITE8_HANDLER( super80_f0_w );
WRITE8_HANDLER( super80r_f0_w );
MACHINE_RESET( super80 );
DRIVER_INIT( super80 );
DRIVER_INIT( super80v );

extern const z80pio_interface super80_pio_intf;

