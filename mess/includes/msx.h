/*
** msx.h : part of MSX1 emulation.
**
** By Sean Young 1999
*/

#define MSX_MAX_CARTS   (2)

typedef struct {
    int type,bank_mask,banks[4];
    UINT8 *mem;
    char *sramfile;
    int pacsram;
} MSX_CART;

typedef struct {
    int run; /* set after init_msx () */
    /* PSG */
    int psg_b,opll_active;
    /* memory */
    UINT8 *empty, *ram, ramp[4];
    /* memory status */
    MSX_CART cart[MSX_MAX_CARTS];
	/* printer */
	UINT8 prn_data, prn_strobe;
	/* mouse */
	UINT16 mouse[2];
    int mouse_stat[2];
	/* rtc */
	int rtc_latch;
	/* disk */
    UINT8 dsk_stat, *disk;
} MSX;

/* start/stop functions */
void init_msx(void);
void init_msx2(void);
void msx_ch_reset (void);
void msx2_ch_reset (void);
void msx_ch_stop (void);
void msx2_ch_stop (void);
int msx_load_rom (int id);
void msx_exit_rom (int id);
int msx_interrupt (void);
int msx2_interrupt (void);
void msx_vdp_interrupt (int);
void msx2_nvram (void *file, int write);

/* I/O functions */
WRITE_HANDLER ( msx_printer_w );
READ_HANDLER ( msx_printer_r );
WRITE_HANDLER ( msx_psg_w );
WRITE_HANDLER ( msx_dsk_w );
READ_HANDLER ( msx_psg_r );
WRITE_HANDLER ( msx_psg_port_a_w );
READ_HANDLER ( msx_psg_port_a_r );
WRITE_HANDLER ( msx_psg_port_b_w );
READ_HANDLER ( msx_psg_port_b_r );
WRITE_HANDLER ( msx_fmpac_w );
READ_HANDLER ( msx_rtc_reg_r );
WRITE_HANDLER ( msx_rtc_reg_w );
WRITE_HANDLER ( msx_rtc_latch_w );
WRITE_HANDLER ( msx_mapper_w );
READ_HANDLER ( msx_mapper_r );

/* memory functions */
WRITE_HANDLER ( msx_writemem0 );
WRITE_HANDLER ( msx_writemem1 );
WRITE_HANDLER ( msx_writemem2 );
WRITE_HANDLER ( msx_writemem3 );

/* cassette functions */
int msx_cassette_init (int id);
void msx_cassette_exit (int id);

/* disk functions */
int msx_floppy_init (int id);


