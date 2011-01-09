/*****************************************************************************
 *
 * includes/c128.h
 *
 * Commodore C128 Home Computer
 *
 * peter.trauner@jk.uni-linz.ac.at
 *
 * Documentation: iDOC (http://www.softwolves.pp.se/idoc)
 *   Christian Janoff <mepk@c64.org>
 *
 ****************************************************************************/

#ifndef __C128_H__
#define __C128_H__

#include "includes/c64.h"
#include "machine/6526cia.h"

class c128_state : public c64_state
{
public:
	c128_state(running_machine &machine, const driver_device_config_base &config)
		: c64_state(machine, config) { }

	UINT8 *c128_basic;
	UINT8 *c128_kernal;
	UINT8 *c128_chargen;
	UINT8 *z80;
	UINT8 *editor;
	UINT8 *internal_function;
	UINT8 *external_function;
	UINT8 *vdcram;
	UINT8 mmu[0x0b];
	int mmu_cpu;
	int mmu_page0;
	int mmu_page1;
	int c64mode;
	int write_io;
	int ram_bottom;
	int ram_top;
	UINT8 *ram;
	UINT8 c64_port_data;
	UINT8 keyline[3];
	int cnt1;
	int sp1;
	int data_out;
	int va1617;
	int monitor;
	int nmilevel;
};


/*----------- defined in machine/c128.c -----------*/

WRITE8_HANDLER(c128_mmu8722_port_w);
READ8_HANDLER(c128_mmu8722_port_r);
WRITE8_HANDLER(c128_mmu8722_ff00_w);
READ8_HANDLER(c128_mmu8722_ff00_r);
WRITE8_HANDLER(c128_write_0000);
WRITE8_HANDLER(c128_write_1000);
WRITE8_HANDLER(c128_write_4000);
WRITE8_HANDLER(c128_write_8000);
WRITE8_HANDLER(c128_write_a000);
WRITE8_HANDLER(c128_write_c000);
WRITE8_HANDLER(c128_write_d000);
WRITE8_HANDLER(c128_write_e000);
WRITE8_HANDLER(c128_write_ff00);
WRITE8_HANDLER(c128_write_ff05);


extern DRIVER_INIT( c128 );
extern DRIVER_INIT( c128pal );
extern DRIVER_INIT( c128d );
extern DRIVER_INIT( c128dpal );
extern DRIVER_INIT( c128dcr );
extern DRIVER_INIT( c128dcrp );
extern DRIVER_INIT( c128d81 );
extern MACHINE_START( c128 );
extern MACHINE_RESET( c128 );
extern INTERRUPT_GEN( c128_frame_interrupt );

extern VIDEO_UPDATE( c128 );

void c128_bankswitch_64(running_machine *machine, int reset);

int c128_dma_read(running_machine *machine, int offset);
int c128_dma_read_color(running_machine *machine, int offset);
void c128_vic_interrupt(running_machine *machine, int level);

UINT8 c128_m6510_port_read(device_t *device, UINT8 direction);
void c128_m6510_port_write(device_t *device, UINT8 direction, UINT8 data);

extern const mos6526_interface c128_ntsc_cia0, c128_pal_cia0;
extern const mos6526_interface c128_ntsc_cia1, c128_pal_cia1;

WRITE_LINE_DEVICE_HANDLER( c128_iec_srq_w );
WRITE_LINE_DEVICE_HANDLER( c128_iec_data_w );

#endif /* __C128_H__ */
