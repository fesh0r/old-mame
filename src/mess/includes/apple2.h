/***************************************************************************

    includes/apple2.h

    Include file to handle emulation of the Apple II series.

***************************************************************************/

#ifndef APPLE2_H_
#define APPLE2_H_

#include "machine/applefdc.h"


/***************************************************************************
    SOFTSWITCH VALUES
***************************************************************************/

#define VAR_80STORE		0x000001
#define VAR_RAMRD		0x000002
#define VAR_RAMWRT		0x000004
#define VAR_INTCXROM	0x000008
#define VAR_ALTZP		0x000010
#define VAR_SLOTC3ROM	0x000020
#define VAR_80COL		0x000040
#define VAR_ALTCHARSET	0x000080
#define VAR_TEXT		0x000100
#define VAR_MIXED		0x000200
#define VAR_PAGE2		0x000400
#define VAR_HIRES		0x000800
#define VAR_AN0			0x001000
#define VAR_AN1			0x002000
#define VAR_AN2			0x004000
#define VAR_AN3			0x008000
#define VAR_LCRAM		0x010000
#define VAR_LCRAM2		0x020000
#define VAR_LCWRITE		0x040000
#define VAR_ROMSWITCH	0x080000

#define VAR_DHIRES		VAR_AN3


/***************************************************************************
    SPECIAL KEYS
***************************************************************************/

#define SPECIALKEY_CAPSLOCK		0x01
#define SPECIALKEY_SHIFT		0x06
#define SPECIALKEY_CONTROL		0x08
#define SPECIALKEY_BUTTON0		0x10	/* open apple */
#define SPECIALKEY_BUTTON1		0x20	/* closed apple */
#define SPECIALKEY_BUTTON2		0x40
#define SPECIALKEY_RESET		0x80


/***************************************************************************
    OTHER
***************************************************************************/

/*----------- defined in drivers/apple2.c -----------*/

MACHINE_DRIVER_EXTERN( apple2e );
INPUT_PORTS_EXTERN( apple2ep );
PALETTE_INIT( apple2 );


/*----------- defined in machine/apple2.c -----------*/

extern UINT32 a2;

extern const applefdc_interface apple2_fdc_interface;

void apple2_iwm_setdiskreg(running_machine *machine, UINT8 data);
UINT8 apple2_iwm_getdiskreg(void);

void apple2_init_common(running_machine *machine);
MACHINE_START( apple2 );
UINT8 apple2_getfloatingbusvalue(running_machine *machine);
READ8_HANDLER( apple2_c0xx_r );
WRITE8_HANDLER( apple2_c0xx_w );

INTERRUPT_GEN( apple2_interrupt );

void apple2_setvar(running_machine *machine, UINT32 val, UINT32 mask);

int apple2_pressed_specialkey(running_machine *machine, UINT8 key);


/*----------- defined in video/apple2.c -----------*/

void apple2_video_start(running_machine *machine, const UINT8 *vram, size_t vram_size, UINT32 ignored_softswitches, int hires_modulo);
VIDEO_START( apple2 );
VIDEO_START( apple2p );
VIDEO_START( apple2e );
VIDEO_UPDATE( apple2 );
void apple2_set_fgcolor(int color);
void apple2_set_bgcolor(int color);
int apple2_get_fgcolor(void);
int apple2_get_bgcolor(void);

/* -----------------------------------------------------------------------
 * New Apple II memory manager
 * ----------------------------------------------------------------------- */

#define APPLE2_MEM_AUX		0x40000000
#define APPLE2_MEM_SLOT		0x80000000
#define APPLE2_MEM_ROM		0xC0000000
#define APPLE2_MEM_FLOATING	0xFFFFFFFF
#define APPLE2_MEM_MASK		0x00FFFFFF

typedef enum
{
	A2MEM_IO		= 0,	/* this is always handlers; never banked memory */
	A2MEM_MONO		= 1,	/* this is a bank where read and write are always in unison */
	A2MEM_DUAL		= 2		/* this is a bank where read and write can go different places */
} bank_disposition_t;

typedef struct _apple2_meminfo apple2_meminfo;
struct _apple2_meminfo
{
	UINT32 read_mem;
	read8_space_func read_handler;
	UINT32 write_mem;
	write8_space_func write_handler;
};

typedef struct _apple2_memmap_entry apple2_memmap_entry;
struct _apple2_memmap_entry
{
	offs_t begin;
	offs_t end;
	void (*get_meminfo)(running_machine *machine, offs_t begin, offs_t end, apple2_meminfo *meminfo);
	bank_disposition_t bank_disposition;
};

typedef struct _apple2_memmap_config apple2_memmap_config;
struct _apple2_memmap_config
{
	int first_bank;
	UINT8 *auxmem;
	UINT32 auxmem_length;
	const apple2_memmap_entry *memmap;
};


/*----------- defined in machine/apple2.c -----------*/

void apple2_setup_memory(running_machine *machine, const apple2_memmap_config *config);
void apple2_update_memory(running_machine *machine);


#endif /* APPLE2_H_ */
