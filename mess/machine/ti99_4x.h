/*
	header file for /machine/ti99_4x.c
*/


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
	offset_fdc_dsr = 0x0000,
	offset_evpc_dsr= 0x2000,
	region_dsr_len = 0x12000
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

/* defines for input ports */
enum
{
	input_port_config = 0,
	input_port_keyboard,
	input_port_caps_lock = input_port_keyboard+8	/* /4a only */
};

/* defines for input port input_port_config */
enum
{
	config_xRAM_bit		= 0,
	config_xRAM_mask	= 0x7,	/* 3 bits */
	config_speech_bit	= 3,
	config_speech_mask	= 0x1,
	config_fdc_bit		= 4,
	config_fdc_mask		= 0x1
};


/* prototypes for machine code */

void init_ti99_4(void);
void init_ti99_4a(void);
void init_ti99_4ev(void);
void init_ti99_4p(void);

void ti99_init_machine(void);
void ti99_stop_machine(void);

int ti99_floppy_init(int id);

int ti99_cassette_init(int id);
void ti99_cassette_exit(int id);

int ti99_load_rom(int id);
void ti99_rom_cleanup(int id);

int ti99_4_vh_start(void);
int ti99_4a_vh_start(void);
int ti99_4ev_vh_start(void);
int ti99_vblank_interrupt(void);
int ti99_4ev_vblank_interrupt(void);

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

READ16_HANDLER ( ti99_expansion_CRU_r );
WRITE16_HANDLER ( ti99_expansion_CRU_w );

READ16_HANDLER ( ti99_rw_expansion );
WRITE16_HANDLER ( ti99_ww_expansion );


READ16_HANDLER ( ti99_4p_expansion_CRU_r );
WRITE16_HANDLER ( ti99_4p_expansion_CRU_w );
READ16_HANDLER ( ti99_4p_rw_expansion );
WRITE16_HANDLER ( ti99_4p_ww_expansion );
