/*****************************************************************************
 *
 * includes/nes.h
 *
 * Nintendo Entertainment System (Famicom)
 *
 ****************************************************************************/

#ifndef NES_H_
#define NES_H_


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define NTSC_CLOCK           N2A03_DEFAULTCLOCK     /* 1.789772 MHz */
#define PAL_CLOCK	           (26601712.0/16)        /* 1.662607 MHz */

#define NES_BATTERY_SIZE 0x2000


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

struct nes_input
{
	UINT32 shift;
	UINT32 i0, i1, i2;
};

/*PPU fast banking constants and structures */

#define CHRROM 0
#define CHRRAM 1

typedef struct
{
	int source;	//defines source of base pointer
	int origin; //defines offset of 0x400 byte segment at base pointer
	UINT8* access;	//source translated + origin -> valid pointer!
} chr_bank;

/*PPU nametable fast banking constants and structures */

#define CIRAM 0
#define ROM 1
#define EXRAM 2
#define MMC5FILL 3
#define CART_NTRAM 4

#define NES_BATTERY 0
#define NES_WRAM 1

typedef struct
{
	int source;		/* defines source of base pointer */
	int origin;		/* defines offset of 0x400 byte segment at base pointer */
	int writable;	/* ExRAM, at least, can be write-protected AND used as nametable */
	UINT8* access;	/* direct access when possible */
} name_table;

typedef void (*nes_prg_callback)(running_machine *machine, int start, int bank);
typedef void (*nes_chr_callback)(running_machine *machine, int start, int bank, int source);

class nes_state
{
public:
	static void *alloc(running_machine &machine) { return auto_alloc_clear(&machine, nes_state(machine)); }

	nes_state(running_machine &machine) { }

	/* input_related - this part has to be cleaned up (e.g. in_2 and in_3 are not really necessary here...) */
	nes_input in_0, in_1, in_2, in_3;
	UINT8 fck_scan, fck_mode;

	int           prg_bank[5];
	chr_bank      chr_map[8];  //quick banking structure, because some of this changes multiple times per scanline!
	name_table    nt_page[4];  //quick banking structure for a maximum of 4K of RAM/ROM/ExRAM

	int chr_open_bus;
	int prgram_bank5_start, battery_bank5_start, empty_bank5_start;

	UINT8 ce_mask, ce_state;
	UINT8 vrc_ls_prg_a, vrc_ls_prg_b, vrc_ls_chr;

	int MMC5_floodtile;
	int MMC5_floodattr;
	int mmc5_vram_control;
	UINT8 mmc5_high_chr;
	UINT8 mmc5_split_scr;
	UINT8 *extended_ntram;
	
	UINT8 mmc5_last_chr_a;
	UINT16 mmc5_vrom_regA[8];
	UINT16 mmc5_vrom_regB[4];
	UINT8 mmc5_prg_regs[4];
	UINT8 mmc5_bank_security;
	UINT8 mmc5_prg_mode;
	UINT8 mmc5_chr_mode;
	UINT8 mmc5_chr_high;
	UINT8 mmc5_split_ctrl;
	UINT8 mmc5_split_yst;
	UINT8 mmc5_split_bank;

	/* video-related */
	int nes_vram_sprite[8]; /* Used only by mmc5 for now */
	int last_frame_flip;

	/* misc */
	write8_space_func   mmc_write_low;
	write8_space_func   mmc_write_mid;
	write8_space_func   mmc_write;
	read8_space_func    mmc_read_low;
	read8_space_func    mmc_read_mid;
	read8_space_func    mmc_read;
	emu_timer	        *irq_timer;

	nes_prg_callback    mmc3_prg_cb;	// these are used to simplify a lot emulation of some MMC3 pirate clones
	nes_chr_callback    mmc3_chr_cb;

	/* devices */
	running_device      *maincpu;
	running_device      *ppu;
	running_device      *sound;
	running_device      *cart;

	/* misc region to be allocated at init */
	// variables which don't change at run-time
	UINT8      *rom;
	UINT8      *vrom;
	UINT8      *vram;
	UINT8      *wram;
	UINT8      *ciram; //PPU nametable RAM - external to PPU!
	UINT8      *battery_ram;
	UINT8      *mapper_ram;
	UINT8      *mapper_bram;

	/***** Mapper-related variables *****/

	// common ones
	int        IRQ_enable, IRQ_enable_latch;
	UINT16     IRQ_count, IRQ_count_latch;
	UINT8      IRQ_toggle;
	UINT8      IRQ_reset;
	UINT8      IRQ_status;
	UINT8      IRQ_mode;
	UINT8      IRQ_clear;
	int        mult1, mult2;

	UINT8 mmc_chr_source;			// This is set at init to CHRROM or CHRRAM. a few mappers can swap between
									// the two (this is done in the specific handlers).

