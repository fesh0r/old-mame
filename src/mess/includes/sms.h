/*****************************************************************************
 *
 * includes/sms.h
 *
 ****************************************************************************/

#ifndef SMS_H_
#define SMS_H_

#define LOG_REG
#define LOG_PAGING
#define LOG_COLOR

#define NVRAM_SIZE             (0x08000)
#define CPU_ADDRESSABLE_SIZE   (0x10000)

#define MAX_CARTRIDGES        16

class sms_state : public driver_device
{
public:
	sms_state(running_machine &machine, const driver_device_config_base &config)
	: driver_device(machine, config) { }

	// device_ts
	device_t *main_cpu;
	device_t *control_cpu;
	device_t *vdp;
	device_t *ym;
	device_t *main_scr;
	device_t *left_lcd;
	device_t *right_lcd;

	UINT8 bios_page_count;
	UINT8 fm_detect;
	UINT8 ctrl_reg;
	int paused;
	UINT8 bios_port;
	UINT8 *BIOS;
	UINT8 *mapper_ram;
	UINT8 mapper[4];
	// we are going to use 1-6, same as bank numbers. Notice, though, that most mappers
	// only work on 16K banks and, hence, banks 4-6 are not always directly set
	// (they often use bank3 + 0x2000 and bank5 + 0x2000)
	UINT8 *banking_bios[7];
	UINT8 *banking_cart[7];
	UINT8 *banking_none[7];
	UINT8 gg_sio[5];
	UINT8 store_control;
	UINT8 input_port0;
	UINT8 input_port1;
	
	// for gamegear LCD persistence hack
	bitmap_t *tmp_bitmap;
	bitmap_t *prev_bitmap;
	
	// for 3D glass binocular hack
	bitmap_t *prevleft_bitmap;
	bitmap_t *prevright_bitmap;
	
	/* Model identifiers */
	UINT8 is_gamegear;
	UINT8 is_region_japan;
	UINT8 has_bios_0400;
	UINT8 has_bios_2000;
	UINT8 has_bios_full;
	UINT8 has_bios;
	UINT8 has_fm;
	
	/* Data needed for Rapid Fire Unit support */
	emu_timer *rapid_fire_timer;
	UINT8 rapid_fire_state_1;
	UINT8 rapid_fire_state_2;
	
	/* Data needed for Paddle Control controller */
	UINT32 last_paddle_read_time;
	UINT8 paddle_read_state;
	
	/* Data needed for Sports Pad controller */
	UINT32 last_sports_pad_time_1;
	UINT32 last_sports_pad_time_2;
	UINT8 sports_pad_state_1;
	UINT8 sports_pad_state_2;
	UINT8 sports_pad_last_data_1;
	UINT8 sports_pad_last_data_2;
	UINT8 sports_pad_1_x;
	UINT8 sports_pad_1_y;
	UINT8 sports_pad_2_x;
	UINT8 sports_pad_2_y;
	
	/* Data needed for Light Phaser */
	emu_timer *lphaser_1_timer;
	emu_timer *lphaser_2_timer;
	UINT8 lphaser_1_latch;
	UINT8 lphaser_2_latch;
	int lphaser_x_offs;	/* Needed to 'calibrate' lphaser; set at cart loading */
	
	/* Data needed for SegaScope (3D glasses) */
	UINT8 sscope_state;
	
	/* Data needed for Terebi Oekaki (TV Draw) */
	UINT8 tvdraw_data;
	
	/* Cartridge slot info */
	UINT8 current_cartridge;
	struct
	{
		UINT8 *ROM;        /* Pointer to ROM image data */
		UINT32 size;       /* Size of the ROM image */
		UINT8 features;    /* on-cartridge special hardware */
		UINT8 *cartSRAM;   /* on-cartridge SRAM */
		UINT8 sram_save;   /* should be the contents of the on-cartridge SRAM be saved */
		UINT8 *cartRAM;    /* additional on-cartridge RAM (64KB for Ernie Els Golf) */
		UINT32 ram_size;   /* size of the on-cartridge RAM */
		UINT8 ram_page;    /* currently swapped in cartridge RAM */
	} cartridge[MAX_CARTRIDGES];
};


/*----------- defined in machine/sms.c -----------*/

/* Function prototypes */
WRITE8_HANDLER( sms_cartram_w );
WRITE8_HANDLER( sms_cartram2_w );
WRITE8_HANDLER( sms_fm_detect_w );
READ8_HANDLER( sms_fm_detect_r );
READ8_HANDLER( sms_input_port_0_r );
READ8_HANDLER( sms_input_port_1_r );
WRITE8_HANDLER( sms_ym2413_register_port_0_w );
WRITE8_HANDLER( sms_ym2413_data_port_0_w );
WRITE8_HANDLER( sms_io_control_w );
READ8_HANDLER( sms_count_r );
WRITE8_HANDLER( sms_sscope_w );
READ8_HANDLER( sms_sscope_r );
WRITE8_HANDLER( sms_mapper_w );
READ8_HANDLER( sms_mapper_r );
WRITE8_HANDLER( sms_bios_w );
WRITE8_HANDLER( gg_sio_w );
READ8_HANDLER( gg_sio_r );
READ8_HANDLER( gg_input_port_2_r );

INPUT_CHANGED( lgun1_changed );
INPUT_CHANGED( lgun2_changed );

void sms_pause_callback( running_machine *machine );
void sms_store_int_callback( running_machine *machine, int state );

DEVICE_START( sms_cart );
DEVICE_IMAGE_LOAD( sms_cart );

MACHINE_START( sms );
MACHINE_RESET( sms );

READ8_HANDLER( sms_store_cart_select_r );
WRITE8_HANDLER( sms_store_cart_select_w );
READ8_HANDLER( sms_store_select1 );
READ8_HANDLER( sms_store_select2 );
READ8_HANDLER( sms_store_control_r );
WRITE8_HANDLER( sms_store_control_w );

#define IO_EXPANSION    (0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE    (0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD         (0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM     (0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM     (0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP         (0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */


DRIVER_INIT( sg1000m3 );
DRIVER_INIT( sms1 );
DRIVER_INIT( smsj );
DRIVER_INIT( sms2kr );
DRIVER_INIT( smssdisp );
DRIVER_INIT( gamegear );
DRIVER_INIT( gamegeaj );

VIDEO_START( sms1 );
VIDEO_START( gamegear );
VIDEO_UPDATE( sms1 );
VIDEO_UPDATE( sms );
VIDEO_UPDATE( gamegear );

#endif /* SMS_H_ */
