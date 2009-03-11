/*****************************************************************************
 *
 * includes/sms.h
 *
 ****************************************************************************/

#ifndef SMS_H_
#define SMS_H_

#define LOG_REG
#define LOG_PAGING
//#define LOG_CURLINE
#define LOG_COLOR

#define NVRAM_SIZE								(0x8000)
#define CPU_ADDRESSABLE_SIZE			(0x10000)


#define FLAG_GAMEGEAR			0x00020000
#define FLAG_BIOS_0400			0x00040000
#define FLAG_BIOS_2000			0x00080000
#define FLAG_BIOS_FULL			0x00100000
#define FLAG_FM				0x00200000
#define FLAG_REGION_JAPAN		0x00400000

#define IS_GAMEGEAR			( space->machine->gamedrv->flags & FLAG_GAMEGEAR )
#define HAS_BIOS_0400			( space->machine->gamedrv->flags & FLAG_BIOS_0400 )
#define HAS_BIOS_2000			( space->machine->gamedrv->flags & FLAG_BIOS_2000 )
#define HAS_BIOS_FULL			( space->machine->gamedrv->flags & FLAG_BIOS_FULL )
#define HAS_BIOS			( space->machine->gamedrv->flags & ( FLAG_BIOS_0400 | FLAG_BIOS_2000 | FLAG_BIOS_FULL ) )
#define HAS_FM				( space->machine->gamedrv->flags & FLAG_FM )
#define IS_REGION_JAPAN			( space->machine->gamedrv->flags & FLAG_REGION_JAPAN )


/*----------- defined in machine/sms.c -----------*/

/* Function prototypes */
WRITE8_HANDLER(sms_cartram_w);
WRITE8_HANDLER(sms_cartram2_w);
WRITE8_HANDLER(sms_fm_detect_w);
 READ8_HANDLER(sms_fm_detect_r);
 READ8_HANDLER(sms_input_port_0_r);
WRITE8_HANDLER(sms_ym2413_register_port_0_w);
WRITE8_HANDLER(sms_ym2413_data_port_0_w);
WRITE8_HANDLER(sms_version_w);
 READ8_HANDLER(sms_version_r);
 READ8_HANDLER(sms_count_r);
WRITE8_HANDLER(sms_mapper_w);
 READ8_HANDLER(sms_mapper_r);
WRITE8_HANDLER(sms_bios_w);
WRITE8_HANDLER(gg_sio_w);
 READ8_HANDLER(gg_sio_r);
 READ8_HANDLER(gg_psg_r);
WRITE8_HANDLER(gg_psg_w);
 READ8_HANDLER(gg_input_port_2_r);

void sms_check_pause_button(const address_space *space);

DEVICE_START( sms_cart );
DEVICE_IMAGE_LOAD( sms_cart );

MACHINE_START(sms);
MACHINE_RESET(sms);

READ8_HANDLER(sms_store_cart_select_r);
WRITE8_HANDLER(sms_store_cart_select_w);
READ8_HANDLER(sms_store_select1);
READ8_HANDLER(sms_store_select2);
READ8_HANDLER(sms_store_control_r);
WRITE8_HANDLER(sms_store_control_w);
void sms_int_callback( running_machine *machine, int state );
void sms_store_int_callback( running_machine *machine, int state );

#define IO_EXPANSION				(0x80)	/* Expansion slot enable (1= disabled, 0= enabled) */
#define IO_CARTRIDGE				(0x40)	/* Cartridge slot enable (1= disabled, 0= enabled) */
#define IO_CARD							(0x20)	/* Card slot disabled (1= disabled, 0= enabled) */
#define IO_WORK_RAM					(0x10)	/* Work RAM disabled (1= disabled, 0= enabled) */
#define IO_BIOS_ROM					(0x08)	/* BIOS ROM disabled (1= disabled, 0= enabled) */
#define IO_CHIP							(0x04)	/* I/O chip disabled (1= disabled, 0= enabled) */


#endif /* SMS_H_ */
