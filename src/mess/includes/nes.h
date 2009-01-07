/*****************************************************************************
 *
 * includes/nes.h
 *
 * Nintendo Entertainment System (Famicom)
 *
 ****************************************************************************/

#ifndef NES_H_
#define NES_H_


#define NTSC_CLOCK		N2A03_DEFAULTCLOCK	/* 1.789772 MHz */
#define PAL_CLOCK		(26601712.0/16)		/* 1.662607 MHz */


/*----------- defined in machine/nes.c -----------*/

void ppu_mirror_custom (int page, int address);
void ppu_mirror_custom_vrom (int page, int address);

WRITE8_HANDLER ( nes_IN0_w );
WRITE8_HANDLER ( nes_IN1_w );

extern unsigned char *battery_ram;

struct nes_struct
{
	/* load-time cart variables which remain constant */
	UINT8 trainer;
	UINT8 battery;
	UINT8 prg_chunks;
	UINT8 chr_chunks;

	/* system variables which don't change at run-time */
	UINT16 mapper;
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 slow_banking;

	UINT8 *rom;
	UINT8 *vrom;
	UINT8 *vram;
	UINT8 *wram;

	/* Variables which can change */
	UINT8 mid_ram_enable;
};

extern struct nes_struct nes;

struct fds_struct
{
	UINT8 *data;
	UINT8 sides;
	UINT8 *ram;

	/* Variables which can change */
	UINT8 motor_on;
	UINT8 door_closed;
	UINT8 current_side;
	UINT32 head_position;
	UINT8 status0;
	UINT8 read_mode;
	UINT8 write_reg;
};

extern struct fds_struct nes_fds;

/* protos */

DEVICE_IMAGE_LOAD(nes_cart);
DEVICE_START(nes_disk);
DEVICE_IMAGE_LOAD(nes_disk);
DEVICE_IMAGE_UNLOAD(nes_disk);

MACHINE_START( nes );
MACHINE_RESET( nes );

READ8_HANDLER( nes_IN0_r );
READ8_HANDLER( nes_IN1_r );

int nes_ppu_vidaccess( running_machine *machine, int num, int address, int data );

void nes_partialhash(char *dest, const unsigned char *data,
	unsigned long length, unsigned int functions);


/*----------- defined in video/nes.c -----------*/

extern int nes_vram_sprite[8];

PALETTE_INIT( nes );
VIDEO_START( nes_ntsc );
VIDEO_START( nes_pal );
VIDEO_UPDATE( nes );


#endif /* NES_H_ */
