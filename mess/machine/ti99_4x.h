/*
	header file for /machine/ti99_4x.c
*/
#include "driver.h"


/* defines */

/* region identifiers */
enum
{
	region_grom = REGION_USER1,
	region_dsr = REGION_USER2,
	region_speech_rom = REGION_SOUND1
};

/* offsets for REGION_CPU1 */
enum
{
	offset_sram = 0x2000,		/* scratch RAM (256 bytes) */
	offset_cart = 0x2100,		/* cartridge ROM/RAM (2*8 kbytes) */
	offset_xram = 0x6100,		/* extended RAM (32 kbytes - 512kb with myarc-like mapper, 1Mb with super AMS) */
	region_cpu1_len = 0x106100	/* total len */
};

enum
{
	offset_rom0_4p = 0x4000,
	offset_rom4_4p = 0xc000,
	offset_rom6_4p = 0x6000,
	offset_rom6b_4p= 0xe000,
	offset_sram_4p = 0x10000,		/* scratch RAM (1kbyte) */
	offset_cart_4p = 0x10400,		/* cartridge ROM/RAM (2*8 kbytes) */
	offset_xram_4p = 0x12400,		/* extended RAM (32 kbytes - 512kb with myarc-like mapper, 1Mb with super AMS) */
	region_cpu1_len_4p = 0x112400	/* total len */
};


/* offsets for region_dsr */
enum
{
	offset_fdc_dsr = 0x0000,		/* TI FDC DSR (8kbytes) */
	offset_bwg_dsr = 0x2000,		/* BwG FDC DSR (32kbytes) */
	offset_bwg_ram = 0xa000,		/* BwG FDC RAM (2kbytes) */
	offset_rs232_dsr = 0xa800,		/* TI RS232 DSR (4kbytes) */
	offset_evpc_dsr= 0xb800,		/* EVPC DSR (64kbytes) */
	offset_ide_ram = 0x1b800,		/* IDE card RAM (32 to 512kbytes) */
	region_dsr_len = 0x11b800
};

/* enum for RAM config */
typedef enum
{
	xRAM_kind_none = 0,
	xRAM_kind_TI,				/* 32kb TI and clones */
	xRAM_kind_super_AMS,		/* 1Mb super AMS */
	xRAM_kind_foundation_128k,	/* 128kb foundation */
	xRAM_kind_foundation_512k,	/* 512kb foundation */
	xRAM_kind_myarc_128k,		/* 128kb myarc clone (no ROM) */
	xRAM_kind_myarc_512k,		/* 512kb myarc clone (no ROM) */
	xRAM_kind_99_4p_1Mb			/* ti99/4p super AMS clone */
} xRAM_kind_t;

/* enum for fdc config */
typedef enum
{
	fdc_kind_none = 0,
	fdc_kind_TI,				/* TI fdc */
	fdc_kind_BwG				/* SNUG's BwG fdc */
} fdc_kind_t;

/* defines for input ports */
enum
{
	input_port_config = 0,
	input_port_keyboard,
	input_port_caps_lock = input_port_keyboard+4,		/* /4a only */
	input_port_IR_joysticks = input_port_keyboard+4,	/* /4 only */
	input_port_IR_keypads = input_port_IR_joysticks+8	/* /4 only */
};

/* defines for input port input_port_config */
enum
{
	config_xRAM_bit		= 0,
	config_xRAM_mask	= 0x7,	/* 3 bits */
	config_speech_bit	= 3,
	config_speech_mask	= 0x1,
	config_fdc_bit		= 4,
	config_fdc_mask		= 0x3,	/* 2 bits */
	config_rs232_bit	= 6,
	config_rs232_mask	= 0x1,
	/* next option only makes sense for ti99/4 */
	config_handsets_bit	= 7,
	config_handsets_mask= 0x1
};


/* prototypes for machine code */

void init_ti99_4(void);
void init_ti99_4a(void);
void init_ti99_4ev(void);
void init_ti99_4p(void);

void machine_init_ti99(void);
void machine_stop_ti99(void);

