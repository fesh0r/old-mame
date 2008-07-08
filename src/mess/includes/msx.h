/*****************************************************************************
 *
 * includes/msx.h
 *
 ****************************************************************************/

#ifndef MSX_H_
#define MSX_H_


#define MSX_MAX_CARTS	(2)

typedef struct {
	/* PSG */
	int psg_b;
	int opll_active;
	/* printer */
	UINT8 prn_data, prn_strobe;
	/* mouse */
	UINT16 mouse[2];
	int mouse_stat[2];
	/* rtc */
	int rtc_latch;
	/* disk */
	UINT8 dsk_stat;
	/* kanji */
	UINT8 *kanji_mem;
	int kanji_latch;
	/* memory */
	msx_slot_layout *layout;
	slot_state *cart_state[MSX_MAX_CARTS];
	slot_state *state[4];
	const msx_slot *slot[4];
	UINT8 *ram_pages[4];
	UINT8 *empty, ram_mapper[4];
	UINT8 ramio_set_bits;
	slot_state *all_state[4][4][4];
	int slot_expanded[4];
	UINT8 primary_slot;
	UINT8 secondary_slot[4];
	UINT8 superloadrunner_bank;
	UINT8 korean90in1_bank;
	UINT8 *top_page;
} MSX;


/*----------- defined in machine/msx.c -----------*/

extern MSX msx1;
extern const ppi8255_interface msx_ppi8255_interface;

/* start/stop functions */
extern DRIVER_INIT( msx );
extern DRIVER_INIT( msx2 );
extern MACHINE_START( msx );
extern MACHINE_START( msx2 );
extern MACHINE_RESET( msx );
extern MACHINE_RESET( msx2 );
extern INTERRUPT_GEN( msx_interrupt );
extern INTERRUPT_GEN( msx2_interrupt );
extern NVRAM_HANDLER( msx2 );

DEVICE_IMAGE_LOAD( msx_cart );
DEVICE_IMAGE_UNLOAD( msx_cart );

void msx_vdp_interrupt(running_machine *machine, int i);

/* I/O functions */
WRITE8_HANDLER ( msx_printer_w );
 READ8_HANDLER ( msx_printer_r );
WRITE8_HANDLER ( msx_psg_w );
 READ8_HANDLER ( msx_psg_r );
WRITE8_HANDLER ( msx_psg_port_a_w );
 READ8_HANDLER ( msx_psg_port_a_r );
WRITE8_HANDLER ( msx_psg_port_b_w );
 READ8_HANDLER ( msx_psg_port_b_r );
WRITE8_HANDLER ( msx_fmpac_w );
 READ8_HANDLER ( msx_rtc_reg_r );
WRITE8_HANDLER ( msx_rtc_reg_w );
WRITE8_HANDLER ( msx_rtc_latch_w );
WRITE8_HANDLER ( msx_90in1_w );

/* disk functions */
DEVICE_IMAGE_LOAD( msx_floppy );

/* new memory emulation */
WRITE8_HANDLER (msx_superloadrunner_w);
WRITE8_HANDLER (msx_page0_w);
WRITE8_HANDLER (msx_page1_w);
WRITE8_HANDLER (msx_page2_w);
WRITE8_HANDLER (msx_page3_w);
WRITE8_HANDLER (msx_sec_slot_w);
 READ8_HANDLER (msx_sec_slot_r);
WRITE8_HANDLER (msx_ram_mapper_w);
 READ8_HANDLER (msx_ram_mapper_r);
 READ8_HANDLER (msx_kanji_r);
WRITE8_HANDLER (msx_kanji_w);

void msx_memory_map_all (running_machine *machine);
void msx_memory_map_page (running_machine *machine, int page);
void msx_memory_init (running_machine *machine);
void msx_memory_set_carts (void);
void msx_memory_reset (running_machine *machine);


#endif /* MSX_H_ */
