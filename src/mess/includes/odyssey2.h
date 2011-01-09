/*****************************************************************************
 *
 * includes/odyssey2.h
 *
 ****************************************************************************/

#ifndef ODYSSEY2_H_
#define ODYSSEY2_H_

#include "streams.h"


#define P1_BANK_LO_BIT            (0x01)
#define P1_BANK_HI_BIT            (0x02)
#define P1_KEYBOARD_SCAN_ENABLE   (0x04)  /* active low */
#define P1_VDC_ENABLE             (0x08)  /* active low */
#define P1_EXT_RAM_ENABLE         (0x10)  /* active low */
#define P1_VDC_COPY_MODE_ENABLE   (0x40)

#define P2_KEYBOARD_SELECT_MASK   (0x07)  /* select row to scan */

#define VDC_CONTROL_REG_STROBE_XY (0x02)

#define I824X_START_ACTIVE_SCAN			6
#define I824X_END_ACTIVE_SCAN			(6 + 160)
#define I824X_START_Y					1
#define I824X_SCREEN_HEIGHT				243
#define I824X_LINE_CLOCKS				228

typedef union {
    UINT8 reg[0x100];
    struct {
	struct {
	    UINT8 y,x,color,res;
	} sprites[4];
	struct {
	    UINT8 y,x,ptr,color;
	} foreground[12];
	struct {
	    struct {
		UINT8 y,x,ptr,color;
	    } single[4];
	} quad[4];
	UINT8 shape[4][8];
	UINT8 control;
	UINT8 status;
	UINT8 collision;
	UINT8 color;
	UINT8 y;
	UINT8 x;
	UINT8 res;
	UINT8 shift1,shift2,shift3;
	UINT8 sound;
	UINT8 res2[5+0x10];
	UINT8 hgrid[2][0x10];
	UINT8 vgrid[0x10];
    } s;
} o2_vdc_t;

typedef struct
{
	UINT8	X;
	UINT8	Y;
	UINT8	Y0;
	UINT8	R;
	UINT8	M;
	UINT8	TA;
	UINT8	TB;
	UINT8	busy;
	UINT8	ram[1024];
} ef9341_t;


class odyssey2_state : public driver_device
{
public:
	odyssey2_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	int the_voice_lrq_state;
	UINT8 *ram;
	UINT8 p1;
	UINT8 p2;
	size_t cart_size;
	o2_vdc_t o2_vdc;
	UINT32 o2_snd_shift[2];
	UINT8 x_beam_pos;
	UINT8 y_beam_pos;
	UINT8 control_status;
	UINT8 collision_status;
	int iff;
	emu_timer *i824x_line_timer;
	emu_timer *i824x_hblank_timer;
	bitmap_t *tmp_bitmap;
	int start_vpos;
	int start_vblank;
	UINT8 lum;
	UINT16 lfsr;
	sound_stream *sh_channel;
	UINT16 sh_count;
	//ef9341_t ef9341;
};


/*----------- defined in video/odyssey2.c -----------*/

extern const UINT8 odyssey2_colors[];

VIDEO_START( odyssey2 );
VIDEO_UPDATE( odyssey2 );
PALETTE_INIT( odyssey2 );
READ8_HANDLER ( odyssey2_t1_r );
READ8_HANDLER ( odyssey2_video_r );
WRITE8_HANDLER ( odyssey2_video_w );
WRITE8_HANDLER ( odyssey2_lum_w );

STREAM_UPDATE( odyssey2_sh_update );

void odyssey2_ef9341_w( running_machine *machine, int command, int b, UINT8 data );
UINT8 odyssey2_ef9341_r( running_machine *machine, int command, int b );

DECLARE_LEGACY_SOUND_DEVICE(ODYSSEY2, odyssey2_sound);

/*----------- defined in machine/odyssey2.c -----------*/

DRIVER_INIT( odyssey2 );
MACHINE_RESET( odyssey2 );

/* i/o ports */
READ8_HANDLER ( odyssey2_bus_r );
WRITE8_HANDLER ( odyssey2_bus_w );

READ8_HANDLER( odyssey2_getp1 );
WRITE8_HANDLER ( odyssey2_putp1 );

READ8_HANDLER( odyssey2_getp2 );
WRITE8_HANDLER ( odyssey2_putp2 );

READ8_HANDLER( odyssey2_getbus );
WRITE8_HANDLER ( odyssey2_putbus );

READ8_HANDLER( odyssey2_t0_r );
void odyssey2_the_voice_lrq_callback( device_t *device, int state );

READ8_HANDLER ( g7400_bus_r );
WRITE8_HANDLER ( g7400_bus_w );

int odyssey2_cart_verify(const UINT8 *cartdata, size_t size);


#endif /* ODYSSEY2_H_ */
