/*****************************************************************************
 *
 * includes/lisa.h
 *
 * Lisa driver declarations
 *
 ****************************************************************************/

#ifndef LISA_H_
#define LISA_H_

#include "machine/6522via.h"

/* lisa MMU segment regs */
typedef struct real_mmu_entry
{
	UINT16 sorg;
	UINT16 slim;
} real_mmu_entry;

/* MMU regs translated into a more efficient format */
enum mmu_entry_t { RAM_stack_r, RAM_r, RAM_stack_rw, RAM_rw, IO, invalid, special_IO };

typedef struct mmu_entry
{
	offs_t sorg;	/* (real_sorg & 0x0fff) << 9 */
	mmu_entry_t type;	/* <-> (real_slim & 0x0f00) */
	int slim;	/* (~ ((real_slim & 0x00ff) << 9)) & 0x01ffff */
} mmu_entry;

enum floppy_hardware_t
{
	twiggy,			/* twiggy drives (Lisa 1) */
	sony_lisa2,		/* 3.5'' drive with LisaLite adapter (Lisa 2) */
	sony_lisa210	/* 3.5'' drive with modified fdc hardware (Lisa 2/10, Mac XL) */
};

enum clock_mode_t
{
	clock_timer_disable = 0,
	timer_disable = 1,
	timer_interrupt = 2,	/* timer underflow generates interrupt */
	timer_power_on = 3		/* timer underflow turns system on if it is off and gens interrupt */
};			/* clock mode */

/* clock registers */
typedef struct _clock_regs_t
{
	long alarm;		/* alarm (20-bit binary) */
	int years;		/* years (4-bit binary ) */
	int days1;		/* days (BCD : 1-366) */
	int days2;
	int days3;
	int hours1;		/* hours (BCD : 0-23) */
	int hours2;
	int minutes1;	/* minutes (BCD : 0-59) */
	int minutes2;
	int seconds1;	/* seconds (BCD : 0-59) */
	int seconds2;
	int tenths;		/* tenths of second (BCD : 0-9) */

	int clock_write_ptr;	/* clock byte to be written next (-1 if clock write disabled) */

	enum clock_mode_t clock_mode;
} clock_regs_t;

typedef struct _lisa_features_t
{
	unsigned int has_fast_timers : 1;	/* I/O board VIAs are clocked at 1.25 MHz (?) instead of .5 MHz (?) (Lisa 2/10, Mac XL) */
										/* Note that the beep routine in boot ROMs implies that
                                        VIA clock is 1.25 times faster with fast timers than with
                                        slow timers.  I read the schematics again and again, and
                                        I simply don't understand : in one case the VIA is
                                        connected to the 68k E clock, which is CPUCK/10, and in
                                        another case, to a generated PH2 clock which is CPUCK/4,
                                        with additionnal logic to keep it in phase with the 68k
                                        memory cycle.  After hearing the beep when MacWorks XL
                                        boots, I bet the correct values are .625 MHz and .5 MHz.
                                        Maybe the schematics are wrong, and PH2 is CPUCK/8.
                                        Maybe the board uses a 6522 variant with different
                                        timings. */
	floppy_hardware_t floppy_hardware;
	unsigned int has_double_sided_floppy : 1;	/* true on lisa 1 and *hacked* lisa 2/10 / Mac XL */
	unsigned int has_mac_xl_video : 1;	/* modified video for MacXL */
} lisa_features_t;


class lisa_state : public driver_device
{
public:
	lisa_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

	UINT8 *fdc_rom;
	UINT8 *fdc_ram;
	UINT8 *ram_ptr;
	UINT8 *rom_ptr;
	UINT8 *videoROM_ptr;
	int setup;
	int seg;
	real_mmu_entry real_mmu_regs[4][128];
	mmu_entry mmu_regs[4][128];
	int diag2;
	int test_parity;
	UINT16 mem_err_addr_latch;
	int parity_error_pending;
	int bad_parity_count;
	UINT8 *bad_parity_table;
	int VTMSK;
	int VTIR;
	UINT16 video_address_latch;
	UINT16 *videoram_ptr;
	int KBIR;
	int FDIR;
	int DISK_DIAG;
	int MT1;
	int PWM_floppy_motor_speed;
	int model;
	lisa_features_t features;
	int COPS_Ready;
	int COPS_command;
	int fifo_data[8];
	int fifo_size;
	int fifo_head;
	int fifo_tail;
	int mouse_data_offset;
	int COPS_force_unplug;
	emu_timer *mouse_timer;
	int hold_COPS_data;
	int NMIcode;
	clock_regs_t clock_regs;
	int key_matrix[8];
	int last_mx;
	int last_my;
	int frame_count;
	int videoROM_address;
};


/*----------- defined in machine/lisa.c -----------*/

extern const via6522_interface lisa_via6522_0_intf;
extern const via6522_interface lisa_via6522_1_intf;

VIDEO_START( lisa );
VIDEO_UPDATE( lisa );

extern NVRAM_HANDLER(lisa);

DRIVER_INIT( lisa2 );
DRIVER_INIT( lisa210 );
DRIVER_INIT( mac_xl );

MACHINE_START( lisa );
MACHINE_RESET( lisa );

INTERRUPT_GEN( lisa_interrupt );

READ8_HANDLER ( lisa_fdc_io_r );
WRITE8_HANDLER ( lisa_fdc_io_w );
READ8_HANDLER ( lisa_fdc_r );
READ8_HANDLER ( lisa210_fdc_r );
WRITE8_HANDLER ( lisa_fdc_w );
WRITE8_HANDLER ( lisa210_fdc_w );
READ16_HANDLER ( lisa_r );
WRITE16_HANDLER ( lisa_w );


#endif /* LISA_H_ */

