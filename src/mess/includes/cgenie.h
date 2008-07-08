/*****************************************************************************
 *
 * includes/cgenie.h
 *
 ****************************************************************************/

#ifndef CGENIE_H_
#define CGENIE_H_


/*----------- defined in audio/cgenie.c -----------*/

WRITE8_HANDLER ( cgenie_sh_control_port_w );
WRITE8_HANDLER ( cgenie_sh_data_port_w );
READ8_HANDLER ( cgenie_sh_control_port_r );
READ8_HANDLER ( cgenie_sh_data_port_r );


/*----------- defined in machine/cgenie.c -----------*/

extern UINT8 *cgenie_fontram;

DEVICE_IMAGE_LOAD( cgenie_cassette );
DEVICE_IMAGE_LOAD( cgenie_floppy );

extern int cgenie_tv_mode;

READ8_HANDLER ( cgenie_psg_port_a_r);
READ8_HANDLER ( cgenie_psg_port_b_r );
WRITE8_HANDLER ( cgenie_psg_port_a_w );
WRITE8_HANDLER ( cgenie_psg_port_b_w );

MACHINE_START( cgenie );
MACHINE_RESET( cgenie );

READ8_HANDLER ( cgenie_colorram_r );
READ8_HANDLER ( cgenie_fontram_r );

WRITE8_HANDLER ( cgenie_colorram_w );
WRITE8_HANDLER ( cgenie_fontram_w );

WRITE8_HANDLER ( cgenie_port_ff_w );
 READ8_HANDLER ( cgenie_port_ff_r );
int cgenie_port_xx_r(int offset);

INTERRUPT_GEN( cgenie_timer_interrupt );
INTERRUPT_GEN( cgenie_frame_interrupt );

 READ8_HANDLER ( cgenie_status_r );
 READ8_HANDLER ( cgenie_track_r );
 READ8_HANDLER ( cgenie_sector_r );
 READ8_HANDLER ( cgenie_data_r );

WRITE8_HANDLER ( cgenie_command_w );
WRITE8_HANDLER ( cgenie_track_w );
WRITE8_HANDLER ( cgenie_sector_w );
WRITE8_HANDLER ( cgenie_data_w );

 READ8_HANDLER ( cgenie_irq_status_r );

WRITE8_HANDLER ( cgenie_motor_w );

 READ8_HANDLER ( cgenie_keyboard_r );
int cgenie_videoram_r(int offset);
WRITE8_HANDLER ( cgenie_videoram_w );

// CRTC 6845
typedef struct
{         
	UINT8    cursor_address_lo;
	UINT8    cursor_address_hi;
	UINT8    screen_address_lo;
	UINT8    screen_address_hi;
	UINT8    cursor_bottom;
	UINT8    cursor_top;
	UINT8    scan_lines;
	UINT8    crt_mode;
	UINT8    vertical_sync_pos;
	UINT8    vertical_displayed;
	UINT8    vertical_adjust;
	UINT8    vertical_total;
	UINT8    horizontal_length;
	UINT8    horizontal_sync_pos;
	UINT8    horizontal_displayed;
	UINT8    horizontal_total;
	UINT8    idx;
	UINT8    cursor_visible;
	UINT8    cursor_phase;
} CRTC6845;


/*----------- defined in video/cgenie.c -----------*/

extern	int 	cgenie_font_offset[4];

VIDEO_START( cgenie );
VIDEO_UPDATE( cgenie );

extern	 READ8_HANDLER ( cgenie_index_r );
extern	 READ8_HANDLER ( cgenie_register_r );

extern	WRITE8_HANDLER ( cgenie_index_w );
extern	WRITE8_HANDLER (	cgenie_register_w );

extern	int 	cgenie_get_register(int indx);

extern	void	cgenie_mode_select(int graphics);


#endif /* CGENIE_H_ */