	UINT8 mmc_cmd1, mmc_cmd2;		// These represent registers where the mapper writes important values
	UINT8 mmc_count;				// This is used as counter in mappers like 1 and 45

	int mmc_prg_base, mmc_prg_mask;	// MMC3 based multigame carts select a block of banks by using these (and then act like normal MMC3),
	int mmc_chr_base, mmc_chr_mask;	// while MMC3 and clones (mapper 118 & 119) simply set them as 0 and 0xff resp.

	UINT8 mmc_prg_bank[6];				// Many mappers writes only some bits of the selected bank (for both PRG and CHR),
	UINT8 mmc_vrom_bank[16];			// hence these are handy to latch bank values.

	UINT16 MMC5_vrom_bank[12];			// MMC5 has 10bit wide VROM regs!
	UINT8 mmc_extra_bank[16];			// some MMC3 clone have 2 series of PRG/CHR banks...
										// we collect them all here: first 4 elements PRG banks, then 6/8 CHR banks

	UINT8 mmc_latch1, mmc_latch2;
	UINT8 mmc_reg[16];

	UINT8 mmc_dipsetting;

	// misc mapper related variables which should be merged with the above one, where possible
	int mmc1_reg_write_enable;
	int mmc1_latch;
	int mmc1_count;

	int mmc3_latch;
	int mmc3_wram_protect;
	int mmc3_alt_irq;

	int MMC5_rom_bank_mode;
	int MMC5_vrom_bank_mode;
	int MMC5_vram_protect;
	int MMC5_scanline;
	int vrom_page_a;
	int vrom_page_b;
	// int vrom_next[4];

	UINT8 mmc6_reg;

	// these might be unified in single mmc_reg[] array, together with state->mmc_cmd1 & state->mmc_cmd2
	// but be careful that MMC3 clones often use state->mmc_cmd1/state->mmc_cmd2 (from base MMC3) AND additional regs below!
	UINT8 mapper83_reg[10];
	UINT8 mapper83_low_reg[4];
	UINT8 txc_reg[4];	// used by mappers 132, 172 & 173
	UINT8 subor_reg[4];	// used by mappers 166 & 167
	UINT8 sachen_reg[8];	// used by mappers 137, 138, 139 & 141
	UINT8 map52_reg_written;
	UINT8 map114_reg, map114_reg_enabled;

	/***** NES-cart related *****/

	/* load-time cart variables which remain constant */
	UINT16 prg_chunks;		// iNES 2.0 allows for more chunks (a recently dumped multigame cart has 256 chunks of both PRG & CHR!)
	UINT16 chr_chunks;
	UINT16 vram_chunks;
	UINT8 trainer;
	UINT8 battery;			// if there is PRG RAM with battery backup
	UINT32 battery_size;
	UINT8 prg_ram;			// if there is PRG RAM with no backup
	UINT32 wram_size;
	UINT32 mapper_ram_size;
	UINT32 mapper_bram_size;

	/* system variables which don't change at run-time */
	UINT16 mapper;		// for iNES
	UINT16 pcb_id;		// for UNIF & xml
	UINT8 four_screen_vram;
	UINT8 hard_mirroring;
	UINT8 crc_hack;	// this is needed to detect different boards sharing the same Mappers (shame on .nes format)
	UINT8 ines20;

	/***** FDS-floppy related *****/

	int     disk_expansion;

	UINT8   fds_sides;
	UINT8   *fds_data;	// here, we store a copy of the disk
	UINT8   *fds_ram;	// here, we emulate the RAM adapter

	/* Variables which can change */
	UINT8   fds_motor_on;
	UINT8   fds_door_closed;
	UINT8   fds_current_side;
	UINT32  fds_head_position;
	UINT8   fds_status0;
	UINT8   fds_read_mode;
	UINT8   fds_write_reg;

	/* these are used in the mapper 20 handlers */
	int     fds_last_side;
	int     fds_count;
};


/*----------- defined in machine/nes.c -----------*/

WRITE8_HANDLER ( nes_IN0_w );
WRITE8_HANDLER ( nes_IN1_w );

/* protos */

DEVICE_IMAGE_LOAD(nes_cart);
DEVICE_START(nes_disk);
DEVICE_IMAGE_LOAD(nes_disk);
DEVICE_IMAGE_UNLOAD(nes_disk);

MACHINE_START( nes );
MACHINE_RESET( nes );

DRIVER_INIT( famicom );

READ8_HANDLER( nes_IN0_r );
READ8_HANDLER( nes_IN1_r );

int nes_ppu_vidaccess( running_device *device, int address, int data );

void nes_partialhash(char *dest, const unsigned char *data, unsigned long length, unsigned int functions);

/*----------- defined in video/nes.c -----------*/

extern int nes_vram_sprite[8];

PALETTE_INIT( nes );
VIDEO_START( nes );
VIDEO_UPDATE( nes );


#endif /* NES_H_ */