int ti99_floppy_load(int id, mame_file *fp, int open_mode);

int ti99_cassette_load(int id, mame_file *fp, int open_mode);

int ti99_rom_load(int id, mame_file *fp, int open_mode);
void ti99_rom_unload(int id);

int video_start_ti99_4(void);
int video_start_ti99_4a(void);
int video_start_ti99_4ev(void);
void ti99_vblank_interrupt(void);
void ti99_4ev_hblank_interrupt(void);

READ16_HANDLER ( ti99_rw_null8bits );
WRITE16_HANDLER ( ti99_ww_null8bits );

READ16_HANDLER ( ti99_rw_cartmem );
WRITE16_HANDLER ( ti99_ww_cartmem );

WRITE16_HANDLER( ti99_ww_wsnd );
READ16_HANDLER ( ti99_rw_rvdp );
WRITE16_HANDLER ( ti99_ww_wvdp );
READ16_HANDLER ( ti99_rw_rv38 );
WRITE16_HANDLER ( ti99_ww_wv38 );
READ16_HANDLER ( ti99_rw_rgpl );
WRITE16_HANDLER( ti99_ww_wgpl );

extern void tms9901_set_int2(int state);


/*
	TI99 peripheral expansion system support
*/

/*
	prototype for CRU handlers in expansion system
*/
typedef int (*cru_read_handler)(int offset);
typedef void (*cru_write_handler)(int offset, int data);

/*
	Descriptor for TI peripheral expansion cards (8-bit bus)
*/
typedef struct ti99_exp_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	mem_read_handler mem_read;		/* card mem read handler (8 bits) */
	mem_write_handler mem_write;	/* card mem write handler (8 bits) */
} ti99_exp_card_handlers_t;

/*
	Descriptor for 16-bit peripheral expansion cards designed by the SNUG for
	use with its SGCPU (a.k.a. 99/4p) system.  (These cards were not designed
	by TI, TI always regarded the TI99 as an 8-bit system.)
*/
typedef struct ti99_4p_exp_16bit_card_handlers_t
{
	cru_read_handler cru_read;		/* card CRU read handler */
	cru_write_handler cru_write;	/* card CRU handler */

	mem_read16_handler mem_read;	/* card mem read handler (16 bits) */
	mem_write16_handler mem_write;	/* card mem write handler (16 bits) */
} ti99_4p_exp_16bit_card_handlers_t;

/* masks for ila and ilb (from actual ILA and ILB registers) */
enum
{
	inta_rs232_1_bit = 0,
	inta_rs232_2_bit = 1,
	inta_rs232_3_bit = 4,
	inta_rs232_4_bit = 5,

	/*inta_rs232_1_mask = (0x80 >> inta_rs232_1_bit),
	inta_rs232_2_mask = (0x80 >> inta_rs232_2_bit),
	inta_rs232_3_mask = (0x80 >> inta_rs232_3_bit),
	inta_rs232_4_mask = (0x80 >> inta_rs232_4_bit),*/

	intb_fdc_bit     = 0,
	intb_ieee488_bit = 1
};

void ti99_exp_set_card_handlers(int cru_base, const ti99_exp_card_handlers_t *handler);
void ti99_4p_exp_set_16bit_card_handlers(int cru_base, const ti99_4p_exp_16bit_card_handlers_t *handler);
void ti99_exp_set_ila_bit(int bit, int state);
void ti99_exp_set_ilb_bit(int bit, int state);

READ16_HANDLER ( ti99_expansion_CRU_r );
WRITE16_HANDLER ( ti99_expansion_CRU_w );
READ16_HANDLER ( ti99_rw_expansion );
WRITE16_HANDLER ( ti99_ww_expansion );

READ16_HANDLER ( ti99_4p_expansion_CRU_r );
WRITE16_HANDLER ( ti99_4p_expansion_CRU_w );
READ16_HANDLER ( ti99_4p_rw_expansion );
WRITE16_HANDLER ( ti99_4p_ww_expansion );
